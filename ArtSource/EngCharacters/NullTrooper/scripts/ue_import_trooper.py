# Null Trooper step F: import export/SK_NullTrooper.fbx + T_NullTrooper_* into
# /Game/TriadDrift/Gameplay/Characters/NullTrooper on the EXISTING mannequin skeleton, build M_NullTrooper /
# MI_NullTrooper, assign PA_Mannequin, create DA_Crew_Trooper_Visuals. Idempotent. Nothing outside that folder is saved.
# Run: powershell -File Tools\td-python.ps1 -Script ArtSource\EngCharacters\NullTrooper\scripts\ue_import_trooper.py
import os
import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
REPO = os.path.abspath(os.path.join(ROOT, "..", "..", ".."))
ASSET = os.environ.get("NT_ASSET", "NullTrooper")          # NullTrooper | NullGrunt | NullHeavy
EXPORT = os.path.join(ROOT, "export") if ASSET == "NullTrooper" else os.path.join(ROOT, "export", ASSET)
DEST = "/Game/TriadDrift/Gameplay/Characters/" + ASSET
MASTER_DIR = "/Game/TriadDrift/Gameplay/Characters/NullTrooper"      # all three share the Trooper's master material
DA_NAME = {"NullTrooper": "DA_Crew_Trooper_Visuals", "NullGrunt": "DA_Crew_Grunt_Visuals", "NullHeavy": "DA_Crew_Heavy_Visuals"}[ASSET]
SKELETON = "/Game/Characters/Mannequins/Meshes/SK_Mannequin"
PHYSICS = "/Game/Characters/Mannequins/Rigs/PA_Mannequin"
EMISSIVE_STRENGTH = 12.0   # Blender bake strength 3 x 4: the prop kits needed 6 to 12x to glow in the game's exposure

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def log(m):
    unreal.log("TDPY: " + str(m))


def import_textures():
    out = {}
    tasks = []
    for suffix in ("BC", "N", "ORM", "E"):
        path = os.path.join(EXPORT, "T_%s_%s.png" % (ASSET, suffix)).replace("\\", "/")
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", path); t.set_editor_property("destination_path", DEST)
        t.set_editor_property("automated", True); t.set_editor_property("replace_existing", True); t.set_editor_property("save", False)
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    for suffix in ("BC", "N", "ORM", "E"):
        tex = eal.load_asset("%s/T_%s_%s" % (DEST, ASSET, suffix))
        if tex is None:
            log("FAILED: texture %s not imported" % suffix)
            return None
        if suffix == "N":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property("srgb", False)
            tex.set_editor_property("flip_green_channel", False)
        elif suffix == "ORM":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
            tex.set_editor_property("srgb", False)
        else:
            tex.set_editor_property("srgb", True)
        eal.save_loaded_asset(tex)
        out[suffix] = tex
        log("texture %s: %dx%d srgb=%s compression=%s" % (tex.get_name(), tex.blueprint_get_size_x(), tex.blueprint_get_size_y(),
                                                         tex.get_editor_property("srgb"), tex.get_editor_property("compression_settings")))
    return out


def ensure_master(tex):
    path = MASTER_DIR + "/M_NullTrooper"
    if eal.does_asset_exist(path):
        # editing a material that already has instances crashes the headless editor: delete the .uasset to rebuild it
        log("master exists, kept: " + path)
        return eal.load_asset(path)
    mat = tools.create_asset("M_NullTrooper", MASTER_DIR, unreal.Material, unreal.MaterialFactoryNew())
    for flag in ("used_with_skeletal_mesh", "used_with_morph_targets"):
        try:
            mat.set_editor_property(flag, True)
        except Exception as e:
            log("note: %s (%s)" % (flag, e))
    st = unreal.MaterialSamplerType

    def sample(param, texture, sampler, y):
        n = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -700, y)
        n.set_editor_property("parameter_name", param)
        n.set_editor_property("texture", texture)      # default texture type must match the sampler type
        n.set_editor_property("sampler_type", sampler)
        return n
    base = sample("BaseColorTex", tex["BC"], st.SAMPLERTYPE_COLOR, -300)
    orm = sample("ORMTex", tex["ORM"], st.SAMPLERTYPE_MASKS, 0)
    normal = sample("NormalTex", tex["N"], st.SAMPLERTYPE_NORMAL, 300)
    emis = sample("EmissiveTex", tex["E"], st.SAMPLERTYPE_COLOR, 600)
    strength = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -700, 850)
    strength.set_editor_property("parameter_name", "EmissiveStrength")
    strength.set_editor_property("default_value", EMISSIVE_STRENGTH)
    mul = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -350, 650)
    mp = unreal.MaterialProperty
    ok = True
    ok &= mel.connect_material_property(base, "RGB", mp.MP_BASE_COLOR)
    ok &= mel.connect_material_property(orm, "R", mp.MP_AMBIENT_OCCLUSION)
    ok &= mel.connect_material_property(orm, "G", mp.MP_ROUGHNESS)
    ok &= mel.connect_material_property(orm, "B", mp.MP_METALLIC)
    ok &= mel.connect_material_property(normal, "RGB", mp.MP_NORMAL)
    ok &= mel.connect_material_expressions(emis, "RGB", mul, "A")
    ok &= mel.connect_material_expressions(strength, "", mul, "B")
    ok &= mel.connect_material_property(mul, "", mp.MP_EMISSIVE_COLOR)
    if not ok:
        log("FAILED: could not wire M_NullTrooper")
        return None
    try:
        mel.recompile_material(mat)
    except RuntimeError:
        log("note: no renderer here, shaders compile on first load")
    eal.save_asset(path, only_if_is_dirty=False)
    log("built " + path)
    return mat


def ensure_instance(master, tex):
    path = DEST + "/MI_" + ASSET
    mi = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(
        "MI_" + ASSET, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, master)
    for param, key in (("BaseColorTex", "BC"), ("ORMTex", "ORM"), ("NormalTex", "N"), ("EmissiveTex", "E")):
        mel.set_material_instance_texture_parameter_value(mi, param, tex[key])
    mel.set_material_instance_scalar_parameter_value(mi, "EmissiveStrength", EMISSIVE_STRENGTH)
    eal.save_asset(path, only_if_is_dirty=False)
    log("material instance %s parent %s" % (path, mi.get_editor_property("parent").get_path_name()))
    return mi


def import_mesh():
    skel = eal.load_asset(SKELETON)
    # drop this mesh's own sockets from an earlier import so the FBX sockets are the only ones (never the skeleton's)
    old = eal.load_asset(DEST + "/SK_" + ASSET) if eal.does_asset_exist(DEST + "/SK_" + ASSET) else None
    if old is not None:
        mine = [str(old.get_socket_by_index(i).get_editor_property("socket_name")) for i in range(old.num_sockets())
                if old.get_socket_by_index(i).get_outer() == old]
        for n_ in mine:
            old.remove_socket(n_)
        if mine: log("removed old mesh sockets: %s" % mine)
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    ui.set_editor_property("skeleton", skel)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("import_animations", False)
    ui.set_editor_property("create_physics_asset", False)
    d = ui.get_editor_property("skeletal_mesh_import_data")
    d.set_editor_property("import_morph_targets", False)
    d.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
    d.set_editor_property("use_t0_as_ref_pose", False)
    d.set_editor_property("preserve_smoothing_groups", True)
    d.set_editor_property("convert_scene", True)
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.join(EXPORT, "SK_%s.fbx" % ASSET).replace("\\", "/"))
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("destination_name", "SK_" + ASSET)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", False)
    t.set_editor_property("options", ui)
    tools.import_asset_tasks([t])
    log("imported: %s" % list(t.get_editor_property("imported_object_paths")))
    return eal.load_asset(DEST + "/SK_" + ASSET)


def main():
    if not eal.does_directory_exist(DEST):
        eal.make_directory(DEST)
    tex = import_textures()
    if not tex: return
    master = ensure_master(tex)
    if master is None: return
    mi = ensure_instance(master, tex)
    mesh = import_mesh()
    if mesh is None:
        log("FAILED: SK_%s not imported" % ASSET)
        return
    sk = mesh.get_editor_property("skeleton")
    log("mesh skeleton: %s" % (sk.get_path_name() if sk else None))
    # material slot
    mats = list(mesh.get_editor_property("materials"))
    log("material slots before: %s" % [str(m.get_editor_property("material_slot_name")) for m in mats])
    for m in mats:
        m.set_editor_property("material_interface", mi)
    mesh.set_editor_property("materials", mats)
    # physics asset (shared mannequin asset: same bone names)
    try:
        pa = eal.load_asset(PHYSICS)
        mesh.set_editor_property("physics_asset", pa)
        log("physics asset: %s" % mesh.get_editor_property("physics_asset").get_path_name())
    except Exception as e:
        log("note: physics asset not assigned (%s)" % e)
    eal.save_asset(DEST + "/SK_" + ASSET, only_if_is_dirty=False)
    mesh = eal.load_asset(DEST + "/SK_" + ASSET)
    b = mesh.get_bounds()
    log("bounds origin (%.1f, %.1f, %.1f) extent (%.1f, %.1f, %.1f) cm" % (b.origin.x, b.origin.y, b.origin.z, b.box_extent.x, b.box_extent.y, b.box_extent.z))
    log("material slots: %s" % [(str(m.get_editor_property("material_slot_name")), m.get_editor_property("material_interface").get_name()
                                 if m.get_editor_property("material_interface") else None) for m in mesh.get_editor_property("materials")])
    for label, fn_ in (("verts LOD0", lambda: unreal.EditorSkeletalMeshLibrary.get_num_verts(mesh, 0)),
                       ("sections LOD0", lambda: unreal.EditorSkeletalMeshLibrary.get_num_sections(mesh, 0)),
                       ("lod count", lambda: unreal.EditorSkeletalMeshLibrary.get_lod_count(mesh))):
        try:
            log("%s: %s" % (label, fn_()))
        except Exception as e:
            log("note: %s unavailable (%s)" % (label, e))
    # sockets come from SOCKET_ empties in the FBX (the Python API cannot set a socket's bone): report the mesh's own
    for i in range(mesh.num_sockets()):
        x = mesh.get_socket_by_index(i)
        if x.get_outer() == mesh:
            loc = x.get_editor_property("relative_location"); rot = x.get_editor_property("relative_rotation")
            log("mesh socket %s on %s at (%.1f, %.1f, %.1f) cm, pitch %.1f yaw %.1f roll %.1f" % (
                x.get_editor_property("socket_name"), x.get_editor_property("bone_name"), loc.x, loc.y, loc.z, rot.pitch, rot.yaw, rot.roll))
    # data asset for gameplay
    da_path = DEST + "/" + DA_NAME
    try:
        da = eal.load_asset(da_path) if eal.does_asset_exist(da_path) else tools.create_asset(
            DA_NAME, DEST, unreal.TDEnemyVisuals, unreal.DataAssetFactory())
        cur = da.get_editor_property("skeletal_mesh")
        if cur is None or str(cur) in ("", "None") or not eal.does_asset_exist(da_path):
            da.set_editor_property("skeletal_mesh", mesh)
            eal.save_asset(da_path, only_if_is_dirty=False)
        else:
            log("data asset already points at a mesh: left untouched")
        log("data asset %s class %s SkeletalMesh=%s" % (da_path, da.get_class().get_name(), da.get_editor_property("skeletal_mesh")))
    except Exception as e:
        log("FAILED: data asset (%s)" % e)
        return
    # re-export for the numeric check in Blender
    out = os.path.join(REPO, "Saved", "NullTrooper", "SK_%s_back.fbx" % ASSET).replace("\\", "/")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    t = unreal.AssetExportTask()
    t.set_editor_property("object", mesh); t.set_editor_property("filename", out)
    t.set_editor_property("automated", True); t.set_editor_property("prompt", False); t.set_editor_property("replace_identical", True)
    t.set_editor_property("exporter", unreal.SkeletalMeshExporterFBX())
    opt = unreal.FbxExportOption(); opt.set_editor_property("level_of_detail", False); opt.set_editor_property("collision", False)
    t.set_editor_property("options", opt)
    log("re-export ok=%s" % unreal.Exporter.run_asset_export_task(t))
    for suffix in ("BC", "N", "ORM", "E"):
        eal.save_asset("%s/T_%s_%s" % (DEST, ASSET, suffix), only_if_is_dirty=True)
    dirty = [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]
    log("still dirty (not saved): %s" % dirty)
    log("assets: %s" % sorted(str(a) for a in eal.list_assets(DEST)))
    if sk and sk.get_path_name().startswith(SKELETON):
        log("OK")
    else:
        log("FAILED: wrong skeleton")


main()

# Null Armada Drone (Rodin mesh_r1): import SM_NullDrone.fbx + T_NullDrone_{BC,N,ORM,E} from
# ArtSource/EngAI/Characters/Drone/mesh_r1 (read-only input) into /Game/TriadDrift/Gameplay/Characters/NullDrone,
# build MI_NullDrone on the existing engineering master M_TD_PropTextured, add collision + sockets, and create
# DA_Crew_Drone_Visuals (UTDEnemyVisuals.Mesh), the asset ATDCrewCharacter::FindCrewVisuals("Drone") looks up.
# Idempotent. Saves nothing outside the NullDrone folder.
# Run: powershell -File Tools\td-python.ps1 -Script ArtSource\EngCharacters\NullDrone\scripts\ue_import_drone.py
import os
import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
SRC = os.path.join(REPO, "ArtSource", "EngAI", "Characters", "Drone", "mesh_r1")
ASSET = "NullDrone"
DEST = "/Game/TriadDrift/Gameplay/Characters/" + ASSET
MASTER = "/Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured"   # BaseColorTex, ORMTex, NormalTex, EmissiveTex, EmissiveStrength
DA_NAME = "DA_Crew_Drone_Visuals"
EMISSIVE_STRENGTH = 6.0
# Measured from the contact sheet (front view): eye centre ~2 cm below the pivot on the +X face; thrusters under the ring.
SOCKETS = {"Muzzle": (41.0, 0.0, -2.0), "Engine_0": (0.0, -33.0, -36.0), "Engine_1": (0.0, 33.0, -36.0)}

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def log(m):
    unreal.log("TDPY: " + str(m))


def import_textures():
    out = {}
    tasks = []
    for suffix in ("BC", "N", "ORM", "E"):
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", os.path.join(SRC, "T_%s_%s.png" % (ASSET, suffix)).replace("\\", "/"))
        t.set_editor_property("destination_path", DEST)
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
            tex.set_editor_property("flip_green_channel", False)   # delivery map is already DirectX
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


def ensure_instance(tex):
    master = eal.load_asset(MASTER)
    path = DEST + "/MI_" + ASSET
    mi = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(
        "MI_" + ASSET, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, master)
    for param, key in (("BaseColorTex", "BC"), ("ORMTex", "ORM"), ("NormalTex", "N"), ("EmissiveTex", "E")):
        mel.set_material_instance_texture_parameter_value(mi, param, tex[key])
    mel.set_material_instance_scalar_parameter_value(mi, "EmissiveStrength", EMISSIVE_STRENGTH)
    eal.save_asset(path, only_if_is_dirty=False)
    log("material instance %s parent %s EmissiveStrength %.1f" % (path, mi.get_editor_property("parent").get_path_name(), EMISSIVE_STRENGTH))
    return mi


def import_mesh():
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", False)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("import_animations", False)
    d = ui.get_editor_property("static_mesh_import_data")
    d.set_editor_property("combine_meshes", True)
    d.set_editor_property("auto_generate_collision", False)
    d.set_editor_property("generate_lightmap_u_vs", False)
    d.set_editor_property("build_nanite", False)
    d.set_editor_property("remove_degenerates", True)
    d.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
    d.set_editor_property("convert_scene", True)
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.join(SRC, "SM_%s.fbx" % ASSET).replace("\\", "/"))
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("destination_name", "SM_" + ASSET)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", False)
    t.set_editor_property("options", ui)
    tools.import_asset_tasks([t])
    log("imported: %s" % list(t.get_editor_property("imported_object_paths")))
    return eal.load_asset(DEST + "/SM_" + ASSET)


def main():
    if not eal.does_directory_exist(DEST):
        eal.make_directory(DEST)
    tex = import_textures()
    if not tex:
        return
    mi = ensure_instance(tex)
    mesh = import_mesh()
    if mesh is None:
        log("FAILED: SM_%s not imported" % ASSET)
        return
    sms = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    # Nanite off (small hovering enemy, 6k tris)
    ns = mesh.get_editor_property("nanite_settings")
    ns.set_editor_property("enabled", False)
    mesh.set_editor_property("nanite_settings", ns)
    # material
    for i in range(len(mesh.static_materials)):
        mesh.set_material(i, mi)
    # simple collision: one convex hull (the corpse tumbles as a physics body), sphere if that is unavailable
    sms.remove_collisions(mesh)
    try:
        sms.set_convex_decomposition_collisions(mesh, 1, 24, 100000)
    except Exception as e:
        log("note: convex collision failed (%s)" % e)
    if sms.get_simple_collision_count(mesh) + sms.get_convex_collision_count(mesh) <= 0:
        sms.add_simple_collisions(mesh, unreal.ScriptCollisionShapeType.SPHERE)
        log("collision: sphere fallback")
    # sockets
    for name in SOCKETS:
        old = mesh.find_socket(name)
        if old:
            mesh.remove_socket(old)
    for name, loc in SOCKETS.items():
        s = unreal.StaticMeshSocket(mesh)
        s.set_editor_property("socket_name", name)
        s.set_editor_property("relative_location", unreal.Vector(*loc))
        if name.startswith("Engine"):
            s.set_editor_property("relative_rotation", unreal.Rotator(pitch=-90.0, yaw=0.0, roll=0.0))   # +X of the socket points down
        mesh.add_socket(s)
    eal.save_asset(DEST + "/SM_" + ASSET, only_if_is_dirty=False)
    mesh = eal.load_asset(DEST + "/SM_" + ASSET)
    box = mesh.get_bounding_box()
    size = (box.max.x - box.min.x, box.max.y - box.min.y, box.max.z - box.min.z)
    centre = ((box.max.x + box.min.x) / 2, (box.max.y + box.min.y) / 2, (box.max.z + box.min.z) / 2)
    log("size %.1f x %.1f x %.1f cm (X x Y x Z), bbox centre (%.1f, %.1f, %.1f)" % (size + centre))
    log("triangles LOD0 %d, verts %d, lods %d, nanite %s" % (mesh.get_num_triangles(0), sms.get_number_verts(mesh, 0), mesh.get_num_lods(),
                                                           mesh.get_editor_property("nanite_settings").get_editor_property("enabled")))
    log("simple collision shapes %d, convex %d" % (sms.get_simple_collision_count(mesh), sms.get_convex_collision_count(mesh)))
    log("sockets %s" % [(str(n), mesh.find_socket(n) is not None) for n in SOCKETS])
    log("material slots %s" % [(str(m.get_editor_property("material_slot_name")), m.get_editor_property("material_interface").get_name()
                                if m.get_editor_property("material_interface") else None) for m in mesh.static_materials])
    # data asset for gameplay: UTDEnemyVisuals.Mesh only
    da_path = DEST + "/" + DA_NAME
    da = eal.load_asset(da_path) if eal.does_asset_exist(da_path) else tools.create_asset(
        DA_NAME, DEST, unreal.TDEnemyVisuals, unreal.DataAssetFactory())
    da.set_editor_property("mesh", mesh)
    eal.save_asset(da_path, only_if_is_dirty=False)
    log("data asset %s class %s Mesh=%s SkeletalMesh=%s" % (da_path, da.get_class().get_name(), da.get_editor_property("mesh"), da.get_editor_property("skeletal_mesh")))
    dirty = [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]
    log("still dirty (not saved): %s" % dirty)
    log("assets: %s" % sorted(str(a) for a in eal.list_assets(DEST)))
    ok = abs(size[1] - 90.0) < 2.0 and mesh.get_num_triangles(0) > 0 and sms.get_simple_collision_count(mesh) + sms.get_convex_collision_count(mesh) > 0
    log("OK" if ok else "FAILED: size or collision check")


main()

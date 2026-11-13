# Warden step 2: import export/SK_Warden.fbx (w10_build.py) into /Game/TriadDrift/Gameplay/Characters/Warden and create
# DA_Crew_Warden_Visuals, the asset ATDCrewCharacter::FindCrewVisuals("Warden") looks up (engineering fallback: an asset
# of the same name in /Game/TriadDrift/Presentation wins).
#   SKEL_Warden: a COPY of the template mannequin skeleton (/Game/Characters/Mannequins/Meshes/SK_Mannequin, read only):
#     same bone names as SKEL_NullCrew, and it carries Epic's per-bone translation retargeting (bones keep THIS mesh's
#     lengths, the pelvis height is scaled), which cannot be set from Python. SKEL_NullCrew is added as a compatible
#     skeleton, so the crew clips in Crew/Anims play on the 260 cm Warden as they are. SKEL_NullCrew is not touched.
#   Textures (2K, Grok Bot's Rodin maps, read from ArtSource/EngAI), MI_Warden / MI_Warden_Shield / MI_Warden_GunArm on
#   the crew master M_NullTrooper, PA_Warden, sockets from the FBX.
# Saves only inside Characters/Warden. Idempotent.
# Run: powershell -File Tools\td-python.ps1 -Script ArtSource\EngCharacters\Warden\scripts\ue_w20_import.py
import os
import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
AI = os.path.join(REPO, "ArtSource", "EngAI", "Characters", "Warden")
FBX = os.path.join(REPO, "ArtSource", "EngCharacters", "Warden", "export", "SK_Warden.fbx")
DEST = "/Game/TriadDrift/Gameplay/Characters/Warden"
MASTER = "/Game/TriadDrift/Gameplay/Characters/NullTrooper/M_NullTrooper"
TEMPLATE_SKEL = "/Game/Characters/Mannequins/Meshes/SK_Mannequin"
CREW_SKEL = "/Game/TriadDrift/Gameplay/Characters/Crew/SKEL_NullCrew"
EMISSIVE_STRENGTH = {"M_Warden": 6.0, "M_Warden_GunArm": 5.0, "M_Warden_Shield": 2.5}   # the shield seams are wide: at 6 they bloom to pink-white in game
SETS = [("Warden", os.path.join(AI, "mesh_r1"), "M_Warden"), ("Warden_ShieldProp", os.path.join(AI, "props_r1"), "M_Warden_Shield"),
        ("Warden_GunArm", os.path.join(AI, "props_r1"), "M_Warden_GunArm")]
MI_NAME = {"M_Warden": "MI_Warden", "M_Warden_Shield": "MI_Warden_Shield", "M_Warden_GunArm": "MI_Warden_GunArm"}
EAL = unreal.EditorAssetLibrary
APE = unreal.AnimPoseExtensions
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
problems = []


def log(m):
    for line in str(m).splitlines():
        unreal.log("TDPY: " + line)


def textures(asset, folder):
    out = {}
    tasks = []
    for suffix in ("BC", "N", "ORM", "E"):
        if EAL.does_asset_exist("%s/T_%s_%s" % (DEST, asset, suffix)):
            continue
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", os.path.join(folder, "T_%s_%s.png" % (asset, suffix)).replace("\\", "/"))
        t.set_editor_property("destination_path", DEST)
        t.set_editor_property("automated", True); t.set_editor_property("replace_existing", True); t.set_editor_property("save", False)
        tasks.append(t)
    if tasks:
        tools.import_asset_tasks(tasks)
    for suffix in ("BC", "N", "ORM", "E"):
        tex = EAL.load_asset("%s/T_%s_%s" % (DEST, asset, suffix))
        if tex is None:
            problems.append("texture %s %s" % (asset, suffix)); continue
        if suffix == "N":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property("srgb", False)
            tex.set_editor_property("flip_green_channel", False)   # delivery maps are already DirectX
        elif suffix == "ORM":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
            tex.set_editor_property("srgb", False)
        else:
            tex.set_editor_property("srgb", True)
        EAL.save_loaded_asset(tex)
        out[suffix] = tex
        log("texture %s %dx%d" % (tex.get_name(), tex.blueprint_get_size_x(), tex.blueprint_get_size_y()))
        if max(tex.blueprint_get_size_x(), tex.blueprint_get_size_y()) > 2048:
            problems.append(tex.get_name() + " larger than 2K")
    return out


def instance(name, tex, strength):
    path = DEST + "/" + name
    mi = EAL.load_asset(path) if EAL.does_asset_exist(path) else tools.create_asset(
        name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, EAL.load_asset(MASTER))
    for param, key in (("BaseColorTex", "BC"), ("ORMTex", "ORM"), ("NormalTex", "N"), ("EmissiveTex", "E")):
        mel.set_material_instance_texture_parameter_value(mi, param, tex[key])
    mel.set_material_instance_scalar_parameter_value(mi, "EmissiveStrength", strength)
    EAL.save_asset(path, only_if_is_dirty=False)
    return mi


def skeleton():
    path = DEST + "/SKEL_Warden"
    if EAL.does_asset_exist(path):
        return EAL.load_asset(path)
    sk = EAL.duplicate_asset(TEMPLATE_SKEL, path)
    if sk is None:
        problems.append("could not copy the template skeleton"); return None
    sk.add_compatible_skeleton(EAL.load_asset(CREW_SKEL))
    EAL.save_asset(path, only_if_is_dirty=False)
    return sk


def import_mesh(skel):
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True); ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    ui.set_editor_property("skeleton", skel)
    ui.set_editor_property("import_materials", False); ui.set_editor_property("import_textures", False)
    ui.set_editor_property("import_animations", False); ui.set_editor_property("create_physics_asset", False)
    d = ui.get_editor_property("skeletal_mesh_import_data")
    d.set_editor_property("import_morph_targets", False)
    d.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
    d.set_editor_property("use_t0_as_ref_pose", False); d.set_editor_property("preserve_smoothing_groups", True)
    d.set_editor_property("convert_scene", True)
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", FBX.replace("\\", "/")); t.set_editor_property("destination_path", DEST)
    t.set_editor_property("destination_name", "SK_Warden"); t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True); t.set_editor_property("save", False); t.set_editor_property("options", ui)
    tools.import_asset_tasks([t])
    log("import produced %s" % sorted(str(p) for p in t.get_editor_property("imported_object_paths")))
    m = EAL.load_asset(DEST + "/SK_Warden")
    return m if isinstance(m, unreal.SkeletalMesh) else None


def main():
    if not EAL.does_directory_exist(DEST):
        EAL.make_directory(DEST)
    mis = {}
    for asset, folder, slot in SETS:
        tex = textures(asset, folder)
        if len(tex) == 4:
            mis[slot] = instance(MI_NAME[slot], tex, EMISSIVE_STRENGTH[slot])
    skel = skeleton()
    if skel is None or problems:
        log("FAILED: %s" % problems); return
    try:
        tree = skel.get_editor_property("bone_tree")
        names = [str(n) for n in APE.get_bone_names(APE.get_reference_pose(skel))]
        modes = {}
        for i, node in enumerate(tree):
            modes.setdefault(str(node.get_editor_property("translation_retargeting_mode")), []).append(names[i] if i < len(names) else i)
        for k, v in modes.items():
            log("retarget mode %s: %d bones e.g. %s" % (k, len(v), v[:8]))
    except Exception as e:
        log("note: bone tree not readable (%s)" % e)
    log("compatible skeletons %s" % [str(s) for s in skel.get_editor_property("compatible_skeletons")])
    old = EAL.load_asset(DEST + "/SK_Warden") if EAL.does_asset_exist(DEST + "/SK_Warden") else None
    if old is not None:
        for n_ in [str(old.get_socket_by_index(i).get_editor_property("socket_name")) for i in range(old.num_sockets())
                   if old.get_socket_by_index(i).get_outer() == old]:
            old.remove_socket(n_)
    mesh = import_mesh(skel)
    if mesh is None:
        log("FAILED: SK_Warden not imported"); return
    if mesh.get_editor_property("skeleton") != skel:
        problems.append("mesh skeleton is %s" % mesh.get_editor_property("skeleton").get_path_name())
    mats = list(mesh.get_editor_property("materials"))
    for m in mats:
        slot = str(m.get_editor_property("material_slot_name"))
        mi = mis.get(slot)
        if mi is None:
            problems.append("no material for slot " + slot); continue
        m.set_editor_property("material_interface", mi)
    mesh.set_editor_property("materials", mats)
    pa_path = DEST + "/PA_Warden"
    if not EAL.does_asset_exist(pa_path):
        pa = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem).create_physics_asset(mesh)
        if pa is not None:
            src = pa.get_path_name().split(".")[0]
            if src != pa_path:
                EAL.rename_asset(src, pa_path)
    pa = EAL.load_asset(pa_path)
    if pa is not None:
        mesh.set_editor_property("physics_asset", pa)
        EAL.save_asset(pa_path, only_if_is_dirty=False)
    else:
        log("note: no physics asset")
    EAL.save_asset(DEST + "/SK_Warden", only_if_is_dirty=False)
    EAL.save_asset(DEST + "/SKEL_Warden", only_if_is_dirty=False)
    mesh = EAL.load_asset(DEST + "/SK_Warden")
    b = mesh.get_bounds()
    log("bounds z %.1f..%.1f  x +-%.1f y +-%.1f (cm)" % (b.origin.z - b.box_extent.z, b.origin.z + b.box_extent.z, b.box_extent.x, b.box_extent.y))
    ref = APE.get_reference_pose(mesh.get_editor_property("skeleton"))
    socks = []
    for i in range(mesh.num_sockets()):
        s = mesh.get_socket_by_index(i)
        if s.get_outer() == mesh:
            socks.append("%s@%s" % (s.get_editor_property("socket_name"), s.get_editor_property("bone_name")))
    log("mesh sockets %s" % socks)
    for want in ("Muzzle", "GunArm", "Shield", "HeadGlow"):
        if not any(x.startswith(want + "@") for x in socks):
            problems.append("socket %s missing" % want)
    log("material slots %s" % [(str(m.get_editor_property("material_slot_name")), m.get_editor_property("material_interface").get_name()
                                 if m.get_editor_property("material_interface") else None) for m in mesh.get_editor_property("materials")])
    try:
        log("verts LOD0 %s sections %s" % (unreal.EditorSkeletalMeshLibrary.get_num_verts(mesh, 0), unreal.EditorSkeletalMeshLibrary.get_num_sections(mesh, 0)))
    except Exception as e:
        log("note %s" % e)
    da_path = DEST + "/DA_Crew_Warden_Visuals"
    da = EAL.load_asset(da_path) if EAL.does_asset_exist(da_path) else tools.create_asset(
        "DA_Crew_Warden_Visuals", DEST, unreal.TDEnemyVisuals, unreal.DataAssetFactory())
    da.set_editor_property("skeletal_mesh", mesh)
    EAL.save_asset(da_path, only_if_is_dirty=False)
    log("data asset %s SkeletalMesh=%s yaw=%s AnimBP=%s" % (da_path, da.get_editor_property("skeletal_mesh").get_name(),
                                                           da.get_editor_property("skeletal_mesh_yaw"), da.get_editor_property("anim_bp_class")))
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    for data in reg.get_assets_by_path(DEST, recursive=False):
        if str(data.asset_class_path.asset_name) == "ObjectRedirector":
            log("removing redirector %s: %s" % (data.package_name, EAL.delete_asset(str(data.package_name))))
    if abs((b.origin.z + b.box_extent.z) - 260.0) > 6.0:
        problems.append("height %.1f, wanted 260" % (b.origin.z + b.box_extent.z))
    log("assets: %s" % sorted(str(a).split("/")[-1].split(".")[0] for a in EAL.list_assets(DEST)))
    log("still dirty: %s" % [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()])
    log("FAILED: %s" % problems if problems else "OK")


main()

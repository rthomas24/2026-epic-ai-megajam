# PilotBlade probe: import the SMPL-H reference FBX (Grok Bot, read only) as a skeletal mesh into the uncooked Dev folder
# and log its reference skeleton; log IK_Pilot's chains and DA_Pilot_Visuals. Saves only under /Game/TriadDrift/Dev/EngAnim.
import os
import unreal
EAL = unreal.EditorAssetLibrary
APE = unreal.AnimPoseExtensions
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
SRC = os.path.join(REPO, "ArtSource", "EngAI", "Animations", "PilotBlade", "r1").replace("\\", "/")
DEV = "/Game/TriadDrift/Dev/EngAnim/PilotBlade"
tools = unreal.AssetToolsHelpers.get_asset_tools()

def log(m):
    for line in str(m).splitlines():
        unreal.log("TDPY: " + line)

def import_mesh():
    if EAL.does_asset_exist(DEV + "/SK_SMPLH"):
        return EAL.load_asset(DEV + "/SK_SMPLH")
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True); ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    ui.set_editor_property("skeleton", None)
    ui.set_editor_property("import_materials", False); ui.set_editor_property("import_textures", False)
    ui.set_editor_property("import_animations", False); ui.set_editor_property("create_physics_asset", False)
    d = ui.get_editor_property("skeletal_mesh_import_data")
    d.set_editor_property("use_t0_as_ref_pose", False); d.set_editor_property("convert_scene", True)
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", SRC + "/A_PilotBlade_RefPose.fbx"); t.set_editor_property("destination_path", DEV)
    t.set_editor_property("destination_name", "SK_SMPLH"); t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True); t.set_editor_property("save", True); t.set_editor_property("options", ui)
    tools.import_asset_tasks([t])
    log("import produced %s" % sorted(str(p) for p in t.get_editor_property("imported_object_paths")))
    return EAL.load_asset(DEV + "/SK_SMPLH")

def ref(tag, skel, names=None):
    p = APE.get_reference_pose(skel)
    for n in APE.get_bone_names(p):
        if names and str(n) not in names: continue
        l = APE.get_bone_pose(p, n, unreal.AnimPoseSpaces.LOCAL); w = APE.get_bone_pose(p, n, unreal.AnimPoseSpaces.WORLD)
        log("%s %-16s Lscale %.3f W (%.1f %.1f %.1f)" % (tag, n, l.scale3d.x, w.translation.x, w.translation.y, w.translation.z))

m = import_mesh()
log("mesh %s skeleton %s" % (m, m.get_editor_property("skeleton").get_path_name() if m else None))
if m: ref("SMPLH", m.get_editor_property("skeleton"))
pil = EAL.load_asset("/Game/TriadDrift/Gameplay/Characters/Pilot/SK_Pilot")
ref("PILOT", pil.get_editor_property("skeleton"))
for i in range(pil.num_sockets()):
    s = pil.get_socket_by_index(i); log("pilot socket %s @ %s" % (s.get_editor_property("socket_name"), s.get_editor_property("bone_name")))
rig = EAL.load_asset("/Game/TriadDrift/Gameplay/Characters/Pilot/Anims/IK_Pilot")
c = unreal.IKRigController.get_controller(rig)
log("pilot retarget root %s" % c.get_retarget_root())
for ch in c.get_retarget_chains():
    log("chain %s" % ch)
for g in c.get_all_goals():
    log("goal %s bone %s" % (g.get_editor_property("goal_name"), g.get_editor_property("bone_name")))
da = EAL.load_asset("/Game/TriadDrift/Gameplay/Characters/Pilot/DA_Pilot_Visuals")
log("DA class %s" % da.get_class().get_name())
for prop in ("skeletal_mesh", "anim_bp_class", "idle", "walk", "run", "idle_anim", "walk_anim", "run_anim", "mesh_yaw", "skeletal_mesh_yaw"):
    try: log("DA.%s = %s" % (prop, da.get_editor_property(prop)))
    except Exception as e: pass
rtg = EAL.load_asset("/Game/TriadDrift/Gameplay/Characters/Pilot/Anims/RTG_Manny_to_Pilot")
rc = unreal.IKRetargeterController.get_controller(rtg)
for i in range(rc.get_num_retarget_ops()):
    log("op %d %s" % (i, rc.get_op_name(i)))
log([x for x in dir(rc) if not x.startswith("_")])
log("OK")

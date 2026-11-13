# Probe: local and component-space reference transforms of root / pelvis / spine / head on SKEL_NullCrew, the SK_Mannequin
# skeleton and the Pilot skeleton; plus one retargeted sequence's local root/pelvis/spine track. Saves nothing.
import unreal

APE = unreal.AnimPoseExtensions
EAL = unreal.EditorAssetLibrary
BONES = ("root", "pelvis", "spine_01", "head", "foot_l")


def log(m):
    unreal.log("TDPY: " + str(m))


def show(tag, pose):
    for b in BONES:
        try:
            l = APE.get_bone_pose(pose, b, unreal.AnimPoseSpaces.LOCAL)
            w = APE.get_bone_pose(pose, b, unreal.AnimPoseSpaces.WORLD)
            r = l.rotation.rotator()
            log("%s %s LOCAL t=(%.3f,%.3f,%.3f) r=(%.1f,%.1f,%.1f) s=(%.3f,%.3f,%.3f) | WORLD t=(%.2f,%.2f,%.2f) s=(%.3f,%.3f,%.3f)" % (
                tag, b, l.translation.x, l.translation.y, l.translation.z, r.roll, r.pitch, r.yaw, l.scale3d.x, l.scale3d.y, l.scale3d.z,
                w.translation.x, w.translation.y, w.translation.z, w.scale3d.x, w.scale3d.y, w.scale3d.z))
        except Exception as e:
            log("%s %s err %s" % (tag, b, e))


for tag, p in (("NC", "/Game/TriadDrift/Gameplay/Characters/Crew/SKEL_NullCrew"),
               ("MANNY", "/Game/Characters/Mannequins/Meshes/SK_Mannequin"),
               ("PILOT", "/Game/TriadDrift/Gameplay/Characters/Pilot/SKEL_Pilot")):
    s = EAL.load_asset(p)
    if s:
        show(tag + "_REF", APE.get_reference_pose(s))
seq = EAL.load_asset("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/MM_Idle")
o = unreal.AnimPoseEvaluationOptions()
show("NC_IDLE_t0", APE.get_anim_pose_at_time(seq, 0.0, o))
o.set_editor_property("optional_skeletal_mesh", EAL.load_asset("/Game/TriadDrift/Gameplay/Characters/NullTrooper/SK_NullTrooper"))
show("NC_IDLE_t0_mesh", APE.get_anim_pose_at_time(seq, 0.0, o))
log("OK")

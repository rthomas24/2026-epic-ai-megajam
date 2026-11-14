# Triad Drift - read A_Pilot_Grip_R back: per-bone local rotation at t=0 against the reference pose (degrees), and
# the BladeSocket on SK_Pilot. Run headless with Tools\td-python.ps1. Animation builder (eng/anim), Nov 13.
import unreal


def log(m):
    unreal.log("TDPY: " + str(m))


seq = unreal.load_asset("/Game/TriadDrift/Gameplay/Characters/Pilot/Anims/A_Pilot_Grip_R")
skel = unreal.load_asset("/Game/TriadDrift/Gameplay/Characters/Pilot/SKEL_Pilot")
log("seq %s skeleton %s length %s" % (seq, seq.get_editor_property("skeleton").get_name(), seq.get_play_length()))
ref = unreal.AnimPoseExtensions.get_reference_pose(skel)
opts = unreal.AnimPoseEvaluationOptions()
pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, 0.0, opts)
for b in ["index_01_r", "index_02_r", "middle_01_r", "pinky_02_r", "thumb_01_r", "thumb_03_r", "hand_r"]:
    a = unreal.AnimPoseExtensions.get_bone_pose(pose, b, unreal.AnimPoseSpaces.LOCAL)
    r = unreal.AnimPoseExtensions.get_ref_bone_pose(ref, b, unreal.AnimPoseSpaces.LOCAL)
    q1, q2 = a.rotation, r.rotation
    d = abs(q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w)
    import math
    log("%-12s %.1f deg from ref" % (b, math.degrees(2 * math.acos(min(1.0, d)))))
try:
    names = unreal.AnimationLibrary.get_animation_track_names(seq)
    log("tracks: %s" % [str(n) for n in names])
except Exception as e:
    log("track names: %s" % e)
mesh = unreal.load_asset("/Game/TriadDrift/Gameplay/Characters/Pilot/SK_Pilot")
s = mesh.find_socket("BladeSocket")
log("socket %s" % (s.relative_location if s else None))
log("OK")

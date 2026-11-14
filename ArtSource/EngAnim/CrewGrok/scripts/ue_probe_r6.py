# Triad Drift - checks: pilot interact (left hand reach, right hand and body facing constant) and the Warden death's
# chest facing over time (no wobble).
import math
import unreal
X = unreal.AnimPoseExtensions
W = unreal.AnimPoseSpaces.WORLD
O = unreal.AnimPoseEvaluationOptions()
def log(m): unreal.log("TDPY: " + str(m))
def P(p, b):
    v = X.get_bone_pose(p, b, W).translation
    return (v.x, v.y, v.z)
def facing(p):
    l, r = P(p, "upperarm_l"), P(p, "upperarm_r")
    return math.degrees(math.atan2(-(r[1] - l[1]), -(r[0] - l[0])))
seq = unreal.load_asset("/Game/TriadDrift/Gameplay/Characters/Pilot/Anims/A_Pilot_Interact")
rows = []
for i in range(int(round(seq.get_play_length() * 10)) + 1):
    t = min(seq.get_play_length(), i / 10.0)
    p = X.get_anim_pose_at_time(seq, t, O)
    pe, hl, hr = P(p, "pelvis"), P(p, "hand_l"), P(p, "hand_r")
    rows.append("%.1f: L %.0f fwd %.0f high | R %.0f,%.0f,%.0f | chest %.0f" % (t, hl[1] - pe[1], hl[2], hr[0] - pe[0], hr[1] - pe[1], hr[2], facing(p)))
log("A_Pilot_Interact %.2f s: left hand forward of the pelvis / height; right hand relative to the pelvis; chest facing deg" % seq.get_play_length())
for r in rows:
    log("   " + r)
seq = unreal.load_asset("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/A_Crew_Warden_Death_Front")
log("A_Crew_Warden_Death_Front chest facing deg every 0.1 s to 1.2 s: " + " ".join("%.0f" % facing(X.get_anim_pose_at_time(seq, i / 10.0, O)) for i in range(13)))
log("OK")

# Triad Drift - numeric checks on the Warden clips after the shield moved to the left forearm: is the shield arm ever
# behind the body (forearm behind the pelvis along the chest's facing), and does a death flip (feet above the hips)?
import math
import unreal
A = "/Game/TriadDrift/Gameplay/Characters/Crew/Anims/"
X = unreal.AnimPoseExtensions
W = unreal.AnimPoseSpaces.WORLD
O = unreal.AnimPoseEvaluationOptions()
def log(m): unreal.log("TDPY: " + str(m))
def P(p, b):
    v = X.get_bone_pose(p, b, W).translation
    return (v.x, v.y, v.z)
for name in ("A_Crew_Warden_Guard", "A_Crew_Warden_Walk", "A_Crew_Warden_Fire_01", "A_Crew_Warden_Summon_01", "A_Crew_Warden_Stagger", "A_Crew_Warden_Slam", "A_Crew_Warden_Death_Front"):
    seq = unreal.load_asset(A + name)
    if not seq:
        log(name + " missing"); continue
    n = int(round(seq.get_play_length() * 10)) + 1
    rows, worst_fwd, worst_flip = [], 1e9, -1e9
    for i in range(n):
        t = min(seq.get_play_length(), i / 10.0)
        p = X.get_anim_pose_at_time(seq, t, O)
        pel, la, ha, hd = P(p, "pelvis"), P(p, "lowerarm_l"), P(p, "hand_l"), P(p, "head")
        sl, sr = P(p, "upperarm_l"), P(p, "upperarm_r")
        right = (sr[0] - sl[0], sr[1] - sl[1])                       # chest right in the ground plane
        ln = math.hypot(*right) or 1.0
        fwd = (right[1] / ln, -right[0] / ln)                         # 90 deg from the shoulders: the way the chest faces
        if fwd[1] < 0 and name.find("Death") < 0:
            fwd = (-fwd[0], -fwd[1])
        mid = ((la[0] + ha[0]) / 2 - pel[0], (la[1] + ha[1]) / 2 - pel[1])
        ahead = mid[0] * fwd[0] + mid[1] * fwd[1]
        feet = max(P(p, "foot_l")[2], P(p, "foot_r")[2])
        worst_fwd = min(worst_fwd, ahead); worst_flip = max(worst_flip, feet - pel[2])
        rows.append("%.1f:%.0f/%.0f/%.0f" % (t, ahead, (la[2] + ha[2]) / 2, feet - pel[2]))
    log("%s (%.2f s) t: forearm ahead of pelvis cm / forearm height cm / highest foot above the hips cm" % (name, seq.get_play_length()))
    log("   " + " ".join(rows))
    log("   least ahead %.0f cm; highest foot above the hips %.0f cm" % (worst_fwd, worst_flip))
log("OK")

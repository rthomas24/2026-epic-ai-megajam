# Triad Drift - numbers from the clips themselves: the Warden's gun (right forearm) elevation and heading per frame in
# the two fire clips, and the Grunt charge's feet per frame (which foot leads, which is down).
# Animation builder (eng/anim), Nov 13.
import math
import unreal
X = unreal.AnimPoseExtensions
W = unreal.AnimPoseSpaces.WORLD
D = "/Game/TriadDrift/Gameplay/Characters/Crew/Anims/"
o = unreal.AnimPoseEvaluationOptions()
for n in ("A_Crew_Warden_Fire_01", "A_Crew_Warden_Fire_02"):
    seq = unreal.load_asset(D + n)
    out = []
    for i in range(int(round(seq.get_play_length() * 30)) + 1):
        p = X.get_anim_pose_at_time(seq, min(seq.get_play_length(), i / 30.0), o)
        e, h = X.get_bone_pose(p, "lowerarm_r", W).translation, X.get_bone_pose(p, "hand_r", W).translation
        d = (h.x - e.x, h.y - e.y, h.z - e.z)
        l = math.sqrt(sum(c * c for c in d))
        out.append("f%d %.2fs el%+.0f hd%+.0f" % (i, i / 30.0, math.degrees(math.asin(d[2] / l)), math.degrees(math.atan2(-d[0], d[1]))))
    unreal.log("TDPY: %s gun elevation / heading (deg; heading + = to its right): %s" % (n, " | ".join(out)))
seq = unreal.load_asset(D + "A_Crew_Grunt_Charge")
out = []
for i in range(int(round(seq.get_play_length() * 30)) + 1):
    p = X.get_anim_pose_at_time(seq, min(seq.get_play_length(), i / 30.0), o)
    l, r = X.get_bone_pose(p, "foot_l", W).translation, X.get_bone_pose(p, "foot_r", W).translation
    out.append("f%d L(y%+.0f z%.0f) R(y%+.0f z%.0f)" % (i, l.y, l.z, r.y, r.z))
unreal.log("TDPY: A_Crew_Grunt_Charge feet (forward y, height z, cm): " + " | ".join(out))
unreal.log("TDPY: OK")

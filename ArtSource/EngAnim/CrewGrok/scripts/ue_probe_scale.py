# Triad Drift - probe imported crew clips: local scale and translation of root / pelvis / thigh at t = 0 against the
# skeleton's reference pose. Animation builder (eng/anim), Nov 13.
import unreal
X = unreal.AnimPoseExtensions
L = unreal.AnimPoseSpaces.LOCAL
D = "/Game/TriadDrift/Gameplay/Characters/Crew/Anims/"
for n in ("A_Crew_Trooper_Idle_Aim", "A_Crew_Trooper_Death_Front", "MF_Rifle_Idle_ADS", "A_Crew_Warden_Fire_01", "A_Crew_Warden_Summon_01"):
    seq = unreal.load_asset(D + n)
    sk = seq.get_editor_property("skeleton")
    ref = X.get_reference_pose(sk)
    p = X.get_anim_pose_at_time(seq, 0.0, unreal.AnimPoseEvaluationOptions())
    out = []
    for b in ("root", "pelvis", "thigh_l", "calf_l", "spine_01"):
        t, r = X.get_bone_pose(p, b, L), X.get_ref_bone_pose(ref, b, L)
        out.append("%s scale %.3f loc (%.1f %.1f %.1f) ref loc (%.1f %.1f %.1f)" % (b, t.scale3d.x, t.translation.x, t.translation.y, t.translation.z, r.translation.x, r.translation.y, r.translation.z))
    unreal.log("TDPY: %s [%s]: %s" % (n, sk.get_name(), " | ".join(out)))
unreal.log("TDPY: OK")

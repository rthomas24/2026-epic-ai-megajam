# Triad Drift - measurements on crew clips for T5 and for the round-3 fixes: melee contact times (hand reach and speed),
# charge foot plants, and the facing of pelvis and chest plus the feet over the Heavy bash. Animation builder, Nov 14.
import math
import unreal
A = "/Game/TriadDrift/Gameplay/Characters/Crew/Anims/"
X = unreal.AnimPoseExtensions
W = unreal.AnimPoseSpaces.WORLD
O = unreal.AnimPoseEvaluationOptions()
def log(m): unreal.log("TDPY: " + str(m))
def rot(q, v):
    x, y, z, w = q.x, q.y, q.z, q.w
    t = (2 * (y * v[2] - z * v[1]), 2 * (z * v[0] - x * v[2]), 2 * (x * v[1] - y * v[0]))
    return (v[0] + w * t[0] + y * t[2] - z * t[1], v[1] + w * t[1] + z * t[0] - x * t[2], v[2] + w * t[2] + x * t[1] - y * t[0])
def qinv(q): return unreal.Quat(-q.x, -q.y, -q.z, q.w)
def qmul(a, b):
    return unreal.Quat(a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
                       a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w, a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z)
def frames(seq):
    n = int(round(seq.get_play_length() * 30)) + 1
    return [X.get_anim_pose_at_time(seq, min(seq.get_play_length(), i / 30.0), O) for i in range(n)]
def pos(p, b):
    v = X.get_bone_pose(p, b, W).translation
    return (v.x, v.y, v.z)
def yaw(p, b, ref):
    q = qmul(X.get_bone_pose(p, b, W).rotation, qinv(X.get_ref_bone_pose(ref, b, W).rotation))
    f = rot(q, (0.0, 1.0, 0.0))
    return math.degrees(math.atan2(-f[0], f[1]))
for name in ("A_Crew_Grunt_Melee_02", "A_Crew_Grunt_Melee_03", "A_Crew_Grunt_Melee"):
    seq = unreal.load_asset(A + name)
    if not seq:
        log(name + " missing"); continue
    fs = frames(seq)
    for h in ("hand_r", "hand_l", "foot_r", "foot_l"):
        ps = [pos(p, h) for p in fs]
        rel = [pos(p, h)[1] - pos(p, "pelvis")[1] for p in fs]
        sp = [math.dist(ps[i + 1], ps[i]) * 30 for i in range(len(ps) - 1)]
        i_reach = max(range(len(rel)), key=lambda i: rel[i]); i_sp = max(range(len(sp)), key=lambda i: sp[i])
        log("%s %s: %.2f s long; furthest forward of the pelvis %.0f cm at %.2f s (height %.0f cm); peak speed %.0f cm/s at %.2f s" % (
            name, h, seq.get_play_length(), rel[i_reach], i_reach / 30.0, ps[i_reach][2], sp[i_sp], (i_sp + 0.5) / 30.0))
seq = unreal.load_asset(A + "A_Crew_Grunt_Charge")
fs = frames(seq)
for f in ("foot_l", "foot_r"):
    z = [pos(p, f)[2] for p in fs]
    zmin = min(z)
    downs = [i / 30.0 for i in range(len(z)) if z[i] < zmin + 2.0 and (i == 0 or z[i - 1] >= zmin + 2.0)]
    log("A_Crew_Grunt_Charge %s: %.2f s loop; plants at %s s (lowest %.1f cm)" % (f, seq.get_play_length(), ["%.2f" % d for d in downs], zmin))
ref = X.get_reference_pose(unreal.load_asset("/Game/TriadDrift/Gameplay/Characters/Crew/SKEL_NullCrew"))
for name in ("A_Crew_Heavy_ShieldBash_Grok", "A_Crew_Heavy_ShieldBash_Authored", "A_Crew_Heavy_DeathRM_Front", "A_Crew_Grunt_DeathRM_Front"):
    seq = unreal.load_asset(A + name)
    fs = frames(seq)
    for i in range(0, len(fs), 3):
        p = fs[i]
        log("%s t%.2f pelvis yaw %4.0f chest yaw %4.0f | pelvis %s | foot_l %s foot_r %s | head z %.0f" % (name, i / 30.0, yaw(p, "pelvis", ref), yaw(p, "spine_05", ref),
            tuple(round(c) for c in pos(p, "pelvis")), tuple(round(c) for c in pos(p, "foot_l")), tuple(round(c) for c in pos(p, "foot_r")), pos(p, "head")[2]))
        if name.endswith("Authored") and i >= 3:
            break

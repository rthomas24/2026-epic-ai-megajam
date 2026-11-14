# Triad Drift - blade-through-body check for every pilot clip the game plays (headless). Per frame (30 fps): the hilt
# (BladeSocket -8.5 .. +19.3 cm) and the beam (+19.3 .. +BEAM cm) as segments against body capsules (head, chest,
# belly, thighs, calves, left arm). Reports the worst gap per clip (negative = inside the body) and when.
#   .\Tools\td-python.ps1 -Script ArtSource\EngAnim\Pilot\scripts\ue_p01_clearance.py   (TD_CLIPS=a,b to limit)
# Animation builder (eng/anim), Nov 14.
import json
import math
import os
import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
ANIMS = "/Game/TriadDrift/Gameplay/Characters/Pilot/Anims"
PILOT = "/Game/TriadDrift/Gameplay/Characters/Pilot"
OPTS = unreal.AnimPoseEvaluationOptions()
WORLD = unreal.AnimPoseSpaces.WORLD
BEAM = float(os.environ.get("TD_BEAM_CM", "90"))
DEFAULT = ("A_Pilot_Idle3_v2,A_Pilot_Walk_v2,A_Pilot_Run,A_Pilot_Run_v2,A_Pilot_Blade_Idle_v2,A_Pilot_Light1,A_Pilot_Light2,A_Pilot_Light3,"
           "A_Pilot_Light1_v3,A_Pilot_Light2_v3,A_Pilot_Light3_v3,A_Pilot_Heavy_v3,A_Pilot_Heavy_v4,A_Pilot_Block,A_Pilot_Block_v2,"
           "A_Pilot_Blade_DeflectFlick,A_Pilot_Blade_DeflectFlick_v2,A_Pilot_AirSlam_Fall_v2,A_Pilot_AirSlam_Land_v3,A_Pilot_HitReact_Front,A_Pilot_HitReact_Back")
CLIPS = [c for c in os.environ.get("TD_CLIPS", DEFAULT).split(",") if c]
CAPS = {  # name: (bone a, bone b, radius cm)
    "head": ("head", "head", 13.0), "chest": ("spine_03", "neck_01", 17.0), "belly": ("pelvis", "spine_02", 16.0),
    "thigh_r": ("thigh_r", "calf_r", 10.0), "calf_r": ("calf_r", "foot_r", 7.5),
    "thigh_l": ("thigh_l", "calf_l", 10.0), "calf_l": ("calf_l", "foot_l", 7.5),
    "arm_l": ("upperarm_l", "lowerarm_l", 6.0), "forearm_l": ("lowerarm_l", "hand_l", 5.0),
}


def log(m):
    unreal.log("TDPY: " + str(m))


def find(name):
    for folder in (ANIMS, PILOT):
        p = "%s/%s" % (folder, name)
        if unreal.EditorAssetLibrary.does_asset_exist(p):
            return unreal.load_asset(p)
    return None


def v(x): return [x.x, x.y, x.z]
def sub(a, b): return [a[i] - b[i] for i in range(3)]
def add(a, b): return [a[i] + b[i] for i in range(3)]
def mul(a, s): return [a[i] * s for i in range(3)]
def dot(a, b): return sum(a[i] * b[i] for i in range(3))


def seg_dist(p1, q1, p2, q2):
    d1, d2, r = sub(q1, p1), sub(q2, p2), sub(p1, p2)
    a, e, f = dot(d1, d1), dot(d2, d2), dot(d2, r)
    c, b = dot(d1, r), dot(d1, d2)
    den = a * e - b * b
    s = min(1.0, max(0.0, (b * f - c * e) / den)) if den > 1e-9 else 0.0
    t = (b * s + f) / e if e > 1e-9 else 0.0
    if t < 0.0:
        t, s = 0.0, (min(1.0, max(0.0, -c / a)) if a > 1e-9 else 0.0)
    elif t > 1.0:
        t, s = 1.0, (min(1.0, max(0.0, (b - c) / a)) if a > 1e-9 else 0.0)
    w = sub(add(p1, mul(d1, s)), add(p2, mul(d2, t)))
    return math.sqrt(dot(w, w))


mesh = unreal.load_asset(PILOT + "/SK_Pilot")
sock = mesh.find_socket("BladeSocket")
sq = sock.relative_rotation.quaternion()
sl = sock.relative_location
res = {}
for name in CLIPS:
    seq = find(name)
    if not seq:
        log("%s missing" % name)
        continue
    L = seq.get_play_length()
    worst = {"hilt": (99.0, "", 0.0), "beam": (99.0, "", 0.0)}
    n = int(L * 30) + 1
    for i in range(n):
        t = min(L, i / 30.0)
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, OPTS)
        g = lambda b: unreal.AnimPoseExtensions.get_bone_pose(pose, b, WORLD)
        hand = g("hand_r")
        base = v(hand.translation + hand.rotation.rotate_vector(sl))
        x = v((hand.rotation * sq).rotate_vector(unreal.Vector(1, 0, 0)))
        parts = {"hilt": (add(base, mul(x, -8.5)), add(base, mul(x, 19.3))), "beam": (add(base, mul(x, 19.3)), add(base, mul(x, BEAM)))}
        P = {}
        for cn, (a, b, r) in CAPS.items():
            pa, pb = v(g(a).translation), v(g(b).translation)
            if a == b:
                pb = add(pa, [0, 0, 10.0])
                pa = add(pa, [0, 0, 2.0])
            P[cn] = (pa, pb, r)
        for pn, (p, q) in parts.items():
            for cn, (pa, pb, r) in P.items():
                gap = seg_dist(p, q, pa, pb) - r - 2.0
                if gap < worst[pn][0]:
                    worst[pn] = (gap, cn, t)
    res[name] = {k: {"gap": round(w[0], 1), "part": w[1], "t": round(w[2], 2)} for k, w in worst.items()}
    log("%-32s hilt %+6.1f (%s @%.2f)  beam %+6.1f (%s @%.2f)" % (name, worst["hilt"][0], worst["hilt"][1], worst["hilt"][2], worst["beam"][0], worst["beam"][1], worst["beam"][2]))
json.dump(res, open(os.path.join(HERE, "..", "review", "clearance.json"), "w"), indent=1)
log("OK")

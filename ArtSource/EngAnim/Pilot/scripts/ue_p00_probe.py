# Triad Drift - pilot melee audit probe (headless): what DA_Pilot_Visuals plays, and per clip its length and the
# hand / blade path (mesh space: the pilot faces +Y, right is -X, up +Z), sampled in GAME time at the code's rate.
#   .\Tools\td-python.ps1 -Script ArtSource\EngAnim\Pilot\scripts\ue_p00_probe.py
# Writes ArtSource/EngAnim/Pilot/review/probe.json. Animation builder (eng/anim), Nov 14.
import json
import math
import os
import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "review", os.environ.get("TD_PROBE_OUT", "probe.json"))
ONLY = [x for x in os.environ.get("TD_PROBE_ONLY", "").split(",") if x]
ANIMS = "/Game/TriadDrift/Gameplay/Characters/Pilot/Anims"
PILOT = "/Game/TriadDrift/Gameplay/Characters/Pilot"
OPTS = unreal.AnimPoseEvaluationOptions()
WORLD = unreal.AnimPoseSpaces.WORLD
CLIPS = {  # name: game play rate
    "A_Pilot_Land_Hard": 1.0, "A_Pilot_Land": 1.0, "A_Pilot_FallLoop": 1.0, "A_Pilot_Run_v2": 1.0,
    "A_Pilot_Light1_v3": 1.6, "A_Pilot_Light2_v3": 1.6, "A_Pilot_Light3_v3": 1.5,
    "A_Pilot_Light1": 1.6, "A_Pilot_Light2": 1.6, "A_Pilot_Light3": 1.5,
    "A_Pilot_Light1_v2": 1.6, "A_Pilot_Light2_v2": 1.6, "A_Pilot_Light3_v2": 1.5,
    "A_Pilot_Heavy_v3": 1.4, "A_Pilot_Blade_HeavyWindup": 1.6, "A_Pilot_AirSlam_Land_v2": 1.0, "A_Pilot_AirSlam_Fall": 1.0,
    "A_Pilot_Block": 1.0, "A_Pilot_Blade_BlockHold": 1.0, "A_Pilot_Blade_DeflectFlick": 1.0, "A_Pilot_HitReact_Front": 1.2,
    "A_Pilot_HitReact_Back": 1.2, "A_Pilot_Idle3_v2": 1.0, "A_Pilot_Walk_v2": 1.0, "A_Pilot_Run": 1.0, "A_Pilot_Blade_Idle": 1.0,
    "A_Pilot_Jog_Fwd": 1.0, "A_Pilot_Jog_Left": 1.0, "A_Pilot_Jog_Right": 1.0, "A_Pilot_Jog_Bwd": 1.0,
    "A_Pilot_Walk_Fwd": 1.0, "A_Pilot_Walk_Left": 1.0, "A_Pilot_Walk_Right": 1.0, "A_Pilot_Walk_Bwd": 1.0,
    "A_Pilot_Blade_Light1": 1.0, "A_Pilot_Blade_Light2": 1.0, "A_Pilot_Blade_Light3": 1.0, "A_Pilot_Blade_HeavyLunge": 1.0,
    "A_Pilot_Blade_AirSlam": 1.0, "A_Pilot_Walk": 1.0, "A_Pilot_Idle3": 1.0,
}


def log(m):
    unreal.log("TDPY: " + str(m))


def find(name):
    for folder in (ANIMS, PILOT):
        p = "%s/%s" % (folder, name)
        if unreal.EditorAssetLibrary.does_asset_exist(p):
            return unreal.load_asset(p)
    return None


mesh = unreal.load_asset(PILOT + "/SK_Pilot")
sock = mesh.find_socket("BladeSocket")
sq = sock.relative_rotation.quaternion() if sock else unreal.Quat()
res = {"socket": None, "visuals": {}, "clips": {}}
if sock:
    res["socket"] = {"bone": str(sock.bone_name), "loc": [sock.relative_location.x, sock.relative_location.y, sock.relative_location.z],
                     "rot": [sock.relative_rotation.roll, sock.relative_rotation.pitch, sock.relative_rotation.yaw]}
da = unreal.load_asset(PILOT + "/DA_Pilot_Visuals")
if da:
    for prop in ("idle", "walk", "run", "anim_bp_class", "mesh_yaw", "mesh_scale"):
        try:
            v = da.get_editor_property(prop)
            res["visuals"][prop] = str(v)
        except Exception as e:  # noqa: BLE001
            res["visuals"][prop] = "?" + str(e)[:60]


def rot(q, v):
    return q.rotate_vector(unreal.Vector(*v))


for n_ in ONLY:
    CLIPS.setdefault(n_, 1.0)
for name, rate in CLIPS.items():
    if ONLY and name not in ONLY:
        continue
    seq = find(name)
    if not seq:
        res["clips"][name] = None
        continue
    L = seq.get_play_length()
    info = {"len": round(L, 3), "game_len": round(L / rate, 3), "rate": rate, "root_motion": bool(seq.get_editor_property("enable_root_motion")),
            "additive": str(seq.get_editor_property("additive_anim_type")), "samples": []}
    steps = max(2, int(L / rate / 0.0334) + 1)
    for i in range(steps + 1):
        tg = min(L / rate, i * 0.0334)
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, tg * rate, OPTS)
        g = lambda b: unreal.AnimPoseExtensions.get_bone_pose(pose, b, WORLD)
        hand, pelvis, root = g("hand_r"), g("pelvis"), g("root")
        fl, fr_, head = g("foot_l"), g("foot_r"), g("head")
        blade = rot(hand.rotation * sq, (1, 0, 0))
        h = hand.translation
        info["samples"].append({
            "t": round(tg, 3),
            "hand": [round(h.x, 1), round(h.y, 1), round(h.z, 1)],
            "blade": [round(blade.x, 2), round(blade.y, 2), round(blade.z, 2)],
            "pelvis": [round(pelvis.translation.x, 1), round(pelvis.translation.y, 1), round(pelvis.translation.z, 1)],
            "root": [round(root.translation.x, 1), round(root.translation.y, 1)],
            "foot_l": [round(fl.translation.x, 1), round(fl.translation.y, 1), round(fl.translation.z, 1)],
            "foot_r": [round(fr_.translation.x, 1), round(fr_.translation.y, 1), round(fr_.translation.z, 1)],
            "head": [round(head.translation.x, 1), round(head.translation.y, 1), round(head.translation.z, 1)],
        })
    res["clips"][name] = info
    log("%s len %.2f (game %.2f)" % (name, L, L / rate))

os.makedirs(os.path.dirname(OUT), exist_ok=True)
json.dump(res, open(OUT, "w"), indent=1)
log("visuals " + json.dumps(res["visuals"]))
log("socket " + json.dumps(res["socket"]))
log("OK")

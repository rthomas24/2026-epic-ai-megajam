# Triad Drift - dump SK_Pilot's right hand reference pose (local and component space), its sockets, and the
# SM_TD_BladeHilt bounds and sockets. Run headless: .\Tools\td-python.ps1 -Script ArtSource\EngAnim\PilotGrip\scripts\inspect_pilot_hand.py
# Writes Saved/PilotGrip/hand_ref.json. Animation builder (eng/anim), Nov 13.
import json
import os
import unreal

OUT = os.path.join(unreal.Paths.project_saved_dir(), "PilotGrip")
os.makedirs(OUT, exist_ok=True)


def log(msg):
    unreal.log("TDPY: " + str(msg))


def tf(t):
    r = t.rotation
    return {
        "loc": [t.translation.x, t.translation.y, t.translation.z],
        "quat": [r.x, r.y, r.z, r.w],
        "rot": [t.rotation.rotator().roll, t.rotation.rotator().pitch, t.rotation.rotator().yaw],
    }


sk = unreal.load_asset("/Game/TriadDrift/Gameplay/Characters/Pilot/SK_Pilot")
skel = sk.get_editor_property("skeleton")
log("mesh %s skeleton %s" % (sk.get_path_name(), skel.get_path_name()))
res = {"mesh_sockets": [], "bones": {}}
for i in range(sk.num_sockets()):
    s = sk.get_socket_by_index(i)
    res["mesh_sockets"].append([str(s.socket_name), str(s.bone_name), [s.relative_location.x, s.relative_location.y, s.relative_location.z]])

pose = unreal.AnimPoseExtensions.get_reference_pose(skel)
names = unreal.AnimPoseExtensions.get_bone_names(pose)
want = ("hand_r", "lowerarm_r", "upperarm_r", "index", "middle", "ring", "pinky", "thumb", "clavicle_r", "root", "pelvis")
for n in names:
    s = str(n)
    if not any(w in s for w in want):
        continue
    if s.endswith("_l"):
        continue
    loc = unreal.AnimPoseExtensions.get_ref_bone_pose(pose, n, unreal.AnimPoseSpaces.LOCAL)
    wld = unreal.AnimPoseExtensions.get_ref_bone_pose(pose, n, unreal.AnimPoseSpaces.WORLD)
    parent = unreal.AnimPoseExtensions.get_parent_bone_name(pose, n) if hasattr(unreal.AnimPoseExtensions, "get_parent_bone_name") else ""
    res["bones"][s] = {"local": tf(loc), "cs": tf(wld), "parent": str(parent)}
res["all_bones"] = [str(n) for n in names]

hilt = unreal.load_asset("/Game/TriadDrift/Gameplay/Props/Blade/SM_TD_BladeHilt")
bb = hilt.get_bounding_box()
res["hilt_bbox"] = [[bb.min.x, bb.min.y, bb.min.z], [bb.max.x, bb.max.y, bb.max.z]]
res["hilt_sockets"] = []
for s in [hilt.find_socket("BladeEmitter")]:
    if s: res["hilt_sockets"].append([str(s.socket_name), [s.relative_location.x, s.relative_location.y, s.relative_location.z],
                                [s.relative_rotation.roll, s.relative_rotation.pitch, s.relative_rotation.yaw]])

da = unreal.load_asset("/Game/TriadDrift/Gameplay/Characters/Pilot/DA_Pilot_Visuals")
if da:
    res["da"] = {k: str(da.get_editor_property(k)) for k in ("idle", "walk", "run", "mesh_scale", "mesh_yaw")}

with open(os.path.join(OUT, "hand_ref.json"), "w") as f:
    json.dump(res, f, indent=1)
log("bones %d written %s" % (len(res["bones"]), os.path.join(OUT, "hand_ref.json")))
log("OK")

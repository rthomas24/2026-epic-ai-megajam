# Triad Drift - write A_Pilot_Grip_R (single-frame AnimSequence on SKEL_Pilot: the right hand closed round the blade
# hilt) and the BladeSocket socket on SKEL_Pilot, from the solver's output (grip_pose.json, made by solve_grip.py).
# Run headless: .\Tools\td-python.ps1 -Script ArtSource\EngAnim\PilotGrip\scripts\ue_make_grip.py
# Only the finger, thumb and metacarpal bones under hand_r get tracks. Animation builder (eng/anim), Nov 13.
import json
import os
import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
POSE = os.path.join(HERE, "..", "grip_pose.json")
FOLDER = "/Game/TriadDrift/Gameplay/Characters/Pilot/Anims"
NAME = "A_Pilot_Grip_R"
SKEL = "/Game/TriadDrift/Gameplay/Characters/Pilot/SKEL_Pilot"
MESH = "/Game/TriadDrift/Gameplay/Characters/Pilot/SK_Pilot"


def log(msg):
    unreal.log("TDPY: " + str(msg))


with open(POSE) as f:
    data = json.load(f)

skel = unreal.load_asset(SKEL)
mesh = unreal.load_asset(MESH)
tools = unreal.AssetToolsHelpers.get_asset_tools()
path = FOLDER + "/" + NAME
seq = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
if not seq:
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", skel)
    seq = tools.create_asset(NAME, FOLDER, unreal.AnimSequence, factory)
log("sequence %s" % seq.get_path_name())

ctrl = seq.get_editor_property("controller")
ctrl.open_bracket(unreal.Text("Pilot grip"), False)
ctrl.set_frame_rate(unreal.FrameRate(30, 1), False)
ctrl.set_number_of_frames(unreal.FrameNumber(1), False)
existing = [str(n) for n in unreal.AnimationLibrary.get_animation_track_names(seq)] if hasattr(unreal.AnimationLibrary, "get_animation_track_names") else []
for b in existing:
    if b not in data["bones"]:
        ctrl.remove_bone_track(b, False)
for bone, t in data["bones"].items():
    if bone not in existing:
        ctrl.add_bone_curve(bone, False)
    loc = unreal.Vector(*t["loc"])
    q = unreal.Quat(*t["quat"])
    one = unreal.Vector(1.0, 1.0, 1.0)
    ctrl.set_bone_track_keys(bone, [loc, loc], [q, q], [one, one], False)
ctrl.close_bracket(False)
seq.set_editor_property("enable_root_motion", False)
unreal.EditorAssetLibrary.save_loaded_asset(seq, False)
log("tracks %d frames %s" % (len(data["bones"]), seq.get_editor_property("number_of_sampled_frames") if hasattr(seq, "number_of_sampled_frames") else "?"))

# BladeSocket on the skeleton (every mesh of SKEL_Pilot gets it): the hilt's pivot (grip centre) in the palm.
# Python cannot name a socket (SocketName is read-only to scripts), so the game module's editor command writes it.
s = data["socket"]
unreal.SystemLibrary.execute_console_command(None, "TD.Ground.GripWriteSocket %s %.4f %.4f %.4f %.4f %.4f %.4f" % (
    skel.get_path_name(), s["loc"][0], s["loc"][1], s["loc"][2], s["rot"][0], s["rot"][1], s["rot"][2]))
unreal.EditorAssetLibrary.save_loaded_asset(skel, False)
found = mesh.find_socket("BladeSocket")
if not found:
    raise RuntimeError("BladeSocket not on SK_Pilot after the write")
log("socket %s on %s loc=%s rot=%s (outer %s)" % (found.socket_name, found.bone_name, found.relative_location, found.relative_rotation, found.get_outer().get_name()))
log("OK")

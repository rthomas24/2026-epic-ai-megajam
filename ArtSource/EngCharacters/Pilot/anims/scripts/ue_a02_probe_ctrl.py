# Read-only probe: AnimDataController and AnimPoseExtensions API in this build.
import unreal


def log(m):
    for line in str(m).splitlines():
        unreal.log("TDPY: " + line)


seq = unreal.EditorAssetLibrary.load_asset("/Game/TriadDrift/Gameplay/Characters/Pilot/Anims/A_Pilot_Light1")
c = seq.controller
log("controller type %s" % type(c))
log("ctrl methods: " + ", ".join(m for m in dir(c) if not m.startswith("_")))
for m in ["resize_number_of_frames", "set_number_of_frames", "resize_in_frames", "remove_bone_track", "set_bone_track_keys",
          "open_bracket", "close_bracket", "set_frame_rate", "update_with_skeleton"]:
    f = getattr(c, m, None)
    log("=== %s" % m)
    log(f.__doc__ if f else "absent")
ape = unreal.AnimPoseExtensions
log("ape methods: " + ", ".join(m for m in dir(ape) if not m.startswith("_")))
for m in ["set_bone_pose", "get_reference_pose", "get_anim_pose_at_frame", "get_bone_pose", "get_ref_bone_pose"]:
    f = getattr(ape, m, None)
    log("=== %s" % m)
    log(f.__doc__ if f else "absent")
log("OK")

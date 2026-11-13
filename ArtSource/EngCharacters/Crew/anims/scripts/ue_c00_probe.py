# Crew anims step 0: probe the pose-evaluation and compression API, and sample the SOURCE MM_Idle from raw and
# compressed data (control). Writes nothing.
import unreal


def log(m):
    for line in str(m).splitlines():
        unreal.log("TDPY: " + line)


for cname, meths in [
    ("AnimPoseExtensions", ["get_anim_pose_at_time", "get_anim_pose_at_frame", "get_bone_pose", "get_bone_names"]),
    ("AnimationLibrary", ["get_bone_pose_for_time", "request_anim_compression", "get_bone_compression_settings",
                          "set_bone_compression_settings", "get_curve_compression_settings", "set_curve_compression_settings"]),
    ("IKRetargetBatchOperation", ["duplicate_and_retarget"]),
]:
    c = getattr(unreal, cname, None)
    for m in meths:
        f = getattr(c, m, None) if c else None
        log("=== %s.%s" % (cname, m))
        log(f.__doc__ if f else "absent")
for s in ("AnimPoseEvaluationOptions", "AnimDataEvalType", "AnimCompressParams", "AnimPoseSpaces"):
    t = getattr(unreal, s, None)
    log("=== " + s)
    log(t.__doc__ if t else "absent")
    if t and hasattr(t, "__members__"):
        log(list(t.__members__.keys()) if hasattr(t, "__members__") else "")

SRC = "/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle"
seq = unreal.EditorAssetLibrary.load_asset(SRC)
log("src bone codec %s curve codec %s" % (seq.get_editor_property("bone_compression_settings"),
                                          seq.get_editor_property("curve_compression_settings")))
for et in ("RAW", "COMPRESSED", "SOURCE"):
    try:
        o = unreal.AnimPoseEvaluationOptions()
        o.set_editor_property("evaluation_type", getattr(unreal.AnimDataEvalType, et))
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, 0.5, o)
        vals = {}
        for b in ("root", "pelvis", "head", "foot_l"):
            tr = unreal.AnimPoseExtensions.get_bone_pose(pose, b, unreal.AnimPoseSpaces.WORLD)
            vals[b] = tr.translation
        log("SRC %s %s" % (et, vals))
    except Exception as e:
        log("SRC %s error %s" % (et, e))
d = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
log("dirty packages: %s" % [p.get_name() for p in d])
log("OK")

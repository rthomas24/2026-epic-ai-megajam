# Crew anims probe: can Python wait for async animation compression, so that "COMPRESSED" evaluation really reads
# compressed data? Writes nothing.
import time
import unreal


def log(m):
    for line in str(m).splitlines():
        unreal.log("TDPY: " + line)


log("attrs: %s" % [a for a in dir(unreal) if "ompil" in a or "Compress" in a])
seq = unreal.EditorAssetLibrary.load_asset("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/MM_Idle")
log("seq props with compress: %s" % [l for l in seq.__class__.__doc__.splitlines() if "ompress" in l])


def diff():
    def p(et):
        o = unreal.AnimPoseEvaluationOptions()
        o.set_editor_property("evaluation_type", getattr(unreal.AnimDataEvalType, et))
        ps = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, 1.234, o)
        return {str(n): unreal.AnimPoseExtensions.get_bone_pose(ps, n, unreal.AnimPoseSpaces.WORLD).translation
                for n in unreal.AnimPoseExtensions.get_bone_names(ps)}
    r, c = p("RAW"), p("COMPRESSED")
    return max((r[b] - c[b]).length() for b in r), c["head"].z


log("diff before %s" % (diff(),))
for cmd in ("a.Compression.WaitForAll", "Anim.WaitForCompression"):
    unreal.SystemLibrary.execute_console_command(None, cmd)
for i in range(20):
    time.sleep(0.5)
    try:
        unreal.AssetCompilingManager.get().process_async_tasks()
    except Exception:
        pass
log("diff after sleep %s" % (diff(),))
log("OK")

# Pilot anims step 3: make the retargeted sequences in place. The root bone's translation is set to zero on every key
# (rotation and scale kept), root motion is switched off. Only sequences in Pilot/Anims are touched and saved.
import unreal

OUT = "/Game/TriadDrift/Gameplay/Characters/Pilot/Anims"
EAL = unreal.EditorAssetLibrary
NAMES = ["A_Pilot_Light1", "A_Pilot_Light2", "A_Pilot_Light3", "A_Pilot_Heavy", "A_Pilot_Dash", "A_Pilot_JumpStart",
         "A_Pilot_FallLoop", "A_Pilot_Land", "A_Pilot_HitReact_Front", "A_Pilot_HitReact_Back",
         "A_Pilot_Death_Front", "A_Pilot_Death_Back"]


def log(m):
    unreal.log("TDPY: " + str(m))


def main():
    ok_all = True
    for name in NAMES:
        path = OUT + "/" + name
        seq = EAL.load_asset(path)
        if seq is None:
            log("missing " + path); ok_all = False; continue
        nkeys = unreal.AnimationLibrary.get_num_frames(seq) + 1
        poses = []
        for f in range(nkeys):
            t = unreal.AnimationLibrary.get_bone_poses_for_frame(seq, ["root", "pelvis"], f, False)
            poses.append(t)
        r0 = poses[0][0].translation; rN = poses[-1][0].translation
        maxr = max(p[0].translation.length() for p in poses)
        pel = [p[1].translation for p in poses]
        log("%s keys=%d root first=%s last=%s max|t|=%.2f pelvis(local) first=%s last=%s" % (
            name, nkeys, r0, rN, maxr, pel[0], pel[-1]))
        ctrl = seq.controller
        if maxr > 0.01:
            pos = [unreal.Vector(0, 0, 0) for _ in poses]
            rot = [p[0].rotation for p in poses]
            scl = [p[0].scale3d for p in poses]
            try:
                ctrl.set_bone_track_keys("root", pos, rot, scl, True)
                log("%s root translation zeroed" % name)
            except Exception as e:
                log("%s set keys failed: %s" % (name, e)); ok_all = False; continue
        seq.set_editor_property("enable_root_motion", False)
        after = max(unreal.AnimationLibrary.get_bone_poses_for_frame(seq, ["root"], f, False)[0].translation.length()
                    for f in range(nkeys))
        log("%s after: root max|t|=%.3f len=%.3f frames=%d" % (name, after, seq.get_play_length(),
            unreal.AnimationLibrary.get_num_frames(seq)))
        log("save %s %s" % (name, EAL.save_asset(path, only_if_is_dirty=False)))
    d = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    log("dirty packages: %s" % [p.get_name() for p in d])
    log("OK" if ok_all else "FAILED")


main()

# Null crew own skeleton, step 2: IK_NullCrew on SK_NullTrooper (auto retarget definition, like IK_Pilot), retargeter
# RTG_Manny_to_NullCrew (source IK_Crew_Manny on SKM_Manny_Simple), batch retarget of the 14 Crew/Anims sequences
# (currently on SK_Mannequin) into Crew/Anims/TmpNC, then: in-place loops (root zeroed, root motion off), non-additive,
# ACL codec; delete the SK_Mannequin versions and move the new ones into Crew/Anims under the exact same names.
# Saves ONLY inside /Game/TriadDrift/Gameplay/Characters/Crew.
# Run: powershell -File Tools\td-python.ps1 -Script ArtSource\EngCharacters\Crew\anims\scripts\ue_n20_retarget.py
import unreal

CREW = "/Game/TriadDrift/Gameplay/Characters/Crew"
OUT = CREW + "/Anims"
TMP = OUT + "/TmpNC"
SRC_MESH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
TGT_MESH = "/Game/TriadDrift/Gameplay/Characters/NullTrooper/SK_NullTrooper"
SKEL = CREW + "/SKEL_NullCrew"
NAMES = ["MM_Idle", "MF_Unarmed_Walk_Fwd", "MF_Unarmed_Jog_Fwd", "MF_Rifle_Idle_ADS", "MF_Rifle_Walk_Fwd", "MF_Rifle_Jog_Fwd",
         "MM_Attack_01", "MM_ChargedAttack", "MM_Rifle_Fire", "MM_HitReact_Front_Lgt_01",
         "MM_Death_Front_01", "MM_Death_Back_01", "MM_Death_Left_01", "MM_Death_Right_01"]
LOOPS = set(NAMES[:6])
EAL = unreal.EditorAssetLibrary
SOT = unreal.RetargetSourceOrTarget
problems = []


def log(m):
    for line in str(m).splitlines():
        unreal.log("TDPY: " + line)


def fail(m):
    problems.append(m)
    log("FAILED: " + m)


def skel_of(seq):
    s = seq.get_editor_property("skeleton")
    return s.get_path_name().split(".")[0] if s else None


def main():
    src_mesh = EAL.load_asset(SRC_MESH)
    tgt_mesh = EAL.load_asset(TGT_MESH)
    if skel_of(tgt_mesh) != SKEL:
        fail("SK_NullTrooper is not on SKEL_NullCrew (%s)" % skel_of(tgt_mesh)); return
    acl = EAL.load_asset("/ACLPlugin/ACLAnimBoneCompressionSettings")
    curve = EAL.load_asset("/ACLPlugin/ACLAnimCurveCompressionSettings") or EAL.load_asset("/Engine/Animation/DefaultAnimCurveCompressionSettings")
    log("codec %s curve %s" % (acl.get_path_name(), curve.get_path_name()))

    src_rig = EAL.load_asset(OUT + "/IK_Crew_Manny")
    sc = unreal.IKRigController.get_controller(src_rig)
    log("IK_Crew_Manny mesh %s root %s chains %d" % (sc.get_skeletal_mesh().get_path_name() if sc.get_skeletal_mesh() else None,
                                                    sc.get_retarget_root(), len(sc.get_retarget_chains())))
    rp = CREW + "/IK_NullCrew"
    tgt_rig = EAL.load_asset(rp) if EAL.does_asset_exist(rp) else unreal.IKRigDefinitionFactory.create_new_ik_rig_asset(CREW, "IK_NullCrew")
    tc = unreal.IKRigController.get_controller(tgt_rig)
    log("IK_NullCrew set mesh %s" % tc.set_skeletal_mesh(tgt_mesh))
    ok = tc.apply_auto_generated_retarget_definition()
    log("IK_NullCrew auto retarget def %s root %s" % (ok, tc.get_retarget_root()))
    if not ok:
        fail("auto retarget definition"); return
    for ch in tc.get_retarget_chains():
        log("  chain %s" % ch)

    rtp = CREW + "/RTG_Manny_to_NullCrew"
    rtg = EAL.load_asset(rtp) if EAL.does_asset_exist(rtp) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "RTG_Manny_to_NullCrew", CREW, unreal.IKRetargeter, unreal.IKRetargetFactory())
    rc = unreal.IKRetargeterController.get_controller(rtg)
    rc.set_ik_rig(SOT.SOURCE, src_rig)
    rc.set_ik_rig(SOT.TARGET, tgt_rig)
    rc.set_preview_mesh(SOT.SOURCE, src_mesh)
    rc.set_preview_mesh(SOT.TARGET, tgt_mesh)
    if rc.get_num_retarget_ops() == 0:
        rc.add_default_ops()
    rc.assign_ik_rig_to_all_ops(SOT.SOURCE, src_rig)
    rc.assign_ik_rig_to_all_ops(SOT.TARGET, tgt_rig)
    rc.auto_map_chains(unreal.AutoMapChainType.EXACT, True)
    log("chain map %s" % rc.get_chain_map())
    for i in range(rc.get_num_retarget_ops()):
        log("op %d %s enabled=%s" % (i, rc.get_op_name(i), rc.get_retarget_op_enabled(i)))
    for p in (rp, rtp):
        log("save %s %s" % (p, EAL.save_asset(p, only_if_is_dirty=False)))

    if not EAL.does_directory_exist(TMP):
        EAL.make_directory(TMP)
    made = []
    for name in NAMES:
        src = OUT + "/" + name
        srcseq = EAL.load_asset(src)
        if srcseq is None:
            fail("source missing " + src); continue
        log("SRC %s skel=%s len=%.4f frames=%d rootmotion=%s" % (name, skel_of(srcseq), srcseq.get_play_length(),
            unreal.AnimationLibrary.get_num_frames(srcseq), srcseq.get_editor_property("enable_root_motion")))
        dst = TMP + "/" + name
        if EAL.does_asset_exist(dst):
            EAL.delete_asset(dst)
        res = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
            [EAL.find_asset_data(src)], src_mesh, tgt_mesh, rtg, "", "", "", "", TMP, False, False, False)
        log("retargeted %s -> %s" % (name, [str(r.package_name) for r in res]))
        seq = EAL.load_asset(dst) if EAL.does_asset_exist(dst) else None
        if seq is None:
            fail("missing after retarget " + dst); continue
        if seq.get_editor_property("additive_anim_type") != unreal.AdditiveAnimationType.AAT_NONE:
            seq.set_editor_property("additive_anim_type", unreal.AdditiveAnimationType.AAT_NONE)
        if name in LOOPS:
            nk = unreal.AnimationLibrary.get_num_frames(seq) + 1
            poses = [unreal.AnimationLibrary.get_bone_poses_for_frame(seq, ["root"], f, False)[0] for f in range(nk)]
            maxr = max(p.translation.length() for p in poses)
            if maxr > 0.01:
                seq.controller.set_bone_track_keys("root", [unreal.Vector(0, 0, 0)] * nk,
                                                   [p.rotation for p in poses], [p.scale3d for p in poses], True)
            seq.set_editor_property("enable_root_motion", False)
            log("%s loop: root max|t| before %.2f, zeroed, root motion off" % (name, maxr))
        else:
            seq.set_editor_property("enable_root_motion", srcseq.get_editor_property("enable_root_motion"))
        unreal.AnimationLibrary.set_bone_compression_settings(seq, acl)
        unreal.AnimationLibrary.set_curve_compression_settings(seq, curve)
        try:
            seq.set_editor_property("preview_pose_asset", None)
        except Exception:
            pass
        EAL.save_asset(dst, only_if_is_dirty=False)
        log("NEWTMP %s skel=%s len=%.4f (src %.4f) frames=%d" % (name, skel_of(seq), seq.get_play_length(), srcseq.get_play_length(),
            unreal.AnimationLibrary.get_num_frames(seq)))
        if skel_of(seq) != SKEL:
            fail("%s retargeted onto %s" % (name, skel_of(seq))); continue
        if abs(seq.get_play_length() - srcseq.get_play_length()) > 0.002:
            fail("%s length changed" % name)
        made.append(name)
    if len(made) != len(NAMES):
        fail("only %d of %d made; old Crew/Anims left untouched" % (len(made), len(NAMES)))
        log("FAILED: %s" % problems); return

    # swap: delete SK_Mannequin versions, move the new ones into Crew/Anims under the exact names
    for name in NAMES:
        final = OUT + "/" + name
        refs = list(EAL.find_package_referencers_for_asset(final, False))
        if refs:
            log("note: %s referencers before delete %s" % (name, refs))
        log("delete old %s %s" % (name, EAL.delete_asset(final)))
        if not EAL.rename_asset(TMP + "/" + name, final):
            fail("rename %s" % name); continue
        EAL.save_asset(final, only_if_is_dirty=False)
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    for data in reg.get_assets_by_path(TMP, recursive=True):
        log("leftover in TmpNC %s %s -> delete %s" % (data.package_name, data.asset_class_path.asset_name, EAL.delete_asset(str(data.package_name))))
    log("delete dir TmpNC %s" % EAL.delete_directory(TMP))
    try:
        unreal.AssetCompilingManager.get().finish_all_compilation()
    except Exception as e:
        log("finish_all_compilation n/a %s" % e)
    for name in NAMES:
        seq = EAL.load_asset(OUT + "/" + name)
        log("FINAL %s skel=%s len=%.4f frames=%d rootmotion=%s codec=%s" % (name, skel_of(seq), seq.get_play_length(),
            unreal.AnimationLibrary.get_num_frames(seq), seq.get_editor_property("enable_root_motion"),
            unreal.AnimationLibrary.get_bone_compression_settings(seq).get_path_name()))
        if skel_of(seq) != SKEL:
            fail("final %s on %s" % (name, skel_of(seq)))
    dirty = [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]
    log("still dirty (not saved): %s" % dirty)
    log("FAILED: %s" % problems if problems else "OK")


main()

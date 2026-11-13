# Crew anims step 1 (route 1): identity IK rig on SKM_Manny_Simple + identity IK Retargeter (Manny -> Manny), then a
# batch duplicate_and_retarget of the template mannequin sequences into Crew/Anims, so every sequence is created fresh
# (new raw data, new compressed data) on SK_Mannequin. Then: loops in place + root motion off, additive sources made
# full-pose, ACL default codec, compression requested, and pose sampling from raw and compressed data.
# Saves ONLY assets inside /Game/TriadDrift/Gameplay/Characters/Crew/Anims.
# Job subset: Saved/CrewAnims/jobs.txt (one asset name per line); absent = all.
# Run: powershell -File Tools\td-python.ps1 -Script ArtSource\EngCharacters\Crew\anims\scripts\ue_c10_retarget.py
import os
import unreal

OUT = "/Game/TriadDrift/Gameplay/Characters/Crew/Anims"
MESH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
SKEL = "/Game/Characters/Mannequins/Meshes/SK_Mannequin"
A = "/Game/Characters/Mannequins/Anims/"
LOOPS = {"MM_Idle", "MF_Unarmed_Walk_Fwd", "MF_Unarmed_Jog_Fwd", "MF_Rifle_Idle_ADS", "MF_Rifle_Walk_Fwd", "MF_Rifle_Jog_Fwd"}
JOBS = [
    A + "Unarmed/MM_Idle",
    A + "Unarmed/Walk/MF_Unarmed_Walk_Fwd",
    A + "Unarmed/Jog/MF_Unarmed_Jog_Fwd",
    A + "Rifle/MF_Rifle_Idle_ADS",
    A + "Rifle/Walk/MF_Rifle_Walk_Fwd",
    A + "Rifle/Jog/MF_Rifle_Jog_Fwd",
    A + "Unarmed/Attack/MM_Attack_01",
    A + "Unarmed/Attack/MM_ChargedAttack",
    A + "Rifle/MM_Rifle_Fire",
    A + "Rifle/HitReact/MM_HitReact_Front_Lgt_01",
    A + "Death/MM_Death_Front_01",
    A + "Death/MM_Death_Back_01",
    A + "Death/MM_Death_Left_01",
    A + "Death/MM_Death_Right_01",
]
EAL = unreal.EditorAssetLibrary
SOT = unreal.RetargetSourceOrTarget
BONES = ("root", "pelvis", "head", "foot_l", "foot_r")


def log(m):
    for line in str(m).splitlines():
        unreal.log("TDPY: " + line)


def dirty(tag):
    d = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    log("%s dirty packages: %s" % (tag, [p.get_name() for p in d]))


def sample(seq, t, et):
    o = unreal.AnimPoseEvaluationOptions()
    o.set_editor_property("evaluation_type", getattr(unreal.AnimDataEvalType, et))
    o.set_editor_property("extract_root_motion", False)
    pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, o)
    return {b: unreal.AnimPoseExtensions.get_bone_pose(pose, b, unreal.AnimPoseSpaces.WORLD).translation for b in BONES}


def report_pose(name, seq, tag):
    L = seq.get_play_length()
    for t in (0.0, L * 0.5, max(0.0, L - 0.034)):
        for et in ("RAW", "COMPRESSED"):
            p = sample(seq, t, et)
            feet = min(p["foot_l"].z, p["foot_r"].z)
            log("POSE %s %s %s t=%.3f head=(%.1f,%.1f,%.1f) pelvis=(%.1f,%.1f,%.1f) foot_l=(%.1f,%.1f,%.1f) root=(%.1f,%.1f,%.1f) head-minfoot=%.1f head-pelvis=%.1f" % (
                tag, name, et, t, p["head"].x, p["head"].y, p["head"].z, p["pelvis"].x, p["pelvis"].y, p["pelvis"].z,
                p["foot_l"].x, p["foot_l"].y, p["foot_l"].z, p["root"].x, p["root"].y, p["root"].z,
                p["head"].z - feet, p["head"].z - p["pelvis"].z))


def make_rig(mesh):
    path = OUT + "/IK_Crew_Manny"
    rig = EAL.load_asset(path) if EAL.does_asset_exist(path) else unreal.IKRigDefinitionFactory.create_new_ik_rig_asset(OUT, "IK_Crew_Manny")
    c = unreal.IKRigController.get_controller(rig)
    log("rig set mesh %s" % c.set_skeletal_mesh(mesh))
    ok = c.apply_auto_generated_retarget_definition()
    log("rig auto retarget def %s root %s chains %d" % (ok, c.get_retarget_root(), len(c.get_retarget_chains())))
    return rig, ok


def make_rtg(rig, mesh):
    rpath = OUT + "/RTG_Crew_Manny_Identity"
    rtg = EAL.load_asset(rpath) if EAL.does_asset_exist(rpath) else unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "RTG_Crew_Manny_Identity", OUT, unreal.IKRetargeter, unreal.IKRetargetFactory())
    rc = unreal.IKRetargeterController.get_controller(rtg)
    rc.set_ik_rig(SOT.SOURCE, rig)
    rc.set_ik_rig(SOT.TARGET, rig)
    rc.set_preview_mesh(SOT.SOURCE, mesh)
    rc.set_preview_mesh(SOT.TARGET, mesh)
    if rc.get_num_retarget_ops() == 0:
        rc.add_default_ops()
    rc.assign_ik_rig_to_all_ops(SOT.SOURCE, rig)
    rc.assign_ik_rig_to_all_ops(SOT.TARGET, rig)
    rc.auto_map_chains(unreal.AutoMapChainType.EXACT, True)
    for i in range(rc.get_num_retarget_ops()):
        log("op %d %s enabled=%s" % (i, rc.get_op_name(i), rc.get_retarget_op_enabled(i)))
    return rtg, rpath


def main():
    os.makedirs(os.path.join(unreal.Paths.project_saved_dir(), "CrewAnims"), exist_ok=True)
    jf = os.path.join(unreal.Paths.project_saved_dir(), "CrewAnims", "jobs.txt")
    only = None
    if os.path.isfile(jf):
        only = set(x.strip() for x in open(jf) if x.strip())
    jobs = [j for j in JOBS if only is None or j.split("/")[-1] in only]
    log("jobs %s" % [j.split("/")[-1] for j in jobs])

    mesh = EAL.load_asset(MESH)
    skel = EAL.load_asset(SKEL)
    acl = EAL.load_asset("/ACLPlugin/ACLAnimBoneCompressionSettings")
    log("compress-related methods: %s" % [m for c in (unreal.AnimSequence, unreal.AnimationLibrary, unreal.AnimSequenceBase) for m in dir(c) if "ompress" in m or "derived" in m.lower()])
    log("ACL settings asset %s" % (acl.get_path_name() if acl else None))
    try:
        log("ACL settings codecs %s" % [c.get_class().get_name() for c in acl.get_editor_property("codecs")])
    except Exception as e:
        log("codec list err %s" % e)
    curve = EAL.load_asset("/ACLPlugin/ACLAnimCurveCompressionSettings") or EAL.load_asset("/Engine/Animation/DefaultAnimCurveCompressionSettings")
    log("curve settings %s" % curve.get_path_name())
    rig, ok = make_rig(mesh)
    if not ok:
        log("FAILED auto retarget definition")
        return
    rtg, rpath = make_rtg(rig, mesh)
    for p in (OUT + "/IK_Crew_Manny", rpath):
        log("save %s %s" % (p, EAL.save_asset(p, only_if_is_dirty=False)))

    done = []
    for src in jobs:
        name = src.split("/")[-1]
        dst = OUT + "/" + name
        srcseq = EAL.load_asset(src)
        if EAL.does_asset_exist(dst):
            log("deleting old %s %s" % (dst, EAL.delete_asset(dst)))
        tmp = OUT + "/Tmp/" + name   # full-pose working copy of an additive source (ue_c05_prep_additive.py)
        rsrc = tmp if EAL.does_asset_exist(tmp) else src
        log("%s retarget source %s" % (name, rsrc))
        ad = EAL.find_asset_data(rsrc)
        res = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
            [ad], mesh, mesh, rtg, "", "", "", "", OUT, False, False, False)
        log("retargeted %s -> %s" % (name, [str(r.package_name) for r in res]))
        if not EAL.does_asset_exist(dst):
            log("MISSING after retarget " + dst)
            continue
        seq = EAL.load_asset(dst)
        log("SRC %s len=%.3f frames=%d additive=%s rootmotion=%s codec=%s" % (
            name, srcseq.get_play_length(), unreal.AnimationLibrary.get_num_frames(srcseq),
            srcseq.get_editor_property("additive_anim_type"), srcseq.get_editor_property("enable_root_motion"),
            srcseq.get_editor_property("bone_compression_settings").get_name()))
        if seq.get_editor_property("additive_anim_type") != unreal.AdditiveAnimationType.AAT_NONE:
            seq.set_editor_property("additive_anim_type", unreal.AdditiveAnimationType.AAT_NONE)
            log("%s made non-additive" % name)
        if name in LOOPS:
            nk = unreal.AnimationLibrary.get_num_frames(seq) + 1
            poses = [unreal.AnimationLibrary.get_bone_poses_for_frame(seq, ["root"], f, False)[0] for f in range(nk)]
            maxr = max(p.translation.length() for p in poses)
            if maxr > 0.01:
                seq.controller.set_bone_track_keys("root", [unreal.Vector(0, 0, 0)] * nk,
                                                   [p.rotation for p in poses], [p.scale3d for p in poses], True)
            log("%s loop root max|t| before=%.2f, root motion off" % (name, maxr))
            seq.set_editor_property("enable_root_motion", False)
        else:
            seq.set_editor_property("enable_root_motion", srcseq.get_editor_property("enable_root_motion"))
        unreal.AnimationLibrary.set_bone_compression_settings(seq, acl)
        unreal.AnimationLibrary.set_curve_compression_settings(seq, curve)
        log("save %s %s" % (name, EAL.save_asset(dst, only_if_is_dirty=False)))
        done.append((name, dst))
    # compression: saving caches compressed data; also wait for any asset compilation
    try:
        unreal.AssetCompilingManager.get().finish_all_compilation()
    except Exception as e:
        log("finish_all_compilation n/a: %s" % e)
    for name, dst in done:
        seq = EAL.load_asset(dst)
        log("NEW %s len=%.3f frames=%d skel=%s additive=%s rootmotion=%s codec=%s" % (
            name, seq.get_play_length(), unreal.AnimationLibrary.get_num_frames(seq),
            seq.get_editor_property("skeleton").get_path_name(), seq.get_editor_property("additive_anim_type"),
            seq.get_editor_property("enable_root_motion"), unreal.AnimationLibrary.get_bone_compression_settings(seq).get_path_name()))
        report_pose(name, seq, "NEW")
    dirty("end")
    log("created %d of %d" % (len(done), len(jobs)))
    log("OK" if len(done) == len(jobs) else "FAILED")


main()

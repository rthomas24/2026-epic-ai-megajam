# EngAnim batch 1 (Nov 13): retarget template mannequin sequences onto SKEL_NullCrew (RTG_Manny_to_NullCrew) and
# SKEL_Pilot (RTG_Manny_to_Pilot) under new names. Creates ONLY new assets:
#   /Game/TriadDrift/Gameplay/Characters/Crew/Anims/A_Crew_*      /Game/TriadDrift/Gameplay/Characters/Pilot/Anims/A_Pilot_*
# Template assets are read only (additive sources are duplicated into <OUT>/TmpEA, made full-pose, retargeted from there,
# and the copies deleted). Existing assets are never overwritten: a job whose target exists is skipped.
# Run: powershell -File Tools\td-python.ps1 -Script ArtSource\EngAnim\Retarget\scripts\ue_r10_retarget.py
import json, os
import unreal

EAL = unreal.EditorAssetLibrary
AL = unreal.AnimationLibrary
A = "/Game/Characters/Mannequins/Anims/"
CH = "/Game/TriadDrift/Gameplay/Characters"
MANNY = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
TARGETS = {
    "crew": dict(out=CH + "/Crew/Anims", mesh=CH + "/NullTrooper/SK_NullTrooper", rtg=CH + "/Crew/RTG_Manny_to_NullCrew", skel="SKEL_NullCrew"),
    "pilot": dict(out=CH + "/Pilot/Anims", mesh=CH + "/Pilot/SK_Pilot", rtg=CH + "/Pilot/Anims/RTG_Manny_to_Pilot", skel="SKEL_Pilot"),
}
DIRS = ["Fwd", "Fwd_Left", "Fwd_Right", "Left", "Right", "Bwd", "Bwd_Left", "Bwd_Right"]
# (target, new name, source, mode)   mode: loop = in place + root motion off; inplace = same (one-shots: the capsule does
# the moving); keep = root travel kept (deaths: the body falls away from its feet); +add = also an additive copy <name>_Add
JOBS = []
# ---- trooper: rifle strafe set, reload / equip / dry fire, aim poses, hit variety
for d in DIRS:
    if d != "Fwd":
        JOBS.append(("crew", "A_Crew_Trooper_Walk_" + d, A + "Rifle/Walk/MF_Rifle_Walk_" + d, "loop"))
        JOBS.append(("crew", "A_Crew_Trooper_Jog_" + d, A + "Rifle/Jog/MF_Rifle_Jog_" + d, "loop"))
JOBS += [
    ("crew", "A_Crew_Trooper_Reload", A + "Rifle/MM_Rifle_Reload", "inplace"),
    ("crew", "A_Crew_Trooper_Equip", A + "Rifle/MM_Rifle_Equip", "inplace"),
    ("crew", "A_Crew_Trooper_DryFire", A + "Rifle/MM_Rifle_DryFire", "inplace"),
    ("crew", "A_Crew_Trooper_Aim_Center", A + "Rifle/AIM/MM_Rifle_Idle_ADS_AO_CC", "inplace"),
    ("crew", "A_Crew_Trooper_Aim_Down", A + "Rifle/AIM/MM_Rifle_Idle_ADS_AO_CD", "inplace"),
    ("crew", "A_Crew_Trooper_Aim_Up", A + "Rifle/AIM/MM_Rifle_Idle_ADS_AO_CU", "inplace"),
    ("crew", "A_Crew_Trooper_JumpStart", A + "Rifle/Jump/MM_Rifle_Jump_Start", "inplace"),
    ("crew", "A_Crew_Trooper_JumpApex", A + "Rifle/Jump/MM_Rifle_Jump_Apex", "inplace"),
    ("crew", "A_Crew_Trooper_FallLand", A + "Rifle/Jump/MM_Rifle_Jump_Fall_Land", "inplace"),
    ("crew", "A_Crew_Hit_Front_Lgt_02", A + "Rifle/HitReact/MM_HitReact_Front_Lgt_02", "inplace+add"),
    ("crew", "A_Crew_Hit_Front_Lgt_03", A + "Rifle/HitReact/MM_HitReact_Front_Lgt_03", "inplace+add"),
    ("crew", "A_Crew_Hit_Front_Lgt_04", A + "Rifle/HitReact/MM_HitReact_Front_Lgt_04", "inplace+add"),
    ("crew", "A_Crew_Hit_Front_Med_01", A + "Rifle/HitReact/MM_HitReact_Front_Med_01", "inplace+add"),
    ("crew", "A_Crew_Hit_Front_Med_02", A + "Rifle/HitReact/MM_HitReact_Front_Med_02", "inplace+add"),
    ("crew", "A_Crew_Hit_Front_Hvy_01", A + "Rifle/HitReact/MM_HitReact_Front_Hvy_01", "inplace+add"),
    ("crew", "A_Crew_Hit_Back_Med_01", A + "Rifle/HitReact/MM_HitReact_Back_Med_01", "inplace+add"),
    ("crew", "A_Crew_Death_Front_02", A + "Death/MM_Death_Front_02", "keep"),
    ("crew", "A_Crew_Death_Front_03", A + "Death/MM_Death_Front_03", "keep"),
    # ---- grunt: melee variety, charge lunge, unarmed strafe set
    ("crew", "A_Crew_Grunt_Attack_02", A + "Unarmed/Attack/MM_Attack_02", "inplace"),
    ("crew", "A_Crew_Grunt_Attack_03", A + "Unarmed/Attack/MM_Attack_03", "inplace"),
    ("crew", "A_Crew_Grunt_Lunge", A + "Unarmed/Jump/MM_Dash", "inplace"),
    ("crew", "A_Crew_Grunt_JumpStart", A + "Unarmed/Jump/MM_Jump", "inplace"),
    ("crew", "A_Crew_Grunt_FallLoop", A + "Unarmed/Jump/MM_Fall_Loop", "loop"),
    ("crew", "A_Crew_Grunt_Land", A + "Unarmed/Jump/MM_Land", "inplace"),
    # ---- heavy: two-hands-forward guard stance and its slow walk set (pistol set), equip as the shield raise
    ("crew", "A_Crew_Heavy_Idle_Guard", A + "Pistol/MF_Pistol_Idle_ADS", "loop"),
    ("crew", "A_Crew_Heavy_GuardRaise", A + "Pistol/MM_Pistol_Equip", "inplace"),
    ("crew", "A_Crew_Heavy_Reload", A + "Pistol/MM_Pistol_Reload", "inplace"),
]
for d in DIRS:
    if d != "Fwd":
        JOBS.append(("crew", "A_Crew_Grunt_Walk_" + d, A + "Unarmed/Walk/MF_Unarmed_Walk_" + d, "loop"))
        JOBS.append(("crew", "A_Crew_Grunt_Jog_" + d, A + "Unarmed/Jog/MF_Unarmed_Jog_" + d, "loop"))
    JOBS.append(("crew", "A_Crew_Heavy_Walk_" + d, A + "Pistol/Walk/MF_Pistol_Walk_" + d, "loop"))
# ---- pilot: full 8-way walk / jog set (for a blend space), wall jump, deaths, hit variety, interaction candidates
for d in DIRS:
    JOBS.append(("pilot", "A_Pilot_Walk_" + d, A + "Unarmed/Walk/MF_Unarmed_Walk_" + d, "loop"))
    JOBS.append(("pilot", "A_Pilot_Jog_" + d, A + "Unarmed/Jog/MF_Unarmed_Jog_" + d, "loop"))
JOBS += [
    ("pilot", "A_Pilot_Idle_Mannequin", A + "Unarmed/MM_Idle", "loop"),
    ("pilot", "A_Pilot_WallJump", A + "Unarmed/Jump/MM_WallJump", "inplace"),
    ("pilot", "A_Pilot_Death_Left", A + "Death/MM_Death_Left_01", "keep"),
    ("pilot", "A_Pilot_Death_Right", A + "Death/MM_Death_Right_01", "keep"),
    ("pilot", "A_Pilot_Death_Front_02", A + "Death/MM_Death_Front_02", "keep"),
    ("pilot", "A_Pilot_Death_Front_03", A + "Death/MM_Death_Front_03", "keep"),
    ("pilot", "A_Pilot_HitReact_Front_Lgt_01", A + "Rifle/HitReact/MM_HitReact_Front_Lgt_01", "inplace+add"),
    ("pilot", "A_Pilot_HitReact_Front_Lgt_02", A + "Rifle/HitReact/MM_HitReact_Front_Lgt_02", "inplace+add"),
    ("pilot", "A_Pilot_HitReact_Front_Lgt_03", A + "Rifle/HitReact/MM_HitReact_Front_Lgt_03", "inplace+add"),
    ("pilot", "A_Pilot_HitReact_Front_Hvy", A + "Rifle/HitReact/MM_HitReact_Front_Hvy_01", "inplace+add"),
    ("pilot", "A_Pilot_Interact_Console", A + "Pistol/MM_Pistol_Reload", "inplace"),
    ("pilot", "A_Pilot_Interact_Draw", A + "Pistol/MM_Pistol_Equip", "inplace"),
    ("pilot", "A_Pilot_Interact_Shoulder", A + "Rifle/MM_Rifle_Equip", "inplace"),
    ("pilot", "A_Pilot_Land_Hard", A + "Rifle/Jump/MM_Rifle_Jump_Fall_Land", "inplace"),
]
problems = []
report = []


def log(m):
    for line in str(m).splitlines():
        unreal.log("TDPY: " + line)


def zero_root(seq):
    nk = AL.get_num_frames(seq) + 1
    poses = [AL.get_bone_poses_for_frame(seq, ["root"], f, False)[0] for f in range(nk)]
    maxr = max(p.translation.length() for p in poses)
    if maxr > 0.01:
        seq.controller.set_bone_track_keys("root", [unreal.Vector(0, 0, 0)] * nk, [p.rotation for p in poses], [p.scale3d for p in poses], True)
    return maxr


def main():
    log("signature: %s" % unreal.IKRetargetBatchOperation.duplicate_and_retarget.__doc__)
    only = None
    jf = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "EngAnim", "retarget_only.txt")
    if os.path.isfile(jf):
        only = set(x.strip() for x in open(jf) if x.strip())
    manny = EAL.load_asset(MANNY)
    acl = EAL.load_asset("/ACLPlugin/ACLAnimBoneCompressionSettings")
    curve = EAL.load_asset("/ACLPlugin/ACLAnimCurveCompressionSettings") or EAL.load_asset("/Engine/Animation/DefaultAnimCurveCompressionSettings")
    for key, T in TARGETS.items():
        T["mesh_obj"] = EAL.load_asset(T["mesh"]); T["rtg_obj"] = EAL.load_asset(T["rtg"])
        log("%s mesh %s rtg %s" % (key, T["mesh_obj"], T["rtg_obj"]))
    for tgt, name, src, mode in JOBS:
        if only is not None and name not in only:
            continue
        T = TARGETS[tgt]
        dst = T["out"] + "/" + name
        if EAL.does_asset_exist(dst):
            log("SKIP exists %s" % dst); report.append(dict(name=name, status="exists")); continue
        srcseq = EAL.load_asset(src)
        if srcseq is None:
            problems.append("no source " + src); continue
        srcname = src.split("/")[-1]
        rsrc = src
        tmp = None
        src_add = srcseq.get_editor_property("additive_anim_type")
        if src_add != unreal.AdditiveAnimationType.AAT_NONE:
            tmp = T["out"] + "/TmpEA/" + srcname
            if EAL.does_asset_exist(tmp):
                EAL.delete_asset(tmp)
            d = EAL.duplicate_asset(src, tmp)
            d.set_editor_property("additive_anim_type", unreal.AdditiveAnimationType.AAT_NONE)
            d.set_editor_property("ref_pose_type", unreal.AdditiveBasePoseType.ABPT_NONE)
            d.set_editor_property("ref_pose_seq", None)
            EAL.save_asset(tmp, only_if_is_dirty=False)
            rsrc = tmp
        try:
            res = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
                [EAL.find_asset_data(rsrc)], manny, T["mesh_obj"], T["rtg_obj"], srcname, name, "", "", T["out"], False, False, True)
        except Exception as e:
            problems.append("retarget %s: %s" % (name, e)); continue
        if tmp:
            EAL.delete_asset(tmp)
        if not EAL.does_asset_exist(dst):
            problems.append("missing after retarget %s (got %s)" % (name, [str(r.package_name) for r in res])); continue
        seq = EAL.load_asset(dst)
        seq.set_editor_property("additive_anim_type", unreal.AdditiveAnimationType.AAT_NONE)
        travel = 0.0
        if mode.startswith("loop") or mode.startswith("inplace"):
            travel = zero_root(seq)
        seq.set_editor_property("enable_root_motion", False)
        AL.set_bone_compression_settings(seq, acl)
        AL.set_curve_compression_settings(seq, curve)
        sk = seq.get_editor_property("skeleton").get_name()
        ok = EAL.save_asset(dst, only_if_is_dirty=False)
        row = dict(name=name, target=tgt, path=dst, source=src, mode=mode, len=round(seq.get_play_length(), 4), frames=AL.get_num_frames(seq),
                   src_len=round(srcseq.get_play_length(), 4), skel=sk, root_travel_removed=round(travel, 1), src_additive=str(src_add), saved=ok, status="new")
        if sk != T["skel"]:
            problems.append("%s on skeleton %s" % (name, sk))
        if abs(seq.get_play_length() - srcseq.get_play_length()) > 0.002:
            problems.append("%s length changed" % name)
        if mode.endswith("+add"):
            adst = dst + "_Add"
            if not EAL.does_asset_exist(adst):
                s = EAL.duplicate_asset(dst, adst)
                s.set_editor_property("additive_anim_type", unreal.AdditiveAnimationType.AAT_LOCAL_SPACE_BASE)
                s.set_editor_property("ref_pose_type", unreal.AdditiveBasePoseType.ABPT_ANIM_FRAME)
                s.set_editor_property("ref_pose_seq", seq)
                s.set_editor_property("ref_frame_index", 0)
                EAL.save_asset(adst, only_if_is_dirty=False)
                row["additive_copy"] = adst
        report.append(row)
        log("NEW %s len=%.3f frames=%d skel=%s root travel removed %.0f" % (name, row["len"], row["frames"], sk, travel))
    for T in TARGETS.values():
        t = T["out"] + "/TmpEA"
        if EAL.does_directory_exist(t):
            log("delete tmp dir %s %s" % (t, EAL.delete_directory(t)))
    try:
        unreal.AssetCompilingManager.get().finish_all_compilation()
    except Exception as e:
        log("finish_all_compilation n/a %s" % e)
    outdir = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "EngAnim")
    os.makedirs(outdir, exist_ok=True)
    rp = os.path.join(outdir, "retarget_report.json")
    old = json.load(open(rp)) if os.path.isfile(rp) else []
    keep = [r for r in old if r.get("status") == "new" and r["name"] not in set(x["name"] for x in report if x["status"] == "new")]
    json.dump(keep + report, open(rp, "w"), indent=1)
    log("new %d of %d jobs" % (sum(1 for r in report if r["status"] == "new"), len(JOBS)))
    log("still dirty: %s" % [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()])
    log("FAILED: %s" % problems if problems else "OK")


main()

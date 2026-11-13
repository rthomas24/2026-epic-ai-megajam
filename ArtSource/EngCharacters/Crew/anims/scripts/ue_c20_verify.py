# Crew anims step 2: in a FRESH editor process, load each new Crew sequence (and its template source as a control),
# sample raw vs compressed poses at 3 times (component space), report head / pelvis / feet heights and the max
# raw-vs-compressed bone difference, then export each new sequence to FBX (with preview mesh) into Saved/CrewAnims.
# Saves nothing in Content.  Job subset: Saved/CrewAnims/jobs.txt.
import os
import unreal

OUT = "/Game/TriadDrift/Gameplay/Characters/Crew/Anims"
EAL = unreal.EditorAssetLibrary
SRCMAP = {
    "MM_Idle": "Unarmed/MM_Idle", "MF_Unarmed_Walk_Fwd": "Unarmed/Walk/MF_Unarmed_Walk_Fwd",
    "MF_Unarmed_Jog_Fwd": "Unarmed/Jog/MF_Unarmed_Jog_Fwd", "MF_Rifle_Idle_ADS": "Rifle/MF_Rifle_Idle_ADS",
    "MF_Rifle_Walk_Fwd": "Rifle/Walk/MF_Rifle_Walk_Fwd", "MF_Rifle_Jog_Fwd": "Rifle/Jog/MF_Rifle_Jog_Fwd",
    "MM_Attack_01": "Unarmed/Attack/MM_Attack_01", "MM_ChargedAttack": "Unarmed/Attack/MM_ChargedAttack",
    "MM_Rifle_Fire": "Rifle/MM_Rifle_Fire", "MM_HitReact_Front_Lgt_01": "Rifle/HitReact/MM_HitReact_Front_Lgt_01",
    "MM_Death_Front_01": "Death/MM_Death_Front_01", "MM_Death_Back_01": "Death/MM_Death_Back_01",
    "MM_Death_Left_01": "Death/MM_Death_Left_01", "MM_Death_Right_01": "Death/MM_Death_Right_01",
}
SAVED = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "CrewAnims").replace("\\", "/")


def log(m):
    for line in str(m).splitlines():
        unreal.log("TDPY: " + line)


def pose(seq, t, et):
    o = unreal.AnimPoseEvaluationOptions()
    o.set_editor_property("evaluation_type", getattr(unreal.AnimDataEvalType, et))
    o.set_editor_property("extract_root_motion", False)
    p = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, o)
    names = unreal.AnimPoseExtensions.get_bone_names(p)
    return {str(n): unreal.AnimPoseExtensions.get_bone_pose(p, n, unreal.AnimPoseSpaces.WORLD).translation for n in names}


def check(tag, name, seq):
    L = seq.get_play_length()
    worst = 1e9
    for t in (0.0, L * 0.5, max(0.0, L - 0.034)):
        r = pose(seq, t, "RAW")
        c = pose(seq, t, "COMPRESSED")
        diff = max((r[b] - c[b]).length() for b in r if b in c)
        feet = min(c["foot_l"].z, c["foot_r"].z)
        span = c["head"].z - feet
        spread = max((c[b] - c["root"]).length() for b in c)
        worst = min(worst, (c["head"] - c["foot_l"]).length())
        log("POSE %s %s t=%.3f COMPRESSED head.z=%.2f pelvis.z=%.2f foot_l.z=%.2f foot_r.z=%.2f head-minfoot.z=%.2f |head-foot_l|=%.2f max|bone-root|=%.2f raw-vs-compressed max=%.5f cm" % (
            tag, name, t, c["head"].z, c["pelvis"].z, c["foot_l"].z, c["foot_r"].z, span,
            (c["head"] - c["foot_l"]).length(), spread, diff))
    return worst


def export(name, seq):
    out = SAVED + "/" + name + ".fbx"
    task = unreal.AssetExportTask()
    task.set_editor_property("object", seq)
    task.set_editor_property("filename", out)
    task.set_editor_property("automated", True)
    task.set_editor_property("prompt", False)
    task.set_editor_property("replace_identical", True)
    task.set_editor_property("exporter", unreal.AnimSequenceExporterFBX())
    opt = unreal.FbxExportOption()
    for k, v in (("ascii", False), ("export_preview_mesh", True), ("force_front_x_axis", False),
                 ("level_of_detail", False), ("collision", False), ("export_morph_targets", False)):
        try:
            opt.set_editor_property(k, v)
        except Exception as e:
            log("note opt %s: %s" % (k, e))
    task.set_editor_property("options", opt)
    ok = unreal.Exporter.run_asset_export_task(task)
    log("export %s ok=%s size=%s" % (name, ok, os.path.getsize(out) if os.path.isfile(out) else 0))
    return ok


def main():
    os.makedirs(SAVED, exist_ok=True)
    jf = SAVED + "/jobs.txt"
    only = set(x.strip() for x in open(jf) if x.strip()) if os.path.isfile(jf) else None
    names = [n for n in SRCMAP if only is None or n in only]
    bad = []
    for name in names:
        seq = EAL.load_asset(OUT + "/" + name)
        if seq is None:
            log("MISSING " + name); bad.append(name); continue
        log("NEW %s len=%.3f frames=%d additive=%s rootmotion=%s codec=%s curvecodec=%s skel=%s" % (
            name, seq.get_play_length(), unreal.AnimationLibrary.get_num_frames(seq),
            seq.get_editor_property("additive_anim_type"), seq.get_editor_property("enable_root_motion"),
            unreal.AnimationLibrary.get_bone_compression_settings(seq).get_path_name(),
            unreal.AnimationLibrary.get_curve_compression_settings(seq).get_path_name(),
            seq.get_editor_property("skeleton").get_path_name()))
        w = check("NEW", name, seq)
        if w < 100:
            bad.append(name)
        src = EAL.load_asset("/Game/Characters/Mannequins/Anims/" + SRCMAP[name])
        check("SRC", name, src)
        if not export(name, seq):
            bad.append(name)
    d = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    log("dirty packages: %s" % [p.get_name() for p in d])
    log("bad %s" % bad)
    log("OK" if not bad else "FAILED")


main()

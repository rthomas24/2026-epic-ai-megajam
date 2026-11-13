# Animation audit step 1: motion metrics for every sequence in our character folders (hit-frame estimate from the
# right hand, loop-point pop, natural ground speed of the loops, additive setup). Writes Saved/EngAnim/metrics.json.
# Saves nothing in Content.  Optional: Saved/EngAnim/metrics_only.txt (one package path prefix per line).
import json, os, math
import unreal

EAL = unreal.EditorAssetLibrary
APE = unreal.AnimPoseExtensions
WORLD = unreal.AnimPoseSpaces.WORLD
ROOTS = ["/Game/TriadDrift/Gameplay/Characters/Pilot", "/Game/TriadDrift/Gameplay/Characters/Crew",
         "/Game/TriadDrift/Gameplay/Characters/NullTrooper", "/Game/TriadDrift/Gameplay/Characters/NullGrunt",
         "/Game/TriadDrift/Gameplay/Characters/NullHeavy"]
OUTDIR = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "EngAnim")
BONES = ["root", "pelvis", "hand_r", "hand_l", "foot_l", "foot_r", "head", "spine_03"]


def log(m):
    unreal.log("TDPY: " + str(m))


def v(t):
    return (t.x, t.y, t.z)


def main():
    os.makedirs(OUTDIR, exist_ok=True)
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    out = []
    opts = unreal.AnimPoseEvaluationOptions()
    try:
        opts.set_editor_property("extract_root_motion", False)
    except Exception:
        pass
    for root in ROOTS:
        for d in reg.get_assets_by_path(root, recursive=True):
            if str(d.asset_class_path.asset_name) != "AnimSequence":
                continue
            pkg = str(d.package_name)
            seq = EAL.load_asset(pkg)
            nf = unreal.AnimationLibrary.get_num_frames(seq)
            L = seq.get_play_length()
            dt = L / nf if nf else 0
            row = {"path": pkg, "len": round(L, 4), "frames": nf,
                   "additive": str(seq.get_editor_property("additive_anim_type")),
                   "ref_pose_type": str(seq.get_editor_property("ref_pose_type")),
                   "ref_pose_seq": seq.get_editor_property("ref_pose_seq").get_name() if seq.get_editor_property("ref_pose_seq") else None,
                   "rootmotion": bool(seq.get_editor_property("enable_root_motion")),
                   "codec": unreal.AnimationLibrary.get_bone_compression_settings(seq).get_name()}
            P = []
            names = None
            for f in range(nf + 1):
                pose = APE.get_anim_pose_at_frame(seq, f, opts)
                if names is None:
                    names = set(str(n) for n in APE.get_bone_names(pose))
                P.append({b: v(APE.get_bone_pose(pose, b, WORLD).translation) for b in BONES if b in names})
            row["pos"] = {b: [[round(c, 1) for c in p[b]] for p in P] for b in P[0]}
            # loop pop: all-bone world position difference first vs last key
            p0 = APE.get_anim_pose_at_frame(seq, 0, opts); pN = APE.get_anim_pose_at_frame(seq, nf, opts)
            pop = 0.0; popb = None
            for n in names:
                a = APE.get_bone_pose(p0, n, WORLD).translation; b = APE.get_bone_pose(pN, n, WORLD).translation
                dd = (a - b).length()
                if dd > pop:
                    pop, popb = dd, n
            row["loop_pop_cm"] = round(pop, 2); row["loop_pop_bone"] = popb
            out.append(row)
    json.dump(out, open(os.path.join(OUTDIR, "metrics.json"), "w"))
    log("sequences %d" % len(out))
    log("dirty packages: %s" % [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()])
    log("OK")


main()

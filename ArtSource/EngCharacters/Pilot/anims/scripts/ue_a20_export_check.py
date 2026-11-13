# Pilot anims step 2: export every A_Pilot_* sequence in Pilot/Anims to FBX (with the SK_Pilot preview mesh) into
# Saved/PilotAnims/ for the Blender contact strips, and log root / pelvis motion. Saves nothing in Content.
import os
import unreal

OUT = "/Game/TriadDrift/Gameplay/Characters/Pilot/Anims"
EAL = unreal.EditorAssetLibrary
DIR = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "PilotAnims").replace("\\", "/")


def log(m):
    unreal.log("TDPY: " + str(m))


def main():
    os.makedirs(DIR, exist_ok=True)
    names = sorted(p.split(".")[0].split("/")[-1] for p in EAL.list_assets(OUT, recursive=False)
                   if p.split("/")[-1].startswith("A_Pilot_"))
    log("sequences %s" % names)
    n_ok = 0
    for name in names:
        seq = EAL.load_asset(OUT + "/" + name)
        if not isinstance(seq, unreal.AnimSequence):
            continue
        log("SEQ %s len=%.3f frames=%d rootmotion=%s rate=%s" % (
            name, seq.get_play_length(), unreal.AnimationLibrary.get_num_frames(seq),
            seq.get_editor_property("enable_root_motion"),
            seq.get_editor_property("target_frame_rate") if hasattr(seq, "target_frame_rate") else "?"))
        out = DIR + "/" + name + ".fbx"
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
        n_ok += 1 if ok else 0
    try:
        d = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
        log("dirty packages: %s" % [p.get_name() for p in d])
    except Exception as e:
        log("dirty err %s" % e)
    log("exported %d" % n_ok)
    log("OK")


main()

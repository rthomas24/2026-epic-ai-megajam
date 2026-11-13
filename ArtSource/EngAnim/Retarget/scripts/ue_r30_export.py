# EngAnim: export every clip named in Saved/EngAnim/retarget_report.json and derive_report.json to FBX (with the
# preview mesh) into Saved/EngAnim/fbx/ for the Blender contact strips. Saves nothing in Content.
import json, os
import unreal

EAL = unreal.EditorAssetLibrary
SAVED = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir()), "EngAnim").replace("\\", "/")
DIR = SAVED + "/fbx"


def log(m):
    unreal.log("TDPY: " + str(m))


def main():
    os.makedirs(DIR, exist_ok=True)
    rows = []
    for fn in ("retarget_report.json", "derive_report.json"):
        p = os.path.join(SAVED, fn)
        if os.path.isfile(p):
            rows += [r for r in json.load(open(p)) if r.get("status") == "new"]
    extra = os.path.join(SAVED, "export_extra.txt")
    if os.path.isfile(extra):
        rows += [dict(name=x.strip().split("/")[-1], path=x.strip()) for x in open(extra) if x.strip()]
    n_ok = 0
    for r in rows:
        out = DIR + "/" + r["name"] + ".fbx"
        if os.path.isfile(out):
            n_ok += 1; continue
        seq = EAL.load_asset(r["path"])
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
                pass
        task.set_editor_property("options", opt)
        ok = unreal.Exporter.run_asset_export_task(task)
        n_ok += 1 if ok else 0
        if not ok:
            log("export failed %s" % r["name"])
    log("exported %d of %d" % (n_ok, len(rows)))
    log("dirty packages: %s" % [p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()])
    log("OK")


main()

# Round-trip test: import work/RT_Manny.fbx (Manny unchanged, re-exported from Blender) on the EXISTING
# mannequin skeleton into a scratch folder, export it back to Saved/, then delete the scratch asset.
import os
import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
REPO = os.path.abspath(os.path.join(ROOT, "..", "..", ".."))
SRC = os.path.join(ROOT, "work", "RT_Manny.fbx").replace("\\", "/")
DEST = "/Game/TriadDrift/Gameplay/Characters/NullTrooper/_RT"
SKELETON = "/Game/Characters/Mannequins/Meshes/SK_Mannequin"
OUT = os.path.join(REPO, "Saved", "NullTrooper", "RT_back.fbx").replace("\\", "/")


def log(m):
    unreal.log("TDPY: " + str(m))


def main():
    eal = unreal.EditorAssetLibrary
    skel = eal.load_asset(SKELETON)
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    ui.set_editor_property("skeleton", skel)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("import_animations", False)
    ui.set_editor_property("create_physics_asset", False)
    d = ui.get_editor_property("skeletal_mesh_import_data")
    d.set_editor_property("import_morph_targets", False)
    d.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
    d.set_editor_property("use_t0_as_ref_pose", False)
    d.set_editor_property("preserve_smoothing_groups", True)
    d.set_editor_property("convert_scene", True)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", SRC)
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("destination_name", "SK_RT")
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    task.set_editor_property("options", ui)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    log("imported paths: %s" % list(task.get_editor_property("imported_object_paths")))
    log("assets in dest: %s" % list(eal.list_assets(DEST)))
    mesh = eal.load_asset(DEST + "/SK_RT")
    if mesh is None:
        log("FAILED: no mesh")
        return
    sk = mesh.get_editor_property("skeleton")
    log("skeleton: %s" % (sk.get_path_name() if sk else None))
    log("skeleton package dirty: %s" % unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())
    b = mesh.get_bounds()
    log("bounds origin %s extent %s" % (b.origin, b.box_extent))
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    t = unreal.AssetExportTask()
    t.set_editor_property("object", mesh); t.set_editor_property("filename", OUT)
    t.set_editor_property("automated", True); t.set_editor_property("prompt", False)
    t.set_editor_property("replace_identical", True)
    t.set_editor_property("exporter", unreal.SkeletalMeshExporterFBX())
    opt = unreal.FbxExportOption(); opt.set_editor_property("level_of_detail", False); opt.set_editor_property("collision", False)
    t.set_editor_property("options", opt)
    ok = unreal.Exporter.run_asset_export_task(t)
    log("re-export ok=%s" % ok)
    # scratch asset was never saved; make sure nothing is left on disk
    eal.delete_directory(DEST)
    if ok and sk and sk.get_path_name().startswith(SKELETON):
        log("OK")
    else:
        log("FAILED")


main()

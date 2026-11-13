# Null Trooper step A: export the template mannequin (reference body + skeleton) to FBX.
# Run: powershell -File Tools\td-python.ps1 -Script ArtSource\EngCharacters\NullTrooper\scripts\ue_export_manny.py
import os
import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(os.path.dirname(HERE), "source", "SKM_Manny_Simple.fbx").replace("\\", "/")
MESH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"


def log(m):
    unreal.log("TDPY: " + str(m))


def main():
    mesh = unreal.EditorAssetLibrary.load_asset(MESH)
    if mesh is None:
        log("FAILED: cannot load " + MESH)
        return
    skel = mesh.get_editor_property("skeleton")
    log("mesh class %s skeleton %s" % (mesh.get_class().get_name(), skel.get_path_name() if skel else None))
    try:
        b = mesh.get_bounds()
        log("bounds origin %s extent %s" % (b.origin, b.box_extent))
    except Exception as e:
        log("note bounds: %s" % e)
    try:
        mats = mesh.get_editor_property("materials")
        log("materials: %s" % [str(m.get_editor_property("material_slot_name")) for m in mats])
    except Exception as e:
        log("note mats: %s" % e)
    pa = mesh.get_editor_property("physics_asset")
    log("physics asset %s" % (pa.get_path_name() if pa else None))

    task = unreal.AssetExportTask()
    task.set_editor_property("object", mesh)
    task.set_editor_property("filename", OUT)
    task.set_editor_property("automated", True)
    task.set_editor_property("prompt", False)
    task.set_editor_property("replace_identical", True)
    task.set_editor_property("exporter", unreal.SkeletalMeshExporterFBX())
    opt = unreal.FbxExportOption()
    for k, v in (("ascii", False), ("level_of_detail", False), ("collision", False),
                 ("vertex_color", False), ("export_morph_targets", False),
                 ("export_preview_mesh", False), ("force_front_x_axis", False)):
        try:
            opt.set_editor_property(k, v)
        except Exception as e:
            log("note opt %s: %s" % (k, e))
    task.set_editor_property("options", opt)
    ok = unreal.Exporter.run_asset_export_task(task)
    log("export ok=%s exists=%s size=%s" % (ok, os.path.isfile(OUT), os.path.getsize(OUT) if os.path.isfile(OUT) else 0))
    if ok and os.path.isfile(OUT):
        log("OK")
    else:
        log("FAILED: export")


main()

# SPDX-License-Identifier: MIT
# Triad Drift - asset intake: import Nicholas's three Fab props (prepared by prep_fab.py) into Unreal, headless:
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Fab\scripts\ue_import_fab.py
#
# Licences recorded in CREDITS.md (CC BY 4.0 for the crates and the core, Fab Standard for the barrel; issues #175-#177).
# The textures are the approved recolours (recolour_fab.py); the seller's colours stay in export/*_orig.png.
#
# Follows ArtSource/EngImport/scripts/ue_import_engai.py: FBX through Interchange with the pipeline
# Gameplay/Ships/Pipelines/PL_Ship_StaticMesh (UCX_ collision by name, no lightmap UVs, no materials, Nanite),
# T_Fab_<Asset>_BC|N|ORM|E at 2K (N DirectX, no green flip; ORM linear Masks), one MI_Fab_<Asset> on M_TD_PropTextured.
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
FAB = os.path.dirname(HERE)
DEST_ROOT = "/Game/TriadDrift/Gameplay/Props/Fab"
PIPELINE = "/Game/TriadDrift/Gameplay/Ships/Pipelines/PL_Ship_StaticMesh"
MASTER = "/Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured"
ASSETS = {"CrateStack": 0.0, "PowerCore": 4.0, "OilBarrel": 0.0}       # asset -> EmissiveStrength

eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
FAILS = []


def log(msg):
    unreal.log("TDPY: " + msg)


def save(asset):
    if not eas.save_loaded_asset(asset, False):
        FAILS.append(asset.get_path_name())


def import_one(name, emissive):
    notes = []
    src = os.path.join(FAB, name, "export")
    dest = "%s/%s" % (DEST_ROOT, name)
    if not eas.does_directory_exist(dest):
        eas.make_directory(dest)
    tex = {}
    tasks = []
    for suffix in ("BC", "N", "ORM", "E"):
        p = os.path.join(src, "T_Fab_%s_%s.png" % (name, suffix))
        if os.path.isfile(p):
            t = unreal.AssetImportTask()
            t.set_editor_property("filename", p)
            t.set_editor_property("destination_path", dest)
            t.set_editor_property("automated", True)
            t.set_editor_property("replace_existing", True)
            t.set_editor_property("save", False)
            tasks.append((suffix, t))
    tools.import_asset_tasks([t for _, t in tasks])
    tcs = unreal.TextureCompressionSettings
    for suffix, _ in tasks:
        t = eas.load_asset("%s/T_Fab_%s_%s" % (dest, name, suffix))
        if t is None:
            notes.append("texture %s missing" % suffix)
            continue
        if suffix == "N":
            t.set_editor_property("compression_settings", tcs.TC_NORMALMAP)
            t.set_editor_property("srgb", False)
            t.set_editor_property("flip_green_channel", False)
        elif suffix == "ORM":
            t.set_editor_property("compression_settings", tcs.TC_MASKS)
            t.set_editor_property("srgb", False)
        else:
            t.set_editor_property("compression_settings", tcs.TC_DEFAULT)
            t.set_editor_property("srgb", True)
        t.set_editor_property("max_texture_size", 2048)
        save(t)
        tex[suffix] = t
    params = unreal.ImportAssetParameters()
    params.is_automated = True
    params.replace_existing = True
    params.override_pipelines.append(unreal.SoftObjectPath("%s.%s" % (PIPELINE, PIPELINE.rsplit("/", 1)[-1])))
    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    mgr.import_asset(dest, unreal.InterchangeManager.create_source_data(os.path.join(src, "SM_Fab_%s.fbx" % name)), params)
    mesh = eas.load_asset("%s/SM_Fab_%s" % (dest, name))
    if mesh is None:
        log("FAILED %s: mesh not imported" % name)
        return None
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    sme.set_generate_lightmap_uv(mesh, False)
    ns = mesh.get_editor_property("nanite_settings")
    ns.enabled = True
    mesh.set_editor_property("nanite_settings", ns)
    hulls = sme.get_convex_collision_count(mesh)
    mi_path = "%s/MI_Fab_%s" % (dest, name)
    mi = eas.load_asset(mi_path) if eas.does_asset_exist(mi_path) else tools.create_asset(
        "MI_Fab_" + name, dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, eas.load_asset(MASTER))
    for pname, suffix in (("BaseColorTex", "BC"), ("ORMTex", "ORM"), ("NormalTex", "N"), ("EmissiveTex", "E")):
        if suffix in tex:
            mel.set_material_instance_texture_parameter_value(mi, pname, tex[suffix])
    mel.set_material_instance_scalar_parameter_value(mi, "EmissiveStrength", emissive if "E" in tex else 0.0)
    save(mi)
    for i in range(len(mesh.static_materials)):
        mesh.set_material(i, mi)
    save(mesh)
    b = mesh.get_bounding_box()
    row = {"mesh": mesh.get_path_name(), "tris": int(mesh.get_num_triangles(0)), "convex_hulls": hulls,
           "bounds_min_cm": [round(b.min.x, 1), round(b.min.y, 1), round(b.min.z, 1)],
           "bounds_max_cm": [round(b.max.x, 1), round(b.max.y, 1), round(b.max.z, 1)],
           "slots": [str(m.get_editor_property("material_slot_name")) for m in mesh.static_materials],
           "mi": mi.get_path_name(), "textures": {k: v.get_path_name() for k, v in tex.items()},
           "nanite": mesh.get_editor_property("nanite_settings").enabled, "notes": notes}
    log("%s %s" % (name, json.dumps(row)))
    return row


def main():
    if not eas.does_asset_exist(PIPELINE) or not eas.does_asset_exist(MASTER):
        log("FAILED: pipeline or master missing")
        return
    rows = [import_one(n, e) for n, e in ASSETS.items()]
    with open(os.path.join(FAB, "ue_import_report.json"), "w", encoding="utf-8") as fh:
        json.dump(rows, fh, indent=1)
    if all(rows) and not FAILS:
        log("OK")
    else:
        log("FAILED: %s" % FAILS)


main()

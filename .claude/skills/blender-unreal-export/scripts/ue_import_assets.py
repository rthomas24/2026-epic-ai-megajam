# SPDX-License-Identifier: MIT
"""
ue_import_assets.py - import Blender exports (SM_*.fbx + T_*.png + *.manifest.json) into Unreal 5.7+.

STATUS: UNTESTED. It was written from the UE 5.7 Python API docs without a running editor. Import the
calibration arrow first (export_for_unreal.py --calibration), check every result line, and if Unreal
rejects a property name, look it up with dir(...) / help(...) and fix this file.

For every *.manifest.json in the export folder:
  1. Textures T_<Asset>_*.png -> BC/E sRGB; N = Normalmap compression, NO green flip (already
     DirectX); ORM = Masks compression, sRGB off.
  2. SM_<Asset>.fbx via Interchange with a saved pipeline (UCX_ collision, no lightmap UVs, no
     sub-folders); falls back to the legacy FBX importer if Interchange fails.
  3. Post-fix: Nanite per manifest; lightmap UV generation off.
  4. Materials, only if the master materials exist:
       baked asset (T_ files)  -> MI_<Asset> from M_TD_Textured (BaseColorTex, ORMTex, NormalTex,
                                  EmissiveTex, EmissiveStrength)
       flat slots              -> MI_<Asset>_<Slot> from M_TD_Flat (BaseColor, Metallic, Roughness,
                                  EmissiveColor, EmissiveStrength), assigned by slot name.

Run inside the Unreal Editor:
  Output Log, Python:  py "C:/path/ue_import_assets.py" "C:/path/to/export" "/Game/TriadDrift/Art/Ships"
  or through an Unreal MCP Python tool (e.g. flopperam `python_execution`):
      exec(open(r"C:/path/ue_import_assets.py").read()); main(r"C:/path/to/export", "/Game/TriadDrift/...")
"""

import glob
import json
import os
import sys

import unreal

MASTER_FLAT = "/Game/TriadDrift/Art/Materials/M_TD_Flat"
MASTER_TEXTURED = "/Game/TriadDrift/Art/Materials/M_TD_Textured"
PIPELINE = "/Game/TriadDrift/Art/Pipelines/PL_TD_StaticMesh"

eas = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def _set(obj, prop, value, report):
    try:
        obj.set_editor_property(prop, value)
    except Exception as ex:  # property names drift between UE versions: report, don't crash
        report.append("could not set {}.{}: {}".format(type(obj).__name__, prop, ex))


def _task(filename, dest):
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", filename)
    t.set_editor_property("destination_path", dest)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    return t


def import_textures(paths, dest, report):
    if not paths:
        return {}
    asset_tools.import_asset_tasks([_task(p, dest) for p in paths])
    out = {}
    for p in paths:
        name = os.path.splitext(os.path.basename(p))[0]
        tex = eas.load_asset("{}/{}".format(dest, name))
        if tex is None:
            report.append("texture not imported: " + p)
            continue
        suffix = name.rsplit("_", 1)[-1]
        if suffix == "N":
            _set(tex, "compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP, report)
            _set(tex, "srgb", False, report)
            _set(tex, "flip_green_channel", False, report)
        elif suffix == "ORM":
            _set(tex, "compression_settings", unreal.TextureCompressionSettings.TC_MASKS, report)
            _set(tex, "srgb", False, report)
        else:
            _set(tex, "srgb", True, report)
        eas.save_loaded_asset(tex)
        out[suffix] = tex
    return out


def ensure_pipeline(report):
    if not eas.does_asset_exist(PIPELINE):
        eas.duplicate_asset("/Interchange/Pipelines/DefaultAssetsPipeline", PIPELINE)
    pipe = eas.load_asset(PIPELINE)
    _set(pipe, "asset_type_sub_folders", False, report)
    mesh = pipe.get_editor_property("mesh_pipeline")
    for k, v in {"import_static_meshes": True, "import_skeletal_meshes": False, "combine_static_meshes": True,
                 "collision": True, "import_collision_according_to_mesh_name": True,
                 "one_convex_hull_per_ucx": True, "generate_lightmap_u_vs": False, "build_nanite": False}.items():
        _set(mesh, k, v, report)
    eas.save_loaded_asset(pipe)
    return unreal.SoftObjectPath("{}.{}".format(PIPELINE, PIPELINE.rsplit("/", 1)[-1]))


def import_mesh(fbx, dest, report):
    try:
        params = unreal.ImportAssetParameters()
        params.is_automated = True
        params.replace_existing = True
        params.override_pipelines.append(ensure_pipeline(report))
        mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
        mgr.import_asset(dest, unreal.InterchangeManager.create_source_data(fbx), params)
        return "interchange"
    except Exception as ex:
        report.append("Interchange import failed ({}); trying the legacy FBX importer".format(ex))
    task = _task(fbx, dest)
    ui = unreal.FbxImportUI()
    _set(ui, "import_mesh", True, report)
    _set(ui, "import_as_skeletal", False, report)
    _set(ui, "import_materials", True, report)
    _set(ui, "import_textures", False, report)
    smd = ui.get_editor_property("static_mesh_import_data")
    for k, v in {"combine_meshes": True, "auto_generate_collision": False, "one_convex_hull_per_ucx": True,
                 "generate_lightmap_u_vs": False, "convert_scene": True}.items():
        _set(smd, k, v, report)
    _set(smd, "normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS, report)
    task.set_editor_property("options", ui)
    asset_tools.import_asset_tasks([task])
    return "legacy"


def _mi(name, dest, parent, vectors=None, scalars=None, textures=None):
    path = "{}/{}".format(dest, name)
    mi = eas.load_asset(path) if eas.does_asset_exist(path) else asset_tools.create_asset(
        name, dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    for k, v in (vectors or {}).items():
        mel.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
    for k, v in (scalars or {}).items():
        mel.set_material_instance_scalar_parameter_value(mi, k, float(v))
    for k, v in (textures or {}).items():
        mel.set_material_instance_texture_parameter_value(mi, k, v)
    eas.save_loaded_asset(mi)
    return mi


def apply_materials(sm, manifest, textures, dest, report):
    asset = manifest["asset"]
    short = asset[3:] if asset.startswith("SM_") else asset
    if textures and eas.does_asset_exist(MASTER_TEXTURED):
        tex_params = {k: textures[s] for k, s in (("BaseColorTex", "BC"), ("ORMTex", "ORM"), ("NormalTex", "N"),
                                                   ("EmissiveTex", "E")) if s in textures}
        strength = max([p.get("emission_strength") or 0.0 for p in manifest.get("material_params", [])] or [0.0])
        mi = _mi("MI_" + short, dest, eas.load_asset(MASTER_TEXTURED),
                 scalars={"EmissiveStrength": strength}, textures=tex_params)
        for i in range(len(sm.static_materials)):
            sm.set_material(i, mi)
        return ["MI_" + short]
    if not eas.does_asset_exist(MASTER_FLAT):
        report.append("No {} / {} yet - kept the imported materials".format(MASTER_FLAT, MASTER_TEXTURED))
        return []
    made = []
    for p in manifest.get("material_params", []):
        if p.get("textured"):
            report.append("{} uses node-driven inputs: bake it (blender-texturing) for a faithful look".format(p["name"]))
            continue
        slot = p["name"]
        mi_name = "MI_" + slot[2:] if slot.startswith("M_") else "MI_" + slot
        mi = _mi(mi_name, dest, eas.load_asset(MASTER_FLAT),
                 vectors={"BaseColor": p["base_color"], "EmissiveColor": p["emission_color"]},
                 scalars={"Metallic": p["metallic"], "Roughness": p["roughness"],
                          "EmissiveStrength": p["emission_strength"]})
        idx = sm.get_material_index(slot)
        if idx >= 0:
            sm.set_material(idx, mi)
            made.append(mi_name)
        else:
            report.append("slot {} not found on {}".format(slot, asset))
    return made


def main(export_dir, dest="/Game/TriadDrift/Art/Imported"):
    results = []
    for man_path in sorted(glob.glob(os.path.join(export_dir, "*.manifest.json"))):
        with open(man_path, encoding="utf-8") as fh:
            manifest = json.load(fh)
        report = []
        asset = manifest["asset"]
        short = asset[3:] if asset.startswith("SM_") else asset
        tex_files = sorted(glob.glob(os.path.join(export_dir, "T_{}_*.png".format(short))))
        textures = import_textures(tex_files, dest, report)
        how = import_mesh(manifest["fbx"], dest, report)
        sm = eas.load_asset("{}/{}".format(dest, asset))
        if sm is None:
            results.append({"asset": asset, "status": "error", "report": report + ["mesh not found after import"]})
            continue
        sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
        try:
            sme.set_generate_lightmap_uv(sm, False)
        except Exception as ex:
            report.append("lightmap toggle: {}".format(ex))
        if manifest.get("nanite_recommended"):
            try:
                ns = sm.get_editor_property("nanite_settings")
                ns.enabled = True
                sm.set_editor_property("nanite_settings", ns)
            except Exception as ex:
                report.append("nanite: {}".format(ex))
        mats = apply_materials(sm, manifest, textures, dest, report)
        eas.save_loaded_asset(sm)
        results.append({"asset": asset, "status": "ok", "importer": how, "textures": sorted(textures),
                        "material_instances": mats, "report": report})
    unreal.log(json.dumps(results, indent=2))
    return results


if __name__ == "__main__" and len(sys.argv) >= 2:
    main(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else "/Game/TriadDrift/Art/Imported")

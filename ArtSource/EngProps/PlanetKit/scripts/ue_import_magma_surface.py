# SPDX-License-Identifier: MIT
# Triad Drift - import SM_Cind_MagmaSurface (build_magma_surface.py) and build its own master M_TD_Magma.
# Needs a renderer (the master is rebuilt in place and compiled; editing a master with instances crashes -nullrhi):
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngProps\PlanetKit\scripts\ue_import_magma_surface.py
#
# Writes ONLY these (all new, under /Game/TriadDrift/Gameplay/Props/Planet):
#   SM_Cind_MagmaSurface             unit slab 100 x 100 x 10 cm, bottom-centre pivot, NO Nanite (flat, 222 tris),
#                                    no collision (ATDMagmaVolume's box owns damage), one slot
#   MI_Cind_MagmaSurface             instance of M_TD_Magma
#   Materials/M_TD_Magma             world-aligned, panning lava master (magma_material.py)
#   Textures/T_Cind_MagmaSurface_M   masks (linear), Textures/T_Cind_MagmaSurface_N (DirectX normal)
# Reads (never saves) /Game/TriadDrift/Gameplay/Props/Planets/T_Ground_MacroNoise and the PlanetKit Interchange
# pipeline PL_PK_StaticMesh (used as it is; Nanite is switched off on the mesh after import).
import json
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
REPO = os.path.dirname(os.path.dirname(os.path.dirname(KIT)))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "ArtSource", "EngProps", "Planets", "scripts"))
import ue_mat_builder  # noqa: E402
import magma_material as mm  # noqa: E402

ASSET = "SM_Cind_MagmaSurface"
SHORT = ASSET[3:]
SRC = os.path.join(KIT, ASSET)
DEST = "/Game/TriadDrift/Gameplay/Props/Planet"
TEX_DEST = DEST + "/Textures"
MASTER = DEST + "/Materials/M_TD_Magma"
MI = DEST + "/MI_" + SHORT
PIPELINE = DEST + "/Pipelines/PL_PK_StaticMesh"
NOISE = "/Game/TriadDrift/Gameplay/Props/Planets/T_Ground_MacroNoise"

eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
problems = []


def log(msg):
    unreal.log("TDPY: " + str(msg))


def import_texture(suffix):
    name = "T_%s_%s" % (SHORT, suffix)
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.join(SRC, name + ".png").replace("\\", "/"))
    t.set_editor_property("destination_path", TEX_DEST)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", False)
    tools.import_asset_tasks([t])
    tex = eas.load_asset("%s/%s" % (TEX_DEST, name))
    if tex is None:
        problems.append("texture not imported: " + name)
        return None
    if suffix == "N":
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("srgb", False)
        tex.set_editor_property("flip_green_channel", False)       # authored DirectX
    else:
        # BC7, not Masks (BC1: 5-6-5 bits): the channel edge is a threshold of R near 0 and stair-steps at 5 bits
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_BC7)
        tex.set_editor_property("srgb", False)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    eas.save_loaded_asset(tex)
    return tex


def import_mesh():
    fbx = os.path.join(SRC, ASSET + ".fbx")
    params = unreal.ImportAssetParameters()
    params.is_automated = True
    params.replace_existing = True
    if eas.does_asset_exist(PIPELINE):
        params.override_pipelines.append(unreal.SoftObjectPath("%s.%s" % (PIPELINE, PIPELINE.rsplit("/", 1)[-1])))
    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    mgr.import_asset(DEST, unreal.InterchangeManager.create_source_data(fbx), params)
    mesh = eas.load_asset("%s/%s" % (DEST, ASSET))
    if mesh is None:
        problems.append("mesh not found after import")
        return None
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    try:
        sme.set_generate_lightmap_uv(mesh, False)
    except Exception as error:
        problems.append("lightmap toggle: %s" % error)
    ns = mesh.get_editor_property("nanite_settings")
    ns.enabled = False
    mesh.set_editor_property("nanite_settings", ns)
    try:
        sme.remove_collisions(mesh)
    except Exception as error:
        problems.append("remove_collisions: %s" % error)
    bs = mesh.get_editor_property("body_setup")
    if bs is not None:
        bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
    return mesh


def build_master(t):
    folder, name = MASTER.rsplit("/", 1)
    if not eas.does_directory_exist(folder):
        eas.make_directory(folder)
    mat = eas.load_asset(MASTER) if eas.does_asset_exist(MASTER) else tools.create_asset(
        name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)
    try:
        mat.set_editor_property("used_with_nanite", False)
    except Exception as error:
        log("note: used_with_nanite: %s" % error)
    mm.build_magma_graph(ue_mat_builder.MB(mat, problems), t)
    mel.recompile_material(mat)
    mel.layout_material_expressions(mat)
    eas.save_asset(MASTER, only_if_is_dirty=False)
    return mat


def main():
    for d in (DEST, TEX_DEST):
        if not eas.does_directory_exist(d):
            eas.make_directory(d)
    t = {"T_Cind_MagmaSurface_M": import_texture("M"), "T_Cind_MagmaSurface_N": import_texture("N"),
         "T_Ground_MacroNoise": eas.load_asset(NOISE)}
    if not all(t.values()):
        log("FAILED: textures missing %s" % [k for k, v in t.items() if not v])
        return
    mesh = import_mesh()
    if mesh is None:
        log("FAILED: " + "; ".join(problems))
        return
    mat = build_master(t)
    mi = eas.load_asset(MI) if eas.does_asset_exist(MI) else tools.create_asset(
        "MI_" + SHORT, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, mat)
    for k, v in mm.DEFAULTS.items():
        mel.set_material_instance_scalar_parameter_value(mi, k, float(v))
    for k, v in mm.VECTORS.items():
        mel.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
    eas.save_loaded_asset(mi)
    for i in range(len(mesh.static_materials)):
        mesh.set_material(i, mi)
    eas.save_loaded_asset(mesh)
    st = mel.get_statistics(mi)
    bb = mesh.get_bounding_box()
    row = {"asset": ASSET, "size_cm": [round(bb.max.x - bb.min.x, 2), round(bb.max.y - bb.min.y, 2), round(bb.max.z - bb.min.z, 2)],
           "min_cm": [round(bb.min.x, 2), round(bb.min.y, 2), round(bb.min.z, 2)],
           "nanite": bool(mesh.get_editor_property("nanite_settings").enabled),
           "material_slots": len(mesh.static_materials), "tris_lod0": int(mesh.get_num_triangles(0)),
           "pixel_instructions": st.get_editor_property("num_pixel_shader_instructions"),
           "vertex_instructions": st.get_editor_property("num_vertex_shader_instructions"),
           "samplers": st.get_editor_property("num_samplers"), "problems": problems}
    out = os.path.join(REPO, "Saved", "MagmaSurface", "import_report.json")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, "w", encoding="utf-8") as fh:
        json.dump(row, fh, indent=2)
    log(json.dumps(row))
    log("OK" if not problems else "FAILED: " + "; ".join(problems))


main()

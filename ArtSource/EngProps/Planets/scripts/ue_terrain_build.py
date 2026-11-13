# SPDX-License-Identifier: MIT
# Create (or rebuild in place) the ENG-34 terrain materials in /Game/TriadDrift/Gameplay/Props/Planets:
#   T_Terrain_{Rock,Detail}_{M,N}  (imported from ../textures, made by build_terrain_textures.py)
#   M_TD_Terrain                   (graph: ue_terrain_material.py)
#   MI_Terrain_Verdance / MI_Terrain_Cinder
# Touches only those seven assets; everything else is loaded read-only. Needs a renderer (materials compile):
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngProps\Planets\scripts\ue_terrain_build.py
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ue_mat_builder  # noqa: E402
import ue_terrain_material as tm  # noqa: E402

TEX = os.path.join(os.path.dirname(HERE), "textures")
DEST = "/Game/TriadDrift/Gameplay/Props/Planets"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
problems = []


def log(m):
    unreal.log("TDPY: " + str(m))


def import_texture(name, normal):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(TEX, name + ".png").replace("\\", "/"))
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    tools.import_asset_tasks([task])
    tex = eal.load_asset("%s/%s" % (DEST, name))
    if tex is None:
        problems.append("import failed: " + name)
        return None
    tex.set_editor_property("srgb", False)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP if normal
                            else unreal.TextureCompressionSettings.TC_MASKS)
    eal.save_loaded_asset(tex)
    return tex


def stats(asset, label):
    st = mel.get_statistics(asset)
    vals = []
    for prop in ("num_pixel_shader_instructions", "num_vertex_shader_instructions", "num_samplers", "num_pixel_texture_samples"):
        try:
            vals.append("%s %s" % (prop[4:], st.get_editor_property(prop)))
        except Exception:
            pass
    log("stats %s: %s" % (label, ", ".join(vals)))


def main():
    t = {}
    for name in ("T_Terrain_Rock_M", "T_Terrain_Rock_N", "T_Terrain_Detail_M", "T_Terrain_Detail_N"):
        t[name] = import_texture(name, name.endswith("_N"))
    for name in ("T_Ground_MacroNoise",) + tuple("T_Ground_%s_%s" % (g, s) for g in ("Verdance", "Cinder") for s in ("BC", "N", "ORM", "E")):
        t[name] = eal.load_asset("%s/%s" % (DEST, name))
        if t[name] is None:
            problems.append("missing " + name)
    if problems:
        for p in problems:
            log("PROBLEM: " + p)
        log("FAILED (%d problems)" % len(problems))
        return
    path = DEST + "/M_TD_Terrain"
    mat = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset("M_TD_Terrain", DEST, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)
    mat.set_editor_property("tangent_space_normal", False)
    tm.build_terrain_graph(ue_mat_builder.MB(mat, problems), t)
    mel.recompile_material(mat)
    mel.layout_material_expressions(mat)
    eal.save_asset(path, only_if_is_dirty=False)
    stats(mat, "M_TD_Terrain")
    for g in ("Verdance", "Cinder"):
        ipath = "%s/MI_Terrain_%s" % (DEST, g)
        inst = eal.load_asset(ipath) if eal.does_asset_exist(ipath) else tools.create_asset(
            "MI_Terrain_" + g, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.clear_all_material_instance_parameters(inst)   # drop overrides of parameters an older graph had
        mel.set_material_instance_parent(inst, mat)
        scalars, vectors = tm.instance_values(g)
        for k, v in scalars.items():
            if not mel.set_material_instance_scalar_parameter_value(inst, k, float(v)):
                problems.append("%s: scalar %s not set" % (g, k))
        for k, v in vectors.items():
            if not mel.set_material_instance_vector_parameter_value(inst, k, unreal.LinearColor(v[0], v[1], v[2], 1.0)):
                problems.append("%s: vector %s not set" % (g, k))
        for pname, suffix in (("BaseColorTex", "BC"), ("NormalTex", "N"), ("ORMTex", "ORM"), ("EmissiveTex", "E")):
            if not mel.set_material_instance_texture_parameter_value(inst, pname, t["T_Ground_%s_%s" % (g, suffix)]):
                problems.append("%s: texture %s not set" % (g, pname))
        mel.update_material_instance(inst)
        eal.save_asset(ipath, only_if_is_dirty=False)
        stats(inst, "MI_Terrain_" + g)
    for p in problems:
        log("PROBLEM: " + p)
    log("FAILED (%d problems)" % len(problems) if problems else "OK")


main()

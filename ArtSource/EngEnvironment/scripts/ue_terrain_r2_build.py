# SPDX-License-Identifier: MIT
# Build (or rebuild in place) the environment pass's terrain materials in /Game/TriadDrift/Gameplay/Props/Planets:
#   T_Terr_<set>_{BC,N}   1K, from Saved/EnvTerrainTex (made by prep_terrain_textures.py from ArtSource/EngAI/Terrain)
#   M_TD_TerrainR2        graph: ue_terrain_r2_graph.py
#   MI_Terrain_Verdance / MI_Terrain_Cinder   re-parented to M_TD_TerrainR2 (r1's master M_TD_Terrain is not touched)
# TD_TERRAIN_R2_MODE=reimport also re-imports the textures; =instances rebuilds only the two instances (fast palette iteration); =revert puts the instances
# back on M_TD_Terrain with r1's values (ArtSource/EngProps/Planets/scripts/ue_terrain_material.py).
#   python ArtSource/EngEnvironment/scripts/prep_terrain_textures.py
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngEnvironment\scripts\ue_terrain_r2_build.py
import json
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "ArtSource", "EngProps", "Planets", "scripts"))
import ue_mat_builder  # noqa: E402
import ue_terrain_r2_graph as tg  # noqa: E402

TEX = os.path.join(REPO, "Saved", "EnvTerrainTex")
DEST = "/Game/TriadDrift/Gameplay/Props/Planets"
MODE = os.environ.get("TD_TERRAIN_R2_MODE", "all")
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
problems = []


def log(m):
    unreal.log("TDPY: " + str(m))


def import_texture(name, normal):
    path = "%s/%s" % (DEST, name)
    if MODE != "reimport" and eal.does_asset_exist(path):
        return eal.load_asset(path)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(TEX, name + ".png").replace("\\", "/"))
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    tools.import_asset_tasks([task])
    tex = eal.load_asset(path)
    if tex is None:
        problems.append("import failed: " + name)
        return None
    tex.set_editor_property("srgb", not normal)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP if normal
                            else unreal.TextureCompressionSettings.TC_DEFAULT)
    eal.save_loaded_asset(tex)
    return tex


def set_instance(g, mat, t, means):
    ipath = "%s/MI_Terrain_%s" % (DEST, g)
    inst = eal.load_asset(ipath)
    mel.clear_all_material_instance_parameters(inst)
    mel.set_material_instance_parent(inst, mat)
    scalars, vectors = tg.instance_values(g, means)
    for k, v in scalars.items():
        if not mel.set_material_instance_scalar_parameter_value(inst, k, float(v)):
            problems.append("%s: scalar %s not set" % (g, k))
    for k, v in vectors.items():
        if not mel.set_material_instance_vector_parameter_value(inst, k, unreal.LinearColor(v[0], v[1], v[2], 1.0)):
            problems.append("%s: vector %s not set" % (g, k))
    i = 0 if g == "Verdance" else 1
    for slot, sets in tg.LAYERS.items():
        for suffix in ("BC", "N"):
            if not mel.set_material_instance_texture_parameter_value(inst, slot + suffix, t["T_Terr_%s_%s" % (sets[i], suffix)]):
                problems.append("%s: texture %s%s not set" % (g, slot, suffix))
    mel.update_material_instance(inst)
    eal.save_asset(ipath, only_if_is_dirty=False)


def revert():
    import ue_terrain_material as tm
    mat = eal.load_asset(DEST + "/M_TD_Terrain")
    for g in ("Verdance", "Cinder"):
        ipath = "%s/MI_Terrain_%s" % (DEST, g)
        inst = eal.load_asset(ipath)
        mel.clear_all_material_instance_parameters(inst)
        mel.set_material_instance_parent(inst, mat)
        scalars, vectors = tm.instance_values(g)
        for k, v in scalars.items():
            mel.set_material_instance_scalar_parameter_value(inst, k, float(v))
        for k, v in vectors.items():
            mel.set_material_instance_vector_parameter_value(inst, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
        for pname, suffix in (("BaseColorTex", "BC"), ("NormalTex", "N"), ("ORMTex", "ORM"), ("EmissiveTex", "E")):
            mel.set_material_instance_texture_parameter_value(inst, pname, eal.load_asset("%s/T_Ground_%s_%s" % (DEST, g, suffix)))
        mel.update_material_instance(inst)
        eal.save_asset(ipath, only_if_is_dirty=False)
    log("OK")


def main():
    if MODE == "revert":
        return revert()
    with open(os.path.join(HERE, "terrain_means.json")) as f:
        means = json.load(f)
    t = {"T_Ground_MacroNoise": eal.load_asset(DEST + "/T_Ground_MacroNoise")}
    for s in means:
        for suffix in ("BC", "N"):
            name = "T_Terr_%s_%s" % (s, suffix)
            t[name] = import_texture(name, suffix == "N")
    if problems or t["T_Ground_MacroNoise"] is None:
        for p in problems:
            log("PROBLEM: " + p)
        log("FAILED (%d problems)" % len(problems))
        return
    path = DEST + "/M_TD_TerrainR2"
    if MODE != "instances" or not eal.does_asset_exist(path):
        mat = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(
            "M_TD_TerrainR2", DEST, unreal.Material, unreal.MaterialFactoryNew())
        mel.delete_all_material_expressions(mat)
        mat.set_editor_property("tangent_space_normal", False)
        tg.build_graph(ue_mat_builder.MB(mat, problems), t, means)
        mel.recompile_material(mat)
        mel.layout_material_expressions(mat)
        eal.save_asset(path, only_if_is_dirty=False)
        try:
            st = mel.get_statistics(mat)
            log("stats: ps instructions %s, samplers %s, texture samples %s" % (
                st.get_editor_property("num_pixel_shader_instructions"), st.get_editor_property("num_samplers"),
                st.get_editor_property("num_pixel_texture_samples")))
        except Exception as e:  # noqa: BLE001
            log("stats unavailable: %s" % e)
    else:
        mat = eal.load_asset(path)
    for g in ("Verdance", "Cinder"):
        set_instance(g, mat, t, means)
    for p in problems:
        log("PROBLEM: " + p)
    log("FAILED (%d problems)" % len(problems) if problems else "OK")


main()

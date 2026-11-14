# SPDX-License-Identifier: MIT
# Cinder basalt column contrast (environment builder, pass 4): the columns read faint from the air, so they get their
# own lit material with a strong value range: a near-black base rising to a lighter weathered grey up the column (local
# height over the mesh's bounds, so it works per instance at any scale), and pale ash on the flat tops (faces that look up).
#   M_TD_ScatterColumn + MI_Scatter_Column_Cinder in /Game/TriadDrift/Gameplay/Props/Scatter (the column mesh's own MI is
#   not touched; the scatter assigns this one through the kind's MaterialPath).
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngEnvironment\scripts\ue_column_build.py
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(REPO, "ArtSource", "EngProps", "Planets", "scripts"))
import ue_mat_builder  # noqa: E402

DEST = "/Game/TriadDrift/Gameplay/Props/Scatter"
ROCK_M = "/Game/TriadDrift/Gameplay/Props/Planets/T_Terrain_Rock_M"
ROCK_N = "/Game/TriadDrift/Gameplay/Props/Planets/T_Terrain_Rock_N"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
MP = unreal.MaterialProperty
ST = unreal.MaterialSamplerType
problems = []
MI_VALUES = {
    "BaseTint": (0.012, 0.010, 0.011), "MidTint": (0.075, 0.062, 0.058), "TopTint": (0.30, 0.27, 0.25),
    "GradientPower": 1.6, "TopStart": 0.55, "TopSharpness": 6.0, "UVScale": 1.0, "Specular": 0.3,
}


def log(m):
    unreal.log("TDPY: " + str(m))


def build():
    path = DEST + "/M_TD_ScatterColumn"
    mat = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(
        "M_TD_ScatterColumn", DEST, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)
    for flag in ("used_with_instanced_static_meshes", "used_with_nanite"):
        try:
            mat.set_editor_property(flag, True)
        except Exception as error:  # noqa: BLE001
            log("note: %s (%s)" % (flag, error))
    b = ue_mat_builder.MB(mat, problems)
    uv = b.mul(b.e(unreal.MaterialExpressionTextureCoordinate), b.scalar("UVScale", 1.0))
    m = b.tex("RockMaskTex", eal.load_asset(ROCK_M), ST.SAMPLERTYPE_MASKS, uv)
    n = b.tex("RockNormalTex", eal.load_asset(ROCK_N), ST.SAMPLERTYPE_NORMAL, uv)
    shade = b.mul(b.lerp(b.const(0.5), b.const(1.4), m, "", "", "R"), b.lerp(b.const(0.5), b.const(1.0), m, "", "", "B"))
    # height up the column, 0 at the base and 1 at the top of the mesh bounds (per instance: local space)
    lp = b.e(unreal.MaterialExpressionLocalPosition)
    lb = b.e(unreal.MaterialExpressionObjectLocalBounds)
    hgt = b.sat(b.div(b.sub(b.mask(lp, False, False, True), b.mask(lb, False, False, True, "Min")),
                      b.sub(b.mask(lb, False, False, True, "Max"), b.mask(lb, False, False, True, "Min"))))
    g = b.pow(hgt, b.scalar("GradientPower", 1.6))
    side = b.lerp(b.vector("BaseTint", MI_VALUES["BaseTint"]), b.vector("MidTint", MI_VALUES["MidTint"]), g)
    up = b.mask(b.e(unreal.MaterialExpressionVertexNormalWS), False, False, True)
    topw = b.sat(b.mul(b.sub(b.add(up, b.mul(b.mask(m, True), b.const(0.15))), b.scalar("TopStart", 0.55)), b.scalar("TopSharpness", 6.0)))
    col = b.mul(b.lerp(side, b.vector("TopTint", MI_VALUES["TopTint"]), topw), shade)
    b.out(col, "", MP.MP_BASE_COLOR)
    b.out(b.lerp(b.const(0.92), b.const(0.75), m, "", "", "G"), "", MP.MP_ROUGHNESS)
    b.out(b.scalar("Specular", 0.3), "", MP.MP_SPECULAR)
    b.out(n, "", MP.MP_NORMAL)
    mel.recompile_material(mat)
    mel.layout_material_expressions(mat)
    eal.save_asset(path, only_if_is_dirty=False)
    ipath = DEST + "/MI_Scatter_Column_Cinder"
    mi = eal.load_asset(ipath) if eal.does_asset_exist(ipath) else tools.create_asset(
        "MI_Scatter_Column_Cinder", DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, mat)
    for k, v in MI_VALUES.items():
        ok = (mel.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
              if isinstance(v, tuple) else mel.set_material_instance_scalar_parameter_value(mi, k, float(v)))
        if not ok:
            problems.append("MI." + k)
    mel.update_material_instance(mi)
    eal.save_asset(ipath, only_if_is_dirty=False)
    for p in problems:
        log("PROBLEM: " + p)
    log("instructions: %s" % mel.get_statistics(mat).get_editor_property("num_pixel_shader_instructions"))


build()
for p in problems:
    log("PROBLEM: " + p)
log("FAILED (%d)" % len(problems) if problems else "OK")

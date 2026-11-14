# SPDX-License-Identifier: MIT
# Dominion interior look pass (environment builder): material instances of the kit only (meshes and the code-built
# layout are T5's).
#   M_DI_AtlasLit (new, DominionInterior/Materials): the kit atlas like M_TD_PropTextured, plus
#       floors (up-facing surfaces) darker and rougher (the ceiling lights mirrored in the metal plates made them read bright) (FloorTint, FloorRoughness*) so the rooms are not bright
#       empty slabs; walls / ceilings get WallTint; a faint per-tile variation from the world position.
#   MI_DI_Atlas         re-parented to M_DI_AtlasLit (same textures)
#   MI_DI_CoreGlow      a cool key on the reactor crystal against the room's orange (CoreColor / strength)
#   MI_DI_Cell          the conduit cells a little less orange-hot, so the core room is not one orange wash
#   MI_DI_Shield        the reactor's shield dome cool blue and thinner (was the master's orange; revert clears it)
# TD_DI_LOOK_MODE=revert puts MI_DI_Atlas back on M_TD_PropTextured and the glows back to the kit README values.
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngEnvironment\scripts\ue_di_look_build.py
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(REPO, "ArtSource", "EngProps", "Planets", "scripts"))
import ue_mat_builder  # noqa: E402

DEST = "/Game/TriadDrift/Gameplay/Props/DominionInterior"
MASTER_OLD = "/Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured"
MODE = os.environ.get("TD_DI_LOOK_MODE", "build")
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
MP = unreal.MaterialProperty
ST = unreal.MaterialSamplerType
problems = []

ATLAS_SCALARS = {"FloorUp": 0.65, "FloorSoft": 0.2, "FloorRoughnessScale": 1.25, "FloorRoughnessBias": 0.15,
                 "TileVar": 0.12, "TileSize": 400.0, "EmissiveStrength": 0.0, "SelfIllum": 0.0}
ATLAS_VECTORS = {"FloorTint": (0.17, 0.18, 0.21), "WallTint": (0.82, 0.84, 0.9)}
# glow instances: (GlowColor, GlowStrength) new / README (revert)
GLOWS = {
    "MI_DI_CoreGlow": (((0.55, 0.85, 1.0), 26.0), ((1.0, 0.86, 0.7), 30.0)),
    "MI_DI_Cell": (((1.0, 0.36, 0.06), 9.0), ((1.0, 0.42, 0.08), 14.0)),
}


def log(m):
    unreal.log("TDPY: " + str(m))


def build_master():
    path = DEST + "/Materials/M_DI_AtlasLit"
    mat = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(
        "M_DI_AtlasLit", DEST + "/Materials", unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)
    b = ue_mat_builder.MB(mat, problems)
    bc = b.tex("BaseColorTex", eal.load_asset(DEST + "/Textures/T_DI_Atlas_BC"), ST.SAMPLERTYPE_COLOR)
    orm = b.tex("ORMTex", eal.load_asset(DEST + "/Textures/T_DI_Atlas_ORM"), ST.SAMPLERTYPE_MASKS)
    nrm = b.tex("NormalTex", eal.load_asset(DEST + "/Textures/T_DI_Atlas_N"), ST.SAMPLERTYPE_NORMAL)
    em = b.tex("EmissiveTex", eal.load_asset("/Engine/EngineResources/Black"), ST.SAMPLERTYPE_COLOR)
    up = b.e(unreal.MaterialExpressionVertexNormalWS)
    upz = b.mask(up, False, False, True)
    floor = b.sat(b.div(b.sub(upz, b.scalar("FloorUp", 0.65)), b.scalar("FloorSoft", 0.2)))
    tint = b.lerp(b.mask(b.vector("WallTint", ATLAS_VECTORS["WallTint"]), True, True, True),
                  b.mask(b.vector("FloorTint", ATLAS_VECTORS["FloorTint"]), True, True, True), floor)
    # per 4 m tile: a faint value step so a floor of 30 identical tiles is not one sheet
    wp = b.e(unreal.MaterialExpressionWorldPosition)
    cell = b.e(unreal.MaterialExpressionFloor)
    b.c(b.div(b.mask(wp, True, True, False), b.scalar("TileSize", 400.0)), "", cell, "")
    h = b.frac(b.mul(b.dot(cell, b.const2(0.1031, 0.1137)), b.const(437.585)))
    var = b.add(b.const(1.0), b.mul(b.sub(h, b.const(0.5)), b.scalar("TileVar", 0.12)))
    base = b.mul(b.mul(bc, tint, "RGB", ""), var)
    b.out(base, "", MP.MP_BASE_COLOR)
    b.out(orm, "R", MP.MP_AMBIENT_OCCLUSION)
    rough = b.lerp(b.mask(orm, False, True, False), b.add(b.mul(b.mask(orm, False, True, False), b.scalar("FloorRoughnessScale", 0.55)),
                                                          b.scalar("FloorRoughnessBias", 0.0)), floor)
    b.out(b.sat(rough), "", MP.MP_ROUGHNESS)
    b.out(orm, "B", MP.MP_METALLIC)
    b.out(nrm, "RGB", MP.MP_NORMAL)
    e = b.add(b.mul(em, b.scalar("EmissiveStrength", 0.0), "RGB", ""), b.mul(base, b.scalar("SelfIllum", 0.0)))
    b.out(e, "", MP.MP_EMISSIVE_COLOR)
    mel.recompile_material(mat)
    mel.layout_material_expressions(mat)
    eal.save_asset(path, only_if_is_dirty=False)
    return mat


def main():
    atlas = eal.load_asset(DEST + "/MI_DI_Atlas")
    tex = {k: mel.get_material_instance_texture_parameter_value(atlas, k) for k in ("BaseColorTex", "ORMTex", "NormalTex")}
    if MODE == "revert":
        mel.set_material_instance_parent(atlas, eal.load_asset(MASTER_OLD))
    else:
        mel.set_material_instance_parent(atlas, build_master())
        for k, v in ATLAS_SCALARS.items():
            if not mel.set_material_instance_scalar_parameter_value(atlas, k, float(v)):
                problems.append("atlas scalar " + k)
        for k, v in ATLAS_VECTORS.items():
            if not mel.set_material_instance_vector_parameter_value(atlas, k, unreal.LinearColor(v[0], v[1], v[2], 1.0)):
                problems.append("atlas vector " + k)
    for k, t in tex.items():
        if t is None or not mel.set_material_instance_texture_parameter_value(atlas, k, t):
            problems.append("atlas texture " + k)
    mel.update_material_instance(atlas)
    eal.save_asset(DEST + "/MI_DI_Atlas", only_if_is_dirty=False)
    for name, (new, old) in GLOWS.items():
        (rgb, strength) = old if MODE == "revert" else new
        mi = eal.load_asset(DEST + "/" + name)
        mel.set_material_instance_vector_parameter_value(mi, "GlowColor", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
        mel.set_material_instance_scalar_parameter_value(mi, "GlowStrength", float(strength))
        mel.update_material_instance(mi)
        eal.save_asset(DEST + "/" + name, only_if_is_dirty=False)
    # the reactor's shield dome: cool and thinner (it was the master's orange, the core read as one orange block)
    sh = eal.load_asset(DEST + "/MI_DI_Shield")
    mel.clear_all_material_instance_parameters(sh)
    if MODE != "revert":
        mel.set_material_instance_vector_parameter_value(sh, "ShieldColor", unreal.LinearColor(0.32, 0.55, 1.0, 1.0))
        mel.set_material_instance_scalar_parameter_value(sh, "ShieldStrength", 4.0)
        mel.set_material_instance_scalar_parameter_value(sh, "ShieldOpacity", 0.22)
    mel.update_material_instance(sh)
    eal.save_asset(DEST + "/MI_DI_Shield", only_if_is_dirty=False)
    for p in problems:
        log("PROBLEM: " + p)
    log("FAILED (%d problems)" % len(problems) if problems else "OK (%s)" % MODE)


main()

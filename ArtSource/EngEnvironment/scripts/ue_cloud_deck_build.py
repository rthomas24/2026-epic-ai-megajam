# SPDX-License-Identifier: MIT
# The descent's cloud deck (ENG-38 stage 2 fallback: a card deck, not a Volumetric Cloud) in
# /Game/TriadDrift/Gameplay/Props/Sky:
#   T_Descent_CloudDeck          1K RGBA, from Grok Bot's ArtSource/EngAI/Descent/CloudDeck/r1 (downscaled into
#                                Saved/EnvTerrainTex by this script's caller: prep below)
#   M_TD_CloudDeck               unlit translucent, two-sided, world-projected: two drifting scales of the deck sheet,
#                                soft where it meets terrain (depth fade), fades out around the camera as the ship
#                                passes through (the whiteout itself is the descent code's), fills to a solid layer far
#                                away so it closes the horizon from above, darker seen from below
#   MI_CloudDeck_Verdance / MI_CloudDeck_Cinder
# It is one quad: put /Engine/BasicShapes/Plane at the deck altitude, scale 2000 (200 km), no collision, no shadow.
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngEnvironment\scripts\ue_cloud_deck_build.py
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(REPO, "ArtSource", "EngProps", "Planets", "scripts"))
import ue_mat_builder  # noqa: E402

SRC = os.path.join(REPO, "Saved", "EnvTerrainTex", "T_Descent_CloudDeck.png")
DEST = "/Game/TriadDrift/Gameplay/Props/Sky"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
E = unreal
MP = unreal.MaterialProperty
problems = []

# name: (default / Verdance, Cinder)
SCALARS = {
    "TileA": (900000.0, 900000.0), "TileB": (2300000.0, 2300000.0), "DriftX": (900.0, 1400.0), "DriftY": (350.0, -500.0),
    "Coverage": (0.15, 0.3), "Contrast": (1.6, 1.4), "Opacity": (0.92, 0.85),
    "SoftDepth": (6000.0, 6000.0), "NearFadeStart": (3000.0, 3000.0), "NearFadeRange": (20000.0, 20000.0),
    "FarFillStart": (1200000.0, 1000000.0), "FarFillRange": (2500000.0, 2000000.0), "BelowDarken": (0.45, 0.4),
    "Glow": (1.0, 1.0),
}
VECTORS = {
    "LitColor": ((0.80, 0.95, 0.90), (0.95, 0.52, 0.30)),
    "ShadowColor": ((0.10, 0.30, 0.32), (0.22, 0.07, 0.04)),
}


def log(m):
    unreal.log("TDPY: " + str(m))


def build(b, tex):
    def P(x):
        return x if isinstance(x, tuple) else (x, "")

    def op(cls, a, c):
        n = b.e(cls)
        a, c = P(num(a)), P(num(c))
        b.c(a[0], a[1], n, "A")
        b.c(c[0], c[1], n, "B")
        return n

    def num(x):
        return b.const(x) if isinstance(x, (int, float)) else x

    def mul(a, c): return op(E.MaterialExpressionMultiply, a, c)
    def add(a, c): return op(E.MaterialExpressionAdd, a, c)
    def sub(a, c): return op(E.MaterialExpressionSubtract, a, c)
    def div(a, c): return op(E.MaterialExpressionDivide, a, c)
    def app(a, c): return op(E.MaterialExpressionAppendVector, a, c)

    def un(cls, a):
        n = b.e(cls)
        a = P(a)
        b.c(a[0], a[1], n, "")
        return n

    def sat(a): return un(E.MaterialExpressionSaturate, a)
    def inv(a): return un(E.MaterialExpressionOneMinus, a)
    def vabs(a): return un(E.MaterialExpressionAbs, a)

    def lerp(a, c, al):
        n = b.e(E.MaterialExpressionLinearInterpolate)
        for v, pin in ((num(a), "A"), (num(c), "B"), (num(al), "Alpha")):
            v = P(v)
            b.c(v[0], v[1], n, pin)
        return n

    def mask(a, r=False, g=False, bl=False):
        n = b.e(E.MaterialExpressionComponentMask, r=r, g=g, b=bl, a=False)
        a = P(a)
        b.c(a[0], a[1], n, "")
        return n

    def S(name): return b.scalar(name, SCALARS[name][0])
    def V(name): return mask(b.vector(name, VECTORS[name][0]), r=True, g=True, bl=True)

    wp = b.e(E.MaterialExpressionWorldPosition)
    cam = b.e(E.MaterialExpressionCameraPositionWS)
    xy = mask(wp, r=True, g=True)
    time = b.e(E.MaterialExpressionTime)
    drift = mul(app(S("DriftX"), S("DriftY")), time)
    uva = div(add(xy, drift), S("TileA"))
    uvb = add(div(sub(app(mask(wp, g=True), mul(mask(wp, r=True), -1.0)), mul(drift, 0.6)), S("TileB")), b.const2(0.37, 0.11))
    ta = b.tex("DeckTex", tex, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, uva)
    tb = b.tex("DeckTex", tex, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, uvb)
    # cover: the fine sheet shaped by the broad one, pushed by Coverage, sharpened by Contrast
    cover = sat(mul(sub(add(mul((ta, "A"), 0.6), mul((tb, "A"), 0.7)), sub(0.5, S("Coverage"))), S("Contrast")))
    lum = sat(add(mul((ta, "R"), 0.6), mul((tb, "R"), 0.4)))
    depth = b.e(E.MaterialExpressionPixelDepth)
    fill = sat(div(sub(depth, S("FarFillStart")), S("FarFillRange")))          # a solid layer towards the horizon
    dz = sub(mask(cam, bl=True), mask(wp, bl=True))
    near = sat(div(sub(vabs(dz), S("NearFadeStart")), S("NearFadeRange")))     # gone around a camera inside the deck
    soft = b.e(E.MaterialExpressionDepthFade)
    sd = S("SoftDepth")
    b.c(sd, "", soft, "FadeDistance")
    alpha = mul(mul(lerp(cover, 1.0, fill), S("Opacity")), mul(near, soft))
    above = sat(add(div(dz, 20000.0), 0.5))
    col = mul(lerp(V("ShadowColor"), V("LitColor"), lerp(lum, 0.75, fill)), lerp(S("BelowDarken"), 1.0, above))
    b.out(mul(col, S("Glow")), "", MP.MP_EMISSIVE_COLOR)
    b.out(alpha, "", MP.MP_OPACITY)


def main():
    path = DEST + "/T_Descent_CloudDeck"
    if not eal.does_asset_exist(path):
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", SRC.replace("\\", "/"))
        task.set_editor_property("destination_path", DEST)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", False)
        tools.import_asset_tasks([task])
    tex = eal.load_asset(path)
    if tex is None:
        log("FAILED (texture import)")
        return
    tex.set_editor_property("srgb", True)
    eal.save_loaded_asset(tex)
    mpath = DEST + "/M_TD_CloudDeck"
    mat = eal.load_asset(mpath) if eal.does_asset_exist(mpath) else tools.create_asset(
        "M_TD_CloudDeck", DEST, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    build(ue_mat_builder.MB(mat, problems), tex)
    mel.recompile_material(mat)
    mel.layout_material_expressions(mat)
    eal.save_asset(mpath, only_if_is_dirty=False)
    for i, g in enumerate(("Verdance", "Cinder")):
        ipath = "%s/MI_CloudDeck_%s" % (DEST, g)
        inst = eal.load_asset(ipath) if eal.does_asset_exist(ipath) else tools.create_asset(
            "MI_CloudDeck_" + g, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.clear_all_material_instance_parameters(inst)
        mel.set_material_instance_parent(inst, mat)
        for k, v in SCALARS.items():
            if not mel.set_material_instance_scalar_parameter_value(inst, k, float(v[i])):
                problems.append("%s: scalar %s" % (g, k))
        for k, v in VECTORS.items():
            if not mel.set_material_instance_vector_parameter_value(inst, k, unreal.LinearColor(v[i][0], v[i][1], v[i][2], 1.0)):
                problems.append("%s: vector %s" % (g, k))
        mel.update_material_instance(inst)
        eal.save_asset(ipath, only_if_is_dirty=False)
    for p in problems:
        log("PROBLEM: " + p)
    log("FAILED (%d problems)" % len(problems) if problems else "OK")


main()

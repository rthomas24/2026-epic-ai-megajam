# SPDX-License-Identifier: MIT
# The planets' sea surfaces (ART-06, environment builder), replacing the tinted engine cylinder ATDPlanetTerrain draws
# for the region sea (Verdance: a lake; Cinder: a lava sea). No new textures: T_Ground_MacroNoise and
# T_Terrain_Detail_N, world-projected, so the flattened cylinder's own UVs do not matter.
#   /Game/TriadDrift/Gameplay/Props/Planets/M_TD_PlanetSea      lit, opaque: still dark water with drifting ripples and
#                                                               bioluminescent specks, or a crusted lava sea that glows
#                                                               through its cracks (LavaAmount 0 / 1)
#   MI_PlanetSea_Verdance, MI_PlanetSea_Cinder
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngEnvironment\scripts\ue_sea_build.py
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(REPO, "ArtSource", "EngProps", "Planets", "scripts"))
import ue_mat_builder  # noqa: E402

DEST = "/Game/TriadDrift/Gameplay/Props/Planets"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
E = unreal
MP = unreal.MaterialProperty
ST = unreal.MaterialSamplerType
problems = []

# name: (Verdance, Cinder)
SCALARS = {
    "LavaAmount": (0.0, 1.0),
    "RippleTile": (2500.0, 9000.0), "RippleSpeed": (60.0, 25.0), "NormalStrength": (0.35, 0.8),
    "Roughness": (0.06, 0.55), "Specular": (0.5, 0.3),
    "NoiseTile": (60000.0, 30000.0), "FlowSpeed": (150.0, 90.0),
    "SpeckStrength": (0.6, 0.0), "SpeckSharp": (30.0, 30.0),
    "LavaGlow": (0.0, 14.0), "CrustAmount": (0.0, 0.55), "CrustSharp": (0.0, 6.0),
}
VECTORS = {
    "WaterColor": ((0.004, 0.030, 0.036), (0.02, 0.006, 0.003)),
    "SpeckColor": ((0.10, 0.85, 1.00), (0.0, 0.0, 0.0)),
    "LavaHot": ((1.0, 0.85, 0.35), (1.0, 0.75, 0.25)),
    "LavaCool": ((0.9, 0.12, 0.02), (0.9, 0.12, 0.015)),
}


def log(m):
    unreal.log("TDPY: " + str(m))


def build(b, noise, nrm):
    def P(x):
        return x if isinstance(x, tuple) else (x, "")

    def num(x):
        return b.const(x) if isinstance(x, (int, float)) else x

    def op(cls, a, c):
        n = b.e(cls)
        a, c = P(num(a)), P(num(c))
        b.c(a[0], a[1], n, "A")
        b.c(c[0], c[1], n, "B")
        return n

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
    xy = mask(wp, r=True, g=True)
    t = b.e(E.MaterialExpressionTime)
    # ripples: two detail-normal layers drifting against each other
    ra = div(add(xy, mul(app(S("RippleSpeed"), 0.0), t)), S("RippleTile"))
    rb = add(div(sub(xy, mul(app(0.0, S("RippleSpeed")), mul(t, 0.7))), mul(S("RippleTile"), 0.63)), b.const2(0.31, 0.77))
    na = b.tex("RippleN", nrm, ST.SAMPLERTYPE_NORMAL, ra)
    nb = b.tex("RippleN", nrm, ST.SAMPLERTYPE_NORMAL, rb)
    nxy = mul(add(mask(na, r=True, g=True), mask(nb, r=True, g=True)), S("NormalStrength"))
    b.out(b.norm(app(nxy, 1.0)), "", MP.MP_NORMAL)
    # large slow flow noise (lava crust, speck patches)
    fa = div(add(xy, mul(app(S("FlowSpeed"), mul(S("FlowSpeed"), 0.4)), t)), S("NoiseTile"))
    fb = add(div(sub(xy, mul(app(mul(S("FlowSpeed"), 0.5), S("FlowSpeed")), t)), mul(S("NoiseTile"), 0.37)), b.const2(0.53, 0.19))
    n1 = b.tex("FlowNoise", noise, ST.SAMPLERTYPE_MASKS, fa)
    n2 = b.tex("FlowNoise", noise, ST.SAMPLERTYPE_MASKS, fb)
    f = add(mul((n1, "R"), 0.6), mul((n2, "G"), 0.4))
    # lava: crust where the flow noise is high, glowing cracks between
    crack = sat(mul(sub(f, sub(1.0, S("CrustAmount"))), S("CrustSharp")))      # 1 = crusted
    heat = inv(crack)
    hotsel = sat(mul(vabs(sub((n2, "B"), 0.5)), 3.0))
    lava_e = mul(mul(lerp(V("LavaCool"), V("LavaHot"), mul(heat, inv(hotsel))), mul(heat, S("LavaGlow"))), add(0.7, mul((n1, "B"), 0.6)))
    # water: bioluminescent specks (ridges of the noise) in patches
    speck = sat(inv(mul(vabs(sub((n1, "B"), 0.5)), S("SpeckSharp"))))
    water_e = mul(V("SpeckColor"), mul(mul(speck, speck), mul(sat(mul(sub((n2, "R"), 0.45), 4.0)), S("SpeckStrength"))))
    la = S("LavaAmount")
    base = lerp(V("WaterColor"), mul(V("WaterColor"), lerp(1.0, 2.0, crack)), la)
    b.out(base, "", MP.MP_BASE_COLOR)
    b.out(lerp(S("Roughness"), lerp(0.25, 0.9, crack), la), "", MP.MP_ROUGHNESS)
    b.out(S("Specular"), "", MP.MP_SPECULAR)
    b.out(lerp(water_e, lava_e, la), "", MP.MP_EMISSIVE_COLOR)


def main():
    noise = eal.load_asset(DEST + "/T_Ground_MacroNoise")
    nrm = eal.load_asset(DEST + "/T_Terrain_Detail_N")
    if noise is None or nrm is None:
        log("FAILED (textures missing)")
        return
    path = DEST + "/M_TD_PlanetSea"
    mat = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(
        "M_TD_PlanetSea", DEST, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)
    build(ue_mat_builder.MB(mat, problems), noise, nrm)
    mel.recompile_material(mat)
    mel.layout_material_expressions(mat)
    eal.save_asset(path, only_if_is_dirty=False)
    for i, g in enumerate(("Verdance", "Cinder")):
        ipath = "%s/MI_PlanetSea_%s" % (DEST, g)
        inst = eal.load_asset(ipath) if eal.does_asset_exist(ipath) else tools.create_asset(
            "MI_PlanetSea_" + g, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
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
    try:
        st = mel.get_statistics(mat)
        log("stats: ps instructions %s, samplers %s" % (st.get_editor_property("num_pixel_shader_instructions"), st.get_editor_property("num_samplers")))
    except Exception as e:  # noqa: BLE001
        log("stats unavailable: %s" % e)
    for p in problems:
        log("PROBLEM: " + p)
    log("FAILED (%d problems)" % len(problems) if problems else "OK")


main()

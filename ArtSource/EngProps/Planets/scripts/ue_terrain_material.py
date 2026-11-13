# SPDX-License-Identifier: MIT
# M_TD_Terrain graph (Unreal editor Python): the ENG-34 planet terrain material, used by MI_Terrain_Verdance and
# MI_Terrain_Cinder on ATDPlanetTerrain's runtime ProceduralMesh chunks.
#
# Vertex contract (TDPlanetTerrain.cpp / TDTerrainField.cpp):
#   UV1 = (height in metres, slope);  colour R = height01, G = slope (1 - N.z: 0 flat, 0.29 at 45 deg, 1 vertical),
#   B = broad mask (~4 km fbm), A = fine mask (~60 m noise).
# Used: UV1.x (height), colour G / B / A. Not used: UV0 (every projection takes the absolute world position, exactly
# like M_TD_Ground, so the lowland layer continues the course slab's pattern) and colour R (height01 is relative to
# the style's theoretical maximum, so a threshold on it moves whenever the relief is retuned; metres / HeightRangeM
# is one number to change).
#
# Layers (bottom to top), all thresholds jittered by the fine mask + a ~17 m world noise (no straight contour bands)
# and shifted by the broad mask (BiomeBreak), so the lines sit higher in one region than in the next:
#   lowland  the planet's ground set (T_Ground_*): M_TD_Ground's world tile, 21 m far tile, macro noise, tints,
#            AO and emissive mask, so at slope 0 / height 0 it is the course ground (seamless basin edge). Left out
#            to stay affordable: the ground's second near copy, its second far tile and the far river-core glow.
#   soil     by slope and by height (T_Terrain_Detail_M.R)
#   rock     on steep faces: side-projected (world YZ / XZ picked by the vertex normal, so no UV stretching on
#            cliffs), near tile x far tile (the near one fades to its mean with distance), world-space normal
#   peak     frost (Verdance) / ash (Cinder) above a height line, thinning on steep faces (T_Terrain_Detail_M.G)
#   valley   an emissive band along a contour of the broad mask on low flat ground: lava rivers with a drifting
#            crust (Cinder) or a faint teal floor glow (Verdance); never within ValleyMinM of the basin level
#   glints   sparse crystal facets on high ground in ~50 m patches (amber on Cinder, cyan frost sparkle on Verdance)
# Normals are output in WORLD space (material.tangent_space_normal = False): vertex normal + projected detail.
# All texture samples use the shared world-group wrap sampler.
#
# NOTE for the terrain code: the valley band follows the contour BroadMask = ValleyCenter. TDTerrainField carves
# Cinder's river beds along a different noise (RN, seed + 61) than BroadMask (seed + 71), so today the lava does not
# sit in the carved beds. Writing the river noise into the mask on Cinder (BroadMask = clamp(0.5 + RN * 4.5)) puts
# it there; then set ValleyWidth to about 0.35.
import math

import unreal

import ue_ground_material as gm

ST = unreal.MaterialSamplerType
MP = unreal.MaterialProperty
M = gm.MACRO

# name: (Verdance, Cinder)
SCALARS = {
    # height source and transition breakup
    "HeightRangeM": (2000.0, 2000.0),      # metres (UV1.x) that map to height 1.0 for the thresholds below
    "HeightBreak": (0.14, 0.14), "BiomeBreak": (0.16, 0.12), "SlopeBreak": (0.08, 0.07),
    # soil
    "SoilSlopeStart": (0.06, 0.05), "SoilSlopeRange": (0.07, 0.06), "SoilHeightStart": (0.22, 0.16),
    "SoilHeightRange": (0.14, 0.12), "SoilRoughness": (0.92, 0.95),
    # rock
    "RockSlopeStart": (0.20, 0.16), "RockSlopeRange": (0.14, 0.12), "RockTile": (1400.0, 1100.0),
    "RockFarScale": (11.0, 13.0), "RockNormalStrength": (1.1, 1.5), "RockRoughness": (1.0, 1.05),
    "RockWarp": (2500.0, 2500.0),          # cm the strata are bent up / down by the macro noise
    # peak (frost / ash)
    "PeakStart": (0.32, 0.34), "PeakRange": (0.10, 0.16), "PeakSlopeMax": (0.36, 0.34), "PeakSlopeRange": (0.30, 0.28),
    "PeakRoughness": (0.55, 0.97), "PeakGlow": (0.22, 0.0),
    # detail set (soil / peak)
    "DetailTile": (500.0, 500.0), "OtherSpecular": (0.35, 0.18),
    # valley band (teal floor glow / lava rivers)
    "ValleyCenter": (0.5, 0.5), "ValleyWidth": (0.14, 0.045), "ValleyBreak": (0.03, 0.012),
    "ValleySlopeMax": (0.05, 0.10), "ValleyTop": (0.07, 0.75), "ValleyFade": (0.06, 0.2),
    "ValleyMinM": (4.0, 3.0), "ValleyMinFadeM": (30.0, 12.0), "ValleyEmissive": (0.05, 9.0),
    "ValleyHeatGain": (1.0, 2.2), "ValleyCrust": (0.55, 1.15), "ValleyBaseBlend": (0.0, 1.0),
    "FlowTile": (9000.0, 6000.0), "FlowX": (0.0, 35.0), "FlowY": (12.0, 60.0),
    # glints
    "GlintStart": (0.46, 0.18), "GlintRange": (0.08, 0.12), "GlintNoiseMin": (0.50, 0.50), "GlintStrength": (1.0, 5.0),
}
VECTORS = {
    "SoilTint": ((0.050, 0.036, 0.030), (0.030, 0.024, 0.022)),
    "RockTintA": ((0.050, 0.045, 0.070), (0.008, 0.007, 0.008)),
    "RockTintB": ((0.170, 0.160, 0.215), (0.040, 0.033, 0.031)),
    "PeakTint": ((0.62, 0.78, 0.80), (0.30, 0.285, 0.275)),
    "ValleyCoreColor": ((0.20, 1.0, 0.80), (1.0, 0.36, 0.04)),
    "ValleyEdgeColor": ((0.05, 0.55, 0.60), (0.80, 0.05, 0.008)),
    "MoltenBase": ((0.02, 0.05, 0.05), (0.12, 0.025, 0.005)),
    "GlintColor": ((0.45, 1.0, 0.95), (1.0, 0.36, 0.03)),
}


def instance_values(g):
    i = 0 if g == "Verdance" else 1
    scalars, vectors = gm.instance_values(g)             # the lowland layer takes the ground instance's values
    for unused in ("BScale", "BlendContrast", "Far2Ratio", "RiverGlow", "RiverCoreThreshold", "RiverCoreSoftness"):
        scalars.pop(unused, None)                        # ground-only features the terrain's lowland layer leaves out
    vectors.pop("RiverColor", None)
    scalars.update({k: v[i] for k, v in SCALARS.items()})
    vectors.update({k: v[i] for k, v in VECTORS.items()})
    return scalars, vectors


def build_terrain_graph(b, t):
    """b: ue_mat_builder.MB on an empty material (tangent_space_normal must be False); t: texture assets by name
    (T_Ground_Verdance_{BC,N,ORM,E}, T_Ground_MacroNoise, T_Terrain_{Rock,Detail}_{M,N})."""
    shared = unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS
    E = unreal

    def P(x):                                    # value = node or (node, output name)
        return x if isinstance(x, tuple) else (x, "")

    def op(cls, a, c, ia="A", ic="B"):
        n = b.e(cls)
        a, c = P(a), P(c)
        b.c(a[0], a[1], n, ia)
        b.c(c[0], c[1], n, ic)
        return n

    def K(v):
        return b.const(v)

    def num(x):
        return K(x) if isinstance(x, (int, float)) else x

    def mul(a, c): return op(E.MaterialExpressionMultiply, num(a), num(c))
    def add(a, c): return op(E.MaterialExpressionAdd, num(a), num(c))
    def sub(a, c): return op(E.MaterialExpressionSubtract, num(a), num(c))
    def div(a, c): return op(E.MaterialExpressionDivide, num(a), num(c))
    def vmax(a, c): return op(E.MaterialExpressionMax, num(a), num(c))
    def app(a, c): return op(E.MaterialExpressionAppendVector, num(a), num(c))

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

    def S(name):
        return b.scalar(name, SCALARS[name][0])

    def Vp(name):
        return b.vector(name, VECTORS[name][0])

    def ramp(x, start, rng):                     # 0 below start, 1 at start + rng
        return sat(div(sub(x, start), rng))

    def tex(name, default, sampler, uv):
        n = b.tex(name, t[default], sampler, uv)
        n.set_editor_property("sampler_source", shared)
        return n

    # ---------------------------------------------------------------- inputs
    vc = b.e(E.MaterialExpressionVertexColor)
    h01, slope, broad, fine = (vc, "R"), (vc, "G"), (vc, "B"), (vc, "A")
    uv1 = b.e(E.MaterialExpressionTextureCoordinate, coordinate_index=1)
    hm = mask(uv1, r=True)
    wp = b.e(E.MaterialExpressionWorldPosition)
    xy = mask(wp, r=True, g=True)
    wx, wy, wz = mask(wp, r=True), mask(wp, g=True), mask(wp, bl=True)
    nv = b.e(E.MaterialExpressionVertexNormalWS)
    tile = b.scalar("TileSize", 400.0)

    def rot_uv(angle, scale_node):
        a = math.radians(angle)
        u = op(E.MaterialExpressionDotProduct, xy, b.const2(math.cos(a), -math.sin(a)))
        v = op(E.MaterialExpressionDotProduct, xy, b.const2(math.sin(a), math.cos(a)))
        return div(app(u, v), mul(tile, scale_node))

    # ---------------------------------------------------------------- macro noise (identical to M_TD_Ground)
    n1 = tex("MacroNoiseTex", "T_Ground_MacroNoise", ST.SAMPLERTYPE_MASKS, rot_uv(M["angle1"], b.scalar("MacroScale1", M["scale1"])))
    n2 = tex("MacroNoiseTex", "T_Ground_MacroNoise", ST.SAMPLERTYPE_MASKS, rot_uv(M["angle2"], b.scalar("MacroScale2", M["scale2"])))
    m = add(mul((n1, "R"), 0.6), mul((n2, "R"), 0.4))
    d1 = mul(sub(m, 0.5), b.scalar("MacroContrast", M["macro_contrast"]))
    depth = b.e(E.MaterialExpressionPixelDepth)
    fa = sat(div(sub(depth, b.scalar("FarStart", M["far_start"])), b.scalar("FarRange", M["far_range"])))

    # ---------------------------------------------------------------- layer weights
    j = add(mul(sub(fine, 0.5), 0.6), mul(sub((n1, "G"), 0.5), 0.4))            # -0.5 .. 0.5 breakup
    bio = sub(broad, 0.5)
    hn = div(hm, S("HeightRangeM"))
    hj = add(add(hn, mul(j, S("HeightBreak"))), mul(bio, S("BiomeBreak")))
    sj = add(slope, mul(j, S("SlopeBreak")))
    rock_w = ramp(sj, S("RockSlopeStart"), S("RockSlopeRange"))
    soil_s = ramp(sj, S("SoilSlopeStart"), S("SoilSlopeRange"))
    soil_h = ramp(hj, S("SoilHeightStart"), S("SoilHeightRange"))
    soil_w = vmax(soil_s, soil_h)
    peak_w = mul(ramp(hj, S("PeakStart"), S("PeakRange")), inv(ramp(slope, S("PeakSlopeMax"), S("PeakSlopeRange"))))
    low_w = mul(mul(inv(soil_w), inv(rock_w)), inv(peak_w))
    band = sat(inv(div(vabs(sub(add(broad, mul(j, S("ValleyBreak"))), S("ValleyCenter"))), S("ValleyWidth"))))
    valley_w = mul(mul(band, inv(ramp(slope, 0.0, S("ValleySlopeMax")))),
                   mul(inv(ramp(hn, S("ValleyTop"), S("ValleyFade"))), ramp(vabs(hm), S("ValleyMinM"), S("ValleyMinFadeM"))))

    # ---------------------------------------------------------------- lowland (the ground set, as M_TD_Ground)
    uva = div(xy, tile)
    far_scale = b.scalar("FarScale", M["far_scale"])
    uvf1 = add(div(xy, mul(tile, far_scale)), b.const2(0.17, 0.71))
    BC, NR, ORM, EM = (("BaseColorTex", "T_Ground_Verdance_BC", ST.SAMPLERTYPE_COLOR), ("NormalTex", "T_Ground_Verdance_N", ST.SAMPLERTYPE_NORMAL),
                       ("ORMTex", "T_Ground_Verdance_ORM", ST.SAMPLERTYPE_MASKS), ("EmissiveTex", "T_Ground_Verdance_E", ST.SAMPLERTYPE_COLOR))
    low_bc = lerp(tex(*BC, uva), tex(*BC, uvf1), fa)
    low_em = lerp(tex(*EM, uva), tex(*EM, uvf1), fa)
    orm = tex(*ORM, uva)
    n_a = tex(*NR, uva)
    n_f = tex(*NR, uvf1)
    low_n = lerp(n_a, lerp(b.const3(0.0, 0.0, 1.0), n_f, b.scalar("FarNormalStrength", M["far_normal"])), fa)
    factor = add(1.0, mul(d1, mul(b.scalar("MacroStrength", M["strength"]), 0.35)))
    tint = lerp(b.vector("MacroTintA", (0.8, 0.8, 0.8)), b.vector("MacroTintB", (1, 1, 1)), sat(add(mul(d1, 0.5), 0.5)))
    low_ao = (orm, "R")
    low_c = mul(mul(low_bc, tint), lerp(1.0, low_ao, b.scalar("AOToBaseColor", 1.0)))
    low_r = add(add((orm, "G"), mul(d1, b.scalar("MacroRoughness", M["macro_rough"]))), mul(fa, b.scalar("FarRoughness", M["far_rough"])))

    # ---------------------------------------------------------------- soil / peak detail set (top projection)
    dtile = S("DetailTile")
    d_n = tex("DetailMaskTex", "T_Terrain_Detail_M", ST.SAMPLERTYPE_MASKS, div(xy, dtile))
    d_nrm = tex("DetailNormalTex", "T_Terrain_Detail_N", ST.SAMPLERTYPE_NORMAL, div(xy, dtile))
    soil_c = mul(Vp("SoilTint"), mul((d_n, "R"), 1.7))
    peak_l = mul((d_n, "G"), 1.4)
    peak_c = mul(Vp("PeakTint"), peak_l)

    # ---------------------------------------------------------------- rock (side projections, near x far)
    nx2 = mul(mask(nv, r=True), mask(nv, r=True))
    ny2 = mul(mask(nv, g=True), mask(nv, g=True))
    wsel = div(nx2, add(add(nx2, ny2), 0.0001))           # 1 = the face looks along X, 0 = along Y
    rtile = S("RockTile")
    nz = mul(wz, -1.0)
    uv_x = div(app(wy, nz), rtile)               # projection along X: U = world Y, V = -world Z
    uv_y = div(app(wx, nz), rtile)
    rfar = mul(rtile, S("RockFarScale"))
    nz_f = mul(add(wz, mul(d1, S("RockWarp"))), -1.0)     # far tile: strata bent by the macro noise
    uvf_x = div(app(wy, nz_f), rfar)
    uvf_y = div(app(wx, nz_f), rfar)
    r_n = lerp(tex("RockMaskTex", "T_Terrain_Rock_M", ST.SAMPLERTYPE_MASKS, uv_y), tex("RockMaskTex", "T_Terrain_Rock_M", ST.SAMPLERTYPE_MASKS, uv_x), wsel)
    r_f = lerp(tex("RockMaskTex", "T_Terrain_Rock_M", ST.SAMPLERTYPE_MASKS, uvf_y),
               tex("RockMaskTex", "T_Terrain_Rock_M", ST.SAMPLERTYPE_MASKS, uvf_x), wsel)
    r_nrm = lerp(tex("RockNormalTex", "T_Terrain_Rock_N", ST.SAMPLERTYPE_NORMAL, uv_y), tex("RockNormalTex", "T_Terrain_Rock_N", ST.SAMPLERTYPE_NORMAL, uv_x), wsel)
    # beyond FarStart + FarRange the near tile is under a few pixels: fade it to its mean (its strata would otherwise
    # survive minification as contour bands) and soften its normal
    r_fl, r_nl = mask(r_f, r=True), lerp(mask(r_n, r=True), 0.45, fa)
    rock_c = mul(lerp(Vp("RockTintA"), Vp("RockTintB"), sat(mul(r_fl, 1.6))), mul(mul(r_nl, 2.2), mask(r_n, bl=True)))
    rock_r = mul(mask(r_n, g=True), S("RockRoughness"))

    # ---------------------------------------------------------------- valley band: flow, heat
    time = b.e(E.MaterialExpressionTime)
    flow = tex("MacroNoiseTex", "T_Ground_MacroNoise", ST.SAMPLERTYPE_MASKS,
               div(sub(xy, mul(app(S("FlowX"), S("FlowY")), time)), S("FlowTile")))
    heat = sat(sub(mul(valley_w, S("ValleyHeatGain")), mul((flow, "G"), S("ValleyCrust"))))
    molten = mul(heat, S("ValleyBaseBlend"))

    # ---------------------------------------------------------------- compose
    base = lerp(lerp(lerp(low_c, soil_c, soil_w), rock_c, rock_w), peak_c, peak_w)
    base = lerp(mul(base, factor), Vp("MoltenBase"), molten)
    rough = lerp(lerp(lerp(low_r, S("SoilRoughness"), soil_w), rock_r, rock_w), S("PeakRoughness"), peak_w)
    b.out(base, "", MP.MP_BASE_COLOR)
    b.out(sat(lerp(rough, 0.35, molten)), "", MP.MP_ROUGHNESS)
    b.out(lerp(low_ao, mask(r_n, bl=True), rock_w), "", MP.MP_AMBIENT_OCCLUSION)
    b.out(K(0.0), "", MP.MP_METALLIC)
    b.out(lerp(S("OtherSpecular"), 0.5, low_w), "", MP.MP_SPECULAR)

    # world-space normal: vertex normal * tangent z + the projected tangent xy
    top_n = lerp(d_nrm, low_n, low_w)
    p_top = app(mask(top_n, r=True, g=True), 0.0)
    rs = mul(S("RockNormalStrength"), lerp(1.0, 0.3, fa))
    rx = mul(mask(r_nrm, r=True), rs)
    p_rock = app(app(mul(rx, inv(wsel)), mul(rx, wsel)), mul(mask(r_nrm, g=True), mul(rs, -1.0)))
    nz_t = lerp(mask(top_n, bl=True), mask(r_nrm, bl=True), rock_w)
    b.out(b.norm(add(mul(nv, nz_t), lerp(p_top, p_rock, rock_w))), "", MP.MP_NORMAL)

    # ---------------------------------------------------------------- emissive
    # lowland: the M_TD_Ground formula (macro glow, river / patch mask stronger with distance, far river cores)
    gmac = sat(add(mul(d1, 1.2), 0.6))
    glow = lerp(1.0, gmac, b.scalar("GlowMacro", M["glow_macro"]))
    rf = add(mul((n1, "B"), 0.65), mul((n2, "B"), 0.35))
    ridge = inv(mul(vabs(sub(rf, 0.5)), 2.0))
    src = lerp(ridge, rf, b.scalar("EmissiveMaskMode", 0.0))
    rm = sat(div(sub(src, b.scalar("EmissiveMaskThreshold", 0.8)), b.scalar("EmissiveMaskSoftness", 0.12)))
    emask = lerp(1.0, rm, lerp(b.scalar("NearEmissiveMask", 0.5), b.scalar("FarEmissiveMask", 1.0), fa))
    k = mul(mul(mul(b.scalar("EmissiveStrength", 4.0), glow), emask), lerp(1.0, b.scalar("FarEmissiveBoost", 1.5), fa))
    e_low = mul(mul(low_em, k), mul(low_w, inv(molten)))
    e_val = mul(mul(lerp(Vp("ValleyEdgeColor"), Vp("ValleyCoreColor"), mul(heat, heat)), heat),
                mul(add(0.6, mul((flow, "R"), 0.8)), S("ValleyEmissive")))
    # glints live on the top-projected detail set, so not on cliffs (they would smear down the face)
    glint_w = mul(mul(ramp(hj, S("GlintStart"), S("GlintRange")), ramp((n2, "G"), S("GlintNoiseMin"), 0.3)), inv(rock_w))
    e_gl = mul(Vp("GlintColor"), mul(mul((d_n, "B"), glint_w), S("GlintStrength")))
    e_pk = mul(peak_c, mul(peak_w, S("PeakGlow")))        # bioluminescent frost: readable on the night side
    b.out(add(add(e_low, e_val), add(e_gl, e_pk)), "", MP.MP_EMISSIVE_COLOR)

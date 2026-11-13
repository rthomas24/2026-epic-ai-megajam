# SPDX-License-Identifier: MIT
# M_TD_TerrainR2 graph (Unreal editor Python): the environment pass's planet terrain material (Nov 13), parent of
# MI_Terrain_Verdance / MI_Terrain_Cinder on ATDPlanetTerrain's runtime ProceduralMesh chunks. It replaces
# M_TD_Terrain (r1, kept on disk untouched) and uses Grok Bot's PBRgen sets (ArtSource/EngAI/Terrain) at 1K.
#
# Vertex contract (TDPlanetTerrain.cpp): UV1.x = height in metres; colour G = slope (1 - N.z), B = broad mask
# (Verdance: ~4 km biome fbm; Cinder: the carved river noise, 0.5 on a river's centre line), A = fine mask (~60 m).
#
# What it does differently from r1:
#   * No visible tiling from any altitude. Every layer's colour is  Tint x (texture / texture mean):  the texture only
#     adds detail around 1.0, so when it fades out with distance (near 4 m tile -> a rotated ~29 m tile at 15-75 m ->
#     the plain mean at 0.3-1.5 km) the far colour IS the near colour. All large-scale variation is procedural: one
#     512 px tileable noise sampled at four rotated, incommensurate world scales (37 m, 230 m, 1.9 km, 7.3 km) drives
#     hue (three tints per flat layer), value and roughness, plus the terrain's own non-repeating broad mask.
#   * Layers by slope and height in METRES (jittered by the fine mask and the 37 m noise, shifted by the broad mask):
#     flat (moss / basalt) -> patches (glowing lichen flora / obsidian) -> low wet ground (soil / lava crust) ->
#     cliff rock on slopes (side-projected) -> peak (lichen frost, tinted / ash) on the highest ground only.
#   * Low ground is wet: darker and smoother. A river / pool band follows a contour of the broad mask on low flat
#     ground: still water that catches the sky on Verdance, the lava rivers with a drifting crust on Cinder (r1's maths).
#   * Emissive: thin veins (ridges of the noise; a few metres wide near, ~30 m wide from altitude so they read on the
#     descent), two colours picked by the 7 km noise, on low ground only; the patch layer glows through its texture.
# Normals are output in WORLD space (material.tangent_space_normal = False). All samples use the shared wrap sampler.
import math

import unreal

ST = unreal.MaterialSamplerType
MP = unreal.MaterialProperty

# name: (Verdance, Cinder)
SCALARS = {
    "HeightRangeM": (2900.0, 2900.0),      # set by the terrain code (unused by this graph; kept so the call is valid)
    "TileSize": (400.0, 400.0), "FarScale": (7.3, 7.3),
    "FarStart": (1500.0, 1500.0), "FarRange": (6000.0, 6000.0),          # near tile -> far tile (camera depth, cm)
    "MeanStart": (30000.0, 30000.0), "MeanRange": (120000.0, 120000.0),  # far tile -> plain mean
    "HueKeep": (0.45, 0.6),                # how much of the texture's own hue variation survives the retint
    "DetailContrast": (1.0, 1.0),
    "NoiseS": (3700.0, 3700.0), "NoiseM": (23000.0, 23000.0), "NoiseL": (190000.0, 190000.0), "NoiseXL": (730000.0, 730000.0),
    "ValueVar": (0.55, 0.5), "HueContrast": (2.2, 2.0), "TintCThreshold": (0.68, 0.62), "TintCSoft": (0.18, 0.2),
    "RoughVar": (0.12, 0.1),
    # thresholds (metres / slope) and their breakup
    "HeightBreakM": (140.0, 140.0), "BiomeBreakM": (260.0, 120.0), "SlopeBreak": (0.08, 0.07),
    "RockSlopeStart": (0.17, 0.15), "RockSlopeRange": (0.13, 0.12),
    "PeakStartM": (1450.0, 1300.0), "PeakRangeM": (550.0, 700.0), "PeakSlopeMax": (0.4, 0.36), "PeakSlopeRange": (0.3, 0.28),
    "LowTopM": (-260.0, 10.0), "LowFadeM": (130.0, 120.0), "LowAmount": (0.85, 0.7),
    "PatchThreshold": (0.72, 0.66), "PatchSoft": (0.06, 0.08), "PatchTopM": (700.0, 900.0), "PatchFadeM": (300.0, 300.0),
    "WetDarken": (0.6, 0.85), "WetRoughness": (0.7, 0.6),
    # roughness / specular / normal
    "FlatRoughness": (0.86, 0.9), "PatchRoughness": (0.7, 0.22), "LowRoughness": (0.9, 0.8), "RockRoughness": (0.82, 0.78),
    "PeakRoughness": (0.7, 0.95), "Specular": (0.15, 0.25), "NormalStrength": (1.0, 1.2),
    "RockTile": (1400.0, 1100.0), "RockFarScale": (9.0, 11.0), "RockNormalStrength": (1.1, 1.4),
    # river / pool band
    "ValleyCenter": (0.5, 0.5), "ValleyWidth": (0.035, 0.3), "ValleyBreak": (0.02, 0.012),
    "ValleySlopeMax": (0.05, 0.10), "ValleyTopM": (220.0, 2100.0), "ValleyFadeM": (200.0, 500.0),
    "ValleyMinM": (4.0, 3.0), "ValleyMinFadeM": (30.0, 12.0), "ValleyEmissive": (0.35, 9.0),
    "ValleyHeatGain": (3.0, 2.2), "ValleyCrust": (0.0, 1.15), "ValleyBaseBlend": (1.0, 1.0), "ValleyRoughness": (0.05, 0.35),
    "FlowTile": (9000.0, 6000.0), "FlowX": (0.0, 35.0), "FlowY": (12.0, 60.0),
    # veins and glows
    "VeinSharpNear": (40.0, 34.0), "VeinSharpFar": (6.0, 14.0), "VeinStrength": (0.45, 1.6), "VeinFarBoost": (1.2, 1.8),
    "VeinTopM": (500.0, 250.0), "VeinFadeM": (400.0, 200.0), "PatchGlow": (0.10, 0.0), "PeakGlow": (0.04, 0.0),
    "VeinPatchiness": (1.0, 1.0),
}
VECTORS = {
    "FlatTintA": ((0.012, 0.090, 0.080), (0.012, 0.012, 0.015)),
    "FlatTintB": ((0.020, 0.135, 0.055), (0.030, 0.027, 0.028)),
    "FlatTintC": ((0.010, 0.055, 0.075), (0.050, 0.022, 0.014)),
    "PatchTint": ((0.030, 0.140, 0.150), (0.008, 0.008, 0.012)),
    "PatchGlowColor": ((0.10, 0.85, 1.00), (0.0, 0.0, 0.0)),
    "LowTint": ((0.020, 0.019, 0.016), (0.018, 0.014, 0.013)),
    "RockTintA": ((0.035, 0.040, 0.060), (0.020, 0.015, 0.014)),
    "RockTintB": ((0.095, 0.105, 0.135), (0.070, 0.042, 0.028)),
    "PeakTint": ((0.200, 0.330, 0.310), (0.210, 0.180, 0.160)),
    "ValleyCoreColor": ((0.25, 1.0, 0.9), (1.0, 0.36, 0.04)),
    "ValleyEdgeColor": ((0.05, 0.45, 0.6), (0.80, 0.05, 0.008)),
    "MoltenBase": ((0.004, 0.022, 0.028), (0.12, 0.025, 0.005)),
    "VeinColorA": ((0.05, 0.9, 1.0), (1.0, 0.22, 0.02)),
    "VeinColorB": ((0.38, 0.20, 1.0), (0.9, 0.07, 0.008)),
}
# layer slot -> (Verdance set, Cinder set)
LAYERS = {
    "Flat": ("Verdance_Moss", "Cinder_Basalt"),
    "Patch": ("Verdance_PeakLichenFrost", "Cinder_Obsidian"),
    "Low": ("Verdance_Soil", "Cinder_LavaCrust"),
    "Rock": ("Verdance_CliffRock", "Cinder_CliffRock"),
    "Peak": ("Verdance_PeakLichenFrost", "Cinder_Ash"),
}


def instance_values(g, means):
    i = 0 if g == "Verdance" else 1
    scalars = {k: v[i] for k, v in SCALARS.items()}
    vectors = {k: v[i] for k, v in VECTORS.items()}
    for slot, sets in LAYERS.items():
        vectors[slot + "Mean"] = tuple(means[sets[i]])
    return scalars, vectors


def build_graph(b, t, means):
    """b: ue_mat_builder.MB on an empty material (tangent_space_normal False); t: textures by name (T_Terr_<set>_BC/_N,
    T_Ground_MacroNoise); means: linear mean base colour by set."""
    shared = unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS
    E = unreal

    def P(x):
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
    def dot(a, c): return op(E.MaterialExpressionDotProduct, num(a), num(c))

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

    def rgb(a):
        return mask(a, r=True, g=True, bl=True)

    def S(name):
        return b.scalar(name, SCALARS[name][0])

    def Vp(name):
        if name.endswith("Mean"):
            return rgb(b.vector(name, means[LAYERS[name[:-4]][0]]))
        return rgb(b.vector(name, VECTORS[name][0]))

    def ramp(x, start, rng):
        return sat(div(sub(x, start), rng))

    def tex(name, default, sampler, uv):
        n = b.tex(name, t[default], sampler, uv)
        n.set_editor_property("sampler_source", shared)
        return n

    # ---------------------------------------------------------------- inputs
    vc = b.e(E.MaterialExpressionVertexColor)
    slope, broad, fine = (vc, "G"), (vc, "B"), (vc, "A")
    uv1 = b.e(E.MaterialExpressionTextureCoordinate, coordinate_index=1)
    hm = mask(uv1, r=True)
    wp = b.e(E.MaterialExpressionWorldPosition)
    xy = mask(wp, r=True, g=True)
    wx, wy, wz = mask(wp, r=True), mask(wp, g=True), mask(wp, bl=True)
    nv = b.e(E.MaterialExpressionVertexNormalWS)
    tile = S("TileSize")
    depth = b.e(E.MaterialExpressionPixelDepth)
    fa = ramp(depth, S("FarStart"), S("FarRange"))
    fm = ramp(depth, S("MeanStart"), S("MeanRange"))

    def rot_uv(angle, size, off=(0.0, 0.0)):
        a = math.radians(angle)
        u = dot(xy, b.const2(math.cos(a), -math.sin(a)))
        v = dot(xy, b.const2(math.sin(a), math.cos(a)))
        return add(div(app(u, v), size), b.const2(*off))

    # ---------------------------------------------------------------- noise at four world scales
    def noise(angle, size_name, off):
        return tex("MacroNoiseTex", "T_Ground_MacroNoise", ST.SAMPLERTYPE_MASKS, rot_uv(angle, S(size_name), off))
    nS, nM = noise(17.0, "NoiseS", (0.11, 0.37)), noise(-51.0, "NoiseM", (0.53, 0.19))
    nL, nXL = noise(23.0, "NoiseL", (0.71, 0.83)), noise(71.0, "NoiseXL", (0.29, 0.61))

    def c(n, ch):
        return (n, ch)

    val = add(1.0, mul(S("ValueVar"), add(add(mul(sub(c(nM, "R"), 0.5), 0.5), mul(sub(c(nL, "R"), 0.5), 0.9)),
                                          mul(sub(c(nXL, "G"), 0.5), 0.6))))
    hsel = sat(add(0.5, mul(S("HueContrast"), add(add(mul(sub(c(nL, "G"), 0.5), 0.6), mul(sub(c(nXL, "R"), 0.5), 0.5)),
                                                  mul(sub(c(nM, "G"), 0.5), 0.2)))))
    csel = ramp(add(mul(c(nXL, "B"), 0.6), mul(c(nL, "B"), 0.4)), S("TintCThreshold"), S("TintCSoft"))

    # ---------------------------------------------------------------- layer weights
    j = add(mul(sub(fine, 0.5), 0.6), mul(sub(c(nS, "G"), 0.5), 0.4))            # -0.5 .. 0.5 breakup
    hj = add(add(hm, mul(j, S("HeightBreakM"))), mul(sub(broad, 0.5), S("BiomeBreakM")))
    sj = add(slope, mul(j, S("SlopeBreak")))
    rock_w = ramp(sj, S("RockSlopeStart"), S("RockSlopeRange"))
    peak_w = mul(ramp(hj, S("PeakStartM"), S("PeakRangeM")), inv(ramp(slope, S("PeakSlopeMax"), S("PeakSlopeRange"))))
    low_w = mul(inv(ramp(hj, S("LowTopM"), S("LowFadeM"))), S("LowAmount"))
    patch_n = add(add(mul(c(nM, "B"), 0.55), mul(c(nL, "R"), 0.45)), mul(j, 0.12))
    patch_w = mul(ramp(patch_n, S("PatchThreshold"), S("PatchSoft")), inv(ramp(hj, S("PatchTopM"), S("PatchFadeM"))))
    band = sat(inv(div(vabs(sub(add(broad, mul(j, S("ValleyBreak"))), S("ValleyCenter"))), S("ValleyWidth"))))
    valley_w = mul(mul(band, inv(ramp(slope, 0.0, S("ValleySlopeMax")))),
                   mul(inv(ramp(hm, S("ValleyTopM"), S("ValleyFadeM"))), ramp(vabs(hm), S("ValleyMinM"), S("ValleyMinFadeM"))))

    # ---------------------------------------------------------------- top-projected layers: detail around 1.0 + normal
    uvn = div(xy, tile)
    uvf = rot_uv(33.0, mul(tile, S("FarScale")), (0.17, 0.71))
    lumw = b.const3(0.3, 0.59, 0.11)

    def detail(sample_near, sample_far, slot):
        d = div(lerp(rgb(sample_near), rgb(sample_far), fa), Vp(slot + "Mean"))
        d = lerp(dot(d, lumw), d, S("HueKeep"))
        d = add(mul(sub(d, 1.0), S("DetailContrast")), 1.0)
        return lerp(d, 1.0, fm)

    def top(slot):
        bc, nr = "T_Terr_%s_BC" % LAYERS[slot][0], "T_Terr_%s_N" % LAYERS[slot][0]
        d = detail(tex(slot + "BC", bc, ST.SAMPLERTYPE_COLOR, uvn), tex(slot + "BC", bc, ST.SAMPLERTYPE_COLOR, uvf), slot)
        return d, rgb(tex(slot + "N", nr, ST.SAMPLERTYPE_NORMAL, uvn))
    flat_d, flat_n = top("Flat")
    patch_d, patch_n_ = top("Patch")
    low_d, low_n = top("Low")
    peak_d, peak_n = top("Peak")
    flat_c = mul(lerp(lerp(Vp("FlatTintA"), Vp("FlatTintB"), hsel), Vp("FlatTintC"), csel), flat_d)
    patch_c = mul(Vp("PatchTint"), patch_d)
    low_c = mul(Vp("LowTint"), low_d)
    peak_c = mul(Vp("PeakTint"), peak_d)

    # ---------------------------------------------------------------- rock (side projections picked by the normal)
    nx2 = mul(mask(nv, r=True), mask(nv, r=True))
    ny2 = mul(mask(nv, g=True), mask(nv, g=True))
    wsel = div(nx2, add(add(nx2, ny2), 0.0001))           # 1 = the face looks along X
    rtile = S("RockTile")
    nz = mul(wz, -1.0)
    uv_x, uv_y = div(app(wy, nz), rtile), div(app(wx, nz), rtile)
    rfar = mul(rtile, S("RockFarScale"))
    uvf_x, uvf_y = div(app(wy, nz), rfar), div(app(wx, nz), rfar)
    rbc, rnr = "T_Terr_%s_BC" % LAYERS["Rock"][0], "T_Terr_%s_N" % LAYERS["Rock"][0]
    r_near = lerp(rgb(tex("RockBC", rbc, ST.SAMPLERTYPE_COLOR, uv_y)), rgb(tex("RockBC", rbc, ST.SAMPLERTYPE_COLOR, uv_x)), wsel)
    r_far = lerp(rgb(tex("RockBC", rbc, ST.SAMPLERTYPE_COLOR, uvf_y)), rgb(tex("RockBC", rbc, ST.SAMPLERTYPE_COLOR, uvf_x)), wsel)
    r_nrm = lerp(rgb(tex("RockN", rnr, ST.SAMPLERTYPE_NORMAL, uv_y)), rgb(tex("RockN", rnr, ST.SAMPLERTYPE_NORMAL, uv_x)), wsel)
    rock_d = detail(r_near, r_far, "Rock")
    rock_c = mul(lerp(Vp("RockTintA"), Vp("RockTintB"), sat(add(mul(sub(c(nM, "R"), 0.5), 1.6), 0.5))), rock_d)

    # ---------------------------------------------------------------- river / pool band: flow, heat
    time = b.e(E.MaterialExpressionTime)
    flow = tex("MacroNoiseTex", "T_Ground_MacroNoise", ST.SAMPLERTYPE_MASKS,
               div(sub(xy, mul(app(S("FlowX"), S("FlowY")), time)), S("FlowTile")))
    heat = sat(sub(mul(valley_w, S("ValleyHeatGain")), mul(c(flow, "G"), S("ValleyCrust"))))
    molten = mul(heat, S("ValleyBaseBlend"))

    # ---------------------------------------------------------------- compose
    wet = mul(low_w, inv(rock_w))
    base = lerp(flat_c, patch_c, patch_w)
    base = lerp(base, low_c, low_w)
    base = mul(base, lerp(1.0, S("WetDarken"), wet))
    base = lerp(lerp(base, rock_c, rock_w), peak_c, peak_w)
    base = lerp(mul(base, val), Vp("MoltenBase"), molten)
    rough = lerp(lerp(S("FlatRoughness"), S("PatchRoughness"), patch_w), S("LowRoughness"), low_w)
    rough = lerp(rough, S("WetRoughness"), mul(wet, c(nS, "R")))
    rough = lerp(lerp(rough, S("RockRoughness"), rock_w), S("PeakRoughness"), peak_w)
    rough = add(rough, mul(sub(c(nM, "R"), 0.5), S("RoughVar")))
    b.out(base, "", MP.MP_BASE_COLOR)
    b.out(sat(lerp(rough, S("ValleyRoughness"), molten)), "", MP.MP_ROUGHNESS)
    b.out(K(0.0), "", MP.MP_METALLIC)
    b.out(mul(S("Specular"), lerp(1.0, 0.2, fa)), "", MP.MP_SPECULAR)   # no grazing sheen on far flats

    # world-space normal: vertex normal * tangent z + the projected tangent xy; detail fades with distance and on water
    top_n = lerp(lerp(lerp(flat_n, patch_n_, patch_w), low_n, low_w), peak_n, peak_w)
    keep = mul(mul(lerp(1.0, 0.35, fa), inv(fm)), inv(molten))
    p_top = mul(app(mask(top_n, r=True, g=True), 0.0), mul(S("NormalStrength"), keep))
    rs = mul(S("RockNormalStrength"), mul(lerp(1.0, 0.35, fa), inv(fm)))
    rx = mul(mask(r_nrm, r=True), rs)
    p_rock = app(app(mul(rx, inv(wsel)), mul(rx, wsel)), mul(mask(r_nrm, g=True), mul(rs, -1.0)))
    b.out(b.norm(add(nv, lerp(p_top, p_rock, rock_w))), "", MP.MP_NORMAL)

    # ---------------------------------------------------------------- emissive
    def ridge(v, sharp):
        r = sat(inv(mul(vabs(sub(v, 0.5)), sharp)))
        return mul(r, r)
    vein_near = ridge(c(nS, "B"), S("VeinSharpNear"))
    vein_far = mul(ridge(add(mul(c(nM, "B"), 0.6), mul(c(nL, "B"), 0.4)), S("VeinSharpFar")), S("VeinFarBoost"))
    vein = add(mul(vein_near, inv(ramp(depth, 4000.0, 12000.0))), mul(vein_far, lerp(0.1, 1.0, fm)))   # thin veins only within ~150 m
    # only in some regions (the 230 m and 1.9 km noise), never on rock, peaks, water or high ground
    region = lerp(1.0, sat(mul(sub(add(mul(c(nL, "G"), 0.6), mul(c(nM, "R"), 0.4)), 0.42), 5.0)), S("VeinPatchiness"))
    vmask = mul(mul(mul(inv(rock_w), inv(peak_w)), mul(inv(molten), region)), inv(ramp(hj, S("VeinTopM"), S("VeinFadeM"))))
    vcol = lerp(Vp("VeinColorA"), Vp("VeinColorB"), sat(mul(sub(c(nXL, "B"), 0.45), 4.0)))
    e_vein = mul(vcol, mul(mul(vein, vmask), mul(S("VeinStrength"), add(0.55, mul(c(nS, "R"), 0.9)))))
    plum = mul(dot(patch_d, lumw), add(0.4, mul(c(nS, "R"), 1.2)))   # the texture near, the 37 m noise far: never a flat glow
    e_patch = mul(Vp("PatchGlowColor"), mul(mul(patch_w, mul(plum, plum)), mul(S("PatchGlow"), mul(inv(rock_w), inv(peak_w)))))
    e_val = mul(mul(lerp(Vp("ValleyEdgeColor"), Vp("ValleyCoreColor"), mul(heat, heat)), heat),
                mul(add(0.6, mul(c(flow, "R"), 0.8)), S("ValleyEmissive")))
    e_pk = mul(peak_c, mul(peak_w, S("PeakGlow")))
    b.out(add(add(e_vein, e_patch), add(e_val, e_pk)), "", MP.MP_EMISSIVE_COLOR)

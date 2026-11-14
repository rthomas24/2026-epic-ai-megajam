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
    # R3: the far tile is gone by ~550 m (it read as a knitted grid from the air); beyond that, colour is procedural only
    "MeanStart": (12000.0, 12000.0), "MeanRange": (45000.0, 45000.0),    # far tile -> plain mean
    # R3: distance at which the small noises (37 m / 230 m, which repeat) hand over to the large ones and the terrain's
    # own non-repeating masks (broad ~4 km, fine ~60 m per vertex)
    "SmallFadeStart": (35000.0, 35000.0), "SmallFadeRange": (150000.0, 150000.0),
    # R3: biome regions readable from 1-5 km (Verdance: dark rainforest floor where the scatter puts rainforest
    # (broad > 0.55), moss in the mixed band, open bright meadow below 0.4; uplands paler; lowlands dark and wet)
    "BroadHue": (2.2, 0.3), "FarVar": (0.7, 0.6),
    "UplandStartM": (420.0, 500.0), "UplandRangeM": (520.0, 500.0), "UplandAmount": (0.8, 0.55),
    "LowlandTopM": (60.0, 40.0), "LowlandFadeM": (160.0, 120.0), "LowlandAmount": (0.75, 0.5),
    "JungleBroad": (0.475, 2.0), "JungleSoft": (0.08, 0.1), "JungleAmount": (0.85, 0.0),
    "StrataScaleM": (90.0, 70.0), "StrataAlongM": (700.0, 600.0), "StrataContrast": (0.75, 1.1), "TriTop": (1.5, 1.5),
    "HueKeep": (0.12, 0.45),               # R3: the moss mean has almost no blue, so its blue-ish pixels turned into cyan cells near the pad                # how much of the texture's own hue variation survives the retint
    "DetailContrast": (1.0, 1.0),
    "MacroTileA": (6000.0, 6000.0), "MacroTileB": (24000.0, 24000.0), "MacroMixStart": (40000.0, 40000.0),
    "MacroMixRange": (80000.0, 80000.0), "MacroContrast": (0.55, 0.45), "MacroFarStart": (180000.0, 180000.0),
    "MacroFarRange": (150000.0, 150000.0), "MacroNormal": (0.6, 0.6),
    "NoiseS": (3700.0, 3700.0), "NoiseM": (23000.0, 23000.0), "NoiseL": (190000.0, 190000.0), "NoiseXL": (730000.0, 730000.0),
    "ValueVar": (0.55, 0.5), "HueContrast": (2.2, 2.0), "TintCThreshold": (0.68, 0.62), "TintCSoft": (0.18, 0.2),
    "RoughVar": (0.12, 0.1),
    # thresholds (metres / slope) and their breakup
    "HeightBreakM": (140.0, 140.0), "BiomeBreakM": (260.0, 120.0), "SlopeBreak": (0.14, 0.07),
    "RockSlopeStart": (0.21, 0.15), "RockSlopeRange": (0.2, 0.12),     # s6/s7: broken cliff foot; s7 moss up to ~38 deg (hills were bare brown cones)
    "PeakStartM": (1450.0, 1300.0), "PeakRangeM": (550.0, 700.0), "PeakSlopeMax": (0.4, 0.36), "PeakSlopeRange": (0.3, 0.28),
    "LowTopM": (-260.0, 10.0), "LowFadeM": (130.0, 120.0), "LowAmount": (0.85, 0.7),
    # scatter v2: the patch (Verdance grove / Cinder obsidian) is a rule on the terrain's own masks, the same numbers as
    # FTDScatterPlan::GroveBroadMax / GroveFineMin / ObsidianFineMin (TDPlanetScatter.h), so groves and crystals stand on it
    "PatchFineMin": (0.62, 0.7), "PatchBroadMax": (0.3, 2.0), "PatchSoft": (0.04, 0.04), "PatchTopM": (700.0, 900.0), "PatchFadeM": (300.0, 300.0),
    "WetDarken": (0.6, 0.85), "WetRoughness": (0.7, 0.6),
    # roughness / specular / normal
    "FlatRoughness": (0.86, 0.9), "PatchRoughness": (0.7, 0.22), "LowRoughness": (0.9, 0.8), "RockRoughness": (0.82, 0.78),
    "PeakRoughness": (0.7, 0.95), "Specular": (0.15, 0.25), "NormalStrength": (1.0, 1.2),
    "RockTile": (900.0, 1100.0), "RockFarScale": (4.0, 11.0), "RockNormalStrength": (1.1, 1.4),   # s6: Verdance cliffs were blurry sheets from the pad
    # river / pool band
    "ValleyCenter": (0.5, 0.5), "ValleyWidth": (0.035, 0.3), "ValleyBreak": (0.02, 0.012),
    "ValleySlopeMax": (0.05, 0.10), "ValleyTopM": (220.0, 2100.0), "ValleyFadeM": (200.0, 500.0),
    "ValleyMinM": (4.0, 3.0), "ValleyMinFadeM": (30.0, 12.0), "ValleyEmissive": (0.35, 9.0),
    "ValleyHeatGain": (3.0, 2.2), "ValleyCrust": (0.0, 1.15), "ValleyBaseBlend": (1.0, 1.0), "ValleyRoughness": (0.05, 0.35),
    "FlowTile": (9000.0, 6000.0), "FlowX": (0.0, 35.0), "FlowY": (12.0, 60.0),
    # veins and glows
    "VeinSharpNear": (40.0, 34.0), "VeinSharpFar": (9.0, 16.0), "VeinStrength": (0.45, 1.6), "VeinFarBoost": (0.55, 3.0),   # Cinder s9: a crack network read from 900 m; s10 sharper (9 smeared into orange blobs at 120-300 m)
    "VeinTopM": (500.0, 600.0), "VeinFadeM": (400.0, 300.0), "PatchGlow": (0.10, 0.0), "PeakGlow": (0.04, 0.0),   # Cinder s9: veins over more ground
    "VeinPatchiness": (1.0, 1.0),
    # pass 4: a minimum ambient (emissive = albedo x floor x the sky's colour) so Cinder's self-shadowed basalt slopes
    # never go to black under the low sun; proportional to the albedo, so nothing lights up on its own. Off on Verdance.
    "AmbientFloor": (0.0, 0.45),
    # s7: biome patches at 200-500 m (meadow / moss / dark forest floor) so the ground from the air is not one green noise
    "MacroBiomeScale": (42000.0, 42000.0), "MacroBiomeAmount": (0.85, 0.85),   # s9: Cinder was flat dark ground from the air
    # s7: a dark wet-rock band where the rock meets the ground (cliff feet, seeps)
    "WetRockDark": (0.45, 0.7),
    # s7 story: Verdance's wreck scar (the same segment as FTDScatterPlan::ScarA/B, course-local = world on the planet
    # maps): charred black ground with ember specks, narrow at the research site, wide outward. Off on Cinder.
    # s9: it starts beyond the basin wall (on the wall behind the research site it stood up as a glowing pillar), wider
    "ScarAX": (24000.0, 0.0), "ScarAY": (15500.0, 0.0), "ScarBX": (60000.0, 0.0), "ScarBY": (43000.0, 0.0),
    "ScarHalfA": (2500.0, 0.0), "ScarHalfB": (9000.0, 0.0), "ScarEmber": (6.0, 0.0),
}
VECTORS = {
    # R3 (Verdance): A = teal moss, B = bright yellow-green moss (open meadow side), C = blue-teal glade
    "FlatTintA": ((0.014, 0.085, 0.060), (0.012, 0.012, 0.015)),
    "FlatTintB": ((0.050, 0.150, 0.030), (0.080, 0.072, 0.066)),     # Cinder s9: pale ash flats
    "FlatTintC": ((0.012, 0.060, 0.090), (0.075, 0.032, 0.016)),     # Cinder s9: rust-stained ground
    "JungleTint": ((0.008, 0.042, 0.026), (0.007, 0.0065, 0.0075)),   # Verdance: under the canopy; Cinder s9: black flows
    "UplandTint": ((0.075, 0.110, 0.070), (0.060, 0.045, 0.040)),     # paler sage / ash-brown uplands
    "LowlandTint": ((0.012, 0.035, 0.032), (0.016, 0.013, 0.012)),    # wet dark lowlands
    "PatchTint": ((0.016, 0.060, 0.075), (0.008, 0.008, 0.012)),
    "PatchGlowColor": ((0.10, 0.85, 1.00), (0.0, 0.0, 0.0)),
    "LowTint": ((0.020, 0.019, 0.016), (0.018, 0.014, 0.013)),
    "RockTintA": ((0.040, 0.046, 0.050), (0.024, 0.020, 0.020)),
    "RockTintB": ((0.100, 0.110, 0.112), (0.085, 0.050, 0.032)),
    "PeakTint": ((0.200, 0.330, 0.310), (0.210, 0.180, 0.160)),
    "ValleyCoreColor": ((0.25, 1.0, 0.9), (1.0, 0.36, 0.04)),
    "ValleyEdgeColor": ((0.05, 0.45, 0.6), (0.80, 0.05, 0.008)),
    "MoltenBase": ((0.004, 0.022, 0.028), (0.12, 0.025, 0.005)),
    "VeinColorA": ((0.05, 0.9, 1.0), (1.0, 0.22, 0.02)),
    "VeinColorB": ((0.38, 0.20, 1.0), (0.9, 0.07, 0.008)),
    "AmbientTint": ((0.6, 0.9, 1.0), (1.0, 0.62, 0.48)),             # the sky colour (Cinder: smoky ember haze)
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

    # R3: the 37 m and 230 m noises repeat, so they only act near (sn = 1 near, 0 far); far away the variation comes from
    # the 1.9 km / 7.3 km noises and the terrain's own masks, which never repeat.
    sn = inv(ramp(depth, S("SmallFadeStart"), S("SmallFadeRange")))
    near_v = add(mul(sub(c(nM, "R"), 0.5), 0.5), mul(sub(fine, 0.5), 0.3))
    far_v = add(add(mul(sub(fine, 0.5), S("FarVar")), mul(sub(c(nXL, "G"), 0.5), 0.6)), mul(sub(broad, 0.5), 0.3))
    val = add(1.0, mul(S("ValueVar"), add(lerp(far_v, near_v, sn), mul(sub(c(nL, "R"), 0.5), 0.9))))
    hsel = sat(add(0.5, mul(S("HueContrast"), add(add(add(mul(sub(c(nL, "G"), 0.5), 0.45), mul(sub(c(nXL, "R"), 0.5), 0.4)),
                                                      mul(mul(sub(c(nM, "G"), 0.5), 0.2), sn)),
                                                  mul(sub(0.5, broad), mul(S("BroadHue"), 0.25))))))
    csel = ramp(add(mul(c(nXL, "B"), 0.6), mul(c(nL, "B"), 0.4)), S("TintCThreshold"), S("TintCSoft"))

    # ---------------------------------------------------------------- layer weights
    j = add(mul(sub(fine, 0.5), 0.6), mul(mul(sub(c(nS, "G"), 0.5), 0.4), sn))   # -0.5 .. 0.5 breakup
    hj = add(add(hm, mul(j, S("HeightBreakM"))), mul(sub(broad, 0.5), S("BiomeBreakM")))
    # s8: the vertex colour's slope comes from the field at the chunk's step, so the basin's steep walls read as gentle
    # and got the flat (top-projected) moss stretched down them: take the steeper of it and the mesh normal's slope
    nslope = inv(mask(nv, bl=True))
    sj = add(vmax(slope, nslope), mul(j, S("SlopeBreak")))
    rock_w = ramp(sj, S("RockSlopeStart"), S("RockSlopeRange"))
    peak_w = mul(ramp(hj, S("PeakStartM"), S("PeakRangeM")), inv(ramp(slope, S("PeakSlopeMax"), S("PeakSlopeRange"))))
    low_w = mul(inv(ramp(hj, S("LowTopM"), S("LowFadeM"))), S("LowAmount"))
    # R3 biome regions on the flats (same inputs as the scatter: broad mask, height above the pad)
    jungle_w = mul(ramp(add(broad, mul(j, 0.08)), S("JungleBroad"), S("JungleSoft")), S("JungleAmount"))
    upland_w = mul(ramp(hj, S("UplandStartM"), S("UplandRangeM")), S("UplandAmount"))
    lowland_w = mul(inv(ramp(hj, S("LowlandTopM"), S("LowlandFadeM"))), S("LowlandAmount"))
    # glowing groves: R3 uses the 1.9 km and 7.3 km noises (patches of a few hundred metres, not 230 m blobs)
    ps = S("PatchSoft")
    fine_ok = ramp(add(fine, mul(sub(c(nS, "G"), 0.5), mul(sn, 0.03))), sub(S("PatchFineMin"), mul(ps, 0.5)), ps)
    broad_ok = inv(ramp(broad, sub(S("PatchBroadMax"), mul(ps, 0.5)), ps))
    patch_w = mul(mul(fine_ok, broad_ok), inv(ramp(hj, S("PatchTopM"), S("PatchFadeM"))))
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
    # R3 pass 4 (flight distances): where the near / far texture tiles have faded to the plain mean (beyond ~0.5 km), the
    # flat layer gets a macro tile instead: the same moss set at two big rotated scales (60 m and 240 m) cross-faded by
    # camera distance, luminance only, so the ground keeps crisp structure from 120 m to ~2 km; gone by MacroFarEnd.
    fbc, fnr = "T_Terr_%s_BC" % LAYERS["Flat"][0], "T_Terr_%s_N" % LAYERS["Flat"][0]
    uvm1 = rot_uv(11.0, S("MacroTileA"), (0.41, 0.13))
    uvm2 = rot_uv(-37.0, S("MacroTileB"), (0.07, 0.59))
    mmix = ramp(depth, S("MacroMixStart"), S("MacroMixRange"))
    mlum = div(dot(lerp(rgb(tex("FlatBC", fbc, ST.SAMPLERTYPE_COLOR, uvm1)), rgb(tex("FlatBC", fbc, ST.SAMPLERTYPE_COLOR, uvm2)), mmix), lumw),
               dot(Vp("FlatMean"), lumw))
    mfar = inv(ramp(depth, S("MacroFarStart"), S("MacroFarRange")))
    macro_d = add(1.0, mul(sub(mlum, 1.0), mul(S("MacroContrast"), mfar)))
    flat_d = lerp(flat_d, macro_d, fm)        # flat_d was exactly 1 where fm = 1
    macro_n = rgb(tex("FlatN", fnr, ST.SAMPLERTYPE_NORMAL, uvm1))
    patch_d, patch_n_ = top("Patch")
    low_d, low_n = top("Low")
    peak_d, peak_n = top("Peak")
    flat_t = lerp(lerp(Vp("FlatTintA"), Vp("FlatTintB"), hsel), Vp("FlatTintC"), csel)
    flat_t = lerp(flat_t, Vp("JungleTint"), jungle_w)
    flat_t = lerp(flat_t, Vp("UplandTint"), upland_w)
    flat_t = lerp(flat_t, Vp("LowlandTint"), lowland_w)
    # s7: 200-500 m biome patches, at every distance (the 37 m / 230 m noises only act near)
    nB = noise(-13.0, "MacroBiomeScale", (0.37, 0.91))
    mb = S("MacroBiomeAmount")
    flat_t = lerp(flat_t, Vp("JungleTint"), mul(ramp(c(nB, "R"), 0.56, 0.12), mb))
    flat_t = lerp(flat_t, Vp("FlatTintB"), mul(inv(ramp(c(nB, "G"), 0.3, 0.12)), mul(mb, 0.7)))
    flat_t = lerp(flat_t, Vp("FlatTintC"), mul(ramp(c(nB, "B"), 0.62, 0.1), mul(mb, 0.6)))
    flat_c = mul(flat_t, flat_d)
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
    # R3 pass 3: triplanar - the top projection joins the two side ones by the squared normal, so the 25-45 degree
    # slopes (between "flat" and "cliff") no longer show the side projections stretched into streaks
    nz2 = mul(mask(nv, bl=True), mask(nv, bl=True))
    wtop = sat(div(mul(nz2, S("TriTop")), add(add(nx2, ny2), add(mul(nz2, S("TriTop")), 0.0001))))
    uv_z, uvf_z = div(xy, rtile), div(xy, rfar)
    r_near = lerp(lerp(rgb(tex("RockBC", rbc, ST.SAMPLERTYPE_COLOR, uv_y)), rgb(tex("RockBC", rbc, ST.SAMPLERTYPE_COLOR, uv_x)), wsel),
                  rgb(tex("RockBC", rbc, ST.SAMPLERTYPE_COLOR, uv_z)), wtop)
    r_far = lerp(lerp(rgb(tex("RockBC", rbc, ST.SAMPLERTYPE_COLOR, uvf_y)), rgb(tex("RockBC", rbc, ST.SAMPLERTYPE_COLOR, uvf_x)), wsel),
                 rgb(tex("RockBC", rbc, ST.SAMPLERTYPE_COLOR, uvf_z)), wtop)
    r_nrm = lerp(rgb(tex("RockN", rnr, ST.SAMPLERTYPE_NORMAL, uv_y)), rgb(tex("RockN", rnr, ST.SAMPLERTYPE_NORMAL, uv_x)), wsel)
    rock_d = detail(r_near, r_far, "Rock")
    # R3: cliff colour from horizontal strata (a noise stretched along the ground, squeezed in height) instead of the
    # top-down 230 m noise, which smeared down every slope into stripes
    along = mul(add(wx, wy), 0.7071)
    uv_st = app(div(along, mul(S("StrataAlongM"), 100.0)), div(wz, mul(S("StrataScaleM"), 100.0)))
    strata = tex("MacroNoiseTex", "T_Ground_MacroNoise", ST.SAMPLERTYPE_MASKS, uv_st)
    rsel = sat(add(mul(sub(add(mul(c(strata, "R"), 0.7), mul(c(nL, "G"), 0.3)), 0.5), mul(S("StrataContrast"), inv(wtop))), 0.5))
    rock_c = mul(lerp(Vp("RockTintA"), Vp("RockTintB"), rsel), rock_d)
    # s7: wet dark band at the rock's foot (where the slope just turned to rock), wider where the fine mask is low
    wetband = sat(mul(rock_w, mul(inv(rock_w), 4.0)))
    rock_c = mul(rock_c, lerp(1.0, S("WetRockDark"), wetband))

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
    # s7 story: the wreck scar (distance to the segment, a ragged edge from the 37 m / 230 m noises). Course-local: the
    # terrain actor sits at the course origin (rotated with it), so the segment is taken in the mesh's local space.
    tpos = b.e(E.MaterialExpressionTransformPosition)
    tpos.set_editor_property("transform_source_type", E.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD)
    tpos.set_editor_property("transform_type", E.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL)
    b.c(wp, "", tpos, "")
    lxy = mask(tpos, r=True, g=True)
    sa = app(S("ScarAX"), S("ScarAY"))
    sd = sub(app(S("ScarBX"), S("ScarBY")), sa)
    st = sat(div(dot(sub(lxy, sa), sd), add(dot(sd, sd), 1.0)))
    sv = sub(lxy, add(sa, mul(sd, st)))
    sdist = un(E.MaterialExpressionSquareRoot, dot(sv, sv))
    shalf = mul(lerp(S("ScarHalfA"), S("ScarHalfB"), st), add(0.75, mul(add(mul(c(nS, "R"), 0.4), mul(c(nM, "G"), 0.6)), 0.5)))
    scar_w = mul(sat(div(sub(shalf, sdist), mul(shalf, 0.6))), mul(ramp(st, 0.0, 0.06), inv(ramp(st, 0.9, 0.1))))   # s11: soft edges into the forest
    # s10: only on the ground, never up a wall (on the basin wall behind the research site it stood up as a glowing pillar)
    scar_w = mul(scar_w, inv(ramp(sj, 0.24, 0.12)))
    # s11: burnt ground, not a slab: black char and grey ash patches by two noises, the char strongest on the centre line
    ash = ramp(add(mul(c(nM, "B"), 0.6), mul(c(nS, "G"), 0.4)), 0.55, 0.15)
    burnt = lerp(mul(b.const3(0.012, 0.010, 0.009), add(0.5, mul(c(nS, "B"), 1.0))), b.const3(0.09, 0.085, 0.08), mul(ash, 0.7))
    base = lerp(base, burnt, mul(un(E.MaterialExpressionSquareRoot, scar_w), 0.95))
    rough = lerp(lerp(S("FlatRoughness"), S("PatchRoughness"), patch_w), S("LowRoughness"), low_w)
    rough = lerp(rough, S("WetRoughness"), mul(wet, c(nS, "R")))
    rough = lerp(lerp(rough, S("RockRoughness"), rock_w), S("PeakRoughness"), peak_w)
    rough = add(rough, mul(sub(lerp(fine, c(nM, "R"), sn), 0.5), S("RoughVar")))
    b.out(base, "", MP.MP_BASE_COLOR)
    b.out(sat(lerp(rough, S("ValleyRoughness"), molten)), "", MP.MP_ROUGHNESS)
    b.out(K(0.0), "", MP.MP_METALLIC)
    b.out(mul(S("Specular"), lerp(1.0, 0.2, fa)), "", MP.MP_SPECULAR)   # no grazing sheen on far flats

    # world-space normal: vertex normal * tangent z + the projected tangent xy; detail fades with distance and on water
    top_n = lerp(lerp(lerp(flat_n, patch_n_, patch_w), low_n, low_w), peak_n, peak_w)
    keep = mul(mul(lerp(1.0, 0.35, fa), inv(fm)), inv(molten))
    p_top = add(mul(app(mask(top_n, r=True, g=True), 0.0), mul(S("NormalStrength"), keep)),
                mul(app(mask(macro_n, r=True, g=True), 0.0), mul(mul(S("MacroNormal"), fm), mul(mfar, inv(molten)))))
    rs = mul(mul(S("RockNormalStrength"), mul(lerp(1.0, 0.35, fa), inv(fm))), inv(wtop))
    rx = mul(mask(r_nrm, r=True), rs)
    p_rock = app(app(mul(rx, inv(wsel)), mul(rx, wsel)), mul(mask(r_nrm, g=True), mul(rs, -1.0)))
    b.out(b.norm(add(nv, lerp(p_top, p_rock, rock_w))), "", MP.MP_NORMAL)

    # ---------------------------------------------------------------- emissive
    def ridge(v, sharp):
        r = sat(inv(mul(vabs(sub(v, 0.5)), sharp)))
        return mul(r, r)
    vein_near = ridge(c(nS, "B"), S("VeinSharpNear"))
    vein_far = mul(ridge(add(mul(c(nL, "B"), 0.7), mul(c(nXL, "G"), 0.3)), S("VeinSharpFar")), S("VeinFarBoost"))   # R3: 1.9 km scale
    vein = add(mul(vein_near, inv(ramp(depth, 4000.0, 12000.0))), mul(vein_far, lerp(0.1, 1.0, fm)))   # thin veins only within ~150 m
    # only in some regions (the 230 m and 1.9 km noise), never on rock, peaks, water or high ground
    region = lerp(1.0, sat(mul(sub(add(mul(c(nL, "G"), 0.6), mul(lerp(c(nXL, "R"), c(nM, "R"), sn), 0.4)), 0.42), 5.0)), S("VeinPatchiness"))
    vmask = mul(mul(mul(inv(rock_w), inv(peak_w)), mul(inv(molten), region)), inv(ramp(hj, S("VeinTopM"), S("VeinFadeM"))))
    vmask = mul(vmask, inv(ramp(sj, 0.08, 0.08)))   # s11: no glowing veins on slopes (they read as orange blobs on the hillsides)
    vcol = lerp(Vp("VeinColorA"), Vp("VeinColorB"), sat(mul(sub(c(nXL, "B"), 0.45), 4.0)))
    e_vein = mul(vcol, mul(mul(vein, vmask), mul(S("VeinStrength"), add(0.55, mul(c(nS, "R"), 0.9)))))
    plum = mul(dot(patch_d, lumw), add(0.4, mul(c(nS, "R"), 1.2)))   # the texture near, the 37 m noise far: never a flat glow
    e_patch = mul(Vp("PatchGlowColor"), mul(mul(patch_w, mul(plum, plum)), mul(S("PatchGlow"), mul(inv(rock_w), inv(peak_w)))))
    e_val = mul(mul(lerp(Vp("ValleyEdgeColor"), Vp("ValleyCoreColor"), mul(heat, heat)), heat),
                mul(add(0.6, mul(c(flow, "R"), 0.8)), S("ValleyEmissive")))
    e_pk = mul(peak_c, mul(peak_w, S("PeakGlow")))
    e_amb = mul(mul(base, Vp("AmbientTint")), mul(S("AmbientFloor"), inv(molten)))
    # s11: irregular embers: two noises at different scales, sparse at the edges (scar_w squared), none in the ash
    ember = mul(sat(mul(sub(add(mul(c(nS, "B"), 0.55), mul(c(nM, "R"), 0.45)), 0.74), 9.0)), mul(mul(mul(scar_w, scar_w), inv(ash)), S("ScarEmber")))
    e_amb = add(e_amb, mul(b.const3(1.0, 0.28, 0.04), ember))
    b.out(add(add(add(e_vein, e_patch), add(e_val, e_pk)), e_amb), "", MP.MP_EMISSIVE_COLOR)

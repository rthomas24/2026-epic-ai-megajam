# M_TD_Ground graph (Unreal editor Python), shared by ue_import_planets.py (full kit build) and
# ue_ground_rebuild.py (in-place rebuild of the ground master only).
#
# Anti-tiling design (the 4 m texture set showed a checkerboard from the 100 m arrival altitude):
#   * near layer (eye height): the 4 m tile (A) and a 1.37x offset copy (B), blended by a ~18 m world noise
#   * far layer (camera depth FarStart..FarStart+FarRange): the same textures at FarScale (~21 m) and
#     FarScale*Far2Ratio (~50 m), blended by a ~45 m world noise; the far normal is flattened
#   * macro layer: a tileable noise texture (T_Ground_MacroNoise) sampled at two incommensurate, rotated world
#     scales (~160 m and ~410 m periods) drives value, tint and roughness
#   * emissive mask: rivers (Cinder: ridge of the macro noise's B channel) or patches (Verdance: the B channel
#     itself) restrict the glow, weakly near the player and strongly at distance, so the lava reads as veins
#     rather than a repeating dot grid
# All texture samples use the shared world-group wrap sampler (17 samples > 16 sampler slots otherwise).
import math

import unreal

ST = unreal.MaterialSamplerType
MP = unreal.MaterialProperty

# shared defaults (scales are in units of TileSize = 400 cm)
MACRO = dict(scale1=40.0, scale2=103.0, angle1=23.0, angle2=-51.0, strength=0.6, b_scale=1.37,
             b_offset=(0.31, 0.57), blend_contrast=1.5, glow_macro=0.5, macro_contrast=3.0,
             far_scale=5.3, far2_ratio=2.37, far2_angle=33.0, far_start=1500.0, far_range=6000.0,
             far_normal=0.8, macro_rough=0.03, far_rough=0.25)
# per-ground instance values
GROUND = {
    "Verdance": dict(emissive=4.0, tint_a=(0.80, 0.80, 0.72), tint_b=(1.0, 1.15, 1.05),
                     mask_mode=1.0, mask_thr=0.40, mask_soft=0.30, near_mask=0.5, far_mask=0.9, far_boost=1.3,
                     river_glow=0.0, river_color=(0.2, 1.0, 0.85), river_thr=0.96, river_soft=0.03),
    "Cinder": dict(emissive=6.0, tint_a=(0.85, 0.82, 0.82), tint_b=(1.1, 1.0, 0.95),
                   mask_mode=0.0, mask_thr=0.88, mask_soft=0.06, near_mask=0.3, far_mask=1.0, far_boost=1.4,
                   river_glow=2.5, river_color=(1.0, 0.32, 0.06), river_thr=0.965, river_soft=0.025),
}


def instance_values(g):
    c = GROUND[g]
    scalars = dict(TileSize=400.0, EmissiveStrength=c["emissive"], MacroStrength=MACRO["strength"],
                   BlendContrast=MACRO["blend_contrast"], GlowMacro=MACRO["glow_macro"], MacroScale1=MACRO["scale1"],
                   MacroScale2=MACRO["scale2"], BScale=MACRO["b_scale"], AOToBaseColor=1.0,
                   MacroContrast=MACRO["macro_contrast"], MacroRoughness=MACRO["macro_rough"],
                   FarScale=MACRO["far_scale"], Far2Ratio=MACRO["far2_ratio"], FarStart=MACRO["far_start"],
                   FarRange=MACRO["far_range"], FarNormalStrength=MACRO["far_normal"], FarRoughness=MACRO["far_rough"],
                   EmissiveMaskMode=c["mask_mode"], EmissiveMaskThreshold=c["mask_thr"],
                   EmissiveMaskSoftness=c["mask_soft"], NearEmissiveMask=c["near_mask"],
                   FarEmissiveMask=c["far_mask"], FarEmissiveBoost=c["far_boost"], RiverGlow=c["river_glow"],
                   RiverCoreThreshold=c["river_thr"], RiverCoreSoftness=c["river_soft"])
    vectors = dict(MacroTintA=c["tint_a"], MacroTintB=c["tint_b"], RiverColor=c["river_color"])
    return scalars, vectors


def build_ground_graph(b, t):
    """b: ue_mat_builder.MB on an empty material; t: dict of texture assets by name (needs T_Ground_Verdance_* and
    T_Ground_MacroNoise as parameter defaults)."""
    shared = unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS

    def tex(name, default, sampler, uv):
        n = b.tex(name, t[default], sampler, uv)
        n.set_editor_property("sampler_source", shared)
        return n

    def ch(n, i):
        return b.mask(n, i == 0, i == 1, i == 2)

    wp = b.e(unreal.MaterialExpressionWorldPosition)
    xy = b.mask(wp, True, True, False)
    tile = b.scalar("TileSize", 400.0)

    def rot_uv(angle, scale_node):
        a = math.radians(angle)
        u = b.dot(xy, b.const2(math.cos(a), -math.sin(a)))
        v = b.dot(xy, b.const2(math.sin(a), math.cos(a)))
        return b.div(b.append(u, v), b.mul(tile, scale_node))

    # ---- macro noise (two rotated, incommensurate world scales)
    n1 = tex("MacroNoiseTex", "T_Ground_MacroNoise", ST.SAMPLERTYPE_MASKS, rot_uv(MACRO["angle1"], b.scalar("MacroScale1", MACRO["scale1"])))
    n2 = tex("MacroNoiseTex", "T_Ground_MacroNoise", ST.SAMPLERTYPE_MASKS, rot_uv(MACRO["angle2"], b.scalar("MacroScale2", MACRO["scale2"])))
    m = b.add(b.mul(ch(n1, 0), b.const(0.6)), b.mul(ch(n2, 0), b.const(0.4)))
    d1 = b.mul(b.sub(m, b.const(0.5)), b.scalar("MacroContrast", MACRO["macro_contrast"]))      # ~ -1..1
    bc_ = b.mul(b.scalar("BlendContrast", MACRO["blend_contrast"]), b.const(2.0))
    near_sel = b.sat(b.add(b.mul(b.sub(ch(n1, 1), b.const(0.5)), bc_), b.const(0.5)))
    far_sel = b.sat(b.add(b.mul(b.sub(ch(n2, 1), b.const(0.5)), bc_), b.const(0.5)))

    # ---- distance blend
    depth = b.e(unreal.MaterialExpressionPixelDepth)
    fa = b.sat(b.div(b.sub(depth, b.scalar("FarStart", MACRO["far_start"])), b.scalar("FarRange", MACRO["far_range"])))

    # ---- texture layers
    uva = b.div(xy, tile)
    uvb = b.add(b.div(xy, b.mul(tile, b.scalar("BScale", MACRO["b_scale"]))), b.const2(*MACRO["b_offset"]))
    far_scale = b.scalar("FarScale", MACRO["far_scale"])
    uvf1 = b.add(b.div(xy, b.mul(tile, far_scale)), b.const2(0.17, 0.71))
    # F2 is rotated (it feeds colour / ORM / emissive only, so no tangent-space normal to correct)
    uvf2 = b.add(rot_uv(MACRO["far2_angle"], b.mul(far_scale, b.scalar("Far2Ratio", MACRO["far2_ratio"]))), b.const2(0.63, 0.29))
    maps = (("BaseColorTex", "T_Ground_Verdance_BC", ST.SAMPLERTYPE_COLOR), ("NormalTex", "T_Ground_Verdance_N", ST.SAMPLERTYPE_NORMAL),
            ("ORMTex", "T_Ground_Verdance_ORM", ST.SAMPLERTYPE_MASKS), ("EmissiveTex", "T_Ground_Verdance_E", ST.SAMPLERTYPE_COLOR))
    A = {p: tex(p, d, s, uva) for p, d, s in maps}
    B = {p: tex(p, d, s, uvb) for p, d, s in maps}
    F1 = {p: tex(p, d, s, uvf1) for p, d, s in maps}
    F2 = {p: tex(p, d, s, uvf2) for p, d, s in maps if p != "NormalTex"}

    def layered(p):
        near = b.lerp(A[p], B[p], near_sel, "RGB", "RGB")
        far = b.lerp(F1[p], F2[p], far_sel, "RGB", "RGB")
        return b.lerp(near, far, fa)
    bc = layered("BaseColorTex")
    orm = layered("ORMTex")
    em = layered("EmissiveTex")
    near_n = b.lerp(A["NormalTex"], B["NormalTex"], near_sel, "RGB", "RGB")
    far_n = b.lerp(b.const3(0.0, 0.0, 1.0), F1["NormalTex"], b.scalar("FarNormalStrength", MACRO["far_normal"]), "", "RGB")
    nn = b.lerp(near_n, far_n, fa)

    # ---- macro value / tint / roughness
    factor = b.add(b.const(1.0), b.mul(d1, b.mul(b.scalar("MacroStrength", MACRO["strength"]), b.const(0.35))))
    tint = b.lerp(b.vector("MacroTintA", (0.8, 0.8, 0.8)), b.vector("MacroTintB", (1, 1, 1)),
                  b.sat(b.add(b.mul(d1, b.const(0.5)), b.const(0.5))))
    ao = b.mask(orm, True, False, False)
    aob = b.lerp(b.const(1.0), ao, b.scalar("AOToBaseColor", 1.0))
    base = b.mul(b.mul(b.mul(bc, factor), tint), aob)
    rough = b.add(b.mask(orm, False, True, False), b.mul(d1, b.scalar("MacroRoughness", MACRO["macro_rough"])))
    rough = b.sat(b.add(rough, b.mul(fa, b.scalar("FarRoughness", MACRO["far_rough"]))))   # no grazing sheen far out
    b.out(base, "", MP.MP_BASE_COLOR)
    b.out(rough, "", MP.MP_ROUGHNESS)
    b.out(ao, "", MP.MP_AMBIENT_OCCLUSION)
    b.out(b.const(0.0), "", MP.MP_METALLIC)
    b.out(b.norm(nn), "", MP.MP_NORMAL)

    # ---- emissive: macro glow, river / patch mask stronger with distance, far boost
    gm = b.sat(b.add(b.mul(d1, b.const(1.2)), b.const(0.6)))
    glow = b.lerp(b.const(1.0), gm, b.scalar("GlowMacro", MACRO["glow_macro"]))
    rf = b.add(b.mul(ch(n1, 2), b.const(0.65)), b.mul(ch(n2, 2), b.const(0.35)))
    ridge = b.one_minus(b.mul(b.unary(unreal.MaterialExpressionAbs, b.sub(rf, b.const(0.5))), b.const(2.0)))
    src = b.lerp(ridge, rf, b.scalar("EmissiveMaskMode", 0.0))
    rm = b.sat(b.div(b.sub(src, b.scalar("EmissiveMaskThreshold", 0.8)), b.scalar("EmissiveMaskSoftness", 0.12)))
    amount = b.lerp(b.scalar("NearEmissiveMask", 0.5), b.scalar("FarEmissiveMask", 1.0), fa)
    emask = b.lerp(b.const(1.0), rm, amount)
    boost = b.lerp(b.const(1.0), b.scalar("FarEmissiveBoost", 1.5), fa)
    k = b.mul(b.mul(b.mul(b.scalar("EmissiveStrength", 4.0), glow), emask), boost)
    # far field: a soft procedural glow along the river cores (Cinder), so the lava reads as continuous veins
    # where the texture's cracks are below a pixel; fades in with distance, brightness broken by the fine noise
    core = b.sat(b.div(b.sub(ridge, b.scalar("RiverCoreThreshold", 0.965)), b.scalar("RiverCoreSoftness", 0.025)))
    flick = b.add(b.const(0.4), b.mul(ch(n1, 1), b.const(0.6)))
    river = b.mul(b.mul(b.mul(core, flick), fa), b.scalar("RiverGlow", 0.0))
    b.out(b.add(b.mul(em, k), b.mul(b.vector("RiverColor", (1.0, 0.32, 0.06)), river)), "", MP.MP_EMISSIVE_COLOR)

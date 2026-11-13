# SPDX-License-Identifier: MIT
# M_TD_Magma graph (Unreal editor Python): the lava surface of ATDMagmaVolume (SM_Cind_MagmaSurface).
#
# The mesh is a unit slab that gameplay stretches NON-UNIFORMLY over each magma volume (field 1600 x 7000 cm, jammer
# lake 3400 x 5000 cm). So nothing here uses the mesh UVs for detail:
#   * the crust plates / channels / cracks (T_Cind_MagmaSurface_M, _N) are sampled in WORLD XY at TileSize, panned
#     along FlowX / FlowY (cm/s) so the crust drifts; a second copy at 1.37 x scale, offset, is blended in by a world
#     macro noise (T_Ground_MacroNoise) so the 8 m tile never repeats visibly
#   * channel width varies over the world (macro noise B), so some seams are wide molten rivers and some are
#     hairlines; the glow is a heat ramp (deep red at the channel edge -> orange-yellow at the core) times a slow
#     flicker panned faster than the crust, so the hot flow visibly moves under the plates
#   * the crust rim: distance to the volume's edge in WORLD cm = UV0 x the object's bounds (exact at any fit),
#     jittered by the macro noise; within RimWidth the channels close up and the crust is ashen and cracked, so the
#     lava sits in a cooled basin instead of reading as a box
#   * a constant World Position Offset lift (LiftCm) keeps the surface above the Cinder ground slab, whose top is
#     coplanar with the volume's top (Z 0) and would z-fight it otherwise
# Emissive lives only in the channels and (dimly) in the cracks; the crust plates are dark, rough basalt.
import unreal

ST = unreal.MaterialSamplerType
MP = unreal.MaterialProperty

DEFAULTS = dict(
    TileSize=800.0, FlowX=0.0, FlowY=5.0, FlowTimeOffset=0.0, MacroScale=3200.0, FlickerScale=700.0, FlickerSpeed=3.0,
    ChannelMin=0.18, ChannelMax=0.65, ChannelPower=2.0, CrackWidth=0.08, CrackGlow=0.18, CrackFadeStart=600.0,
    CrackFadeRange=1500.0, EmissiveStrength=2.5, RimWidth=50.0, RimSoft=150.0, RimJitter=120.0, LiftCm=2.0,
    NormalStrength=0.6, CrustSpecular=0.15,
)
VECTORS = dict(
    HotCore=(1.0, 0.32, 0.03), HotEdge=(0.80, 0.05, 0.008), CrackColor=(0.85, 0.10, 0.015),
    CrustDark=(0.005, 0.0045, 0.0045), CrustLight=(0.020, 0.017, 0.016), RimAsh=(0.022, 0.014, 0.011),
    MoltenBase=(0.12, 0.025, 0.005),
)


def build_magma_graph(b, t):
    """b: ue_mat_builder.MB on an empty material; t: dict with T_Cind_MagmaSurface_M / _N and T_Ground_MacroNoise."""
    D, V = DEFAULTS, VECTORS
    shared = unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS

    def tex(name, default, sampler, uv):
        n = b.tex(name, t[default], sampler, uv)
        n.set_editor_property("sampler_source", shared)
        return n

    def ch(n, i):
        return b.mask(n, i == 0, i == 1, i == 2)

    def smooth01(x):                                  # x already saturated: x*x*(3-2x)
        return b.mul(b.mul(x, x), b.sub(b.const(3.0), b.mul(x, b.const(2.0))))

    wp = b.e(unreal.MaterialExpressionWorldPosition)
    xy = b.mask(wp, True, True, False)
    time = b.add(b.e(unreal.MaterialExpressionTime), b.scalar("FlowTimeOffset", D["FlowTimeOffset"]))
    flow = b.append(b.scalar("FlowX", D["FlowX"]), b.scalar("FlowY", D["FlowY"]))
    pan = b.mul(flow, time)
    tile = b.scalar("TileSize", D["TileSize"])

    # ---- macro noise (world, not panned: the river widths stay put while the crust drifts through them)
    macro = tex("MacroNoiseTex", "T_Ground_MacroNoise", ST.SAMPLERTYPE_MASKS, b.div(xy, b.scalar("MacroScale", D["MacroScale"])))
    sel = b.sat(b.add(b.mul(b.sub(ch(macro, 1), b.const(0.5)), b.const(3.0)), b.const(0.5)))

    # ---- crust layers (world UV, panned) and the anti-tiling copy
    uva = b.div(b.sub(xy, pan), tile)
    uvb = b.add(b.div(b.sub(xy, b.mul(pan, b.const(0.85))), b.mul(tile, b.const(1.37))), b.const2(0.31, 0.57))
    ma = tex("MaskTex", "T_Cind_MagmaSurface_M", ST.SAMPLERTYPE_LINEAR_COLOR, uva)
    mb = tex("MaskTex", "T_Cind_MagmaSurface_M", ST.SAMPLERTYPE_LINEAR_COLOR, uvb)
    na = tex("NormalTex", "T_Cind_MagmaSurface_N", ST.SAMPLERTYPE_NORMAL, uva)
    nb = tex("NormalTex", "T_Cind_MagmaSurface_N", ST.SAMPLERTYPE_NORMAL, uvb)
    # union, not a lerp: each layer's edge fields are pushed up by the other's share and the minimum is kept, so in a
    # transition both channel networks show (narrowed) instead of averaging into mushy blobs; the normal follows the
    # layer that wins
    push_a = b.mul(b.sat(b.sub(b.mul(sel, b.const(2.0)), b.const(1.0))), b.const(0.7))            # 0 for sel <= 0.5
    push_b = b.mul(b.sat(b.sub(b.mul(b.one_minus(sel), b.const(2.0)), b.const(1.0))), b.const(0.7))
    ra = b.add(ch(ma, 0), push_a)
    rb = b.add(ch(mb, 0), push_b)
    R = b.e(unreal.MaterialExpressionMin)
    b.c(ra, "", R, "A")
    b.c(rb, "", R, "B")
    G = b.e(unreal.MaterialExpressionMin)
    b.c(b.add(ch(ma, 1), push_a), "", G, "A")
    b.c(b.add(ch(mb, 1), push_b), "", G, "B")
    win_b = b.sat(b.mul(b.sub(ra, rb), b.const(12.0)))
    Bv = b.lerp(ch(ma, 2), ch(mb, 2), win_b)
    nrm = b.lerp(na, nb, win_b, "RGB", "RGB")

    # ---- crust rim: distance to the volume edge in world cm (UV0 x object bounds), jittered
    uv0 = b.e(unreal.MaterialExpressionTextureCoordinate)
    bounds = b.e(unreal.MaterialExpressionObjectBounds)
    half = b.const2(0.5, 0.5)
    inset = b.sub(half, b.unary(unreal.MaterialExpressionAbs, b.sub(uv0, half)))     # 0 at the edge .. 0.5 centre
    dxy = b.mul(inset, b.mask(bounds, True, True, False))
    mn = b.e(unreal.MaterialExpressionMin)
    b.c(b.mask(dxy, True, False, False), "", mn, "A")
    b.c(b.mask(dxy, False, True, False), "", mn, "B")
    ed = b.add(mn, b.mul(b.sub(ch(macro, 0), b.const(0.5)), b.scalar("RimJitter", D["RimJitter"])))
    inner = smooth01(b.sat(b.div(b.sub(ed, b.scalar("RimWidth", D["RimWidth"])), b.scalar("RimSoft", D["RimSoft"]))))
    rim = b.one_minus(inner)

    # ---- channels: width from the macro noise, closed at the rim
    wsel = smooth01(b.sat(b.mul(b.sub(ch(macro, 2), b.const(0.25)), b.const(2.0))))
    width = b.mul(b.lerp(b.scalar("ChannelMin", D["ChannelMin"]), b.scalar("ChannelMax", D["ChannelMax"]), wsel), inner)
    heat_lin = b.sat(b.sub(b.const(1.0), b.div(R, b.add(width, b.const(0.005)))))
    heat = b.pow(heat_lin, b.scalar("ChannelPower", D["ChannelPower"]))
    plate = b.one_minus(b.sat(b.mul(heat_lin, b.const(4.0))))       # 1 on the crust, 0 in the molten channel
    crack = b.mul(b.sat(b.sub(b.const(1.0), b.div(G, b.scalar("CrackWidth", D["CrackWidth"])))), plate)

    # ---- flicker: two noise layers panned faster than the crust, crossing
    fs = b.scalar("FlickerScale", D["FlickerScale"])
    fspd = b.scalar("FlickerSpeed", D["FlickerSpeed"])
    f1 = tex("MacroNoiseTex", "T_Ground_MacroNoise", ST.SAMPLERTYPE_MASKS, b.div(b.sub(xy, b.mul(pan, fspd)), fs))
    f2 = tex("MacroNoiseTex", "T_Ground_MacroNoise", ST.SAMPLERTYPE_MASKS,
             b.add(b.div(b.add(xy, b.mul(b.mul(pan, fspd), b.const(0.6))), b.mul(fs, b.const(0.61))), b.const2(0.43, 0.11)))
    flick = b.add(b.const(0.55), b.mul(b.add(ch(f1, 1), ch(f2, 0)), b.const(0.45)))   # ~0.55 .. 1.45

    # ---- emissive
    col = b.lerp(b.vector("HotEdge", V["HotEdge"]), b.vector("HotCore", V["HotCore"]), b.mul(heat, heat))
    es = b.scalar("EmissiveStrength", D["EmissiveStrength"])
    e_ch = b.mul(b.mul(b.mul(col, heat), flick), es)
    crack_amt = b.mul(b.mul(crack, b.scalar("CrackGlow", D["CrackGlow"])), b.lerp(b.const(0.6), b.const(1.0), inner))
    # hairline cracks alias into dotted sparkles beyond ~15 m: fade them out with camera depth
    depth = b.e(unreal.MaterialExpressionPixelDepth)
    near = b.one_minus(b.sat(b.div(b.sub(depth, b.scalar("CrackFadeStart", D["CrackFadeStart"])),
                                   b.scalar("CrackFadeRange", D["CrackFadeRange"]))))
    crack_amt = b.mul(crack_amt, near)
    e_cr = b.mul(b.mul(b.vector("CrackColor", V["CrackColor"]), crack_amt), es)
    b.out(b.add(e_ch, e_cr), "", MP.MP_EMISSIVE_COLOR)

    # ---- base colour / roughness / normal
    crust = b.lerp(b.vector("CrustDark", V["CrustDark"]), b.vector("CrustLight", V["CrustLight"]), Bv)
    crust = b.lerp(crust, b.vector("RimAsh", V["RimAsh"]), b.mul(rim, b.const(0.7)))
    base = b.lerp(crust, b.vector("MoltenBase", V["MoltenBase"]), heat_lin)
    b.out(base, "", MP.MP_BASE_COLOR)
    b.out(b.lerp(b.const(0.9), b.const(0.35), heat_lin), "", MP.MP_ROUGHNESS)
    b.out(b.const(0.0), "", MP.MP_METALLIC)
    # low specular on the crust: at the grazing arrival / eye views the default 0.5 mirrors the bright Cinder sky
    # and washes the dark basalt out to tan
    b.out(b.lerp(b.scalar("CrustSpecular", D["CrustSpecular"]), b.const(0.5), heat_lin), "", MP.MP_SPECULAR)
    flat = b.const3(0.0, 0.0, 1.0)
    nn = b.lerp(flat, nrm, b.mul(b.scalar("NormalStrength", D["NormalStrength"]), plate))
    b.out(b.norm(nn), "", MP.MP_NORMAL)

    # ---- lift above the coplanar ground slab
    b.out(b.append(b.const2(0.0, 0.0), b.scalar("LiftCm", D["LiftCm"])), "", MP.MP_WORLD_POSITION_OFFSET)

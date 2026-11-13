# M_TD_Planet surface graph (night side pass, Nov 13), shared by ue_import_planets.py (fresh build) and
# ue_rebuild_planet_surface.py (in-place rebuild of the existing master, rendered editor only).
#
# Why unlit: the space light rig (ATDPlaceholderSky, Space style) adds a violet fill light that shines from the
# anti-sun side at 15% of the sun, plus a sky light, so a lit planet had its night side lit as brightly as a dim day.
# The surface now lights itself from the SunDirection vector (set by ATDPlanetAtmosphere::SyncSunDirection), so its
# terminator is the same under any rig, Nick's skybox included:
#   emissive = albedo * (SunColor * SunIntensity * wrapLambert(N_detail, L) * horizon(N_geo, L) * AO + AmbientColor * NightAmbient)
#            + EmissiveTex * EmissiveStrength * lerp(1, night(N_geo, L), NightGlowMask) * (1 - clouds * CloudGlowOcclusion)
#            + SurfaceRimColor * SurfaceRimStrength * fresnel * dayLimb(N_geo, L)
# Parameter names the code or the instances set are unchanged (SunDirection, EmissiveStrength, NightGlowMask, ...).
import unreal

ST = unreal.MaterialSamplerType
MP = unreal.MaterialProperty

SUN_DIR = (0.779, 0.546, 0.309)     # ATDSkySphere's baked sun (direction TOWARDS the sun); code overrides it per level

# Defaults on the master (instances override per planet, see INSTANCE_VALUES).
DEFAULTS = dict(SunIntensity=2.5, NightAmbient=0.10, TerminatorSoftness=0.12, NightGlowStart=0.05, NightGlowSharpness=4.0,
                NightGlowMask=1.0, CloudGlowOcclusion=0.75, EmissiveStrength=6.0, SurfaceRimStrength=0.5)

# Per planet look. Day side ~ albedo * 3 (the old 8 lux sun / pi = 2.5, plus what the lit material got from the sky
# light and specular); night ambient ~3-4% of that (Cinder more, its basalt albedo is very dark).
INSTANCE_VALUES = {
    "Verdance": dict(scalars=dict(SunIntensity=3.0, NightAmbient=0.10, EmissiveStrength=4.5, NightGlowMask=1.0,
                                  CloudGlowOcclusion=0.8, TerminatorSoftness=0.12),
                     vectors=dict(SunColor=(1.0, 0.95, 0.86), AmbientColor=(0.45, 0.60, 1.0))),
    # Cinder's albedo is dark (basalt): more ambient so the silhouette reads, stronger lava at night.
    "Cinder": dict(scalars=dict(SunIntensity=3.6, NightAmbient=0.30, EmissiveStrength=9.0, NightGlowMask=1.0,
                                CloudGlowOcclusion=0.6, TerminatorSoftness=0.12),
                   vectors=dict(SunColor=(1.0, 0.92, 0.82), AmbientColor=(0.70, 0.50, 1.0))),
}
ATMO_VALUES = dict(SunMaskAmount=0.92)


def build_surface_graph(b, t):
    """b: ue_mat_builder.MB on an empty material; t: dict of the Verdance textures (sampler defaults)."""
    m = b.m
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    uv = b.e(unreal.MaterialExpressionTextureCoordinate)
    bc = b.tex("BaseColorTex", t["T_Planet_Verdance_BC"], ST.SAMPLERTYPE_COLOR, uv)
    nn = b.tex("NormalTex", t["T_Planet_Verdance_N"], ST.SAMPLERTYPE_NORMAL, uv)
    orm = b.tex("ORMTex", t["T_Planet_Verdance_ORM"], ST.SAMPLERTYPE_MASKS, uv)
    em = b.tex("EmissiveTex", t["T_Planet_Verdance_E"], ST.SAMPLERTYPE_COLOR, uv)
    # clouds: U offset = frac(time * CloudPanSpeed)
    tm = b.e(unreal.MaterialExpressionTime)
    pan = b.frac(b.mul(tm, b.scalar("CloudPanSpeed", 0.0015)))
    cuv = b.add(uv, b.append(pan, b.const(0.0)))
    cl = b.tex("CloudTex", t["T_Planet_Verdance_Clouds"], ST.SAMPLERTYPE_MASKS, cuv)
    ca = b.mul(cl, b.scalar("CloudOpacity", 0.9), "R")
    albedo = b.lerp(bc, b.vector("CloudColor", (1, 1, 1)), ca, "RGB")

    # light vector and the two normals: geometric (terminator, glow mask) and detail (relief on the day side)
    sun = b.vector("SunDirection", SUN_DIR)
    L = b.norm(b.mask(sun, True, True, True))
    ng = b.e(unreal.MaterialExpressionVertexNormalWS)
    nt = b.lerp(nn, b.const3(0, 0, 1), ca, "RGB")
    nd = b.norm(b.tangent_to_world(nt))
    ndl_g = b.dot(ng, L)
    ndl_d = b.dot(nd, L)
    soft = b.scalar("TerminatorSoftness", DEFAULTS["TerminatorSoftness"])
    wrap = b.sat(b.div(b.add(ndl_d, soft), b.add(b.const(1.0), soft)))                 # wrapped Lambert on the relief
    horizon = b.sat(b.div(b.add(ndl_g, soft), b.mul(soft, b.const(2.0))))             # relief never lights past the terminator
    ao = b.lerp(orm, b.const(1.0), ca, "R")
    diffuse = b.mul(b.mul(wrap, horizon), ao)
    sun_rgb = b.mul(b.vector("SunColor", (1.0, 0.95, 0.86)), b.scalar("SunIntensity", DEFAULTS["SunIntensity"]))
    amb_rgb = b.mul(b.vector("AmbientColor", (0.45, 0.6, 1.0)), b.scalar("NightAmbient", DEFAULTS["NightAmbient"]))
    light = b.add(b.mul(sun_rgb, diffuse), amb_rgb)
    lit = b.mul(albedo, light)

    # night glow: 0 on the day side, ramps in just past the terminator
    night = b.sat(b.mul(b.sub(b.scalar("NightGlowStart", DEFAULTS["NightGlowStart"]), ndl_g),
                        b.scalar("NightGlowSharpness", DEFAULTS["NightGlowSharpness"])))
    glowmask = b.lerp(b.const(1.0), night, b.scalar("NightGlowMask", DEFAULTS["NightGlowMask"]))
    cloud_block = b.one_minus(b.mul(ca, b.scalar("CloudGlowOcclusion", DEFAULTS["CloudGlowOcclusion"])))
    glow = b.mul(b.mul(b.mul(em, b.scalar("EmissiveStrength", DEFAULTS["EmissiveStrength"]), "RGB"), glowmask), cloud_block)

    # day-side limb in-scatter, gone on the night side
    fres = b.e(unreal.MaterialExpressionFresnel, exponent=4.0, base_reflect_fraction=0.0)
    b.c(b.e(unreal.MaterialExpressionVertexNormalWS), "", fres, "Normal")
    limb = b.sat(b.add(b.mul(ndl_g, b.const(1.5)), b.const(0.2)))
    rim = b.mul(b.mul(b.vector("SurfaceRimColor", (0.2, 0.8, 1.0)), b.scalar("SurfaceRimStrength", DEFAULTS["SurfaceRimStrength"])),
                b.mul(fres, limb))
    b.out(b.add(b.add(lit, glow), rim), "", MP.MP_EMISSIVE_COLOR)

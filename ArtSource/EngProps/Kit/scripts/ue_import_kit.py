# SPDX-License-Identifier: MIT
# Triad Drift - import the planet-outpost Kit (ArtSource/EngProps/Kit/export/SM_Kit_*.fbx) into Unreal. Forked from
# DominionInterior/scripts/ue_import_di_kit.py. Headless, from the repo root (or through ue_kit_all.py):
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Kit\scripts\ue_import_kit.py
#
# Writes ONLY under /Game/TriadDrift/Gameplay/Props/Kit:
#   Textures/T_Kit_Trim_BC|N|ORM   the shared 2K trim sheet
#   MI_Kit_Trim                     instance of /Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured (read only)
#   MI_Kit_Glow_Verdance (teal, the mesh default), MI_Kit_Glow_Cinder (ember), MI_Kit_Screen
#                                   instances of /Game/TriadDrift/Gameplay/Props/Dominion/M_TD_DominionGlow (read only)
#   Pipelines/PL_Kit_StaticMesh, SM_Kit_*
import glob
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
REPO = os.path.dirname(os.path.dirname(os.path.dirname(KIT)))
DEST = "/Game/TriadDrift/Gameplay/Props/Kit"
TEX_DEST = DEST + "/Textures"
PIPELINE = DEST + "/Pipelines/PL_Kit_StaticMesh"
MASTER_TEX = "/Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured"
MASTER_GLOW = "/Game/TriadDrift/Gameplay/Props/Dominion/M_TD_DominionGlow"
SIZE_CHECK = os.path.join(KIT, "review", "size_check_r5.json")

GLOWS = {   # MI name: (slot key it fills by default, scalars, colour)
    # strips at 4 (r4; 8 flooded the outpost room), the beacon lamp at 60 (reads from the approach)
    "MI_Kit_Glow_Verdance": ("Glow", {"GlowStrength": 4.0, "PulseAmount": 0.0}, (0.05, 0.85, 0.75)),
    "MI_Kit_Glow_Cinder": (None, {"GlowStrength": 4.0, "PulseAmount": 0.0}, (1.0, 0.36, 0.06)),
    "MI_Kit_Light": ("Light", {"GlowStrength": 10.0, "PulseAmount": 0.0}, (0.9, 0.95, 1.0)),
    "MI_Kit_Beacon_Verdance": ("Beacon", {"GlowStrength": 60.0, "PulseAmount": 0.25, "PulseRate": 0.8}, (0.1, 1.0, 0.85)),
    "MI_Kit_Beacon_Cinder": (None, {"GlowStrength": 60.0, "PulseAmount": 0.25, "PulseRate": 0.8}, (1.0, 0.4, 0.08)),
}
SCREEN_MI = "MI_Kit_Screen"
SET = {"stem": "T_Kit_Trim", "trim_mi": "MI_Kit_Trim", "export": "export", "prefix": "SM_Kit_", "extras": True}
PILLAR_MASTER = "/Game/TriadDrift/Gameplay/Props/Kit/Materials/M_Kit_LightPillar"
DECAL_MASTER = "/Game/TriadDrift/Gameplay/Props/Kit/Materials/M_Kit_Decal"
TRIMD_MASTER = "/Game/TriadDrift/Gameplay/Props/Kit/Materials/M_KitD_Trim"


def ensure_trim_grime(tex, notes):
    """M_KitD_Trim (r8, review r3: the walls read as wallpaper): the textured trim (BaseColorTex / NormalTex / ORMTex,
    the same parameter names as M_TD_PropTextured) times a world-position value and grime variation, no new textures:
    two Noise nodes on the world position (about 9 m blotches and 1.5 m mottling) darken the base colour by up to
    GrimeAmount (default 0.45) and raise the roughness by up to GrimeRough (default 0.2)."""
    ensure_dir("/Game/TriadDrift/Gameplay/Props/Kit/Materials")
    folder, name = TRIMD_MASTER.rsplit("/", 1)
    mat = eas.load_asset(TRIMD_MASTER) if eas.does_asset_exist(TRIMD_MASTER) else tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)

    def node(cls, x, y):
        return mel.create_material_expression(mat, cls, x, y)

    def tp(pname, t, sampler, y):
        n = node(unreal.MaterialExpressionTextureSampleParameter2D, -1200, y)
        n.set_editor_property("parameter_name", pname)
        n.set_editor_property("texture", t)
        n.set_editor_property("sampler_type", sampler)
        return n

    def sp(pname, v, y):
        n = node(unreal.MaterialExpressionScalarParameter, -1200, y)
        n.set_editor_property("parameter_name", pname)
        n.set_editor_property("default_value", v)
        return n

    def noise(scale, y):
        n = node(unreal.MaterialExpressionNoise, -1200, y)
        for k, v in (("scale", scale), ("levels", 3), ("output_min", 0.0), ("output_max", 1.0), ("quality", 1)):
            _set(n, k, v, notes)
        return n
    S = unreal.MaterialSamplerType
    bc = tp("BaseColorTex", tex["BC"], S.SAMPLERTYPE_COLOR, -600)
    nm = tp("NormalTex", tex["N"], S.SAMPLERTYPE_NORMAL, -300)
    orm = tp("ORMTex", tex["ORM"], S.SAMPLERTYPE_MASKS, 0)
    n1, n2 = noise(0.0011, 300), noise(0.0065, 500)
    amt, rgh = sp("GrimeAmount", 0.45, 700), sp("GrimeRough", 0.2, 800)
    g = node(unreal.MaterialExpressionMultiply, -900, 400)           # 0..1 grime mask = n1 * n2
    inv = node(unreal.MaterialExpressionOneMinus, -750, 400)
    lerp = node(unreal.MaterialExpressionLinearInterpolate, -600, 400)   # value = lerp(1 - amt, 1, 1 - mask)
    one_m_amt = node(unreal.MaterialExpressionOneMinus, -900, 700)
    one = node(unreal.MaterialExpressionConstant, -750, 650)
    one.set_editor_property("r", 1.0)
    bco = node(unreal.MaterialExpressionMultiply, -400, -500)
    gr = node(unreal.MaterialExpressionMultiply, -600, 800)           # roughness add = mask * GrimeRough
    ro = node(unreal.MaterialExpressionAdd, -400, 100)
    ok = True
    ok &= mel.connect_material_expressions(n1, "", g, "A")
    ok &= mel.connect_material_expressions(n2, "", g, "B")
    ok &= mel.connect_material_expressions(g, "", inv, "")
    ok &= mel.connect_material_expressions(amt, "", one_m_amt, "")
    ok &= mel.connect_material_expressions(one_m_amt, "", lerp, "A")
    ok &= mel.connect_material_expressions(one, "", lerp, "B")
    ok &= mel.connect_material_expressions(inv, "", lerp, "Alpha")
    # r11: BandTint (default white = no change) recolours only the painted red band: mask = saturate((R - max(G, B)) x 4)
    bt = node(unreal.MaterialExpressionVectorParameter, -1200, -800)
    bt.set_editor_property("parameter_name", "BandTint")
    bt.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    mgb = node(unreal.MaterialExpressionMax, -1000, -900)
    rs = node(unreal.MaterialExpressionSubtract, -900, -900)
    four = node(unreal.MaterialExpressionConstant, -900, -980)
    four.set_editor_property("r", 4.0)
    rm = node(unreal.MaterialExpressionMultiply, -800, -900)
    msk = node(unreal.MaterialExpressionSaturate, -700, -900)
    tinted = node(unreal.MaterialExpressionMultiply, -800, -750)
    bsel = node(unreal.MaterialExpressionLinearInterpolate, -600, -800)
    ok &= mel.connect_material_expressions(bc, "G", mgb, "A")
    ok &= mel.connect_material_expressions(bc, "B", mgb, "B")
    ok &= mel.connect_material_expressions(bc, "R", rs, "A")
    ok &= mel.connect_material_expressions(mgb, "", rs, "B")
    ok &= mel.connect_material_expressions(rs, "", rm, "A")
    ok &= mel.connect_material_expressions(four, "", rm, "B")
    ok &= mel.connect_material_expressions(rm, "", msk, "")
    ok &= mel.connect_material_expressions(bc, "RGB", tinted, "A")
    ok &= mel.connect_material_expressions(bt, "", tinted, "B")
    ok &= mel.connect_material_expressions(bc, "RGB", bsel, "A")
    ok &= mel.connect_material_expressions(tinted, "", bsel, "B")
    ok &= mel.connect_material_expressions(msk, "", bsel, "Alpha")
    ok &= mel.connect_material_expressions(bsel, "", bco, "A")
    ok &= mel.connect_material_expressions(lerp, "", bco, "B")
    ok &= mel.connect_material_expressions(g, "", gr, "A")
    ok &= mel.connect_material_expressions(rgh, "", gr, "B")
    ok &= mel.connect_material_expressions(orm, "G", ro, "A")
    ok &= mel.connect_material_expressions(gr, "", ro, "B")
    # space r2: BaseTint (default white) and SelfIllum (default 0: the interior is unchanged) - the space set pieces use
    # them for two-tone panels and a faint self-lit read on their shadow side
    btint = node(unreal.MaterialExpressionVectorParameter, -400, -700)
    btint.set_editor_property("parameter_name", "BaseTint")
    btint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    silum = node(unreal.MaterialExpressionScalarParameter, -400, -300)
    silum.set_editor_property("parameter_name", "SelfIllum")
    silum.set_editor_property("default_value", 0.0)
    bfin = node(unreal.MaterialExpressionMultiply, -250, -600)
    emi = node(unreal.MaterialExpressionMultiply, -250, -350)
    ok &= mel.connect_material_expressions(bco, "", bfin, "A")
    ok &= mel.connect_material_expressions(btint, "", bfin, "B")
    ok &= mel.connect_material_expressions(bfin, "", emi, "A")
    ok &= mel.connect_material_expressions(silum, "", emi, "B")
    ok &= mel.connect_material_property(bfin, "", unreal.MaterialProperty.MP_BASE_COLOR)
    ok &= mel.connect_material_property(emi, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    ok &= mel.connect_material_property(nm, "RGB", unreal.MaterialProperty.MP_NORMAL)
    ok &= mel.connect_material_property(orm, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    ok &= mel.connect_material_property(ro, "", unreal.MaterialProperty.MP_ROUGHNESS)
    ok &= mel.connect_material_property(orm, "B", unreal.MaterialProperty.MP_METALLIC)
    if not ok:
        log("FAILED: could not wire " + TRIMD_MASTER)
        return None
    try:
        mel.recompile_material(mat)
    except Exception:
        notes.append("M_KitD_Trim compiles on first load")
    eas.save_asset(TRIMD_MASTER, only_if_is_dirty=False)
    return mat
DECAL_SRC = os.path.join(os.path.dirname(os.path.dirname(KIT)), "EngAI", "Props", "DominionInterior", "plans_r1", "textures")


ROUTE_MI = [None]


def ensure_decal(notes):
    """M_Kit_Decal + MI_KitD_Decal: Grok Bot's decal atlas (PR #82, T_DI_Decals_{BC,N,ORM,E}: trident, Triad mark,
    hazard frame, route arrow) as a masked material: opacity = BC alpha, emissive = E x DecalEmissive (default 4)."""
    tasks = []
    for k in ("BC", "N", "ORM", "E"):
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", os.path.join(DECAL_SRC, "T_DI_Decals_%s.png" % k))
        t.set_editor_property("destination_path", TEX_DEST)
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", False)
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    tex = {}
    for k in ("BC", "N", "ORM", "E"):
        t = eas.load_asset("%s/T_DI_Decals_%s" % (TEX_DEST, k))
        if t is None:
            notes.append("decal texture missing " + k)
            return None
        if k == "N":
            _set(t, "compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP, notes)
            _set(t, "srgb", False, notes)
        elif k == "ORM":
            _set(t, "compression_settings", unreal.TextureCompressionSettings.TC_MASKS, notes)
            _set(t, "srgb", False, notes)
        eas.save_loaded_asset(t)
        tex[k] = t
    ensure_dir("/Game/TriadDrift/Gameplay/Props/Kit/Materials")
    folder, name = DECAL_MASTER.rsplit("/", 1)
    mat = eas.load_asset(DECAL_MASTER) if eas.does_asset_exist(DECAL_MASTER) else tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    # the decal meshes are also scattered as instances (the planet scatter: SM_Kit_ScorchLines on Verdance's wreck scar)
    for usage in (unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES, unreal.MaterialUsage.MATUSAGE_NANITE):
        try:
            mel.set_material_usage(mat, usage)
        except Exception as error:
            notes.append("M_Kit_Decal usage %s: %s" % (usage, error))
    mat.set_editor_property("used_with_instanced_static_meshes", True)
    mel.delete_all_material_expressions(mat)

    def node(cls, x, y):
        return mel.create_material_expression(mat, cls, x, y)

    def tp(pname, t, sampler, y):
        n = node(unreal.MaterialExpressionTextureSampleParameter2D, -900, y)
        n.set_editor_property("parameter_name", pname)
        n.set_editor_property("texture", t)
        n.set_editor_property("sampler_type", sampler)
        return n
    S = unreal.MaterialSamplerType
    bc = tp("DecalBC", tex["BC"], S.SAMPLERTYPE_COLOR, -400)
    nm = tp("DecalN", tex["N"], S.SAMPLERTYPE_NORMAL, -100)
    orm = tp("DecalORM", tex["ORM"], S.SAMPLERTYPE_MASKS, 200)
    em = tp("DecalE", tex["E"], S.SAMPLERTYPE_COLOR, 500)
    st = node(unreal.MaterialExpressionScalarParameter, -900, 750)
    st.set_editor_property("parameter_name", "DecalEmissive")
    st.set_editor_property("default_value", 4.0)
    m = node(unreal.MaterialExpressionMultiply, -500, 550)
    tint = node(unreal.MaterialExpressionVectorParameter, -900, 900)
    tint.set_editor_property("parameter_name", "DecalTint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    ta = node(unreal.MaterialExpressionScalarParameter, -900, 1050)
    ta.set_editor_property("parameter_name", "TintAmount")
    ta.set_editor_property("default_value", 0.0)

    def tinted(src, out, y):
        d = node(unreal.MaterialExpressionDesaturation, -700, y)
        mm = node(unreal.MaterialExpressionMultiply, -600, y)
        lp = node(unreal.MaterialExpressionLinearInterpolate, -450, y)
        r = True
        r &= mel.connect_material_expressions(src, out, d, "")
        r &= mel.connect_material_expressions(d, "", mm, "A")
        r &= mel.connect_material_expressions(tint, "", mm, "B")
        r &= mel.connect_material_expressions(src, out, lp, "A")
        r &= mel.connect_material_expressions(mm, "", lp, "B")
        r &= mel.connect_material_expressions(ta, "", lp, "Alpha")
        return lp, r
    bct, ok1 = tinted(bc, "RGB", -250)
    emt, ok2 = tinted(em, "RGB", 650)
    ok = ok1 and ok2
    ok &= mel.connect_material_expressions(emt, "", m, "A")
    ok &= mel.connect_material_expressions(st, "", m, "B")
    ok &= mel.connect_material_property(bct, "", unreal.MaterialProperty.MP_BASE_COLOR)
    ok &= mel.connect_material_property(bc, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
    ok &= mel.connect_material_property(nm, "RGB", unreal.MaterialProperty.MP_NORMAL)
    ok &= mel.connect_material_property(orm, "G", unreal.MaterialProperty.MP_ROUGHNESS)
    ok &= mel.connect_material_property(orm, "B", unreal.MaterialProperty.MP_METALLIC)
    ok &= mel.connect_material_property(m, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if not ok:
        log("FAILED: could not wire " + DECAL_MASTER)
        return None
    try:
        mel.recompile_material(mat)
    except Exception:
        notes.append("M_Kit_Decal compiles on first load")
    eas.save_asset(DECAL_MASTER, only_if_is_dirty=False)
    mi = instance("MI_KitD_Decal", mat)
    # r9 (review r4): the trident in a darker crimson, less emissive, so it reads as inlaid armour and not an icon
    mel.set_material_instance_vector_parameter_value(mi, "DecalTint", unreal.LinearColor(0.55, 0.012, 0.008, 1.0))
    mel.set_material_instance_scalar_parameter_value(mi, "TintAmount", 0.85)
    mel.set_material_instance_scalar_parameter_value(mi, "DecalEmissive", 1.5)
    eas.save_loaded_asset(mi)
    route = instance("MI_KitD_Decal_Route", mat)          # r8: the route chevrons in Dominion crimson (review r3)
    mel.set_material_instance_vector_parameter_value(route, "DecalTint", unreal.LinearColor(1.2, 0.015, 0.008, 1.0))
    mel.set_material_instance_scalar_parameter_value(route, "TintAmount", 1.0)
    mel.set_material_instance_scalar_parameter_value(route, "DecalEmissive", 4.0)
    eas.save_loaded_asset(route)
    ROUTE_MI[0] = route
    return mi


def ensure_stone(master_tex, notes):
    """MI_Kit_Stone: the same pieces in the ancient-stone trim (T_KitS_Trim_*), for the relay's plinth."""
    tasks = []
    for p in sorted(glob.glob(os.path.join(KIT, "textures", "T_KitS_Trim_*.png"))):
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", p)
        t.set_editor_property("destination_path", TEX_DEST)
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", False)
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    tex = {}
    for suffix in ("BC", "N", "ORM"):
        t = eas.load_asset("%s/T_KitS_Trim_%s" % (TEX_DEST, suffix))
        if t is None:
            notes.append("stone texture missing " + suffix)
            return None
        if suffix == "N":
            _set(t, "compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP, notes)
            _set(t, "srgb", False, notes)
        elif suffix == "ORM":
            _set(t, "compression_settings", unreal.TextureCompressionSettings.TC_MASKS, notes)
            _set(t, "srgb", False, notes)
        eas.save_loaded_asset(t)
        tex[suffix] = t
    mi = instance("MI_Kit_Stone", master_tex)
    mel.set_material_instance_texture_parameter_value(mi, "BaseColorTex", tex["BC"])
    mel.set_material_instance_texture_parameter_value(mi, "ORMTex", tex["ORM"])
    mel.set_material_instance_texture_parameter_value(mi, "NormalTex", tex["N"])
    mel.set_material_instance_scalar_parameter_value(mi, "EmissiveStrength", 0.0)
    eas.save_loaded_asset(mi)
    return mi


def ensure_pillar(notes):
    """M_Kit_LightPillar + MI_Kit_LightPillar_Verdance / _Cinder: additive, unlit, two-sided. Emissive = PillarColor x
    PillarStrength x V^PillarFalloff (bright at the base, V = 1 there in Unreal) x (1 - Fresnel) (soft silhouette)."""
    ensure_dir(DEST + "/Materials")
    folder, name = PILLAR_MASTER.rsplit("/", 1)
    mat = eas.load_asset(PILLAR_MASTER) if eas.does_asset_exist(PILLAR_MASTER) else tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    mel.delete_all_material_expressions(mat)

    def node(cls, x, y):
        return mel.create_material_expression(mat, cls, x, y)
    uv = node(unreal.MaterialExpressionTextureCoordinate, -1200, 0)
    mask = node(unreal.MaterialExpressionComponentMask, -1000, 0)
    mask.set_editor_property("r", False)
    mask.set_editor_property("g", True)
    fall = node(unreal.MaterialExpressionScalarParameter, -1000, 150)
    fall.set_editor_property("parameter_name", "PillarFalloff")
    fall.set_editor_property("default_value", 1.6)
    pw = node(unreal.MaterialExpressionPower, -800, 50)
    fres = node(unreal.MaterialExpressionFresnel, -1000, 300)
    one_minus = node(unreal.MaterialExpressionOneMinus, -800, 300)
    col = node(unreal.MaterialExpressionVectorParameter, -800, -200)
    col.set_editor_property("parameter_name", "PillarColor")
    col.set_editor_property("default_value", unreal.LinearColor(0.1, 1.0, 0.85, 1))
    st = node(unreal.MaterialExpressionScalarParameter, -800, -50)
    st.set_editor_property("parameter_name", "PillarStrength")
    st.set_editor_property("default_value", 14.0)    # r4b: 6 did not read at 300 m against the Verdance haze
    m1 = node(unreal.MaterialExpressionMultiply, -600, 0)
    m2 = node(unreal.MaterialExpressionMultiply, -450, 100)
    m3 = node(unreal.MaterialExpressionMultiply, -300, 0)
    ok = True
    ok &= mel.connect_material_expressions(uv, "", mask, "")
    ok &= mel.connect_material_expressions(mask, "", pw, "Base")
    ok &= mel.connect_material_expressions(fall, "", pw, "Exp")
    ok &= mel.connect_material_expressions(fres, "", one_minus, "")
    ok &= mel.connect_material_expressions(pw, "", m1, "A")
    ok &= mel.connect_material_expressions(one_minus, "", m1, "B")
    ok &= mel.connect_material_expressions(m1, "", m2, "A")
    ok &= mel.connect_material_expressions(st, "", m2, "B")
    ok &= mel.connect_material_expressions(m2, "", m3, "A")
    ok &= mel.connect_material_expressions(col, "", m3, "B")
    ok &= mel.connect_material_property(m3, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if not ok:
        log("FAILED: could not wire " + PILLAR_MASTER)
        return None
    try:
        mel.recompile_material(mat)
    except Exception:
        notes.append("M_Kit_LightPillar compiles on first load")
    eas.save_asset(PILLAR_MASTER, only_if_is_dirty=False)
    out = {}
    for nm, rgb in (("MI_Kit_LightPillar_Verdance", (0.0, 0.9, 0.65)), ("MI_Kit_LightPillar_Cinder", (1.0, 0.4, 0.08))):
        mi = instance(nm, mat)
        mel.set_material_instance_vector_parameter_value(mi, "PillarColor", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
        eas.save_loaded_asset(mi)
        out[nm] = mi
    return out["MI_Kit_LightPillar_Verdance"]


def configure_dominion():
    """The Dominion set: /Props/Kit/Dominion, the T_KitD sheet, red strips, white work lights; the meshes keep the
    DominionInterior names (SM_DI_*) so T5's course switches by its folder string only."""
    global DEST, TEX_DEST, PIPELINE, SIZE_CHECK, GLOWS, NO_COLLISION, SCREEN_MI
    DEST = "/Game/TriadDrift/Gameplay/Props/Kit/Dominion"
    TEX_DEST = DEST + "/Textures"
    PIPELINE = DEST + "/Pipelines/PL_KitD_StaticMesh"
    SIZE_CHECK = os.path.join(KIT, "review", "size_check_D_r6.json")
    # review r1 fix 1: a deep red (renders red, not salmon) at half the strength
    GLOWS = {"MI_KitD_Glow": ("Glow", {"GlowStrength": 3.0, "PulseAmount": 0.0}, (1.0, 0.025, 0.006)),     # #FF2A10 (review r2)
             # r9 (review r4): the ceiling strips dimmed and warmed so the arena stops reading as a clean-room
             "MI_KitD_Light": ("Light", {"GlowStrength": 1.4, "PulseAmount": 0.0}, (1.0, 0.62, 0.42))}
    NO_COLLISION = ()
    SCREEN_MI = None
    SET.update({"stem": "T_KitD_Trim", "trim_mi": "MI_KitD_Trim", "export": "export_dominion", "prefix": "SM_DI_", "extras": False,
                "decals": True})
NO_COLLISION = ("SM_Kit_LightStrip_200", "SM_Kit_LightPillar")

eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def log(msg):
    unreal.log("TDPY: " + msg)


def _set(obj, prop, value, notes):
    try:
        obj.set_editor_property(prop, value)
    except Exception as error:
        notes.append("could not set %s.%s (%s)" % (type(obj).__name__, prop, error))


def ensure_dir(path):
    if not eas.does_directory_exist(path):
        eas.make_directory(path)


def import_textures(notes):
    out = {}
    tasks = []
    for p in sorted(glob.glob(os.path.join(KIT, "textures", SET["stem"] + "_*.png"))):
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", p)
        t.set_editor_property("destination_path", TEX_DEST)
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", False)
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    for suffix in ("BC", "N", "ORM"):
        tex = eas.load_asset("%s/%s_%s" % (TEX_DEST, SET["stem"], suffix))
        if tex is None:
            notes.append("texture missing: " + suffix)
            continue
        if suffix == "N":
            _set(tex, "compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP, notes)
            _set(tex, "srgb", False, notes)
            _set(tex, "flip_green_channel", False, notes)
        elif suffix == "ORM":
            _set(tex, "compression_settings", unreal.TextureCompressionSettings.TC_MASKS, notes)
            _set(tex, "srgb", False, notes)
        else:
            _set(tex, "compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT, notes)
            _set(tex, "srgb", True, notes)
        eas.save_loaded_asset(tex)
        out[suffix] = tex
    return out


def instance(name, parent):
    path = "%s/%s" % (DEST, name)
    mi = eas.load_asset(path) if eas.does_asset_exist(path) else tools.create_asset(
        name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    return mi


SCREEN_MASTER = "/Game/TriadDrift/Gameplay/Props/Kit/Materials/M_Kit_Screen"


def ensure_screen_master(notes):
    """M_Kit_Screen (#62): the console glass shows an image. Parameters: ScreenTexture (texture, the 0..1 UVs cover the
    glass; default T_Kit_ScreenDefault), ScreenPower (emissive strength, default 3), ScreenTint (colour, default white).
    Emissive = ScreenTexture x ScreenTint x ScreenPower over a near-black glossy glass."""
    ensure_dir(DEST + "/Materials")
    tex_path = TEX_DEST + "/T_Kit_ScreenDefault"
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.join(KIT, "textures", "T_Kit_ScreenDefault.png"))
    t.set_editor_property("destination_path", TEX_DEST)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    tools.import_asset_tasks([t])
    default_tex = eas.load_asset(tex_path)
    if default_tex is None:
        log("FAILED: %s" % tex_path)
        return None
    folder, name = SCREEN_MASTER.rsplit("/", 1)
    mat = eas.load_asset(SCREEN_MASTER) if eas.does_asset_exist(SCREEN_MASTER) else tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)

    def node(cls, x, y):
        return mel.create_material_expression(mat, cls, x, y)
    tx = node(unreal.MaterialExpressionTextureSampleParameter2D, -900, 0)
    tx.set_editor_property("parameter_name", "ScreenTexture")
    tx.set_editor_property("texture", default_tex)
    tint = node(unreal.MaterialExpressionVectorParameter, -900, 250)
    tint.set_editor_property("parameter_name", "ScreenTint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    pw = node(unreal.MaterialExpressionScalarParameter, -900, 400)
    pw.set_editor_property("parameter_name", "ScreenPower")
    pw.set_editor_property("default_value", 3.0)
    m1 = node(unreal.MaterialExpressionMultiply, -600, 100)
    m2 = node(unreal.MaterialExpressionMultiply, -400, 150)
    base = node(unreal.MaterialExpressionConstant3Vector, -600, -200)
    base.set_editor_property("constant", unreal.LinearColor(0.01, 0.012, 0.015, 1))
    rough = node(unreal.MaterialExpressionConstant, -600, -80)
    rough.set_editor_property("r", 0.08)
    ok = True
    ok &= mel.connect_material_expressions(tx, "RGB", m1, "A")
    ok &= mel.connect_material_expressions(tint, "", m1, "B")
    ok &= mel.connect_material_expressions(m1, "", m2, "A")
    ok &= mel.connect_material_expressions(pw, "", m2, "B")
    ok &= mel.connect_material_property(m2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    ok &= mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    ok &= mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if not ok:
        log("FAILED: could not wire " + SCREEN_MASTER)
        return None
    try:
        mel.recompile_material(mat)
    except Exception:
        notes.append("M_Kit_Screen compiles on first load")
    eas.save_asset(SCREEN_MASTER, only_if_is_dirty=False)
    return mat


def ensure_pipeline(notes):
    ensure_dir(DEST + "/Pipelines")
    if not eas.does_asset_exist(PIPELINE):
        eas.duplicate_asset("/Interchange/Pipelines/DefaultAssetsPipeline", PIPELINE)
    pipe = eas.load_asset(PIPELINE)
    _set(pipe, "asset_type_sub_folders", False, notes)
    mesh = pipe.get_editor_property("mesh_pipeline")
    for k, v in {"import_static_meshes": True, "import_skeletal_meshes": False,
                 "collision": True, "import_collision_according_to_mesh_name": True, "one_convex_hull_per_ucx": True,
                 "generate_lightmap_u_vs": False, "build_nanite": False}.items():
        _set(mesh, k, v, notes)
    try:
        sp = pipe.get_editor_property("material_pipeline")
        _set(sp, "import_materials", False, notes)
        try:
            _set(sp.get_editor_property("texture_pipeline"), "import_textures", False, notes)
        except Exception:
            pass
    except Exception as error:
        notes.append("pipeline material_pipeline: %s" % error)
    eas.save_loaded_asset(pipe)
    return unreal.SoftObjectPath("%s.%s" % (PIPELINE, PIPELINE.rsplit("/", 1)[-1]))


def main():
    notes = []
    with open(SIZE_CHECK, encoding="utf-8") as fh:
        expect = {r["asset"]: r for r in json.load(fh)}
    ensure_dir(DEST)
    ensure_dir(TEX_DEST)
    master_tex, master_glow = eas.load_asset(MASTER_TEX), eas.load_asset(MASTER_GLOW)
    if master_tex is None or master_glow is None:
        log("FAILED: missing master %s or %s" % (MASTER_TEX, MASTER_GLOW))
        return
    tex = import_textures(notes)
    if len(tex) != 3:
        log("FAILED: atlas textures %s %s" % (sorted(tex), notes))
        return
    mis = {}
    trim_parent = master_tex
    if SET.get("decals"):                          # the Dominion set: its own trim master with the grime variation
        trim_parent = ensure_trim_grime(tex, notes)
        if trim_parent is None:
            return
    atlas = instance(SET["trim_mi"], trim_parent)
    mel.set_material_instance_texture_parameter_value(atlas, "BaseColorTex", tex["BC"])
    mel.set_material_instance_texture_parameter_value(atlas, "ORMTex", tex["ORM"])
    mel.set_material_instance_texture_parameter_value(atlas, "NormalTex", tex["N"])
    mel.set_material_instance_scalar_parameter_value(atlas, "EmissiveStrength", 0.0)
    eas.save_loaded_asset(atlas)
    mis["Trim"] = atlas
    if SET.get("decals"):
        # r11 (Lead): the deflect corridor's own trim and strips: the red band -30 % and toward crimson, the strips -30 %
        # and crimson, so the crew's orange marks do not merge with them (the preview uses them on that corridor only)
        cor = instance("MI_KitD_Trim_Corridor", trim_parent)
        for k_ in ("BC", "ORM", "N"):
            mel.set_material_instance_texture_parameter_value(cor, {"BC": "BaseColorTex", "ORM": "ORMTex", "N": "NormalTex"}[k_], tex[k_])
        mel.set_material_instance_vector_parameter_value(cor, "BandTint", unreal.LinearColor(0.7, 0.42, 0.95, 1.0))
        eas.save_loaded_asset(cor)
        gc = instance("MI_KitD_Glow_Corridor", master_glow)
        mel.set_material_instance_scalar_parameter_value(gc, "GlowStrength", 1.8)
        mel.set_material_instance_scalar_parameter_value(gc, "PulseAmount", 0.0)
        mel.set_material_instance_vector_parameter_value(gc, "GlowColor", unreal.LinearColor(1.0, 0.0, 0.012, 1.0))   # deep crimson (0.85 / 0.07 bloomed pink)
        eas.save_loaded_asset(gc)
    for name, (key, scalars, rgb) in GLOWS.items():
        mi = instance(name, master_glow)
        for k, v in scalars.items():
            mel.set_material_instance_scalar_parameter_value(mi, k, v)
        mel.set_material_instance_vector_parameter_value(mi, "GlowColor", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
        eas.save_loaded_asset(mi)
        if key:
            mis[key] = mi
    if SET.get("decals"):
        dmi = ensure_decal(notes)
        if dmi is None:
            log("FAILED: decals %s" % notes)
            return
        mis["Decal"] = dmi
        mis["Route"] = ROUTE_MI[0]
    if SET.get("extras"):
        stone = ensure_stone(master_tex, notes)
        if stone is None:
            log("FAILED: MI_Kit_Stone %s" % notes)
            return
        mis["Stone"] = stone                       # ART-18: the artefact's slot
        pil = ensure_pillar(notes)
        if pil is None:
            return
        mis["Pillar"] = pil
    if SCREEN_MI:
        scr = ensure_screen_master(notes)
        if scr is None:
            return
        mi = instance(SCREEN_MI, scr)
        eas.save_loaded_asset(mi)
        mis["Screen"] = mi
    pipe = ensure_pipeline(notes)
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    rows, bad = [], 0
    for man_path in sorted(glob.glob(os.path.join(KIT, SET["export"], SET["prefix"] + "*.manifest.json"))):
        with open(man_path, encoding="utf-8") as fh:
            man = json.load(fh)
        asset = man["asset"]
        fbx = os.path.join(KIT, SET["export"], asset + ".fbx")
        n = []
        params = unreal.ImportAssetParameters()
        params.is_automated = True
        params.replace_existing = True
        params.override_pipelines.append(pipe)
        mgr.import_asset(DEST, unreal.InterchangeManager.create_source_data(fbx), params)
        mesh = eas.load_asset("%s/%s" % (DEST, asset))
        if mesh is None:
            log("FAILED %s: not found after import" % asset)
            bad += 1
            continue
        try:
            sme.set_generate_lightmap_uv(mesh, False)
        except Exception as error:
            n.append("lightmap: %s" % error)
        try:
            ns = mesh.get_editor_property("nanite_settings")
            ns.enabled = False
            mesh.set_editor_property("nanite_settings", ns)
        except Exception as error:
            n.append("nanite: %s" % error)
        slots = []
        for i, sm in enumerate(mesh.static_materials):
            slot = str(sm.material_slot_name)
            key = next((k for k in mis if slot.endswith("_" + k) or slot == k), None)
            if key is None:
                n.append("slot %s unmapped" % slot)
                continue
            mesh.set_material(i, mis[key])
            slots.append("%d:%s" % (i, key))
        if asset in NO_COLLISION:
            try:
                sme.remove_collisions(mesh)
            except Exception as error:
                n.append("remove_collisions: %s" % error)
        eas.save_loaded_asset(mesh)
        bb = mesh.get_bounding_box()
        mn = [round(bb.min.x, 2), round(bb.min.y, 2), round(bb.min.z, 2)]
        mx = [round(bb.max.x, 2), round(bb.max.y, 2), round(bb.max.z, 2)]
        try:
            tris = int(mesh.get_num_triangles(0))
        except Exception:
            tris = -1
        # hull extents are not readable from Python in 5.9: count them (they are authored inside the stated box)
        try:
            hulls = int(sme.get_convex_collision_count(mesh))
        except Exception as error:
            hulls = -1
            n.append("hull count: %s" % error)
        side = None
        if asset == "SM_Kit_Railing_400":
            # handedness check on an asymmetric piece: the rail (walkway) / the junction column (corner) must be on +Y
            try:
                desc = mesh.get_static_mesh_description(0)
                ys = []
                for i in range(desc.get_vertex_count()):
                    v = desc.get_vertex_position(unreal.VertexID(i))
                    if v.z > 50.0:
                        ys.append(v.y)
                side = "+Y" if ys and min(ys) > -0.01 else ("-Y" if ys and max(ys) < 0 else "mixed %d" % len(ys))
            except Exception as error:
                n.append("vertex read: %s" % error)
        e = expect.get(asset)
        # Unreal's Y is Blender's -Y through FBX: compare against the mirrored box too and say which one matched
        same = e and all(abs(mn[i] - e["min_cm"][i]) < 0.6 and abs(mx[i] - e["max_cm"][i]) < 0.6 for i in range(3))
        mirrored = e and all(abs(mn[i] - (e["min_cm"][i] if i != 1 else -e["max_cm"][1])) < 0.6
                             and abs(mx[i] - (e["max_cm"][i] if i != 1 else -e["min_cm"][1])) < 0.6 for i in range(3))
        want_col = asset not in NO_COLLISION
        ok = bool(same or mirrored) and tris == man["tris"] and len(slots) == len(mesh.static_materials) \
            and hulls == (len(man.get('collision', [])) if want_col else 0) and side in (None, '+Y')
        if not ok:
            bad += 1
        row = {"asset": asset, "ok": ok, "min_cm": mn, "max_cm": mx, "size_cm": [round(mx[i] - mn[i], 2) for i in range(3)],
               "bounds_match": "same" if same else ("y-mirrored" if mirrored else "NO"), "tris": tris, "slots": slots,
               "hulls": hulls, "asymmetric_side": side, "notes": n}
        rows.append(row)
        log(json.dumps(row))
    out = os.path.join(KIT, "review", "ue_import_report.json")
    with open(out, "w", encoding="utf-8") as fh:
        json.dump(rows, fh, indent=1)
    log("report: %s; notes %s" % (out, notes))
    log("imported %d mesh(es), %d failed" % (len(rows), bad))
    if rows and not bad:
        log("OK")
    else:
        log("FAILED: see the lines above")


if __name__ == "__main__":
    if os.environ.get("TD_KIT_SET") == "dominion":
        configure_dominion()
    main()

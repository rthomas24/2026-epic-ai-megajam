# SPDX-License-Identifier: MIT
# Triad Drift - the Dominion mothership exterior kit: materials, meshes and the visuals data asset. Headless:
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngShips\Dominion\scripts\ue_import_dominion.py
#
# Writes ONLY under /Game/TriadDrift/Gameplay/Props/Dominion and /Game/TriadDrift/Gameplay/Characters/Dominion:
#   Props/Dominion/T_Dominion_PanelMask, T_Dominion_PanelN       tiling panel textures (linear data)
#   Props/Dominion/M_TD_DominionPanel                            lit master: triplanar in the object's local space,
#        scale-aware (the code's placeholder hull is a cube scaled to 600 x 160 x 100 m), two tile sizes, world-space
#        normal from the panel normal map, sparse light strips. No UVs needed.
#   Props/Dominion/MI_TD_DominionPanel (the path ATDDominion::HullMaterial loads), MI_TD_DominionTrim
#   Props/Dominion/M_TD_DominionGlow + MI_TD_DominionGlowRed / GlowWhite / GlowEngine
#   Props/Dominion/SM_Dominion_* (Nanite, UCX collision, slots mapped by name to the instances above)
#   Characters/Dominion/DA_Dominion_Visuals (UTDDominionVisuals; found by name when Presentation has none)
# Idempotent. Masters are created once; delete them (and their instances) to rebuild a graph.
# Pass "materials" as TD_DOMINION_STAGE env var to stop after the materials.

import glob
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
REPO = os.path.dirname(os.path.dirname(os.path.dirname(KIT)))
DEST = "/Game/TriadDrift/Gameplay/Props/Dominion"
DA_DEST = "/Game/TriadDrift/Gameplay/Characters/Dominion"
PANEL = DEST + "/M_TD_DominionPanel"
GLOW = DEST + "/M_TD_DominionGlow"
PIPELINE = DEST + "/PL_Dominion_StaticMesh"

eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
NOTES = []


def log(msg):
    unreal.log("TDPY: " + msg)


def _set(obj, prop, value, notes=NOTES):
    try:
        obj.set_editor_property(prop, value)
        return True
    except Exception as error:
        notes.append("could not set %s.%s (%s)" % (type(obj).__name__, prop, error))
        return False


def ensure_dir(path):
    if not eas.does_directory_exist(path):
        eas.make_directory(path)


def _enum(cls_name, *names):
    cls = getattr(unreal, cls_name, None)
    if cls is None:
        raise RuntimeError("no enum " + cls_name)
    for n in names:
        if hasattr(cls, n):
            return getattr(cls, n)
    raise RuntimeError("%s has none of %s (has %s)" % (cls_name, names, [a for a in dir(cls) if a.isupper()]))


# ------------------------------------------------------------------------------------------------ textures

def import_textures():
    paths = sorted(glob.glob(os.path.join(KIT, "textures", "T_Dominion_*.png")))
    tasks = []
    for p in paths:
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", p)
        t.set_editor_property("destination_path", DEST)
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", False)
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    out = {}
    for p in paths:
        name = os.path.splitext(os.path.basename(p))[0]
        tex = eas.load_asset("%s/%s" % (DEST, name))
        if tex is None:
            NOTES.append("texture not imported: " + name)
            continue
        _set(tex, "srgb", False)
        if name.endswith("_PanelN"):
            # sampled raw in the triplanar code (x, y = 2 * RG - 1): BC5 keeps RG exactly
            _set(tex, "compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            _set(tex, "flip_green_channel", False)
        else:
            _set(tex, "compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        eas.save_loaded_asset(tex)
        out[name] = tex
    return out


# ------------------------------------------------------------------------------------------------ materials

PANEL_HLSL = r"""
float3 s = float3(length(Ax), length(Ay), length(Az));
float3 p = P * s;                                   // local position in cm (scale-aware)
float3 n = normalize(N * s * s + 1e-6);             // inverse-transpose of the world normal into local space
float3 w = pow(abs(n), 4.0); w /= (w.x + w.y + w.z);
float2 uX = p.yz / Tile, uY = p.xz / Tile, uZ = p.xy / Tile;
float4 m = Texture2DSample(MaskTex, MaskTexSampler, uX) * w.x
         + Texture2DSample(MaskTex, MaskTexSampler, uY) * w.y
         + Texture2DSample(MaskTex, MaskTexSampler, uZ) * w.z;
float2 o = float2(0.37, 0.61);
float4 M = Texture2DSample(MaskTex, MaskTexSampler, p.yz / MacroTile + o) * w.x
         + Texture2DSample(MaskTex, MaskTexSampler, p.xz / MacroTile + o) * w.y
         + Texture2DSample(MaskTex, MaskTexSampler, p.xy / MacroTile + o) * w.z;
float2 tX = Texture2DSample(NrmTex, NrmTexSampler, uX).xy * 2.0 - 1.0;
float2 tY = Texture2DSample(NrmTex, NrmTexSampler, uY).xy * 2.0 - 1.0;
float2 tZ = Texture2DSample(NrmTex, NrmTexSampler, uZ).xy * 2.0 - 1.0;
float3 pert = w.x * float3(0.0, tX.x, tX.y) + w.y * float3(tY.x, 0.0, tY.y) + w.z * float3(tZ.x, tZ.y, 0.0);
float3 nl = normalize(n + pert * NrmStr);
OutNrm = nl / (s * s);                              // feed Local->World (R S): gives R S^-1 nl = the world normal
float macroSeam = lerp(1.0, M.g, MacroStr);
float cav = saturate(m.g * macroSeam);
float tone = saturate(m.r * 0.75 + (M.r - 0.5) * MacroStr * 1.2 + 0.1);
float3 base = lerp(ColA, ColB, tone);
base = lerp(SeamCol, base, cav);
OutAO = lerp(0.35, 1.0, cav);
OutRough = saturate(Rough + (m.r - 0.5) * 0.16 + (1.0 - cav) * 0.25);
OutMetal = saturate(Metal);
OutEmis = LightCol * LightStr * m.a * macroSeam;
return base;
"""


def _expr(mat, cls, x, y, **props):
    n = mel.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        _set(n, k, v)
    return n


def _scalar(mat, name, value, x, y, group="Dominion"):
    return _expr(mat, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=value, group=group)


def _vector(mat, name, rgb, x, y, group="Dominion"):
    return _expr(mat, unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name,
                 default_value=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0), group=group)


def _connect(a, ao, b, bi, errors):
    if not mel.connect_material_expressions(a, ao, b, bi):
        errors.append("connect %s.%s -> %s.%s" % (a.get_name(), ao, b.get_name(), bi))


def _prop(a, ao, prop, errors):
    if not mel.connect_material_property(a, ao, prop):
        errors.append("connect %s.%s -> %s" % (a.get_name(), ao, prop))


def build_panel(textures):
    ensure_dir(DEST)
    if eas.does_asset_exist(PANEL):
        return eas.load_asset(PANEL)
    folder, name = PANEL.rsplit("/", 1)
    mat = tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    for flag in ("used_with_nanite", "used_with_instanced_static_meshes", "used_with_static_lighting"):
        _set(mat, flag, True)
    _set(mat, "tangent_space_normal", False)
    errors = []
    wp = _expr(mat, unreal.MaterialExpressionWorldPosition, -2200, -300)
    tp = _expr(mat, unreal.MaterialExpressionTransformPosition, -1950, -300)
    _set(tp, "transform_source_type", _enum("MaterialPositionTransformSource", "TRANSFORMPOSSOURCE_WORLD"))
    _set(tp, "transform_type", _enum("MaterialPositionTransformSource", "TRANSFORMPOSSOURCE_LOCAL"))
    _connect(wp, "", tp, "", errors)
    vsrc_local = _enum("MaterialVectorCoordTransformSource", "TRANSFORMSOURCE_LOCAL")
    vsrc_world = _enum("MaterialVectorCoordTransformSource", "TRANSFORMSOURCE_WORLD")
    vdst_world = _enum("MaterialVectorCoordTransform", "TRANSFORM_WORLD")
    vdst_local = _enum("MaterialVectorCoordTransform", "TRANSFORM_LOCAL")
    axes = []
    for i, v in enumerate(((1, 0, 0), (0, 1, 0), (0, 0, 1))):
        c = _expr(mat, unreal.MaterialExpressionConstant3Vector, -2200, -100 + 120 * i,
                  constant=unreal.LinearColor(v[0], v[1], v[2], 0.0))
        t = _expr(mat, unreal.MaterialExpressionTransform, -1950, -100 + 120 * i,
                  transform_source_type=vsrc_local, transform_type=vdst_world)
        _connect(c, "", t, "", errors)
        axes.append(t)
    vn = _expr(mat, unreal.MaterialExpressionVertexNormalWS, -2200, 300)
    vnl = _expr(mat, unreal.MaterialExpressionTransform, -1950, 300, transform_source_type=vsrc_world, transform_type=vdst_local)
    _connect(vn, "", vnl, "", errors)
    mask = _expr(mat, unreal.MaterialExpressionTextureObjectParameter, -1950, 450, parameter_name="PanelMask",
                 texture=textures.get("T_Dominion_PanelMask"))
    nrm = _expr(mat, unreal.MaterialExpressionTextureObjectParameter, -1950, 650, parameter_name="PanelNormal",
                texture=textures.get("T_Dominion_PanelN"))
    _set(mask, "sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    _set(nrm, "sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    params = [
        ("Tile", _scalar(mat, "TileCm", 3200.0, -1700, 800)),
        ("MacroTile", _scalar(mat, "MacroTileCm", 25600.0, -1700, 880)),
        ("ColA", _vector(mat, "ColorDark", (0.011, 0.012, 0.014), -1700, 960)),
        ("ColB", _vector(mat, "ColorLight", (0.034, 0.036, 0.042), -1700, 1060)),
        ("SeamCol", _vector(mat, "SeamColor", (0.002, 0.002, 0.003), -1700, 1160)),
        ("Metal", _scalar(mat, "Metallic", 0.55, -1700, 1260)),
        ("Rough", _scalar(mat, "Roughness", 0.42, -1700, 1340)),
        ("NrmStr", _scalar(mat, "NormalStrength", 1.0, -1700, 1420)),
        ("MacroStr", _scalar(mat, "MacroStrength", 0.6, -1700, 1500)),
        ("LightCol", _vector(mat, "LightColor", (1.0, 0.14, 0.02), -1700, 1580)),
        ("LightStr", _scalar(mat, "LightStrength", 8.0, -1700, 1680)),
    ]
    f1 = _enum("CustomMaterialOutputType", "CMOT_FLOAT1")
    f3 = _enum("CustomMaterialOutputType", "CMOT_FLOAT3")
    def cin(n):
        c = unreal.CustomInput()
        c.set_editor_property("input_name", n)
        return c

    def cout(n, t):
        c = unreal.CustomOutput()
        c.set_editor_property("output_name", n)
        c.set_editor_property("output_type", t)
        return c
    inputs = [cin(n) for n in ("P", "N", "Ax", "Ay", "Az", "MaskTex", "NrmTex")] + [cin(n) for n, _ in params]
    outs = [cout("OutRough", f1), cout("OutMetal", f1), cout("OutAO", f1), cout("OutEmis", f3), cout("OutNrm", f3)]
    cu = _expr(mat, unreal.MaterialExpressionCustom, -1200, 0, code=PANEL_HLSL, output_type=f3,
               description="Dominion triplanar panel", inputs=inputs, additional_outputs=outs)
    for src, pin in ((tp, "P"), (vnl, "N"), (axes[0], "Ax"), (axes[1], "Ay"), (axes[2], "Az"), (mask, "MaskTex"),
                     (nrm, "NrmTex")):
        _connect(src, "", cu, pin, errors)
    for pin, node in params:
        _connect(node, "", cu, pin, errors)
    nw = _expr(mat, unreal.MaterialExpressionTransform, -800, 400, transform_source_type=vsrc_local, transform_type=vdst_world)
    _connect(cu, "OutNrm", nw, "", errors)
    nn = _expr(mat, unreal.MaterialExpressionNormalize, -600, 400)
    _connect(nw, "", nn, "", errors)
    mp = unreal.MaterialProperty
    _prop(cu, "", mp.MP_BASE_COLOR, errors)
    _prop(cu, "OutRough", mp.MP_ROUGHNESS, errors)
    _prop(cu, "OutMetal", mp.MP_METALLIC, errors)
    _prop(cu, "OutAO", mp.MP_AMBIENT_OCCLUSION, errors)
    _prop(cu, "OutEmis", mp.MP_EMISSIVE_COLOR, errors)
    _prop(nn, "", mp.MP_NORMAL, errors)
    if errors:
        log("FAILED wiring %s: %s" % (PANEL, errors))
        return None
    try:
        mel.recompile_material(mat)
    except Exception:
        pass
    eas.save_asset(PANEL, only_if_is_dirty=False)
    log("built " + PANEL)
    return mat


def build_glow():
    # TD_DOMINION_REBUILD=Glow rebuilds the graph in place (same asset, every instance keeps its parent)
    rebuild = "Glow" in os.environ.get("TD_DOMINION_REBUILD", "")
    if eas.does_asset_exist(GLOW):
        mat = eas.load_asset(GLOW)
        if not rebuild:
            return mat
        mel.delete_all_material_expressions(mat)
    else:
        folder, name = GLOW.rsplit("/", 1)
        mat = tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    for flag in ("used_with_nanite", "used_with_instanced_static_meshes"):
        _set(mat, flag, True)
    errors = []
    col = _vector(mat, "GlowColor", (1.0, 0.25, 0.04), -900, -200)
    strength = _scalar(mat, "GlowStrength", 20.0, -900, -60)
    rate = _scalar(mat, "PulseRate", 1.2, -1300, 120)
    amount = _scalar(mat, "PulseAmount", 0.0, -900, 260)
    base = _scalar(mat, "BaseLevel", 0.04, -900, -320)
    time = _expr(mat, unreal.MaterialExpressionTime, -1300, 40)
    tm = _expr(mat, unreal.MaterialExpressionMultiply, -1100, 80)
    sn = _expr(mat, unreal.MaterialExpressionSine, -950, 80)
    am = _expr(mat, unreal.MaterialExpressionMultiply, -750, 160)
    one = _expr(mat, unreal.MaterialExpressionAdd, -600, 160, const_b=1.0)
    cs = _expr(mat, unreal.MaterialExpressionMultiply, -600, -150)
    em = _expr(mat, unreal.MaterialExpressionMultiply, -400, -60)
    bc = _expr(mat, unreal.MaterialExpressionMultiply, -400, -320)
    _connect(time, "", tm, "A", errors)
    _connect(rate, "", tm, "B", errors)
    _connect(tm, "", sn, "", errors)
    _connect(sn, "", am, "A", errors)
    _connect(amount, "", am, "B", errors)
    _connect(am, "", one, "A", errors)
    _connect(col, "", cs, "A", errors)
    _connect(strength, "", cs, "B", errors)
    _connect(cs, "", em, "A", errors)
    _connect(one, "", em, "B", errors)
    _connect(col, "", bc, "A", errors)
    _connect(base, "", bc, "B", errors)
    mp = unreal.MaterialProperty
    # KeepFraction (default 1 = every light on): only the lights whose cell (KeepCellCm, local position) hashes below
    # it stay lit, the rest go dark: rows of window squares become sparse "eye" slits without touching the mesh
    wp = _expr(mat, unreal.MaterialExpressionWorldPosition, -1300, 400)
    tp = _expr(mat, unreal.MaterialExpressionTransformPosition, -1100, 400)
    _set(tp, "transform_source_type", _enum("MaterialPositionTransformSource", "TRANSFORMPOSSOURCE_WORLD"))
    _set(tp, "transform_type", _enum("MaterialPositionTransformSource", "TRANSFORMPOSSOURCE_LOCAL"))
    _connect(wp, "", tp, "", errors)
    keep = _scalar(mat, "KeepFraction", 1.0, -1300, 520)
    cell = _scalar(mat, "KeepCellCm", 400.0, -1300, 600)

    def cin(n):
        c = unreal.CustomInput()
        c.set_editor_property("input_name", n)
        return c
    cu = _expr(mat, unreal.MaterialExpressionCustom, -800, 450, output_type=_enum("CustomMaterialOutputType", "CMOT_FLOAT1"),
               code="float3 c = floor(P / max(C, 1.0)); float h = frac(sin(dot(c, float3(12.9898, 78.233, 37.719))) * 43758.5453); return (K >= 0.999) ? 1.0 : step(h, K);",
               description="sparse lights", inputs=[cin("P"), cin("K"), cin("C")])
    _connect(tp, "", cu, "P", errors)
    _connect(keep, "", cu, "K", errors)
    _connect(cell, "", cu, "C", errors)
    em2 = _expr(mat, unreal.MaterialExpressionMultiply, -250, 100)
    _connect(em, "", em2, "A", errors)
    _connect(cu, "", em2, "B", errors)
    _prop(bc, "", mp.MP_BASE_COLOR, errors)
    _prop(em2, "", mp.MP_EMISSIVE_COLOR, errors)
    rough = _expr(mat, unreal.MaterialExpressionConstant, -400, 100, r=0.3)
    _prop(rough, "", mp.MP_ROUGHNESS, errors)
    if errors:
        log("FAILED wiring %s: %s" % (GLOW, errors))
        return None
    try:
        mel.recompile_material(mat)
    except Exception:
        pass
    eas.save_asset(GLOW, only_if_is_dirty=False)
    log("built " + GLOW)
    return mat


# name -> (parent, scalars, vectors)
INSTANCES = {
    "MI_TD_DominionPanel": ("panel", {"LightStrength": 8.0}, {"LightColor": (1.0, 0.14, 0.02)}),
    "MI_TD_DominionTrim": ("panel", {"TileCm": 1200.0, "MacroTileCm": 9600.0, "Metallic": 0.6, "Roughness": 0.52,
                                      "MacroStrength": 0.35, "LightStrength": 0.0, "NormalStrength": 0.8},
                           {"ColorDark": (0.006, 0.006, 0.007), "ColorLight": (0.016, 0.016, 0.018)}),
    "MI_TD_DominionArmor": ("panel", {"TileCm": 6400.0, "MacroTileCm": 38400.0, "Metallic": 0.5, "Roughness": 0.4,
                                       "MacroStrength": 0.3, "LightStrength": 0.0, "NormalStrength": 0.6},
                            {"ColorDark": (0.026, 0.027, 0.031), "ColorLight": (0.07, 0.072, 0.08)}),
    "MI_TD_DominionGlowRed": ("glow", {"GlowStrength": 20.0, "PulseAmount": 0.0}, {"GlowColor": (1.0, 0.14, 0.02)}),
    "MI_TD_DominionGlowWhite": ("glow", {"GlowStrength": 30.0, "PulseAmount": 0.06, "PulseRate": 0.8},
                                {"GlowColor": (0.9, 0.95, 1.0)}),
    "MI_TD_DominionGlowEngine": ("glow", {"GlowStrength": 45.0, "PulseAmount": 0.08, "PulseRate": 7.0},
                                 {"GlowColor": (1.0, 0.3, 0.05)}),
}
# The exterior in the Null code (second ship builder, Nov 14; Lead: it read violet-blue with rows of bright white window
# squares and a light-grey superstructure). Parameters only: armour near-black with a slight warm tint, less metal and
# more roughness (the magenta / blue sky no longer lies on it as a violet sheen), the light-grey superstructure down
# to the same value, red-orange seam light along the armour's major panel breaks, the white window rows cut to a third
# and dimmed. The red bay (GlowRed) and the trench guide lines are not touched. TD_DOMINION_LOOK=old keeps the values above.
if os.environ.get("TD_DOMINION_LOOK", "") != "old":
    # second set (review of the first: seams amber like the Bastion and the guide lines, white lights half-cut and
    # patchy, the warm tint brown under the nebula): seams a true red at a lower strength (a strong emissive tone-maps
    # to yellow), lights kept or dropped by 15 m cells (whole clusters), armour neutral near-black
    RED = (1.0, 0.045, 0.012)
    INSTANCES["MI_TD_DominionPanel"] = ("panel", {"LightStrength": 5.0, "Metallic": 0.3, "Roughness": 0.6},
                                         {"LightColor": RED, "ColorDark": (0.0080, 0.0077, 0.0075), "ColorLight": (0.022, 0.021, 0.0205)})
    INSTANCES["MI_TD_DominionTrim"] = ("panel", {"TileCm": 1200.0, "MacroTileCm": 9600.0, "Metallic": 0.3, "Roughness": 0.62,
                                                 "MacroStrength": 0.35, "LightStrength": 0.0, "NormalStrength": 0.8},
                                        {"ColorDark": (0.0058, 0.0056, 0.0054), "ColorLight": (0.014, 0.0135, 0.013)})
    INSTANCES["MI_TD_DominionArmor"] = ("panel", {"TileCm": 6400.0, "MacroTileCm": 38400.0, "Metallic": 0.3, "Roughness": 0.6,
                                                  "MacroStrength": 0.3, "LightStrength": 4.0, "NormalStrength": 0.6},
                                         {"ColorDark": (0.0095, 0.0092, 0.0089), "ColorLight": (0.024, 0.023, 0.0224), "LightColor": RED})
    INSTANCES["MI_TD_DominionGlowWhite"] = ("glow", {"GlowStrength": 16.0, "PulseAmount": 0.06, "PulseRate": 0.8,
                                                      "KeepFraction": 0.4, "KeepCellCm": 1500.0},
                                             {"GlowColor": (1.0, 1.0, 1.0)})
else:
    INSTANCES["MI_TD_DominionPanel"][1].update({"Metallic": 0.55, "Roughness": 0.42})
    INSTANCES["MI_TD_DominionPanel"][2].update({"ColorDark": (0.011, 0.012, 0.014), "ColorLight": (0.034, 0.036, 0.042)})
    INSTANCES["MI_TD_DominionArmor"][2].update({"LightColor": (1.0, 0.14, 0.02)})
    INSTANCES["MI_TD_DominionGlowWhite"][1].update({"KeepFraction": 1.0, "KeepCellCm": 400.0})
SLOT_TO_MI = {"Panel": "MI_TD_DominionPanel", "Armor": "MI_TD_DominionArmor", "Trim": "MI_TD_DominionTrim", "GlowRed": "MI_TD_DominionGlowRed",
              "GlowWhite": "MI_TD_DominionGlowWhite", "GlowEngine": "MI_TD_DominionGlowEngine"}


def build_instances(panel, glow):
    out = {}
    for name, (parent, scalars, vectors) in INSTANCES.items():
        path = "%s/%s" % (DEST, name)
        mi = eas.load_asset(path) if eas.does_asset_exist(path) else tools.create_asset(
            name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, panel if parent == "panel" else glow)
        for k, v in scalars.items():
            mel.set_material_instance_scalar_parameter_value(mi, k, v)
        for k, v in vectors.items():
            mel.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
        eas.save_loaded_asset(mi)
        out[name] = mi
    return out


# ------------------------------------------------------------------------------------------------ meshes

def ensure_pipeline():
    if not eas.does_asset_exist(PIPELINE):
        eas.duplicate_asset("/Interchange/Pipelines/DefaultAssetsPipeline", PIPELINE)
    pipe = eas.load_asset(PIPELINE)
    _set(pipe, "asset_type_sub_folders", False)
    mesh = pipe.get_editor_property("mesh_pipeline")
    for k, v in {"import_static_meshes": True, "import_skeletal_meshes": False, "combine_static_meshes": True,
                 "collision": True, "import_collision_according_to_mesh_name": True,
                 "one_convex_hull_per_ucx": True, "generate_lightmap_u_vs": False, "build_nanite": True}.items():
        _set(mesh, k, v)
    try:
        sp = pipe.get_editor_property("material_pipeline")
        _set(sp, "import_materials", False)
        try:
            _set(sp.get_editor_property("texture_pipeline"), "import_textures", False)
        except Exception:
            pass
    except Exception as error:
        NOTES.append("pipeline material_pipeline: %s" % error)
    eas.save_loaded_asset(pipe)
    return unreal.SoftObjectPath("%s.%s" % (PIPELINE, PIPELINE.rsplit("/", 1)[-1]))


def import_mesh(fbx):
    params = unreal.ImportAssetParameters()
    params.is_automated = True
    params.replace_existing = True
    params.override_pipelines.append(ensure_pipeline())
    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    mgr.import_asset(DEST, unreal.InterchangeManager.create_source_data(fbx), params)


def main():
    ensure_dir(DEST)
    ensure_dir(DA_DEST)
    tex = import_textures()
    panel = build_panel(tex)
    glow = build_glow()
    if panel is None or glow is None:
        log("FAILED: masters")
        return
    mis = build_instances(panel, glow)
    log("instances: " + ", ".join(sorted(mis)))
    rows = []
    if os.environ.get("TD_DOMINION_STAGE", "") == "materials":
        log("NOTES: " + "; ".join(NOTES))
        log("OK")
        return
    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    meshes = {}
    bad = 0
    for man_path in sorted(glob.glob(os.path.join(KIT, "SM_Dominion_*", "SM_Dominion_*.manifest.json"))):
        with open(man_path, encoding="utf-8") as fh:
            man = json.load(fh)
        asset = man["asset"]
        fbx = os.path.join(os.path.dirname(man_path), asset + ".fbx")
        notes = []
        import_mesh(fbx)
        mesh = eas.load_asset("%s/%s" % (DEST, asset))
        if mesh is None:
            log("FAILED %s: not found after import" % asset)
            bad += 1
            continue
        try:
            sme.set_generate_lightmap_uv(mesh, False)
        except Exception as error:
            notes.append("lightmap: %s" % error)
        try:
            ns = mesh.get_editor_property("nanite_settings")
            ns.enabled = True
            mesh.set_editor_property("nanite_settings", ns)
        except Exception as error:
            notes.append("nanite: %s" % error)
        slots = []
        for i, sm in enumerate(mesh.static_materials):
            slot = str(sm.material_slot_name)
            key = next((k for k in SLOT_TO_MI if slot.endswith("_" + k) or slot == k), None)
            if key is None:
                notes.append("slot %s has no instance mapping" % slot)
                continue
            mesh.set_material(i, mis[SLOT_TO_MI[key]])
            slots.append("%s->%s" % (slot, SLOT_TO_MI[key]))
        hulls = sme.get_convex_collision_count(mesh)
        eas.save_loaded_asset(mesh)
        b = mesh.get_bounds()
        try:
            tris = int(mesh.get_num_triangles(0))
        except Exception:
            tris = -1
        row = {"asset": asset, "tris": tris,
               "size_cm": [round(b.box_extent.x * 2, 1), round(b.box_extent.y * 2, 1), round(b.box_extent.z * 2, 1)],
               "bounds_origin_cm": [round(b.origin.x, 1), round(b.origin.y, 1), round(b.origin.z, 1)],
               "manifest_size_cm": man.get("size_cm"), "collision_hulls": hulls, "slots": slots, "notes": notes}
        rows.append(row)
        meshes[asset] = mesh
        log(json.dumps(row))
    cls = unreal.load_class(None, "/Script/unrealGameJam.TDDominionVisuals")
    da = None
    if cls is None:
        log("FAILED: class TDDominionVisuals not found")
        bad += 1
    elif meshes:
        path = DA_DEST + "/DA_Dominion_Visuals"
        da = eas.load_asset(path) if eas.does_asset_exist(path) else None
        if da is None:
            factory = unreal.DataAssetFactory()
            factory.set_editor_property("data_asset_class", cls)
            da = tools.create_asset("DA_Dominion_Visuals", DA_DEST, cls, factory)
        for prop, asset in (("hull_mesh", "SM_Dominion_Hull"), ("bridge_mesh", "SM_Dominion_Bridge"),
                            ("trench_mesh", "SM_Dominion_TrenchWall"), ("shield_emitter_mesh", "SM_Dominion_ShieldEmitter"),
                            ("generator_mesh", "SM_Dominion_Generator"), ("reactor_mesh", "SM_Dominion_Reactor"),
                            ("generator_core_mesh", "SM_Dominion_GeneratorCore")):
            if asset in meshes:
                _set(da, prop, meshes[asset])
        eas.save_loaded_asset(da)
        got = {p: str(da.get_editor_property(p)) for p in ("hull_mesh", "bridge_mesh", "trench_mesh", "shield_emitter_mesh")}
        log("DA_Dominion_Visuals: " + json.dumps(got))
        rows.append({"data_asset": path, "properties": got})
        if eas.does_asset_exist("/Game/TriadDrift/Presentation/DA_Dominion_Visuals"):
            log("NOTE: /Game/TriadDrift/Presentation/DA_Dominion_Visuals exists and wins over this one")
    out = os.path.join(REPO, "Saved", "Dominion", "Dominion_import_report.json")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, "w", encoding="utf-8") as fh:
        json.dump({"rows": rows, "notes": NOTES}, fh, indent=2)
    log("NOTES: " + "; ".join(NOTES))
    log("imported %d mesh(es), DA %s, %d failed" % (len(meshes), "yes" if da else "no", bad))
    if not bad:
        log("OK")
    else:
        log("FAILED: see above")


main()

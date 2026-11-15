# Triad Drift - per-pilot suits (issue #209): add SuitTint / SuitAccent / SuitAccentGlow to M_Pilot (mask derived in the
# material from the base colour: light, unsaturated = armour panels; saturated = trim / accent), keep the old material as
# M_Pilot_v1, and make MI_Pilot_Wren / _Kato / _Moss beside it. Defaults leave the current look unchanged.
#   .\Tools\td-python.ps1 -Script ArtSource\EngCharacters\Pilot\suits\ue_pilot_suits.py
# Animation builder (eng/anim), Nov 14.
import unreal

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
P = "/Game/TriadDrift/Gameplay/Characters/Pilot/"


def log(m): unreal.log("TDPY: " + str(m))


if not EAL.does_asset_exist(P + "M_Pilot_v1"):
    EAL.duplicate_asset(P + "M_Pilot", P + "M_Pilot_v1")
    EAL.save_asset(P + "M_Pilot_v1", only_if_is_dirty=False)
    log("kept M_Pilot_v1")
m = unreal.load_asset(P + "M_Pilot")
exprs = MEL.get_material_expressions(m)
if any(e.get_class().get_name() == "MaterialExpressionCustom" for e in exprs):
    log("M_Pilot already has the suit nodes; leaving the graph")
else:
    bc = MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_BASE_COLOR)

    def vparam(name, val, y):
        e = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -900, y)
        e.set_editor_property("parameter_name", name)
        e.set_editor_property("default_value", val)
        return e

    tint = vparam("SuitTint", unreal.LinearColor(1, 1, 1, 1), 300)
    acc = vparam("SuitAccent", unreal.LinearColor(0.0, 0.85, 1.0, 1), 500)
    glow = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -900, 700)
    glow.set_editor_property("parameter_name", "SuitAccentGlow")
    glow.set_editor_property("default_value", 0.0)

    def custom(code, y, out_type):
        c = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -400, y)
        c.set_editor_property("code", code)
        c.set_editor_property("output_type", out_type)
        ins = []
        for n in ("BC", "Tint", "Accent", "Glow"):
            ci = unreal.CustomInput()
            ci.set_editor_property("input_name", n)
            ins.append(ci)
        c.set_editor_property("inputs", ins)
        return c

    masks = ("float lum = dot(BC.rgb, float3(0.299, 0.587, 0.114));\n"
             "float sat = max(BC.r, max(BC.g, BC.b)) - min(BC.r, min(BC.g, BC.b));\n"
             "float panel = saturate((lum - 0.42) / 0.16) * (1.0 - saturate(sat * 5.0));\n"
             "float accent = saturate((sat - 0.22) * 6.0);\n")
    col = custom(masks + "float3 c = lerp(BC.rgb, BC.rgb * Tint.rgb, panel);\n"
                 "float3 hue = Accent.rgb * (0.35 + 1.2 * max(BC.r, max(BC.g, BC.b)));\n"
                 "return lerp(c, hue, accent);", 0, unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    emi = custom(masks + "return Accent.rgb * accent * Glow;", 250, unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    for c in (col, emi):
        MEL.connect_material_expressions(bc, "RGB", c, "BC")
        MEL.connect_material_expressions(tint, "", c, "Tint")
        MEL.connect_material_expressions(acc, "", c, "Accent")
        MEL.connect_material_expressions(glow, "", c, "Glow")
    MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(emi, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    # no recompile here (td-python runs -nullrhi): the shaders compile on the next rendering load
    m.modify()
    EAL.save_asset(P + "M_Pilot", only_if_is_dirty=False)
    log("M_Pilot: SuitTint, SuitAccent, SuitAccentGlow added")

SUITS = {
    "MI_Pilot_Wren": (unreal.LinearColor(1.0, 0.95, 0.9, 1), unreal.LinearColor(1.0, 0.12, 0.75, 1), 3.0),   # warm white, magenta
    "MI_Pilot_Kato": (unreal.LinearColor(0.88, 0.95, 1.0, 1), unreal.LinearColor(0.05, 0.85, 1.0, 1), 3.0),  # cool white, cyan
    "MI_Pilot_Moss": (unreal.LinearColor(0.95, 0.82, 0.6, 1), unreal.LinearColor(1.0, 0.55, 0.05, 1), 3.0),  # sand, amber
}
base = unreal.load_asset(P + "MI_Pilot")
for name, (tint, accent, glow) in SUITS.items():
    path = P + name
    mi = unreal.load_asset(path) if EAL.does_asset_exist(path) else EAL.duplicate_asset(P + "MI_Pilot", path)
    mi.set_editor_property("parent", m)
    MEL.set_material_instance_vector_parameter_value(mi, "SuitTint", tint)
    MEL.set_material_instance_vector_parameter_value(mi, "SuitAccent", accent)
    MEL.set_material_instance_scalar_parameter_value(mi, "SuitAccentGlow", glow)
    MEL.update_material_instance(mi)
    EAL.save_asset(path, only_if_is_dirty=False)
    log("%s: tint %s accent %s glow %.1f" % (path, tint, accent, glow))
log("vec params now %s" % [str(x) for x in MEL.get_vector_parameter_names(m)])
log("OK")

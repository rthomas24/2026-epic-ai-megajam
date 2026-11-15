# Triad Drift - pilot suits r2 (issue #209, Lead's review of r1): a third mask by luminance (dark, unsaturated: the grey
# panels and undersuit) takes a strong pilot colour: SuitDark (colour), SuitDarkAmount (0 = unchanged, the default) and
# SuitDarkGlow (small emissive lift). The visor taking the accent colour is accepted. Moss's panels warmer.
#   .\Tools\td-python.ps1 -Script ArtSource\EngCharacters\Pilot\suits\ue_pilot_suits_r2.py
# Animation builder (eng/anim), Nov 14.
import unreal

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
P = "/Game/TriadDrift/Gameplay/Characters/Pilot/"


def log(m): unreal.log("TDPY: " + str(m))


m = unreal.load_asset(P + "M_Pilot")
exprs = MEL.get_material_expressions(m)
byname = {}
for e in exprs:
    try:
        byname[str(e.get_editor_property("parameter_name"))] = e
    except Exception:
        pass
customs = [e for e in exprs if e.get_class().get_name() == "MaterialExpressionCustom"]
bc = MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_BASE_COLOR)
tex = [e for e in exprs if e.get_class().get_name() == "MaterialExpressionTextureSampleParameter2D" and str(e.get_editor_property("parameter_name")) == "BaseColorTex"][0]


def vparam(name, val, y):
    if name in byname:
        return byname[name]
    e = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -900, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", val)
    return e


def sparam(name, val, y):
    if name in byname:
        return byname[name]
    e = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -900, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", val)
    return e


tint, acc, glow = byname["SuitTint"], byname["SuitAccent"], byname["SuitAccentGlow"]
dark = vparam("SuitDark", unreal.LinearColor(0.12, 0.12, 0.13, 1), 900)
damt = sparam("SuitDarkAmount", 0.0, 1100)
dglow = sparam("SuitDarkGlow", 0.0, 1300)
NAMES = ("BC", "Tint", "Accent", "Glow", "Dark", "DarkAmt", "DarkGlow")
masks = ("float lum = dot(BC.rgb, float3(0.299, 0.587, 0.114));\n"
         "float sat = max(BC.r, max(BC.g, BC.b)) - min(BC.r, min(BC.g, BC.b));\n"
         "float panel = saturate((lum - 0.42) / 0.16) * (1.0 - saturate(sat * 5.0));\n"
         "float accent = saturate((sat - 0.22) * 6.0);\n"
         "float darkm = (1.0 - saturate((lum - 0.06) / 0.16)) * saturate((lum - 0.015) / 0.03) * (1.0 - saturate(sat * 5.0)) * DarkAmt;\n")
code_col = (masks + "float3 c = lerp(BC.rgb, BC.rgb * Tint.rgb, panel);\n"
            "float3 hue = Accent.rgb * (0.35 + 1.2 * max(BC.r, max(BC.g, BC.b)));\n"
            "c = lerp(c, hue, accent);\n"
            "float3 deep = Dark.rgb * (0.55 + 3.0 * lum);\n"
            "return lerp(c, deep, darkm);")
code_emi = masks + "return Accent.rgb * accent * Glow + Dark.rgb * darkm * DarkGlow;"
customs.sort(key=lambda e: len(e.get_editor_property("code")))  # the emissive node has the shorter code
emi, col = customs[0], customs[1]
for c, code in ((col, code_col), (emi, code_emi)):
    c.set_editor_property("code", code)
    ins = []
    for n in NAMES:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        ins.append(ci)
    c.set_editor_property("inputs", ins)
    MEL.connect_material_expressions(tex, "RGB", c, "BC")
    MEL.connect_material_expressions(tint, "", c, "Tint")
    MEL.connect_material_expressions(acc, "", c, "Accent")
    MEL.connect_material_expressions(glow, "", c, "Glow")
    MEL.connect_material_expressions(dark, "", c, "Dark")
    MEL.connect_material_expressions(damt, "", c, "DarkAmt")
    MEL.connect_material_expressions(dglow, "", c, "DarkGlow")
MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(emi, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
m.modify()
EAL.save_asset(P + "M_Pilot", only_if_is_dirty=False)
log("M_Pilot: SuitDark / SuitDarkAmount / SuitDarkGlow added (inputs %s)" % (NAMES,))
SUITS = {
    "MI_Pilot_Wren": dict(tint=(1.0, 0.94, 0.9), accent=(1.0, 0.12, 0.75), dark=(0.55, 0.02, 0.36), dglow=0.4),
    "MI_Pilot_Kato": dict(tint=(0.86, 0.95, 1.0), accent=(0.05, 0.85, 1.0), dark=(0.0, 0.36, 0.48), dglow=0.4),
    "MI_Pilot_Moss": dict(tint=(1.0, 0.76, 0.48), accent=(1.0, 0.55, 0.05), dark=(0.62, 0.2, 0.02), dglow=0.4),
}
for name, s in SUITS.items():
    mi = unreal.load_asset(P + name)
    MEL.set_material_instance_vector_parameter_value(mi, "SuitTint", unreal.LinearColor(*s["tint"], 1))
    MEL.set_material_instance_vector_parameter_value(mi, "SuitAccent", unreal.LinearColor(*s["accent"], 1))
    MEL.set_material_instance_vector_parameter_value(mi, "SuitDark", unreal.LinearColor(*s["dark"], 1))
    MEL.set_material_instance_scalar_parameter_value(mi, "SuitDarkAmount", 1.0)
    MEL.set_material_instance_scalar_parameter_value(mi, "SuitDarkGlow", s["dglow"])
    MEL.update_material_instance(mi)
    EAL.save_asset(P + name, only_if_is_dirty=False)
    log("%s: %s" % (name, s))
log("OK")

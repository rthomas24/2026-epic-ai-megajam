# Builds /Game/TriadDrift/Gameplay/Materials/M_TD_PortalMembrane (engineering placeholder for the portal gate surface).
# Run headless: powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\PortalMembrane\make_portal_membrane.py
#
# Unlit, additive, two-sided: a bright rim fading to a see-through centre (you see the far side through the gate),
# a slow three-arm swirl times a radial ripple, tinted per gate (Tint, Brightness). One Custom node: cheap.
# Radius comes from LocalPosition on the engine Cylinder (radius 50 cm in local space, axis Z), which is what
# ATDPortalGate's Surface uses. Nick's PortalIdle VFX still wins (the gate hides this surface when it has art).
import unreal

PATH = "/Game/TriadDrift/Gameplay/Materials"
NAME = "M_TD_PortalMembrane"
MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()

full = f"{PATH}/{NAME}"
if unreal.EditorAssetLibrary.does_asset_exist(full):
    mat = unreal.EditorAssetLibrary.load_asset(full)
    MEL.delete_all_material_expressions(mat)
else:
    mat = tools.create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())

mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", True)

local = MEL.create_material_expression(mat, unreal.MaterialExpressionLocalPosition, -900, -200)
time = MEL.create_material_expression(mat, unreal.MaterialExpressionTime, -900, 0)
tint = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -900, 150)
tint.set_editor_property("parameter_name", "Tint")
tint.set_editor_property("default_value", unreal.LinearColor(1.0, 0.35, 1.0, 1.0))
bright = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -900, 350)
bright.set_editor_property("parameter_name", "Brightness")
bright.set_editor_property("default_value", 3.0)
speed = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -900, 450)
speed.set_editor_property("parameter_name", "SwirlSpeed")
speed.set_editor_property("default_value", 1.0)

custom = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -500, 0)
custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
custom.set_editor_property("description", "PortalMembrane")
code = (
    "float r = length(P.xy) / 50.0;\n"
    "float a = atan2(P.y, P.x);\n"
    "float t = T * S;\n"
    "float rim = pow(saturate(r), 5.0);\n"
    "float swirl = 0.5 + 0.5 * sin(a * 3.0 + r * 9.0 - t * 1.2);\n"
    "float ripple = 0.5 + 0.5 * sin(r * 22.0 - t * 2.5);\n"
    "float body = 0.05 + 0.22 * swirl * ripple * r;\n"
    "float edge = 1.0 - smoothstep(0.97, 1.02, r);\n"
    "return Tint.rgb * B * (rim * 1.8 + body) * edge;\n"
)
custom.set_editor_property("code", code)
inputs = []
for n in ["P", "T", "Tint", "B", "S"]:
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    inputs.append(ci)
custom.set_editor_property("inputs", inputs)

MEL.connect_material_expressions(local, "", custom, "P")
MEL.connect_material_expressions(time, "", custom, "T")
MEL.connect_material_expressions(tint, "", custom, "Tint")
MEL.connect_material_expressions(bright, "", custom, "B")
MEL.connect_material_expressions(speed, "", custom, "S")
MEL.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

try:
    MEL.recompile_material(mat)
except RuntimeError as e:  # headless (-nullrhi): the material compiles when the game loads it
    print("TDPY: no renderer, saving uncompiled: " + str(e)[:80])
unreal.EditorAssetLibrary.save_asset(full, only_if_is_dirty=False)
print("TDPY: built " + full)
print("TDPY: OK")

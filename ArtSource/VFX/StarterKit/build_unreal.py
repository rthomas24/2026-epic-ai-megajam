"""Build original, texture-free VFX materials. Run only after claim publication.

Run inside UE 5.9 Python; writes only /Game/TriadDrift/VFX/StarterKit.
Existing assets are refused, never overwritten. No maps or gameplay are touched.
"""
from pathlib import Path
import json
import unreal

ROOT = "/Game/TriadDrift/VFX/StarterKit"
HERE = Path(__file__).resolve().parent
ED = unreal.EditorAssetLibrary
ML = unreal.MaterialEditingLibrary
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
PALETTES = {
    "Rift": (1.0, 0.025, 0.48, 1.0),
    "Lancer": (0.025, 0.76, 1.0, 1.0),
    "Bastion": (1.0, 0.37, 0.035, 1.0),
}
EFFECTS = ("EngineTrail", "MuzzleFlash", "HullImpact", "ShieldImpact")
SCALARS = {
    "Intensity": 5.0, "Length": 1.0, "Width": 0.5, "Speed": 1.5,
    "Lifetime": 0.32, "StartTime": 0.0, "PreviewLoop": 1.0,
    "Softness": 1.0,
}


def create(name, cls, factory):
    path = ROOT + "/" + name
    if ED.does_asset_exist(path):
        raise RuntimeError("Refusing to overwrite " + path)
    obj = TOOLS.create_asset(name, ROOT, cls, factory)
    if not obj:
        raise RuntimeError("Asset creation failed: " + path)
    return obj


def node(material, cls, x, y):
    return ML.create_material_expression(material, cls, x, y)


def connect(source, target, pin, output=""):
    if not ML.connect_material_expressions(source, output, target, pin):
        raise RuntimeError("Material connection failed: " + pin)


def build_material(effect):
    material = create("M_TD_" + effect, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    custom = node(material, unreal.MaterialExpressionCustom, 200, 0)
    custom.set_editor_property("code", (HERE / "shaders" / (effect + ".ush")).read_text())
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
    names = ["UV", "Clock", "Tint"] + list(SCALARS)
    inputs = []
    for name in names:
        value = unreal.CustomInput()
        value.set_editor_property("input_name", name)
        inputs.append(value)
    custom.set_editor_property("inputs", inputs)
    uv = node(material, unreal.MaterialExpressionTextureCoordinate, -800, -400)
    connect(uv, custom, "UV")
    clock = node(material, unreal.MaterialExpressionTime, -800, -250)
    connect(clock, custom, "Clock")
    tint = node(material, unreal.MaterialExpressionVectorParameter, -800, -100)
    tint.set_editor_property("parameter_name", "Color")
    tint.set_editor_property("default_value", unreal.LinearColor(0.025, 0.76, 1.0, 1.0))
    connect(tint, custom, "Tint", "RGB")
    for index, (name, default) in enumerate(SCALARS.items()):
        value = node(material, unreal.MaterialExpressionScalarParameter, -500, 100 + index * 100)
        value.set_editor_property("parameter_name", name)
        value.set_editor_property("default_value", default)
        connect(value, custom, name)
    rgb = node(material, unreal.MaterialExpressionComponentMask, 450, -100)
    for channel in ("r", "g", "b"):
        rgb.set_editor_property(channel, True)
    rgb.set_editor_property("a", False)
    alpha = node(material, unreal.MaterialExpressionComponentMask, 450, 100)
    for channel in ("r", "g", "b"):
        alpha.set_editor_property(channel, False)
    alpha.set_editor_property("a", True)
    connect(custom, rgb, "Input")
    connect(custom, alpha, "Input")
    if not ML.connect_material_property(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError("Emissive connection failed")
    if not ML.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY):
        raise RuntimeError("Opacity connection failed")
    ML.layout_material_expressions(material)
    ML.recompile_material(material)
    ED.save_loaded_asset(material)
    return material


def main():
    # Preflight the whole batch to avoid partial overwrites.
    names = ["M_TD_" + effect for effect in EFFECTS]
    names += ["MI_TD_" + effect + "_" + ship for effect in EFFECTS for ship in PALETTES]
    collisions = [name for name in names if ED.does_asset_exist(ROOT + "/" + name)]
    if collisions:
        raise RuntimeError("Existing assets; no changes made: " + ", ".join(collisions))
    ED.make_directory(ROOT)
    assets = []
    for effect in EFFECTS:
        material = build_material(effect)
        assets.append(material.get_path_name())
        for ship, color in PALETTES.items():
            instance = create("MI_TD_" + effect + "_" + ship,
                              unreal.MaterialInstanceConstant,
                              unreal.MaterialInstanceConstantFactoryNew())
            ML.set_material_instance_parent(instance, material)
            ML.set_material_instance_vector_parameter_value(instance, "Color", unreal.LinearColor(*color))
            lifetime = 1.0 if effect == "EngineTrail" else (0.12 if effect == "MuzzleFlash" else 0.42)
            ML.set_material_instance_scalar_parameter_value(instance, "Lifetime", lifetime)
            ML.set_material_instance_scalar_parameter_value(instance, "Intensity", 3.5 if ship == "Bastion" else 5.0)
            ML.update_material_instance(instance)
            ED.save_loaded_asset(instance)
            assets.append(instance.get_path_name())
    unreal.log("TD_VFX_SUCCESS " + json.dumps({"assets": assets, "count": len(assets)}))


if __name__ == "__main__":
    main()

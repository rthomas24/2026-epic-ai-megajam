# Editor script (run through ue_exec.py): the two master materials every generated tree uses.
# A biome's look (colours, glow) is set per species on material INSTANCES of these, never by
# changing the tree's structure.
#
#   python ue_exec.py --port 8017 make_materials.py --args '{"folder": "/Game/TriadDrift/Gameplay/Vegetation/Materials"}'
#
# M_TDVeg_Foliage  two-sided foliage, opaque, no textures. Parameters: FoliageColor, TipColor,
#                  GlowColor, GlowStrength, GlowTipPower, VeinGlow, Roughness, InstanceTint.
#                  The leaf meshes carry UV0.x = 0 at the leaf base .. 1 at the tip, UV0.y = 0.5 on the midrib.
# M_TDVeg_Bark     default lit. Parameters: BarkColor, GlowColor, RimGlow, Roughness.
FOLDER = ARGS.get("folder", "/Game/TriadDrift/Gameplay/Vegetation/Materials")
mel = unreal.MaterialEditingLibrary


def new_material(name):
    path = "%s/%s" % (FOLDER, name)
    material = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if material is None:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    else:
        mel.delete_all_material_expressions(material)
    # Trees are Nanite static meshes placed as instances: without these flags a packaged game
    # falls back to the grey default material.
    for usage in ("used_with_nanite", "used_with_instanced_static_meshes", "used_with_static_lighting"):
        material.set_editor_property(usage, True)
    return material


def node(material, cls, x, y, **properties):
    expression = mel.create_material_expression(material, cls, x, y)
    for key, value in properties.items():
        expression.set_editor_property(key, value)
    return expression


def link(source, target, pin, source_pin=""):
    if not mel.connect_material_expressions(source, source_pin, target, pin):
        raise RuntimeError("could not connect %s -> %s.%s" % (source.get_name(), target.get_name(), pin))


def scalar(material, name, value, x, y):
    return node(material, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=value)


def vector(material, name, rgb, x, y):
    return node(material, unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name, default_value=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))


def binary(material, cls, a, b, x, y):
    expression = node(material, cls, x, y)
    link(a, expression, "A")
    link(b, expression, "B")
    return expression


def build_foliage():
    m = new_material("M_TDVeg_Foliage")
    m.set_editor_property("two_sided", True)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
    uv = node(m, unreal.MaterialExpressionTextureCoordinate, -1500, 0)
    along = node(m, unreal.MaterialExpressionComponentMask, -1300, -60, r=True, g=False, b=False, a=False)
    across = node(m, unreal.MaterialExpressionComponentMask, -1300, 80, r=False, g=True, b=False, a=False)
    link(uv, along, "")
    link(uv, across, "")

    foliage = vector(m, "FoliageColor", (0.02, 0.14, 0.13), -1100, -500)
    tip = vector(m, "TipColor", (0.08, 0.03, 0.2), -1100, -320)
    colour = node(m, unreal.MaterialExpressionLinearInterpolate, -800, -400)
    link(foliage, colour, "A")
    link(tip, colour, "B")
    link(along, colour, "Alpha")
    # Each instance of the tree is tinted a little differently, so a forest of one mesh does not look stamped.
    tint_amount = scalar(m, "InstanceTint", 0.25, -1100, -160)
    random_value = node(m, unreal.MaterialExpressionPerInstanceRandom, -1100, -80)
    half = node(m, unreal.MaterialExpressionConstant, -1100, -20, r=0.5)
    centred = binary(m, unreal.MaterialExpressionSubtract, random_value, half, -900, -80)
    swing = binary(m, unreal.MaterialExpressionMultiply, centred, tint_amount, -750, -80)
    one = node(m, unreal.MaterialExpressionConstant, -750, -20, r=1.0)
    factor = binary(m, unreal.MaterialExpressionAdd, one, swing, -600, -80)
    base = binary(m, unreal.MaterialExpressionMultiply, colour, factor, -420, -300)
    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    # Light passing through the leaf.
    subsurface = binary(m, unreal.MaterialExpressionMultiply, base, node(m, unreal.MaterialExpressionConstant, -420, -180, r=0.6), -250, -220)
    mel.connect_material_property(subsurface, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)

    # Glow: leaf tips, plus a thin line along the midrib.
    tip_power = scalar(m, "GlowTipPower", 5.0, -1100, 200)
    tip_mask = node(m, unreal.MaterialExpressionPower, -900, 140)
    link(along, tip_mask, "Base")
    link(tip_power, tip_mask, "Exp")
    # midrib = (1 - |2v - 1|) ^ 12
    two = node(m, unreal.MaterialExpressionConstant, -1300, 200, r=2.0)
    doubled = binary(m, unreal.MaterialExpressionMultiply, across, two, -1100, 320)
    shifted = binary(m, unreal.MaterialExpressionSubtract, doubled, node(m, unreal.MaterialExpressionConstant, -1100, 400, r=1.0), -950, 320)
    distance = node(m, unreal.MaterialExpressionAbs, -820, 320)
    link(shifted, distance, "")
    near = node(m, unreal.MaterialExpressionOneMinus, -700, 320)
    link(distance, near, "")
    midrib = node(m, unreal.MaterialExpressionPower, -580, 320)
    link(near, midrib, "Base")
    link(node(m, unreal.MaterialExpressionConstant, -700, 400, r=12.0), midrib, "Exp")
    vein = binary(m, unreal.MaterialExpressionMultiply, midrib, scalar(m, "VeinGlow", 0.35, -580, 420), -440, 340)
    vein_on_blade = binary(m, unreal.MaterialExpressionMultiply, vein, along, -300, 340)
    mask = binary(m, unreal.MaterialExpressionAdd, tip_mask, vein_on_blade, -160, 200)
    glow = vector(m, "GlowColor", (0.05, 0.9, 1.0), -500, 80)
    strength = scalar(m, "GlowStrength", 1.0, -500, 240)
    coloured = binary(m, unreal.MaterialExpressionMultiply, glow, strength, -300, 100)
    emissive = binary(m, unreal.MaterialExpressionMultiply, coloured, mask, -60, 120)
    mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(scalar(m, "Roughness", 0.55, -250, 480), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(node(m, unreal.MaterialExpressionConstant, -250, 560, r=0.25), "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m.get_path_name()


def build_bark():
    m = new_material("M_TDVeg_Bark")
    colour = vector(m, "BarkColor", (0.04, 0.03, 0.05), -700, -200)
    mel.connect_material_property(colour, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(scalar(m, "Roughness", 0.85, -700, 300), "", unreal.MaterialProperty.MP_ROUGHNESS)
    # A faint glowing rim, as if the bark carried luminous moss.
    fresnel = node(m, unreal.MaterialExpressionFresnel, -700, 60, exponent=3.0, base_reflect_fraction=0.0)
    glow = vector(m, "GlowColor", (0.05, 0.9, 1.0), -700, -20)
    rim = binary(m, unreal.MaterialExpressionMultiply, glow, scalar(m, "RimGlow", 0.05, -700, 200), -480, 60)
    emissive = binary(m, unreal.MaterialExpressionMultiply, rim, fresnel, -280, 80)
    mel.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m)
    return m.get_path_name()


if not unreal.EditorAssetLibrary.does_directory_exist(FOLDER):
    unreal.EditorAssetLibrary.make_directory(FOLDER)
[build_foliage(), build_bark()]

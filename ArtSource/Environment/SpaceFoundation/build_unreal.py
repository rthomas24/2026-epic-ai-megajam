"""Run in UE 5.9 Python after generating textures and publishing claims.

Creates assets only in /Game/TriadDrift/Environment/SpaceFoundation. Does not
spawn actors, change maps, touch project settings, or use gameplay interfaces.
Refuses existing assets rather than overwriting manual edits.
"""
from pathlib import Path
import json
import unreal as ue

SOURCE = Path(__file__).resolve().parent
DEST = '/Game/TriadDrift/Environment/SpaceFoundation'
LIB = ue.MaterialEditingLibrary
TOOLS = ue.AssetToolsHelpers.get_asset_tools()


def require_absent(name):
    path = DEST + '/' + name
    if ue.EditorAssetLibrary.does_asset_exist(path):
        raise RuntimeError('Refusing to overwrite existing asset: ' + path)


def node(material, cls, x, y, **props):
    expression = LIB.create_material_expression(material, cls, x, y)
    for key, value in props.items():
        expression.set_editor_property(key, value)
    return expression


def connect(a, a_pin, b, b_pin):
    if not LIB.connect_material_expressions(a, a_pin, b, b_pin):
        raise RuntimeError('Material connection failed: ' + b_pin)


def scalar(material, name, value, y):
    return node(material, ue.MaterialExpressionScalarParameter, -800, y,
                parameter_name=name, default_value=value, group='Space Foundation')


def main():
    presets = json.loads((SOURCE / 'presets.json').read_text(encoding='utf-8'))
    names = ['T_TD_Nebula_BC', 'T_TD_Stars_BC', 'M_TD_SpaceFoundation'] + [p['asset'] for p in presets['material_instances']]
    # Preflight the whole batch before changing the Content Browser.
    for name in names:
        require_absent(name)
    for texture in names[:2]:
        path = SOURCE / 'export' / (texture + '.png')
        if not path.is_file():
            raise RuntimeError('Generate source texture first: ' + str(path))
    ue.EditorAssetLibrary.make_directory(DEST)
    textures = []
    for name in names[:2]:
        task = ue.AssetImportTask()
        task.set_editor_property('filename', str(SOURCE / 'export' / (name + '.png')))
        task.set_editor_property('destination_path', DEST)
        task.set_editor_property('destination_name', name)
        task.set_editor_property('automated', True)
        task.set_editor_property('replace_existing', False)
        task.set_editor_property('save', True)
        TOOLS.import_asset_tasks([task])
        texture = ue.EditorAssetLibrary.load_asset(DEST + '/' + name)
        if not texture:
            raise RuntimeError('Texture import failed: ' + name)
        texture.set_editor_property('srgb', True)
        texture.set_editor_property('address_x', ue.TextureAddress.TA_WRAP)
        texture.set_editor_property('address_y', ue.TextureAddress.TA_CLAMP)
        texture.set_editor_property('lod_group', ue.TextureGroup.TEXTUREGROUP_SKYBOX)
        textures.append(texture)
    material = TOOLS.create_asset('M_TD_SpaceFoundation', DEST, ue.Material, ue.MaterialFactoryNew())
    material.set_editor_property('shading_model', ue.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property('two_sided', True)
    material.set_editor_property('is_sky', True)
    # CameraVectorWS points from surface to camera. Negating gives outward sky direction.
    direction = node(material, ue.MaterialExpressionCameraVectorWS, -1400, -200)
    yaw = scalar(material, 'YawTurns', 0.0, 100)
    projection = node(material, ue.MaterialExpressionCustom, -1050, -200,
        code='float3 d = normalize(-Direction); return float2(frac(atan2(d.y,d.x)*0.1591549431 + Yaw), acos(clamp(d.z,-1.0,1.0))*0.3183098862);',
        output_type=ue.CustomMaterialOutputType.CMOT_FLOAT2,
        inputs=[ue.CustomInput(input_name='Direction'), ue.CustomInput(input_name='Yaw')],
        description='Seam-safe spherical direction projection')
    connect(direction, '', projection, 'Direction')
    connect(yaw, '', projection, 'Yaw')
    nebula = node(material, ue.MaterialExpressionTextureSampleParameter2D, -600, -350,
                  parameter_name='NebulaTexture', texture=textures[0])
    stars = node(material, ue.MaterialExpressionTextureSampleParameter2D, -600, 200,
                 parameter_name='StarTexture', texture=textures[1])
    connect(projection, '', nebula, 'UVs')
    connect(projection, '', stars, 'UVs')
    nebula_tint = node(material, ue.MaterialExpressionVectorParameter, -600, -50,
        parameter_name='NebulaTint', default_value=ue.LinearColor(1, 1, 1, 1), group='Space Foundation')
    nebula_gain = scalar(material, 'NebulaIntensity', 1.0, -500)
    star_gain = scalar(material, 'StarIntensity', 1.8, 450)
    brightness = scalar(material, 'SkyIntensity', 1.0, 650)
    tint = node(material, ue.MaterialExpressionMultiply, -300, -200)
    connect(nebula, 'RGB', tint, 'A'); connect(nebula_tint, '', tint, 'B')
    cloud_scale = node(material, ue.MaterialExpressionMultiply, -100, -200)
    connect(tint, '', cloud_scale, 'A'); connect(nebula_gain, '', cloud_scale, 'B')
    star_scale = node(material, ue.MaterialExpressionMultiply, -100, 200)
    connect(stars, 'RGB', star_scale, 'A'); connect(star_gain, '', star_scale, 'B')
    combined = node(material, ue.MaterialExpressionAdd, 150, 0)
    connect(cloud_scale, '', combined, 'A'); connect(star_scale, '', combined, 'B')
    gain = node(material, ue.MaterialExpressionMultiply, 350, 0)
    connect(combined, '', gain, 'A'); connect(brightness, '', gain, 'B')
    if not LIB.connect_material_property(gain, '', ue.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError('Emissive connection failed')
    LIB.layout_material_expressions(material)
    LIB.recompile_material(material)
    created = [material] + textures
    for preset in presets['material_instances']:
        instance = TOOLS.create_asset(preset['asset'], DEST, ue.MaterialInstanceConstant, ue.MaterialInstanceConstantFactoryNew())
        LIB.set_material_instance_parent(instance, material)
        for name, value in preset['scalars'].items():
            if not LIB.set_material_instance_scalar_parameter_value(instance, name, value):
                raise RuntimeError('Missing scalar parameter: ' + name)
        tint = preset['nebula_tint_linear']
        LIB.set_material_instance_vector_parameter_value(instance, 'NebulaTint', ue.LinearColor(*tint, 1))
        created.append(instance)
    for asset in created:
        if not ue.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
            raise RuntimeError('Asset save failed: ' + asset.get_path_name())
    ue.log('SpaceFoundation created: ' + ', '.join(a.get_name() for a in created))


if __name__ == '__main__':
    main()

"""Create four owned hull-impact materials only; fresh Lead native grant required.

No Niagara lifecycle duplication, data-asset binding, map or existing-asset saves.
"""
import hashlib
import json
from pathlib import Path
import time
import traceback
import unreal as ue

HERE = Path(__file__).resolve().parent
REPO = next(p for p in HERE.parents if (p / 'unrealGameJam.uproject').is_file())
DEST = '/Game/TriadDrift/VFX/CombatReadability01/HullImpact01'
CONFIG = json.loads((HERE / 'impact.json').read_text())
ED = ue.EditorAssetLibrary
ML = ue.MaterialEditingLibrary
TOOLS = ue.AssetToolsHelpers.get_asset_tools()


def node(material, cls, **properties):
    value = ML.create_material_expression(material, cls)
    assert value
    for key, default in properties.items():
        value.set_editor_property(key, default)
    return value


def main(out):
    assert ue.SystemLibrary.get_engine_version().startswith('5.9.0-60351404')
    assert CONFIG['destination'] == DEST and set(CONFIG['palettes']) == {'Rift', 'Lancer', 'Bastion'}
    assert set(CONFIG['variation_by_palette']) == set(CONFIG['palettes'])
    assert 0 < CONFIG['intensity'] <= 2 and all(0 <= x < 1 for x in CONFIG['variation_by_palette'].values())
    expected = REPO / 'Content/TriadDrift/VFX/CombatReadability01/HullImpact01'
    actual = Path(ue.Paths.project_content_dir()) / 'TriadDrift/VFX/CombatReadability01/HullImpact01'
    assert actual.resolve() == expected.resolve(), 'Lead must mount only the owned VFX root'
    names = ['M_HullImpact'] + ['MI_HullImpact_' + p for p in CONFIG['palettes']]
    assert not any(ED.does_asset_exist(DEST + '/' + n) for n in names), 'Existing output: preserve and stop'
    ED.make_directory(DEST)
    material = TOOLS.create_asset(names[0], DEST, ue.Material, ue.MaterialFactoryNew())
    assert material
    for key, value in {'blend_mode': ue.BlendMode.BLEND_ADDITIVE,
                       'shading_model': ue.MaterialShadingModel.MSM_UNLIT, 'two_sided': True}.items():
        material.set_editor_property(key, value)
    ML.set_material_usage(material, ue.MaterialUsage.MATUSAGE_NIAGARA_SPRITES)
    inputs = []
    for name in ('UV', 'Age', 'Tint', 'Intensity', 'Variation'):
        item = ue.CustomInput()
        item.set_editor_property('input_name', name)
        inputs.append(item)
    custom = node(material, ue.MaterialExpressionCustom, inputs=inputs,
        code=(HERE / 'hull_impact.ush').read_text(), output_type=ue.CustomMaterialOutputType.CMOT_FLOAT4)
    sources = [(node(material, ue.MaterialExpressionTextureCoordinate), '', 'UV'),
               (node(material, ue.MaterialExpressionParticleRelativeTime), '', 'Age'),
               (node(material, ue.MaterialExpressionVectorParameter, parameter_name='Color',
                     default_value=ue.LinearColor(*CONFIG['palettes']['Lancer'])), 'RGB', 'Tint'),
               (node(material, ue.MaterialExpressionScalarParameter, parameter_name='Intensity',
                     default_value=CONFIG['intensity']), '', 'Intensity'),
               (node(material, ue.MaterialExpressionScalarParameter, parameter_name='Variation',
                     default_value=CONFIG['variation_by_palette']['Lancer']), '', 'Variation')]
    for source, output, target in sources:
        assert ML.connect_material_expressions(source, output, custom, target)
    for channels, target in ((('r', 'g', 'b'), ue.MaterialProperty.MP_EMISSIVE_COLOR),
                             (('a',), ue.MaterialProperty.MP_OPACITY)):
        mask = node(material, ue.MaterialExpressionComponentMask,
                    **{channel: channel in channels for channel in ('r', 'g', 'b', 'a')})
        assert ML.connect_material_expressions(custom, '', mask, 'Input')
        assert ML.connect_material_property(mask, '', target)
    ML.layout_material_expressions(material)
    errors = ML.recompile_material(material)
    assert not errors, errors
    assets = [material]
    for label, color in CONFIG['palettes'].items():
        instance = TOOLS.create_asset('MI_HullImpact_' + label, DEST,
            ue.MaterialInstanceConstant, ue.MaterialInstanceConstantFactoryNew())
        assert instance
        ML.set_material_instance_parent(instance, material)
        ML.set_material_instance_vector_parameter_value(instance, 'Color', ue.LinearColor(*color))
        ML.set_material_instance_scalar_parameter_value(instance, 'Variation', CONFIG['variation_by_palette'][label])
        ML.update_material_instance(instance)
        assets.append(instance)
    statistics = {}
    for asset in assets:
        assert asset.get_path_name().startswith(DEST + '/')
        stats = ML.get_statistics(asset)
        statistics[asset.get_path_name()] = {key: stats.get_editor_property(key) for key in (
            'num_pixel_shader_instructions', 'num_vertex_shader_instructions', 'num_samplers', 'num_pixel_texture_samples')}
        assert statistics[asset.get_path_name()]['num_pixel_shader_instructions'] > 0
    for asset in assets:
        assert ED.save_loaded_asset(asset, only_if_is_dirty=False)
    (out / 'result.json').write_text(json.dumps({'stage': 'materials_authored_only_pending_fresh_reload_Niagara_and_gameplay',
        'assets': [a.get_path_name() for a in assets], 'statistics': statistics,
        'niagara_created': False, 'data_asset_edited': False, 'gameplay_validated': False}, indent=2) + '\n')


if __name__ == '__main__':
    out = HERE / 'Saved' / ('materials_' + str(time.time_ns()))
    out.mkdir(parents=True)
    (out / 'inputs.json').write_text(json.dumps({
        'engine': ue.SystemLibrary.get_engine_version(),
        'sha256': {n: hashlib.sha256((HERE / n).read_bytes()).hexdigest()
                   for n in ('build_materials.py', 'hull_impact.ush', 'impact.json')}}, indent=2) + '\n')
    try:
        main(out)
    except Exception:
        (out / 'failure.json').write_text(json.dumps({'traceback': traceback.format_exc(),
            'successful_native_save_not_assumed': True}, indent=2) + '\n')
        raise

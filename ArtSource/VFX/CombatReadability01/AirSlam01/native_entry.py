"""Bounded native stages: probe templates, author two materials, fresh reload.

No gameplay maps, presentation data assets, shared settings or engine saves.
Does not pretend a material-only stage is a completed Niagara system.
"""
import json
import os
from pathlib import Path
import sys
import traceback

sys.dont_write_bytecode = True
import unreal as ue

HERE = Path(__file__).resolve().parent
OUT = Path(os.environ['TD_AIR_SLAM_REPORT']).resolve()
STAGE = os.environ['TD_AIR_SLAM_STAGE']
DEST = '/Game/TriadDrift/VFX/CombatReadability01/AirSlam01'
NAMES = ('M_AirSlam_Ring', 'M_AirSlam_Dust')
ED, ML = ue.EditorAssetLibrary, ue.MaterialEditingLibrary


def write(name, value):
    (OUT / name).write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


def tool(name, **arguments):
    result = ue.ToolsetRegistry.execute_tool('NiagaraToolsets.NiagaraToolset_System', name, json.dumps(arguments))
    assert result.is_complete, name + ': async result requires a later editor-ticked stage'
    assert not result.error, str(result.error)
    value = json.loads(result.value)
    assert 'returnValue' in value
    return value['returnValue']


def probe():
    template = '/Niagara/DefaultAssets/Templates/Systems/RadialBurst'
    obj = ED.load_asset(template)
    assert isinstance(obj, ue.NiagaraSystem), 'Installed template missing'
    summary = tool('GetSystemSummary', system={'refPath': template})
    write('template_summary.json', summary)
    assert summary['emitters'], 'No template emitters'
    topologies = {}
    for item in summary['emitters']:
        name = item['emitterName']
        target = {'system': {'refPath': template}, 'emitterName': name}
        topology = tool('GetEmitterTopology', emitterRef=target)
        renderers = []
        for renderer in topology['renderers']:
            renderer_ref = dict(target, rendererIndex=renderer['rendererIndex'])
            renderers.append({'reference': renderer_ref,
                'data': tool('GetRendererData', rendererRef=renderer_ref),
                'schema': tool('GetRendererSchema', rendererClass=renderer['rendererClass'])})
        topologies[name] = {'topology': topology, 'data': tool('GetEmitterData', emitterRef=target),
            'input_values': tool('GetEmitterInputValues', emitterRef=target), 'renderers': renderers}
    write('template_topologies.json', topologies)
    return {'template': template, 'emitters': [e['emitterName'] for e in summary['emitters']], 'engine_template_saved': False}


def node(material, cls, **properties):
    result = ML.create_material_expression(material, cls)
    assert result
    for field, value in properties.items():
        result.set_editor_property(field, value)
    return result


def statistics(material):
    stats = ML.get_statistics(material)
    values = {key: stats.get_editor_property(key) for key in (
        'num_pixel_shader_instructions', 'num_vertex_shader_instructions', 'num_samplers', 'num_pixel_texture_samples')}
    assert values['num_pixel_shader_instructions'] > 0, 'Material not compiled'
    return values


def create_material(name, ring, compile_shader):
    obj = ue.AssetToolsHelpers.get_asset_tools().create_asset(name, DEST, ue.Material, ue.MaterialFactoryNew())
    assert obj
    obj.set_editor_property('blend_mode', ue.BlendMode.BLEND_ADDITIVE if ring else ue.BlendMode.BLEND_TRANSLUCENT)
    obj.set_editor_property('shading_model', ue.MaterialShadingModel.MSM_UNLIT if ring else ue.MaterialShadingModel.MSM_DEFAULT_LIT)
    obj.set_editor_property('two_sided', True)
    ML.set_material_usage(obj, ue.MaterialUsage.MATUSAGE_NIAGARA_SPRITES)
    names = ('UV', 'Age', 'Tint', 'Intensity') if ring else ('UV', 'Age', 'Variation', 'Tint')
    inputs = []
    for field in names:
        item = ue.CustomInput()
        item.set_editor_property('input_name', field)
        inputs.append(item)
    expression = node(obj, ue.MaterialExpressionCustom, inputs=inputs,
        code=(HERE / ('air_slam_ring.ush' if ring else 'air_slam_dust.ush')).read_text(),
        output_type=ue.CustomMaterialOutputType.CMOT_FLOAT4)
    color = [0.04, 0.65, 0.9, 1] if ring else [1.0, 0.85, 0.65, 1]
    sources = {'UV': (node(obj, ue.MaterialExpressionTextureCoordinate), ''),
        'Age': (node(obj, ue.MaterialExpressionParticleRelativeTime), ''),
        'Tint': (node(obj, ue.MaterialExpressionVectorParameter, parameter_name='Color', default_value=ue.LinearColor(*color)), 'RGB'),
        'Intensity': (node(obj, ue.MaterialExpressionScalarParameter, parameter_name='Intensity', default_value=1.0), ''),
        'Variation': (node(obj, ue.MaterialExpressionScalarParameter, parameter_name='Variation', default_value=.47), '')}
    for field in names:
        source, output = sources[field]
        assert ML.connect_material_expressions(source, output, expression, field)
    rgb = node(obj, ue.MaterialExpressionComponentMask, r=True, g=True, b=True, a=False)
    alpha = node(obj, ue.MaterialExpressionComponentMask, r=False, g=False, b=False, a=True)
    assert ML.connect_material_expressions(expression, '', rgb, 'Input')
    assert ML.connect_material_expressions(expression, '', alpha, 'Input')
    assert ML.connect_material_property(rgb, '', ue.MaterialProperty.MP_EMISSIVE_COLOR if ring else ue.MaterialProperty.MP_BASE_COLOR)
    assert ML.connect_material_property(alpha, '', ue.MaterialProperty.MP_OPACITY)
    if not ring:
        rough = node(obj, ue.MaterialExpressionConstant, r=1.0)
        assert ML.connect_material_property(rough, '', ue.MaterialProperty.MP_ROUGHNESS)
    ML.layout_material_expressions(obj)
    if compile_shader:
        errors = ML.recompile_material(obj)
        assert not errors, errors
    return obj


def graph_check(material, ring):
    assert material.get_editor_property('blend_mode') == (ue.BlendMode.BLEND_ADDITIVE if ring else ue.BlendMode.BLEND_TRANSLUCENT)
    assert material.get_editor_property('shading_model') == (ue.MaterialShadingModel.MSM_UNLIT if ring else ue.MaterialShadingModel.MSM_DEFAULT_LIT)
    assert ML.get_num_material_expressions(material) >= 8
    color_property = ue.MaterialProperty.MP_EMISSIVE_COLOR if ring else ue.MaterialProperty.MP_BASE_COLOR
    color_node = ML.get_material_property_input_node(material, color_property)
    opacity_node = ML.get_material_property_input_node(material, ue.MaterialProperty.MP_OPACITY)
    assert isinstance(color_node, ue.MaterialExpressionComponentMask)
    assert isinstance(opacity_node, ue.MaterialExpressionComponentMask)
    color_inputs = ML.get_inputs_for_material_expression(material, color_node)
    opacity_inputs = ML.get_inputs_for_material_expression(material, opacity_node)
    assert len(color_inputs) == 1 and len(opacity_inputs) == 1
    custom = color_inputs[0]
    assert isinstance(custom, ue.MaterialExpressionCustom) and custom == opacity_inputs[0]
    assert custom.get_editor_property('code') == (HERE / ('air_slam_ring.ush' if ring else 'air_slam_dust.ush')).read_text()
    assert len(ML.get_inputs_for_material_expression(material, custom)) == 4
    return {'graph_expressions': ML.get_num_material_expressions(material), 'shader_compile_attempted': False,
            'color_and_opacity_connected_to_exact_source_kernel': True}


def main():
    assert OUT.is_relative_to(HERE / 'Saved/NativeRuns') and not OUT.exists()
    OUT.mkdir(parents=True, exist_ok=False)
    engine = ue.SystemLibrary.get_engine_version()
    assert engine.startswith('5.9.0-60351404'), engine
    assert Path(ue.Paths.project_content_dir()).resolve() == (HERE / 'Saved/H00/Content').resolve()
    actual_output = Path(ue.Paths.project_content_dir()) / 'TriadDrift/VFX/CombatReadability01/AirSlam01'
    expected = HERE.parents[3] / 'Content/TriadDrift/VFX/CombatReadability01/AirSlam01'
    assert actual_output.resolve() == expected.resolve()
    report = {'stage': STAGE, 'engine': engine, 'niagara_system_authored': False,
              'gameplay_bound': False, 'shared_paths_edited': False, 'assets': [], 'statistics': {},
              'render_validated': False, 'structural_checks': {}}
    if STAGE in ('probe', 'materials', 'structure'):
        report['probe'] = probe()
    if STAGE in ('materials', 'structure'):
        assert not any(ED.does_asset_exist(DEST + '/' + name) for name in NAMES), 'Preserve existing outputs'
        ED.make_directory(DEST)
        assets = [create_material(name, i == 0, STAGE == 'materials') for i, name in enumerate(NAMES)]
        for i, obj in enumerate(assets):
            if STAGE == 'materials':
                report['statistics'][obj.get_path_name()] = statistics(obj)
            else:
                report['structural_checks'][obj.get_path_name()] = graph_check(obj, i == 0)
        for obj in assets:
            assert obj.get_path_name().startswith(DEST + '/')
            assert ED.save_loaded_asset(obj, only_if_is_dirty=False)
            report['assets'].append(obj.get_path_name())
    elif STAGE in ('verify-materials', 'verify-structure'):
        assets = [ED.load_asset(DEST + '/' + name) for name in NAMES]
        assert all(assets) and all(isinstance(obj, ue.Material) for obj in assets)
        for i, obj in enumerate(assets):
            report['assets'].append(obj.get_path_name())
            if STAGE == 'verify-materials':
                report['statistics'][obj.get_path_name()] = statistics(obj)
            else:
                report['structural_checks'][obj.get_path_name()] = graph_check(obj, i == 0)
    write('result.json', report)


if __name__ == '__main__':
    try:
        main()
    except Exception:
        if OUT.exists():
            write('failure.json', {'traceback': traceback.format_exc(), 'native_success': False})
        raise

"""One owned Niagara muzzle family. Run inside UE only after Lead's native grant.

Uses installed NiagaraToolset operations, not direct engine-template mutation.
Unknown template topology/input names stop before saving; no automatic retry.
"""
import hashlib
import json
from pathlib import Path
import re
import sys
import time
import traceback

sys.dont_write_bytecode = True
import unreal as ue

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[2]
CONFIG = json.loads((HERE / 'burst.json').read_text())
DEST = CONFIG['destination']
ED = ue.EditorAssetLibrary
ML = ue.MaterialEditingLibrary
TOOLS = ue.AssetToolsHelpers.get_asset_tools()
NIAGARA = 'NiagaraToolsets.NiagaraToolset_System'
EVENTS = []
RUN_DIR = None


def record(event):
    EVENTS.append(event)
    if RUN_DIR is not None:
        with (RUN_DIR / 'operations.jsonl').open('a', encoding='utf-8') as output:
            output.write(json.dumps(event) + '\n')


def ref(path):
    return {'refPath': path}


def tool(name, **arguments):
    record({'phase': 'begin', 'tool': name, 'arguments': arguments})
    result = ue.ToolsetRegistry.execute_tool(NIAGARA, name, json.dumps(arguments))
    record({'phase': 'returned', 'tool': name, 'is_complete': result.is_complete,
            'error': str(result.error)})
    assert result.is_complete, name + ' requires an editor tick; stop and preserve diagnostics'
    assert not result.error, str(result.error)
    result = json.loads(result.value)
    assert 'returnValue' in result or name.startswith(('Set', 'Remove'))
    record({'phase': 'complete', 'tool': name, 'arguments': arguments, 'result': result})
    return result.get('returnValue')


def stack(system, emitter, script='', module='', inputs=None, renderer=-1):
    return {'system': ref(system), 'emitterName': emitter, 'scriptName': script,
            'moduleName': module, 'inputNameStack': inputs or [], 'rendererIndex': renderer}


def save_owned(obj):
    assert obj.get_path_name().startswith(DEST + '/')
    assert ED.save_loaded_asset(obj, only_if_is_dirty=False)


def shader_material():
    material = TOOLS.create_asset('M_MuzzleBurst', DEST, ue.Material, ue.MaterialFactoryNew())
    assert material
    for key, value in {'blend_mode': ue.BlendMode.BLEND_ADDITIVE,
                       'shading_model': ue.MaterialShadingModel.MSM_UNLIT,
                       'two_sided': True}.items():
        material.set_editor_property(key, value)
    ML.set_material_usage(material, ue.MaterialUsage.MATUSAGE_NIAGARA_SPRITES)
    custom = ML.create_material_expression(material, ue.MaterialExpressionCustom)
    inputs = []
    for name in ('UV', 'Age', 'Tint', 'Intensity', 'Variation'):
        item = ue.CustomInput()
        item.set_editor_property('input_name', name)
        inputs.append(item)
    custom.set_editor_property('inputs', inputs)
    custom.set_editor_property('code', (HERE / 'muzzle_burst.ush').read_text())
    custom.set_editor_property('output_type', ue.CustomMaterialOutputType.CMOT_FLOAT4)
    uv = ML.create_material_expression(material, ue.MaterialExpressionTextureCoordinate)
    age = ML.create_material_expression(material, ue.MaterialExpressionParticleRelativeTime)
    tint = ML.create_material_expression(material, ue.MaterialExpressionVectorParameter)
    tint.set_editor_property('parameter_name', 'Color')
    tint.set_editor_property('default_value', ue.LinearColor(*CONFIG['palettes']['Lancer']))
    intensity = ML.create_material_expression(material, ue.MaterialExpressionScalarParameter)
    intensity.set_editor_property('parameter_name', 'Intensity')
    intensity.set_editor_property('default_value', CONFIG['intensity'])
    variation = ML.create_material_expression(material, ue.MaterialExpressionScalarParameter)
    variation.set_editor_property('parameter_name', 'Variation')
    variation.set_editor_property('default_value', 0.47)
    for source, pin, output in ((uv, 'UV', ''), (age, 'Age', ''),
                                (tint, 'Tint', 'RGB'), (intensity, 'Intensity', ''),
                                (variation, 'Variation', '')):
        assert ML.connect_material_expressions(source, output, custom, pin)
    for channels, prop in ((('r', 'g', 'b'), ue.MaterialProperty.MP_EMISSIVE_COLOR),
                           (('a',), ue.MaterialProperty.MP_OPACITY)):
        mask = ML.create_material_expression(material, ue.MaterialExpressionComponentMask)
        for channel in ('r', 'g', 'b', 'a'):
            mask.set_editor_property(channel, channel in channels)
        assert ML.connect_material_expressions(custom, '', mask, 'Input')
        assert ML.connect_material_property(mask, '', prop)
    ML.layout_material_expressions(material)
    errors = ML.recompile_material(material)
    assert not errors, errors
    assert ML.get_statistics(material).get_editor_property('num_pixel_shader_instructions') > 0
    return material


def literal_input(target, type_path, value):
    return tool('SetStackInputData', stackInputRef=target,
                inputData={'struct': ref(type_path), 'value': {'value': value}})


def simple_name(value):
    return re.sub('[^a-z0-9]', '', value.lower())


def build_system(label, material_path):
    name = 'NS_MuzzleBurst_' + label
    system = DEST + '/' + name
    created = tool('CreateNiagaraSystem', assetName=name, assetPath=DEST,
                   templateSystem=ref(CONFIG['template_system']))
    assert created and created['refPath'].startswith(system + '.')
    for emitter in tool('GetSystemSummary', system=ref(system))['emitters']:
        tool('RemoveEmitter', emitterRef=stack(system, emitter['emitterName']))
    topology = tool('AddEmitter', system=ref(system), templateEmitter=ref(CONFIG['template_emitter']),
                    emitterName='CompactMuzzle')
    emitter = topology['emitterName']
    emitter_ref = stack(system, emitter)
    data = json.loads(tool('GetEmitterData', emitterRef=emitter_ref)['propertyValues'])
    data.update({'SimTarget': 'CPUSim', 'bLocalSpace': True})
    tool('SetEmitterData', emitterRef=emitter_ref, emitterData={'propertyValues': json.dumps(data)})
    topology = tool('GetEmitterTopology', emitterRef=emitter_ref)
    assert len(topology['renderers']) == 1
    renderer = topology['renderers'][0]
    assert 'NiagaraSpriteRendererProperties' in renderer['rendererClass']['refPath']
    renderer_ref = stack(system, emitter, renderer=renderer['rendererIndex'])
    # Existing schema/property serializer handles refs inside the JSON blob.
    renderer_data = {'Material': ref(material_path)}
    tool('SetRendererData', rendererRef=renderer_ref,
         rendererData={'propertyValues': json.dumps(renderer_data)})
    changed_count = changed_size = once_verified = False
    for script_key in ('emitterSpawnScript', 'emitterUpdateScript', 'particleSpawnScript', 'particleUpdateScript'):
        script = topology[script_key]
        for module in script['modules']:
            module_ref = stack(system, emitter, script['scriptName'], module['moduleName'])
            module_asset = module.get('moduleScript') or {}
            module_path = module_asset.get('refPath', '')
            if script_key == 'particleSpawnScript' and 'InitializeParticle' not in module_path:
                tool('RemoveModule', moduleRef=module_ref)
                continue
            if script_key == 'particleUpdateScript' and not any(
                    token in module_path for token in ('ParticleState', 'SolveForcesAndVelocity')):
                tool('RemoveModule', moduleRef=module_ref)
                continue
            for item in module['inputs']:
                if not item.get('bIsEditable', False):
                    continue
                names = [item['name']]
                key = simple_name(names[0])
                target = stack(system, emitter, script['scriptName'], module['moduleName'], names)
                if 'EmitterState' in module_path and key == 'loopbehavior':
                    current = tool('GetStackInputData', stackInputRef=target)
                    enum_name = current.get('value', {}).get('enumName', '')
                    display_name = current.get('value', {}).get('displayName', '')
                    record({'phase': 'verify_one_shot', 'system': system,
                            'enum_name': enum_name, 'display_name': display_name})
                    native_token = enum_name.split('::')[-1] if isinstance(enum_name, str) else None
                    assert native_token == 'Once' or display_name == 'Once', 'Template must be one-shot; inspect before changing lifecycle'
                    once_verified = True
                if 'SpawnBurst' in module_path and key in ('spawncount', 'count'):
                    literal_input(target, '/Script/Niagara.NiagaraInt32', CONFIG['spawn_count'])
                    changed_count = True
                if 'InitializeParticle' in module_path and key in ('uniformspritesize', 'spritesize'):
                    # Only a scalar uniform-size input is accepted; vector modes stop.
                    schema = tool('GetStackInputSchema', stackInputRef=target)
                    assert 'NiagaraFloat' in json.dumps(schema), 'Unexpected non-scalar sprite size'
                    literal_input(target, '/Script/Niagara.NiagaraFloat', CONFIG['sprite_diameter_cm'])
                    changed_size = True
    assert changed_count and changed_size and once_verified, 'Required count, size or one-shot lifecycle not verified'
    entry = {'variable': {'name': 'Particles.Lifetime', 'type': {
                'classStructOrEnum': ref('/Script/Niagara.NiagaraFloat')}},
             'defaultValue': {'struct': ref('/Script/Niagara.NiagaraFloat'),
                              'value': {'value': CONFIG['lifetime_seconds']}}}
    tool('AddSetParametersModule', moduleLocationRef=stack(system, emitter, 'ParticleSpawnScript'),
         parameters=[entry])
    return ED.load_asset(system)


def main():
    assert ue.SystemLibrary.get_engine_version().startswith('5.9.0-60351404')
    assert CONFIG['spawn_count'] == 1 and 0 < CONFIG['lifetime_seconds'] <= 0.06
    assert 0 < CONFIG['sprite_diameter_cm'] <= 36 and 0 < CONFIG['intensity'] <= 2.5
    assert set(CONFIG['palettes']) == set(CONFIG['variation_by_palette']) == {'Rift', 'Lancer', 'Bastion'}
    assert all(0 <= value < 1 for value in CONFIG['variation_by_palette'].values())
    actual = (Path(ue.Paths.project_content_dir()) / 'TriadDrift/VFX/CombatReadability01').resolve()
    expected = (REPO / 'Content/TriadDrift/VFX/CombatReadability01').resolve()
    assert actual == expected, 'Lead must mount only the owned VFX output folder before authoring'
    names = ['M_MuzzleBurst'] + ['MI_MuzzleBurst_' + p for p in CONFIG['palettes']] + [
        'NS_MuzzleBurst_' + p for p in CONFIG['palettes']]
    assert not any(ED.does_asset_exist(DEST + '/' + name) for name in names), 'Existing output; never overwrite'
    assert ED.load_asset(CONFIG['template_system']) and ED.load_asset(CONFIG['template_emitter'])
    ED.make_directory(DEST)
    material = shader_material()
    assets = [material]
    for label, color in CONFIG['palettes'].items():
        instance = TOOLS.create_asset('MI_MuzzleBurst_' + label, DEST, ue.MaterialInstanceConstant,
                                      ue.MaterialInstanceConstantFactoryNew())
        assert instance
        ML.set_material_instance_parent(instance, material)
        ML.set_material_instance_vector_parameter_value(instance, 'Color', ue.LinearColor(*color))
        ML.set_material_instance_scalar_parameter_value(instance, 'Variation', CONFIG['variation_by_palette'][label])
        ML.update_material_instance(instance)
        assets.append(instance)
        assets.append(build_system(label, instance.get_path_name()))
    # Save only after all required topology/input edits have passed.
    for asset in assets:
        assert asset
        save_owned(asset)
    report = {'stage': 'native_authored_pending_reload_compile_diagnostics_and_gameplay',
              'engine': ue.SystemLibrary.get_engine_version(), 'config': CONFIG,
              'source_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'assets': [obj.get_path_name() for obj in assets], 'operations': EVENTS,
              'presentation_data_asset_edited': False, 'gameplay_validated': False}
    output = RUN_DIR / 'result.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    ue.log('COMPACT_MUZZLE_AUTHORED ' + str(output))


if __name__ == '__main__':
    RUN_DIR = HERE / 'Saved' / ('author_' + str(time.time_ns()))
    RUN_DIR.mkdir(parents=True, exist_ok=False)
    (RUN_DIR / 'inputs.json').write_text(json.dumps({
        'engine': ue.SystemLibrary.get_engine_version(),
        'source_sha256': {name: hashlib.sha256((HERE / name).read_bytes()).hexdigest()
                          for name in ('build_muzzle.py', 'burst.json', 'muzzle_burst.ush')},
        'config': CONFIG}, indent=2) + '\n')
    try:
        main()
    except Exception:
        (RUN_DIR / 'failure.json').write_text(json.dumps({
            'stage': 'failed_native_authoring', 'traceback': traceback.format_exc(),
            'successful_native_save_not_assumed': True, 'operations': EVENTS}, indent=2) + '\n')
        raise

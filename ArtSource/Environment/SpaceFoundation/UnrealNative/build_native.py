"""UE 5.9 Python: reuse frozen builder, add owned drop-in environment rigs.

Run in the isolated content-only host, never in another agent's editor.
Existing native names are refused. Re-running is not a repair operation.
"""
import importlib.util
import json
from pathlib import Path
import unreal as ue

HERE = Path(__file__).resolve().parent
DEST = '/Game/TriadDrift/Environment/SpaceFoundation'
ED = ue.EditorAssetLibrary
ML = ue.MaterialEditingLibrary
TOOLS = ue.AssetToolsHelpers.get_asset_tools()


def custom_input(input_name):
    value = ue.CustomInput()
    value.set_editor_property('input_name', input_name)
    return value


def frozen_builder():
    path = HERE.parent / 'build_unreal.py'
    spec = importlib.util.spec_from_file_location('td_frozen_builder', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    class CompatibilityAPI:
        # UE 5.9's generated CustomInput binding rejects constructor keywords.
        CustomInput = staticmethod(custom_input)

        def __getattr__(self, name):
            return getattr(ue, name)

    module.ue = CompatibilityAPI()
    module.SOURCE = HERE
    module.DEST = DEST
    return module


def checked_save(asset):
    if not ED.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('Could not save ' + asset.get_path_name())


def apply_native_tuning():
    tuning = json.loads((HERE / 'native_tuning.json').read_text())
    for label, parameters in tuning.items():
        instance = ED.load_asset(DEST + '/MI_TD_Space_' + label)
        changed = False
        for name, value in parameters.items():
            if abs(ML.get_material_instance_scalar_parameter_value(instance, name) - value) < 1e-6:
                continue
            if not ML.set_material_instance_scalar_parameter_value(instance, name, value):
                raise RuntimeError('Missing tuning parameter: ' + name)
            changed = True
        if changed:
            ML.update_material_instance(instance)
            checked_save(instance)
    return tuning


def post_process(bloom):
    settings = ue.PostProcessSettings()
    for name, value in {
        'override_auto_exposure_method': True,
        'auto_exposure_method': ue.AutoExposureMethod.AEM_MANUAL,
        'override_auto_exposure_apply_physical_camera_exposure': True,
        'auto_exposure_apply_physical_camera_exposure': False,
        'override_auto_exposure_bias': True, 'auto_exposure_bias': 0.0,
        'override_bloom_intensity': True, 'bloom_intensity': bloom,
        'override_vignette_intensity': True, 'vignette_intensity': 0.0,
        'override_motion_blur_amount': True, 'motion_blur_amount': 0.0,
    }.items():
        settings.set_editor_property(name, value)
    return settings


def blueprint_rig(label, instance, light):
    name = 'BP_GalaxySkybox_' + label
    path = DEST + '/' + name
    if ED.does_asset_exist(path):
        raise RuntimeError('Refusing existing actor ' + path)
    factory = ue.BlueprintFactory()
    factory.set_editor_property('parent_class', ue.Actor)
    bp = TOOLS.create_asset(name, DEST, ue.Blueprint, factory)
    if not bp:
        raise RuntimeError('Blueprint creation failed')
    sub = ue.get_engine_subsystem(ue.SubobjectDataSubsystem)
    lib = ue.SubobjectDataBlueprintFunctionLibrary
    root = sub.k2_gather_subobject_data_for_blueprint(bp)[0]

    def component(cls, name):
        params = ue.AddNewSubobjectParams()
        for key, value in {'parent_handle': root, 'new_class': cls, 'blueprint_context': bp}.items():
            params.set_editor_property(key, value)
        handle, reason = sub.add_new_subobject(params)
        if str(reason):
            raise RuntimeError('Component failed: ' + str(reason))
        sub.rename_subobject(handle, name)
        return lib.get_object_for_blueprint(lib.get_data(handle), bp)

    sphere = component(ue.StaticMeshComponent, 'SkySphere')
    sphere.set_static_mesh(ED.load_asset('/Engine/BasicShapes/Sphere'))
    sphere.set_material(0, instance)
    # Engine sphere diameter is 100cm; radius 50km encloses the combat volume.
    sphere.set_editor_property('relative_scale3d', ue.Vector(100000, 100000, 100000))
    sphere.set_collision_enabled(ue.CollisionEnabled.NO_COLLISION)
    sphere.set_editor_property('cast_shadow', False)
    sphere.set_editor_property('visible_in_ray_tracing', False)
    sphere.set_editor_property('affect_distance_field_lighting', False)
    sphere.set_editor_property('receives_decals', False)
    sphere.set_editor_property('mobility', ue.ComponentMobility.STATIC)
    sun = component(ue.DirectionalLightComponent, 'Sunlight')
    sun.set_editor_property('mobility', ue.ComponentMobility.MOVABLE)
    sun.set_editor_property('relative_rotation', ue.Rotator(pitch=light['sun_pitch_deg'], yaw=light['sun_yaw_deg'], roll=0))
    sun.set_editor_property('intensity', light['directional_lux'])
    sun.set_editor_property('use_temperature', True)
    sun.set_editor_property('temperature', light['directional_temperature_kelvin'])
    pp = component(ue.PostProcessComponent, 'PreviewExposure')
    pp.set_editor_property('unbound', True)
    pp.set_editor_property('priority', -10.0)
    pp.set_editor_property('settings', post_process(light['bloom_intensity']))
    # Default actor tick is disabled; no construction/event graph or gameplay API.
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    checked_save(bp)
    return bp


def main():
    if not ue.SystemLibrary.get_engine_version().startswith('5.9.0-60351404'):
        raise RuntimeError('Expected UE 5.9 CL60351404: ' + ue.SystemLibrary.get_engine_version())
    names = ['T_TD_Nebula_BC', 'T_TD_Stars_BC', 'M_TD_SpaceFoundation']
    labels = ['Hero', 'Combat', 'Verdance']
    names += ['MI_TD_Space_' + label for label in labels]
    names += ['BP_GalaxySkybox_' + label for label in labels]
    for name in names:
        if ED.does_asset_exist(DEST + '/' + name):
            raise RuntimeError('Existing asset; no changes made: ' + name)
    builder = frozen_builder()
    nodes = []
    original_node = builder.node

    def record_node(*args, **kwargs):
        node = original_node(*args, **kwargs)
        nodes.append(node)
        return node

    builder.node = record_node
    builder.main()
    tuning = apply_native_tuning()
    for name in ('T_TD_Nebula_BC', 'T_TD_Stars_BC'):
        texture = ED.load_asset(DEST + '/' + name)
        texture.set_editor_property('never_stream', True)
        texture.set_editor_property('compression_settings', ue.TextureCompressionSettings.TC_BC7)
        texture.set_editor_property('filter', ue.TextureFilter.TF_TRILINEAR)
        checked_save(texture)
    master = ED.load_asset(DEST + '/M_TD_SpaceFoundation')
    # The original spherical UV wraps correctly at the texture level, but implicit
    # derivatives across atan2's branch cut select blurry mips. Unwrap U gradients.
    projection = next(n for n in nodes if isinstance(n, ue.MaterialExpressionCustom))
    gradients = original_node(master, ue.MaterialExpressionCustom, -800, 900,
        code='float2 x=ddx(UV); float2 y=ddy(UV); x.x-=round(x.x); y.x-=round(y.x); return float4(x,y);',
        output_type=ue.CustomMaterialOutputType.CMOT_FLOAT4,
        inputs=[custom_input('UV')], description='Unwrapped longitude gradients')
    builder.connect(projection, '', gradients, 'UV')
    dx = original_node(master, ue.MaterialExpressionComponentMask, -500, 850, r=True, g=True, b=False, a=False)
    dy = original_node(master, ue.MaterialExpressionComponentMask, -500, 1000, r=False, g=False, b=True, a=True)
    builder.connect(gradients, '', dx, 'Input')
    builder.connect(gradients, '', dy, 'Input')
    for sample in [n for n in nodes if isinstance(n, ue.MaterialExpressionTextureSampleParameter2D)]:
        sample.set_editor_property('mip_value_mode', ue.TextureMipValueMode.TMVM_DERIVATIVE)
        builder.connect(dx, '', sample, 'DDX(UVs)')
        builder.connect(dy, '', sample, 'DDY(UVs)')
    stars = next(n for n in nodes if isinstance(n, ue.MaterialExpressionTextureSampleParameter2D)
                 and str(n.get_editor_property('parameter_name')) == 'StarTexture')
    star_scale = next(n for n in nodes if isinstance(n, ue.MaterialExpressionMultiply)
                      and stars in ML.get_inputs_for_material_expression(master, n))
    star_intensity = next(n for n in nodes if isinstance(n, ue.MaterialExpressionScalarParameter)
                          and str(n.get_editor_property('parameter_name')) == 'StarIntensity')
    clock = original_node(master, ue.MaterialExpressionTime, -1000, 1200)
    amount = original_node(master, ue.MaterialExpressionScalarParameter, -1000, 1350,
        parameter_name='TwinkleAmount', default_value=0.08, group='Space Foundation')
    rate = original_node(master, ue.MaterialExpressionScalarParameter, -1000, 1500,
        parameter_name='TwinkleRate', default_value=0.22, group='Space Foundation')
    twinkle = original_node(master, ue.MaterialExpressionCustom, -600, 1250,
        code='return 1.0 + saturate(Amount)*sin(Clock*Rate*6.2831853 + UV.x*232.4778564 + UV.y*119.3805208);',
        output_type=ue.CustomMaterialOutputType.CMOT_FLOAT1,
        inputs=[custom_input(n) for n in ('Clock', 'UV', 'Amount', 'Rate')],
        description='Subtle sky-direction star twinkle; no actor tick')
    for src, pin in ((clock, 'Clock'), (projection, 'UV'), (amount, 'Amount'), (rate, 'Rate')):
        builder.connect(src, '', twinkle, pin)
    animated_gain = original_node(master, ue.MaterialExpressionMultiply, -250, 1250)
    builder.connect(twinkle, '', animated_gain, 'A')
    builder.connect(star_intensity, '', animated_gain, 'B')
    builder.connect(animated_gain, '', star_scale, 'B')
    ML.layout_material_expressions(master)
    errors = ML.recompile_material(master)
    if errors:
        raise RuntimeError('Material compile errors: ' + str(errors))
    checked_save(master)
    presets = json.loads((HERE / 'presets.json').read_text())
    light_presets = presets['lighting_presets']
    light_presets['verdance'] = dict(light_presets['hero'], directional_lux=7,
        directional_temperature_kelvin=7300, sun_pitch_deg=-18, sun_yaw_deg=28, bloom_intensity=0.16)
    assets = names[:6]
    for label in labels:
        material = ED.load_asset(DEST + '/MI_TD_Space_' + label)
        ML.update_material_instance(material)
        checked_save(material)
        actor = blueprint_rig(label, material, light_presets[label.lower()])
        assets.append(actor.get_name())
    world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    ue.SystemLibrary.execute_console_command(world, 'Editor.AsyncTextureCompilationFinishAll')
    ue.SystemLibrary.execute_console_command(world, 'Editor.AsyncStaticMeshCompilationFinishAll')
    report = {
        'engine': ue.SystemLibrary.get_engine_version(),
        'assets': [DEST + '/' + name for name in assets],
        'shader_errors': list(errors), 'lighting_presets': light_presets, 'native_tuning': tuning,
        'texture_samples': 2, 'sky_draws': 1, 'runtime_particles': 0,
        'note': 'Budget counts, not measured GPU timing. Isolated content host; gameplay integration pending.'}
    (HERE / 'review').mkdir(exist_ok=True)
    (HERE / 'review' / 'build.json').write_text(json.dumps(report, indent=2) + '\n')
    ue.log('TD_ENV_BUILD_SUCCESS ' + json.dumps(report))


if __name__ == '__main__':
    main()

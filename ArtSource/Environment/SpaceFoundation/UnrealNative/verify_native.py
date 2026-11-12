"""Separate-process native reload, component audit, isolated maps and renders."""
import json
from pathlib import Path
import unreal as ue
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from build_native import DEST, post_process, apply_native_tuning


def main():
    report_path = HERE / 'review' / 'build.json'
    build = json.loads(report_path.read_text())
    loaded = []
    for path in build['assets']:
        obj = ue.EditorAssetLibrary.load_asset(path)
        if not obj:
            raise RuntimeError('Fresh-process reload failed: ' + path)
        loaded.append(obj)
    tuning = apply_native_tuning()
    # Correct pre-verification SCS sunlight templates using named Python Rotator fields.
    sub = ue.get_engine_subsystem(ue.SubobjectDataSubsystem)
    lib = ue.SubobjectDataBlueprintFunctionLibrary
    for label in ('Hero', 'Combat', 'Verdance'):
        bp = ue.EditorAssetLibrary.load_asset(DEST + '/BP_GalaxySkybox_' + label)
        light = build['lighting_presets'][label.lower()]
        for handle in sub.k2_gather_subobject_data_for_blueprint(bp):
            obj = lib.get_object_for_blueprint(lib.get_data(handle), bp)
            if isinstance(obj, ue.DirectionalLightComponent):
                rotation = obj.get_editor_property('relative_rotation')
                if abs(rotation.pitch - light['sun_pitch_deg']) > 0.01 or abs(rotation.yaw - light['sun_yaw_deg']) > 0.01 or abs(rotation.roll) > 0.01:
                    obj.set_editor_property('relative_rotation', ue.Rotator(pitch=light['sun_pitch_deg'], yaw=light['sun_yaw_deg'], roll=0))
                    ue.BlueprintEditorLibrary.compile_blueprint(bp)
                    ue.EditorAssetLibrary.save_loaded_asset(bp)
    master = ue.EditorAssetLibrary.load_asset(DEST + '/M_TD_SpaceFoundation')
    # GetStatistics submits and finishes compilation of the SAVED shader state.
    # RecompileMaterial here would invalidate child render proxies just before capture.
    errors = []
    stats = ue.MaterialEditingLibrary.get_statistics(master)
    statistics = {name: stats.get_editor_property(name) for name in (
        'num_vertex_shader_instructions', 'num_pixel_shader_instructions',
        'num_samplers', 'num_vertex_texture_samples', 'num_pixel_texture_samples')}
    editor = ue.get_editor_subsystem(ue.UnrealEditorSubsystem)
    actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
    levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    # Sky art is small, always visible and should not stream low mips on spawn.
    for name in ('T_TD_Nebula_BC', 'T_TD_Stars_BC'):
        texture = ue.EditorAssetLibrary.load_asset(DEST + '/' + name)
        for name, value in {'never_stream': True,
                            'compression_settings': ue.TextureCompressionSettings.TC_BC7,
                            'filter': ue.TextureFilter.TF_TRILINEAR}.items():
            if texture.get_editor_property(name) != value:
                texture.set_editor_property(name, value)
        ue.EditorAssetLibrary.save_loaded_asset(texture)
    results = []
    for label in ('Hero', 'Combat', 'Verdance'):
        map_path = DEST + '/Review/L_SpaceFoundation_' + label
        bp = ue.EditorAssetLibrary.load_asset(DEST + '/BP_GalaxySkybox_' + label)
        if ue.EditorAssetLibrary.does_asset_exist(map_path):
            if not levels.load_level(map_path):
                raise RuntimeError('Could not reload owned review level')
            rig = next(a for a in actors.get_all_level_actors() if a.get_class() == bp.generated_class())
        else:
            if not levels.new_level(map_path):
                raise RuntimeError('Could not create isolated review level')
            rig = actors.spawn_actor_from_class(bp.generated_class(), ue.Vector())
        rig.set_actor_label('Space Foundation / ' + label)
        mesh = rig.get_component_by_class(ue.StaticMeshComponent)
        sun = rig.get_component_by_class(ue.DirectionalLightComponent)
        exposure = rig.get_component_by_class(ue.PostProcessComponent)
        assert mesh and sun and exposure, 'Missing rig component after reload'
        assert mesh.get_collision_enabled() == ue.CollisionEnabled.NO_COLLISION
        assert not mesh.get_editor_property('cast_shadow')
        assert not mesh.get_editor_property('visible_in_ray_tracing')
        assert mesh.get_material(0).get_name() == 'MI_TD_Space_' + label
        camera = next((a for a in actors.get_all_level_actors() if isinstance(a, ue.CameraActor)), None)
        if camera is None:
            camera = actors.spawn_actor_from_class(ue.CameraActor, ue.Vector())
        camera.set_actor_label('Review camera')
        camera.set_actor_rotation(ue.Rotator(pitch=10, yaw=125, roll=0), False)
        camera.get_component_by_class(ue.CameraComponent).set_editor_property('field_of_view', 90.0)
        if not levels.save_current_level():
            raise RuntimeError('Review level save failed')
        world = editor.get_editor_world()
        capture_actor = actors.spawn_actor_from_class(ue.SceneCapture2D, ue.Vector())
        capture = capture_actor.get_component_by_class(ue.SceneCaptureComponent2D)
        capture.set_editor_property('capture_every_frame', False)
        capture.set_editor_property('capture_on_movement', False)
        capture.set_editor_property('capture_source', ue.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
        capture.set_editor_property('fov_angle', 90.0)
        capture.set_editor_property('post_process_settings', post_process(build['lighting_presets'][label.lower()]['bloom_intensity']))
        capture.set_editor_property('post_process_blend_weight', 1.0)
        target = ue.RenderingLibrary.create_render_target2d(world, 1280, 720, ue.TextureRenderTargetFormat.RTF_RGBA8)
        target.set_editor_property('target_gamma', 2.2)
        capture.set_editor_property('texture_target', target)
        ue.SystemLibrary.execute_console_command(world, 'Editor.AsyncTextureCompilationFinishAll')
        ue.SystemLibrary.execute_console_command(world, 'Editor.AsyncStaticMeshCompilationFinishAll')
        instance_stats = ue.MaterialEditingLibrary.get_statistics(mesh.get_material(0))
        assert instance_stats.get_editor_property('num_pixel_shader_instructions') > 0, 'Missing instance shader'
        for name, size in (('T_TD_Nebula_BC', (1024, 512)), ('T_TD_Stars_BC', (2048, 1024))):
            texture = ue.EditorAssetLibrary.load_asset(DEST + '/' + name)
            assert (texture.blueprint_get_size_x(), texture.blueprint_get_size_y()) == size, 'Unfinished texture resource'
        shots = [('Panorama', ue.Rotator(pitch=10, yaw=125, roll=0))]
        if label == 'Hero':
            shots += [('LongitudeSeam', ue.Rotator(pitch=0, yaw=0, roll=0)), ('NorthPole', ue.Rotator(pitch=89, yaw=0, roll=0))]
        files = []
        rotations = {}
        for title, rotation in shots:
            capture_actor.set_actor_rotation(rotation, False)
            actual = capture_actor.get_actor_rotation()
            assert all(abs(getattr(actual, axis) - getattr(rotation, axis)) < 0.01 for axis in ('pitch', 'yaw', 'roll'))
            rotations[title] = {axis: getattr(actual, axis) for axis in ('pitch', 'yaw', 'roll')}
            capture.capture_scene()
            name = label + '_' + title + '.png'
            ue.RenderingLibrary.export_render_target(world, target, str(HERE / 'review'), name)
            path = HERE / 'review' / name
            if not path.is_file() or path.stat().st_size < 1000:
                raise RuntimeError('Missing native capture: ' + str(path))
            files.append(name)
        actors.destroy_actor(capture_actor)
        ue.RenderingLibrary.release_render_target2d(target)
        if not levels.save_current_level():
            raise RuntimeError('Could not save owned review level after capture cleanup')
        results.append({'preset': label, 'map': map_path, 'screenshots': files,
                        'capture_rotations_deg': rotations,
                        'sun_rotation_deg': {axis: getattr(sun.get_editor_property('relative_rotation'), axis) for axis in ('pitch', 'yaw', 'roll')},
                        'collision': 'disabled', 'cast_shadow': False,
                        'sun_lux': sun.get_editor_property('intensity')})
    report = {'engine': ue.SystemLibrary.get_engine_version(), 'fresh_process_reload': len(loaded), 'native_tuning': tuning,
              'shader_errors': list(errors), 'material_statistics': statistics,
              'presets': results, 'gameplay_PIE': 'not run; isolated content-only host',
              'packaged_build': 'not tested', 'combat_gpu_timing': 'not measured'}
    (HERE / 'review' / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')
    ue.log('TD_ENV_VERIFY_SUCCESS ' + json.dumps(report))


if __name__ == '__main__':
    main()

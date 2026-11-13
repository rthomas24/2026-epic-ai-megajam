"""Editor-only, in-memory Hero probe. Loaded by the guarded launcher's startup command.

No save, import, asset mutation, progression cheat or arbitrary-code endpoint.
MCP owns PIE lifecycle; this tool spawns and touches only its own live PIE actor.
"""
import json
import os
from pathlib import Path
import sys

sys.dont_write_bytecode = True
import unreal as ue
from toolset_registry.registration import Registration

BASE = '/Game/TriadDrift/Environment/SpaceFoundation'
CANDIDATE = BASE + '/HeroM02'
RIG_CLASS = CANDIDATE + '/BP_GalaxySkybox_HeroM02.BP_GalaxySkybox_HeroM02_C'
MATERIALS = {'M01': BASE + '/MI_TD_Space_Hero',
             'M02': CANDIDATE + '/MI_HeroM02_Benchmark'}
STATE = {'rig': None, 'previous_world': None, 'world_stopped': False, 'settings': []}


def world():
    result = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
    assert result and 'L_Galaxy' in result.get_path_name(), 'Actual Galaxy PIE world required'
    return result


def tagged(w, tag):
    return list(ue.GameplayStatics.get_all_actors_with_tag(w, ue.Name(tag)))


def live_rig(w):
    rigs = tagged(w, 'TD.Presentation.Sky')
    assert len(rigs) == 1, 'Expected exactly one presentation sky'
    rig = rigs[0]
    assert rig.get_class().get_path_name() == RIG_CLASS, 'Unexpected sky class'
    assert not tagged(w, 'TD.Placeholder.Sky'), 'Placeholder lighting has not yielded'
    assert not tagged(w, 'TD.Placeholder.Stars'), 'Placeholder stars have not yielded'
    return rig


def values(obj, fields):
    return {field: float(getattr(obj, field)) for field in fields}


def snapshot():
    w = world()
    rig = live_rig(w)
    mesh = rig.get_component_by_class(ue.StaticMeshComponent)
    assert mesh.get_collision_enabled() == ue.CollisionEnabled.NO_COLLISION
    assert not mesh.get_editor_property('cast_shadow')
    camera = ue.GameplayStatics.get_player_camera_manager(w, 0)
    assert camera, 'Player camera missing'
    hud_cls = ue.load_class(None, '/Game/TriadDrift/UI/TriadHUD/WBP_SpaceHUD.WBP_SpaceHUD_C')
    assert hud_cls, 'Saved native HUD WBP missing'
    widgets = ue.WidgetBlueprintLibrary.get_all_widgets_of_class(w, hud_cls, False)
    visible = [item for item in widgets if item.is_visible()]
    assert len(visible) == 1, 'Exactly one visible saved native HUD required'
    sun = rig.get_component_by_class(ue.DirectionalLightComponent)
    pp = rig.get_component_by_class(ue.PostProcessComponent)
    settings = pp.get_editor_property('settings')
    pc = ue.GameplayStatics.get_player_controller(w, 0)
    pawn = pc.get_pawn()
    assert pawn, 'Possessed ship missing'
    return {
        'world': w.get_path_name(), 'world_wrapper_id': id(w),
        'world_time_seconds': ue.GameplayStatics.get_time_seconds(w),
        'paused': ue.GameplayStatics.is_game_paused(w),
        'rig': rig.get_path_name(), 'class': rig.get_class().get_path_name(),
        'rig_position': values(rig.get_actor_location(), ('x', 'y', 'z')),
        'rig_rotation': values(rig.get_actor_rotation(), ('pitch', 'yaw', 'roll')),
        'material': mesh.get_material(0).get_path_name(),
        'camera_position': values(camera.get_camera_location(), ('x', 'y', 'z')),
        'camera_rotation': values(camera.get_camera_rotation(), ('pitch', 'yaw', 'roll')),
        'fov': camera.get_fov_angle(), 'pawn': pawn.get_path_name(),
        'pawn_class': pawn.get_class().get_path_name(),
        'hud_class': visible[0].get_class().get_path_name(),
        'sun_lux': sun.get_editor_property('intensity'),
        'sun_rotation': values(sun.get_world_rotation(), ('pitch', 'yaw', 'roll')),
        'post_process': {key: str(settings.get_editor_property(key)) for key in (
            'auto_exposure_method', 'auto_exposure_bias',
            'auto_exposure_apply_physical_camera_exposure', 'bloom_intensity',
            'vignette_intensity', 'motion_blur_amount')},
        'placeholder_lights': 0, 'placeholder_stars': 0,
    }


def prepare():
    assert ue.SystemLibrary.get_engine_version().startswith('5.9.0-60351404')
    isolated_user = Path(os.environ['TD_HERO_RUN_DIR']).resolve() / 'RuntimeUser'
    saved = Path(ue.Paths.project_saved_dir()).resolve()
    assert saved.is_relative_to(isolated_user), 'Human save directory is not isolated'
    editor = ue.get_editor_subsystem(ue.UnrealEditorSubsystem)
    assert editor.get_game_world() is None, 'Prepare must run before PIE'
    w = editor.get_editor_world()
    assert w and w.get_path_name() == '/Game/TriadDrift/Maps/L_Galaxy.L_Galaxy'
    assert STATE['rig'] is None and not tagged(w, 'TD.Presentation.Sky')
    # In-memory settings only. Restore these before exit; never call SaveConfig.
    for cls, properties in (
        (ue.EditorLoadingSavingSettings, {'auto_save_enable': False}),
        (ue.LevelEditorPlaySettings, {'new_window_width': 1920, 'new_window_height': 1080}),
    ):
        obj = ue.get_default_object(cls)
        for name, value in properties.items():
            STATE['settings'].append((obj, name, obj.get_editor_property(name)))
            obj.set_editor_property(name, value)
    cls = ue.load_class(None, RIG_CLASS)
    assert cls
    shader_stats = {}
    for label, path in MATERIALS.items():
        material = ue.load_asset(path)
        assert material
        # Waits for an existing material's SM6 shader resource; no package save.
        stats = ue.MaterialEditingLibrary.get_statistics(material)
        shader_stats[label] = {name: stats.get_editor_property(name) for name in (
            'num_pixel_shader_instructions', 'num_samplers', 'num_pixel_texture_samples')}
        assert shader_stats[label]['num_pixel_shader_instructions'] > 0
    return {'map_saved': False, 'editor_world_actor_created': False,
            'engine': ue.SystemLibrary.get_engine_version(),
            'material_shader_stats': shader_stats, 'isolated_saved_directory': str(saved)}


def spawn_live():
    w = world()
    assert not tagged(w, 'TD.Presentation.Sky'), 'A sky already exists in this PIE world'
    cls = ue.load_class(None, RIG_CLASS)
    assert cls
    identity = ue.Transform()
    statics = ue.get_default_object(ue.GameplayStatics)
    # The native UFUNCTIONs have BlueprintInternalUseOnly and therefore no direct
    # Python attributes. Object.call_method is the installed engine's supported
    # reflection bridge for UFUNCTIONs without Python glue (PyWrapperObject.cpp).
    actor = statics.call_method('BeginDeferredActorSpawnFromClass', args=(
        w, cls, identity, ue.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,
        None, ue.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    assert actor, 'Deferred spawn in the actual PIE world failed'
    STATE['rig'] = actor
    finished = statics.call_method('FinishSpawningActor', args=(
        actor, identity, ue.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    assert finished == actor and actor.get_world() == w
    assert actor.get_class().get_path_name() == RIG_CLASS
    assert values(actor.get_actor_location(), ('x', 'y', 'z')) == dict(x=0, y=0, z=0)
    assert values(actor.get_actor_rotation(), ('pitch', 'yaw', 'roll')) == dict(pitch=0, yaw=0, roll=0)
    assert values(actor.get_actor_scale3d(), ('x', 'y', 'z')) == dict(x=1, y=1, z=1)
    assert 'TD.Presentation.Sky' in [str(tag) for tag in actor.get_editor_property('tags')]
    assert actor.get_component_by_class(ue.StaticMeshComponent).get_material(0) == ue.load_asset(MATERIALS['M02'])
    # Do not snapshot yet: the world's existing placeholder rig/star timer must
    # receive >0.5 seconds of unpaused game time before checking takeover.
    return {'live_rig': actor.get_path_name(), 'world': w.get_path_name(),
            'spawn_world_time_seconds': ue.GameplayStatics.get_time_seconds(w),
            'editor_world_actor_created': False, 'map_saved': False,
            'spawn_api': 'GameplayStatics native UFUNCTIONs via Object.call_method'}


@ue.uclass()
class HeroReadabilityProbe(ue.ToolsetDefinition):
    @ue.ufunction(static=True, ret=str, params=[str], meta={'AICallable': '', 'AIAccessMode': 'Write'})
    def run(action):
        if action == 'prepare':
            result = prepare()
        elif action == 'spawn_live':
            result = spawn_live()
        elif action == 'snapshot':
            result = snapshot()
        elif action == 'inventory':
            w = world()
            rig = live_rig(w)
            others = []
            for actor in ue.GameplayStatics.get_all_actors_of_class(w, ue.Actor):
                if actor == rig:
                    continue
                for component in actor.get_components_by_class(ue.ActorComponent):
                    if isinstance(component, ue.DirectionalLightComponent) or (
                        isinstance(component, ue.PostProcessComponent) and component.get_editor_property('unbound')):
                        others.append(component.get_path_name())
                if isinstance(actor, ue.PostProcessVolume) and actor.get_editor_property('unbound'):
                    others.append(actor.get_path_name())
            result = {'additional_directional_or_unbound_pp': others, 'removed_by_probe': []}
        elif action in ('pause', 'resume'):
            w = world()
            ue.GameplayStatics.set_game_paused(w, action == 'pause')
            result = snapshot()
        elif action in MATERIALS:
            w = world()
            rig = live_rig(w)
            rig.get_component_by_class(ue.StaticMeshComponent).set_material(0, ue.load_asset(MATERIALS[action]))
            result = snapshot()
        elif action == 'remember_world':
            # str(UObject) includes the native address (PyWrapperObject.cpp:781).
            # Store no strong reference to an old UWorld across StopPIE/GC.
            STATE['previous_world'] = str(world())
            result = snapshot()
        elif action == 'release_live_references':
            STATE['rig'] = None
            result = {'own_python_actor_reference_released': True, 'actor_not_destroyed_by_probe': True}
        elif action == 'confirm_world_stopped':
            assert ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world() is None
            STATE['world_stopped'] = True
            result = {'active_game_world_absent': True}
        elif action == 'verify_new_world':
            assert STATE['previous_world'] is not None and STATE['world_stopped']
            assert str(world()) != STATE['previous_world'], 'PIE native world representation did not change'
            result = snapshot()
            result['new_uobject_world'] = True
            result['previous_world_description'] = STATE['previous_world']
            result['new_world_description'] = str(world())
            result['fresh_process_reload'] = False
            STATE['previous_world'] = None
            STATE['world_stopped'] = False
        elif action == 'cleanup':
            assert ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world() is None
            # PIE teardown already destroyed this test's world-owned live actor.
            # No editor-world actor has ever been placed or removed.
            STATE['rig'] = None
            STATE['previous_world'] = None
            for obj, name, value in reversed(STATE['settings']):
                obj.set_editor_property(name, value)
            STATE['settings'].clear()
            result = {'own_live_actor_released_with_pie': True, 'editor_world_actor_created': False,
                      'settings_restored_in_memory': True, 'saved': False}
        elif action == 'quit':
            assert STATE['rig'] is None and not STATE['settings']
            ue.SystemLibrary.quit_editor()
            result = {'quit_requested': True}
        else:
            raise ValueError('Unknown bounded probe action: ' + action)
        return json.dumps(result)


_registration = Registration([HeroReadabilityProbe])
assert _registration.register(), 'Probe toolset registration failed'
ue.log('HERO_GAMEPLAY_PROBE_REGISTERED')

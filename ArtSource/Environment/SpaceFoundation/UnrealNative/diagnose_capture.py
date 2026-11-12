"""Read-only native capture diagnostics; writes images/JSON only in owned review."""
import json
from pathlib import Path
import sys
import unreal as ue
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from build_native import DEST, post_process

levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
levels.load_level(DEST + '/Review/L_SpaceFoundation_Hero')
world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
rig = next(a for a in actors.get_all_level_actors() if a.get_class().get_name() == 'BP_GalaxySkybox_Hero_C')
mesh = rig.get_component_by_class(ue.StaticMeshComponent)
state = {'location': str(mesh.get_world_location()), 'scale': str(mesh.get_world_scale()),
         'material': mesh.get_material(0).get_path_name(), 'visible': mesh.is_visible(),
         'parameters': {n: ue.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(mesh.get_material(0), n)
                        for n in ('SkyIntensity', 'NebulaIntensity', 'StarIntensity')}}
camera = actors.spawn_actor_from_class(ue.SceneCapture2D, ue.Vector())
camera.set_actor_rotation(ue.Rotator(pitch=10, yaw=125, roll=0), False)
capture = camera.get_component_by_class(ue.SceneCaptureComponent2D)
capture.set_editor_property('capture_every_frame', False)
capture.set_editor_property('capture_on_movement', False)
capture.set_editor_property('always_persist_rendering_state', True)
capture.set_editor_property('fov_angle', 90.0)
target = ue.RenderingLibrary.create_render_target2d(world,1280,720,ue.TextureRenderTargetFormat.RTF_RGBA8)
target.set_editor_property('target_gamma', 2.2)
capture.set_editor_property('texture_target', target)
# Commandlets do not tick the editor's async asset completion queue. A loaded
# texture can still be a 32px placeholder, and a mesh can lack its render data.
ue.SystemLibrary.execute_console_command(world, 'Editor.AsyncTextureCompilationFinishAll')
ue.SystemLibrary.execute_console_command(world, 'Editor.AsyncStaticMeshCompilationFinishAll')
ue.MaterialEditingLibrary.recompile_material(ue.EditorAssetLibrary.load_asset(DEST+'/M_TD_SpaceFoundation'))
for title, source, bias in (
    ('HDR',ue.SceneCaptureSource.SCS_SCENE_COLOR_HDR,0),
    ('LDR',ue.SceneCaptureSource.SCS_FINAL_COLOR_LDR,0),
    ('LDR_EV6',ue.SceneCaptureSource.SCS_FINAL_COLOR_LDR,6)):
    capture.set_editor_property('capture_source', source)
    pp=post_process(0.12)
    pp.set_editor_property('auto_exposure_bias',bias)
    capture.set_editor_property('post_process_settings',pp)
    capture.set_editor_property('post_process_blend_weight',1.0)
    for _ in range(3):
        capture.capture_scene()
    ue.RenderingLibrary.export_render_target(world,target,str(HERE/'review'),'Diagnostic_'+title+'.png')
state['texture_info'] = []
for name in ('T_TD_Nebula_BC','T_TD_Stars_BC'):
    texture=ue.EditorAssetLibrary.load_asset(DEST+'/'+name)
    state['texture_info'].append({'name':name,'never_stream':texture.get_editor_property('never_stream'),
                                  'size':[texture.blueprint_get_size_x(),texture.blueprint_get_size_y()]})
(HERE/'review'/'capture_diagnostic.json').write_text(json.dumps(state,indent=2)+'\n')
ue.log('TD_CAPTURE_DIAG '+json.dumps(state))

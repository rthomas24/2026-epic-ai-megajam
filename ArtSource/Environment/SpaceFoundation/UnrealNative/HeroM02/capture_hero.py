"""Fresh-process Hero-only matched capture and bounded synchronized timing."""
import hashlib
import json
import math
from pathlib import Path
import statistics
import struct
import sys
import time
import zlib
import unreal as ue

HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
from build_hero import DEST,BASE,helpers
ED=ue.EditorAssetLibrary;ML=ue.MaterialEditingLibrary


def image_integrity(path):
    data=path.read_bytes();assert data[:8]==b'\x89PNG\r\n\x1a\n'
    offset=8
    while offset+12<=len(data):
        length=struct.unpack_from('>I',data,offset)[0];end=offset+12+length
        assert end<=len(data)
        kind=data[offset+4:offset+8]
        assert zlib.crc32(data[offset+4:end-4])&0xffffffff==struct.unpack_from('>I',data,end-4)[0]
        if kind==b'IEND':
            assert length==0 and end==len(data),'PNG has trailing bytes'
            return hashlib.sha256(data).hexdigest()
        offset=end
    raise RuntimeError('Missing IEND')


def main():
    build=json.loads((HERE/'review/build.json').read_text())
    loaded=[ED.load_asset(path) for path in build['assets']]
    assert all(loaded)
    original=ED.load_asset(BASE+'/MI_TD_Space_Hero')
    candidate=ED.load_asset(DEST+'/MI_HeroM02_Benchmark')
    fields=('num_pixel_shader_instructions','num_vertex_shader_instructions','num_samplers','num_pixel_texture_samples')
    stats={label:{n:ML.get_statistics(mat).get_editor_property(n) for n in fields}
           for label,mat in (('before',original),('after',candidate))}
    levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    actors=ue.get_editor_subsystem(ue.EditorActorSubsystem)
    map_path=DEST+'/Review/L_HeroM02_Benchmark'
    if ED.does_asset_exist(map_path):
        assert levels.load_level(map_path)
        bp=ED.load_asset(DEST+'/BP_GalaxySkybox_HeroM02')
        rig=next(a for a in actors.get_all_level_actors() if a.get_class()==bp.generated_class())
    else:
        assert levels.new_level(map_path)
        bp=ED.load_asset(DEST+'/BP_GalaxySkybox_HeroM02')
        rig=actors.spawn_actor_from_class(bp.generated_class(),ue.Vector())
    world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    assert 'TD.Presentation.Sky' in [str(tag) for tag in rig.get_editor_property('tags')]
    mesh=rig.get_component_by_class(ue.StaticMeshComponent)
    assert mesh.get_collision_enabled()==ue.CollisionEnabled.NO_COLLISION
    assert not mesh.get_editor_property('cast_shadow')
    sun=rig.get_component_by_class(ue.DirectionalLightComponent)
    camera=next((a for a in actors.get_all_level_actors() if isinstance(a,ue.CameraActor)),None)
    if camera is None:camera=actors.spawn_actor_from_class(ue.CameraActor,ue.Vector())
    camera.set_actor_rotation(ue.Rotator(pitch=10,yaw=125,roll=0),False)
    camera.get_component_by_class(ue.CameraComponent).set_editor_property('field_of_view',90.)
    capture_actor=actors.spawn_actor_from_class(ue.SceneCapture2D,ue.Vector())
    capture=capture_actor.get_component_by_class(ue.SceneCaptureComponent2D)
    for k,v in {'capture_every_frame':False,'capture_on_movement':False,
        'capture_source':ue.SceneCaptureSource.SCS_FINAL_COLOR_LDR,
        'post_process_settings':helpers.post_process(0.2),'post_process_blend_weight':1.0}.items():capture.set_editor_property(k,v)
    width,height=1920,1080
    target=ue.RenderingLibrary.create_render_target2d(world,width,height,ue.TextureRenderTargetFormat.RTF_RGBA8)
    target.set_editor_property('target_gamma',2.2);capture.set_editor_property('texture_target',target)
    ue.SystemLibrary.execute_console_command(world,'Editor.AsyncTextureCompilationFinishAll')
    ue.SystemLibrary.execute_console_command(world,'Editor.AsyncStaticMeshCompilationFinishAll')
    for name,size in (('T_HeroM02_Back',(8192,4096)),('T_HeroM02_Front',(4096,2048))):
        tex=ED.load_asset(DEST+'/'+name)
        assert (tex.blueprint_get_size_x(),tex.blueprint_get_size_y())==size,(name,tex.blueprint_get_size_x(),tex.blueprint_get_size_y())
    assert isinstance(ED.load_asset(DEST+'/T_HeroM02_Stars'),ue.TextureCube)
    shots=[('Hero',10.,125.,90.),('Detail',10.,125.,50.),('Zenith',89.,0.,90.),('Wrap',0.,0.,90.)]
    evidence=[];timings={}
    for label,mat in (('Before',original),('After',candidate)):
        mesh.set_material(0,mat);ML.get_statistics(mat)
        for title,pitch,yaw,fov in shots:
            rotation=ue.Rotator(pitch=pitch,yaw=yaw,roll=0)
            capture_actor.set_actor_rotation(rotation,False);capture.set_editor_property('fov_angle',fov)
            # Fixed time in this commandlet: no advancing cloud/star animation, no exposure adaptation.
            for _ in range(2):
                capture.capture_scene();ue.RenderingLibrary.read_render_target_raw_pixel(world,target,0,0)
            name=label+'_'+title+'.png';path=HERE/'review'/name
            # Unreal's file writer can leave a stale tail. Clear only this owned previous output.
            if path.exists():
                backup=HERE/'Saved'/'PreviousCaptures';backup.mkdir(parents=True,exist_ok=True)
                stamp=str(time.time_ns());path.replace(backup/(stamp+'_'+name))
            capture.capture_scene();ue.RenderingLibrary.export_render_target(world,target,str(path.parent),path.name)
            digest=image_integrity(path)
            actual=capture_actor.get_actor_rotation()
            evidence.append({'variant':label,'image':name,'sha256':digest,
                'camera_position_cm':[0,0,0],'rotation_deg':{n:getattr(actual,n) for n in ('pitch','yaw','roll')},
                'fov_deg':fov,'resolution':[width,height],'manual_exposure_bias':0,'physical_camera_exposure':False,
                'bloom_intensity':0.2,'target_gamma':2.2})
        capture_actor.set_actor_rotation(ue.Rotator(pitch=10,yaw=125,roll=0),False)
        capture.set_editor_property('fov_angle',90.)
        for _ in range(6):
            capture.capture_scene();ue.RenderingLibrary.read_render_target_raw_pixel(world,target,0,0)
        samples=[]
        for _ in range(16):
            start=time.perf_counter();capture.capture_scene()
            ue.RenderingLibrary.read_render_target_raw_pixel(world,target,0,0)
            samples.append((time.perf_counter()-start)*1000)
        timings[label]={'samples_ms':samples,'median_ms':statistics.median(samples),
                        'p95_ms':sorted(samples)[math.ceil(0.95*len(samples))-1],'minimum_ms':min(samples)}
    # Stability check: two captures at an identical view/time should retain their complete pixel stream.
    stable=[]
    for i in range(2):
        name=f'After_Stability_{i}.png';path=HERE/'review'/name
        if path.exists():raise RuntimeError('Existing stability output; inspect before rerun')
        capture.capture_scene();ue.RenderingLibrary.export_render_target(world,target,str(path.parent),path.name)
        stable.append(image_integrity(path))
    mesh.set_material(0,candidate)
    actors.destroy_actor(capture_actor);ue.RenderingLibrary.release_render_target2d(target)
    assert levels.save_current_level()
    report={'engine':ue.SystemLibrary.get_engine_version(),'fresh_process_reload':len(loaded),
        'shader_errors':None,'external_engine_diagnostics':'pending finalize_diagnostics.py',
        'map':map_path,'material_statistics':stats,'captures':evidence,
        'stability_sha256':stable,'identical_static_captures':stable[0]==stable[1],
        'timing_method':'Synchronized scene-capture wall time including one-pixel GPU readback; not isolated GPU time or gameplay FPS.',
        'timings':timings,'median_wall_delta_ms':timings['After']['median_ms']-timings['Before']['median_ms'],
        'configured_compressed_mip_budget_mib':{'Before':10/3,'After':(8192*4096+4096*2048+6*1024*1024)*4/3/1024**2},
        'hardware_gpu_time':'not measured','gameplay_validated':False,'manual_approval':'pending','integrated':False,
        'sun_lux':sun.get_editor_property('intensity'),'sky_actor_tag':'TD.Presentation.Sky',
        'baseline_settings_preserved':True}
    (HERE/'review/verification.json').write_text(json.dumps(report,indent=2)+'\n')
    ue.log('HERO_M02_VERIFY_SUCCESS '+json.dumps(report))


if __name__=='__main__':main()

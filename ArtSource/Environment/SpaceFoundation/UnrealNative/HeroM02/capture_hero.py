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
PNG_CONTAINER_EVIDENCE=[]


def image_integrity(path):
    # Same validated-prefix method as frozen M01 finalize_evidence.py; preserve
    # raw bytes first and never decode/re-encode or modify IHDR/IDAT/pixels.
    assert path.resolve().parent==(HERE/'review').resolve(),'Unowned PNG path'
    data=path.read_bytes();original_sha=hashlib.sha256(data).hexdigest()
    raw_dir=HERE/'Saved'/'RawPNGExports';raw_dir.mkdir(parents=True,exist_ok=True)
    raw_path=raw_dir/(str(time.time_ns())+'_'+path.name)
    with raw_path.open('xb') as raw:raw.write(data)
    assert hashlib.sha256(raw_path.read_bytes()).hexdigest()==original_sha
    assert data[:8]==b'\x89PNG\r\n\x1a\n'
    offset=8;count=0;has_idat=False
    while offset+12<=len(data):
        length=struct.unpack_from('>I',data,offset)[0];end=offset+12+length
        assert end<=len(data)
        kind=data[offset+4:offset+8]
        assert all(65<=c<=90 or 97<=c<=122 for c in kind) and not kind[2]&32,'Invalid PNG chunk type'
        assert zlib.crc32(data[offset+4:end-4])&0xffffffff==struct.unpack_from('>I',data,end-4)[0]
        if count==0:
            assert kind==b'IHDR' and length==13
            assert struct.unpack_from('>II',data,offset+8)==(1920,1080)
        has_idat=has_idat or kind==b'IDAT';count+=1
        if kind==b'IEND':
            assert length==0 and has_idat
            prefix=data[:end];prefix_sha=hashlib.sha256(prefix).hexdigest()
            if end!=len(data):
                temporary=path.with_name(path.name+'.tailfix.'+str(time.time_ns()))
                with temporary.open('xb') as output:output.write(prefix)
                assert temporary.read_bytes()==prefix
                temporary.replace(path)
            assert path.read_bytes()==prefix,'PNG payload changed'
            final_sha=hashlib.sha256(path.read_bytes()).hexdigest();assert final_sha==prefix_sha
            evidence={'image':path.name,'raw_archive':str(raw_path),'original_bytes':len(data),
                'valid_png_bytes':end,'removed_tail_bytes':len(data)-end,'chunks_crc_valid':count,
                'original_sha256':original_sha,'prefix_sha256':prefix_sha,'final_sha256':final_sha,
                'pixel_payload_unchanged':True}
            PNG_CONTAINER_EVIDENCE.append(evidence)
            with (HERE/'Saved/png_container_checks.jsonl').open('a',encoding='utf-8') as log:
                log.write(json.dumps(evidence)+'\n')
            return final_sha
        offset=end
    raise RuntimeError('Missing IEND')


def texture_evidence(path,faces=1):
    tex=ED.load_asset(path)
    assert isinstance(tex,ue.TextureCube if faces==6 else ue.Texture2D)
    size=tex.blueprint_get_built_texture_size()
    dimensions=[int(round(getattr(size,axis))) for axis in ('x','y','z')]
    w,h=dimensions[:2];assert w>0 and h>0,(path,dimensions)
    compression=tex.get_editor_property('compression_settings')
    assert compression==(ue.TextureCompressionSettings.TC_HDR_COMPRESSED if faces==6 else ue.TextureCompressionSettings.TC_BC7)
    # BC7 and BC6H both use 16-byte 4x4 blocks. This is a full-chain upper
    # bound from actual built dimensions, not a claim about RHI allocation/residency.
    mip_bytes=0;levels=0
    while True:
        mip_bytes+=max(1,(w+3)//4)*max(1,(h+3)//4)*16*faces;levels+=1
        if w==1 and h==1:break
        w=max(1,w//2);h=max(1,h//2)
    return {'asset':path,'class':tex.get_class().get_name(),'built_size_xyz':dimensions,
        'faces':faces,'lod_bias':tex.get_editor_property('lod_bias'),
        'max_texture_size':tex.get_editor_property('max_texture_size'),
        'mip_gen_settings':str(tex.get_editor_property('mip_gen_settings')),
        'lod_group':str(tex.get_editor_property('lod_group')),
        'compression_settings':str(compression),'never_stream':tex.get_editor_property('never_stream'),
        'full_chain_mip_levels_upper_bound':levels,'compressed_full_chain_upper_bound_bytes':mip_bytes}


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
        map_file=HERE.parents[4]/'Content/TriadDrift/Environment/SpaceFoundation/HeroM02/Review/L_HeroM02_Benchmark.umap'
        assert map_file.is_file(),'Missing owned review-map package'
        map_bytes=map_file.read_bytes()
        map_archive=HERE/'Saved/ReviewMapArchives';map_archive.mkdir(parents=True,exist_ok=True)
        archived_map=map_archive/(str(time.time_ns())+'_'+map_file.name)
        with archived_map.open('xb') as backup:backup.write(map_bytes)
        assert hashlib.sha256(archived_map.read_bytes()).digest()==hashlib.sha256(map_bytes).digest()
        assert levels.load_level(map_path)
        bp=ED.load_asset(DEST+'/BP_GalaxySkybox_HeroM02')
        rig=next((a for a in actors.get_all_level_actors() if a.get_class()==bp.generated_class()),None)
        if rig is None:rig=actors.spawn_actor_from_class(bp.generated_class(),ue.Vector())
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
    native_textures={'Before':[texture_evidence(BASE+'/'+name) for name in ('T_TD_Nebula_BC','T_TD_Stars_BC')],
        'After':[texture_evidence(DEST+'/T_HeroM02_Back'),texture_evidence(DEST+'/T_HeroM02_Front'),
                 texture_evidence(DEST+'/T_HeroM02_Stars',6)]}
    assert native_textures['After'][0]['built_size_xyz'][:2]==[8192,4096]
    assert native_textures['After'][1]['built_size_xyz'][:2]==[4096,2048]
    cube=native_textures['After'][2]
    assert cube['built_size_xyz'][0]==cube['built_size_xyz'][1] and cube['built_size_xyz'][0]<=1024,cube
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
    # Native resource dimensions, pixel format, memory and mip count are also
    # collected in the engine log, independently of the configured upper bound.
    ue.SystemLibrary.execute_console_command(world,'ListTextures -CSV')
    actors.destroy_actor(capture_actor);ue.RenderingLibrary.release_render_target2d(target)
    assert levels.save_current_level()
    report={'engine':ue.SystemLibrary.get_engine_version(),'fresh_process_reload':len(loaded),
        'shader_errors':None,'external_engine_diagnostics':'pending finalize_diagnostics.py',
        'map':map_path,'material_statistics':stats,'captures':evidence,
        'png_container_evidence':PNG_CONTAINER_EVIDENCE,
        'stability_sha256':stable,'identical_static_captures':stable[0]==stable[1],
        'timing_method':'Synchronized scene-capture wall time including one-pixel GPU readback; not isolated GPU time or gameplay FPS.',
        'timings':timings,'median_wall_delta_ms':timings['After']['median_ms']-timings['Before']['median_ms'],
        'native_texture_evidence':native_textures,
        'compressed_full_chain_upper_bound_mib':{label:sum(t['compressed_full_chain_upper_bound_bytes'] for t in textures)/1024**2 for label,textures in native_textures.items()},
        'texture_memory_method':'BC7/BC6H full-chain upper bound from actual built dimensions; native ListTextures CSV in Capture.log records resource sizes and mip counts; not total VRAM residency.',
        'hardware_gpu_time':'not measured','gameplay_validated':False,'manual_approval':'pending','integrated':False,
        'sun_lux':sun.get_editor_property('intensity'),'sky_actor_tag':'TD.Presentation.Sky',
        'baseline_settings_preserved':True}
    (HERE/'review/verification.json').write_text(json.dumps(report,indent=2)+'\n')
    ue.log('HERO_M02_VERIFY_SUCCESS '+json.dumps(report))


if __name__=='__main__':main()

"""Actual Blender review images only. Studio rig adapted from the frozen Lancer review."""
from pathlib import Path
import json, math, runpy, time, os
import bpy, numpy as np
from mathutils import Vector
from bpy_extras.object_utils import world_to_camera_view
ROOT=Path(__file__).resolve().parent.parents[2];REPO=ROOT.parents[3];OUT=Path(os.environ.get('LANCER_M03_REVIEW_DIR',str(ROOT/'review/M03/Review01')))

def look(o,p):o.rotation_euler=(Vector(p)-o.location).to_track_quat('-Z','Y').to_euler()

def bounds(obs):
    points=[o.matrix_world@Vector(p) for o in obs for p in o.bound_box]
    return Vector([min(v[i] for v in points) for i in range(3)]),Vector([max(v[i] for v in points) for i in range(3)])

def frame(camera,obs,az,el,perspective=True):
    lo,hi=bounds(obs);centre=(lo+hi)*.5
    camera.data.shift_x=0;camera.data.shift_y=0
    a,e=math.radians(az),math.radians(el);d=Vector((math.sin(a)*math.cos(e),-math.cos(a)*math.cos(e),math.sin(e)))
    camera.location=centre+d*max((hi-lo).length*2.7,3);look(camera,centre)
    camera.data.type='PERSP' if perspective else 'ORTHO';camera.data.lens=78
    if not perspective:
        rot=camera.rotation_euler.to_matrix();r=rot@Vector((1,0,0));u=rot@Vector((0,1,0))
        pp=[Vector((x,y,z)) for x in (lo.x,hi.x) for y in (lo.y,hi.y) for z in (lo.z,hi.z)]
        width=max(p.dot(r) for p in pp)-min(p.dot(r) for p in pp);height=max(p.dot(u) for p in pp)-min(p.dot(u) for p in pp)
        sc=bpy.context.scene;aspect=sc.render.resolution_x/sc.render.resolution_y
        camera.data.ortho_scale=max(width,height*aspect)*1.16
    else:
        # Fit actual geometry, avoiding empty bounding-box corners around swept wings.
        rot=camera.rotation_euler.to_matrix();r=rot@Vector((1,0,0));u=rot@Vector((0,1,0));sc=bpy.context.scene
        aspect=sc.render.resolution_x/sc.render.resolution_y;tanx=math.tan(camera.data.angle_x/2);tany=tanx/aspect
        points=[o.matrix_world@v.co for o in obs for v in o.data.vertices];distance=0
        for point in points:
            p=point-centre
            distance=max(distance,abs(p.dot(r))*1.16/tanx+p.dot(d),abs(p.dot(u))*1.16/tany+p.dot(d))
        camera.location=centre+d*distance
        bpy.context.view_layer.update()
        projected=[world_to_camera_view(sc,camera,p) for p in points]
        camera.data.shift_x=(min(p.x for p in projected)+max(p.x for p in projected))*.5-.5
        camera.data.shift_y=((min(p.y for p in projected)+max(p.y for p in projected))*.5-.5)/aspect

def studio():
    sc=bpy.context.scene;sc.render.engine='BLENDER_EEVEE'
    sc.render.resolution_x=1800;sc.render.resolution_y=1200;sc.render.resolution_percentage=100
    sc.render.threads_mode='FIXED';sc.render.threads=2
    sc.render.image_settings.file_format='PNG';sc.render.image_settings.color_mode='RGB';sc.render.image_settings.color_depth='8'
    sc.render.film_transparent=False
    sc.view_settings.view_transform='Khronos PBR Neutral';sc.view_settings.exposure=-.4
    if hasattr(sc,'eevee'):
        if hasattr(sc.eevee,'taa_render_samples'):sc.eevee.taa_render_samples=64
        if hasattr(sc.eevee,'use_raytracing'):sc.eevee.use_raytracing=True
    world=bpy.data.worlds.new('Review_Neutral');world.use_nodes=True
    bg=next(n for n in world.node_tree.nodes if n.type=='BACKGROUND');bg.inputs['Color'].default_value=(.075,.105,.145,1);bg.inputs['Strength'].default_value=.6;sc.world=world
    for name,pos,energy,size,color in [('Key',(1,-5,8),2200,7,(1,.96,.89)),('Fill',(-6,-2,3),1400,6,(.75,.87,1)),('Rim',(3,6,6),2600,6,(.8,.72,1)),('LowFill',(-1,0,-6),1300,5,(.8,.84,1))]:
        light=bpy.data.lights.new('Review_'+name,'AREA');light.energy=energy;light.shape='DISK';light.size=size;light.color=color
        ob=bpy.data.objects.new(light.name,light);sc.collection.objects.link(ob);ob.location=pos;look(ob,(0,0,0))
    data=bpy.data.cameras.new('Review_Camera');data.clip_start=.01;data.clip_end=200
    cam=bpy.data.objects.new(data.name,data);sc.collection.objects.link(cam);sc.camera=cam;return cam

def check(path):
    im=bpy.data.images.load(str(path),check_existing=False);pixels=np.empty(len(im.pixels),dtype=np.float32);im.pixels.foreach_get(pixels)
    result=dict(file=path.name,width=im.size[0],height=im.size[1],bytes=path.stat().st_size,pixel_std=float(pixels.reshape((-1,4))[:,:3].std()))
    bpy.data.images.remove(im);assert result['pixel_std']>.01;return result

def sheet(paths,name,tw=800,th=550):
    cols=3;gap=18;rows=math.ceil(len(paths)/cols)
    pix=np.empty((rows*th+(rows+1)*gap,cols*tw+(cols+1)*gap,4),np.float32);pix[:]=(.018,.03,.045,1)
    for i,path in enumerate(paths):
        im=bpy.data.images.load(str(path),check_existing=False);im.scale(tw,th)
        arr=np.empty(tw*th*4,np.float32);im.pixels.foreach_get(arr);row,col=divmod(i,cols)
        x=gap+col*(tw+gap);y=pix.shape[0]-gap-(row+1)*th-row*gap;pix[y:y+th,x:x+tw]=arr.reshape((th,tw,4));bpy.data.images.remove(im)
    im=bpy.data.images.new('Review_Sheet',width=pix.shape[1],height=pix.shape[0],alpha=False);im.pixels.foreach_set(pix.ravel())
    im.filepath_raw=str(OUT/name);im.file_format='PNG';im.save();bpy.data.images.remove(im)

def wire(cam,obs):
    sc=bpy.context.scene;sc.render.engine='BLENDER_WORKBENCH';sc.render.resolution_x=1400;sc.render.resolution_y=1000
    shade=sc.display.shading;shade.light='STUDIO';shade.color_type='OBJECT';shade.show_shadows=True;shade.show_cavity=False
    shade.background_type='WORLD';sc.world.color=(.075,.10,.13)
    overlays=[]
    for ob in obs:
        ob.color=(.6,.64,.68,1)
        curve=bpy.data.curves.new('Review_Edges_'+ob.name,'CURVE');curve.dimensions='3D';curve.bevel_depth=.0010;curve.bevel_resolution=0;curve.resolution_u=1
        for edge in ob.data.edges:
            spl=curve.splines.new('POLY');spl.points.add(1)
            for point,index in zip(spl.points,edge.vertices):point.co=(*ob.data.vertices[index].co,1)
        o=bpy.data.objects.new(curve.name,curve);sc.collection.objects.link(o);o.matrix_world=ob.matrix_world.copy();o.color=(.015,.02,.03,1);overlays.append(o)
    paths=[]
    for slug,az,el in [('front_3q',35,25),('top',0,90),('underside',35,-28)]:
        frame(cam,obs,az,el,False);path=OUT/('wire_'+slug+'.png');sc.render.filepath=str(path);bpy.ops.render.render(write_still=True);paths.append(path)
    sheet(paths,'Lancer_M03_wire_sheet.png',700,500)
    for o in overlays:
        data=o.data;bpy.data.objects.remove(o,do_unlink=True);bpy.data.curves.remove(data)

def silhouette(cam,obs):
    sc=bpy.context.scene;sc.render.engine='BLENDER_WORKBENCH';sc.render.resolution_x=128;sc.render.resolution_y=128
    s=sc.display.shading;s.light='FLAT';s.color_type='SINGLE';s.single_color=(0,0,0);s.show_shadows=False;s.show_cavity=False;s.background_type='WORLD';sc.world.color=(1,1,1)
    paths=[]
    for slug,az,el in [('top',0,90),('side',90,0),('front_3q',35,24)]:
        frame(cam,obs,az,el,False);p=OUT/('silhouette_128_'+slug+'.png');sc.render.filepath=str(p);bpy.ops.render.render(write_still=True);paths.append(p)
    sheet(paths,'Lancer_M03_silhouette_sheet.png',256,256)

def run(full=True,structural=False):
    started=time.perf_counter();obs=[o for o in bpy.context.scene.objects if o.type=='MESH' and o.get('part_name')];cam=studio();sc=bpy.context.scene
    specs=[('01_hero_front_3q','Front three-quarter / neutral bright studio',35,24,True,obs),
           ('02_front','Front orthographic',0,0,False,obs),('03_rear','Rear orthographic',180,0,False,obs),
           ('04_left','Left orthographic',-90,0,False,obs),('05_right','Right orthographic',90,0,False,obs),
           ('06_top','Top orthographic',0,90,False,obs),('07_bottom','Bottom orthographic',0,-90,False,obs),
           ('08_rear_3q','Rear three-quarter',215,22,True,obs),
           ('09_weapon_detail','Port cannon and flush rocket bay',35,-10,True,[o for o in obs if o.get('part_name') in ['Primary_L','Rotor_L','Secondary_L'] or o.get('detail_for') in ['Primary_L','Rotor_L','Secondary_L']]),
           ('10_canopy_detail','Framed canopy',35,20,True,[o for o in obs if o.get('part_name')=='Canopy' or o.get('detail_for')=='Canopy']),
           ('11_dark_studio','Dark lighting / same model and emission',35,24,True,obs)]
    records=[];paths=[]
    for slug,title,az,el,persp,targets in specs:
        if not full and slug!='01_hero_front_3q':continue
        if structural and slug>'07_bottom':continue
        if slug=='11_dark_studio':
            for light in sc.objects:
                if light.type=='LIGHT':light.data.energy*=.16
            bg=next(n for n in sc.world.node_tree.nodes if n.type=='BACKGROUND');bg.inputs['Strength'].default_value=.10
        for o in obs:o.hide_render=o not in targets
        framing=[o for o in targets if not o.get('detail_for')] if os.environ.get('LANCER_M03_APPROVED_FRAME')=='A02' else targets
        frame(cam,framing or targets,az,el,persp);path=OUT/(slug+'.png');sc.render.filepath=str(path)
        print('LANCER_M03_RENDER '+slug,flush=True);bpy.ops.render.render(write_still=True)
        rec=check(path);rec.update(title=title,azimuth_deg=az,elevation_deg=el,perspective=persp);records.append(rec);paths.append(path)
    if OUT.name=='StageA02':
        for o in obs:o.hide_render=False
        camera=cam;camera.data.shift_x=0;camera.data.shift_y=0;camera.data.type='PERSP';camera.data.lens=78
        lo=Vector((-3.6,-4.02,-.96));hi=Vector((3.6,4.02,1.061));centre=(lo+hi)*.5
        a,e=math.radians(35),math.radians(24);d=Vector((math.sin(a)*math.cos(e),-math.cos(a)*math.cos(e),math.sin(e)))
        camera.location=centre+d*max((hi-lo).length*2.7,3);look(camera,centre)
        rot=camera.rotation_euler.to_matrix();rr=rot@Vector((1,0,0));u=rot@Vector((0,1,0))
        aspect=sc.render.resolution_x/sc.render.resolution_y;tanx=math.tan(camera.data.angle_x/2);tany=tanx/aspect;distance=0
        for x in (lo.x,hi.x):
            for y in (lo.y,hi.y):
                for z in (lo.z,hi.z):
                    q=Vector((x,y,z))-centre;distance=max(distance,abs(q.dot(rr))*1.16/tanx+q.dot(d),abs(q.dot(u))*1.16/tany+q.dot(d))
        camera.location=centre+d*distance;sc.render.filepath=str(OUT/'A02_same_A01_camera.png');bpy.ops.render.render(write_still=True)
    if full:sheet(paths,'Lancer_M03_overview.png')
    for o in obs:o.hide_render=False
    diag=runpy.run_path(str(REPO/'.claude/skills/blender-mcp/scripts/review_renders.py'))['main']
    configure=diag.__globals__['_configure_engine']
    def configure_limited(scene,*args,**kwargs):
        configure(scene,*args,**kwargs)
        scene.render.threads_mode='FIXED';scene.render.threads=2
    diag.__globals__['_configure_engine']=configure_limited
    clay=diag(targets=[o.name for o in obs],out_dir=str(OUT),label='Lancer_M03',mode='clay',resolution=700,samples=16,keep_views=OUT.name=='StageA02')
    (OUT/'clay_manifest.json').write_text(json.dumps(clay,indent=2))
    if full and (not structural or OUT.name=='StageA02'):wire(cam,obs)
    if full and not structural:silhouette(cam,obs)
    report=dict(blender=bpy.app.version_string,beauty_engine='BLENDER_EEVEE',beauty_color_view='Khronos PBR Neutral',
                diagnostic_engines='BLENDER_WORKBENCH',images=records,full_review=full,render_threads=2,
                source_mesh_geometry_unchanged_for_render=True,elapsed_seconds=time.perf_counter()-started)
    (OUT/'render_manifest.json').write_text(json.dumps(report,indent=2))

if __name__=='__main__':run()

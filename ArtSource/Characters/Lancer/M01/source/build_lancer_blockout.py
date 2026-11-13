"""Original armor PRIMARY FORMS on a static owned mannequin copy. No new anatomy/rig."""
import bpy, bmesh, json, math, os, hashlib, runpy
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree

ROOT=next(p for p in Path(__file__).resolve().parents if (p/'unrealGameJam.uproject').is_file())
OWN=Path(__file__).resolve().parent.parent
BASE=OWN/'source/base/Lancer_Manny_Reference_v02.blend'
OUT=OWN/'source/armor/Blockout03'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    assert os.environ.get('TD_LANCER_BLOCKOUT_CLEARED')=='BUILD'
    assert Path(bpy.data.filepath).resolve()==BASE.resolve()
    original=sha(BASE);assert original=='6eaaed062f11d3dc37de22556d6492a0cb647b8b644d4757c2b65b0441e03607'
    assert not OUT.exists();OUT.mkdir(parents=True)
    scene=bpy.context.scene;scene.render.threads_mode='FIXED';scene.render.threads=2
    ref=bpy.data.collections['REF_Manny_NotForExport'];assert ref.hide_render and not ref['export_allowed']
    rig=next(o for o in ref.objects if o.type=='ARMATURE')
    bone=lambda n:rig.matrix_world @ rig.data.bones[n].head_local
    col=bpy.data.collections.new('MODEL_Lancer_Blockout03_STATIC');scene.collection.children.link(col)
    col['stage']='Primary armor forms; no rigging or production approval'
    kit=runpy.run_path(str(ROOT/'.claude/skills/blender-game-asset/scripts/hardsurface.py'))
    def material(name,hexcolor,roughness,emission=0):
        m=bpy.data.materials.new(name);rgb=tuple(int(hexcolor[i:i+2],16)/255 for i in (0,2,4));m.diffuse_color=(*rgb,1)
        if m.node_tree is None:m.use_nodes=True  # Installed5.1 compatibility; no external material dependency.
        node=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
        linear=tuple(x/12.92 if x<=.04045 else ((x+.055)/1.055)**2.4 for x in rgb)
        node.inputs['Base Color'].default_value=(*linear,1);node.inputs['Roughness'].default_value=roughness
        node.inputs['Metallic'].default_value=0
        if emission:node.inputs['Emission Color'].default_value=(*linear,1);node.inputs['Emission Strength'].default_value=emission
        return m
    white=material('M_Lancer_ArmorWhite','DDE5E9',.4);dark=material('M_Lancer_FlexGraphite','252B33',.76)
    cyan=material('M_Lancer_CyanOptics','21C8E6',.22,1.1)
    parts=[]
    def obj(name,verts,faces,mat,bevel=.0015):
        me=bpy.data.meshes.new('GEO_'+name);me.from_pydata(verts,[],faces);me.update()
        bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free();me.update()
        # Independent nonoverlapping planar face charts for blockout QA; final texel layout is later.
        uv=me.uv_layers.new(name='UVMap');grid=math.ceil(math.sqrt(len(me.polygons)))
        for index,poly in enumerate(me.polygons):
            axes=[i for i in range(3) if i!=max(range(3),key=lambda j:abs(poly.normal[j]))]
            coords=[(me.vertices[me.loops[k].vertex_index].co[axes[0]],me.vertices[me.loops[k].vertex_index].co[axes[1]]) for k in poly.loop_indices]
            low=[min(c[i] for c in coords) for i in range(2)];span=[max(c[i] for c in coords)-low[i] for i in range(2)]
            assert min(span)>1e-9, 'Degenerate armor face before UV layout'
            for k,c in zip(poly.loop_indices,coords):uv.data[k].uv=(((index%grid)+.04+.92*(c[0]-low[0])/span[0])/grid,((index//grid)+.04+.92*(c[1]-low[1])/span[1])/grid)
        ob=bpy.data.objects.new('SM_Lancer_'+name,me);col.objects.link(ob);me.materials.append(mat)
        if bevel:kit['add_bevel'](ob,width=bevel,segments=2);kit['add_weighted_normal'](ob)
        parts.append(ob);return ob
    dg=bpy.context.evaluated_depsgraph_get();sources=[o for o in ref.objects if o.type=='MESH'];assert len(sources)==1
    src=sources[0];me=bpy.data.meshes.new_from_object(src.evaluated_get(dg));me.transform(src.matrix_world)
    body=bpy.data.objects.new('SM_Lancer_OwnedBase_StaticCopy',me);col.objects.link(body);me.materials.clear();me.materials.append(dark)
    for p in me.polygons:p.material_index=0
    allpoints=[v.co.copy() for v in me.vertices];height=max(p.z for p in allpoints)
    # Keep the complete closed owned body for this silhouette pass; no new open neck loops.
    head=bone('head')
    bm=bmesh.new();bm.from_mesh(me);bvh=BVHTree.FromBMesh(bm);bm.free()
    panel_specs=[];surface_fit=[];pelvis_fit=[]
    def panel(name,outline,side,mat=white,offset=.018,thick=.008,mount=True):
        panel_specs.append(dict(name=name,outline=outline,side=side,mat=mat,offset=offset,thick=thick))
        if mount:
            n=len(outline);cx=sum(x for x,z in outline)/n;cz=sum(z for x,z in outline)/n
            panel(name+'_Mount',[(cx-.012,cz-.014),(cx+.012,cz-.014),(cx+.012,cz+.014),(cx-.012,cz+.014)],side,dark,.021,.026,False)
            panel_specs[-1]['parent']=name
    def fit_panels():
        # Measure each horizontal body cross-section, never invent a missing surface.
        # Clamp only inward towards the torso centre; keep height and chevron order.
        # All panel and support samples must pass before any panel is constructed.
        max_fit=.065;step=.001;torso_low=hip-.18;torso_high=shoulder+.03
        center_y=bone('pelvis').y
        def cast(x,z,side):
            hit,normal,_,_=bvh.ray_cast(Vector((x,side*.9,z)),Vector((0,-side,0)),1.8)
            if hit is None:return None
            if not (torso_low<=hit.z<=torso_high and abs(hit.x)<=.23):return None
            if not (0 < side*(hit.y-center_y) < .23 and side*normal.y>.15):return None
            return hit
        fitted=[];resolved={}
        for spec in panel_specs:
            if spec['name']=='PelvisFront':
                # The lower crotch is not a continuous torso cross-section. Measure
                # the actual central/side boundary, and shorten only the guard's
                # lower edge; the upper attachment and owned body stay unchanged.
                requested=min(z for x,z in spec['outline']);upper=max(z for x,z in spec['outline'])
                lower_x=sorted(x for x,z in spec['outline'] if abs(z-requested)<1e-8)
                assert len(lower_x)==2
                samples=[lower_x[0]+(lower_x[1]-lower_x[0])*i/8 for i in range(9)]
                max_raise=.080;fitted_z=None
                for i in range(81):
                    candidate=requested+i*.001
                    if candidate>upper-.035:break
                    if all(cast(x,candidate,spec['side']) is not None for x in samples):
                        fitted_z=candidate;break
                if fitted_z is None or fitted_z-requested>max_raise+1e-8:
                    raise RuntimeError('No measured pelvis lower boundary within80mm; guard retained in plan')
                spec['outline']=[(x,fitted_z if abs(z-requested)<1e-8 else z) for x,z in spec['outline']]
                pelvis_fit.append(dict(panel=spec['name'],requested_lower_z=requested,measured_lower_z=fitted_z,vertical_correction_m=fitted_z-requested,max_correction_m=max_raise,cross_section_x_samples=samples,upper_attachment_z=upper))
            if spec.get('parent'):
                parent=resolved[spec['parent']];cx=sum(p.x for p in parent)/len(parent);cz=sum(p.z for p in parent)/len(parent)
                spec['outline']=[(cx-.012,cz-.014),(cx+.012,cz-.014),(cx+.012,cz+.014),(cx-.012,cz+.014)]
            hits=[]
            for x,z in spec['outline']:
                if not (torso_low<=z<=torso_high and abs(x)<=.23):
                    raise RuntimeError('Panel sample outside anatomical torso region: '+spec['name'])
                if cast(0,z,spec['side']) is None:
                    raise RuntimeError('No central torso cross-section: '+spec['name']+repr(z))
                hit=None
                for i in range(math.ceil(max_fit/step)+1):
                    dx=min(abs(x),i*step);sample_x=math.copysign(abs(x)-dx,x)
                    hit=cast(sample_x,z,spec['side'])
                    if hit is not None:break
                if hit is None or abs(hit.x-x)>max_fit+1e-8:
                    raise RuntimeError('Bounded torso cross-section fit failed: '+spec['name']+repr((x,z)))
                hits.append(hit)
                surface_fit.append(dict(panel=spec['name'],requested_xz=[x,z],surface=list(hit),distance_m=abs(hit.x-x),side=spec['side']))
            # Prevent inward fits collapsing or reversing a panel footprint.
            area=sum(hits[i].x*hits[(i+1)%len(hits)].z-hits[(i+1)%len(hits)].x*hits[i].z for i in range(len(hits)))/2
            outline=spec['outline'];original_area=sum(outline[i][0]*outline[(i+1)%len(outline)][1]-outline[(i+1)%len(outline)][0]*outline[i][1] for i in range(len(outline)))/2
            if abs(area)<1e-5 or area*original_area<=0 or min((hits[i]-hits[(i+1)%len(hits)]).length for i in range(len(hits)))<.003:
                raise RuntimeError('Collapsed torso panel after fitting: '+spec['name'])
            fitted.append((spec,hits));resolved[spec['name']]=hits
        return fitted
    def construct_panel(spec,hits):
        outer=[(p.x,p.y+spec['side']*spec['offset'],p.z) for p in hits]
        inner=[(x,y-spec['side']*spec['thick'],z) for x,y,z in outer];n=len(outer)
        faces=[tuple(range(n)),tuple(range(2*n-1,n-1,-1))]+[(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
        return obj(spec['name'],outer+inner,faces,spec['mat'])
    def shell(name,a,b,profiles,mat=white,arc=math.pi,segments=12,thick=.007,solid=False):
        a,b=Vector(a),Vector(b);axis=(b-a).normalized();front=Vector((0,-1,0));front=(front-axis*front.dot(axis)).normalized();right=axis.cross(front).normalized()
        full=abs(arc-math.pi)<1e-6;count=segments if full else segments+1
        verts=[]
        for inner in (False,True):
            for t,w,d in profiles:
                for j in range(count):
                    angle=-arc+2*arc*j/segments
                    verts.append(a+(b-a)*t+right*math.sin(angle)*(w-(thick if inner else 0))+front*math.cos(angle)*(d-(thick if inner else 0)))
        rings=len(profiles);layer=rings*count;faces=[]
        if solid:
            assert full
            faces=[(r*count+j,r*count+(j+1)%count,(r+1)*count+(j+1)%count,(r+1)*count+j) for r in range(rings-1) for j in range(count)]
            faces.extend([tuple(range(count-1,-1,-1)),tuple((rings-1)*count+j for j in range(count))])
            return obj(name,verts[:layer],faces,mat)
        for r in range(rings-1):
            for j in range(segments):
                k=(j+1)%count
                faces.extend([(r*count+j,r*count+k,(r+1)*count+k,(r+1)*count+j),(layer+r*count+k,layer+r*count+j,layer+(r+1)*count+j,layer+(r+1)*count+k)])
        for j in range(segments):
            k=(j+1)%count;faces.extend([(j,k,layer+k,layer+j),((rings-1)*count+k,(rings-1)*count+j,layer+(rings-1)*count+j,layer+(rings-1)*count+k)])
        if not full:
            for r in range(rings-1):
                for j in (0,count-1):faces.append((r*count+j,(r+1)*count+j,layer+(r+1)*count+j,layer+r*count+j))
        return obj(name,verts,faces,mat)
    # Chest/back split chevrons. No arbitrary greebles; sternum/waist remain articulated.
    shoulder=(bone('upperarm_l').z+bone('upperarm_r').z)*.5
    for side,label in ((-1,'Front'),(1,'Back')):
        for sign,lr in ((1,'L'),(-1,'R')):
            points=[(.018,shoulder-.025),(.095,shoulder+.012),(.18,shoulder-.055),(.195,shoulder-.135),(.12,shoulder-.225),(.035,shoulder-.185)]
            panel(label+'_Pectoral_'+lr,[(sign*x,z) for x,z in points],side)
        panel(label+'_AbdomenUpper',[(-.09,shoulder-.235),(.09,shoulder-.235),(.07,shoulder-.32),(-.07,shoulder-.32)],side)
        panel(label+'_AbdomenLower',[(-.075,shoulder-.335),(.075,shoulder-.335),(.055,shoulder-.405),(-.055,shoulder-.405)],side)
    hip=bone('pelvis').z
    panel('PelvisFront',[(-.065,hip-.035),(.065,hip-.035),(.035,hip-.16),(-.035,hip-.16)],-1)
    fitted_panels=fit_panels()
    for spec,hits in fitted_panels:construct_panel(spec,hits)
    for suffix in ('l','r'):
        upper,elbow,wrist=bone('upperarm_'+suffix),bone('lowerarm_'+suffix),bone('hand_'+suffix)
        thigh,knee,ankle=bone('thigh_'+suffix),bone('calf_'+suffix),bone('foot_'+suffix)
        shell('Shoulder_'+suffix,upper,elbow,[(.01,.105,.105),(.15,.12,.11),(.32,.095,.095)],arc=1.9)
        shell('Bicep_'+suffix,upper,elbow,[(.39,.074,.073),(.52,.079,.075),(.83,.065,.066)],arc=1.8)
        shell('Forearm_'+suffix,elbow,wrist,[(.18,.067,.071),(.38,.072,.077),(.87,.052,.058)],arc=2.05)
        shell('HipGuard_'+suffix,thigh,knee,[(.005,.105,.119),(.05,.108,.121),(.105,.102,.113)],arc=1.8)
        shell('Thigh_'+suffix,thigh,knee,[(.12,.101,.114),(.35,.108,.118),(.86,.076,.083)],arc=2.03)
        shell('Knee_'+suffix,knee,ankle,[(.005,.076,.092),(.1,.08,.096),(.23,.066,.078)],arc=1.65)
        shell('Shin_'+suffix,knee,ankle,[(.28,.068,.083),(.42,.075,.087),(.84,.052,.061)],arc=1.9)
        footpoints=[p for p in allpoints if abs(p.x-ankle.x)<.085 and p.z<.16]
        ymin,ymax=min(p.y for p in footpoints),max(p.y for p in footpoints)
        center=Vector((ankle.x,(ymin+ymax)/2,0))
        shell('Boot_'+suffix,center+Vector((0,0,.015)),center+Vector((0,0,.155)),[(0,.077,(ymax-ymin)/2+.008),(.25,.083,(ymax-ymin)/2+.012),(1,.061,.11)],arc=math.pi)
    # Closed layered helmet, fitted to actual source head dimensions; separate optical band.
    hp=[p for p in allpoints if p.z>head.z];cx=(min(p.x for p in hp)+max(p.x for p in hp))/2;cy=(min(p.y for p in hp)+max(p.y for p in hp))/2
    width=(max(p.x for p in hp)-min(p.x for p in hp))/2+.012;depth=(max(p.y for p in hp)-min(p.y for p in hp))/2+.016
    bottom=head.z-.022;top=height+.015;span=top-bottom;a=Vector((cx,cy,bottom));b=Vector((cx,cy,top))
    shell('HelmetClosedCore',a,b,[(0,width*.64,depth*.67),(.18,width*.94,depth*.93),(.56,width,depth),(.83,width*.93,depth*.92),(1,.025,.03)],dark,solid=True)
    shell('HelmetCrown',a,b,[(.66,width+.006,depth+.006),(.84,width*.95+.006,depth*.93+.006),(1,.027,.032)],white)
    shell('HelmetJaw',a,b,[(.02,width*.66+.006,depth*.7+.006),(.2,width*.96+.006,depth*.96+.006),(.47,width+.006,depth+.006)],white,arc=2.16)
    shell('CyanVisor',a,b,[(.49,width+.008,depth+.009),(.63,width+.008,depth+.009)],cyan,arc=1.32,segments=16,thick=.002)
    names=[body.name]+[o.name for o in parts]
    scene['stage']='Lancer armor primary-form blockout; human silhouette approval pending'
    scene['no_new_rig']=True;scene['source_reference_sha256']=original
    bpy.context.view_layer.update()
    evaluated=bpy.context.evaluated_depsgraph_get();armor_tris=0
    for ob in parts:
        ev=ob.evaluated_get(evaluated);geo=ev.to_mesh()
        try:geo.calc_loop_triangles();armor_tris+=len(geo.loop_triangles)
        finally:ev.to_mesh_clear()
    assert armor_tris<=25000, 'Original armor blockout exceeds separate25k budget'
    budget=runpy.run_path(str(ROOT/'.claude/skills/blender-mcp/scripts/asset_report.py'))['main'](targets=names,tri_budget=130000)
    assembly=runpy.run_path(str(ROOT/'.claude/skills/blender-mcp/scripts/assembly_check.py'))['main'](targets=names)
    qa=runpy.run_path('C:/Users/nicho/.agents/skills/blender-asset-qa/scripts/qa_check.py')['run'](objects=names,profile='game')
    technical=budget['totals']['errors']==0 and budget['totals']['tris']<=130000 and assembly.get('ok',False) and qa['pass']
    blend=OUT/'Lancer_Armor_Blockout_v03.blend';assert bpy.ops.wm.save_as_mainfile(filepath=str(blend),check_existing=False)=={'FINISHED'}
    assert sha(BASE)==original
    report=dict(stage='PRIMARY FORM REVIEW ONLY',technical_checks_passed=technical,blend_sha256=sha(blend),reference_sha256=original,objects=names,original_armor_objects=[o.name for o in parts],original_armor_evaluated_triangles=armor_tris,model_material_roles=[white.name,dark.name,cyan.name],immutable_reference_excluded_from_model_budget=True,body_source='Complete closed owned Manny static copy; no head/neck deletion',budget=budget,assembly=assembly,qa=qa,limits=['No rig/weights/animation/native changes','Armor UVs are basic individual face charts; final texel layout/baking later','UV/final texturing and production topology not accepted','Full visual and independent review required before secondary forms'])
    (OUT/'blockout-qa.json').write_text(json.dumps(report,indent=2)+'\n')
    (OUT/'surface-fit.json').write_text(json.dumps(dict(method='Measured horizontal body cross-section; inward-only bounded torso fit; separately measured pelvic lower boundary',max_distance_m=.065,pelvis_lower_boundary=pelvis_fit,samples=surface_fit),indent=2)+'\n')
    if not technical:print('LANCER_BLOCKOUT_QA_FAILED diagnostic blend only; inspect full report before any acceptance')
    print('LANCER_BLOCKOUT_BUILD_OK '+str(blend))

if __name__=='__main__':main()

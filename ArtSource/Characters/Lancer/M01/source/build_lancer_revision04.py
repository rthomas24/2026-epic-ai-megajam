"""Reference-shaped primary armor surfaces over the unchanged owned Manny body.

Revision04 replaces outline-only plates and fixed-radius tubes. No anatomy,
rigging, hidden-body edits, secondary detail, native assets or acceptance claims.
"""
import bpy, bmesh, hashlib, json, math, os, runpy
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree

OWN = Path(__file__).resolve().parent.parent
ROOT = next(p for p in OWN.parents if (p/'unrealGameJam.uproject').is_file())
BASE = OWN/'source/base/Lancer_Manny_Reference_v02.blend'
OUT = OWN/'source/armor/Revision04'
BASE_SHA = '6eaaed062f11d3dc37de22556d6492a0cb647b8b644d4757c2b65b0441e03607'

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    assert os.environ.get('TD_LANCER_BLOCKOUT_CLEARED') == 'BUILD'
    assert Path(bpy.data.filepath).resolve() == BASE.resolve()
    assert sha(BASE) == BASE_SHA and not OUT.exists()
    OUT.mkdir(parents=True)
    scene=bpy.context.scene;scene.render.threads_mode='FIXED';scene.render.threads=2
    ref=bpy.data.collections['REF_Manny_NotForExport']
    assert ref.hide_render and not ref['export_allowed']
    rig=next(o for o in ref.objects if o.type=='ARMATURE')
    bone=lambda name: rig.matrix_world @ rig.data.bones[name].head_local
    sources=[o for o in ref.objects if o.type=='MESH'];assert len(sources)==1
    src=sources[0];dg=bpy.context.evaluated_depsgraph_get()
    bodymesh=bpy.data.meshes.new_from_object(src.evaluated_get(dg));bodymesh.transform(src.matrix_world)
    col=bpy.data.collections.new('MODEL_Lancer_Revision04_STATIC');scene.collection.children.link(col)
    col['stage']='Reference fidelity primary forms; human review pending'
    kit=runpy.run_path(str(ROOT/'.claude/skills/blender-game-asset/scripts/hardsurface.py'))
    def material(name,color,roughness,emission=0):
        m=bpy.data.materials.new(name);rgb=tuple(int(color[i:i+2],16)/255 for i in (0,2,4));m.diffuse_color=(*rgb,1)
        if m.node_tree is None:m.use_nodes=True
        node=next(n for n in m.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
        linear=tuple(c/12.92 if c<=.04045 else ((c+.055)/1.055)**2.4 for c in rgb)
        node.inputs['Base Color'].default_value=(*linear,1);node.inputs['Roughness'].default_value=roughness
        if emission:node.inputs['Emission Color'].default_value=(*linear,1);node.inputs['Emission Strength'].default_value=emission
        return m
    white=material('M_Lancer_ArmorWhite','E0E6E9',.4)
    dark=material('M_Lancer_FlexGraphite','202730',.75)
    cyan=material('M_Lancer_CyanOptics','13B9D4',.22,.6)
    body=bpy.data.objects.new('SM_Lancer_OwnedBase_StaticCopy',bodymesh);col.objects.link(body)
    bodymesh.materials.clear();bodymesh.materials.append(dark)
    for face in bodymesh.polygons:face.material_index=0
    points=[v.co.copy() for v in bodymesh.vertices]
    bm=bmesh.new();bm.from_mesh(bodymesh);body_bvh=BVHTree.FromBMesh(bm);bm.free()
    parts=[];fit_log=[];clearance_failures=[]
    # Fitting and collision use DIFFERENT trees. Preserve source skin ownership
    # only as a read-only anatomical classifier; the new model remains static.
    assert len(src.data.vertices)==len(bodymesh.vertices), 'Source vertex association changed'
    group_names={g.index:g.name for g in src.vertex_groups}
    memberships=[{group_names[g.group] for g in v.groups if g.weight>=.15 and g.group in group_names} for v in src.data.vertices]
    assert any(memberships), 'Missing source anatomical ownership; no spatial-only fallback'
    region_trees={};region_summary={}
    def limb_bones(suffix,prefixes):
        # Manny twist influences are part of their limb, not holes in its BVH.
        return {name for name in group_names.values() if name.endswith('_'+suffix) and any(name.startswith(prefix+'_') for prefix in prefixes)}
    def region(name,bones,predicate):
        selected=[]
        for face in bodymesh.polygons:
            indices=list(face.vertices)
            center=sum((points[i] for i in indices),Vector())/len(indices)
            if predicate(center) and any(memberships[i].intersection(bones) for i in indices):selected.append(indices)
        if not selected:raise RuntimeError('Empty anatomical region: '+name)
        tree=BVHTree.FromPolygons(points,selected,all_triangles=False)
        region_trees[name]=tree;region_summary[name]=dict(source_bones=sorted(bones),faces=len(selected));return tree
    torso_tree=region('torso',{'pelvis','spine_01','spine_02','spine_03','spine_04','spine_05','clavicle_l','clavicle_r'},lambda p:abs(p.x)<.24 and bone('pelvis').z-.18<=p.z<=bone('upperarm_l').z+.04)
    head_point=bone('head')
    head_tree=region('head',{'head','neck_01','neck_02'},lambda p:p.z>=head_point.z-.04 and abs(p.x-head_point.x)<.16)
    for suffix in ('l','r'):
        sign=1 if bone('thigh_'+suffix).x>0 else -1
        region('arm_'+suffix,limb_bones(suffix,('clavicle','upperarm','lowerarm','hand')),lambda p,s=sign:s*p.x>.035)
        region('leg_'+suffix,limb_bones(suffix,('thigh','calf','foot','ball')),lambda p,s=sign:s*p.x>.025 and p.z<bone('pelvis').z+.06)
        ankle=bone('foot_'+suffix)
        region('foot_'+suffix,limb_bones(suffix,('calf','foot','ball')),lambda p,s=sign,x=ankle.x:s*p.x>.025 and abs(p.x-x)<.11 and p.z<.205)
    def mesh_object(name,vertices,faces,mat=white,bevel=.001):
        me=bpy.data.meshes.new('GEO_'+name);me.from_pydata(vertices,[],faces);me.update()
        bm=bmesh.new();bm.from_mesh(me);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(me);bm.free();me.update()
        # Independent planar charts; final texture layout is outside this milestone.
        uv=me.uv_layers.new(name='UVMap');grid=math.ceil(math.sqrt(len(me.polygons)))
        for n,poly in enumerate(me.polygons):
            axes=[i for i in range(3) if i!=max(range(3),key=lambda j:abs(poly.normal[j]))]
            coords=[tuple(me.vertices[me.loops[k].vertex_index].co[i] for i in axes) for k in poly.loop_indices]
            low=[min(c[i] for c in coords) for i in range(2)];span=[max(c[i] for c in coords)-low[i] for i in range(2)]
            if min(span)<1e-10:raise RuntimeError('Degenerate primary surface: '+name)
            for k,c in zip(poly.loop_indices,coords):uv.data[k].uv=(((n%grid)+.04+.92*(c[0]-low[0])/span[0])/grid,((n//grid)+.04+.92*(c[1]-low[1])/span[1])/grid)
        ob=bpy.data.objects.new('SM_Lancer_'+name,me);col.objects.link(ob);me.materials.append(mat)
        if bevel:kit['add_bevel'](ob,width=bevel,segments=2);kit['add_weighted_normal'](ob)
        parts.append(ob);return ob
    def surface(origin,direction,tree):
        direction=Vector(direction).normalized();origin=Vector(origin)
        hit,normal,_,_=tree.ray_cast(origin+direction*.45,-direction,.9)
        return hit,normal
    def triangles(rows,cols,wrap=False):
        faces=[]
        for r in range(rows-1):
            for c in range(cols if wrap else cols-1):
                a=r*cols+c;b=r*cols+(c+1)%cols;d=(r+1)*cols+c;e=(r+1)*cols+(c+1)%cols
                faces.extend([(a,b,e),(a,e,d)])
        return faces
    def patch(name,outer,directions,rows,cols,fit_tree,mat=white,thick=.007,wrap=False,attach=True):
        """Closed tessellated cage; check every face centroid AND edge midpoint.

        Outward adjustment only, <=40mm; failures remain explicit in acceptance.
        The unmodified full body remains visible wherever armor does not cover it.
        """
        outer=[Vector(p) for p in outer];directions=[Vector(d).normalized() for d in directions]
        initial=[p.copy() for p in outer]
        faces=triangles(rows,cols,wrap);max_adjust=0;checks=0;outside_samples=0
        for iteration in range(5):
            changed=False
            for face in faces:
                for weights in ((1/3,1/3,1/3),(.5,.5,0),(0,.5,.5),(.5,0,.5)):
                    p=sum((outer[i]*w for i,w in zip(face,weights)),Vector())
                    d=sum((directions[i]*w for i,w in zip(face,weights)),Vector()).normalized()
                    hit,normal=surface(p,d,fit_tree);checks+=1
                    if hit is None:
                        outside_samples+=1;continue  # Explicitly counted silhouette-exterior samples.
                    needed=thick+.005-(p-hit).dot(d)
                    if needed>0:
                        if needed>.040 or any((outer[i]+directions[i]*(needed+.001)-initial[i]).length>.040 for i in face):
                            clearance_failures.append(dict(part=name,reason='Face exceeds40mm total correction bound',needed_m=needed))
                            continue
                        for i in face:outer[i]+=directions[i]*(needed+.001)
                        max_adjust=max(max_adjust,needed+.001);changed=True
            if not changed:break
        violations=[];global_collisions=[];minimum_full_clearance=None
        for face in faces:
            for weights in ((1/3,1/3,1/3),(.5,.5,0),(0,.5,.5),(.5,0,.5)):
                p=sum((outer[i]*w for i,w in zip(face,weights)),Vector());d=sum((directions[i]*w for i,w in zip(face,weights)),Vector()).normalized()
                hit,normal=surface(p,d,fit_tree)
                if hit is not None and (p-hit).dot(d)<thick+.004:
                    violations.append((p-hit).dot(d))
                # Never use another body region to drive a fit. Independently
                # check the INNER skin against the entire closed owned body.
                inner_sample=p-d*thick
                nearest,normal,_,_=body_bvh.find_nearest(inner_sample)
                if nearest is None:raise RuntimeError('Full body collision tree empty')
                signed=(inner_sample-nearest).dot(normal)
                minimum_full_clearance=signed if minimum_full_clearance is None else min(minimum_full_clearance,signed)
                if signed<.002:global_collisions.append(signed)
        if violations:clearance_failures.append(dict(part=name,reason='Residual sampled clearance',count=len(violations),minimum_m=min(violations)))
        if global_collisions:clearance_failures.append(dict(part=name,reason='Separate full-body collision/clearance',count=len(global_collisions),minimum_m=min(global_collisions)))
        inner=[p-d*thick for p,d in zip(outer,directions)];n=len(outer)
        closed=list(faces)+[tuple(n+i for i in reversed(f)) for f in faces]
        edges={}
        for f in faces:
            for a,b in zip(f,f[1:]+f[:1]):
                key=tuple(sorted((a,b)));edges[key]=edges.get(key,0)+1
        for (a,b),count in edges.items():
            if count==1:closed.append((a,b,n+b,n+a))
        ob=mesh_object(name,outer+inner,closed,mat)
        fit_log.append(dict(part=name,face_samples=checks,outside_region_samples=outside_samples,final_violations=len(violations),full_body_collision_samples=len(global_collisions),minimum_full_body_clearance_m=minimum_full_clearance,max_single_adjustment_m=max_adjust,max_total_adjustment_m=max((p-q).length for p,q in zip(outer,initial))))
        if attach:
            index=(rows//2)*cols+cols//2;p=outer[index];d=directions[index]
            hit,normal=surface(p,d,fit_tree)
            if hit is not None:
                # Real graphite support embedded into body and armor; no detached floaters.
                tangent=d.cross(Vector((0,0,1)))
                if tangent.length<.1:tangent=d.cross(Vector((1,0,0)))
                tangent.normalize();up=tangent.cross(d).normalized()
                start=hit-d*.005;end=p-d*(thick-.002)
                verts=[center+tangent*x*.009+up*y*.010 for center in (start,end) for x,y in ((-1,-1),(1,-1),(1,1),(-1,1))]
                mesh_object(name+'_Seat',verts,[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)],dark,.0007)
        return ob
    def torso_row(coords,side):
        # Fit the whole row with ONE width scale, preserving cell spacing and
        # the authored chevron. Independent clipping could collapse edge columns.
        width=max(abs(x) for x,z in coords);d=Vector((0,side,0))
        for step in range(66):
            scale=1-step*.001/max(width,.001)
            if scale<.55:break
            hits=[]
            for x,z in coords:
                hit,normal=surface((x*scale,0,z),d,torso_tree)
                if hit is None or normal.dot(d)<=.15 or abs(hit.x)>=.24:break
                hits.append(hit)
            if len(hits)==len(coords):return hits
        raise RuntimeError('No bounded coherent torso row '+repr((coords,side)))
    shoulder=(bone('upperarm_l').z+bone('upperarm_r').z)/2
    pelvis=bone('pelvis')
    # Broad white breastplate/scapular armor: filled curved surfaces, not outlines.
    for side,label in ((-1,'Breastplate'),(1,'ScapularBack')):
        for sign,lr in ((1,'L'),(-1,'R')):
            outer=[];dirs=[];rows=9;cols=8
            for r in range(rows):
                v=r/(rows-1);coords=[]
                for c in range(cols):
                    u=c/(cols-1);width=.145+.035*math.sin(v*math.pi*.8)
                    x=sign*(.005+u*width)
                    bottom=shoulder-.245+.045*u;top=shoulder-.018-.055*u
                    z=bottom*(1-v)+top*v;coords.append((x,z))
                for c,hit in enumerate(torso_row(coords,side)):
                    u=c/(cols-1)
                    lift=.017+.006*math.sin(u*math.pi)*math.sin(v*math.pi)
                    outer.append(hit+Vector((0,side*lift,0)));dirs.append((0,side,0))
            patch(label+'_'+lr,outer,dirs,rows,cols,torso_tree)
        for index,(upper,lower,width) in enumerate(((.258,.321,.095),(.335,.394,.078),(.408,.457,.067))):
            outer=[];dirs=[]
            for r in range(4):
                v=r/3;coords=[]
                for c in range(9):
                    u=c/8;x=(2*u-1)*width*(.82+.18*v)
                    z=shoulder-lower+(lower-upper)*v+.009*abs(2*u-1)
                    coords.append((x,z))
                for hit in torso_row(coords,side):outer.append(hit+Vector((0,side*.016,0)));dirs.append((0,side,0))
            patch(('Abdomen' if side==-1 else 'Lumbar')+str(index),outer,dirs,4,9,torso_tree)
    # Pelvic guard ends above the measured central/side crotch gap, never bridges anatomy.
    lower=pelvis.z-.16;upper=pelvis.z-.035
    for step in range(81):
        z=lower+step*.001
        if z>upper-.035:raise RuntimeError('No pelvic boundary within planned height')
        if all(surface((x,0,z),(0,-1,0),torso_tree)[0] is not None for x in (-.035,-.0175,0,.0175,.035)):
            lower=z;break
    else:raise RuntimeError('No measured pelvic boundary within80mm')
    outer=[];dirs=[]
    for r in range(5):
        v=r/4;coords=[]
        for c in range(7):
            u=c/6;x=(2*u-1)*(.035+.03*v);z=lower+(upper-lower)*v
            coords.append((x,z))
        for hit in torso_row(coords,-1):outer.append(hit+Vector((0,-.016,0)));dirs.append((0,-1,0))
    patch('PelvicGuard',outer,dirs,5,7,torso_tree)
    def limb_plate(name,a,b,t0,t1,theta0,theta1,fit_tree,bulge=.005,rows=7,cols=9,radius_limit=.2):
        a,b=Vector(a),Vector(b);axis=(b-a).normalized();front=Vector((0,-1,0));front=(front-axis*front.dot(axis)).normalized();right=axis.cross(front).normalized()
        outer=[];dirs=[]
        for r in range(rows):
            v=r/(rows-1)
            for c in range(cols):
                u=c/(cols-1)
                # Shaped oblique ends and a tapered centre ridge replace tube cuffs.
                t=t0+(t1-t0)*v+.025*abs(2*u-1)*math.cos(v*math.pi)
                angle=theta0+(theta1-theta0)*u
                direction=right*math.sin(angle)+front*math.cos(angle);center=a+(b-a)*t
                hit,normal=surface(center,direction,fit_tree)
                if hit is None or not .005<(hit-center).dot(direction)<radius_limit or abs((hit-center).dot(axis))>.003 or normal.dot(direction)<.05:
                    raise RuntimeError('Limb region radial fit invalid: '+name+repr((t,angle)))
                lift=.016+bulge*math.sin(u*math.pi)*math.sin(v*math.pi)
                outer.append(hit+direction*lift);dirs.append(direction)
        return patch(name,outer,dirs,rows,cols,fit_tree)
    for suffix in ('l','r'):
        upperarm,elbow,wrist=bone('upperarm_'+suffix),bone('lowerarm_'+suffix),bone('hand_'+suffix)
        hip,knee,ankle=bone('thigh_'+suffix),bone('calf_'+suffix),bone('foot_'+suffix)
        axis=(elbow-upperarm).normalized();front=Vector((0,-1,0));front=(front-axis*front.dot(axis)).normalized();right=axis.cross(front).normalized()
        outward=Vector((1 if upperarm.x>0 else -1,0,0));outer_angle=math.atan2(outward.dot(right),outward.dot(front))
        arm_tree=region_trees['arm_'+suffix];leg_tree=region_trees['leg_'+suffix];foot_tree=region_trees['foot_'+suffix]
        limb_plate('DeltoidCap_'+suffix,upperarm,elbow,.015,.38,outer_angle-1.25,outer_angle+1.25,arm_tree,.014)
        limb_plate('UpperArmPlate_'+suffix,upperarm,elbow,.44,.81,-1.0,1.0,arm_tree,.004,5,7)
        limb_plate('Bracer_'+suffix,elbow,wrist,.17,.88,-1.2,1.2,arm_tree,.009)
        leg_axis=(knee-hip).normalized();leg_front=Vector((0,-1,0));leg_front=(leg_front-leg_axis*leg_front.dot(leg_axis)).normalized();leg_right=leg_axis.cross(leg_front).normalized()
        outward=Vector((1 if hip.x>0 else -1,0,0));leg_angle=math.atan2(outward.dot(leg_right),outward.dot(leg_front))
        limb_plate('ThighFront_'+suffix,hip,knee,.12,.82,-1.0,1.0,leg_tree,.009)
        limb_plate('ThighOuter_'+suffix,hip,knee,.15,.77,leg_angle-.42,leg_angle+.42,leg_tree,.005,7,5)
        limb_plate('Kneecap_'+suffix,knee,ankle,-.045,.17,-.92,.92,leg_tree,.013,5,7)
        limb_plate('ShinFront_'+suffix,knee,ankle,.25,.85,-1.08,1.08,leg_tree,.009)
        limb_plate('CalfBack_'+suffix,knee,ankle,.30,.78,math.pi-.72,math.pi+.72,leg_tree,.004,6,7)
        # Measured foot perimeter produces a practical toe/heel/ankle silhouette.
        outer=[];dirs=[];rows=5;cols=16
        for r,z in enumerate((.022,.045,.080,.120,.158)):
            section=[p for p in points if (1 if ankle.x>0 else -1)*p.x>.025 and abs(p.x-ankle.x)<.085 and abs(p.z-z)<.012]
            if not section:raise RuntimeError('Missing measured foot section')
            cy=(min(p.y for p in section)+max(p.y for p in section))/2
            for c in range(cols):
                angle=2*math.pi*c/cols;d=Vector((math.sin(angle),-math.cos(angle),0));center=Vector((ankle.x,cy,z))
                hit,normal=surface(center,d,foot_tree)
                if hit is None or not .005<(hit-center).dot(d)<.25 or normal.dot(d)<.05 or (1 if ankle.x>0 else -1)*hit.x<=.025:raise RuntimeError('Foot anatomical radial fit invalid')
                outer.append(hit+d*.014);dirs.append(d)
        patch('Boot_'+suffix,outer,dirs,rows,cols,foot_tree,wrap=True)
    # One continuous helmet cage with an integral optic seat: no loose visor or open crown.
    head=bone('head');hp=[p for p in points if p.z>head.z]
    cx=(min(p.x for p in hp)+max(p.x for p in hp))/2;cy=(min(p.y for p in hp)+max(p.y for p in hp))/2
    w=(max(p.x for p in hp)-min(p.x for p in hp))/2+.018
    depth=(max(p.y for p in hp)-min(p.y for p in hp))/2+.022
    bottom=head.z-.018;top=max(p.z for p in hp)+.020;height=top-bottom
    profiles=[(0,.64,.76),(.12,.78,.91),(.32,.96,1.0),(.46,1.02,1.01),(.50,1.03,1.04),(.64,1.03,1.05),(.69,1.02,1.06),(.80,.98,.98),(.91,.81,.80),(.98,.46,.43)]
    verts=[];cols=24
    for t,sx,sy in profiles:
        z=bottom+height*t
        for c in range(cols):
            angle=2*math.pi*c/cols;d=Vector((math.sin(angle),-math.cos(angle),0));center=Vector((cx,cy,z))
            radius=1/math.sqrt((math.sin(angle)/(w*sx))**2+(math.cos(angle)/(depth*sy))**2)
            hit,normal=surface(center,d,head_tree)
            if hit is not None and abs(hit.x-cx)<.16 and hit.z>head.z-.02:
                radius=max(radius,(hit-center).dot(d)+.012)
            verts.append(center+d*radius)
    faces=[];face_materials=[]
    for r in range(len(profiles)-1):
        for c in range(cols):
            a=r*cols+c;b=r*cols+(c+1)%cols;d=(r+1)*cols+c;e=(r+1)*cols+(c+1)%cols
            angle=2*math.pi*(c+.5)/cols;front_angle=min(angle,2*math.pi-angle)
            role=2 if r==4 and front_angle<1.18 else (1 if r in (3,5) and front_angle<1.4 else 0)
            faces.extend([(a,b,e),(a,e,d)]);face_materials.extend([role,role])
    for r,center_z in ((0,bottom-.002),(len(profiles)-1,top)):
        center_index=len(verts);verts.append(Vector((cx,cy,center_z)))
        for c in range(cols):faces.append((r*cols+c,r*cols+(c+1)%cols,center_index));face_materials.append(0)
    helmet=mesh_object('Helmet_CrownBrowCheekChin',verts,faces,white,.0012)
    helmet.data.materials.append(dark);helmet.data.materials.append(cyan)
    for poly,role in zip(helmet.data.polygons,face_materials):poly.material_index=role
    # Sample the exposed helmet cage interiors too; the lower cap is its neck seat.
    helmet_checks=0;helmet_violations=[]
    for face in faces[:(len(profiles)-1)*cols*2]:
        for weights in ((1/3,1/3,1/3),(.5,.5,0),(0,.5,.5),(.5,0,.5)):
            p=sum((verts[i]*weight for i,weight in zip(face,weights)),Vector())
            d=Vector((p.x-cx,p.y-cy,0)).normalized();hit,normal=surface(p,d,head_tree);helmet_checks+=1
            if hit is not None and (p-hit).dot(d)<.004:helmet_violations.append((p-hit).dot(d))
            nearest,normal,_,_=body_bvh.find_nearest(p)
            if nearest is not None and (p-nearest).dot(normal)<.004:helmet_violations.append((p-nearest).dot(normal))
    fit_log.append(dict(part=helmet.name,face_samples=helmet_checks,final_violations=len(helmet_violations)))
    if helmet_violations:clearance_failures.append(dict(part=helmet.name,reason='Helmet face clearance',count=len(helmet_violations),minimum_m=min(helmet_violations)))
    # Faceted side housings are fitted to the actual helmet cage, not loose discs.
    bm=bmesh.new();bm.from_mesh(helmet.data);helmet_bvh=BVHTree.FromBMesh(bm);bm.free()
    for sign,lr in ((1,'L'),(-1,'R')):
        center=Vector((cx,cy,bottom+height*.55));verts=[];faces=[];count=12
        for layer in range(2):
            for c in range(count):
                angle=2*math.pi*c/count;y=center.y+math.cos(angle)*.039;z=center.z+math.sin(angle)*.043
                hit,normal=surface((cx,y,z),(sign,0,0),helmet_bvh)
                if hit is None:raise RuntimeError('Ear housing misses helmet')
                verts.append(hit+Vector((sign*(-.003 if layer==0 else .014),0,0)))
        faces.extend([tuple(range(count-1,-1,-1)),tuple(count+c for c in range(count))])
        faces.extend((c,(c+1)%count,(c+1)%count+count,c+count) for c in range(count))
        mesh_object('Helmet_EarHousing_'+lr,verts,faces,white,.001)
    names=[body.name]+[ob.name for ob in parts]
    scene['stage']='Revision04 reference-shaped primary armor; not approved or rigged';scene['no_new_rig']=True
    bpy.context.view_layer.update();dg=bpy.context.evaluated_depsgraph_get();armor_tris=0
    for ob in parts:
        ev=ob.evaluated_get(dg);geo=ev.to_mesh()
        try:geo.calc_loop_triangles();armor_tris+=len(geo.loop_triangles)
        finally:ev.to_mesh_clear()
    budget=runpy.run_path(str(ROOT/'.claude/skills/blender-mcp/scripts/asset_report.py'))['main'](targets=names,tri_budget=130000)
    assembly=runpy.run_path(str(ROOT/'.claude/skills/blender-mcp/scripts/assembly_check.py'))['main'](targets=names)
    qa=runpy.run_path('C:/Users/nicho/.agents/skills/blender-asset-qa/scripts/qa_check.py')['run'](objects=names,profile='game')
    technical=armor_tris<=25000 and budget['totals']['errors']==0 and budget['totals']['tris']<=130000 and assembly.get('ok',False) and qa['pass'] and not clearance_failures
    blend=OUT/'Lancer_Armor_Revision04.blend'
    assert bpy.ops.wm.save_as_mainfile(filepath=str(blend),check_existing=False)=={'FINISHED'} and sha(BASE)==BASE_SHA
    report=dict(stage='PRIMARY FORM REVIEW ONLY',technical_checks_passed=technical,blend_sha256=sha(blend),reference_sha256=BASE_SHA,objects=names,original_armor_evaluated_triangles=armor_tris,model_material_roles=[white.name,dark.name,cyan.name],budget=budget,assembly=assembly,qa=qa,clearance_failures=clearance_failures,limits=['Complete owned body unchanged; no new anatomy/rig/weights/native assets','Basic individual face UV charts, not final texture layout','Actual front/back/side/top/beauty visual and independent review still required'])
    (OUT/'blockout-qa.json').write_text(json.dumps(report,indent=2)+'\n')
    (OUT/'surface-fit.json').write_text(json.dumps(dict(method='Anatomically associated regional BVH fitting; separate full-body collision BVH; face centroid/edge midpoint clearance',regions=region_summary,pelvis_lower_z=lower,parts=fit_log,failures=clearance_failures),indent=2)+'\n')
    if not technical:print('LANCER_BLOCKOUT_QA_FAILED diagnostic blend only; no acceptance')
    print('LANCER_BLOCKOUT_BUILD_OK '+str(blend))

if __name__=='__main__':main()

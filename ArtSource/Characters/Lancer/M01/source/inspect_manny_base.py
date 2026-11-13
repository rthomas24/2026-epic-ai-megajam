"""Import existing reference only; no new anatomy, armor, rig, pose or render."""
import hashlib
import json
import os
from pathlib import Path
import runpy
import bpy
from mathutils import Vector


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    cfg=json.loads(Path(os.environ['TD_LANCER_BASE_CONTEXT']).read_text(encoding='utf-8-sig'))
    owned=Path(__file__).resolve().parent.parent
    source=owned/'source/base/UE5_Manny_Simple_readonly_v02.fbx'
    expected=json.loads(source.with_suffix('.export.json').read_text())
    output=owned/'source/base/Lancer_Manny_Reference_v01.blend'
    inventory=output.with_suffix('.inventory.json')
    assert cfg['owned_root']==str(owned) and not output.exists() and not inventory.exists()
    assert sha(source)==expected['fbx']['sha256']
    assert bpy.app.background and not bpy.data.filepath and bpy.app.version[:2]==(5,1)
    env=runpy.run_path('C:/Users/nicho/.agents/skills/blender-core/scripts/env_report.py')['run']()
    assert set(bpy.data.objects.keys()) <= {'Cube','Camera','Light'}
    for ob in list(bpy.data.objects): bpy.data.objects.remove(ob,do_unlink=True)
    scene=bpy.context.scene
    scene.name='Lancer_M01_ReferenceFoundation'
    scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1.0
    scene.unit_settings.length_unit='METERS'
    scene.render.threads_mode='FIXED';scene.render.threads=2
    result=bpy.ops.import_scene.fbx(filepath=str(source),global_scale=1.0,use_anim=False,
                                  automatic_bone_orientation=False,ignore_leaf_bones=False,use_image_search=False)
    assert result=={'FINISHED'}
    bpy.context.view_layer.update()
    objects=list(scene.objects); meshes=[o for o in objects if o.type=='MESH']
    rigs=[o for o in objects if o.type=='ARMATURE']
    assert meshes and len(rigs)==1 and len(bpy.data.actions)==0
    rig=rigs[0];names={b.name for b in rig.data.bones}
    expected_names={b['name'] for b in expected['bones']}
    assert names==expected_names and len(names)==89, repr(dict(missing=sorted(expected_names-names),extra=sorted(names-expected_names)))
    reference=bpy.data.collections.new('REF_Manny_NotForExport');scene.collection.children.link(reference)
    reference['export_allowed']=False;reference['purpose']='Owned mannequin reference only, not finished Lancer art'
    reference.hide_render=True
    for ob in objects:
        for collection in list(ob.users_collection):collection.objects.unlink(ob)
        reference.objects.link(ob)
    points=[o.matrix_world @ v.co for o in meshes for v in o.data.vertices]
    low=[min(p[i] for p in points) for i in range(3)];high=[max(p[i] for p in points) for i in range(3)]
    size=[high[i]-low[i] for i in range(3)]
    assert 1.70<size[2]<1.90, 'Unexpected world-space height; do not rescale blindly'
    mesh_stats=[]
    for ob in meshes:
        ob.data.calc_loop_triangles()
        mesh_stats.append(dict(name=ob.name,vertices=len(ob.data.vertices),triangles=len(ob.data.loop_triangles),
                               materials=[m.name if m else None for m in ob.data.materials],
                               scale=list(ob.scale),rotation=list(ob.rotation_euler),
                               uv_layers=list(ob.data.uv_layers.keys()),modifiers=[m.type for m in ob.modifiers]))
    material_names=sorted({m.name for o in meshes for m in o.data.materials if m})
    assert len(material_names)==2, 'Unexpected material role count'
    def head(name): return rig.matrix_world @ rig.data.bones[name].head_local
    toes=((head('ball_l')-head('foot_l'))+(head('ball_r')-head('foot_r')))*0.5
    assert sha(source)==expected['fbx']['sha256']
    saved=bpy.ops.wm.save_as_mainfile(filepath=str(output),check_existing=False)
    assert saved=={'FINISHED'} and output.stat().st_size>0
    report=dict(stage='Imported reference foundation only; no original character armor or visual approval',
                environment=env,source_fbx_sha256=sha(source),blend_sha256=sha(output),
                world_bounds_m=dict(min=low,max=high,size=size),mesh_stats=mesh_stats,
                total_vertices=sum(o['vertices'] for o in mesh_stats),
                total_triangles=sum(o['triangles'] for o in mesh_stats),
                bone_count=len(names),bone_names=sorted(names),material_names=material_names,
                toe_direction_world=list(toes),forward_axis_limit='Toe vector recorded; no axis correction or retargeting performed',
                reference_collection=reference.name,reference_render_hidden=True,export_allowed=False,
                rig_scale=list(rig.scale),rig_rotation=list(rig.rotation_euler),new_rig_or_weights=False,
                topology_limit='Inventory only; full armor QA and visual checks remain a later milestone')
    with inventory.open('x',encoding='utf-8') as f:json.dump(report,f,indent=2);f.write('\n')
    print('LANCER_BASE_INVENTORY_OK '+json.dumps(dict(blend=str(output),triangles=report['total_triangles'],bones=len(names))))


if __name__=='__main__':main()

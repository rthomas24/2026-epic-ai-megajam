import bpy,sys,math; sys.path.insert(0,'/tmp/rwork'); from rlib import *
sc=reset(); sc.render.resolution_x=900; sc.render.resolution_y=600
cols={'Rift':(0.78,0.78,0.80),'Lancer':(0.72,0.74,0.77),'Bastion':(0.42,0.43,0.45)}
for name,c in cols.items():
    for o in [x for x in sc.objects if x.type=='MESH']: bpy.data.objects.remove(o)
    m=bpy.data.materials.new(name); m.use_nodes=True; nt=m.node_tree; N=nt.nodes; L=nt.links; p=N['Principled BSDF']
    p.inputs['Base Color'].default_value=(*c,1); p.inputs['Metallic'].default_value=0.3 if name!='Rift' else 0.0
    tc=N.new('ShaderNodeTexCoord'); mp=N.new('ShaderNodeMapping'); mp.inputs['Scale'].default_value=(1,1,1); L.new(tc.outputs['UV'],mp.inputs[0])
    tn=N.new('ShaderNodeTexImage'); tn.image=img(f'/tmp/rwork/detail/T_{name}_Detail_N.png','Non-Color'); L.new(mp.outputs[0],tn.inputs[0])
    s2=N.new('ShaderNodeSeparateColor'); L.new(tn.outputs[0],s2.inputs[0]); inv=N.new('ShaderNodeMath'); inv.operation='SUBTRACT'; inv.inputs[0].default_value=1; L.new(s2.outputs[1],inv.inputs[1])
    cc=N.new('ShaderNodeCombineColor'); L.new(s2.outputs[0],cc.inputs[0]); L.new(inv.outputs[0],cc.inputs[1]); L.new(s2.outputs[2],cc.inputs[2])
    nm=N.new('ShaderNodeNormalMap'); L.new(cc.outputs[0],nm.inputs['Color']); L.new(nm.outputs[0],p.inputs['Normal'])
    tr=N.new('ShaderNodeTexImage'); tr.image=img(f'/tmp/rwork/detail/T_{name}_Detail_RM.png','Non-Color'); L.new(mp.outputs[0],tr.inputs[0])
    s3=N.new('ShaderNodeSeparateColor'); L.new(tr.outputs[0],s3.inputs[0])
    mr=N.new('ShaderNodeMapRange'); mr.inputs[3].default_value=0.2; mr.inputs[4].default_value=0.6; L.new(s3.outputs[1],mr.inputs[0]); L.new(mr.outputs[0],p.inputs['Roughness'])
    mix=N.new('ShaderNodeMixRGB'); mix.blend_type='MULTIPLY'; mix.inputs[0].default_value=1; mix.inputs[1].default_value=(*c,1); L.new(s3.outputs[0],mix.inputs[2]); L.new(mix.outputs[0],p.inputs['Base Color'])
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.8,location=(-0.95,0,0.8),segments=96,ring_count=48); sp=bpy.context.object; bpy.ops.object.shade_smooth(); sp.data.materials.append(m)
    bpy.ops.mesh.primitive_plane_add(size=2.0,location=(1.0,0,0.8),rotation=(math.radians(90),0,math.radians(-25))); pl=bpy.context.object; pl.data.materials.append(m)
    # sphere UV spans 2*pi*r ~5 m: scale sphere UV x by 5 to keep ~1 m tile
    for l in sp.data.uv_layers.active.data: l.uv=(l.uv[0]*5,l.uv[1]*2.5)
    for l in pl.data.uv_layers.active.data: l.uv=(l.uv[0]*2,l.uv[1]*2)
    cam(sc,(0,-3.0,1.05),(0,0,0.8),lens=28)
    render(sc,f'/tmp/rwork/sh/det_{name}.png')

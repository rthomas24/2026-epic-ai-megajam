import bpy,sys,os,shutil
T='/tmp/rwork/tex/rift'; OUT='/tmp/rwork/out'
bpy.ops.wm.open_mainfile(filepath='/tmp/rwork/b2.blend')
sc=bpy.context.scene
for o in list(sc.objects):
    if o.name!='SM_Rift': bpy.data.objects.remove(o)
o=bpy.data.objects['SM_Rift']; me=o.data
for mm in list(bpy.data.meshes):
    if mm!=me and mm.name=='SM_Rift': mm.name='SM_Rift_old'
me.name='SM_Rift'
for a in ('zone','src_face'):
    if a in me.attributes: me.attributes.remove(me.attributes[a])
def mk(name,ngl=False,estr=1.0):
    old=bpy.data.materials.get(name)
    if old: old.name=name+'_old'
    m=bpy.data.materials.new(name); m.use_nodes=True; nt=m.node_tree; N=nt.nodes; L=nt.links; p=N['Principled BSDF']
    def im(f,cs):
        t=N.new('ShaderNodeTexImage'); t.image=bpy.data.images.load(f); t.image.colorspace_settings.name=cs; return t
    bc=im(f'{T}/T_Rift_BC.png','sRGB'); L.new(bc.outputs[0],p.inputs['Base Color'])
    orm=im(f'{T}/T_Rift_ORM.png','Non-Color'); sp=N.new('ShaderNodeSeparateColor'); L.new(orm.outputs[0],sp.inputs[0])
    L.new(sp.outputs[1],p.inputs['Roughness']); L.new(sp.outputs[2],p.inputs['Metallic'])
    nm=im('/tmp/rwork/tex/T_Rift_N_GL.png' if ngl else f'{T}/T_Rift_N.png','Non-Color'); nmap=N.new('ShaderNodeNormalMap'); L.new(nm.outputs[0],nmap.inputs['Color']); L.new(nmap.outputs[0],p.inputs['Normal'])
    e=im(f'{T}/T_Rift_E.png','sRGB'); L.new(e.outputs[0],p.inputs['Emission Color']); p.inputs['Emission Strength'].default_value=estr
    return m
mat=mk('M_Rift',ngl=False,estr=1.0)
me.materials.clear(); me.materials.append(mat)
socks={'Muzzle_0':(2.52,0.37,-0.6),'Muzzle_1':(2.52,-0.37,-0.6),'Engine':(-2.52,0,-0.34),'Engine_0':(-2.52,0.64,-0.34),'Engine_1':(-2.52,-0.64,-0.34),'Portal':(-1.7,0,0.55),'Cockpit':(0.6,0,0),'WingTip_L':(2.38,1.76,-1.09),'WingTip_R':(2.38,-1.76,-1.09)}
for k,v in socks.items():
    e=bpy.data.objects.new('SOCKET_'+k,None); e.empty_display_size=0.1; sc.collection.objects.link(e); e.parent=o; e.location=v
sc.unit_settings.system='METRIC'; sc.unit_settings.scale_length=1.0
bpy.ops.wm.save_as_mainfile(filepath=f'{OUT}/SM_Rift_r2_work.blend')
bpy.ops.object.select_all(action='SELECT')
bpy.ops.export_scene.fbx(filepath=f'{OUT}/SM_Rift.fbx',use_selection=False,object_types={'MESH','EMPTY'},apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',
    axis_forward='-Z',axis_up='Y',mesh_smooth_type='FACE',use_tspace=True,use_mesh_modifiers=True,add_leaf_bones=False,bake_anim=False,path_mode='STRIP',embed_textures=False,use_triangles=True)
# GLB: centimetre values, Z-up, OpenGL normal embedded
me.materials[0]=mk('M_Rift',ngl=True,estr=1.0)
o.scale=(100,100,100)
bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active=o
bpy.ops.export_scene.gltf(filepath=f'{OUT}/SM_Rift.glb',export_format='GLB',export_yup=False,export_tangents=True,export_normals=True,export_texcoords=True,export_materials='EXPORT',export_image_format='AUTO',use_selection=False,export_apply=False)
print('exported')

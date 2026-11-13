import bpy
bpy.ops.wm.open_mainfile(filepath='/tmp/rwork/b2.blend')
sc=bpy.context.scene; sc.render.engine='CYCLES'; sc.cycles.device='CPU'; sc.cycles.samples=48
o=bpy.data.objects['SM_Rift']
for x in list(sc.objects):
    if x!=o: bpy.data.objects.remove(x)
im=bpy.data.images.new('AO',2048,2048,alpha=False); im.colorspace_settings.name='Non-Color'
m=o.data.materials[0]; m.use_nodes=True; n=m.node_tree.nodes.new('ShaderNodeTexImage'); n.image=im; m.node_tree.nodes.active=n
bpy.context.view_layer.objects.active=o; o.select_set(True)
sc.world=sc.world or bpy.data.worlds.new('W')
sc.render.bake.margin=6
sc.cycles.ao_bounces_render=0
bpy.ops.object.bake(type='AO')
im.filepath_raw='/tmp/rwork/tex/ao_bake.png'; im.file_format='PNG'; im.save()
print('baked')

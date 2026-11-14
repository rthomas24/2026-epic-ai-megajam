import bpy, sys
args=sys.argv[sys.argv.index('--')+1:]; root, dist, out = args[0], float(args[1]), args[2]
sc=bpy.context.scene; sc.render.engine='CYCLES'; sc.cycles.samples=96; sc.cycles.device='CPU'
for o in bpy.data.objects:
    o.hide_render = (o.name!=root)
o=bpy.data.objects[root]; o.hide_set(False); o.hide_viewport=False
if not sc.world: sc.world=bpy.data.worlds.new("W")
sc.world.light_settings.distance=dist
img=bpy.data.images.new("AO_bake",2048,2048,alpha=False,float_buffer=True); img.colorspace_settings.name='Non-Color'
mat=o.data.materials[0]; nt=mat.node_tree; n=nt.nodes.new('ShaderNodeTexImage'); n.image=img; nt.nodes.active=n
for n2 in nt.nodes: n2.select=False
n.select=True
bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active=o
sc.render.bake.margin=16; sc.render.bake.use_selected_to_active=False
bpy.ops.object.bake(type='AO')
img.filepath_raw=out; img.file_format='PNG'; img.save()
print("[ao] baked", root, dist, out)

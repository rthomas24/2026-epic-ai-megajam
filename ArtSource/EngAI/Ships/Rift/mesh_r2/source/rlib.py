import bpy, math, mathutils, os
from mathutils import Vector
def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    sc=bpy.context.scene
    for e in ('BLENDER_EEVEE_NEXT','BLENDER_EEVEE'):
        try: sc.render.engine=e; break
        except Exception: pass
    sc.render.resolution_x=800; sc.render.resolution_y=800
    sc.render.film_transparent=False
    sc.view_settings.view_transform='AgX' if 'AgX' in [i.identifier for i in sc.view_settings.bl_rna.properties['view_transform'].enum_items] else 'Filmic'
    try: sc.eevee.taa_render_samples=32
    except: pass
    w=bpy.data.worlds.new('W'); sc.world=w; w.use_nodes=True
    bg=w.node_tree.nodes['Background']; bg.inputs[0].default_value=(0.32,0.33,0.36,1); bg.inputs[1].default_value=0.9
    def sun(name,rot,en,col=(1,1,1)):
        l=bpy.data.lights.new(name,'SUN'); l.energy=en; l.color=col; o=bpy.data.objects.new(name,l); o.rotation_euler=[math.radians(a) for a in rot]; sc.collection.objects.link(o); return o
    sun('key',(50,10,35),3.2); sun('fill',(60,0,-140),1.2,(0.85,0.9,1)); sun('rim',(-40,0,160),1.6)
    return sc
def img(path, cs='sRGB'):
    i=bpy.data.images.load(path, check_existing=False); i.colorspace_settings.name=cs; return i
def rift_mat(name, tdir, prefix='T_Rift', estr=3.0, clay=False):
    m=bpy.data.materials.new(name); m.use_nodes=True; nt=m.node_tree; N=nt.nodes; L=nt.links
    p=N['Principled BSDF']
    if clay:
        p.inputs['Base Color'].default_value=(0.62,0.62,0.64,1); p.inputs['Roughness'].default_value=0.45; return m
    bc=N.new('ShaderNodeTexImage'); bc.image=img(f'{tdir}/{prefix}_BC.png')
    L.new(bc.outputs[0],p.inputs['Base Color'])
    orm=N.new('ShaderNodeTexImage'); orm.image=img(f'{tdir}/{prefix}_ORM.png','Non-Color')
    sp=N.new('ShaderNodeSeparateColor'); L.new(orm.outputs[0],sp.inputs[0])
    L.new(sp.outputs[1],p.inputs['Roughness']); L.new(sp.outputs[2],p.inputs['Metallic'])
    nm=N.new('ShaderNodeTexImage'); nm.image=img(f'{tdir}/{prefix}_N.png','Non-Color')
    s2=N.new('ShaderNodeSeparateColor'); L.new(nm.outputs[0],s2.inputs[0])
    inv=N.new('ShaderNodeMath'); inv.operation='SUBTRACT'; inv.inputs[0].default_value=1; L.new(s2.outputs[1],inv.inputs[1])
    cc=N.new('ShaderNodeCombineColor'); L.new(s2.outputs[0],cc.inputs[0]); L.new(inv.outputs[0],cc.inputs[1]); L.new(s2.outputs[2],cc.inputs[2])
    nmap=N.new('ShaderNodeNormalMap'); L.new(cc.outputs[0],nmap.inputs['Color']); L.new(nmap.outputs[0],p.inputs['Normal'])
    e=N.new('ShaderNodeTexImage'); e.image=img(f'{tdir}/{prefix}_E.png')
    L.new(e.outputs[0],p.inputs['Emission Color']); p.inputs['Emission Strength'].default_value=estr
    return m
def cam(sc, loc, target, ortho=None, lens=50, name='cam'):
    c=bpy.data.cameras.new(name); o=bpy.data.objects.new(name,c); sc.collection.objects.link(o)
    o.location=Vector(loc); d=Vector(target)-o.location; o.rotation_euler=d.to_track_quat('-Z','Y').to_euler()
    if ortho: c.type='ORTHO'; c.ortho_scale=ortho
    else: c.lens=lens
    c.clip_start=0.01; c.clip_end=100
    sc.camera=o; return o
def render(sc, path):
    sc.render.filepath=path; bpy.ops.render.render(write_still=True)

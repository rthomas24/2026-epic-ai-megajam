"""Triad Drift ENG-37: original procedural VFX meshes (no third-party sources), written as FBX for Unreal.
  SM_TD_VFX_Flame   a flame / plume body of revolution along +X: nozzle at x=0, tip at x=100 cm, radius 50 cm. V = 0..1 along it
  SM_TD_VFX_Bolt    a spindle along X centred on the origin, 100 cm long, radius 50 cm. V = 0..1 along it
  SM_TD_VFX_Disc    a flat quad in the YZ plane (normal +X), 100 x 100 cm, UV 0..1: rings and ripples that lie on a surface
  SM_TD_VFX_Shard_A/B/C  faceted debris chunks, about 100 cm across
usage: "<Blender 5.2>/blender.exe" --background --factory-startup --python gen_meshes.py -- <out_dir>"""
import bpy, bmesh, math, random, sys, os

out = sys.argv[sys.argv.index('--') + 1]
os.makedirs(out, exist_ok=True)


def new_obj(name, bm, smooth):
    me = bpy.data.meshes.new(name)
    bm.normal_update()
    bm.to_mesh(me)
    bm.free()
    for p in me.polygons:
        p.use_smooth = smooth
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    return ob


def lathe(name, profile, seg=28):
    """profile: list of (x, radius, v). Body of revolution around X with U around, V along."""
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new('UVMap')
    rings = []
    for x, r, v in profile:
        rings.append([bm.verts.new((x, r * math.cos(2 * math.pi * i / seg), r * math.sin(2 * math.pi * i / seg))) for i in range(seg)])
    for j in range(len(profile) - 1):
        for i in range(seg):
            a, b = rings[j][i], rings[j][(i + 1) % seg]
            c, d = rings[j + 1][(i + 1) % seg], rings[j + 1][i]
            f = bm.faces.new((a, b, c, d))
            us = (i / seg, (i + 1) / seg, (i + 1) / seg, i / seg)
            vs = (profile[j][2], profile[j][2], profile[j + 1][2], profile[j + 1][2])
            for loop, u, v in zip(f.loops, us, vs):
                loop[uv].uv = (u, v)
    return new_obj(name, bm, True)


n = 20
flame = [(t, 0.5 * (0.72 + 0.28 * math.sin(min(t / 0.18, 1.0) * math.pi / 2)) * (1 - t) ** 0.85 + 0.002, t) for t in [i / n for i in range(n + 1)]]
lathe('SM_TD_VFX_Flame', flame)
bolt = [(t - 0.5, 0.5 * max(math.sin(math.pi * t), 0.0) ** 0.75 + 0.002, t) for t in [i / n for i in range(n + 1)]]
lathe('SM_TD_VFX_Bolt', bolt)

bm = bmesh.new()
uv = bm.loops.layers.uv.new('UVMap')
vs = [bm.verts.new((0, y, z)) for y, z in ((-0.5, -0.5), (0.5, -0.5), (0.5, 0.5), (-0.5, 0.5))]
f = bm.faces.new(vs)
for loop, t in zip(f.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
    loop[uv].uv = t
new_obj('SM_TD_VFX_Disc', bm, False)

for k, (name, sq) in enumerate((('SM_TD_VFX_Shard_A', (1.0, 0.55, 0.30)), ('SM_TD_VFX_Shard_B', (1.0, 0.8, 0.18)), ('SM_TD_VFX_Shard_C', (0.8, 0.7, 0.6)))):
    rnd = random.Random(1337 + k)
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=1, radius=0.5, calc_uvs=True)
    for v in bm.verts:
        s = rnd.uniform(0.55, 1.25)
        v.co = (v.co.x * s * sq[0], v.co.y * s * sq[1], v.co.z * s * sq[2])
    new_obj(name, bm, False)

for ob in list(bpy.context.scene.collection.objects):
    if not ob.name.startswith('SM_TD_VFX_'):
        continue
    bpy.ops.object.select_all(action='DESELECT')
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    bpy.ops.export_scene.fbx(filepath=os.path.join(out, ob.name + '.fbx'), use_selection=True, object_types={'MESH'},
                             global_scale=1.0, apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
                             axis_forward='-Z', axis_up='Y', use_space_transform=True, bake_space_transform=False,
                             mesh_smooth_type='FACE', use_triangles=True, add_leaf_bones=False, bake_anim=False, path_mode='AUTO')
    print('wrote', ob.name)

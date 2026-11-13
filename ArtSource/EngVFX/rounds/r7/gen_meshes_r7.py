"""Triad Drift ENG-38 round 7: original procedural VFX meshes for the atmospheric entry, written as FBX for Unreal.
  SM_TD_VFX_BowArc   the bow shock as SIX plasma streamers (soft tubes), not a shell: they leave one point 4.2 m ahead of
                     the nose and sweep out and back along a parabola (radius 7 m at the tail, 15 m long), in three
                     planes through the ship's axis (level, and 38 degrees up and down), left and right. Seen from the
                     chase camera they are glowing arcs sweeping back past the wings. Real size, in ship-local axes
                     (+X = nose, origin = the ship's origin). V = 0 at the apex .. 1 at the tail, U round the tube.
usage: "<Blender 5.2>/blender.exe" --background --factory-startup --python gen_meshes_r7.py -- <out_dir>"""
import bpy, bmesh, math, sys, os
from mathutils import Vector

out = sys.argv[sys.argv.index('--') + 1]
os.makedirs(out, exist_ok=True)
APEX_X, LENGTH, RADIUS = 4.2, 15.0, 7.0
SEG, SIDES = 28, 8


def centre(s, a):
    r = RADIUS * math.sqrt(s)
    return Vector((APEX_X - LENGTH * s, r * math.cos(a), r * math.sin(a)))


bm = bmesh.new()
uv = bm.loops.layers.uv.new('UVMap')
for a_deg in (0.0, 180.0, 38.0, 142.0, -38.0, -142.0):
    a = math.radians(a_deg)
    rings = []
    for i in range(SEG + 1):
        s = (i / SEG) ** 1.6                    # more rings near the apex, where the curve is tight
        s = max(s, 0.0004)
        c = centre(s, a)
        t = (centre(min(s + 0.002, 1.0), a) - centre(max(s - 0.002, 0.0002), a)).normalized()
        n1 = t.cross(Vector((math.sin(a) * 0.0 + 0.0, -math.sin(a), math.cos(a)))).normalized()   # in the arm's plane
        n2 = t.cross(n1).normalized()
        rad = 0.07 + 0.42 * s      # first build (0.16 + 0.80 s) read as a bundle of white sticks
        rings.append([bm.verts.new(c + (n1 * math.cos(2 * math.pi * k / SIDES) + n2 * math.sin(2 * math.pi * k / SIDES)) * rad) for k in range(SIDES)])
    for i in range(SEG):
        for k in range(SIDES):
            k2 = (k + 1) % SIDES
            f = bm.faces.new((rings[i][k], rings[i][k2], rings[i + 1][k2], rings[i + 1][k]))
            vs = ((k / SIDES, i / SEG), ((k + 1) / SIDES, i / SEG), ((k + 1) / SIDES, (i + 1) / SEG), (k / SIDES, (i + 1) / SEG))
            for loop, (u, v) in zip(f.loops, vs):
                loop[uv].uv = (u, v)
me = bpy.data.meshes.new('SM_TD_VFX_BowArc')
bm.normal_update()
bm.to_mesh(me)
bm.free()
for p in me.polygons:
    p.use_smooth = True
ob = bpy.data.objects.new('SM_TD_VFX_BowArc', me)
bpy.context.scene.collection.objects.link(ob)
bpy.ops.object.select_all(action='DESELECT')
ob.select_set(True)
bpy.context.view_layer.objects.active = ob
bpy.ops.export_scene.fbx(filepath=os.path.join(out, ob.name + '.fbx'), use_selection=True, object_types={'MESH'},
                         global_scale=1.0, apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
                         axis_forward='-Z', axis_up='Y', use_space_transform=True, bake_space_transform=False,
                         mesh_smooth_type='FACE', use_triangles=True, add_leaf_bones=False, bake_anim=False, path_mode='AUTO')
print('wrote', ob.name, len(me.polygons), 'polys')

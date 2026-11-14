# SPDX-License-Identifier: MIT
# Far-tier forest canopy clumps for Verdance (environment builder, Nov 14): three irregular, lumpy crown masses, each a
# few merged low-poly blobs displaced by noise, with a darker concave underside baked into vertex colour (R = light,
# 1 on the sunlit top, ~0.2 under the crown). Under 300 triangles each. Code-built, no AI mesh, no download.
# Units: Blender metres = Unreal metres (the FBX export scales to cm). Pivot at the base centre, +Z up.
#   "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup --python ArtSource/EngEnvironment/canopy/build_canopy.py
import math
import os
import random

import bmesh
import bpy
from mathutils import Vector, noise

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "export")
os.makedirs(OUT, exist_ok=True)

VARIANTS = [
    # name, seed, blobs: (x, y, z, radius) in metres
    ("SM_Scatter_Verd_CanopyStand_01", 11, [(0, 0, 14, 11), (9, 3, 12, 8), (-8, 4, 11, 8)]),
    ("SM_Scatter_Verd_CanopyStand_02", 23, [(0, 0, 15, 12), (11, -4, 12, 9), (-10, -2, 13, 9), (2, 10, 11, 7)]),
    ("SM_Scatter_Verd_CanopyStand_03", 37, [(0, 0, 17, 13), (13, 5, 14, 10), (-12, 6, 13, 9), (4, -12, 12, 8)]),
]


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def blob(bm, cx, cy, cz, r, rng):
    # A subdivided octahedron-ish ico (subdivisions 1 = 80 faces) squashed vertically, lumpy, its underside pushed up
    # (concave) so the crown reads from below.
    ret = bmesh.ops.create_icosphere(bm, subdivisions=1, radius=r)
    seed = rng.random() * 100.0
    for v in ret["verts"]:
        p = v.co.copy()
        n = noise.noise(Vector((p.x / r * 1.7 + seed, p.y / r * 1.7, p.z / r * 1.7)))
        p *= 1.0 + 0.28 * n
        p.z *= 0.62
        if p.z < 0:
            p.z *= 0.35  # flat, slightly concave underside
            p.x *= 0.9
            p.y *= 0.9
        v.co = p + Vector((cx, cy, cz))
    return ret["verts"]


def build(name, seed, blobs):
    rng = random.Random(seed)
    mesh = bpy.data.meshes.new(name)
    bm = bmesh.new()
    for (x, y, z, r) in blobs:
        blob(bm, x + rng.uniform(-1, 1), y + rng.uniform(-1, 1), z, r, rng)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    # vertex colour: light from the face normal and the height in the crown; dark, slightly bluer underside
    col = mesh.color_attributes.new("Col", "BYTE_COLOR", "CORNER")
    zs = [v.co.z for v in mesh.vertices]
    zmin, zmax = min(zs), max(zs)
    for poly in mesh.polygons:
        for li in poly.loop_indices:
            vi = mesh.loops[li].vertex_index
            h = (mesh.vertices[vi].co.z - zmin) / max(0.01, zmax - zmin)
            up = poly.normal.z
            light = max(0.15, min(1.0, 0.25 + 0.55 * max(0.0, up) + 0.4 * h))
            col.data[li].color = (light, light, light, 1.0)
    for p in mesh.polygons:
        p.use_smooth = True
    tris = sum(len(p.vertices) - 2 for p in mesh.polygons)
    return obj, tris


def main():
    report = []
    for name, seed, blobs in VARIANTS:
        reset()
        obj, tris = build(name, seed, blobs)
        assert tris <= 300, (name, tris)
        bpy.context.view_layer.objects.active = obj
        obj.select_set(True)
        path = os.path.join(OUT, name + ".fbx")
        bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_scale_options="FBX_SCALE_UNITS", axis_forward="X",
                                 axis_up="Z", mesh_smooth_type="FACE", colors_type="LINEAR", add_leaf_bones=False)
        dims = obj.dimensions
        report.append("%s: %d tris, %.1f x %.1f x %.1f m" % (name, tris, dims.x, dims.y, dims.z))
    for r in report:
        print("CANOPY:", r)


main()

# SPDX-License-Identifier: MIT
# Far-tier forest clumps for Verdance from the real PVE trees (environment builder, Nov 14, pass 4): each clump is 5-9
# trees (Kapok, Brazil nut, trumpet tree, cacao; exported by ue_export_trees.py), trunks dropped (bark slot removed: the
# clumps only show beyond ~400 m), crowns decimated so a whole clump stays under 600 triangles, merged, with a baked
# vertex light (R: bright crown tops, dark undersides) for M_TD_CanopyStand. Code-built from our own trees.
#   "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup --python ArtSource/EngEnvironment/canopy/build_tree_clumps.py
import math
import os
import random

import bpy

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "source")
OUT = os.path.join(HERE, "export")
os.makedirs(OUT, exist_ok=True)
TREES = {  # name: real height (m), from the Unreal bounds
    "Kapok": ("SM_TDVeg_Kapok_S100_11", 61.2), "Kapok70": ("SM_TDVeg_Kapok_S070_12", 48.8),
    "Brazil": ("SM_TDVeg_BrazilNut_S100_21", 39.4), "Brazil70": ("SM_TDVeg_BrazilNut_S070_22", 30.5),
    "Trumpet": ("SM_TDVeg_TrumpetTree_S100_31", 16.1), "Cacao": ("SM_TDVeg_Cacao_S100_61", 9.2),
}
CLUMPS = [  # canopy trees only (the trumpet tree and cacao are understory and never show from 300 m up); radius in crowns
    ("SM_Scatter_Verd_TreeClump_01", 5, ["Kapok", "Brazil", "Brazil70", "Brazil70", "Kapok70"], 0.42),
    ("SM_Scatter_Verd_TreeClump_02", 7, ["Brazil", "Brazil", "Brazil70", "Brazil70", "Kapok70", "Brazil", "Brazil70"], 0.95),
    ("SM_Scatter_Verd_TreeClump_03", 9, ["Kapok70", "Brazil", "Brazil", "Brazil70", "Brazil70", "Kapok70", "Brazil70", "Brazil", "Brazil70"], 0.9),
    ("SM_Scatter_Verd_TreeClump_04", 6, ["Kapok70", "Brazil", "Brazil70", "Brazil70", "Brazil", "Brazil"], 0.6),
]
MAX_TRIS = 600


def tris(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def load_crown(key):
    name, height = TREES[key]
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=os.path.join(SRC, name + ".fbx"))
    new = [o for o in bpy.data.objects if o not in before and o.type == "MESH"]
    bpy.ops.object.select_all(action="DESELECT")
    for o in new:
        o.select_set(True)
    bpy.context.view_layer.objects.active = new[0]
    if len(new) > 1:
        bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    # drop the bark (trunks and branches): keep only faces whose material is the foliage
    keep = {i for i, m in enumerate(obj.data.materials) if m and "oliage" in m.name}
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="DESELECT")
    bpy.ops.object.mode_set(mode="OBJECT")
    for p in obj.data.polygons:
        p.select = p.material_index not in keep
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.delete(type="FACE")
    bpy.ops.object.mode_set(mode="OBJECT")
    # normalise to the tree's real height, base at 0
    zs = [v.co.z for v in obj.data.vertices]
    h = max(zs) - min(zs) if zs else 1.0
    zmax = max(zs) if zs else 1.0
    # foliage spans part of the height: scale by the full tree height ratio measured on the source (crown top = height)
    s = height / max(zmax, 0.01)
    obj.scale = (s, s, s)
    bpy.ops.object.transform_apply(scale=True)
    return obj


def to_crown_hull(obj, rng):
    """The leaf cards decimate into confetti, so each crown becomes its silhouette: the convex hull of its foliage, with
    a lumpy, noise-displaced surface and a flattened underside (the real crown's extent and height kept)."""
    import bmesh
    from mathutils import Vector, noise
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    hull = bmesh.ops.convex_hull(bm, input=bm.verts)
    keep = set(hull["geom"])
    bmesh.ops.delete(bm, geom=[g for g in list(bm.verts) if g not in keep], context="VERTS")
    # subdivide the hull so the cauliflower lumps can be displaced in, then flatten the underside
    bmesh.ops.triangulate(bm, faces=bm.faces)
    for _ in range(3):
        bmesh.ops.subdivide_edges(bm, edges=bm.edges, cuts=1, use_grid_fill=True)
    zs = [v.co.z for v in bm.verts]
    zmin, zmax = min(zs), max(zs)
    seed = Vector((rng.random() * 50, rng.random() * 50, 0))
    c = sum((v.co for v in bm.verts), Vector()) / max(1, len(bm.verts))
    size = max(1.0, zmax - zmin)
    bm.normal_update()
    for v in bm.verts:
        p = v.co / size
        # lumps: billowy (abs) noise at two scales, outward along the normal, stronger on top
        n = 1.0 - abs(noise.noise(p * 3.2 + seed)) * 2.0
        n2 = 1.0 - abs(noise.noise(p * 7.5 + seed * 2)) * 2.0
        up = max(0.2, v.normal.z * 0.5 + 0.5)
        v.co += v.normal * size * (0.11 * n + 0.05 * n2) * up
        lo = zmin + (zmax - zmin) * 0.35
        if v.co.z < lo:
            v.co.z = lo - (lo - v.co.z) * 0.3  # flatter underside
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()


def decimate(obj, target):
    t = tris(obj)
    if t > target:
        mod = obj.modifiers.new("dec", "DECIMATE")
        mod.ratio = max(0.005, target / t)
        bpy.ops.object.modifier_apply(modifier=mod.name)
    return tris(obj)


def bake_light(obj):
    mesh = obj.data
    while mesh.color_attributes:
        mesh.color_attributes.remove(mesh.color_attributes[0])
    col = mesh.color_attributes.new("Col", "BYTE_COLOR", "CORNER")
    zs = [v.co.z for v in mesh.vertices]
    zmin, zmax = min(zs), max(zs)
    for poly in mesh.polygons:
        for li in poly.loop_indices:
            v = mesh.vertices[mesh.loops[li].vertex_index]
            hgt = (v.co.z - zmin) / max(0.01, zmax - zmin)
            light = max(0.15, min(1.0, 0.2 + 0.35 * max(0.0, poly.normal.z) + 0.6 * hgt))
            col.data[li].color = (light, light, light, 1.0)


def build(name, seed, keys, radius):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    rng = random.Random(seed)
    per = MAX_TRIS // len(keys) - 4
    parts = []
    for i, key in enumerate(keys):
        obj = load_crown(key)
        to_crown_hull(obj, rng)
        decimate(obj, per)
        a = rng.uniform(0, 2 * math.pi)
        width = max(obj.dimensions.x, obj.dimensions.y)
        if i == 0:
            crown = width
        # a ring of crowns that touch and overlap: the gap is set by the first (largest) crown's width
        r = radius * crown * math.sqrt(rng.uniform(0.15, 1.0)) * (0.1 if i == 0 else 1.0)
        obj.location = (r * math.cos(a), r * math.sin(a), rng.uniform(-1.5, 0.5))
        obj.rotation_euler = (0, 0, rng.uniform(0, 2 * math.pi))
        sc = rng.uniform(0.85, 1.15)
        obj.scale = (sc, sc, sc)
        parts.append(obj)
    bpy.ops.object.select_all(action="DESELECT")
    for o in parts:
        o.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.view_layer.objects.active
    obj.name = name
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    # one material slot (M_TD_CanopyStand is set in Unreal)
    obj.data.materials.clear()
    t = tris(obj)
    if t > MAX_TRIS:
        decimate(obj, MAX_TRIS)
        t = tris(obj)
    bake_light(obj)
    for p in obj.data.polygons:
        p.use_smooth = True
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, name + ".fbx"), use_selection=True, apply_scale_options="FBX_SCALE_UNITS",
                             axis_forward="X", axis_up="Z", mesh_smooth_type="FACE", colors_type="LINEAR", add_leaf_bones=False)
    d = obj.dimensions
    print("CLUMP: %s: %d tris, %d trees, %.0f x %.0f x %.0f m" % (name, t, len(keys), d.x, d.y, d.z))


for i, (name, seed, keys, radius) in enumerate(CLUMPS):
    build(name, seed, keys, radius)

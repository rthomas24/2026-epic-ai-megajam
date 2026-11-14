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
MAX_TRIS = 3600	# s7: puffed crowns (Nanite in Unreal); s4 was 600 with one smooth hull per crown
CORE_TRIS = 60
PUFFS = 6
PUFF_TRIS = 120


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


def add_puffs(obj, rng, count):
    """s7: a crown is a cluster of leafy puffs on its hull (broken, cauliflower silhouette instead of one smooth lump):
    noise-displaced icospheres (80 tris) seated on the upper hull, each tagged with its own random value in the vertex
    colour's G channel (per-puff hue jitter in M_TD_CanopyStand)."""
    import bmesh
    from mathutils import Vector, noise
    verts = [obj.matrix_world @ v.co for v in obj.data.vertices]
    if not verts:
        return []
    zs = [v.z for v in verts]
    zmin, zmax = min(zs), max(zs)
    xs, ys = [v.x for v in verts], [v.y for v in verts]
    width = max(max(xs) - min(xs), max(ys) - min(ys))
    upper = [v for v in verts if v.z > zmin + (zmax - zmin) * 0.35] or verts
    puffs = []
    for k in range(count):
        c = upper[rng.randrange(len(upper))]
        r = width * rng.uniform(0.2, 0.32)
        bm = bmesh.new()
        bmesh.ops.create_icosphere(bm, subdivisions=2, radius=r)
        seed = Vector((rng.random() * 40, rng.random() * 40, rng.random() * 40))
        for v in bm.verts:
            n = noise.noise(v.co / r * 1.7 + seed)
            v.co *= 1.0 + 0.28 * n
            if v.co.z < -r * 0.35:
                v.co.z = -r * 0.35 + (v.co.z + r * 0.35) * 0.4
        me = bpy.data.meshes.new("puff")
        bm.to_mesh(me)
        bm.free()
        po = bpy.data.objects.new("puff", me)
        bpy.context.scene.collection.objects.link(po)
        po.location = c + Vector((0, 0, r * 0.15))
        po["hue"] = rng.random()
        puffs.append(po)
    return puffs


def decimate(obj, target):
    t = tris(obj)
    if t > target:
        mod = obj.modifiers.new("dec", "DECIMATE")
        mod.ratio = max(0.005, target / t)
        bpy.ops.object.modifier_apply(modifier=mod.name)
    return tris(obj)


def decimate_obj(obj, target):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    decimate(obj, target)


def hue_tag(obj, value=None):
    """Per-puff hue jitter in a colour attribute "Hue" (copied to the vertex colour's G at the bake)."""
    me = obj.data
    v = obj.get("hue", 0.5) if value is None else value
    a = me.attributes.get("Hue") or me.attributes.new("Hue", "FLOAT", "POINT")
    for d in a.data:
        d.value = v


def add_underside(obj):
    """#158 (B2-05): seen from below near the Verdance pad, the puff clusters left sky between the crowns (fragmented
    undersides). A shallow dome cap over the clump's footprint (2D convex hull, inset 8 %) at the crowns' lower fifth,
    sagging a little toward the middle, closes the underside; the bake gives it the dark under-leaf value."""
    import bmesh
    from mathutils import Vector, geometry
    me = obj.data
    pts = [v.co.copy() for v in me.vertices]
    if len(pts) < 8:
        return
    zs = sorted(p.z for p in pts)
    z = zs[int(len(zs) * 0.2)]
    hull = geometry.convex_hull_2d([(p.x, p.y) for p in pts])
    ring = [Vector((pts[i].x, pts[i].y, z)) for i in hull]
    c = sum(ring, Vector()) / len(ring)
    ring = [c + (r - c) * 0.92 for r in ring]
    bm = bmesh.new()
    bm.from_mesh(me)
    centre = bm.verts.new(c - Vector((0, 0, (zs[-1] - zs[0]) * 0.05)))
    rv = [bm.verts.new(r) for r in ring]
    for i in range(len(rv)):
        try:
            bm.faces.new((centre, rv[(i + 1) % len(rv)], rv[i]))   # facing down
        except ValueError:
            pass
    bm.to_mesh(me)
    bm.free()
    me.update()


def bake_light(obj):
    mesh = obj.data
    while mesh.color_attributes:
        mesh.color_attributes.remove(mesh.color_attributes[0])
    hue = mesh.attributes.get("Hue")
    hv = [d.value for d in hue.data] if hue else None
    col = mesh.color_attributes.new("Col", "BYTE_COLOR", "CORNER")
    zs = [v.co.z for v in mesh.vertices]
    zmin, zmax = min(zs), max(zs)
    for poly in mesh.polygons:
        for li in poly.loop_indices:
            v = mesh.vertices[mesh.loops[li].vertex_index]
            hgt = (v.co.z - zmin) / max(0.01, zmax - zmin)
            light = max(0.15, min(1.0, 0.2 + 0.35 * max(0.0, poly.normal.z) + 0.6 * hgt))
            g = hv[mesh.loops[li].vertex_index] if hv else 0.5
            col.data[li].color = (light, g, light, 1.0)


def build(name, seed, keys, radius):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    rng = random.Random(seed)
    parts = []
    extra = []
    for i, key in enumerate(keys):
        obj = load_crown(key)
        to_crown_hull(obj, rng)
        decimate(obj, CORE_TRIS)
        for po in add_puffs(obj, rng, PUFFS):
            decimate_obj(po, PUFF_TRIS)
            hue_tag(po)
            po.parent = obj
            po.matrix_parent_inverse = obj.matrix_world.inverted()
            extra.append((obj, po))
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
    bpy.context.view_layer.update()
    for _, po in extra:
        mw = po.matrix_world.copy()
        po.parent = None
        po.matrix_world = mw
    for o in parts:
        hue_tag(o, 0.5)
    bpy.ops.object.select_all(action="DESELECT")
    for o in parts + [po for _, po in extra]:
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
    add_underside(obj)
    bake_light(obj)
    for p in obj.data.polygons:
        p.use_smooth = True
    bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, name + ".fbx"), use_selection=True, apply_scale_options="FBX_SCALE_UNITS",
                             axis_forward="X", axis_up="Z", mesh_smooth_type="FACE", colors_type="LINEAR", add_leaf_bones=False)
    d = obj.dimensions
    print("CLUMP: %s: %d tris, %d trees, %.0f x %.0f x %.0f m" % (name, t, len(keys), d.x, d.y, d.z))


for i, (name, seed, keys, radius) in enumerate(CLUMPS):
    build(name, seed, keys, radius)

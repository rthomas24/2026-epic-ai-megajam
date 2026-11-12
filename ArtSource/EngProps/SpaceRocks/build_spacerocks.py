# SPDX-License-Identifier: MIT
"""
build_spacerocks.py - builds ONE asset of the "SpaceRocks" kit, start to finish, headless.

    blender --background --factory-startup --python build_spacerocks.py -- --asset SM_Asteroid_01
    blender --background --factory-startup --python build_spacerocks.py -- --asset SM_Asteroid_01 --stage shape --wip-dir <dir>

Everything is seeded, so a re-run reproduces the same mesh. Per asset it:
  1. builds a high-poly source rock (icosphere -> stretch/lumps/craters -> exact plane cuts ->
     boolean bites / cleft), normalised to a 100 cm bounding sphere centred on the origin;
  2. decimates a copy to the game mesh, unions the crystal inclusions into it (watertight),
     re-normalises (minimum enclosing sphere -> radius exactly 0.5 m, pivot at its centre);
  3. marks hard edges, unwraps (smart project, islands split at every hard edge, re-packed),
     applies face-weighted normals;
  4. bakes BC / N (DirectX) / ORM / E from the high-poly source with the repo's
     blender-texturing/bake_texture_set.py, then tints the baked cavities cooler in the BC map;
  5. adds one convex UCX_ hull (asteroids only), saves the .blend, runs the repo's
     asset_report / assembly_check / review_renders, and exports the FBX + manifest with
     blender-unreal-export/export_for_unreal.py.

The helper scripts used are the repo's own (.claude/skills/blender-*/scripts); nothing in them is
modified. Sizes are metres in Blender (1 m = 100 Unreal units).
"""

import json
import math
import os
import random
import runpy
import sys
import time

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector, noise
from mathutils.bvhtree import BVHTree

HERE = os.path.dirname(os.path.abspath(__file__))
REVIEW_DIR = os.path.join(HERE, "review")
SHARP_ANGLE_DEG = 32.0
TARGET_RADIUS_M = 0.5          # the sizing contract: 100 cm diameter bounding sphere, pivot at its centre


def _repo_root():
    d = HERE
    while True:
        if os.path.isdir(os.path.join(d, ".claude", "skills")):
            return d
        parent = os.path.dirname(d)
        if parent == d:
            raise RuntimeError("repo root (.claude/skills) not found above " + HERE)
        d = parent


SKILLS = os.path.join(_repo_root(), ".claude", "skills")
rocks = runpy.run_path(os.path.join(SKILLS, "blender-game-asset", "scripts", "procedural_rocks.py"))
hs = runpy.run_path(os.path.join(SKILLS, "blender-game-asset", "scripts", "hardsurface.py"))
bk = runpy.run_path(os.path.join(SKILLS, "blender-texturing", "scripts", "bake_texture_set.py"))
ex = runpy.run_path(os.path.join(SKILLS, "blender-unreal-export", "scripts", "export_for_unreal.py"))
rv = runpy.run_path(os.path.join(SKILLS, "blender-mcp", "scripts", "review_renders.py"))
ar = runpy.run_path(os.path.join(SKILLS, "blender-mcp", "scripts", "asset_report.py"))
ac = runpy.run_path(os.path.join(SKILLS, "blender-mcp", "scripts", "assembly_check.py"))

# --------------------------------------------------------------------------- the kit

# Palettes are sRGB hex. dark/mid/light = broad stone tones, patch = lighter mineral patches,
# vein = thin light veins, cool = multiplier applied in baked cavities ("cooler shadows").
PALETTES = {
    "basalt_blue":  dict(dark="#3E4A63", mid="#63718E", light="#8F9BB3", patch="#A99A82", vein="#D8D2C4"),
    "rust":         dict(dark="#5A4337", mid="#8C6A54", light="#B39478", patch="#C9B594", vein="#E8DCC6"),
    "ochre":        dict(dark="#5C4A34", mid="#8E7650", light="#B59E74", patch="#9AA3A8", vein="#E6DDC8"),
    "violet_slate": dict(dark="#453E5E", mid="#6C6389", light="#968EAF", patch="#B3A6C9", vein="#DCD3EA"),
}
CAVITY_COOL = (0.50, 0.58, 0.95)     # sRGB-space multiplier in the deepest cavities (darker and bluer)
CYAN = dict(glow="#2EE6FF", base="#1C9DBD")
VIOLET = dict(glow="#A24DFF", base="#6B2BD9")
EMISSION_STRENGTH = 2.0

# Every rock is one or more faceted "lobes" (stretched lumpy icospheres with exact plane cuts),
# optionally split by wedge cuts ("wedges": wide ones are step notches, the narrow deep one is the
# cleft), fused by a voxel remesh, then given conchoidal flake scars ("scallop": amplitude m, cells/m).
SPECS = {
    # Chunky and faceted: an angular block with a second block fused on, and two stepped ledges.
    "SM_Asteroid_01": dict(
        seed=11, palette="basalt_blue", voxel=0.0065, scallop=(0.016, 4.2, 0.005, 10.0),
        lobes=[
            dict(subdiv=5, stretch=(1.05, 0.90, 0.80), lump=(0.08, 1.3),
                 cuts=[dict(spread=6, frac=(0.56, 0.72), jitter=0.2), dict(spread=10, frac=(0.84, 0.94), jitter=0.35)]),
            dict(subdiv=5, stretch=(0.78, 0.60, 0.52), lump=(0.08, 1.5), offset=(0.62, -0.44, 0.40), rot=(20, 35, 10),
                 cuts=[dict(spread=6, frac=(0.55, 0.75), jitter=0.25), dict(spread=6, frac=(0.85, 0.94), jitter=0.35)]),
        ],
        wedges=[dict(along=(0.3, 1.0, 0.25), up=(0.85, 0.1, 0.55), offset=0.20, depth=0.36, mouth=0.42, skew=0.75,
                     zigzag=0.03, stations=4, depth_ends=(1.0, 0.85)),
                dict(along=(1.0, 0.25, -0.3), up=(-0.3, -0.55, -0.8), offset=-0.18, depth=0.30, mouth=0.36, skew=-0.7,
                     zigzag=0.03, stations=4, depth_ends=(0.9, 1.0))],
        tris=3000, tex=1024, collision=True),
    # Elongated: a long tapering shard with facets running down its length, one blunt broken end,
    # a stepped notch near the blunt end with cyan crystals growing out of it.
    "SM_Asteroid_02": dict(
        seed=23, palette="rust", voxel=0.0055, scallop=(0.012, 5.0, 0.004, 11.0),
        lobes=[
            dict(subdiv=5, stretch=(1.0, 0.44, 0.36), lump=(0.09, 1.6), taper=0.52, bend=0.16,
                 cuts=[dict(ring=5, frac=(0.58, 0.82), tilt=0.38),
                       dict(dirs=[(1.0, 0.45, -0.35), (-1.0, -0.6, 0.5), (-0.8, 0.7, -0.3)], fracs=[0.80, 0.90, 0.88]),
                       dict(spread=8, frac=(0.88, 0.96), jitter=0.35)]),
        ],
        wedges=[dict(along=(0.2, 1.0, 0.0), up=(0.3, 0.0, 1.0), offset=0.36, depth=0.40, mouth=0.32, skew=0.5,
                     zigzag=0.03, stations=4, depth_ends=(1.0, 0.85), is_cleft=True)],
        crystals=dict(colour=CYAN, in_cleft=[(-0.03, -0.35, 4, 0.175)], clusters=[
            dict(origin=(0.24, -1.0, 0.03), dir=(0.0, 1.0, 0.0), count=3, height=0.105, spread=0.04)]),
        tris=2850, tex=1024, collision=True),
    # Cratered and rounder: one giant crater with a central peak, several large and medium ones.
    "SM_Asteroid_03": dict(
        seed=37, palette="ochre", voxel=0.0065, scallop=(0.005, 6.0, 0.0, 0.0),
        lobes=[dict(subdiv=6, stretch=(1.0, 0.94, 0.87), lump=(0.06, 1.1),
                    craters=dict(giant=1, large=3, medium=5, small=4),
                    cuts=[dict(spread=3, frac=(0.70, 0.80), jitter=0.3)])],
        tris=3200, tex=1024, collision=True),
    # Fractured: one faceted mass split by a deep, wide zig-zag cleft with a side crack; violet
    # crystals grow out of the cleft walls.
    "SM_Asteroid_04": dict(
        seed=41, palette="violet_slate", voxel=0.0065, scallop=(0.014, 4.6, 0.005, 10.5),
        lobes=[
            dict(subdiv=5, stretch=(1.20, 0.98, 0.90), lump=(0.10, 1.2),
                 cuts=[dict(spread=8, frac=(0.66, 0.82), jitter=0.25), dict(spread=10, frac=(0.86, 0.95), jitter=0.35)]),
        ],
        wedges=[dict(along=(0.22, 1.0, 0.1), up=(0.12, 0.0, 1.0), offset=-0.16, depth=1.22, mouth=0.52,
                     zigzag=0.10, stations=8, depth_ends=(1.05, 0.80), is_cleft=True),
                dict(along=(1.0, -0.35, 0.05), up=(0.0, -0.45, 1.0), offset=0.10, depth=0.50, mouth=0.10,
                     zigzag=0.06, stations=7, depth_ends=(0.25, 1.1))],
        crystals=dict(colour=VIOLET, in_cleft=[(-0.17, 0.6, 4, 0.18), (0.05, -0.6, 4, 0.20), (0.24, 0.5, 3, 0.15)]),
        tris=2850, tex=1024, collision=True),
    # Debris: simple shards for scattering between asteroids.
    "SM_SpaceDebris_01": dict(   # flat slab shard
        seed=53, palette="basalt_blue", voxel=0.006, scallop=(0.012, 4.5, 0.0, 0.0),
        lobes=[dict(subdiv=4, stretch=(1.0, 0.62, 0.24), lump=(0.06, 1.5),
                    cuts=[dict(dirs=[(0.05, 0.1, 1.0), (0.1, -0.05, -1.0)], fracs=[0.70, 0.76]),
                          dict(ring_z=5, frac=(0.56, 0.82), tilt=0.35)])],
        tris=420, tex=512, collision=False),
    "SM_SpaceDebris_02": dict(   # long splinter
        seed=59, palette="rust", voxel=0.006, scallop=(0.008, 5.0, 0.0, 0.0),
        lobes=[dict(subdiv=4, stretch=(1.0, 0.30, 0.24), lump=(0.05, 1.8), taper=0.62,
                    cuts=[dict(ring=4, frac=(0.44, 0.66), tilt=0.45),
                          dict(dirs=[(1.0, 0.5, 0.3), (-1.0, 0.6, -0.5)], fracs=[0.80, 0.93])])],
        tris=380, tex=512, collision=False),
    "SM_SpaceDebris_03": dict(   # blocky wedge chunk
        seed=67, palette="ochre", voxel=0.006, scallop=(0.014, 4.0, 0.0, 0.0),
        lobes=[dict(subdiv=4, stretch=(1.0, 0.82, 0.66), lump=(0.07, 1.4),
                    cuts=[dict(spread=9, frac=(0.58, 0.78), jitter=0.3)])],
        tris=440, tex=512, collision=False),
}

# --------------------------------------------------------------------------- small utilities


def log(*a):
    print("[spacerocks]", *a)
    sys.stdout.flush()


def rand_rot(rng):
    return (Matrix.Rotation(rng.uniform(0, math.tau), 3, "X") @ Matrix.Rotation(rng.uniform(0, math.tau), 3, "Y")
            @ Matrix.Rotation(rng.uniform(0, math.tau), 3, "Z"))


def spread_dirs(n, rng, jitter=0.2):
    """n well-spread directions (Fibonacci sphere, randomly rotated, jittered)."""
    rot = rand_rot(rng)
    golden = math.pi * (3.0 - math.sqrt(5.0))
    out = []
    for i in range(n):
        z = 1.0 - 2.0 * (i + 0.5) / n
        r = math.sqrt(max(0.0, 1.0 - z * z))
        d = rot @ Vector((math.cos(golden * i) * r, math.sin(golden * i) * r, z))
        d += Vector((rng.gauss(0, jitter), rng.gauss(0, jitter), rng.gauss(0, jitter)))
        out.append(d.normalized())
    return out


def mesh_points(me):
    co = np.empty(len(me.vertices) * 3, dtype=np.float64)
    me.vertices.foreach_get("co", co)
    return co.reshape(-1, 3)


def min_ball(points, iterations=30000):
    """Minimum enclosing sphere (Badoiu-Clarkson). Returns (centre, radius); the radius is the exact
    farthest-point distance from the returned centre, so the sphere always encloses every point."""
    c = points.mean(axis=0)
    for i in range(1, iterations + 1):
        d = points - c
        k = int(np.argmax(np.einsum("ij,ij->i", d, d)))
        c = c + (points[k] - c) / (i + 1.0)
    d = points - c
    return c, float(np.sqrt(np.einsum("ij,ij->i", d, d).max()))


def tri_count(me):
    return sum(len(p.vertices) - 2 for p in me.polygons)


def remove_object(obj):
    me = obj.data
    bpy.data.objects.remove(obj, do_unlink=True)
    if me is not None and me.users == 0:
        bpy.data.meshes.remove(me)


def bm_to_object(name, bm, coll):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    obj = bpy.data.objects.new(name, me)
    coll.objects.link(obj)
    return obj


def cleanup_mesh(me, weld=2e-5):
    """Weld, drop degenerate geometry, triangulate, make normals consistent (outward)."""
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=weld)
    bmesh.ops.dissolve_degenerate(bm, edges=list(bm.edges), dist=weld)
    loose = [v for v in bm.verts if not v.link_faces]
    if loose:
        bmesh.ops.delete(bm, geom=loose, context="VERTS")
    bmesh.ops.triangulate(bm, faces=list(bm.faces), quad_method="BEAUTY", ngon_method="BEAUTY")
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    boundary = sum(1 for e in bm.edges if e.is_boundary)
    nonman = sum(1 for e in bm.edges if not e.is_manifold and not e.is_boundary)
    closed = boundary == 0 and nonman == 0
    if closed and bm.calc_volume(signed=True) < 0:
        bmesh.ops.reverse_faces(bm, faces=list(bm.faces))
    bm.to_mesh(me)
    bm.free()
    me.update()
    return {"boundary_edges": boundary, "non_manifold_edges": nonman, "closed": closed}


# --------------------------------------------------------------------------- shape building


def base_blob(spec, rng):
    """Stretched, lumpy icosphere; optional taper / bend along X; optional craters."""
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=spec["subdiv"], radius=1.0)
    sx, sy, sz = spec["stretch"]
    amp, freq = spec["lump"]
    taper, bend = spec.get("taper", 0.0), spec.get("bend", 0.0)
    off = Vector((rng.uniform(-100, 100), rng.uniform(-100, 100), rng.uniform(-100, 100)))
    craters = make_craters(spec.get("craters"), rng)
    for v in bm.verts:
        n = v.co.normalized()
        h = noise.fractal(n * freq + off, 1.0, 2.0, 2, noise_basis="PERLIN_ORIGINAL") * amp
        for c_dir, c_r, c_depth, c_pow, c_peak in craters:
            x = n.angle(c_dir) / c_r
            if x < 2.2:
                rim = 0.32 * math.exp(-((x - 1.0) / 0.24) ** 2)
                bowl = (x ** c_pow - 1.0) if x < 1.0 else 0.0
                peak = c_peak * math.exp(-(x / 0.2) ** 2)
                h += c_depth * (bowl + rim + peak)
        p = n * (1.0 + h)
        t = p.x                                   # -1..1 along the long axis (before stretch)
        s = 1.0 - taper * t
        v.co = Vector((p.x * sx, (p.y * s) * sy + bend * t * t * sx, (p.z * s) * sz))
    bm.normal_update()
    return bm


def make_craters(counts, rng):
    """[(direction, angular radius, depth, floor exponent, central peak)] - big ones first."""
    if not counts:
        return []
    out = []

    def add(n, radius, depth, power, peak=0.0, min_sep=0.0):
        made, tries = 0, 0
        while made < n and tries < 400:
            tries += 1
            d = Vector((rng.gauss(0, 1), rng.gauss(0, 1), rng.gauss(0, 1))).normalized()
            r = rng.uniform(*radius)
            if any(d.angle(o[0]) < (r + o[1]) * min_sep for o in out):
                continue
            out.append((d, r, rng.uniform(*depth), power, peak))
            made += 1

    add(counts.get("giant", 0), (0.66, 0.72), (0.27, 0.30), 3.0, peak=0.22)
    add(counts.get("large", 0), (0.30, 0.42), (0.12, 0.15), 3.0, min_sep=0.85)
    add(counts.get("medium", 0), (0.15, 0.22), (0.06, 0.08), 2.5, min_sep=0.8)
    add(counts.get("small", 0), (0.08, 0.11), (0.03, 0.04), 2.0, min_sep=0.8)
    return out


def plane_cut(bm, normal, frac):
    """Exact planar fracture: remove everything beyond `frac` of the body's extent along `normal`
    and cap the hole. Returns False (and leaves a closed mesh) if the cap could not be made."""
    n = Vector(normal).normalized()
    support = max(v.co.dot(n) for v in bm.verts)
    geom = list(bm.verts) + list(bm.edges) + list(bm.faces)
    bmesh.ops.bisect_plane(bm, geom=geom, dist=1e-7, plane_co=n * (support * frac), plane_no=n,
                           clear_outer=True, clear_inner=False)
    edges = [e for e in bm.edges if e.is_boundary]
    if edges:
        res = bmesh.ops.holes_fill(bm, edges=edges, sides=0)
        faces = [f for f in res["faces"] if f.is_valid]
        if faces:
            bmesh.ops.triangulate(bm, faces=faces, quad_method="BEAUTY", ngon_method="BEAUTY")
    left = [e for e in bm.edges if e.is_boundary]
    if left:
        bmesh.ops.triangle_fill(bm, use_beauty=True, use_dissolve=False, edges=left)
    return not any(e.is_boundary for e in bm.edges)


def apply_cuts(bm, spec, rng):
    sx, sy, sz = spec["stretch"]
    n_ok = 0
    for group in spec.get("cuts", []):
        if "spread" in group:
            # Directions are spread on the unit sphere, then mapped to the ellipsoid's surface normals
            # so the facets are evenly distributed over a stretched body.
            dirs = [Vector((d.x / sx, d.y / sy, d.z / sz)) for d in spread_dirs(group["spread"], rng, group["jitter"])]
            fracs = [rng.uniform(*group["frac"]) for _ in dirs]
        elif "ring" in group or "ring_z" in group:
            # Facets running along the long axis (ring) or around a slab's rim (ring_z).
            k = group.get("ring", group.get("ring_z"))
            start = rng.uniform(0, math.tau)
            dirs, fracs = [], []
            for i in range(k):
                a = start + math.tau * (i + rng.uniform(-0.25, 0.25)) / k
                tilt = rng.uniform(-group["tilt"], group["tilt"])
                if "ring" in group:
                    dirs.append(Vector((tilt, math.cos(a) / sy, math.sin(a) / sz)))
                else:
                    dirs.append(Vector((math.cos(a) / sx, math.sin(a) / sy, tilt)))
                fracs.append(rng.uniform(*group["frac"]))
        else:
            dirs = [Vector(d) for d in group["dirs"]]
            fracs = list(group["fracs"])
        for d, f in zip(dirs, fracs):
            n_ok += bool(plane_cut(bm, d, f))
    bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=1e-6)
    bmesh.ops.dissolve_degenerate(bm, edges=list(bm.edges), dist=1e-6)
    bmesh.ops.triangulate(bm, faces=list(bm.faces), quad_method="BEAUTY", ngon_method="BEAUTY")
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    return n_ok


def boolean_apply(obj, cutter, operation):
    """Exact boolean through the repo's hardsurface helper, applied, cutter removed."""
    hs["boolean"](obj, cutter, operation=operation, solver="EXACT")
    hs["apply_modifiers"](obj)
    remove_object(cutter)
    return cleanup_mesh(obj.data)


def build_lobe(lobe, rng):
    """One faceted mass: lumpy stretched icosphere -> exact plane cuts -> placed."""
    bm = base_blob(lobe, rng)
    apply_cuts(bm, lobe, rng)
    rx, ry, rz = (math.radians(a) for a in lobe.get("rot", (0.0, 0.0, 0.0)))
    m = (Matrix.Translation(Vector(lobe.get("offset", (0.0, 0.0, 0.0)))) @ Matrix.Rotation(rz, 4, "Z")
         @ Matrix.Rotation(ry, 4, "Y") @ Matrix.Rotation(rx, 4, "X"))
    bmesh.ops.transform(bm, matrix=m, verts=list(bm.verts))
    return bm


def cleft_frame(c):
    along = Vector(c["along"]).normalized()
    up = Vector(c["up"])
    up = (up - along * up.dot(along)).normalized()
    side = up.cross(along).normalized()
    return along, up, side


def wedge_cut(lobes, c, rng, coll):
    """Cut a zig-zag wedge out of every lobe (exact boolean difference). A wide shallow wedge is a
    step notch; a narrow deep one is the cleft. Returns the stations used and the body's top along
    `up` (so crystals can be placed inside the cleft)."""
    along, up, side = cleft_frame(c)
    pts = [v.co for o in lobes for v in o.data.vertices]
    top = max(p.dot(up) for p in pts)
    half_len = max(abs(p.dot(along)) for p in pts) * 1.2
    z_top = top + 0.3
    n = c["stations"]
    rings, stations = [], []
    for i in range(n):
        t = i / (n - 1.0)
        s = -half_len + 2.0 * half_len * t
        y = c["offset"] + c["zigzag"] * (1.0 if i % 2 else -1.0) * rng.uniform(0.55, 1.0)
        depth = c["depth"] * (c["depth_ends"][0] * (1 - t) + c["depth_ends"][1] * t) * rng.uniform(0.93, 1.05)
        mouth = c["mouth"] * rng.uniform(0.8, 1.25)
        w_top = mouth * (depth + 0.3) / depth
        skew = c.get("skew", 0.0)          # 0 = symmetric V; towards 1 = one steep wall + one ledge (a step)
        apex = along * s + side * y + up * (top - depth)
        rings.append((apex, along * s + side * (y - w_top * (1.0 - skew)) + up * z_top,
                      along * s + side * (y + w_top * (1.0 + skew)) + up * z_top))
        stations.append(dict(s=s, y=y, depth=depth, mouth=mouth))
    cm = bmesh.new()
    vr = [[cm.verts.new(p) for p in ring] for ring in rings]
    for i in range(n - 1):
        a0, l0, r0 = vr[i]
        a1, l1, r1 = vr[i + 1]
        cm.faces.new((a0, a1, l1, l0))
        cm.faces.new((a0, r0, r1, a1))
        cm.faces.new((l0, l1, r1, r0))
    cm.faces.new(vr[0])
    cm.faces.new(vr[-1][::-1])
    bmesh.ops.triangulate(cm, faces=list(cm.faces))
    bmesh.ops.recalc_face_normals(cm, faces=list(cm.faces))
    cutter = bm_to_object("CUT_wedge", cm, coll)
    for o in lobes:
        hs["boolean"](o, cutter, operation="DIFFERENCE", solver="EXACT")
        hs["apply_modifiers"](o)
        cleanup_mesh(o.data)
    remove_object(cutter)
    return stations, top


def fuse(lobes, name, coll):
    """Join the (overlapping) lobes into one object; the voxel remesh that follows unions them."""
    bm = bmesh.new()
    for o in lobes:
        bm.from_mesh(o.data)
    obj = bm_to_object(name, bm, coll)
    for o in lobes:
        remove_object(o)
    return obj


def remesh(obj, voxel):
    """Voxel remesh: one closed, evenly tessellated skin (fuses the lobes, drops internal faces,
    rounds the fracture edges by about one voxel)."""
    mod = obj.modifiers.new("Remesh", "REMESH")
    mod.mode = "VOXEL"
    mod.voxel_size = voxel
    mod.adaptivity = 0.0
    mod.use_smooth_shade = True
    hs["apply_modifiers"](obj)
    return cleanup_mesh(obj.data)


def scallop(obj, params, rng):
    """Flake scars: displace along the normal by a Manhattan-distance Voronoi F1 (piecewise planar
    dents with crisp ridges between them, continuous everywhere so nothing tears). Two octaves in
    two differently rotated frames so the facet directions do not line up."""
    amp1, freq1, amp2, freq2 = params
    r1, r2 = rand_rot(rng), rand_rot(rng)
    o1 = Vector((rng.uniform(-50, 50), rng.uniform(-50, 50), rng.uniform(-50, 50)))
    o2 = Vector((rng.uniform(-50, 50), rng.uniform(-50, 50), rng.uniform(-50, 50)))
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.normal_update()
    moves = []
    for v in bm.verts:
        h = 0.0
        if amp1:
            d, _p = noise.voronoi((r1 @ v.co) * freq1 + o1, distance_metric="MANHATTAN")
            h += (d[0] - 0.55) * amp1
        if amp2:
            d, _p = noise.voronoi((r2 @ v.co) * freq2 + o2, distance_metric="MANHATTAN")
            h += (d[0] - 0.55) * amp2
        moves.append(v.normal * h)
    for v, mv in zip(bm.verts, moves):
        v.co += mv
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()


def normalise(objs, points, extra_points=()):
    """Move/scale `objs` (mesh data) so the minimum enclosing sphere of `points` is centred on the
    origin with radius TARGET_RADIUS_M. Returns the transform and the measured numbers."""
    c, r = min_ball(points)
    m = Matrix.Diagonal((TARGET_RADIUS_M / r,) * 3 + (1.0,)) @ Matrix.Translation(Vector(-c))
    for o in objs:
        o.data.transform(m)
        o.data.update()
    return m, {"centre_before": [float(x) for x in c], "radius_before": r}


def decimate(obj, target_tris):
    tris = tri_count(obj.data)
    if tris > target_tris:
        mod = obj.modifiers.new("Decimate", "DECIMATE")
        mod.decimate_type = "COLLAPSE"
        mod.ratio = target_tris / float(tris)
        mod.use_collapse_triangulate = True
        hs["apply_modifiers"](obj)
    return cleanup_mesh(obj.data)


# --------------------------------------------------------------------------- crystals


def add_crystals(low, spec, rng, coll, cleft_info=None):
    """Faceted crystal inclusions (the repo's procedural_rocks._crystal), embedded in the surface.
    They are boolean-unioned into the game mesh (stays watertight) and also returned as one source
    object for the bake. Returns (source_object, anchor_points)."""
    cs = spec["crystals"]
    bm_low = bmesh.new()
    bm_low.from_mesh(low.data)
    bvh = BVHTree.FromBMesh(bm_low)
    clusters = list(cs.get("clusters", []))
    if cleft_info and cs.get("in_cleft"):
        along, up, side = cleft_info["frame"]
        top = cleft_info["top"]
        stations = cleft_info["stations"]
        for s_pos, lean, count, height in cs["in_cleft"]:
            # Interpolate the cleft's apex line at this point along it, and cast from INSIDE the cleft
            # (a little over half-way down) towards one wall, so the cluster grows out of the wall.
            for q0, q1 in zip(stations, stations[1:]):
                if q0["s"] <= s_pos <= q1["s"]:
                    t = (s_pos - q0["s"]) / (q1["s"] - q0["s"])
                    y = q0["y"] * (1 - t) + q1["y"] * t
                    depth = q0["depth"] * (1 - t) + q1["depth"] * t
                    break
            else:
                raise ValueError("in_cleft position {} is outside the cleft".format(s_pos))
            origin = along * s_pos + side * y + up * (top - depth * 0.62)
            direction = (-up * 0.8 + side * lean).normalized()
            clusters.append(dict(origin=origin, dir=direction, count=count, height=height, spread=0.05))
    pieces, anchors = [], []
    for cl in clusters:
        o, d = Vector(cl["origin"]), Vector(cl["dir"]).normalized()
        hit, nrm, _idx, _dist = bvh.ray_cast(o, d)
        if hit is None:
            log("WARNING crystal cluster ray missed", cl)
            continue
        anchors.append(hit.copy())
        t1 = nrm.orthogonal().normalized()
        t2 = nrm.cross(t1).normalized()
        for k in range(cl["count"]):
            if k == 0:
                p, n, h = hit, nrm, cl["height"]
                direction = (nrm + Vector((rng.uniform(-0.12, 0.12), rng.uniform(-0.12, 0.12),
                                           rng.uniform(-0.12, 0.12)))).normalized()
            else:
                a = rng.uniform(0, math.tau)
                r = cl["spread"] * rng.uniform(0.45, 1.0)
                q = hit + (t1 * math.cos(a) + t2 * math.sin(a)) * r + nrm * 0.06
                h2 = bvh.ray_cast(q, -nrm)
                if h2[0] is None:
                    h2 = bvh.find_nearest(q)
                p, n = h2[0], h2[1]
                h = cl["height"] * rng.uniform(0.38, 0.78)
                outward = (p - hit)
                outward = outward.normalized() if outward.length > 1e-6 else t1
                direction = (n * 0.5 + nrm * 0.5 + outward * rng.uniform(0.25, 0.65)).normalized()
            radius = h * rng.uniform(0.20, 0.27)
            base = p - direction * (h * 0.32)
            cb = bmesh.new()
            rocks["_crystal"](cb, base, direction, radius, h, 6, rng.uniform(1.2, 1.8), rng)
            bmesh.ops.triangulate(cb, faces=list(cb.faces))
            bmesh.ops.recalc_face_normals(cb, faces=list(cb.faces))
            pieces.append(cb)
    bm_low.free()
    if not pieces:
        return None, anchors
    # Source object for the bake: all crystals in one mesh (overlapping is fine for a bake source).
    src_bm = bmesh.new()
    for cb in pieces:
        tmp = bpy.data.meshes.new("tmp")
        cb.to_mesh(tmp)
        src_bm.from_mesh(tmp)
        bpy.data.meshes.remove(tmp)
    src = bm_to_object("SRC_{}_Crystals".format(spec["short"]), src_bm, coll)
    src.data.shade_flat()
    # Game mesh: union one crystal at a time (robust), then clean up.
    for i, cb in enumerate(pieces):
        cutter = bm_to_object("CUT_crystal_{:02d}".format(i), cb, coll)
        hs["boolean"](low, cutter, operation="UNION", solver="EXACT")
    hs["apply_modifiers"](low)
    for o in [o for o in bpy.data.objects if o.name.startswith("CUT_crystal_")]:
        remove_object(o)
    # The unions leave sub-millimetre slivers at the seams: weld them, but never at the price of
    # an open or non-manifold mesh (fall back to a finer weld if that happens).
    backup = low.data.copy()
    for weld in (4e-4, 2e-4, 1.1e-4, 2e-5):
        stats = cleanup_mesh(low.data, weld=weld)
        if stats["closed"]:
            break
        restored = backup.copy()
        old = low.data
        low.data = restored
        bpy.data.meshes.remove(old)
    bpy.data.meshes.remove(backup)
    low.data.name = low.name
    log("crystals:", len(pieces), "union ->", stats, "weld", weld)
    return src, anchors


# --------------------------------------------------------------------------- UVs and normals


def mark_sharp(me, angle_deg):
    me.shade_smooth()
    me.set_sharp_from_angle(angle=math.radians(angle_deg))


def split_uv_at_sharp(me):
    """A hard edge needs a UV split, or the two faces share texels with different tangent frames
    (a visible line in the normal map). Detach UV islands along every sharp edge: each connected
    piece is moved to its own cell of a wide grid (so no two pieces touch or overlap), and
    pack_islands then brings them all back into 0..1."""
    bm = bmesh.new()
    bm.from_mesh(me)
    uv = bm.loops.layers.uv.verify()
    bm.faces.ensure_lookup_table()

    def loop_uv(face, vert):
        for lp in face.loops:
            if lp.vert is vert:
                return lp[uv].uv
        return None

    comp = {}
    n_comp = 0
    for f in bm.faces:
        if f.index in comp:
            continue
        comp[f.index] = n_comp
        stack = [f]
        while stack:
            g = stack.pop()
            for e in g.edges:
                if not e.smooth or len(e.link_faces) != 2:
                    continue
                o = e.link_faces[0] if e.link_faces[1] is g else e.link_faces[1]
                if o.index in comp:
                    continue
                if all((loop_uv(g, v) - loop_uv(o, v)).length < 1e-6 for v in e.verts):
                    comp[o.index] = n_comp
                    stack.append(o)
        n_comp += 1
    cols = int(math.ceil(math.sqrt(n_comp)))
    for f in bm.faces:
        k = comp[f.index]
        shift = Vector((2.0 * (k % cols), 2.0 * (k // cols)))
        for lp in f.loops:
            lp[uv].uv = lp[uv].uv + shift
    bm.to_mesh(me)
    bm.free()
    return n_comp


def pack_uv(obj, margin):
    vl = bpy.context.view_layer
    for o in vl.objects:
        o.select_set(False)
    obj.select_set(True)
    vl.objects.active = obj
    bpy.ops.object.mode_set(mode="EDIT")
    try:
        bpy.ops.mesh.select_all(action="SELECT")
        try:
            bpy.ops.uv.select_all(action="SELECT")
        except Exception:
            pass
        bpy.ops.uv.pack_islands(rotate=True, margin_method="FRACTION", margin=margin)
    finally:
        bpy.ops.object.mode_set(mode="OBJECT")


def fix_uv_overlaps(obj, margin, rounds=5):
    """Smart project can fold a little of an island over itself beside an overhang (the cleft
    walls, crystal roots). Detach every face found overlapping another into its own island and
    re-pack, until the raster test finds no overlap. Returns the number of faces detached."""
    detached = 0
    for _ in range(rounds):
        bad = uv_stats(obj.data, 2048, return_bad=True)
        if not bad:
            break
        uvl = obj.data.uv_layers.active.data
        for i, fi in enumerate(sorted(bad)):
            poly = obj.data.polygons[fi]
            shift = Vector((10.0 + 2.0 * (i % 40), 10.0 + 2.0 * (i // 40)))
            for li in range(poly.loop_start, poly.loop_start + poly.loop_total):
                uvl[li].uv = Vector(uvl[li].uv) + shift
        detached += len(bad)
        pack_uv(obj, margin)
    return detached


def uv_stats(me, res=1024, return_bad=False):
    """Rasterise every UV triangle (strictly-inside test at pixel centres): any pixel covered twice is
    a real overlap. Also reports the UV bounds and the share of the 0..1 square that is used."""
    uv = np.empty(len(me.loops) * 2, dtype=np.float32)
    me.uv_layers.active.data.foreach_get("uv", uv)
    uv = uv.reshape(-1, 2)
    count = np.zeros((res, res), dtype=np.uint8)
    owner = np.full((res, res), -1, dtype=np.int32)
    areas = {}
    bad = set()
    for poly in me.polygons:
        p = uv[poly.loop_start:poly.loop_start + 3] * res
        x0, x1 = max(int(math.floor(p[:, 0].min())), 0), min(int(math.ceil(p[:, 0].max())), res)
        y0, y1 = max(int(math.floor(p[:, 1].min())), 0), min(int(math.ceil(p[:, 1].max())), res)
        if x1 <= x0 or y1 <= y0:
            continue
        xs, ys = np.meshgrid(np.arange(x0, x1) + 0.5, np.arange(y0, y1) + 0.5)
        (ax, ay), (bx, by), (cx, cy) = p
        den = (by - cy) * (ax - cx) + (cx - bx) * (ay - cy)
        if abs(den) < 1e-12:
            continue
        w0 = ((by - cy) * (xs - cx) + (cx - bx) * (ys - cy)) / den
        w1 = ((cy - ay) * (xs - cx) + (ax - cx) * (ys - cy)) / den
        inside = (w0 > 1e-5) & (w1 > 1e-5) & ((1.0 - w0 - w1) > 1e-5)
        areas[poly.index] = abs(den)
        sub_owner = owner[y0:y1, x0:x1]
        clash = inside & (sub_owner >= 0)
        if clash.any():
            # Of each overlapping pair, flag the smaller face (it is the one to detach).
            for other in np.unique(sub_owner[clash]):
                bad.add(poly.index if areas[poly.index] <= areas[int(other)] else int(other))
        sub_owner[inside & (sub_owner < 0)] = poly.index
        count[y0:y1, x0:x1] += inside
    if return_bad:
        return bad
    covered = int((count > 0).sum())
    overlap = int((count > 1).sum())
    return {"uv_min": [round(float(uv[:, 0].min()), 4), round(float(uv[:, 1].min()), 4)],
            "uv_max": [round(float(uv[:, 0].max()), 4), round(float(uv[:, 1].max()), 4)],
            "inside_0_1": bool(uv.min() >= 0.0 and uv.max() <= 1.0),
            "coverage": round(covered / float(res * res), 4),
            "overlap_pixels": overlap,
            "overlap_share_of_covered": round(overlap / float(max(covered, 1)), 6),
            "raster": res}


def triangle_quality(me):
    """Sliver check: aspect = longest edge^2 / (2 * area)... >= 1.15 for equilateral; big = thin."""
    co = mesh_points(me)
    idx = np.array([p.vertices[:] for p in me.polygons], dtype=np.int64)
    a, b, c = co[idx[:, 0]], co[idx[:, 1]], co[idx[:, 2]]
    e = np.stack([np.linalg.norm(b - a, axis=1), np.linalg.norm(c - b, axis=1), np.linalg.norm(a - c, axis=1)], axis=1)
    area = 0.5 * np.linalg.norm(np.cross(b - a, c - a), axis=1)
    aspect = (e.max(axis=1) ** 2) / np.maximum(2.0 * area, 1e-18)
    return {"aspect_median": round(float(np.median(aspect)), 2), "aspect_p95": round(float(np.percentile(aspect, 95)), 2),
            "aspect_max": round(float(aspect.max()), 1), "slivers_over_25": int((aspect > 25).sum()),
            "min_area_cm2": round(float(area.min()) * 1e4, 5), "shortest_edge_cm": round(float(e.min()) * 100, 4)}


def deviation(low, sources):
    """How far the game mesh is from the source surface (drives the bake's cage extrusion)."""
    def bvh_of(objs):
        bm = bmesh.new()
        for o in objs:
            bm.from_mesh(o.data)
        tree = BVHTree.FromBMesh(bm)
        return tree, bm
    low_tree, low_bm = bvh_of([low])
    dist = []
    for o in sources:
        for v in o.data.vertices:
            hit = low_tree.find_nearest(v.co)
            if hit[0] is not None:
                dist.append(hit[3])
    low_bm.free()
    d = np.array(dist)
    # ...and the other way round: game-mesh vertices and face centres to the source surface. A flat
    # game-mesh face can sit a few millimetres INSIDE the source (it cuts across the flake scars);
    # a cage smaller than that starts the bake ray inside the rock and it returns the far side.
    src_tree, src_bm = bvh_of(sources)
    back = [src_tree.find_nearest(v.co)[3] for v in low.data.vertices]
    back += [src_tree.find_nearest(f.center)[3] for f in low.data.polygons]
    src_bm.free()
    b = np.array(back)
    return {"max_m": float(d.max()), "mean_m": float(d.mean()), "p99_m": float(np.percentile(d, 99.0)),
            "p999_m": float(np.percentile(d, 99.9)), "game_to_source_p995_m": float(np.percentile(b, 99.5)),
            "game_to_source_max_m": float(b.max())}


# --------------------------------------------------------------------------- materials


def store_curvature(me, iterations=2):
    """Per-vertex convexity of the bake source (positive on ridges and fracture edges, negative in
    creases), smoothed, stored as the float attribute "curv" for the source material: light worn
    edges and darker creases, deterministic and noise-free."""
    n = len(me.vertices)
    co = mesh_points(me)
    nor = np.empty(n * 3, dtype=np.float64)
    me.vertices.foreach_get("normal", nor)
    nor = nor.reshape(-1, 3)
    ed = np.empty(len(me.edges) * 2, dtype=np.int32)
    me.edges.foreach_get("vertices", ed)
    ed = ed.reshape(-1, 2)
    cnt = np.zeros(n)
    np.add.at(cnt, ed[:, 0], 1.0)
    np.add.at(cnt, ed[:, 1], 1.0)
    cnt = np.maximum(cnt, 1.0)
    acc = np.zeros((n, 3))
    np.add.at(acc, ed[:, 0], co[ed[:, 1]])
    np.add.at(acc, ed[:, 1], co[ed[:, 0]])
    edge_len = float(np.linalg.norm(co[ed[:, 0]] - co[ed[:, 1]], axis=1).mean())
    curv = -((acc / cnt[:, None] - co) * nor).sum(axis=1) / edge_len
    for _ in range(iterations):
        a = np.zeros(n)
        np.add.at(a, ed[:, 0], curv[ed[:, 1]])
        np.add.at(a, ed[:, 1], curv[ed[:, 0]])
        curv = 0.5 * curv + 0.5 * a / cnt
    attr = me.attributes.get("curv") or me.attributes.new("curv", "FLOAT", "POINT")
    attr.data.foreach_set("value", curv.astype(np.float32))
    return {"p05": float(np.percentile(curv, 5)), "p50": float(np.percentile(curv, 50)),
            "p95": float(np.percentile(curv, 95)), "p99": float(np.percentile(curv, 99))}


def _set(nt, sock, val):
    if isinstance(val, bpy.types.NodeSocket):
        nt.links.new(val, sock)
    elif isinstance(val, (tuple, list)):
        sock.default_value = (val[0], val[1], val[2], 1.0) if len(sock.default_value) == 4 else tuple(val[:3])
    else:
        sock.default_value = val


def _mix(nt, fac, a, b):
    n = nt.nodes.new("ShaderNodeMix")
    n.data_type = "RGBA"
    n.clamp_factor = True
    _set(nt, n.inputs[0], fac)
    _set(nt, n.inputs[6], a)
    _set(nt, n.inputs[7], b)
    return n.outputs[2]


def _math(nt, op, a, b=None, clamp=False):
    n = nt.nodes.new("ShaderNodeMath")
    n.operation = op
    n.use_clamp = clamp
    _set(nt, n.inputs[0], a)
    if b is not None:
        _set(nt, n.inputs[1], b)
    return n.outputs[0]


def _ramp(nt, fac, stops):
    n = nt.nodes.new("ShaderNodeValToRGB")
    cr = n.color_ramp
    while len(cr.elements) < len(stops):
        cr.elements.new(0.5)
    for el, (pos, col) in zip(cr.elements, stops):
        el.position = pos
        el.color = (col, col, col, 1.0) if isinstance(col, (int, float)) else (col[0], col[1], col[2], 1.0)
    nt.links.new(fac, n.inputs[0])
    return n.outputs[0]


def make_source_rock_material(name, pal_name, seed, glow=None, anchors=(), glow_radius=0.17):
    """Procedural rock for the BAKE SOURCE only (object coordinates, so it does not depend on UVs):
    broad three-tone stone, faint strata, a few lighter mineral patches and thin veins, a roughness
    range, rounded edges (Bevel node) plus medium bump for the normal map, and - when `glow` is given -
    a soft emissive halo on the rock around each crystal cluster."""
    pal = {k: hs["hex_color"](v) for k, v in PALETTES[pal_name].items()}
    mat = bpy.data.materials.new(name)
    nt = mat.node_tree
    bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
    coord = nt.nodes.new("ShaderNodeTexCoord")
    base_map = nt.nodes.new("ShaderNodeMapping")
    base_map.inputs["Location"].default_value = (seed * 1.371, seed * 0.713, seed * 2.137)
    nt.links.new(coord.outputs["Object"], base_map.inputs["Vector"])
    pos = base_map.outputs["Vector"]

    def noise_tex(scale, detail, rough, offset=(0.0, 0.0, 0.0), stretch=(1.0, 1.0, 1.0)):
        m = nt.nodes.new("ShaderNodeMapping")
        m.inputs["Location"].default_value = offset
        m.inputs["Scale"].default_value = stretch
        nt.links.new(pos, m.inputs["Vector"])
        t = nt.nodes.new("ShaderNodeTexNoise")
        t.inputs["Scale"].default_value = scale
        t.inputs["Detail"].default_value = detail
        t.inputs["Roughness"].default_value = rough
        nt.links.new(m.outputs["Vector"], t.inputs["Vector"])
        return t.outputs["Factor"]

    # Broad tone: big patches of dark / mid / light stone.
    col = _ramp(nt, noise_tex(2.3, 3.0, 0.55), [(0.38, pal["dark"]), (0.50, pal["mid"]), (0.62, pal["light"])])
    # Faint strata: a noise stretched along one axis reads as layered rock at a distance.
    strata = _ramp(nt, noise_tex(1.5, 2.0, 0.5, offset=(7.0, 3.0, 1.0), stretch=(0.5, 0.7, 5.0)),
                   [(0.45, 0.0), (0.55, 1.0)])
    col = _mix(nt, _math(nt, "MULTIPLY", strata, 0.30), col, pal["dark"])
    # A few lighter mineral patches with a defined edge.
    patch = _ramp(nt, noise_tex(1.8, 2.0, 0.5, offset=(-4.0, 9.0, 5.0)), [(0.615, 0.0), (0.655, 1.0)])
    col = _mix(nt, _math(nt, "MULTIPLY", patch, 0.8), col, pal["patch"])

    # A few thin light veins: iso-lines of a distorted noise (they wander like real veins), gated by
    # a broad noise so only some stretches survive.
    def vein_lines(scale, offset, width):
        m = nt.nodes.new("ShaderNodeMapping")
        m.inputs["Location"].default_value = offset
        nt.links.new(pos, m.inputs["Vector"])
        t = nt.nodes.new("ShaderNodeTexNoise")
        t.inputs["Scale"].default_value = scale
        t.inputs["Detail"].default_value = 1.5
        t.inputs["Roughness"].default_value = 0.45
        t.inputs["Distortion"].default_value = 0.7
        nt.links.new(m.outputs["Vector"], t.inputs["Vector"])
        d = _math(nt, "ABSOLUTE", _math(nt, "SUBTRACT", t.outputs["Factor"], 0.5))
        return _ramp(nt, d, [(0.0, 1.0), (width, 0.0)])

    gate = _ramp(nt, noise_tex(1.1, 1.0, 0.5, offset=(11.0, -6.0, 2.0)), [(0.56, 0.0), (0.63, 1.0)])
    vein = _math(nt, "MULTIPLY", vein_lines(1.25, (2.0, 8.0, -3.0), 0.010), gate)
    col = _mix(nt, _math(nt, "MULTIPLY", vein, 0.8), col, pal["vein"])
    # Worn, lighter fracture edges and darker creases, from the stored convexity.
    attr = nt.nodes.new("ShaderNodeAttribute")
    attr.attribute_name = "curv"
    curv = attr.outputs.get("Factor") or attr.outputs.get("Fac")
    chips = _ramp(nt, noise_tex(9.0, 2.0, 0.6, offset=(4.0, -2.0, 7.0)), [(0.40, 0.0), (0.58, 1.0)])
    edge = _math(nt, "MULTIPLY", _ramp(nt, curv, [(0.03, 0.0), (0.16, 1.0)]), chips)
    crease = _ramp(nt, _math(nt, "MULTIPLY", curv, -1.0), [(0.006, 0.0), (0.05, 1.0)])
    col = _mix(nt, _math(nt, "MULTIPLY", edge, 0.55), col, pal["light"])
    col = _mix(nt, _math(nt, "MULTIPLY", crease, 0.60), col, pal["dark"])
    nt.links.new(col, bsdf.inputs["Base Color"])

    # Roughness range 0.78..0.94, veins and patches a little glossier.
    rough = nt.nodes.new("ShaderNodeMapRange")
    nt.links.new(noise_tex(5.0, 2.0, 0.5, offset=(3.0, 3.0, 3.0)), rough.inputs[0])
    rough.inputs[3].default_value = 0.78
    rough.inputs[4].default_value = 0.94
    r = _math(nt, "SUBTRACT", rough.outputs[0], _math(nt, "MULTIPLY", vein, 0.30))
    r = _math(nt, "SUBTRACT", r, _math(nt, "MULTIPLY", patch, 0.08))
    r = _math(nt, "SUBTRACT", r, _math(nt, "MULTIPLY", edge, 0.10))
    nt.links.new(r, bsdf.inputs["Roughness"])
    bsdf.inputs["Metallic"].default_value = 0.0

    # Normal: rounded fracture edges (Bevel node) + medium bump + a little fine grain.
    bevel = nt.nodes.new("ShaderNodeBevel")
    bevel.samples = 8
    bevel.inputs["Radius"].default_value = 0.007
    b1 = nt.nodes.new("ShaderNodeBump")
    b1.inputs["Strength"].default_value = 0.40
    b1.inputs["Distance"].default_value = 0.012
    nt.links.new(noise_tex(6.5, 4.0, 0.55, offset=(1.0, 2.0, 3.0)), b1.inputs["Height"])
    nt.links.new(bevel.outputs["Normal"], b1.inputs["Normal"])
    b2 = nt.nodes.new("ShaderNodeBump")
    b2.inputs["Strength"].default_value = 0.14
    b2.inputs["Distance"].default_value = 0.003
    nt.links.new(noise_tex(19.0, 3.0, 0.5, offset=(5.0, 1.0, 8.0)), b2.inputs["Height"])
    nt.links.new(b1.outputs["Normal"], b2.inputs["Normal"])
    nt.links.new(b2.outputs["Normal"], bsdf.inputs["Normal"])

    # Emissive halo around the crystal clusters (colour only; the strength lives in the material).
    if glow is not None and anchors:
        total = None
        for a in anchors:
            dist = nt.nodes.new("ShaderNodeVectorMath")
            dist.operation = "DISTANCE"
            nt.links.new(coord.outputs["Object"], dist.inputs[0])
            dist.inputs[1].default_value = tuple(a)
            fall = nt.nodes.new("ShaderNodeMapRange")
            nt.links.new(dist.outputs["Value"], fall.inputs[0])
            fall.inputs[1].default_value = 0.0
            fall.inputs[2].default_value = glow_radius
            fall.inputs[3].default_value = 1.0
            fall.inputs[4].default_value = 0.0
            f = _math(nt, "POWER", fall.outputs[0], 2.0)
            total = f if total is None else _math(nt, "ADD", total, f, clamp=True)
        halo = _mix(nt, _math(nt, "MULTIPLY", total, 0.15), (0.0, 0.0, 0.0), glow)
        nt.links.new(halo, bsdf.inputs["Emission Color"])
        bsdf.inputs["Emission Strength"].default_value = EMISSION_STRENGTH
    else:
        bsdf.inputs["Emission Color"].default_value = (0.0, 0.0, 0.0, 1.0)
        bsdf.inputs["Emission Strength"].default_value = 0.0
    return mat


def fill_bake_misses(short, rounds=10):
    """A ray that misses the source leaves the texel at the image's start value: black base colour
    and zero AO / roughness (a dark, shiny speck beside a sharp corner or a crack rim). No real texel
    of these palettes is that dark, so treat near-black base colour as a miss and grow the
    neighbouring colours into it, in the base-colour and ORM maps alike."""
    bc = bpy.data.images.get("T_{}_BC".format(short))
    orm = bpy.data.images.get("T_{}_ORM".format(short))
    if bc is None or orm is None:
        return {"status": "skipped"}
    w, h = bc.size
    arrs = []
    for img in (bc, orm):
        a = np.empty(w * h * 4, dtype=np.float32)
        img.pixels.foreach_get(a)
        arrs.append(a.reshape(h, w, 4))
    miss = arrs[0][:, :, :3].max(axis=2) < 0.04
    first = int(miss.sum())
    for _ in range(rounds):
        if not miss.any():
            break
        good = (~miss).astype(np.float32)
        weight = np.zeros((h, w), dtype=np.float32)
        acc = [np.zeros((h, w, 3), dtype=np.float32) for _ in arrs]
        for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            g = np.roll(good, (dy, dx), axis=(0, 1))
            weight += g
            for k, a in enumerate(arrs):
                acc[k] += np.roll(a[:, :, :3] * good[:, :, None], (dy, dx), axis=(0, 1))
        fill = miss & (weight > 0)
        for k, a in enumerate(arrs):
            a[:, :, :3][fill] = acc[k][fill] / weight[fill][:, None]
        miss = miss & ~fill
    for img, a in zip((bc, orm), arrs):
        img.pixels.foreach_set(a.ravel())
        img.save()
    return {"status": "ok", "texels_missed": first, "texels_left": int(miss.sum())}


def cool_cavities(short, strength=0.8, ao_hi=0.82, ao_lo=0.32):
    """Tint the baked cavities darker and bluer in the base-colour map, using the baked AO
    (ORM red channel): the kit's 'cooler shadows'. Deterministic and noise-free."""
    bc = bpy.data.images.get("T_{}_BC".format(short))
    orm = bpy.data.images.get("T_{}_ORM".format(short))
    if bc is None or orm is None:
        return {"status": "skipped"}
    w, h = bc.size
    a = np.empty(w * h * 4, dtype=np.float32)
    o = np.empty(w * h * 4, dtype=np.float32)
    bc.pixels.foreach_get(a)
    orm.pixels.foreach_get(o)
    a = a.reshape(h, w, 4)
    ao = o.reshape(h, w, 4)[:, :, 0]
    t = np.clip((ao_hi - ao) / (ao_hi - ao_lo), 0.0, 1.0)
    k = (t * t * (3.0 - 2.0 * t)) * strength
    for ch in range(3):
        a[:, :, ch] *= 1.0 + k * (CAVITY_COOL[ch] - 1.0)
    bc.pixels.foreach_set(a.ravel())
    bc.save()
    return {"status": "ok", "share_of_texels_tinted": round(float((k > 0.02).mean()), 4)}


def emissive_surface_share(me, short, threshold=0.5):
    """Share of the game mesh's SURFACE AREA whose emissive texel (at the face centre) is bright:
    the crystals themselves, not the faint halo. The art rule is 5% or less."""
    img = bpy.data.images.get("T_{}_E".format(short))
    if img is None:
        return 0.0
    w, h = img.size
    px = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(px)
    px = px.reshape(h, w, 4)
    uv = me.uv_layers.active.data
    lit = total = 0.0
    for poly in me.polygons:
        u = sum(uv[li].uv.x for li in poly.loop_indices) / poly.loop_total
        v = sum(uv[li].uv.y for li in poly.loop_indices) / poly.loop_total
        total += poly.area
        if px[min(h - 1, int(v * h)), min(w - 1, int(u * w)), :3].max() > threshold:
            lit += poly.area
    return round(lit / total, 4)


def image_stats(short):
    out = {}
    for suffix in ("BC", "N", "ORM", "E"):
        img = bpy.data.images.get("T_{}_{}".format(short, suffix))
        if img is None:
            continue
        w, h = img.size
        px = np.empty(w * h * 4, dtype=np.float32)
        img.pixels.foreach_get(px)
        px = px.reshape(-1, 4)[:, :3]
        info = {"size": [w, h], "mean_rgb": [round(float(v), 4) for v in px.mean(axis=0)],
                "min": round(float(px.min()), 4), "max": round(float(px.max()), 4)}
        if suffix == "E":
            info["lit_share"] = round(float((px.max(axis=1) > 0.2).mean()), 4)
        if suffix == "N":
            info["std_rg"] = [round(float(px[:, 0].std()), 4), round(float(px[:, 1].std()), 4)]
        out[suffix] = info
    return out


# --------------------------------------------------------------------------- the build


def build(asset, stage="full", wip_dir=None):
    t0 = time.time()
    spec = dict(SPECS[asset])
    short = asset[3:]
    spec["short"] = short
    rng = random.Random(spec["seed"])
    asset_dir = os.path.join(HERE, asset)
    os.makedirs(asset_dir, exist_ok=True)
    os.makedirs(REVIEW_DIR, exist_ok=True)
    coll = rocks["ensure_collection"]("SpaceRocks")
    src_coll = rocks["ensure_collection"]("SpaceRocks_BakeSource")

    # ---- 1. high-poly source
    lobes = [bm_to_object("lobe_{}".format(i), build_lobe(lobe, rng), src_coll)
             for i, lobe in enumerate(spec["lobes"])]
    cleft_info = None
    for w in spec.get("wedges", []):
        stations, top = wedge_cut(lobes, w, rng, src_coll)
        if w.get("is_cleft"):
            cleft_info = {"stations": stations, "top": top, "frame": cleft_frame(w)}
    high = fuse(lobes, "SRC_{}_High".format(short), src_coll)
    _m1, n1 = normalise([high], mesh_points(high.data))
    if cleft_info:
        # Directions are unchanged by a uniform scale + translation; positions are not.
        s = TARGET_RADIUS_M / n1["radius_before"]
        c0 = Vector(n1["centre_before"])
        along, up, side = cleft_info["frame"]
        for st_ in cleft_info["stations"]:
            st_["s"] = (st_["s"] - c0.dot(along)) * s
            st_["y"] = (st_["y"] - c0.dot(side)) * s
            st_["depth"] *= s
        cleft_info["top"] = (cleft_info["top"] - c0.dot(up)) * s
    remesh(high, spec["voxel"])
    scallop(high, spec["scallop"], rng)
    high_stats = cleanup_mesh(high.data)
    high_tris = tri_count(high.data)
    log(asset, "source:", high_tris, "tris", high_stats)

    # ---- 2. game mesh
    low = high.copy()
    low.data = high.data.copy()
    low.name = asset
    low.data.name = asset
    coll.objects.link(low)
    bake_mat = bpy.data.materials.new("M_{}_BakeTarget".format(short))
    low.data.materials.append(bake_mat)
    crystal_tris = 0
    if spec.get("crystals"):
        n_c = sum(c["count"] for c in spec["crystals"].get("clusters", [])) + \
            sum(c[2] for c in spec["crystals"].get("in_cleft", []))
        crystal_tris = n_c * 46
    low_stats = decimate(low, spec["tris"] - crystal_tris)
    log("decimated:", tri_count(low.data), low_stats)
    # Measured before the crystals go in (their roots are buried in the rock by design).
    dev = deviation(low, [high])
    sources = [high]
    anchors = []
    if spec.get("crystals"):
        src_c, anchors = add_crystals(low, spec, rng, src_coll, cleft_info)
        if src_c is not None:
            sources.append(src_c)
    m2, norm_info = normalise([low] + sources, mesh_points(low.data))
    anchors = [m2 @ a for a in anchors]

    # ---- 3. hard edges, UVs, normals
    mark_sharp(low.data, SHARP_ANGLE_DEG)
    uv_method = rocks["smart_uv"](low, angle_deg=66.0, island_margin=0.02)
    if uv_method != "smart_project":
        raise RuntimeError("smart UV project failed: " + uv_method)
    islands = split_uv_at_sharp(low.data)
    uv_margin = 0.008 if spec["tex"] >= 1024 else 0.012
    pack_uv(low, margin=uv_margin)
    uv_detached = fix_uv_overlaps(low, uv_margin)
    hs["add_weighted_normal"](low, keep_sharp=True)
    hs["apply_modifiers"](low)
    low.data.name = asset
    high.data.shade_smooth()
    high.data.set_sharp_from_angle(angle=math.radians(28.0))

    pts = mesh_points(low.data)
    radius = float(np.sqrt((pts * pts).sum(axis=1).max()))
    geo = {
        "tris": tri_count(low.data), "verts": len(low.data.vertices),
        "bounding_sphere_radius_m_from_pivot": radius,
        "bounding_sphere_diameter_cm": round(radius * 200.0, 4),
        "aabb_size_cm": [round(float(v) * 100.0, 2) for v in (pts.max(axis=0) - pts.min(axis=0))],
        "uv_islands": islands + uv_detached, "uv_faces_detached_to_fix_overlap": uv_detached,
        "uv": uv_stats(low.data, 2048),
        "triangle_quality": triangle_quality(low.data),
        "source_tris": high_tris, "source_closed": high_stats,
        "normalisation": norm_info,
    }
    log("geometry:", json.dumps(geo))

    if stage == "shape":
        out = wip_dir or os.path.join(HERE, "_wip")
        rep = ar["main"](targets=[asset], tri_budget=3500)
        log("issues:", json.dumps(rep["issues"]))
        r = rv["main"](targets=[asset], out_dir=out, label=asset, mode="clay", resolution=384)
        log("clay sheet:", r["sheet"])
        return {"asset": asset, "geo": geo, "sheet": r["sheet"], "seconds": round(time.time() - t0, 1)}

    # ---- 4. source materials + bake
    colour = spec.get("crystals", {}).get("colour")
    glow = hs["hex_color"](colour["glow"]) if colour else None
    rock_mat = make_source_rock_material("M_{}_Source".format(short), spec["palette"], spec["seed"],
                                         glow=glow, anchors=anchors)
    high.data.materials.clear()
    high.data.materials.append(rock_mat)
    log("curvature:", store_curvature(high.data))
    if len(sources) > 1:
        crystal_mat = rocks["make_material"]("M_{}_SourceCrystal".format(short), hs["hex_color"](colour["base"]),
                                             roughness=0.18, metallic=0.0, emission=glow,
                                             emission_strength=EMISSION_STRENGTH)
        sources[1].data.materials.clear()
        sources[1].data.materials.append(crystal_mat)
    # Cage just outside the source surface (99% of it, and 99.5% of the game mesh's own distance to
    # it). A big cage makes rays near hard edges and thin crystals start beyond the neighbouring
    # surface and pick up the wrong one; a small one starts inside the rock.
    cage = min(0.016, max(0.004, max(dev["p99_m"], dev["game_to_source_p995_m"]) * 1.3 + 0.0015))
    log("deviation:", dev, "cage:", cage)
    bake = bk["main"](target=asset, out_dir=asset_dir, size=spec["tex"], samples=32, ao_samples=64, margin=16,
                      source_objects=[o.name for o in sources], cage_extrusion=cage, max_ray_distance=0.0,
                      build_preview_material=True)
    log("bake:", json.dumps(bake))
    misses = fill_bake_misses(short)
    log("bake misses:", misses)
    cavity = cool_cavities(short)
    preview = low.data.materials[0]
    preview.name = "M_{}_Rock".format(short)
    if bake_mat.users == 0:
        bpy.data.materials.remove(bake_mat)
    for o in sources:
        o.hide_render = True
        o.hide_viewport = True

    # ---- 5. collision, save, checks, review renders, export
    collision = []
    if spec["collision"]:
        hull = ex["_hull_object"]("UCX_{}_00".format(asset), [v.co.copy() for v in low.data.vertices], coll,
                                  max_points=32)
        hull.parent = low
        hull.display_type = "WIRE"
        collision.append(hull.name)
    blend_path = os.path.join(asset_dir, asset + ".blend")
    bpy.ops.wm.save_as_mainfile(filepath=blend_path, compress=True)
    bpy.ops.file.make_paths_relative()
    bpy.ops.wm.save_mainfile()
    if os.path.exists(blend_path + "1"):
        os.remove(blend_path + "1")

    budget = (1500, 3500) if spec["collision"] else (200, 600)
    report = ar["main"](targets=[asset], tri_budget=budget[1])
    assembly = ac["main"](targets=[asset])
    clay = rv["main"](targets=[asset], out_dir=REVIEW_DIR, label=asset, mode="clay")
    material = rv["main"](targets=[asset], out_dir=REVIEW_DIR, label=asset, mode="material")
    wire = rv["main"](targets=[asset], out_dir=REVIEW_DIR, label=asset, mode="wireframe",
                      views=("front_3q", "top", "back_3q"), resolution=448)
    manifest = ex["main"](root=asset, out_dir=asset_dir, collision="auto" if spec["collision"] else "none")
    # Extra, kit-specific facts for whoever imports this (the skill's manifest keys are untouched).
    man_path = os.path.join(asset_dir, asset + ".manifest.json")
    with open(man_path, encoding="utf-8") as fh:
        man = json.load(fh)
    man["fbx_relative"] = asset + ".fbx"
    man["sizing_contract"] = {"bounding_sphere_diameter_cm": 100.0, "pivot": "centre of the bounding sphere",
                              "measured_diameter_cm_in_blend": geo["bounding_sphere_diameter_cm"]}
    man["emission_strength"] = bake["emission_strength"]
    man["texture_size"] = spec["tex"]
    with open(man_path, "w", encoding="utf-8") as fh:
        json.dump(man, fh, indent=2)

    low_me = low.data
    checks = {
        "tris_in_budget": budget[0] <= geo["tris"] <= budget[1],
        "single_material_slot": len(low_me.materials) == 1,
        "closed_manifold": all(o.get("closed") for o in report["objects"] if o["name"] == asset),
        "no_ngons_all_triangles": all(len(p.vertices) == 3 for p in low_me.polygons),
        "no_floating_parts": bool(assembly.get("ok")),
        "bounding_sphere_100cm": abs(geo["bounding_sphere_diameter_cm"] - 100.0) < 0.01,
        "uv_inside_0_1": geo["uv"]["inside_0_1"],
        "uv_no_overlap": geo["uv"]["overlap_pixels"] == 0,
        "asset_report_errors": report["totals"]["errors"],
        "asset_report_warnings": report["totals"]["warnings"],
    }
    health = {
        "asset": asset, "checks": checks, "geometry": geo, "emissive_surface_share": emissive_surface_share(low.data, short),
        "bake": bake, "bake_miss_fill": misses, "cavity_tint": cavity,
        "bake_cage_m": cage, "deviation_source_to_game_mesh_m": dev, "textures": image_stats(short),
        "collision": collision, "asset_report": report, "assembly_check": assembly,
        "review_sheets": [os.path.basename(clay["sheet"]), os.path.basename(material["sheet"]),
                          os.path.basename(wire["sheet"])],
        "export": {k: manifest.get(k) for k in ("status", "tris", "size_cm", "collision", "collision_source",
                                                "materials", "textures", "warnings", "errors", "welded_verts")},
        "seconds": round(time.time() - t0, 1),
    }
    with open(os.path.join(REVIEW_DIR, asset + "_mesh_health.json"), "w", encoding="utf-8") as fh:
        json.dump(health, fh, indent=2)
    log("CHECKS", asset, json.dumps(checks), "emissive surface share", health["emissive_surface_share"])
    log("ISSUES", json.dumps(report["issues"]))
    return health


def _cli():
    import argparse
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser(description="Build one SpaceRocks asset")
    p.add_argument("--asset", required=True, choices=sorted(SPECS))
    p.add_argument("--stage", default="full", choices=["shape", "full"])
    p.add_argument("--wip-dir")
    a = p.parse_args(argv)
    res = build(a.asset, a.stage, a.wip_dir)
    print("__RESULT__" + json.dumps({"asset": a.asset, "seconds": res.get("seconds"),
                                     "checks": res.get("checks"), "sheet": res.get("sheet")}))


if __name__ == "__main__":
    _cli()

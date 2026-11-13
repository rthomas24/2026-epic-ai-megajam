# SPDX-License-Identifier: MIT
"""
nf_lib.py - shared pipeline for the Null Fleet enemy ships (copied from SpaceDressing sd_lib.py) (Triad Drift), Blender 5.2, headless.

Everything an asset builder needs after it has made its parts:
  - part helpers: stamp a local pattern coordinate ("lc") on every part, place, join, centre of mass
  - procedural "role" materials (paint / hazard / solar cells / foil / bare metal / glow) driven by
    object coordinates and the per-part "lc" attribute, all Cycles-bake friendly (no AO or Bevel
    shader nodes, which would be noisy in the bake script's 1-sample EMIT passes)
  - face attribute "edge" (narrow convex chamfer strips) for edge wear
  - UV unwrap + pack + overlap repair, bake to BC / N / ORM / E with the repo's bake_texture_set.py,
    cavity tint from the baked AO, bake-miss fill
  - review: clay sheet, material sheet and a "game distance" strip (black space + bright point light,
    bright nebula), stacked into one review PNG
  - UCX_ convex hulls, export through the repo's export_for_unreal.py, health JSON

The repo's helper scripts (.claude/skills/blender-*/scripts) are used as they are; nothing in them is
modified. Units: metres in Blender (1 m = 100 Unreal units), front = -Y, up = +Z.
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
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
REVIEW_DIR = os.path.join(KIT, "review")


def _repo_root():
    d = HERE
    while True:
        if os.path.isdir(os.path.join(d, ".claude", "skills")):
            return d
        parent = os.path.dirname(d)
        if parent == d:
            raise RuntimeError("repo root (.claude/skills) not found above " + HERE)
        d = parent


REPO = _repo_root()
SKILLS = os.path.join(REPO, ".claude", "skills")
hs = runpy.run_path(os.path.join(SKILLS, "blender-game-asset", "scripts", "hardsurface.py"))
rocks = runpy.run_path(os.path.join(SKILLS, "blender-game-asset", "scripts", "procedural_rocks.py"))
bk = runpy.run_path(os.path.join(SKILLS, "blender-texturing", "scripts", "bake_texture_set.py"))
ex = runpy.run_path(os.path.join(SKILLS, "blender-unreal-export", "scripts", "export_for_unreal.py"))
rv = runpy.run_path(os.path.join(SKILLS, "blender-mcp", "scripts", "review_renders.py"))
ar = runpy.run_path(os.path.join(SKILLS, "blender-mcp", "scripts", "asset_report.py"))
ac = runpy.run_path(os.path.join(SKILLS, "blender-mcp", "scripts", "assembly_check.py"))

EMISSION_STRENGTH = 2.0      # Blender-side strength for every glow (<= 3 keeps colours saturated)


def log(*a):
    print("[nullfleet]", *a)
    sys.stdout.flush()


def H(hexstr):
    return hs["hex_color"](hexstr)


def tri_count(me):
    return sum(len(p.vertices) - 2 for p in me.polygons)


def mesh_points(me):
    co = np.empty(len(me.vertices) * 3, dtype=np.float64)
    me.vertices.foreach_get("co", co)
    return co.reshape(-1, 3)


def ensure_collection(name):
    return hs["ensure_collection"](name)


def bm_to_object(name, bm, coll, uv=True):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    obj = bpy.data.objects.new(name, me)
    coll.objects.link(obj)
    return obj


def remove_object(obj):
    me = obj.data
    bpy.data.objects.remove(obj, do_unlink=True)
    if me is not None and me.users == 0:
        bpy.data.meshes.remove(me)


_uid = [0]


def uname(prefix):
    _uid[0] += 1
    return "{}_{:04d}".format(prefix, _uid[0])


# =========================================================================== geometry primitives
# All return a NEW object in local coordinates (not yet placed). Smooth/flat shading is set by the
# finishing step (bevel -> harden normals, or smooth_by_angle).

def obj_from_bm(bm, coll, name=None):
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    return bm_to_object(name or uname("part"), bm, coll)


def box(coll, size, center=(0.0, 0.0, 0.0)):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0, matrix=Matrix.Translation(center) @ Matrix.Diagonal((*size, 1.0)))
    return obj_from_bm(bm, coll)


def cyl(coll, r, depth, segments=16, axis="Z", center=(0.0, 0.0, 0.0), r_top=None):
    bm = bmesh.new()
    rot = {"X": Matrix.Rotation(math.radians(90), 4, "Y"), "Y": Matrix.Rotation(math.radians(-90), 4, "X"),
           "Z": Matrix.Identity(4)}[axis]
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=segments, radius1=r,
                          radius2=r if r_top is None else r_top, depth=depth,
                          matrix=Matrix.Translation(center) @ rot)
    return obj_from_bm(bm, coll)


def lathe(coll, profile, segments=24, axis="Z", arc=None, center=(0.0, 0.0, 0.0)):
    """Revolve a closed or open (pos, radius) profile around `axis`. radius 0 closes to a point.
    `arc` = (a0, a1) in degrees for a partial revolution (ends capped)."""
    profile = list(profile)
    if profile[0] != profile[-1]:            # open outline: close both ends onto the axis
        if profile[0][1] > 1e-9:
            profile.insert(0, (profile[0][0], 0.0))
        if profile[-1][1] > 1e-9:
            profile.append((profile[-1][0], 0.0))
    bm = bmesh.new()
    full = arc is None
    a0, a1 = (0.0, 360.0) if full else arc
    steps = segments if full else max(1, segments)
    rings = []
    count = steps if full else steps + 1
    for i in range(count):
        a = math.radians(a0 + (a1 - a0) * i / float(steps))
        c, s = math.cos(a), math.sin(a)
        ring = []
        for pos, r in profile:
            if axis == "Z":
                p = Vector((r * c, r * s, pos))
            elif axis == "Y":
                p = Vector((r * c, pos, r * s))
            else:
                p = Vector((pos, r * c, r * s))
            ring.append(bm.verts.new(p + Vector(center)))
        rings.append(ring)
    n = len(profile)
    pairs = list(zip(rings, rings[1:])) + ([(rings[-1], rings[0])] if full else [])
    for ra, rb in pairs:
        for j in range(n - 1):
            quad = [ra[j], ra[j + 1], rb[j + 1], rb[j]]
            uniq = []
            for v in quad:
                if v not in uniq:
                    uniq.append(v)
            if len(uniq) >= 3:
                try:
                    bm.faces.new(uniq)
                except ValueError:
                    pass
    if not full:
        for ring in (rings[0], rings[-1]):
            vs = []
            for v in ring:
                if v not in vs:
                    vs.append(v)
            if len(vs) >= 3:
                try:
                    bm.faces.new(vs)
                except ValueError:
                    pass
    bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=1e-6)
    return obj_from_bm(bm, coll)


def revolve_closed(coll, profile, arc, steps, axis="Y"):
    """Revolve a CLOSED polygon profile [(r, a), ...] (radius, axial) over arc (deg) -> a capped
    ring segment (torus-like solid section). Used for the gate ring."""
    bm = bmesh.new()
    a0, a1 = arc
    full = abs(a1 - a0) >= 359.999
    rings = []
    for i in range(steps if full else steps + 1):
        t = math.radians(a0 + (a1 - a0) * i / float(steps))
        c, s = math.cos(t), math.sin(t)
        ring = []
        for r, ax in profile:
            if axis == "Y":
                ring.append(bm.verts.new((r * c, ax, r * s)))
            else:
                ring.append(bm.verts.new((r * c, r * s, ax)))
        rings.append(ring)
    n = len(profile)
    pairs = list(zip(rings, rings[1:])) + ([(rings[-1], rings[0])] if full else [])
    for ra, rb in pairs:
        for j in range(n):
            bm.faces.new((ra[j], ra[(j + 1) % n], rb[(j + 1) % n], rb[j]))
    if not full:
        bm.faces.new(rings[0])
        bm.faces.new(list(reversed(rings[-1])))
    return obj_from_bm(bm, coll)


def extrude_profile(coll, profile, length, segments=1, axis="Y"):
    """Extrude a closed 2D profile [(u, v), ...] along `axis` (centred), with `segments` length
    subdivisions (so the part can be bent later). For axis Y the profile is (x, z)."""
    bm = bmesh.new()
    rings = []
    for i in range(segments + 1):
        t = -length * 0.5 + length * i / float(segments)
        ring = []
        for u, v in profile:
            if axis == "Y":
                ring.append(bm.verts.new((u, t, v)))
            elif axis == "X":
                ring.append(bm.verts.new((t, u, v)))
            else:
                ring.append(bm.verts.new((u, v, t)))
        rings.append(ring)
    n = len(profile)
    for ra, rb in zip(rings, rings[1:]):
        for j in range(n):
            bm.faces.new((ra[j], ra[(j + 1) % n], rb[(j + 1) % n], rb[j]))
    bm.faces.new(rings[0])
    bm.faces.new(list(reversed(rings[-1])))
    return obj_from_bm(bm, coll)


def ibeam_profile(h, w, t_web, t_flange):
    hw, hh = w * 0.5, h * 0.5
    tw = t_web * 0.5
    return [(-hw, -hh), (hw, -hh), (hw, -hh + t_flange), (tw, -hh + t_flange), (tw, hh - t_flange),
            (hw, hh - t_flange), (hw, hh), (-hw, hh), (-hw, hh - t_flange), (-tw, hh - t_flange),
            (-tw, -hh + t_flange), (-hw, -hh + t_flange)]


def chamfer_rect(w, h, c):
    hw, hh = w * 0.5, h * 0.5
    return [(-hw + c, -hh), (hw - c, -hh), (hw, -hh + c), (hw, hh - c), (hw - c, hh), (-hw + c, hh),
            (-hw, hh - c), (-hw, -hh + c)]


def bar(coll, p0, p1, w, h=None, up=(0.0, 0.0, 1.0), segments=1, profile=None):
    """Rectangular (or custom-profile) member from p0 to p1. Returns (obj, matrix) - the object is
    built along local Y and `matrix` places it; pass matrix to Part.add(..., matrix=matrix)."""
    p0, p1 = Vector(p0), Vector(p1)
    d = p1 - p0
    L = d.length
    prof = profile or chamfer_rect(w, h if h is not None else w, min(w, h or w) * 0.12)
    obj = extrude_profile(coll, prof, L, segments=segments, axis="Y")
    y = d.normalized()
    upv = Vector(up)
    if abs(y.dot(upv.normalized())) > 0.98:
        upv = Vector((1.0, 0.0, 0.0)) if abs(y.x) < 0.9 else Vector((0.0, 1.0, 0.0))
    x = y.cross(upv).normalized()
    z = x.cross(y).normalized()
    m = Matrix((x, y, z)).transposed().to_4x4()
    m.translation = (p0 + p1) * 0.5
    return obj, m


def tube_path(coll, points, radius, sides=8, cap=True, radius_fn=None):
    """Sweep a circle along a polyline (parallel-transport frames). Cables, pipes, conduits."""
    pts = [Vector(p) for p in points]
    bm = bmesh.new()
    tangents = []
    for i in range(len(pts)):
        a = pts[max(i - 1, 0)]
        b = pts[min(i + 1, len(pts) - 1)]
        tangents.append((b - a).normalized())
    ref = Vector((0.0, 0.0, 1.0)) if abs(tangents[0].z) < 0.9 else Vector((1.0, 0.0, 0.0))
    nrm = tangents[0].cross(ref).normalized()
    rings = []
    for i, (p, t) in enumerate(zip(pts, tangents)):
        if i > 0:
            prev = tangents[i - 1]
            axis = prev.cross(t)
            if axis.length > 1e-8:
                ang = prev.angle(t)
                nrm = (Matrix.Rotation(ang, 3, axis.normalized()) @ nrm).normalized()
        bin_ = t.cross(nrm).normalized()
        r = radius * (radius_fn(i / max(1, len(pts) - 1)) if radius_fn else 1.0)
        ring = []
        for k in range(sides):
            a = math.tau * k / sides
            ring.append(bm.verts.new(p + (nrm * math.cos(a) + bin_ * math.sin(a)) * r))
        rings.append(ring)
    for ra, rb in zip(rings, rings[1:]):
        for k in range(sides):
            bm.faces.new((ra[k], ra[(k + 1) % sides], rb[(k + 1) % sides], rb[k]))
    if cap:
        bm.faces.new(list(reversed(rings[0])))
        bm.faces.new(rings[-1])
    return obj_from_bm(bm, coll)


def bezier_points(p0, p1, p2, p3, n):
    p0, p1, p2, p3 = map(Vector, (p0, p1, p2, p3))
    out = []
    for i in range(n + 1):
        t = i / float(n)
        u = 1.0 - t
        out.append(p0 * u ** 3 + p1 * 3 * u * u * t + p2 * 3 * u * t * t + p3 * t ** 3)
    return out


def grid_plate(coll, sx, sy, nx, ny, thickness, jag=0.0, jag_sides=(1, 1, 1, 1), seed=0, holes=0.0):
    """Rectangular plate in XY (thickness along Z, centred) made of an nx * ny grid so it can be
    bent. jag > 0 pushes boundary vertices inward by random amounts on the chosen sides
    (-x, +x, -y, +y) - a torn edge. holes > 0 deletes that share of the boundary cells."""
    rng = random.Random(seed)
    bm = bmesh.new()
    verts = [[bm.verts.new((-sx * 0.5 + sx * i / nx, -sy * 0.5 + sy * j / ny, 0.0)) for j in range(ny + 1)]
             for i in range(nx + 1)]
    faces = {}
    for i in range(nx):
        for j in range(ny):
            faces[(i, j)] = bm.faces.new((verts[i][j], verts[i + 1][j], verts[i + 1][j + 1], verts[i][j + 1]))
    if holes > 0.0:
        kill = []
        for (i, j), f in faces.items():
            border = (i == 0 and jag_sides[0]) or (i == nx - 1 and jag_sides[1]) or \
                     (j == 0 and jag_sides[2]) or (j == ny - 1 and jag_sides[3])
            if border and rng.random() < holes:
                kill.append(f)
        bmesh.ops.delete(bm, geom=kill, context="FACES")
        loose = [v for v in bm.verts if not v.link_faces]
        bmesh.ops.delete(bm, geom=loose, context="VERTS")
    if jag > 0.0:
        cell = min(sx / nx, sy / ny)
        for v in bm.verts:
            if not v.is_boundary:
                continue
            x, y = v.co.x, v.co.y
            side = None
            if jag_sides[0] and x < -sx * 0.5 + 1e-4:
                side = Vector((1, 0, 0))
            elif jag_sides[1] and x > sx * 0.5 - 1e-4:
                side = Vector((-1, 0, 0))
            elif jag_sides[2] and y < -sy * 0.5 + 1e-4:
                side = Vector((0, 1, 0))
            elif jag_sides[3] and y > sy * 0.5 - 1e-4:
                side = Vector((0, -1, 0))
            if side is not None:
                v.co += side * min(0.92, rng.uniform(0.0, jag) * 1.3) * cell
                v.co += Vector((rng.uniform(-0.25, 0.25), rng.uniform(-0.25, 0.25), 0.0)) * cell
    # solidify by hand (closed shell)
    top = list(bm.faces)
    ret = bmesh.ops.solidify(bm, geom=top, thickness=thickness)
    for v in bm.verts:
        v.co.z += thickness * 0.5
    return obj_from_bm(bm, coll)


# =========================================================================== deformers

def bend(obj, axis="X", radius=10.0, along="Y", start=0.0, plane_axis="Z"):
    """Bend mesh data: coordinates along `along` beyond `start` are wrapped onto an arc of `radius`
    curving toward +plane_axis. (A simple, stable 'simple deform' replacement.)"""
    ia, ip = "XYZ".index(along), "XYZ".index(plane_axis)
    for v in obj.data.vertices:
        co = v.co.copy()
        s = co[ia] - start
        if s <= 0.0:
            continue
        theta = s / radius
        h = co[ip]
        rr = radius - h
        co[ia] = start + rr * math.sin(theta)
        co[ip] = radius - rr * math.cos(theta)
        v.co = co
    obj.data.update()


def twist_rotate_tail(obj, along="Y", start=0.0, angle_deg=20.0, axis="X", pivot=None):
    """Rotate every vertex beyond `start` along `along` by angle about `axis` through pivot: a kink."""
    ia = "XYZ".index(along)
    piv = Vector(pivot) if pivot is not None else Vector((0.0, 0.0, 0.0))
    if pivot is None:
        piv[ia] = start
    R = Matrix.Rotation(math.radians(angle_deg), 3, axis)
    for v in obj.data.vertices:
        if v.co[ia] > start:
            v.co = piv + R @ (v.co - piv)
    obj.data.update()


def jag_end(obj, along="Y", end=+1, depth=0.3, seed=0):
    """Push the end-cap vertices of a member back by random amounts: a torn, irregular tip."""
    rng = random.Random(seed)
    ia = "XYZ".index(along)
    vals = [v.co[ia] for v in obj.data.vertices]
    lim = max(vals) if end > 0 else min(vals)
    for v in obj.data.vertices:
        if abs(v.co[ia] - lim) < 1e-5:
            v.co[ia] -= end * rng.uniform(0.0, depth)
    obj.data.update()


# =========================================================================== parts

class Part:
    """Collects the parts of one asset. Each part: materials assigned, optional bevel + weighted
    normals applied, local pattern coordinate stamped (attribute 'lc'), deformed, placed."""

    def __init__(self, asset, coll_name):
        self.asset = asset
        self.coll = ensure_collection(coll_name)
        self.parts = []
        self.hulls = []          # lists of world-space points (each becomes one UCX_ hull)
        self.max_bevel = 0.0

    def add(self, obj, mat, bevel=None, segments=2, angle=30.0, smooth=None, matrix=None,
            lc_offset=None, deform=None, hull=False, wn=True):
        clean_part(obj.data)
        if not obj.data.materials:
            obj.data.materials.append(mat)
        elif mat is not None and obj.data.materials[0] is None:
            obj.data.materials[0] = mat
        if bevel:
            hs["add_bevel"](obj, width=bevel, segments=segments, angle_deg=angle)
            self.max_bevel = max(self.max_bevel, bevel)
            if wn:
                hs["add_weighted_normal"](obj)
            hs["apply_modifiers"](obj)
            mark_edge_faces(obj.data, bevel, factor=1.9)
        else:
            mark_edge_faces(obj.data, 0.0)
            if smooth is not None:
                hs["smooth_by_angle"](obj, smooth)
            else:
                obj.data.shade_flat()
        stamp_lc(obj, lc_offset)
        if deform is not None:
            deform(obj)
        if matrix is not None:
            obj.matrix_world = matrix
        if hull:
            self.hulls.append([obj.matrix_world @ v.co for v in obj.data.vertices])
        self.parts.append(obj)
        return obj

    def snap_floating(self, tol=0.01, rounds=4):
        """Any group of parts that does not touch the main body is moved (rigidly, shortest way) until
        it overlaps the body by 5 cm. Returns the moves made. Assembly safety net for tangled debris."""
        from mathutils.bvhtree import BVHTree
        moves = []
        for _ in range(rounds):
            trees, pts = [], []
            for o in self.parts:
                mw = o.matrix_world
                vs = [mw @ v.co for v in o.data.vertices]
                trees.append(BVHTree.FromPolygons(vs, [p.vertices[:] for p in o.data.polygons]))
                pts.append(vs)
            n = len(self.parts)
            parent = list(range(n))

            def find(i):
                while parent[i] != i:
                    parent[i] = parent[parent[i]]
                    i = parent[i]
                return i
            for i in range(n):
                for j in range(i + 1, n):
                    if find(i) == find(j):
                        continue
                    hit = bool(trees[i].overlap(trees[j]))
                    if not hit:
                        step = max(1, len(pts[i]) // 60)
                        for c in pts[i][::step]:
                            r = trees[j].find_nearest(c)
                            if r[0] is not None and r[3] <= tol:
                                hit = True
                                break
                    if hit:
                        parent[find(i)] = find(j)
            groups = {}
            for i in range(n):
                groups.setdefault(find(i), []).append(i)
            if len(groups) == 1:
                break
            main = max(groups.values(), key=lambda g: sum(len(pts[k]) for k in g))
            for g in groups.values():
                if g is main:
                    continue
                best = None
                for k in g:
                    step = max(1, len(pts[k]) // 200)
                    for c in pts[k][::step]:
                        for mk in main:
                            r = trees[mk].find_nearest(c)
                            if r[0] is not None and (best is None or r[3] < best[0]):
                                best = (r[3], c, r[0])
                if best is None:
                    continue
                d = best[2] - best[1]
                d = d + d.normalized() * 0.05 if d.length > 1e-6 else d
                for k in g:
                    self.parts[k].matrix_world = Matrix.Translation(d) @ self.parts[k].matrix_world
                moves.append([self.parts[k].name for k in g][:3] + [round(d.length, 3)])
        return moves

    def hull_from(self, objs):
        pts = []
        for o in objs:
            pts.extend(o.matrix_world @ v.co for v in o.data.vertices)
        self.hulls.append(pts)

    def join(self, name=None):
        """Join every part into a fresh object at the identity transform (so no custom normals
        ever pass through a rotated object matrix)."""
        name = name or self.asset
        anchor = bpy.data.objects.new(name, bpy.data.meshes.new(name))
        self.coll.objects.link(anchor)
        target = hs["join"]([anchor] + self.parts, name=name)
        self.parts = []
        return target


def clean_part(me, weld=1e-5):
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=weld)
    bmesh.ops.dissolve_degenerate(bm, edges=list(bm.edges), dist=weld)
    loose = [v for v in bm.verts if not v.link_faces]
    if loose:
        bmesh.ops.delete(bm, geom=loose, context="VERTS")
    bm.to_mesh(me)
    bm.free()
    me.update()


def stamp_lc(obj, offset=None):
    """Local pattern coordinate: the part's own vertex positions at build time, so panel lines,
    stripes and solar cells stay aligned with the part after it is bent, rotated and joined."""
    me = obj.data
    off = Vector(offset) if offset is not None else Vector((0.0, 0.0, 0.0))
    attr = me.attributes.get("lc") or me.attributes.new("lc", "FLOAT_VECTOR", "POINT")
    vals = []
    for v in me.vertices:
        p = v.co + off
        vals.extend((p.x, p.y, p.z))
    attr.data.foreach_set("vector", vals)


def place(loc=(0.0, 0.0, 0.0), rot=(0.0, 0.0, 0.0), order="XYZ"):
    rx, ry, rz = (math.radians(a) for a in rot)
    m = Matrix.Translation(Vector(loc)) @ \
        Matrix.Rotation(rz, 4, "Z") @ Matrix.Rotation(ry, 4, "Y") @ Matrix.Rotation(rx, 4, "X")
    return m


def edge_area_share(me):
    attr = me.attributes.get("edge")
    if attr is None:
        return 0.0
    vals = [d.value for d in attr.data]
    tot = sum(p.area for p in me.polygons)
    return round(sum(p.area for p, v in zip(me.polygons, vals) if v > 0.5) / max(tot, 1e-9), 4)


def center_of_mass(me):
    """Volume centroid (signed tetrahedra; overlapping closed parts are counted twice, which is
    fine for a pivot). Falls back to the area centroid for open meshes."""
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.triangulate(bm, faces=list(bm.faces))
    vol = 0.0
    acc = Vector((0.0, 0.0, 0.0))
    area = 0.0
    acc_a = Vector((0.0, 0.0, 0.0))
    for f in bm.faces:
        a, b, c = (v.co for v in f.verts)
        v6 = a.dot(b.cross(c))
        vol += v6
        acc += (a + b + c) * v6
        ar_ = f.calc_area()
        area += ar_
        acc_a += (a + b + c) / 3.0 * ar_
    bm.free()
    if abs(vol) > 1e-9:
        return acc / (4.0 * vol)
    return acc_a / max(area, 1e-12)


def recentre(obj, extra_point_lists=()):
    com = center_of_mass(obj.data)
    obj.data.transform(Matrix.Translation(-com))
    obj.data.update()
    for pts in extra_point_lists:
        for i in range(len(pts)):
            pts[i] = pts[i] - com
    return [float(c) for c in com]


def mark_edge_faces(me, width, factor=2.6):
    """FACE attribute 'edge' = 1 on narrow faces that border a convex crease: the chamfer strips the
    bevels made. Drives edge wear in the role materials (crisp, noise-free, any asset scale)."""
    bm = bmesh.new()
    bm.from_mesh(me)
    bm.faces.ensure_lookup_table()
    bm.normal_update()
    vals = []
    if width <= 0.0:
        attr = me.attributes.get("edge") or me.attributes.new("edge", "FLOAT", "FACE")
        attr.data.foreach_set("value", [0.0] * len(me.polygons))
        bm.free()
        return 0.0
    thresh = width * factor
    for f in bm.faces:
        longest = max(e.calc_length() for e in f.edges)
        narrow = 2.0 * f.calc_area() / max(longest, 1e-9)
        val = 0.0
        if narrow < thresh:
            for e in f.edges:
                if len(e.link_faces) != 2:
                    continue
                g = e.link_faces[0] if e.link_faces[1] is f else e.link_faces[1]
                ang = f.normal.angle(g.normal, 0.0)
                convex = (g.calc_center_median() - f.calc_center_median()).dot(f.normal) < 0.0
                if convex and ang > math.radians(8):
                    val = 1.0
                    break
        vals.append(val)
    bm.free()
    attr = me.attributes.get("edge") or me.attributes.new("edge", "FLOAT", "FACE")
    attr.data.foreach_set("value", vals)
    return round(sum(vals) / max(1, len(vals)), 4)


# =========================================================================== materials

class Shader:
    """Small node-graph builder on top of a Principled BSDF."""

    def __init__(self, name):
        old = bpy.data.materials.get(name)
        if old is not None:
            bpy.data.materials.remove(old)
        self.mat = bpy.data.materials.new(name)
        self.nt = self.mat.node_tree
        self.bsdf = next(n for n in self.nt.nodes if n.type == "BSDF_PRINCIPLED")
        self.bsdf.inputs["Emission Color"].default_value = (0.0, 0.0, 0.0, 1.0)
        self.bsdf.inputs["Emission Strength"].default_value = 0.0
        tc = self.nt.nodes.new("ShaderNodeTexCoord")
        self.obj = tc.outputs["Object"]
        self._attrs = {}

    # -- plumbing
    def link_or_set(self, sock, val):
        if isinstance(val, bpy.types.NodeSocket):
            self.nt.links.new(val, sock)
        elif isinstance(val, (tuple, list)):
            n = len(sock.default_value)
            sock.default_value = tuple(val[:3]) + ((1.0,) if n == 4 else ()) if n in (3, 4) else val
        else:
            sock.default_value = val

    def attr(self, name, out="Fac"):
        key = (name, out)
        if key not in self._attrs:
            n = self.nt.nodes.new("ShaderNodeAttribute")
            n.attribute_name = name
            o = {"Fac": ("Fac", "Factor"), "Vector": ("Vector",), "Color": ("Color",)}[out]
            sock = None
            for nm in o:
                sock = n.outputs.get(nm)
                if sock is not None:
                    break
            self._attrs[key] = sock
        return self._attrs[key]

    @property
    def lc(self):
        return self.attr("lc", "Vector")

    def mapping(self, vec, loc=(0, 0, 0), scale=(1, 1, 1), rot=(0, 0, 0)):
        m = self.nt.nodes.new("ShaderNodeMapping")
        m.inputs["Location"].default_value = loc
        m.inputs["Scale"].default_value = scale
        m.inputs["Rotation"].default_value = rot
        self.nt.links.new(vec, m.inputs["Vector"])
        return m.outputs["Vector"]

    def noise(self, vec, scale, detail=2.0, rough=0.5, distortion=0.0, offset=(0, 0, 0), stretch=(1, 1, 1)):
        v = self.mapping(vec, loc=offset, scale=stretch) if (any(offset) or tuple(stretch) != (1, 1, 1)) else vec
        t = self.nt.nodes.new("ShaderNodeTexNoise")
        t.inputs["Scale"].default_value = scale
        t.inputs["Detail"].default_value = detail
        t.inputs["Roughness"].default_value = rough
        t.inputs["Distortion"].default_value = distortion
        self.nt.links.new(v, t.inputs["Vector"])
        return t.outputs.get("Factor") or t.outputs.get("Fac")

    def voronoi(self, vec, scale, out="Distance", feature="F1", randomness=1.0):
        t = self.nt.nodes.new("ShaderNodeTexVoronoi")
        t.feature = feature
        t.inputs["Scale"].default_value = scale
        t.inputs["Randomness"].default_value = randomness
        self.nt.links.new(vec, t.inputs["Vector"])
        return t.outputs[out]

    def white(self, vec):
        t = self.nt.nodes.new("ShaderNodeTexWhiteNoise")
        t.noise_dimensions = "3D"
        self.nt.links.new(vec, t.inputs["Vector"])
        return t.outputs["Value"]

    def ramp(self, fac, stops):
        n = self.nt.nodes.new("ShaderNodeValToRGB")
        cr = n.color_ramp
        while len(cr.elements) < len(stops):
            cr.elements.new(0.5)
        for el, (pos, col) in zip(cr.elements, stops):
            el.position = pos
            el.color = (col, col, col, 1.0) if isinstance(col, (int, float)) else (col[0], col[1], col[2], 1.0)
        self.nt.links.new(fac, n.inputs[0])
        return n.outputs[0]

    def mix(self, fac, a, b):
        n = self.nt.nodes.new("ShaderNodeMix")
        n.data_type = "RGBA"
        n.clamp_factor = True
        self.link_or_set(n.inputs[0], fac)
        self.link_or_set(n.inputs[6], a)
        self.link_or_set(n.inputs[7], b)
        return n.outputs[2]

    def m(self, op, a, b=None, clamp=False):
        n = self.nt.nodes.new("ShaderNodeMath")
        n.operation = op
        n.use_clamp = clamp
        self.link_or_set(n.inputs[0], a)
        if b is not None:
            self.link_or_set(n.inputs[1], b)
        return n.outputs[0]

    def vm(self, op, a, b=None, scale=None):
        n = self.nt.nodes.new("ShaderNodeVectorMath")
        n.operation = op
        self.link_or_set(n.inputs[0], a)
        if b is not None:
            self.link_or_set(n.inputs[1], b)
        if scale is not None:
            n.inputs["Scale"].default_value = scale
        return n.outputs["Value"] if op in ("DOT_PRODUCT", "LENGTH", "DISTANCE") else n.outputs["Vector"]

    def sep(self, vec):
        n = self.nt.nodes.new("ShaderNodeSeparateXYZ")
        self.nt.links.new(vec, n.inputs[0])
        return n.outputs["X"], n.outputs["Y"], n.outputs["Z"]

    def maprange(self, val, a, b, c=0.0, d=1.0, clamp=True, smooth=False):
        n = self.nt.nodes.new("ShaderNodeMapRange")
        if smooth:
            n.interpolation_type = "SMOOTHSTEP"
        n.clamp = clamp
        self.link_or_set(n.inputs[0], val)
        n.inputs[1].default_value = a
        n.inputs[2].default_value = b
        n.inputs[3].default_value = c
        n.inputs[4].default_value = d
        return n.outputs[0]

    def line_dist(self, coord, spacing, offset=0.0):
        """distance (m) from a scalar coordinate to the nearest grid line of `spacing`."""
        f = self.m("FRACT", self.m("ADD", self.m("DIVIDE", coord, spacing), offset + 0.5))
        return self.m("MULTIPLY", self.m("ABSOLUTE", self.m("SUBTRACT", f, 0.5)), spacing)

    def finish(self, color, rough, metal, height=None, bump_strength=0.3, bump_distance=0.02,
               emission=None, emission_strength=0.0):
        self.link_or_set(self.bsdf.inputs["Base Color"], color)
        self.link_or_set(self.bsdf.inputs["Roughness"], rough)
        self.link_or_set(self.bsdf.inputs["Metallic"], metal)
        if height is not None:
            b = self.nt.nodes.new("ShaderNodeBump")
            b.inputs["Strength"].default_value = bump_strength
            b.inputs["Distance"].default_value = bump_distance
            self.nt.links.new(height, b.inputs["Height"])
            self.nt.links.new(b.outputs["Normal"], self.bsdf.inputs["Normal"])
        if emission is not None:
            self.link_or_set(self.bsdf.inputs["Emission Color"], emission)
            self.bsdf.inputs["Emission Strength"].default_value = emission_strength
        mc = color if isinstance(color, (tuple, list)) else (0.5, 0.5, 0.5)
        self.mat.diffuse_color = (mc[0], mc[1], mc[2], 1.0)
        return self.mat


def role_paint(name, color, color2=None, metallic=0.3, rough=0.42, wear=0.45, wear_color="#A7ADB3",
               fade=0.0, fade_color=None, grime=0.25, panel=None, stripes=None, cells=None, label=None,
               scale=1.0, bump=0.5, seed=0, scorch=0.0, dent=1.0, rough_var=0.07, var_amount=1.0):
    """Weathered paint. `scale` ~ the asset's characteristic size in metres / 10 (pattern sizes).
    panel=(spacing_m, width_m)    seams on the part's local grid (lc), darker and recessed
    stripes=(colA, colB, width_m, (dx, dy, dz))   hazard stripes on lc (replace the paint colour)
    cells=(cell_color, line_color, (su, sv), (axis_u, axis_v), line_w)   solar cell grid on lc
    label=(color, (u0, u1), (v0, v1), (axis_u, axis_v))   a painted block (stencil/ID patch) on lc"""
    s = Shader(name)
    o = Vector((seed * 3.17, seed * 1.31, seed * 2.03))
    col1 = H(color) if isinstance(color, str) else color
    col2 = H(color2) if isinstance(color2, str) else (color2 or tuple(c * 0.82 for c in col1))
    var = s.noise(s.obj, 0.55 / scale, 3.0, 0.55, offset=tuple(o))
    col = s.mix(s.m("MULTIPLY", s.ramp(var, [(0.35, 0.0), (0.68, 1.0)]), var_amount), col1, col2)
    lc = s.lc
    lx, ly, lz = s.sep(lc)
    axes = {"X": lx, "Y": ly, "Z": lz}
    metal = metallic
    rgh = rough
    if stripes is not None:
        ca, cb, width, d = stripes
        coord = s.vm("DOT_PRODUCT", lc, d)
        st = s.m("GREATER_THAN", s.m("FRACT", s.m("DIVIDE", coord, width)), 0.5)
        col = s.mix(st, H(ca), H(cb))
        col = s.mix(s.m("MULTIPLY", s.ramp(var, [(0.4, 0.0), (0.7, 1.0)]), 0.25), col, (0.0, 0.0, 0.0))
    if cells is not None:
        c_cell, c_line, (su, sv), (au, av), lw = cells
        du = s.line_dist(axes[au], su, 0.0)
        dv = s.line_dist(axes[av], sv, 0.0)
        line = s.m("MAXIMUM", s.maprange(du, lw, lw * 0.5, 0.0, 1.0), s.maprange(dv, lw, lw * 0.5, 0.0, 1.0))
        # per-cell tint: some cells darker/dead
        cid = s.vm("FLOOR", s.vm("DIVIDE", lc, (su, sv, su)))
        wn = s.white(cid)
        cellc = s.mix(s.ramp(wn, [(0.0, 0.0), (0.8, 0.0), (0.82, 1.0)]), H(c_cell), tuple(c * 0.45 for c in H(c_cell)))
        cellc = s.mix(s.m("MULTIPLY", s.ramp(wn, [(0.0, 1.0), (0.3, 0.0)]), 0.35), cellc, tuple(min(1, c * 1.6 + 0.02) for c in H(c_cell)))
        col = s.mix(line, cellc, H(c_line))
        metal = s.mix(line, (0.55, 0.55, 0.55), (0.9, 0.9, 0.9))
        rgh = s.mix(line, (0.18, 0.18, 0.18), (0.35, 0.35, 0.35))
    if label is not None:
        lcol, (u0, u1), (v0, v1), (au, av) = label
        inu = s.m("MULTIPLY", s.m("GREATER_THAN", axes[au], u0), s.m("LESS_THAN", axes[au], u1))
        inv = s.m("MULTIPLY", s.m("GREATER_THAN", axes[av], v0), s.m("LESS_THAN", axes[av], v1))
        block = s.m("MULTIPLY", inu, inv)
        # stencil break-up: worn
        block = s.m("MULTIPLY", block, s.ramp(s.noise(s.obj, 2.2 / scale, 2.0, 0.5, offset=(9, 1, 4)), [(0.25, 0.0), (0.35, 1.0)]))
        col = s.mix(block, col, H(lcol))
    if fade > 0.0:
        fc = H(fade_color) if fade_color else (0.62, 0.62, 0.6)
        f = s.ramp(s.noise(s.obj, 0.22 / scale, 2.0, 0.5, offset=(4, 7, 1)), [(0.3, 0.0), (0.75, 1.0)])
        col = s.mix(s.m("MULTIPLY", f, fade), col, fc)
    # grime streaks (stretched noise along Z) + cavity-ish darkening in panel seams
    if grime > 0.0:
        g = s.ramp(s.noise(s.obj, 1.1 / scale, 3.0, 0.6, offset=(2, 5, 8), stretch=(1.0, 1.0, 0.18)),
                   [(0.45, 0.0), (0.75, 1.0)])
        col = s.mix(s.m("MULTIPLY", g, grime), col, H("#3A3632"))
    if scorch > 0.0:
        sc_m = s.ramp(s.noise(s.obj, 0.7 / scale, 4.0, 0.65, offset=(13, 4, 2), distortion=0.4), [(0.6, 0.0), (0.72, 1.0)])
        col = s.mix(s.m("MULTIPLY", sc_m, scorch), col, H("#1D1917"))
    seam = None
    if panel is not None:
        sp, w = panel
        dmin = s.m("MINIMUM", s.m("MINIMUM", s.line_dist(lx, sp, 0.37), s.line_dist(ly, sp, 0.21)),
                   s.line_dist(lz, sp, 0.13))
        seam = s.maprange(dmin, w, w * 0.3, 0.0, 1.0)
        col = s.mix(s.m("MULTIPLY", seam, 0.75), col, tuple(c * 0.35 for c in col1))
    # edge wear: chamfer strips (face attribute) broken up by chips noise
    if wear > 0.0:
        chips = s.ramp(s.noise(s.obj, 6.0 / scale, 3.0, 0.6, offset=(1, 9, 3)), [(0.42, 0.0), (0.55, 1.0)])
        e = s.m("MULTIPLY", s.m("MULTIPLY", s.attr("edge"), chips), wear)
        e = s.m("MINIMUM", s.m("MULTIPLY", e, 2.5), 1.0)
        col = s.mix(e, col, H(wear_color))
        metal = s.mix(e, (metallic,) * 3 if not isinstance(metal, bpy.types.NodeSocket) else metal, (0.95, 0.95, 0.95))
        rgh = s.mix(e, (rough,) * 3 if not isinstance(rgh, bpy.types.NodeSocket) else rgh, (0.28, 0.28, 0.28))
    rv_ = s.noise(s.obj, 1.6 / scale, 2.0, 0.5, offset=(6, 2, 2))
    rnd = s.maprange(rv_, 0.3, 0.7, -rough_var, rough_var)
    if isinstance(rgh, bpy.types.NodeSocket):
        rgh = s.m("ADD", s.sep_rgb(rgh) if False else rgh, rnd)
    else:
        rgh = s.m("ADD", rough, rnd)
    if seam is not None:
        rgh = s.m("ADD", rgh, s.m("MULTIPLY", seam, 0.2))
    # normal: fine grain + seams recessed
    # normal detail: fine grain + soft medium dents (hammered sheet metal) + seams
    h = s.m("ADD", s.m("MULTIPLY", s.noise(s.obj, 22.0 / scale, 4.0, 0.6, offset=(3, 3, 7)), 0.45),
            s.m("MULTIPLY", s.noise(s.obj, 3.5 / scale, 3.0, 0.5, offset=(8, 1, 6)), dent))
    if seam is not None:
        h = s.m("SUBTRACT", h, seam)
    return s.finish(col, rgh, metal, height=h, bump_strength=bump, bump_distance=0.03 * scale)


def role_metal(name, color, rough=0.38, metallic=0.9, scale=1.0, grime=0.3, seed=0, aniso=False):
    s = Shader(name)
    c1 = H(color)
    var = s.noise(s.obj, 0.8 / scale, 3.0, 0.55, offset=(seed, 2, 5))
    col = s.mix(s.ramp(var, [(0.3, 0.0), (0.7, 1.0)]), c1, tuple(c * 0.7 for c in c1))
    if grime:
        g = s.ramp(s.noise(s.obj, 1.3 / scale, 3.0, 0.6, offset=(7, 1, 3), stretch=(1, 1, 0.2)), [(0.5, 0.0), (0.8, 1.0)])
        col = s.mix(s.m("MULTIPLY", g, grime), col, H("#2E2A26"))
    rgh = s.maprange(s.noise(s.obj, 2.4 / scale, 3.0, 0.5, offset=(1, 4, 9)), 0.3, 0.7, rough - 0.1, rough + 0.12)
    h = s.m("ADD", s.m("MULTIPLY", s.noise(s.obj, 30.0 / scale, 4.0, 0.6, offset=(5, 5, 5)), 0.4),
            s.m("MULTIPLY", s.noise(s.obj, 4.0 / scale, 3.0, 0.5, offset=(2, 7, 1)), 0.8))
    return s.finish(col, rgh, metallic, height=h, bump_strength=0.4, bump_distance=0.025 * scale)


def role_foil(name, color="#D9A63C", scale=1.0):
    """Crinkled multi-layer insulation foil (gold)."""
    s = Shader(name)
    c = H(color)
    vor = s.voronoi(s.obj, 6.0 / scale, "Distance")
    n = s.noise(s.obj, 3.0 / scale, 4.0, 0.6, distortion=0.6)
    col = s.mix(s.ramp(n, [(0.3, 0.0), (0.7, 1.0)]), c, tuple(min(1.0, cc * 1.25) for cc in c))
    col = s.mix(s.m("MULTIPLY", s.ramp(vor, [(0.0, 1.0), (0.12, 0.0)]), 0.5), col, tuple(cc * 0.6 for cc in c))
    rgh = s.maprange(vor, 0.0, 0.5, 0.12, 0.3)
    h = s.m("ADD", s.m("MULTIPLY", vor, 0.8), s.m("MULTIPLY", n, 0.5))
    return s.finish(col, rgh, 1.0, height=h, bump_strength=0.55, bump_distance=0.03 * scale)


def role_glow(name, color, level=1.0, base="#151A1E", pattern=None, scale=1.0):
    """Emissive. The colour carries the relative brightness (level), the strength is the kit-wide
    EMISSION_STRENGTH, so one EmissiveStrength in Unreal drives the whole asset."""
    s = Shader(name)
    c = tuple(min(1.0, x * level) for x in H(color))
    em = c
    if pattern == "bars":
        lx, ly, lz = s.sep(s.lc)
        b = s.m("GREATER_THAN", s.m("FRACT", s.m("DIVIDE", s.m("ADD", lx, lz), 0.6 * scale)), 0.25)
        em = s.mix(b, tuple(x * 0.25 for x in c), c)
    s.finish(H(base), 0.3, 0.0, emission=em, emission_strength=EMISSION_STRENGTH)
    s.mat.diffuse_color = (*c, 1.0)
    return s.mat


# =========================================================================== UVs

def smart_uv(obj, angle=66.0, margin=0.004):
    m = hs["smart_uv"](obj, angle_deg=angle, island_margin=margin)
    if m != "smart_project":
        raise RuntimeError("smart UV project failed: " + m)


def pack_uv(obj, margin, rotate=True):
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
        bpy.ops.uv.pack_islands(rotate=rotate, margin_method="FRACTION", margin=margin)
    finally:
        bpy.ops.object.mode_set(mode="OBJECT")


def _poly_tris(poly):
    ls = list(range(poly.loop_start, poly.loop_start + poly.loop_total))
    return [(ls[0], ls[i], ls[i + 1]) for i in range(1, len(ls) - 1)]


def uv_stats(me, res=1024, return_bad=False):
    """Rasterise every UV triangle: a pixel covered twice is a real overlap."""
    uv = np.empty(len(me.loops) * 2, dtype=np.float32)
    me.uv_layers.active.data.foreach_get("uv", uv)
    uv = uv.reshape(-1, 2)
    count = np.zeros((res, res), dtype=np.uint8)
    owner = np.full((res, res), -1, dtype=np.int32)
    areas = {}
    bad = set()
    for poly in me.polygons:
        for tri in _poly_tris(poly):
            p = uv[list(tri)] * res
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
            areas[poly.index] = areas.get(poly.index, 0.0) + abs(den)
            sub_owner = owner[y0:y1, x0:x1]
            clash = inside & (sub_owner >= 0) & (sub_owner != poly.index)
            if clash.any():
                for other in np.unique(sub_owner[clash]):
                    other = int(other)
                    bad.add(poly.index if areas[poly.index] <= areas.get(other, 0.0) else other)
            sub_owner[inside & (sub_owner < 0)] = poly.index
            count[y0:y1, x0:x1] += inside
    if return_bad:
        return bad
    covered = int((count > 0).sum())
    overlap = int((count > 1).sum())
    return {"inside_0_1": bool(uv.min() >= -1e-6 and uv.max() <= 1.0 + 1e-6),
            "coverage": round(covered / float(res * res), 4), "overlap_pixels": overlap,
            "overlap_share_of_covered": round(overlap / float(max(covered, 1)), 6), "raster": res}


def fix_uv_overlaps(obj, margin, rounds=4):
    detached = 0
    for _ in range(rounds):
        bad = uv_stats(obj.data, 1024, return_bad=True)
        if not bad:
            break
        uvl = obj.data.uv_layers.active.data
        for i, fi in enumerate(sorted(bad)):
            poly = obj.data.polygons[fi]
            shift = Vector((10.0 + 2.0 * (i % 60), 10.0 + 2.0 * (i // 60)))
            for li in range(poly.loop_start, poly.loop_start + poly.loop_total):
                uvl[li].uv = Vector(uvl[li].uv) + shift
        detached += len(bad)
        pack_uv(obj, margin)
    return detached


def unwrap(obj, tex, angle=66.0):
    margin = 0.0035 if tex >= 2048 else 0.006
    smart_uv(obj, angle=angle, margin=margin)
    pack_uv(obj, margin)
    fixed = fix_uv_overlaps(obj, margin)
    return {"margin": margin, "faces_detached_for_overlap": fixed}


# =========================================================================== bake + post

def bake(obj, asset_dir, tex, sources=None, cage=0.02, ao_distance=2.0, samples=48, ao_samples=64):
    scene = bpy.context.scene
    if scene.world is None:
        scene.world = bpy.data.worlds.new("World")
    scene.world.light_settings.distance = ao_distance
    try:
        scene.render.bake.use_cage = False
    except Exception:
        pass
    res = bk["main"](target=obj.name, out_dir=asset_dir, size=tex, samples=samples, ao_samples=ao_samples,
                     margin=16, source_objects=sources, cage_extrusion=cage, max_ray_distance=0.0,
                     build_preview_material=True)
    return res


def _img(short, suffix):
    return bpy.data.images.get("T_{}_{}".format(short, suffix))


def _px(img):
    w, h = img.size
    a = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(a)
    return a.reshape(h, w, 4)


def fill_bake_misses(short, rounds=12, dark=0.02):
    """Selected-to-active rays that miss leave black texels: grow neighbours into them."""
    bc, orm = _img(short, "BC"), _img(short, "ORM")
    if bc is None or orm is None:
        return {"status": "skipped"}
    h, w = bc.size[1], bc.size[0]
    arrs = [_px(bc), _px(orm)]
    miss = arrs[0][:, :, :3].max(axis=2) < dark
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


def cavity_tint(short, tint=(0.55, 0.55, 0.6), strength=0.75, ao_hi=0.85, ao_lo=0.3):
    """Darken (and tint) the baked base colour where the baked AO is low: cavity dirt."""
    bc, orm = _img(short, "BC"), _img(short, "ORM")
    if bc is None or orm is None:
        return {"status": "skipped"}
    a, o = _px(bc), _px(orm)
    ao = o[:, :, 0]
    t = np.clip((ao_hi - ao) / (ao_hi - ao_lo), 0.0, 1.0)
    k = (t * t * (3.0 - 2.0 * t)) * strength
    for ch in range(3):
        a[:, :, ch] *= 1.0 + k * (tint[ch] - 1.0)
    bc.pixels.foreach_set(a.ravel())
    bc.save()
    return {"status": "ok", "share_tinted": round(float((k > 0.02).mean()), 4)}


def emissive_surface_share(me, short, threshold=0.25):
    img = _img(short, "E")
    if img is None:
        return 0.0
    px = _px(img)
    h, w = px.shape[:2]
    uv = me.uv_layers.active.data
    lit = total = 0.0
    for poly in me.polygons:
        u = sum(uv[li].uv.x for li in poly.loop_indices) / poly.loop_total
        v = sum(uv[li].uv.y for li in poly.loop_indices) / poly.loop_total
        total += poly.area
        if px[min(h - 1, max(0, int(v * h))), min(w - 1, max(0, int(u * w))), :3].max() > threshold:
            lit += poly.area
    return round(lit / max(total, 1e-9), 4)


def image_stats(short):
    out = {}
    for suffix in ("BC", "N", "ORM", "E"):
        img = _img(short, suffix)
        if img is None:
            continue
        px = _px(img).reshape(-1, 4)[:, :3]
        info = {"size": list(img.size), "mean_rgb": [round(float(v), 4) for v in px.mean(axis=0)],
                "min": round(float(px.min()), 4), "max": round(float(px.max()), 4)}
        if suffix == "N":
            info["std_rg"] = [round(float(px[:, 0].std()), 4), round(float(px[:, 1].std()), 4)]
        if suffix == "ORM":
            info["roughness_p5_p95"] = [round(float(np.percentile(px[:, 1], 5)), 3), round(float(np.percentile(px[:, 1], 95)), 3)]
            info["metallic_mean"] = round(float(px[:, 2].mean()), 3)
        out[suffix] = info
    return out


# =========================================================================== review

def _load_rgba(path):
    img = bpy.data.images.load(path, check_existing=False)
    a = _px(img).copy()
    bpy.data.images.remove(img)
    return a


def _save_rgba(arr, path):
    h, w = arr.shape[:2]
    img = bpy.data.images.new("__sd_sheet", width=w, height=h, alpha=False)
    img.pixels.foreach_set(arr.astype(np.float32).ravel())
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


def _nebula_world(world):
    nt = world.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputWorld")
    bg = nt.nodes.new("ShaderNodeBackground")
    tc = nt.nodes.new("ShaderNodeTexCoord")
    n1 = nt.nodes.new("ShaderNodeTexNoise")
    n1.inputs["Scale"].default_value = 1.6
    n1.inputs["Detail"].default_value = 6.0
    n1.inputs["Distortion"].default_value = 0.8
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    # Null Fleet: the game's bright magenta / teal nebulae (the worst case for black hulls is the bright parts)
    cr = ramp.color_ramp
    cr.elements[0].position = 0.25
    cr.elements[0].color = (0.04, 0.01, 0.10, 1.0)
    cr.elements[1].position = 0.62
    cr.elements[1].color = (0.85, 0.10, 0.62, 1.0)
    e = cr.elements.new(0.45)
    e.color = (0.03, 0.42, 0.48, 1.0)
    e2 = cr.elements.new(0.78)
    e2.color = (1.0, 0.45, 0.85, 1.0)
    nt.links.new(tc.outputs["Generated"], n1.inputs["Vector"])
    nt.links.new(n1.outputs.get("Factor") or n1.outputs.get("Fac"), ramp.inputs[0])
    nt.links.new(ramp.outputs[0], bg.inputs["Color"])
    bg.inputs["Strength"].default_value = 1.0
    nt.links.new(bg.outputs["Background"], out.inputs["Surface"])


def game_distance_strip(asset, out_path, cell=448):
    """Four cells, black space unless noted:
       1 near (~asset fills 55% of the frame), bright point light from the camera's upper left
       2 game distance (fills ~14%), same light
       3 game distance against a bright nebula, sun from the side
       4 backlit (light behind, emissive and rim only), fills 30%."""
    obj = bpy.data.objects[asset]
    pts = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
    lo = Vector([min(p[i] for p in pts) for i in range(3)])
    hi = Vector([max(p[i] for p in pts) for i in range(3)])
    centre = (lo + hi) * 0.5
    R = max((hi - lo).length * 0.5, 1e-3)
    scene = bpy.data.scenes.new("__sd_gd")
    scene.collection.objects.link(obj)
    world = bpy.data.worlds.new("__sd_gd_world")
    scene.world = world
    cam_d = bpy.data.cameras.new("__sd_cam")
    cam = bpy.data.objects.new("__sd_cam", cam_d)
    scene.collection.objects.link(cam)
    scene.camera = cam
    cam_d.lens = 50.0
    tanh = 18.0 / 50.0
    light_d = bpy.data.lights.new("__sd_pt", "POINT")
    light = bpy.data.objects.new("__sd_pt", light_d)
    scene.collection.objects.link(light)
    sun_d = bpy.data.lights.new("__sd_sun", "SUN")
    sun = bpy.data.objects.new("__sd_sun", sun_d)
    scene.collection.objects.link(sun)
    r = scene.render
    r.engine = "BLENDER_EEVEE"
    try:
        scene.eevee.taa_render_samples = 32
    except Exception:
        pass
    r.resolution_x = r.resolution_y = cell
    r.film_transparent = False
    r.image_settings.file_format = "PNG"
    cells = []
    # Null Fleet: cells 2-4 are TRUE game scale: the camera has the pixel density of a 1920 px wide frame with the
    # game's 90 deg horizontal FOV, at 150 m and 300 m; cell 4 is the 300 m view cropped and enlarged 4x
    # (nearest) so the real pixel count of the silhouette can be judged.
    game_lens = 18.0 / (math.tan(math.radians(45.0)) * cell / 1920.0)
    views = [("near", 0.55, (35.0, 18.0), "point", False, None, 1),
             ("m150", None, (35.0, 12.0), "sun", True, 150.0, 1),
             ("m300", None, (300.0, 10.0), "sun", True, 300.0, 1),
             ("m300x4", None, (35.0, 12.0), "sun", True, 300.0, 4)]
    try:
        for tag, fill, (az, el), mode, neb, dist_m, zoom in views:
            a, e = math.radians(az), math.radians(el)
            d = Vector((math.sin(a) * math.cos(e), -math.cos(a) * math.cos(e), math.sin(e)))
            if dist_m is None:
                cam_d.lens = 50.0
                D = R / (fill * tanh)
            else:
                cam_d.lens = game_lens * zoom
                D = dist_m
            cam.location = centre + d * D
            cam.rotation_euler = (-d).to_track_quat("-Z", "Y").to_euler()
            cam_d.clip_start = max(0.01, D * 0.002)
            cam_d.clip_end = D * 4.0
            if neb:
                world.use_nodes = True if hasattr(world, "use_nodes") else None
                _nebula_world(world)
            else:
                nt = world.node_tree
                nt.nodes.clear()
                out = nt.nodes.new("ShaderNodeOutputWorld")
                bg = nt.nodes.new("ShaderNodeBackground")
                bg.inputs["Color"].default_value = (0.0, 0.0, 0.0, 1.0)
                bg.inputs["Strength"].default_value = 0.0
                nt.links.new(bg.outputs["Background"], out.inputs["Surface"])
            light.hide_render = mode not in ("point", "back")
            sun.hide_render = mode != "sun"
            if mode == "point":
                right = d.cross(Vector((0, 0, 1))).normalized()
                lp = centre + d * (R * 3.0) + Vector((0, 0, 1)) * R * 2.0 - right * R * 2.0
            else:
                lp = centre - d * (R * 3.0) + Vector((0, 0, 1)) * R * 1.0
            light.location = lp
            dist = (lp - centre).length
            light_d.energy = 4.0 * math.pi * dist * dist * (6.0 if mode == "point" else 9.0)
            light_d.shadow_soft_size = R * 0.05
            sun_d.energy = 4.0
            sun.rotation_euler = Vector((0.4, -1.0, -0.3)).to_track_quat("-Z", "Y").to_euler()
            path = out_path[:-4] + "_{}.png".format(tag)
            r.filepath = path
            r.resolution_x = r.resolution_y = cell // zoom
            bpy.ops.render.render(write_still=True, scene=scene.name)
            px = _load_rgba(path)
            if zoom > 1:
                px = np.repeat(np.repeat(px, zoom, axis=0), zoom, axis=1)
            cells.append(px)
            os.remove(path)
    finally:
        scene.collection.objects.unlink(obj)
        for o in (cam, light, sun):
            bpy.data.objects.remove(o, do_unlink=True)
        bpy.data.cameras.remove(cam_d)
        bpy.data.lights.remove(light_d)
        bpy.data.lights.remove(sun_d)
        bpy.data.scenes.remove(scene)
        bpy.data.worlds.remove(world)
    gap = 6
    strip = np.full((cell + 2 * gap, 4 * cell + 5 * gap, 4), 0.06, dtype=np.float32)
    strip[:, :, 3] = 1.0
    for i, c in enumerate(cells):
        x0 = gap + i * (cell + gap)
        strip[gap:gap + cell, x0:x0 + cell] = c
    return strip


def review(asset, tag, mode_views=("front_3q", "right", "top", "back_3q")):
    """Writes review/<asset>_<tag>_clay_sheet.png and review/<asset>_<tag>_review.png
    (top row: material front 3/4, front, side, back 3/4; bottom row: game-distance strip)."""
    os.makedirs(REVIEW_DIR, exist_ok=True)
    label = "{}_{}".format(asset, tag)
    clay = rv["main"](targets=[asset], out_dir=REVIEW_DIR, label=label, mode="clay",
                      views=("front_3q", "front", "right", "top"), columns=4, resolution=384)
    mat = rv["main"](targets=[asset], out_dir=REVIEW_DIR, label=label + "_tmp", mode="material",
                     views=mode_views, columns=4, resolution=448)
    sheet = _load_rgba(mat["sheet"])
    os.remove(mat["sheet"])
    strip = game_distance_strip(asset, os.path.join(REVIEW_DIR, label + "_gd.png"))
    w = max(sheet.shape[1], strip.shape[1])

    def pad(a):
        out = np.full((a.shape[0], w, 4), 0.06, dtype=np.float32)
        out[:, :, 3] = 1.0
        out[:, :a.shape[1]] = a
        return out
    # Blender image rows run bottom-to-top: the strip goes first so it ends up at the bottom.
    combined = np.concatenate([pad(strip), pad(sheet)], axis=0)
    path = os.path.join(REVIEW_DIR, label + "_review.png")
    _save_rgba(combined, path)
    return {"review": path, "clay": clay["sheet"], "ortho_cm_per_px": mat.get("ortho_cm_per_px"),
            "warnings": clay.get("warnings", []) + mat.get("warnings", [])}


# =========================================================================== collision + export

def add_hulls(obj, hull_points, coll, max_points=40):
    names = []
    for i, pts in enumerate(hull_points):
        if len(pts) < 4:
            continue
        hname = "UCX_{}_{:02d}".format(obj.name, i)
        h = ex["_hull_object"](hname, [Vector(p) for p in pts], coll, max_points=max_points)
        h.parent = obj
        h.display_type = "WIRE"
        h.hide_render = True
        names.append(h.name)
    return names


def add_sockets(obj, sockets, coll):
    """sockets: [(name, (x, y, z) in the asset's local metres)]. Empties named SOCKET_<name>, parented to the root,
    rotated so their +X points along the asset's front (-Y); the exporter turns that into Unreal +X."""
    out = []
    for name, loc in sockets or ():
        e = bpy.data.objects.new("SOCKET_" + name, None)
        e.empty_display_type = "ARROWS"
        e.empty_display_size = 0.3
        coll.objects.link(e)
        e.parent = obj
        e.matrix_parent_inverse = Matrix.Identity(4)
        e.location = Vector(loc)
        e.rotation_euler = (0.0, 0.0, math.radians(-90.0))
        out.append({"name": name, "blender_m": [round(float(c), 4) for c in loc],
                    "ue_cm": [round(-float(loc[1]) * 100.0, 1), round(-float(loc[0]) * 100.0, 1),
                              round(float(loc[2]) * 100.0, 1)]})
    return out


def finish_asset(asset, obj, info, tex, tag, budget, collision_hulls, sources=(), extra_manifest=None, sockets=None):
    """Save .blend, run the repo checks, review renders, export FBX + manifest, write health json."""
    short = asset[3:]
    asset_dir = os.path.join(KIT, asset)
    coll = obj.users_collection[0]
    hull_names = add_hulls(obj, collision_hulls, coll)
    sock_info = add_sockets(obj, sockets, coll)
    extra_manifest = dict(extra_manifest or {})
    extra_manifest["ue_sockets"] = sock_info
    for o in sources:
        o.hide_render = True
        o.hide_viewport = True
    blend_path = os.path.join(asset_dir, asset + ".blend")
    bpy.ops.wm.save_as_mainfile(filepath=blend_path, compress=True)
    try:
        bpy.ops.file.make_paths_relative()
    except Exception:
        pass
    bpy.ops.wm.save_mainfile()
    if os.path.exists(blend_path + "1"):
        os.remove(blend_path + "1")
    report = ar["main"](targets=[asset], tri_budget=budget[1])
    assembly = ac["main"](targets=[asset])
    rev = review(asset, tag)
    manifest = ex["main"](root=asset, out_dir=asset_dir, collision="auto")
    man_path = os.path.join(asset_dir, asset + ".manifest.json")
    with open(man_path, encoding="utf-8") as fh:
        man = json.load(fh)
    man["fbx_relative"] = asset + ".fbx"
    man["texture_size"] = tex
    man["emission_strength"] = info.get("bake", {}).get("emission_strength", 0.0)
    man["nanite"] = True
    if extra_manifest:
        man.update(extra_manifest)
    with open(man_path, "w", encoding="utf-8") as fh:
        json.dump(man, fh, indent=2)
    me = obj.data
    pts = mesh_points(me)
    size_cm = [round(float(v) * 100.0, 1) for v in (pts.max(axis=0) - pts.min(axis=0))]
    checks = {
        "tris": tri_count(me),
        "tris_in_budget": budget[0] <= tri_count(me) <= budget[1],
        "material_slots": len(me.materials),
        "no_floating_parts": bool(assembly.get("ok")),
        "uv_inside_0_1": info.get("uv", {}).get("inside_0_1"),
        "uv_overlap_pixels": info.get("uv", {}).get("overlap_pixels"),
        "asset_report_errors": report["totals"]["errors"],
        "asset_report_warnings": report["totals"]["warnings"],
        "collision_hulls": len(hull_names),
        "size_cm": size_cm,
        "export_status": manifest.get("status"),
    }
    health = {"asset": asset, "tag": tag, "checks": checks, "info": info, "textures": image_stats(short),
              "emissive_surface_share": emissive_surface_share(me, short),
              "asset_report_issues": report["issues"], "assembly": {k: assembly.get(k) for k in
                                                                    ("ok", "islands", "groups", "floating", "tolerance_m")},
              "review": rev, "export": {k: manifest.get(k) for k in ("status", "tris", "size_cm", "collision",
                                                                     "collision_source", "materials", "textures",
                                                                     "warnings", "errors")}}
    with open(os.path.join(REVIEW_DIR, asset + "_health.json"), "w", encoding="utf-8") as fh:
        json.dump(health, fh, indent=2, default=str)
    log("CHECKS", asset, json.dumps(checks), "emissive share", health["emissive_surface_share"])
    log("ISSUES", json.dumps(report["issues"][:12]))
    return health


def finish_hard_surface(part, asset, tex, tag, budget, ao_distance, tint=(0.62, 0.6, 0.58), tint_strength=0.7,
                        extra_manifest=None, overlap_uv=None, samples=48, post_scale=1.0, snap=True,
                        sockets=None, recentre_com=True):
    """Join -> recentre at the centre of mass -> edge attribute -> UVs -> bake -> post -> finish."""
    t0 = time.time()
    info = {}
    if snap and overlap_uv is None:
        info["snapped_floating_groups"] = part.snap_floating()
    obj = part.join()
    hulls = part.hulls
    if post_scale != 1.0:
        obj.data.transform(Matrix.Diagonal((post_scale,) * 3 + (1.0,)))
        obj.data.update()
        for h in hulls:
            for i in range(len(h)):
                h[i] = h[i] * post_scale
        info["post_scale"] = post_scale
    if recentre_com == "bbox":
        # pivot at the bounding-box centre (the collision sphere's centre in gameplay); COM reported
        pts_ = mesh_points(obj.data)
        c_ = Vector([float(v) for v in (pts_.max(axis=0) + pts_.min(axis=0)) * 0.5])
        info["centre_of_mass_m_before"] = [round(float(v), 3) for v in center_of_mass(obj.data)]
        obj.data.transform(Matrix.Translation(-c_))
        obj.data.update()
        for h_ in hulls:
            for i_ in range(len(h_)):
                h_[i_] = h_[i_] - c_
        info["com_shift_m"] = [float(v) for v in c_]
        info["pivot"] = "bbox_centre"
    elif recentre_com:
        info["com_shift_m"] = recentre(obj, hulls)
        info["pivot"] = "centre_of_mass"
    else:
        info["com_shift_m"] = [0.0, 0.0, 0.0]
        info["centre_of_mass_m"] = [round(float(c), 3) for c in center_of_mass(obj.data)]
    shift = Vector(info["com_shift_m"])
    sockets = [(n, Vector(p) - shift) for n, p in (sockets or ())]
    if obj.data.attributes.get("edge") is None:
        mark_edge_faces(obj.data, part.max_bevel or 0.02)
    info["edge_area_share"] = edge_area_share(obj.data)
    if overlap_uv is None:
        info["uv_unwrap"] = unwrap(obj, tex)
    else:
        info["uv_unwrap"] = overlap_uv
    info["uv"] = uv_stats(obj.data, 1024) if overlap_uv is None else {"inside_0_1": True, "overlap_pixels": "by design (repeated segments share UVs)"}
    asset_dir = os.path.join(KIT, asset)
    os.makedirs(asset_dir, exist_ok=True)
    info["bake"] = bake(obj, asset_dir, tex, sources=None, ao_distance=ao_distance, samples=samples)
    info["cavity"] = cavity_tint(asset[3:], tint=tint, strength=tint_strength)
    obj.data.materials[0].name = "M_{}".format(asset[3:])
    info["build_seconds"] = round(time.time() - t0, 1)
    return finish_asset(asset, obj, info, tex, tag, budget, hulls, extra_manifest=extra_manifest, sockets=sockets)

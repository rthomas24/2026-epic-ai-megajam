# SPDX-License-Identifier: MIT
"""
propkit.py - shared library for the engineering-made ground prop kits (EngProps).

Headless only (blender --background --factory-startup --python build_<kit>.py). It provides:

  * Builder      - bmesh primitives that carry their own look as corner colour attributes
                   (Col = base colour + roughness, Glow = emissive colour, Par = pattern controls):
                   sweep / tube / blade / lathe / rock / dome.
  * materials    - attribute-driven procedural materials (flora, mossy rock, basalt).
  * atlas_bake   - joins a group of assets, unwraps them into ONE shared 0-1 UV atlas, bakes it with
                   the blender-texturing skill script and writes the UVs back to every asset.
  * make_ucx     - one simple convex UCX_ hull per asset.
  * check / review / contact sheet / first-person view / packaging (per-asset .blend, FBX, manifest)
    and an FBX round-trip check.

The measured checks, review renders, bake and export are done by the repo's skill scripts
(.claude/skills/blender-*/scripts); this file only adds what they do not cover.
A copy of this file lives in each kit's scripts folder so each kit regenerates on its own.
"""

import json
import math
import os
import random
import runpy
import time

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector, noise

HERE = os.path.dirname(os.path.abspath(__file__))
KIT_DIR = os.path.dirname(HERE)


def _repo_root():
    d = HERE
    while True:
        if os.path.isdir(os.path.join(d, ".claude", "skills")):
            return d
        nd = os.path.dirname(d)
        if nd == d:
            raise RuntimeError("repo root (.claude/skills) not found above " + HERE)
        d = nd


ROOT = _repo_root()
_SKILLS = os.path.join(ROOT, ".claude", "skills")
_cache = {}


def skill(rel):
    """Load one of the repo's skill scripts (cached)."""
    if rel not in _cache:
        _cache[rel] = runpy.run_path(os.path.join(_SKILLS, rel))
    return _cache[rel]


def hexc(value):
    return skill("blender-game-asset/scripts/hardsurface.py")["hex_color"](value)


def mixc(a, b, t):
    return tuple(a[i] + (b[i] - a[i]) * t for i in range(3))


def mulc(a, k):
    return tuple(c * k for c in a)


def smooth01(x, a=0.0, b=1.0):
    t = min(1.0, max(0.0, (x - a) / (b - a))) if b != a else 0.0
    return t * t * (3.0 - 2.0 * t)


def lerp(a, b, t):
    return a + (b - a) * t


def rot_to(direction):
    """4x4 rotation taking +Z to `direction`."""
    return Vector(direction).normalized().to_track_quat("Z", "Y").to_matrix().to_4x4()


# ============================================================================ builder

def _frames(pts, side=None):
    n = len(pts)
    tans = []
    for i in range(n):
        t = pts[min(i + 1, n - 1)] - pts[max(i - 1, 0)]
        tans.append(t.normalized() if t.length > 1e-9 else Vector((0.0, 0.0, 1.0)))
    out = []
    if side is not None:
        side = Vector(side)
        prev = None
        for t in tans:
            s = side - t * side.dot(t)
            if s.length < 1e-4:
                s = prev if prev is not None else t.orthogonal()
            s = s.normalized()
            prev = s
            out.append((s, t.cross(s).normalized(), t))
    else:
        s = tans[0].orthogonal().normalized()
        for i, t in enumerate(tans):
            if i > 0:
                s = tans[i - 1].rotation_difference(t) @ s
            s = (s - t * s.dot(t)).normalized()
            out.append((s, t.cross(s).normalized(), t))
    return out


class Builder:
    """Accumulates one asset's geometry in a bmesh. Every primitive is a closed shell."""

    def __init__(self):
        self.bm = bmesh.new()
        ll = self.bm.loops.layers.float_color
        self.lcol = ll.new("Col")
        self.lglow = ll.new("Glow")
        self.lpar = ll.new("Par")
        self.vt = self.bm.verts.layers.float.new("pt")
        self.vu = self.bm.verts.layers.float.new("pu")
        self.vtag = self.bm.verts.layers.int.new("tag")
        self.fj = self.bm.faces.layers.int.new("fj")
        self.fk = self.bm.faces.layers.int.new("fk")
        self.hull_tags = None  # None = every vertex goes into the collision hull

    # ---------------------------------------------------------------- painting
    def _init(self, faces):
        for f in faces:
            for lp in f.loops:
                lp[self.lcol] = (0.5, 0.5, 0.5, 0.6)
                lp[self.lglow] = (0.0, 0.0, 0.0, 1.0)
                lp[self.lpar] = (0.5, 0.5, 0.0, 0.0)

    def paint(self, faces, fn=None, **kw):
        """kw / fn(vert, face) -> dict with any of: col (rgb linear), rough, glow (rgb linear),
        noise (colour variation 0-1), bump (0-1), mask, mask2 (pattern controls)."""
        for f in faces:
            for lp in f.loops:
                d = dict(kw)
                if fn is not None:
                    r = fn(lp.vert, f)
                    if r:
                        d.update(r)
                if "col" in d or "rough" in d:
                    c = lp[self.lcol]
                    col = d.get("col", (c[0], c[1], c[2]))
                    lp[self.lcol] = (col[0], col[1], col[2], d.get("rough", c[3]))
                if "glow" in d:
                    g = d["glow"]
                    lp[self.lglow] = (g[0], g[1], g[2], 1.0)
                if "noise" in d or "bump" in d or "mask" in d or "mask2" in d:
                    p = lp[self.lpar]
                    lp[self.lpar] = (d.get("noise", p[0]), d.get("bump", p[1]),
                                     d.get("mask", p[2]), d.get("mask2", p[3]))

    def t(self, v):
        return v[self.vt]

    def u(self, v):
        return v[self.vu]

    # ---------------------------------------------------------------- sweep family
    def sweep(self, pts, ring_fn, side=None, cap0="flat", cap1="flat", smooth=True, tag=0, xf=None):
        """Sweep a cross-section along a polyline. ring_fn(i, t) -> [(x, y), ...] offsets in the
        (side, normal) plane, counter-clockwise about the direction of travel, or None for a point."""
        bm = self.bm
        pts = [Vector(p) for p in pts]
        fr = _frames(pts, side)
        n = len(pts)
        rings = []
        for i, (p, (s, nn, _t)) in enumerate(zip(pts, fr)):
            tt = i / float(n - 1)
            offs = ring_fn(i, tt)
            ring = []
            if not offs:
                co = xf @ p if xf is not None else p
                v = bm.verts.new(co)
                v[self.vt], v[self.vu], v[self.vtag] = tt, 0.0, tag
                ring.append(v)
            else:
                m = len(offs)
                for j, (x, y) in enumerate(offs):
                    co = p + s * x + nn * y
                    if xf is not None:
                        co = xf @ co
                    v = bm.verts.new(co)
                    v[self.vt], v[self.vu], v[self.vtag] = tt, j / float(m), tag
                    ring.append(v)
            rings.append(ring)
        faces = []

        def face(vs, j, k):
            f = bm.faces.new(vs)
            f.smooth = smooth
            f[self.fj], f[self.fk] = j, k
            faces.append(f)

        for i in range(n - 1):
            a, b = rings[i], rings[i + 1]
            if len(a) == 1 and len(b) == 1:
                continue
            m = max(len(a), len(b))
            for j in range(m):
                j2 = (j + 1) % m
                if len(a) == 1:
                    face([a[0], b[j2], b[j]], j, i)
                elif len(b) == 1:
                    face([a[j], a[j2], b[0]], j, i)
                else:
                    face([a[j], a[j2], b[j2], b[j]], j, i)
        if cap0 == "flat" and len(rings[0]) > 2:
            face(list(reversed(rings[0])), -1, -1)
        if cap1 == "flat" and len(rings[-1]) > 2:
            face(list(rings[-1]), -1, -2)
        self._init(faces)
        return faces

    def tube(self, pts, radii, sides=6, rmod=None, squash=1.0, phase=0.0, **kw):
        def ring(i, _t):
            r = radii[i]
            if r <= 1e-6:
                return None
            out = []
            for j in range(sides):
                a = phase + math.tau * j / sides
                rr = r * (rmod(i, j) if rmod else 1.0)
                out.append((rr * math.cos(a), rr * math.sin(a) * squash))
            return out
        return self.sweep(pts, ring, **kw)

    def blade(self, pts, width, side, thick=0.012, cup=0.0, **kw):
        """Leaf / frond: lens cross-section (right edge, top ridge, left edge, keel)."""
        def val(x, t):
            return x(t) if callable(x) else x

        def ring(_i, t):
            w = width[_i] if isinstance(width, (list, tuple)) else val(width, t)
            if w <= 1e-5:
                return None
            th = val(thick, t)
            c = val(cup, t) * w
            return [(w, c), (0.0, th), (-w, c), (0.0, -th * 0.7)]
        kw.setdefault("cap0", "flat")
        return self.sweep(pts, ring, side=side, **kw)

    # ---------------------------------------------------------------- lathe
    def lathe(self, profile, sides=16, xf=None, rmod=None, zmod=None, smooth=True, tag=0, phase=0.0):
        """Revolve (r, z) points about +Z. Run the profile bottom -> outside -> top. r == 0 at an end
        makes a pole; otherwise the end is capped flat. rmod/zmod(angle, k, r, z) shape the rings."""
        bm = self.bm
        n = len(profile)
        rings = []
        for k, (r, z) in enumerate(profile):
            ring = []
            if r <= 1e-7:
                co = Vector((0.0, 0.0, z + (zmod(0.0, k, 0.0, z) if zmod else 0.0)))
                if xf is not None:
                    co = xf @ co
                v = bm.verts.new(co)
                v[self.vt], v[self.vu], v[self.vtag] = k / float(n - 1), 0.0, tag
                ring.append(v)
            else:
                for j in range(sides):
                    a = phase + math.tau * j / sides
                    rr = r * (rmod(a, k, r, z) if rmod else 1.0)
                    zz = z + (zmod(a, k, r, z) if zmod else 0.0)
                    co = Vector((rr * math.cos(a), rr * math.sin(a), zz))
                    if xf is not None:
                        co = xf @ co
                    v = bm.verts.new(co)
                    v[self.vt], v[self.vu], v[self.vtag] = k / float(n - 1), j / float(sides), tag
                    ring.append(v)
            rings.append(ring)
        faces = []

        def face(vs, j, k):
            f = bm.faces.new(vs)
            f.smooth = smooth
            f[self.fj], f[self.fk] = j, k
            faces.append(f)

        for k in range(n - 1):
            a, b = rings[k], rings[k + 1]
            if len(a) == 1 and len(b) == 1:
                continue
            for j in range(sides):
                j2 = (j + 1) % sides
                if len(a) == 1:
                    face([a[0], b[j2], b[j]], j, k)
                elif len(b) == 1:
                    face([a[j], a[j2], b[0]], j, k)
                else:
                    face([a[j], a[j2], b[j2], b[j]], j, k)
        if len(rings[0]) > 2:
            face(list(reversed(rings[0])), -1, -1)
        if len(rings[-1]) > 2:
            face(list(rings[-1]), -1, -2)
        self._init(faces)
        return faces

    def dome(self, center, normal, radius, height, sides=6, sink=1.3, tag=0, smooth=True):
        """Small raised bump (glow spot, pod) sunk into the surface it sits on."""
        xf = Matrix.Translation(Vector(center)) @ rot_to(normal)
        prof = [(0.0, -height * sink), (radius, -height * sink), (radius * 0.82, height * 0.45), (0.0, height)]
        return self.lathe(prof, sides=sides, xf=xf, tag=tag, smooth=smooth)

    # ---------------------------------------------------------------- rocks
    def rock(self, size, seed=0, subdiv=3, lump=0.18, freq=1.0, cuts=7, cut_range=(0.72, 0.94),
             crisp=False, bevel=0.0, down_bias=0.6, floor=None, xf=None, smooth=True, tag=0, squash_pow=1.0,
             bevel_segments=1, bevel_angle=28.0, taper=0.0, clips=()):
        """Chiselled boulder: a lumpy sphere cut by random planes. crisp=False flattens the caps in
        place (soft, worn facets); crisp=True slices and caps them (sharp fractured facets) and can
        chamfer the edges (bevel, metres). size = full (x, y, z) extent in metres before `xf`.
        clips = extra (point, normal) planes (crisp only): what lies on the normal's side is cut away,
        so two calls with the same seed and opposite normals give two halves with matching faces.
        floor = local z (metres) below which the shape is cut flat."""
        tb = None
        for attempt in range(5):
            rng = random.Random(seed + 7919 * attempt)  # a shape that does not bevel cleanly is re-rolled
            if tb is not None:
                tb.free()
            tb = bmesh.new()
            bmesh.ops.create_icosphere(tb, subdivisions=subdiv, radius=1.0)
            off = Vector((rng.uniform(-50, 50), rng.uniform(-50, 50), rng.uniform(-50, 50)))
            for v in tb.verts:
                nrm = v.co.normalized()
                h = noise.fractal(nrm * freq + off, 1.0, 2.0, 2, noise_basis="PERLIN_ORIGINAL") * lump
                v.co = nrm * (1.0 + h)
            planes = []
            for _ in range(cuts):
                d = Vector((rng.gauss(0, 1), rng.gauss(0, 1), rng.gauss(0, 1)))
                if d.z < 0:
                    d.z *= (1.0 - down_bias)
                d.normalize()
                planes.append((d, rng.uniform(*cut_range)))
            half = Vector(size) * 0.5
            if crisp:
                for v in tb.verts:
                    k = 1.0 - taper * (v.co.z + 1.0) * 0.5
                    v.co = Vector((v.co.x * half.x * k, v.co.y * half.y * k, v.co.z * half.z))
                for d, frac in planes:
                    dd = Vector((d.x / half.x, d.y / half.y, d.z / half.z)).normalized()
                    ext = max(v.co.dot(dd) for v in tb.verts)
                    self._slice(tb, dd * (ext * frac), dd)
                for co, no in clips:
                    self._slice(tb, Vector(co), Vector(no).normalized())
                if floor is not None:
                    self._slice(tb, Vector((0, 0, floor)), Vector((0, 0, -1)))
                bmesh.ops.dissolve_limit(tb, angle_limit=math.radians(2.0), verts=tb.verts[:], edges=tb.edges[:])
                if bevel > 0.0:
                    edges = [e for e in tb.edges if len(e.link_faces) == 2
                             and e.calc_face_angle(0.0) > math.radians(bevel_angle)]
                    bmesh.ops.bevel(tb, geom=edges, offset=bevel, offset_type="OFFSET", segments=bevel_segments,
                                    profile=0.5, affect="EDGES", clamp_overlap=True)
                bmesh.ops.remove_doubles(tb, verts=tb.verts[:], dist=0.0005)
                bmesh.ops.dissolve_degenerate(tb, dist=0.0003, edges=tb.edges[:])
                bmesh.ops.triangulate(tb, faces=tb.faces[:], quad_method="BEAUTY", ngon_method="BEAUTY")
            else:
                for d, frac in planes:
                    ext = max(v.co.dot(d) for v in tb.verts)
                    h = ext * frac
                    for v in tb.verts:
                        s = v.co.dot(d) - h
                        if s > 0.0:
                            v.co -= d * s
                for v in tb.verts:
                    z = v.co.z
                    if squash_pow != 1.0:
                        z = math.copysign(abs(z) ** squash_pow, z)
                    v.co = Vector((v.co.x * half.x, v.co.y * half.y, z * half.z))
                if floor is not None:
                    for v in tb.verts:
                        if v.co.z < floor:
                            v.co.z = floor
                    bmesh.ops.remove_doubles(tb, verts=tb.verts[:], dist=0.0005)
                    bmesh.ops.dissolve_degenerate(tb, dist=0.0003, edges=tb.edges[:])
            bad = sum(1 for e in tb.edges if len(e.link_faces) != 2)
            if not bad:
                break
            print("rock seed {} attempt {}: {} non-manifold edges, retrying".format(seed, attempt, bad))
        tb.normal_update()
        vmap = {}
        for v in tb.verts:
            nv = self.bm.verts.new(xf @ v.co if xf is not None else v.co)
            nv[self.vtag] = tag
            vmap[v] = nv
        faces = []
        for f in tb.faces:
            try:
                nf = self.bm.faces.new([vmap[v] for v in f.verts])
            except ValueError:
                continue
            nf.smooth = smooth
            nf[self.fj], nf[self.fk] = -1, -1
            faces.append(nf)
        tb.free()
        self._init(faces)
        return faces

    @staticmethod
    def _slice(tb, co, no):
        geom = tb.verts[:] + tb.edges[:] + tb.faces[:]
        bmesh.ops.bisect_plane(tb, geom=geom, dist=1e-6, plane_co=co, plane_no=no, clear_outer=True)
        edges = [e for e in tb.edges if e.is_boundary]
        if edges:
            bmesh.ops.triangle_fill(tb, use_beauty=True, use_dissolve=False, edges=edges, normal=no)

    # ---------------------------------------------------------------- placement
    def push_until_touch(self, faces, others, toward, step=0.015, embed=0.04, max_steps=300):
        """Slide a shell along `toward` until it intersects `others`, then sink it `embed` further,
        so secondary pieces are always attached to the main body (never floating)."""
        from mathutils.bvhtree import BVHTree

        def tree(fs):
            idx, polys = {}, []
            for f in fs:
                polys.append([idx.setdefault(v, len(idx)) for v in f.verts])
            coords = [None] * len(idx)
            for v, i in idx.items():
                coords[i] = v.co.copy()
            return BVHTree.FromPolygons(coords, polys)
        target = tree(others)
        verts = {v for f in faces for v in f.verts}
        d = Vector(toward).normalized()
        moved = 0.0
        for _ in range(max_steps):
            if tree(faces).overlap(target):
                break
            for v in verts:
                v.co += d * step
            moved += step
        for v in verts:
            v.co += d * embed
        return moved + embed

    # ---------------------------------------------------------------- finish
    def finish(self, name, collection, sharp_angle=None, recentre=False):
        bm = self.bm
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
        if recentre:  # pivot = centre of the footprint on the ground plane
            foot = [v.co for v in bm.verts if v.co.z < 0.08] or [v.co for v in bm.verts]
            cx = (max(p.x for p in foot) + min(p.x for p in foot)) * 0.5
            cy = (max(p.y for p in foot) + min(p.y for p in foot)) * 0.5
            for v in bm.verts:
                v.co.x -= cx
                v.co.y -= cy
        tags = self.hull_tags
        hull_pts = [v.co.copy() for v in bm.verts if tags is None or v[self.vtag] in tags]
        me = bpy.data.meshes.new(name)
        bm.to_mesh(me)
        bm.free()
        for an in ("pt", "pu", "tag", "fj", "fk"):
            at = me.attributes.get(an)
            if at is not None:
                me.attributes.remove(at)
        if sharp_angle:
            me.set_sharp_from_angle(angle=math.radians(sharp_angle))
        obj = bpy.data.objects.new(name, me)
        collection.objects.link(obj)
        return obj, hull_pts


# ============================================================================ materials

class Nodes:
    """Small helper to write shader graphs compactly."""

    def __init__(self, name):
        self.mat = bpy.data.materials.new(name)
        self.nt = self.mat.node_tree
        self.bsdf = next(n for n in self.nt.nodes if n.type == "BSDF_PRINCIPLED")

    def node(self, kind, **props):
        n = self.nt.nodes.new(kind)
        for k, v in props.items():
            setattr(n, k, v)
        return n

    def put(self, sock, value):
        if hasattr(value, "is_linked") or isinstance(value, bpy.types.NodeSocket):
            self.nt.links.new(value, sock)
        elif isinstance(value, (tuple, list)):
            sock.default_value = tuple(value) + (1.0,) * (len(sock.default_value) - len(value))
        else:
            sock.default_value = value

    def math(self, op, a, b=None, c=None, clamp=False):
        n = self.node("ShaderNodeMath", operation=op, use_clamp=clamp)
        self.put(n.inputs[0], a)
        if b is not None:
            self.put(n.inputs[1], b)
        if c is not None:
            self.put(n.inputs[2], c)
        return n.outputs[0]

    def vscale(self, vec, scalar):
        n = self.node("ShaderNodeVectorMath", operation="SCALE")
        self.put(n.inputs[0], vec)
        self.put(n.inputs["Scale"], scalar)
        return n.outputs[0]

    def mix(self, fac, a, b):
        n = self.node("ShaderNodeMix", data_type="RGBA", blend_type="MIX")
        ins = {s.identifier: s for s in n.inputs}
        self.put(ins["Factor_Float"], fac)
        self.put(ins["A_Color"], a)
        self.put(ins["B_Color"], b)
        return next(s for s in n.outputs if s.identifier == "Result_Color")

    def remap(self, v, a, b, c=0.0, d=1.0, smooth=True):
        n = self.node("ShaderNodeMapRange")
        n.interpolation_type = "SMOOTHSTEP" if smooth else "LINEAR"
        n.clamp = True
        self.put(n.inputs["Value"], v)
        for key, val in (("From Min", a), ("From Max", b), ("To Min", c), ("To Max", d)):
            n.inputs[key].default_value = val
        return n.outputs["Result"]

    def noise(self, vec, scale, detail=4.0, rough=0.55):
        n = self.node("ShaderNodeTexNoise")
        self.nt.links.new(vec, n.inputs["Vector"])
        n.inputs["Scale"].default_value = scale
        n.inputs["Detail"].default_value = detail
        n.inputs["Roughness"].default_value = rough
        return n.outputs["Fac"]

    def voronoi(self, vec, scale, feature="F1"):
        n = self.node("ShaderNodeTexVoronoi", feature=feature)
        self.nt.links.new(vec, n.inputs["Vector"])
        n.inputs["Scale"].default_value = scale
        return n.outputs["Distance"]

    def attr(self, name):
        n = self.node("ShaderNodeAttribute", attribute_name=name)
        outs = list(n.outputs)
        return outs[0], outs[3]  # colour, alpha

    def sep(self, col):
        n = self.node("ShaderNodeSeparateColor")
        self.nt.links.new(col, n.inputs[0])
        return n.outputs[0], n.outputs[1], n.outputs[2]


def _core(g, emission_strength, ground_dark=0.55, coarse=5.0, fine=20.0):
    """Shared part of every material: Col/Glow/Par attributes, colour variation, bump, ground fade."""
    col, rough = g.attr("Col")
    glow, _a = g.attr("Glow")
    par, par_a = g.attr("Par")
    pr, pg, pb = g.sep(par)
    coord = g.node("ShaderNodeTexCoord").outputs["Object"]
    geo = g.node("ShaderNodeNewGeometry")
    sepz = g.node("ShaderNodeSeparateXYZ")
    g.nt.links.new(geo.outputs["Position"], sepz.inputs[0])
    sepn = g.node("ShaderNodeSeparateXYZ")
    g.nt.links.new(geo.outputs["Normal"], sepn.inputs[0])
    n_coarse = g.noise(coord, coarse, 5.0, 0.6)
    n_fine = g.noise(coord, fine, 3.0, 0.55)
    # colour variation: Col * (1 + noiseAmount * (lerp(0.5, 1.4, n) - 1))
    var = g.math("ADD", g.math("MULTIPLY", pr, g.math("SUBTRACT", g.remap(n_coarse, 0.25, 0.75, 0.5, 1.4), 1.0)), 1.0)
    ground = g.remap(sepz.outputs["Z"], -0.03, 0.22, ground_dark, 1.0)
    base = g.vscale(col, g.math("MULTIPLY", var, ground))
    g.bsdf.inputs["Emission Strength"].default_value = emission_strength
    g.bsdf.inputs["Metallic"].default_value = 0.0
    return {"col": col, "rough": rough, "glow": glow, "pr": pr, "pg": pg, "pb": pb, "pa": par_a,
            "coord": coord, "nz": sepn.outputs["Z"], "z": sepz.outputs["Z"], "n_coarse": n_coarse,
            "n_fine": n_fine, "base": base}


def _finish(g, base, rough, emit, height, strength, distance=0.02):
    bump = g.node("ShaderNodeBump")
    bump.inputs["Distance"].default_value = distance
    g.put(bump.inputs["Strength"], strength)
    g.put(bump.inputs["Height"], height)
    g.put(g.bsdf.inputs["Base Color"], base)
    g.put(g.bsdf.inputs["Roughness"], rough)
    g.put(g.bsdf.inputs["Emission Color"], emit)
    g.nt.links.new(bump.outputs["Normal"], g.bsdf.inputs["Normal"])
    return g.mat


def flora_material(name, emission_strength=2.0, fine=20.0, ground_dark=0.6):
    """Attribute-driven: used for mushrooms, ferns, glow plants and crystals."""
    g = Nodes(name)
    c = _core(g, emission_strength, ground_dark=ground_dark, fine=fine)
    rough = g.math("ADD", c["rough"], g.math("MULTIPLY", g.math("SUBTRACT", c["n_fine"], 0.5), 0.12), clamp=True)
    line = g.math("ADD", g.math("SUBTRACT", 1.0, c["pa"]),
                  g.math("MULTIPLY", c["pa"], g.remap(c["pb"], 0.78, 0.98)), clamp=True)
    emit = g.vscale(c["glow"], g.math("MULTIPLY", g.remap(c["n_coarse"], 0.2, 0.8, 0.82, 1.0), line))
    return _finish(g, c["base"], rough, emit, c["n_fine"], c["pg"])


def mossy_rock_material(name, moss_dark, moss_light, lichen_a, lichen_b, emission_strength=2.0):
    """Stone from Col, moss on up-facing surfaces (Par.b = amount), glowing lichen spots (Par.a)."""
    g = Nodes(name)
    c = _core(g, emission_strength, ground_dark=0.5, coarse=3.2, fine=24.0)
    n_mid = g.noise(c["coord"], 9.0, 4.0, 0.6)
    # strata: faint darker bands
    band = g.node("ShaderNodeTexWave", wave_type="BANDS", bands_direction="Z")
    g.nt.links.new(c["coord"], band.inputs["Vector"])
    band.inputs["Scale"].default_value = 3.0
    band.inputs["Distortion"].default_value = 4.0
    band.inputs["Detail"].default_value = 2.0
    stone = g.vscale(c["base"], g.remap(band.outputs["Fac"], 0.2, 0.8, 0.86, 1.06))
    n_patch = g.noise(c["coord"], 2.6, 3.0, 0.5)
    n_edge = g.noise(c["coord"], 11.0, 3.0, 0.6)
    moss_n = g.math("ADD", g.math("MULTIPLY", g.math("SUBTRACT", n_patch, 0.5), 1.3),
                    g.math("MULTIPLY", g.math("SUBTRACT", n_edge, 0.5), 0.5))
    moss_m = g.remap(g.math("ADD", c["pb"], moss_n), 0.42, 0.56)
    moss_c = g.mix(g.remap(n_mid, 0.3, 0.7, 0.15, 0.85), moss_dark, moss_light)
    base = g.mix(moss_m, stone, moss_c)
    # lichen: glowing spots where Par.a is painted, large at the heart of a patch and small at its edge
    thr = g.math("ADD", 0.05, g.math("MULTIPLY", c["pa"], 0.33))
    gate = g.remap(c["pa"], 0.08, 0.25)
    big = g.remap(g.math("SUBTRACT", thr, g.voronoi(c["coord"], 5.5)), 0.0, 0.07)
    small = g.remap(g.math("SUBTRACT", g.math("MULTIPLY", thr, 0.55), g.voronoi(c["coord"], 13.0)), 0.0, 0.05)
    spot = g.math("MULTIPLY", g.math("MAXIMUM", big, small), gate)
    lich_c = g.mix(g.remap(n_mid, 0.35, 0.65), lichen_a, lichen_b)
    base = g.mix(g.math("MULTIPLY", spot, 0.7), base, g.vscale(lich_c, 0.55))
    emit = g.vscale(lich_c, spot)
    rough = g.math("ADD", g.math("ADD", c["rough"], g.math("MULTIPLY", moss_m, 0.14)),
                   g.math("MULTIPLY", g.math("SUBTRACT", c["n_fine"], 0.5), 0.1), clamp=True)
    height = g.math("ADD", c["n_fine"], g.math("MULTIPLY", g.math("MULTIPLY", moss_m, n_mid), 1.6))
    return _finish(g, base, rough, emit, height, c["pg"], distance=0.03)


def basalt_material(name, ash, magma, magma_hot, emission_strength=2.0):
    """Dark basalt from Col, ash on up-facing surfaces (Par.b), ember cracks (Par.a)."""
    g = Nodes(name)
    c = _core(g, emission_strength, ground_dark=0.7, coarse=2.6, fine=22.0)
    n_mid = g.noise(c["coord"], 7.0, 3.0, 0.55)
    ash_m = g.math("MULTIPLY", c["pb"],
                   g.remap(g.math("ADD", c["nz"], g.math("MULTIPLY", g.math("SUBTRACT", n_mid, 0.5), 0.8)), 0.55, 0.95))
    base = g.mix(g.math("MULTIPLY", ash_m, 0.8), c["base"], ash)
    edge = g.voronoi(c["coord"], 2.3, feature="DISTANCE_TO_EDGE")
    # cracks only where Par.a asks, in patches (coarse noise), and fading out with height: heat from below
    gate = g.math("MULTIPLY", g.math("MULTIPLY", g.remap(c["pa"], 0.2, 0.6), g.remap(c["n_coarse"], 0.47, 0.62)),
                  g.remap(c["z"], 0.12, 0.75, 1.0, 0.0))
    crack = g.math("MULTIPLY", g.remap(edge, 0.0, 0.024, 1.0, 0.0), gate)
    halo = g.math("MULTIPLY", g.remap(edge, 0.0, 0.1, 1.0, 0.0), gate)
    base = g.vscale(base, g.math("SUBTRACT", 1.0, g.math("MULTIPLY", halo, 0.4)))
    emit = g.mix(crack, g.vscale(magma, g.math("MULTIPLY", halo, 0.05)), g.vscale(g.mix(0.25, magma, magma_hot), 0.4))
    rough = g.math("ADD", g.math("ADD", c["rough"], g.math("MULTIPLY", ash_m, 0.2)),
                   g.math("MULTIPLY", g.math("SUBTRACT", c["n_fine"], 0.5), 0.16), clamp=True)
    height = g.math("SUBTRACT", c["n_fine"], g.math("MULTIPLY", halo, 1.2))
    return _finish(g, base, rough, emit, height, c["pg"], distance=0.025)


# ============================================================================ atlas + bake

def _tri_count(me):
    return sum(len(p.vertices) - 2 for p in me.polygons)


def atlas_bake(set_name, objs, tex_dir, size=1024, weights=None, cavity=0.3, samples=8, ao_samples=24,
               angle=66.0, margin_frac=0.004, spread=30.0, shape="CONCAVE"):
    """Unwrap `objs` into one shared UV atlas, bake T_<set_name>_BC/N/ORM/E with the texturing skill
    script, write the UVs back to the objects and give them the baked material M_<set_name>."""
    t0 = time.time()
    bake = skill("blender-texturing/scripts/bake_texture_set.py")
    rocks = skill("blender-game-asset/scripts/procedural_rocks.py")
    scene = bpy.context.scene
    bm = bmesh.new()
    ranges = []
    for i, obj in enumerate(objs):
        nv0 = len(bm.verts)
        nl0 = sum(len(f.verts) for f in bm.faces)
        bm.from_mesh(obj.data)
        bm.verts.ensure_lookup_table()
        dx = spread * i
        for v in bm.verts[nv0:]:
            v.co.x += dx
        nl1 = sum(len(f.verts) for f in bm.faces)
        ranges.append((nl0, nl1, dx))
    me = bpy.data.meshes.new("BAKE_" + set_name)
    bm.to_mesh(me)
    bm.free()
    for p_src in objs[0].data.materials:
        me.materials.append(p_src)
    job = bpy.data.objects.new("BAKE_" + set_name, me)
    scene.collection.objects.link(job)
    method = rocks["smart_uv"](job, angle_deg=angle, island_margin=0.0)
    if method != "smart_project":
        raise RuntimeError("smart UV failed for {}: {}".format(set_name, method))
    uvl = me.uv_layers["UVMap"]
    n = len(uvl.data)
    uv = np.empty(n * 2, dtype=np.float32)
    uvl.data.foreach_get("uv", uv)
    uv = uv.reshape(n, 2)
    for (a, b, _dx), w in zip(ranges, weights or [1.0] * len(objs)):
        c = uv[a:b].mean(axis=0)
        uv[a:b] = (uv[a:b] - c) * w + c
    uvl.data.foreach_set("uv", uv.ravel())
    vl = bpy.context.view_layer
    for o in vl.objects:
        o.select_set(False)
    job.select_set(True)
    vl.objects.active = job
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    try:
        bpy.ops.uv.select_all(action="SELECT")
    except Exception:
        pass
    bpy.ops.uv.pack_islands(rotate=True, margin_method="FRACTION", margin=margin_frac, shape_method=shape)
    bpy.ops.object.mode_set(mode="OBJECT")
    job.select_set(False)
    uvl = me.uv_layers["UVMap"]
    uv = np.empty(n * 2, dtype=np.float32)
    uvl.data.foreach_get("uv", uv)
    uv = uv.reshape(n, 2)
    if uv.min() < -1e-4 or uv.max() > 1.0 + 1e-4:
        raise RuntimeError("{}: UVs outside 0-1 after packing ({} .. {})".format(set_name, uv.min(), uv.max()))

    res = bake["main"](target=job.name, out_dir=tex_dir, asset_name=set_name, size=size, samples=samples,
                       ao_samples=ao_samples, margin=6, build_preview_material=True)
    # cavity: multiply a little of the baked AO into base colour
    if cavity > 0.0:
        bc = bpy.data.images.get("T_{}_BC".format(set_name))
        orm = bpy.data.images.get("T_{}_ORM".format(set_name))
        if bc is not None and orm is not None:
            w, h = bc.size
            a = np.empty(w * h * 4, dtype=np.float32)
            b = np.empty(w * h * 4, dtype=np.float32)
            bc.pixels.foreach_get(a)
            orm.pixels.foreach_get(b)
            a = a.reshape(-1, 4)
            ao = b.reshape(-1, 4)[:, 0:1]
            a[:, :3] *= 1.0 - cavity * (1.0 - ao)
            bc.pixels.foreach_set(a.ravel())
            bc.save()
    mat = bpy.data.materials.get(res["preview_material"])
    mat.name = "M_" + set_name
    # write the atlas UVs back, checking that loop order really matches
    density = {}
    for obj, (a, b, dx) in zip(objs, ranges):
        om = obj.data
        if len(om.loops) != b - a:
            raise RuntimeError("{}: loop count mismatch on UV write-back".format(obj.name))
        co_j = np.empty(len(me.vertices) * 3, dtype=np.float32)
        me.vertices.foreach_get("co", co_j)
        co_j = co_j.reshape(-1, 3)
        li = np.empty(len(me.loops), dtype=np.int32)
        me.loops.foreach_get("vertex_index", li)
        co_o = np.empty(len(om.vertices) * 3, dtype=np.float32)
        om.vertices.foreach_get("co", co_o)
        co_o = co_o.reshape(-1, 3)
        lo = np.empty(len(om.loops), dtype=np.int32)
        om.loops.foreach_get("vertex_index", lo)
        d = co_j[li[a:b]] - co_o[lo]
        d[:, 0] -= dx
        if float(np.abs(d).max()) > 1e-4:
            raise RuntimeError("{}: joined mesh order differs from the source mesh".format(obj.name))
        ouv = om.uv_layers.get("UVMap") or om.uv_layers.new(name="UVMap")
        ouv.data.foreach_set("uv", uv[a:b].ravel())
        om.materials.clear()
        om.materials.append(mat)
        # texel density (px per metre) = sqrt(uv area * size^2 / world area)
        area3d = sum(p.area for p in om.polygons)
        uva = 0.0
        for p in om.polygons:
            pts = uv[a + p.loop_start: a + p.loop_start + p.loop_total]
            x, y = pts[:, 0], pts[:, 1]
            uva += 0.5 * abs(float(np.dot(x, np.roll(y, -1)) - np.dot(y, np.roll(x, -1))))
        density[obj.name] = round(math.sqrt(uva * size * size / max(area3d, 1e-9)), 1)
        om.update()
    bpy.data.objects.remove(job, do_unlink=True)
    bpy.data.meshes.remove(me)
    files = {k: os.path.basename(v) for k, v in res["files"].items()}
    return {"set": set_name, "material": mat.name, "files": files, "size": size,
            "emission_strength": res["emission_strength"], "normal_convention": res["normal_convention"],
            "px_per_m": density, "uv_fill": round(float(sum(
                (density[o.name] ** 2) * sum(p.area for p in o.data.polygons) for o in objs)) / (size * size), 3),
            "seconds": round(time.time() - t0, 1), "warnings": res.get("warnings", [])}


# ============================================================================ collision

def _fib_dirs(n):
    golden = math.pi * (3.0 - math.sqrt(5.0))
    out = []
    for i in range(n):
        y = 1.0 - (i / float(n - 1)) * 2.0
        r = math.sqrt(max(0.0, 1.0 - y * y))
        out.append(Vector((math.cos(golden * i) * r, y, math.sin(golden * i) * r)))
    return out


def make_ucx(obj, pts, max_points=22, floor=-0.02):
    """One simple convex hull (support points in `max_points` directions), parented to the asset."""
    pts = [Vector((p.x, p.y, max(p.z, floor))) for p in pts]
    chosen = {}
    for d in _fib_dirs(max_points):
        best = max(pts, key=lambda p: p.dot(d))
        chosen[tuple(round(c, 5) for c in best)] = best
    bm = bmesh.new()
    for p in chosen.values():
        bm.verts.new(p)
    bmesh.ops.convex_hull(bm, input=list(bm.verts))
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    me = bpy.data.meshes.new("UCX_" + obj.name)
    bm.to_mesh(me)
    bm.free()
    ucx = bpy.data.objects.new("UCX_" + obj.name, me)
    for c in obj.users_collection:
        c.objects.link(ucx)
    ucx.parent = obj
    ucx.display_type = "WIRE"
    ucx.hide_render = True
    return ucx


# ============================================================================ checks + review

def check_asset(name, budget, review_dir):
    rep = skill("blender-mcp/scripts/asset_report.py")["main"](targets=[name])  # budget is checked below
    asm = skill("blender-mcp/scripts/assembly_check.py")["main"](targets=[name])
    obj = bpy.data.objects[name]
    me = obj.data
    zs = [v.co.z for v in me.vertices]
    xs = [v.co.x for v in me.vertices]
    ys = [v.co.y for v in me.vertices]
    main_rep = next(o for o in rep["objects"] if o["name"] == name)
    out = {
        "asset": name,
        "tris": main_rep["tris"],
        "budget": list(budget),
        "in_budget": budget[0] <= main_rep["tris"] <= budget[1],
        "height_cm": round(max(zs) * 100.0, 1),
        "below_ground_cm": round(-min(zs) * 100.0, 1),
        "width_cm": [round((max(xs) - min(xs)) * 100.0, 1), round((max(ys) - min(ys)) * 100.0, 1)],
        "base_center_offset_cm": None,
        "material_slots": main_rep["materials"],
        "uv_layers": main_rep.get("uv_layers"),
        "errors": rep["totals"]["errors"],
        "warnings": rep["totals"]["warnings"],
        "issues": rep["issues"],
        "assembly_ok": asm.get("ok"),
        "assembly": {k: asm.get(k) for k in ("islands", "groups", "floating", "tolerance_m")},
        "asset_report": rep,
    }
    # base centre: centroid of the footprint within 6 cm of the ground plane
    foot = [v.co for v in me.vertices if v.co.z < 0.08] or [v.co for v in me.vertices]
    if foot:
        cx = (max(p.x for p in foot) + min(p.x for p in foot)) * 0.5
        cy = (max(p.y for p in foot) + min(p.y for p in foot)) * 0.5
        out["base_center_offset_cm"] = [round(cx * 100.0, 1), round(cy * 100.0, 1)]
    os.makedirs(review_dir, exist_ok=True)
    with open(os.path.join(review_dir, name + "_report.json"), "w", encoding="utf-8") as fh:
        json.dump(out, fh, indent=1)
    return out


def review_asset(name, review_dir, modes=("clay", "material"), resolution=400):
    rr = skill("blender-mcp/scripts/review_renders.py")["main"]
    out = {}
    for mode in modes:
        r = rr(targets=[name], out_dir=review_dir, label=name, mode=mode, resolution=resolution, samples=16)
        out[mode] = os.path.basename(r["sheet"])
    return out


# ============================================================================ reference figure

def make_figure(collection):
    """180 cm scale figure for the contact sheets (never exported)."""
    b = Builder()
    grey = (0.32, 0.33, 0.36)

    def part(faces):
        b.paint(faces, col=grey, rough=0.7, noise=0.0, bump=0.0)

    for sx in (-1, 1):
        part(b.tube([Vector((sx * 0.09, 0, 0.0)), Vector((sx * 0.095, 0, 0.45)), Vector((sx * 0.1, 0, 0.9))],
                    [0.05, 0.06, 0.085], sides=8))
        part(b.tube([Vector((sx * 0.215, 0, 1.47)), Vector((sx * 0.25, 0, 1.15)), Vector((sx * 0.27, 0, 0.82))],
                    [0.05, 0.043, 0.035], sides=8))
    part(b.lathe([(0.0, 0.84), (0.15, 0.88), (0.14, 1.05), (0.19, 1.38), (0.17, 1.5), (0.06, 1.54), (0.05, 1.6),
                  (0.0, 1.6)], sides=12, rmod=lambda a, k, r, z: 1.0 if abs(math.sin(a)) < 0.5 else 0.72))
    part(b.lathe([(0.0, 1.57), (0.08, 1.6), (0.108, 1.69), (0.08, 1.78), (0.0, 1.8)], sides=12))
    obj, _ = b.finish("REF_Figure_180cm", collection)
    g = Nodes("M_REF_Figure")
    g.bsdf.inputs["Base Color"].default_value = (0.3, 0.31, 0.34, 1.0)
    g.bsdf.inputs["Roughness"].default_value = 0.7
    g.bsdf.inputs["Emission Color"].default_value = (0.3, 0.31, 0.34, 1.0)
    g.bsdf.inputs["Emission Strength"].default_value = 0.12
    obj.data.materials.append(g.mat)
    return obj


# ============================================================================ kit renders

_TMP = "__kitsheet__"


def _load_png(path):
    img = bpy.data.images.load(path, check_existing=False)
    w, h = img.size
    px = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(px)
    bpy.data.images.remove(img)
    return px.reshape(h, w, 4)


def _save_png(arr, path):
    h, w = arr.shape[:2]
    img = bpy.data.images.new(_TMP + "out", width=w, height=h, alpha=False)
    img.pixels.foreach_set(arr.astype(np.float32).ravel())
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


def _label_material():
    name = "M_REF_Label"
    m = bpy.data.materials.get(name)
    if m is None:
        g = Nodes(name)
        g.bsdf.inputs["Base Color"].default_value = (0.02, 0.02, 0.02, 1.0)
        g.bsdf.inputs["Emission Color"].default_value = (0.85, 0.87, 0.9, 1.0)
        g.bsdf.inputs["Emission Strength"].default_value = 1.0
        m = g.mat
    return m


def _text(scene, body, loc, size=0.15, temp=None):
    cu = bpy.data.curves.new(_TMP + "txt", "FONT")
    cu.body = body
    cu.size = size
    cu.align_x = "CENTER"
    cu.materials.append(_label_material())
    ob = bpy.data.objects.new(_TMP + "txt", cu)
    ob.location = loc
    ob.rotation_euler = (math.radians(90.0), 0.0, 0.0)
    scene.collection.objects.link(ob)
    if temp is not None:
        temp.append(ob)
    return ob


def _temp_scene(engine, view_transform, bg, world_strength, samples=24):
    scene = bpy.data.scenes.new(_TMP + "scene")
    scene.render.engine = engine
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.view_settings.view_transform = view_transform
    world = bpy.data.worlds.new(_TMP + "world")
    scene.world = world
    nt = world.node_tree
    bgn = next(n for n in nt.nodes if n.type == "BACKGROUND")
    bgn.inputs["Color"].default_value = (bg[0], bg[1], bg[2], 1.0)
    bgn.inputs["Strength"].default_value = world_strength
    if engine == "BLENDER_EEVEE":
        scene.eevee.taa_render_samples = samples
    else:
        sh = scene.display.shading
        sh.light = "STUDIO"
        sh.color_type = "SINGLE"
        sh.single_color = (0.72, 0.72, 0.74)
        sh.show_cavity = True
        sh.cavity_type = "BOTH"
        sh.show_object_outline = True
        scene.display.render_aa = "8"
    return scene, world


def _sun(scene, name, energy, direction, color=(1, 1, 1), temp=None, angle=6.0):
    ld = bpy.data.lights.new(_TMP + name, "SUN")
    ld.energy = energy
    ld.color = color
    ld.angle = math.radians(angle)
    lo = bpy.data.objects.new(_TMP + name, ld)
    lo.rotation_euler = Vector(direction).normalized().to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(lo)
    if temp is not None:
        temp.append(lo)
    return lo


def _cleanup(scene, world, temp):
    for o in temp:
        data = o.data
        bpy.data.objects.remove(o, do_unlink=True)
        if data is not None and data.users == 0:
            if isinstance(data, bpy.types.Mesh):
                bpy.data.meshes.remove(data)
            elif isinstance(data, bpy.types.Curve):
                bpy.data.curves.remove(data)
            elif isinstance(data, bpy.types.Light):
                bpy.data.lights.remove(data)
            elif isinstance(data, bpy.types.Camera):
                bpy.data.cameras.remove(data)
    bpy.data.scenes.remove(scene)
    bpy.data.worlds.remove(world)


def contact_sheet(items, figure, out_path, mode="lit", ppm=120.0, max_row_w=12.5, gap=0.45, title=""):
    """Every piece in rows at ONE scale (orthographic front view, `ppm` pixels per metre) next to the
    180 cm figure. items = [(object, label)]. mode: lit | dark | clay."""
    engine = "BLENDER_WORKBENCH" if mode == "clay" else "BLENDER_EEVEE"
    bg = {"lit": (0.11, 0.12, 0.14), "dark": (0.004, 0.004, 0.006), "clay": (0.16, 0.17, 0.19)}[mode]
    scene, world = _temp_scene(engine, "Standard", bg, {"lit": 1.0, "dark": 1.0, "clay": 1.0}[mode])
    temp = []
    try:
        if mode != "clay":
            k = 1.0 if mode == "lit" else 0.03
            _sun(scene, "key", 3.2 * k, (0.45, 0.8, -0.75), temp=temp)
            _sun(scene, "fill", 1.1 * k, (-0.8, 0.5, -0.25), color=(0.75, 0.85, 1.0), temp=temp)
            _sun(scene, "rim", 1.6 * k, (-0.2, -1.0, -0.5), temp=temp)
        cam_d = bpy.data.cameras.new(_TMP + "cam")
        cam_d.type = "ORTHO"
        cam = bpy.data.objects.new(_TMP + "cam", cam_d)
        scene.collection.objects.link(cam)
        temp.append(cam)
        scene.camera = cam
        cam.rotation_euler = (math.radians(90.0), 0.0, 0.0)

        def dims(o):
            xs = [v.co.x for v in o.data.vertices]
            zs = [v.co.z for v in o.data.vertices]
            return min(xs), max(xs), max(zs)

        fig_dims = dims(figure)
        rows, cur, cur_w = [], [], 0.0
        for obj, label in items:
            x0, x1, _z = dims(obj)
            w = max(x1 - x0, 0.9) + gap
            if cur and cur_w + w > max_row_w:
                rows.append(cur)
                cur, cur_w = [], 0.0
            cur.append((obj, label))
            cur_w += w
        if cur:
            rows.append(cur)
        strips = []
        for row in rows:
            placed = []
            x = gap
            for obj, label in [(figure, "180 cm")] + row:
                x0, x1, zt = dims(obj)
                w = max(x1 - x0, 0.9)
                cx = x + w * 0.5
                dup = obj.copy()
                dup.location = (cx - (x0 + x1) * 0.5, 0.0, 0.0)
                dup.hide_render = False
                dup.parent = None
                scene.collection.objects.link(dup)
                placed.append(dup)
                placed.append(_text(scene, label, (cx, -3.0, -0.33), size=0.13))
                x += w + gap
            row_w = x
            row_h = max(dims(o)[2] for o, _l in row + [(figure, "")]) + 0.25
            z_lo = -0.55
            # ground line
            gm = bpy.data.meshes.new(_TMP + "ground")
            gb = bmesh.new()
            bmesh.ops.create_cube(gb, size=1.0, matrix=Matrix.Translation((row_w * 0.5, 4.0, -0.006))
                                  @ Matrix.Diagonal((row_w, 0.02, 0.012, 1.0)))
            gb.to_mesh(gm)
            gb.free()
            gm.materials.append(_label_material())
            go = bpy.data.objects.new(_TMP + "ground", gm)
            scene.collection.objects.link(go)
            placed.append(go)
            res_x = int(round(row_w * ppm))
            res_y = int(round((row_h - z_lo) * ppm))
            scene.render.resolution_x, scene.render.resolution_y = res_x, res_y
            scene.render.resolution_percentage = 100
            cam_d.ortho_scale = max(row_w, row_h - z_lo)
            cam.location = (row_w * 0.5, -40.0, (row_h + z_lo) * 0.5)
            cam_d.clip_start, cam_d.clip_end = 0.1, 200.0
            tmp_png = out_path + ".row.png"
            scene.render.filepath = tmp_png
            bpy.ops.render.render(write_still=True, scene=scene.name)
            strips.append(_load_png(tmp_png))
            os.remove(tmp_png)
            for o in placed:
                data = o.data
                bpy.data.objects.remove(o, do_unlink=True)
                if isinstance(data, bpy.types.Curve) and data.users == 0:
                    bpy.data.curves.remove(data)
                elif isinstance(data, bpy.types.Mesh) and data.users == 0 and data.name.startswith(_TMP):
                    bpy.data.meshes.remove(data)
        width = max(s.shape[1] for s in strips)
        pad = 8
        total_h = sum(s.shape[0] for s in strips) + pad * (len(strips) + 1)
        sheet = np.zeros((total_h, width + 2 * pad, 4), dtype=np.float32)
        sheet[:, :, :3] = np.array(bg) * 0.5
        sheet[:, :, 3] = 1.0
        y = total_h - pad
        for s in strips:  # first row at the top (image rows run bottom-to-top)
            h, w = s.shape[:2]
            y -= h
            sheet[y:y + h, pad:pad + w] = s
            sheet[y:y + h, pad:pad + w, 3] = 1.0
            y -= pad
        _save_png(sheet, out_path)
    finally:
        _cleanup(scene, world, temp)
    return {"path": out_path, "rows": len(strips), "px_per_m": ppm, "mode": mode,
            "size_px": [int(sheet.shape[1]), int(sheet.shape[0])],
            "figure_height_cm": round(fig_dims[2] * 100.0, 1)}


def first_person_view(layout, figure, out_path, ground=(0.02, 0.03, 0.03), sky=(0.02, 0.025, 0.05),
                      sky_strength=1.0, sun=(0.35, (0.4, 0.5, 1.0)), fig_at=None, res=(1280, 720),
                      cam=(0.0, -8.4, 1.54), look=(0.0, 0.0, 1.3), lens=24.0):
    """Perspective check from first-person eye height (154 cm) in dusk-like light, so the glow and the
    silhouettes are judged the way the player meets them. layout = [(object, x, y, rot_deg)]."""
    scene, world = _temp_scene("BLENDER_EEVEE", "AgX", sky, sky_strength, samples=32)
    temp = []
    try:
        try:
            scene.eevee.use_raytracing = True
        except Exception:
            pass
        energy, color = sun
        _sun(scene, "key", energy, (0.5, 0.6, -0.45), color=color, temp=temp, angle=12.0)
        _sun(scene, "rim", energy * 0.6, (-0.5, -0.8, -0.3), color=color, temp=temp, angle=12.0)
        gm = bpy.data.meshes.new(_TMP + "floor")
        gb = bmesh.new()
        bmesh.ops.create_grid(gb, x_segments=1, y_segments=1, size=60.0)
        gb.to_mesh(gm)
        gb.free()
        g = Nodes(_TMP + "floor")
        g.bsdf.inputs["Base Color"].default_value = (ground[0], ground[1], ground[2], 1.0)
        g.bsdf.inputs["Roughness"].default_value = 0.85
        gm.materials.append(g.mat)
        go = bpy.data.objects.new(_TMP + "floor", gm)
        scene.collection.objects.link(go)
        temp.append(go)
        for obj, x, y, rot in layout:
            dup = obj.copy()
            dup.parent = None
            dup.hide_render = False
            dup.location = (x, y, 0.0)
            dup.rotation_euler = (0.0, 0.0, math.radians(rot))
            scene.collection.objects.link(dup)
            temp.append(dup)
        if fig_at is not None:
            dup = figure.copy()
            dup.location = (fig_at[0], fig_at[1], 0.0)
            scene.collection.objects.link(dup)
            temp.append(dup)
        cam_d = bpy.data.cameras.new(_TMP + "cam")
        cam_d.lens = lens
        cam_o = bpy.data.objects.new(_TMP + "cam", cam_d)
        cam_o.location = cam
        cam_o.rotation_euler = (Vector(look) - Vector(cam)).to_track_quat("-Z", "Y").to_euler()
        scene.collection.objects.link(cam_o)
        temp.append(cam_o)
        scene.camera = cam_o
        scene.render.resolution_x, scene.render.resolution_y = res
        scene.render.resolution_percentage = 100
        scene.render.filepath = out_path
        bpy.ops.render.render(write_still=True, scene=scene.name)
    finally:
        fm = bpy.data.materials.get(_TMP + "floor")
        _cleanup(scene, world, temp)
        if fm is not None and fm.users == 0:
            bpy.data.materials.remove(fm)
    return {"path": out_path, "eye_height_cm": round(cam[2] * 100.0, 1), "lens_mm": lens}


# ============================================================================ packaging

def package(kit_dir, asset_names, sets_by_asset, set_info, extra_manifest=None):
    """Write <kit>/<Asset>/<Asset>.blend (asset + UCX only), export the FBX and manifest with the
    blender-unreal-export skill script, and add the shared texture set to the manifest."""
    tex_dir = os.path.join(kit_dir, "textures")
    tmp = os.path.join(bpy.app.tempdir, "engprops_kit_tmp.blend")
    bpy.ops.wm.save_as_mainfile(filepath=tmp, check_existing=False, copy=True)
    results = {}
    for name in asset_names:
        bpy.ops.wm.open_mainfile(filepath=tmp)
        keep = {name, "UCX_" + name}
        for o in list(bpy.data.objects):
            if o.name not in keep:
                bpy.data.objects.remove(o, do_unlink=True)
        for sc in list(bpy.data.scenes):
            if sc != bpy.context.scene:
                bpy.data.scenes.remove(sc)
        for c in list(bpy.data.collections):
            if not c.all_objects:
                bpy.data.collections.remove(c)
        bpy.data.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)
        scene = bpy.context.scene
        scene.name = name
        asset_dir = os.path.join(kit_dir, name)
        os.makedirs(asset_dir, exist_ok=True)
        blend_path = os.path.join(asset_dir, name + ".blend")
        bpy.ops.wm.save_as_mainfile(filepath=blend_path, check_existing=False, compress=True, relative_remap=True)
        ex = skill("blender-unreal-export/scripts/export_for_unreal.py")
        # The skill's exporter fails in its own clean-up when the asset root has no child object
        # (view layer returns None after the temporary copy is removed; seen on Blender 5.2.2). A
        # temporary empty child avoids it; it is not exported and is not in the saved .blend.
        anchor = None
        if not bpy.data.objects[name].children:
            anchor = bpy.data.objects.new("NOTE_export_anchor", None)
            bpy.data.objects[name].users_collection[0].objects.link(anchor)
            anchor.parent = bpy.data.objects[name]
            bpy.context.view_layer.update()
        # collision="none": only the authored UCX_ hull (if the asset has one) is exported
        man = ex["main"](root=name, out_dir=asset_dir, textures_dir=tex_dir, collision="none")
        if anchor is not None:
            bpy.data.objects.remove(anchor, do_unlink=True)
        info = set_info[sets_by_asset[name]]
        man_path = os.path.join(asset_dir, name + ".manifest.json")
        rel = os.path.relpath(os.path.join(asset_dir, name + ".fbx"), ROOT).replace("\\", "/")
        man["fbx"] = rel
        man["fbx_path_is"] = "relative to the repository root"
        man["pivot"] = "root object origin ({}): the base centre, on the ground plane".format(name)
        man["textures"] = [info["files"][k] for k in ("BC", "N", "ORM", "E") if k in info["files"]]
        man["texture_set"] = {
            "name": info["set"],
            "shared_atlas": True,
            "dir": os.path.relpath(tex_dir, ROOT).replace("\\", "/"),
            "files": info["files"],
            "size": info["size"],
            "emission_strength": info["emission_strength"],
            "normal_convention": info["normal_convention"],
            "material": info["material"],
            "px_per_m": info["px_per_m"].get(name),
        }
        if extra_manifest and name in extra_manifest:
            man.update(extra_manifest[name])
        man.pop("seconds", None)
        with open(man_path, "w", encoding="utf-8") as fh:
            json.dump(man, fh, indent=2)
        for junk in (blend_path + "1",):
            if os.path.exists(junk):
                os.remove(junk)
        results[name] = {"tris": man["tris"], "size_cm": man["size_cm"], "collision": man["collision"],
                         "materials": man["materials"], "status": man["status"], "warnings": man["warnings"],
                         "blend_kb": round(os.path.getsize(blend_path) / 1024.0, 1),
                         "fbx_kb": round(os.path.getsize(os.path.join(asset_dir, name + ".fbx")) / 1024.0, 1)}
    try:
        os.remove(tmp)
    except OSError:
        pass
    return results


def fbx_round_trip(kit_dir, asset_names, expect):
    """Re-import every exported FBX into an empty file and compare with what was built."""
    out = {}
    for name in asset_names:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        bpy.ops.import_scene.fbx(filepath=os.path.join(kit_dir, name, name + ".fbx"))
        objs = {o.name: o for o in bpy.data.objects}
        sm = objs.get(name)
        e = expect[name]
        r = {"objects": sorted(objs), "ok": False}
        if sm is not None:
            me = sm.data
            tris = _tri_count(me)
            dims = [round(d * 100.0, 1) for d in sm.dimensions]
            zs = [(sm.matrix_world @ v.co).z for v in me.vertices]
            ucx = sorted(n for n in objs if n.startswith("UCX_"))
            r.update({
                "tris": tris, "dimensions_cm": dims, "height_cm": round(max(zs) * 100.0, 1),
                "scale": [round(s, 4) for s in sm.scale], "uv_layers": [u.name for u in me.uv_layers],
                "materials": [m.name if m else None for m in me.materials], "collision": ucx,
                "all_triangles": all(len(p.vertices) == 3 for p in me.polygons),
            })
            want_ucx = ["UCX_{}_00".format(name)] if e["collision"] else []
            r["ok"] = (tris == e["tris"] and abs(r["height_cm"] - e["height_cm"]) < 0.6 and ucx == want_ucx
                       and len(me.materials) == 1 and len(me.uv_layers) == 1 and r["all_triangles"]
                       and all(abs(s - 1.0) < 1e-3 for s in sm.scale))
        out[name] = r
    return out

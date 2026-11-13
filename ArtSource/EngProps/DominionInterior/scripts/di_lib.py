# SPDX-License-Identifier: MIT
# DominionInterior - geometry helpers. Pieces are authored in CENTIMETRES (Unreal units, +X forward, +Z up) as
# lists of polygons whose UVs point into the shared atlas (di_regions.py); Piece.build() makes the Blender mesh
# (metres) with its UCX_ collision children.
import math
import os
import sys

import bmesh
import bpy
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from di_regions import ATLAS, REGIONS  # noqa: E402

AX = {"x": 0, "y": 1, "z": 2}
INSET = 1.0     # px kept clear inside every region (mip bleed)
GLOW = ("GlowRed", "GlowWhite", "Screen", "Cell", "CoreGlow", "Shield")


def rotz(deg):
    return Matrix.Rotation(math.radians(deg), 4, "Z")


def window(lo, hi, size=400.0):
    """The 400 cm grid cell (c0, c1) holding [lo, hi]; falls back to the range itself when it spans more."""
    base = math.floor((lo + size / 2 + 1e-6) / size) * size - size / 2
    if hi <= base + size + 1e-6:
        return base, base + size
    return lo, hi


class Piece:
    def __init__(self, name, expect=None):
        self.name = name
        self.expect = expect          # ((minx, miny, minz), (maxx, maxy, maxz)) in cm
        self.faces = []               # (verts, uvs, material name)
        self.col = []                 # (lo, hi) boxes
        self.hulls = []               # lists of points
        self.M = Matrix.Identity(4)
        self.notes = []

    # ---------------------------------------------------------------- transforms
    class _At:
        def __init__(self, piece, m):
            self.p, self.m = piece, m

        def __enter__(self):
            self.old = self.p.M
            self.p.M = self.old @ self.m
            return self.p

        def __exit__(self, *a):
            self.p.M = self.old

    def at(self, m):
        return Piece._At(self, m)

    # ---------------------------------------------------------------- faces
    def _uv(self, reg, u, v):
        if reg is None:
            return (0.985, 0.985)
        rx, ry, rw, rh = REGIONS[reg]
        if u < -0.002 or u > 1.002 or v < -0.002 or v > 1.002:
            self.notes.append("uv outside %s: %.3f %.3f" % (reg, u, v))
        u, v = min(1.0, max(0.0, u)), min(1.0, max(0.0, v))
        px = rx + INSET + u * (rw - 2 * INSET)
        py = ry + INSET + (1.0 - v) * (rh - 2 * INSET)
        return (px / ATLAS, 1.0 - py / ATLAS)

    def poly(self, vs, reg=None, U=None, V=None, uv=None, mat="Atlas", n=None):
        """vs: points (cm). U / V: (axis, c0, c1[, t0, t1]) maps that coordinate range onto the region (or a
        sub-range t0..t1 of it). uv: explicit region-normalised pairs. n: wanted normal (winding is fixed to it)."""
        vs = [Vector(v) for v in vs]
        if mat in GLOW:
            reg = None
        if uv is None and reg is not None:
            def m(spec, p):
                a, c0, c1 = spec[0], spec[1], spec[2]
                t = (p[AX[a]] - c0) / (c1 - c0) if abs(c1 - c0) > 1e-9 else 0.0
                if len(spec) == 5:
                    t = spec[3] + t * (spec[4] - spec[3])
                return t
            uv = [(m(U, p), m(V, p)) for p in vs]
        if uv is None:
            uv = [(0, 0)] * len(vs)
        if n is not None:
            nn = Vector((0, 0, 0))
            for i in range(len(vs)):
                a, b = vs[i], vs[(i + 1) % len(vs)]
                nn += Vector(((a.y - b.y) * (a.z + b.z), (a.z - b.z) * (a.x + b.x), (a.x - b.x) * (a.y + b.y)))
            if nn.dot(Vector(n)) < 0:
                vs, uv = vs[::-1], uv[::-1]
        out = [self.M @ v for v in vs]
        if self.M.determinant() < 0:
            out, uv = out[::-1], uv[::-1]
        self.faces.append((out, [self._uv(reg, a, b) for a, b in uv], mat))

    def box(self, lo, hi, reg="sw_gun", faces="xXyYzZ", regs=None, uvo=None, mat="Atlas", mats=None, top_along="x"):
        """Axis box. faces: lower case = the min side of that axis. regs / uvo / mats: per-face overrides."""
        x0, y0, z0 = lo
        x1, y1, z1 = hi
        regs, uvo, mats = regs or {}, uvo or {}, mats or {}
        wx, wy = window(x0, x1), window(y0, y1)
        side = {
            "x": ([(x0, y0, z0), (x0, y1, z0), (x0, y1, z1), (x0, y0, z1)], (-1, 0, 0), ("y",) + wy, ("z", z0, z1)),
            "X": ([(x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1)], (1, 0, 0), ("y",) + wy, ("z", z0, z1)),
            "y": ([(x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1)], (0, -1, 0), ("x",) + wx, ("z", z0, z1)),
            "Y": ([(x0, y1, z0), (x1, y1, z0), (x1, y1, z1), (x0, y1, z1)], (0, 1, 0), ("x",) + wx, ("z", z0, z1)),
            "z": ([(x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0)], (0, 0, -1), None, None),
            "Z": ([(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)], (0, 0, 1), None, None),
        }
        for f in faces:
            vs, n, U, V = side[f]
            if U is None:
                if top_along == "x":
                    U, V = ("x",) + wx, ("y", y0, y1)
                else:
                    U, V = ("y",) + wy, ("x", x0, x1)
            if f in uvo:
                U, V = uvo[f]
            self.poly(vs, regs.get(f, reg), U, V, mat=mats.get(f, mat), n=n)

    def prism(self, prof, plane, e0, e1, regs="sw_gun", cap_reg="sw_gun", caps=True, mat="Atlas", fitu=False,
              seg_mats=None, vr=None):
        """Extrude a 2D polygon. plane 'xz' -> extruded along y; 'yz' -> along x; 'xy' -> along z.
        regs: one region or one per segment (None skips that side). Sides: U runs along the extrusion (400 cm
        grid window, or fitted), V across the segment."""
        ext = {"xz": "y", "yz": "x", "xy": "z"}[plane]

        def P(a, b, e):
            return {"xz": (a, e, b), "yz": (e, a, b), "xy": (a, b, e)}[plane]

        area = sum(prof[i][0] * prof[(i + 1) % len(prof)][1] - prof[(i + 1) % len(prof)][0] * prof[i][1]
                   for i in range(len(prof)))
        sgn = 1.0 if area > 0 else -1.0
        w = (e0, e1) if (fitu or ext == "z") else window(e0, e1)
        u0, u1 = (e0 - w[0]) / (w[1] - w[0]), (e1 - w[0]) / (w[1] - w[0])
        for i in range(len(prof)):
            reg = regs if isinstance(regs, str) else regs[i]
            if reg is None:
                continue
            a, b = prof[i], prof[(i + 1) % len(prof)]
            n2 = (sgn * (b[1] - a[1]), -sgn * (b[0] - a[0]))
            n3 = P(n2[0], n2[1], 0.0)
            v0, v1 = (vr[i] if vr and vr[i] else (0.0, 1.0))
            self.poly([P(a[0], a[1], e0), P(a[0], a[1], e1), P(b[0], b[1], e1), P(b[0], b[1], e0)], reg,
                      uv=[(u0, v0), (u1, v0), (u1, v1), (u0, v1)], n=n3,
                      mat=(seg_mats[i] if seg_mats and seg_mats[i] else mat))
        if caps:
            la, ha = min(p[0] for p in prof), max(p[0] for p in prof)
            lb, hb = min(p[1] for p in prof), max(p[1] for p in prof)
            for e, s in ((e0, -1.0), (e1, 1.0)):
                self.poly([P(a, b, e) for a, b in prof], cap_reg,
                          uv=[((a - la) / (ha - la), (b - lb) / (hb - lb)) for a, b in prof], n=P(0, 0, s), mat=mat)

    def lathe(self, prof, n, regs="sw_gun", reps=1, center=(0, 0), mat="Atlas", a0=0.0, a1=360.0, seg_mats=None,
              vr=None, closed=False):
        """Revolve (r, z) points about Z. regs: one per profile segment or one for all. reps: texture repeats
        around the turn (n must divide by it)."""
        m = len(prof) if closed else len(prof) - 1
        for i in range(n):
            t0 = math.radians(a0 + (a1 - a0) * i / n)
            t1 = math.radians(a0 + (a1 - a0) * (i + 1) / n)
            u0 = (i * reps / n) % 1.0
            u1 = u0 + reps / n
            for j in range(m):
                reg = regs if isinstance(regs, str) else regs[j]
                if reg is None:
                    continue
                (r0, z0), (r1, z1) = prof[j], prof[(j + 1) % len(prof)]
                v0, v1 = (vr[j] if vr and vr[j] else (0.0, 1.0))
                pts = [(center[0] + r0 * math.cos(t0), center[1] + r0 * math.sin(t0), z0),
                       (center[0] + r0 * math.cos(t1), center[1] + r0 * math.sin(t1), z0),
                       (center[0] + r1 * math.cos(t1), center[1] + r1 * math.sin(t1), z1),
                       (center[0] + r1 * math.cos(t0), center[1] + r1 * math.sin(t0), z1)]
                uvs = [(u0, v0), (u1, v0), (u1, v1), (u0, v1)]
                if r0 < 1e-6:
                    pts, uvs = pts[1:], uvs[1:]
                elif r1 < 1e-6:
                    pts, uvs = pts[:3], uvs[:3]
                tm = 0.5 * (t0 + t1)
                dr, dz = r1 - r0, z1 - z0
                nn = (dz * math.cos(tm), dz * math.sin(tm), -dr)   # right of the travel direction = outward
                self.poly(pts, reg, uv=uvs, n=nn, mat=(seg_mats[j] if seg_mats and seg_mats[j] else mat))

    # ---------------------------------------------------------------- collision
    def cbox(self, lo, hi):
        pts = [self.M @ Vector((x, y, z)) for x in (lo[0], hi[0]) for y in (lo[1], hi[1]) for z in (lo[2], hi[2])]
        self.hulls.append(pts)

    def chull(self, pts):
        self.hulls.append([self.M @ Vector(p) for p in pts])

    def bounds(self):
        vs = [v for f in self.faces for v in f[0]]
        return (tuple(round(min(v[i] for v in vs), 3) for i in range(3)),
                tuple(round(max(v[i] for v in vs), 3) for i in range(3)))

    def all_points(self):
        return [v for f in self.faces for v in f[0]]

    # ---------------------------------------------------------------- Blender mesh
    def build(self, materials, collection, with_collision=True, smooth_deg=38.0):
        names = []
        for _, _, m in self.faces:
            if m not in names:
                names.append(m)
        names.sort(key=lambda m: (m != "Atlas", m))
        bm = bmesh.new()
        uvl = bm.loops.layers.uv.new("UVMap")
        # Pieces are authored in Unreal's axes. FBX hands Blender's +Y to Unreal as -Y (measured on import: the walkway
        # rail landed on -Y), so the Blender mesh is built as the Y mirror: winding reversed, Y negated.
        for vs, uvs, m in self.faces:
            vs, uvs = vs[::-1], uvs[::-1]
            bv = [bm.verts.new(Vector((v.x, -v.y, v.z)) * 0.01) for v in vs]
            try:
                f = bm.faces.new(bv)
            except ValueError:
                continue
            f.material_index = names.index(m)
            f.smooth = True
            for loop, uv in zip(f.loops, uvs):
                loop[uvl].uv = uv
        bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=1e-5)
        if getattr(self, "fix_normals", False):
            bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
        smooth_deg = getattr(self, "smooth_deg", smooth_deg)
        mesh = bpy.data.meshes.new(self.name)
        bm.to_mesh(mesh)
        bm.free()
        for m in names:
            mesh.materials.append(materials[m])
        mesh.set_sharp_from_angle(angle=math.radians(smooth_deg))
        obj = bpy.data.objects.new(self.name, mesh)
        collection.objects.link(obj)
        if with_collision:
            for i, pts in enumerate(self.hulls):
                hb = bmesh.new()
                for p in pts:
                    hb.verts.new(Vector((p.x, -p.y, p.z)) * 0.01)
                bmesh.ops.convex_hull(hb, input=list(hb.verts))
                me = bpy.data.meshes.new("UCX_%s_%02d" % (self.name, i))
                hb.to_mesh(me)
                hb.free()
                me.materials.append(materials[names[0]])
                o = bpy.data.objects.new(me.name, me)
                collection.objects.link(o)
                o.parent = obj
                o.hide_render = True
                o.display_type = "WIRE"
        return obj

    def tris(self):
        return sum(len(f[0]) - 2 for f in self.faces)

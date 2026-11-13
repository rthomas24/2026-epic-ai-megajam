# SPDX-License-Identifier: MIT
"""
sq_ships.py - the three player ships of the Squad (Rift, Lancer, Bastion), route C: code-built from a written
design (engineering fallback; the art team's DA_*_Visuals in /Game/TriadDrift/Presentation replace them).
Blender 5.2, headless. Units metres, front -Y, up +Z (the exporter turns -Y into Unreal +X).
Unreal (x, y, z) = Blender (-y, -x, z) in this file's comments.

Shared squad design language (so the three read as one squad):
  - flowing superellipse hulls with circumferential panel grooves, rounded-chamfer hard surface
  - the SAME canopy (dark glossy bubble, graphite arches and sills) and the same dorsal livery spine behind it
  - the SAME engine nozzle (graphite bell with ribs, pearl-white collar with a squad-colour band, recessed glow core
    and hot lip in the squad colour)
  - pearl white (dominant) / graphite (secondary) / squad colour paint (accent) + squad colour emissive
Distinct silhouettes from behind (the chase view): Rift = up-swept crescent wing + V tails + twin engines;
Lancer = narrow needle, short drooping swept wings, one big engine + two small, single tall dorsal fin;
Bastion = wide flat block between two big missile pods, four engines in a row, a raised dome emitter on top.

    blender --background --factory-startup --python sq_build.py -- --asset SM_Squad_Rift --tag r1 [--stage blockout]
"""

import math
import random

import bmesh
import bpy
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

import sq_lib as L

COLL = "Squad"

SQUAD = {
    "Rift": {"accent": "#C40E9C", "accent2": "#A80B86", "glow": "#FF2BD6"},
    "Lancer": {"accent": "#0596D2", "accent2": "#0482B8", "glow": "#3FE8FF"},
    "Bastion": {"accent": "#E8860A", "accent2": "#CF7608", "glow": "#FFB21F"},
}
WHITE, WHITE2 = "#E4E8EE", "#D6DBE3"
GRAPHITE, GRAPHITE2 = "#30353D", "#292D34"
SEG = 32          # ring resolution of hulls


# =========================================================================== small math helpers

def rot(deg, axis):
    return Matrix.Rotation(math.radians(deg), 4, axis)


def T(x, y, z):
    return Matrix.Translation((x, y, z))


def V(*a):
    return Vector(a)


def coll():
    return L.ensure_collection(COLL)


# =========================================================================== geometry

def poly_loft(rings, cap=True, name=None):
    """Rings of Vectors (same count) or single Vectors (tips), connected in order, capped."""
    bm = bmesh.new()
    vr = []
    for r in rings:
        if isinstance(r, Vector):
            vr.append([bm.verts.new(r)])
        else:
            vr.append([bm.verts.new(p) for p in r])
    for a, b in zip(vr, vr[1:]):
        if len(a) == 1 and len(b) == 1:
            continue
        if len(a) == 1:
            for i in range(len(b)):
                bm.faces.new((a[0], b[(i + 1) % len(b)], b[i]))
        elif len(b) == 1:
            for i in range(len(a)):
                bm.faces.new((a[i], a[(i + 1) % len(a)], b[0]))
        else:
            for i in range(len(a)):
                bm.faces.new((a[i], a[(i + 1) % len(a)], b[(i + 1) % len(b)], b[i]))
    if cap:
        for r in (vr[0], vr[-1]):
            if len(r) > 2:
                bm.faces.new(r)
    return L.obj_from_bm(bm, coll(), name)


def sec(w, ht, hb, n=2.6, verts=SEG, flat=0.0):
    """Smooth superellipse section (x, z): half width w/2, top ht, bottom hb. flat > 0 flattens the belly."""
    pts = []
    for i in range(verts):
        t = 2.0 * math.pi * i / verts
        c, s = math.cos(t), math.sin(t)
        x = (w * 0.5) * math.copysign(abs(c) ** (2.0 / n), c)
        h = ht if s >= 0 else hb
        ns = n if s >= 0 else n + flat
        z = h * math.copysign(abs(s) ** (2.0 / ns), s)
        pts.append((x, z))
    return pts


def hull(stations, verts=SEG):
    """stations: dicts {y, w, ht, hb, z=0, x=0, n=2.6, flat=0, groove=0, gw=0.022}; w == 0 -> tip point."""
    rings = []
    for s in stations:
        x0, z0 = s.get("x", 0.0), s.get("z", 0.0)
        if s["w"] <= 1e-6:
            rings.append(V(x0, s["y"], z0))
            continue
        g = s.get("groove", 0.0)
        pts = sec(s["w"], s["ht"], s["hb"], s.get("n", 2.6), verts, s.get("flat", 0.0))
        if g:
            gw = s.get("gw", 0.022)
            ptsi = sec(s["w"] - 2 * g, s["ht"] - g, s["hb"] - g, s.get("n", 2.6), verts, s.get("flat", 0.0))
            rings.append([V(x0 + px, s["y"] - gw * 0.5, z0 + pz) for px, pz in pts])
            rings.append([V(x0 + px, s["y"], z0 + pz) for px, pz in ptsi])
            rings.append([V(x0 + px, s["y"] + gw * 0.5, z0 + pz) for px, pz in pts])
        else:
            rings.append([V(x0 + px, s["y"], z0 + pz) for px, pz in pts])
    return poly_loft(rings)


def foil(chord, t):
    """Rounded airfoil (y, z) from the leading edge, top then bottom."""
    return [(0.0, 0.0), (0.05 * chord, 0.33 * t), (0.18 * chord, 0.5 * t), (0.5 * chord, 0.46 * t),
            (0.82 * chord, 0.25 * t), (chord, 0.05 * t), (chord, -0.04 * t), (0.82 * chord, -0.17 * t),
            (0.5 * chord, -0.3 * t), (0.18 * chord, -0.33 * t), (0.05 * chord, -0.22 * t)]


def nudge(st, dx=-0.004):
    """A wing station moved 4 mm inboard and a hair thinner: the next segment overlaps the last one without coincident vertices."""
    x, yle, c, t, z = st
    return (x + dx, yle + 0.002, c - 0.004, t * 0.97, z)


def wing_loft(stations):
    """stations: (x, y_le, chord, thickness, z). Rings along x, capped at both ends."""
    return poly_loft([[V(x, yle + py, z + pz) for py, pz in foil(c, t)] for x, yle, c, t, z in stations])


def mirrored(obj):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.transform(bm, matrix=Matrix.Diagonal((-1.0, 1.0, 1.0, 1.0)), verts=list(bm.verts))
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    me = bpy.data.meshes.new(obj.name + "_mx")
    bm.to_mesh(me)
    bm.free()
    o = bpy.data.objects.new(L.uname("mx"), me)
    coll().objects.link(o)
    return o


def both(make, P, mat, **kw):
    o = make()
    m = mirrored(o)
    P.add(o, mat, **kw)
    P.add(m, mat, **kw)
    return o, m


def tube(points, r, sides=8, cap=True):
    return L.tube_path(coll(), points, r, sides=sides, cap=cap)


def lathe_y(profile, segments=20, center=(0.0, 0.0, 0.0)):
    o = L.lathe(coll(), [(p, r) for p, r in profile], segments=segments, axis="Y")
    o.data.transform(T(*center))
    return o


def lathe_z(profile, segments=24, center=(0.0, 0.0, 0.0)):
    o = L.lathe(coll(), [(p, r) for p, r in profile], segments=segments, axis="Z")
    o.data.transform(T(*center))
    return o


def torus(R, r, axis="Y", steps=40, sides=12, center=(0.0, 0.0, 0.0), squash=1.0):
    prof = [(R + r * math.cos(a), r * squash * math.sin(a)) for a in [math.tau * k / sides for k in range(sides)]]
    o = L.revolve_closed(coll(), prof, (0.0, 360.0), steps, axis=axis)
    o.data.transform(T(*center))
    return o


def box(size, center=(0.0, 0.0, 0.0)):
    return L.box(coll(), size, center)


def bar(p0, p1, w, h=None, up=(0.0, 0.0, 1.0)):
    o, m = L.bar(coll(), p0, p1, w, h, up=up)
    o.data.transform(m)
    return o


class Surf:
    """Ray-cast helper on the parts built so far (world space): put details ON the hull."""

    def __init__(self, objs):
        verts, polys = [], []
        for o in objs:
            mw = o.matrix_world
            base = len(verts)
            verts.extend(mw @ v.co for v in o.data.vertices)
            polys.extend([base + i for i in p.vertices] for p in o.data.polygons)
        self.tree = BVHTree.FromPolygons(verts, polys)

    def hit(self, origin, direction):
        loc, nrm, _i, _d = self.tree.ray_cast(Vector(origin), Vector(direction).normalized())
        return loc, nrm

    def top(self, x, y, z_from=3.0):
        loc, nrm = self.hit((x, y, z_from), (0, 0, -1))
        return loc, nrm


def frame_on(loc, nrm, up=(0.0, 0.0, 1.0)):
    n = Vector(nrm).normalized()
    u = Vector(up)
    if abs(n.dot(u)) > 0.95:
        u = Vector((0.0, -1.0, 0.0))
    x = u.cross(n).normalized()
    y = n.cross(x).normalized()
    m = Matrix((x, y, n)).transposed().to_4x4()
    m.translation = loc
    return m


def plate_on(P, mat, target, region, normal=(0, 0, 1), min_dot=0.5, thickness=0.02, gap=0.014, bevel=0.005):
    """Layered armour/livery plate from the target's faces inside region (hardsurface.plates), applied + added."""
    pl = L.hs["plates"](target, normal=normal, min_dot=min_dot, region=region, thickness=thickness, gap=gap,
                        lift=0.002, bevel=0.0, material=mat, name=L.uname("plate"))
    if pl is None:
        return None
    L.hs["apply_modifiers"](pl)
    pl.parent = None
    pl.matrix_world = Matrix.Identity(4)
    pl.data.materials.clear()
    P.add(pl, mat, bevel=bevel, segments=1, angle=35.0)
    return pl


def greeble_cluster(P, mat, surf, center, normal, count, size, height, spread, seed, along=None):
    rng = random.Random(seed)
    n = Vector(normal).normalized()
    t1 = (Vector(along) if along else n.orthogonal()).normalized()
    t1 = (t1 - n * t1.dot(n)).normalized()
    t2 = n.cross(t1).normalized()
    for _ in range(count):
        off = t1 * rng.uniform(-spread, spread) + t2 * rng.uniform(-spread * 0.5, spread * 0.5)
        o0 = Vector(center) + off + n * 2.0 * spread
        loc, nrm = surf.hit(o0, -n)
        if loc is None:
            continue
        sx, sy, sz = rng.uniform(*size), rng.uniform(*size), rng.uniform(*height)
        M = frame_on(loc, nrm, up=t2)
        b = box((sx, sy, sz + 0.02), (0, 0, sz * 0.5 - 0.01))
        P.add(b, mat, bevel=min(sx, sy, sz) * 0.15, segments=1, matrix=M)


def glow_line(P, m, pts, r, mat="glow", sides=6):
    pts = [Vector(q) for q in pts]
    t = tube(pts, r, sides=sides)
    P.add(t, m[mat], smooth=50.0)
    return t


# =========================================================================== materials

def mats(prefix, ship, scale=0.4):
    c = SQUAD[ship]
    clean = dict(dent=0.15, rough_var=0.03, var_amount=0.25)
    m = {}
    m["white"] = L.role_paint(prefix + "_White", WHITE, WHITE2, metallic=0.22, rough=0.34, wear=0.35,
                              wear_color="#9EA6B1", grime=0.05, scale=scale, seed=1, bump=0.15, **clean)
    m["graphite"] = L.role_paint(prefix + "_Graphite", GRAPHITE, GRAPHITE2, metallic=0.45, rough=0.44, wear=0.55,
                                 wear_color="#8C949E", grime=0.03, scale=scale, seed=2, bump=0.2, **clean)
    m["accent"] = L.role_paint(prefix + "_Accent", c["accent"], c["accent2"], metallic=0.3, rough=0.3, wear=0.3,
                               wear_color="#C9CED6", grime=0.03, scale=scale, seed=3, bump=0.12, **clean)
    m["metal"] = L.role_metal(prefix + "_Metal", "#59606A", rough=0.3, metallic=0.95, scale=scale, grime=0.15, seed=4)
    m["dark"] = L.role_paint(prefix + "_Dark", "#0F1115", "#121419", metallic=0.2, rough=0.66, wear=0.0, grime=0.0,
                             scale=scale, seed=5, bump=0.1)
    m["glass"] = L.role_paint(prefix + "_Glass", "#0E1C27", "#14293A", metallic=0.6, rough=0.07, wear=0.0, grime=0.0,
                              scale=scale, seed=6, bump=0.0, dent=0.0, rough_var=0.01, var_amount=0.6)
    m["glow"] = L.role_glow(prefix + "_Glow", c["glow"], level=1.0, base="#140A12")
    m["light"] = L.role_glow(prefix + "_Light", "#F2F6FF", level=1.0, base="#141414")
    m["glow_dim"] = L.role_glow(prefix + "_GlowDim", c["glow"], level=0.28, base="#0E0A0E")
    return m


# =========================================================================== shared squad parts

def nozzle(P, m, center, r, length, segs=32, ribs=8, glow="glow"):
    """The squad engine: graphite bell (ribbed), pearl-white collar with a squad-colour band, recessed glow
    core and a glowing hot lip just inside the exit. Built along +Y (aft) from center."""
    cx, cy, cz = center
    L_ = length
    bell = lathe_y([(0.0, r * 0.86), (L_ * 0.2, r * 0.95), (L_ * 0.7, r * 1.0), (L_, r * 1.06), (L_, r * 0.95),
                    (L_ * 0.55, r * 0.84), (L_ * 0.55, 0.0)], segments=segs, center=(cx, cy, cz))
    P.add(bell, m["metal"], smooth=40.0)
    core = lathe_y([(L_ * 0.55 - 0.01, r * 0.84), (L_ * 0.6, r * 0.62), (L_ * 0.66, 0.0)], segments=segs,
                   center=(cx, cy, cz))
    P.add(core, m[glow], smooth=60.0)
    # small turbine hub + thin vanes: a bright glow ring around a small dark hub
    hub = lathe_y([(L_ * 0.5, r * 0.22), (L_ * 0.7, r * 0.19), (L_ * 0.82, 0.0)], segments=segs, center=(cx, cy, cz))
    P.add(hub, m["metal"], smooth=50.0)
    for k in range(6):
        a = math.tau * k / 6
        dd = V(math.cos(a), 0.0, math.sin(a))
        vane = bar(V(cx, cy + L_ * 0.64, cz) + dd * r * 0.18, V(cx, cy + L_ * 0.64, cz) + dd * r * 0.8, r * 0.045,
                   r * 0.04, up=(0, 1, 0))
        P.add(vane, m["graphite"], bevel=r * 0.008, segments=1)
    lip = lathe_y([(L_ * 0.84, r * 0.93), (L_ * 0.99, r * 0.985), (L_ * 0.99, r * 0.9), (L_ * 0.84, r * 0.87),
                   (L_ * 0.84, r * 0.93)], segments=segs, center=(cx, cy, cz))
    P.add(lip, m[glow], smooth=60.0)
    collar = lathe_y([(-L_ * 0.18, r * 1.1), (L_ * 0.28, r * 1.14), (L_ * 0.34, r * 1.07), (L_ * 0.34, r * 0.8),
                      (-L_ * 0.18, r * 0.8), (-L_ * 0.18, r * 1.1)], segments=segs, center=(cx, cy, cz))
    P.add(collar, m["white"], bevel=r * 0.03, segments=2, angle=25.0)
    band = lathe_y([(L_ * 0.33, r * 1.1), (L_ * 0.42, r * 1.1), (L_ * 0.42, r * 0.96), (L_ * 0.33, r * 0.96),
                    (L_ * 0.33, r * 1.1)], segments=segs, center=(cx, cy, cz))
    P.add(band, m["accent"], smooth=30.0)
    for k in range(ribs):
        a = math.tau * (k + 0.5) / ribs
        d = V(math.cos(a), 0.0, math.sin(a))
        p0 = V(cx, cy + L_ * 0.44, cz) + d * (r * 1.0)
        p1 = V(cx, cy + L_ * 0.94, cz) + d * (r * 1.045)
        rb = bar(p0, p1, r * 0.09, r * 0.07, up=tuple(d))
        P.add(rb, m["graphite"], bevel=r * 0.015, segments=1)


CANOPY = (1.30, 0.50, 0.30)      # the squad canopy: length, width, height (m) at scale 1


def squad_canopy(P, m, y0, z0, k=1.0):
    """The ONE squad canopy, scaled per ship (k). Returns its rear y."""
    Ln, w, h = (v * k for v in CANOPY)
    canopy(P, m, y0, y0 + Ln, w, h, z0)
    return y0 + Ln


def canopy(P, m, y0, y1, w, h, z0, arches=(0.44, 0.78)):
    """The squad canopy: a dark glossy bubble sunk into the hull, graphite sill rails and arches."""
    Ln = y1 - y0
    st = [(0.0, 0.0, 0.0), (0.1, 0.42, 0.42), (0.3, 0.82, 0.82), (0.55, 1.0, 1.0), (0.82, 0.86, 0.8), (1.0, 0.5, 0.4)]
    rings = []
    for f, fw, fh in st:
        y = y0 + Ln * f
        if fw <= 0.0:
            rings.append(V(0.0, y, z0 + h * 0.15))
        else:
            rings.append([V(px, y, z0 + pz) for px, pz in sec(w * fw, h * fh, 0.09, n=2.3, verts=SEG)])
    glass = poly_loft(rings)
    P.add(glass, m["glass"], smooth=50.0)

    def half_w(f):
        for (fa, wa, _ha), (fb, wb, _hb) in zip(st, st[1:]):
            if fa <= f <= fb:
                return w * 0.5 * (wa + (wb - wa) * (f - fa) / (fb - fa))
        return 0.0

    def height(f):
        for (fa, _wa, ha), (fb, _wb, hb) in zip(st, st[1:]):
            if fa <= f <= fb:
                return h * (ha + (hb - ha) * (f - fa) / (fb - fa))
        return 0.0
    # sills
    for sx in (1, -1):
        pts = [V(sx * (half_w(f) * 0.985), y0 + Ln * f, z0 + 0.012) for f in (0.08, 0.2, 0.35, 0.55, 0.75, 0.9, 1.0)]
        P.add(tube(pts, 0.024, sides=6), m["graphite"], smooth=50.0)
    # arches (frame bars over the top)
    for f in arches:
        y = y0 + Ln * f
        hw, hh = half_w(f), height(f)
        pts = [V(px * 1.02, y, z0 + pz * 1.02 + 0.006) for px, pz in sec(hw * 2.0, hh, 0.0, n=2.3, verts=SEG)
               if pz >= -1e-6]
        pts.sort(key=lambda p: p.x)
        P.add(tube(pts, 0.022, sides=6), m["graphite"], smooth=50.0)
    # centre rib over the rear pane
    pts = [V(0, y0 + Ln * f, z0 + height(f) + 0.008) for f in (arches[0], 0.6, 0.7, arches[1], 0.9)]
    P.add(tube(pts, 0.018, sides=6), m["graphite"], smooth=50.0)


def spine(P, m, y0, y1, w0, w1, z0, z1, h=0.06):
    """Dorsal livery spine behind the canopy (squad colour), shared by the three ships."""
    rings = [[V(px, y0, z0 + pz) for px, pz in sec(w0, h, 0.04, n=3.0, verts=16)],
             [V(px, (y0 + y1) * 0.5, (z0 + z1) * 0.5 + pz) for px, pz in sec((w0 + w1) * 0.5, h * 0.9, 0.04, n=3.0,
                                                                          verts=16)],
             [V(px, y1, z1 + pz) for px, pz in sec(w1, h * 0.6, 0.04, n=3.0, verts=16)]]
    o = poly_loft(rings)
    P.add(o, m["accent"], bevel=0.006, segments=1, angle=35.0)
    return o


def nav_light(P, m, loc, r=0.035, mat="light"):
    o = L.lathe(coll(), [(0.0, r), (r * 0.6, r * 0.8), (r * 1.0, 0.0)], segments=10, axis="Z")
    o.data.transform(T(*loc) @ T(0, 0, -r * 0.3))
    P.add(o, m[mat], smooth=60.0)


def surface_strip(P, m, surf, pts_xy, z_from, r, mat="glow", lift=0.004, down=(0, 0, -1)):
    """A glowing strip that lies ON a surface: ray-cast each (x, y) from above."""
    pts = []
    for x, y in pts_xy:
        loc, nrm = surf.hit((x, y, z_from), down)
        if loc is not None:
            pts.append(loc + nrm * (lift + r * 0.4))
    if len(pts) >= 2:
        glow_line(P, m, pts, r, mat=mat, sides=6)
    return pts


def vents(P, m, center, normal, along, n, pitch, length, width, depth=0.03):
    """A row of dark vent slats in a graphite frame on a surface."""
    nrm = Vector(normal).normalized()
    a = Vector(along).normalized()
    M = frame_on(Vector(center), nrm, up=tuple(a))
    total = pitch * n
    fr = box((width + 0.06, total + 0.06, depth), (0, 0, -depth * 0.4))
    P.add(fr, m["graphite"], bevel=0.008, segments=1, matrix=M)
    for i in range(n):
        y = -total * 0.5 + pitch * (i + 0.5)
        s = box((width, pitch * 0.45, depth * 0.6), (0, y, depth * 0.15))
        P.add(s, m["dark"], bevel=0.004, segments=1, matrix=M)


# =========================================================================== 1. RIFT

def build_rift(asset, tag, stage):
    """Rift, the trickster (Unreal collision sphere 220 cm). ~5.0 m long, 5.0 m span. Forked nose with a glowing
    magenta portal ring held between twin prongs, crescent wings sweeping forward and up at the tips, narrow
    canopy, twin engines under canted V tails."""
    P = L.Part(asset, COLL)
    m = mats("SQ_Rift", "Rift")
    body = [
        {"y": -1.80, "w": 0.0, "z": -0.02},
        {"y": -1.72, "w": 0.42, "ht": 0.14, "hb": 0.12},
        {"y": -1.50, "w": 0.86, "ht": 0.27, "hb": 0.20, "n": 2.6},
        {"y": -1.05, "w": 1.00, "ht": 0.34, "hb": 0.24, "groove": 0.012},
        {"y": -0.30, "w": 1.08, "ht": 0.38, "hb": 0.26},
        {"y": 0.45, "w": 1.14, "ht": 0.38, "hb": 0.27, "groove": 0.012},
        {"y": 1.25, "w": 1.24, "ht": 0.34, "hb": 0.26, "n": 2.9},
        {"y": 1.85, "w": 1.30, "ht": 0.30, "hb": 0.25, "n": 3.0, "groove": 0.012},
        {"y": 2.12, "w": 1.20, "ht": 0.26, "hb": 0.22, "n": 3.0},
        {"y": 2.18, "w": 1.06, "ht": 0.2, "hb": 0.17, "n": 3.0},
    ]
    fus = hull(body)
    P.add(fus, m["white"], smooth=36.0)
    P.hull_from([fus])
    # twin prongs (the forked nose): slim lofts curving slightly inward to the tips
    def prong():
        st = [{"y": -2.46, "w": 0.0, "x": 0.31, "z": 0.0},
              {"y": -2.38, "w": 0.12, "ht": 0.07, "hb": 0.06, "x": 0.315, "z": 0.0},
              {"y": -2.10, "w": 0.22, "ht": 0.12, "hb": 0.10, "x": 0.33, "z": 0.0, "n": 2.8},
              {"y": -1.70, "w": 0.27, "ht": 0.16, "hb": 0.13, "x": 0.35, "z": 0.0, "n": 2.8, "groove": 0.008},
              {"y": -1.20, "w": 0.30, "ht": 0.18, "hb": 0.14, "x": 0.36, "z": 0.0, "n": 2.8},
              {"y": -0.80, "w": 0.22, "ht": 0.12, "hb": 0.10, "x": 0.34, "z": 0.0}]
        return hull(st, verts=16)
    pa, pb = both(prong, P, m["white"], smooth=36.0)
    # portal ring between the prongs: graphite housing, glowing magenta inner ring
    ring_c = (0.0, -2.02, 0.20)
    housing = torus(0.42, 0.085, axis="Y", steps=48, sides=12, center=ring_c, squash=1.5)
    P.add(housing, m["graphite"], smooth=40.0)
    inner = torus(0.335, 0.036, axis="Y", steps=48, sides=8, center=ring_c, squash=1.4)
    P.add(inner, m["glow"], smooth=60.0)
    # crescent wings (white root, magenta outer third), swept forward and up at the tips
    wst = [(0.45, 0.05, 1.80, 0.22, -0.02), (1.10, -0.28, 1.22, 0.18, 0.02), (1.68, -0.76, 0.80, 0.14, 0.10),
           (2.14, -1.34, 0.52, 0.10, 0.24), (2.44, -1.98, 0.30, 0.06, 0.42)]
    # wing-root fairings blending the crescent into the fuselage
    both(lambda: hull([{"y": -0.10, "w": 0.0, "x": 0.50, "z": 0.0},
                       {"y": 0.25, "w": 0.24, "ht": 0.13, "hb": 0.10, "x": 0.56, "z": 0.0},
                       {"y": 1.10, "w": 0.34, "ht": 0.16, "hb": 0.11, "x": 0.62, "z": 0.0, "groove": 0.008},
                       {"y": 1.75, "w": 0.24, "ht": 0.11, "hb": 0.09, "x": 0.58, "z": 0.0},
                       {"y": 1.95, "w": 0.0, "x": 0.54, "z": 0.0}], verts=20), P, m["white"], smooth=36.0)
    both(lambda: wing_loft(wst[:2]), P, m["white"], bevel=0.01, segments=2, angle=30.0)
    both(lambda: wing_loft([nudge(wst[1])] + wst[2:]), P, m["accent"], bevel=0.008, segments=2, angle=30.0)
    # engine nacelles + twin nozzles
    for sx in (1, -1):
        nac = hull([{"y": 0.9, "w": 0.0, "x": sx * 0.44, "z": -0.02},
                    {"y": 1.05, "w": 0.30, "ht": 0.18, "hb": 0.17, "x": sx * 0.44, "z": -0.02},
                    {"y": 1.5, "w": 0.50, "ht": 0.27, "hb": 0.25, "x": sx * 0.44, "z": -0.02, "groove": 0.01},
                    {"y": 2.15, "w": 0.52, "ht": 0.27, "hb": 0.26, "x": sx * 0.44, "z": -0.02},
                    {"y": 2.24, "w": 0.48, "ht": 0.25, "hb": 0.24, "x": sx * 0.44, "z": -0.02}], verts=20)
        P.add(nac, m["accent"], smooth=36.0)
        nozzle(P, m, (sx * 0.44, 2.14, -0.02), 0.2, 0.36)
    # canted V tails over the nacelles
    fst = [(0.0, 0.95, 1.20, 0.09, 0.0), (0.48, 1.50, 0.70, 0.065, 0.0), (0.80, 1.98, 0.32, 0.04, 0.0)]

    def fin():
        o = wing_loft(fst)
        o.data.transform(T(0.40, 0, 0.14) @ rot(-62, "Y"))
        return o
    both(fin, P, m["accent"], bevel=0.008, segments=2, angle=30.0)
    # canopy (narrow) + dorsal livery spine
    squad_canopy(P, m, -1.35, 0.27, 1.0)
    spine(P, m, -0.12, 1.75, 0.30, 0.18, 0.36, 0.30)
    if stage == "blockout":
        return P, [], {}
    surf = Surf(P.parts)
    # fin tips and leading-edge accents
    for sx in (1, -1):
        tip = T(0.40, 0, 0.14) @ rot(-62, "Y") @ V(0.80, 1.98 + 0.05, 0.0)
        nav_light(P, m, (sx * tip.x, tip.y, tip.z + 0.02), 0.03, "glow")
        # wingtip light (magenta, the squad colour)
        nav_light(P, m, (sx * 2.43, -1.86, 0.45), 0.035, "glow")
        # glow strip along the crescent, mid chord on the wing top
        surface_strip(P, m, surf, [(sx * (x + 0.02), yle + c * 0.28) for x, yle, c, t, z in wst[1:4]] +
                      [(sx * 2.36, -1.62)], 1.5, 0.014)
        # magenta trailing-edge strip on the outer 60% of the crescent (keeps the crescent readable far away)
        surface_strip(P, m, surf, [(sx * x, yle + c * 0.84) for x, yle, c, t, z in wst[1:5]], 1.5, 0.02)
        # graphite trailing-edge flap panels
        for (xa, ya), (xb, yb) in [((0.75, 1.55), (1.15, 0.98)), ((1.25, 0.82), (1.62, 0.12))]:
            la, na = surf.top(sx * xa, ya - 0.08)
            lb, nb = surf.top(sx * xb, yb - 0.08)
            if la is not None and lb is not None:
                fl = bar(la + na * 0.012, lb + nb * 0.012, 0.18, 0.025)
                P.add(fl, m["graphite"], bevel=0.006, segments=1)
        # prong guns: slim barrels under the prong tips (Muzzle_0/1)
        g = lathe_y([(-2.50, 0.0), (-2.50, 0.035), (-2.2, 0.04), (-2.1, 0.055), (-1.55, 0.055), (-1.55, 0.0)],
                    segments=12, center=(sx * 0.33, 0, -0.10))
        P.add(g, m["metal"], smooth=40.0)
        # cooling vents on the nacelle tops
        loc, nrm = surf.top(sx * 0.44, 1.62)
        if loc is not None:
            vents(P, m, loc, nrm, (0, 1, 0), 5, 0.09, 0.5, 0.16)
    # magenta accent plate across the nose top (squad chevron)
    plate_on(P, m["accent"], fus, ((-0.6, -1.65, 0.1), (0.6, -1.36, 1.0)), normal=(0, 0, 1), min_dot=0.4)
    # graphite armour plates on the fuselage flanks behind the wings
    for sx in (1, -1):
        lo = (0.2, 1.2, -0.3) if sx > 0 else (-1.0, 1.2, -0.3)
        hi = (1.0, 1.9, 0.15) if sx > 0 else (-0.2, 1.9, 0.15)
        plate_on(P, m["graphite"], fus, (lo, hi), normal=(sx, 0, 0), min_dot=0.62)
    plate_on(P, m["graphite"], fus, ((-1, -1.7, -1), (1, 2.1, 0.0)), normal=(0, 0, -1), min_dot=0.62)
    for sx in (1, -1):
        plate_on(P, m["accent"], fus, ((min(sx * 0.2, sx * 0.62), 0.25, 0.1), (max(sx * 0.2, sx * 0.62), 1.75, 1.0)),
                 normal=(0, 0, 1), min_dot=0.6)
    # greebles behind the canopy and at the engine roots
    surf = Surf(P.parts)
    greeble_cluster(P, m["metal"], surf, (0.0, 1.95, 0.32), (0, 0, 1), 5, (0.04, 0.1), (0.02, 0.04), 0.14, seed=3,
                    along=(0, 1, 0))
    for sx in (1, -1):
        greeble_cluster(P, m["graphite"], surf, (sx * 0.62, 1.55, 0.22), (sx * 0.5, 0, 1), 4, (0.04, 0.08),
                        (0.015, 0.035), 0.12, seed=9 + sx, along=(0, 1, 0))
    sockets = [("Muzzle_0", (0.33, -2.52, -0.10)), ("Muzzle_1", (-0.33, -2.52, -0.10)),
               ("Engine", (0.0, 2.52, -0.02)), ("Engine_0", (0.44, 2.52, -0.02)), ("Engine_1", (-0.44, 2.52, -0.02)),
               ("Portal", ring_c), ("Cockpit", (0.0, -0.75, 0.52)),
               ("WingTip_L", (2.43, -1.86, 0.45)), ("WingTip_R", (-2.43, -1.86, 0.45))]
    return P, sockets, {"ship": "Rift", "ue_radius_cm": 220}


# =========================================================================== 2. LANCER

def build_lancer(asset, tag, stage):
    """Lancer, the interceptor (collision sphere 220 cm): ~5.3 m long needle, 3.0 m span. Armoured ram prow with
    four ridges and a metal ram spike, four rapid-cannon barrels in two cheek pods, short swept-back drooping
    wings, one big main engine plus two small, a single tall dorsal fin."""
    P = L.Part(asset, COLL)
    m = mats("SQ_Lancer", "Lancer")
    body = [
        {"y": -2.36, "w": 0.0, "z": -0.01},
        {"y": -2.28, "w": 0.16, "ht": 0.08, "hb": 0.07},
        {"y": -1.85, "w": 0.40, "ht": 0.18, "hb": 0.15},
        {"y": -1.20, "w": 0.62, "ht": 0.27, "hb": 0.21, "groove": 0.01},
        {"y": -0.40, "w": 0.76, "ht": 0.34, "hb": 0.25},
        {"y": 0.40, "w": 0.84, "ht": 0.35, "hb": 0.27, "groove": 0.012},
        {"y": 1.20, "w": 0.94, "ht": 0.36, "hb": 0.30, "n": 2.8},
        {"y": 1.85, "w": 0.98, "ht": 0.37, "hb": 0.33, "n": 2.9, "groove": 0.012},
        {"y": 2.06, "w": 0.94, "ht": 0.35, "hb": 0.32, "n": 2.9},
        {"y": 2.12, "w": 0.82, "ht": 0.3, "hb": 0.28, "n": 2.9},
    ]
    fus = hull(body)
    P.add(fus, m["white"], smooth=36.0)
    P.hull_from([fus])
    # ram prow: a gunmetal armour sleeve over the nose, a metal ram spike and four armoured ridges (X pattern)
    sleeve = hull([{"y": -2.42, "w": 0.0, "z": -0.01},
                   {"y": -2.30, "w": 0.22, "ht": 0.11, "hb": 0.10, "n": 2.4},
                   {"y": -1.95, "w": 0.42, "ht": 0.2, "hb": 0.17, "n": 2.4},
                   {"y": -1.62, "w": 0.52, "ht": 0.25, "hb": 0.2, "n": 2.4, "groove": 0.01},
                   {"y": -1.40, "w": 0.58, "ht": 0.27, "hb": 0.21, "n": 2.4},
                   {"y": -1.32, "w": 0.52, "ht": 0.24, "hb": 0.18, "n": 2.4}], verts=20)
    P.add(sleeve, m["graphite"], smooth=36.0)
    spike = lathe_y([(-2.72, 0.0), (-2.56, 0.035), (-2.38, 0.07), (-2.25, 0.08), (-2.25, 0.0)], segments=16,
                    center=(0, 0, -0.01))
    P.add(spike, m["metal"], smooth=40.0)
    ridges = []
    for ang in (35, 145, 215, 325):
        a = math.radians(ang)
        d = V(math.cos(a), 0, math.sin(a))

        def rpt(y, r):
            return V(0, y, -0.01) + V(d.x * r * 1.2, 0, d.z * r)
        pts = [rpt(-2.52, 0.04), rpt(-2.2, 0.12), rpt(-1.8, 0.21), rpt(-1.4, 0.27), rpt(-1.15, 0.26)]
        rg = poly_loft([[p + V(0, 0, 0) + V(-d.z, 0, d.x) * s * hw + d * hh for s, hh in ((1, 0.0), (0, 0.15),
                                                                                         (-1, 0.0), (0, -0.04))]
                        for p, hw in zip(pts, (0.018, 0.04, 0.048, 0.045, 0.03))])
        P.add(rg, m["accent"] if ang in (35, 145) else m["graphite"], bevel=0.006, segments=1, angle=30.0)
        ridges.append(pts)
    # cheek cannon pods, two barrels each (four rapid cannons)
    muzzles = []
    for sx in (1, -1):
        pod = hull([{"y": -1.55, "w": 0.0, "x": sx * 0.36, "z": -0.11},
                    {"y": -1.45, "w": 0.16, "ht": 0.13, "hb": 0.12, "x": sx * 0.36, "z": -0.11, "n": 3.0},
                    {"y": -1.20, "w": 0.22, "ht": 0.17, "hb": 0.15, "x": sx * 0.36, "z": -0.11, "n": 3.2},
                    {"y": -0.30, "w": 0.24, "ht": 0.17, "hb": 0.15, "x": sx * 0.35, "z": -0.11, "n": 3.2, "groove": 0.008},
                    {"y": 0.15, "w": 0.0, "x": sx * 0.30, "z": -0.10}], verts=16)
        P.add(pod, m["graphite"], smooth=36.0)
        for k, dz in enumerate((0.055, -0.065)):
            bx = sx * 0.40
            g = lathe_y([(-2.02, 0.0), (-2.02, 0.026), (-1.92, 0.03), (-1.86, 0.022), (-1.45, 0.024),
                         (-1.40, 0.04), (-1.20, 0.04), (-1.20, 0.0)], segments=10, center=(bx, 0, -0.11 + dz))
            P.add(g, m["metal"], smooth=40.0)
            col = lathe_y([(-1.98, 0.034), (-1.90, 0.038), (-1.90, 0.024), (-1.98, 0.024), (-1.98, 0.034)],
                          segments=10, center=(bx, 0, -0.11 + dz))
            P.add(col, m["accent"], smooth=40.0)
            muzzles.append((bx, -2.04, -0.11 + dz))
    # short swept-back wings (white, cyan tips), drooping
    wst = [(0.38, 0.45, 1.55, 0.13, -0.06), (0.95, 0.98, 1.0, 0.10, -0.12), (1.50, 1.55, 0.42, 0.06, -0.24)]
    tipst = [nudge((0.95, 0.98, 1.0, 0.10, -0.12)), (1.50, 1.55, 0.42, 0.06, -0.24)]
    both(lambda: wing_loft(wst[:2]), P, m["accent"], bevel=0.01, segments=2, angle=30.0)
    both(lambda: wing_loft(tipst), P, m["white"], bevel=0.008, segments=2, angle=30.0)
    # small forward canards (silhouette: a needle with a second, smaller wing pair)
    cst = [(0.24, -0.95, 0.46, 0.05, 0.02), (0.62, -0.70, 0.22, 0.035, 0.0)]
    both(lambda: wing_loft(cst), P, m["graphite"], bevel=0.005, segments=1, angle=30.0)
    # main engine (big, centre) + two small engines in wing-root nacelles
    nozzle(P, m, (0.0, 2.04, 0.0), 0.33, 0.48)
    for sx in (1, -1):
        nac = hull([{"y": 0.85, "w": 0.0, "x": sx * 0.58, "z": -0.08},
                    {"y": 1.0, "w": 0.2, "ht": 0.12, "hb": 0.11, "x": sx * 0.58, "z": -0.08},
                    {"y": 1.4, "w": 0.3, "ht": 0.16, "hb": 0.15, "x": sx * 0.58, "z": -0.08, "groove": 0.008},
                    {"y": 2.0, "w": 0.3, "ht": 0.16, "hb": 0.15, "x": sx * 0.58, "z": -0.08}], verts=16)
        P.add(nac, m["accent"], smooth=36.0)
        nozzle(P, m, (sx * 0.58, 1.98, -0.08), 0.12, 0.24, segs=16, ribs=6)
    # single tall dorsal fin, swept
    fst = [(0.0, 0.42, 1.75, 0.10, 0.0), (0.55, 1.08, 1.05, 0.08, 0.0), (1.0, 1.75, 0.45, 0.05, 0.0)]
    # ventral keel blade under the ram prow (graphite)
    kst = [(0.0, -2.15, 1.25, 0.06, 0.0), (0.16, -1.85, 0.80, 0.04, 0.0)]
    keel = wing_loft(kst)
    keel.data.transform(T(0, 0, -0.10) @ rot(90, "Y"))
    P.add(keel, m["graphite"], bevel=0.006, segments=1, angle=30.0)

    def fin():
        o = wing_loft(fst)
        o.data.transform(T(0, 0, 0.26) @ rot(-90, "Y"))
        return o
    f = fin()
    P.add(f, m["accent"], bevel=0.008, segments=2, angle=30.0)
    # canopy + spine
    squad_canopy(P, m, -0.95, 0.25, 1.0)
    spine(P, m, 0.22, 0.75, 0.26, 0.16, 0.34, 0.33)
    if stage == "blockout":
        return P, [], {}
    surf = Surf(P.parts)
    # cyan glow lines along the two upper ram ridges and on the fin's leading edge
    for pts in ridges[:2]:
        glow_line(P, m, [p * 1.0 + (p - V(0, p.y, -0.01)).normalized() * 0.075 for p in pts[1:4]], 0.012)
    fin_le = [T(0, 0, 0.26) @ rot(-90, "Y") @ V(x, yle - 0.012, 0.0) for x, yle, c, t, z in fst]
    glow_line(P, m, fin_le[:2] + [fin_le[1].lerp(fin_le[2], 0.85)], 0.014)
    for sx in (1, -1):
        rail = lathe_y([(1.02, 0.0), (1.08, 0.03), (1.2, 0.05), (1.95, 0.05), (2.02, 0.035), (2.02, 0.0)],
                       segments=12, center=(sx * 1.53, 0, -0.27))
        P.add(rail, m["graphite"], smooth=40.0)
        for k in range(2):
            rk = lathe_y([(0.80 + k * 0.12, 0.0), (0.88 + k * 0.12, 0.022), (0.96 + k * 0.12, 0.028),
                          (1.5, 0.028), (1.5, 0.0)], segments=10, center=(sx * (1.53 + (k - 0.5) * 0.06), 0, -0.33))
            P.add(rk, m["accent"], smooth=40.0)
        nav_light(P, m, (sx * 1.53, 2.04, -0.27), 0.03, "glow")
        surface_strip(P, m, surf, [(sx * 0.55, 0.62), (sx * 0.95, 1.12), (sx * 1.3, 1.6)], 1.5, 0.012)
        loc, nrm = surf.hit((sx * 1.0, -0.2, 0.08), (-sx, 0, 0))
        if loc is not None:
            vents(P, m, loc, nrm, (0, 1, 0), 6, 0.08, 0.5, 0.12)
    plate_on(P, m["accent"], fus, ((-0.5, 0.40, 0.1), (0.5, 1.82, 1.0)), normal=(0, 0, 1), min_dot=0.5)
    for sx in (1, -1):
        lo = (0.1, -1.25, -0.5) if sx > 0 else (-1.0, -1.25, -0.5)
        hi = (1.0, 0.40, 0.4) if sx > 0 else (-0.1, 0.40, 0.4)
        plate_on(P, m["accent"], fus, (lo, hi), normal=(sx, 0, 0), min_dot=0.62)
    for sx in (1, -1):
        lo = (0.1, 0.5, -0.5) if sx > 0 else (-1.0, 0.5, -0.5)
        hi = (1.0, 1.8, 0.1) if sx > 0 else (-0.1, 1.8, 0.1)
        plate_on(P, m["graphite"], fus, (lo, hi), normal=(sx, 0, 0), min_dot=0.62)
    plate_on(P, m["graphite"], fus, ((-1, -1.3, -1), (1, 2.0, 0.0)), normal=(0, 0, -1), min_dot=0.62)
    surf = Surf(P.parts)
    greeble_cluster(P, m["metal"], surf, (0.0, 1.0, 0.36), (0, 0, 1), 4, (0.04, 0.08), (0.015, 0.03), 0.1, seed=4,
                    along=(0, 1, 0))
    for sx in (1, -1):
        greeble_cluster(P, m["graphite"], surf, (sx * 0.4, 1.7, 0.2), (sx * 0.6, 0, 0.8), 4, (0.04, 0.08),
                        (0.015, 0.03), 0.12, seed=11 + sx, along=(0, 1, 0))
    sockets = [("Muzzle_%d" % i, p) for i, p in enumerate(muzzles)]
    sockets += [("Engine", (0.0, 2.52, 0.0)), ("Engine_0", (0.0, 2.52, 0.0)), ("Engine_1", (0.58, 2.22, -0.08)),
                ("Engine_2", (-0.58, 2.22, -0.08)), ("Ram", (0.0, -2.72, -0.01)), ("Cockpit", (0.0, -0.35, 0.5)),
                ("WingTip_L", (1.53, 2.04, -0.27)), ("WingTip_R", (-1.53, 2.04, -0.27))]
    return P, sockets, {"ship": "Lancer", "ue_radius_cm": 220}


# =========================================================================== 3. BASTION

def build_bastion(asset, tag, stage):
    """Bastion, the heavy (collision sphere 220 cm): ~5.1 m long, 4.6 m span. Broad armoured hull between two
    big missile pods (visible hatches, amber-tipped tubes in front), a raised shield-dome emitter ring on the
    back, four engines in a row, twin chin cannons."""
    P = L.Part(asset, COLL)
    m = mats("SQ_Bastion", "Bastion")
    body = [
        {"y": -2.32, "w": 1.00, "ht": 0.14, "hb": 0.18, "z": -0.12, "n": 3.2, "flat": 1.5},
        {"y": -2.25, "w": 1.30, "ht": 0.22, "hb": 0.26, "z": -0.10, "n": 3.3, "flat": 1.5},
        {"y": -1.90, "w": 1.62, "ht": 0.36, "hb": 0.33, "z": -0.06, "n": 3.4, "flat": 1.5},
        {"y": -1.30, "w": 1.90, "ht": 0.50, "hb": 0.38, "z": -0.02, "n": 3.5, "flat": 1.5, "groove": 0.014},
        {"y": -0.30, "w": 2.00, "ht": 0.60, "hb": 0.40, "n": 3.6, "flat": 1.5},
        {"y": 0.70, "w": 2.00, "ht": 0.60, "hb": 0.40, "n": 3.6, "flat": 1.5, "groove": 0.014},
        {"y": 1.60, "w": 1.92, "ht": 0.54, "hb": 0.38, "n": 3.6, "flat": 1.5},
        {"y": 2.12, "w": 1.80, "ht": 0.47, "hb": 0.35, "n": 3.6, "flat": 1.5, "groove": 0.012},
        {"y": 2.30, "w": 1.66, "ht": 0.41, "hb": 0.31, "n": 3.6, "flat": 1.5},
        {"y": 2.34, "w": 1.48, "ht": 0.34, "hb": 0.26, "n": 3.6, "flat": 1.5},
    ]
    fus = hull(body)
    P.add(fus, m["white"], smooth=36.0)
    P.hull_from([fus])
    # missile pods (graphite), on stub pylons
    pod_x = 1.62
    pods = []
    for sx in (1, -1):
        st = [{"y": -1.62, "w": 0.58, "ht": 0.25, "hb": 0.25, "x": sx * pod_x, "z": -0.06, "n": 3.6},
              {"y": -1.56, "w": 0.66, "ht": 0.30, "hb": 0.29, "x": sx * pod_x, "z": -0.07, "n": 3.6},
              {"y": -1.25, "w": 0.76, "ht": 0.37, "hb": 0.33, "x": sx * pod_x, "z": -0.08, "n": 3.4, "groove": 0.012},
              {"y": -0.40, "w": 0.78, "ht": 0.38, "hb": 0.34, "x": sx * pod_x, "z": -0.08, "n": 3.4, "groove": 0.012},
              {"y": 1.00, "w": 0.78, "ht": 0.38, "hb": 0.34, "x": sx * pod_x, "z": -0.08, "n": 3.4, "groove": 0.012},
              {"y": 1.85, "w": 0.72, "ht": 0.35, "hb": 0.31, "x": sx * pod_x, "z": -0.08, "n": 3.4},
              {"y": 2.00, "w": 0.62, "ht": 0.30, "hb": 0.27, "x": sx * pod_x, "z": -0.08, "n": 3.4}]
        pod = hull(st)
        P.add(pod, m["accent"], smooth=36.0)
        P.hull_from([pod])
        pods.append(pod)
        # broad swept wing through the pod (white, amber tip beyond the pod)
        def mk(stations, sx=sx):
            o = wing_loft(stations)
            if sx < 0:
                o2 = mirrored(o)
                L.remove_object(o)
                return o2
            return o
        wst = [(0.80, -0.55, 2.40, 0.22, -0.24), (1.62, 0.02, 1.70, 0.16, -0.31), (2.06, 0.34, 1.22, 0.11, -0.36)]
        P.add(mk(wst), m["white"], bevel=0.012, segments=2, angle=30.0)
        P.add(mk([nudge(wst[-1]), (2.44, 0.72, 0.70, 0.07, -0.44)]), m["accent"], bevel=0.008, segments=2, angle=30.0)
        # pod rear nozzle + hull nozzles
        nozzle(P, m, (sx * pod_x, 1.90, -0.08), 0.24, 0.40)
        nozzle(P, m, (sx * 0.46, 2.22, -0.02), 0.28, 0.42)
        # outboard pod fin (small, canted)
        fst = [(0.0, 0.6, 1.1, 0.08, 0.0), (0.40, 1.1, 0.6, 0.05, 0.0)]

        def pfin(sx=sx):
            o = wing_loft(fst)
            o.data.transform(T(pod_x + 0.22, 0, 0.18) @ rot(-35, "Y"))
            if sx < 0:
                o2 = mirrored(o)
                L.remove_object(o)
                return o2
            return o
        P.add(pfin(), m["accent"], bevel=0.007, segments=2, angle=30.0)
    # raised dorsal deck (graphite, second tier) carrying the dome emitter
    deck = hull([{"y": -0.75, "w": 0.0, "z": 0.50},
                 {"y": -0.55, "w": 0.70, "ht": 0.10, "hb": 0.10, "z": 0.52, "n": 3.0},
                 {"y": 0.10, "w": 1.20, "ht": 0.17, "hb": 0.12, "z": 0.53, "n": 3.4, "groove": 0.01},
                 {"y": 1.40, "w": 1.24, "ht": 0.17, "hb": 0.12, "z": 0.50, "n": 3.4, "groove": 0.01},
                 {"y": 2.10, "w": 1.02, "ht": 0.12, "hb": 0.12, "z": 0.42, "n": 3.4},
                 {"y": 2.30, "w": 0.80, "ht": 0.06, "hb": 0.10, "z": 0.36, "n": 3.4}])
    P.add(deck, m["accent"], smooth=36.0)
    # dorsal shield-dome emitter: pedestal, raised ring on three struts, amber dish
    dc = V(0.0, 0.75, 0.62)
    ped = lathe_z([(-0.12, 0.46), (0.04, 0.46), (0.07, 0.40), (0.09, 0.30), (0.09, 0.0)], segments=40,
                  center=tuple(dc))
    P.add(ped, m["graphite"], bevel=0.01, segments=2, angle=30.0)
    rz = dc.z + 0.24
    ring = torus(0.64, 0.085, axis="Z", steps=56, sides=12, center=(dc.x, dc.y, rz), squash=1.45)
    P.add(ring, m["white"], smooth=40.0)
    band = torus(0.555, 0.034, axis="Z", steps=56, sides=8, center=(dc.x, dc.y, rz), squash=2.2)
    P.add(band, m["glow"], smooth=60.0)
    lens = lathe_z([(0.08, 0.20), (0.13, 0.17), (0.17, 0.10), (0.19, 0.0)], segments=24, center=tuple(dc))
    P.add(lens, m["metal"], smooth=50.0)
    tip = lathe_z([(0.18, 0.07), (0.21, 0.045), (0.225, 0.0)], segments=16, center=tuple(dc))
    P.add(tip, m["glow"], smooth=50.0)
    for k in range(4):
        a = math.tau * k / 4 + math.pi / 4
        p0 = dc + V(math.cos(a) * 0.36, math.sin(a) * 0.36, 0.02)
        p1 = dc + V(math.cos(a) * 0.64, math.sin(a) * 0.64, 0.24)
        P.add(bar(p0, p1, 0.08, 0.06, up=(math.cos(a), math.sin(a), 0)), m["graphite"], bevel=0.008, segments=1)
    # canopy (same squad canopy) + spine
    squad_canopy(P, m, -1.95, 0.36, 1.15)
    spine(P, m, -0.55, 0.12, 0.36, 0.30, 0.52, 0.56)
    # chin cannons (heavy): housings + barrels
    muzzles = []
    for sx in (1, -1):
        hs_ = hull([{"y": -2.12, "w": 0.22, "ht": 0.11, "hb": 0.10, "x": sx * 0.36, "z": -0.32, "n": 3.2},
                    {"y": -1.70, "w": 0.30, "ht": 0.15, "hb": 0.13, "x": sx * 0.36, "z": -0.32, "n": 3.2},
                    {"y": -1.0, "w": 0.30, "ht": 0.15, "hb": 0.13, "x": sx * 0.36, "z": -0.30, "n": 3.2},
                    {"y": -0.70, "w": 0.0, "x": sx * 0.36, "z": -0.26}], verts=16)
        P.add(hs_, m["graphite"], smooth=36.0)
        g = lathe_y([(-2.60, 0.0), (-2.60, 0.06), (-2.48, 0.075), (-2.40, 0.06), (-2.12, 0.062), (-2.08, 0.085),
                     (-1.95, 0.085), (-1.95, 0.0)], segments=14, center=(sx * 0.36, 0, -0.33))
        P.add(g, m["metal"], smooth=40.0)
        muzzles.append((sx * 0.36, -2.62, -0.33))
    if stage == "blockout":
        return P, [], {}
    surf = Surf(P.parts)
    # pod details: missile tube openings with amber warhead tips (front), hatches on top
    for sx in (1, -1):
        cx = sx * pod_x
        for (ox, oz) in ((-0.17, 0.11), (0.17, 0.11), (-0.17, -0.13), (0.17, -0.13)):
            c = V(cx + ox, -1.64, -0.06 + oz)
            tubo = lathe_y([(-0.02, 0.095), (0.06, 0.095), (0.06, 0.072), (0.0, 0.072), (0.0, 0.0)], segments=14,
                           center=(c.x, c.y, c.z))
            P.add(tubo, m["metal"], smooth=40.0)
            tip = lathe_y([(-0.06, 0.0), (-0.03, 0.04), (0.02, 0.06), (0.02, 0.0)], segments=12, center=(c.x, c.y, c.z))
            P.add(tip, m["glow"], smooth=50.0)
        # three hatch doors along the pod top (lit amber seam), with hinge bars
        for k, y in enumerate((-0.80, -0.05, 0.70)):
            loc, nrm = surf.top(cx, y)
            if loc is None:
                continue
            M = frame_on(loc, nrm, up=(0, 1, 0))
            door = box((0.46, 0.66, 0.035), (0, 0, 0.008))
            P.add(door, m["white"], bevel=0.008, segments=1, matrix=M)
            seam = box((0.34, 0.025, 0.02), (0, 0.26, 0.03))
            P.add(seam, m["glow"], bevel=0.004, segments=1, matrix=M)
            for hx in (-0.2, 0.2):
                hb = box((0.07, 0.10, 0.03), (hx, -0.33, 0.012))
                P.add(hb, m["accent"], bevel=0.006, segments=1, matrix=M)
        # amber stripe on the pod's outer flank
        loc_a, n_a = surf.hit((cx + sx * 1.0, -1.15, 0.10), (-sx, 0, 0))
        loc_b, n_b = surf.hit((cx + sx * 1.0, -0.05, 0.10), (-sx, 0, 0))
        if loc_a is not None and loc_b is not None:
            glow_line(P, m, [loc_a + n_a * 0.01, loc_b + n_b * 0.01], 0.014)
        nav_light(P, m, (cx + sx * 0.3, 1.45, 0.26), 0.035, "glow")
        # side vents on the hull
        loc, nrm = surf.hit((sx * 1.25, -1.55, 0.12), (-sx, 0, 0))
        if loc is not None:
            vents(P, m, loc, nrm, (0, 1, 0), 5, 0.1, 0.55, 0.14)
    # amber accent plates: nose chevron and the hull's rear deck around the dome
    plate_on(P, m["accent"], fus, ((-1.2, -2.2, 0.0), (1.2, -1.98, 1.0)), normal=(0, 0, 1), min_dot=0.35)
    plate_on(P, m["white"], pods[0], ((pod_x - 0.5, -1.6, 0.0), (pod_x + 0.5, -1.15, 1.0)), normal=(0, 0, 1),
             min_dot=0.3)
    plate_on(P, m["white"], pods[1], ((-pod_x - 0.5, -1.6, 0.0), (-pod_x + 0.5, -1.15, 1.0)), normal=(0, 0, 1),
             min_dot=0.3)
    for sx in (1, -1):
        lo = (0.25, 1.2, 0.1) if sx > 0 else (-0.95, 1.2, 0.1)
        hi = (0.95, 2.05, 1.0) if sx > 0 else (-0.25, 2.05, 1.0)
        plate_on(P, m["graphite"], fus, (lo, hi), normal=(0, 0, 1), min_dot=0.6)
    plate_on(P, m["graphite"], fus, ((-1.2, -2.0, -1), (1.2, 2.2, 0.0)), normal=(0, 0, -1), min_dot=0.6)
    surf = Surf(P.parts)
    greeble_cluster(P, m["metal"], surf, (0.0, 1.75, 0.5), (0, 0, 1), 6, (0.05, 0.12), (0.02, 0.05), 0.18, seed=5,
                    along=(0, 1, 0))
    for sx in (1, -1):
        greeble_cluster(P, m["graphite"], surf, (sx * pod_x, 1.75, 0.3), (0, 0, 1), 4, (0.05, 0.1), (0.02, 0.04),
                        0.14, seed=21 + sx, along=(0, 1, 0))
    sockets = [("Muzzle_0", muzzles[0]), ("Muzzle_1", muzzles[1]),
               ("Engine", (0.0, 2.62, -0.04)),
               ("Engine_0", (0.46, 2.62, -0.02)), ("Engine_1", (-0.46, 2.62, -0.02)),
               ("Engine_2", (pod_x, 2.30, -0.08)), ("Engine_3", (-pod_x, 2.30, -0.08)),
               ("Dome", (dc.x, dc.y, dc.z + 0.24)), ("Cockpit", (0.0, -1.4, 0.62)),
               ("MissilePod_L", (pod_x, -1.70, -0.06)), ("MissilePod_R", (-pod_x, -1.70, -0.06))]
    return P, sockets, {"ship": "Bastion", "ue_radius_cm": 220}


BUILDERS = {
    # asset: (builder, texture size, tri budget (min, max), AO distance m)
    "SM_Squad_Rift": (build_rift, 2048, (15000, 40000), 0.6),
    "SM_Squad_Lancer": (build_lancer, 2048, (15000, 40000), 0.6),
    "SM_Squad_Bastion": (build_bastion, 2048, (15000, 40000), 0.7),
}

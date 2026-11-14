# SPDX-License-Identifier: MIT
"""
nf_ships.py - the Null Armada's five enemy ships (route C: code-built from a written design), plus the
carrier's launch bay and point-defence turret. Blender 5.2, headless. Units metres, front -Y, up +Z
(the exporter turns -Y into Unreal +X). Unreal (x, y, z) = Blender (-y, -x, z) in this file's comments.

Faction language (from the NullTrooper sheets): black gunmetal armour plates, red-orange emissive
seams and pinstripes, glowing white "eyes", angular, insectile, predatory, heavier at the front.
No squad colours (magenta / cyan / amber) anywhere.

    blender --background --factory-startup --python nf_build.py -- --asset SM_Null_Fighter --tag r1 [--stage blockout]
"""

import math
import random

import bmesh
import bpy
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

import nf_lib as L

COLL = "NullFleet"
GLOW_K = 1.0          # pinstripe thickness multiplier (1.6 in r1-r5; thinned in e1/e2 to pay for the engine plumes)

PAL = {
    "armour": "#16171B", "armour2": "#0E0F12",      # matte black gunmetal (dominant)
    "plate": "#2A2C32", "plate2": "#202227",        # lighter gunmetal plates (secondary)
    "crimson": "#4A0A12", "crimson2": "#36070D",    # dark crimson accent (palette: Null Armada secondary)
    "metal": "#3C3F46",                              # bare metal: nozzles, barrels, mechanisms
    "wear": "#6E737B",
    "glow": "#FF3C14",                               # red-orange seams
    "engine": "#FF7A30",                             # hot red-orange engine core (e1: hotter, full level)
    "eye": "#F4F7FF",                                # white eyes
}


# r1 review: chamfers too small to catch light or carry edge wear -> every bevel 1.3x wider, one more segment
BEVEL_K = 1.3


class NFPart(L.Part):
    def add(self, obj, mat, bevel=None, segments=2, **kw):
        if bevel:
            bevel *= BEVEL_K
            if bevel >= 0.02:      # the extra segment only where it shows (big parts); keeps fighters on budget
                segments += 1
        o = super().add(obj, mat, bevel=bevel, segments=segments, **kw)
        if bevel:   # bevel collapse at spike tips / tight insets leaves coincident vertices: weld them per part
            _weld_if_needed(o.data)
        return o


def _weld_if_needed(me, dist=1e-4):
    bm = bmesh.new()
    bm.from_mesh(me)
    tm = bmesh.ops.find_doubles(bm, verts=list(bm.verts), dist=dist)["targetmap"]
    if tm:
        bmesh.ops.weld_verts(bm, targetmap=tm)
        bm.to_mesh(me)
        me.update()
    bm.free()


# =========================================================================== small math helpers

def rot(deg, axis):
    return Matrix.Rotation(math.radians(deg), 4, axis)


def T(x, y, z):
    return Matrix.Translation((x, y, z))


def V(*a):
    return Vector(a)


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
    return L.obj_from_bm(bm, L.ensure_collection(COLL), name)


def armour_sec(w, ht, hb, chine=0.0, ridge=0.28, shoulder=0.86, keel=0.55, flat_top=0.0):
    """Angular 10-point hull section (x, z): sharp top ridge, upper shoulders, a sharp chine at the widest
    point (z = chine * ht), lower flanks, keel. Insectile, faceted - the Null Armada's hull language."""
    hw = w * 0.5
    ch = chine * (ht if chine >= 0 else hb)
    half = [(0.0, ht), (hw * ridge, ht * (0.93 - flat_top * -0.07)), (hw * shoulder, ch + (ht - ch) * 0.42),
            (hw, ch), (hw * keel, -hb * 0.62)]
    pts = half + [(0.0, -hb)] + [(-x, z) for x, z in reversed(half[1:])]
    return pts


def ring_y(y, pts, x0=0.0, z0=0.0):
    return [V(x0 + px, y, z0 + pz) for px, pz in pts]


def hull_loft(stations, name=None):
    """stations: (y, w, ht, hb, z0, kwargs) ; w == 0 -> tip point at (0, y, z0)."""
    rings = []
    for st in stations:
        y, w, ht, hb, z0 = st[:5]
        kw = st[5] if len(st) > 5 else {}
        if w <= 1e-6:
            rings.append(V(kw.get("x", 0.0), y, z0))
        else:
            rings.append(ring_y(y, armour_sec(w, ht, hb, **kw), kw.get("x", 0.0) if False else 0.0, z0))
    return poly_loft(rings, name=name)


def blade_sec(chord, t, le_share=0.24):
    """Angular wing section in (y, z) from the leading edge: sharp LE and TE, faceted top and bottom."""
    return [(0.0, 0.0), (chord * le_share, t * 0.5), (chord * 0.68, t * 0.36), (chord, 0.0),
            (chord * 0.68, -t * 0.32), (chord * le_share, -t * 0.42)]


def blade_wing(stations):
    """stations: (x, y_le, chord, thickness, z). Rings along x, capped at both ends."""
    rings = []
    for x, yle, c, t, z in stations:
        rings.append([V(x, yle + py, z + pz) for py, pz in blade_sec(c, t)])
    return poly_loft(rings)


def mirrored(obj):
    """New object: obj's mesh mirrored across X (normals fixed). obj itself is kept."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.transform(bm, matrix=Matrix.Diagonal((-1.0, 1.0, 1.0, 1.0)), verts=list(bm.verts))
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.normal_update()
    me = bpy.data.meshes.new(obj.name + "_mx")
    bm.to_mesh(me)
    bm.free()
    o = bpy.data.objects.new(L.uname("mx"), me)
    L.ensure_collection(COLL).objects.link(o)
    return o


def both(make, P, mat, **add_kw):
    """make() -> object built on +X; adds it and its mirror."""
    o = make()
    m = mirrored(o)
    P.add(o, mat, **add_kw)
    P.add(m, mat, **add_kw)
    return o, m


def one(make, P, mat, **add_kw):
    """make() -> a symmetric object built across x = 0; added once (same return shape as both())."""
    o = make()
    P.add(o, mat, **add_kw)
    return o, o


def tube(points, r, sides=4, cap=True):
    return L.tube_path(L.ensure_collection(COLL), points, r, sides=sides, cap=cap)


def lathe_y(profile, segments=16, center=(0.0, 0.0, 0.0)):
    """profile [(y, r)], revolved about an axis parallel to Y through center."""
    o = L.lathe(L.ensure_collection(COLL), [(p, r) for p, r in profile], segments=segments, axis="Y")
    o.data.transform(T(*center))
    return o


def box(size, center=(0.0, 0.0, 0.0)):
    return L.box(L.ensure_collection(COLL), size, center)


def bar(p0, p1, w, h=None, up=(0.0, 0.0, 1.0)):
    o, m = L.bar(L.ensure_collection(COLL), p0, p1, w, h, up=up)
    o.data.transform(m)
    return o


def limb_sec(w, h, kind="hex"):
    if kind == "blade":
        return [(0.0, h * 0.5), (w * 0.5, 0.0), (0.0, -h * 0.5), (-w * 0.5, 0.0)]
    return [(0.0, h * 0.5), (w * 0.5, h * 0.22), (w * 0.5, -h * 0.22), (0.0, -h * 0.5), (-w * 0.5, -h * 0.22),
            (-w * 0.5, h * 0.22)]


def limb(points, widths, heights, up=(0.0, 0.0, 1.0), kind="hex", tip=False):
    """Angular limb / strut / arm swept along a polyline. widths/heights per point (section across the path;
    'height' is along `up` projected). tip=True closes the last point to a spike."""
    pts = [Vector(p) for p in points]
    n = len(pts)
    rings = []
    for i, p in enumerate(pts):
        a = pts[max(i - 1, 0)]
        b = pts[min(i + 1, n - 1)]
        t = (b - a).normalized()
        U = Vector(up)
        if abs(t.dot(U.normalized())) > 0.97:
            U = Vector((0.0, -1.0, 0.0))
        x = t.cross(U).normalized()
        z = x.cross(t).normalized()
        if tip and i == n - 1:
            rings.append(p.copy())
            continue
        rings.append([p + x * sx + z * sz for sx, sz in limb_sec(widths[i], heights[i], kind)])
    return poly_loft(rings)


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


def frame_on(loc, nrm, up=(0.0, 0.0, 1.0)):
    """Matrix placing local +Z along nrm at loc."""
    n = Vector(nrm).normalized()
    u = Vector(up)
    if abs(n.dot(u)) > 0.95:
        u = Vector((0.0, -1.0, 0.0))
    x = u.cross(n).normalized()
    y = n.cross(x).normalized()
    m = Matrix((x, y, n)).transposed().to_4x4()
    m.translation = loc
    return m


# =========================================================================== materials

def mats(prefix, scale, panel_spacing):
    m = {}
    panel = None if panel_spacing is None else (panel_spacing * 1.7, 0.008 * max(1.0, scale * 2))
    # r1 review: no cloudy mottling (var / dents / roughness noise turned down), visible edge wear
    clean = dict(dent=0.2, rough_var=0.03, var_amount=0.35)
    # r2 review: silver seam wear and grey plates washed the black out at 300 m -> darker, quieter wear
    m["armour"] = L.role_paint(prefix + "_Armour", "#121316", "#101114", metallic=0.4, rough=0.5, wear=0.3,
                               wear_color="#4E535A", grime=0.04, panel=panel, scale=scale, seed=1, bump=0.25, **clean)
    m["plate"] = L.role_paint(prefix + "_Plate", "#1B1C20", "#17181B", metallic=0.5, rough=0.4, wear=0.35,
                              wear_color="#5A5F66", grime=0.03, scale=scale, seed=2, bump=0.25, **clean)
    # r1 review: dark-crimson paint read as rust / off-faction; the accent is now a lighter, glossier gunmetal
    m["crimson"] = L.role_paint(prefix + "_Accent", "#24262B", "#202227", metallic=0.6, rough=0.32, wear=0.35,
                                wear_color="#60656C", grime=0.03, scale=scale, seed=3, bump=0.2, **clean)
    m["metal"] = L.role_metal(prefix + "_Metal", PAL["metal"], rough=0.3, metallic=0.95, scale=scale, grime=0.2, seed=4)
    m["dark"] = L.role_paint(prefix + "_Dark", "#08080A", "#0B0B0E", metallic=0.2, rough=0.7, wear=0.0, grime=0.0,
                             scale=scale, seed=5, bump=0.2)
    # e1 (Lead: no engine glow at range): seams at half level in the E map, engines and eyes at full level, and
    # the MI's EmissiveStrength doubled - seams stay as bright as before, engines and eyes read ~2x hotter.
    m["glow"] = L.role_glow(prefix + "_Glow", PAL["glow"], level=0.5, base="#120604")
    m["engine"] = L.role_glow(prefix + "_Engine", PAL["engine"], level=1.0, base="#140804")
    m["eye"] = L.role_glow(prefix + "_Eye", PAL["eye"], level=1.0, base="#1A1B1E")
    return m


# =========================================================================== shared parts

def nozzle(P, m, center, r, length, glow_inset=0.5, petals=0):
    """Engine bell along +Y (aft) starting at center: dark metal bell, red-orange core set inside."""
    cx, cy, cz = center
    prof = [(0.0, r * 0.82), (length * 0.25, r * 0.92), (length * 0.85, r), (length, r * 1.04),
            (length, r * 0.88), (length * glow_inset, r * 0.8), (length * glow_inset, 0.0)]
    o = lathe_y(prof, segments=16, center=(cx, cy, cz))
    P.add(o, m["metal"], smooth=40.0)
    # (e2: the separate core disc inside the bell is gone; the plume below starts inside the bell and fills it)
    # exhaust plume stub: a glowing cone standing out of the bell, so the engine reads from the side and the
    # front quarters at range, not only from straight behind (e1)
    plume = lathe_y([(length * 0.8, r * 0.8), (length + r * 0.4, r * 0.66), (length + r * 0.8, r * 0.36),
                     (length + r * 1.2, 0.0)], segments=16, center=(cx, cy, cz))
    P.add(plume, m["engine"], smooth=60.0)
    # hot lip: a thin glowing ring just inside the bell's exit, so the engines read from the side too
    lip = lathe_y([(length * 0.9, r * 0.9), (length * 0.99, r * 0.97), (length * 0.99, r * 0.86),
                   (length * 0.9, r * 0.82), (length * 0.9, r * 0.9)], segments=16, center=(cx, cy, cz))
    P.add(lip, m["engine"], smooth=60.0)
    # outer collar ring (armour), overlapping the bell's front
    collar = lathe_y([(-length * 0.1, r * 1.12), (length * 0.32, r * 1.16), (length * 0.4, r * 1.02),
                      (length * 0.4, r * 0.7), (-length * 0.1, r * 0.7)], segments=8, center=(cx, cy, cz))
    P.add(collar, m["plate"], bevel=r * 0.04, segments=1, angle=25.0)


def lens(P, m, loc, nrm, r, depth=None, socket_mat=None):
    """White eye: a faceted dome lens in a dark bezel, sitting on the surface (loc, nrm)."""
    M = frame_on(loc, nrm)
    depth = depth or r * 0.9
    bez = L.cyl(P.coll, r * 1.35, depth, segments=8, axis="Z", center=(0, 0, -depth * 0.25))
    P.add(bez, socket_mat or m["dark"], bevel=r * 0.08, segments=1, matrix=M)
    dome = L.lathe(P.coll, [(0.0, r), (r * 0.35, r * 0.85), (r * 0.6, r * 0.45), (r * 0.7, 0.0)], segments=8, axis="Z")
    dome.data.transform(T(0, 0, depth * 0.1))
    P.add(dome, m["eye"], smooth=50.0, matrix=M)


def slit(P, m, loc, nrm, length, height, depth, up=(0.0, 0.0, 1.0), along=None, frame=True):
    """White eye slit: a thin glowing bar on the surface, in a dark frame. `along` = slit direction."""
    M = frame_on(loc, nrm, up)
    if along is not None:
        n = Vector(nrm).normalized()
        a = Vector(along)
        a = (a - n * a.dot(n)).normalized()
        y = n.cross(a).normalized()
        M = Matrix((a, y, n)).transposed().to_4x4()
        M.translation = loc
    if frame:
        f = box((length + height * 1.6, height * 2.4, depth), (0, 0, -depth * 0.3))
        P.add(f, m["dark"], bevel=height * 0.25, segments=1, matrix=M)
    s = box((length, height, depth * 0.6), (0, 0, depth * 0.05))
    P.add(s, m["eye"], bevel=height * 0.2, segments=1, matrix=M)


def pz(obj, thickness, depth, **kw):
    panelize(obj, thickness, depth, **kw)
    return obj


def panelize(obj, thickness, depth, fraction=0.85, seed=0, normal=None, min_dot=-1.0):
    """Inset + recess the hull's facets: bevelled plate breaks with a recessed seam all round (trooper style).
    Call on a fresh loft BEFORE P.add (the bevel then rounds the plate edges)."""
    return L.hs["inset_panels"](obj, normal=normal, min_dot=min_dot, fraction=fraction, thickness=thickness,
                                depth=-depth, seed=seed, min_area=(thickness * 6.0) ** 2)


def eye_chevron(P, m, surf, y_from, half_w, z_mid, z_tip, r, n=9, dip=0.0):
    """White chevron eye wrapped across a forward face: ray-cast points from the front, a glowing tube on a
    dark backing tube. z goes from z_mid at the centre (minus dip) to z_tip at +-half_w."""
    pts = []
    for i in range(n):
        x = -half_w + 2.0 * half_w * i / (n - 1)
        t = abs(x) / half_w
        z = z_mid - dip * (1.0 - t) + (z_tip - z_mid) * t
        loc, nrm = surf.hit((x, y_from, z), (0, 1, 0))
        if loc is not None:
            pts.append((loc, nrm))
    if len(pts) < 2:
        return []
    back = tube([l + nr * (r * 0.3) for l, nr in pts], r * 1.7, sides=6)
    P.add(back, m["dark"], smooth=40.0)
    eye = tube([l + nr * (r * 1.4) for l, nr in pts], r, sides=6)
    P.add(eye, m["eye"], smooth=40.0)
    return [[round(c, 3) for c in l] for l, _n in pts]


def hex_ring(P, m, center, normal, radius, r, mat="glow"):
    M = frame_on(Vector(center), Vector(normal))
    pts = [M @ V(radius * math.cos(a), radius * math.sin(a), 0.0) for a in [math.tau * k / 6 + math.pi / 6 for k in range(7)]]
    return glow_line(P, m, pts, r, mat)


def greeble_cluster(P, mat, surf, center, normal, count, size, height, spread, seed, along=None):
    """Hand-placed mechanical boxes on a surface patch (clustered, never sprinkled)."""
    rng = random.Random(seed)
    n = Vector(normal).normalized()
    t1 = (Vector(along) if along else n.orthogonal()).normalized()
    t1 = (t1 - n * t1.dot(n)).normalized()
    t2 = n.cross(t1).normalized()
    for _ in range(count):
        off = t1 * rng.uniform(-spread, spread) + t2 * rng.uniform(-spread * 0.6, spread * 0.6)
        o0 = Vector(center) + off + n * 2.0 * spread
        loc, nrm = surf.hit(o0, -n)
        if loc is None:
            continue
        sx, sy, sz = rng.uniform(*size), rng.uniform(*size), rng.uniform(*height)
        M = frame_on(loc, nrm, up=t2)
        b = box((sx, sy, sz + 0.02), (0, 0, sz * 0.5 - 0.01))
        P.add(b, mat, bevel=min(sx, sy, sz) * 0.12, segments=1, matrix=M)


def glow_line(P, m, pts, r, mat="glow"):
    """Red-orange pinstripe along a polyline (diamond tube: reads as a lit seam)."""
    pts = [Vector(q) for q in pts]
    if len(pts) > 2 and (pts[0] - pts[-1]).length < 1e-6:     # closed loop: no coincident end rings
        pts[-1] = pts[-2].lerp(pts[0], 0.97)
    t = tube(pts, r * GLOW_K, sides=4)
    P.add(t, m[mat], smooth=None)
    return t


def chine_points(stations, side, frac=1.0, push=0.0, zfix=None):
    """Points along the hull chine (the widest point of armour_sec) for each station (y, w, ht, hb, z0, kw)."""
    out = []
    for st in stations:
        y, w, ht, hb, z0 = st[:5]
        if w <= 1e-6:
            continue
        kw = st[5] if len(st) > 5 else {}
        ch = kw.get("chine", 0.0)
        cz = ch * (ht if ch >= 0 else hb)
        out.append(V(side * (w * 0.5 * frac + push), y, z0 + cz if zfix is None else zfix))
    return out


def shoulder_points(stations, side, push=0.0):
    out = []
    for st in stations:
        y, w, ht, hb, z0 = st[:5]
        if w <= 1e-6:
            continue
        kw = st[5] if len(st) > 5 else {}
        sh = kw.get("shoulder", 0.86)
        ch = kw.get("chine", 0.0) * ht
        out.append(V(side * (w * 0.5 * sh + push), y, z0 + ch + (ht - ch) * 0.42 + push))
    return out


def interp_stations(stations, y):
    """Linear interpolation of (w, ht, hb, z0) at y."""
    s = [st for st in stations if st[1] > 1e-6]
    for a, b in zip(s, s[1:]):
        if a[0] <= y <= b[0]:
            t = (y - a[0]) / max(1e-9, b[0] - a[0])
            return tuple(a[i] + (b[i] - a[i]) * t for i in range(1, 5))
    st = s[0] if y < s[0][0] else s[-1]
    return st[1], st[2], st[3], st[4]


# =========================================================================== 1. Shard Fighter

def build_fighter(asset, tag, stage):
    """Shard Fighter (Unreal radius 220 cm): ~5.2 m long, 4.6 m span. Dagger fuselage, forward-swept
    crescent blade wings whose tips reach past the cockpit, a single white eye slit, twin red-orange nozzles."""
    P = NFPart(asset, COLL)
    sc = 0.3
    m = mats("NF_Fighter", sc, None)
    st = [(-2.75, 0, 0, 0, -0.05),
          (-2.40, 0.55, 0.13, 0.10, -0.04, {}),
          (-1.85, 1.10, 0.28, 0.20, 0.0, {"chine": 0.05}),
          (-1.25, 1.30, 0.40, 0.26, 0.0, {"chine": 0.1}),
          (-0.55, 0.98, 0.42, 0.28, 0.0, {}),
          (0.40, 0.92, 0.40, 0.30, 0.0, {}),
          (1.30, 1.05, 0.36, 0.30, 0.0, {}),
          (2.05, 1.00, 0.30, 0.26, 0.0, {}),
          (2.22, 0.94, 0.27, 0.24, 0.0, {})]
    hull = pz(hull_loft(st), 0.035, 0.012, seed=1)
    P.add(hull, m["armour"], bevel=0.008, segments=2, angle=28.0)
    # head: an angular armoured brow with a steep, forward-raked face that carries the eye
    can = poly_loft([V(0, -2.08, 0.10),
                     ring_y(-1.82, armour_sec(0.62, 0.20, 0.06, chine=0.2), z0=0.22),
                     ring_y(-1.35, armour_sec(0.86, 0.30, 0.10, chine=0.2), z0=0.28),
                     ring_y(-0.60, armour_sec(0.66, 0.30, 0.12, chine=0.25), z0=0.28),
                     ring_y(0.45, armour_sec(0.40, 0.18, 0.10, chine=0.2), z0=0.30),
                     V(0, 1.05, 0.32)])
    P.add(can, m["plate"], bevel=0.01, segments=2, angle=26.0)
    P.hull_from([hull, can])
    # forward-swept crescent blade wings (tips ahead of the cockpit, hooked down)
    wst = [(0.30, -0.55, 2.35, 0.32, 0.0), (1.15, -0.95, 1.62, 0.24, -0.06), (1.85, -1.52, 1.02, 0.15, -0.16),
           (2.30, -2.18, 0.45, 0.09, -0.32)]
    wa, wb = both(lambda: pz(blade_wing(wst), 0.03, 0.01, seed=2, normal=(0, 0, 1), min_dot=0.5), P, m["armour"],
                  bevel=0.007, segments=2, angle=24.0)
    P.hull_from([wa])
    P.hull_from([wb])
    # canted tail fins (V), for the side and front silhouette
    fst = [(0.20, 0.75, 1.45, 0.12, 0.0), (0.85, 1.35, 0.90, 0.08, 0.0), (1.25, 1.85, 0.45, 0.05, 0.0)]

    def fin():
        o = blade_wing(fst)
        o.data.transform(T(0.02, 0, 0.12) @ rot(-42, "Y"))
        return o
    both(fin, P, m["armour"], bevel=0.01, segments=1, angle=24.0)
    # wing-root fairings: angular strakes that blend the head into the wing roots
    both(lambda: limb([(0.42, -1.75, 0.02), (0.62, -0.9, 0.06), (0.68, 0.2, 0.04), (0.55, 1.2, 0.02)],
                      [0.30, 0.42, 0.40, 0.2], [0.22, 0.30, 0.28, 0.16], kind="hex"),
         P, m["plate"], bevel=0.012, segments=1, angle=26.0)
    # forward mandible blades off the head's cheeks (heavier, predatory front)
    both(lambda: limb([(0.42, -1.55, -0.06), (0.62, -2.25, -0.1), (0.5, -2.85, -0.14)], [0.26, 0.17, 0.02],
                      [0.16, 0.1, 0.02], kind="hex", tip=True), P, m["plate"], bevel=0.008, segments=1, angle=26.0)
    # ventral keel blade
    keel = blade_wing([(0.0, 0.35, 1.6, 0.09, 0.0), (0.42, 1.05, 0.8, 0.05, 0.0)])
    keel.data.transform(T(0, 0, -0.24) @ rot(90, "Y"))
    P.add(keel, m["plate"], bevel=0.006, segments=1, angle=24.0)
    # engine pods + nozzles
    for sx in (1, -1):
        pod = lathe_y([(1.15, 0.0), (1.3, 0.17), (1.6, 0.25), (2.25, 0.26), (2.25, 0.0)], segments=8,
                      center=(sx * 0.40, 0, -0.04))
        P.add(pod, m["armour"], bevel=0.01, segments=1, angle=30.0)
        nozzle(P, m, (sx * 0.40, 2.18, -0.04), 0.22, 0.30)
    surf = Surf(P.parts)
    # the single white eye: a chevron slit wrapped across the head's raked front face (front, sides, 3/4)
    eye = eye_chevron(P, m, surf, -3.2, 0.5, 0.26, 0.17, 0.065, n=11, dip=0.05)
    # second eye under the nose, for head-on passes from below
    loc, nrm = surf.hit((0.0, -2.35, -1.0), (0, 0, 1))
    if loc is not None:
        slit(P, m, loc + nrm * 0.005, nrm, 0.42, 0.08, 0.08, along=(1, 0, 0))
    if stage == "blockout":
        return P, [], {}
    # wing-root guns (Muzzle_0/1), slim barrels under the leading edge
    muzzles = []
    for sx in (1, -1):
        g = lathe_y([(-2.05, 0.0), (-2.05, 0.05), (-1.3, 0.06), (-1.0, 0.09), (0.1, 0.09), (0.1, 0.0)],
                    segments=8, center=(sx * 0.6, 0, -0.12))
        P.add(g, m["metal"], smooth=40.0)
        hp = box((0.2, 1.3, 0.16), (sx * 0.56, -0.75, -0.08))
        P.add(hp, m["plate"], bevel=0.015, segments=1)
        muzzles.append(("Muzzle_%d" % (0 if sx > 0 else 1), (sx * 0.6, -2.08, -0.12)))
    # red-orange pinstripes: hull chines, wing leading edges, canopy seam
    for sx in (1, -1):
        pts = [p + V(0, 0, 0.004) for p in chine_points(st[2:8], sx, push=0.006)]
        glow_line(P, m, pts, 0.012)
        le = [V(sx * (x + 0.02), yle - 0.01, z) for x, yle, c, t, z in wst]
        glow_line(P, m, le, 0.022)
        # fin edges
        fe = [T(0.02, 0, 0.12) @ rot(-42, "Y") @ V(x, yle + c + 0.01, z) for x, yle, c, t, z in fst]
        glow_line(P, m, [V(sx * p.x, p.y, p.z) for p in fe], 0.02)
        # seam where wing meets fuselage (a short slanted stripe on the wing top)
        glow_line(P, m, [V(sx * 0.62, -0.5, 0.075), V(sx * 1.18, -0.68, 0.035), V(sx * 1.7, -1.12, 0.0)], 0.014)
    # dorsal spine plate + red seam behind the canopy
    sp = poly_loft([ring_y(0.5, armour_sec(0.30, 0.10, 0.04), z0=0.36), ring_y(1.6, armour_sec(0.36, 0.10, 0.05), z0=0.32),
                    ring_y(2.2, armour_sec(0.22, 0.06, 0.04), z0=0.26)])
    P.add(sp, m["plate"], bevel=0.008, segments=1)
    hex_ring(P, m, (0.0, 1.15, 0.455), (0, 0.04, 1), 0.12, 0.014)
    # greebles behind the canopy and around the engine roots
    surf = Surf(P.parts)
    greeble_cluster(P, m["metal"], surf, (0.0, 1.55, 0.42), (0, 0, 1), 6, (0.05, 0.12), (0.02, 0.05), 0.22, seed=3,
                    along=(0, 1, 0))
    for sx in (1, -1):
        greeble_cluster(P, m["metal"], surf, (sx * 0.62, 1.4, 0.2), (sx * 0.7, 0, 0.7), 4, (0.04, 0.09), (0.02, 0.04),
                        0.15, seed=7 + sx, along=(0, 1, 0))
    engines = [("Engine_0", (0.40, 2.48, -0.04)), ("Engine_1", (-0.40, 2.48, -0.04))]
    return P, muzzles + engines, {"role": "fighter", "ue_radius_cm": 220, "eye_m": eye}


# =========================================================================== 2. Hook Interceptor

def build_interceptor(asset, tag, stage):
    """Hook Interceptor (radius 260 cm): ~6.3 m long. A hooked, beaked prow that overhangs a recessed chin,
    a brow crest rising to a dorsal fin, swept wings with down-hooked tips, two missile pods, three white eyes."""
    P = NFPart(asset, COLL)
    sc = 0.35
    m = mats("NF_Interceptor", sc, 0.6)
    st = [(-3.20, 0, 0, 0, -1.15),
          (-3.00, 0.24, 0.20, 0.08, -0.86, {}),
          (-2.70, 0.48, 0.34, 0.09, -0.40, {"keel": 0.8}),
          (-2.25, 0.66, 0.38, 0.10, 0.08, {"keel": 0.8}),
          (-1.65, 0.96, 0.46, 0.34, 0.14, {"chine": 0.1}),
          (-0.90, 0.86, 0.38, 0.30, 0.06, {}),
          (0.30, 0.84, 0.34, 0.30, 0.0, {}),
          (1.60, 0.80, 0.30, 0.27, 0.0, {}),
          (2.60, 0.72, 0.27, 0.24, 0.0, {}),
          (2.95, 0.66, 0.24, 0.22, 0.0, {})]
    hull = pz(hull_loft(st), 0.035, 0.012, seed=3)
    P.add(hull, m["armour"], bevel=0.008, segments=2, angle=28.0)
    # lower jaw: set back under the beak, so the beak overhangs a notch (the hook reads in side view)
    jaw = poly_loft([V(0, -2.45, -0.30), ring_y(-2.15, armour_sec(0.36, 0.08, 0.12), z0=-0.20),
                     ring_y(-1.40, armour_sec(0.62, 0.12, 0.16), z0=-0.16),
                     ring_y(-0.60, armour_sec(0.64, 0.10, 0.14), z0=-0.15)])
    P.add(jaw, m["plate"], bevel=0.01, segments=1, angle=28.0)
    # brow crest: a blade ridge over the head (the visual weight sits forward); low along the back
    cr = poly_loft([V(0, -2.60, 0.20),
                    [V(-0.08, -2.1, 0.44), V(0.08, -2.1, 0.44), V(0.0, -2.1, 0.66)],
                    [V(-0.10, -1.4, 0.56), V(0.10, -1.4, 0.56), V(0.0, -1.4, 0.80)],
                    [V(-0.09, -0.4, 0.40), V(0.09, -0.4, 0.40), V(0.0, -0.4, 0.58)],
                    [V(-0.07, 1.8, 0.28), V(0.07, 1.8, 0.28), V(0.0, 1.8, 0.42)],
                    V(0, 2.6, 0.26)])
    P.add(cr, m["plate"], bevel=0.008, segments=1, angle=24.0)
    P.hull_from([hull, jaw, cr])
    # twin raked tail blades (low; they no longer out-weigh the head)
    tfst = [(0.0, 1.55, 1.25, 0.10, 0.0), (0.45, 2.15, 0.75, 0.07, 0.0), (0.70, 2.70, 0.32, 0.04, 0.0)]

    def tail_blade():
        o = blade_wing(tfst)
        o.data.transform(T(0.14, 0, 0.26) @ rot(-58, "Y"))
        return o
    both(tail_blade, P, m["crimson"], bevel=0.008, segments=1, angle=24.0)
    # mandible canards: short forward-swept blades on the head's cheeks
    both(lambda: blade_wing([(0.30, -1.95, 0.75, 0.07, 0.06), (0.78, -2.42, 0.32, 0.035, 0.0)]), P, m["plate"],
         bevel=0.005, segments=1, angle=24.0)
    # swept-back wings with a down-hooked claw tip
    wst = [(0.32, -0.50, 2.40, 0.30, -0.02), (1.25, 0.35, 1.45, 0.20, -0.06), (1.85, 0.90, 0.85, 0.12, -0.12)]
    wa, wb = both(lambda: pz(blade_wing(wst), 0.03, 0.01, seed=4, normal=(0, 0, 1), min_dot=0.5), P, m["armour"],
                  bevel=0.007, segments=2, angle=24.0)
    P.hull_from([wa])
    P.hull_from([wb])
    # separate down-turned claw at each wing tip, hooking forward
    both(lambda: limb([(1.80, 0.95, -0.08), (2.05, 1.30, -0.45), (2.05, 1.05, -0.95), (1.95, 0.55, -1.25)],
                      [0.34, 0.28, 0.18, 0.02], [0.55, 0.36, 0.2, 0.02], up=(1, 0, 0), kind="hex", tip=True),
         P, m["plate"], bevel=0.008, segments=1, angle=26.0)
    # missile pods under the wings
    pods = []
    for sx in (1, -1):
        pod = poly_loft([ring_y(-1.05, armour_sec(0.30, 0.15, 0.15, chine=0.0), x0=0, z0=0),
                         ring_y(-0.95, armour_sec(0.42, 0.21, 0.21), z0=0),
                         ring_y(0.9, armour_sec(0.44, 0.22, 0.22), z0=0),
                         ring_y(1.35, armour_sec(0.30, 0.14, 0.14), z0=0)])
        pod.data.transform(T(sx * 1.12, 0, -0.33))
        P.add(pod, m["armour"], bevel=0.01, segments=1, angle=28.0)
        pyl = box((0.08, 0.9, 0.3), (sx * 1.12, 0.0, -0.13))
        P.add(pyl, m["metal"], bevel=0.01, segments=1)
        pods.append(pod)
    for sx in (1, -1):
        nozzle(P, m, (sx * 0.24, 2.90, -0.02), 0.2, 0.28)
    surf = Surf(P.parts)
    # three white eyes in a triangle on the beak's face: one high on the brow, two lower on the flanks
    eyes = []
    # (r2 review: the two lower eyes were hidden) - the flank eyes are now cast from the front quarters
    for (ox, oy, oz, d, r) in ((0.0, -3.6, 0.40, (0, 1, -0.1), 0.15), (1.5, -1.95, 0.3, (-1.0, 0.0, -0.15), 0.12),
                               (-1.5, -1.95, 0.3, (1.0, 0.0, -0.15), 0.12)):
        loc, nrm = surf.hit((ox, oy, oz), d)
        if loc is not None:
            if ox == 0.0:
                lens(P, m, loc, nrm, r)
            else:   # flank eyes: slits raked along the beak
                slit(P, m, loc + nrm * 0.005, nrm, 0.42, 0.11, 0.1, along=(0.0, -1.0, -0.35))
            eyes.append([round(c, 3) for c in loc])
    if stage == "blockout":
        return P, [], {}
    muzzles = []
    # chin guns
    for i, sx in enumerate((1, -1)):
        g = lathe_y([(-2.4, 0.0), (-2.4, 0.03), (-1.6, 0.04), (-1.2, 0.06), (-0.9, 0.06), (-0.9, 0.0)],
                    segments=8, center=(sx * 0.2, 0, -0.27))
        P.add(g, m["metal"], smooth=40.0)
        muzzles.append(("Muzzle_%d" % i, (sx * 0.2, -2.42, -0.27)))
    # missile tubes in the pod noses: dark tube mouths with red warhead tips
    for k, sx in enumerate((1, -1)):
        for j, (dx, dz) in enumerate(((0.0, 0.07), (-0.08, -0.06), (0.08, -0.06))):
            c = V(sx * 1.12 + dx, -1.07, -0.33 + dz)
            tub = lathe_y([(-0.04, 0.0), (-0.04, 0.05), (0.15, 0.05), (0.15, 0.0)], segments=8, center=tuple(c))
            P.add(tub, m["dark"], smooth=40.0)
            tip = lathe_y([(-0.02, 0.0), (0.0, 0.028), (0.06, 0.038), (0.06, 0.0)], segments=8, center=tuple(c))
            P.add(tip, m["glow"], smooth=40.0)
        muzzles.append(("Muzzle_%d" % (2 + k), (sx * 1.12, -1.12, -0.33)))
    # pinstripes: chines, wing leading edges, pod spines, brow
    for sx in (1, -1):
        # one long seam per side: along the head's chine, then out along the wing's leading edge
        seam = [p + V(0, 0, 0.003) for p in chine_points(st[3:5], sx, push=0.008)]
        seam += [V(sx * (x + 0.015), yle - 0.01, z) for x, yle, c, t, z in wst]
        glow_line(P, m, seam, 0.024)
        glow_line(P, m, [V(sx * 1.12, -0.9, -0.33 + 0.215), V(sx * 1.12, 0.85, -0.33 + 0.225)], 0.012)
        glow_line(P, m, [V(sx * 1.83, 0.82, -0.05), V(sx * 2.0, 1.12, -0.45), V(sx * 1.98, 0.9, -0.95),
                         V(sx * 1.93, 0.62, -1.15)], 0.016)
    surf = Surf(P.parts)
    greeble_cluster(P, m["metal"], surf, (0.0, 1.9, 0.35), (0, 0, 1), 6, (0.05, 0.12), (0.02, 0.05), 0.25, seed=5,
                    along=(0, 1, 0))
    for sx in (1, -1):
        greeble_cluster(P, m["metal"], surf, (sx * 0.5, 2.2, 0.1), (sx, 0, 0.5), 4, (0.04, 0.1), (0.02, 0.04), 0.18,
                        seed=11 + sx, along=(0, 1, 0))
    engines = [("Engine_0", (0.24, 3.18, -0.02)), ("Engine_1", (-0.24, 3.18, -0.02))]
    return P, muzzles + engines, {"role": "interceptor", "ue_radius_cm": 260, "eyes_m": eyes}


# =========================================================================== 3. Wall Gunship

def wall_band(z0, z1, x_half, y_front, sweep, lean, thick, tip_drop):
    """One horizontal band of the gunship's chevron shield wall (+X half). Stations along x."""
    # full width in one closed piece (two mirrored halves left 50 coincident vertices on the centre line)
    rings = []
    half = [0.0, x_half * 0.35, x_half * 0.7, x_half * 0.92, x_half]
    xs = [-x for x in reversed(half[1:])] + half
    for i, x in enumerate(xs):
        t = abs(x) / x_half
        yc = y_front + sweep * t
        a, b = z0 * (1 - tip_drop * t * t), z1 * (1 - tip_drop * t * t)
        k = 0.55 if i in (0, len(xs) - 1) else 1.0
        zc = (a + b) * 0.5
        a, b = zc + (a - zc) * k, zc + (b - zc) * k
        # front face leans back at the top; the band's mid line is a raised prow ridge
        mid = (a + b) * 0.5
        pts = [(yc + lean * (b - mid), b), (yc - 0.12, mid), (yc + lean * (a - mid) * -1.0, a),
               (yc + thick + lean * 0.2 * (a - mid) * -1.0, a), (yc + thick, mid), (yc + thick + lean * (b - mid), b)]
        rings.append([V(x, py, pz) for py, pz in pts])
    return poly_loft(rings)


def build_gunship(asset, tag, stage):
    """Wall Gunship (radius 700 cm): ~14.5 m long, 16.5 m wide. A chevron shield wall of three stacked
    armour bands with glowing seams and a band of four white eyes; a heavy body behind; side cannon
    blisters outboard of the wall; a dorsal missile launcher; four engines."""
    P = NFPart(asset, COLL)
    sc = 1.2
    m = mats("NF_Gunship", sc, 1.4)
    st = [(-4.6, 7.6, 2.0, 1.8, 0.2, {"chine": 0.1}),
          (-2.0, 8.6, 2.3, 2.0, 0.2, {"chine": 0.1}),
          (2.0, 8.2, 2.2, 1.9, 0.2, {"chine": 0.1}),
          (5.4, 6.8, 1.8, 1.7, 0.2, {"chine": 0.1}),
          (6.7, 6.0, 1.5, 1.4, 0.2, {"chine": 0.1})]
    body = pz(hull_loft(st), 0.25, 0.06, seed=5)
    P.add(body, m["armour"], bevel=0.04, segments=2, angle=28.0)
    P.hull_from([body])
    # shield wall: glowing core behind three armour bands (the gaps between bands are the seams)
    bands = [(1.05, 3.5), (-0.70, 0.85), (-2.9, -0.90)]
    one(lambda: wall_band(-2.5, 2.9, 6.1, -5.9, 3.1, 0.25, 0.9, 0.25), P, m["dark"], bevel=0.03, segments=1, angle=30.0)
    # glowing seams recessed in the two gaps between the bands
    if stage != "blockout":
        for za, zb in ((0.86, 1.04), (-0.89, -0.71)):
            one(lambda za=za, zb=zb: wall_band(za, zb, 6.0, -6.35, 3.1, 0.3, 0.3, 0.22), P, m["glow"],
                 bevel=0.01, segments=1, angle=30.0)
    wall_objs = []
    for i, (z0, z1) in enumerate(bands):
        mk = (lambda z0=z0, z1=z1, i=i: pz(wall_band(z0, z1, 6.6 - i * 0.3, -6.6 + i * 0.15, 3.3, 0.45, 1.6, 0.22),
                                           0.2, 0.05, seed=6 + i, normal=(0, -1, 0), min_dot=0.3))
        a, b = one(mk, P, m["plate"] if i == 1 else m["armour"], bevel=0.03, segments=2, angle=26.0)
        wall_objs += [a, b]
    P.hull_from(wall_objs)
    # dark visor slot at the chevron apex that carries the eyes
    one(lambda: wall_band(-0.34, 0.48, 2.7, -6.58, 3.3 * 2.7 / 6.3, 0.3, 0.6, 0.0), P, m["dark"], bevel=0.02,
         segments=1, angle=30.0)
    # forward-raked horns off the wall's top corners and a chin blade under the prow
    for sx in (1, -1):
        h = limb([(sx * 5.2, -2.6, 2.3), (sx * 6.4, -4.2, 3.1), (sx * 7.0, -6.0, 3.9)], [1.0, 0.7, 0.1],
                 [0.8, 0.5, 0.1], kind="hex", tip=True)
        P.add(h, m["plate"], bevel=0.03, segments=1, angle=26.0)
    chin = limb([(0.0, -4.2, -2.3), (0.0, -6.2, -2.7), (0.0, -7.2, -3.0)], [2.2, 1.0, 0.1], [0.6, 0.4, 0.1],
                kind="hex", tip=True)
    P.add(chin, m["plate"], bevel=0.03, segments=1, angle=26.0)
    # side cannon blisters (outboard of the wall) with twin barrels
    for sx in (1, -1):
        bl = lathe_y([(-2.6, 0.0), (-2.45, 0.7), (-1.8, 1.15), (0.2, 1.25), (1.4, 1.05), (1.9, 0.0)], segments=10,
                     center=(sx * 7.3, -0.6, 0.35))
        P.add(bl, m["armour"], bevel=0.03, segments=1, angle=30.0)
        arm = box((3.4, 2.6, 1.1), (sx * 5.6, -0.4, 0.35))
        P.add(arm, m["plate"], bevel=0.05, segments=1)
    for i in range(4):
        sx = 1 if i < 2 else -1
        dz = 0.35 if i % 2 == 0 else -0.25
        b = lathe_y([(-6.9, 0.0), (-6.9, 0.16), (-6.2, 0.16), (-6.1, 0.22), (-4.6, 0.2), (-3.4, 0.28), (-2.2, 0.28),
                     (-2.2, 0.0)], segments=10, center=(sx * 7.3, 0, 0.35 + dz))
        P.add(b, m["metal"], smooth=40.0)
    for sx in (1, -1):
        for x in (1.35, 3.7):
            nozzle(P, m, (sx * x, 6.55, 0.15), 0.65, 0.8)
    surf = Surf(P.parts)
    # four white eyes in the middle band
    eyes = []
    for x in (0.72, 2.05, -0.72, -2.05):
        loc, nrm = surf.hit((x, -9.0, 0.05), (0, 1, 0))
        if loc is not None:
            slit(P, m, loc + nrm * 0.01, nrm, 1.2, 0.46, 0.4, along=(1, 0, 0))
            eyes.append([round(c, 3) for c in loc])
    if stage == "blockout":
        return P, [], {}
    muzzles = []
    for i in range(4):
        sx = 1 if i < 2 else -1
        dz = 0.35 if i % 2 == 0 else -0.25
        muzzles.append(("Muzzle_%d" % i, (sx * 7.3, -6.95, 0.35 + dz)))
    # dorsal missile launcher (salvo of 3): an angled box with three hatches whose seams glow
    lb = poly_loft([ring_y(0.4, [(-1.3, 0.0), (1.3, 0.0), (1.1, 0.9), (-1.1, 0.9)], z0=2.3),
                    ring_y(3.2, [(-1.3, 0.0), (1.3, 0.0), (1.1, 0.9), (-1.1, 0.9)], z0=2.25)])
    P.add(lb, m["plate"], bevel=0.04, segments=2)
    for k, x in enumerate((-0.72, 0.0, 0.72)):
        h = box((0.56, 2.2, 0.1), (x, 1.8, 3.2))
        P.add(h, m["armour"], bevel=0.02, segments=1)
        glow_line(P, m, [V(x - 0.31, 0.75, 3.185), V(x - 0.31, 2.85, 3.185)], 0.03)
    # crimson accent plates on the body flanks behind the blisters
    for sx in (1, -1):
        cp = poly_loft([ring_y(1.0, [(0, -0.9), (sx * 0.18, -0.9), (sx * 0.18, 1.0), (0, 0.9)], x0=sx * 4.05, z0=0.4),
                        ring_y(4.6, [(0, -0.7), (sx * 0.18, -0.7), (sx * 0.18, 0.8), (0, 0.7)], x0=sx * 3.48, z0=0.4)])
        P.add(cp, m["crimson"], bevel=0.03, segments=1)
    # pinstripes: hull chines and the top deck edge
    for sx in (1, -1):
        glow_line(P, m, [p + V(0, 0, 0.01) for p in chine_points(st, sx, push=0.02)], 0.05)
        glow_line(P, m, [p for p in shoulder_points(st[1:], sx, push=0.015)], 0.04)
    surf = Surf(P.parts)
    greeble_cluster(P, m["metal"], surf, (0.0, 4.6, 2.4), (0, 0, 1), 10, (0.18, 0.45), (0.08, 0.2), 1.2, seed=3,
                    along=(0, 1, 0))
    for sx in (1, -1):
        greeble_cluster(P, m["metal"], surf, (sx * 4.2, 3.5, 1.4), (sx * 0.6, 0, 0.8), 6, (0.15, 0.35), (0.06, 0.15),
                        0.9, seed=9 + sx, along=(0, 1, 0))
    for c, n in (((0.0, -1.8, 2.6), (0, 0, 1)), ((5.0, -2.5, 1.2), (0.5, 0, 1)), ((-5.0, -2.5, 1.2), (-0.5, 0, 1))):
        greeble_cluster(P, m["metal"], surf, c, n, 8, (0.15, 0.4), (0.06, 0.18), 1.0, seed=int(c[0] * 3 + 40),
                        along=(0, 1, 0))
    engines = [("Engine_%d" % i, (sx * x, 7.35, 0.15)) for i, (sx, x) in enumerate(((1, 1.35), (1, 3.7), (-1, 1.35),
                                                                                    (-1, 3.7)))]
    return P, muzzles + engines, {"role": "gunship", "ue_radius_cm": 700, "eyes_m": eyes,
                                  "frontal_shield": "the chevron wall is the shield plate; its glowing seams can be "
                                                    "driven by EmissiveStrength while the shield is up"}


# =========================================================================== 4. Maul Bomber

def build_bomber(asset, tag, stage):
    """Maul Bomber (radius 500 cm): ~12 m span, ~10 m long, low. A manta-like flying wing with down-turned
    claw tips, a belly mine-bay hatch, two heavy engine nacelles and a third central nozzle."""
    P = NFPart(asset, COLL)
    sc = 0.9
    m = mats("NF_Bomber", sc, 1.0)
    st = [(-4.75, 0, 0, 0, -0.15),
          (-4.30, 0.9, 0.30, 0.25, -0.1, {}),
          (-3.40, 1.9, 0.62, 0.48, 0.0, {}),
          (-1.80, 2.9, 0.95, 0.72, 0.05, {}),
          (0.80, 3.3, 1.05, 0.82, 0.05, {}),
          (3.20, 3.0, 0.92, 0.76, 0.05, {}),
          (4.55, 2.5, 0.72, 0.62, 0.05, {})]
    body = pz(hull_loft(st), 0.12, 0.035, seed=8)
    P.add(body, m["armour"], bevel=0.025, segments=2, angle=28.0)
    P.hull_from([body])
    # thick wedge-section wing blended into the body: reads as one heavy manta mass
    wst = [(1.20, -2.55, 6.4, 1.4, 0.0), (3.40, -0.75, 4.3, 1.0, -0.08), (5.05, 0.75, 2.7, 0.62, -0.3),
           (5.85, 1.45, 1.5, 0.38, -0.85), (6.05, 1.75, 0.9, 0.2, -1.35)]
    wa, wb = both(lambda: pz(blade_wing(wst), 0.1, 0.03, seed=9, normal=(0, 0, 1), min_dot=0.4), P, m["armour"],
                  bevel=0.02, segments=2, angle=24.0)
    P.hull_from([wa])
    P.hull_from([wb])
    # forward "maul" claws along the drooped wingtips
    cst = [(5.75, -1.5, 3.4, 0.5, -0.80), (5.95, -1.9, 1.9, 0.32, -1.15), (6.02, -2.15, 0.7, 0.14, -1.45)]

    def claw():
        rings = []
        for x, yle, c, t, z in cst:
            rings.append([V(x + pz, yle + py, z) for py, pz in blade_sec(c, t)])
        return poly_loft(rings)
    both(claw, P, m["plate"], bevel=0.015, segments=1, angle=24.0)
    # nose mandibles: two hooked prongs ahead of the body, low
    for sx in (1, -1):
        md = limb([(sx * 0.8, -2.6, -0.2), (sx * 1.45, -4.2, -0.32), (sx * 1.15, -5.0, -0.48), (sx * 0.55, -5.35, -0.6)],
                  [1.5, 1.1, 0.6, 0.04], [1.0, 0.75, 0.42, 0.04], kind="hex", tip=True)
        # raised armoured spine where the wing meets the body
        sp = limb([(sx * 1.55, -2.9, 0.35), (sx * 1.75, -0.5, 0.75), (sx * 1.7, 2.6, 0.7)], [0.7, 0.8, 0.6],
                  [0.45, 0.55, 0.4], kind="hex")
        P.add(sp, m["plate"], bevel=0.03, segments=1, angle=26.0)
        P.add(md, m["plate"], bevel=0.015, segments=1, angle=26.0)
    # trailing-edge teeth: two notched blades on each wing's trailing edge
    for sx in (1, -1):
        for x, y in ((3.4, 3.45), (4.6, 3.3)):
            th = limb([(sx * x, y - 0.6, 0.0), (sx * (x + 0.1), y + 0.35, -0.05), (sx * (x + 0.15), y + 0.9, -0.1)],
                      [0.7, 0.45, 0.05], [0.22, 0.14, 0.04], kind="hex", tip=True)
            P.add(th, m["armour"], bevel=0.012, segments=1, angle=26.0)
    # heavy engine nacelles
    for sx in (1, -1):
        nac = poly_loft([ring_y(0.2, armour_sec(0.8, 0.4, 0.4), z0=-0.1),
                         ring_y(1.0, armour_sec(1.7, 0.85, 0.82), z0=-0.1),
                         ring_y(4.6, armour_sec(1.85, 0.92, 0.9), z0=-0.1),
                         ring_y(4.95, armour_sec(1.75, 0.86, 0.84), z0=-0.1)])
        panelize(nac, 0.1, 0.03, seed=10)
        nac.data.transform(T(sx * 2.05, 0, 0))
        P.add(nac, m["plate"], bevel=0.03, segments=2, angle=28.0)
        P.hull_from([nac])
        nozzle(P, m, (sx * 2.05, 4.85, -0.1), 0.72, 0.75)
    nozzle(P, m, (0.0, 4.5, 0.25), 0.5, 0.55)
    # belly mine bay: a housing under the body with a split hatch
    bay = poly_loft([ring_y(0.2, [(-1.0, 0.1), (1.0, 0.1), (0.85, -0.42), (-0.85, -0.42)], z0=-0.55),
                     ring_y(3.3, [(-1.0, 0.1), (1.0, 0.1), (0.85, -0.42), (-0.85, -0.42)], z0=-0.55)])
    P.add(bay, m["plate"], bevel=0.03, segments=1)
    surf = Surf(P.parts)
    eyes = []
    for sx in (1, -1):
        loc, nrm = surf.hit((sx * 0.62, -6.0, 0.18), (0, 1, -0.05))
        if loc is not None:
            slit(P, m, loc + nrm * 0.01, nrm, 0.62, 0.14, 0.16, along=(sx * 1.0, 0.0, -0.45))
            eyes.append([round(c, 3) for c in loc])
        # a bright eye pair on the head above the mandibles
        loc, nrm = surf.hit((sx * 0.45, -7.0, 0.75), (0, 1, -0.12))
        if loc is not None:
            lens(P, m, loc, nrm, 0.2)
            eyes.append([round(c, 3) for c in loc])
    if stage == "blockout":
        return P, [], {}
    # hatch doors with glowing seams and a glowing mine slot
    # doors parted: the slot between them shows three mines (dark domes with a hot red core)
    for sx in (1, -1):
        d = box((0.62, 2.7, 0.08), (sx * 0.62, 1.75, -0.99))
        P.add(d, m["armour"], bevel=0.015, segments=1)
    slotg = box((0.5, 2.6, 0.05), (0, 1.75, -0.985))
    P.add(slotg, m["glow"], bevel=0.01, segments=1)
    for y in (0.95, 1.75, 2.55):
        mine = L.lathe(P.coll, [(0.0, 0.26), (-0.12, 0.24), (-0.22, 0.14), (-0.26, 0.0)], segments=12, axis="Z")
        mine.data.transform(T(0, y, -0.92))
        P.add(mine, m["metal"], smooth=40.0)
        core = L.cyl(P.coll, 0.08, 0.04, segments=8, axis="Z", center=(0, y, -1.19))
        P.add(core, m["engine"], smooth=40.0)
    glow_line(P, m, [V(-0.9, 0.36, -1.0), V(0.9, 0.36, -1.0)], 0.025)
    glow_line(P, m, [V(-0.9, 3.14, -1.0), V(0.9, 3.14, -1.0)], 0.025)
    for k in range(3):
        chev = [V(-0.5, 0.9 + k * 0.7, -1.04), V(0.0, 0.55 + k * 0.7, -1.04), V(0.5, 0.9 + k * 0.7, -1.04)]
        glow_line(P, m, chev, 0.02)
    muzzles = [("Muzzle_0", (0.0, 1.75, -1.1))]
    # pinstripes: body chine, wing leading edges, nacelle spines, slanted wing seams
    for sx in (1, -1):
        glow_line(P, m, [p + V(0, 0, 0.004) for p in chine_points(st[2:], sx, push=0.01)], 0.022)
        glow_line(P, m, [V(sx * (x + 0.02), yle - 0.01, z) for x, yle, c, t, z in wst[1:4]], 0.026)
        glow_line(P, m, [V(sx * 2.05, 1.05, 0.75 + 0.012), V(sx * 2.05, 4.55, 0.82 + 0.012)], 0.02)
        glow_line(P, m, [V(sx * 1.6, -1.6, 0.26), V(sx * 3.2, -0.3, 0.17), V(sx * 4.6, 0.95, -0.12)], 0.016)
        # crimson accent plates on the wing tops
        cp = poly_loft([[V(sx * 3.3, -0.2, 0.15), V(sx * 4.4, 0.55, 0.05), V(sx * 4.2, 1.6, 0.05), V(sx * 3.2, 1.3, 0.15)],
                        [V(sx * 3.3, -0.2, 0.26), V(sx * 4.4, 0.55, 0.16), V(sx * 4.2, 1.6, 0.16), V(sx * 3.2, 1.3, 0.26)]])
        P.add(cp, m["crimson"], bevel=0.02, segments=1)
    surf = Surf(P.parts)
    greeble_cluster(P, m["metal"], surf, (0.0, 3.0, 1.2), (0, 0, 1), 8, (0.12, 0.3), (0.05, 0.14), 0.7, seed=4,
                    along=(0, 1, 0))
    for sx in (1, -1):
        greeble_cluster(P, m["metal"], surf, (sx * 2.05, 0.9, 0.9), (sx * 0.3, -0.3, 1), 5, (0.1, 0.25), (0.04, 0.1),
                        0.45, seed=6 + sx, along=(0, 1, 0))
    engines = [("Engine_0", (2.05, 5.6, -0.1)), ("Engine_1", (-2.05, 5.6, -0.1)), ("Engine_2", (0.0, 5.05, 0.25))]
    return P, muzzles + engines, {"role": "bomber", "ue_radius_cm": 500, "eyes_m": eyes}


# =========================================================================== 5. Hive Carrier

# Mount points from TDEnemyShip::SpawnCarrierParts / MountPart (Radius 1800 + half extent + clearance 150).
# Unreal -> Blender (x, y, z) = (-uy, -ux, uz).
BAY_DIST = 18.0 + 1.5 + 1.5            # bay centre, m (half extent = 300 cm deep / 2)
TURRET_DIST = 18.0 + 1.2 + 1.5         # turret centre, m (fixed 120 cm "half extent" in the code)


def carrier_mounts():
    bays = []
    for ux, uy, uz in ((0.0, -1.0, 0.0), (0.0, 1.0, 0.0), (-0.3, 0.0, 1.0)):
        d = Vector((-uy, -ux, uz)).normalized()
        bays.append((d * BAY_DIST, d))
    turrets = []
    for i in range(4):
        a = math.pi * 0.5 * i + math.pi * 0.25
        u = Vector((math.cos(a), math.sin(a), 0.6)).normalized()
        d = Vector((-u.y, -u.x, u.z))
        turrets.append((d * TURRET_DIST, d))
    return bays, turrets


CARRIER_ENV = [(-22.4, 0.0), (-21.0, 4.2), (-18.0, 9.6), (-14.0, 12.0), (-8.0, 13.4), (-2.0, 13.0), (6.0, 12.2),
               (14.0, 10.4), (21.4, 8.2)]


def carrier_w(y):
    for (y0, w0), (y1, w1) in zip(CARRIER_ENV, CARRIER_ENV[1:]):
        if y0 <= y <= y1:
            return w0 + (w1 - w0) * (y - y0) / (y1 - y0)
    return CARRIER_ENV[-1][1] if y > 0 else 0.0


def carrier_sec(y, k=1.0):
    w = carrier_w(y) * k
    return armour_sec(w, w * 0.42, w * 0.37, chine=0.05)


def build_carrier(asset, tag, stage):
    """Hive Carrier (radius 1800 cm): ~44 m long. A segmented insect hull of overlapping armour shells (head,
    thorax, three abdomen plates) with stag-beetle mandibles; two cranked arms ending in bay collars; a raked
    dorsal sail carrying the dorsal bay; four horn pylons rising to the turret mounts; a hive of white eyes on
    the head; a cluster of engines at the stern."""
    P = NFPart(asset, COLL)
    sc = 3.0
    m = mats("NF_Carrier", sc, 3.2)
    bays, turrets = carrier_mounts()
    z0 = 0.2
    # overlapping shells: each starts with a flared rim that overlaps the previous shell's tapered rear
    shells = [(-14.4, -8.2), (-8.6, -2.0), (-2.4, 4.0), (3.6, 10.0), (9.6, 15.8), (15.4, 21.4)]
    hull_parts = []
    head = poly_loft([V(0, -22.4, -0.6)] + [ring_y(y, carrier_sec(y, k), z0=z0 - (0.8 if y < -20 else 0.0))
                                            for y, k in ((-21.0, 1.0), (-18.0, 1.0), (-15.6, 1.05), (-14.0, 0.95))])
    panelize(head, 0.4, 0.1, seed=29, fraction=0.9)
    P.add(head, m["plate"], bevel=0.08, segments=2, angle=28.0)
    hull_parts.append(head)
    # heavy brow plate overhanging the eyes (a chevron of armour across the head)
    brow = limb([(-4.6, -18.6, 1.6), (-2.2, -20.2, 2.3), (0.0, -21.0, 2.5), (2.2, -20.2, 2.3), (4.6, -18.6, 1.6)],
                [1.9, 2.1, 2.2, 2.1, 1.9], [0.8, 0.9, 0.95, 0.9, 0.8], up=(0, 0.6, 1), kind="hex")
    P.add(brow, m["armour"], bevel=0.06, segments=2, angle=28.0)
    for i, (ya, yb) in enumerate(shells):
        rings = [ring_y(ya, carrier_sec(ya, 1.0), z0=z0), ring_y(ya + 0.7, carrier_sec(ya + 0.7, 1.07), z0=z0),
                 ring_y(ya + 2.0, carrier_sec(ya + 2.0, 1.05), z0=z0), ring_y(yb, carrier_sec(yb, 0.93), z0=z0)]
        o = pz(poly_loft(rings), 0.45, 0.12, seed=30 + i, fraction=0.9)
        P.add(o, m["armour"], bevel=0.08, segments=2, angle=28.0)
        hull_parts.append(o)
    core = poly_loft([ring_y(-14.5, carrier_sec(-14.5, 0.86), z0=z0), ring_y(21.3, carrier_sec(21.3, 0.86), z0=z0)])
    P.add(core, m["dark"], bevel=0.06, segments=1, angle=30.0)
    P.hull_from(hull_parts)
    # stag-beetle mandibles off the head
    for sx in (1, -1):
        md = limb([(sx * 3.6, -16.5, -2.2), (sx * 5.6, -20.0, -2.6), (sx * 4.6, -22.0, -2.9), (sx * 2.6, -22.6, -3.0)],
                  [2.6, 2.0, 1.2, 0.1], [1.9, 1.4, 0.9, 0.1], kind="hex", tip=True)
        P.add(md, m["plate"], bevel=0.06, segments=1, angle=26.0)
    # cranked arms ending in bay collars (bay inner face at |x| = 19.5)
    inner = BAY_DIST - 1.5
    for sx in (1, -1):
        arm = limb([(sx * 4.5, 2.6, -0.6), (sx * 10.5, -1.8, 0.9), (sx * 16.0, -0.9, 0.3), (sx * (inner - 0.6), 0.0, 0.0)],
                   [7.0, 4.6, 4.8, 5.6], [3.8, 2.6, 3.0, 4.2], kind="hex")
        panelize(arm, 0.35, 0.1, seed=40 + sx, fraction=0.9)
        P.add(arm, m["armour"], bevel=0.06, segments=2, angle=28.0)
        collar = limb([(sx * (inner - 1.4), 0.0, 0.0), (sx * (inner - 0.6), 0.0, 0.0), (sx * (inner + 0.2), 0.0, 0.0)],
                      [7.6, 8.0, 7.4], [5.6, 6.0, 5.4], kind="hex")
        P.add(collar, m["plate"], bevel=0.07, segments=2, angle=28.0)
        spike = limb([(sx * 9.6, -1.0, 1.6), (sx * 11.2, -4.0, 3.2), (sx * 12.0, -7.2, 4.4)], [1.8, 1.1, 0.1],
                     [1.4, 0.8, 0.1], kind="hex", tip=True)
        P.add(spike, m["plate"], bevel=0.05, segments=1, angle=26.0)
        P.hull_from([arm, collar])
    # raked dorsal sail carrying the dorsal bay (top face tilted to the bay's mounting direction)
    dpos, ddir = bays[2]
    top_c = dpos - ddir * 1.5
    t_axis = Vector((0.0, ddir.z, -ddir.y)).normalized()

    def sail_pts(lx, ly):
        return [(0.0, -ly * 0.5), (lx * 0.5, -ly * 0.12), (lx * 0.5, ly * 0.32), (lx * 0.3, ly * 0.5),
                (-lx * 0.3, ly * 0.5), (-lx * 0.5, ly * 0.32), (-lx * 0.5, -ly * 0.12)]
    sail_rings = []
    for zc, ly, lx, yc in ((4.6, 17.0, 5.5, 3.0), (9.5, 12.0, 2.8, 4.6), (14.2, 8.4, 4.0, 5.4)):
        sail_rings.append([V(px, yc + py, zc) for px, py in sail_pts(lx, ly)])
    sail_rings.append([top_c + 0.25 * ddir + Vector((px, 0.0, 0.0)) + t_axis * py for px, py in sail_pts(7.2, 5.6)])
    sail = pz(poly_loft(sail_rings), 0.4, 0.12, seed=45, fraction=0.95)
    P.add(sail, m["armour"], bevel=0.06, segments=2, angle=28.0)
    P.hull_from([sail])
    # ribbed sail: three rib plates on each flank and a bevelled spine on the leading edge
    for sx in (1, -1):
        for zc, y0, y1, hw in ((6.4, 1.5, 9.4, 2.2), (9.0, 2.6, 8.4, 1.5), (11.6, 3.5, 7.9, 1.62)):
            rib = limb([(sx * (hw + 0.05), y0, zc), (sx * (hw + 0.05), y1, zc)], [0.55, 0.55], [0.7, 0.7],
                       up=(sx, 0, 0), kind="hex")
            P.add(rib, m["plate"], bevel=0.05, segments=1, angle=30.0)
    spine = limb([(0, -5.6, 4.4), (0, -1.6, 9.5), (0, 0.9, 14.3)], [0.9, 0.8, 0.7], [0.9, 0.8, 0.7],
                 up=(1, 0, 0), kind="hex")
    P.add(spine, m["plate"], bevel=0.05, segments=1, angle=30.0)
    # trailing-edge rib spines raking back off the sail
    for zc, y_rear in ((6.2, 10.5), (9.0, 9.4), (11.8, 8.6)):
        rs = limb([(0, y_rear - 1.2, zc), (0, y_rear + 1.0, zc + 0.9), (0, y_rear + 2.6, zc + 1.9)], [0.7, 0.5, 0.05],
                  [1.1, 0.8, 0.05], up=(1, 0, 0), kind="hex", tip=True)
        P.add(rs, m["plate"], bevel=0.04, segments=1, angle=28.0)
    # horn pylons rising from the hull shoulders to the turret pads
    for tpos, tdir in turrets:
        pad_c = tpos - Vector((0, 0, 0.5))
        sx = 1 if tpos.x > 0 else -1
        horn = limb([(sx * 3.6, tpos.y, 2.9), (sx * 8.6, tpos.y, 6.4), (pad_c.x, pad_c.y, pad_c.z - 0.8)],
                    [2.6, 1.9, 1.6], [4.2, 2.9, 2.3], up=(0, 1, 0), kind="hex")
        P.add(horn, m["plate"], bevel=0.06, segments=1, angle=28.0)
        pad = L.cyl(P.coll, 1.05, 1.0, segments=6, axis="Z", center=(pad_c.x, pad_c.y, pad_c.z - 0.45), r_top=0.95)
        P.add(pad, m["armour"], bevel=0.05, segments=1)
    for sx in (1, -1):
        for x, z, r in ((2.4, 1.4, 1.3), (2.4, -1.3, 1.1)):
            nozzle(P, m, (sx * x, 21.1, z), r, 1.25)
    nozzle(P, m, (0.0, 20.9, 3.0), 0.9, 1.1)
    surf = Surf(P.parts)
    # the hive of eyes: six narrow white slits in two rows of three, sunk in black, under the brow
    eyes = []
    for (x, z, ln) in ((-2.3, 0.9, 1.3), (0.0, 1.1, 1.4), (2.3, 0.9, 1.3), (-1.5, -0.4, 1.1), (1.5, -0.4, 1.1),
                       (0.0, -0.25, 0.9)):
        loc, nrm = surf.hit((x, -30.0, z), (0, 1, 0))
        if loc is not None:
            slit(P, m, loc + nrm * 0.02, nrm, ln, 0.3, 0.45, along=(1, 0, -0.25 * (1 if x > 0 else -1 if x < 0 else 0)))
            eyes.append([round(c, 2) for c in loc])
    mounts = {"bays_blender_m": [[round(c, 3) for c in p] for p, d in bays],
              "turrets_blender_m": [[round(c, 3) for c in p] for p, d in turrets]}
    if stage == "blockout":
        return P, [], mounts
    # glowing seams at the shell rims (where each shell overlaps the last) and chine pinstripes
    for ya, yb in shells:
        sec = carrier_sec(ya + 0.15, 1.0)
        pts = [V(px, ya + 0.15, z0 + pz) for px, pz in sec] + [V(sec[0][0], ya + 0.15, z0 + sec[0][1])]
        glow_line(P, m, pts, 0.05)
    for sx in (1, -1):
        for ya, yb in shells:
            a = ya + 2.0
            pa = V(sx * (carrier_w(a) * 1.05 * 0.5 + 0.03), a, z0 + 0.05 * carrier_w(a) * 1.05 * 0.42)
            pb = V(sx * (carrier_w(yb) * 0.93 * 0.5 + 0.03), yb - 0.3, z0 + 0.05 * carrier_w(yb) * 0.93 * 0.42)
            glow_line(P, m, [pa, pb], 0.045)
        # bay collar rim
        x = inner + 0.25
        rim = [V(sx * x, p.y, p.z) for p in [V(0, 0, 2.95), V(0, 3.95, 1.3), V(0, 3.95, -1.3), V(0, 0, -2.95),
                                               V(0, -3.95, -1.3), V(0, -3.95, 1.3), V(0, 0, 2.95)]]
        glow_line(P, m, rim, 0.12)
        glow_line(P, m, [V(sx * 5.4, 0.2, 1.3), V(sx * 10.5, -3.0, 2.15), V(sx * 15.8, -2.6, 1.85)], 0.08)
    for tpos, tdir in turrets:
        c = tpos - Vector((0, 0, 0.5))
        ring_pts = [V(c.x + 1.0 * math.cos(a), c.y + 1.0 * math.sin(a), c.z - 0.06)
                    for a in [math.tau * k / 6 for k in range(7)]]
        glow_line(P, m, ring_pts, 0.07)
    # sail: one strong seam up the leading-edge spine (r2 review: no outline round the cap)
    glow_line(P, m, [V(0, -5.95, 4.6), V(0, -1.95, 9.6), V(0, 0.55, 14.3)], 0.09)
    # crimson plates on the thorax flanks
    for sx in (1, -1):
        cp = poly_loft([[V(sx * 6.4, -12.0, 0.6), V(sx * 6.6, -5.0, 0.6), V(sx * 6.0, -5.0, 3.0), V(sx * 5.8, -11.5, 2.8)],
                        [V(sx * 6.9, -12.0, 0.6), V(sx * 7.1, -5.0, 0.6), V(sx * 6.5, -5.0, 3.0), V(sx * 6.3, -11.5, 2.8)]])
        P.add(cp, m["crimson"], bevel=0.04, segments=1)
    surf = Surf(P.parts)
    for k, (c, n) in enumerate((((0.0, -7.0, 6.0), (0, 0, 1)), ((0.0, 10.5, 5.2), (0, 0, 1)), ((0.0, 17.5, 4.5), (0, 0, 1)),
                                ((5.0, 16.0, 1.5), (1, 0, 0.3)), ((-5.0, 16.0, 1.5), (-1, 0, 0.3)),
                                ((0.0, 0.0, -5.0), (0, 0, -1)), ((0.0, 12.0, -4.4), (0, 0, -1)),
                                ((10.5, -1.8, -1.0), (0, 0, -1)), ((-10.5, -1.8, -1.0), (0, 0, -1)),
                                ((0.0, -10.0, 5.6), (0, 0, 1)))):
        greeble_cluster(P, m["metal"], surf, c, n, 12, (0.4, 1.1), (0.15, 0.45), 2.6, seed=20 + k, along=(0, 1, 0))
    engines = [("Engine_%d" % i, (sx * 2.4, 22.4, z)) for i, (sx, z) in enumerate(((1, 1.4), (-1, 1.4), (1, -1.3), (-1, -1.3)))]
    return P, engines, dict(role="carrier", ue_radius_cm=1800, eyes_m=eyes, **mounts)


# =========================================================================== carrier parts

def build_bay(asset, tag, stage):
    """Carrier launch bay (Unreal box 300 deep (+X outward) x 600 wide x 400 tall, centred). In Blender: door
    faces -Y, 6 m along X, 3 m deep along Y, 4 m tall. The weak point: big glowing launch slot and door seams."""
    P = NFPart(asset, COLL)
    sc = 0.4
    m = mats("NF_Bay", sc, 0.8)
    # tapered pod: back (mating) face 6 x 4 at y=+1.5, door face 5.2 x 3.3 at y=-1.5
    def rect(w, h, ch):
        return [(-w / 2 + ch, -h / 2), (w / 2 - ch, -h / 2), (w / 2, -h / 2 + ch), (w / 2, h / 2 - ch),
                (w / 2 - ch, h / 2), (-w / 2 + ch, h / 2), (-w / 2, h / 2 - ch), (-w / 2, -h / 2 + ch)]
    # armoured housing: back (mating) face 5.9 x 3.9 at y=+1.5, tapering to the front face at y=-0.85
    # r2 review: wedge-shaped pod, widest behind the door lip, tapering into the carrier collar
    body = poly_loft([ring_y(1.5, rect(4.6, 2.9, 0.6)), ring_y(0.4, rect(6.0, 4.0, 0.7)),
                      ring_y(-0.85, rect(5.6, 3.7, 0.6))])
    panelize(body, 0.12, 0.04, seed=50, fraction=0.9)
    P.add(body, m["armour"], bevel=0.025, segments=2, angle=28.0)
    P.hull_from([body])
    # heavy lip frame (y -0.75 .. -1.55) around a 4.6 x 2.9 opening: the doors sit ~40 cm behind its front
    for size, c in (((5.6, 0.8, 0.42), (0, -1.15, 1.66)), ((5.6, 0.8, 0.42), (0, -1.15, -1.66)),
                    ((0.5, 0.8, 3.0), (2.55, -1.15, 0.0)), ((0.5, 0.8, 3.0), (-2.55, -1.15, 0.0))):
        f = box(size, c)
        P.add(f, m["plate"], bevel=0.04, segments=2)
    # glowing launch slot and two angled door leaves (top / bottom), recessed
    slot = box((3.6, 0.3, 0.7), (0, -0.92, 0.0))
    P.add(slot, m["glow"], bevel=0.02, segments=1)
    for sz in (1, -1):
        d = poly_loft([[V(-2.3, -0.88, sz * 0.2), V(2.3, -0.88, sz * 0.2), V(2.3, -0.88, sz * 1.46), V(-2.3, -0.88, sz * 1.46)],
                       [V(-2.3, -1.12, sz * 0.24), V(2.3, -1.12, sz * 0.24), V(2.3, -1.02, sz * 1.42), V(-2.3, -1.02, sz * 1.42)]])
        P.add(d, m["armour"], bevel=0.025, segments=1)
    # clamp arms on the back corners that bite into the carrier's collar
    for sx in (1, -1):
        for sz in (1, -1):
            c = limb([(sx * 2.7, 0.6, sz * 1.7), (sx * 3.05, 1.5, sz * 1.95), (sx * 2.85, 2.1, sz * 1.75)],
                     [0.45, 0.4, 0.05], [0.5, 0.42, 0.05], kind="hex", tip=True)
            P.add(c, m["metal"], bevel=0.02, segments=1)
    # chitin ribs arching over the back and under the belly
    for y in (-0.2, 0.55, 1.25):
        k = 1.0 - max(0.0, y - 0.4) * 0.22
        for sz in (1, -1):
            rib = limb([(-2.9 * k, y, sz * 1.55 * k), (0.0, y, sz * 2.12 * k), (2.9 * k, y, sz * 1.55 * k)],
                       [0.34, 0.38, 0.34], [0.3, 0.34, 0.3], up=(0, 1, 0), kind="hex")
            P.add(rib, m["plate"], bevel=0.03, segments=1, angle=28.0)
    if stage != "blockout":
        fr = [V(-2.3, -1.2, 0.0), V(-2.3, -1.2, 1.45), V(2.3, -1.2, 1.45), V(2.3, -1.2, -1.45),
              V(-2.3, -1.2, -1.45), V(-2.3, -1.2, 0.0)]
        glow_line(P, m, fr, 0.05)
        for sz in (1, -1):
            glow_line(P, m, [V(-1.6, -1.06, sz * 0.9), V(0.0, -1.1, sz * 0.62), V(1.6, -1.06, sz * 0.9)], 0.04)
        surf = Surf(P.parts)
        loc, nrm = surf.hit((0, 0.4, 4.0), (0, 0, -1))
        lens(P, m, loc, nrm, 0.2)
        # b5 (Nov 13 evening, ship builder): seen from the side or behind, the bay read as a flat black box in the
        # game (Lead / Ryan). Lit seams ride the housing so it reads as Null armour from every side: two runs along each
        # flank, a chevron on the back, a seam over the top and under the belly, all ray-cast onto the skin.
        def on_skin(origin, direction, lift=0.012):
            hl, hn = surf.hit(origin, direction)
            return None if hl is None else hl + hn * lift
        for sx in (1, -1):
            for zz in (0.95, -0.95):
                run = [on_skin((sx * 5.0, y, zz), (-sx, 0, 0)) for y in (-0.6, -0.1, 0.4, 0.9, 1.3)]
                run = [q for q in run if q is not None]
                if len(run) >= 2:
                    glow_line(P, m, run, 0.035)
        for sz in (1, -1):
            run = [on_skin((x, 0.4, sz * 5.0), (0, 0, -sz)) for x in (-2.2, -1.1, 0.0, 1.1, 2.2)]
            run = [q for q in run if q is not None]
            if len(run) >= 2:
                glow_line(P, m, run, 0.035)
        back = [on_skin((x, 5.0, z), (0, -1, 0)) for x, z in ((-1.7, 0.9), (0.0, 0.25), (1.7, 0.9))]
        back = [q for q in back if q is not None]
        if len(back) >= 2:
            glow_line(P, m, back, 0.04)
        greeble_cluster(P, m["metal"], surf, (0, 0.6, 2.0), (0, 0, 1), 6, (0.12, 0.3), (0.05, 0.12), 0.9, seed=2,
                        along=(1, 0, 0))
        greeble_cluster(P, m["metal"], surf, (0, 0.6, -2.0), (0, 0, -1), 5, (0.12, 0.3), (0.05, 0.12), 0.9, seed=3,
                        along=(1, 0, 0))
    return P, [("Launch", (0.0, -1.6, 0.0))], {"role": "carrier_bay", "ue_box_cm": [300, 600, 400]}


def build_turret(asset, tag, stage):
    """Point-defence turret (Unreal: cylinder 150 cm across, 100 cm tall, centred, +Z up, +X barrels).
    In Blender: barrels toward -Y. Hex armoured ring base, squat faceted head, twin barrels, a white eye."""
    P = NFPart(asset, COLL)
    sc = 0.2
    m = mats("NF_Turret", sc, 0.4)
    base = L.cyl(P.coll, 0.75, 0.42, segments=6, axis="Z", center=(0, 0, -0.29), r_top=0.68)
    P.add(base, m["armour"], bevel=0.02, segments=2)
    # bulked mantlet with a sloped glacis
    head = poly_loft([ring_y(-0.72, armour_sec(0.8, 0.12, 0.16, chine=0.0), z0=0.1),
                      ring_y(-0.35, armour_sec(1.28, 0.4, 0.34), z0=0.12),
                      ring_y(0.45, armour_sec(1.32, 0.4, 0.34), z0=0.1),
                      ring_y(0.66, armour_sec(1.0, 0.26, 0.24), z0=0.1)])
    panelize(head, 0.05, 0.025, seed=60, fraction=0.8)
    P.add(head, m["plate"], bevel=0.015, segments=2, angle=28.0)
    P.hull_from([base, head])
    muzzles = []
    for i, sx in enumerate((1, -1)):
        b = lathe_y([(-1.45, 0.0), (-1.45, 0.05), (-1.3, 0.05), (-1.25, 0.065), (-0.45, 0.06), (-0.3, 0.09),
                     (0.0, 0.09), (0.0, 0.0)], segments=8, center=(sx * 0.22, 0, 0.1))
        P.add(b, m["metal"], smooth=40.0)
        muzzles.append(("Muzzle_%d" % i, (sx * 0.22, -1.47, 0.1)))
    if stage != "blockout":
        glow_line(P, m, [V(0.7 * math.cos(a), 0.7 * math.sin(a), -0.1) for a in
                         [math.tau * k / 6 for k in range(7)]], 0.014)
        surf = Surf(P.parts)
        loc, nrm = surf.hit((0, -2.0, 0.32), (0, 1, 0))
        lens(P, m, loc, nrm, 0.13)
        brow = limb([(-0.4, -0.5, 0.47), (0.0, -0.62, 0.55), (0.4, -0.5, 0.47)], [0.14, 0.16, 0.14], [0.1, 0.12, 0.1],
                    up=(0, -0.3, 1), kind="hex")
        P.add(brow, m["armour"], bevel=0.01, segments=1, angle=28.0)
        for sx in (1, -1):
            glow_line(P, m, [V(sx * 0.62, -0.3, 0.12), V(sx * 0.64, 0.42, 0.11)], 0.014)
            tip = lathe_y([(-1.47, 0.055), (-1.4, 0.075), (-1.33, 0.075), (-1.33, 0.055), (-1.47, 0.055)], segments=10,
                          center=(sx * 0.22, 0, 0.1))
            P.add(tip, m["engine"], smooth=50.0)
    return P, muzzles, {"role": "carrier_turret", "ue_cylinder_cm": [150, 100]}


BUILDERS = {
    "SM_Null_Fighter": (build_fighter, 2048, (4000, 10000), 0.6),
    "SM_Null_Interceptor": (build_interceptor, 2048, (6000, 14000), 0.7),
    "SM_Null_Gunship": (build_gunship, 2048, (12000, 25000), 2.0),
    "SM_Null_Bomber": (build_bomber, 2048, (10000, 22000), 1.6),
    "SM_Null_Carrier": (build_carrier, 4096, (25000, 60000), 5.0),
    "SM_Null_CarrierBay": (build_bay, 1024, (500, 6000), 0.8),
    "SM_Null_Turret": (build_turret, 1024, (500, 5000), 0.4),
}
# Pivot at the bounding-box centre where the centre of mass would push the nose past 1.25 x the collision radius
BBOX_ORIGIN = {"SM_Null_Fighter", "SM_Null_Interceptor", "SM_Null_Bomber"}
# e1: pivots pinned to the values the r3-r5 builds computed (bbox centre for fighter / interceptor / bomber, centre
# of mass for the gunship), so adding the engine plumes moves no socket and no placement.
PINNED_PIVOT = {"SM_Null_Fighter": (0.0, -0.13469207286834717, 0.1601601541042328),
                "SM_Null_Interceptor": (0.0, -0.009296536445617676, -0.1691438853740692),
                "SM_Null_Bomber": (0.0, 0.14136028289794922, -0.16573476791381836),
                "SM_Null_Gunship": (-0.0008077544043771923, -1.1682953834533691, 0.43138551712036133)}
KEEP_ORIGIN = {"SM_Null_Carrier", "SM_Null_CarrierBay", "SM_Null_Turret"}   # mount points depend on the origin

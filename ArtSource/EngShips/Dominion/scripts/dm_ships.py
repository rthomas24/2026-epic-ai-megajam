# SPDX-License-Identifier: MIT
"""
dm_ships.py - the Dominion mothership exterior kit (Triad Drift), Blender 5.2 headless.

Every part is authored in UNREAL coordinates in metres (X forward = bow, Y right, Z up, actor-relative for the hull,
piece-relative for the others) and converted to Blender's authoring frame (front = -Y) with UE_TO_BL, the inverse of
what export_for_unreal.py does on export (Unreal (x, y, z) cm = (-y, -x, z) * 100 of Blender metres).

Builders return a dm_lib.Part (parts + UCX point lists). No textures are baked: the materials are world-aligned in
Unreal (M_TD_DominionPanel triplanar) and flat emissive (M_TD_DominionGlow); slots are named M_Dominion_<Role>.
"""

import math
import os
import random

import bmesh
import bpy
from mathutils import Matrix, Vector

import dm_lib as L

UE_TO_BL = Matrix(((0.0, -1.0, 0.0, 0.0), (-1.0, 0.0, 0.0, 0.0), (0.0, 0.0, 1.0, 0.0), (0.0, 0.0, 0.0, 1.0)))
TEX_DIR = os.path.join(L.KIT, "textures")

# --------------------------------------------------------------------------------------------- code contract (m)
E = Vector((300.0, 80.0, 50.0))
TRENCH_Y = 25.0                     # wall centre |Y|
TRENCH_X = (-180.0, 240.0)          # wall extent (centre 30, length 420)
BRIDGE_C = Vector((-195.0, 0.0, 80.0))
BRIDGE_HALF = Vector((37.5, 20.0, 30.0))
WALL_C = Vector((30.0, -25.0, 65.0))
WALL_HALF = Vector((210.0, 2.0, 15.0))
EMITTERS = [Vector((165.0, -64.0, 70.0)), Vector((165.0, 64.0, 70.0)), Vector((-105.0, -64.0, 70.0)),
            Vector((-105.0, 64.0, 70.0))]
REACTOR = (Vector((165.0, 0.0, 65.0)), 12.5)
HANGAR = Vector((330.0, 0.0, -20.0))
TURRETS = [Vector((-180.0, -80.0, 55.0)), Vector((180.0, 80.0, 55.0))]


# --------------------------------------------------------------------------------------------- materials (preview)

def _panel_mat(name, tile, dark, light, seam, rough, metal, light_strength=0.0):
    s = L.Shader(name)
    nt = s.nt
    img_path = os.path.join(TEX_DIR, "T_Dominion_PanelMask.png")
    img = bpy.data.images.get("T_Dominion_PanelMask.png") or bpy.data.images.load(img_path, check_existing=True)
    img.colorspace_settings.name = "Non-Color"
    mp = s.mapping(s.obj, scale=(1.0 / tile,) * 3)
    tx = nt.nodes.new("ShaderNodeTexImage")
    tx.image = img
    tx.projection = "BOX"
    tx.projection_blend = 0.25
    nt.links.new(mp, tx.inputs["Vector"])
    mp2 = s.mapping(s.obj, loc=(0.37, 0.61, 0.2), scale=(1.0 / (tile * 8.0),) * 3)
    tx2 = nt.nodes.new("ShaderNodeTexImage")
    tx2.image = img
    tx2.projection = "BOX"
    tx2.projection_blend = 0.25
    nt.links.new(mp2, tx2.inputs["Vector"])
    sep = nt.nodes.new("ShaderNodeSeparateColor")
    nt.links.new(tx.outputs["Color"], sep.inputs[0])
    sep2 = nt.nodes.new("ShaderNodeSeparateColor")
    nt.links.new(tx2.outputs["Color"], sep2.inputs[0])
    R, G, B = sep.outputs[0], sep.outputs[1], sep.outputs[2]
    macro_seam = s.m("ADD", s.m("MULTIPLY", sep2.outputs[1], 0.6), 0.4)
    cav = s.m("MULTIPLY", G, macro_seam, clamp=True)
    tone = s.m("ADD", s.m("MULTIPLY", R, 0.75), s.m("MULTIPLY", s.m("SUBTRACT", sep2.outputs[0], 0.5), 0.72), clamp=True)
    tone = s.m("ADD", tone, 0.1, clamp=True)
    col = s.mix(tone, dark, light)
    col = s.mix(cav, seam, col)
    rgh = s.m("ADD", rough, s.m("MULTIPLY", s.m("SUBTRACT", 1.0, cav), 0.25))
    em = None
    if light_strength > 0:
        em = s.mix(tx.outputs["Alpha"], (0.0, 0.0, 0.0), (1.0, 0.14, 0.02))
    s.finish(col, rgh, metal, height=B, bump_strength=0.6, bump_distance=0.04, emission=em,
             emission_strength=light_strength)
    return s.mat


def _glow_mat(name, rgb, strength=3.0):
    s = L.Shader(name)
    s.finish(tuple(c * 0.05 for c in rgb), 0.3, 0.0, emission=rgb, emission_strength=strength)
    s.mat.diffuse_color = (*rgb, 1.0)
    return s.mat


def materials():
    # Unreal linear values (see ue_import_dominion.py); Blender preview shows the same numbers
    return {
        "Panel": _panel_mat("M_Dominion_Panel", 32.0, (0.011, 0.012, 0.014), (0.034, 0.036, 0.042),
                            (0.002, 0.002, 0.003), 0.42, 0.55, light_strength=1.3),
        "Armor": _panel_mat("M_Dominion_Armor", 64.0, (0.026, 0.027, 0.031), (0.07, 0.072, 0.08),
                            (0.002, 0.002, 0.003), 0.4, 0.5),
        "Trim": _panel_mat("M_Dominion_Trim", 12.0, (0.006, 0.006, 0.007), (0.016, 0.016, 0.018),
                           (0.002, 0.002, 0.002), 0.52, 0.6),
        "GlowRed": _glow_mat("M_Dominion_GlowRed", (1.0, 0.14, 0.02), 1.3),
        "GlowWhite": _glow_mat("M_Dominion_GlowWhite", (0.9, 0.95, 1.0), 1.6),
        "GlowEngine": _glow_mat("M_Dominion_GlowEngine", (1.0, 0.3, 0.05), 1.6),
    }


# --------------------------------------------------------------------------------------------- UE-space geometry

def to_bl(bm, coll, place=None):
    """bmesh in Unreal metres -> Blender object (verts mapped by UE_TO_BL, normals recalculated)."""
    m = UE_TO_BL @ (place if place is not None else Matrix.Identity(4))
    bmesh.ops.transform(bm, matrix=m, verts=list(bm.verts))
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    return L.bm_to_object(L.uname("dm"), bm, coll)


def bl_pts(pts):
    return [UE_TO_BL @ Vector(p) for p in pts]


def loft_bm(sections, cap=True):
    """sections: [(x, [(y, z), ...]), ...] closed polygons with the same point count -> closed solid."""
    bm = bmesh.new()
    rings = [[bm.verts.new((x, y, z)) for (y, z) in poly] for x, poly in sections]
    n = len(rings[0])
    for ra, rb in zip(rings, rings[1:]):
        for j in range(n):
            bm.faces.new((ra[j], ra[(j + 1) % n], rb[(j + 1) % n], rb[j]))
    if cap:
        bm.faces.new(rings[0])
        bm.faces.new(list(reversed(rings[-1])))
    return bm


def loft3_bm(rings_pts):
    """rings of full 3D points (same count) -> closed solid."""
    bm = bmesh.new()
    rings = [[bm.verts.new(Vector(p)) for p in ring] for ring in rings_pts]
    n = len(rings[0])
    for ra, rb in zip(rings, rings[1:]):
        for j in range(n):
            bm.faces.new((ra[j], ra[(j + 1) % n], rb[(j + 1) % n], rb[j]))
    bm.faces.new(rings[0])
    bm.faces.new(list(reversed(rings[-1])))
    return bm


def box_bm(center, size):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0, matrix=Matrix.Translation(Vector(center)) @ Matrix.Diagonal((*size, 1.0)))
    return bm


def slab_bm(quad, normal, t_out, t_in):
    n = Vector(normal).normalized()
    front = [Vector(p) + n * t_out for p in quad]
    back = [Vector(p) - n * t_in for p in quad]
    return loft3_bm([back, front])


def wedge_bm(quad, normal, t_a, t_b, t_in):
    """slab whose thickness goes from t_a (quad verts 0, 3) to t_b (verts 1, 2): a shingle."""
    n = Vector(normal).normalized()
    t = (t_a, t_b, t_b, t_a)
    front = [Vector(p) + n * ti for p, ti in zip(quad, t)]
    back = [Vector(p) - n * t_in for p in quad]
    return loft3_bm([back, front])


def cyl_bm(r, depth, segs=16, axis="X", center=(0, 0, 0), r_top=None):
    bm = bmesh.new()
    rot = {"X": Matrix.Rotation(math.radians(90), 4, "Y"), "Y": Matrix.Rotation(math.radians(-90), 4, "X"),
           "Z": Matrix.Identity(4)}[axis]
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=segs, radius1=r,
                          radius2=r if r_top is None else r_top, depth=depth,
                          matrix=Matrix.Translation(Vector(center)) @ rot)
    return bm


def lathe_x_bm(profile, segs, center):
    """Revolve [(dx, r), ...] around the UE X axis through center; profile ends on the axis (r = 0) are closed."""
    bm = bmesh.new()
    c = Vector(center)
    rings = []
    for i in range(segs):
        a = math.tau * i / segs
        ring = []
        for dx, r in profile:
            ring.append(bm.verts.new(c + Vector((dx, r * math.cos(a), r * math.sin(a)))))
        rings.append(ring)
    n = len(profile)
    for i in range(segs):
        ra, rb = rings[i], rings[(i + 1) % segs]
        for j in range(n - 1):
            q = [ra[j], ra[j + 1], rb[j + 1], rb[j]]
            u = []
            for v in q:
                if v not in u:
                    u.append(v)
            if len(u) >= 3:
                try:
                    bm.faces.new(u)
                except ValueError:
                    pass
    bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=1e-5)
    return bm


def wedge_x_bm(x0, x1, sec0, sec1):
    return loft_bm([(x0, sec0), (x1, sec1)])


class Kit:
    def __init__(self, asset, mats, post=None):
        self.post = post
        self.P = L.Part(asset, asset + "_parts")
        self.mats = mats
        self.coll = self.P.coll
        self.ucx = []

    def add(self, bm, role, bevel=None, seg=2, angle=30.0, place=None, hull=False, smooth=None):
        if self.post is not None:
            place = self.post @ (place if place is not None else Matrix.Identity(4))
        obj = to_bl(bm, self.coll, place)
        self.P.add(obj, self.mats[role], bevel=bevel, segments=seg, angle=angle, smooth=smooth, wn=True)
        if hull:
            self.ucx.append([obj.matrix_world @ v.co for v in obj.data.vertices])
        return obj

    def hull_pts(self, ue_pts):
        if self.post is not None:
            ue_pts = [self.post @ Vector(p) for p in ue_pts]
        self.ucx.append(bl_pts(ue_pts))


# --------------------------------------------------------------------------------------------- hull

def sec(wt, zt, wm, z2, z1, wl, zl, wk, zb):
    return [(wt, zt), (wm, z2), (wm, z1), (wl, zl), (wk, zb), (-wk, zb), (-wl, zl), (-wm, z1), (-wm, z2), (-wt, zt)]


STATIONS = [
    (-298.0, sec(70, 46, 72, 40, -8, 48, -28, 18, -32)),
    (-240.0, sec(72, 50, 73, 44, -6, 48, -29, 18, -33)),
    (-180.0, sec(72, 50, 73, 44, -6, 49, -31, 18, -35)),
    (-120.0, sec(72, 50, 73, 44, -6, 49, -33, 18, -37)),
    (-30.0, sec(58, 50, 60, 44, -5, 42, -36, 18, -40)),
    (70.0, sec(58, 50, 60, 44, -4, 44, -42, 19, -46)),
    (140.0, sec(74, 50, 76, 44, -4, 52, -44, 21, -48)),
    (190.0, sec(76, 50, 78, 44, -4, 54, -46, 21, -50)),
    (215.0, sec(72, 50, 76, 44, -2, 50, -44, 18, -52)),
    (262.0, sec(60, 46, 64, 40, 0, 40, -40, 14, -50)),
    (296.0, sec(46, 38, 48, 32, 2, 28, -34, 10, -42)),
]
BENDS = [-298.0, -282.0, -240.0, 215.0, 262.0, 296.0]


CITADEL = [
    (186.0, sec(74, 49, 77, 42, -4, 53, -43, 20, -49)),
    (224.0, sec(110, 50.8, 130, 40, -16, 92, -60, 30, -72)),
    (268.0, sec(104, 50.8, 126, 38, -16, 86, -60, 28, -74)),
    (300.0, sec(80, 44, 100, 34, -12, 62, -52, 20, -62)),
]
# engine block: the stern flares wider and taller than the waist (arrowhead plan, dagger profile)
ENGINE_BLOCK = [
    (-250.0, sec(70, 49, 72, 43, -6, 47, -28, 17, -32)),
    (-266.0, sec(88, 56, 98, 48, -18, 68, -38, 26, -44)),
    (-300.0, sec(92, 56, 102, 48, -20, 72, -40, 28, -46)),
]


def core_point(x, k, st=None):
    """Point k of the core section at station x (linear between stations)."""
    st = st or STATIONS
    for (xa, sa), (xb, sb) in zip(st, st[1:]):
        if xa - 1e-6 <= x <= xb + 1e-6:
            t = (x - xa) / (xb - xa)
            ya, za = sa[k]
            yb, zb = sb[k]
            return Vector((x, ya + (yb - ya) * t, za + (zb - za) * t))
    raise ValueError(x)


def strip_point(x, k0, k1, s, st=None):
    a, b = core_point(x, k0, st), core_point(x, k1, st)
    return a + (b - a) * s


def plate_layout(x0, x1, rng, lo, hi, gap):
    xs = [x0]
    while xs[-1] < x1 - 1e-6:
        step = rng.uniform(lo, hi)
        if x1 - (xs[-1] + step) < lo * 0.6:
            step = x1 - xs[-1]
        xs.append(min(x1, xs[-1] + step))
    return [(a + gap * 0.5, b - gap * 0.5) for a, b in zip(xs, xs[1:])]


def build_hull(asset, mats, stage):
    K = Kit(asset, mats)
    rng = random.Random(11)
    # ---- primary: core
    core = loft_bm(STATIONS)
    K.add(core, "Trim", bevel=0.6, seg=2, angle=25.0)
    # UCX: three convex chunks of the core
    for chunk in [(i, i + 1) for i in range(len(STATIONS) - 1)]:
        pts = []
        for i in chunk:
            x, poly = STATIONS[i]
            pts += [(x, y, z) for (y, z) in poly]
        K.hull_pts(pts)

    # ---- prow: forward-raked chevron wedge over the hangar
    def prow_sec(hw, ztop, zbot, taper):
        return [(hw - taper, ztop), (hw, ztop - 4.0), (hw * 0.92, zbot), (-hw * 0.92, zbot), (-hw, ztop - 4.0),
                (-hw + taper, ztop)]
    prow = [(244.0, prow_sec(64.0, 54.0, 24.0, 8.0)), (272.0, prow_sec(60.0, 66.0, 20.0, 10.0)),
            (304.0, prow_sec(46.0, 62.0, 17.0, 8.0)), (326.0, prow_sec(18.0, 50.0, 20.0, 3.0)),
            (340.0, prow_sec(2.5, 42.0, 28.0, 0.5))]
    # bow citadel: the heavy head (wider and deeper than the hull)
    K.add(loft_bm(CITADEL), "Trim", bevel=0.6, seg=2, angle=25.0)
    for i in range(len(CITADEL) - 1):
        K.hull_pts([(x, y, z) for x, poly in CITADEL[i:i + 2] for (y, z) in poly])
    # shoulder blocks: raked armour masses outboard of the trench end (|y| >= 30), the head rises above the deck
    for sgn in (-1, 1):
        rings = []
        for x, yi, yo, zt in ((196.0, 31.0, 80.0, 52.0), (234.0, 30.0, 120.0, 80.0), (286.0, 34.0, 118.0, 86.0),
                              (308.0, 42.0, 92.0, 62.0)):
            ring = [(yi, 50.0), (yo, 50.0), (yo - 5.0, zt - 4.0), (yo - 12.0, zt), (yi + 5.0, zt), (yi, zt - 3.0)]
            rings.append([(x, y * sgn, z) for (y, z) in ring])
        K.add(loft3_bm(rings), "Armor", bevel=0.6, seg=2, angle=25.0)
        for ra, rb in zip(rings, rings[1:]):
            K.hull_pts(ra + rb)
        # red edge outline along the outer top edge
        edge = [(r[3][0], r[3][1], r[3][2]) for r in rings]
        for e0, e1 in zip(edge, edge[1:]):
            a0, a1 = Vector(e0), Vector(e1)
            q = [a0 + Vector((0, 0, 0.5)), a1 + Vector((0, 0, 0.5)), a1 - Vector((0, 0, 0.6)), a0 - Vector((0, 0, 0.6))]
            K.add(slab_bm(q, Vector((0, sgn, 0.6)), 0.6, 0.6), "GlowRed")
    K.add(loft_bm(prow), "Panel", bevel=0.5, seg=2, hull=True)
    # prow crest blades (forward-raked spikes past the tip)
    for sgn in (-1, 1):
        y = 13.0 * sgn
        blade = loft3_bm([[(274.0, y - 1.2, 67.0), (274.0, y + 1.2, 67.0), (274.0, y + 1.2, 63.0), (274.0, y - 1.2, 63.0)],
                          [(322.0, y * 0.7 - 0.6, 53.5), (322.0, y * 0.7 + 0.6, 53.5), (322.0, y * 0.7 + 0.6, 49.0), (322.0, y * 0.7 - 0.6, 49.0)],
                          [(352.0, y * 0.35 - 0.1, 46.0), (352.0, y * 0.35 + 0.1, 46.0), (352.0, y * 0.35 + 0.1, 45.2), (352.0, y * 0.35 - 0.1, 45.2)]])
        K.add(blade, "Trim", bevel=0.15, seg=1)
    # ---- jaws (mandibles) either side of the hangar mouth: thick wedges curving in and up (crescent from the front)
    JAW = [(280.0, 92.0, -20.0, 14.0, 46.0), (332.0, 80.0, -30.0, 11.0, 32.0), (376.0, 62.0, -28.0, 6.5, 18.0),
           (410.0, 44.0, -20.0, 1.2, 3.0)]
    for sgn in (-1, 1):
        rings = []
        for x, y, z, t, h in JAW:
            yy = y * sgn
            rings.append([(x, yy - t * sgn, z + h * 0.5), (x, yy + t * sgn, z + h * 0.5), (x, yy + t * 0.6 * sgn, z - h * 0.5),
                          (x, yy - t * 0.6 * sgn, z - h * 0.5)])
        K.add(loft3_bm(rings), "Armor", bevel=0.5, seg=2)
        for ra, rb in zip(rings, rings[1:]):
            K.hull_pts(ra + rb)
        g, tops = [], []
        for x, y, z, t, h in JAW[:-1] + [(JAW[-1][0] - 6.0, JAW[-1][1] + 3.0, JAW[-1][2] - 1.5, 2.0, 5.0)]:
            yi = (y - 0.8 * t - 0.35) * sgn
            g.append([(x, yi - 0.6 * sgn, z + h * 0.03), (x, yi + 0.6 * sgn, z + h * 0.03), (x, yi + 0.6 * sgn, z - h * 0.03),
                      (x, yi - 0.6 * sgn, z - h * 0.03)])
            tops.append(Vector((x, y * sgn, z + h * 0.5 + 0.25)))
        K.add(loft3_bm(g), "GlowRed")
        for a0, a1 in zip(tops, tops[1:]):
            q = [a0 + Vector((0, 0, 0.6)), a1 + Vector((0, 0, 0.4)), a1 - Vector((0, 0, 0.4)), a0 - Vector((0, 0, 0.6))]
            K.add(slab_bm(q, Vector((0, 0, 1)), 0.5, 0.6), "GlowRed")
        for (xa, ya, za, ta, ha), (xb, yb, zb, tb, hb) in zip(JAW[:2], JAW[1:3]):
            xa, xb = xa + 4.0, xb - 3.0
            fa = (xa - JAW[0][0]) / max(1.0, JAW[1][0] - JAW[0][0])
            q = [Vector((xa, (ya + ta - 0.6) * sgn, za + 0.4 * ha)), Vector((xb, (yb + tb - 0.6) * sgn, zb + 0.4 * hb)),
                 Vector((xb, (yb + 0.72 * tb) * sgn, zb - 0.15 * hb)), Vector((xa, (ya + 0.72 * ta) * sgn, za - 0.15 * ha))]
            K.add(wedge_bm(q, Vector((0.0, sgn, 0.3)), 1.4, 3.0, 1.0), "Armor", bevel=0.35, seg=2)
    # ---- keel blade (ventral), forward raked
    keel = []
    for x, t, zt, zb in ((-210.0, 1.0, -32.0, -35.0), (-120.0, 2.4, -35.0, -42.0), (60.0, 3.6, -44.0, -66.0),
                         (200.0, 4.6, -48.0, -104.0), (262.0, 4.2, -66.0, -112.0), (318.0, 0.8, -58.0, -94.0)):
        keel.append([(x, -t, zt), (x, t, zt), (x, t * 0.4, zb), (x, -t * 0.4, zb)])
    K.add(loft3_bm(keel), "Armor", bevel=0.4, seg=2)
    for ra, rb in zip(keel, keel[1:]):
        K.hull_pts(ra + rb)
    kl = [Vector((x, 0.0, zb - 0.2)) for x, t, zt, zb in ((60.0, 3.6, -44.0, -66.0), (200.0, 4.6, -48.0, -104.0),
                                                          (262.0, 4.2, -66.0, -112.0), (318.0, 0.8, -58.0, -94.0))]
    for sgn in (-1, 1):
        for a0, a1 in zip(kl[1:], kl[2:]):
            for f0, f1 in ((0.15, 0.3), (0.6, 0.75)):
                b0, b1 = a0.lerp(a1, f0), a0.lerp(a1, f1)
                q = [b0 + Vector((0, 0, 6.0)), b1 + Vector((0, 0, 6.0)), b1 + Vector((0, 0, 5.4)), b0 + Vector((0, 0, 5.4))]
                K.add(slab_bm(q, Vector((0, sgn, 0)), 2.6, 0.0), "GlowRed")
    # ---- engines
    K.add(loft_bm(ENGINE_BLOCK), "Trim", bevel=0.6, seg=2, angle=25.0)
    for i in range(len(ENGINE_BLOCK) - 1):
        K.hull_pts([(x, y, z) for x, poly in ENGINE_BLOCK[i:i + 2] for (y, z) in poly])
    engines = [(0.0, 10.0, 24.0, 30.0), (48.0, 6.0, 17.0, 24.0), (-48.0, 6.0, 17.0, 24.0), (24.0, -26.0, 11.0, 17.0),
               (-24.0, -26.0, 11.0, 17.0), (82.0, 16.0, 12.0, 18.0), (-82.0, 16.0, 12.0, 18.0)]
    eng_pts = []
    for y, z, R, Lb in engines:
        c = Vector((-300.0, y, z))
        prof = [(0.0, 0.62 * R), (-0.25 * Lb, 0.66 * R), (-0.65 * Lb, 0.85 * R), (-Lb, R), (-Lb - 0.6, 0.97 * R),
                (-Lb, 0.9 * R), (-0.6 * Lb, 0.66 * R), (-0.2 * Lb, 0.46 * R), (-0.12 * Lb, 0.0)]
        K.add(lathe_x_bm(prof, 40, c), "Trim", smooth=40.0)
        K.add(lathe_x_bm([(-0.11 * Lb, 0.47 * R), (-0.13 * Lb, 0.0)], 40, c), "GlowEngine", smooth=40.0)
        # hot inner band
        K.add(lathe_x_bm([(-0.2 * Lb, 0.48 * R), (-0.45 * Lb, 0.6 * R), (-0.42 * Lb, 0.56 * R), (-0.19 * Lb, 0.44 * R)], 40, c),
              "GlowEngine", smooth=40.0)
        # collar with a red ring
        K.add(cyl_bm(R * 1.12, 6.0, 40, "X", (-299.0, y, z)), "Panel", bevel=0.3, seg=2, angle=40.0)
        K.add(lathe_x_bm([(-301.6, R * 1.11), (-301.6, R * 1.15), (-302.6, R * 1.15), (-302.6, R * 1.11), (-301.6, R * 1.11)],
                         48, (0.0, y, z)), "GlowRed", smooth=40.0)
        for a in range(12):
            ang = math.tau * a / 12
            eng_pts.append((-302.0 - Lb, y + R * math.cos(ang), z + R * math.sin(ang)))
            eng_pts.append((-296.0, y + R * 1.12 * math.cos(ang), z + R * 1.12 * math.sin(ang)))
    K.hull_pts(eng_pts)
    # stern cowl: deep hood, raked cheeks and a lower lip wrapping the engine cluster
    hood = loft3_bm([[(-299.0, -96.0, 40.0), (-299.0, 96.0, 40.0), (-299.0, 92.0, 56.0), (-299.0, -92.0, 56.0)],
                     [(-342.0, -80.0, 40.0), (-342.0, 80.0, 40.0), (-336.0, 80.0, 46.0), (-336.0, -80.0, 46.0)]])
    K.add(hood, "Trim", bevel=0.5, seg=2, hull=True)
    for sgn in (-1, 1):
        ch = loft3_bm([[(-299.0, 96.0 * sgn, -38.0), (-299.0, 104.0 * sgn, -38.0), (-299.0, 104.0 * sgn, 44.0), (-299.0, 96.0 * sgn, 44.0)],
                       [(-326.0, 94.0 * sgn, -30.0), (-326.0, 99.0 * sgn, -30.0), (-340.0, 98.0 * sgn, 42.0), (-340.0, 93.0 * sgn, 42.0)]])
        K.add(ch, "Armor", bevel=0.5, seg=2, hull=True)
        K.add(slab_bm([Vector((-300.0, 104.0 * sgn, 30.0)), Vector((-338.0, 99.0 * sgn, 30.0)), Vector((-338.0, 99.0 * sgn, 28.6)),
                       Vector((-300.0, 104.0 * sgn, 28.6))], Vector((0, sgn, 0)), 0.6, 0.4), "GlowRed")
    lip = loft3_bm([[(-299.0, -64.0, -48.0), (-299.0, 64.0, -48.0), (-299.0, 64.0, -40.0), (-299.0, -64.0, -40.0)],
                    [(-326.0, -52.0, -46.0), (-326.0, 52.0, -46.0), (-326.0, 52.0, -42.0), (-326.0, -52.0, -42.0)]])
    K.add(lip, "Armor", bevel=0.4, seg=2, hull=True)
    # stern fins (forward leaning blades over the engine block)
    for sgn in (-1, 1):
        y = 40.0 * sgn
        y = 58.0 * sgn
        fin = loft3_bm([[(-300.0, y - 2.4, 54.0), (-300.0, y + 2.4, 54.0), (-292.0, y + 1.6, 80.0), (-292.0, y - 1.6, 80.0)],
                        [(-262.0, y - 2.4, 55.0), (-262.0, y + 2.4, 55.0), (-270.0, y + 1.2, 86.0), (-270.0, y - 1.2, 86.0)]])
        K.add(fin, "Panel", bevel=0.3, seg=2, hull=True)
        K.add(slab_bm([Vector((-292.5, y + 1.6 * sgn, 79.0)), Vector((-270.5, y + 1.2 * sgn, 85.0)), Vector((-270.5, y + 1.2 * sgn, 83.6)),
                       Vector((-292.5, y + 1.6 * sgn, 77.6))], Vector((0, sgn, 0)), 0.4, 0.4), "GlowRed")
    spine = loft3_bm([[(-298.0, -3.0, 55.0), (-252.0, -3.0, 55.0), (-262.0, -0.8, 132.0), (-288.0, -1.0, 118.0)],
                      [(-298.0, 3.0, 55.0), (-252.0, 3.0, 55.0), (-262.0, 0.8, 132.0), (-288.0, 1.0, 118.0)]])
    K.add(spine, "Armor", bevel=0.4, seg=2, hull=True)
    for sgn in (-1, 1):
        q = [Vector((-287.0, 1.0 * sgn, 117.0)), Vector((-263.0, 0.8 * sgn, 130.5)), Vector((-264.0, 1.1 * sgn, 128.5)),
             Vector((-287.5, 1.3 * sgn, 115.0))]
        K.add(slab_bm(q, Vector((0, sgn, 0)), 0.6, 0.6), "GlowRed")
        for k in range(3):
            zz = 70.0 + 14.0 * k
            K.add(slab_bm([Vector((-276.0 + 1.5 * k, 2.6 * sgn, zz + 1.6)), Vector((-266.0 + 1.0 * k, 2.4 * sgn, zz + 1.6)),
                           Vector((-266.0 + 1.0 * k, 2.4 * sgn, zz)), Vector((-276.0 + 1.5 * k, 2.6 * sgn, zz))], Vector((0, sgn, 0)), 0.4, 1.0),
                  "GlowWhite")
    # ---- crescent blade wings (forward swept, anhedral): the faction's fighter wing, at capital scale
    def blade_ring(y, le, te, z, t, sgn):
        yy = y * sgn
        mid = le * 0.35 + te * 0.65
        return [(le, yy, z), (mid, yy, z + t), (te, yy, z + t * 0.25), (te, yy, z - t * 0.25), (mid, yy, z - t * 0.55)]
    WING = [(56.0, -40.0, -218.0, -6.0, 12.0), (100.0, -6.0, -150.0, -12.0, 9.0), (136.0, 42.0, -64.0, -8.0, 6.0),
            (160.0, 82.0, 28.0, 16.0, 2.0)]
    for sgn in (-1, 1):
        rings = []
        for x, f in ((-232.0, 0.15), (-190.0, 0.9), (-120.0, 1.2), (-60.0, 0.9), (-28.0, 0.15)):
            yb = core_point(x, 2).y - 2.0
            rings.append([(x, (yb - 6.0 * f) * sgn, 8.0 * f), (x, (yb + 16.0 * f) * sgn, -6.0),
                          (x, (yb - 6.0 * f) * sgn, -6.0 - 18.0 * f)])
        K.add(loft3_bm(rings), "Panel", bevel=0.4, seg=2, angle=25.0)
    for sgn in (-1, 1):
        rings = [blade_ring(y, le, te, z, t, sgn) for (y, le, te, z, t) in WING]
        K.add(loft3_bm(rings), "Panel", bevel=0.45, seg=2, angle=25.0)
        for ra, rb in zip(rings, rings[1:]):
            K.hull_pts(ra + rb)
        # red leading-edge light and a white eye pair near the tip
        le = [(le_ + 1.2, y * sgn, z + 0.2) for (y, le_, te, z, t) in WING]
        for (p0, p1) in zip(le, le[1:]):
            a0, a1 = Vector(p0), Vector(p1)
            q = [a0 + Vector((0, 0, 0.7)), a1 + Vector((0, 0, 0.35)), a1 - Vector((0, 0, 0.35)), a0 - Vector((0, 0, 0.7))]
            K.add(slab_bm(q, Vector((1.0, 0.0, 0.2)), 0.5, 0.6), "GlowRed")
    # ---- bow horns: forward-raked armour blades on the upper bow flanks (the brow's cheekbones)
    for sgn in (-1, 1):
        rings = []
        for x, y, z, t, h in ((206.0, 110.0, 36.0, 6.0, 18.0), (252.0, 134.0, 38.0, 6.0, 15.0), (304.0, 142.0, 39.0, 3.5, 8.0),
                              (352.0, 146.0, 40.0, 0.6, 1.6)):
            rings.append([(x, (y - t) * sgn, z + h * 0.5), (x, (y + t) * sgn, z + h * 0.3), (x, (y + t * 0.5) * sgn, z - h * 0.5),
                          (x, (y - t) * sgn, z - h * 0.5)])
        K.add(loft3_bm(rings), "Panel", bevel=0.4, seg=2, hull=True)
    # ---- flank tiers: two rows of long forward-raked armour slabs stepping down the sides (lower row prouder)
    tier_glow = 0.0
    for sgn in (-1, 1):
        for tier, (za, zb, ta, tb, spans) in enumerate((
                (16.0, 40.0, 1.5, 4.5, ((-236.0, -134.0), (-118.0, -44.0), (-26.0, 56.0), (74.0, 126.0), (144.0, 178.0))),
                (-14.0, 14.0, 4.0, 9.0, ((-226.0, -134.0), (-114.0, -44.0), (-26.0, 56.0), (74.0, 126.0), (144.0, 176.0))))):
            for (x0, x1) in spans:
                rake = 12.0
                w0, w1 = core_point(x0, 1).y, core_point(min(x1 + rake, 188.0), 1).y
                q = [Vector((x0, w0 * sgn, za)), Vector((x1, w1 * sgn, za)), Vector((min(x1 + rake, 190.0), w1 * sgn, zb)),
                     Vector((x0 + rake, w0 * sgn, zb))]
                K.add(wedge_bm(q, Vector((0, sgn, 0)), ta, tb, 1.0), "Armor", bevel=0.5, seg=2)
                K.hull_pts([tuple(pp + Vector((0, sgn * t, 0))) for pp in q for t in (-1.0, tb)])
                # edge outline: front raked edge + top edge (red), in the step's shadow
                fe = [Vector((x1 + 1.75, w1 * sgn, za + 1.0)), Vector((x1 + 2.45, w1 * sgn, za + 1.0)),
                      Vector((min(x1 + rake, 190.0) + 2.45, w1 * sgn, zb - 1.0)), Vector((min(x1 + rake, 190.0) + 1.75, w1 * sgn, zb - 1.0))]
                # only a 1.2 m deep line at the outer lip of the step (a full-depth strip was a glowing fin seen from below)
                fe = [pp + Vector((0.0, (tb - 1.4) * sgn, 0.0)) for pp in fe]
                K.add(slab_bm(fe, Vector((0, sgn, 0)), 1.2, 0.0), "GlowRed")
                tier_glow += 0.7 * (zb - za)
                xs0, xs1 = x0 + rake + 3.0, min(x1 + rake, 190.0) - 3.0
                n_d = max(2, int((xs1 - xs0) / 26.0))
                for d_ in range(n_d):
                    u0 = xs0 + (xs1 - xs0) * d_ / n_d + 2.0
                    u1 = xs0 + (xs1 - xs0) * (d_ + 1) / n_d - 2.0

                    def outer(u, inset):
                        f = (u - (x0 + rake)) / max(1.0, (x1 - x0))
                        w = w0 + (w1 - w0) * f
                        t_ = ta + (tb - ta) * f
                        return Vector((u, (w + t_ - inset) * sgn, zb + 0.06))
                    q = [outer(u0, 0.9), outer(u1, 0.9), outer(u1, 0.3), outer(u0, 0.3)]
                    K.add(slab_bm(q, Vector((0, 0, 1)), 0.25, 0.35), "GlowRed")
                    tier_glow += 0.6 * (u1 - u0)
                n_eyes = (4 if x1 - x0 > 70.0 else 2) if tier == 0 else 2
                ez, eh = (34.0, 2.4) if tier == 0 else (8.0, 1.6)
                if True:
                    for e in range(n_eyes):
                        xe = x1 - 22.0 - e * 11.0 - (8.0 if e >= 2 else 0.0)
                        tt = ta + (tb - ta) * (xe + 9.0 - x0) / (x1 - x0)
                        we = core_point(xe + 4.0, 1).y
                        eq = [Vector((xe, (we + tt) * sgn, ez)), Vector((xe + 8.0, (we + tt) * sgn, ez)),
                              Vector((xe + 9.5, (we + tt) * sgn, ez - eh)), Vector((xe + 1.5, (we + tt) * sgn, ez - eh))]
                        K.add(slab_bm(eq, Vector((0, sgn, 0)), 0.4, 1.6), "GlowWhite")
                        tier_glow += 19.0
    # turret hardpoint blisters under the deck-edge pads (gameplay turrets at (+/-180, +/-80, 55))
    for x in (-180.0, 180.0):
        for sgn in (-1, 1):
            bl = loft3_bm([[(x - 9.0, 66.0 * sgn, 50.5), (x + 9.0, 66.0 * sgn, 50.5), (x + 7.0, 84.0 * sgn, 50.5), (x - 7.0, 84.0 * sgn, 50.5)],
                           [(x - 6.0, 66.0 * sgn, 38.0), (x + 6.0, 66.0 * sgn, 38.0), (x + 4.0, 76.0 * sgn, 42.0), (x - 4.0, 76.0 * sgn, 42.0)]])
            K.add(bl, "Armor", bevel=0.3, seg=2, hull=True)
    if stage == "blockout":
        return K

    # ---- secondary: armour plates on the core's and the citadel's strips
    gap = 1.4
    glow_area = tier_glow

    def plates_on(st, intervals, strips, seed, heavy):
        nonlocal glow_area
        for k, (kind, rake, lo, hi) in strips.items():
            for (xa, xb) in intervals:
                if kind == "shoulder" and xb <= -282.0:
                    continue
                lay = plate_layout(xa, xb, random.Random(int(xa * 7) + k + seed), lo, hi, gap)
                for pi, (x0, x1) in enumerate(lay):
                    for side in (0, 1):
                        k0, k1 = (k, k + 1) if side == 0 else (9 - k, 8 - k)
                        s0, s1 = 0.06, 0.94
                        rk = rake if x1 - x0 > rake * 3 else 0.0
                        q = [strip_point(x0, k0, k1, s0, st), strip_point(x1, k0, k1, s0, st),
                             strip_point(min(x1 + rk, xb), k0, k1, s1, st), strip_point(min(x0 + rk, xb), k0, k1, s1, st)]
                        if k1 < k0:
                            q = [q[1], q[0], q[3], q[2]]
                        a, b = q[0], q[1]
                        nrm = (b - a).cross(q[3] - a)
                        mid = sum(q, Vector()) * 0.25
                        if nrm.dot(Vector((0.0, mid.y, mid.z))) < 0:
                            nrm = -nrm
                        n1 = nrm.normalized()
                        shingle = kind == "flank" or heavy
                        if shingle:
                            # forward-raked shingle: thin at the aft edge, proud at the front edge
                            ta, tb = (1.0, 3.4) if not heavy else (1.4, 4.2)
                            if k1 < k0:
                                K.add(wedge_bm([q[1], q[0], q[3], q[2]], nrm, ta, tb, 0.8), "Armor", bevel=0.3, seg=2)
                            else:
                                K.add(wedge_bm(q, nrm, ta, tb, 0.8), "Armor", bevel=0.3, seg=2)
                        else:
                            K.add(slab_bm(q, nrm, 1.3, 0.8), "Panel", bevel=0.3, seg=2, angle=30.0)
                            if random.Random(pi * 31 + k + side).random() < 0.3 and x1 - x0 > 20.0:
                                cq = [pp.lerp(mid, 0.22) for pp in q]
                                K.add(slab_bm(cq, nrm, 2.3, 0.0), "Panel", bevel=0.25, seg=2, angle=30.0)
                        # eyes: white slits near the proud front edge of every flank shingle
                        if kind == "flank" and x1 - x0 > 30.0 and not heavy:
                            for e in range(2 if x1 - x0 > 60 else 1):
                                xe = x1 - 13.0 - e * 10.0
                                tt = 1.0 + 2.4 * (xe + 4.0 - x0) / (x1 - x0)
                                top = strip_point(xe, k0, k1, 0.16, st) if side == 0 else strip_point(xe, k1, k0, 0.84, st)
                                base = top + n1 * tt
                                eq = [base, base + Vector((6.5, 0.0, 0.0)), base + Vector((7.8, 0.0, -1.6)),
                                      base + Vector((1.3, 0.0, -1.6))]
                                K.add(slab_bm(eq, n1, 0.35, 0.9), "GlowWhite")
                                glow_area += 7.0 * 1.6
                        # a red seam in EVERY gap in front of a shingle (the plate-outline language)
                        if shingle and x1 < xb - 2.0:
                            xs = x1 + gap * 0.5
                            if side == 0:
                                p0, p1 = strip_point(xs, k0, k1, 0.08, st), strip_point(min(xs + rk, xb), k0, k1, 0.92, st)
                            else:
                                p0, p1 = strip_point(xs, k1, k0, 0.92, st), strip_point(min(xs + rk, xb), k1, k0, 0.08, st)
                            gq = [p0 - Vector((0.32, 0, 0)), p0 + Vector((0.32, 0, 0)), p1 + Vector((0.32, 0, 0)), p1 - Vector((0.32, 0, 0))]
                            gq = [pp + n1 * ((3.4 if not heavy else 4.2) - 1.3) for pp in gq]
                            K.add(slab_bm(gq, n1, 1.0, 0.0), "GlowRed")
                            glow_area += 0.9 * (p1 - p0).length

    core_iv = [(-246.0, -180.0), (-180.0, -120.0), (-120.0, -30.0), (-30.0, 70.0), (70.0, 140.0), (140.0, 190.0)]
    plates_on(None, core_iv, {0: ("shoulder", 3.0, 26, 48), 1: ("flank", 12.0, 52, 92), 2: ("chine", 2.0, 24, 44),
                              3: ("lowkeel", 0.0, 22, 40)}, 0, False)
    plates_on(CITADEL, [(204.0, 224.0), (224.0, 268.0), (268.0, 298.0)],
              {0: ("shoulder", 4.0, 20, 40), 1: ("flank", 8.0, 30, 48), 2: ("chine", 5.0, 22, 40), 3: ("lowkeel", 3.0, 22, 40)},
              5, True)
    # bottom plates (split by the keel)
    for st, ivs in ((None, core_iv), (CITADEL, [(204.0, 224.0), (224.0, 268.0), (268.0, 298.0)])):
        for (xa, xb) in ivs:
            for pi, (x0, x1) in enumerate(plate_layout(xa, xb, random.Random(int(xa) + 99), 24, 46, gap)):
                for s0, s1 in ((0.06, 0.42), (0.58, 0.94)):
                    q = [strip_point(x0, 4, 5, s0, st), strip_point(x1, 4, 5, s0, st), strip_point(x1, 4, 5, s1, st),
                         strip_point(x0, 4, 5, s1, st)]
                    K.add(slab_bm(q, Vector((0, 0, -1)), 1.2, 0.8), "Panel", bevel=0.3, seg=2)

    for sgn in (-1, 1):
        # deck-edge light line, dashed every 40 m
        x = -236.0
        while x < 186.0:
            xe = min(x + 24.0, 188.0)
            a0, a1 = core_point(x, 0), core_point(xe, 0)
            q = [a0 + Vector((0, 0, -0.3)), a1 + Vector((0, 0, -0.3)), a1 + Vector((0, 0, -1.0)), a0 + Vector((0, 0, -1.0))]
            q = [Vector((pp.x, pp.y * (1 if sgn > 0 else -1), pp.z)) for pp in q]
            K.add(slab_bm(q, Vector((0, sgn, 0.3)), 1.6, 0.3), "GlowRed")
            glow_area += 0.7 * (xe - x)
            x += 40.0

    def dz(x):
        z = core_point(max(-298.0, min(296.0, x)), 0).z
        if x < -246.0:
            z = max(z, core_point(max(-300.0, x), 0, ENGINE_BLOCK).z)
        if 190.0 <= x <= 300.0:
            z = max(z, core_point(x, 0, CITADEL).z)
        return z
    # deck plates outboard of the trench walls (|y| 28 .. edge), two rows; aft deck full width
    for sgn in (-1, 1):
        for (xa, xb) in ((-244.0, 192.0),):
            for row, (ya, yb) in enumerate(((29.0, 51.0), (52.5, 72.0))):
                for x0, x1 in plate_layout(xa, xb, random.Random(int(xa) * 3 + row * 17 + (sgn > 0)), 18, 40, gap):
                    yb_ = min(yb, core_point(x0, 0).y - 1.5, core_point(x1, 0).y - 1.5) if xa < 0 or x1 < 190 else yb + 6.0
                    q = [Vector((x0, ya * sgn, dz(x0))), Vector((x1, ya * sgn, dz(x1))),
                         Vector((x1, yb_ * sgn, dz(x1))), Vector((x0, yb_ * sgn, dz(x0)))]
                    K.add(slab_bm(q, Vector((0, 0, 1)), 0.9, 0.8), "Panel", bevel=0.25, seg=2)

    # ---- bow face: hangar mouth, eyes under the prow
    xf = 300.0
    # hangar mouth: a deep lit tunnel framed by heavy lips (68 x 40 m opening; launch point (330, 0, -20) in front)
    K.add(box_bm((xf + 0.4, 0.0, -24.0), (1.0, 66.0, 38.0)), "Trim")
    for zz in (-38.0, -30.0, -22.0, -14.0):
        K.add(box_bm((xf + 1.0, 0.0, zz), (0.6, 62.0, 1.4)), "GlowRed")
    for (c, sz) in (((xf + 7.0, 0.0, -2.0), (14.0, 80.0, 5.0)), ((xf + 5.0, 0.0, -46.0), (10.0, 76.0, 5.0)),
                    ((xf + 6.0, -36.5, -24.0), (12.0, 6.0, 44.0)), ((xf + 6.0, 36.5, -24.0), (12.0, 6.0, 44.0))):
        K.add(box_bm(c, sz), "Trim", bevel=0.5, seg=2)
    K.hull_pts([(xf + dx, y, z) for dx in (0.0, 14.0) for y in (-40.0, 40.0) for z in (-49.0, 0.5)])
    for i in range(7):    # internal bulkhead ribs (depth cues)
        y = -27.0 + 9.0 * i
        K.add(box_bm((xf + 2.0, y, -24.0), (3.0, 1.0, 38.0)), "Trim", bevel=0.1, seg=1)
    for sgn in (-1, 1):   # white light columns inside the lips
        K.add(box_bm((xf + 6.0, 33.2 * sgn, -24.0), (8.0, 0.5, 30.0)), "GlowWhite")
    for i in range(9):    # landing lights on the lower lip and an outline on the upper lip
        K.add(box_bm((xf + 10.2, -32.0 + 8.0 * i, -43.6), (0.4, 2.4, 0.8)), "GlowWhite")
    K.add(box_bm((xf + 14.1, 0.0, -1.0), (0.4, 78.0, 1.0)), "GlowRed")
    # bow eyes: one chevron row of five big slits under the brow (the focal point)
    for (y, z, w, h, tilt) in ((0.0, 12.0, 14.0, 3.4, 0.0), (19.0, 10.5, 16.0, 3.0, 1.2), (-19.0, 10.5, 16.0, 3.0, -1.2),
                               (37.0, 7.5, 12.0, 2.4, 1.6), (-37.0, 7.5, 12.0, 2.4, -1.6)):
        q = [Vector((xf, y - w * 0.5, z + h * 0.5 - tilt)), Vector((xf, y + w * 0.5, z + h * 0.5 + tilt)),
             Vector((xf, y + w * 0.5, z - h * 0.5 + tilt)), Vector((xf, y - w * 0.5, z - h * 0.5 - tilt))]
        K.add(slab_bm(q, Vector((1, 0, 0)), 0.7, 0.3), "GlowWhite")
        glow_area += w * h
        cen = sum(q, Vector()) * 0.25
        sq = [pp + (pp - cen) * Vector((0.0, 0.12, 0.6)) for pp in q]
        K.add(slab_bm(sq, Vector((1, 0, 0)), 0.4, 0.3), "Trim")
    # brow lip: a heavy overhanging slab right over the eyes
    K.add(loft3_bm([[(xf - 2.0, -46.0, 18.0), (xf - 2.0, 46.0, 18.0), (xf - 2.0, 46.0, 23.0), (xf - 2.0, -46.0, 23.0)],
                    [(xf + 14.0, -38.0, 17.0), (xf + 14.0, 38.0, 17.0), (xf + 12.0, 38.0, 21.0), (xf + 12.0, -38.0, 21.0)]]),
          "Armor", bevel=0.4, seg=2)
    K.add(box_bm((xf + 13.6, 0.0, 17.6), (0.6, 70.0, 0.9)), "GlowRed")

    # ---- sponsons (weapon batteries) on the chines
    for x in (-150.0, -60.0, 30.0, 120.0):
        for sgn in (-1, 1):
            a = core_point(x, 2 if sgn > 0 else 7)
            b = core_point(x, 3 if sgn > 0 else 6)
            m = (a + b) * 0.5
            out = Vector((0.0, sgn * 1.0, -0.55)).normalized()
            c = m + out * 3.0
            K.add(box_bm(c, (20.0, 9.0, 8.0)), "Panel", bevel=0.4, seg=2,
                  place=None)
            K.add(box_bm(c + Vector((1.0, 0.0, 0.0)) + out * 4.2, (14.0, 2.0, 5.0)), "Trim", bevel=0.2, seg=1)
            for dy in (-1.8, 1.8):
                K.add(cyl_bm(0.75, 16.0, 12, "X", c + Vector((16.0, dy, -0.5)) + out * 1.0), "Trim", smooth=40.0)
                K.add(cyl_bm(0.85, 0.6, 12, "X", c + Vector((24.0, dy, -0.5)) + out * 1.0), "GlowRed", smooth=40.0)
            K.add(box_bm(c + Vector((-6.0, 0.0, 0.0)) + out * 4.6, (3.0, 4.0, 0.6)), "GlowWhite")

    # ---- dorsal: turret pads, emitter collars, trench floor
    for x in (-180.0, 180.0):
        for sgn in (-1, 1):
            K.add(cyl_bm(4.5, 1.6, 24, "Z", (x, 77.0 * sgn, 51.0)), "Trim", bevel=0.25, seg=2, angle=40.0)
            K.add(cyl_bm(4.8, 0.5, 24, "Z", (x, 77.0 * sgn, 50.4)), "GlowRed", smooth=40.0)
    for p in EMITTERS:
        K.add(cyl_bm(10.0, 1.4, 6, "Z", (p.x, p.y, 51.2)), "Trim", bevel=0.3, seg=2)
    # trench floor (|y| < 22): ribs, conduits, the glowing centre channel, runway lights, hatches
    tx0, tx1 = -238.0, 238.0
    x = tx0 + 4.0
    i = 0
    while x < tx1 - 4.0:
        zf = core_point(x, 0).z
        for sgn in (-1, 1):
            K.add(box_bm((x, sgn * 12.25, zf + 1.2), (1.6, 18.5, 2.4)), "Trim", bevel=0.2, seg=1)
        if i % 2 == 0:
            for sgn in (-1, 1):
                K.add(box_bm((x + 6.0, sgn * 6.0, zf + 0.15), (0.8, 0.8, 0.4)), "GlowWhite")
        if i % 3 == 1:
            for sgn in (-1, 1):
                K.add(box_bm((x + 6.0, sgn * 11.5, zf + 0.35), (8.0, 7.0, 0.7)), "Panel", bevel=0.15, seg=1)
        x += 12.0
        i += 1
    for sgn in (-1, 1):
        for (xa, xb) in ((tx0, 214.0), (216.0, tx1)):
            za, zb = core_point(xa, 0).z, core_point(xb, 0).z
            K.add(loft3_bm([[(xa, sgn * 16.0 + dy, za + 1.1 + dz) for dy, dz in
                             ((math.cos(t) * 1.1, math.sin(t) * 1.1) for t in (j * math.tau / 10 for j in range(10)))],
                            [(xb, sgn * 16.0 + dy, zb + 1.1 + dz) for dy, dz in
                             ((math.cos(t) * 1.1, math.sin(t) * 1.1) for t in (j * math.tau / 10 for j in range(10)))]]),
                  "Trim", smooth=40.0)
    for (xa, xb) in ((tx0, 214.0), (216.0, tx1)):
        za, zb = core_point(xa, 0).z, core_point(xb, 0).z
        q = [Vector((xa, -0.5, za)), Vector((xb, -0.5, zb)), Vector((xb, 0.5, zb)), Vector((xa, 0.5, za))]
        K.add(slab_bm(q, Vector((0, 0, 1)), 0.12, 0.3), "GlowRed")
        glow_area += 1.0 * (xb - xa)
        for sgn in (-1, 1):
            q = [Vector((xa, sgn * 0.6, za)), Vector((xb, sgn * 0.6, zb)), Vector((xb, sgn * 2.8, zb)), Vector((xa, sgn * 2.8, za))]
            K.add(slab_bm(q, Vector((0, 0, 1)), 0.5, 0.3), "Trim")
    # trench landmarks: machinery blocks every 60 m (big / medium rhythm) and the reactor's cradle
    for j, xm in enumerate((-150.0, -90.0, -30.0, 30.0, 90.0)):
        big = j % 2 == 0
        for sgn in (-1, 1):
            zf = core_point(xm, 0).z
            hgt, ln = (7.0, 16.0) if big else (4.0, 9.0)
            blk = loft3_bm([[(xm - ln * 0.5, 13.0 * sgn, zf), (xm + ln * 0.5, 13.0 * sgn, zf), (xm + ln * 0.5 - 2.0, 15.0 * sgn, zf + hgt),
                             (xm - ln * 0.5, 15.0 * sgn, zf + hgt)],
                            [(xm - ln * 0.5, 22.5 * sgn, zf), (xm + ln * 0.5, 22.5 * sgn, zf), (xm + ln * 0.5 - 2.0, 22.5 * sgn, zf + hgt),
                             (xm - ln * 0.5, 22.5 * sgn, zf + hgt)]])
            K.add(blk, "Armor", bevel=0.25, seg=2)
            for v in range(3 if big else 2):
                K.add(box_bm((xm - ln * 0.3 + v * 3.2, 14.2 * sgn, zf + hgt * 0.55), (1.6, 0.6, hgt * 0.5)), "GlowRed")
    rc, rr = REACTOR
    for i in range(4):
        a = math.tau * (i + 0.5) / 4
        d = Vector((math.cos(a), math.sin(a), 0.0))
        t = Vector((-d.y, d.x, 0.0)) * 1.4
        b0 = Vector((rc.x, rc.y, 50.0)) + d * 20.0
        b1 = Vector((rc.x, rc.y, 50.0)) + d * 12.0
        tip = Vector((rc.x, rc.y, 61.0)) + d * 15.0
        arm = loft3_bm([[b0 - t, b1 - t, tip - t * 0.5], [b0 + t, b1 + t, tip + t * 0.5]])
        K.add(arm, "Trim", bevel=0.2, seg=1)
        K.add(box_bm(tip - d * 0.8 + Vector((0, 0, -1.5)), (1.0, 1.0, 2.5)), "GlowRed")
    K.add(cyl_bm(14.0, 1.2, 32, "Z", (rc.x, rc.y, 50.6)), "Trim", bevel=0.2, seg=1, angle=40.0)
    for i in range(16):
        a = math.tau * i / 16
        d = Vector((math.cos(a), math.sin(a), 0.0))
        K.add(box_bm(Vector((rc.x, rc.y, 51.25)) + d * 12.8, (1.2, 1.2, 0.5)), "GlowEngine",
              place=None)
    # dorsal vents / greeble clusters near the emitters and the bridge base
    grng = random.Random(3)
    for cx, cy in ((165.0, 64.0), (-105.0, 64.0), (-262.0, 30.0), (230.0, 40.0)):
        for sgn in (-1, 1):
            for j in range(7):
                bx = cx + grng.uniform(-24, 24)
                by = (cy + grng.uniform(-9, 9)) * sgn
                if abs(bx - cx) < 11 and abs(abs(by) - cy) < 11 and cy == 64.0:
                    continue
                sx, sy, sz = grng.uniform(3, 9), grng.uniform(2, 6), grng.uniform(1.0, 2.6)
                edge_y = core_point(max(-298.0, min(296.0, bx)), 0).y - sy * 0.5 - 1.0
                by = max(-edge_y, min(edge_y, by))
                zf = core_point(max(-296.0, min(294.0, bx)), 0).z + 0.9
                K.add(box_bm((bx, by, zf + sz * 0.5), (sx, sy, sz)), "Trim", bevel=0.15, seg=1)
                if grng.random() < 0.3:
                    K.add(box_bm((bx, by, zf + sz + 0.05), (sx * 0.6, sy * 0.3, 0.12)), "GlowRed")
    K.glow_area = glow_area
    return K


# --------------------------------------------------------------------------------------------- bridge (local: pivot at its centre)

BRIDGE_STRETCH = Matrix.Translation((0.0, 0.0, 14.4)) @ Matrix.Diagonal((1.0, 1.0, 1.48, 1.0))   # tower to 80 m above the deck


def build_bridge(asset, mats, stage):
    K = Kit(asset, mats, post=BRIDGE_STRETCH)
    hx, hy, hz = BRIDGE_HALF
    # plinth
    K.add(box_bm((0.0, 0.0, -hz + 3.0), (74.0, 39.0, 6.0)), "Trim", bevel=0.5, seg=2, hull=True)
    # tower: tapered, leaning forward
    tower = [(-hz + 6.0, [(-30.0, -16.0), (24.0, -16.0), (28.0, -10.0), (28.0, 10.0), (24.0, 16.0), (-30.0, 16.0)]),
             (10.0, [(-24.0, -10.0), (22.0, -10.0), (28.0, -6.0), (28.0, 6.0), (22.0, 10.0), (-24.0, 10.0)])]
    rings = [[(x, y, z) for (x, y) in poly] for z, poly in tower]
    K.add(loft3_bm(rings), "Panel", bevel=0.5, seg=2, hull=True)
    # hammerhead command deck with a chevron front
    head = [(10.0, [(-28.0, -19.0), (26.0, -19.0), (37.0, 0.0), (26.0, 19.0), (-28.0, 19.0)]),
            (20.0, [(-26.0, -17.5), (24.0, -17.5), (35.0, 0.0), (24.0, 17.5), (-26.0, 17.5)]),
            (24.0, [(-20.0, -12.0), (16.0, -12.0), (24.0, 0.0), (16.0, 12.0), (-20.0, 12.0)])]
    K.add(loft3_bm([[(x, y, z) for (x, y) in poly] for z, poly in head]), "Panel", bevel=0.4, seg=2, hull=True)
    # brow visor band of white windows across the chevron front
    for sgn in (-1, 1):
        a = Vector((25.0, 18.25 * sgn, 0.0))
        b = Vector((36.0, 0.0, 0.0))
        d = (b - a)
        for k in range(5):
            p0 = a + d * (0.06 + 0.18 * k)
            p1 = a + d * (0.06 + 0.18 * k + 0.13)
            q = [p0 + Vector((0, 0, 16.6)), p1 + Vector((0, 0, 16.6)), p1 + Vector((0, 0, 14.2)), p0 + Vector((0, 0, 14.2))]
            nrm = Vector((d.y * -sgn, d.x * sgn, 0.0)).normalized() if False else Vector((1.0, 0.55 * sgn, 0.0)).normalized()
            K.add(slab_bm(q, nrm, 0.3, 1.2), "GlowWhite")
        # big eye below the visor
        q = [Vector((27.0, 12.0 * sgn, 7.0)), Vector((27.8, 3.0 * sgn, 8.5)), Vector((27.8, 3.0 * sgn, 6.5)), Vector((27.0, 12.0 * sgn, 5.4))]
        K.add(slab_bm(q, Vector((1, 0, 0)), 0.5, 0.5), "GlowWhite")
    if stage != "blockout":
        # side armour plates with red seams
        for sgn in (-1, 1):
            for i, (x0, x1) in enumerate(((-28.0, -10.0), (-9.0, 8.0), (9.0, 23.0))):
                z0, z1 = -hz + 8.0, 6.0
                def yat(x, z):
                    t = (z - (-hz + 6.0)) / (10.0 - (-hz + 6.0))
                    return (16.0 + (10.0 - 16.0) * t) * sgn
                q = [Vector((x0, yat(x0, z0), z0)), Vector((x1, yat(x1, z0), z0)), Vector((x1 + 2.0, yat(x1, z1), z1)), Vector((x0 + 2.0, yat(x0, z1), z1))]
                nrm = Vector((0.0, sgn, 0.25)).normalized()
                K.add(slab_bm(q, nrm, 1.0, 0.6), "Panel", bevel=0.2, seg=2)
                if i < 2:
                    gq = [Vector((x1 + 0.2, yat(x1, z0), z0)), Vector((x1 + 0.8, yat(x1, z0), z0)),
                          Vector((x1 + 2.8, yat(x1, z1), z1)), Vector((x1 + 2.2, yat(x1, z1), z1))]
                    K.add(slab_bm(gq, nrm, 0.7, 0.3), "GlowRed")
        # crown: sensor fin + antenna spikes (forward raked)
        fin = loft3_bm([[(-18.0, -1.0, 24.0), (8.0, -1.0, 24.0), (8.0, 1.0, 24.0), (-18.0, 1.0, 24.0)],
                        [(-14.0, -0.4, 29.5), (-2.0, -0.4, 29.5), (-2.0, 0.4, 29.5), (-14.0, 0.4, 29.5)]])
        K.add(fin, "Trim", bevel=0.15, seg=1)
        for sgn in (-1, 1):
            sp = loft3_bm([[(4.0, 8.0 * sgn - 0.6, 23.0), (6.0, 8.0 * sgn - 0.6, 23.0), (6.0, 8.0 * sgn + 0.6, 23.0), (4.0, 8.0 * sgn + 0.6, 23.0)],
                           [(12.0, 9.0 * sgn - 0.05, 29.8), (12.3, 9.0 * sgn - 0.05, 29.8), (12.3, 9.0 * sgn + 0.05, 29.8), (12.0, 9.0 * sgn + 0.05, 29.8)]])
            K.add(sp, "Trim")
            K.add(box_bm((-30.5, 16.5 * sgn, -hz + 9.0), (3.0, 3.0, 1.0)), "GlowRed")
        # aft vents
        for k in range(4):
            K.add(box_bm((-26.2, -9.0 + 6.0 * k, -2.0), (2.0, 4.0, 10.0)), "Trim", bevel=0.15, seg=1)
        K.add(box_bm((-26.6, 0.0, 18.0), (1.2, 20.0, 1.0)), "GlowRed")
        # plinth step, aft comms block, head ledge, side pods with eyes, front ribs
        K.add(box_bm((0.0, 0.0, -hz + 7.0), (66.0, 34.0, 3.0)), "Panel", bevel=0.4, seg=2)
        K.add(box_bm((-31.0, 0.0, -14.0), (10.0, 24.0, 16.0)), "Panel", bevel=0.4, seg=2)
        for k in range(5):
            K.add(box_bm((-36.2, -8.0 + 4.0 * k, -14.0), (0.8, 2.4, 12.0)), "Trim", bevel=0.1, seg=1)
        K.add(box_bm((-36.1, 0.0, -8.0), (0.8, 18.0, 0.6)), "GlowRed")
        ledge = [(8.5, [(-30.0, -20.0), (27.0, -20.0), (38.5, 0.0), (27.0, 20.0), (-30.0, 20.0)]),
                 (10.5, [(-29.0, -19.5), (26.5, -19.5), (37.8, 0.0), (26.5, 19.5), (-29.0, 19.5)])]
        K.add(loft3_bm([[(x, y, z) for (x, y) in poly] for z, poly in ledge]), "Trim", bevel=0.25, seg=2)
        for sgn in (-1, 1):
            pod = loft3_bm([[(-12.0, 14.0 * sgn, -12.0), (16.0, 14.0 * sgn, -12.0), (22.0, 14.0 * sgn, -4.0), (-12.0, 14.0 * sgn, 2.0)],
                            [(-10.0, 19.5 * sgn, -10.0), (13.0, 19.5 * sgn, -10.0), (18.0, 19.5 * sgn, -5.0), (-10.0, 19.5 * sgn, 0.0)]])
            K.add(pod, "Panel", bevel=0.3, seg=2)
            for k in range(3):
                q = [Vector((-6.0 + 7.0 * k, 19.6 * sgn, -3.2)), Vector((-1.5 + 7.0 * k, 19.6 * sgn, -3.0)),
                     Vector((-1.5 + 7.0 * k, 19.6 * sgn, -4.2)), Vector((-6.0 + 7.0 * k, 19.6 * sgn, -4.4))]
                K.add(slab_bm(q, Vector((0, sgn, 0)), 0.2, 0.4), "GlowWhite")
            K.add(box_bm((2.0, 19.6 * sgn, -8.6), (22.0, 0.5, 0.5)), "GlowRed")
        for k, y in enumerate((-9.0, 0.0, 9.0)):
            rib = loft3_bm([[(26.0, y - 1.0, -hz + 8.0), (29.0, y - 1.0, -hz + 8.0), (29.0, y + 1.0, -hz + 8.0), (26.0, y + 1.0, -hz + 8.0)],
                            [(25.5, y - 0.8, 9.0), (29.8, y - 0.8, 9.0), (29.8, y + 0.8, 9.0), (25.5, y + 0.8, 9.0)]])
            K.add(rib, "Trim", bevel=0.2, seg=1)
        K.add(box_bm((28.2, 0.0, -14.0), (0.8, 22.0, 0.8)), "GlowRed")
        for sgn in (-1, 1):
            e = loft3_bm([[(28.3, 9.8 * sgn, -hz + 8.0), (28.8, 9.8 * sgn, -hz + 8.0), (28.8, 10.6 * sgn, -hz + 8.0), (28.3, 10.6 * sgn, -hz + 8.0)],
                          [(28.3, 6.2 * sgn, 9.5), (28.8, 6.2 * sgn, 9.5), (28.8, 7.0 * sgn, 9.5), (28.3, 7.0 * sgn, 9.5)]])
            K.add(e, "GlowRed")
        for k in range(4):
            y = -7.5 + 5.0 * k
            K.add(box_bm((28.4, y, 2.0), (0.8, 3.4, 1.2)), "GlowWhite")
    return K


# --------------------------------------------------------------------------------------------- trench wall (local: pivot at its centre, LEFT wall, inner face +Y)

def build_wall(asset, mats, stage):
    K = Kit(asset, mats)
    hx, hy, hz = WALL_HALF
    core = loft_bm([(-hx, [(-hy, -hz), (hy, -hz), (hy, hz - 4.0), (-hy, hz - 4.0)]),
                    (hx, [(-hy, -hz), (hy, -hz), (hy, hz - 4.0), (-hy, hz - 4.0)])])
    K.add(core, "Panel", bevel=0.4, seg=2)
    K.hull_pts([(sx * hx, sy * hy, sz * hz) for sx in (-1, 1) for sy in (-1, 1) for sz in (-1, 1)])
    # forward-raked crest blades along the top
    pattern = [(34.0, 1.0), (12.0, 0.35), (8.0, 0.15), (12.0, 0.35)]    # (length m, height share): 1 : 0.4 : 0.1 rhythm
    x = -hx + 1.0
    i = 0
    while x < hx - 6.0:
        ln, share = pattern[i % len(pattern)]
        x1 = min(x + ln, hx - 1.0)
        boost = math.exp(-((x + ln * 0.5 - 135.0) / 60.0) ** 2)          # the towers rise around the reactor
        top = hz - 4.0 + 1.0 + share * (3.0 + 4.0 * boost)
        if share >= 1.0:
            top = hz + 3.0 * boost
        blade = loft3_bm([[(x, -hy, hz - 4.5), (x1, -hy, hz - 4.5), (x1 + 2.5, -hy * 0.6, top), (x + 0.4 * ln, -hy * 0.6, top)],
                          [(x, hy, hz - 4.5), (x1, hy, hz - 4.5), (x1 + 2.5, hy * 0.6, top), (x + 0.4 * ln, hy * 0.6, top)]])
        K.add(blade, "Armor" if share >= 1.0 else "Panel", bevel=0.25, seg=2)
        if top > hz - 2.0:      # tall enough above the old wall box to fly into: its own convex hull
            K.hull_pts([(x, -hy, hz - 4.5), (x1, -hy, hz - 4.5), (x1 + 2.5, -hy * 0.6, top), (x + 0.4 * ln, -hy * 0.6, top),
                        (x, hy, hz - 4.5), (x1, hy, hz - 4.5), (x1 + 2.5, hy * 0.6, top), (x + 0.4 * ln, hy * 0.6, top)])
        if share >= 1.0:
            K.add(box_bm((x1 - 1.0, hy * 0.62, top - 1.6), (3.0, 0.4, 0.6)), "GlowRed")
        x = x1 + 1.5
        i += 1
    if stage != "blockout":
        # inner face (+Y): ribs every 14 m, recessed panels, a red light band and white runway lights
        x = -hx + 7.0
        k = 0
        while x < hx - 3.0:
            K.add(box_bm((x, hy + 0.5, -1.0), (1.6, 1.0, 2 * hz - 6.0)), "Trim", bevel=0.2, seg=1)
            K.add(box_bm((x + 7.0, hy + 0.12, 4.5), (11.0, 0.3, 6.5)), "Panel", bevel=0.12, seg=1)
            K.add(box_bm((x + 7.0, hy + 0.12, -6.5), (11.0, 0.3, 9.0)), "Panel", bevel=0.12, seg=1)
            K.add(box_bm((x + 7.0, hy + 0.35, 9.2), (1.2, 0.3, 0.7)), "GlowWhite")
            K.add(box_bm((x + 7.0, hy + 0.2, -0.8), (5.0, 0.4, 0.5)), "GlowRed")
            x += 14.0
            k += 1
        # outer face (-Y): buttresses standing on the deck, one red seam each
        x = -hx + 15.0
        while x < hx - 10.0:
            bt = loft3_bm([[(x - 2.0, -hy, -hz), (x + 2.0, -hy, -hz), (x + 2.0, -hy, 2.0), (x - 2.0, -hy, 2.0)],
                           [(x - 1.5, -hy - 7.0, -hz), (x + 1.5, -hy - 7.0, -hz), (x + 1.5, -hy - 0.5, 1.0), (x - 1.5, -hy - 0.5, 1.0)]])
            K.add(bt, "Panel", bevel=0.25, seg=2)
            K.add(box_bm((x + 2.3, -hy - 1.6, -7.0), (0.5, 0.4, 12.0)), "GlowRed")
            x += 30.0
        # raked end caps
        for sgn in (-1, 1):
            cap = loft3_bm([[(sgn * hx, -hy - 1.0, -hz), (sgn * hx, hy + 1.0, -hz), (sgn * hx, hy + 1.0, hz - 6.0), (sgn * hx, -hy - 1.0, hz - 6.0)],
                            [(sgn * (hx + 6.0), -hy * 0.5, -hz), (sgn * (hx + 6.0), hy * 0.5, -hz), (sgn * (hx - 2.0), hy * 0.5, hz - 3.0), (sgn * (hx - 2.0), -hy * 0.5, hz - 3.0)]])
            K.add(cap, "Trim", bevel=0.25, seg=2)
    return K


# --------------------------------------------------------------------------------------------- shield emitter (local: pivot at its centre)

def build_emitter(asset, mats, stage):
    K = Kit(asset, mats)
    K.add(cyl_bm(7.4, 4.0, 6, "Z", (0.0, 0.0, -18.0), r_top=6.6), "Trim", bevel=0.3, seg=2)
    K.add(cyl_bm(5.2, 24.0, 6, "Z", (0.0, 0.0, -4.0), r_top=2.8), "Panel", bevel=0.3, seg=2)
    K.add(cyl_bm(6.8, 0.6, 6, "Z", (0.0, 0.0, -15.6)), "GlowRed")
    for i in range(3):    # three forward-raked fins
        a = math.tau * i / 3 + math.radians(0.0)
        d = Vector((math.cos(a), math.sin(a), 0.0))
        t = Vector((-d.y, d.x, 0.0)) * 0.5
        p = [Vector((0, 0, -15.0)) + d * 3.5, Vector((0, 0, -15.0)) + d * 7.3, Vector((0, 0, 4.0)) + d * 6.0, Vector((0, 0, 10.0)) + d * 3.0]
        fin = loft3_bm([[q - t for q in p], [q + t for q in p]])
        K.add(fin, "Panel", bevel=0.12, seg=1)
    # crown prongs cradling the lens
    for i in range(3):
        a = math.tau * (i + 0.5) / 3
        d = Vector((math.cos(a), math.sin(a), 0.0))
        t = Vector((-d.y, d.x, 0.0)) * 0.45
        p = [Vector((0, 0, 7.0)) + d * 2.4, Vector((0, 0, 7.0)) + d * 3.4, Vector((0, 0, 16.0)) + d * 5.2, Vector((0, 0, 20.0)) + d * 3.6]
        K.add(loft3_bm([[q - t for q in p], [q + t for q in p]]), "Trim", bevel=0.1, seg=1)
    # stacked collars with red seams, base bolts, white strips under the crown
    for z, r in ((-11.0, 4.9), (-3.0, 4.1), (5.0, 3.4)):
        K.add(cyl_bm(r, 1.6, 6, "Z", (0.0, 0.0, z)), "Trim", bevel=0.15, seg=1)
        K.add(cyl_bm(r - 0.15, 2.4, 6, "Z", (0.0, 0.0, z)), "GlowRed")
    for i in range(6):
        a = math.tau * (i + 0.5) / 6
        K.add(box_bm((math.cos(a) * 6.4, math.sin(a) * 6.4, -15.8), (1.4, 1.4, 0.8)), "Trim", bevel=0.1, seg=1)
    for i in range(3):
        a = math.tau * (i + 0.5) / 3
        d = Vector((math.cos(a), math.sin(a), 0.0))
        K.add(box_bm(d * 3.1 + Vector((0, 0, 1.0)), (0.5, 0.5, 4.0)), "GlowWhite",
              place=None)
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=24, v_segments=12, radius=2.6, matrix=Matrix.Translation((0, 0, 14.0)))
    K.add(bm, "GlowWhite", smooth=60.0)
    K.add(cyl_bm(3.2, 0.5, 24, "Z", (0.0, 0.0, 11.2)), "GlowRed", smooth=40.0)
    return K


BUILDERS = {
    # asset: (builder, tri budget (lo, hi), placements in the actor (UE m) for the review assembly, mirror_y)
    "SM_Dominion_Hull": (build_hull, (60000, 300000), [Vector((0, 0, 0))], False),
    "SM_Dominion_Bridge": (build_bridge, (3000, 60000), [BRIDGE_C], False),
    "SM_Dominion_TrenchWall": (build_wall, (5000, 60000), [WALL_C, Vector((WALL_C.x, -WALL_C.y, WALL_C.z))], True),
    "SM_Dominion_ShieldEmitter": (build_emitter, (2000, 30000), EMITTERS, False),
}

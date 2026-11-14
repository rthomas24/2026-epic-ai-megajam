"""SM_DominionMothership (issue #41) - mesh_r1 procedural hard-surface build, Blender 4.5.3 headless.

Design: Nicholas's approved v3 turnaround (concept_r1/source/sheet-DominionMothership-v3_PICK.png).
Contract: T4 local space (Unreal cm, +X bow, +Y starboard, +Z up, origin = hull centre).

All geometry is authored here in UNREAL axes (metres = cm / 100) and mirrored in Y once at the end, because the
repo's Blender->Unreal FBX convention (axis_forward='-Z', axis_up='Y', as in Bastion/Lancer mesh_r1) maps
Blender (x, y, z) m -> Unreal (100x, -100y, 100z) cm.

usage: blender -b --factory-startup --python build_dominion_mothership.py -- TEX2K_DIR OUT_DIR
(TEX2K_DIR must also hold T_DominionMothership_N_OpenGL.png, written by make_textures.py, for the GLB/preview)
"""
import bpy, bmesh, math, random, sys, os, json
from mathutils import Vector, Matrix

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
TEX = argv[0] if len(argv) > 0 else "/tmp/dm_build/tex2k"
OUT = argv[1] if len(argv) > 1 else "/tmp/dm_build/export"
os.makedirs(OUT, exist_ok=True)
NAME = "SM_DominionMothership"
RNG = random.Random(4101)

# ---------------------------------------------------------------- atlas (must match make_textures.py)
ATLAS = {  # name: (u0, v0, u1, v1, world_w_m, world_h_m, mode)
    "ARMOR":      (0.0,   0.5,    0.5,    1.0,    48, 48, "uniform"),
    "ARMOR_DARK": (0.5,   0.5,    1.0,    1.0,    48, 48, "uniform"),
    "FLOOR":      (0.0,   0.25,   0.5,    0.5,    48, 24, "uniform"),
    "WALL":       (0.0,   0.0,    0.5,    0.25,   48, 24, "uniform"),
    "BANNER":     (0.5,   0.0,    0.625,  0.5,    0, 0, "fit"),
    "TRIM_RED":   (0.625, 0.0,    0.75,   0.25,   12, 24, "uniform"),
    "METAL":      (0.75,  0.0,    1.0,    0.25,   24, 24, "uniform"),
    "AMBER":      (0.625, 0.25,   0.6875, 0.3125, 0, 0, "fit"),
    "WHITE":      (0.6875, 0.25,  0.75,   0.3125, 0, 0, "fit"),
    "RECESS":     (0.625, 0.3125, 0.75,   0.375,  24, 12, "uniform"),
    "ENGINE":     (0.625, 0.375,  0.75,   0.5,    0, 0, "fit"),
    "HANGAR":     (0.75,  0.25,   1.0,    0.5,    48, 48, "uniform"),
}
REG = list(ATLAS.keys())
RI = {n: i for i, n in enumerate(REG)}

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'; scene.unit_settings.scale_length = 1.0

# ---------------------------------------------------------------- primitives (UE axes, metres)
BM = bmesh.new()
COLL = []          # (points, label) -> one convex UCX hull each

def V(*a): return Vector(a)
def Y(s, a, b): return (min(s * a, s * b), max(s * a, s * b))

def hexa(pts, reg, faces_reg=None, bevel=0.0, bm=None):
    """pts: 8 corners: bottom (-u-v, +u-v, +u+v, -u+v) then top in the same order."""
    bm = bm or BM
    vs = [bm.verts.new(p) for p in pts]
    idx = {"bottom": (0, 3, 2, 1), "top": (4, 5, 6, 7), "-v": (0, 1, 5, 4), "+u": (1, 2, 6, 5), "+v": (2, 3, 7, 6), "-u": (3, 0, 4, 7)}
    fs = []
    for k, ii in idx.items():
        f = bm.faces.new([vs[i] for i in ii]); f.material_index = RI[(faces_reg or {}).get(k, reg)]; fs.append(f)
    if bevel > 0:
        edges = list({e for f in fs for e in f.edges})
        bmesh.ops.bevel(bm, geom=edges, offset=bevel, offset_type='OFFSET', segments=1, profile=0.5, affect='EDGES', clamp_overlap=True)
    return pts

def box(lo, hi, reg, faces_reg=None, bevel=0.0, coll=False):
    (x0, y0, z0), (x1, y1, z1) = lo, hi
    pts = [V(x0, y0, z0), V(x1, y0, z0), V(x1, y1, z0), V(x0, y1, z0), V(x0, y0, z1), V(x1, y0, z1), V(x1, y1, z1), V(x0, y1, z1)]
    hexa(pts, reg, faces_reg, bevel)
    if coll: COLL.append((pts, coll))
    return pts

def ybox(x0, x1, s, ya, yb, z0, z1, reg, faces_reg=None, bevel=0.0, coll=False):
    y0, y1 = Y(s, ya, yb); return box((x0, y0, z0), (x1, y1, z1), reg, faces_reg, bevel, coll)

def obox(c, u, hu, hv, hw, reg, faces_reg=None, bevel=0.0, shear=0.0, w=V(0, 0, 1), taper=1.0, coll=False):
    u = Vector(u).normalized(); v = Vector(w).cross(u).normalized(); w = u.cross(v).normalized()
    pts = []
    for sw in (-1, 1):
        for (su, sv) in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
            t = taper if su > 0 else 1.0
            pts.append(c + u * (su * hu + shear * sw) + v * (sv * hv * t) + w * (sw * hw * t))
    hexa(pts, reg, faces_reg, bevel)
    if coll: COLL.append((pts, coll))
    return pts

def cyl(c, r0, r1, length, reg, segs=24, axis='Z', cap0=None, cap1=None):
    c = Vector(c)
    if axis == 'Z': a, b, n = V(1, 0, 0), V(0, 1, 0), V(0, 0, 1)
    else: a, b, n = V(0, 1, 0), V(0, 0, 1), V(1, 0, 0)
    ring0, ring1, pts = [], [], []
    for i in range(segs):
        t = 2 * math.pi * (i + 0.5) / segs; d = a * math.cos(t) + b * math.sin(t)
        p0 = c + d * r0; p1 = c + d * r1 + n * length
        ring0.append(BM.verts.new(p0)); ring1.append(BM.verts.new(p1)); pts += [p0, p1]
    for i in range(segs):
        j = (i + 1) % segs; f = BM.faces.new([ring0[i], ring0[j], ring1[j], ring1[i]]); f.material_index = RI[reg]
    f0 = BM.faces.new(list(reversed(ring0))); f0.material_index = RI[cap0 or reg]
    f1 = BM.faces.new(ring1); f1.material_index = RI[cap1 or reg]
    return pts

def pyramid(c, r, tip, reg, sides=6):
    c = Vector(c); tipv = BM.verts.new(Vector(tip)); ring = []; pts = [Vector(tip)]
    for i in range(sides):
        t = 2 * math.pi * i / sides
        p = c + V(math.cos(t) * r * 1.3, math.sin(t) * r, 0); ring.append(BM.verts.new(p)); pts.append(p)
    for i in range(sides):
        f = BM.faces.new([ring[i], ring[(i + 1) % sides], tipv]); f.material_index = RI[reg]
    f = BM.faces.new(ring); f.material_index = RI[reg]
    return pts

def loft(stations, reg, bm, cap_regs=("METAL", "ARMOR")):
    rings = [[bm.verts.new(V(x, y, z)) for (y, z) in prof] for (x, prof) in stations]
    n = len(rings[0])
    for a, b in zip(rings[:-1], rings[1:]):
        for i in range(n):
            j = (i + 1) % n; f = bm.faces.new([a[i], a[j], b[j], b[i]]); f.material_index = RI[reg]
    f0 = bm.faces.new(list(reversed(rings[0]))); f0.material_index = RI[cap_regs[0]]
    f1 = bm.faces.new(rings[-1]); f1.material_index = RI[cap_regs[1]]

def tapered_beam(p0, p1, w0, w1, h0, h1, reg, fr=None, bevel=0.5, coll=None):
    u = (p1 - p0).normalized(); wv = V(0, 0, 1); v = wv.cross(u).normalized(); pts = []
    for sw in (-1, 1):
        for (pe, sv) in ((0, -1), (1, -1), (1, 1), (0, 1)):
            base, ww, hh = (p0, w0, h0) if pe == 0 else (p1, w1, h1)
            pts.append(base + v * (sv * ww / 2) + wv * (sw * hh / 2))
    hexa(pts, reg, fr, bevel)
    if coll: COLL.append((pts, coll))
    return pts

def mirror8(pts, s):   # keep hexa winding consistent when a part is built for -Y
    return pts if s > 0 else [pts[3], pts[2], pts[1], pts[0], pts[7], pts[6], pts[5], pts[4]]

# ================================================================ 1. MAIN HULL (octagonal loft) + bow hangar pocket
PROFILE = [(0, 48.5), (25, 48.5), (25, 50), (34, 50), (51, 50), (67, 50), (78, 36), (78, 18), (78, 0), (78, -18), (68, -34),
           (56, -50), (28, -50), (0, -50), (-28, -50), (-56, -50), (-68, -34), (-78, -18), (-78, 0), (-78, 18), (-78, 36),
           (-67, 50), (-51, 50), (-34, 50), (-25, 50), (-25, 48.5)]
def prof_at(x):
    """Bow head (X 240..300) tapers its outer width 80 -> 68 m; |y| <= 34 (trench + bay) is never narrowed."""
    s = 1.0 if x <= 240 else 1.0 - 0.15 * (x - 240) / 60.0
    out = []
    for (y, z) in PROFILE:
        ay = abs(y); yy = ay if ay <= 34 else 34 + (ay - 34) * ((78 * s - 34) / 44.0)
        out.append((math.copysign(yy, y) if y != 0 else 0.0, z))
    return out
STATIONS = sorted(set([-300 + 25 * i for i in range(22)] + [250, 260, 270, 280, 290, 300]))
hbm = bmesh.new()
loft([(x, prof_at(x)) for x in STATIONS], "ARMOR", hbm, cap_regs=("METAL", "ARMOR"))
bmesh.ops.recalc_face_normals(hbm, faces=hbm.faces)
for f in hbm.faces:
    if len(f.verts) == 4:
        c = f.calc_center_median(); n = f.normal
        if c.z > 48 and abs(c.y) < 25.01: f.material_index = RI["RECESS"]
        elif abs(n.z) > 0.7: f.material_index = RI["ARMOR_DARK"]
aft_pts = [V(x, y, (50.0 if (abs(y) <= 25 and z > 48) else z)) for x in STATIONS if x <= 240 for (y, z) in prof_at(x)]
aft_pts += [V(x, sy * 80, z) for x in (-300, 240) for sy in (-1, 1) for z in (-16, 36)]   # include the flank armour skin
COLL.append((aft_pts, "hull_main_aft_of_bay"))
for sgn in (1, -1):
    pts = []
    for x in (240, 300):
        pts += [V(x, y, z) for (y, z) in prof_at(x) if sgn * y >= 25] + [V(x, 25 * sgn, 50), V(x, 25 * sgn, -50)]
    COLL.append((pts, "hull_bow_beside_bay_" + ("stbd" if sgn > 0 else "port")))
COLL.append(([V(x, y, z) for x in (240, 300) for y in (-25, 25) for z in (-2.5, 50)], "hull_bow_over_bay"))
COLL.append(([V(x, y, z) for x in (240, 300) for y in (-25, 25) for z in (-50, -37.5)], "hull_bow_under_bay"))
hme = bpy.data.meshes.new("hull_tmp"); hbm.to_mesh(hme); hbm.free()
hobj = bpy.data.objects.new("hull_tmp", hme); scene.collection.objects.link(hobj)
cbm = bmesh.new()
hexa([V(x, y, z) for z in (-37.5, -2.5) for (x, y) in ((240, -25), (310, -25), (310, 25), (240, 25))], "HANGAR", bm=cbm)
cut = bpy.data.meshes.new("cut_tmp"); cbm.to_mesh(cut); cbm.free()
cobj = bpy.data.objects.new("cut_tmp", cut); scene.collection.objects.link(cobj)

# ================================================================ 2. BOW: mouth frame, hangar interior, prow brow, mandibles, crests
# Everything at X > 300 stays at |Y| >= 28.3 or Z >= +0.3 or Z <= -40.3: the launch channel is empty.
box((300, -34, 0.3), (305, 34, 6.5), "METAL", bevel=0.4, coll="bay_frame_lintel")
box((300, -34, 0.3), (304.6, 34, 0.9), "AMBER")
for s in (1, -1):
    ybox(300, 305, s, 28.3, 34, -46, 0.3, "METAL", bevel=0.4, coll="bay_frame_post_" + ("stbd" if s > 0 else "port"))
    ybox(300, 304.6, s, 28.3, 28.9, -40, 0, "AMBER")
box((300, -34, -46), (305, 34, -40.3), "METAL", bevel=0.4, coll="bay_frame_sill")
box((300, -34, -40.9), (304.6, 34, -40.3), "AMBER")
for s in (1, -1):
    ybox(296, 299.4, s, 24.7, 25, -37, -3, "AMBER")                        # mouth rim lights
    for k in range(10):
        x0 = 243 + k * 5.7
        ybox(x0, x0 + 3.2, s, 20, 21, -2.8, -2.5, "AMBER")                  # ceiling lights
        ybox(x0, x0 + 2.0, s, 10, 10.8, -37.5, -37.25, "AMBER")             # deck guide lights
    for k in range(6):
        x0 = 246 + k * 9; ybox(x0, x0 + 1.2, s, 24.4, 25, -37.5, -2.5, "METAL")   # wall ribs
box((240, -25, -37.5), (241.5, 25, -2.5), "HANGAR")                          # aft bulkhead
box((240, -14, -37.5), (242.2, 14, -12), "METAL", bevel=0.3)
box((240, -12, -36), (242.4, 12, -14), "HANGAR")
box((240, -22, -9.5), (242.3, 22, -8.8), "AMBER")
box((240, -22, -37.5), (242.3, -16, -10.5), "METAL", bevel=0.3); box((240, 16, -37.5), (242.3, 22, -10.5), "METAL", bevel=0.3)
brow = [V(294, -42, 4), V(312, -17, 8), V(312, 17, 8), V(294, 42, 4), V(294, -40, 47), V(312, -14, 37), V(312, 14, 37), V(294, 40, 47)]
hexa(brow, "ARMOR", {"+u": "ARMOR_DARK"}, bevel=0.6); COLL.append((brow, "prow_brow"))
for s in (1, -1):
    for (y0, y1) in ((3, 7.5), (9.5, 13.5)): ybox(312, 312.4, s, y0, y1, 21, 22.6, "WHITE")   # eye slits
    for k in range(3):
        obox(V(300 + k * 2, s * (36 - k * 6), 26 - k * 2), V(1, -s * 0.9, 0), 6, 1.0, 14 - k * 2, "ARMOR_DARK", bevel=0.4, shear=2.5)
box((312, -2, 29), (312.4, 2, 31.4), "WHITE")
for s in (1, -1):   # mandible jaws (|Y| >= 30)
    m = mirror8([V(286, s * 30, -48), V(334, s * 31, -40), V(334, s * 40, -40), V(286, s * 58, -48),
                 V(286, s * 30, -6), V(334, s * 31, -31), V(334, s * 40, -31), V(286, s * 58, -6)], s)
    hexa(m, "ARMOR_DARK", {"top": "ARMOR"}, bevel=0.5); COLL.append((m, "mandible_" + ("stbd" if s > 0 else "port")))
    ybox(322, 333.2, s, 31.2, 39.6, -33.2, -32.4, "AMBER")
    obox(V(268, s * 30, 53), V(1, 0, 0), 26, 4, 3.5, "ARMOR", {"top": "TRIM_RED"}, bevel=0.5, shear=-4)    # crests framing the trench entry
    obox(V(266, s * 45, 52), V(1, 0, 0), 24, 9, 2.5, "ARMOR_DARK", bevel=0.5, shear=-3)
    for k in range(4): ybox(246 + k * 12, 252 + k * 12, s, 35, 55, 54.6, 55.4, "METAL")

# ================================================================ 3. FLANK ARMOUR SHINGLES, AMBER SEAMS, EYES, BANNERS
BANNERS = (215.0, -5.0, -215.0)
ROWS = ((-16, -4.2), (-2.2, 9.8), (11.8, 23.8), (25.8, 34.5))
ang = math.atan2(14, 11)
for s in (1, -1):
    for (z0, z1) in ROWS:
        x = -292.0
        while x < 236:
            x1 = min(x + RNG.uniform(16, 24), 238)
            obox(V((x + x1) / 2, s * 78.7, (z0 + z1) / 2), V(1, 0, 0), (x1 - x) / 2 - 0.6, 0.7, (z1 - z0) / 2, "ARMOR", bevel=0.35, shear=1.6)
            x = x1 + 0.8
    for (za, zb) in ((-4.2, -2.2), (9.8, 11.8), (23.8, 25.8)):
        x = -290.0
        while x < 234:
            L = RNG.uniform(6, 22)
            if RNG.random() < 0.28: ybox(x, x + L, s, 78.0, 78.5, za + 0.55, zb - 0.55, "AMBER")
            x += L + RNG.uniform(2, 10)
    nrm = V(0, s * math.sin(ang), math.cos(ang)).normalized()
    x = -290.0
    while x < 236:   # upper chamfer plates + white eye slits
        x1 = min(x + RNG.uniform(18, 26), 238)
        obox(V((x + x1) / 2, s * 72.5, 43.0) + nrm * 0.5, V(1, 0, 0), (x1 - x) / 2 - 0.6, 6.5, 0.6, "ARMOR_DARK", bevel=0.3, w=nrm)
        if RNG.random() < 0.5: obox(V((x + x1) / 2, s * 75.8, 39.0) + nrm * 0.2, V(1, 0, 0), 2.2, 0.35, 0.35, "WHITE", w=nrm)
        x = x1 + 0.6
    n2 = V(0, s * 16, -10).normalized()
    x = -290.0
    while x < 236:   # lower chamfer plates
        x1 = min(x + RNG.uniform(18, 28), 238)
        obox(V((x + x1) / 2, s * 73, -26) + n2 * 0.5, V(1, 0, 0), (x1 - x) / 2 - 0.6, 9.0, 0.6, "ARMOR_DARK", bevel=0.3, w=n2)
        x = x1 + 0.6
    for bx in BANNERS:   # deep-red vertical banners: outward face carries the trident
        ybox(bx - 10, bx + 10, s, 79.3, 80.0, -24, 34, "TRIM_RED", {"+v" if s > 0 else "-v": "BANNER"}, bevel=0.15)
        ybox(bx - 11, bx + 11, s, 79.3, 80.0, 34, 37, "METAL", bevel=0.15)
        ybox(bx - 11, bx + 11, s, 79.3, 80.0, -27, -24, "METAL", bevel=0.15)
        obox(V(bx, s * 72.5, 43.4) + nrm * 0.9, V(1, 0, 0), 10, 6.8, 0.5, "TRIM_RED", w=nrm)

# ================================================================ 4. FLYABLE DORSAL TRENCH
TX0, TX1 = -180.0, 240.0
for s in (1, -1):
    inner = "+v" if s < 0 else "-v"
    x = TX0
    while x < TX1 - 0.1:   # armoured wall, 14 x 30 m segments, Y +/-25..34, Z 50..80
        x1 = min(x + 30, TX1); ybox(x, x1, s, 25, 34, 50, 80, "ARMOR", {inner: "WALL", "top": "METAL"}, bevel=0.25); x = x1
    COLL.append(([V(x, s * y, z) for x in (TX0, TX1) for y in (25, 34) for z in (50, 80)], "trench_wall_" + ("stbd" if s > 0 else "port")))
    ybox(TX0, TX1, s, 24.6, 34.6, 80, 81.2, "ARMOR_DARK", bevel=0.2)          # rim cap
    for xr0 in range(int(TX0) + 26, int(TX1) - 4, 60): ybox(xr0, xr0 + 5, s, 24.5, 34.7, 80.4, 81.6, "TRIM_RED")
    xr = TX0 + 2
    while xr < TX1 - 1:                                                       # inner ribs (|Y| >= 24.2)
        ybox(xr - 0.8, xr + 0.8, s, 24.2, 25, 50, 79.4, "METAL", bevel=0.2); xr += 15
    for z0 in (64.5, 69.0): ybox(TX0 + 1, TX1 - 1, s, 24.4, 25, z0, z0 + 1.1, "METAL")   # pipe runs
    x = TX0 + 4
    while x < TX1 - 6:                                                        # dashed amber light course
        L = RNG.uniform(4, 9); ybox(x, x + L, s, 24.75, 25, 56.8, 57.5, "AMBER"); x += L + RNG.uniform(5, 12)
    for k in range(int((TX1 - TX0) / 4)):                                     # wall greebles
        xg = TX0 + 3 + k * 4 + RNG.uniform(-1, 1)
        if RNG.random() < 0.55:
            zg = RNG.uniform(51, 63); ybox(xg, xg + RNG.uniform(1, 2.6), s, 24.45, 25, zg, zg + RNG.uniform(1, 4), "METAL")
    xb = TX0 + 10
    while xb < TX1 - 4:                                                       # outer buttresses
        pts = [V(xb - 1.6, s * 34, 50), V(xb + 1.6, s * 34, 50), V(xb + 1.6, s * 41.5, 50), V(xb - 1.6, s * 41.5, 50),
               V(xb - 1.6, s * 34, 77), V(xb + 1.6, s * 34, 77), V(xb + 1.6, s * 41.5, 52.5), V(xb - 1.6, s * 41.5, 52.5)]
        hexa(mirror8(pts, s), "ARMOR_DARK", bevel=0.25); xb += 24
TRENCH_TURRETS = [(-150, -17, 77), (-60, 17, 77), (30, -17, 77), (120, 17, 77)]
for (tx, ty, tz) in TRENCH_TURRETS:   # wall-rim brackets; they stay at |Y| >= 23.69
    s = 1 if ty > 0 else -1
    ybox(tx - 4, tx + 4, s, 23.7, 25, 71, 80.6, "METAL", bevel=0.25)
    ybox(tx - 2.5, tx + 2.5, s, 23.7, 23.9, 74, 78.5, "TRIM_RED")
    ybox(tx - 3, tx + 3, s, 25, 33, 80.6, 82.4, "METAL", bevel=0.25)
    ybox(tx - 0.6, tx + 0.6, s, 23.69, 23.75, 72, 73, "AMBER")
LANES = ((-24.6, -14.2), (-12.8, -1.4), (1.4, 12.8), (14.2, 24.6))
x = -298.0
while x < 298:   # floor plates, top flush at Z = 50
    x1 = min(x + 9.4, 299.5)
    for (y0, y1) in LANES: box((x, y0, 48.5), (x1, y1, 50.0), "METAL", {"top": "FLOOR"}, bevel=0.12)
    x += 10.0
x = -297.0
while x < 297: box((x, -0.6, 48.5), (x + 6, 0.6, 49.3), "AMBER"); x += 10.0   # recessed glowing centreline conduit
for s in (1, -1):
    ybox(-298, 298, s, 13.0, 14.0, 48.5, 49.4, "METAL"); ybox(-298, 298, s, 1.0, 1.3, 48.5, 49.6, "METAL")

# ================================================================ 5. DORSAL SIDE DECKS, STERN DECK, GREEBLES
deck_hi = {}
for s in (1, -1):
    x = -168.0
    while x < 236:
        x1 = min(x + RNG.uniform(14, 30), 238)
        ya, yb = 42 + RNG.uniform(0, 3), 62 + RNG.uniform(0, 5); h1 = RNG.uniform(3, 9)
        ybox(x, x1, s, ya, yb, 50, 50 + h1, "ARMOR", {"top": "ARMOR_DARK"}, bevel=0.45)
        top = 50 + h1
        if RNG.random() < 0.55 and (x1 - x) > 12:
            xa, xb2 = x + RNG.uniform(1.5, 4), x1 - RNG.uniform(1.5, 4)
            yc0, yc1 = ya + RNG.uniform(2, 5), yb - RNG.uniform(2, 6); h2 = RNG.uniform(2, 5)
            ybox(xa, xb2, s, yc0, yc1, top, top + h2, "ARMOR_DARK", {"top": "METAL"}, bevel=0.35); top += h2
            if RNG.random() < 0.4: ybox(xa + 1, xb2 - 1, s, yc0 - 0.3, yc0 + 0.2, top - 2, top - 1.3, "AMBER")
        half = 0 if (x + x1) / 2 < 34 else 1
        deck_hi[(s, half)] = max(deck_hi.get((s, half), 50), top)
        for g in range(int((x1 - x) * (yb - ya) / 20)):   # roof greebles
            gx = RNG.uniform(x + 1, x1 - 3); gy = RNG.uniform(ya + 1, yb - 3)
            gw, gl, gh = RNG.uniform(0.8, 3.2), RNG.uniform(0.8, 4.5), RNG.uniform(0.3, 1.8)
            ybox(gx, gx + gl, s, gy, gy + gw, 50 + h1, 50 + h1 + gh, RNG.choice(("METAL", "ARMOR_DARK", "METAL")), bevel=0.08 if gh > 0.6 else 0)
        x = x1 + RNG.uniform(0.6, 2.5)
    for half, (xa, xb2) in enumerate(((-170.0, 34.0), (34.0, 240.0))):
        zt = deck_hi.get((s, half), 60)
        COLL.append(([V(xx, s * yy, zz) for xx in (xa, xb2) for yy in (41.5, 68) for zz in (50, zt)], f"dorsal_deck_{'stbd' if s > 0 else 'port'}_{'aft' if half == 0 else 'fwd'}"))
    for k in range(26):
        vx = -280 + k * 20 + RNG.uniform(-3, 3); ybox(vx, vx + 4, s, 66, 69, 50, 51.2, "METAL")
    for (ax, ay, ah) in ((-120, 56, 18), (-40, 50, 12), (60, 58, 16), (150, 52, 11), (205, 60, 9)):
        obox(V(ax, s * ay, 58 + ah / 2), V(0, 0, 1), ah / 2 + 4, 0.35, 0.35, "METAL", w=V(1, 0, 0), taper=0.4)
    # tiered stern citadel flanking the bridge (v3: stepped superstructure rising to the spire)
    for (xa, xb2, ya, yb, z0, z1, nm_) in ((-296, -186, 21, 62, 50, 60, "a"), (-288, -196, 20.5, 48, 60, 70, "b"), (-280, -206, 20, 34, 70, 80, "c")):
        ybox(xa, xb2, s, ya, yb, z0, z1, "ARMOR", {"top": "ARMOR_DARK"}, bevel=0.5, coll=f"stern_citadel_{'stbd' if s > 0 else 'port'}_{nm_}")
        for k in range(int((xb2 - xa) / 9)):
            if RNG.random() < 0.45: ybox(xa + 3 + k * 9, xa + 7 + k * 9, s, yb - 0.1, yb + 0.25, z1 - 4, z1 - 3.2, "AMBER" if RNG.random() < 0.6 else "WHITE")
        for g in range(int((xb2 - xa) * (yb - ya) / 40)):
            gx = RNG.uniform(xa + 1, xb2 - 4); gy = RNG.uniform(ya + 1, yb - 3)
            ybox(gx, gx + RNG.uniform(1, 4), s, gy, gy + RNG.uniform(1, 3), z1, z1 + RNG.uniform(0.3, 1.6), "METAL")
    for (fx, fh) in ((-276, 34), (-252, 26)):
        fin = [V(fx - 8, s * 60, 60), V(fx + 8, s * 60, 60), V(fx + 8, s * 62, 60), V(fx - 8, s * 62, 60),
               V(fx - 11, s * 60, 60 + fh), V(fx - 4, s * 60, 60 + fh * 0.7), V(fx - 4, s * 62, 60 + fh * 0.7), V(fx - 11, s * 62, 60 + fh)]
        hexa(mirror8(fin, s), "ARMOR_DARK", {"top": "TRIM_RED"}, bevel=0.3)
box((-298, -21, 50), (-233, 21, 52.5), "METAL", bevel=0.3)

# ================================================================ 6. BRIDGE / CITADEL + SPIRE (bulk centred ~(-195, 0, 80))
def frustum(x0, x1, y, z0, z1, top=0.9, reg="ARMOR", fr=None, bevel=0.5, coll=None):
    xc = (x0 + x1) / 2; hx = (x1 - x0) / 2; yt = y * top
    pts = [V(x0, -y, z0), V(x1, -y, z0), V(x1, y, z0), V(x0, y, z0),
           V(xc - hx * top, -yt, z1), V(xc + hx * top, -yt, z1), V(xc + hx * top, yt, z1), V(xc - hx * top, yt, z1)]
    hexa(pts, reg, fr, bevel)
    if coll: COLL.append((pts, coll))
frustum(-232.5, -180.0, 20, 50, 80, 0.94, "ARMOR", {"top": "ARMOR_DARK"}, 0.6, "bridge_tier1")
brow_b = [V(-196, -18, 87), V(-157.5, -12, 88.5), V(-157.5, 12, 88.5), V(-196, 18, 87), V(-196, -17, 104), V(-162, -11, 99), V(-162, 11, 99), V(-196, 17, 104)]
hexa(brow_b, "ARMOR_DARK", {"top": "ARMOR"}, 0.5); COLL.append((brow_b, "bridge_forward_brow"))
frustum(-228, -186, 16.5, 80, 96, 0.9, "ARMOR", {"top": "METAL"}, 0.5)
frustum(-222, -192, 11.5, 96, 110, 0.85, "ARMOR_DARK", {"top": "METAL"}, 0.4)
COLL.append(([V(x, y, z) for x in (-228, -186) for y in (-16.5, 16.5) for z in (80, 96)] + [V(x, y, 110) for x in (-222, -192) for y in (-11.5, 11.5)], "bridge_tiers_2_3"))
frustum(-214, -200, 5.5, 110, 150, 0.4, "ARMOR_DARK", {"top": "METAL"}, 0.3, "bridge_spire_base")
obox(V(-207, 0, 166), V(0, 0, 1), 18, 0.8, 0.8, "METAL", w=V(1, 0, 0), taper=0.3)
for s in (1, -1):
    obox(V(-212, s * 8, 124), V(0, 0, 1), 16, 0.6, 0.6, "METAL", w=V(1, 0, 0), taper=0.3)
    obox(V(-196, s * 13, 110), V(0, 0, 1), 12, 0.5, 0.5, "METAL", w=V(1, 0, 0), taper=0.3)
    fin = [V(-230, s * 18.6, 78), V(-200, s * 18.6, 78), V(-200, s * 20, 78), V(-230, s * 20, 78),
           V(-232, s * 18.6, 116), V(-222, s * 18.6, 100), V(-222, s * 20, 100), V(-232, s * 20, 116)]
    hexa(mirror8(fin, s), "ARMOR_DARK", {"+v" if s > 0 else "-v": "TRIM_RED"}, 0.2)
    for k in range(4): ybox(-226 + k * 9, -220 + k * 9, s, 16.6, 16.9, 90, 91.2, "WHITE")
    ybox(-230, -182, s, 19.6, 20.2, 66, 66.8, "AMBER"); ybox(-220, -194, s, 11.2, 11.5, 102, 102.8, "AMBER")
box((-157.6, -9, 92), (-157.2, 9, 93.6), "WHITE")
box((-182.0, -24.9, 50), (-180.6, 24.9, 79.5), "HANGAR")      # trench aft bulkhead (under the brow), stays at X <= -180
box((-180.6, -1.5, 52), (-180.2, 1.5, 76), "AMBER")
box((-180.6, -20, 52), (-180.1, 20, 53), "AMBER")

# ================================================================ 7. STERN ENGINE BANKS
box((-306, -74, -40), (-299.5, 74, 44), "METAL", {"-u": "RECESS"}, bevel=0.6)
eng = []
for y in (-48, -24, 0, 24, 48):
    eng += cyl(V(-306, y, -8), 10.5, 11.6, -16, "METAL", segs=24, axis='X', cap1="RECESS")
    cyl(V(-321.6, y, -8), 9.3, 9.3, -0.45, "ENGINE", segs=24, axis='X', cap1="ENGINE")
    cyl(V(-306, y, -8), 12.4, 12.4, -2.2, "ARMOR_DARK", segs=24, axis='X')
for (y, z, r) in ((-44, 26, 6.5), (-16, 28, 6.5), (16, 28, 6.5), (44, 26, 6.5)):
    eng += cyl(V(-306, y, z), r, r * 1.1, -11, "METAL", segs=20, axis='X', cap1="RECESS")
    cyl(V(-316.8, y, z), r * 0.88, r * 0.88, -0.45, "ENGINE", segs=20, axis='X', cap1="ENGINE")
COLL.append((eng + [V(x, y, z) for x in (-306, -299.5) for y in (-74, 74) for z in (-40, 44)], "stern_engine_block"))
for s in (1, -1): obox(V(-290, s * 76.5, 10), V(1, 0, 0), 12, 1.5, 26, "ARMOR_DARK", bevel=0.4, shear=-3)

# ================================================================ 8. VENTRAL KEEL BLADE, FINS, SPIRES
KEEL = [(200, -52), (150, -60), (90, -68), (30, -74), (-30, -78), (-90, -84), (-150, -88), (-190, -80), (-220, -56)]
krings, kpts = [], []
for (x, zb) in KEEL:
    prof = [(0, -48), (3.2, -50), (2.4, (zb - 50) / 2 - 2), (0.6, zb), (-0.6, zb), (-2.4, (zb - 50) / 2 - 2), (-3.2, -50)]
    krings.append((x, prof)); kpts += [V(x, y, z) for (y, z) in prof]
loft(list(reversed(krings)), "ARMOR_DARK", BM, cap_regs=("METAL", "METAL"))
COLL.append((kpts, "ventral_keel_blade"))
x = -200.0
while x < 185:
    if RNG.random() < 0.5: box((x, -2.9, -60), (x + RNG.uniform(3, 7), 2.9, -59.3), "AMBER")
    x += 14
for s in (1, -1):
    fin = [V(-150, s * 38, -49), V(-30, s * 38, -49), V(-30, s * 41, -49), V(-150, s * 41, -49),
           V(-140, s * 38, -78), V(-95, s * 38, -66), V(-95, s * 41, -66), V(-140, s * 41, -78)]
    fin = mirror8(fin, s); hexa(fin, "ARMOR_DARK", {"+v" if s > 0 else "-v": "TRIM_RED"}, 0.3)
    COLL.append((fin, "ventral_fin_" + ("stbd" if s > 0 else "port")))
SPIRES = [(-60, 0, 9, 78), (-95, 26, 6.5, 50), (-95, -26, 6.5, 50), (-130, 44, 5.5, 38), (-130, -44, 5.5, 38),
          (-25, 40, 5, 30), (-25, -40, 5, 30), (30, 50, 4.5, 22), (30, -50, 4.5, 22), (110, 38, 4.5, 20), (110, -38, 4.5, 20),
          (-178, 22, 6, 34), (-178, -22, 6, 34), (215, 44, 4, 15), (215, -44, 4, 15), (160, 0, 5, 18)]
for (sx, sy, r, L) in SPIRES:
    p = pyramid(V(sx, sy, -49), r, (sx - L * 0.12, sy, -50 - L), "ARMOR_DARK")
    if L > 70: COLL.append((p, "ventral_spire_main"))
    cyl(V(sx, sy, -53), r * 1.25, r * 1.1, 3, "METAL", segs=12)
    if L > 30: box((sx - 0.4 - L * 0.06, sy - 0.4, -50 - L * 0.5), (sx + 0.4 - L * 0.06, sy + 0.4, -50 - L * 0.42), "AMBER")

# ================================================================ 9. HULL TURRET SPONSONS
HULL_TURRETS = [(-180, -80, 55), (180, 80, 55)]
for (hx, hy, hz) in HULL_TURRETS:
    s = 1 if hy > 0 else -1
    ybox(hx - 9, hx + 9, s, 70, 88, 34, 52.8, "ARMOR_DARK", {"top": "METAL"}, bevel=0.6, coll=f"hull_turret_sponson_{'stbd' if s > 0 else 'port'}")
    br = [V(hx - 7, s * 79, 14), V(hx + 7, s * 79, 14), V(hx + 7, s * 86, 34), V(hx - 7, s * 86, 34),
          V(hx - 7, s * 79, 35), V(hx + 7, s * 79, 35), V(hx + 7, s * 86, 35), V(hx - 7, s * 86, 35)]
    hexa(mirror8(br, s), "ARMOR", bevel=0.4)
    cyl(V(hx, hy, 52.8), 6.5, 6.0, 1.4, "METAL", segs=24)
    ybox(hx - 9.2, hx + 9.2, s, 87.8, 88.3, 44, 45, "AMBER")

# ================================================================ 10. OUTRIGGER ARMS + GENERATOR PLINTHS + GUARD PERCHES
GENS = [(i, V(275 * (1 if math.cos(math.radians(45 + 90 * i)) > 0 else -1), 275 * (1 if math.sin(math.radians(45 + 90 * i)) > 0 else -1), 0), 45 + 90 * i) for i in range(4)]
for (i, C, yaw) in GENS:
    sx = 1 if C.x > 0 else -1; sy = 1 if C.y > 0 else -1; tag = f"gen{i}"
    ybox(sx * 130 - 17, sx * 130 + 17, sy, 76, 96, -18, 14, "ARMOR_DARK", {"top": "ARMOR"}, bevel=0.8, coll=f"outrigger_shoulder_{tag}")
    ybox(sx * 130 - 17.4, sx * 130 + 17.4, sy, 95.6, 96.4, 6, 10, "TRIM_RED")
    P0 = V(sx * 132, sy * 90, -2); P1 = V(sx * 200, sy * 182, -4)
    to_c = (C - P1); to_c.z = 0; P2 = C - to_c.normalized() * 16; P2.z = -6
    tapered_beam(P0, P1 + (P1 - P0).normalized() * 6, 24, 18, 15, 12, "ARMOR", {"top": "ARMOR_DARK"}, 0.6, f"outrigger_arm_{tag}_inner")
    tapered_beam(P1, P2, 18, 14, 12, 10, "ARMOR", {"top": "ARMOR_DARK"}, 0.6, f"outrigger_arm_{tag}_outer")
    cyl(P1 + V(0, 0, -8), 12, 12, 16, "ARMOR_DARK", segs=16, cap1="METAL")
    tapered_beam(V(P0.x, P0.y, 4.75), V(P1.x, P1.y, 1.75), 3.2, 3.2, 0.5, 0.5, "TRIM_RED", bevel=0)
    tapered_beam(V(P1.x, P1.y, 1.75), V(P2.x, P2.y, -1.25), 3.2, 3.2, 0.5, 0.5, "TRIM_RED", bevel=0)
    d = (P1 - P0); u = d.normalized(); pv = V(-u.y, u.x, 0) * 0.8
    fa, fb = P0 + d * 0.15, P0 + d * 0.85
    fin = [V(fa.x, fa.y, 3.5) - pv, V(fb.x, fb.y, 1.5) - pv, V(fb.x, fb.y, 1.5) + pv, V(fa.x, fa.y, 3.5) + pv,
           V(fa.x, fa.y, 14) - pv, V(fb.x, fb.y, 4.5) - pv, V(fb.x, fb.y, 4.5) + pv, V(fa.x, fa.y, 14) + pv]
    hexa(fin, "ARMOR_DARK", {"top": "TRIM_RED"}, 0.2)
    for t in (0.2, 0.4, 0.6, 0.8):
        for (a, b, ww, zc) in ((P0, P1, 21, -2), (P1, P2, 16, -5)):
            p = a + (b - a) * t; uu = (b - a).normalized(); vv = V(0, 0, 1).cross(uu).normalized()
            for sv in (-1, 1): obox(p + vv * sv * (ww / 2 + 0.05) + V(0, 0, -1), uu, 3.0, 0.15, 0.35, "AMBER")
    for t in (0.25, 0.6):
        p = P1 + (P2 - P1) * t
        tapered_beam(p + V(0, 0, -5), P1 + (P2 - P1) * (t + 0.25) + V(0, 0, -16), 3, 3, 3, 3, "METAL", bevel=0.2)
    pad = cyl(C + V(0, 0, -9), 24, 24, 8.5, "ARMOR_DARK", segs=40, cap0="METAL", cap1="ARMOR_DARK")     # plinth top Z = -0.5
    cyl(C + V(0, 0, -4.2), 24.7, 24.7, 2.2, "METAL", segs=40)
    hub = cyl(C + V(0, 0, -22), 5.5, 15, 13, "METAL", segs=20)
    COLL.append((pad + hub, f"generator_plinth_{tag}"))
    for k in range(20):
        a = 2 * math.pi * k / 20
        obox(C + V(math.cos(a) * 24.75, math.sin(a) * 24.75, -1.6), V(-math.sin(a), math.cos(a), 0), 1.6, 0.25, 0.4, "AMBER")
    for k in range(8):
        a = 2 * math.pi * (k + 0.5) / 8
        obox(C + V(math.cos(a) * 21, math.sin(a) * 21, -12.5), V(math.cos(a), math.sin(a), 0), 4.5, 1.2, 3.5, "ARMOR", bevel=0.2, taper=0.6)
    yr = math.radians(yaw); T = V(-math.sin(yr), math.cos(yr), 0)
    for k in (-1, 1):   # guard perches at +/-30 m along the tangent, mount top Z 14.2 (turret at Z 15)
        Pp = C + T * (k * 30)
        tapered_beam(C + T * (k * 18) + V(0, 0, -5), Pp + V(0, 0, -5), 7, 6, 5, 5, "ARMOR_DARK", {"top": "TRIM_RED"}, 0.3)
        per = cyl(Pp + V(0, 0, -9), 4.8, 3.6, 21.6, "ARMOR_DARK", segs=16, cap1="METAL")
        top = cyl(Pp + V(0, 0, 12.6), 5.6, 5.6, 1.6, "METAL", segs=20)
        COLL.append((per + top, f"guard_perch_{tag}_{'a' if k < 0 else 'b'}"))

# ================================================================ 11. assemble, mirror to Blender axes, UVs, material
me = bpy.data.meshes.new(NAME); BM.to_mesh(me); BM.free()
obj = bpy.data.objects.new(NAME, me); scene.collection.objects.link(obj)
mod = hobj.modifiers.new("bay", 'BOOLEAN'); mod.operation = 'DIFFERENCE'; mod.solver = 'EXACT'; mod.object = cobj
mats = [bpy.data.materials.new("R_" + n) for n in REG]
for o in (obj, hobj, cobj):
    for m in mats: o.data.materials.append(m)
bpy.context.view_layer.objects.active = hobj; hobj.select_set(True)
bpy.ops.object.modifier_apply(modifier="bay"); bpy.data.objects.remove(cobj)
bpy.ops.object.select_all(action='DESELECT')
obj.select_set(True); hobj.select_set(True); bpy.context.view_layer.objects.active = obj
bpy.ops.object.join()

bm = bmesh.new(); bm.from_mesh(obj.data)
bmesh.ops.scale(bm, vec=(1, -1, 1), verts=bm.verts)
bmesh.ops.reverse_faces(bm, faces=bm.faces)
bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0005)
bmesh.ops.dissolve_degenerate(bm, edges=bm.edges, dist=0.0005)
bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
uvl = bm.loops.layers.uv.new("UVMap")
urng = random.Random(77)
for f in bm.faces:
    reg = REG[f.material_index]; u0, v0, u1, v1, ww, wh, mode = ATLAS[reg]
    n = f.normal; ax = max(range(3), key=lambda i: abs(n[i])); sgn = 1 if n[ax] >= 0 else -1
    if ax == 0: U, Vv = V(0, sgn, 0), V(0, 0, 1)        # U x V = face normal: no mirrored texels
    elif ax == 1: U, Vv = V(-sgn, 0, 0), V(0, 0, 1)
    else: U, Vv = V(1, 0, 0), V(0, sgn, 0)
    co = [(l.vert.co.dot(U), l.vert.co.dot(Vv)) for l in f.loops]
    us = [c[0] for c in co]; vs = [c[1] for c in co]
    umin, vmin = min(us), min(vs); eu, ev = max(max(us) - umin, 1e-6), max(max(vs) - vmin, 1e-6)
    m = 0.012
    if mode == "fit":
        for l, (cu, cv) in zip(f.loops, co):
            a = m + (cu - umin) / eu * (1 - 2 * m); b = m + (cv - vmin) / ev * (1 - 2 * m)
            l[uvl].uv = (u0 + a * (u1 - u0), v0 + b * (v1 - v0))
    else:
        fu, fv = eu / ww, ev / wh
        k = min(1.0, (1 - 2 * m) / max(fu, fv, 1e-9))
        ou = m + urng.random() * max(0.0, (1 - 2 * m) - fu * k); ov = m + urng.random() * max(0.0, (1 - 2 * m) - fv * k)
        for l, (cu, cv) in zip(f.loops, co):
            l[uvl].uv = (u0 + (ou + (cu - umin) / ww * k) * (u1 - u0), v0 + (ov + (cv - vmin) / wh * k) * (v1 - v0))
bmesh.ops.triangulate(bm, faces=bm.faces, quad_method='BEAUTY', ngon_method='BEAUTY')
region_area = {}
for f in bm.faces: region_area[REG[f.material_index]] = region_area.get(REG[f.material_index], 0) + f.calc_area()
bm.to_mesh(obj.data); bm.free()
me = obj.data
for p in me.polygons: p.use_smooth = False
tris = len(me.polygons)
me.materials.clear()
mat = bpy.data.materials.new("M_DominionMothership"); mat.use_nodes = True; me.materials.append(mat)
nt = mat.node_tree; nt.nodes.clear()
outn = nt.nodes.new("ShaderNodeOutputMaterial"); bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled"); nt.links.new(bsdf.outputs[0], outn.inputs[0])
def img(name, cs):
    n = nt.nodes.new("ShaderNodeTexImage"); n.image = bpy.data.images.load(os.path.join(TEX, name)); n.image.colorspace_settings.name = cs; return n
bc = img("T_DominionMothership_BC.png", "sRGB"); nt.links.new(bc.outputs[0], bsdf.inputs["Base Color"])
orm = img("T_DominionMothership_ORM.png", "Non-Color"); sep = nt.nodes.new("ShaderNodeSeparateColor"); nt.links.new(orm.outputs[0], sep.inputs[0])
nt.links.new(sep.outputs[1], bsdf.inputs["Roughness"]); nt.links.new(sep.outputs[2], bsdf.inputs["Metallic"])
ngl = img("T_DominionMothership_N_OpenGL.png", "Non-Color"); nm = nt.nodes.new("ShaderNodeNormalMap")
nt.links.new(ngl.outputs[0], nm.inputs["Color"]); nt.links.new(nm.outputs[0], bsdf.inputs["Normal"])
em = img("T_DominionMothership_E.png", "sRGB"); nt.links.new(em.outputs[0], bsdf.inputs["Emission Color"]); bsdf.inputs["Emission Strength"].default_value = 1.0

# ---------------------------------------------------------------- UCX convex hulls (mirrored like the mesh)
ucx = []
for idx, (pts, label) in enumerate(COLL):
    cb = bmesh.new()
    for p in pts: cb.verts.new(V(p.x, -p.y, p.z))
    res = bmesh.ops.convex_hull(cb, input=cb.verts, use_existing_faces=False)
    for g in res.get("geom_interior", []) + res.get("geom_unused", []):
        if isinstance(g, bmesh.types.BMVert) and g.is_valid: cb.verts.remove(g)
    bmesh.ops.remove_doubles(cb, verts=cb.verts, dist=0.001)
    bmesh.ops.recalc_face_normals(cb, faces=cb.faces); bmesh.ops.triangulate(cb, faces=cb.faces)
    nm_ = f"UCX_{NAME}_{idx:02d}"
    cm = bpy.data.meshes.new(nm_); cb.to_mesh(cm); cb.free()
    co = bpy.data.objects.new(nm_, cm); scene.collection.objects.link(co); co.display_type = 'WIRE'; co.hide_render = True
    ucx.append({"name": nm_, "part": label, "verts": len(cm.vertices), "tris": len(cm.polygons)})

# ---------------------------------------------------------------- sockets (UE cm -> Blender m, yaw negated by the Y mirror)
SOCKETS = [("SOCKET_Reactor", (16500, 0, 6500), 0)]
for (i, C, yaw) in GENS: SOCKETS.append((f"SOCKET_Generator_{i}", (C.x * 100, C.y * 100, 0), yaw))
for i, (tx, ty, tz) in enumerate(TRENCH_TURRETS): SOCKETS.append((f"SOCKET_TrenchTurret_{i}", (tx * 100, ty * 100, tz * 100), 0))
for i, (hx, hy, hz) in enumerate(HULL_TURRETS): SOCKETS.append((f"SOCKET_HullTurret_{i}", (hx * 100, hy * 100, hz * 100), 0))
SOCKETS.append(("SOCKET_Bay", (30000, 0, -2000), 0))
for (n_, (x, y, z), yaw) in SOCKETS:
    e = bpy.data.objects.new(n_, None); scene.collection.objects.link(e)
    e.empty_display_type = 'ARROWS'; e.empty_display_size = 6
    e.parent = obj; e.matrix_parent_inverse = Matrix.Identity(4)
    e.location = (x / 100.0, -y / 100.0, z / 100.0); e.rotation_euler = (0, 0, math.radians(-yaw))

bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "SM_DominionMothership_build.blend"))
bpy.ops.object.select_all(action='DESELECT')
for o in scene.objects:
    if o.name == NAME or o.name.startswith("UCX_") or o.name.startswith("SOCKET_"): o.select_set(True)
bpy.context.view_layer.objects.active = obj
bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, NAME + ".fbx"), use_selection=True, object_types={'MESH', 'EMPTY'},
    apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS', global_scale=1.0, axis_forward='-Z', axis_up='Y',
    mesh_smooth_type='FACE', use_tspace=True, use_triangles=True, use_mesh_modifiers=True, add_leaf_bones=False,
    bake_anim=False, path_mode='STRIP', embed_textures=False, use_custom_props=False)
bpy.ops.object.select_all(action='DESELECT')
for o in scene.objects:
    if o.name == NAME or o.name.startswith("SOCKET_"): o.select_set(True)
bpy.ops.export_scene.gltf(filepath=os.path.join(OUT, NAME + ".glb"), export_format='GLB', use_selection=True,
    export_apply=True, export_yup=True, export_tangents=True, export_image_format='AUTO', export_extras=False)
json.dump({"tris": tris, "verts": len(me.vertices), "region_area_m2": {k: round(v, 1) for k, v in region_area.items()}, "ucx": ucx,
           "sockets": [{"name": n_, "ue_cm": [round(c, 3) for c in p], "yaw_deg": yw} for (n_, p, yw) in SOCKETS]},
          open(os.path.join(OUT, "build_report.json"), "w"), indent=1)
print("DONE tris", tris, "ucx", len(ucx))

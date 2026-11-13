# SPDX-License-Identifier: MIT
# Triad Drift - Outpost: make Grok Bot's research outpost and console walkable and game-ready.
#   blender --background --factory-startup --python scripts/op_build.py -- [--tag r1] [--export]
# Reads (never writes) ArtSource/EngAI/Props/VerdanceOutpost/mesh_r1 (or $OUTPOST_SRC). Writes Outpost_<tag>.blend,
# review/build_<tag>.json, review/rail_uv_<tag>.json and, with --export, export/SM_Outpost_*.fbx + manifests.
#
# What it changes on the outpost (everything else is as delivered):
#   1. RAMP: the ramp islands (deck, two rails, brackets) are stretched along X about the hinge so the slope
#      drops from 38 to about 19 degrees; the normals of those islands are corrected for the stretch.
#   2. BAY FLOOR: the loose floor pad sloped from 166 (back) to 153 cm (front); it is flattened to 153 and a
#      dark skirting closes the wall feet.
#   3. PILES: a dark pile under every foot and under the ramp toe, from the ground down 150 cm, so nothing
#      floats on a slope (hidden on flat ground).
#   4. COLLISION: UCX_ convex boxes / prisms: deck, bay floor, three cabin blocks, bay roof, equipment box,
#      awning, ramp wedge, two ramp rails.
# Coordinates in this file are Blender's, in cm: +X forward (ramp, open bay), +Z up, and Y is the MIRROR of
# Unreal's (FBX hands Blender +Y to Unreal as -Y).
import bpy, bmesh, sys, os, json, math, runpy
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import op_common as C
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
TAG = argv[argv.index("--tag") + 1] if "--tag" in argv else "r1"
EXPORT = "--export" in argv

RAMP_HINGE_X = 219.3      # cm, top edge of the ramp island
RAMP_STRETCH = 2.25       # 38.0 deg -> 19.1 deg
BAY = dict(x0=-66.0, x1=112.0, y0=-239.0, y1=76.0, floor=153.0, ceil=399.0)
DECK_TOP = 140.0
report = {"tag": TAG}


def islands_of(bm):
    """Connected components after welding coincident verts (the FBX splits verts at UV / normal seams)."""
    parent = list(range(len(bm.verts)))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    tm = bmesh.ops.find_doubles(bm, verts=bm.verts, dist=1e-5)["targetmap"]
    for a, b in tm.items():
        parent[find(a.index)] = find(b.index)
    for e in bm.edges:
        parent[find(e.verts[0].index)] = find(e.verts[1].index)
    groups = {}
    for v in bm.verts:
        groups.setdefault(find(v.index), []).append(v)
    return list(groups.values())


def bbox(vs):
    return ([min(v.co[i] for v in vs) * 100 for i in range(3)], [max(v.co[i] for v in vs) * 100 for i in range(3)])


def add_box(bm, lo, hi, uv_layer, uv, mat_index=0):
    """Axis-aligned box in cm, outward normals, all UVs on one texel."""
    x0, y0, z0 = [c / 100 for c in lo]; x1, y1, z1 = [c / 100 for c in hi]
    v = [bm.verts.new(p) for p in ((x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0), (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1))]
    for idx in ((0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)):
        f = bm.faces.new([v[i] for i in idx])
        f.material_index = mat_index
        f.smooth = False
        for l in f.loops:
            l[uv_layer].uv = uv
    return v


def ucx(name, parent, pts_cm):
    """Convex hull collision object from points in cm."""
    bm = bmesh.new()
    for p in pts_cm:
        bm.verts.new([c / 100 for c in p])
    bmesh.ops.convex_hull(bm, input=list(bm.verts))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(o)
    o.parent = parent
    o.display_type = "WIRE"; o.hide_render = True
    return o


def box_pts(x0, x1, y0, y1, z0, z1):
    return [(x, y, z) for x in (x0, x1) for y in (y0, y1) for z in (z0, z1)]


def prism_xy(poly, z0, z1):
    return [(x, y, z) for (x, y) in poly for z in (z0, z1)]


def prism_xz(poly, y0, y1):
    return [(x, y, z) for (x, z) in poly for y in (y0, y1)]


# ------------------------------------------------------------------ outpost
C.clear_scene()
src = C.import_fbx("SM_VerdanceOutpost")
src.name = "SM_Outpost_Cabin"; src.data.name = "SM_Outpost_Cabin"
me = src.data
src_normals = [tuple(cn.vector) for cn in me.corner_normals]
bm = bmesh.new(); bm.from_mesh(me)
bm.verts.ensure_lookup_table()
uvl = bm.loops.layers.uv.active
isl = islands_of(bm)
report["source_islands"] = len(isl)

ramp_verts, pad_verts, foot_pts, rail_faces = set(), set(), [], set()
dark_uv = None
for vs in isl:
    lo, hi = bbox(vs)
    is_ramp = lo[0] >= RAMP_HINGE_X - 0.5 and lo[1] >= -146 and hi[1] <= 6 and hi[2] < 175
    if is_ramp:
        ramp_verts.update(v.index for v in vs)
        if hi[1] - lo[1] < 12 and hi[0] - lo[0] > 100:      # the two handrails
            for v in vs:
                rail_faces.update(f.index for f in v.link_faces)
        continue
    if lo[2] > 150 and hi[2] < 168 and lo[0] > -70 and hi[0] < 95:
        pad_verts.update(v.index for v in vs)
        continue
    if lo[2] < 1.0 and hi[2] < 125:                         # legs and feet
        foot_pts.extend(v.co.copy() for v in vs if v.co.z * 100 < 3.0)
        if dark_uv is None and hi[2] > 100 and (hi[0] - lo[0]) < 40 and (hi[1] - lo[1]) < 40:
            # a single leg: take one of its tall side faces as the "dark frame metal" texel
            f = max((f for v in vs for f in v.link_faces if abs(f.normal.z) < 0.2), key=lambda f: f.calc_area())
            dark_uv = sum((l[uvl].uv for l in f.loops), Vector((0, 0))) / len(f.loops)
report["ramp_verts"] = len(ramp_verts); report["pad_verts"] = len(pad_verts); report["dark_uv"] = [round(c, 4) for c in dark_uv]

# 1. ramp stretch
ramp_before = bbox([bm.verts[i] for i in ramp_verts])
for i in ramp_verts:
    v = bm.verts[i]
    v.co.x = (RAMP_HINGE_X + (v.co.x * 100 - RAMP_HINGE_X) * RAMP_STRETCH) / 100
ramp_after = bbox([bm.verts[i] for i in ramp_verts])
report["ramp_bbox_before_cm"] = [[round(c, 1) for c in b] for b in ramp_before]
report["ramp_bbox_after_cm"] = [[round(c, 1) for c in b] for b in ramp_after]
# 2. bay pad
for i in pad_verts:
    v = bm.verts[i]
    v.co.z = min(v.co.z, BAY["floor"] / 100)
# rail UV triangles (for the accent mask) before new geometry is added
bm.faces.ensure_lookup_table()
rail_uv = [[[round(l[uvl].uv.x, 5), round(l[uvl].uv.y, 5)] for l in bm.faces[fi].loops] for fi in sorted(rail_faces)]
# skirting along the three bay walls
t = 4.0
add_box(bm, (BAY["x0"], BAY["y0"], 150), (BAY["x0"] + t, BAY["y1"], 171), uvl, dark_uv)
add_box(bm, (BAY["x0"], BAY["y1"] - t, 150), (100, BAY["y1"], 171), uvl, dark_uv)
add_box(bm, (BAY["x0"], BAY["y0"], 150), (100, BAY["y0"] + t, 171), uvl, dark_uv)
# 3. piles: cluster the foot points
clusters = []
for p in foot_pts:
    for c in clusters:
        if abs(c[0].x - p.x) < 0.45 and abs(c[0].y - p.y) < 0.45:
            c.append(p); break
    else:
        clusters.append([p])
piles = []
for c in clusters:
    lo = [min(p[i] for p in c) * 100 + 3 for i in range(2)]; hi = [max(p[i] for p in c) * 100 - 3 for i in range(2)]
    if hi[0] - lo[0] < 6 or hi[1] - lo[1] < 6:
        continue
    add_box(bm, (lo[0], lo[1], -150), (hi[0], hi[1], 0.5), uvl, dark_uv)
    piles.append([round(lo[0]), round(lo[1]), round(hi[0]), round(hi[1])])
# 3b. lights (emissive through T_Outpost_Cabin_E): a beacon on the antenna tip and two bars under the awning edge.
# Their UVs sit on a texel block that no delivered triangle uses (found on a 256 grid, conservative bounding boxes).
N = 256
used = bytearray(N * N)
for f in bm.faces:
    us = [l[uvl].uv.x % 1.0 for l in f.loops]; vs_ = [l[uvl].uv.y % 1.0 for l in f.loops]
    for gy in range(max(0, int(min(vs_) * N) - 1), min(N, int(max(vs_) * N) + 2)):
        row = gy * N
        for gx in range(max(0, int(min(us) * N) - 1), min(N, int(max(us) * N) + 2)):
            used[row + gx] = 1
glow_uv = None
B = 6
for gy in range(2, N - B - 2):
    for gx in range(2, N - B - 2):
        if not any(used[(gy + j) * N + gx + i] for j in range(B) for i in range(B)):
            glow_uv = Vector(((gx + B / 2) / N, (gy + B / 2) / N)); break
    if glow_uv:
        break
report["glow_uv"] = [round(c, 5) for c in glow_uv]; report["glow_block_uv"] = B / N
top = max(bm.verts, key=lambda v: v.co.z)
tx, ty, tz = [c * 100 for c in top.co]
add_box(bm, (tx - 5, ty - 5, tz - 6), (tx + 5, ty + 5, tz + 12), uvl, glow_uv)
report["beacon_cm"] = [round(tx, 1), round(ty, 1), round(tz + 3, 1)]
for ya, yb in ((-215, -150), (-12, 53)):
    add_box(bm, (224, ya, 416.5), (231, yb, 421.5), uvl, glow_uv)
tip = ramp_after[1][0]
add_box(bm, (tip - 30, ramp_after[0][1] + 2, -150), (tip - 1, ramp_after[1][1] - 2, -0.5), uvl, dark_uv)
report["piles"] = piles
bm.to_mesh(me); bm.free()
# normals: keep the delivered ones; correct the ramp islands for the stretch; new boxes are flat
me.update()
poly_n = [tuple(p.normal) for p in me.polygons]
new_normals = []
for li, loop in enumerate(me.loops):
    if li < len(src_normals):
        n = Vector(src_normals[li])
        if loop.vertex_index in ramp_verts:
            n = Vector((n.x / RAMP_STRETCH, n.y, n.z)).normalized()
        new_normals.append(tuple(n))
    else:
        new_normals.append(None)
for p in me.polygons:
    for li in p.loop_indices:
        if new_normals[li] is None:
            new_normals[li] = poly_n[p.index]
me.normals_split_custom_set(new_normals)
me.materials.clear()
me.materials.append(C.preview_material("M_Outpost_Cabin", "T_VerdanceOutpost"))

# measured ramp slope (largest up-facing sloped face among ramp verts)
best = None
for p in me.polygons:
    if all(v in ramp_verts for v in p.vertices) and 0.3 < p.normal.z < 0.999 and abs(p.normal.y) < 0.05:
        if best is None or p.area > best.area:
            best = p
report["ramp_slope_deg"] = round(math.degrees(math.acos(best.normal.z)), 2)
report["ramp_slope_deg_source"] = 38.0

# 4. collision (Blender cm)
cab = src
ramp_top_x = RAMP_HINGE_X + (229.0 - RAMP_HINGE_X) * RAMP_STRETCH
slope = math.tan(math.radians(report["ramp_slope_deg"]))


def rz(x):
    return max(0.0, 141.3 - (x - ramp_top_x) * slope)


ucx("UCX_Cabin_Deck", cab, box_pts(-400, 228, -385, 385, 118, DECK_TOP))
ucx("UCX_Cabin_BayFloor", cab, box_pts(BAY["x0"], BAY["x1"], BAY["y0"], BAY["y1"], DECK_TOP - 2, BAY["floor"]))
ucx("UCX_Cabin_Rear", cab, prism_xy([(-338, -300), (-338, 300), (-256, 358), (BAY["x0"], 358), (BAY["x0"], -358), (-256, -358)], DECK_TOP - 2, 520))
ucx("UCX_Cabin_SideA", cab, prism_xy([(BAY["x0"], BAY["y1"]), (BAY["x0"], 358), (28, 358), (110, 300), (118, 280), (118, BAY["y1"])], DECK_TOP - 2, 520))
ucx("UCX_Cabin_SideB", cab, prism_xy([(BAY["x0"], BAY["y0"]), (BAY["x0"], -358), (28, -358), (110, -300), (118, -280), (118, BAY["y0"])], DECK_TOP - 2, 520))
ucx("UCX_Cabin_BayRoof", cab, box_pts(BAY["x0"], 118, BAY["y0"], BAY["y1"], BAY["ceil"], 520))
ucx("UCX_Cabin_EquipBox", cab, box_pts(118, 192, 97, 272, DECK_TOP - 2, 347))
ucx("UCX_Cabin_Awning", cab, prism_xz([(118, 462), (118, 480), (240, 440), (240, 421)], -250, 88))
ucx("UCX_Cabin_Ramp", cab, prism_xz([(RAMP_HINGE_X, 118), (RAMP_HINGE_X, 141.3), (ramp_top_x, 141.3), (tip, 0), (tip - 70, 0)], ramp_before[0][1], ramp_before[1][1]))
rail0 = RAMP_HINGE_X + (234.1 - RAMP_HINGE_X) * RAMP_STRETCH
rail1 = RAMP_HINGE_X + (388.5 - RAMP_HINGE_X) * RAMP_STRETCH
for nm, (ya, yb) in (("UCX_Cabin_RailA", (-142.1, -134.0)), ("UCX_Cabin_RailB", (-7.2, 0.8))):
    # The pilot steps up 45 cm and auto-mantles any ledge 50 to 180 cm high whose top is walkable (normal.z >= 0.7),
    # so a flat-topped rail guard gets climbed. The guard therefore follows the modelled rail (50 cm above the ramp)
    # and ends in a 72 degree ridge, which is neither a step nor a ledge.
    ym = (ya + yb) * 0.5
    pts = []
    for x in (ramp_top_x - 6, rail1 + 3):
        pts += [(x, ya, rz(x) - 20), (x, yb, rz(x) - 20), (x, ya, rz(x) + 50), (x, yb, rz(x) + 50), (x, ym, rz(x) + 62)]
    ucx(nm, cab, pts)
report["ramp"] = {"hinge_x": RAMP_HINGE_X, "slope_starts_x": round(ramp_top_x, 1), "tip_x": round(tip, 1), "width_cm": round(ramp_before[1][1] - ramp_before[0][1], 1),
                  "clear_between_rails_cm": round(-7.2 - -134.0, 1), "y_range_blender": [round(ramp_before[0][1], 1), round(ramp_before[1][1], 1)]}

# ------------------------------------------------------------------ console
con = C.import_fbx("SM_ResearchConsole")
con.name = "SM_Outpost_Console"; con.data.name = "SM_Outpost_Console"
con.data.materials.clear()
con.data.materials.append(C.preview_material("M_Outpost_Console", "T_ResearchConsole", emissive=True))
ucx("UCX_Console_Body", con, box_pts(-43.5, 43.5, -80, 130, 0, 150))
ucx("UCX_Console_Tower", con, box_pts(-24, 40, -130, -97, 0, 148))
sock = bpy.data.objects.new("SOCKET_Interact", None)
bpy.context.scene.collection.objects.link(sock)
sock.parent = con
sock.location = (0.95, 0.0, 0.0)      # 95 cm in front of the pivot (51 cm in front of the desk edge), on the floor
con.location = (4.5, -9.0, 0.0)       # parked beside the outpost in the .blend only

bpy.ops.wm.save_as_mainfile(filepath=os.path.join(C.KIT, "Outpost_%s.blend" % TAG))
with open(os.path.join(C.KIT, "review", "rail_uv_%s.json" % TAG), "w") as fh:
    json.dump(rail_uv, fh)

if EXPORT:
    ex = runpy.run_path(os.path.join(C.REPO, ".claude", "skills", "blender-unreal-export", "scripts", "export_for_unreal.py"))
    con.location = (0, 0, 0)
    bpy.context.view_layer.update()
    for root in ("SM_Outpost_Cabin", "SM_Outpost_Console"):
        r = ex["main"](root=root, out_dir=os.path.join(C.KIT, "export"), front_axis="+X", nanite_tris=10 ** 9)
        report["export_" + root] = {k: r.get(k) for k in ("status", "tris", "size_cm", "collision", "sockets", "welded_verts", "warnings", "errors")}
with open(os.path.join(C.KIT, "review", "build_%s.json" % TAG), "w") as fh:
    json.dump(report, fh, indent=1)
print("__RESULT__ " + json.dumps(report))

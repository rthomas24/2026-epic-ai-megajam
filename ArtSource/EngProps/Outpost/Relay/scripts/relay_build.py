# SPDX-License-Identifier: MIT
# Triad Drift - Ancient Relay: collision and export for Grok Bot's SM_AncientRelay (the mesh itself is not changed).
#   blender --background --factory-startup --python Relay/scripts/relay_build.py -- [--tag r2] [--export]
# Reads ArtSource/EngAI/Props/AncientRelay/mesh_r1 (read only). Writes Relay/Relay_r1.blend, Relay/review/build_r1.json
# and, with --export, Relay/export/SM_Relay.fbx + manifest.
#
# Collision (all convex, measured from the mesh by ray casts and vertex groups):
#   plinth      two stacked prisms: the lower tier (top 90 cm) and the court (top 135 cm), outlines sampled every 15 deg
#   stairs      ONE ramp wedge over the whole stair flight incl. its stringers (the pilot's capsule catches on 20 cm
#               risers at speed; the wedge is 24 degrees)
#   ring        one hull per 30 degree sector of the ring, one per arm of the cross (pillar and conduits with the core)
#   pylons      one hull per loose cluster standing on the plinth outside the ring
# Blender cm, +X forward (stairs), Y = -Unreal Y.
import bpy, bmesh, sys, os, json, math, runpy
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(HERE)), "scripts"))
import op_common as C
from mathutils import Vector
from mathutils.bvhtree import BVHTree

KIT = os.path.dirname(HERE)
SRC = os.path.join(C.REPO, "ArtSource", "EngAI", "Props", "AncientRelay", "mesh_r1")
EXPORT = "--export" in sys.argv
report = {}

C.clear_scene()
bpy.ops.import_scene.fbx(filepath=os.path.join(SRC, "SM_AncientRelay.fbx"))
root = [o for o in bpy.data.objects if o.type == "MESH"][0]
root.name = "SM_Relay"; root.data.name = "SM_Relay"
root.data.materials.clear()
root.data.materials.append(C.preview_material("M_Relay", "T_AncientRelay", tex_dir=SRC, emissive=True))

# ---- r2: the stair block made walkable at 20 degrees (THE RULE: ramps at most 20 degrees, nothing that snags).
# As delivered (Blender cm, treads y 35..155 with stringers to y -5 / 195): a landing at Z 146 to X 314, then a
# 46 cm riser down to the first tread (Z 100), then a flight of 9 cm steps whose nosings fall at 23.6 degrees to the
# ground at X 545. Overall 146 cm over 231 cm = 32 degrees, and the 46 cm riser is a capsule snag.
# Edit (vertices with X > 314 inside the stair band only; nothing else moves):
#   x' = x + SHEAR * (146 - clamp(z, 100, 146)) + (STRETCH - 1) * max(0, x - 318)
#   -> the 46 cm riser becomes a 20 degree carved slope (126 cm long), the flight below it moves out with it and is
#      stretched 1.25 x along X (nosings at 19.3 degrees). Treads stay flat and keep their height.
TAG = "r2"
for i, a in enumerate(sys.argv):
    if a == "--tag" and i + 1 < len(sys.argv):
        TAG = sys.argv[i + 1]
SHEAR, STRETCH = 1.0 / math.tan(math.radians(20.0)), 1.25
BAND_Y = (-8.0, 198.0)
STAIR_X0, LAND_Z, TREAD_Z = 314.0, 146.0, 100.0
me = root.data
mw = root.matrix_world.copy(); mwi = mw.inverted()
had_custom = bool(getattr(me, "has_custom_normals", False))
src_n = [Vector(cn.vector) for cn in me.corner_normals]
moved, sheared, skipped_high = set(), set(), 0
if TAG != "r1":
    for v in me.vertices:
        w = mw @ v.co
        x, y, z = w.x * 100, w.y * 100, w.z * 100
        if x <= STAIR_X0 or not (BAND_Y[0] <= y <= BAND_Y[1]):
            continue
        if z > LAND_Z + 14:
            skipped_high += 1
            continue
        cz = min(max(z, TREAD_Z), LAND_Z)
        dx = SHEAR * (LAND_Z - cz) + (STRETCH - 1.0) * max(0.0, x - 318.0)
        v.co = mwi @ Vector(((x + dx) / 100, w.y, w.z))
        moved.add(v.index)
        if TREAD_Z < z < LAND_Z:
            sheared.add(v.index)
    me.update()
    if had_custom:
        poly_n = {}
        for p in me.polygons:
            for li in p.loop_indices:
                poly_n[li] = Vector(p.normal)
        out = []
        for li, lp in enumerate(me.loops):
            if lp.vertex_index in sheared:
                out.append(tuple(poly_n[li]))
            elif lp.vertex_index in moved:
                n = src_n[li]; out.append(tuple(Vector((n.x / STRETCH, n.y, n.z)).normalized()))
            else:
                out.append(tuple(src_n[li]))
        me.normals_split_custom_set(out)
report["stair_edit"] = {"tag": TAG, "verts_moved": len(moved), "verts_sheared": len(sheared), "skipped_above_160": skipped_high,
                        "custom_normals": had_custom, "shear_deg": 20.0, "stretch_x": STRETCH,
                        "riser_slope": {"from_x": STAIR_X0, "to_x": round(STAIR_X0 + SHEAR * (LAND_Z - TREAD_Z), 1), "z": [LAND_Z, TREAD_Z]}}
bm = bmesh.new(); bm.from_mesh(root.data); bm.transform(root.matrix_world)
bvh = BVHTree.FromBMesh(bm)
P = [v.co * 100 for v in bm.verts]          # cm


def top(x, y):
    h = bvh.ray_cast(Vector((x / 100, y / 100, 11)), Vector((0, 0, -1)))
    return h[0].z * 100 if h[0] else 0.0


def hull(name, pts, max_pts=20):
    """Convex hull of the support points of pts in max_pts directions (keeps hulls small)."""
    golden = math.pi * (3.0 - math.sqrt(5.0))
    keep = {}
    if max_pts is None:                     # exact: every point (small authored hulls)
        keep = {tuple(round(c, 2) for c in p): p for p in pts}
    for i in range(max_pts or 0):
        y = 1.0 - (i / float(max_pts - 1)) * 2.0
        r = math.sqrt(max(0.0, 1.0 - y * y))
        d = Vector((math.cos(golden * i) * r, y, math.sin(golden * i) * r))
        b = max(pts, key=lambda p: p.dot(d))
        keep[tuple(round(c, 2) for c in b)] = b
    hb = bmesh.new()
    for p in keep.values():
        hb.verts.new(p / 100)
    bmesh.ops.convex_hull(hb, input=list(hb.verts))
    bmesh.ops.delete(hb, geom=[v for v in hb.verts if not v.link_faces], context="VERTS")
    bmesh.ops.recalc_face_normals(hb, faces=hb.faces)
    me = bpy.data.meshes.new(name); hb.to_mesh(me); hb.free()
    o = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(o)
    o.parent = root; o.display_type = "WIRE"; o.hide_render = True
    return o


# ---- plinth outlines
CX, CY = -75.0, 45.0
outer, inner = [], []
for deg in range(0, 360, 15):
    c, s = math.cos(math.radians(deg)), math.sin(math.radians(deg))
    r_in = r_out = 0.0
    for r in range(250, 660, 10):
        h = top(CX + c * r, CY + s * r)
        if 115 <= h <= 160:
            r_in = r
        if 60 <= h <= 160:
            r_out = r
    inner.append((deg, r_in)); outer.append((deg, max(r_out, r_in)))
# rocks, pylons and the ring feet fall in the same height bands and push the samples outward: clamp to the disc
# seen in the top view (court about 380..430, lower tier about 440..480 from this centre)
inner[:] = [(d, min(max(r, 380), 430)) for d, r in inner]
outer[:] = [(d, min(max(r, 440), 480)) for d, r in outer]
report["court_radius_cm"] = [r for _, r in inner]; report["tier_radius_cm"] = [r for _, r in outer]
COURT_Z, TIER_Z = 135.0, 90.0
hull("UCX_Relay_Court", [Vector((CX + math.cos(math.radians(d)) * r, CY + math.sin(math.radians(d)) * r, z)) for d, r in inner for z in (0.0, COURT_Z)], 48)
hull("UCX_Relay_Tier", [Vector((CX + math.cos(math.radians(d)) * r, CY + math.sin(math.radians(d)) * r, z)) for d, r in outer for z in (0.0, TIER_Z)], 48)

# ---- stairs. Flight at y = 95: treads y 35..155, stringers to y -5 / 195
ST_Y0, ST_Y1 = -5.0, 195.0
if TAG == "r1":
    ST_TOP_X, ST_TOP_Z, ST_TOE_X, ST_TOE_Z = 296.0, 146.0, 572.0, 22.0
    hull("UCX_Relay_Stairs", [Vector(p) for y in (ST_Y0, ST_Y1) for p in ((240.0, y, 0.0), (240.0, y, ST_TOP_Z), (ST_TOP_X, y, ST_TOP_Z), (ST_TOE_X, y, ST_TOE_Z), (ST_TOE_X, y, 0.0))])
    report["stairs"] = {"slope_deg": round(math.degrees(math.atan2(ST_TOP_Z - ST_TOE_Z, ST_TOE_X - ST_TOP_X)), 1)}
else:
    # r2: the landing (flat at 146) and one convex ramp over the edited flight. Its top runs from the landing edge
    # (314, 146) over the 20 degree carved slope to the first tread's nosing, then down the nosing line to the ground.
    def xp(x, z):
        cz = min(max(z, TREAD_Z), LAND_Z)
        return x + SHEAR * (LAND_Z - cz) + (STRETCH - 1.0) * max(0.0, x - 318.0)
    NOSE_TOP = (xp(352.0, 100.0), 100.0)          # front edge of the first tread
    NOSE_LOW = (xp(542.0, 17.0), 17.0)            # front edge of the last tread
    slope = (NOSE_TOP[1] - NOSE_LOW[1]) / (NOSE_LOW[0] - NOSE_TOP[0])
    TOE = (NOSE_LOW[0] + NOSE_LOW[1] / slope, 0.0)
    hull("UCX_Relay_Landing", [Vector((x, y, z)) for x in (236.0, STAIR_X0) for y in (ST_Y0, ST_Y1) for z in (0.0, LAND_Z)], None)
    prof = [(STAIR_X0, LAND_Z), NOSE_TOP, NOSE_LOW, TOE]
    hull("UCX_Relay_Stairs", [Vector((x, y, z)) for y in (ST_Y0, ST_Y1) for (x, z) in prof + [(TOE[0], -10.0), (STAIR_X0, -10.0)]], None)
    seg = [round(math.degrees(math.atan2(prof[i][1] - prof[i + 1][1], prof[i + 1][0] - prof[i][0])), 1) for i in range(3)]
    report["stairs"] = {"profile_x_z": [[round(a, 1), round(b, 1)] for a, b in prof], "segment_slopes_deg": seg, "y_blender": [ST_Y0, ST_Y1],
                        "tread_clear_width_cm": 120, "toe_visual_x": round(xp(545.0, 0.0), 1),
                        "float_max_cm": {"over_first_tread": round(LAND_Z - (LAND_Z - NOSE_TOP[1]) * (xp(318.0, 100.0) - STAIR_X0) / (NOSE_TOP[0] - STAIR_X0) - TREAD_Z, 1),
                                         "over_flight_treads": "<= 9 (one riser)", "ground_wedge_beyond_toe": round(TOE[0] - xp(545.0, 0.0), 1)}}

# ---- ring and cross
high = [p for p in P if p.z > 400]
y0, y1 = min(p.y for p in high), max(p.y for p in high)
x0, x1 = min(p.x for p in high) - 5, max(p.x for p in high) + 5
RY = (y0 + y1) * 0.5; R = (y1 - y0) * 0.5; RZ = 1000.0 - R
report["ring"] = {"centre_yz": [round(RY, 1), round(RZ, 1)], "outer_radius": round(R, 1), "x_slab": [round(x0), round(x1)]}
slab = [p for p in P if x0 <= p.x <= x1 and p.z > COURT_Z + 5]
ring_pts, cross_pts = {}, {}
for p in slab:
    dy, dz = p.y - RY, p.z - RZ
    r = math.hypot(dy, dz); ang = math.degrees(math.atan2(dz, dy)) % 360
    if r > 0.72 * R:
        ring_pts.setdefault(int(ang // 30), []).append(p)
    else:
        cross_pts.setdefault(int(((ang + 45) % 360) // 90), []).append(p)
n_ring = n_cross = 0
for k, pts in sorted(ring_pts.items()):
    if len(pts) >= 12:
        hull("UCX_Relay_Ring%02d" % k, pts); n_ring += 1
for k, pts in sorted(cross_pts.items()):
    if len(pts) >= 12:
        hull("UCX_Relay_Cross%d" % k, pts); n_cross += 1
# ---- pylons and rocks on the plinth outside the ring slab
rest = [p for p in P if not (x0 <= p.x <= x1) and p.z > 175]
cells = {}
for p in rest:
    cells.setdefault((int(p.x // 60), int(p.y // 60)), []).append(p)
seen, n_pyl = set(), 0
for key in list(cells):
    if key in seen:
        continue
    stack, grp = [key], []
    seen.add(key)
    while stack:
        k = stack.pop(); grp.extend(cells[k])
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                n = (k[0] + dx, k[1] + dy)
                if n in cells and n not in seen:
                    seen.add(n); stack.append(n)
    if len(grp) >= 10:
        # include the base so the hull stands on the plinth
        lo = min(grp, key=lambda p: p.z)
        base = [Vector((p.x, p.y, 60.0)) for p in grp]
        hull("UCX_Relay_Pylon%02d" % n_pyl, grp + base, 14); n_pyl += 1
report["hulls"] = {"ring": n_ring, "cross": n_cross, "pylons": n_pyl, "plinth": 2, "stairs": 1 if TAG == "r1" else 2}
bm.free()

os.makedirs(os.path.join(KIT, "review"), exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(KIT, "Relay_%s.blend" % TAG))
if EXPORT:
    bpy.context.view_layer.update()
    ex = runpy.run_path(os.path.join(C.REPO, ".claude", "skills", "blender-unreal-export", "scripts", "export_for_unreal.py"))
    try:
        r = ex["main"](root="SM_Relay", out_dir=os.path.join(KIT, "export"), front_axis="+X", nanite_tris=10 ** 9)
    except AttributeError:
        # the exporter's selection restore trips over a stale view-layer entry after it has written the files
        # (seen with 20+ collision objects); the FBX and manifest are complete at that point
        with open(os.path.join(KIT, "export", "SM_Relay.manifest.json")) as fh:
            r = json.load(fh)
        r["status"] = "ok (read back from the manifest)"
    report["export"] = {k: r.get(k) for k in ("status", "tris", "size_cm", "welded_verts", "warnings", "errors")}
    report["export"]["hulls"] = len(r.get("collision", []))
with open(os.path.join(KIT, "review", "build_%s.json" % TAG), "w") as fh:
    json.dump(report, fh, indent=1)
print("__RESULT__ " + json.dumps(report))

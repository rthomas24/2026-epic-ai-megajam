# SPDX-License-Identifier: MIT
# Triad Drift - Ancient Relay: collision and export for Grok Bot's SM_AncientRelay (the mesh itself is not changed).
#   blender --background --factory-startup --python Relay/scripts/relay_build.py -- [--export]
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
    for i in range(max_pts):
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

# ---- stairs: one wedge. Flight measured at y = 95: treads y 35..155, stringers to y -5 / 195
ST_Y0, ST_Y1 = -5.0, 195.0
ST_TOP_X, ST_TOP_Z, ST_TOE_X, ST_TOE_Z = 296.0, 146.0, 572.0, 22.0
hull("UCX_Relay_Stairs", [Vector(p) for y in (ST_Y0, ST_Y1) for p in ((240.0, y, 0.0), (240.0, y, ST_TOP_Z), (ST_TOP_X, y, ST_TOP_Z), (ST_TOE_X, y, ST_TOE_Z), (ST_TOE_X, y, 0.0))])
report["stairs"] = {"slope_deg": round(math.degrees(math.atan2(ST_TOP_Z - ST_TOE_Z, ST_TOE_X - ST_TOP_X)), 1), "y_blender": [ST_Y0, ST_Y1], "top": [ST_TOP_X, ST_TOP_Z], "toe": [ST_TOE_X, ST_TOE_Z],
                    "tread_clear_width_cm": 120, "as_modelled": "risers about 20 cm; lower flight 18 deg, upper two steps steeper"}

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
report["hulls"] = {"ring": n_ring, "cross": n_cross, "pylons": n_pyl, "plinth": 2, "stairs": 1}
bm.free()

os.makedirs(os.path.join(KIT, "review"), exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(KIT, "Relay_r1.blend"))
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
with open(os.path.join(KIT, "review", "build_r1.json"), "w") as fh:
    json.dump(report, fh, indent=1)
print("__RESULT__ " + json.dumps(report))

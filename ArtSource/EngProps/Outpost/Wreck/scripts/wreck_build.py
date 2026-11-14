# SPDX-License-Identifier: MIT
# Triad Drift - Verdance wreck: a way in, lights and collision for Grok Bot's SM_VerdanceWreck.
#   blender --background --factory-startup --python Wreck/scripts/wreck_build.py -- [--export]
# Reads ArtSource/EngAI/Props/VerdanceWreck/mesh_r1 (read only). Writes Wreck/Wreck_r1.blend, Wreck/review/build_r1.json,
# Wreck/textures/T_Wreck_E.png (via wreck_textures.py) and, with --export, Wreck/export/SM_Wreck.fbx + manifest.
#
# Measured on the delivery (Blender cm; Y = -Unreal Y): the ruptured midsection is open right through between
# X -225 and 375; its deck is at Z 242..258 between Y -300 and 300, the ceiling at 535..583. Outside each side a
# hanging stair (X -25..175) drops at 42 degrees to 97 cm above the ground: not reachable and too steep.
# Added geometry (the delivered triangles are not touched):
#   RAMP    three fallen deck plates and two rails from the +Y deck lip (Z 252) down to the ground, 20 degrees,
#           230 cm wide, covering the hanging stair on that side
#   LIGHTS  three failing-light boxes under the ceiling of the opening (emissive through T_Wreck_E)
#   PILES   none (the hull sits on the ground along its length)
# Collision: deck slab, roof over the opening, ramp wedge with two ridged rail guards, and one hull per 400 cm
# section of the ship fore and aft of the opening.
import bpy, bmesh, sys, os, json, math, runpy
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(HERE)), "scripts"))
import op_common as C
from mathutils import Vector
from mathutils.bvhtree import BVHTree

KIT = os.path.dirname(HERE)
SRC = os.path.join(C.REPO, "ArtSource", "EngAI", "Props", "VerdanceWreck", "mesh_r1")
EXPORT = "--export" in sys.argv
OPEN_X0, OPEN_X1 = -225.0, 375.0
DECK_Z, DECK_Y = 252.0, 300.0
RAMP_X0, RAMP_X1 = -40.0, 190.0
RAMP_DEG = 20.0
PLATE_UV = (0.550, 0.378, 0.622, 0.450)      # a grey ribbed deck panel in T_VerdanceWreck_BC (u0, v0, u1, v1)
DARK_UV = (0.41, 0.219)
report = {}

C.clear_scene()
bpy.ops.import_scene.fbx(filepath=os.path.join(SRC, "SM_VerdanceWreck.fbx"))
root = [o for o in bpy.data.objects if o.type == "MESH"][0]
root.name = "SM_Wreck"; root.data.name = "SM_Wreck"
me = root.data
src_normals = [tuple(cn.vector) for cn in me.corner_normals]
bm = bmesh.new(); bm.from_mesh(me)
uvl = bm.loops.layers.uv.active
bvh = BVHTree.FromBMesh(bm)
P = [v.co * 100 for v in bm.verts]


def add_box8(pts, uvs):
    """Box from 8 corners (bottom 4 then top 4, counter-clockwise seen from above); uvs: one uv, or a function of the corner index."""
    v = [bm.verts.new(Vector(p) / 100) for p in pts]
    for idx in ((0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)):
        f = bm.faces.new([v[i] for i in idx])
        f.smooth = False
        for l, i in zip(f.loops, idx):
            l[uvl].uv = uvs(i) if callable(uvs) else uvs


# ---- free texel block for the lights
N = 256
used = bytearray(N * N)
for f in bm.faces:
    us = [l[uvl].uv.x % 1.0 for l in f.loops]; vs_ = [l[uvl].uv.y % 1.0 for l in f.loops]
    for gy in range(max(0, int(min(vs_) * N) - 1), min(N, int(max(vs_) * N) + 2)):
        for gx in range(max(0, int(min(us) * N) - 1), min(N, int(max(us) * N) + 2)):
            used[gy * N + gx] = 1
glow_uv, B = None, 5
while glow_uv is None and B >= 2:
    for gy in range(1, N - B - 1):
        for gx in range(1, N - B - 1):
            if not any(used[(gy + j) * N + gx + i] for j in range(B) for i in range(B)):
                glow_uv = Vector(((gx + B / 2) / N, (gy + B / 2) / N)); break
        if glow_uv:
            break
    if glow_uv is None:
        B -= 1
report["glow_uv"] = [round(c, 5) for c in glow_uv] if glow_uv else None
report["glow_block_uv"] = B / N

# ---- ramp: three plates on one plane, two rails
run = DECK_Z / math.tan(math.radians(RAMP_DEG))
y_top, y_toe = DECK_Y - 12.0, DECK_Y + run
report["ramp"] = {"slope_deg": RAMP_DEG, "run_cm": round(run, 1), "rise_cm": DECK_Z, "x": [RAMP_X0, RAMP_X1], "y_blender": [round(y_top, 1), round(y_toe, 1)],
                  "width_cm": RAMP_X1 - RAMP_X0, "clear_between_rails_cm": RAMP_X1 - RAMP_X0 - 24.0}


def rz(y):
    return DECK_Z - (y - DECK_Y) * math.tan(math.radians(RAMP_DEG))


u0, v0, u1, v1 = PLATE_UV
n_pl = 3
seg = (y_toe + 14.0 - y_top) / n_pl
for i in range(n_pl):
    ya, yb = y_top + seg * i + (3.0 if i else 0.0), y_top + seg * (i + 1)
    skew = (-6.0, 5.0, -3.0)[i]
    xa, xb = RAMP_X0 + 12 + skew, RAMP_X1 - 12 + skew
    uv_of = {0: (u0, v0), 1: (u1, v0), 2: (u1, v1), 3: (u0, v1), 4: (u0, v0), 5: (u1, v0), 6: (u1, v1), 7: (u0, v1)}
    add_box8([(xa, ya, rz(ya) - 12), (xb, ya, rz(ya) - 12), (xb, yb, rz(yb) - 12), (xa, yb, rz(yb) - 12),
              (xa, ya, rz(ya)), (xb, ya, rz(ya)), (xb, yb, rz(yb)), (xa, yb, rz(yb))], lambda k: uv_of[k])
for xa, xb in ((RAMP_X0, RAMP_X0 + 12), (RAMP_X1 - 12, RAMP_X1)):
    ya, yb = y_top, y_toe + 14.0
    add_box8([(xa, ya, rz(ya) - 22), (xb, ya, rz(ya) - 22), (xb, yb, rz(yb) - 22), (xa, yb, rz(yb) - 22),
              (xa, ya, rz(ya) + 14), (xb, ya, rz(ya) + 14), (xb, yb, rz(yb) + 14), (xa, yb, rz(yb) + 14)], DARK_UV)
    # two props under each rail
    for yp in (y_top + run * 0.3, y_top + run * 0.62):
        add_box8([(xa, yp - 7, -40), (xb, yp - 7, -40), (xb, yp + 7, -40), (xa, yp + 7, -40),
                  (xa, yp - 7, rz(yp) - 20), (xb, yp - 7, rz(yp) - 20), (xb, yp + 7, rz(yp) - 20), (xa, yp + 7, rz(yp) - 20)], DARK_UV)

# ---- lights under the ceiling of the opening
lights = []
if glow_uv:
    for x, y in ((40.0, -140.0), (190.0, 120.0), (-90.0, 60.0)):
        h = bvh.ray_cast(Vector((x / 100, y / 100, 4.0)), Vector((0, 0, 1)))
        zc = h[0].z * 100 if h[0] else 560.0
        add_box8([(x - 22, y - 6, zc - 9), (x + 22, y - 6, zc - 9), (x + 22, y + 6, zc - 9), (x - 22, y + 6, zc - 9),
                  (x - 22, y - 6, zc + 2), (x + 22, y - 6, zc + 2), (x + 22, y + 6, zc + 2), (x - 22, y + 6, zc + 2)], glow_uv)
        lights.append([x, y, round(zc - 4, 1)])
report["lights_blender_cm"] = lights
bm.normal_update()
bm.to_mesh(me); bm.free()
me.update()
poly_n = [tuple(p.normal) for p in me.polygons]
normals = [None] * len(me.loops)
for p in me.polygons:
    for li in p.loop_indices:
        normals[li] = src_normals[li] if li < len(src_normals) else poly_n[p.index]
me.normals_split_custom_set(normals)
me.materials.clear()
me.materials.append(C.preview_material("M_Wreck", "T_VerdanceWreck", tex_dir=SRC))


# ---- collision
def hull(name, pts, max_pts=20):
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
        hb.verts.new(Vector(p) / 100)
    bmesh.ops.convex_hull(hb, input=list(hb.verts))
    bmesh.ops.delete(hb, geom=[v for v in hb.verts if not v.link_faces], context="VERTS")
    bmesh.ops.recalc_face_normals(hb, faces=hb.faces)
    m = bpy.data.meshes.new(name); hb.to_mesh(m); hb.free()
    o = bpy.data.objects.new(name, m)
    bpy.context.scene.collection.objects.link(o)
    o.parent = root; o.display_type = "WIRE"; o.hide_render = True
    return o


def box(x0, x1, y0, y1, z0, z1):
    return [Vector((x, y, z)) for x in (x0, x1) for y in (y0, y1) for z in (z0, z1)]


hull("UCX_Wreck_Deck", box(OPEN_X0, OPEN_X1, -DECK_Y, DECK_Y, 20.0, DECK_Z), 8)
roof = [p for p in P if OPEN_X0 <= p.x <= OPEN_X1 and p.z > 545]
hull("UCX_Wreck_Roof", roof + box(OPEN_X0, OPEN_X1, -DECK_Y, DECK_Y, 548.0, 600.0), 24)
sections = []
edges = [-1600, -1200, -800, -400, OPEN_X0, None, OPEN_X1, 800, 1200, 1600.01]
for a, b in zip(edges[:-1], edges[1:]):
    if a is None or b is None:
        continue
    pts = [p for p in P if a <= p.x < b]
    if len(pts) >= 20:
        hull("UCX_Wreck_Hull%02d" % len(sections), pts, 26); sections.append([a, b])
report["hull_sections_x"] = sections
hull("UCX_Wreck_Ramp", [Vector(p) for x in (RAMP_X0, RAMP_X1) for p in ((x, DECK_Y - 12, 20.0), (x, DECK_Y - 12, DECK_Z), (x, DECK_Y, DECK_Z), (x, y_toe, 0.0), (x, y_toe - 60, -20.0))], 12)
for nm, (xa, xb) in (("UCX_Wreck_RailA", (RAMP_X0, RAMP_X0 + 12)), ("UCX_Wreck_RailB", (RAMP_X1 - 12, RAMP_X1))):
    pts = []
    for y in (DECK_Y, y_toe + 10):
        pts += [(xa, y, rz(y) - 22), (xb, y, rz(y) - 22), (xa, y, rz(y) + 30), (xb, y, rz(y) + 30), ((xa + xb) / 2, y, rz(y) + 48)]
    hull(nm, [Vector(p) for p in pts], 12)
report["walk"] = {"deck_z": DECK_Z, "deck_x": [OPEN_X0, OPEN_X1], "deck_y_blender": [-DECK_Y, DECK_Y], "ceiling_z": [535, 583], "clear_height_cm": 283,
                  "opening_width_along_x_cm": OPEN_X1 - OPEN_X0}

bpy.ops.wm.save_as_mainfile(filepath=os.path.join(KIT, "Wreck_r1.blend"))
if EXPORT:
    bpy.context.view_layer.update()
    ex = runpy.run_path(os.path.join(C.REPO, ".claude", "skills", "blender-unreal-export", "scripts", "export_for_unreal.py"))
    try:
        r = ex["main"](root="SM_Wreck", out_dir=os.path.join(KIT, "export"), front_axis="+X", nanite_tris=10 ** 9)
    except AttributeError:
        # the exporter's selection restore trips over a stale view-layer entry after it has written the files
        with open(os.path.join(KIT, "export", "SM_Wreck.manifest.json")) as fh:
            r = json.load(fh)
        r["status"] = "ok (read back from the manifest)"
    report["export"] = {k: r.get(k) for k in ("status", "tris", "size_cm", "welded_verts", "warnings", "errors")}
    report["export"]["hulls"] = len(r.get("collision", []))
with open(os.path.join(KIT, "review", "build_r1.json"), "w") as fh:
    json.dump(report, fh, indent=1)
print("__RESULT__ " + json.dumps(report))

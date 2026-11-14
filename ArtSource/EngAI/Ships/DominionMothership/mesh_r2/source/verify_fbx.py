"""Re-import SM_DominionMothership.fbx / .glb and verify scale, tris, sockets, UCX and clearances (Unreal cm).
usage: blender -b --factory-startup --python verify_fbx.py -- EXPORT_DIR TEX2K_DIR OUT_JSON"""
import bpy, bmesh, sys, os, json, math, re, struct
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree
argv = sys.argv[sys.argv.index("--") + 1:]
EXP, TEX, OUTJ = argv
bpy.ops.wm.read_factory_settings(use_empty=True)
fbx = os.path.join(EXP, "SM_DominionMothership.fbx")
raw = open(fbx, "rb").read()
def fbx_prop(name):
    i = raw.find(name.encode()); j = raw.find(b"D", i + len(name)); return struct.unpack("<d", raw[j + 1:j + 9])[0]
R = {"fbx_UnitScaleFactor": fbx_prop("UnitScaleFactor"), "fbx_OriginalUnitScaleFactor": fbx_prop("OriginalUnitScaleFactor")}
bpy.ops.import_scene.fbx(filepath=fbx)
objs = bpy.context.scene.objects
sm = objs["SM_DominionMothership"]
dg = bpy.context.evaluated_depsgraph_get()
def ue_verts(o):
    M = o.matrix_world
    return np.array([[(M @ v.co).x * 100, -(M @ v.co).y * 100, (M @ v.co).z * 100] for v in o.data.vertices])
def bvh_ue(o):
    M = o.matrix_world
    vs = [Vector(((M @ v.co).x * 100, -(M @ v.co).y * 100, (M @ v.co).z * 100)) for v in o.data.vertices]
    return BVHTree.FromPolygons(vs, [list(p.vertices) for p in o.data.polygons]), np.array(vs)
smv = ue_verts(sm)
R["import_object_scale"] = list(sm.scale); R["tris"] = len(sm.data.polygons)
R["all_tris_triangles"] = all(len(p.vertices) == 3 for p in sm.data.polygons)
R["bounds_cm"] = {"min": smv.min(0).round(1).tolist(), "max": smv.max(0).round(1).tolist()}
core = smv[(np.abs(smv[:, 1]) <= 8400) & (smv[:, 0] >= -30000) & (smv[:, 0] <= 30000) & (smv[:, 2] >= -5000) & (smv[:, 2] <= 5000)]
R["hull_core_bounds_cm"] = {"min": core.min(0).round(1).tolist(), "max": core.max(0).round(1).tolist()}
hullv = smv[(np.abs(smv[:, 1]) <= 8000.5) & (np.abs(smv[:, 2]) <= 5000.5) & (np.abs(smv[:, 0]) <= 30000.5)]
R["hull_core_extent_cm"] = (hullv.max(0) - hullv.min(0)).round(1).tolist()
def box_bvh(lo, hi):
    vs = [Vector((x, y, z)) for x in (lo[0], hi[0]) for y in (lo[1], hi[1]) for z in (lo[2], hi[2])]
    f = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]
    return BVHTree.FromPolygons(vs, f)
def cyl_bvh(c, r, z0, z1, n=32):
    vs = []; f = []
    for i in range(n):
        a = 2 * math.pi * i / n; vs += [Vector((c[0] + r * math.cos(a), c[1] + r * math.sin(a), z0)), Vector((c[0] + r * math.cos(a), c[1] + r * math.sin(a), z1))]
    for i in range(n):
        j = (i + 1) % n; f.append((2 * i, 2 * j, 2 * j + 1, 2 * i + 1))
    f.append(tuple(2 * i for i in range(n))); f.append(tuple(2 * i + 1 for i in range(n)))
    return BVHTree.FromPolygons(vs, f)
def inside_box(v, lo, hi): return int(np.sum(np.all((v > np.array(lo)) & (v < np.array(hi)), axis=1)))
sm_bvh, _ = bvh_ue(sm)
ucx = [o for o in objs if o.name.startswith("UCX_")]
ucx_bvh = [(o.name, *bvh_ue(o)) for o in ucx]
ZONES = {
    "trench_corridor_|Y|<2350_X-18000..24000_Z5001..8500": ((-18000, -2350, 5001), (24000, 2350, 8500)),
    "trench_forward_entry_X24001..31500_|Y|<2490_Z5001..12000": ((24001, -2490, 5001), (31500, 2490, 12000)),
    "bay_launch_channel_X>30000_|Y|<2800_Z-4000..0": ((30001, -2800, -4000), (34000, 2800, 0)),
    "bay_glide_pocket_X24250..29990_|Y|<2430_Z-3720..-285": ((24250, -2430, -3720), (29990, 2430, -285)),
}
UCX_ZONES = {
    "ucx_trench_corridor_|Y|<2450": ((-18000, -2450, 5001), (24000, 2450, 8500)),
    "ucx_trench_forward_entry": ((24001, -2490, 5001), (31500, 2490, 12000)),
    "ucx_bay_launch_channel": ((30001, -2800, -4000), (34000, 2800, 0)),
    "ucx_bay_glide_pocket_X24000..30000_|Y|<2499_Z-3749..-251": ((24001, -2499, -3749), (30000, 2499, -251)),
}
R["render_mesh_clearance"] = {}
for k, (lo, hi) in ZONES.items():
    R["render_mesh_clearance"][k] = {"verts_inside": inside_box(smv, lo, hi), "tri_box_overlaps": len(sm_bvh.overlap(box_bvh(lo, hi)))}
R["ucx_clearance"] = {}
for k, (lo, hi) in UCX_ZONES.items():
    hits = []
    for (n, b, v) in ucx_bvh:
        if inside_box(v, lo, hi) or b.overlap(box_bvh(lo, hi)): hits.append(n)
    R["ucx_clearance"][k] = hits
loc, nrm, idx, dist = sm_bvh.find_nearest(Vector((16500, 0, 6500)))
R["reactor_min_clearance_cm"] = round(dist, 1)
R["reactor_ucx_min_clearance_cm"] = round(min(b.find_nearest(Vector((16500, 0, 6500)))[3] for (n, b, v) in ucx_bvh), 1)
R["generator_dome_volume_r1800_Z0..4000"] = {}
for (gx, gy) in ((27500, 27500), (-27500, 27500), (-27500, -27500), (27500, -27500)):
    cb = cyl_bvh((gx, gy), 1800, 1, 4000)
    rv = smv[((smv[:, 0] - gx) ** 2 + (smv[:, 1] - gy) ** 2 < 1800 ** 2) & (smv[:, 2] > 0) & (smv[:, 2] < 4000)]
    uh = [n for (n, b, v) in ucx_bvh if b.overlap(cb)]
    zt = smv[((smv[:, 0] - gx) ** 2 + (smv[:, 1] - gy) ** 2 < 2400 ** 2)][:, 2].max()
    R["generator_dome_volume_r1800_Z0..4000"][f"({gx},{gy})"] = {"mesh_verts_inside": int(len(rv)), "mesh_tri_overlaps": len(sm_bvh.overlap(cb)), "ucx_overlaps": uh, "plinth_top_z_cm": round(float(zt), 1)}
socks = {}
for o in objs:
    if o.name.startswith("SOCKET_"):
        M = o.matrix_world; p = M.translation; xa = M.to_3x3() @ Vector((1, 0, 0)); za = M.to_3x3() @ Vector((0, 0, 1))
        xu = Vector((xa.x, -xa.y, xa.z)).normalized()
        socks[o.name] = {"ue_cm": [round(p.x * 100, 1), round(-p.y * 100, 1), round(p.z * 100, 1)], "parent": o.parent.name if o.parent else None,
                         "ue_local_x_axis": [round(c, 4) for c in xu], "ue_yaw_deg": round(math.degrees(math.atan2(xu.y, xu.x)) % 360, 2), "z_up": round(za.z, 4)}
R["sockets"] = dict(sorted(socks.items()))
R["ucx"] = []
for (n, b, v) in ucx_bvh:
    o = objs[n]
    bm = bmesh.new(); bm.from_mesh(o.data); bm.normal_update()
    cen = sum((vv.co for vv in bm.verts), Vector()) / len(bm.verts)
    convex = all(all((vv.co - f.verts[0].co).dot(f.normal) <= 1e-4 for vv in bm.verts) and (cen - f.verts[0].co).dot(f.normal) < 0 for f in bm.faces)
    closed = all(e.is_manifold for e in bm.edges); bm.free()
    R["ucx"].append({"name": n, "verts": len(o.data.vertices), "tris": len(o.data.polygons), "convex": convex, "closed": closed,
                     "min_cm": v.min(0).round(0).tolist(), "max_cm": v.max(0).round(0).tolist()})
R["ucx_count"] = len(ucx); R["ucx_all_convex_closed"] = all(u["convex"] and u["closed"] for u in R["ucx"])
# emissive area share (area-weighted sampling of E through the mesh UVs)
E = None
try:
    img = bpy.data.images.load(os.path.join(TEX, "T_DominionMothership_E.png")); W, H = img.size
    px = np.empty(W * H * 4, np.float32); img.pixels.foreach_get(px); E = px.reshape(H, W, 4)[:, :, :3].max(-1)
except Exception as ex:
    R["emissive_err"] = str(ex)
if E is not None:
    me = sm.data; uv = me.uv_layers.active.data; M = sm.matrix_world
    tot = 0.0; emi = 0.0
    for p in me.polygons:
        a = p.area
        uvs = [uv[li].uv for li in p.loop_indices]
        pts = [(sum(u[0] for u in uvs) / 3, sum(u[1] for u in uvs) / 3)] + [((2 * uvs[i][0] + uvs[(i + 1) % 3][0] + uvs[(i + 2) % 3][0]) / 4, (2 * uvs[i][1] + uvs[(i + 1) % 3][1] + uvs[(i + 2) % 3][1]) / 4) for i in range(3)]
        lit = np.mean([E[min(H - 1, max(0, int(v_ * H))), min(W - 1, max(0, int(u_ * W)))] > 0 for (u_, v_) in pts])
        tot += a; emi += a * lit
    R["emissive_area_share_pct"] = round(100 * emi / tot, 2); R["surface_area_m2"] = round(tot, 0)
# GLB
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=os.path.join(EXP, "SM_DominionMothership.glb"))
g = [o for o in bpy.context.scene.objects if o.type == 'MESH']
gv = np.array([list(g[0].matrix_world @ v.co) for v in g[0].data.vertices])
R["glb"] = {"mesh_objects": [o.name for o in g], "tris": sum(len(o.data.polygons) for o in g), "bounds_m_min": gv.min(0).round(2).tolist(), "bounds_m_max": gv.max(0).round(2).tolist(),
            "sockets": sorted(o.name for o in bpy.context.scene.objects if o.name.startswith("SOCKET_")),
            "images": sorted(i.name for i in bpy.data.images)}
json.dump(R, open(OUTJ, "w"), indent=1)
print("VERIFY_OK")

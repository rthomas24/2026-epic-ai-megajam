# SPDX-License-Identifier: MIT
# RiftHero step 1: measure the Rodin mesh (read-only source), save height maps, render the "before" clay sheet.
#   blender --background --factory-startup --python rh_analyze.py
import os, sys
import bpy, numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rh_common as C

bpy.ops.wm.read_factory_settings(use_empty=True)
ob = C.import_rodin("SM_RiftRodin")
me = ob.data
V = np.array([v.co[:] for v in me.vertices])
lo, hi = V.min(0), V.max(0)
res = {"verts": len(V), "tris": sum(len(p.vertices) - 2 for p in me.polygons),
       "lo_cm": (lo * 100).round(1).tolist(), "hi_cm": (hi * 100).round(1).tolist(),
       "centroid_cm": (V.mean(0) * 100).round(1).tolist()}
bvh = BVHTree.FromObject(ob, bpy.context.evaluated_depsgraph_get())
# symmetry: mean distance from mirrored verts to the surface
samp = V[:: max(1, len(V) // 4000)]
for y0 in (-0.02, -0.01, 0.0, 0.01, 0.02):
    d = [bvh.find_nearest(Vector((p[0], 2 * y0 - p[1], p[2])))[3] for p in samp]
    res["sym_err_cm_y0=%+.2f" % y0] = round(float(np.mean(d)) * 100, 2)

step = 0.04
xs = np.arange(-2.52, 2.5201, step); ys = np.arange(-2.12, 2.1201, step); zs = np.arange(-1.16, 1.1601, step)
top = np.full((len(xs), len(ys)), np.nan); bot = top.copy()
for i, x in enumerate(xs):
    for j, y in enumerate(ys):
        h = bvh.ray_cast(Vector((x, y, 5)), Vector((0, 0, -1)))
        if h[0] is not None:
            top[i, j] = h[0].z
            bot[i, j] = bvh.ray_cast(Vector((x, y, -5)), Vector((0, 0, 1)))[0].z
rear = np.full((len(ys), len(zs)), np.nan); nose = rear.copy()      # x of first hit seen from behind / from the nose
side = np.full((len(xs), len(zs)), np.nan)                           # y of first hit seen from +Y... (|y| max)
for j, y in enumerate(ys):
    for k, z in enumerate(zs):
        h = bvh.ray_cast(Vector((-5, y, z)), Vector((1, 0, 0)))
        if h[0] is not None:
            rear[j, k] = h[0].x
            nose[j, k] = bvh.ray_cast(Vector((5, y, z)), Vector((-1, 0, 0)))[0].x
for i, x in enumerate(xs):
    for k, z in enumerate(zs):
        h = bvh.ray_cast(Vector((x, 5, z)), Vector((0, -1, 0)))
        if h[0] is not None:
            side[i, k] = h[0].y
np.savez(os.path.join(C.ANALYSIS, "rodin_maps.npz"), xs=xs, ys=ys, zs=zs, top=top, bot=bot, rear=rear, nose=nose, side=side)

rr = C.skill("blender-mcp/scripts/review_renders.py")
sheet = rr["main"](targets=[ob.name], out_dir=C.REVIEW, label="00_rodin_original", mode="clay", views=C.VIEWS, resolution=800)
res["sheet"] = sheet.get("sheet") or sheet
rep = C.skill("blender-mcp/scripts/asset_report.py")["main"](targets=[ob.name])
asm = C.skill("blender-mcp/scripts/assembly_check.py")["main"](targets=[ob.name])
res["report_issues"] = len(rep.get("issues", [])); res["assembly"] = {k: asm[k] for k in list(asm)[:5]}
C.out(res)

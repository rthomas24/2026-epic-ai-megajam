# SPDX-License-Identifier: MIT
# Finds candidate socket positions on a Rodin ship: engine nozzles (emissive clusters) and gun muzzles
# (forward-most tips). Prints raw Blender coordinates in cm. Read-only on the source.
#   blender --background --factory-startup --python rs_analyze.py -- <Ship>
import json
import os
import sys

import bpy
import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rs_common as C


def load(ship):
    s = C.SHIPS[ship]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=os.path.join(s["src"], "SM_%s.fbx" % s["stem"]))
    obs = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    return s, obs


def world_co(ob):
    me = ob.data
    n = len(me.vertices)
    co = np.empty(n * 3, np.float32)
    me.vertices.foreach_get("co", co)
    co = co.reshape(n, 3)
    M = np.array(ob.matrix_world)
    return me, co @ M[:3, :3].T + M[:3, 3]


def clusters(pts, w, radius):
    out = []
    left = np.ones(len(pts), bool)
    for i in np.argsort(-w):
        if not left[i]:
            continue
        m = left & (np.linalg.norm(pts - pts[i], axis=1) < radius)
        c = (pts[m] * w[m, None]).sum(0) / w[m].sum()
        m = left & (np.linalg.norm(pts - c, axis=1) < radius)
        if m.sum() == 0:
            continue
        c = (pts[m] * w[m, None]).sum(0) / w[m].sum()
        out.append((c, float(w[m].sum()), int(m.sum()), pts[m].min(0), pts[m].max(0)))
        left &= ~m
    return out


def main():
    ship = sys.argv[sys.argv.index("--") + 1]
    s, obs = load(ship)
    res = {"ship": ship, "objects": [(o.name, len(o.data.polygons), [round(v, 3) for v in o.scale],
                                      [round(v, 3) for v in o.rotation_euler]) for o in obs]}
    ob = obs[0]
    me, co = world_co(ob)
    mn, mx = co.min(0), co.max(0)
    L = mx - mn
    res["bbox_cm"] = [(mn * 100).round(1).tolist(), (mx * 100).round(1).tolist()]
    res["uv_layers"] = [u.name for u in me.uv_layers]
    res["materials"] = [m.name if m else None for m in me.materials]
    nl = len(me.loops)
    lv = np.empty(nl, np.int32)
    me.loops.foreach_get("vertex_index", lv)
    npoly = len(me.polygons)
    ls = np.empty(npoly, np.int32)
    me.polygons.foreach_get("loop_start", ls)
    area = np.empty(npoly, np.float32)
    me.polygons.foreach_get("area", area)
    cen = (co[lv[ls]] + co[lv[ls + 1]] + co[lv[ls + 2]]) / 3.0
    epath = os.path.join(s["src"], "T_%s_E.png" % s["stem"])
    if os.path.isfile(epath) and me.uv_layers:
        uv = np.empty(nl * 2, np.float32)
        me.uv_layers[0].data.foreach_get("uv", uv)
        uv = uv.reshape(nl, 2)
        fuv = (uv[ls] + uv[ls + 1] + uv[ls + 2]) / 3.0
        img = bpy.data.images.load(epath)
        W, H = img.size
        px = np.empty(W * H * 4, np.float32)
        img.pixels.foreach_get(px)
        px = px.reshape(H, W, 4)
        xi = np.clip((fuv[:, 0] % 1.0 * W).astype(int), 0, W - 1)
        yi = np.clip((fuv[:, 1] % 1.0 * H).astype(int), 0, H - 1)
        e = px[yi, xi, :3]
        lum = e.max(1)
        res["emissive_area_share"] = round(float((area * (lum > 0.25)).sum() / area.sum()), 4)
        m = lum > 0.5
        if m.any():
            cl = clusters(cen[m], (area * lum)[m], 0.22)
            cl.sort(key=lambda c: -c[1])
            res["emissive_clusters_cm"] = [
                dict(c=(c[0] * 100).round(0).tolist(), w=round(c[1] * 1e4, 1), n=c[2],
                     mn=(c[3] * 100).round(0).tolist(), mx=(c[4] * 100).round(0).tolist()) for c in cl[:12]]
    tips = {}
    for name, ax, sign in (("+X", 0, 1), ("-X", 0, -1), ("+Y", 1, 1), ("-Y", 1, -1)):
        v = co[:, ax] * sign
        pts = co[v > v.max() - 0.3 * L[ax]]
        lat = 1 - ax
        best = {}
        for b, p in zip(np.round(pts[:, lat] / 0.08).astype(int), pts):
            if b not in best or p[ax] * sign > best[b][ax] * sign:
                best[b] = p
        tips[name] = sorted([[int(round(float(x) * 100)) for x in p] for p in best.values()], key=lambda p: p[lat])
    print("__RESULT__" + json.dumps(res))
    for k, v in tips.items():
        print("__TIPS__ %s %s" % (k, json.dumps(v)))


main()

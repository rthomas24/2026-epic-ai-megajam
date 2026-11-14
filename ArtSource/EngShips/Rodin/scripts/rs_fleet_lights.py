# SPDX-License-Identifier: MIT
# Triad Drift - Null Armada readability (Lead, Nov 14): white running lights and wider seams in the EMISSIVE MAP of
# an enemy ship, no new geometry. The white "eyes" Rodin painted sit on the nose only, so from behind or the side an
# enemy showed no white at all; and its thin orange seams broke into flecks at 20-80 m.
#   blender --background --factory-startup --python rs_fleet_lights.py -- <Ship> [<Ship> ...]
# Reads the ship's Rodin mesh and T_<Ship>_E.png (rs_common.SHIPS), writes ArtSource/EngShips/FleetLights/T_<Ship>_E.png
# (the import script takes the emissive map from there when the file exists) and a preview sheet beside it.
#   1. seams: the emissive map is dilated a little (a max filter of SEAM_PX texels, at 60 %), so the lines are wider
#      and hold together when the map is minified;
#   2. running lights: white texels wherever the surface lies inside small boxes placed by rule on the hull's own
#      extremes: the rear corners, the wing tips / flanks at stations along the hull, the top of the tail. Sizes
#      scale with the hull, counts with its length (LIGHTS).
import math
import os
import sys

import bpy
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(HERE)), "RiftHero", "scripts"))
import rs_common as C
import rh_closeparts as CP
import rh_textures as T

OUT = os.path.join(C.OUT, "FleetLights")
# ship -> flank stations along the hull, light half-size as a fraction of the length, seam dilation in texels (at the
# map's own resolution)
LIGHTS = {
    "NullFighter": dict(stations=1, size=0.008, seam_px=2),
    "NullInterceptor": dict(stations=1, size=0.008, seam_px=2),
    "NullBomber": dict(stations=2, size=0.006, seam_px=2),
    "NullGunship": dict(stations=3, size=0.006, seam_px=2),
    "NullCarrier": dict(stations=5, size=0.005, seam_px=0)      # its map is already a fifth lit: no dilation,
}
WHITE = (0.55, 0.55, 0.55)       # dimmer than the eyes (review: the first lights were blown-out blobs brighter than the eyes)


def dilate(a, r):
    out = a.copy()
    for dy in range(-r, r + 1):
        for dx in range(-r, r + 1):
            if dx * dx + dy * dy > r * r or (dx == 0 and dy == 0):
                continue
            out = np.maximum(out, np.roll(np.roll(a, dy, 0), dx, 1))
    return out


def main(ship):
    s = C.SHIPS[ship]
    cfg = LIGHTS[ship]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=os.path.join(s["src"], "SM_%s.fbx" % s["stem"]))
    ob = next(o for o in bpy.data.objects if o.type == "MESH")
    ob.data.transform(ob.matrix_world)
    ob.matrix_world.identity()
    V = np.array([v.co[:] for v in ob.data.vertices])
    lo, hi = V.min(0), V.max(0)
    L = hi[0] - lo[0]
    h = cfg["size"] * L

    def box(p, k=1.0):
        # (x0, x1, |y| max, |y| min, z0, z1) around a point; |y| is mirrored, so one box lights both sides
        return (p[0] - h * k, p[0] + h * k, abs(p[1]) + h * k, max(0.0, abs(p[1]) - h * k), p[2] - h * k, p[2] + h * k)

    boxes = []
    R = V[V[:, 1] > 0]                                         # one side; the boxes mirror
    rear = R[R[:, 0] < lo[0] + 0.10 * L]
    if len(rear):
        boxes.append(box(rear[np.argmax(rear[:, 1])], 1.3))    # rear corners
        boxes.append(box(rear[np.argmax(rear[:, 2])], 1.0))    # rear top
    for i in range(cfg["stations"]):                           # flanks / wing tips at stations along the hull
        x0 = lo[0] + L * (0.18 + 0.64 * (i + 0.5) / cfg["stations"])
        st = R[np.abs(R[:, 0] - x0) < 0.06 * L]
        if len(st):
            boxes.append(box(st[np.argmax(st[:, 1])], 1.0))
    top = V[np.abs(V[:, 1]) < 0.08 * (hi[1] - lo[1])]
    aft = top[top[:, 0] < lo[0] + 0.45 * L]
    if len(aft):
        boxes.append(box(aft[np.argmax(aft[:, 2])], 1.0))      # the top of the tail / tower
        boxes.append(box(aft[np.argmin(aft[:, 2])], 1.0))      # and the keel under it

    e = T.load(os.path.join(s["src"], "T_%s_E.png" % s["stem"]))
    res = {"ship": ship, "size_m": [round(float(x), 2) for x in (hi - lo)], "boxes": len(boxes), "light_half_m": round(float(h), 3)}
    wide = dilate(e, cfg["seam_px"]) if cfg["seam_px"] else e
    out = np.maximum(e, wide * 0.5)
    mask = CP._paint_mask(ob, e.shape[:2], None, boxes)
    out[mask] = WHITE
    res["light_px"] = int(mask.sum())
    res["lit_frac_before"] = round(float((e.max(-1) > 0.2).mean()), 4)
    res["lit_frac_after"] = round(float((out.max(-1) > 0.2).mean()), 4)
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, "T_%s_E.png" % s["stem"])
    T.save(out, path, True)
    res["out"] = path
    print("__RESULT__" + repr(res))


for name in sys.argv[sys.argv.index("--") + 1:]:
    main(name)

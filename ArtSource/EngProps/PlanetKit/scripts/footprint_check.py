# SPDX-License-Identifier: MIT
"""
footprint_check.py - measures every PlanetKit mesh against the gameplay footprint table and prints PASS / FAIL.

    blender --background --factory-startup --python footprint_check.py [-- --assets a,b]

For each asset (its saved .blend, the exact mesh that is exported):
  bounds   : X / Y / Z size in cm and the pivot (bottom centre: min z = 0 and x / y centred; centre: all centred)
             PASS when every fixed dimension is within +-2 cm.
  top      : walkable tops - a grid of rays straight down over the inner 80 % of the footprint; PASS when >= 90 % of
             the hits are within +-2 cm of the walkable height (flagstone joints, glyph grooves and the node socket are
             the expected misses).
  sides    : rays from each box face straight in, on a grid between 5 % and 90 % of the height; reports the median
             and 90th-percentile gap between the box and the mesh. For "tight" sides (gameplay collides with them and
             the mesh must sit on the box) PASS needs median <= 2 cm. Other sides (rock, wreck, decoration) are
             reported only.
Writes review/footprint_check.json and prints one line per asset plus a summary.
"""

import json
import os
import sys

import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)

# name: (dims cm (None = free), pivot, walkable top cm or None, tight sides, top sampling region (fraction of the
# footprint) or a list of (x0, x1, y0, y1) in cm)
SPEC = {
    "SM_Cind_OutpostWall": ((150, 1500, 600), "bottom", None, ["-X"], None),
    "SM_Cind_OutpostWall_Doorway": ((150, 600, 600), "bottom", None, ["-X", "+X"], None, {"-X": (220, 300), "+X": (220, 300)}),
    "SM_Cind_JammerTower": ((None, None, 4200), "bottom", None, [], None),
    "SM_Cind_Crate_A": ((300, 220, 140), "bottom", 140, ["-X", "+X", "-Y", "+Y"], 0.7),
    "SM_Cind_Crate_B": ((360, 300, 140), "bottom", 140, ["-X", "+X", "-Y", "+Y"], 0.7),
    "SM_Verd_Wreck_Hull": ((2600, 900, 700), "bottom", None, [], None),
    "SM_Verd_Wreck_Engine": ((900, 700, 700), "bottom", None, [], None),
    "SM_Verd_Wreck_Wing": ((600, 160, 240), "bottom", None, [], None),
    "SM_Verd_Wreck_Fin": ((500, 160, 220), "bottom", None, [], None),
    "SM_Verd_RelayPlatform_S": ((600, 600, 110), "bottom", 110, ["-X", "+X", "-Y", "+Y"], 0.8),
    "SM_Verd_RelayPlatform_M": ((600, 600, 290), "bottom", 290, ["-X", "+X", "-Y", "+Y"], 0.8),
    "SM_Verd_RelayPlatform_L": ((1000, 1000, 330), "bottom", 330, ["-X", "+X", "-Y", "+Y"], 0.8),
    "SM_Verd_Plateau": ((2600, 2600, 600), "bottom", 600, ["-X", "+X", "-Y", "+Y"], 0.8),
    "SM_Verd_RuinPillar": ((120, 120, 600), "bottom", None, [], None),
    "SM_Verd_NodeStub": ((320, 320, 180), "bottom", 180, [], [(-125, -68, -125, 125), (68, 125, -125, 125), (-68, 68, -125, -68), (-68, 68, 68, 125)]),
    "SM_Verd_Log": ((500, 110, 140), "bottom", None, [], None),
    "SM_Cind_ChokeRock_A": ((400, 1200, 600), "bottom", None, [], None),
    "SM_Cind_ChokeRock_B": ((400, 1400, 600), "bottom", None, [], None),
    "SM_Cind_SteppingStone": ((280, 280, 160), "bottom", 160, [], 0.45),
    "SM_Cind_CollapseSlab": ((610, 320, 60), "bottom", 60, ["-X", "+X", "-Y", "+Y"], 0.8),
    "SM_Cind_DebrisChunk": ((None, None, None), "centre", None, [], None),
    "SM_Console": ((60, 60, 120), "bottom", None, [], None),
    "SM_Door": ((30, 620, 600), "bottom", None, ["-X", "+X"], None),
    "SM_ExtractionCore": ((55, 55, 110), "centre", None, [], None),
    "SM_RoutePost": ((34, 34, 154), "bottom", None, [], None),
    "SM_Trailhead": ((500, 500, 480), "bottom", 40, [], [(-240, -60, -240, 240), (60, 240, -240, 240)]),
}
TOL = 2.0


def analyse(name, spec):
    dims, pivot, top_h, tight, region = spec[:5]
    side_band = spec[5] if len(spec) > 5 else {}       # |u| range (cm) to sample per side, e.g. the doorway posts
    obj = bpy.data.objects[name]
    me = obj.data
    co = np.empty(len(me.vertices) * 3)
    me.vertices.foreach_get("co", co)
    co = co.reshape(-1, 3) * 100.0                      # cm
    lo, hi = co.min(axis=0), co.max(axis=0)
    size = hi - lo
    res = {"asset": name, "size_cm": [round(float(v), 1) for v in size], "min_cm": [round(float(v), 1) for v in lo],
           "max_cm": [round(float(v), 1) for v in hi]}
    ok = True
    dim_ok = []
    for i in range(3):
        if dims[i] is None:
            dim_ok.append(None)
            continue
        good = abs(size[i] - dims[i]) <= TOL
        dim_ok.append(bool(good))
        ok &= good
    if pivot == "bottom":
        piv_ok = abs(lo[2]) <= 0.5 and abs(lo[0] + hi[0]) <= 1.0 and abs(lo[1] + hi[1]) <= 1.0
    else:
        piv_ok = all(abs(lo[i] + hi[i]) <= 1.0 for i in range(3))
    res["dims_ok"] = dim_ok
    res["pivot"] = pivot
    res["pivot_ok"] = bool(piv_ok)
    ok &= piv_ok
    # triangulate first (beauty, as the FBX export does): BVHTree.FromPolygons fans long n-gons with collinear
    # bevel vertices into slivers and misses hits
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.triangulate(bm, faces=list(bm.faces), quad_method="BEAUTY", ngon_method="BEAUTY")
    for v in bm.verts:
        v.co *= 100.0
    bvh = BVHTree.FromBMesh(bm)
    bm.free()
    if top_h is not None:
        hits = []
        rects = region if isinstance(region, list) else [(lo[0] * region, hi[0] * region, lo[1] * region, hi[1] * region)]
        for (x0, x1, y0, y1) in rects:
            # sample offsets from a low-discrepancy (golden ratio) sequence: a regular grid can line up with the
            # flagstone joints and report a joint for a whole row
            for i in range(625):
                x = x0 + (x1 - x0) * ((i * 0.6180339887 + 0.13) % 1.0)
                y = y0 + (y1 - y0) * ((i * 0.7548776662 + 0.29) % 1.0)
                if True:
                    r = bvh.ray_cast(Vector((x, y, hi[2] + 50.0)), Vector((0, 0, -1)))
                    if r[0] is not None:
                        hits.append(r[0].z)
        hits = np.array(hits)
        share = float((np.abs(hits - top_h) <= TOL).mean()) if len(hits) else 0.0
        res["top"] = {"walkable_cm": top_h, "samples": int(len(hits)), "share_within_2cm": round(share, 3),
                      "max_cm": round(float(hits.max()), 2) if len(hits) else None,
                      "median_cm": round(float(np.median(hits)), 2) if len(hits) else None}
        top_ok = share >= 0.9 and (hits.max() <= top_h + TOL)
        res["top"]["ok"] = bool(top_ok)
        ok &= top_ok
    sides = {}
    zmin, zmax = lo[2] + 0.05 * size[2], lo[2] + 0.9 * size[2]
    for key, axis, sgn in (("-X", 0, -1), ("+X", 0, 1), ("-Y", 1, -1), ("+Y", 1, 1)):
        other = 1 - axis
        gaps = []
        face = hi[axis] if sgn > 0 else lo[axis]
        us = np.linspace(lo[other] + 0.05 * size[other], hi[other] - 0.05 * size[other], 30)
        if key in side_band:
            a0, a1 = side_band[key]
            us = np.concatenate([np.linspace(-a1 + 4, -a0 - 4, 15), np.linspace(a0 + 4, a1 - 4, 15)])
        for u in us:
            for z in np.linspace(zmin, zmax, 20):
                o = [0.0, 0.0, z]
                o[axis] = face + sgn * 10.0
                o[other] = u
                d = [0.0, 0.0, 0.0]
                d[axis] = -sgn
                r = bvh.ray_cast(Vector(o), Vector(d))
                if r[0] is not None:
                    gaps.append(abs(face - r[0][axis]))
                else:
                    gaps.append(size[axis])
        gaps = np.array(gaps)
        s = {"median_gap_cm": round(float(np.median(gaps)), 2), "p90_gap_cm": round(float(np.percentile(gaps, 90)), 2),
             "share_within_2cm": round(float((gaps <= TOL).mean()), 3), "tight": key in tight}
        if key in tight:
            s["ok"] = bool(s["median_gap_cm"] <= TOL)
            ok &= s["ok"]
        sides[key] = s
    res["sides"] = sides
    res["PASS"] = bool(ok)
    return res


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    names = sorted(SPEC)
    if "--assets" in argv:
        names = [n for n in argv[argv.index("--assets") + 1].split(",") if n]
    out = []
    for n in names:
        path = os.path.join(KIT, n, n + ".blend")
        if not os.path.isfile(path):
            out.append({"asset": n, "PASS": False, "error": "no .blend"})
            print("FOOTPRINT FAIL %-30s no .blend" % n)
            continue
        bpy.ops.wm.open_mainfile(filepath=path)
        r = analyse(n, SPEC[n])
        out.append(r)
        top = r.get("top")
        tight = {k: v["median_gap_cm"] for k, v in r["sides"].items()}
        print("FOOTPRINT %s %-30s size %s cm (spec %s) pivot %s %s | top %s | side median gaps %s" % (
            "PASS" if r["PASS"] else "FAIL", n, r["size_cm"], list(SPEC[n][0]), r["pivot"],
            "ok" if r["pivot_ok"] else "BAD",
            ("%s cm: %.0f%% within 2 cm, max %s" % (top["walkable_cm"], top["share_within_2cm"] * 100, top["max_cm"])) if top else "-",
            tight))
    def clean(o):
        if isinstance(o, float) and (o != o or o in (float("inf"), float("-inf"))):
            return None
        if isinstance(o, dict):
            return {k: clean(v) for k, v in o.items()}
        if isinstance(o, list):
            return [clean(v) for v in o]
        return o
    with open(os.path.join(KIT, "review", "footprint_check.json"), "w", encoding="utf-8") as fh:
        json.dump(clean(out), fh, indent=2, default=lambda o: o.item() if hasattr(o, "item") else str(o))
    print("FOOTPRINT SUMMARY %d / %d PASS" % (sum(1 for r in out if r.get("PASS")), len(out)))


main()

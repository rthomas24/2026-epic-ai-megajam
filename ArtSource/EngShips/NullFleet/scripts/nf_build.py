# SPDX-License-Identifier: MIT
"""
nf_build.py - builds ONE Null Fleet ship (or carrier part), headless.

    blender --background --factory-startup --python nf_build.py -- --asset SM_Null_Fighter --tag r1 [--stage blockout]

--stage blockout: builds the forms only (no detail pass, no bake) and writes review/<asset>_<tag>_clay_sheet.png.
--stage full (default): detail, bake (BC / N / ORM / E), checks, review sheets, FBX + manifest in <asset>/.
"""

import json
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import bpy  # noqa: E402

import nf_lib as L  # noqa: E402
import nf_ships as S  # noqa: E402


def main():
    import argparse
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser()
    p.add_argument("--asset", required=True, choices=sorted(S.BUILDERS))
    p.add_argument("--tag", default="r1")
    p.add_argument("--stage", default="full", choices=("blockout", "full"))
    p.add_argument("--samples", type=int, default=32)
    a = p.parse_args(argv)
    t0 = time.time()
    fn, tex, budget, ao = S.BUILDERS[a.asset]
    P, sockets, extra = fn(a.asset, a.tag, a.stage)
    if a.stage == "blockout":
        obj = P.join()
        me = obj.data
        pts = L.mesh_points(me)
        size = [round(float(v) * 100.0, 1) for v in (pts.max(axis=0) - pts.min(axis=0))]
        lo = [round(float(v) * 100.0, 1) for v in pts.min(axis=0)]
        hi = [round(float(v) * 100.0, 1) for v in pts.max(axis=0)]
        os.makedirs(L.REVIEW_DIR, exist_ok=True)
        clay = L.rv["main"](targets=[a.asset], out_dir=L.REVIEW_DIR, label="{}_{}".format(a.asset, a.tag), mode="clay",
                            views=("front_3q", "front", "right", "top", "back_3q", "low_3q"), columns=3, resolution=420)
        mat = L.rv["main"](targets=[a.asset], out_dir=L.REVIEW_DIR, label="{}_{}_flat".format(a.asset, a.tag),
                           mode="material", views=("front_3q", "back_3q"), columns=2, resolution=420)
        res = {"asset": a.asset, "stage": "blockout", "tris": L.tri_count(me), "size_cm_blender_xyz": size,
               "min_cm": lo, "max_cm": hi, "clay": clay["sheet"], "flat": mat["sheet"], "extra": extra,
               "seconds": round(time.time() - t0, 1)}
    else:
        health = L.finish_hard_surface(P, a.asset, tex, a.tag, budget, ao_distance=ao, sockets=sockets,
                                       recentre_com=(False if a.asset in S.KEEP_ORIGIN else
                                                     "bbox" if a.asset in S.BBOX_ORIGIN else True), extra_manifest=extra,
                                       samples=a.samples, tint=(0.55, 0.5, 0.5), tint_strength=0.6)
        res = {"asset": a.asset, "stage": "full", "checks": health["checks"],
               "emissive_share": health["emissive_surface_share"], "review": health["review"].get("review"),
               "clay": health["review"].get("clay"), "com_shift_m": health["info"].get("com_shift_m"),
               "seconds": round(time.time() - t0, 1)}
    print("__RESULT__" + json.dumps(res, default=str))


if __name__ == "__main__":
    main()

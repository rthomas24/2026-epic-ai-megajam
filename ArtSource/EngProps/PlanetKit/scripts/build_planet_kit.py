# SPDX-License-Identifier: MIT
"""
build_planet_kit.py - builds PlanetKit assets headless, one fresh scene per asset.

    blender --background --factory-startup --python build_planet_kit.py -- --assets SM_Cind_OutpostWall,SM_Door --tag r1 [--nobake]
    blender ... -- --assets all --tag r3

--nobake: clay / procedural-material review round (no UVs, bake, FBX); the .blend and review sheets are still written.
Writes ArtSource/EngProps/PlanetKit/<asset>/ (.blend, .fbx, .manifest.json, T_*.png) and review/<asset>_<tag>_*.png,
review/<asset>_health.json. Everything is seeded: a re-run reproduces the same asset.
"""

import json
import os
import sys
import traceback

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import bpy  # noqa: E402

import pk_lib as L  # noqa: E402
import pk_ext  # noqa: E402
import pk_cinder  # noqa: E402
import pk_verdance  # noqa: E402
import pk_small  # noqa: E402
import pk_wreck  # noqa: E402

BUILDERS = {}
BUILDERS.update(pk_cinder.BUILDERS)
BUILDERS.update(pk_verdance.BUILDERS)
BUILDERS.update(pk_small.BUILDERS)
BUILDERS.update(pk_wreck.BUILDERS)      # round-3 rebuild of the hull and engine
import pk_r9  # noqa: E402
BUILDERS.update(pk_r9.BUILDERS)         # round-9 rebuild: wreck hull / wing / fin, log, collapse slab, debris


def build_one(asset, tag, do_bake):
    spec = BUILDERS[asset]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    P = L.Part(asset, asset)
    spec["fn"](P)
    return pk_ext.finish_pk(P, asset, spec["family"], tag, spec["dims"], tex=spec.get("tex", 2048),
                            budget=spec.get("budget", (500, 40000)), pivot=spec.get("pivot", "bottom"),
                            fit_axes=spec.get("fit_axes", (True, True, True)), do_bake=do_bake,
                            ao_distance=spec.get("ao", 1.0), detail_uv=spec.get("detail", False),
                            extra_manifest=spec.get("manifest"), snap=spec.get("snap", False))


def main():
    import argparse
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser()
    p.add_argument("--assets", required=True)
    p.add_argument("--tag", default="r1")
    p.add_argument("--nobake", action="store_true")
    a = p.parse_args(argv)
    names = sorted(BUILDERS) if a.assets == "all" else [n.strip() for n in a.assets.split(",") if n.strip()]
    out = []
    for n in names:
        try:
            h = build_one(n, a.tag, not a.nobake)
            c = h["checks"]
            out.append({"asset": n, "ok": True, "tris": c["tris"], "fit": c["fit"], "errors": c["asset_report_errors"],
                        "floating_ok": c["no_floating_parts"], "export": c["export_status"],
                        "emissive_share": h.get("emissive_surface_share"), "review": h["review"]["review"]})
        except Exception:
            traceback.print_exc()
            out.append({"asset": n, "ok": False, "error": traceback.format_exc()[-600:]})
        print("__ASSET__" + json.dumps(out[-1], default=str))
        sys.stdout.flush()
    print("__RESULT__" + json.dumps(out, default=str))


if __name__ == "__main__":
    main()

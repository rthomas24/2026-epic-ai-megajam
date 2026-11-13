# SPDX-License-Identifier: MIT
"""
build_space_dressing.py - builds ONE asset of the SpaceDressing kit, start to finish, headless.

    blender --background --factory-startup --python build_space_dressing.py -- --asset SM_Ast_Ice_01 [--tag r2]

Writes ArtSource/EngProps/SpaceDressing/<asset>/ (.blend, .fbx, .manifest.json, T_*.png) and
review/<asset>_<tag>_review.png, review/<asset>_<tag>_clay_sheet.png, review/<asset>_health.json.
Everything is seeded: a re-run reproduces the same asset.
"""

import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import sd_asteroids  # noqa: E402
import sd_hard  # noqa: E402

ALL = sorted(list(sd_asteroids.SPECS) + list(sd_hard.BUILDERS))


def main():
    import argparse
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser()
    p.add_argument("--asset", required=True, choices=ALL)
    p.add_argument("--tag", default="r1")
    a = p.parse_args(argv)
    if a.asset in sd_asteroids.SPECS:
        res = sd_asteroids.build(a.asset, a.tag)
    else:
        res = sd_hard.build(a.asset, a.tag)
    print("__RESULT__" + json.dumps({"asset": a.asset, "checks": res.get("checks"),
                                     "emissive_share": res.get("emissive_surface_share"),
                                     "review": res.get("review", {}).get("review")}, default=str))


if __name__ == "__main__":
    main()

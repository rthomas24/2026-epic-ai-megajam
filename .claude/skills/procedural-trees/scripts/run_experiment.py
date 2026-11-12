#!/usr/bin/env python3
"""Shell-side driver for experiment.py: measure what grower or foliage overrides do to one species.

    python run_experiment.py --port 8017 ../biomes/verdance.json Cacao [--stage 1.0] [--seed 61]
        [--grower name="(TrunkGrowth=(BranchTargetLength=1.0))" ...]      FPVGrowerParams text
        [--foliage name="(SpacingSettings=(BranchDensity=7))" ...]         parametric foliage text
        [--param branch_angle_deg=30 ...]                                  normalised parameter overrides
        [--keep name]                                                      leave that result on disk for a capture

Prints one line per override: height, crown width and triangle count, next to the species' targets.
Use it before changing pve_lib.py: one run answers "which knob does that?" in seconds.
"""

import argparse
import json
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))


def pairs(items):
    return dict(item.split("=", 1) for item in items)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("biome")
    parser.add_argument("species")
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--stage", type=float, default=1.0)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--grower", nargs="*", default=[])
    parser.add_argument("--foliage", nargs="*", default=[])
    parser.add_argument("--param", nargs="*", default=[])
    parser.add_argument("--keep", default="")
    options = parser.parse_args()

    with open(options.biome, encoding="utf-8") as handle:
        biome = json.load(handle)
    arguments = {"biome": biome, "species": options.species, "stage": options.stage, "seed": options.seed,
                 "overrides": pairs(options.grower) or {"base": "()"}, "foliage": pairs(options.foliage),
                 "params": {key: float(value) for key, value in pairs(options.param).items()}}
    if options.keep:
        arguments["keep"] = options.keep
    with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False, encoding="utf-8") as handle:
        json.dump(arguments, handle)
        args_file = handle.name
    try:
        run = subprocess.run([sys.executable, os.path.join(HERE, "ue_exec.py"), "--port", str(options.port), "--timeout", "1200",
                              "--lib", os.path.join(HERE, "pve_lib.py"), os.path.join(HERE, "experiment.py"), "--args", "@" + args_file],
                             capture_output=True, text=True)
    finally:
        os.unlink(args_file)
    try:
        result = json.loads(run.stdout)
        rows = result["results"]
    except (ValueError, KeyError, TypeError):
        print("FAILED:\n%s\n%s" % (run.stdout[-3000:], run.stderr[-1000:]))
        return 1
    print("target: %s" % result["target"])
    for name, row in rows.items():
        print("  %-28s %s" % (name, row))
    if result.get("kept"):
        print("kept: %s" % result["kept"])
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""Shell-side driver: (re)build the verification map from a biome report.

    python run_demo_map.py --port 8017 ../biomes/verdance.report.json [--map /Game/.../L_VegetationDemo]
                           [--forest Kapok=6 BrazilNut=10 ...] [--exposure 1.0] [--layout out.json]

Writes the layout the editor returns (where every tree stands, the mannequin's height) to --layout
(default: <report>.layout.json): verify_biome.py and the capture commands read positions from it.
"""

import argparse
import json
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("report")
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--map", default="/Game/TriadDrift/Gameplay/Vegetation/Maps/L_VegetationDemo")
    parser.add_argument("--forest", nargs="*", default=[])
    parser.add_argument("--exposure", type=float, default=1.0, help="fixed exposure of the map, EV100")
    parser.add_argument("--layout", default="")
    options = parser.parse_args()

    with open(options.report, encoding="utf-8") as handle:
        report = json.load(handle)
    arguments = {"report": report, "map": options.map, "exposure_ev100": options.exposure,
                 "forest": {item.split("=")[0]: int(item.split("=")[1]) for item in options.forest}}
    with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False, encoding="utf-8") as handle:
        json.dump(arguments, handle)
        args_file = handle.name
    try:
        run = subprocess.run([sys.executable, os.path.join(HERE, "ue_exec.py"), "--port", str(options.port), "--timeout", "900",
                              os.path.join(HERE, "build_demo_map.py"), "--args", "@" + args_file], capture_output=True, text=True)
    finally:
        os.unlink(args_file)
    try:
        layout = json.loads(run.stdout)
        layout["map"]
    except (ValueError, KeyError, TypeError):
        print("FAILED:\n%s\n%s" % (run.stdout[-3000:], run.stderr[-1000:]))
        return 1
    layout_path = options.layout or os.path.splitext(options.report)[0].replace(".report", "") + ".layout.json"
    with open(layout_path, "w", encoding="utf-8") as handle:
        json.dump(layout, handle, indent=1)
    print("map %s: %d line-up trees, %d forest instances, mannequin %.1f cm, line-up %.0f m long" % (
        layout["map"], len(layout["lineup"]), layout.get("forest_instances", 0), layout["mannequin_height_cm"], layout["lineup_length_cm"] / 100.0))
    print("layout: %s" % layout_path)
    return 0


if __name__ == "__main__":
    sys.exit(main())

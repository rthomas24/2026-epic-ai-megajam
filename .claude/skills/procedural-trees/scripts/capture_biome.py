#!/usr/bin/env python3
"""Shell-side: take the standard set of verification captures of a demo map, from its layout file.

    python capture_biome.py --port 8017 ../biomes/verdance.layout.json --out <folder>

Shots (PNG, 1600 x 900):
  01_lineup          every mature tree and its younger stages, the mannequin at the left end
  02_eye_understorey the player's view on foot (eye height 154 cm) among the smallest species
  03_eye_emergent    on foot at the base of the tallest tree, looking up
  04_ship_approach   the forest patch from above and outside, as a ship on approach sees it
  05_forest_floor    on foot inside the forest patch
  06_nanite_triangles the forest patch in the Nanite triangle visualisation
Open them and look: a capture that was not looked at verifies nothing.
"""

import argparse
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("layout")
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--out", required=True)
    options = parser.parse_args()
    with open(options.layout, encoding="utf-8") as handle:
        layout = json.load(handle)

    mature = [entry for entry in layout["lineup"] if entry["location_cm"][0] == 0.0]
    tallest = max(mature, key=lambda entry: entry["height_m"])
    smallest = sorted(mature, key=lambda entry: entry["height_m"])[:2]
    length = layout["lineup_length_cm"]
    top = tallest["height_m"] * 100.0
    back = max(9000.0, top * 2.0)
    shots = [("01_lineup", (-back, length * 0.5, top * 0.25), (0.0, length * 0.5, top * 0.42), 80, "")]
    near = smallest[0]["location_cm"]
    other = smallest[-1]["location_cm"]
    shots.append(("02_eye_understorey", (-1400.0, min(near[1], other[1]) - 500.0, 154.0), (0.0, (near[1] + other[1]) * 0.5, 450.0), 75, ""))
    base = tallest["location_cm"]
    shots.append(("03_eye_emergent", (base[0] - 1800.0, base[1] - 900.0, 154.0), (base[0], base[1], top * 0.6), 90, ""))
    if layout.get("forest_centre_cm"):
        cx, cy = layout["forest_centre_cm"]
        radius = layout["forest_radius_cm"]
        shots.append(("04_ship_approach", (cx - radius * 2.2, cy - radius * 0.6, 9000.0), (cx, cy, 1500.0), 70, ""))
        shots.append(("05_forest_floor", (cx - radius * 0.35, cy, 154.0), (cx, cy + 600.0, 700.0), 80, ""))
        shots.append(("06_nanite_triangles", (cx - radius * 2.2, cy - radius * 0.6, 9000.0), (cx, cy, 1500.0), 70, "VisualizeNanite"))

    failed = 0
    for name, location, target, fov, mode in shots:
        command = [sys.executable, os.path.join(HERE, "capture.py"), "--port", str(options.port), "--out", os.path.join(options.out, name + ".png"),
                   "--location"] + ["%.1f" % v for v in location] + ["--look-at"] + ["%.1f" % v for v in target] + ["--fov", str(fov), "--size", "1600", "900"]
        if mode:
            command += ["--mode", mode, "--sub-mode", "Triangles"]
        run = subprocess.run(command, capture_output=True, text=True)
        print((run.stdout or run.stderr).strip()[-300:])
        failed += 1 if run.returncode else 0
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())

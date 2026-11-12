#!/usr/bin/env python3
"""Print a biome report as Markdown tables: one row per generated mesh, measured against its targets.

    python report_table.py ../biomes/verdance.report.json [--mannequin-cm 180.5]

Use it to fill the README and the hand-off report. Nothing is rounded up: a tree that missed its
target shows the miss.
"""

import argparse
import json
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("report")
    parser.add_argument("--mannequin-cm", type=float, default=180.5)
    options = parser.parse_args()
    with open(options.report, encoding="utf-8") as handle:
        report = json.load(handle)

    print("| Species | Latin name | Layer | Archetype | Why | Defaulted from the archetype |")
    print("|---|---|---|---|---|---|")
    for species in report["species"]:
        print("| %s | *%s* | %s | %s | %s | %s |" % (species["common_name"], species["latin_name"], species.get("layer", ""), species.get("archetype", ""),
                                               species.get("archetype_reason", ""), ", ".join(species.get("defaulted") or []) or "nothing"))
    print()
    print("| Mesh | Stage | Seed | Height m (target) | Mannequins tall | Crown m (target) | Trunk base m (target dbh) | Triangles | Nanite | LODs | Passes |")
    print("|---|---|---|---|---|---|---|---|---|---|---|")
    total = 0
    for species in report["species"]:
        for variant in species["variants"]:
            total += variant["triangles"]
            trunk = variant.get("trunk_base_diameter_m")
            print("| `%s` | %.2f | %d | %.2f (%.2f) | %.1f | %.2f (%.2f) | %s (%.2f) | %d | %s | %d | %d |" % (
                variant["mesh"].rsplit("/", 1)[-1], variant["stage"], variant["seed"], variant["height_m"], variant["target_height_m"],
                variant["height_m"] * 100.0 / options.mannequin_cm, variant["crown_width_m"], variant["target_crown_width_m"],
                ("%.2f" % trunk) if trunk is not None else "n/a", variant.get("target_trunk_diameter_m", 0.0),
                variant["triangles"], "yes" if variant["nanite"] else "no", variant["lods"], len(variant["passes"])))
    count = sum(len(s["variants"]) for s in report["species"])
    print()
    print("%d meshes, %d triangles in all, %d on average." % (count, total, total // max(1, count)))
    misses = []
    for species in report["species"]:
        for variant in species["variants"]:
            height_error = abs(variant["height_m"] - variant["target_height_m"]) / variant["target_height_m"]
            crown_error = abs(variant["crown_width_m"] - variant["target_crown_width_m"]) / variant["target_crown_width_m"]
            if height_error > 0.03 or crown_error > 0.12:
                misses.append("`%s` (height %+.0f%%, crown %+.0f%%)" % (
                    variant["mesh"].rsplit("/", 1)[-1], 100.0 * (variant["height_m"] / variant["target_height_m"] - 1.0),
                    100.0 * (variant["crown_width_m"] / variant["target_crown_width_m"] - 1.0)))
    print("Outside tolerance (height 3%%, crown 12%%): %s" % (", ".join(misses) if misses else "none"))
    return 0


if __name__ == "__main__":
    sys.exit(main())

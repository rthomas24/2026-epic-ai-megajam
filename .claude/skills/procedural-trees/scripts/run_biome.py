#!/usr/bin/env python3
"""Shell-side driver: generate the trees of a biome file in a running editor and save the report.

    python run_biome.py --port 8017 ../biomes/verdance.json [--only Kapok Cacao] [--variants 0] [--report out.json]

Sends one editor call per species (so no single call runs for minutes), merges the results and
writes them to --report (default: <biome file>.report.json next to the biome file).
Start the editor first:  powershell -File Tools\\td-play-mcp.ps1 -Editor -Port 8017 -Map /Engine/Maps/Entry
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
    parser.add_argument("biome")
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--only", nargs="*", default=[])
    parser.add_argument("--variants", nargs="*", type=int, default=None)
    parser.add_argument("--report", default="")
    options = parser.parse_args()

    with open(options.biome, encoding="utf-8") as handle:
        biome = json.load(handle)
    report_path = options.report or os.path.splitext(options.biome)[0] + ".report.json"
    merged = {"biome": biome["biome"], "species": []}
    if os.path.exists(report_path):
        with open(report_path, encoding="utf-8") as handle:
            merged = json.load(handle)

    wanted = [s["id"] for s in biome["species"] if not options.only or s["id"] in options.only]
    failed = 0
    for species_id in wanted:
        arguments = {"biome": biome, "only": [species_id]}
        if options.variants is not None:
            arguments["variants"] = options.variants
        with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False, encoding="utf-8") as handle:
            json.dump(arguments, handle)
            args_file = handle.name
        try:
            run = subprocess.run([sys.executable, os.path.join(HERE, "ue_exec.py"), "--port", str(options.port), "--timeout", "1800",
                                  "--lib", os.path.join(HERE, "pve_lib.py"), os.path.join(HERE, "generate_biome.py"), "--args", "@" + args_file],
                                 capture_output=True, text=True)
        finally:
            os.unlink(args_file)
        try:
            result = json.loads(run.stdout)
            entry = result["species"][0]
        except (ValueError, KeyError, IndexError, TypeError):
            failed += 1
            print("FAILED %s:\n%s\n%s" % (species_id, run.stdout[-3000:], run.stderr[-1000:]))
            continue
        previous = next((s for s in merged["species"] if s["id"] == species_id), None)
        if previous and options.variants is not None:
            # keep the variants that were not regenerated
            kept = [v for v in previous.get("variants", []) if v["mesh"] not in {n["mesh"] for n in entry["variants"]}]
            entry["variants"] = sorted(kept + entry["variants"], key=lambda v: -v["stage"])
            for key in ("archetype", "archetype_reason", "defaulted", "notes", "preset"):
                entry.setdefault(key, previous.get(key))
        merged["species"] = [s for s in merged["species"] if s["id"] != species_id] + [entry]
        for variant in entry["variants"]:
            print("%-12s %-16s stage %.2f seed %-3d  height %6.2f m (target %6.2f)  crown %6.2f m (target %6.2f)  %6d tris  nanite=%s" % (
                species_id, entry.get("archetype", ""), variant["stage"], variant["seed"], variant["height_m"], variant["target_height_m"],
                variant["crown_width_m"], variant["target_crown_width_m"], variant["triangles"], variant["nanite"]))
    order = {s["id"]: i for i, s in enumerate(biome["species"])}
    merged["species"].sort(key=lambda s: order.get(s["id"], 99))
    with open(report_path, "w", encoding="utf-8") as handle:
        json.dump(merged, handle, indent=1)
    print("report: %s" % report_path)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())

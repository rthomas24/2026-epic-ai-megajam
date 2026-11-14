"""Check the presentation packages exported by UnrealPak -ListContainer and -CSV."""

import argparse
import csv
import sys
from pathlib import Path


REQUIRED = (
    "Presentation/DA_Credits",
    "Audio/CreditsMusic01/MUS_Credits_CathedralPulse_Full_01",
    "Audio/CreditsMusic01/MUS_Credits_EtherealPulseRemix_Full_02",
    "Audio/DominionInterior01/MS_TD_DominionInteriorMusic",
    "Audio/DominionInterior01/MUS_DominionInterior_Calm_01",
    "Audio/VerdanceTheme01/MUS_Verdance_Main_Full_01",
    "Audio/DominionAssault02/MUS_DominionAssault_PushTheAttack_Full_01",
    "Audio/IdleAmbient01/MUS_SpaceIdle_Ambient1_Full_01",
)
CONTENT_ROOT = "../../../unrealgamejam/content/triaddrift/"


def read_exports(path):
    exports = set()
    with path.open(encoding="utf-8-sig", newline="") as stream:
        rows = csv.DictReader(stream, skipinitialspace=True)
        if not {"Filename", "ChunkType"} <= set(rows.fieldnames or ()):
            raise ValueError(f"{path}: expected UnrealPak IoStore CSV columns")
        for row in rows:
            if row["ChunkType"].strip() == "ExportBundleData":
                name = row["Filename"].strip().replace("\\", "/").casefold()
                if name.startswith(CONTENT_ROOT):
                    exports.add(name[len(CONTENT_ROOT):])
    return exports


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("listing", type=Path, nargs="+", help="CSV file(s) from one build")
    exports = set()
    try:
        for path in parser.parse_args().listing:
            exports.update(read_exports(path))
    except (OSError, ValueError, csv.Error) as exc:
        print(exc, file=sys.stderr)
        return 2
    controls = {"maps/l_galaxy.umap", "presentation/da_ui_style.uasset"}
    missing_controls = controls - exports
    if missing_controls:
        print("Incomplete project listing; missing controls: " + ", ".join(sorted(missing_controls)),
              file=sys.stderr)
        return 2
    missing = []
    for package in REQUIRED:
        present = package.casefold() + ".uasset" in exports
        if not present:
            missing.append(package)
        print(f"{'FOUND' if present else 'MISSING'}  /Game/TriadDrift/{package}")
    print(f"{len(REQUIRED) - len(missing)}/{len(REQUIRED)} presentation packages in supplied containers")
    return int(bool(missing))


if __name__ == "__main__":
    sys.exit(main())

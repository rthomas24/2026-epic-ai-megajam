"""Create DA_Credits in a matching editor or verify an existing copy without modifying it."""
import json
from pathlib import Path

import unreal


FOLDER = "/Game/TriadDrift/Presentation"
NAME = "DA_Credits"
PACKAGE = f"{FOLDER}/{NAME}"


def read_sections(asset):
    return [
        {
            "heading": str(section.get_editor_property("heading")),
            "lines": [str(line) for line in section.get_editor_property("lines")],
        }
        for section in asset.get_editor_property("sections")
    ]


def run():
    expected = json.loads(Path(__file__).with_name("sections.json").read_text(encoding="utf-8"))
    library = unreal.EditorAssetLibrary
    if library.does_asset_exist(PACKAGE):
        asset = library.load_asset(PACKAGE)
        if not isinstance(asset, unreal.TDCreditsData) or read_sections(asset) != expected:
            raise RuntimeError(f"Existing {PACKAGE} differs from this credits revision")
        unreal.log(f"Verified {PACKAGE}: {len(expected)} sections")
        return

    sections = []
    for row in expected:
        section = unreal.TDCreditsSection()
        section.set_editor_property("heading", unreal.Text(row["heading"]))
        section.set_editor_property("lines", [unreal.Text(line) for line in row["lines"]])
        sections.append(section)

    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.TDCreditsData)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        NAME, FOLDER, unreal.TDCreditsData, factory
    )
    if not asset:
        raise RuntimeError(f"Could not create {PACKAGE}")
    asset.set_editor_property("sections", sections)
    if read_sections(asset) != expected:
        raise RuntimeError("Credits changed during conversion to Unreal text")
    if not library.save_loaded_asset(asset):
        raise RuntimeError(f"Could not save {PACKAGE}")
    unreal.log(f"Saved {PACKAGE}; run again in a fresh editor to verify")


if __name__ == "__main__":
    run()

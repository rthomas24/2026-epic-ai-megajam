# Triad Drift - build DT_BarksFallback from Tools/td_barks.csv (dialogs.md). Owner: Teammate 3.
# Run headless after Tools/td_barks_from_dialogs.py:
#   powershell -File Tools\td-python.ps1 -Script Tools\td-barks-import.py
#
# The table lives in /Game/TriadDrift/Gameplay/Characters/Barks (always cooked). Row struct FTDBarkRow; row names
# "<BarkId>__<Speaker>". Idempotent: an existing table is refilled in place. Nicholas's DT_Barks (Presentation) wins
# row by row at runtime; this table is never his.

import os
import unreal

DIR = "/Game/TriadDrift/Gameplay/Characters/Barks"
NAME = "DT_BarksFallback"
CSV = os.path.join(unreal.Paths.project_dir(), "Tools", "td_barks.csv")


def log(message):
    unreal.log("TDPY: " + message)


def main():
    assets = unreal.EditorAssetLibrary
    path = "%s/%s" % (DIR, NAME)
    if not os.path.exists(CSV):
        log("missing %s: run Tools/td_barks_from_dialogs.py first" % CSV)
        return
    if not assets.does_directory_exist(DIR):
        assets.make_directory(DIR)
    row_struct = unreal.load_object(None, "/Script/unrealGameJam.TDBarkRow")
    if row_struct is None:
        log("FTDBarkRow not found (build the C++ first)")
        return
    table = assets.load_asset(path) if assets.does_asset_exist(path) else None
    if table is None:
        factory = unreal.DataTableFactory()
        factory.set_editor_property("struct", row_struct)
        table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, DIR, unreal.DataTable, factory)
    if table is None:
        log("could not create %s" % path)
        return
    ok = unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(table, CSV, row_struct)
    if not ok:
        log("CSV import failed")
        return
    names = unreal.DataTableFunctionLibrary.get_data_table_row_names(table)
    assets.save_loaded_asset(table)
    log("%s: %d rows" % (path, len(names)))
    log("OK")


main()

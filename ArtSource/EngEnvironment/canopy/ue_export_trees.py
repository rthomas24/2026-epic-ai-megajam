# SPDX-License-Identifier: MIT
# Export the PVE trees (Gameplay/Props/Trees, built with the procedural-trees skill) to FBX for the far-tier clumps.
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngEnvironment\canopy\ue_export_trees.py
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "source")
os.makedirs(OUT, exist_ok=True)
NAMES = ["SM_TDVeg_Kapok_S100_11", "SM_TDVeg_BrazilNut_S100_21", "SM_TDVeg_BrazilNut_S070_22", "SM_TDVeg_TrumpetTree_S100_31",
         "SM_TDVeg_Cacao_S100_61", "SM_TDVeg_Kapok_S070_12"]
eal = unreal.EditorAssetLibrary
bad = []
for n in NAMES:
    a = eal.load_asset("/Game/TriadDrift/Gameplay/Props/Trees/" + n)
    if a is None:
        bad.append(n)
        continue
    mats = [m.get_editor_property("material_slot_name") for m in a.get_editor_property("static_materials")]
    t = unreal.AssetExportTask()
    t.set_editor_property("object", a)
    t.set_editor_property("filename", os.path.join(OUT, n + ".fbx"))
    t.set_editor_property("automated", True)
    t.set_editor_property("prompt", False)
    t.set_editor_property("replace_identical", True)
    t.set_editor_property("exporter", unreal.StaticMeshExporterFBX())
    ok = unreal.Exporter.run_asset_export_task(t)
    unreal.log("TDPY: %s exported %s slots %s" % (n, ok, mats))
    if not ok:
        bad.append(n)
unreal.log("TDPY: " + ("FAILED %s" % bad if bad else "OK"))

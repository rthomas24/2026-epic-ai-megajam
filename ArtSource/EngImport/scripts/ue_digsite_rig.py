# SPDX-License-Identifier: MIT
# Triad Drift - import builder: rebuild ONLY BP_DigSite_Cabin from its kit manifest (which now carries Grok Bot's drill
# rig as dressing beside the cabin), with the structures builder's own ue_make_sites.build(). The other site classes
# are not touched. Headless (after ue_import_engai.py has imported SM_CinderDrillRig):
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngImport\scripts\ue_digsite_rig.py
import json
import os
import runpy

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
KIT = os.path.join(REPO, "ArtSource", "EngProps", "Kit")


def log(msg):
    unreal.log("TDPY: " + msg)


def main():
    sites = runpy.run_path(os.path.join(KIT, "scripts", "ue_make_sites.py"), run_name="td_digsite_rig")
    with open(os.path.join(KIT, "sites", "DigSite_Cabin.manifest.json"), encoding="utf-8") as fh:
        man = json.load(fh)
    ok = sites["build"](man)
    # ue_make_sites saves without checking: a package held by a running game (error 32) must fail this run
    saved = unreal.EditorAssetLibrary.save_asset(man["actor"], only_if_is_dirty=False)
    bp = unreal.EditorAssetLibrary.load_asset(man["actor"])
    rig = None
    sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    lib = unreal.SubobjectDataBlueprintFunctionLibrary
    for h in sds.k2_gather_subobject_data_for_blueprint(bp)[1:]:
        o = lib.get_object_for_blueprint(lib.get_data(h), bp)
        if isinstance(o, unreal.StaticMeshComponent) and str(o.get_name()).startswith("DrillRig"):
            rig = o
    if rig is not None:
        loc = rig.get_editor_property("relative_location")
        log("DrillRig component: mesh %s at (%.0f, %.0f, %.0f) yaw %.0f" % (
            rig.get_editor_property("static_mesh").get_path_name() if rig.get_editor_property("static_mesh") else None,
            loc.x, loc.y, loc.z, rig.get_editor_property("relative_rotation").yaw))
    log("OK" if ok and saved and rig is not None else "FAILED: build %s, saved %s, rig %s" % (ok, saved, rig is not None))


main()

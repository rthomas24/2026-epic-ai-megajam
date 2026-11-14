# SPDX-License-Identifier: MIT
# Triad Drift - BP_Outpost_Site_Verdance / _Cinder: the assembled placement (cabin + console) as an actor class, built
# from export/Outpost_Site.manifest.json. Used for the in-game walk test (TD.Dev.SpawnClass) and as a ready-made site.
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Outpost\scripts\ue_make_outpost_site.py
# Writes only /Game/TriadDrift/Gameplay/Props/Outpost/BP_Outpost_Site_*.
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
DEST = "/Game/TriadDrift/Gameplay/Props/Outpost"
eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
lib = unreal.SubobjectDataBlueprintFunctionLibrary


def log(msg):
    unreal.log("TDPY: " + msg)


def make(name, manifest, variant):
    path = "%s/%s" % (DEST, name)
    if eas.does_asset_exist(path):
        eas.delete_asset(path)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.Actor)
    bp = tools.create_asset(name, DEST, unreal.Blueprint, factory)
    handles = sds.k2_gather_subobject_data_for_blueprint(bp)
    parent = handles[0]
    made = []
    for i, piece in enumerate(manifest["pieces"]):
        params = unreal.AddNewSubobjectParams(parent_handle=parent, new_class=unreal.StaticMeshComponent, blueprint_context=bp)
        handle, fail = sds.add_new_subobject(params)
        if not lib.is_handle_valid(handle):
            log("FAILED: add component %s (%s)" % (piece["name"], fail))
            return False
        sds.rename_subobject(handle, unreal.Text(piece["name"]))
        comp = lib.get_object_for_blueprint(lib.get_data(handle), bp)
        comp.set_static_mesh(eas.load_asset(piece["mesh"]))
        comp.set_material(0, eas.load_asset(piece["materials"][variant]))
        loc, rot = piece["location_cm"], piece["rotation_deg"]
        comp.set_editor_property("relative_location", unreal.Vector(loc[0], loc[1], loc[2]))
        comp.set_editor_property("relative_rotation", unreal.Rotator(roll=rot[0], pitch=rot[1], yaw=rot[2]))
        comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
        if i == 0:
            parent = handle        # the cabin becomes the parent of the rest
        made.append(piece["name"])
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eas.save_asset(path, only_if_is_dirty=False)
    log("%s: %s" % (name, made))
    return True


def main():
    with open(os.path.join(KIT, "export", "Outpost_Site.manifest.json"), encoding="utf-8") as fh:
        manifest = json.load(fh)
    ok = make("BP_Outpost_Site_Verdance", manifest, "Verdance") and make("BP_Outpost_Site_Cinder", manifest, "Cinder")
    log("OK" if ok else "FAILED")


main()

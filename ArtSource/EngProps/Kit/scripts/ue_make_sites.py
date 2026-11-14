# SPDX-License-Identifier: MIT
# Triad Drift - rebuild the walkable site actor classes from the kit, from sites/*.manifest.json (kit_sites.py).
# Keeps the class names T5 places (BP_Relay, BP_Relay_Awake, BP_Wreck, BP_Outpost_Site_Verdance / _Cinder): an
# existing class is emptied of its mesh components and refilled, never deleted (deleting fails while it is referenced,
# and keeping it keeps every reference valid). Headless (or through ue_kit_all.py):
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Kit\scripts\ue_make_sites.py
import glob
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
sds = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
lib = unreal.SubobjectDataBlueprintFunctionLibrary


def log(msg):
    unreal.log("TDPY: " + msg)


def open_class(path):
    folder, name = path.rsplit("/", 1)
    if eas.does_asset_exist(path):
        bp = eas.load_asset(path)
        handles = sds.k2_gather_subobject_data_for_blueprint(bp)
        doomed = []
        for h in handles[1:]:
            obj = lib.get_object_for_blueprint(lib.get_data(h), bp)
            if isinstance(obj, unreal.StaticMeshComponent):
                doomed.append(h)
        for h in reversed(doomed):
            try:
                sds.delete_subobject(handles[0], h, bp)
            except Exception as error:
                log("note: delete_subobject %s" % error)
        return bp, False
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", unreal.Actor)
    return tools.create_asset(name, folder, unreal.Blueprint, factory), True


def slot_index(mesh, key):
    for i, sm in enumerate(mesh.static_materials):
        if str(sm.material_slot_name).endswith(key):
            return i
    return -1


def build(man):
    path = man["actor"]
    bp, new = open_class(path)
    if bp is None:
        log("FAILED: no class %s" % path)
        return False
    handles = sds.k2_gather_subobject_data_for_blueprint(bp)
    root = handles[0]
    made, bad = 0, []
    for pc in man["pieces"]:
        mesh = eas.load_asset(pc["mesh"])
        if mesh is None:
            bad.append("missing mesh " + pc["mesh"])
            continue
        params = unreal.AddNewSubobjectParams(parent_handle=root, new_class=unreal.StaticMeshComponent, blueprint_context=bp)
        handle, fail = sds.add_new_subobject(params)
        if not lib.is_handle_valid(handle):
            bad.append("add %s (%s)" % (pc["name"], fail))
            continue
        sds.rename_subobject(handle, unreal.Text(pc["name"]))
        comp = lib.get_object_for_blueprint(lib.get_data(handle), bp)
        comp.set_static_mesh(mesh)
        for idx, mi in pc.get("materials", {}).items():
            comp.set_material(int(idx), eas.load_asset(mi))
        for key, mi in pc.get("slots", {}).items():
            i = slot_index(mesh, key)
            if i >= 0:
                comp.set_material(i, eas.load_asset(mi))
        x, y, z = pc["location_cm"]
        comp.set_editor_property("relative_location", unreal.Vector(x, y, z))
        comp.set_editor_property("relative_rotation", unreal.Rotator(roll=0.0, pitch=0.0, yaw=pc.get("yaw_deg", 0.0)))
        comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
        made += 1
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    eas.save_asset(path, only_if_is_dirty=False)
    names = []
    for h in sds.k2_gather_subobject_data_for_blueprint(bp)[1:]:
        o = lib.get_object_for_blueprint(lib.get_data(h), bp)
        if isinstance(o, unreal.StaticMeshComponent):
            names.append(str(o.get_name()).split("_GEN")[0])
    names = sorted(set(names))          # the subobject list reports each component twice (template + variable)
    stale = [n for n in names if n not in [pc["name"] for pc in man["pieces"]]]
    if len(names) != made or stale:
        bad.append("component list %d (%s), expected %d; not from the manifest: %s" % (len(names), names[:4], made, stale))
    log("%s: %d components%s%s" % (path, made, " (new class)" if new else "", (" PROBLEMS %s" % bad) if bad else ""))
    return not bad


def main():
    ok = True
    for p in sorted(glob.glob(os.path.join(KIT, "sites", "*.manifest.json"))):
        with open(p, encoding="utf-8") as fh:
            ok = build(json.load(fh)) and ok
    log("OK" if ok else "FAILED: see the lines above")


if __name__ == "__main__":
    main()

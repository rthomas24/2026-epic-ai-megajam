"""Triad Drift - wire the fallback sounds into the engineering fallback data assets, INSIDE the editor.

Sent to a running editor by the ue-game-audio skill's runner, which defines SPEC (build/spec_wire.json):

    python .claude/skills/ue-game-audio/scripts/ue_run_python.py --port 8016 ArtSource/EngAudio/scripts/ue_wire.py --spec ArtSource/EngAudio/build/spec_wire.json

Rules it keeps: it only ever touches data assets under /Game/TriadDrift/Gameplay/ (never Presentation), it creates
an asset only when no asset of that name exists anywhere, and on every asset it sets nothing but Sound fields
(plus the one "keep" value that preserves the look gameplay had before the asset existed).
"""
RESULT = {"created": [], "set": [], "skipped": [], "errors": []}
registry = unreal.AssetRegistryHelpers.get_asset_registry()


def find_asset(name):
    """Presentation first (it wins at runtime: never touched), then anywhere under Gameplay."""
    found = [d for d in registry.get_assets_by_path("/Game/TriadDrift", recursive=True) if str(d.asset_name) == name]
    for data in found:
        if str(data.package_path).startswith("/Game/TriadDrift/Presentation"):
            return "presentation", data
    for data in found:
        if str(data.package_path).startswith("/Game/TriadDrift/Gameplay"):
            return "gameplay", data
    return ("other", found[0]) if found else (None, None)


def object_path(path):
    return "%s.%s" % (path, path.rsplit("/", 1)[1])


registry.scan_paths_synchronous(["/Game/TriadDrift"], True)
for class_name, names in SPEC["create"].items():
    for name in names:
        where, data = find_asset(name)
        if where:
            continue
        try:
            factory = unreal.DataAssetFactory()
            factory.set_editor_property("data_asset_class", getattr(unreal, class_name))
            asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, SPEC["folder"], getattr(unreal, class_name), factory)
            if not asset:
                raise RuntimeError("create_asset returned nothing")
            unreal.EditorAssetLibrary.save_loaded_asset(asset)
            RESULT["created"].append("%s/%s (%s)" % (SPEC["folder"], name, class_name))
        except Exception as error:
            RESULT["errors"].append("create %s: %s" % (name, str(error)[:300]))

touched = {}
for item in SPEC["slots"] + SPEC.get("keep", []):
    where, data = find_asset(item["asset"])
    if where != "gameplay":
        RESULT["skipped"].append("%s.%s (%s)" % (item["asset"], item["field"], "Presentation has this asset: it wins, left alone" if where == "presentation" else "no such asset"))
        continue
    try:
        asset = touched.get(item["asset"]) or data.get_asset()
        touched[item["asset"]] = asset
        if "color" in item:
            asset.set_editor_property(item["field"], unreal.LinearColor(*item["color"]))
            RESULT["set"].append("%s.%s = colour (kept look)" % (item["asset"], item["field"]))
            continue
        sound = unreal.load_asset(object_path(item["sound"]))
        if not sound:
            raise RuntimeError("sound missing: %s" % item["sound"])
        if item["soft"]:
            asset.set_editor_property(item["field"], sound)
        else:
            cue = asset.get_editor_property(item["field"])
            cue.set_editor_property("Sound", sound)
            asset.set_editor_property(item["field"], cue)
        RESULT["set"].append("%s.%s = %s" % (item["asset"], item["field"], item["sound"].rsplit("/", 1)[1]))
    except Exception as error:
        RESULT["errors"].append("%s.%s: %s" % (item["asset"], item["field"], str(error)[:300]))

for name, asset in touched.items():
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
        RESULT["errors"].append("save failed: %s" % name)
RESULT["assets"] = sorted("%s" % a.get_path_name().split(".")[0] for a in touched.values())
RESULT

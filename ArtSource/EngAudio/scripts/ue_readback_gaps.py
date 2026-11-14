"""Triad Drift - read back the "gaps" sounds (recipes/p5_gaps.json) from the editor: for every asset its sound class,
attenuation, concurrency, looping and volume, and for every data slot which sound it holds. Sent by the
ue-game-audio skill's ue_run_python.py with SPEC = {"sounds": {name: {path, att, con, cls, looping}}, "slots": [...]}
(make_spec.py --only ... --tag gaps writes the slots; this script's caller adds "sounds" from build/manifest.json).
"errors" lists anything that is not as the recipe says: a 3D sound without attenuation or concurrency is an error."""
OUT = {"sounds": {}, "slots": {}, "errors": []}
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.scan_paths_synchronous(["/Game/TriadDrift"], True)


def obj(path):
    return unreal.load_asset("%s.%s" % (path, path.rsplit("/", 1)[1]))


def name_of(o):
    return o.get_name() if o else None


for name, want in SPEC["sounds"].items():
    sound = obj(want["path"])
    if not sound:
        OUT["errors"].append("missing: " + want["path"])
        continue
    con = [name_of(c) for c in sound.get_editor_property("concurrency_set")]
    info = {"asset": sound.get_name(), "type": type(sound).__name__, "class": name_of(sound.get_editor_property("sound_class_object")),
            "attenuation": name_of(sound.get_editor_property("attenuation_settings")), "concurrency": con,
            "looping": bool(sound.get_editor_property("looping")) if isinstance(sound, unreal.SoundWave) else None,
            "volume": round(sound.get_editor_property("volume"), 3) if isinstance(sound, unreal.SoundWave) else None}
    OUT["sounds"][name] = info
    if info["class"] != "SC_TDE_" + want["cls"]:
        OUT["errors"].append("%s: class %s" % (name, info["class"]))
    if want["att"] and (info["attenuation"] != "ATT_TDE_" + want["att"] or con != ["SCON_TDE_" + want["con"]]):
        OUT["errors"].append("%s: 3D sound with attenuation %s, concurrency %s" % (name, info["attenuation"], con))
    if not want["att"] and info["attenuation"]:
        OUT["errors"].append("%s: 2D sound carries attenuation %s" % (name, info["attenuation"]))
    if want["looping"] and info["looping"] is not True:
        OUT["errors"].append("%s: loop is not set to loop" % name)
for item in SPEC["slots"]:
    key = "%s.%s" % (item["asset"], item["field"])
    found = [d for d in registry.get_assets_by_path("/Game/TriadDrift/Gameplay", recursive=True) if str(d.asset_name) == item["asset"]]
    if not found:
        OUT["slots"][key] = "NO SUCH ASSET (slot not set)"
        continue
    value = found[0].get_asset()
    for part in item["field"].split("."):
        value = value.get_editor_property(part)
    sound = value.get_editor_property("Sound")
    OUT["slots"][key] = str(sound.get_path_name()).split(".")[0] if sound else None
    if OUT["slots"][key] != item["sound"]:
        OUT["errors"].append("%s holds %s, wanted %s" % (key, OUT["slots"][key], item["sound"]))
OUT

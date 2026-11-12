# Triad Drift - create the five game maps (C-01). Lead-owned.
# Run headless:  powershell -File Tools\td-python.ps1 -Script Tools\td-make-maps.py
#
# Each map is created EMPTY as a World Partition level (so One File Per Actor is on and both
# humans can place actors without touching the .umap), with streaming switched off: these are
# small jam levels and nothing placed far from the origin may ever unload.
# Gameplay actors are spawned from C++ by the game modes; nothing is placed here.
#
# Idempotent: maps that already exist and already have streaming off are left alone. Claim the maps in CLAIMS.md before running.

import unreal

MAP_DIR = "/Game/TriadDrift/Maps"
MAPS = ["L_MainMenu", "L_Galaxy", "L_Planet_Verdance", "L_Planet_Cinder", "L_Transition"]


def log(message):
    unreal.log("TDPY: " + message)


def disable_streaming(world):
    """Uses UTDEditorTools (Core) because the engine does not expose this switch to Python."""
    if not unreal.TDEditorTools.set_world_partition_streaming(world, False):
        return None
    if unreal.TDEditorTools.is_world_partition_streaming_enabled(world):
        return None
    return "streaming off"


def main():
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    editor_assets = unreal.EditorAssetLibrary
    if not editor_assets.does_directory_exist(MAP_DIR):
        editor_assets.make_directory(MAP_DIR)

    created, kept, failed = [], [], []
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    for name in MAPS:
        path = "%s/%s" % (MAP_DIR, name)
        existed = editor_assets.does_asset_exist(path)
        opened = level_editor.load_level(path) if existed else level_editor.new_level(path, True)
        if not opened:
            failed.append(name)
            log("%s: could not %s" % (name, "open" if existed else "create"))
            continue

        world = editor.get_editor_world()
        was_streaming = unreal.TDEditorTools.is_world_partition_streaming_enabled(world)
        if existed and not was_streaming:
            kept.append(name)
            log("%s already exists with streaming off, left alone" % name)
            continue

        note = disable_streaming(world)
        saved = note is not None and level_editor.save_current_level()
        if saved and editor_assets.does_asset_exist(path):
            (kept if existed else created).append(name)
            log("%s %s (%s)" % (name, "fixed" if existed else "created", note))
        else:
            failed.append(name)
            log("%s: %s" % (name, "streaming switch failed" if note is None else "save failed"))

    log("created=%d kept=%d failed=%d" % (len(created), len(kept), len(failed)))
    if not failed:
        log("OK")


main()

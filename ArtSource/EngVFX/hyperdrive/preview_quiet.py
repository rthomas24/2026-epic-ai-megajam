# Hyperdrive look (T7): PIE preview helper. Stops the wave director (no new waves lock the hyperdrive) and reports.
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
n = 0
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.TDWaveDirector):
    a.stop_and_clear()
    n += 1
'wave directors stopped: %d' % n

# 10s: put the flown ship 150 m from the nearest live Dominion hull turret, facing it (turrets fire inside 250 m).
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
ts = [t for t in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.TDEnemyTurret) if not t.get_editor_property('hidden')]
if not ts:
    r = 'no turret'
else:
    p = pawn.get_actor_location()
    t = sorted(ts, key=lambda a: (a.get_actor_location() - p).length())[0]
    tl = t.get_actor_location()
    up = t.get_actor_up_vector()
    at = tl + up * 15000.0
    rot = unreal.MathLibrary.find_look_at_rotation(at, tl)
    pawn.set_actor_location_and_rotation(at, rot, False, True)
    r = 'turret %s at %s; ship placed 150 m along its up axis' % (t.get_name(), tl)
r

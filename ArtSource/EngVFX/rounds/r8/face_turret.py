# 12s: put the flown ship DIST m from the nearest Dominion turret, on the line from the turret to the ship, facing it.
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
pc = unreal.GameplayStatics.get_player_controller(world, 0)
best = None
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    if 'TDEnemyTurret' in a.get_class().get_name():
        d = (a.get_actor_location() - pawn.get_actor_location()).length()
        if best is None or d < best[0]:
            best = (d, a)
if not best:
    out = 'no turret'
else:
    t = best[1]
    tl = t.get_actor_location()
    v = pawn.get_actor_location() - tl
    v = v.normal()
    at = tl + v * (DIST * 100.0)
    rot = unreal.MathLibrary.find_look_at_rotation(at, tl)
    pawn.set_actor_location_and_rotation(at, rot, False, True)
    if pc:
        pc.set_control_rotation(rot)
    out = 'turret %s was %.0f m; ship now %d m from it' % (t.get_name(), best[0] / 100.0, DIST)
out

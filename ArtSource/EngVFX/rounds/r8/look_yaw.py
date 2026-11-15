# 12u: turn the flown ship's view by YAW degrees (control rotation and the pawn), for look-back captures in flight.
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
pc = unreal.GameplayStatics.get_player_controller(world, 0)
r = pawn.get_actor_rotation()
nr = unreal.Rotator(0.0, 0.0, r.yaw + YAW)
pawn.set_actor_rotation(nr, False)
if pc:
    pc.set_control_rotation(nr)
d = None
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    if a.get_class().get_name() == 'TDDominion':
        d = (a.get_actor_location() - pawn.get_actor_location()).length() / 100.0
'yaw %.0f -> %.0f; Dominion centre %s m' % (r.yaw, nr.yaw, ('%.0f' % d) if d else '?')

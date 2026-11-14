# Hyperdrive look (T7): spawn the drop-out one-shot 25 m ahead of the PIE ship, facing the ship's way, as the cue does.
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
xf = pawn.get_actor_transform()
loc = unreal.MathLibrary.transform_location(xf, unreal.Vector(2500, 0, 0))
c = unreal.NiagaraFunctionLibrary.spawn_system_at_location(world, unreal.load_asset('/Game/TriadDrift/Gameplay/VFX/Hyperdrive/NS_TD_Hyperdrive_DropOut'),
                                                          loc, pawn.get_actor_rotation(), unreal.Vector(1, 1, 1), True, True, unreal.NCPoolMethod.NONE, True)
'dropout spawned, active %s' % bool(c and c.is_active())

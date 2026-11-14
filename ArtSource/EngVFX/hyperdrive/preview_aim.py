# Hyperdrive look (T7): PIE preview helper. Turns the player's ship to face the objective marker (so a jump runs its full
# length instead of hitting the combat-zone edge), through the real mouse with TD.Input.TurnToward.
import math
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
marks = unreal.GameplayStatics.get_all_actors_with_tag(world, 'TD.Objective.Marker')
out = []
if marks and pawn:
    d = marks[0].get_actor_location() - pawn.get_actor_location()
    yaw = math.degrees(math.atan2(d.y, d.x))
    pitch = math.degrees(math.atan2(d.z, math.sqrt(d.x * d.x + d.y * d.y)))
    unreal.SystemLibrary.execute_console_command(world, 'TD.Input.TurnToward %.1f %.1f 3' % (yaw, pitch))
    out.append('marker %s at %.1f km, turning to yaw %.1f pitch %.1f' % (marks[0].get_name(), d.length() / 100000.0, yaw, pitch))
else:
    out.append('no marker (%d) or pawn' % len(marks))
"\n".join(out)

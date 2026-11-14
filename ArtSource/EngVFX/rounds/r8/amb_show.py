# Round 12: place ambient systems on the ground round the pilot (on foot) for a capture. CMD = [(name, fwd m, right m, up m), ...]
# or 'clear'. A name starting with '/' is a full path; an optional 5th value is a yaw relative to the pilot's facing (degrees). Ground = pilot location minus the capsule half-height.
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
out = []
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    for k in a.get_components_by_tag(unreal.NiagaraComponent, 'EngVFXAmb'):
        k.destroy_component(k)
if CMD != 'clear':
    p = pawn.get_actor_location() - unreal.Vector(0, 0, 90)
    f = pawn.get_actor_forward_vector(); f.z = 0; f = f.normal()
    r = unreal.Vector(-f.y, f.x, 0)
    yaw0 = pawn.get_actor_rotation().yaw
    for row in CMD:
        n, fw, rt, up = row[:4]; yaw = yaw0 + (row[4] if len(row) > 4 else 0.0) if len(row) > 4 else 0.0
        at = p + f * (fw * 100.0) + r * (rt * 100.0) + unreal.Vector(0, 0, up * 100.0)
        c = unreal.NiagaraFunctionLibrary.spawn_system_at_location(world, unreal.load_asset(n if n.startswith('/') else '/Game/TriadDrift/Gameplay/VFX/Systems/Ambient/' + n), at,
                unreal.Rotator(0, 0, yaw), unreal.Vector(1, 1, 1), True, True, unreal.NCPoolMethod.NONE, True)
        if c:
            c.component_tags = ['EngVFXAmb']
        out.append('%s %s' % (n, 'ok' if c else 'FAILED'))
"\n".join(out) or 'cleared'

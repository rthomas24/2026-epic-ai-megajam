# 12g: spawn a tractor beam the way ATDDockingBay does (root at the mouth, +X at the ship, User.Length cm, User.Intensity)
# relative to the flown ship. CMD = (system path, mouth fwd m, mouth right m, yaw of +X relative to the ship's facing, length m, intensity) or 'clear'.
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    for k in a.get_components_by_tag(unreal.NiagaraComponent, 'EngVFXTB'):
        k.destroy_component(k)
out = 'cleared'
if CMD != 'clear':
    path, fw, rt, yaw, length, inten = CMD[:6]
    pre = CMD[6] if len(CMD) > 6 else 'User.'   # 12g probe: the variable name form
    rot = pawn.get_actor_rotation()
    f = pawn.get_actor_forward_vector(); r = pawn.get_actor_right_vector()
    at = pawn.get_actor_location() + f * (fw * 100.0) + r * (rt * 100.0)
    c = unreal.NiagaraFunctionLibrary.spawn_system_at_location(world, unreal.load_asset(path), at, unreal.Rotator(0.0, rot.pitch, rot.yaw + yaw),
            unreal.Vector(1, 1, 1), True, True, unreal.NCPoolMethod.NONE, True)
    if c:
        c.component_tags = ['EngVFXTB']
        if length is not None:
            c.set_variable_float(pre + 'Length', length * 100.0)
        c.set_variable_float(pre + 'Intensity', inten)
    out = '%s at %s yaw %s: %s' % (path.split('/')[-1], (fw, rt), yaw, 'ok' if c else 'FAILED')
out

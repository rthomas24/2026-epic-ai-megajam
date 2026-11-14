# Hyperdrive look (T7): attach one system to the PIE player ship (as the hyperdrive's attached cue does) and report it.
# NAME prepended by the caller; 'clear' destroys what this helper attached.
NAME = globals().get('NAME', 'NS_TD_Hyperdrive_Tunnel')
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
out = []
if NAME == 'clear':
    n = 0
    for c in pawn.get_components_by_tag(unreal.NiagaraComponent, 'T7Preview'):
        c.destroy_component(c)
        n += 1
    out.append('cleared %d' % n)
else:
    s = unreal.load_asset('/Game/TriadDrift/Gameplay/VFX/Hyperdrive/' + NAME)
    c = unreal.NiagaraFunctionLibrary.spawn_system_attached(s, pawn.root_component, 'None', unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0),
                                                           unreal.AttachLocation.KEEP_RELATIVE_OFFSET, True, True, unreal.NCPoolMethod.NONE, True)
    if c:
        c.component_tags = list(c.component_tags) + ['T7Preview']
    out.append('%s attached, active %s' % (NAME, bool(c and c.is_active())))
"\n".join(out)

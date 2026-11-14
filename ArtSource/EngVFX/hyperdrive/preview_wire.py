# Hyperdrive look (T7): PIE preview only. Assigns the three systems to DA_Travel_FX IN MEMORY (never saved: the effects
# builder owns that asset and assigns the slots for real) and checks that each system activates in the PIE world.
# Run with PIE running:  python ArtSource/EngVFX/scripts/run_editor_py.py <port> ArtSource/EngVFX/hyperdrive/preview_wire.py
HD = '/Game/TriadDrift/Gameplay/VFX/Hyperdrive/'
out = []
da = unreal.load_asset('/Game/TriadDrift/Gameplay/Characters/FX/DA_Travel_FX')
for field, name in (('hyperdrive_spool', 'NS_TD_Hyperdrive_Spool'), ('hyperdrive_tunnel', 'NS_TD_Hyperdrive_Tunnel'),
                    ('hyperdrive_drop_out', 'NS_TD_Hyperdrive_DropOut')):
    s = unreal.load_asset(HD + name)
    cue = da.get_editor_property(field)
    before = str(cue.get_editor_property('vfx'))
    cue.set_editor_property('vfx', s)
    da.set_editor_property(field, cue)
    out.append('%s: %s -> %s (sound kept: %s)' % (field, before.split('/')[-1][:60], name, str(cue.get_editor_property('sound')).split('/')[-1][:50]))
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0) if world else None
if pawn:
    for name in ('NS_TD_Hyperdrive_Spool', 'NS_TD_Hyperdrive_Tunnel', 'NS_TD_Hyperdrive_DropOut'):
        c = unreal.NiagaraFunctionLibrary.spawn_system_at_location(world, unreal.load_asset(HD + name), pawn.get_actor_location() + unreal.Vector(0, 0, 80000),
                                                                   unreal.Rotator(0, 0, 0), unreal.Vector(1, 1, 1), True, True, unreal.NCPoolMethod.NONE, True)
        out.append('%s active: %s' % (name, bool(c and c.is_active())))
        if c:
            c.destroy_component(c)
else:
    out.append('no PIE pawn')
"\n".join(out)

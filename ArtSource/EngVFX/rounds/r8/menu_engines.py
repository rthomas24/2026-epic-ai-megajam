# 10h (Nicholas's playtest #90 no. 4): preview engine glow + short trail on the menu fly-by ships. Attaches the player engine
# system of each squad ship (NS_TD_Engine_<Ship>) at every Engine / Engine_N socket of the backdrop's squad meshes, Throttle
# 0.8, Boost 0.25 (a short trail), tagged EngVFXPreview. CMD = 'on' | 'off'.
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
SYS = '/Game/TriadDrift/Gameplay/VFX/Systems/'
out = []
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    for k in a.get_components_by_tag(unreal.NiagaraComponent, 'EngVFXPreview'):
        k.destroy_component(k)
if CMD == 'on':
    for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        for m in a.get_components_by_class(unreal.StaticMeshComponent):
            sm = m.static_mesh
            if not sm or sm.get_name() not in ('SM_Rift', 'SM_Lancer', 'SM_Bastion'):
                continue
            ship = sm.get_name()[3:]
            socks = [s for s in ['Engine'] + ['Engine_%d' % i for i in range(9)] if m.does_socket_exist(s)] or ['None']
            for s in socks:
                c = unreal.NiagaraFunctionLibrary.spawn_system_attached(unreal.load_asset(SYS + 'NS_TD_Engine_' + ship), m, s, unreal.Vector(0, 0, 0),
                        unreal.Rotator(0, 0, 0), unreal.AttachLocation.SNAP_TO_TARGET, True, True, unreal.NCPoolMethod.NONE, True)
                c.component_tags = ['EngVFXPreview']
                c.set_variable_float('Throttle', 0.8); c.set_variable_float('Boost', 0.25)
            out.append('%s on %s: %s' % (ship, a.get_name(), socks))
"\n".join(out) or 'no squad meshes found'

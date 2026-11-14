# Round 10d: attach (or remove) NS_TD_Magma_Heat on the TDMagmaVolume nearest course-local (5600, 4800) - T5's outpost-route
# field the environment builder's s13 shots look at. CMD = 'on' | 'off' (prepended by the caller).
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
out = []
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    for tg in ('EngVFXPreview', 'TD.MagmaGlow'):   # 10e: main's UTDMagmaLookSubsystem attaches it too (its light stays)
        for k in a.get_components_by_tag(unreal.NiagaraComponent, tg):
            k.destroy_component(k)
if CMD == 'on':
    vs = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.TDMagmaVolume)
    t = unreal.Vector(5600, 4800, 0)
    v = sorted(vs, key=lambda a: (a.get_actor_location() - t).length())[0]
    o, e = v.get_actor_bounds(False)
    c = unreal.NiagaraFunctionLibrary.spawn_system_attached(unreal.load_asset('/Game/TriadDrift/Gameplay/VFX/Systems/NS_TD_Magma_Heat'), v.root_component, 'None',
        unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0), unreal.AttachLocation.KEEP_RELATIVE_OFFSET, True, True, unreal.NCPoolMethod.NONE, True)
    c.component_tags = ['EngVFXPreview']
    c.set_variable_float('SizeX', e.x * 2.0); c.set_variable_float('SizeY', e.y * 2.0); c.set_variable_float('Intensity', 1.0)
    out.append('on %s at %s size %.0f x %.0f' % (v.get_name(), v.get_actor_location(), e.x * 2, e.y * 2))
else:
    out.append('off')
"\n".join(out)

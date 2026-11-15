# 12m: what does a TD.Dev.SpawnMine projectile carry? Lists live TDProjectile actors near the pawn with their components.
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
out = []
for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
    if 'TDProjectile' not in a.get_class().get_name() or a.is_hidden_ed():
        pass
    if 'TDProjectile' not in a.get_class().get_name():
        continue
    d = (a.get_actor_location() - pawn.get_actor_location()).length() / 100.0
    if d > 400:
        continue
    comps = []
    for c in a.get_components_by_class(unreal.SceneComponent):
        s = c.get_class().get_name()
        extra = ''
        if isinstance(c, unreal.NiagaraComponent):
            extra = ' asset=%s active=%s' % (str(c.get_asset()).split('.')[-1][:40] if c.get_asset() else None, c.is_active())
        elif isinstance(c, unreal.StaticMeshComponent):
            extra = ' mesh=%s' % (str(c.static_mesh).split('.')[-1][:40] if c.static_mesh else None)
        comps.append('%s vis=%s%s' % (s, c.is_visible(), extra))
    out.append('%s %.0f m hidden=%s: %s' % (a.get_name(), d, a.get_editor_property('hidden'), '; '.join(comps)))
"\n".join(out) or 'no projectiles'

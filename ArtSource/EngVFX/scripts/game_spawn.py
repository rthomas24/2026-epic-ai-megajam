# Triad Drift ENG-37: in-game preview helper. Runs in the EDITOR session's Python (port 8017) against its Play-In-Editor
# world (a -game process has no Python toolset), through g.sh / run_editor_py.py.
#   CMD = ('pie_start',) / ('pie_stop',) Nothing is saved: systems are spawned directly, no data asset is touched.
#
# Set CMD before this file's text (a wrapper prepends it), e.g.
#   CMD = ('row', ['NS_TD_Explosion_Small', ...], 2600, 900)   one-shots in a row in front of the player ship
#   CMD = ('tracers', 'NS_TD_Tracer_Lancer')                   attach a tracer to every live projectile, hide the placeholder
#   CMD = ('engine', 'NS_TD_Engine_Lancer', boost01, (x, y, z)) attach an exhaust to the player ship at an offset
#   CMD = ('at_enemies', 'NS_TD_Explosion_Medium')             spawn at every enemy ship
#   CMD = ('clear',)                                           destroy everything this helper spawned
SYS = '/Game/TriadDrift/Gameplay/VFX/Systems/'
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()   # the PIE world
if CMD[0] == 'pie_start':
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_begin_play()
elif CMD[0] == 'pie_stop':
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).editor_request_end_play()
pawn = unreal.GameplayStatics.get_player_pawn(world, 0) if world else None
NFL = unreal.NiagaraFunctionLibrary
out = []


def load(name):
    return unreal.load_asset(SYS + name)


def tag(comp):
    if comp:
        comp.component_tags = list(comp.component_tags) + ['EngVFXPreview']
    return comp


kind = CMD[0]
if kind == 'row':
    names, dist, gap = CMD[1], CMD[2], CMD[3]
    xf = pawn.get_actor_transform()
    for i, n in enumerate(names):
        y = (i - (len(names) - 1) / 2.0) * gap
        loc = unreal.MathLibrary.transform_location(xf, unreal.Vector(dist, y, CMD[5] if len(CMD) > 5 else 250.0))
        rot = pawn.get_actor_rotation()
        if len(CMD) > 4:
            rot = unreal.MathLibrary.compose_rotators(unreal.Rotator(0, 0, CMD[4]), rot)   # roll, pitch, yaw: yaw offset
        c = tag(NFL.spawn_system_at_location(world, load(n), loc, rot, unreal.Vector(1, 1, 1), True, True, unreal.NCPoolMethod.NONE, True))
        out.append('%s %s' % (n, 'ok' if c else 'FAILED'))
elif kind == 'attach_row':
    names, dist, gap = CMD[1], CMD[2], CMD[3]
    for i, n in enumerate(names):
        y = (i - (len(names) - 1) / 2.0) * gap
        c = tag(NFL.spawn_system_attached(load(n), pawn.root_component, 'None', unreal.Vector(dist, y, 250.0), unreal.Rotator(0, 0, 0),
                                          unreal.AttachLocation.KEEP_RELATIVE_OFFSET, True, True, unreal.NCPoolMethod.NONE, True))
        out.append('%s %s' % (n, 'ok' if c else 'FAILED'))
elif kind == 'tracers':
    n = 0
    for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.TDProjectile):
        if a.get_editor_property('hidden'):
            continue
        tag(NFL.spawn_system_attached(load(CMD[1]), a.root_component, 'None', unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0),
                                      unreal.AttachLocation.SNAP_TO_TARGET, True, True, unreal.NCPoolMethod.NONE, True))
        for m in a.get_components_by_class(unreal.StaticMeshComponent):
            m.set_visibility(False, False)
        n += 1
    out.append('tracers attached: %d' % n)
elif kind == 'engine':
    c = tag(NFL.spawn_system_attached(load(CMD[1]), pawn.root_component, 'None', unreal.Vector(*CMD[3]), unreal.Rotator(0, 0, 0),
                                      unreal.AttachLocation.KEEP_RELATIVE_OFFSET, True, True, unreal.NCPoolMethod.NONE, True))
    if c:
        c.set_variable_float('Boost', float(CMD[2]))
        if len(CMD) > 4:
            c.set_variable_float('Scale', float(CMD[4]))
    out.append('engine %s' % ('ok' if c else 'FAILED'))
elif kind == 'attach_set':   # ('attach_set', name, (x, y, z), {'Power': 1.0, ...}): attach to the player ship and set user floats
    c = tag(NFL.spawn_system_attached(load(CMD[1]), pawn.root_component, 'None', unreal.Vector(*CMD[2]), unreal.Rotator(0, 0, 0),
                                      unreal.AttachLocation.KEEP_RELATIVE_OFFSET, True, True, unreal.NCPoolMethod.NONE, True))
    for k, v in (CMD[3] if len(CMD) > 3 else {}).items():
        c.set_variable_float(k, float(v))
    out.append('%s %s' % (CMD[1], 'ok' if c else 'FAILED'))
elif kind == 'pickups':   # ('pickups', dist, gap[, names]): the four pickup idles in a row in front of the ship, world space
    # (T2's ATDPickup is not on this branch, and Python cannot spawn an actor into the PIE world to rebuild its shapes)
    dist, gap = CMD[1], CMD[2]
    names = CMD[3] if len(CMD) > 3 else ['NS_TD_Pickup_Hull', 'NS_TD_Pickup_Shield', 'NS_TD_Pickup_Power', 'NS_TD_Pickup_Overcharge']
    xf = pawn.get_actor_transform()
    for i in range(len(names)):
        loc = unreal.MathLibrary.transform_location(xf, unreal.Vector(dist, (i - (len(names) - 1) / 2.0) * gap, 0.0))
        c = tag(NFL.spawn_system_at_location(world, load(names[i]), loc, unreal.Rotator(0, 0, 0), unreal.Vector(1, 1, 1), True, True, unreal.NCPoolMethod.NONE, True))
        out.append('%s %s' % (names[i], 'ok' if c and c.is_active() else 'NOT ACTIVE'))
elif kind == 'at_enemies':
    n = 0
    for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.TDEnemyShip):
        tag(NFL.spawn_system_at_location(world, load(CMD[1]), a.get_actor_location(), a.get_actor_rotation(), unreal.Vector(1, 1, 1), True, True, unreal.NCPoolMethod.NONE, True))
        n += 1
    out.append('spawned at %d enemies' % n)
elif kind == 'clear':
    n = 0
    for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor):
        for c in a.get_components_by_tag(unreal.NiagaraComponent, 'EngVFXPreview'):
            c.destroy_component(c)
            n += 1
    out.append('cleared %d' % n)
elif kind == 'info':
    b = pawn.get_actor_bounds(True)
    out.append('pawn %s bounds extent %s vel %s' % (pawn.get_name(), b[1], pawn.get_velocity()))
    out.append('sockets %s' % [str(s) for s in pawn.root_component.get_all_socket_names()][:20])
    for c in pawn.get_components_by_class(unreal.SceneComponent)[:40]:
        out.append('  comp %s %s rel %s' % (c.get_class().get_name(), c.get_name(), c.get_editor_property('relative_location')))
"\n".join(out)

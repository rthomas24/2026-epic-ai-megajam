# #163: drive the pilot's blade in the editor's Play-In-Editor world (editor session Python; a -game process has none).
# A wrapper prepends   ACT = '<action>'   (see mk_acts() in blade_cap.py), e.g. 'light', 'heavy', 'slam', 'jump',
# 'block', 'unblock', 'retract', 'ignite', 'info', 'pie_start', 'pie_stop', 'con:<console command>'.
import json
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
out = []
if ACT == 'pie_start':
    les.editor_request_begin_play()
    out.append('pie requested')
elif ACT == 'pie_stop':
    les.editor_request_end_play()
    out.append('pie stop requested')
elif not world:
    out.append('NO PIE WORLD')
else:
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    blade = pawn.get_component_by_class(unreal.TDEnergyBladeComponent) if pawn else None
    if ACT.startswith('con:'):
        unreal.SystemLibrary.execute_console_command(world, ACT[4:])
        out.append('con ' + ACT[4:])
    elif not blade:
        out.append('NO BLADE on %s' % pawn)
    elif ACT == 'light':
        out.append('light %s' % blade.light_attack())
    elif ACT == 'heavy':
        out.append('heavy %s' % blade.heavy_lunge())
    elif ACT == 'jump':
        pawn.jump()
        out.append('jump')
    elif ACT == 'slam':
        out.append('slam %s' % blade.air_slam())
    elif ACT == 'block':
        blade.set_block_held(True)
        out.append('block')
    elif ACT == 'unblock':
        blade.set_block_held(False)
        out.append('unblock')
    elif ACT == 'retract':
        blade.call_method('Retract') if hasattr(blade, 'call_method') else None
        out.append('retract')
    elif ACT == 'turn':
        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        r = pc.get_control_rotation()
        pc.set_control_rotation(unreal.Rotator(r.roll, r.pitch, r.yaw + 35.0))
        out.append('turn')
    elif ACT == 'info':
        aura = None
        for c in pawn.get_components_by_class(unreal.NiagaraComponent):
            if c.get_name() == 'BladeAura':
                aura = c
        beam = pawn.get_visible_blade_beam() if hasattr(pawn, 'get_visible_blade_beam') else None
        out.append(json.dumps({'pawn': pawn.get_name(), 'loc': str(pawn.get_actor_location()), 'state': str(blade.get_blade_state()),
                               'aura': bool(aura), 'aura_active': bool(aura and aura.is_active()),
                               'beam': beam.get_name() if beam else None,
                               'beam_mesh': beam.static_mesh.get_name() if beam and beam.static_mesh else None}))
'\n'.join(out)

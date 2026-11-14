# #163: wire the blade's new systems into DA_Blade_FX (Gameplay/Characters/FX). Sets ONLY cue.vfx; every Sound,
# shake and scale is read before and after and the asset is not saved if any of them changed.
#   run_editor_py.py <port> ArtSource/EngVFX/Blade/scripts/blade_wire.py        (SAVE = False: dry run, prints only)
# 'NONE' clears a cue's VFX (its sound stays). Undo: PLAN_BEFORE (the round-6 wiring) below.
SYS = '/Game/TriadDrift/Gameplay/VFX/Systems/'
DA = '/Game/TriadDrift/Gameplay/Characters/FX/DA_Blade_FX'
SAVE = globals().get('SAVE', True)
PLAN_BEFORE = {'air_slam': 'NS_TD_Blade_AirSlam', 'swing_light': 'NS_TD_Blade_SwingArc', 'swing_heavy': 'NS_TD_Blade_SwingArc_Heavy',
               'deflect': 'NS_TD_Blade_Deflect', 'perfect_deflect': 'NS_TD_Blade_PerfectDeflect'}
PLAN_163 = {
    'ignite': 'NS_TD_Blade_Ignite', 'retract': 'NS_TD_Blade_Retract', 'hit': 'NS_TD_Blade_Hit',
    'deflect': 'NS_TD_Blade_Deflect_V2', 'perfect_deflect': 'NS_TD_Blade_PerfectDeflect_V2', 'air_slam': 'NS_TD_Blade_AirSlam_V2',
    # the aura's swing sheet follows the real blade now: the crescent in front of the chest and the per-swing trail
    # (attached to the first-person blade, wrong in third person) would double it
    'swing_light': 'NONE', 'swing_heavy': 'NONE', 'swing_trail': 'NONE',
}
PLAN = globals().get('PLAN') or PLAN_163
da = unreal.load_asset(DA)


def state():
    out = {}
    for f in ('ignite', 'retract', 'swing_light', 'swing_heavy', 'swing_trail', 'hit', 'block', 'deflect', 'perfect_deflect', 'guard_break', 'air_slam'):
        c = da.get_editor_property(f)
        out[f] = (str(c.get_editor_property('vfx')).split("'")[-2] if c.get_editor_property('vfx') else None,
                  str(c.get_editor_property('sound')), str(c.get_editor_property('shake')), c.get_editor_property('shake_scale'), str(c.get_editor_property('vfx_scale')))
    return out


before = state()
log = ['BEFORE ' + str({k: v[0] for k, v in before.items()})]
for field, name in PLAN.items():
    cue = da.get_editor_property(field)
    if name == 'NONE':
        cue.set_editor_property('vfx', None)
    else:
        s = unreal.load_asset(SYS + name)
        if not s:
            log.append('NO SYSTEM ' + name)
            continue
        cue.set_editor_property('vfx', s)
    da.set_editor_property(field, cue)
after = state()
changed = [k for k in before if before[k][1:] != after[k][1:]]
log.append('AFTER ' + str({k: v[0] for k, v in after.items()}))
if changed:
    log.append('NON-VFX FIELDS CHANGED %s: not saved' % changed)
elif SAVE:
    log.append('saved %s' % unreal.EditorAssetLibrary.save_loaded_asset(da, False))
log.append('sounds: ' + str({k: v[1].split('/')[-1] for k, v in after.items()}))
"\n".join(log)

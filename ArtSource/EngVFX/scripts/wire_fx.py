# Triad Drift ENG-37 round 5: wiring that became possible with T1/T4's code (nozzle sockets, Throttle/Boost, break-up cues).
# Sets ONLY cue.vfx and cue.vfx_scale; reads every Sound field before and after and refuses to save on any difference.
#   python ArtSource/EngVFX/scripts/run_editor_py.py 8017 ArtSource/EngVFX/scripts/wire_fx.py
# PLAN value: system name, or (system name, uniform VFXScale), or (None, scale) to change the scale only.
SYS = '/Game/TriadDrift/Gameplay/VFX/Systems/'
CH = '/Game/TriadDrift/Gameplay/Characters/'
SAVE = globals().get('SAVE', True)
PLAN_R6 = {   # round 6 (the five DA_Enemy_*_Visuals and everything under Characters/Squad are NOT to be saved this round)
    'FX/DA_Travel_FX': {'atmospheric_entry': 'NS_TD_Entry'},
    'FX/DA_Blade_FX': {'air_slam': 'NS_TD_Blade_AirSlam', 'swing_light': 'NS_TD_Blade_SwingArc', 'swing_heavy': 'NS_TD_Blade_SwingArc_Heavy',
                       'deflect': 'NS_TD_Blade_Deflect', 'perfect_deflect': 'NS_TD_Blade_PerfectDeflect'},
}
PLAN_R7 = {   # round 7: per-type enemy exhaust. EngineTrail VFX only; the VFXScale the asset holds is left alone.
    'NullFleet/DA_Enemy_Fighter_Visuals': {'engine_trail': 'NS_TD_Engine_Enemy_Fighter'},
    'NullFleet/DA_Enemy_Interceptor_Visuals': {'engine_trail': 'NS_TD_Engine_Enemy_Interceptor'},
}
PLAN_R8 = {   # round 8: the round-6 leftovers whose slots exist (hyperdrive is Teammate 7's look; its slots wait for T7's systems)
    'FX/DA_Travel_FX': {'portal_open': 'NS_TD_Portal_Open', 'portal_close': 'NS_TD_Portal_Close'},
    'FX/DA_Ability_EMPPulse_FX': {'activate': 'NS_TD_EMP_Pulse'},
    'FX/DA_Ability_Cloak_FX': {'loop': 'NS_TD_Cloak_Shimmer'},
    'FX/DA_Ability_GravityWell_FX': {'loop': 'NS_TD_GravityWell'},
    # the Act 1 distortion field (TDDistortionField scales VFXScale by its radius / 1 km itself, so 1 here)
    'Dominion/DA_Dominion_Visuals': {'distortion_field_idle': ('NS_TD_Dominion_Field', 1.0), 'distortion_field_collapse': ('NS_TD_Dominion_FieldCollapse', 1.0),
                                     'distortion_field_contact': ('NS_TD_Impact_Shield', 3.0)},
}
PLAN = globals().get('PLAN') or PLAN_R8
PLAN_R5 = {
    'NullFleet/DA_Enemy_Fighter_Visuals': {'engine_trail': 'NS_TD_Engine_Enemy'},
    'NullFleet/DA_Enemy_Interceptor_Visuals': {'engine_trail': 'NS_TD_Engine_Enemy'},
    'NullFleet/DA_Enemy_Gunship_Visuals': {'engine_trail': ('NS_TD_Engine_Enemy', 2.0)},
    'NullFleet/DA_Enemy_Bomber_Visuals': {'engine_trail': ('NS_TD_Engine_Enemy', 1.5)},
    'NullFleet/DA_Enemy_Carrier_Visuals': {'engine_trail': ('NS_TD_Engine_Enemy', 5.0), 'explosion': (None, 2.5)},
    'Dominion/DA_Dominion_Visuals': {'hull_section_explosion': 'NS_TD_Explosion_HullSection', 'breakup_final_blast': 'NS_TD_Explosion_FinalBlast',
                                     # the distortion field: authored 1 km radius, the hull mesh's half length is 376 m
                                     'shield_idle': ('NS_TD_Dominion_Field', 0.46), 'shield_collapse': ('NS_TD_Dominion_FieldCollapse', 0.46)},
}


def sounds(da):
    out = {}
    for prop in dir(da):
        if prop.startswith('_'):
            continue
        try:
            v = da.get_editor_property(prop)
        except Exception:
            continue
        if isinstance(v, unreal.TDFXCue):
            out[prop + '.sound'] = str(v.get_editor_property('sound'))
        elif 'sound' in prop.lower() or 'loop' in prop.lower() or 'music' in prop.lower():
            out[prop] = str(v)
    return out


log, dump = [], []
for rel, fields in PLAN.items():
    da = unreal.load_asset(CH + rel)
    if not da:
        log.append('MISSING ' + rel); continue
    before = sounds(da)
    done = []
    for field, val in fields.items():
        name, scale = val if isinstance(val, tuple) else (val, None)
        try:
            cue = da.get_editor_property(field)
            if name:
                s = unreal.load_asset(SYS + name)
                if not s:
                    log.append('NO SYSTEM ' + name); continue
                cue.set_editor_property('vfx', s)
            if scale is not None:
                cue.set_editor_property('vfx_scale', unreal.Vector(scale, scale, scale))
            da.set_editor_property(field, cue)
            c2 = da.get_editor_property(field)
            done.append('%s=%s x%s' % (field, str(c2.get_editor_property('vfx')).split('.')[-1][:40], c2.get_editor_property('vfx_scale').x))
        except Exception as ex:
            log.append('FAIL %s.%s: %s' % (rel, field, str(ex)[:200]))
    after = sounds(da)
    if before != after:
        log.append('SOUND CHANGED on %s, not saved' % rel); continue
    if SAVE and not unreal.EditorAssetLibrary.save_loaded_asset(da, False):
        log.append('SAVE FAILED ' + rel)
    for k in sorted(before):
        dump.append('%s | %s | before=%s | after=%s | %s' % (rel, k, before[k], after.get(k), 'SAME' if before[k] == after.get(k) else 'CHANGED'))
    log.append('%s: %s | %d sound fields unchanged' % (rel, done, len(after)))
if SAVE:
    _p = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()) + 'ArtSource/EngVFX/review/wire_sound_check.txt'
    with open(_p, 'a', encoding='utf-8') as f:
        f.write('==== wire_r5 run: %d sound fields read, %d changed\n' % (len(dump), sum(1 for l in dump if l.endswith('CHANGED'))))
        f.write('\n'.join(dump) + '\n')
"\n".join(log)

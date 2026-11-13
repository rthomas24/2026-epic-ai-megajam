# Triad Drift ENG-37: wires the tier-1 Niagara systems into the ENGINEERING FALLBACK data assets
# (Content/TriadDrift/Gameplay/Characters/**). Sets ONLY VFX fields of cues and the weapon Tracer; never touches Sound,
# Shake, meshes or colours. Reads every Sound field of every touched asset before and after and reports any difference.
# Run in the editor (not during PIE):
#   python ArtSource/EngVFX/scripts/run_editor_py.py 8017 ArtSource/EngVFX/scripts/wire_tier1.py
# Set UNWIRE = True (a wrapper line before this text) to clear exactly the fields listed here again.
SYS = '/Game/TriadDrift/Gameplay/VFX/Systems/'
CH = '/Game/TriadDrift/Gameplay/Characters/'
UNWIRE = globals().get('UNWIRE', False)
SKIP_TRACERS = globals().get('SKIP_TRACERS', False)
H, SH = 'NS_TD_Impact_Hull', 'NS_TD_Impact_Shield'


def weapon(tracer, muzzle, detonation=None):
    d = {'muzzle': muzzle, 'impact_hull': H, 'impact_shield': SH, 'impact_world': H}
    if tracer and not SKIP_TRACERS:
        d['tracer'] = tracer
    if detonation:
        d['detonation'] = detonation
    return d


def ship(name, boom):
    return {'engine_trail': 'NS_TD_Engine_' + name, 'boost_loop': 'NS_TD_EngineBoost_' + name, 'hull_hit': H, 'shield_hit': SH, 'explosion': boom}


def enemy(boom):
    # EngineTrail is NOT wired for enemies: the code attaches it at the hull mesh origin, not at the nozzle (code hook wanted).
    return {'hull_hit': H, 'shield_hit': SH, 'explosion': boom}


PLAN = {
    'FX/DA_PhasePulseLasers_FX': weapon('NS_TD_Tracer_Rift', 'NS_TD_Muzzle_Rift'),
    'FX/DA_SplitterBolt_FX': weapon('NS_TD_Tracer_Rift', 'NS_TD_Muzzle_Rift'),
    'FX/DA_NeedleCannons_FX': weapon('NS_TD_Tracer_Lancer', 'NS_TD_Muzzle_Lancer'),
    'FX/DA_SpikeRockets_FX': weapon(None, 'NS_TD_Muzzle_Lancer', 'NS_TD_Explosion_Small'),
    'FX/DA_BreakerBlaster_FX': weapon('NS_TD_Tracer_Bastion', 'NS_TD_Muzzle_Bastion', 'NS_TD_Explosion_Small'),
    'FX/DA_FlakBurst_FX': weapon('NS_TD_Tracer_Bastion', 'NS_TD_Muzzle_Bastion', 'NS_TD_Explosion_Medium'),
    'FX/DA_Enemy_PulseGuns_FX': weapon('NS_TD_Tracer_Enemy', 'NS_TD_Muzzle_Enemy'),
    'FX/DA_Enemy_HeavyBolt_FX': weapon('NS_TD_Tracer_Enemy', 'NS_TD_Muzzle_Enemy'),
    'FX/DA_Enemy_PointDefense_FX': weapon('NS_TD_Tracer_Enemy', 'NS_TD_Muzzle_Enemy'),
    'Squad/DA_Rift_Visuals': ship('Rift', 'NS_TD_Explosion_Medium'),
    'Squad/DA_Lancer_Visuals': ship('Lancer', 'NS_TD_Explosion_Medium'),
    'Squad/DA_Bastion_Visuals': ship('Bastion', 'NS_TD_Explosion_Medium'),
    'NullFleet/DA_Enemy_Fighter_Visuals': enemy('NS_TD_Explosion_Small'),
    'NullFleet/DA_Enemy_Interceptor_Visuals': enemy('NS_TD_Explosion_Small'),
    'NullFleet/DA_Enemy_Gunship_Visuals': enemy('NS_TD_Explosion_Medium'),
    'NullFleet/DA_Enemy_Bomber_Visuals': enemy('NS_TD_Explosion_Medium'),
    'NullFleet/DA_Enemy_Carrier_Visuals': enemy('NS_TD_Explosion_Large'),
}


def sounds(da):
    """Every sound reference on the asset: plain Sound properties and the Sound of every cue struct."""
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


log = []
for rel, fields in PLAN.items():
    path = CH + rel
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        log.append('MISSING ' + rel)
        continue
    da = unreal.load_asset(path)
    before = sounds(da)
    done = []
    for field, sysname in fields.items():
        target = None if UNWIRE else unreal.load_asset(SYS + sysname)
        if not UNWIRE and target is None:
            log.append('NO SYSTEM %s for %s.%s' % (sysname, rel, field))
            continue
        try:
            cur = da.get_editor_property(field)
            if isinstance(cur, unreal.TDFXCue):
                cur.set_editor_property('vfx', target)
                da.set_editor_property(field, cur)
            else:
                da.set_editor_property(field, target)
            done.append(field)
        except Exception as ex:
            log.append('FAIL %s.%s: %s' % (rel, field, str(ex)[:200]))
    after = sounds(da)
    if before != after:
        log.append('SOUND CHANGED on %s: %s' % (rel, {k: (before.get(k), after.get(k)) for k in set(before) | set(after) if before.get(k) != after.get(k)}))
        continue  # do not save an asset whose sounds changed
    unreal.EditorAssetLibrary.save_loaded_asset(da)
    set_sounds = sum(1 for v in after.values() if v not in ('None', ''))
    log.append('%s: %s %s | sound fields set: %d of %d (unchanged)' % (rel, 'cleared' if UNWIRE else 'wired', done, set_sounds, len(after)))
"\n".join(log)

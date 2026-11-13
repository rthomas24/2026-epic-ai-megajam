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
# Opt-in parts (set by a wrapper line before this text). Default = the Job-1 set: tracers, explosions (as weapon
# detonations) and the hull impact, in the weapon FX assets under Characters/FX only.
WIRE_MUZZLE = globals().get('WIRE_MUZZLE', False)
WIRE_SHIELD = globals().get('WIRE_SHIELD', False)
WIRE_ENGINES = globals().get('WIRE_ENGINES', False)
# The ship visuals (Characters/Squad, Characters/NullFleet) hold the ship-death Explosion, HullHit / ShieldHit and the
# engine cues. They are outside the ENG-37 builder's writable paths, so this is off unless the Lead turns it on.
WIRE_SHIPS = globals().get('WIRE_SHIPS', False)
ONLY_TRACERS = globals().get('ONLY_TRACERS', False)   # touch only the Tracer field (with UNWIRE: clear just the tracers)
SAVE = globals().get('SAVE', True)   # False: change the loaded assets only (a Play-In-Editor preview), write nothing
H, SH = 'NS_TD_Impact_Hull', 'NS_TD_Impact_Shield'


def weapon(tracer, muzzle, detonation=None):
    d = {'impact_hull': H, 'impact_world': H}
    if WIRE_MUZZLE and muzzle:
        d['muzzle'] = muzzle
    if WIRE_SHIELD:
        d['impact_shield'] = SH
    if tracer and not SKIP_TRACERS:
        d['tracer'] = tracer
    if detonation:
        d['detonation'] = detonation
    return d


SHIP_H, SHIP_SH = 'NS_TD_Ship_HullHit', 'NS_TD_Ship_ShieldHit'   # light: the weapon's impact cue plays at the same point


def ship(name, boom):
    # BoostLoop stays EMPTY: the EngineTrail system holds the boost flame (code drives User.Throttle / User.Boost).
    d = {'hull_hit': SHIP_H, 'explosion': boom}
    if WIRE_SHIELD:
        d['shield_hit'] = SHIP_SH
    if WIRE_ENGINES:
        d['engine_trail'] = 'NS_TD_Engine_' + name
    return d


def enemy(boom, **more):
    # EngineTrail is NOT wired for enemies: the code attaches it at the hull mesh origin, not at the nozzle (code hook wanted).
    d = {'hull_hit': SHIP_H, 'explosion': boom}
    if WIRE_SHIELD:
        d['shield_hit'] = SHIP_SH
    d.update(more)
    return d


PLAN = {
    'FX/DA_PhasePulseLasers_FX': weapon('NS_TD_Tracer_Rift', 'NS_TD_Muzzle_Rift'),
    'FX/DA_SplitterBolt_FX': weapon('NS_TD_Tracer_Rift', 'NS_TD_Muzzle_Rift'),
    'FX/DA_NeedleCannons_FX': weapon('NS_TD_Tracer_Lancer', 'NS_TD_Muzzle_Lancer'),
    'FX/DA_SpikeRockets_FX': weapon(None, 'NS_TD_Muzzle_Lancer', 'NS_TD_Explosion_Small'),
    'FX/DA_BreakerBlaster_FX': weapon('NS_TD_Tracer_Bastion', 'NS_TD_Muzzle_Bastion', 'NS_TD_Explosion_Small'),
    'FX/DA_FlakBurst_FX': weapon('NS_TD_Tracer_Bastion', 'NS_TD_Muzzle_Bastion', 'NS_TD_Explosion_Medium'),
    'FX/DA_MissileSwarm_FX': weapon(None, None, 'NS_TD_Explosion_Small'),
    'FX/DA_Enemy_PulseGuns_FX': weapon('NS_TD_Tracer_Enemy', 'NS_TD_Muzzle_Enemy'),
    'FX/DA_Enemy_HeavyBolt_FX': weapon('NS_TD_Tracer_Enemy', 'NS_TD_Muzzle_Enemy'),
    'FX/DA_Enemy_PointDefense_FX': weapon('NS_TD_Tracer_Enemy', 'NS_TD_Muzzle_Enemy'),
    'FX/DA_Enemy_HomingMissile_FX': weapon(None, None, 'NS_TD_Explosion_Small'),
    'FX/DA_Enemy_SalvoMissile_FX': weapon(None, None, 'NS_TD_Explosion_Small'),
    'FX/DA_Enemy_PlasmaMines_FX': weapon(None, None, 'NS_TD_Explosion_Medium'),
}
if WIRE_SHIPS:
    PLAN.update({
        'Squad/DA_Rift_Visuals': ship('Rift', 'NS_TD_Explosion_Medium'),
        'Squad/DA_Lancer_Visuals': ship('Lancer', 'NS_TD_Explosion_Medium'),
        'Squad/DA_Bastion_Visuals': ship('Bastion', 'NS_TD_Explosion_Medium'),
        'NullFleet/DA_Enemy_Fighter_Visuals': enemy('NS_TD_Explosion_Small'),
        'NullFleet/DA_Enemy_Interceptor_Visuals': enemy('NS_TD_Explosion_Small'),
        'NullFleet/DA_Enemy_Gunship_Visuals': enemy('NS_TD_Explosion_Medium'),
        'NullFleet/DA_Enemy_Bomber_Visuals': enemy('NS_TD_Explosion_Medium'),
        'NullFleet/DA_Enemy_Carrier_Visuals': enemy('NS_TD_Explosion_Large', bay_destroyed='NS_TD_Explosion_Small', turret_destroyed='NS_TD_Explosion_Small'),
        'Dominion/DA_DominionTurret_Visuals': {'turret_destroyed': 'NS_TD_Explosion_Small', 'explosion': 'NS_TD_Explosion_Small'},
        'Dominion/DA_Dominion_Visuals': {'generator_explosion': 'NS_TD_Explosion_Medium', 'reactor_phase_change': 'NS_TD_Explosion_Medium',
                                         'reactor_destroyed': 'NS_TD_Explosion_Large'},
    })


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
sound_dump = []
for rel, fields in PLAN.items():
    path = CH + rel
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        log.append('MISSING ' + rel)
        continue
    da = unreal.load_asset(path)
    before = sounds(da)
    done = []
    for field, sysname in fields.items():
        if ONLY_TRACERS and field != 'tracer':
            continue
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
    if SAVE and not unreal.EditorAssetLibrary.save_loaded_asset(da, False):
        log.append('SAVE FAILED %s (is a game process holding the file open?)' % rel)
    for k in sorted(set(before) | set(after)):
        sound_dump.append('%s | %s | before=%s | after=%s | %s' % (rel, k, before.get(k), after.get(k), 'SAME' if before.get(k) == after.get(k) else 'CHANGED'))
    set_sounds = sum(1 for v in after.values() if v not in ('None', ''))
    log.append('%s: %s %s | sound fields set: %d of %d (unchanged)' % (rel, 'cleared' if UNWIRE else 'wired', done, set_sounds, len(after)))
if SAVE:
    _p = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()) + 'ArtSource/EngVFX/review/wire_sound_check.txt'
    with open(_p, 'a', encoding='utf-8') as f:
        f.write('==== wire_tier1 run (%s; muzzle=%s shield=%s engines=%s ships=%s): %d sound fields read, %d set before, %d set after, %d changed\n' % (
            'UNWIRE' if UNWIRE else 'wire', WIRE_MUZZLE, WIRE_SHIELD, WIRE_ENGINES, WIRE_SHIPS, len(sound_dump),
            sum(1 for l in sound_dump if '| before=None |' not in l), sum(1 for l in sound_dump if '| after=None |' not in l),
            sum(1 for l in sound_dump if l.endswith('CHANGED'))))
        f.write('\n'.join(sound_dump) + '\n')
"\n".join(log)

# Triad Drift ENG-37, tier 1: tracers, muzzle flashes, engine exhaust, impacts, explosions.
# Run inside the editor:
#   python ArtSource/EngVFX/scripts/run_editor_py.py 8017 ArtSource/EngVFX/scripts/build_tier1.py
# Rebuilds every NS_TD_* system of this tier in place from the specs below (the specs are the source of truth).
# Conventions: +X is forward. User.Color tints, User.Scale sizes; engines also read User.Throttle and User.Boost (0..1).
import sys
import importlib
sys.path.insert(0, unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()) + 'ArtSource/EngVFX/scripts')
import tdvfx
importlib.reload(tdvfx)
E, X, L = tdvfx.E, tdvfx.X, tdvfx.L
ES, EU, PS, PU = tdvfx.ES, tdvfx.EU, tdvfx.PS, tdvfx.PU
INIT = 'InitializeParticle'
ONLY = globals().get('ONLY')  # optional prefix filter, set by a wrapper script

# squad / enemy colour code (linear). Kept saturated; the materials add the white-hot core.
RIFT = (1.0, 0.10, 0.85, 1.0)
LANCER = (0.10, 0.80, 1.0, 1.0)
BASTION = (1.0, 0.55, 0.08, 1.0)
ENEMY = (1.0, 0.22, 0.05, 1.0)
FIRE = (1.0, 0.42, 0.10, 1.0)
SHIELD = (0.25, 0.60, 1.0, 1.0)
FADE = 'pow(1.0 - Particles.NormalizedAge, %s)'
FLICK = '(0.80 + 0.40*frac(sin(Engine.Time*%s)*43758.5))'


def col(k=1.0, a=1.0, src='User.Color'):
    return X('float4(%s.rgb*%s, %s)' % (src, k, a))


def once(dur=1.0):
    return [(EU, 'EmitterState', 'Loop Behavior', E('Once')), (EU, 'EmitterState', 'Loop Duration', float(dur))]


def fade_alpha(power=1.5, mul='1.0'):
    return [(PU, 'ScaleColor', 'ScaleRGB', False), (PU, 'ScaleColor', 'ScaleA', True),
            (PU, 'ScaleColor', 'Scale Alpha', X('%s*%s' % (FADE % power, mul)))]


def grow(expr, base='rate'):
    """Sprite size multiplier over life (expr may use Particles.NormalizedAge)."""
    if base == 'burst':  # the burst template carries the older ScaleSpriteSize (a float2 factor)
        return [(PU, 'ScaleSpriteSize', 'Scale Factor', X('float2(1.0, 1.0)*(%s)' % expr))]
    return [(PU, 'ScaleSpriteSize', 'Scale Sprite Size Mode', E('Uniform')),
            (PU, 'ScaleSpriteSize', 'Uniform Scale Factor', X(expr))]


def single(name, mat, size, k=1.0, flicker=None, local=True, life=None, extra=None, rend=None, offset=None):
    """One persistent sprite (a looping particle): cores and halos."""
    sets = [(PS, INIT, 'Color Mode', E('Direct Set')), (PS, INIT, 'Color', col(k))]
    if isinstance(size, tuple):
        sets += [(PS, INIT, 'Sprite Size Mode', E('Non-Uniform')), (PS, INIT, 'Sprite Size', X('float2(%s, %s)*User.Scale' % size))]
    else:
        sets += [(PS, INIT, 'Sprite Size Mode', E('Uniform')), (PS, INIT, 'Uniform Sprite Size', X('%s*User.Scale' % size))]
    if offset:
        sets += [(PS, INIT, 'UsePositionOffset', True), (PS, INIT, 'Position Offset', X('float3(%s,%s,%s)*User.Scale' % offset))]
    add = []
    if flicker:
        add.append((PU, 'Update/Color/ScaleColor'))
        sets += [(PU, 'ScaleColor', 'ScaleA', False), (PU, 'ScaleColor', 'Scale RGB', X('float3(1,1,1)*' + FLICK % flicker))]
    return {'name': name, 'base': 'single', 'props': {'bLocalSpace': local}, 'add': add + (extra or {}).get('add', []),
            'set': sets + (extra or {}).get('set', []), 'mat': mat, 'rend': rend or {}}


def needle(name, width, length, k=1.0, flicker=None, mat='M_TD_VFX_Spark', back=0.0):
    """A persistent sprite stretched along +X (velocity aligned): bolt heads, motor flames."""
    e = single(name, mat, (width, length), k, flicker, rend={'Alignment': 'VelocityAligned'},
               offset=(back, 0, 0) if back else None)
    e['add'] = [(PS, 'Spawn/Velocity/AddVelocity')] + e['add']
    e['set'] += [(PS, 'AddVelocity', 'Velocity', (4.0, 0.0, 0.0))]
    return e


def ribbon(name, width, life, k=1.0, rate=150.0, mat='M_TD_VFX_Ribbon', taper=0.75, power=1.4, tile=700):
    return {'name': name, 'base': 'ribbon',
            'add': [(EU, 'Emitter/SpawnRate'), (PU, 'Ribbons/ScaleRibbonWidth'), (PU, 'Update/Color/ScaleColor')],
            'set': [(EU, 'EmitterState', 'Loop Behavior', E('Infinite')), (EU, 'SpawnRate', 'SpawnRate', float(rate)),
                    (PS, INIT, 'Lifetime', float(life)), (PS, INIT, 'Color Mode', E('Direct Set')), (PS, INIT, 'Color', col(k)),
                    (PS, INIT, 'Ribbon Width Mode', E('Direct Set')), (PS, INIT, 'Ribbon Width', X('%s*User.Scale' % width)),
                    (PU, 'ScaleRibbonWidth', 'Ribbon Width Scale', X('1.0 - Particles.NormalizedAge*%s' % taper))] + fade_alpha(power),
            'mat': mat, 'rend': {'UV0Settings': {'distributionMode': 'TiledOverRibbonLength', 'tilingLength': tile}}}


def shed(name, mat, rate, life, size, speed, k=1.0, radius=10.0, drag=2.0, curl=0.0, local=False, aligned=False,
         power=1.5, growth=None, cone=None, burst=None, dur=1.0, rate_expr=None, gpu=False, life_mul=None, sort=False, alpha=1.0, fade_in=None, stretch=None, rgb=None):
    """Many small particles: sparks, wisps, puffs, embers. rate (continuous) or burst (count, one shot).
    cone = (axis xyz, angle) shoots them in a cone (emitter local space); otherwise outwards from the centre."""
    base = 'burst' if burst else 'rate'
    add = [(PU, 'Update/Size/ScaleSpriteSize')] if (growth and base == 'rate') else []
    sets = []
    if burst:
        sets += once(dur) + [(EU, 'SpawnBurst_Instantaneous', 'Spawn Count', X('int(%s)' % burst) if isinstance(burst, str) else int(burst))]
    else:
        sets += [(EU, 'EmitterState', 'Life Cycle Mode', E('Self')), (EU, 'EmitterState', 'Loop Behavior', E('Infinite')),
                 (EU, 'SpawnRate', 'SpawnRate', X(rate_expr) if rate_expr else float(rate))]
    sets += [(PS, INIT, 'Lifetime Min', float(life[0])), (PS, INIT, 'Lifetime Max', float(life[1])),
             (PS, INIT, 'Color Mode', E('Direct Set')),
             (PS, INIT, 'Color', X('float4(%s, %s, %s, %s)' % (rgb + (alpha,))) if rgb else col(k, alpha))]
    if stretch:  # thin streaks: width x (width*stretch), velocity aligned
        sets += [(PS, INIT, 'Sprite Size Mode', E('Random Non-Uniform')),
                 (PS, INIT, 'Sprite Size Min', X('float2(%s, %s)*User.Scale' % (size[0], size[0] * stretch))),
                 (PS, INIT, 'Sprite Size Max', X('float2(%s, %s)*User.Scale' % (size[1], size[1] * stretch)))]
    else:
        sets += [(PS, INIT, 'Uniform Sprite Size Min', X('%s*User.Scale' % size[0])), (PS, INIT, 'Uniform Sprite Size Max', X('%s*User.Scale' % size[1]))]
    sets += [(PS, 'ShapeLocation', 'Sphere Radius', X('%s*User.Scale' % radius))]
    spd = X('lerp(%s, %s, Particles.MaterialRandom)*User.Scale' % speed)
    if cone:
        sets += [(PS, 'AddVelocity', 'Velocity Mode', E('In Cone')), (PS, 'AddVelocity', 'Velocity Speed', spd),
                 (PS, 'AddVelocity', 'Cone Axis', cone[0]), (PS, 'AddVelocity', 'Cone Angle', float(cone[1]))]
    else:
        sets += [(PS, 'AddVelocity', 'Velocity Mode', E('From Point')), (PS, 'AddVelocity', 'Velocity Speed', spd)]
    sets += [(PU, 'GravityForce', 'Gravity', (0.0, 0.0, 0.0)), (PU, 'Drag', 'Drag', float(drag))] + fade_alpha(power, 'saturate(Particles.NormalizedAge*%s)' % fade_in if fade_in else '1.0')
    if curl:
        add.append((PU, 'Update/Forces/CurlNoiseForce'))
        sets += [(PU, 'CurlNoiseForce', 'Noise Strength', X('%s*User.Scale' % curl)), (PU, 'CurlNoiseForce', 'Noise Frequency', 0.004)]
    if growth:
        sets += grow(growth, base)
    rend = {'Alignment': 'VelocityAligned'} if (aligned or stretch) else {}
    if sort:
        rend['SortMode'] = 'ViewDistance'
    props = {'bLocalSpace': local}
    if gpu:
        props.update({'SimTarget': 'GPUComputeSim', 'CalculateBoundsMode': 'Fixed',
                      'FixedBounds': {'min': {'x': -3000, 'y': -3000, 'z': -3000}, 'max': {'x': 3000, 'y': 3000, 'z': 3000}, 'isValid': True}})
    return {'name': name, 'base': base, 'props': props, 'add': add, 'set': sets, 'mat': mat, 'rend': rend}


def flash(name, mat, size, life, k=1.0, growth=None, power=1.0, delay=0.0, rot=True):
    """One short-lived sprite: flashes, rings, ripples."""
    e = shed(name, mat, 0, (life, life), (size, size), (0, 0), k=k, radius=0.0, drag=0.0, power=power, growth=growth, burst=1, dur=life + delay + 0.05)
    if delay:
        e['set'].append((EU, 'SpawnBurst_Instantaneous', 'Spawn Time', float(delay)))
    return e


# --------------------------------------------------------------------------------------------- tracers
def tracer(color, head_w, head_l, halo, rib_w, rib_life, wisp_mat='M_TD_VFX_Ribbon', spark_rate=40.0, flick=71.3, extra=()):
    return [needle('Head', head_w, head_l, 1.0, flick),
            single('Halo', 'M_TD_VFX_Glow', halo, 0.38, flick * 1.31, offset=(head_l * 0.38, 0, 0)),
            ribbon('Trail', rib_w, rib_life, 0.9, mat=wisp_mat),
            shed('Sparks', 'M_TD_VFX_Glow', spark_rate * 1.6, (0.18, 0.45), (14, 30), (40, 260), k=1.0, radius=14.0, drag=1.5, curl=900.0)] + list(extra)


SYSTEMS = []


def S(name, user, emitters):
    if ONLY and not any(name.startswith(o) for o in ONLY):
        return
    SYSTEMS.append((name, user, emitters))


def U(color, scale=1.0, **kw):
    d = {'Color': color, 'Scale': float(scale)}
    d.update(kw)
    return d


# Rift: a fat phasing plasma bolt: pulsing halo, wide wispy ribbon, plasma wisps shed sideways.
S('NS_TD_Tracer_Rift', U(RIFT), tracer(RIFT, 70, 360, 230, 62, 0.085, spark_rate=30.0, flick=37.7, extra=[
    shed('Wisps', 'M_TD_VFX_Fire', 70.0, (0.16, 0.30), (45, 95), (20, 160), k=0.9, radius=18.0, drag=1.0, curl=1600.0, growth='1.0 + Particles.NormalizedAge*0.8'),
    single('Phase', 'M_TD_VFX_Ring', 150, 0.8, None, extra={'add': [(PU, 'Update/Size/ScaleSpriteSize')],
           'set': grow('0.55 + 0.75*frac(Engine.Time*5.5)')})]))
# Lancer: a tight fast needle with a thin streak that lingers.
S('NS_TD_Tracer_Lancer', U(LANCER), tracer(LANCER, 34, 520, 120, 22, 0.11, wisp_mat='M_TD_VFX_Ribbon_Clean', spark_rate=26.0, flick=113.1))
# Bastion: a heavy shell: flickering motor flame behind it and a short smoke trail.
S('NS_TD_Tracer_Bastion', U(BASTION), tracer(BASTION, 80, 300, 210, 46, 0.07, spark_rate=45.0, flick=53.9, extra=[
    needle('Motor', 95, 330, 0.9, 61.7, mat='M_TD_VFX_Jet', back=-190.0),
    shed('Smoke', 'M_TD_VFX_Smoke', 55.0, (0.35, 0.6), (60, 110), (10, 90), radius=10.0, drag=1.0, curl=500.0, power=1.0, rgb=(0.30, 0.22, 0.16), alpha=0.16, fade_in=5.0,
         growth='1.0 + Particles.NormalizedAge*1.6', sort=True)]))
# Enemy: a red-orange bolt, shorter and rounder than the squad's, with embers.
S('NS_TD_Tracer_Enemy', U(ENEMY), tracer(ENEMY, 64, 300, 190, 44, 0.07, spark_rate=34.0, flick=47.1))

# --------------------------------------------------------------------------------------------- muzzle flashes
def muzzle(color, size, tongue, sparks):
    return [flash('Flash', 'M_TD_VFX_Flare', size * 0.7, 0.07, 1.0, growth='0.6 + Particles.NormalizedAge*0.8'),
            flash('Glow', 'M_TD_VFX_Glow', size * 0.8, 0.08, 0.3),
            shed('Tongue', 'M_TD_VFX_Jet_Rev', 0, (0.06, 0.09), (tongue * 0.45, tongue * 0.6), (60, 120), k=1.2, radius=0.0, drag=0.0,
                 aligned=True, cone=((1.0, 0.0, 0.0), 4.0), burst=2, dur=0.2, power=0.8),
            shed('Sparks', 'M_TD_VFX_Spark', 0, (0.10, 0.26), (7, 13), (1500, 5200), k=1.4, radius=6.0, drag=3.0,
                 stretch=9, cone=((1.0, 0.0, 0.0), 22.0), burst=sparks * 2, dur=0.4)]


S('NS_TD_Muzzle_Rift', U(RIFT), muzzle(RIFT, 230, 300, 9))
S('NS_TD_Muzzle_Lancer', U(LANCER), muzzle(LANCER, 170, 380, 8))
S('NS_TD_Muzzle_Bastion', U(BASTION), muzzle(BASTION, 300, 340, 14))
S('NS_TD_Muzzle_Enemy', U(ENEMY), muzzle(ENEMY, 200, 260, 7))

# --------------------------------------------------------------------------------------------- engines
THR = '(0.45 + 0.55*User.Throttle + 0.9*User.Boost)'


def engine(color):
    jet = shed('Jet', 'M_TD_VFX_Jet_Rev', 55.0, (0.07, 0.11), (150, 190), (40, 60), k=0.60, radius=0.0, drag=0.0, local=True,
               aligned=True, cone=((-1.0, 0.0, 0.0), 2.0), power=0.6)
    for i, s in enumerate(jet['set']):
        if s[2] == 'Uniform Sprite Size Min':
            jet['set'][i] = (PS, INIT, 'Uniform Sprite Size Min', X('250*User.Scale*%s' % THR))
        if s[2] == 'Uniform Sprite Size Max':
            jet['set'][i] = (PS, INIT, 'Uniform Sprite Size Max', X('320*User.Scale*%s' % THR))
    flame = shed('Flame', 'M_TD_VFX_Fire', 0, (0.08, 0.18), (40, 70), ('900*%s' % THR, '2100*%s' % THR), k=0.40, radius=10.0, drag=1.5,
                 curl=2500.0, local=True, cone=((-1.0, 0.0, 0.0), 7.0), growth='1.0 - Particles.NormalizedAge*0.55',
                 rate_expr='170.0*(0.5 + 0.5*User.Throttle + 0.6*User.Boost)')
    plume = shed('Plume', 'M_TD_VFX_Glow', 0, (0.30, 0.50), (120, 180), ('700*%s' % THR, '1300*%s' % THR), k=0.10, radius=10.0, drag=0.6,
                 local=True, cone=((-1.0, 0.0, 0.0), 5.0), growth='1.0 + Particles.NormalizedAge*0.9', rate_expr='34.0')
    after = shed('Afterburn', 'M_TD_VFX_Spark', 0, (0.12, 0.30), (6, 11), (2600, 5200), k=1.5, radius=22.0, drag=1.0, local=True, stretch=10,
                 cone=((-1.0, 0.0, 0.0), 11.0), rate_expr='110.0*User.Boost')
    rings = shed('Rings', 'M_TD_VFX_Ring', 0, (0.16, 0.22), (120, 150), (1500, 1900), k=0.9, radius=0.0, drag=0.0, local=True,
                 cone=((-1.0, 0.0, 0.0), 1.0), growth='0.7 + Particles.NormalizedAge*0.9', rate_expr='14.0*User.Boost')
    return [single('Core', 'M_TD_VFX_Glow', '115*%s' % THR, 0.6, 83.3), jet, flame, plume, after, rings]


for _n, _c in (('Rift', RIFT), ('Lancer', LANCER), ('Bastion', BASTION), ('Enemy', ENEMY)):
    S('NS_TD_Engine_' + _n, U(_c, 1.0, Throttle=1.0, Boost=0.0), engine(_c))
    # the same exhaust with the afterburner on: for the BoostLoop cue slot, which is a separate attached loop
    S('NS_TD_EngineBoost_' + _n, U(_c, 1.0, Throttle=1.0, Boost=1.0), engine(_c)[3:])

# --------------------------------------------------------------------------------------------- impacts
S('NS_TD_Impact_Hull', U(FIRE), [
    flash('Flash', 'M_TD_VFX_Flare', 260, 0.09, 1.2, growth='0.5 + Particles.NormalizedAge'),
    flash('Glow', 'M_TD_VFX_Glow', 380, 0.16, 0.45),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.18, 0.50), (6, 12), (900, 4200), k=1.5, radius=10.0, drag=2.2, stretch=10,
         cone=((1.0, 0.0, 0.0), 62.0), burst=18, dur=0.7),
    shed('Flecks', 'M_TD_VFX_Glow', 0, (0.35, 0.8), (10, 24), (200, 1300), k=0.9, radius=14.0, drag=1.6, curl=700.0,
         cone=((1.0, 0.0, 0.0), 80.0), burst=10, dur=1.0, power=2.0),
    shed('Scorch', 'M_TD_VFX_Fire', 0, (0.16, 0.30), (90, 150), (60, 300), k=0.9, radius=20.0, drag=2.0,
         cone=((1.0, 0.0, 0.0), 70.0), burst=4, dur=0.5, growth='0.7 + Particles.NormalizedAge*0.9'),
    shed('Smoke', 'M_TD_VFX_Smoke', 0, (0.5, 0.9), (90, 150), (60, 260), radius=20.0, drag=1.5, rgb=(0.20, 0.13, 0.10), alpha=0.30,
         cone=((1.0, 0.0, 0.0), 60.0), burst=3, dur=1.0, power=1.0, fade_in=4.0, growth='1.0 + Particles.NormalizedAge*1.4', sort=True)])
S('NS_TD_Impact_Shield', U(SHIELD), [
    flash('Ripple', 'M_TD_VFX_Shield', 520, 0.38, 1.0, growth='0.55 + Particles.NormalizedAge*0.75', power=0.6),
    flash('Glow', 'M_TD_VFX_Glow', 300, 0.14, 0.6),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.14, 0.34), (6, 11), (700, 2800), k=1.5, radius=14.0, drag=2.5, stretch=8,
         cone=((1.0, 0.0, 0.0), 85.0), burst=12, dur=0.5),
    shed('Motes', 'M_TD_VFX_Glow', 0, (0.3, 0.6), (9, 20), (100, 700), k=1.0, radius=40.0, drag=2.0, curl=900.0, burst=8, dur=0.8, power=2.0)])

# --------------------------------------------------------------------------------------------- explosions
def explosion(n):
    """n scales the particle counts (1 small, 2 medium, 3.2 large); User.Scale scales the size in the world."""
    return [flash('Flash', 'M_TD_VFX_Glow', 1100, 0.08, 0.30, growth='0.5 + Particles.NormalizedAge*0.7', power=1.2),
            flash('Star', 'M_TD_VFX_Flare', 1500, 0.10, 0.6, growth='0.5 + Particles.NormalizedAge'),
            shed('Fireball', 'M_TD_VFX_Fire', 0, (0.55, 1.05), (400, 660), (400, 1500), k=0.16, radius=150.0, drag=3.2, curl=900.0,
                 burst=int(9 * n), dur=1.4, power=0.7, growth='0.55 + pow(Particles.NormalizedAge, 0.45)*1.1'),
            shed('Core', 'M_TD_VFX_Fire', 0, (0.25, 0.45), (380, 560), (40, 260), k=0.18, radius=60.0, drag=3.0,
                 burst=int(3 * n), dur=0.6, power=0.6, growth='0.6 + Particles.NormalizedAge*0.7'),
            flash('Shockwave', 'M_TD_VFX_Ring', 3000, 0.30, 0.28, growth='0.12 + pow(Particles.NormalizedAge, 0.55)*0.95', power=2.2),
            shed('Sparks', 'M_TD_VFX_Spark', 0, (0.35, 0.95), (9, 20), (1800, 7500), k=1.6, radius=60.0, drag=1.9, stretch=14,
                 burst=int(26 * n), dur=1.1, power=1.2),
            shed('Chunks', 'M_TD_VFX_Spark', 0, (0.7, 1.3), (16, 30), (600, 2600), k=0.9, radius=80.0, drag=0.9, stretch=4,
                 burst=int(7 * n), dur=1.4, power=1.0),
            shed('Smoke', 'M_TD_VFX_Smoke', 0, (0.9, 1.5), (320, 480), (150, 700), radius=200.0, drag=2.2, curl=500.0, rgb=(0.20, 0.13, 0.10), alpha=0.32,
                 burst=int(5 * n), dur=1.6, power=0.9, fade_in=3.0, growth='0.7 + Particles.NormalizedAge*1.2', sort=True),
            shed('Embers', 'M_TD_VFX_Glow', 0, (0.8, 1.5), (12, 30), (150, 1500), k=1.1, radius=150.0, drag=1.4, curl=1300.0,
                 burst=int(18 * n), dur=1.6, power=2.0)]


S('NS_TD_Explosion_Small', U(FIRE, 0.45), explosion(1.0))
S('NS_TD_Explosion_Medium', U(FIRE, 1.0), explosion(2.0))
S('NS_TD_Explosion_Large', U(FIRE, 2.2), explosion(3.2))

fx = tdvfx.Builder(call_tool)
for _name, _user, _emitters in SYSTEMS:
    await fx.system(_name, _emitters, user=_user)
    await asyncio.sleep(0)
fx.report()

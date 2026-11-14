# Triad Drift #163: the energy blade's Niagara systems. NOT run on its own: it reuses build_tier1.py's helpers, so the
# batch is   ONLY = ['__none__']  +  build_tier1.py  +  BLADE_ONLY = [...]  +  this file   (see blade_build.sh).
# Conventions as tier 1: User.Color (the code sets the pilot's squad colour on every blade cue and on the aura),
# User.Scale. Every system here is NEW (the round-6 blade systems are kept as they were).
#
# NS_TD_Blade_Aura: always on while the blade is lit, attached to the visible beam's origin (the emitter) with +X up
# the blade and absolute scale. The code sets User.Length (cm), User.TipVelocity (world cm/s), User.TipSpeed every frame.
BLADE_ONLY = globals().get('BLADE_ONLY')
SYSTEMS_B = []
MAGENTA = (1.0, 0.17, 0.84, 1.0)


def SB(name, user, emitters):
    if BLADE_ONLY and name not in BLADE_ONLY:
        return
    SYSTEMS_B.append((name, user, emitters))


def UA(**kw):
    d = {'Color': MAGENTA, 'Scale': 1.0, 'Length': 95.0, 'TipVelocity': (0.0, 0.0, 0.0), 'TipSpeed': 0.0}
    d.update(kw)
    return d


# moving: 0 still .. 1 a fast swing (the tip at 2,100 cm/s); walking with the blade is about 0.05-0.15
MOVE = 'pow(saturate((User.TipSpeed - 420.0)/1700.0), 1.25)'


def along(e, lo, hi, thick=1.5):
    """Spawn along the blade between lo and hi (fractions of User.Length), on the emitter's local +X."""
    e['set'] = [x for x in e['set'] if not (x[1] == 'ShapeLocation')]
    e['set'] += [(PS, 'ShapeLocation', 'Shape Primitive', E('Box / Plane')),
                 (PS, 'ShapeLocation', 'Box Size', X('float3(User.Length*%s, %s, %s)' % (hi - lo, thick, thick))),
                 (PS, 'ShapeLocation', 'Offset', X('float3(User.Length*%s, 0.0, 0.0)' % ((hi + lo) * 0.5)))]
    return e


def persistent_light(name, radius, k, at=0.5, flick=None):
    e = single(name, 'M_TD_VFX_Glow', 1, 1.0, flick, offset=(95.0 * at, 0, 0))
    e['rclass'] = '/Script/Niagara.NiagaraLightRendererProperties'
    e['rend'] = {'RadiusScale': radius / 100.0, 'bAlphaScalesBrightness': True, 'bAffectsTranslucency': False}
    e.pop('mat', None)
    e['set'] = [x for x in e['set'] if x[2] != 'Color'] + [(PS, INIT, 'Color', X('float4(User.Color.rgb*%s, 1.0)' % k))]
    return e


def sweep(name, at, width, life, k, power=1.3, rate=150.0):
    """The swing sheet: a world-space ribbon at `at` of the blade whose width runs ALONG the blade (side vector = the
    blade axis, carried in Particles.Velocity as a 1 cm/s velocity up +X), so it fills the arc the blade sweeps.
    Its brightness follows the tip speed at the moment each segment was laid down."""
    e = trail(name, 1, life, k, rate=rate, taper=0.0, power=power)
    e['mat'] = 'M_TD_VFX_BladeSweep'
    e['set'] = [x for x in e['set'] if x[2] not in ('Ribbon Width', 'Color')]
    e['set'] += [(PS, INIT, 'Ribbon Width', X('User.Length*%s' % width)),
                 (PS, INIT, 'Color', X('float4(User.Color.rgb*%s, %s)' % (k, MOVE))),
                 (PS, INIT, 'UsePositionOffset', True), (PS, INIT, 'Position Offset', X('float3(User.Length*%s, 0.0, 0.0)' % at))]
    e['add'] = [(PS, 'Spawn/Velocity/AddVelocity')] + e['add']
    e['set'] += [(PS, 'AddVelocity', 'Velocity Mode', E('In Cone')), (PS, 'AddVelocity', 'Cone Axis', (1.0, 0.0, 0.0)),
                 (PS, 'AddVelocity', 'Cone Angle', 0.0), (PS, 'AddVelocity', 'Velocity Speed', 1.0)]
    e['rend'] = {'UV0Settings': {'distributionMode': 'ScaledUniformly'}, 'FacingMode': 'CustomSideVector',
                 'RibbonFacingBinding': {'boundVariable': {'name': 'Particles.Velocity'}}}
    return e


def thrown(name, mat, rate_expr, life, size, inherit, spread, k, grav=-650.0, drag=1.2, stretch=None, lo=0.35, hi=1.0, curl=0.0, rgb_white=0.4):
    """World-space particles thrown off the blade: spawn rate and velocity follow the tip (User.TipVelocity)."""
    e = shed(name, mat, 0.0, life, size, (0, 0), k=k, radius=0.0, drag=drag, rate_expr=rate_expr, stretch=stretch, curl=curl)
    e = along(e, lo, hi)
    e['set'] = [x for x in e['set'] if x[1] != 'AddVelocity' and x[2] != 'Color']
    e['set'] += [(PS, INIT, 'Color', X('float4(lerp(User.Color.rgb, float3(1,1,1), %s)*%s, 1.0)' % (rgb_white, k))),
                 (PS, 'AddVelocity', 'Velocity Mode', E('Linear')),
                 (PS, 'AddVelocity', 'Velocity', X('User.TipVelocity*lerp(%s, %s, Particles.MaterialRandom) + (frac(Particles.MaterialRandom*float3(13.71, 29.37, 47.13)) - 0.5)*%s' % (inherit[0], inherit[1], spread)))]
    e['set'] = [x for x in e['set'] if x[1] != 'GravityForce'] + [(PU, 'GravityForce', 'Gravity', (0.0, 0.0, float(grav)))]
    return e


# ------------------------------------------------------------------------------------------------ the aura (always on)
SB('NS_TD_Blade_Aura', UA(), [
    persistent_light('Light', 300, 2.2, at=0.45, flick=31.4),
    single('Emitter', 'M_TD_VFX_Glow', 13, 0.9, 113.1, offset=(1.5, 0, 0)),
    along(shed('Embers', 'M_TD_VFX_BladeEmber', 26.0, (0.35, 0.8), (0.5, 1.2), (5, 25), k=1.5, radius=0.0, drag=1.5, curl=260.0, power=1.4), 0.05, 0.95, 2.5),
    along(spin(shed('Arcs', 'M_TD_VFX_FlipArc', 7.0, (0.07, 0.15), (5, 11), (0, 0), k=1.3, radius=0.0, drag=0.0, local=True, power=0.6)), 0.08, 0.92, 0.5),
    sweep('Sweep', 0.56, 0.86, 0.20, 1.4),
    sweep('SweepTip', 0.93, 0.16, 0.12, 2.2, power=1.6),
    thrown('Sparks', 'M_TD_VFX_Spark', '460.0*%s' % MOVE, (0.3, 0.6), (0.5, 1.1), (0.3, 0.8), 260.0, 1.8, stretch=9),
    thrown('Motes', 'M_TD_VFX_BladeEmber', '120.0*%s' % MOVE, (0.35, 0.7), (1.0, 2.4), (0.08, 0.25), 90.0, 1.5, grav=-120.0, drag=2.0, curl=300.0, lo=0.2),
])

# ------------------------------------------------------------------------------------------------ ignite / retract
# Attached to the visible beam's emitter, +X up the blade (the code attaches Ignite / Retract there).
SB('NS_TD_Blade_Ignite', U(MAGENTA), [
    flash('Flash', 'M_TD_VFX_Flare', 46, 0.09, 1.5, growth='0.4 + Particles.NormalizedAge*0.9'),
    flash('Glow', 'M_TD_VFX_Glow', 70, 0.20, 0.8),
    offset(mesh_flash('Rush', 'SM_TD_VFX_Bolt', 'M_TD_VFX_Bolt', '0.95, 0.06, 0.06', 0.16, 1.6, growth='float3(min(1.0, Particles.NormalizedAge*1.3 + 0.05), 1.0, 1.0)', power=2.0), 47, 0, 0),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.15, 0.4), (0.6, 1.2), (300, 1300), k=1.8, radius=1.5, drag=2.4, stretch=10, cone=((1.0, 0.0, 0.0), 35.0), burst=26, dur=0.5),
    shed('Ring', 'M_TD_VFX_BladeEmber', 0, (0.2, 0.45), (0.8, 1.6), (150, 420), k=1.6, radius=1.0, drag=3.0, cone=((1.0, 0.0, 0.0), 95.0), burst=18, dur=0.6),
    light('Light', 280, 0.22, k=4.0, rgb=(1.0, 0.6, 0.9)),
])
SB('NS_TD_Blade_Retract', U(MAGENTA), [
    flash('Flash', 'M_TD_VFX_Glow', 40, 0.14, 1.1, growth='1.0 - Particles.NormalizedAge*0.8'),
    along(shed('Collapse', 'M_TD_VFX_BladeEmber', 0, (0.08, 0.16), (0.8, 1.5), (0, 0), k=1.6, radius=0.0, drag=0.0, burst=20, dur=0.3), 0.1, 0.95, 1.0),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.1, 0.3), (0.5, 1.0), (200, 700), k=1.5, radius=1.0, drag=3.0, stretch=8, cone=((1.0, 0.0, 0.0), 70.0), burst=12, dur=0.4),
    spin(shed('Wisp', 'M_TD_VFX_Smoke', 0, (0.4, 0.8), (6, 12), (10, 40), radius=1.5, drag=1.5, rgb=(0.5, 0.5, 0.55), alpha=0.35, burst=3, dur=0.9, power=0.8,
              growth='0.7 + Particles.NormalizedAge*1.5', sort=True)),
])

# ------------------------------------------------------------------------------------------------ hit (world point, +X = the pilot's facing)
SB('NS_TD_Blade_Hit', U(MAGENTA), [
    flash('Flash', 'M_TD_VFX_Flare', 75, 0.08, 1.6, growth='0.5 + Particles.NormalizedAge'),
    flash('Glow', 'M_TD_VFX_Glow', 120, 0.16, 0.7),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.18, 0.5), (0.9, 2.0), (500, 2100), k=1.9, radius=4.0, drag=2.2, stretch=13, cone=((1.0, 0.0, 0.0), 55.0), burst=42, dur=0.6),
    shed('Hot', 'M_TD_VFX_Spark', 0, (0.08, 0.18), (1.6, 2.8), (900, 2600), k=2.4, radius=2.0, drag=3.0, stretch=18, cone=((1.0, 0.0, 0.0), 30.0), burst=10, dur=0.3),
    shed('Embers', 'M_TD_VFX_BladeEmber', 0, (0.35, 0.8), (1.0, 2.2), (120, 520), k=1.5, radius=6.0, drag=2.0, curl=300.0, cone=((1.0, 0.0, 0.0), 80.0), burst=14, dur=0.9),
    spin(shed('Smoke', 'M_TD_VFX_Smoke', 0, (0.5, 0.9), (16, 30), (20, 80), radius=4.0, drag=1.6, rgb=(0.32, 0.30, 0.32), alpha=0.45, cone=((1.0, 0.0, 0.0), 60.0),
              burst=3, dur=1.0, power=0.8, growth='0.8 + Particles.NormalizedAge*1.3', sort=True)),
    light('Light', 380, 0.14, k=5.0, rgb=(1.0, 0.75, 0.9)),
])
# set the light's colour from User.Color for the hit (light() takes a fixed rgb)
for _n, _u, _ems in SYSTEMS_B:
    for _e in _ems:
        if _e['name'] == 'Light' and _e.get('rclass', '').endswith('LightRendererProperties') and _n != 'NS_TD_Blade_Aura':
            _e['set'] = [x for x in _e['set'] if x[2] != 'Color'] + [(PS, INIT, 'Color', X('float4(lerp(User.Color.rgb, float3(1,1,1), 0.35)*5.0, 1.0)'))]

# ------------------------------------------------------------------------------------------------ deflect / perfect deflect / air slam (V2)
SB('NS_TD_Blade_Deflect_V2', U(MAGENTA), [
    flash('Flash', 'M_TD_VFX_Flare', 80, 0.08, 1.5, growth='0.5 + Particles.NormalizedAge'),
    flash('Glow', 'M_TD_VFX_Glow', 110, 0.14, 0.6),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.14, 0.4), (1.0, 2.2), (400, 1700), k=1.8, radius=2.0, drag=2.8, stretch=12, cone=((1.0, 0.0, 0.0), 50.0), burst=30, dur=0.5),
    shed('Embers', 'M_TD_VFX_BladeEmber', 0, (0.3, 0.6), (0.9, 1.8), (100, 420), k=1.4, radius=3.0, drag=2.2, curl=250.0, cone=((1.0, 0.0, 0.0), 75.0), burst=10, dur=0.7),
    light('Light', 300, 0.12, k=4.5),
])
SB('NS_TD_Blade_PerfectDeflect_V2', U(MAGENTA), [
    flash('Flash', 'M_TD_VFX_Flare', 220, 0.13, 1.7, growth='0.5 + Particles.NormalizedAge'),
    flash('Glow', 'M_TD_VFX_Glow', 280, 0.22, 0.75),
    mesh_flash('Ring', 'SM_TD_VFX_Disc', 'M_TD_VFX_Shock', '1.0, 2.6, 2.6', 0.30, 1.3, growth='float3(1,1,1)*(0.1 + pow(Particles.NormalizedAge, 0.5)*0.9)', power=1.3),
    mesh_flash('Ring2', 'SM_TD_VFX_Disc', 'M_TD_VFX_Shock', '1.0, 1.6, 1.6', 0.22, 1.6, growth='float3(1,1,1)*(0.1 + pow(Particles.NormalizedAge, 0.6)*0.9)', power=1.5, delay=0.05),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.2, 0.55), (1.3, 3.0), (500, 2400), k=2.0, radius=2.0, drag=2.4, stretch=14, cone=((1.0, 0.0, 0.0), 70.0), burst=54, dur=0.6),
    shed('Glints', 'M_TD_VFX_BladeEmber', 0, (0.4, 0.9), (1.2, 2.6), (150, 700), k=1.7, radius=6.0, drag=2.0, curl=350.0, burst=22, dur=1.0),
    light('Light', 650, 0.24, k=5.0),
])
SB('NS_TD_Blade_AirSlam_V2', U(MAGENTA), [
    opacity(flat_ring('Ring', 620, 0.38, 1.5), 0.75),
    opacity(flat_ring('Ring2', 470, 0.55, 0.65, mat='M_TD_VFX_Ring', growth='0.15 + pow(Particles.NormalizedAge, 0.7)*0.85'), 0.75),
    flash('Flash', 'M_TD_VFX_Glow', 280, 0.13, 0.85),
    opacity(shed('Sparks', 'M_TD_VFX_Spark', 0, (0.2, 0.55), (1.6, 3.4), (350, 1400), k=1.8, radius=30.0, drag=2.4, stretch=12, cone=((0.0, 0.0, 1.0), 82.0), burst=46, dur=0.7), 0.75),
    shed('Embers', 'M_TD_VFX_BladeEmber', 0, (0.5, 1.0), (1.2, 2.6), (150, 600), k=1.5, radius=40.0, drag=1.8, curl=300.0, cone=((0.0, 0.0, 1.0), 70.0), burst=20, dur=1.1),
    opacity(spin(shed('Dust', 'M_TD_VFX_Smoke', 0, (0.6, 1.1), (60, 110), (200, 520), radius=60.0, drag=2.6, rgb=(0.45, 0.50, 0.55), alpha=0.5, cone=((0.0, 0.0, 1.0), 88.0),
                      burst=16, dur=1.2, power=0.9, growth='0.6 + Particles.NormalizedAge*1.4', sort=True)), 0.75),
    chunks('Chunks', 'SM_TD_VFX_Shard_A', 7, (0.5, 0.9), (0.06, 0.12), (300, 750), drag=0.8, radius=50.0, cone=((0.0, 0.0, 1.0), 55.0), dur=1.2, spin=(1.0, 4.0)),
    light('Light', 520, 0.25, k=4.5),
])
for _n, _u, _ems in SYSTEMS_B:   # (the loop above ran before these were added)
    for _e in _ems:
        if _e['name'] == 'Light' and _e.get('rclass', '').endswith('LightRendererProperties') and _n != 'NS_TD_Blade_Aura':
            _e['set'] = [x for x in _e['set'] if x[2] != 'Color'] + [(PS, INIT, 'Color', X('float4(lerp(User.Color.rgb, float3(1,1,1), 0.35)*5.0, 1.0)'))]

fxb = tdvfx.Builder(call_tool)
for _name, _user, _emitters in SYSTEMS_B:
    await fxb.system(_name, _emitters, user=_user)
    await asyncio.sleep(0)
fxb.report()

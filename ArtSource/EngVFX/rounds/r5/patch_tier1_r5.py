# One-off: the round 5 edits to build_tier1.py (kept as a record; already applied). Run from the repo root.
p = 'ArtSource/EngVFX/scripts/build_tier1.py'
s = open(p, encoding='utf-8').read()


def rep(a, b):
    global s
    assert s.count(a) == 1, (s.count(a), a)
    s = s.replace(a, b)


# ---- tracer: dark body + white needle core
rep("""    return [mesh_single('Bolt', 'SM_TD_VFX_Bolt', 'M_TD_VFX_Bolt', '%s, %s, %s' % (L, W, W), 1.5 * hot, flick, offset=(-length * 0.30, 0, 0),
                        live_scale='float3(1,1,1)*' + DIST),
            white,""", """    # Round 5: a dark translucent body under the bolt (drawn first) so the shot reads against the bright magenta nebula,
    # and a thin white-hot needle along the axis (squad shots only; the enemy's stays a ball).
    shadow = mesh_single('Shadow', 'SM_TD_VFX_Bolt', 'M_TD_VFX_BoltShadow', '%s, %s, %s' % (L * 1.12, W * 2.1, W * 2.1), 1.0, None,
                         offset=(-length * 0.30, 0, 0), live_scale='float3(1,1,1)*' + DIST)
    shadow['rend']['SortOrderHint'] = -10
    core = []
    if needle_core:
        nd = needle('Needle', width * 0.34, length * 0.80, 1.0, flick * 0.9, back=-length * 0.30)
        nd['set'] = [x for x in nd['set'] if x[2] != 'Color'] + [(PS, INIT, 'Color', X('float4(lerp(User.Color.rgb, float3(1,1,1), 0.85)*2.6, 1.0)')),
                                                                 (PU, 'ScaleSpriteSize', 'Scale Sprite Size Mode', E('Uniform')), (PU, 'ScaleSpriteSize', 'Uniform Scale Factor', X(DIST))]
        nd['add'].append((PU, 'Update/Size/ScaleSpriteSize'))
        core = [nd]
    return [shadow,
            mesh_single('Bolt', 'SM_TD_VFX_Bolt', 'M_TD_VFX_Bolt', '%s, %s, %s' % (L, W, W), 1.5 * hot, flick, offset=(-length * 0.30, 0, 0),
                        live_scale='float3(1,1,1)*' + DIST),
            white] + core + [""")
rep("def tracer(length, width, sheath=3.2, rib_life=0.16, spark_rate=40.0, flick=71.3, extra=(), hot=1.0):",
    "def tracer(length, width, sheath=3.2, rib_life=0.16, spark_rate=40.0, flick=71.3, extra=(), hot=1.0, needle_core=True):")
rep("tracer(230, 125, sheath=3.0, rib_life=0.07, spark_rate=30.0, flick=47.1, hot=0.7))",
    "tracer(230, 125, sheath=3.0, rib_life=0.07, spark_rate=30.0, flick=47.1, hot=0.7, needle_core=False))")
# ---- engines
rep("THR = '(0.35 + 0.65*User.Throttle)'\nLEN = '(0.45 + 0.75*User.Throttle + 0.9*User.Boost)'",
    "THR = '(0.22 + 0.78*User.Throttle)'            # round 5: a stopped ship idles visibly smaller\nLEN = '(0.20 + 1.00*User.Throttle + 0.9*User.Boost)'")
rep("def engine(boost_only=False):",
    "def engine(boost_only=False, kk=1.0):\n    # kk < 1 keeps the flame's own colour (less white-hot core): the Null fleet's exhaust must read red-orange.")
rep("'-4.2, 1.25, 1.25', 0.85, None,", "'-4.2, 1.25, 1.25', 0.85 * kk, None,")
rep("'-2.3, 0.7, 0.7', 1.7, None,", "'-2.3, 0.7, 0.7', 1.7 * kk * kk, None,")
rep("'(110*%s + 60*User.Boost)' % THR, 1.6, 57.9)", "'(110*%s + 60*User.Boost)' % THR, 1.6 * kk * kk, 57.9)")
rep("""    S('NS_TD_Engine_' + _n, U(_c, 1.0, Throttle=0.6, Boost=0.0), engine())""", """    _kk = 0.62 if _n == 'Enemy' else 1.0
    _c = (1.0, 0.10, 0.015, 1.0) if _n == 'Enemy' else _c
    S('NS_TD_Engine_' + _n, U(_c, 1.0, Throttle=0.6, Boost=0.0), engine(kk=_kk))""")
# ---- ship hit cues: now the ram / beam / splash / melee look, +X = back along the hit
i0 = s.index("S('NS_TD_Ship_HullHit'")
i1 = s.index("# --------------------------------------------------------------------------------------------- explosions")
s = s[:i0] + '''# Round 5 (T4, fe7ca8f): a projectile hit plays only the weapon's impact cue. These two now play for rams, beams, splash
# and melee, with +X pointing back along the hit: a strong directional spray, a flash and a few hot bits.
S('NS_TD_Ship_HullHit', U(FIRE), [
    flash('Flash', 'M_TD_VFX_Flare', 380, 0.08, 1.2, growth='(0.5 + Particles.NormalizedAge)*' + NEAR),
    flash('Glow', 'M_TD_VFX_Glow', 520, 0.16, 0.45, growth=NEAR),
    shed('Spray', 'M_TD_VFX_Spark', 0, (0.22, 0.60), (6, 13), (2200, 7500), k=1.7, radius=14.0, drag=2.0, stretch=16,
         cone=((1.0, 0.0, 0.0), 32.0), burst=34, dur=0.7),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.20, 0.50), (5, 10), (900, 3600), k=1.5, radius=14.0, drag=2.2, stretch=11,
         cone=((1.0, 0.0, 0.0), 85.0), burst=14, dur=0.6),
    shed('Flecks', 'M_TD_VFX_Glow', 0, (0.35, 0.9), (9, 20), (150, 1600), k=1.0, radius=16.0, drag=1.6, curl=700.0,
         cone=((1.0, 0.0, 0.0), 70.0), burst=12, dur=1.0, power=2.0),
    chunks('Bits', 'SM_TD_VFX_Shard_B', 5, (0.5, 1.0), (0.10, 0.26), (600, 2400), drag=0.6, radius=10.0, cone=((1.0, 0.0, 0.0), 50.0), dur=1.1, spin=(1.5, 5.0))])
S('NS_TD_Ship_ShieldHit', U(SHIELD_HIT), [
    mesh_flash('Ring', 'SM_TD_VFX_Disc', 'M_TD_VFX_Shock', '1.0, 6.0, 6.0', 0.32, 0.55, growth='float3(1,1,1)*(0.16 + pow(Particles.NormalizedAge, 0.6)*0.84)*' + NEAR, power=1.3),
    flash('Glow', 'M_TD_VFX_Glow', 240, 0.14, 0.35, growth=NEAR),
    shed('Spray', 'M_TD_VFX_Spark', 0, (0.14, 0.34), (4, 9), (1400, 4800), k=1.3, radius=8.0, drag=2.8, stretch=12,
         cone=((1.0, 0.0, 0.0), 45.0), burst=16, dur=0.5),
    shed('Motes', 'M_TD_VFX_Glow', 0, (0.25, 0.6), (7, 15), (250, 1300), k=1.0, radius=20.0, drag=2.5, curl=600.0,
         cone=((1.0, 0.0, 0.0), 80.0), burst=12, dur=0.7, power=2.0)])

''' + s[i1:]
# ---- explosions
i0 = s.index("def explosion(tier):")
i1 = s.index("S('NS_TD_Explosion_Small'")
s = s[:i0] + '''def spark_trails(count, speed, life=0.9, size=(26, 44), trail_life=0.38, bounds=26000):
    """Round 5: real spark trails. 'Leaders' are GPU sparks flying out; 'Followers' are spawned along every leader's path
    (attribute reader, emitters taken from the engine's AttributeReaderTrails template) and shrink away behind it."""
    fb = {'SimTarget': 'GPUComputeSim', 'CalculateBoundsMode': 'Fixed',
          'FixedBounds': {'min': {'x': -bounds, 'y': -bounds, 'z': -bounds}, 'max': {'x': bounds, 'y': bounds, 'z': bounds}, 'isValid': True}}
    lead = {'name': 'Leaders', 'base': 'leaders', 'props': dict(fb), 'remove': [(PU, 'Collision')], 'mat': 'M_TD_VFX_Glow',
            'set': [(EU, 'EmitterState', 'Life Cycle Mode', E('Self')), (EU, 'EmitterState', 'Loop Behavior', E('Once')), (EU, 'EmitterState', 'Loop Duration', float(life + 0.2)),
                    (EU, 'SpawnBurst_Instantaneous', 'Spawn Count', int(count)),
                    (PS, INIT, 'Lifetime', float(life)), (PS, INIT, 'Color Mode', E('Direct Set')), (PS, INIT, 'Color', col(2.2)),
                    (PS, INIT, 'Uniform Sprite Size Min', X('%s*%s' % (size[0], SCL))), (PS, INIT, 'Uniform Sprite Size Max', X('%s*%s' % (size[1], SCL))),
                    (PS, 'AddVelocity', 'Velocity Mode', E('From Point')),
                    (PS, 'AddVelocity', ['Velocity Speed', 'Minimum'], X('%s*%s' % (speed[0], SCL))), (PS, 'AddVelocity', ['Velocity Speed', 'Maximum'], X('%s*%s' % (speed[1], SCL))),
                    (PU, 'GravityForce', 'Gravity', (0.0, 0.0, 0.0)), (PU, 'Drag', 'Drag', 1.7)]}
    follow = {'name': 'Followers', 'base': 'followers', 'props': dict(fb), 'mat': 'M_TD_VFX_Glow',
              'set': [(EU, 'EmitterState', 'Life Cycle Mode', E('Self')), (EU, 'EmitterState', 'Loop Behavior', E('Once')), (EU, 'EmitterState', 'Loop Duration', float(life + 0.2)),
                      (EU, 'SpawnParticlesFromOtherEmitter', 'Spawn Rate', 260.0),
                      (PS, INIT, 'Lifetime', float(trail_life)), (PS, INIT, 'Color Mode', E('Direct Set')), (PS, INIT, 'Color', col(1.3)),
                      (PS, INIT, 'Sprite Size Mode', E('Uniform')), (PS, INIT, 'Uniform Sprite Size', X('%s*%s' % (size[0] * 0.55, SCL))),
                      (PU, 'ScaleSpriteSize', 'Scale Factor', X('float2(1.0, 1.0)*pow(1.0 - Particles.NormalizedAge, 1.5)'))]}
    return [lead, follow]


def explosion(tier):
    # Round 5: the body of the blast is procedural (layers of noise-eroded black-body fire sprites that expand, curl and
    # cool, M_TD_VFX_Fireball), with Grok Bot's flipbook kept as ONE dim layer underneath for large-scale shape. Real
    # spark trails, more tumbling mesh debris with cooling edges (three shard meshes), rim-lit smoke, no painted scrap.
    n = {1: 1.0, 2: 1.8, 3: 2.6}[tier]
    e = [flash('Flash', 'M_TD_VFX_Glow', 1500, 0.09, 0.9, growth='0.5 + Particles.NormalizedAge*0.7', power=1.2),
         flash('Star', 'M_TD_VFX_Flare', 1700, 0.10, 0.7, growth='0.5 + Particles.NormalizedAge'),
         light('Light', 4000, 0.30),
         spin(shed('Blast', 'M_TD_VFX_FlipExplosion', 0, (1.0, 1.25), (1500, 1900), (0, 60), radius=40.0 * tier, drag=3.0, rgb=(0.42, 0.42, 0.42),
                   burst=1, dur=1.4, power=0.35, growth='0.62 + pow(Particles.NormalizedAge, 0.5)*0.55')),
         spin(shed('Core', 'M_TD_VFX_Fireball', 0, (0.28, 0.45), (460, 680), (40, 260), radius=60.0, drag=3.0, rgb=(1.7, 1.7, 1.7),
                   burst=int(3 * n), dur=0.6, power=0.6, growth='0.55 + Particles.NormalizedAge*0.8')),
         spin(shed('Fireball', 'M_TD_VFX_Fireball', 0, (0.60, 1.25), (340, 620), (500, 2100), radius=170.0, drag=2.8, curl=1100.0, rgb=(1.0, 1.0, 1.0),
                   burst=int(13 * n), dur=1.6, power=0.7, growth='0.45 + pow(Particles.NormalizedAge, 0.45)*1.25')),
         spin(shed('Licks', 'M_TD_VFX_Fireball', 0, (0.35, 0.80), (150, 300), (1500, 4200), radius=120.0, drag=3.4, curl=1600.0, rgb=(1.1, 1.1, 1.1),
                   burst=int(12 * n), dur=1.1, power=0.8, growth='0.6 + pow(Particles.NormalizedAge, 0.5)*1.0')),
         flash('Shockwave', 'M_TD_VFX_FlipRing', 5200, 0.34, 1.0, growth='0.06 + pow(Particles.NormalizedAge, 0.5)*0.95', power=1.4),
         shed('Sparks', 'M_TD_VFX_Spark', 0, (0.45, 1.20), (9, 20), (2600, 10500), k=1.9, radius=60.0, drag=2.2, stretch=26,
              burst=int(26 * n), dur=1.3, power=1.4),
         chunks('Chunks', 'SM_TD_VFX_Shard_A', int(4 * n), (1.2, 2.2), (0.35, 0.9), (700, 3000), radius=80.0),
         chunks('Plates', 'SM_TD_VFX_Shard_B', int(4 * n), (1.3, 2.4), (0.30, 0.8), (900, 3600), radius=80.0, spin=(1.0, 4.0)),
         spin(shed('SmokeRim', 'M_TD_VFX_SmokeRim', 0, (1.2, 2.0), (420, 640), (250, 1100), radius=200.0, drag=2.0, curl=500.0, rgb=(0.34, 0.30, 0.28), alpha=0.8,
                   burst=int(4 * n), dur=2.2, power=0.7, fade_in=6.0, growth='0.6 + pow(Particles.NormalizedAge, 0.6)*1.2', sort=True)),
         spin(shed('Smoke', 'M_TD_VFX_FlipSmoke', 0, (2.2, 3.6), (520, 760), (150, 700), radius=180.0, drag=1.5, curl=300.0, rgb=(0.52, 0.45, 0.40), alpha=0.85,
                   burst=int(4 * n), dur=3.8, power=0.6, growth='0.6 + pow(Particles.NormalizedAge, 0.6)*1.3', sort=True)),
         shed('Embers', 'M_TD_VFX_Glow', 0, (0.9, 1.8), (12, 30), (150, 1700), k=1.2, radius=150.0, drag=1.3, curl=1300.0,
              burst=int(24 * n), dur=2.0, power=2.0)]
    e += spark_trails(int(9 * n), (2600, 9000))
    if tier >= 2:
        e += [chunks('Lumps', 'SM_TD_VFX_Shard_C', int(2 * n), (1.4, 2.6), (0.4, 1.1), (400, 1800), radius=100.0)]
    if tier >= 3:
        e += [pops()]
    return e


''' + s[i1:]
rep("spin(shed('Blast', 'M_TD_VFX_FlipExplosion', 0, (1.0, 1.3), (1500, 1900), (0, 60), radius=60.0, drag=3.0, rgb=(1.3, 1.3, 1.3),",
    "spin(shed('Blast', 'M_TD_VFX_FlipExplosion', 0, (1.0, 1.3), (1500, 1900), (0, 60), radius=60.0, drag=3.0, rgb=(0.6, 0.6, 0.6),")
rep("""         shed('Fireball', 'M_TD_VFX_Fireball', 0, (0.55, 1.1), (300, 520), (700, 2300), radius=140.0, drag=3.0, rgb=(1.0, 1.0, 1.0),
              burst=10 if final else 5, dur=1.5, power=0.7, growth='0.5 + pow(Particles.NormalizedAge, 0.45)*1.15'),""",
    """         spin(shed('Fireball', 'M_TD_VFX_Fireball', 0, (0.60, 1.25), (340, 620), (500, 2100), radius=170.0, drag=2.8, rgb=(1.0, 1.0, 1.0),
                   burst=18 if final else 10, dur=1.6, power=0.7, growth='0.45 + pow(Particles.NormalizedAge, 0.45)*1.25')),""")
rep("(650, 1000), (0, 80), radius=560.0, drag=2.0, rgb=(1.3, 1.3, 1.3),", "(650, 1000), (0, 80), radius=560.0, drag=2.0, rgb=(0.9, 0.9, 0.9),")
rep("(EU, 'EmitterState', 'Loop Duration', 1.5)]   # round 4: staggered over 1.5 s", "(EU, 'EmitterState', 'Loop Duration', 2.4)]   # round 5: staggered over 2.4 s")
open(p, 'w', encoding='utf-8').write(s)
print('patched')

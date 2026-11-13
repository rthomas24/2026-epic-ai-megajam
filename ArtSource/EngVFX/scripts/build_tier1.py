# Triad Drift ENG-37, tier 1 (round 3): tracers, muzzle flashes, engine exhaust, impacts, explosions.
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
SHIELD_HIT = (0.22, 0.62, 1.0, 1.0)   # pale cyan-blue (the ring material adds white on top)
# round 5: sprites and world-space particles do not follow the component's scale, so a cue's VFXScale did nothing to
# them. SCL folds the owner's scale in. Local-space mesh particles already follow the component: they keep User.Scale.
SCL = '(User.Scale*Engine.Owner.Scale.x)'
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
    # Inactive Response = Kill: a pooled projectile only Deactivates its tracer when it is released, and a looping
    # particle would otherwise hang in space where the shot ended until its loop ran out (seen in play, round 4).
    sets = [(EU, 'EmitterState', 'Life Cycle Mode', E('Self')), (EU, 'EmitterState', 'Loop Behavior', E('Infinite')), (EU, 'EmitterState', 'Inactive Response', E('Kill')), (PS, INIT, 'Color Mode', E('Direct Set')), (PS, INIT, 'Color', col(k))]
    if isinstance(size, tuple):
        sets += [(PS, INIT, 'Sprite Size Mode', E('Non-Uniform')), (PS, INIT, 'Sprite Size', X('float2(%s, %s)*' % size + SCL))]
    else:
        sets += [(PS, INIT, 'Sprite Size Mode', E('Uniform')), (PS, INIT, 'Uniform Sprite Size', X('%s*%s' % (size, SCL)))]
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
                    (PS, INIT, 'Ribbon Width Mode', E('Direct Set')), (PS, INIT, 'Ribbon Width', X('%s*%s' % (width, SCL))),
                    (PU, 'ScaleRibbonWidth', 'Ribbon Width Scale', X('1.0 - Particles.NormalizedAge*%s' % taper))] + fade_alpha(power),
            'mat': mat, 'rend': {'UV0Settings': {'distributionMode': 'TiledOverRibbonLength', 'tilingLength': tile}}}


def shed(name, mat, rate, life, size, speed, k=1.0, radius=10.0, drag=2.0, curl=0.0, local=False, aligned=False,
         power=1.5, growth=None, cone=None, burst=None, dur=1.0, rate_expr=None, gpu=False, life_mul=None, sort=False, alpha=1.0, fade_in=None, stretch=None, rgb=None):
    """Many small particles: sparks, wisps, puffs, embers. rate (continuous) or burst (count, one shot).
    cone = (axis xyz, angle) shoots them in a cone (emitter local space); otherwise outwards from the centre."""
    base = 'burst' if burst else 'rate'
    PSC = 'User.Scale' if local else SCL   # positions and speeds of a local-space emitter already follow the component
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
                 (PS, INIT, 'Sprite Size Min', X('float2(%s, %s)*' % (size[0], size[0] * stretch) + SCL)),
                 (PS, INIT, 'Sprite Size Max', X('float2(%s, %s)*' % (size[1], size[1] * stretch) + SCL))]
    else:
        sets += [(PS, INIT, 'Uniform Sprite Size Min', X('%s*%s' % (size[0], SCL))), (PS, INIT, 'Uniform Sprite Size Max', X('%s*%s' % (size[1], SCL)))]
    sets += [(PS, 'ShapeLocation', 'Sphere Radius', X('%s*%s' % (radius, PSC)))]
    spd = X('lerp(%s, %s, Particles.MaterialRandom)*' % speed + PSC)
    if cone:
        sets += [(PS, 'AddVelocity', 'Velocity Mode', E('In Cone')), (PS, 'AddVelocity', 'Velocity Speed', spd),
                 (PS, 'AddVelocity', 'Cone Axis', cone[0]), (PS, 'AddVelocity', 'Cone Angle', float(cone[1]))]
    else:
        sets += [(PS, 'AddVelocity', 'Velocity Mode', E('From Point')), (PS, 'AddVelocity', 'Velocity Speed', spd)]
    sets += [(PU, 'GravityForce', 'Gravity', (0.0, 0.0, 0.0)), (PU, 'Drag', 'Drag', float(drag))] + fade_alpha(power, 'saturate(Particles.NormalizedAge*%s)' % fade_in if fade_in else '1.0')
    if curl:
        add.append((PU, 'Update/Forces/CurlNoiseForce'))
        sets += [(PU, 'CurlNoiseForce', 'Noise Strength', X('%s*%s' % (curl, PSC))), (PU, 'CurlNoiseForce', 'Noise Frequency', 0.004)]
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


VFXROOT = '/Game/TriadDrift/Gameplay/VFX/'
WHITE = (1.0, 1.0, 1.0, 1.0)


def ref(path):
    return {'refPath': path + '.' + path.split('/')[-1]}


def meshed(e, mesh, mat):
    """Turn an emitter spec into a mesh-particle emitter (mesh and material from the VFX folder)."""
    e['rclass'] = '/Script/Niagara.NiagaraMeshRendererProperties'
    e['rend_pre'] = {'bOverrideMaterials': True}
    e['rend'] = {'Meshes': [{'mesh': ref(VFXROOT + 'Meshes/' + mesh), 'scale': {'x': 1, 'y': 1, 'z': 1}}],
                 'OverrideMaterials': [{'explicitMat': ref(VFXROOT + 'Materials/' + mat)}], 'bCastShadows': False}
    e.pop('mat', None)
    e['set'] = [x for x in e['set'] if 'Sprite' not in x[2]]
    return e


def mesh_single(name, mesh, mat, scale, k=1.0, flicker=None, offset=None, live_scale=None):
    """One persistent mesh particle in emitter-local space. scale = 'x, y, z' expression in units of the 100 cm mesh
    (User.Scale applied). live_scale = float3 expression evaluated every frame (reads User.* live)."""
    e = single(name, mat, 1, k, flicker, offset=offset)
    meshed(e, mesh, mat)
    e['set'] += [(PS, INIT, 'Mesh Scale Mode', E('Non-Uniform')), (PS, INIT, 'Mesh Scale', X('float3(%s)*User.Scale' % scale))]
    if live_scale:
        e['add'].append((PU, 'Update/Size/ScaleMeshSize'))
        e['set'].append((PU, 'ScaleMeshSize', 'Scale Factor', X(live_scale)))
    return e


def mesh_flash(name, mesh, mat, scale, life, k=1.0, growth=None, power=1.0, delay=0.0, rgb=None):
    """One short-lived mesh particle in emitter-local space (so +X of the system is +X of the mesh)."""
    e = flash(name, mat, 1, life, k, power=power, delay=delay)
    e['props'] = {'bLocalSpace': True}
    meshed(e, mesh, mat)
    e['remove'] = [(PU, 'ScaleSpriteSize')]
    e['set'] = [x for x in e['set'] if x[1] not in ('ScaleSpriteSize', 'ShapeLocation', 'AddVelocity')]
    e['set'] += [(PS, 'ShapeLocation', 'Sphere Radius', 0.0), (PS, 'AddVelocity', 'Velocity Speed', 0.0),
                 (PS, INIT, 'Mesh Scale Mode', E('Non-Uniform')), (PS, INIT, 'Mesh Scale', X('float3(%s)*User.Scale' % scale))]
    if rgb:
        e['set'].append((PS, INIT, 'Color', X('float4(%s, %s, %s, 1.0)' % rgb)))
    if growth:
        e['add'] = e.get('add', []) + [(PU, 'Update/Size/ScaleMeshSize')]
        e['set'].append((PU, 'ScaleMeshSize', 'Scale Factor', X(growth)))
    return e


def chunks(name, mesh, count, life, size, speed, drag=0.5, radius=60.0, cone=None, dur=3.0, spin=(0.5, 2.5)):
    """Tumbling debris meshes: glowing edges that cool (M_TD_VFX_Debris), shrinking away at the end of their life."""
    e = shed(name, 'M_TD_VFX_Debris', 0, life, (1, 1), speed, k=1.0, radius=radius, drag=drag, burst=count, dur=dur, cone=cone, power=0.01, rgb=(1.0, 1.0, 1.0))
    meshed(e, mesh, 'M_TD_VFX_Debris')
    e['remove'] = [(PU, 'ScaleSpriteSize')]
    e['add'] = [(PS, 'Spawn/Orientation/V3/InitialMeshOrientation'), (PU, 'Update/Orientation/V3/UpdateMeshOrientation'), (PU, 'Update/Size/ScaleMeshSize')]
    e['keep_solver_order'] = True
    e['set'] += [(PS, INIT, 'Mesh Scale Mode', E('Non-Uniform')),
                 (PS, INIT, 'Mesh Scale', X('float3(1.0, 0.6 + 0.8*frac(Particles.MaterialRandom*7.13), 0.5 + 0.9*frac(Particles.MaterialRandom*3.71))*lerp(%s, %s, frac(Particles.MaterialRandom*5.77))*' % size + SCL)),
                 (PS, 'InitialMeshOrientation', 'Use Rotation Vector', True),
                 (PS, 'InitialMeshOrientation', 'Rotation', X('float3(frac(Particles.MaterialRandom*11.3), frac(Particles.MaterialRandom*17.7), frac(Particles.MaterialRandom*23.9))')),
                 (PU, 'UpdateMeshOrientation', 'Rotation Vector', X('normalize(float3(frac(Particles.MaterialRandom*3.3)-0.5, frac(Particles.MaterialRandom*9.1)-0.5, frac(Particles.MaterialRandom*13.7)-0.5))*lerp(%s, %s, Particles.MaterialRandom)' % spin)),
                 (PU, 'ScaleMeshSize', 'Scale Factor', X('float3(1,1,1)*saturate((1.0 - Particles.NormalizedAge)*6.0)'))]
    return e


def light(name, radius, life, k=6.0, rgb=(1.0, 0.6, 0.25), delay=0.0):
    """A brief point light (particle light renderer)."""
    e = flash(name, 'M_TD_VFX_Glow', 1, life, 1.0, power=2.0, delay=delay)
    e['rclass'] = '/Script/Niagara.NiagaraLightRendererProperties'
    e['rend'] = {'RadiusScale': radius / 100.0, 'bAlphaScalesBrightness': True, 'bAffectsTranslucency': False}
    e.pop('mat', None)
    e['set'].append((PS, INIT, 'Color', X('float4(%s, %s, %s, 1.0)' % tuple(c * k for c in rgb))))
    return e


def trail(name, width, life, k=1.0, rate=180.0, taper=0.9, power=1.6):
    """A short clean ribbon: no tiled noise (no seam), fades and narrows towards the tail."""
    e = ribbon(name, width, life, k, rate, mat='M_TD_VFX_Ribbon_Trail', taper=taper, power=power)
    e['rend'] = {'UV0Settings': {'distributionMode': 'ScaledUniformly'}}
    return e


def spin(e):
    """Random sprite rotation (flipbook puffs must not all sit the same way up)."""
    e['set'] += [(PS, INIT, 'Sprite Rotation Mode', E('Random'))]
    return e


def offset(e, x, y=0.0, z=0.0):
    e['set'] += [(PS, INIT, 'UsePositionOffset', True), (PS, INIT, 'Position Offset', X('float3(%s,%s,%s)*User.Scale' % (x, y, z)))]
    return e


SYSTEMS = []


def S(name, user, emitters):
    if ONLY and not any(name.startswith(o) for o in ONLY):
        return
    SYSTEMS.append((name, user, emitters))


def U(color, scale=1.0, **kw):
    d = {'Color': color, 'Scale': float(scale)}
    d.update(kw)
    return d


# --------------------------------------------------------------------------------------------- tracers
# The player sees his own shots from BEHIND (chase camera), so a tracer is built from things that have volume or face
# the camera: a mesh bolt with a white-hot centre, a small white core sprite, a coloured sheath glow, then the ribbon
# and the shed particles, which only show from the side.
# A shot keeps a readable size on screen: the bolt grows with its distance from the camera (Engine.Owner.LODDistance,
# the component's distance to the nearest view) and shrinks when it passes close by, like the placeholder streak did.
DIST = 'clamp(Engine.Owner.LODDistance/2600.0, 0.35, 9.0)'
SPRITE_SCALE = True   # also grow the two glow sprites with distance


def glow_single(name, size, k, flick):
    e = single(name, 'M_TD_VFX_Glow', size, k, flick)
    if SPRITE_SCALE:
        e['add'].append((PU, 'Update/Size/ScaleSpriteSize'))
        e['set'] += grow(DIST)
    return e


def tracer(length, width, sheath=3.2, rib_life=0.16, spark_rate=40.0, flick=71.3, extra=(), hot=1.0, needle_core=True):
    # Round 4: the shot must be the most readable thing on screen after the reticle. A white-hot core (the bolt mesh
    # driven over 1 so its centre clips to white, plus a white core sprite), a saturated sheath in the squad colour, a
    # short fading trail and a small additive glow.
    L, W = length / 100.0, width / 100.0
    white = single('White', 'M_TD_VFX_Glow', width * 1.15, 1.0, flick * 1.31)
    white['set'] = [x for x in white['set'] if x[2] != 'Color'] + [(PS, INIT, 'Color', X('float4(lerp(User.Color.rgb, float3(1,1,1), 0.75)*%s, 1.0)' % (2.2 * hot))),
                                                                    (PU, 'ScaleSpriteSize', 'Scale Sprite Size Mode', E('Uniform')), (PU, 'ScaleSpriteSize', 'Uniform Scale Factor', X(DIST))]
    white['add'].append((PU, 'Update/Size/ScaleSpriteSize'))
    # Round 5: a dark translucent body under the bolt (drawn first) so the shot reads against the bright magenta nebula,
    # and a thin white-hot needle along the axis (squad shots only; the enemy's stays a ball).
    shadow = mesh_single('Shadow', 'SM_TD_VFX_Bolt', 'M_TD_VFX_BoltShadow', '%s, %s, %s' % (L * 1.06, W * 1.45, W * 1.45), 1.0, None,   # 2.1 wide read as a dark bubble end-on (seen in play)
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
            white] + core + [
            glow_single('Sheath', width * sheath, 0.55, flick * 0.77),
            trail('Trail', width * 0.95, rib_life, 1.1),
            shed('Sparks', 'M_TD_VFX_Glow', spark_rate, (0.15, 0.40), (width * 0.12, width * 0.30), (40, 300), k=1.2, radius=width * 0.3, drag=1.5, curl=900.0)] + list(extra)


# Rift: a fat plasma bolt shedding plasma wisps.
S('NS_TD_Tracer_Rift', U(RIFT), tracer(620, 85, sheath=3.6, spark_rate=34.0, flick=37.7, extra=[
    shed('Wisps', 'M_TD_VFX_Fire', 60.0, (0.14, 0.28), (60, 120), (20, 160), k=0.9, radius=24.0, drag=1.0, curl=1600.0, growth='1.0 + Particles.NormalizedAge*0.8')]))
# Lancer: a tight fast needle.
S('NS_TD_Tracer_Lancer', U(LANCER), tracer(820, 60, sheath=3.8, rib_life=0.18, spark_rate=24.0, flick=113.1))
# Bastion: a heavy shell with a motor flame and a smoke-and-fire trail.
S('NS_TD_Tracer_Bastion', U(BASTION), tracer(420, 115, sheath=3.2, rib_life=0.10, spark_rate=40.0, flick=53.9, extra=[
    mesh_single('Motor', 'SM_TD_VFX_Flame', 'M_TD_VFX_Flame', '-3.2, 1.3, 1.3', 1.1, 61.7, offset=(-190, 0, 0)),
    shed('FireTrail', 'M_TD_VFX_Fireball', 70.0, (0.18, 0.34), (90, 150), (10, 120), radius=12.0, drag=1.0, curl=600.0, rgb=(0.9, 0.9, 0.9),
         growth='0.8 + Particles.NormalizedAge*0.9'),
    spin(shed('Smoke', 'M_TD_VFX_FlipSmoke', 40.0, (0.5, 0.9), (130, 200), (10, 90), radius=12.0, drag=1.0, curl=500.0, power=0.8, rgb=(0.32, 0.28, 0.27), alpha=0.6,
              growth='0.8 + Particles.NormalizedAge*1.6', sort=True))]))
# Enemy: a red-orange ball of plasma, short and round where the squad's shots are long needles; dimmer core.
S('NS_TD_Tracer_Enemy', U(ENEMY), tracer(230, 125, sheath=3.0, rib_life=0.07, spark_rate=30.0, flick=47.1, hot=0.7, needle_core=False))

# --------------------------------------------------------------------------------------------- muzzle flashes
# Directional: a short flame cone along +X, a spiked star seen down the barrel, a few forward sparks. 2-3 frames.
def muzzle(size, sparks):
    k = size / 100.0
    return [mesh_flash('Cone', 'SM_TD_VFX_Flame', 'M_TD_VFX_Flame', '%s, %s, %s' % (k * 2.6, k * 0.9, k * 0.9), 0.06, 1.5,
                       growth='float3(0.6 + Particles.NormalizedAge*0.7, 1.15 - Particles.NormalizedAge*0.5, 1.15 - Particles.NormalizedAge*0.5)', power=0.7),
            mesh_flash('Star', 'SM_TD_VFX_Disc', 'M_TD_VFX_Flare', '1.0, %s, %s' % (k * 2.2, k * 2.2), 0.05, 1.3,
                       growth='float3(1,1,1)*(0.7 + Particles.NormalizedAge*0.6)'),
            flash('Glow', 'M_TD_VFX_Glow', size * 1.3, 0.06, 0.5),
            shed('Sparks', 'M_TD_VFX_Spark', 0, (0.08, 0.20), (6, 11), (2200, 6000), k=1.4, radius=6.0, drag=3.0,
                 stretch=10, cone=((1.0, 0.0, 0.0), 14.0), burst=sparks, dur=0.3)]


S('NS_TD_Muzzle_Rift', U(RIFT), muzzle(150, 8))
S('NS_TD_Muzzle_Lancer', U(LANCER), muzzle(110, 6))
S('NS_TD_Muzzle_Bastion', U(BASTION), muzzle(200, 12))
S('NS_TD_Muzzle_Enemy', U(ENEMY), muzzle(140, 6))

# --------------------------------------------------------------------------------------------- engines
# Attached at the nozzle, blowing along -X. User.Throttle (0..1, default 0.6) and User.Boost (0 or 1) are read every
# frame: the flame grows longer, wider and brighter; Boost adds a long second flame with shock diamonds and sparks.
THR = '(0.22 + 0.78*User.Throttle)'            # round 5: a stopped ship idles visibly smaller
LEN = '(0.20 + 1.00*User.Throttle + 0.9*User.Boost)'
FLK = '(0.92 + 0.16*frac(sin(Engine.Time*%s)*43758.5))'


def engine(boost_only=False, kk=1.0, lm=1.0, wm=1.0):
    # lm / wm: length and width of the flames (round 7: per-type enemy exhausts).
    # kk < 1 keeps the flame's own colour (less white-hot core): the Null fleet's exhaust must read red-orange.
    outer = mesh_single('Flame', 'SM_TD_VFX_Flame', 'M_TD_VFX_Flame', '%s, %s, %s' % (-4.2 * lm, 1.25 * wm, 1.25 * wm), 0.85 * kk, None,
                        live_scale='float3(%s*%s, %s, %s)' % (LEN, FLK % 61.3, THR, THR))
    inner = mesh_single('Inner', 'SM_TD_VFX_Flame', 'M_TD_VFX_Flame', '%s, %s, %s' % (-2.3 * lm, 0.7 * wm, 0.7 * wm), 1.7 * kk * kk, None,
                        live_scale='float3(%s*%s, %s, %s)' % (LEN, FLK % 97.1, THR, THR))
    boost = mesh_single('BoostFlame', 'SM_TD_VFX_Flame', 'M_TD_VFX_Flame_Boost', '-9.5, 1.0, 1.0', 1.3, None,
                        live_scale='float3(1,1,1)*User.Boost*%s' % (FLK % 43.7))
    glow = single('Glow', 'M_TD_VFX_Glow', '(260*%s + 160*User.Boost)' % THR, 0.6, 83.3)
    hot = single('Hot', 'M_TD_VFX_Glow', '(110*%s + 60*User.Boost)' % THR, 1.6 * kk * kk, 57.9)
    wisps = shed('Wisps', 'M_TD_VFX_Fire', 0, (0.10, 0.22), (60, 110), ('1200*%s' % LEN, '2600*%s' % LEN), k=0.5, radius=18.0, drag=1.2,
                 curl=2200.0, local=True, cone=((-1.0, 0.0, 0.0), 6.0), growth='1.0 - Particles.NormalizedAge*0.5',
                 rate_expr='90.0*(0.3 + 0.7*User.Throttle + 0.6*User.Boost)')
    embers = shed('Embers', 'M_TD_VFX_Glow', 0, (0.25, 0.6), (8, 20), ('900*%s' % LEN, '2400*%s' % LEN), k=1.3, radius=30.0, drag=0.8, curl=1500.0,
                  local=True, cone=((-1.0, 0.0, 0.0), 9.0), power=2.0, rate_expr='40.0*(0.2 + 0.8*User.Throttle + User.Boost)')
    after = shed('Afterburn', 'M_TD_VFX_Spark', 0, (0.12, 0.30), (6, 11), (3000, 6000), k=1.5, radius=26.0, drag=1.0, local=True, stretch=11,
                 cone=((-1.0, 0.0, 0.0), 10.0), rate_expr='120.0*User.Boost')
    if boost_only:
        return [boost, after]
    return [outer, inner, boost, glow, hot, wisps, embers, after]


for _n, _c in (('Rift', RIFT), ('Lancer', LANCER), ('Bastion', BASTION), ('Enemy', ENEMY)):
    _kk = 0.62 if _n == 'Enemy' else 1.0
    _c = (1.0, 0.10, 0.015, 1.0) if _n == 'Enemy' else _c
    S('NS_TD_Engine_' + _n, U(_c, 1.0, Throttle=0.6, Boost=0.0), engine(kk=_kk))
    # the afterburner alone, always on: for the BoostLoop cue slot, which is a separate attached loop
    S('NS_TD_EngineBoost_' + _n, U(_c, 1.0, Throttle=1.0, Boost=1.0), engine(True))

# Round 7: the interceptor and the fighter must be told apart by their exhaust alone.
S('NS_TD_Engine_Enemy_Interceptor', U((1.0, 0.80, 0.30, 1.0), 1.0, Throttle=0.6, Boost=0.0), engine(kk=1.0, lm=2.1, wm=0.55))   # long, thin, hot white-yellow
S('NS_TD_Engine_Enemy_Fighter', U((1.0, 0.05, 0.015, 1.0), 1.0, Throttle=0.6, Boost=0.0), engine(kk=0.55, lm=0.55, wm=1.0))    # short, red

# --------------------------------------------------------------------------------------------- impacts
# +X of the system is the surface normal (gameplay passes ImpactNormal.Rotation()).
S('NS_TD_Impact_Hull', U(FIRE), [
    flash('Flash', 'M_TD_VFX_Flare', 300, 0.07, 1.4, growth='0.5 + Particles.NormalizedAge'),
    flash('Glow', 'M_TD_VFX_Glow', 420, 0.14, 0.5),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.18, 0.55), (6, 13), (1200, 5200), k=1.6, radius=10.0, drag=2.2, stretch=11,
         cone=((1.0, 0.0, 0.0), 55.0), burst=22, dur=0.7),
    shed('Flecks', 'M_TD_VFX_Glow', 0, (0.35, 0.8), (10, 24), (200, 1500), k=1.0, radius=14.0, drag=1.6, curl=700.0,
         cone=((1.0, 0.0, 0.0), 80.0), burst=10, dur=1.0, power=2.0),
    chunks('Bits', 'SM_TD_VFX_Shard_A', 4, (0.5, 0.9), (0.10, 0.24), (500, 1900), drag=0.6, radius=10.0, cone=((1.0, 0.0, 0.0), 60.0), dur=1.0, spin=(1.5, 5.0)),
    shed('Scorch', 'M_TD_VFX_Fireball', 0, (0.16, 0.30), (110, 180), (60, 320), radius=20.0, drag=2.0, rgb=(0.8, 0.8, 0.8),
         cone=((1.0, 0.0, 0.0), 70.0), burst=4, dur=0.5, growth='0.7 + Particles.NormalizedAge*0.9'),
    spin(shed('Smoke', 'M_TD_VFX_FlipSmoke', 0, (0.7, 1.2), (150, 240), (60, 260), radius=20.0, drag=1.5, rgb=(0.30, 0.27, 0.27), alpha=0.7,
              cone=((1.0, 0.0, 0.0), 60.0), burst=3, dur=1.3, power=0.8, growth='0.8 + Particles.NormalizedAge*1.3', sort=True))])
# Round 4: SMALL. A ring from 0.4 m to 2.5 m radius in 0.3 s at about half brightness, a hexagon ripple inside it
# (procedural M_TD_VFX_Hex, tinted, no white), a small glow and a few fast sparks. It must read as "a hit on a shield
# here" and never hide the ship, also when eight land in a second.
# NEAR: a hit close to the camera (a wingman beside the chase camera, 17 m behind the ship) shrinks, so it cannot fill the screen.
NEAR = 'clamp(Engine.Owner.LODDistance/1700.0, 0.28, 1.0)'
S('NS_TD_Impact_Shield', U(SHIELD_HIT), [
    mesh_flash('Ring', 'SM_TD_VFX_Disc', 'M_TD_VFX_Shock', '1.0, 5.0, 5.0', 0.30, 0.50, growth='float3(1,1,1)*(0.16 + pow(Particles.NormalizedAge, 0.6)*0.84)*' + NEAR, power=1.3),
    mesh_flash('Hex', 'SM_TD_VFX_Disc', 'M_TD_VFX_Hex', '1.0, 4.2, 4.2', 0.28, 0.20, growth='float3(1,1,1)*(0.45 + Particles.NormalizedAge*0.55)*' + NEAR, power=0.8),
    flash('Glow', 'M_TD_VFX_Glow', 110, 0.10, 0.35, growth=NEAR),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.10, 0.26), (4, 8), (900, 3200), k=1.2, radius=6.0, drag=3.0, stretch=10,
         cone=((1.0, 0.0, 0.0), 75.0), burst=7, dur=0.4)])

# Ship-level hit cues (UTDShipVisuals / UTDEnemyVisuals HullHit, ShieldHit). An enemy ship plays these IN ADDITION to the
# weapon's impact cue at the same point, with no rotation, so they are light and camera-facing: no ring, no flash.
# Round 5 (T4, fe7ca8f): a projectile hit plays only the weapon's impact cue. These two now play for rams, beams, splash
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

# --------------------------------------------------------------------------------------------- explosions
# The blast, the smoke and the main ring use Grok Bot's textures (T_TD_VFX_Grok_*, sources ArtSource/EngAI/VFX); the rest
# is procedural. Life of an explosion: a white flash and a brief light; a white-hot core; fire that cools white -> orange -> deep red as
# it expands and erodes; a fast thin shockwave; streaking sparks and tumbling debris with glowing edges; dark smoke that
# lingers and drifts. tier 1 small, 2 medium (more blasts and debris), 3 large (secondary blasts for 0.6 s).
def pops():
    """Secondary blasts for the large explosion: smaller flipbook blasts going off around the centre for 0.6 s.
    One emitter on purpose: every extra emitter costs gigabytes of editor memory when the system is rebuilt."""
    e = spin(shed('Pops', 'M_TD_VFX_FlipExplosion', 5.0, (0.7, 0.95), (650, 1000), (0, 80), radius=560.0, drag=2.0, rgb=(0.9, 0.9, 0.9),
                  power=0.35, growth='0.6 + pow(Particles.NormalizedAge, 0.5)*0.6'))
    e['set'] += [(EU, 'EmitterState', 'Loop Behavior', E('Once')), (EU, 'EmitterState', 'Loop Duration', 2.4)]   # round 5: staggered over 2.4 s
    return e


def spark_trails(count, speed, life=0.9, size=(15, 26), trail_life=0.45, bounds=26000):
    """Round 5: real spark trails. 'Leaders' are GPU sparks flying out; 'Followers' are spawned along every leader's path
    (attribute reader, emitters taken from the engine's AttributeReaderTrails template) and shrink away behind it."""
    fb = {'SimTarget': 'GPUComputeSim', 'CalculateBoundsMode': 'Fixed',
          'FixedBounds': {'min': {'x': -bounds, 'y': -bounds, 'z': -bounds}, 'max': {'x': bounds, 'y': bounds, 'z': bounds}, 'isValid': True}}
    lead = {'name': 'Leaders', 'base': 'leaders', 'props': dict(fb), 'remove': [(PU, 'Collision')], 'mat': 'M_TD_VFX_Glow',
            'set': [(EU, 'EmitterState', 'Life Cycle Mode', E('Self')), (EU, 'EmitterState', 'Loop Behavior', E('Once')), (EU, 'EmitterState', 'Loop Duration', float(life + 0.2)),
                    (EU, 'SpawnBurst_Instantaneous', 'Spawn Count', int(count)),
                    (PS, INIT, 'Lifetime', float(life)), (PS, INIT, 'Color Mode', E('Direct Set')), (PS, INIT, 'Color', col(2.2)),
                    (PS, INIT, 'Uniform Sprite Size Min', X('%s*%s' % (size[0], SCL))), (PS, INIT, 'Uniform Sprite Size Max', X('%s*%s' % (size[1], SCL))),
                    (PS, 'AddVelocity', 'Velocity Mode', E('In Cone')), (PS, 'AddVelocity', 'Cone Angle', 360.0),   # leaders start AT the centre: 'From Point' gave them no direction
                    (PS, 'AddVelocity', ['Velocity Speed', 'Minimum'], X('%s*%s' % (speed[0], SCL))), (PS, 'AddVelocity', ['Velocity Speed', 'Maximum'], X('%s*%s' % (speed[1], SCL))),
                    (PU, 'GravityForce', 'Gravity', (0.0, 0.0, 0.0)), (PU, 'Drag', 'Drag', 1.7)]}
    follow = {'name': 'Followers', 'base': 'followers', 'props': dict(fb), 'mat': 'M_TD_VFX_Glow',
              'set': [(EU, 'EmitterState', 'Life Cycle Mode', E('Self')), (EU, 'EmitterState', 'Loop Behavior', E('Once')), (EU, 'EmitterState', 'Loop Duration', float(life + 0.2)),
                      (EU, 'SpawnParticlesFromOtherEmitter', 'Spawn Rate', 420.0),
                      (PS, INIT, 'Lifetime', float(trail_life)), (PS, INIT, 'Color Mode', E('Direct Set')), (PS, INIT, 'Color', col(1.3)),
                      (PS, INIT, 'Sprite Size Mode', E('Uniform')), (PS, INIT, 'Uniform Sprite Size', X('%s*%s' % (size[0] * 0.8, SCL))),
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
         spin(shed('Fireball', 'M_TD_VFX_Fireball', 0, (0.80, 1.60), (340, 620), (500, 2100), radius=170.0, drag=2.8, curl=1100.0, rgb=(1.0, 1.0, 1.0),
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


S('NS_TD_Explosion_Small', U(FIRE, 0.45), explosion(1))
S('NS_TD_Explosion_Medium', U(FIRE, 1.0), explosion(2))
S('NS_TD_Explosion_Large', U(FIRE, 2.2), explosion(3))


def explosion_cheap(final=False):
    """Six sprite emitters, no meshes, no light: for chains of 20 at once (Dominion hull sections) and the final blast."""
    e = [flash('Flash', 'M_TD_VFX_Glow', 1500, 0.10, 0.9, growth='0.5 + Particles.NormalizedAge*0.7', power=1.2),
         spin(shed('Blast', 'M_TD_VFX_FlipExplosion', 0, (1.0, 1.3), (1500, 1900), (0, 60), radius=60.0, drag=3.0, rgb=(0.6, 0.6, 0.6),
                   burst=3 if final else 1, dur=1.4, power=0.35, growth='0.62 + pow(Particles.NormalizedAge, 0.5)*0.55')),
         spin(shed('Fireball', 'M_TD_VFX_Fireball', 0, (0.60, 1.25), (340, 620), (500, 2100), radius=170.0, drag=2.8, rgb=(1.0, 1.0, 1.0),
                   burst=18 if final else 10, dur=1.6, power=0.7, growth='0.45 + pow(Particles.NormalizedAge, 0.45)*1.25')),
         flash('Shockwave', 'M_TD_VFX_FlipRing', 5200, 0.36, 0.9, growth='0.06 + pow(Particles.NormalizedAge, 0.5)*0.95', power=1.4),
         shed('Sparks', 'M_TD_VFX_Spark', 0, (0.45, 1.20), (9, 20), (2600, 10500), k=1.9, radius=60.0, drag=2.2, stretch=26,
              burst=40 if final else 14, dur=1.3, power=1.4),
         spin(shed('Smoke', 'M_TD_VFX_FlipSmoke', 0, (2.0, 3.2), (520, 760), (150, 700), radius=180.0, drag=1.5, rgb=(0.52, 0.45, 0.40), alpha=0.8,
                   burst=6 if final else 2, dur=3.4, power=0.6, growth='0.6 + pow(Particles.NormalizedAge, 0.6)*1.3', sort=True))]
    if final:
        e += [pops()]
    return e


# Dominion break-up (fields arrive with T4's commit): HullSectionExplosion is authored for a 50 m radius (gameplay scales
# 0.5x-1.7x, about 20 in a chain); BreakupFinalBlast for a 500 m radius. At Scale 1 the blast sprite has a 9 m radius.
S('NS_TD_Explosion_HullSection', U(FIRE, 5.5), explosion_cheap())
S('NS_TD_Explosion_FinalBlast', U(FIRE, 55.0), explosion_cheap(True))

fx = tdvfx.Builder(call_tool)
for _name, _user, _emitters in SYSTEMS:
    await fx.system(_name, _emitters, user=_user)
    await asyncio.sleep(0)
fx.report()

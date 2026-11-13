# Triad Drift ENG-37, tier 2 and story effects (round 5). NOT run on its own: it reuses build_tier1.py's helpers, so
# the batch file is   ONLY = ['__none__']  +  build_tier1.py  +  ONLY2 = [...prefixes]  +  this file   (see rebuild_all.sh, T2).
# Every system here is UNWIRED unless wire_r5.py lists it; the slot each is meant for is in the comment above it.
# Conventions as tier 1: +X forward, User.Color / User.Scale, sizes in cm at Scale 1.
ONLY2 = globals().get('ONLY2')
SYSTEMS2 = []
SPHERE = '/Engine/BasicShapes/Sphere'   # 100 cm across


def S2(name, user, emitters):
    if ONLY2 and not any((name == o[1:]) if o.startswith('=') else name.startswith(o) for o in ONLY2):   # '=Name' is an exact match
        return
    SYSTEMS2.append((name, user, emitters))


def engine_mesh(e, path):
    e['rend']['Meshes'][0]['mesh'] = ref(path)
    return e


def shell(name, radius, k=1.0, mat='M_TD_VFX_Field', pulse=None):
    """A persistent sphere shell of the given radius (cm at Scale 1), local space."""
    d = radius / 50.0
    return engine_mesh(mesh_single(name, 'SM_TD_VFX_Disc', mat, '%s, %s, %s' % (d, d, d), k, None, live_scale=pulse), SPHERE)


def shell_flash(name, radius, life, growth, k=1.0, mat='M_TD_VFX_Field', power=1.0, delay=0.0):
    d = radius / 50.0
    return engine_mesh(mesh_flash(name, 'SM_TD_VFX_Disc', mat, '%s, %s, %s' % (d, d, d), life, k, growth=growth, power=power, delay=delay), SPHERE)


def looping(e, life):
    """A 'single' emitter whose particle lives `life` seconds and respawns (materials that animate over Age)."""
    e['set'] += [(PS, INIT, 'Lifetime', float(life))]
    return e


def flat_ring(name, size, life, k=1.0, growth='0.1 + pow(Particles.NormalizedAge, 0.5)*0.9', mat='M_TD_VFX_Shock', power=1.3):
    """A ring lying on the system's local XY plane (normal +Z): ground slams. size = final diameter in cm.
    The disc mesh's normal is +X; the mesh renderer's Velocity facing turns +X along the particle's velocity, which is a
    token 1 cm/s straight up. (InitialMeshOrientation with a rotation vector left the ring standing upright.)"""
    e = mesh_flash(name, 'SM_TD_VFX_Disc', mat, '1.0, %s, %s' % (size / 100.0, size / 100.0), life, k, growth='float3(1,1,1)*(%s)' % growth, power=power)
    e['set'] = [x for x in e['set'] if x[1] != 'AddVelocity'] + [(PS, 'AddVelocity', 'Velocity Mode', E('In Cone')), (PS, 'AddVelocity', 'Cone Axis', (0.0, 0.0, 1.0)),
                                                                 (PS, 'AddVelocity', 'Cone Angle', 0.0), (PS, 'AddVelocity', 'Velocity Speed', 1.0)]
    e['rend']['FacingMode'] = 'Velocity'
    return e


def arcs(name, rate, size, radius, life=(0.10, 0.22), k=1.4, burst=None, dur=0.5, local=True):
    return spin(shed(name, 'M_TD_VFX_FlipArc', rate, life, size, (0, 0), k=k, radius=radius, drag=0.0, power=0.6, burst=burst, dur=dur, local=local))


FIELD = (0.45, 0.25, 1.0, 1.0)      # the Dominion's distortion field: violet-blue, away from both squad and enemy colours
EMP = (0.35, 0.75, 1.0, 1.0)
PORTAL = RIFT
BLADE = (0.35, 0.85, 1.0, 1.0)
CORE = (1.0, 0.25, 0.12, 1.0)
WHITE4 = (1.0, 1.0, 1.0, 1.0)

# ============================================================================================= story
# Slot: UTDDominionVisuals.ShieldIdle (looping, attached at the Dominion's centre). Authored as a 1 km radius shell at
# Scale 1: set the cue's VFXScale to (field radius in km). Cheap: two sphere meshes, one sprite, about 14 arc sprites.
S2('NS_TD_Dominion_Field', U(FIELD), [
    shell('Shell', 100000, 0.45, pulse='float3(1,1,1)*(1.0 + 0.012*sin(Engine.Time*1.7))'),
    shell('Inner', 97000, 0.15, pulse='float3(1,1,1)*(1.0 + 0.015*sin(Engine.Time*1.1 + 2.0))'),
    single('Halo', 'M_TD_VFX_Glow', 330000, 0.10, 3.1),
    arcs('Arcs', 26.0, (20000, 42000), 100000, life=(0.25, 0.6), k=0.9)])
# Slot: UTDDominionVisuals.ShieldCollapse (one shot at the centre when the last generator dies). 1 km radius at Scale 1.
S2('NS_TD_Dominion_FieldCollapse', U(FIELD), [
    shell_flash('Flare', 100000, 0.5, 'float3(1,1,1)*(1.0 + 0.06*Particles.NormalizedAge)', k=1.5, power=0.6),
    shell_flash('Implode', 100000, 1.3, 'float3(1,1,1)*(1.0 - 0.92*pow(Particles.NormalizedAge, 1.6))', k=1.6, power=0.4, delay=0.35),
    flash('Wave', 'M_TD_VFX_Ring', 520000, 1.6, 0.9, growth='0.08 + pow(Particles.NormalizedAge, 0.6)*0.92', power=1.5, delay=1.5),
    flash('Flash', 'M_TD_VFX_Glow', 300000, 0.5, 0.8, growth='0.4 + Particles.NormalizedAge*0.8', power=1.5, delay=1.5),
    arcs('Arcs', 0, (14000, 30000), 95000, life=(0.3, 1.2), k=0.9, burst=40, dur=1.6, local=False),   # seen in play: bigger arcs whited out the screen near a generator
    shed('Shards', 'M_TD_VFX_Spark', 0, (0.8, 1.8), (500, 1100), (20000, 90000), k=1.6, radius=95000.0, drag=0.8, stretch=22, burst=140, dur=2.0, power=1.2)])
# Slot: none yet (code hook wanted: T3's transmissions, the escape sequence, a hit of "static"). A one-shot, camera-facing
# tear of chromatic bars, 0.3 s; spawn it attached in front of the camera (authored 12 m wide) or at a world point.
S2('NS_TD_Glitch_Tear', U(WHITE4), [
    flash('Tear', 'M_TD_VFX_Glitch', 1200, 0.30, 0.9, power=0.5),
    flash('Tear2', 'M_TD_VFX_Glitch', 1500, 0.22, 0.5, power=0.5, delay=0.06),
    flash('Flash', 'M_TD_VFX_Glow', 1600, 0.10, 0.12)])

# ============================================================================================= travel and powers
# Slots: UTDTravelFX.PortalOpen / PortalClose (+X = the portal's facing). Authored for a 6 m portal.
S2('NS_TD_Portal_Open', U(PORTAL), [
    mesh_flash('Ring', 'SM_TD_VFX_Disc', 'M_TD_VFX_Shock', '1.0, 9.0, 9.0', 0.40, 1.2, growth='float3(1,1,1)*(0.10 + pow(Particles.NormalizedAge, 0.5)*0.90)', power=1.2),
    mesh_flash('Swirl', 'SM_TD_VFX_Disc', 'M_TD_VFX_Swirl', '1.0, 7.0, 7.0', 0.55, 1.3, growth='float3(1,1,1)*(0.2 + pow(Particles.NormalizedAge, 0.4)*0.8)', power=0.8),
    flash('Flash', 'M_TD_VFX_Flare', 700, 0.10, 1.2, growth='0.5 + Particles.NormalizedAge'),
    shed('RimSparks', 'M_TD_VFX_Spark', 0, (0.25, 0.7), (5, 11), (900, 3800), k=1.6, radius=300.0, drag=2.2, stretch=14, cone=((1.0, 0.0, 0.0), 88.0), burst=48, dur=0.8),
    shed('Motes', 'M_TD_VFX_Glow', 0, (0.5, 1.2), (10, 26), (100, 900), k=1.1, radius=300.0, drag=1.5, curl=900.0, burst=30, dur=1.3, power=2.0)])
S2('NS_TD_Portal_Close', U(PORTAL), [
    mesh_flash('Ring', 'SM_TD_VFX_Disc', 'M_TD_VFX_Shock', '1.0, 7.0, 7.0', 0.30, 1.2, growth='float3(1,1,1)*(1.0 - 0.92*pow(Particles.NormalizedAge, 0.7))', power=0.4),
    mesh_flash('Swirl', 'SM_TD_VFX_Disc', 'M_TD_VFX_Swirl', '1.0, 6.5, 6.5', 0.30, 1.2, growth='float3(1,1,1)*(1.0 - 0.9*Particles.NormalizedAge)', power=0.4),
    flash('Pop', 'M_TD_VFX_Flare', 420, 0.09, 1.3, growth='0.5 + Particles.NormalizedAge', delay=0.28),
    shed('RimSparks', 'M_TD_VFX_Spark', 0, (0.2, 0.5), (4, 9), (600, 2600), k=1.5, radius=40.0, drag=2.5, stretch=12, burst=26, dur=0.9)])
# Slot: Lancer UTDShipVisuals.PowerActive (looping, attached at the ship origin, +X forward) while the ram runs.
S2('NS_TD_Ram_Streak', U(LANCER), [
    mesh_single('Shock', 'SM_TD_VFX_Flame', 'M_TD_VFX_Flame', '-7.0, 4.2, 4.2', 0.55, 37.3, offset=(420, 0, 0)),
    single('Nose', 'M_TD_VFX_Glow', 420, 0.9, 61.7, offset=(330, 0, 0)),
    shed('Streaks', 'M_TD_VFX_Spark', 220.0, (0.12, 0.30), (7, 14), (5000, 11000), k=1.6, radius=170.0, drag=0.5, local=True, stretch=34, cone=((-1.0, 0.0, 0.0), 4.0)),
    shed('Embers', 'M_TD_VFX_Glow', 70.0, (0.25, 0.6), (9, 22), (1500, 4000), k=1.2, radius=150.0, drag=0.8, curl=1500.0, local=True, cone=((-1.0, 0.0, 0.0), 12.0), power=2.0)])
# Slot: UTDWeaponFX.Tracer of DA_MissileSwarm_FX / DA_SpikeRockets_FX (and the _Enemy variant for the Null missiles).
def missile(k=1.0):
    return [mesh_single('Motor', 'SM_TD_VFX_Flame', 'M_TD_VFX_Flame', '-2.6, 0.8, 0.8', 1.2 * k, 61.7, offset=(-60, 0, 0)),
            glow_single('Glow', 150, 0.9 * k, 47.9),
            trail('Trail', 46, 0.55, 1.0, rate=120.0, taper=0.6, power=1.2),
            shed('Fire', 'M_TD_VFX_Fireball', 60.0, (0.16, 0.30), (60, 100), (10, 90), radius=8.0, drag=1.0, curl=500.0, rgb=(0.9, 0.9, 0.9), growth='0.8 + Particles.NormalizedAge*0.9'),
            spin(shed('Smoke', 'M_TD_VFX_FlipSmoke', 34.0, (0.6, 1.1), (90, 150), (10, 70), radius=8.0, drag=1.0, curl=500.0, power=0.8, rgb=(0.34, 0.30, 0.29), alpha=0.5,
                      growth='0.8 + Particles.NormalizedAge*1.8', sort=True))]


S2('NS_TD_Missile_Trail', U(BASTION), missile())
S2('NS_TD_Missile_Trail_Enemy', U(ENEMY), missile(0.8))
# Slot: UTDAbilityFX.Activate of DA_Ability_EMPPulse_FX (one shot at the ship). Authored for a 60 m radius pulse.
S2('NS_TD_EMP_Pulse', U(EMP), [
    flash('Ring', 'M_TD_VFX_Ring', 12000, 0.55, 1.3, growth='0.05 + pow(Particles.NormalizedAge, 0.55)*0.95', power=1.2),
    flash('Ring2', 'M_TD_VFX_Shock', 12000, 0.70, 0.9, growth='0.04 + pow(Particles.NormalizedAge, 0.7)*0.96', power=1.4, delay=0.05),
    shell_flash('Dome', 6000, 0.55, 'float3(1,1,1)*(0.05 + pow(Particles.NormalizedAge, 0.55)*0.95)', k=0.8, power=1.2),
    flash('Flash', 'M_TD_VFX_Flare', 2400, 0.12, 1.0, growth='0.5 + Particles.NormalizedAge'),
    arcs('Arcs', 0, (700, 1800), 2600, life=(0.15, 0.5), k=1.6, burst=44, dur=0.7, local=False),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.3, 0.7), (8, 16), (4000, 11000), k=1.5, radius=100.0, drag=1.8, stretch=18, burst=70, dur=0.9)])
# Slot: Bastion UTDShipVisuals.PowerActive (looping, attached). Authored for a 30 m radius dome.
S2('NS_TD_ShieldDome', U(BASTION), [
    shell('Dome', 3000, 0.50, pulse='float3(1,1,1)*(1.0 + 0.01*sin(Engine.Time*3.0))'),
    shell('Hex', 2960, 0.16, mat='M_TD_VFX_Hex'),
    shed('Motes', 'M_TD_VFX_Glow', 40.0, (0.6, 1.4), (12, 30), (0, 120), k=1.0, radius=3000.0, drag=0.5, curl=400.0, local=True, power=1.5, fade_in=5.0)])
# Slot: UTDAbilityFX.Loop of DA_Ability_Cloak_FX (looping, attached; the ship itself is faded by code). 6 m shimmer.
S2('NS_TD_Cloak_Shimmer', U((0.55, 0.85, 1.0, 1.0)), [
    shell('Shimmer', 600, 0.16, pulse='float3(1.25, 1.0, 0.45)*(1.0 + 0.03*sin(Engine.Time*5.0))'),
    shed('Motes', 'M_TD_VFX_Glow', 26.0, (0.4, 0.9), (5, 12), (0, 160), k=0.8, radius=420.0, drag=1.0, curl=500.0, local=True, power=1.5, fade_in=5.0)])
# Slot: UTDAbilityFX.Loop of DA_Ability_GravityWell_FX (looping, at the well). Authored for a 40 m pull radius.
S2('NS_TD_GravityWell', U((0.60, 0.30, 1.0, 1.0)), [
    looping(single('Swirl', 'M_TD_VFX_Swirl', 7000, 1.1, None), 2.4),
    looping(single('SwirlB', 'M_TD_VFX_Swirl', 4200, 0.9, None), 1.5),
    single('Core', 'M_TD_VFX_Glow', 900, 1.3, 23.1),
    shed('Infall', 'M_TD_VFX_Spark', 120.0, (0.5, 0.9), (8, 16), (-5200, -3000), k=1.4, radius=4000.0, drag=0.0, local=True, stretch=16, power=0.6, fade_in=4.0),
    arcs('Arcs', 8.0, (500, 1100), 500, life=(0.1, 0.25), k=1.2)])
# Slot: UTDTravelFX.HyperdriveTunnel (looping, attached to the ship, +X forward).
S2('NS_TD_Hyperdrive_Streaks', U((0.70, 0.85, 1.0, 1.0)), [
    shed('Streaks', 'M_TD_VFX_Spark', 420.0, (0.25, 0.5), (10, 22), (30000, 60000), k=1.5, radius=2600.0, drag=0.0, local=True, stretch=120, cone=((-1.0, 0.0, 0.0), 1.5), fade_in=6.0),
    single('Ahead', 'M_TD_VFX_Glow', 5000, 0.35, 11.3, offset=(20000, 0, 0)),
    shed('Motes', 'M_TD_VFX_Glow', 120.0, (0.2, 0.4), (14, 30), (20000, 40000), k=1.2, radius=1500.0, drag=0.0, local=True, cone=((-1.0, 0.0, 0.0), 2.0))])

# ============================================================================================= on foot
# Slot: UTDBladeFX.SwingLight / SwingHeavy (attached to the blade tip while it swings).
S2('NS_TD_Blade_Swing', U(BLADE), [
    trail('Arc', 34, 0.16, 1.6, rate=240.0, taper=0.85, power=1.3),
    trail('Edge', 9, 0.20, 2.6, rate=240.0, taper=0.6, power=1.2),
    shed('Motes', 'M_TD_VFX_Glow', 60.0, (0.15, 0.35), (2, 5), (10, 120), k=1.3, radius=6.0, drag=2.0, curl=300.0, power=2.0)])
# Slot: UTDBladeFX.Deflect (one shot at the blade, +X towards the shooter).
S2('NS_TD_Blade_Deflect', U(BLADE), [
    flash('Flash', 'M_TD_VFX_Flare', 70, 0.07, 1.3, growth='0.5 + Particles.NormalizedAge'),
    flash('Glow', 'M_TD_VFX_Glow', 90, 0.12, 0.5),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.12, 0.35), (1.2, 2.6), (300, 1400), k=1.7, radius=2.0, drag=3.0, stretch=12, cone=((1.0, 0.0, 0.0), 50.0), burst=22, dur=0.5)])
# Slot: UTDBladeFX.PerfectDeflect.
S2('NS_TD_Blade_PerfectDeflect', U((0.85, 0.95, 1.0, 1.0)), [
    flash('Flash', 'M_TD_VFX_Flare', 200, 0.12, 1.5, growth='0.5 + Particles.NormalizedAge'),
    flash('Glow', 'M_TD_VFX_Glow', 260, 0.20, 0.7),
    mesh_flash('Ring', 'SM_TD_VFX_Disc', 'M_TD_VFX_Shock', '1.0, 2.4, 2.4', 0.28, 1.2, growth='float3(1,1,1)*(0.1 + pow(Particles.NormalizedAge, 0.5)*0.9)', power=1.3),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.18, 0.5), (1.5, 3.2), (400, 2200), k=1.9, radius=2.0, drag=2.6, stretch=14, cone=((1.0, 0.0, 0.0), 70.0), burst=46, dur=0.6),
    light('Light', 600, 0.2, k=4.0, rgb=(0.6, 0.85, 1.0))])
# Slot: UTDBladeFX.AirSlam. Local XY plane, pivot at the ground contact, +Z up, 6 m across.
S2('NS_TD_Blade_AirSlam', U(BLADE), [
    flat_ring('Ring', 600, 0.38, 1.4),
    flat_ring('Ring2', 460, 0.55, 0.6, mat='M_TD_VFX_Ring', growth='0.15 + pow(Particles.NormalizedAge, 0.7)*0.85'),
    flash('Flash', 'M_TD_VFX_Glow', 260, 0.12, 0.8),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.2, 0.55), (1.6, 3.4), (350, 1300), k=1.7, radius=30.0, drag=2.4, stretch=12, cone=((0.0, 0.0, 1.0), 82.0), burst=40, dur=0.7),
    spin(shed('Dust', 'M_TD_VFX_Smoke', 0, (0.6, 1.1), (60, 110), (200, 520), radius=60.0, drag=2.6, rgb=(0.45, 0.50, 0.55), alpha=0.5, cone=((0.0, 0.0, 1.0), 88.0),
              burst=16, dur=1.2, power=0.9, growth='0.6 + Particles.NormalizedAge*1.4', sort=True))])
# Slots: the crew blaster's UTDWeaponFX.Tracer and ImpactHull / ImpactWorld (on-foot scale).
S2('NS_TD_Crew_Bolt', U(ENEMY), tracer(70, 9, sheath=3.2, rib_life=0.06, spark_rate=14.0, flick=47.1, hot=0.8, needle_core=False))
S2('NS_TD_Crew_BoltImpact', U(ENEMY), [
    flash('Flash', 'M_TD_VFX_Flare', 45, 0.07, 1.2, growth='0.5 + Particles.NormalizedAge'),
    flash('Glow', 'M_TD_VFX_Glow', 60, 0.14, 0.5),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.12, 0.4), (1.0, 2.2), (200, 900), k=1.6, radius=1.5, drag=2.5, stretch=11, cone=((1.0, 0.0, 0.0), 60.0), burst=14, dur=0.5),
    spin(shed('Smoke', 'M_TD_VFX_Smoke', 0, (0.5, 0.9), (16, 30), (10, 50), radius=3.0, drag=1.5, rgb=(0.3, 0.27, 0.27), alpha=0.5, cone=((1.0, 0.0, 0.0), 60.0),
              burst=3, dur=1.0, power=0.8, growth='0.8 + Particles.NormalizedAge*1.3', sort=True))])
# Slot: the drone's thrusters (sockets Engine_0 / Engine_1), attached, blowing along -X. Engine flame at 1/8 size.
S2('NS_TD_Drone_Thruster', U((1.0, 0.10, 0.015, 1.0), 0.13, Throttle=0.7, Boost=0.0), engine(kk=0.7)[:6])   # [:5] (persistent emitters only) built clean but never activated: a system needs one spawning emitter

# ============================================================================================= reactor core (UTDReactorCoreFX, T5)
# Orb: looping at the core's centre; 3 m radius.
S2('NS_TD_Core_Orb', U(CORE), [
    shell('Shell', 300, 0.9, pulse='float3(1,1,1)*(1.0 + 0.04*sin(Engine.Time*4.0))'),
    looping(single('Swirl', 'M_TD_VFX_Swirl', 900, 1.2, None), 1.8),
    single('Heart', 'M_TD_VFX_Glow', 520, 1.6, 31.7),
    single('Halo', 'M_TD_VFX_Glow', 1500, 0.35, 7.3),
    arcs('Arcs', 16.0, (200, 480), 300, life=(0.08, 0.2), k=1.5),
    shed('Motes', 'M_TD_VFX_Glow', 50.0, (0.5, 1.2), (4, 10), (40, 320), k=1.2, radius=320.0, drag=1.0, curl=600.0, local=True, power=1.6)])
# Conduit break: one shot, +X out of the conduit.
S2('NS_TD_Core_ConduitBreak', U(EMP), [
    flash('Flash', 'M_TD_VFX_Flare', 320, 0.09, 1.4, growth='0.5 + Particles.NormalizedAge'),
    arcs('Arcs', 30.0, (120, 300), 60, life=(0.08, 0.2), k=1.6, local=False),
    shed('Spray', 'M_TD_VFX_Spark', 0, (0.25, 0.7), (2, 4.5), (600, 2600), k=1.8, radius=8.0, drag=2.0, stretch=14, cone=((1.0, 0.0, 0.0), 40.0), burst=60, dur=0.9),
    spin(shed('Smoke', 'M_TD_VFX_SmokeRim', 0, (0.9, 1.6), (70, 130), (40, 220), radius=20.0, drag=1.6, rgb=(0.30, 0.28, 0.30), alpha=0.6, cone=((1.0, 0.0, 0.0), 50.0),
              burst=6, dur=1.8, power=0.8, growth='0.7 + Particles.NormalizedAge*1.4', sort=True))])
# Shield drop: one shot at the core, 4 m radius shell that flares and falls apart.
S2('NS_TD_Core_ShieldDrop', U(EMP), [
    shell_flash('Flare', 400, 0.25, 'float3(1,1,1)*(1.0 + 0.05*Particles.NormalizedAge)', k=2.4, power=0.5),
    shell_flash('Fall', 400, 0.7, 'float3(1.0 + 0.3*Particles.NormalizedAge, 1.0 + 0.3*Particles.NormalizedAge, 1.0 - 0.8*Particles.NormalizedAge)', k=1.2, power=1.5, delay=0.2),
    arcs('Arcs', 0, (160, 380), 400, life=(0.1, 0.4), k=1.5, burst=30, dur=0.6, local=False),
    shed('Shards', 'M_TD_VFX_Spark', 0, (0.3, 0.8), (2, 5), (300, 1500), k=1.6, radius=400.0, drag=1.5, stretch=12, burst=80, dur=1.0)])
# Laser sweep: looping beam along +X from the emitter (30 m), rotate the component to sweep it.
S2('NS_TD_Core_LaserSweep', U(CORE), [
    needle('Beam', 70, 3000, 1.3, 91.3, back=1500),
    needle('BeamCore', 22, 3000, 2.4, 131.7, back=1500),
    single('Source', 'M_TD_VFX_Glow', 240, 1.6, 53.1),
    shed('Sparks', 'M_TD_VFX_Spark', 60.0, (0.15, 0.4), (1.5, 3.5), (200, 1200), k=1.6, radius=6.0, drag=2.0, local=True, stretch=12, cone=((1.0, 0.0, 0.0), 70.0))])
# Heart hit: one shot on the exposed heart, +X back along the hit.
S2('NS_TD_Core_HeartHit', U(CORE), [
    flash('Flash', 'M_TD_VFX_Flare', 420, 0.09, 1.5, growth='0.5 + Particles.NormalizedAge'),
    flash('Glow', 'M_TD_VFX_Glow', 600, 0.2, 0.6),
    mesh_flash('Ring', 'SM_TD_VFX_Disc', 'M_TD_VFX_Shock', '1.0, 5.0, 5.0', 0.3, 1.0, growth='float3(1,1,1)*(0.1 + pow(Particles.NormalizedAge, 0.5)*0.9)', power=1.3),
    arcs('Arcs', 0, (160, 360), 120, life=(0.1, 0.3), k=1.5, burst=14, dur=0.5, local=False),
    shed('Spray', 'M_TD_VFX_Spark', 0, (0.25, 0.7), (2.5, 5.5), (700, 3200), k=1.8, radius=10.0, drag=2.0, stretch=15, cone=((1.0, 0.0, 0.0), 45.0), burst=60, dur=0.9)])
# Core destroyed: one shot, the chamber-filling blast (authored 20 m radius), white-hot then the core's red.
S2('NS_TD_Core_Destroyed', U(FIRE, 2.2), [
    flash('Flash', 'M_TD_VFX_Glow', 3000, 0.25, 1.4, growth='0.5 + Particles.NormalizedAge*0.7', power=1.2),
    flash('Star', 'M_TD_VFX_Flare', 2600, 0.14, 1.0, growth='0.5 + Particles.NormalizedAge'),
    shell_flash('Burst', 900, 0.45, 'float3(1,1,1)*(0.1 + pow(Particles.NormalizedAge, 0.5)*0.9)', k=1.6, power=1.2),
    spin(shed('Fireball', 'M_TD_VFX_Fireball', 0, (0.7, 1.4), (340, 620), (500, 2400), radius=170.0, drag=2.6, curl=1100.0, rgb=(1.0, 1.0, 1.0),
              burst=26, dur=1.8, power=0.7, growth='0.45 + pow(Particles.NormalizedAge, 0.45)*1.25')),
    flash('Shockwave', 'M_TD_VFX_FlipRing', 5200, 0.4, 1.0, growth='0.06 + pow(Particles.NormalizedAge, 0.5)*0.95', power=1.4),
    arcs('Arcs', 0, (300, 700), 500, life=(0.15, 0.6), k=1.6, burst=30, dur=1.0, local=False),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.5, 1.3), (9, 20), (2600, 10500), k=1.9, radius=60.0, drag=2.2, stretch=26, burst=80, dur=1.5, power=1.4),
    spin(shed('Smoke', 'M_TD_VFX_SmokeRim', 0, (1.6, 2.8), (420, 640), (250, 1100), radius=200.0, drag=2.0, curl=500.0, rgb=(0.34, 0.30, 0.28), alpha=0.8,
              burst=8, dur=3.0, power=0.7, fade_in=6.0, growth='0.6 + pow(Particles.NormalizedAge, 0.6)*1.2', sort=True))])

# ============================================================================================= planetary descent (ENG-38, round 6)
# Grok Bot's descent textures (ArtSource/EngAI/Descent). Everything is attached at the ship's origin, +X forward.
# User.Intensity (0..1, default 1) is for T1's entry curve; until code sets it the systems ramp themselves in over
# 1.3 s of emitter age, so the plain AtmosphericEntry cue (attached loop, no parameters) already builds up.
ENTRY = (1.0, 0.42, 0.12, 1.0)   # deeper: the first orange went cream over the bright planet
RAMP = '(User.Intensity*saturate(Emitter.Age/1.3))'
EFLK = '(0.90 + 0.20*frac(sin(Engine.Time*%s)*43758.5))'


def ramped(e):
    """Fade a persistent emitter's particle with the entry strength (particle alpha; every VFX material multiplies by it)."""
    if not any(a[1].endswith('ScaleColor') for a in e['add']):
        e['add'].append((PU, 'Update/Color/ScaleColor'))
        e['set'].append((PU, 'ScaleColor', 'ScaleRGB', False))
    e['set'] += [(PU, 'ScaleColor', 'ScaleA', True), (PU, 'ScaleColor', 'Scale Alpha', X(RAMP))]
    return e


def entry_sheath():
    # Seen from the chase camera (behind and above, looking along +X) a shell round the ship reads as a soap bubble or a
    # target ring (both tried, both seen in play). So the sheath is fire: flames born round the nose and the wing roots
    # that stream back over the hull and grow, a small hot shock cap on the nose (flame mesh, apex forward, faded out
    # before its widest point), a point of white heat and a soft halo that rim-lights the silhouette.
    grow_ = 'float3(0.75 + 0.25*%s, 0.80 + 0.20*%s, 0.80 + 0.20*%s)*%s'
    return [ramped(mesh_single('Cap', 'SM_TD_VFX_Flame', 'M_TD_VFX_Sheath', '5.0, 3.4, 3.4', 1.2, None, offset=(-120, 0, 0),
                               live_scale=grow_ % (RAMP, RAMP, RAMP, EFLK % 87.7))),
            ramped(single('Nose', 'M_TD_VFX_Glow', 230, 0.8, 71.3, offset=(270, 0, 0))),
            ramped(single('Halo', 'M_TD_VFX_Glow', 1100, 0.28, 13.7, offset=(60, 0, 0))),
            # black-body fire (white-hot at birth, orange, deep red as it erodes); M_TD_VFX_Fire read as pale dust puffs in play
            offset(spin(shed('Flames', 'M_TD_VFX_Fireball', 0, (0.30, 0.55), (150, 280), (1600, 3400), radius=170.0, drag=0.6, curl=1800.0, local=True, rgb=(1.0, 1.0, 1.0),
                             cone=((-1.0, 0.0, 0.0), 14.0), growth='0.6 + Particles.NormalizedAge*1.5', rate_expr='170.0*' + RAMP)), 230),
            offset(spin(shed('WingFlames', 'M_TD_VFX_Fireball', 0, (0.25, 0.45), (90, 180), (1600, 3200), radius=330.0, drag=0.6, curl=1600.0, local=True, rgb=(1.0, 1.0, 1.0),
                             cone=((-1.0, 0.0, 0.0), 10.0), growth='0.6 + Particles.NormalizedAge*1.4', rate_expr='110.0*' + RAMP)), 120)]


def entry_streaks():
    return [offset(shed('Streaks', 'M_TD_VFX_HeatStreak', 0, (0.25, 0.50), (24, 52), (5000, 11000), k=1.5, radius=340.0, drag=0.0, local=True,
                        stretch=9, cone=((-1.0, 0.0, 0.0), 5.0), rate_expr='150.0*' + RAMP, fade_in=8.0), 260),
            offset(shed('Embers', 'M_TD_VFX_Glow', 0, (0.30, 0.70), (8, 20), (3000, 8000), k=1.4, radius=300.0, drag=0.3, curl=1500.0, local=True,
                        cone=((-1.0, 0.0, 0.0), 10.0), rate_expr='120.0*' + RAMP, power=2.0), 260),
            offset(shed('Licks', 'M_TD_VFX_Fire', 0, (0.12, 0.25), (90, 170), (2500, 5000), k=0.7, radius=220.0, drag=1.0, curl=2000.0, local=True,
                        cone=((-1.0, 0.0, 0.0), 8.0), growth='1.0 + Particles.NormalizedAge*0.8', rate_expr='110.0*' + RAMP), 240)]


def entry_cone():
    # The moment of contact: a Mach ring opening round the ship, a second one behind it, a flash.
    return [mesh_flash('Cone', 'SM_TD_VFX_Disc', 'M_TD_VFX_FlipShockCone', '1.0, 16.0, 16.0', 0.45, 0.9, growth='float3(1,1,1)*(0.15 + pow(Particles.NormalizedAge, 0.5)*0.85)', power=1.2),
            mesh_flash('Cone2', 'SM_TD_VFX_Disc', 'M_TD_VFX_FlipShockCone', '1.0, 26.0, 26.0', 0.60, 0.5, growth='float3(1,1,1)*(0.10 + pow(Particles.NormalizedAge, 0.6)*0.90)', power=1.4, delay=0.12),
            flash('Flash', 'M_TD_VFX_Glow', 1400, 0.25, 0.6, growth='0.5 + Particles.NormalizedAge*0.7')]


# Slot: UTDTravelFX.AtmosphericEntry (attached loop on the ship root for the 2.5 s dive): contact ring, sheath, streaks.
# (NS_TD_Entry itself is specified further down, once the debris exists)
# The parts on their own (for separate slots / other timing):
S2('NS_TD_Entry_HeatStreaks', U(ENTRY, 1.0, Intensity=1.0), entry_streaks()[:2])
S2('NS_TD_Entry_ShockCone', U(ENTRY, 1.0, Intensity=1.0), entry_cone())
# Slot wanted (T1): a one-shot at the ship as it breaks through the cloud deck, +X forward. Authored for a 30 m cloud wall.
S2('NS_TD_Entry_CloudPunch', U(WHITE4, 1.0, Intensity=1.0), [
    shed('Punch', 'M_TD_VFX_CloudPunch', 0, (0.9, 1.2), (2600, 3600), (0, 200), radius=300.0, drag=0.0, rgb=(1.0, 1.0, 1.0), alpha=0.9,
         burst=3, dur=1.3, power=0.5, growth='0.8 + Particles.NormalizedAge*0.9', sort=True),
    spin(shed('Shreds', 'M_TD_VFX_CloudWisp', 0, (0.4, 0.9), (500, 1100), (3000, 9000), radius=900.0, drag=0.5, rgb=(1.0, 1.0, 1.0), alpha=0.7,
              cone=((-1.0, 0.0, 0.0), 25.0), burst=26, dur=1.0, power=0.8, sort=True))])
# Slot wanted (T1): a loop attached to the ship while it descends under the deck: vapour streaming back past the camera.
S2('NS_TD_Entry_CloudWisps', U(WHITE4, 1.0, Intensity=1.0), [
    offset(spin(shed('Wisps', 'M_TD_VFX_CloudWisp', 0, (0.5, 0.9), (900, 2200), (9000, 16000), radius=2600.0, drag=0.0, local=True, rgb=(1.0, 1.0, 1.0), alpha=0.55,
                     cone=((-1.0, 0.0, 0.0), 3.0), rate_expr='26.0*' + RAMP, power=0.8, fade_in=5.0, sort=True)), 6000),
    offset(shed('Mist', 'M_TD_VFX_Spark', 0, (0.25, 0.45), (5, 10), (14000, 24000), k=0.5, radius=1800.0, drag=0.0, local=True, stretch=40,
                cone=((-1.0, 0.0, 0.0), 2.0), rate_expr='90.0*' + RAMP, fade_in=6.0), 5000)])

# ---- round 6, item 1b (Ryan): debris streaking past during the burn, and the same look for the EXIT (take-off to space).
# Everything is ship-local (+X = the nose), so one set of systems serves both directions as long as the nose points along
# the flight path. User.Intensity 0..1 scales every rate and brightness; nothing needs the velocity vector.
def entry_debris():
    return [offset(shed('Sparks', 'M_TD_VFX_Spark', 0, (0.20, 0.45), (5, 11), (7000, 15000), k=1.7, radius=420.0, drag=0.0, local=True, stretch=16,
                        cone=((-1.0, 0.0, 0.0), 7.0), rate_expr='170.0*' + RAMP, fade_in=8.0), 420),
            offset(shed('HullSparks', 'M_TD_VFX_Spark', 0, (0.15, 0.40), (4, 8), (2500, 7000), k=1.6, radius=160.0, drag=0.6, local=True, stretch=12,
                        cone=((-1.0, 0.0, 0.0), 26.0), rate_expr='70.0*' + RAMP), 150),
            # dark ash: translucent near-black streaks, so the debris still reads when the screen ramps to white
            offset(shed('Ash', 'M_TD_VFX_Smoke', 0, (0.22, 0.45), (7, 16), (6000, 13000), radius=520.0, drag=0.0, local=True, stretch=7, rgb=(0.05, 0.04, 0.04), alpha=0.85,
                        cone=((-1.0, 0.0, 0.0), 6.0), rate_expr='130.0*' + RAMP, power=0.6, fade_in=8.0), 450),
            # ice / ablation flecks: pale blue-white points tumbling back
            offset(shed('Ice', 'M_TD_VFX_Glow', 0, (0.25, 0.55), (6, 15), (4000, 10000), k=1.0, radius=380.0, drag=0.2, curl=1200.0, local=True, rgb=(0.75, 0.92, 1.0),
                        cone=((-1.0, 0.0, 0.0), 9.0), rate_expr='60.0*' + RAMP, power=1.5), 380)]


S2('NS_TD_Entry_Debris', U(ENTRY, 1.0, Intensity=1.0), entry_debris())

# Exit (slot wanted from T1, e.g. UTDTravelFX.AtmosphericExit: attached loop on the ship root while it climbs out): the
# same fire, streaks and debris, thinning out over 3 s on their own, or driven down by User.Intensity.
# Exit breakaway (slot wanted, one shot on the ship as it leaves the atmosphere, +X forward): a Mach ring, a flash, and
# embers falling away behind.
S2('NS_TD_Exit_Breakaway', U(ENTRY, 1.0, Intensity=1.0), [
    mesh_flash('Ring', 'SM_TD_VFX_Disc', 'M_TD_VFX_FlipShockCone', '1.0, 22.0, 22.0', 0.55, 1.0, growth='float3(1,1,1)*(0.12 + pow(Particles.NormalizedAge, 0.5)*0.88)', power=1.2),
    flash('Flash', 'M_TD_VFX_Flare', 1500, 0.14, 1.0, growth='0.5 + Particles.NormalizedAge'),
    flash('Glow', 'M_TD_VFX_Glow', 1800, 0.30, 0.5, growth='0.5 + Particles.NormalizedAge*0.7'),
    shed('Embers', 'M_TD_VFX_Glow', 0, (0.6, 1.4), (10, 24), (1500, 6000), k=1.3, radius=250.0, drag=1.2, curl=1200.0, cone=((-1.0, 0.0, 0.0), 30.0), burst=70, dur=1.6, power=2.0),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.3, 0.8), (5, 11), (3000, 9000), k=1.6, radius=200.0, drag=1.5, stretch=16, cone=((-1.0, 0.0, 0.0), 35.0), burst=50, dur=1.0)])

# ---- round 7: the entry as a shaped event at chase-camera scale (Lead: "piercing the atmosphere").
def tint(e, rgb, k=1.0):
    """Give a persistent emitter a fixed colour instead of User.Color."""
    e['set'] = [x for x in e['set'] if not (x[1] == INIT and x[2] == 'Color')] + [(PS, INIT, 'Color', X('float4(%s, %s, %s, 1.0)' % tuple(c * k for c in rgb)))]
    return e


def entry_bow():
    # The bow shock: SM_TD_VFX_BowArc is six soft plasma tubes leaving a point 4.2 m ahead of the nose and sweeping out
    # and back (7 m radius, 15 m long), white-blue at the shock and orange in the wake (M_TD_VFX_BowArc). Two layers at
    # different squash give twelve arcs. Behind the ship, three world-space ribbons are left in the air as it dives:
    # at 36 m/s and 1.3 s of life they are about 45 m of wake from the tail and the wing tips.
    grow_ = 'float3(0.80 + 0.20*%s, (0.55 + 0.45*%s)*%s, (0.55 + 0.45*%s)*%s)*%s'
    wake = []
    for nm, y, w in (('Wake', 0, 80), ('WakeL', -250, 46), ('WakeR', 250, 46)):
        e = trail(nm, w, 1.3, 1.0, rate=70.0, taper=0.55, power=1.3)
        e['set'] = [x for x in e['set'] if x[2] != 'Ribbon Width'] + [(PS, INIT, 'Ribbon Width', X('%s*%s*%s' % (w, SCL, RAMP)))]
        wake.append(offset(e, -150, y, 0) if y else e)
    return [ramped(mesh_single('Bow', 'SM_TD_VFX_BowArc', 'M_TD_VFX_BowArc', '1.0, 1.0, 1.0', 1.0, None,
                               live_scale=grow_ % (RAMP, RAMP, '1.0', RAMP, '1.0', EFLK % 53.1))),
            ramped(mesh_single('Bow2', 'SM_TD_VFX_BowArc', 'M_TD_VFX_BowArc', '1.12, 1.0, 1.0', 0.6, None,
                               live_scale=grow_ % (RAMP, RAMP, '1.30', RAMP, '0.62', EFLK % 87.7))),
            tint(ramped(single('Shock', 'M_TD_VFX_Glow', 380, 1.0, 97.3, offset=(410, 0, 0))), (0.70, 0.88, 1.0), 1.1)] + wake


def entry_sheath():   # replaces the round 6 version: the same fire, larger and longer
    grow_ = 'float3(0.75 + 0.25*%s, 0.80 + 0.20*%s, 0.80 + 0.20*%s)*%s'
    return [ramped(mesh_single('Cap', 'SM_TD_VFX_Flame', 'M_TD_VFX_Sheath', '5.0, 3.4, 3.4', 1.2, None, offset=(-120, 0, 0),
                               live_scale=grow_ % (RAMP, RAMP, RAMP, EFLK % 87.7))),
            ramped(single('Nose', 'M_TD_VFX_Glow', 230, 0.8, 71.3, offset=(270, 0, 0))),
            ramped(single('Halo', 'M_TD_VFX_Glow', 1900, 0.10, 13.7, offset=(60, 0, 0))),
            # The fire starts at the TAIL and the wing tips and streams away behind: born at the nose it buried the ship in a
            # cream cloud (seen in play). The hull itself shows T1's HeatGlow.
            offset(spin(shed('Flames', 'M_TD_VFX_Fireball', 0, (0.40, 0.80), (170, 320), (2200, 5200), radius=200.0, drag=0.5, curl=2200.0, local=True, rgb=(1.0, 0.60, 0.34),
                             cone=((-1.0, 0.0, 0.0), 13.0), growth='0.6 + Particles.NormalizedAge*1.8', rate_expr='120.0*' + RAMP)), -260),
            offset(spin(shed('WingFlames', 'M_TD_VFX_Fireball', 0, (0.35, 0.65), (110, 220), (2200, 4800), radius=330.0, drag=0.5, curl=1800.0, local=True, rgb=(1.0, 0.60, 0.34),
                             cone=((-1.0, 0.0, 0.0), 9.0), growth='0.6 + Particles.NormalizedAge*1.6', rate_expr='90.0*' + RAMP)), -120)]


S2('NS_TD_Entry_BowShock', U(ENTRY, 1.0, Intensity=1.0), entry_bow())
S2('NS_TD_Entry_PlasmaSheath', U(ENTRY, 1.0, Intensity=1.0), entry_sheath())
S2('NS_TD_Entry', U(ENTRY, 1.0, Intensity=1.0), entry_cone()[:1] + entry_cone()[2:] + entry_bow() + entry_sheath() + entry_streaks()[:2] + entry_debris())
_RAMP_IN = RAMP
RAMP = '(User.Intensity*saturate(1.0 - Emitter.Age/3.0))'
S2('NS_TD_Exit', U(ENTRY, 1.0, Intensity=1.0), entry_bow() + entry_sheath()[1:] + entry_streaks()[:2] + entry_debris())
RAMP = _RAMP_IN

# ============================================================================================= Rift's dorsal ring (round 7)
# Slot wanted: a looped attached cue on UTDShipVisuals at the socket "Portal" (ring centre, +X along the ring's axis),
# with the floats User.Power (0.5 idle, 1 while the portal blaster is open) and User.Radius (cm, the ring's inner radius).
RPOW = '(0.35 + 0.65*User.Power)'
def in_plane(e):
    """Spawn on a ring in the plane across the system's +X (the ring's own plane) instead of on a sphere."""
    e['set'] = [(PS, 'ShapeLocation', 'Shape Primitive', E('Ring'))] + [x for x in e['set'] if x[2] != 'Sphere Radius'] + [(PS, 'ShapeLocation', 'Ring Radius', X('User.Radius*User.Scale'))]
    e['verbose_dump'] = True
    return e


S2('NS_TD_Rift_RingIdle', U(RIFT, 1.0, Power=0.5, Radius=44.0), [
    looping(mesh_single('Swirl', 'SM_TD_VFX_Disc', 'M_TD_VFX_Swirl', '1.0, User.Radius/50.0, User.Radius/50.0', '(0.30 + 0.60*User.Power)', None), 1.6),   # brighter washed the magenta out to white
    tint(looping(mesh_single('SwirlV', 'SM_TD_VFX_Disc', 'M_TD_VFX_Swirl', '1.0, User.Radius/72.0, User.Radius/72.0', 1.0, None), 1.1), (0.50, 0.20, 1.0), 0.6),
    single('Core', 'M_TD_VFX_Glow', '(User.Radius*(0.55 + 0.75*User.Power)*(0.90 + 0.10*sin(Engine.Time*4.0)))', 1.1, 31.7),
    in_plane(shed('Motes', 'M_TD_VFX_Glow', 0, (0.45, 0.85), (1.6, 4.2), ('-1.7*User.Radius', '-0.9*User.Radius'), k=1.3, radius='User.Radius', drag=0.0, curl=40.0, local=True,
                  rate_expr='(18.0 + 70.0*User.Power)', power=0.7, fade_in=5.0)),
    in_plane(shed('Streaks', 'M_TD_VFX_Spark', 0, (0.30, 0.55), (0.8, 1.6), ('-2.6*User.Radius', '-1.6*User.Radius'), k=1.5, radius='(User.Radius*1.05)', drag=0.0, local=True, stretch=14,
                  rate_expr='(10.0 + 50.0*User.Power)', power=0.6, fade_in=5.0, rgb=(0.65, 0.35, 1.0))),
    spin(shed('Arcs', 'M_TD_VFX_FlipArc', 0, (0.08, 0.2), ('User.Radius*0.9', 'User.Radius*1.5'), (0, 0), k=1.3, radius='(User.Radius*0.3)', drag=0.0, power=0.6, local=True,
              rate_expr='(2.0 + 12.0*User.Power)'))])

# ============================================================================================= pickups (round 8, for T2)
# Four looping idles and a collect burst each, built to a 1 m core, Z up. Colours stay clear of the squad's magenta / cyan /
# amber and the enemy's red / orange; each has its own silhouette so it reads without colour:
#   Hull (green)        a cross          Shield (white-blue)   a hexagon ripple in a bubble
#   Power (violet)      an upright crystal with a swirl        Overcharge (gold-white)  a spiked star throwing sparks
# Slots: whatever T2 names them (a looped attached cue per pickup type and a one-shot cue at the pickup on collect).
PK_HULL = (0.22, 1.0, 0.36, 1.0)
PK_SHIELD = (0.72, 0.90, 1.0, 1.0)
PK_POWER = (0.55, 0.22, 1.0, 1.0)
PK_OVER = (1.0, 0.86, 0.50, 1.0)
PULSE = '(0.85 + 0.15*sin(Engine.Time*%s))'


def upright(e):
    """Turn a persistent mesh emitter's +X to the system's +Z (mesh renderer Velocity facing, a token velocity straight up)."""
    e['add'] = [(PS, 'Spawn/Velocity/AddVelocity')] + e['add']
    e['set'] += [(PS, 'AddVelocity', 'Velocity', (0.0, 0.0, 2.0))]
    e['rend']['FacingMode'] = 'Velocity'
    return e


def pk_common(halo=230):
    return [single('Halo', 'M_TD_VFX_Glow', '(%s*%s)' % (halo, PULSE % 2.6), 0.32, None),
            shed('Motes', 'M_TD_VFX_Glow', 14.0, (0.8, 1.6), (2.5, 6.0), (10, 60), k=1.2, radius=70.0, drag=0.5, curl=120.0, local=True, power=1.2, fade_in=4.0)]


def pk_collect(extra=()):
    return [flash('Flash', 'M_TD_VFX_Flare', 320, 0.10, 1.3, growth='0.5 + Particles.NormalizedAge'),
            flash('Glow', 'M_TD_VFX_Glow', 420, 0.22, 0.6, growth='0.5 + Particles.NormalizedAge*0.8'),
            flash('Ring', 'M_TD_VFX_Shock', 520, 0.32, 1.1, growth='0.10 + pow(Particles.NormalizedAge, 0.5)*0.90', power=1.3),
            shed('Sparks', 'M_TD_VFX_Spark', 0, (0.20, 0.50), (2.5, 5.5), (500, 2200), k=1.7, radius=20.0, drag=2.6, stretch=12, burst=30, dur=0.6),
            shed('Motes', 'M_TD_VFX_Glow', 0, (0.5, 1.1), (4, 10), (80, 520), k=1.2, radius=40.0, drag=1.8, curl=500.0, burst=22, dur=1.2, power=1.6)] + list(extra)


# Hull repair: a green cross (two bars facing the camera), a hot centre, motes rising.
S2('NS_TD_Pickup_Hull', U(PK_HULL), pk_common() + [
    single('BarH', 'M_TD_VFX_Glow', ('(118*%s)' % (PULSE % 3.1), 40), 1.5, None),
    single('BarV', 'M_TD_VFX_Glow', (40, '(118*%s)' % (PULSE % 3.1)), 1.5, None),
    single('Core', 'M_TD_VFX_Glow', 46, 1.6, 23.7),
    shed('Rise', 'M_TD_VFX_Spark', 10.0, (0.6, 1.1), (2.0, 4.0), (60, 150), k=1.4, radius=45.0, drag=0.0, local=True, stretch=7, cone=((0.0, 0.0, 1.0), 6.0), power=1.0, fade_in=5.0)])
S2('NS_TD_Pickup_Hull_Collect', U(PK_HULL), pk_collect([
    flash('BarH', 'M_TD_VFX_Glow', 1, 0.28, 1.6, growth='float2(4.2, 0.9)*(0.4 + Particles.NormalizedAge*0.9)*60.0', power=1.0),
    flash('BarV', 'M_TD_VFX_Glow', 1, 0.28, 1.6, growth='float2(0.9, 4.2)*(0.4 + Particles.NormalizedAge*0.9)*60.0', power=1.0)]))
# Shield cell: a hexagon ripple that keeps spreading inside a faint bubble, with a thin rim.
S2('NS_TD_Pickup_Shield', U(PK_SHIELD), pk_common(210) + [
    looping(single('Hex', 'M_TD_VFX_Hex', 128, 0.9, None), 1.3),
    single('Rim', 'M_TD_VFX_Ring', 150, 0.6, None),
    shed('Glints', 'M_TD_VFX_Glow', 10.0, (0.3, 0.6), (3.0, 7.0), (0, 10), k=1.4, radius=56.0, drag=0.0, local=True, power=1.0, fade_in=4.0)])   # first build (a sphere shell and a Shock rim) never activated
S2('NS_TD_Pickup_Shield_Collect', U(PK_SHIELD), pk_collect([
    flash('Hex', 'M_TD_VFX_Hex', 420, 0.35, 1.2, growth='0.35 + Particles.NormalizedAge*0.65', power=0.8),
    shell_flash('Bubble', 60, 0.35, 'float3(1,1,1)*(1.0 + 2.2*pow(Particles.NormalizedAge, 0.6))', k=0.9, power=1.2)]))
# Power cell: an upright violet crystal (the bolt spindle stood on end) inside a turning swirl, small arcs.
S2('NS_TD_Pickup_Power', U(PK_POWER), pk_common(210) + [
    upright(mesh_single('Crystal', 'SM_TD_VFX_Bolt', 'M_TD_VFX_Bolt', '1.5, 0.55, 0.55', 3.0, 41.3)),
    looping(single('Swirl', 'M_TD_VFX_Swirl', 150, 0.5, None), 2.2),
    arcs('Arcs', 5.0, (40, 80), 30, life=(0.08, 0.2), k=1.2)])
S2('NS_TD_Pickup_Power_Collect', U(PK_POWER), pk_collect([
    arcs('Arcs', 0, (70, 150), 50, life=(0.1, 0.3), k=1.0, burst=10, dur=0.5, local=False),
    spin(flash('Swirl', 'M_TD_VFX_Swirl', 460, 0.4, 1.0, growth='0.3 + Particles.NormalizedAge*0.7', power=0.8))]))
# Overcharge: a spiked gold-white star that throws sparks outward all the time.
S2('NS_TD_Pickup_Overcharge', U(PK_OVER), pk_common(250) + [
    looping(single('Star', 'M_TD_VFX_Flare', '(190*%s)' % (PULSE % 6.3), 1.0, None), 0.5),
    looping(single('Star2', 'M_TD_VFX_Flare', 120, 1.3, None), 0.33),
    single('Core', 'M_TD_VFX_Glow', 60, 1.8, 67.1),
    shed('Sparks', 'M_TD_VFX_Spark', 34.0, (0.25, 0.5), (2.0, 4.5), (160, 420), k=1.6, radius=12.0, drag=1.0, local=True, stretch=11, power=1.0)])
S2('NS_TD_Pickup_Overcharge_Collect', U(PK_OVER), pk_collect([
    flash('Star', 'M_TD_VFX_Flare', 620, 0.16, 1.5, growth='0.4 + Particles.NormalizedAge*0.8'),
    shed('Burst', 'M_TD_VFX_Spark', 0, (0.25, 0.6), (3.0, 6.5), (1200, 3600), k=1.9, radius=10.0, drag=2.2, stretch=16, burst=40, dur=0.7)]))

# Slots: UTDBladeFX.SwingLight / SwingHeavy. The code plays these ATTACHED TO THE CHARACTER ROOT (not the blade tip), so
# a ribbon would trail the body: this is a self-contained crescent in front of the chest instead (camera-facing).
def swing(size, k):
    arc = offset(flash('Arc', 'M_TD_VFX_Slash', size, 0.20, k, power=0.3), 95, 0, 35)
    arc['props'] = {'bLocalSpace': True}
    glint = offset(shed('Glints', 'M_TD_VFX_Glow', 0, (0.12, 0.3), (2, 5), (60, 420), k=1.3, radius=40.0, drag=2.5, local=True, burst=10, dur=0.4, power=2.0), 95, 0, 60)
    return [arc, glint]


S2('NS_TD_Blade_SwingArc', U(BLADE), swing(230, 1.4))
S2('NS_TD_Blade_SwingArc_Heavy', U(BLADE), swing(320, 1.9))

fx2 = tdvfx.Builder(call_tool)
for _name, _user, _emitters in SYSTEMS2:
    _sys = await fx2.system(_name, _emitters, user=_user)
    for _e in _emitters:
        if _e.get('verbose_dump'):
            try:
                fx2.log.append(await fx2.dump(_sys, _e['name'], ['ShapeLocation']))
            except Exception as _ex:
                fx2.log.append('dump failed ' + str(_ex)[:200])
    await asyncio.sleep(0)
fx2.report()

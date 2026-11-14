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
# Round 8: the shell, the inner shell and the halo fade out when the camera is INSIDE the field (seen in play: from inside
# the field overlaid the whole sky with violet marbling). OUTSIDE = 0 well inside, 1 from the shell outwards; the cue's
# VFXScale scales the radius (Engine.Owner.Scale) and Engine.Owner.LODDistance is the camera's distance to the centre.
OUTSIDE = 'saturate((Engine.Owner.LODDistance - 0.85*100000.0*User.Scale*Engine.Owner.Scale.x)/(0.15*100000.0*User.Scale*Engine.Owner.Scale.x))'


def alpha_by(e, expr):
    """Scale a persistent emitter's particle alpha every frame by expr (keeps an existing flicker on Scale RGB)."""
    if not any(a[1].endswith('ScaleColor') for a in e['add']):   # the flickering halo already has one (Scale RGB)
        e['add'].append((PU, 'Update/Color/ScaleColor'))
        e['set'].append((PU, 'ScaleColor', 'ScaleRGB', False))
    e['set'] = [x for x in e['set'] if x[2] != 'ScaleA'] + [(PU, 'ScaleColor', 'ScaleA', True), (PU, 'ScaleColor', 'Scale Alpha', X(expr))]
    return e


def outside_only(e):
    return alpha_by(e, OUTSIDE)


S2('NS_TD_Dominion_Field', U(FIELD), [
    outside_only(shell('Shell', 100000, 0.45, pulse='float3(1,1,1)*(1.0 + 0.012*sin(Engine.Time*1.7))')),
    outside_only(shell('Inner', 97000, 0.15, pulse='float3(1,1,1)*(1.0 + 0.015*sin(Engine.Time*1.1 + 2.0))')),
    outside_only(single('Halo', 'M_TD_VFX_Glow', 330000, 0.10, 3.1)),
    arcs('Arcs', 26.0, (20000, 42000), 100000, life=(0.25, 0.6), k=0.9)])
# Slot: UTDDominionVisuals.ShieldCollapse (one shot at the centre when the last generator dies). 1 km radius at Scale 1.
S2('NS_TD_Dominion_FieldCollapse', U(FIELD), [
    shell_flash('Flare', 100000, 0.5, 'float3(1,1,1)*(1.0 + 0.06*Particles.NormalizedAge)', k=1.5, power=0.6),
    shell_flash('Implode', 100000, 1.3, 'float3(1,1,1)*(1.0 - 0.92*pow(Particles.NormalizedAge, 1.6))', k=1.6, power=0.4, delay=0.35),
    flash('Wave', 'M_TD_VFX_Ring', 520000, 1.6, 0.9, growth='0.08 + pow(Particles.NormalizedAge, 0.6)*0.92', power=1.5, delay=1.5),
    flash('Flash', 'M_TD_VFX_Glow', 300000, 0.5, 0.8, growth='0.4 + Particles.NormalizedAge*0.8', power=1.5, delay=1.5),
    arcs('Arcs', 0, (14000, 30000), 95000, life=(0.3, 1.2), k=0.9, burst=40, dur=1.6, local=False),   # seen in play: bigger arcs whited out the screen near a generator
    shed('Shards', 'M_TD_VFX_Spark', 0, (0.8, 1.8), (500, 1100), (20000, 90000), k=1.6, radius=95000.0, drag=0.8, stretch=22, burst=140, dur=2.0, power=1.2)])
# ---- round 10 (Lead's 03:00 audit, ENG-37): the approach field must read "cannot pass yet"; the shield drop gets its own
# one shot. New names (the round-8 systems stay for ART-HISTORY).
# Slot: UTDDominionVisuals.DistortionFieldIdle (the approach field; TDDistortionField scales VFXScale by radius / 1 km).
# A hex lattice (about 100 m cells at 1 km) over the old marbled shell, with a slow bright band sweeping pole to pole,
# so the barrier reads as a wall, not weather. Outside only (from inside, the old lesson: no violet sky).
S2('NS_TD_Dominion_ApproachField', U(FIELD), [
    outside_only(shell('Shell', 100000, 0.40, pulse='float3(1,1,1)*(1.0 + 0.012*sin(Engine.Time*1.7))')),
    # 10b: FieldHex, not DomeHex (DomeHex shows its lattice only at the rim)
    outside_only(shell('Lattice', 100400, 0.85, mat='M_TD_VFX_FieldHex2', pulse='float3(1,1,1)*(1.0 + 0.006*sin(Engine.Time*2.3 + 1.0))')),
    outside_only(single('Halo', 'M_TD_VFX_Glow', 330000, 0.10, 3.1)),
    arcs('Arcs', 34.0, (20000, 42000), 100000, life=(0.25, 0.6), k=1.0)])
# Slot: UTDDominionVisuals.ShieldCollapse (the 4th generator dies; one shot at the centre). Authored at 1 km radius
# (the asset's VFXScale 0.46 makes it the shield's 460 m). 0 s the lattice flashes white-violet; 0.25 s arcs race over it;
# 0.5 s the shell shatters outward into streaking shards with a ring wave; 1.4 s a last inward flash where it was.
S2('NS_TD_Dominion_ShieldDrop', U(FIELD), [
    shell_flash('Flash', 100500, 0.45, 'float3(1,1,1)*(1.0 + 0.02*Particles.NormalizedAge)', k=2.6, mat='M_TD_VFX_DomeHex', power=0.7),
    shell_flash('Glow', 100000, 0.6, 'float3(1,1,1)*(1.0 + 0.05*Particles.NormalizedAge)', k=1.6, power=0.6),
    shell_flash('Break', 100800, 0.9, 'float3(1,1,1)*(1.0 + 0.35*pow(Particles.NormalizedAge, 0.7))', k=1.8, mat='M_TD_VFX_DomeHex', power=1.3, delay=0.45),
    arcs('Arcs', 0, (16000, 34000), 98000, life=(0.2, 0.55), k=1.3, burst=60, dur=0.9, local=False),
    flash('Wave', 'M_TD_VFX_Ring', 300000, 1.4, 1.0, growth='0.6 + pow(Particles.NormalizedAge, 0.6)*0.8', power=1.4, delay=0.5),
    shed('Shards', 'M_TD_VFX_Spark', 0, (0.9, 1.8), (600, 1300), (25000, 80000), k=1.8, radius=99000.0, drag=0.6, stretch=24, burst=180, dur=2.4, power=1.2),
    flash('Core', 'M_TD_VFX_Flare', 120000, 0.35, 1.2, growth='0.3 + Particles.NormalizedAge', power=1.0, delay=1.4)])
# ART-18 Aftermath (T4 spawns it; no data slot): a row of 12 exterior lights on the Dominion, 400 m long along +Y, facing
# +X, pulsing in the HUD navigation pulse's rhythm. User.Pulse 0..1 is fed each frame by T4 from the HUD pulse; while it is
# below 0 (never set) an internal three-pulse cycle runs (0 / 0.45 / 0.9 s, the third held, every 3 s). The ripple along
# the row is 30 ms per light. Every light is re-spawned 20 times a second (no persistent sprite), so Deactivate fades it
# in 0.1 s. Sizes grow with the camera distance (x1 at 1 km, x4 at 4 km and beyond).
ROWD = 'clamp(Engine.Owner.LODDistance/60000.0, 1.0, 5.0)'   # 10b: unseen at 1.5 km at /100000


def row_pulse(lag):
    t = 'frac((Engine.Time - %s)/3.0)*3.0' % lag
    internal = '(saturate(1.0 - abs(%s - 0.10)/0.10) + saturate(1.0 - abs(%s - 0.55)/0.10) + saturate(1.0 - abs(%s - 1.15)/0.25))' % (t, t, t)
    return 'lerp(%s, User.Pulse, step(0.0, User.Pulse))' % internal


def row_light(i, n=12, length=40000.0):
    y = -length / 2 + length * i / (n - 1)
    p = row_pulse(0.03 * i)
    return offset(shed('Light%02d' % i, 'M_TD_VFX_Glow', 0, (0.08, 0.08), ('(2700.0 + 6300.0*%s)*%s' % (p, ROWD), '(2700.0 + 6300.0*%s)*%s' % (p, ROWD)), (0, 0),   # 10c: 1.5x (Lead)
                       k=2.4, radius=0.0, drag=0.0, local=True, power=0.5, rate_expr='20.0'), 300.0, y)


# Slot: UTDDominionVisuals.TractorBeam (T4: a loop at the bay mouth, +X out of the bay toward the ship; User.Length cm
# each frame, User.Intensity 0..1). A pale-blue cone from a 60 m mouth narrowing toward the ship, motes pulled in from
# the ship's end toward the bay, and a soft glow in the mouth.
S2('NS_TD_Dominion_TractorBeam', U((0.35, 0.65, 1.0, 1.0), 1.0, Length=20000.0, Intensity=1.0), [
    # 10d: the flame-mesh cone read as a white exhaust jet; the beam is now a column of big soft blue glows streaming from
    # the ship's end into the bay (reads as a pull), the motes, and the glow in the mouth.
    # 10e: the column itself (the tapered flame mesh with the rim-lit field material: a soft blue tube, no white core)
    mesh_single('Column', 'SM_TD_VFX_Flame', 'M_TD_VFX_Field', '1.0, 30.0, 30.0', 1.2, None,
                live_scale='float3(User.Length/(100.0*User.Scale), 1.0, 1.0)*User.Intensity'),
    offset(shed('Stream', 'M_TD_VFX_Glow', 0, (2.5, 4.0), (1400, 2600), (3500, 5000), k=4.0, radius=600.0, drag=0.0, local=True,
                cone=((-1.0, 0.0, 0.0), 2.0), rate_expr='40.0*User.Intensity', power=0.7, fade_in=5.0), 'User.Length/User.Scale'),
    offset(shed('Motes', 'M_TD_VFX_Glow', 0, (1.2, 2.4), (60, 140), (4000, 7000), k=1.4, radius=900.0, drag=0.0, local=True, stretch=None,
                cone=((-1.0, 0.0, 0.0), 6.0), rate_expr='70.0*User.Intensity', power=0.8, fade_in=4.0), 'User.Length/User.Scale'),
    shed('Mouth', 'M_TD_VFX_Glow', 0, (0.3, 0.3), (5000, 6000), (0, 0), k=0.5, radius=0.0, drag=0.0, local=True, power=0.6, rate_expr='12.0*User.Intensity')])

# Review only (not for gameplay): M_TD_RegionHaze on a 600 m engine sphere as a mesh particle, so the haze can be
# photographed in PIE (Python cannot add a mesh component there). T6 uses a plain sphere actor.
# 10d: a system of persistent emitters only never activates (round 3's trap, seen again: active=False); a token spawner keeps it alive
def keepalive():
    return shed('KeepAlive', 'M_TD_VFX_Glow', 0, (0.1, 0.1), (1, 1), (0, 0), k=0.0, radius=0.0, drag=0.0, local=True, rate_expr='2.0')
S2('NS_TD_Debug_HazeSphere', U((1.0, 1.0, 1.0, 1.0), 1.0), [shell('Sphere', 30000, 1.0, mat='MI_TD_RegionHaze_Review'), keepalive()])

# 10d debug (review only): why the region beacon and the haze never show. Same emitter with (A) a fixed 8.4 m size,
# (B) the size straight from Engine.Owner.LODDistance (0 if the engine does not fill it); the haze sphere with the
# known-good field material, and with the haze base material (not the instance).
def dbg_beacon(size_expr):
    return [shed('Beacon', 'M_TD_VFX_Glow', 0, (0.1, 0.1), (size_expr, size_expr), (0, 0), radius=0.0, drag=0.0, local=True, power=0.01, k=2.5, rate_expr='20.0')]
S2('NS_TD_Debug_BeaconFixed', U((1.0, 0.62, 0.18, 1.0), 1.0), dbg_beacon('840.0'))
S2('NS_TD_Debug_BeaconLOD', U((1.0, 0.62, 0.18, 1.0), 1.0), dbg_beacon('(0.028*Engine.Owner.LODDistance)'))
S2('NS_TD_Debug_HazeField', U((0.70, 0.85, 1.0, 1.0), 1.0), [shell('Sphere', 30000, 1.0)])
S2('NS_TD_Debug_HazeBase', U((1.0, 1.0, 1.0, 1.0), 1.0), [shell('Sphere', 30000, 1.0, mat='M_TD_RegionHaze'), keepalive()])

# 10f debug (one lock turn): is a lone sprite at the system origin culled by degenerate dynamic bounds? (The light row and
# the plume beacons sit beside other geometry and show; the region beacon and BeaconFixed, alone at the origin, never did.)
def fixed_bounds(e, half=5000.0):
    e['props'].update({'CalculateBoundsMode': 'Fixed', 'FixedBounds': {'min': {'x': -half, 'y': -half, 'z': -half}, 'max': {'x': half, 'y': half, 'z': half}, 'isValid': True}})
    return e
S2('NS_TD_Debug_BeaconFixedB', U((1.0, 0.62, 0.18, 1.0), 1.0), [fixed_bounds(dbg_beacon('840.0')[0])])
S2('NS_TD_Debug_GlowOffset', U((1.0, 0.62, 0.18, 1.0), 1.0), [offset(dbg_beacon('840.0')[0], 300.0)])
S2('NS_TD_Debug_RangeA', U((1.0, 0.62, 0.18, 1.0), 1.0), [fixed_bounds(shed('Beacon', 'MI_TD_VFX_RangeGlow_Plume', 0, (0.1, 0.1), (1520.0, 1520.0), (0, 0), radius=0.0, drag=0.0, local=True, power=0.01, k=8.0, rate_expr='20.0'))])
S2('NS_TD_Debug_Haze2', U((1.0, 1.0, 1.0, 1.0), 1.0), [shell('Sphere', 30000, 1.0, mat='M_TD_RegionHaze2'), keepalive()])

# ---- round 10j (T4's play, #105 / #111): the tractor beam seen from BEHIND the ship (the chase camera looks down the beam,
# so v1 read as a faint ring at the bay mouth). v2, new asset (v1 kept): a wide cone opening toward the player (the tapered
# mesh placed at the ship end, pointing back into the bay), bright motes and streaks spawned around the ship streaming past
# the camera into the bay, rings travelling down the cone and shrinking with it, and a soft blue tint around the ship.
# Same slot and parameters (UTDDominionVisuals.TractorBeam, +X out of the bay, User.Length cm, User.Intensity).
TB2 = (0.40, 0.75, 1.0, 1.0)


def tb_ring():
    e = shed('Rings', 'M_TD_VFX_Ring', 0, (2.6, 2.6), (1, 1), (3800, 3800), k=1.6, radius=0.0, drag=0.0, local=True,
             cone=((-1.0, 0.0, 0.0), 0.0), rate_expr='1.4*(0.45 + 0.55*User.Intensity)', power=0.8, fade_in=6.0)
    meshed(e, 'SM_TD_VFX_Disc', 'M_TD_VFX_Ring')
    e['set'] += [(PS, INIT, 'Mesh Scale Mode', E('Non-Uniform')), (PS, INIT, 'Mesh Scale', X('float3(1.0, 46.0, 46.0)*User.Scale')),
                 (PU, 'ScaleMeshSize', 'Scale Factor', X('float3(1.0, 1.0, 1.0)*(1.0 - 0.75*Particles.NormalizedAge)'))]
    e['add'] = e.get('add', []) + [(PU, 'Update/Size/ScaleMeshSize')]
    return offset(e, 'User.Length/User.Scale')


# 10m (dock capture: v2 did not show): TDDockingBay::UpdateBeam attaches the cue at the bay root (the mouth, +X outward,
# world scale 1) and sets User.Length (ship to mouth, cm) and User.Intensity = the PULL, which is low until the ship is
# close; so every part now runs at (0.45 + 0.55 x Intensity), the cone uses the flame material (glows through its body,
# readable from inside it; the rim-lit field material vanished end-on), and the streaks and motes are 3-4x larger.
TBI = '(0.45 + 0.55*User.Intensity)'
S2('NS_TD_Dominion_TractorBeam2', U(TB2, 1.0, Length=20000.0, Intensity=1.0), [
    offset(mesh_single('Cone', 'SM_TD_VFX_Flame', 'M_TD_VFX_Flame', '1.0, 55.0, 55.0', 0.32, None,
                       live_scale='float3(-User.Length/(100.0*User.Scale), 1.0, 1.0)*' + TBI), 'User.Length/User.Scale'),
    tb_ring(),
    offset(shed('Streaks', 'M_TD_VFX_Spark', 0, (0.5, 0.9), (60, 110), (7000, 11000), k=3.0, radius=2400.0, drag=0.0, local=True, stretch=20,
                rgb=(0.7, 0.9, 1.0), cone=((-1.0, 0.0, 0.0), 4.0), rate_expr='70.0*' + TBI, fade_in=6.0), 'User.Length/User.Scale + 1500.0'),
    offset(shed('Motes', 'M_TD_VFX_Glow', 0, (1.2, 2.2), (300, 600), (4000, 7000), k=2.4, radius=2800.0, drag=0.0, local=True,
                cone=((-1.0, 0.0, 0.0), 5.0), rate_expr='45.0*' + TBI, power=0.8, fade_in=4.0), 'User.Length/User.Scale + 1000.0'),
    offset(shed('Tint', 'M_TD_VFX_Glow', 0, (0.4, 0.4), (5500, 6500), (0, 0), k=0.3, radius=0.0, drag=0.0, local=True, power=0.5,
                rate_expr='8.0*' + TBI), 'User.Length/User.Scale'),
    shed('Mouth', 'M_TD_VFX_Glow', 0, (0.3, 0.3), (5000, 6000), (0, 0), k=0.8, radius=0.0, drag=0.0, local=True, power=0.6, rate_expr='12.0*' + TBI)])

# ---- round 10j (T4's play): the Maul bomber's plasma mine (was the placeholder disc). Slot: the Tracer of
# DA_Enemy_PlasmaMines_FX (a persistent system on each pooled mine; with a Tracer and no ProjectileMesh the placeholder hides).
# A dark spiked core (six tumbling shards in the cooling-debris material, near black with red edges), a red light pulsing
# with a double arming blink every 1.2 s, and a halo that grows with the camera distance so a mine reads at 300 m.
MINE = (1.0, 0.12, 0.03, 1.0)
MBLINK = '(0.25 + 0.75*saturate(1.0 - abs(frac(Engine.Time/1.2)*1.2 - 0.06)/0.05) + 0.75*saturate(1.0 - abs(frac(Engine.Time/1.2)*1.2 - 0.24)/0.05))'


def mine_spike(i):
    d = [(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1)][i]
    return mesh_single('Spike%d' % i, 'SM_TD_VFX_Shard_A', 'M_TD_VFX_Debris', '1.4, 0.7, 0.7', 1.0, None, offset=(70.0 * d[0], 70.0 * d[1], 70.0 * d[2]))


S2('NS_TD_Mine_Plasma', U(MINE, 1.0), [
    single('Core', 'M_TD_VFX_Glow', 260, 0.35, None),
    shed('Light', 'M_TD_VFX_Glow', 0, (0.1, 0.1), ('(240.0*%s)' % MBLINK,) * 2, (0, 0), k=3.0, radius=0.0, drag=0.0, local=True, power=0.6, rate_expr='20.0'),
    shed('Halo', 'M_TD_VFX_Glow', 0, (0.1, 0.1), ('(500.0 + 0.03*clamp(Engine.Owner.LODDistance, 0.0, 60000.0))*%s' % MBLINK,) * 2, (0, 0), k=1.6,
         radius=0.0, drag=0.0, local=True, power=0.6, rate_expr='20.0')] + [mine_spike(i) for i in range(6)])

# ---- round 10n (Lead): (a) the Dominion turret's 1.0 s lock telegraph before each burst (T4 adds the timing; slot:
# the turret's telegraph cue, e.g. UTDEnemyVisuals.TurretTelegraph / T4's field). One shot at the muzzle, +X = the aim:
# a red-orange glint that charges up over 1.0 s and snaps white at the end, with a dashed aim line racing out along +X.
TLK = (1.0, 0.28, 0.06, 1.0)
TDIST = 'clamp(Engine.Owner.LODDistance/6000.0, 1.0, 4.0)'   # 10o: the head-on read grows with distance (x4 by 240 m)


def tele_line():
    e = shed('AimLine', 'M_TD_VFX_Spark', 0, (0.12, 0.18), (22, 34), (30000, 36000), k=2.6, radius=0.0, drag=0.0, local=True, stretch=40,
             cone=((1.0, 0.0, 0.0), 0.3), rate_expr='26.0')
    e['set'] = [x for x in e['set'] if x[2] != 'Loop Behavior'] + [(EU, 'EmitterState', 'Loop Behavior', E('Once')), (EU, 'EmitterState', 'Loop Duration', 1.0)]
    return e


S2('NS_TD_Turret_LockTelegraph', U(TLK, 1.0), [
    flash('Charge', 'M_TD_VFX_Glow', '(1200.0*%s)' % TDIST, 1.0, 2.2, growth='0.25 + 0.75*pow(Particles.NormalizedAge, 1.5)', power=0.2),
    flash('Core', 'M_TD_VFX_Flare', '(520.0*%s)' % TDIST, 1.0, 2.6, growth='0.2 + 0.8*Particles.NormalizedAge', power=0.3),
    flash('Snap', 'M_TD_VFX_Flare', '(1400.0*%s)' % TDIST, 0.12, 2.8, growth='0.6 + Particles.NormalizedAge', delay=0.95),
    tele_line()])

# (b) the "switch grace" after an auto-switch on death (T2 adds the 1.5 s grace): a hex shield shimmer wrapped round the
# squad ship in the pilot's colour (User.Color: Rift magenta, Lancer cyan, Bastion amber), flashing in, pulsing twice and
# fading out over 1.5 s. One shot attached at the ship root; authored for a 6 m ship at Scale 1.
S2('NS_TD_Ship_SwitchGrace', U((1.0, 0.10, 0.85, 1.0), 1.0), [
    # 10o: 5.2 m read as a big flat bubble from the chase camera: 3.3 m, stronger hex, less fill
    shell_flash('Hex', 330, 1.5, 'float3(1,1,1)*(1.0 + 0.04*sin(Particles.NormalizedAge*12.566))', k=2.2, mat='M_TD_VFX_DomeHex', power=1.6),
    shell_flash('Glow', 320, 1.5, 'float3(1,1,1)*(1.0 + 0.02*Particles.NormalizedAge)', k=0.22, power=2.0),
    flash('In', 'M_TD_VFX_Glow', 1000, 0.25, 1.0, growth='0.6 + Particles.NormalizedAge', power=1.2),
    shed('Glints', 'M_TD_VFX_Glow', 0, (0.2, 0.4), (40, 80), (0, 60), k=3.0, radius=320.0, drag=1.0, local=True, burst=40, dur=1.5, power=1.0)])

S2('NS_TD_Dominion_LightRow', U((0.65, 0.92, 1.0, 1.0), 1.0, Pulse=-1.0), [row_light(i) for i in range(12)])

# ---- round 10 MAJOR (T7's play): Cinder magma kills but does not read as dangerous on foot. A looping hazard layer for
# each ATDMagmaVolume, attached at its root (the surface; +Z up). User.SizeX / User.SizeY = the footprint in cm (the Size
# passed to SetSize), User.Intensity 0..1. Every rate scales with the area, clamped, so a 4 x 4 m pool and a 60 x 60 m lake
# both stay cheap: embers rising (tiny additive points), lava spatter arcing up and falling back (bright blobs with
# gravity), and a few small rising heat shimmers (additive, small: no big translucent sheets, little overdraw).
# Spawn positions: a sphere of radius half the shorter side, at the surface (the half below it is hidden by the surface).
MAREA = '(User.SizeX*User.SizeY/10000.0)'
MRAD = '(0.5*min(User.SizeX, User.SizeY))'


def magma_rate(per_m2, lo, hi):
    return 'clamp(%s*%s, %s, %s)*User.Intensity' % (MAREA, per_m2, lo, hi)


def with_gravity(e, g=-980.0):
    e['set'] = [x for x in e['set'] if x[1] != 'GravityForce'] + [(PU, 'GravityForce', 'Gravity', (0.0, 0.0, g))]
    return e


S2('NS_TD_Magma_Heat', U((1.0, 0.42, 0.08, 1.0), 1.0, SizeX=800.0, SizeY=800.0, Intensity=1.0), [
    # 10d (captured on foot at 5 and 20 m: off and on looked identical against the bright lava): 3-4x larger, HDR-bright
    shed('Embers', 'M_TD_VFX_Glow', 0, (1.2, 2.6), (20, 45), (80, 260), k=2.2, radius=MRAD, drag=0.4, curl=90.0, local=True,
         rgb=(9.0, 3.6, 0.6), cone=((0.0, 0.0, 1.0), 25.0), rate_expr=magma_rate(0.25, 6.0, 70.0), power=1.4, fade_in=6.0),
    # 10e (Lead: at 5 m the streaks crossed the whole view): size and count about 40% down, still arcs at 20 m
    with_gravity(shed('Spatter', 'M_TD_VFX_Spark', 0, (0.6, 1.0), (24, 48), (450, 900), k=2.6, radius=MRAD, drag=0.1, local=True, stretch=4,
                      rgb=(12.0, 5.0, 0.8), cone=((0.0, 0.0, 1.0), 30.0), rate_expr=magma_rate(0.06, 3.0, 24.0), power=0.8)),   # 10f: count back up (size stays) so the arcs read at 20 m
    shed('Shimmer', 'M_TD_VFX_Fire', 0, (0.8, 1.4), (90, 180), (60, 140), k=0.35, radius=MRAD, drag=0.6, local=True,
         cone=((0.0, 0.0, 1.0), 10.0), growth='0.6 + Particles.NormalizedAge*0.8', rate_expr=magma_rate(0.02, 2.0, 12.0), power=1.0, fade_in=4.0)])

# round 10 (Lead, for T6): a set piece's range beacon at its beacon point. One emitter, one particle re-spawned 10 times a
# second (Init-only size: no persistent sprite, no Update ScaleColor). About 0.8 degrees across (core), held between 300 m
# and 3 km from the camera (closer it stops shrinking with distance, so it never covers the piece); 0.4 Hz pulse in size
# and brightness. User.Color per region: Watchline amber (1.0, 0.62, 0.18), Quiet Graveyard (0.70, 0.85, 1.0), Survey
# Scar Null red (1.0, 0.10, 0.04), Shattered Moons ember orange (1.0, 0.38, 0.08).
BPULSE = '(0.80 + 0.20*sin(Engine.Time*2.5133))'
BSIZE = '(0.06*clamp(Engine.Owner.LODDistance, 30000.0, 300000.0))'   # 10g: Glow again, sprite about 3.4 degrees (core about 1 degree), held between 300 m and 3 km
S2('NS_TD_Region_Beacon', U((1.0, 0.62, 0.18, 1.0), 1.0), [
    shed('Beacon', 'M_TD_VFX_Glow', 0, (0.1, 0.1), ('%s*%s' % (BSIZE, BPULSE), '%s*%s' % (BSIZE, BPULSE)), (0, 0), radius=0.0, drag=0.0, local=True, power=0.6,
         k=12.0, rate_expr='20.0')])   # 10g: the range-glow custom material rendered dark; plain Glow, pulse in the size   # 10e: the 0.4 Hz pulse is in the alpha (the material multiplies it)

# Slot: none yet (code hook wanted: T3's transmissions, the escape sequence, a hit of "static"). A one-shot, camera-facing
# tear of chromatic bars, 0.3 s; spawn it attached in front of the camera (authored 12 m wide) or at a world point.
# Round 9 (T3's slot, UTDStoryFX.TransmissionStatic): played 60 cm in front of the camera, so sized to the view at that
# distance (about 120 cm across at the chase camera's FOV): a full-screen tear, all of it over within 0.36 s.
S2('NS_TD_Glitch_Tear', U(WHITE4), [
    flash('Tear', 'M_TD_VFX_Glitch', 130, 0.30, 0.9, power=0.5),
    flash('Tear2', 'M_TD_VFX_Glitch', 160, 0.22, 0.5, power=0.5, delay=0.06),
    flash('Flash', 'M_TD_VFX_Glow', 170, 0.10, 0.12)])

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
# Round 9 (T7's playtest, T2 traced it to Power2Active): from inside it must read as a faint amber hex shimmer, nearly clear
# in the middle, a thin rim where the dome crosses the screen; from outside a readable dome. One fresnel-driven shell
# (M_TD_VFX_DomeHex) and a few motes. Slot: Bastion UTDShipVisuals.Power2Active (played on the dome actor, DomeRadius 30 m).
S2('NS_TD_ShieldDome', U(BASTION), [
    # inside the dome (camera closer to the centre than the radius) the whole shell drops to 0.35
    alpha_by(shell('Dome', 3000, 0.9, mat='M_TD_VFX_DomeHex', pulse='float3(1,1,1)*(1.0 + 0.008*sin(Engine.Time*3.0))'),
             'lerp(0.35, 1.0, saturate((Engine.Owner.LODDistance - 2850.0*User.Scale*Engine.Owner.Scale.x)/(300.0*User.Scale*Engine.Owner.Scale.x)))'),
    shed('Motes', 'M_TD_VFX_Glow', 14.0, (0.8, 1.6), (12, 26), (0, 90), k=0.8, radius=2900.0, drag=0.5, curl=300.0, local=True, power=1.5, fade_in=5.0)])
# Slot: UTDAbilityFX.Loop of DA_Ability_Cloak_FX (looping, attached; the ship itself is faded by code). 6 m shimmer.
# 10f (T2's play: the flattened shell drew a grey-white "saucer" round the ship): no shell any more. The hull itself is
# carried by M_TD_VFX_CloakOverlay (an overlay material T2 sets on the ship's meshes while cloaked); this system only adds
# magenta glints flickering close to the hull and a few motes trailing off it. Rift magenta.
# 10g: SM_Rift is Nanite, so an overlay material on the hull draws nothing. Instead the hull mesh itself, 3% larger, as a
# mesh particle with the rim-lit additive field material in Rift magenta: the silhouette glows and shimmer cells crawl over
# it, attached at the ship root like the hull (zero rotation, scale 1). Glints were 8-20 cm: invisible at the chase camera.
def cloak_hull():
    # 10h: SM_TD_VFX_RiftCloakShell = a non-Nanite copy of SM_Rift (Niagara mesh particles drew nothing with the Nanite
    # hull), and M_TD_VFX_CloakOverlay on it: translucent, a deep magenta inside (the hull reads darker) and a magenta rim.
    return mesh_single('Hull', 'SM_TD_VFX_RiftCloakShell', 'M_TD_VFX_CloakShell2', '1.06, 1.06, 1.06', 1.0, None,   # 10i: stronger rim, denser veil, 1.06
                       live_scale='float3(1,1,1)*(1.0 + 0.006*sin(Engine.Time*9.0))')


# 10l (cloak2 turn): with the hull shell emitter (SM_TD_VFX_RiftCloakShell) the whole system reports INACTIVE right after
# spawning, through the ability and through a plain spawn, so nothing drew. The shell is left out (cloak_hull() kept for a
# later fix); the cloak ships as the glints and motes, which ran and read in r10g.
S2('NS_TD_Cloak_Shimmer', U((1.0, 0.12, 0.85, 1.0)), [
    shed('Glints', 'M_TD_VFX_Glow', 0, (0.15, 0.35), (40, 90), (0, 40), k=4.0, radius=330.0, drag=2.0, local=True, power=1.2, rate_expr='70.0'),
    shed('Motes', 'M_TD_VFX_Glow', 18.0, (0.4, 0.9), (5, 12), (0, 160), k=1.2, radius=360.0, drag=1.0, curl=500.0, local=True, power=1.5, fade_in=5.0)])
# Slot: UTDAbilityFX.Loop of DA_Ability_GravityWell_FX (looping, at the well). Authored for a 40 m pull radius.
# Round 8, seen in play: the Bastion drops the well about 9 m away, so from the chase camera (26 m from it) the 70 m swirl
# sprites covered the whole screen in violet. The swirls and the core now fade as the camera comes inside 50 m.
# Second look: at 0.42 alpha the 70 m sprites still filled the view, so they are also capped to about 1.1x the camera
# distance (an angle, not a size) and fade to 0.1 inside 30 m.
WELL_NEAR = 'saturate((Engine.Owner.LODDistance - 3000.0*User.Scale)/(3000.0*User.Scale))*0.9 + 0.1'
WCAP = 'min(1.0, 1.1*Engine.Owner.LODDistance/(%s*User.Scale))'
S2('NS_TD_GravityWell', U((0.60, 0.30, 1.0, 1.0)), [
    alpha_by(looping(single('Swirl', 'M_TD_VFX_Swirl', '(7000*%s)' % (WCAP % 7000), 1.1, None), 2.4), WELL_NEAR),
    alpha_by(looping(single('SwirlB', 'M_TD_VFX_Swirl', '(4200*%s)' % (WCAP % 4200), 0.9, None), 1.5), WELL_NEAR),
    alpha_by(single('Core', 'M_TD_VFX_Glow', 900, 1.3, 23.1), WELL_NEAR),
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
# Round 8 (Nicholas, issue #31): "keep the effect as is, just increase transparency by 25%": the rings', shards' and
# dust's particle alpha x 0.75 (every VFX material multiplies by it); nothing else changed.
def opacity(e, f):
    e['set'] = [x if not (x[1] == INIT and x[2] == 'Color' and isinstance(x[3], X)) else (x[0], x[1], x[2], X('(%s)*float4(1.0, 1.0, 1.0, %s)' % (x[3].hlsl, f)))
                for x in e['set']]
    return e


S2('NS_TD_Blade_AirSlam', U(BLADE), [
    opacity(flat_ring('Ring', 600, 0.38, 1.4), 0.75),
    opacity(flat_ring('Ring2', 460, 0.55, 0.6, mat='M_TD_VFX_Ring', growth='0.15 + pow(Particles.NormalizedAge, 0.7)*0.85'), 0.75),
    flash('Flash', 'M_TD_VFX_Glow', 260, 0.12, 0.8),
    opacity(shed('Sparks', 'M_TD_VFX_Spark', 0, (0.2, 0.55), (1.6, 3.4), (350, 1300), k=1.7, radius=30.0, drag=2.4, stretch=12, cone=((0.0, 0.0, 1.0), 82.0), burst=40, dur=0.7), 0.75),
    opacity(spin(shed('Dust', 'M_TD_VFX_Smoke', 0, (0.6, 1.1), (60, 110), (200, 520), radius=60.0, drag=2.6, rgb=(0.45, 0.50, 0.55), alpha=0.5, cone=((0.0, 0.0, 1.0), 88.0),
                      burst=16, dur=1.2, power=0.9, growth='0.6 + Particles.NormalizedAge*1.4', sort=True)), 0.75)])
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

# ---- round 9 (ART-06, environment audit): planet ambient particles, attached to the camera, world-space GPU sprites
# spawned in a 25 m sphere round it (so they parallax as the camera moves). Cheap and bounded (rates, short lives,
# fixed bounds). User.Fade 0..1: the spawning code sets it from the height above ground (1 below 450 m, 0 above 600 m).
# Slots wanted: UTDTravelFX.PlanetAmbientVerdance / PlanetAmbientCinder (FTDFXCue), played attached to the player's
# camera by the planet's look code (ATDPlaceholderSky, which knows the planet style), User.Fade set every tick.
AMB = '(User.Fade)'
# round 9b: x3 size and rate (first look: nearly invisible on foot in daylight)
# round 9c: born within 10 m of the camera (25 m spread them too thin to see on foot), larger
S2('NS_TD_Ambient_Verdance', U((0.75, 1.0, 0.55, 1.0), 1.0, Fade=1.0), [
    shed('Pollen', 'M_TD_VFX_Glow', 0, (4.0, 7.0), (12, 22), (5, 30), k=0.7, radius=1000.0, drag=0.4, curl=60.0, rgb=(1.0, 0.95, 0.70), alpha=0.8,
         rate_expr='120.0*' + AMB, power=0.8, fade_in=3.0, gpu=True),
    shed('Spores', 'M_TD_VFX_Glow', 0, (3.0, 6.0), (28, 60), (10, 50), k=1.6, radius=900.0, drag=0.3, curl=90.0, rgb=(0.35, 1.0, 0.85), alpha=1.0,
         rate_expr='27.0*' + AMB, power=1.2, fade_in=4.0, gpu=True)])
S2('NS_TD_Ambient_Cinder', U((1.0, 0.45, 0.12, 1.0), 1.0, Fade=1.0), [
    shed('Ash', 'M_TD_VFX_Smoke', 0, (3.0, 6.0), (11, 22), (60, 160), radius=1000.0, drag=0.2, curl=40.0, rgb=(0.20, 0.19, 0.19), alpha=0.55,   # 9d: -40%, fainter (Lead: grey blobs near the camera)
         cone=((0.0, 0.0, -1.0), 25.0), rate_expr='180.0*' + AMB, power=0.6, fade_in=3.0, gpu=True),
    shed('Embers', 'M_TD_VFX_Glow', 0, (2.0, 4.0), (14, 30), (80, 240), k=1.8, radius=900.0, drag=0.3, curl=120.0, rgb=(1.0, 0.45, 0.10),
         cone=((0.0, 0.0, 1.0), 30.0), rate_expr='48.0*' + AMB, power=1.4, fade_in=4.0, gpu=True)])

# ---- round 9 (ART-16, T4): the Dominion's one-shots, all SpawnSystemAtLocation at a world point.
# ReactorExposed: once, when the 4th generator falls, at the reactor (its rotation): the core uncovered. Authored for a
# 25 m reactor at Scale 1: a white flare, a red heat bloom, the shell falling apart, arcs and a shock ring.
S2('NS_TD_Reactor_Exposed', U(CORE, 1.0), [
    flash('Flare', 'M_TD_VFX_Flare', 14000, 0.22, 1.6, growth='0.4 + Particles.NormalizedAge'),
    flash('Bloom', 'M_TD_VFX_Glow', 18000, 1.4, 1.0, growth='0.5 + Particles.NormalizedAge*0.6', power=1.4),
    shell_flash('Shell', 2500, 0.9, 'float3(1,1,1)*(1.0 + 0.6*pow(Particles.NormalizedAge, 0.6))', k=1.4, power=1.6),
    arcs('Arcs', 0, (1200, 2600), 2500, life=(0.15, 0.5), k=1.5, burst=24, dur=1.0, local=False),
    flash('Ring', 'M_TD_VFX_FlipRing', 32000, 0.8, 1.0, growth='0.06 + pow(Particles.NormalizedAge, 0.5)*0.95', power=1.4)])
# Round 9f, seen at real scale (25 m reactor, 160 m away in T4's stage): the flare read small; both doubled.
# ReactorHit: every hull hit on the exposed reactor (a high rate): under 0.5 s and cheap, three emitters.
S2('NS_TD_Reactor_Hit', U(CORE, 1.0), [
    # 9m (Lead): a hit must read on every bolt from 300-400 m behind the Rift: flash and glow 1.5x, plus a short radial spark burst
    flash('Flash', 'M_TD_VFX_Flare', 2250, 0.08, 1.7, growth='0.5 + Particles.NormalizedAge'),
    flash('Glow', 'M_TD_VFX_Glow', 3600, 0.18, 0.9),
    shed('Burst', 'M_TD_VFX_Spark', 0, (0.15, 0.3), (40, 80), (4000, 9000), k=1.9, radius=60.0, drag=2.6, stretch=10, burst=10, dur=0.35, power=1.2),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.2, 0.45), (10, 22), (1800, 6000), k=1.8, radius=80.0, drag=2.2, stretch=14, burst=16, dur=0.5, power=1.3)])
# DockingBayOpen: once when the bay opens (the doors take 2.5 s), at the bay actor, +X out of the mouth. Authored for a
# 60 m mouth: steam venting out of it for 2.5 s, the bay's lights coming on, a tractor glow building in the mouth.
DOCK = (0.75, 0.88, 1.0, 1.0)


def timed(e, secs):
    """A rate emitter that runs once for secs (then the system ends)."""
    e['set'] = [x for x in e['set'] if x[2] not in ('Loop Behavior',)] + [(EU, 'EmitterState', 'Loop Behavior', E('Once')), (EU, 'EmitterState', 'Loop Duration', float(secs))]
    return e


S2('NS_TD_Dominion_DockOpen', U(DOCK, 1.0), [
    timed(spin(shed('Steam', 'M_TD_VFX_SmokeRim', 0, (1.6, 2.6), (300, 800), (800, 2200), radius=2600.0, drag=0.9, curl=300.0, rgb=(0.80, 0.84, 0.88), alpha=0.5,
                    cone=((1.0, 0.0, 0.0), 25.0), rate_expr='34.0', power=0.8, fade_in=5.0, growth='0.6 + Particles.NormalizedAge*1.6', sort=True)), 2.5),
    shed('Lights', 'M_TD_VFX_Glow', 0, (2.6, 3.2), (90, 160), (0, 0), k=2.0, radius=3200.0, drag=0.0, rgb=(1.0, 0.82, 0.55), burst=26, dur=3.2, power=0.4, fade_in=3.0),
    flash('Tractor', 'M_TD_VFX_Glow', 6500, 3.0, 0.45, growth='0.3 + Particles.NormalizedAge*0.7', power=0.35),
    shed('Vents', 'M_TD_VFX_Spark', 0, (0.3, 0.7), (6, 12), (900, 2600), k=1.5, radius=2400.0, drag=1.6, stretch=12, cone=((1.0, 0.0, 0.0), 30.0), burst=30, dur=1.0)])

# ---- round 9 (ART-16): the escape. Slots wanted from T5 (DA_ReactorCore_FX): EscapeRunLoop, attached to the on-foot pawn
# for the 90 s run (+X = the pawn's facing); EscapeFlightLoop, attached to the flown ship's root for the escape flight
# (+X = the nose). Both read User.Intensity 0..1 (default 1) for the code to ramp the collapse up. Only spawning emitters.
ESC = '(User.Intensity)'
# round 9b: x3 size and rate
S2('NS_TD_Escape_RunLoop', U((1.0, 0.55, 0.22, 1.0), 1.0, Intensity=1.0), [
    # a rain of sparks from the ceiling, ahead of and around the runner
    offset(shed('SparkRain', 'M_TD_VFX_Spark', 0, (0.5, 1.1), (7.5, 15.0), (600, 1500), k=1.8, radius=1400.0, drag=0.4, local=False, stretch=16,
                cone=((0.0, 0.0, -1.0), 14.0), rate_expr='210.0*' + ESC, fade_in=6.0), 700, 0, 900),
    # embers drifting through the corridor
    offset(shed('Embers', 'M_TD_VFX_Glow', 0, (0.8, 1.8), (12, 30), (40, 220), k=1.4, radius=1200.0, drag=0.6, curl=700.0, local=False,
                rate_expr='135.0*' + ESC, power=1.6, fade_in=4.0), 500, 0, 200),
    # dark falling debris streaks
    offset(shed('Debris', 'M_TD_VFX_Smoke', 0, (0.5, 0.9), (24, 54), (700, 1600), radius=1300.0, drag=0.2, local=False, stretch=6, rgb=(0.06, 0.05, 0.05), alpha=0.9,
                cone=((0.0, 0.0, -1.0), 10.0), rate_expr='66.0*' + ESC, power=0.6, fade_in=6.0), 600, 0, 900),
    # hot smoke haze rolling along the corridor
    offset(spin(shed('Haze', 'M_TD_VFX_SmokeRim', 0, (2.0, 3.4), (300, 620), (60, 260), radius=900.0, drag=0.8, curl=200.0, local=False, rgb=(0.36, 0.30, 0.28), alpha=0.35,
                     rate_expr='9.0*' + ESC, power=0.8, fade_in=5.0, growth='0.7 + Particles.NormalizedAge*0.8', sort=True)), 900, 0, 150)])
S2('NS_TD_Escape_FlightLoop', U((1.0, 0.50, 0.18, 1.0), 1.0, Intensity=1.0), [
    # burning wreckage streaming past the cockpit: born far ahead, flying back past the ship
    offset(spin(shed('Fire', 'M_TD_VFX_Fireball', 0, (0.5, 0.9), (260, 620), (9000, 15000), radius=3000.0, drag=0.0, curl=900.0, local=True, rgb=(1.0, 0.62, 0.34),
                     cone=((-1.0, 0.0, 0.0), 4.0), rate_expr='28.0*' + ESC, growth='0.6 + Particles.NormalizedAge*1.4')), 4000),
    offset(shed('Embers', 'M_TD_VFX_Spark', 0, (0.3, 0.6), (14, 26), (10000, 18000), k=1.7, radius=3200.0, drag=0.0, local=True, stretch=16,
                cone=((-1.0, 0.0, 0.0), 3.0), rate_expr='220.0*' + ESC, fade_in=6.0), 4000),
    offset(shed('Ash', 'M_TD_VFX_Smoke', 0, (0.35, 0.6), (26, 52), (9000, 15000), radius=3200.0, drag=0.0, local=True, stretch=6, rgb=(0.05, 0.04, 0.04), alpha=0.9,
                cone=((-1.0, 0.0, 0.0), 3.0), rate_expr='120.0*' + ESC, power=0.6, fade_in=6.0), 4000),
    offset(spin(shed('Smoke', 'M_TD_VFX_SmokeRim', 0, (0.6, 1.1), (600, 1200), (8000, 13000), radius=3600.0, drag=0.0, local=True, rgb=(0.30, 0.26, 0.24), alpha=0.5,
                     cone=((-1.0, 0.0, 0.0), 4.0), rate_expr='6.0*' + ESC, power=0.8, fade_in=5.0, growth='0.8 + Particles.NormalizedAge*1.2', sort=True)), 4500)])

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
# Round 9s (Lead / T1: mid-glide the few grey puffs ahead read as explosion smoke): a full-screen cloud layer the ship flies
# INSIDE for 1-2 s, then breaks out of. Attached to the ship, +X forward; wisps born in a 40 m ball around the chase camera
# and ahead of the ship, streaking back past it. User.Color = the cloud tint (default pale teal-white, Verdance; Cinder:
# ash-brown (0.45, 0.36, 0.30)); User.Embers 0..1 adds glowing embers (Cinder); User.Intensity 0..1 fades it all.
S2('NS_TD_Entry_CloudWisps', U((0.78, 0.92, 0.92, 1.0), 1.0, Intensity=1.0, Embers=0.0), [
    # v2 (Lead): the base is a soft, nearly uniform veil over 70-80% of the frame (big low-contrast blobs, no curl), slow so
    # it lingers around the chase camera; Cinder (User.Embers 1) gets fewer of them, so the same veil at lower opacity.
    offset(shed('Veil', 'M_TD_VFX_CloudVeil', 0, (0.9, 1.4), (5000, 9000), (2500, 5000), radius=2200.0, drag=0.0, local=True, alpha=0.24,
                cone=((-1.0, 0.0, 0.0), 3.0), rate_expr='19.0*(1.0 - 0.25*User.Embers)*' + RAMP, power=0.8, fade_in=8.0, sort=True), 900),
    # the streaking wisps ride on top for speed: fewer and fainter than v1 so they read as motion, not as smoke
    offset(spin(shed('Wisps', 'M_TD_VFX_CloudWisp', 0, (0.4, 0.7), (1400, 3200), (11000, 17000), radius=3800.0, drag=0.0, local=True, alpha=0.40,
                     cone=((-1.0, 0.0, 0.0), 4.0), rate_expr='45.0*' + RAMP, power=0.9, fade_in=6.0, sort=True)), 2500),
    offset(shed('Streaks', 'M_TD_VFX_Spark', 0, (0.25, 0.45), (8, 16), (14000, 24000), k=0.6, radius=3000.0, drag=0.0, local=True, stretch=40,
                cone=((-1.0, 0.0, 0.0), 2.0), rate_expr='120.0*' + RAMP, fade_in=6.0), 3000),
    offset(shed('Embers', 'M_TD_VFX_Glow', 0, (0.4, 0.8), (14, 34), (8000, 14000), k=1.8, radius=3000.0, drag=0.0, curl=800.0, local=True, rgb=(1.0, 0.45, 0.10),
                cone=((-1.0, 0.0, 0.0), 6.0), rate_expr='80.0*User.Embers*' + RAMP, power=1.4), 2500)])

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
    mesh_flash('Ring', 'SM_TD_VFX_Disc', 'M_TD_VFX_FlipShockCone', '1.0, 30.0, 30.0', 0.55, 1.0, growth='float3(1,1,1)*(0.12 + pow(Particles.NormalizedAge, 0.5)*0.88)', power=1.2),
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


# ---- round 8: the entry for T1's Director's Cut shot (Lead, Nov 13). A director camera about 25 m ahead of the ship, 18 m
# to the side and 6 m below, looking back at it in three-quarter front profile, the planet's dark limb behind; then a cut
# to the chase camera inside the white cover. From there the round 7 entry buried the ship: the flame cap (5 m long,
# 3.4 m wide round the nose) and the 19 m halo covered the hull from every side. Now:
#   - the bow shock reads side-on as one clean standing arc off the nose (the second, squashed layer is dimmer);
#   - the hull stays visible and is LIT by the burn: an orange point light over the back, a white-blue one at the shock;
#     no fire is born on the hull's sides (only a small hot cap ahead of the nose);
#   - plasma leaves the belly, the tail and the wing tips in long streamers (world-space ribbons, 2 s = ~70 m of wake);
#   - embers and debris cross the whole frame (born in a wide volume ahead of the ship, not only round it).
def plight(name, radius, rgb, k, off=None):
    """A persistent point light (particle light renderer), faded with the entry strength."""
    e = ramped(single(name, 'M_TD_VFX_Glow', 1, 1.0, None, offset=off))
    e['rclass'] = '/Script/Niagara.NiagaraLightRendererProperties'
    e['rend'] = {'RadiusScale': radius / 100.0, 'bAlphaScalesBrightness': True, 'bAffectsTranslucency': False}
    e.pop('mat', None)
    return tint(e, rgb, k)


def dc_bow():
    grow_ = 'float3(0.80 + 0.20*%s, (0.55 + 0.45*%s)*%s, (0.55 + 0.45*%s)*%s)*%s'
    wake = []
    for nm, y, z, w in (('Wake', 0, 0, 90), ('WakeL', -330, 0, 52), ('WakeR', 330, 0, 52), ('WakeU', 0, -110, 70)):
        e = trail(nm, w, 2.0, 1.0, rate=80.0, taper=0.45, power=1.1)
        e['set'] = [x for x in e['set'] if x[2] != 'Ribbon Width'] + [(PS, INIT, 'Ribbon Width', X('%s*%s*%s' % (w, SCL, RAMP)))]
        wake.append(offset(e, -150 if y == 0 and z == 0 else -40, y, z))
    return [ramped(mesh_single('Bow', 'SM_TD_VFX_BowArc', 'M_TD_VFX_BowArc', '1.0, 1.0, 1.0', 1.1, None,
                               live_scale=grow_ % (RAMP, RAMP, '1.0', RAMP, '1.0', EFLK % 53.1))),
            ramped(mesh_single('Bow2', 'SM_TD_VFX_BowArc', 'M_TD_VFX_BowArc', '1.12, 1.0, 1.0', 0.32, None,
                               live_scale=grow_ % (RAMP, RAMP, '1.30', RAMP, '0.62', EFLK % 87.7))),
            tint(ramped(single('Shock', 'M_TD_VFX_Glow', 420, 1.0, 97.3, offset=(430, 0, 0))), (0.70, 0.88, 1.0), 1.2)] + wake


def dc_sheath():
    grow_ = 'float3(0.75 + 0.25*%s, 0.80 + 0.20*%s, 0.80 + 0.20*%s)*%s'
    return [ramped(mesh_single('Cap', 'SM_TD_VFX_Flame', 'M_TD_VFX_Sheath', '2.4, 1.5, 1.5', 1.2, None, offset=(170, 0, 0),
                               live_scale=grow_ % (RAMP, RAMP, RAMP, EFLK % 87.7))),
            ramped(single('Nose', 'M_TD_VFX_Glow', 200, 0.8, 71.3, offset=(330, 0, 0))),
            plight('HullLight', 1400, (1.0, 0.45, 0.16), 9.0, off=(-60, 0, 260)),
            plight('ShockLight', 900, (0.65, 0.85, 1.0), 7.0, off=(520, 0, 0)),
            # fire only behind the tail, under the belly and off the wing tips, streaming away
            offset(spin(shed('Flames', 'M_TD_VFX_Fireball', 0, (0.45, 0.90), (170, 320), (2400, 5400), radius=140.0, drag=0.5, curl=2000.0, local=True, rgb=(1.0, 0.60, 0.34),
                             cone=((-1.0, 0.0, 0.0), 9.0), growth='0.6 + Particles.NormalizedAge*1.9', rate_expr='110.0*' + RAMP)), -330),
            offset(spin(shed('Belly', 'M_TD_VFX_Fireball', 0, (0.35, 0.70), (110, 210), (2400, 5000), radius=120.0, drag=0.5, curl=1600.0, local=True, rgb=(1.0, 0.60, 0.34),
                             cone=((-1.0, 0.0, 0.0), 6.0), growth='0.6 + Particles.NormalizedAge*1.6', rate_expr='70.0*' + RAMP)), -60, 0, -150),
            offset(spin(shed('TipL', 'M_TD_VFX_Fireball', 0, (0.30, 0.60), (70, 150), (2400, 4800), radius=60.0, drag=0.5, curl=1400.0, local=True, rgb=(1.0, 0.60, 0.34),
                             cone=((-1.0, 0.0, 0.0), 5.0), growth='0.6 + Particles.NormalizedAge*1.7', rate_expr='45.0*' + RAMP)), -120, -340, 0),
            offset(spin(shed('TipR', 'M_TD_VFX_Fireball', 0, (0.30, 0.60), (70, 150), (2400, 4800), radius=60.0, drag=0.5, curl=1400.0, local=True, rgb=(1.0, 0.60, 0.34),
                             cone=((-1.0, 0.0, 0.0), 5.0), growth='0.6 + Particles.NormalizedAge*1.7', rate_expr='45.0*' + RAMP)), -120, 340, 0)]


def dc_debris():
    near = entry_debris()
    # across the frame: embers and ash born in a wide volume ahead of the ship, streaking back past it (and past the
    # director camera 25 m ahead)
    return near + [
        offset(shed('FarEmbers', 'M_TD_VFX_Spark', 0, (0.30, 0.55), (8, 16), (9000, 16000), k=1.6, radius=2600.0, drag=0.0, local=True, stretch=14,
                    cone=((-1.0, 0.0, 0.0), 3.0), rate_expr='90.0*' + RAMP, fade_in=6.0), 2600),
        offset(shed('FarAsh', 'M_TD_VFX_Smoke', 0, (0.30, 0.55), (14, 30), (8000, 14000), radius=2600.0, drag=0.0, local=True, stretch=6, rgb=(0.05, 0.04, 0.04), alpha=0.85,
                    cone=((-1.0, 0.0, 0.0), 3.0), rate_expr='50.0*' + RAMP, power=0.6, fade_in=6.0), 2600)]


# Trimmed to 18 emitters (Lead: each must earn its place on screen): no contact flash (a 14 m glow whites out the director
# shot), no second bow layer, no belly ribbon (the belly fire carries it), no nose glow (the cap and the shock do),
# no hull sparks or ice flecks, one ember emitter.
def _pick(es, drop):
    return [e for e in es if e['name'] not in drop]


ENTRY_DROP = ('Flash', 'Bow2', 'WakeU', 'Nose', 'HullSparks', 'Ice', 'Embers')
S2('NS_TD_Entry', U(ENTRY, 1.0, Intensity=1.0), _pick(entry_cone()[:1] + dc_bow() + dc_sheath() + entry_streaks()[:2] + dc_debris(), ENTRY_DROP))
_RAMP_IN = RAMP
RAMP = '(User.Intensity*saturate(1.0 - Emitter.Age/3.0))'
# Exit: the same burn, thinning out over 3 s on its own or driven down by User.Intensity (T1: AtmosphericExit).
S2('NS_TD_Exit', U(ENTRY, 1.0, Intensity=1.0), _pick(dc_bow() + dc_sheath()[1:] + entry_streaks()[:2] + dc_debris(), ENTRY_DROP))
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


# Round 8 (Lead, after Ryan saw the Rift up close in the Dominion hangar: the fill read as a static, cloudy purple texture;
# and the machine is over its video memory budget): ONE emitter, one particle. A disc mesh with M_TD_VFX_PortalVortex
# (the animated vortex: swirl, glow falling off into a dark centre, white-hot rim, sparks spiralling in, all in the
# shader), sized to User.Radius (cm, default 44: the new portal disc), 3 cm behind the socket so it sits over the
# ship's own fill as seen from the chase camera. User.Power (0.5 idle, 1 with the blaster open) drives its strength
# through the particle alpha every frame. The round 7 version (two swirls, core, motes, streaks, arcs) was hidden behind
# the hull's opaque disc in play.
# The same material works straight on the ship's disc (MI_TD_VFX_RiftPortalVortex, PortalPower from T1's
# SetShipMaterialScalar): then it shows from both sides and costs no particles at all.
_ring = mesh_single('Vortex', 'SM_TD_VFX_Disc', 'M_TD_VFX_PortalVortex', '1.0, User.Radius/50.0, User.Radius/50.0', 1.0, None, offset=(-3, 0, 0))
_ring['add'].append((PU, 'Update/Color/ScaleColor'))
_ring['set'] += [(PU, 'ScaleColor', 'ScaleRGB', False), (PU, 'ScaleColor', 'ScaleA', True), (PU, 'ScaleColor', 'Scale Alpha', X('(0.35 + 0.65*User.Power)'))]
# A system of persistent emitters only never activates (round 5), so a handful of sparks falls into the ring too
# (4 to 10 a second, alive 0.4 s: at most four on screen).
_glints = offset(shed('Glints', 'M_TD_VFX_Spark', 0, (0.30, 0.45), (0.8, 1.4), ('-2.4*User.Radius', '-1.8*User.Radius'), k=1.4, radius='(User.Radius*0.95)', drag=0.0,
                      local=True, stretch=10, rate_expr='(4.0 + 6.0*User.Power)', power=0.8, fade_in=5.0), -4)
S2('NS_TD_Rift_RingIdle', U(RIFT, 1.0, Power=0.5, Radius=44.0), [_ring, _glints])

# ============================================================================================= pickups (round 8, for T2)
# Four looping idles and a collect burst each, Z up, sized to T2's 3.4 m pickup (ATDPickup::Size). Colours are T2's
# (ATDPickup::ColorOf), clear of the squad's magenta / cyan / amber and the enemy's red / orange; each has its own
# silhouette so it reads without colour:
#   Hull (green)  a crisp plus           Shield (white-blue)  a hexagon ripple in a rim, bright core
#   Power (violet) an upright crystal in a swirl              Overcharge (gold-white)  a spiked star throwing sparks
# Round 8b, seen in play (PIE, Waves1to2): at 1 m core sizes the idles were dots at 45 m and gone at 200 m. Every sprite
# now also grows with the distance to the camera (PKD: 1x inside 40 m, up to 4x at 160 m and beyond), like the tracers,
# so a pickup stays a readable beacon at 200 m and is its real size when the player is about to take it.
# Slots wanted (T2): a looped cue attached to the pickup actor per type, and a one-shot cue at the pickup on collect.
PK_HULL = (0.25, 1.0, 0.35, 1.0)
PK_SHIELD = (0.42, 0.70, 1.0, 1.0)
PK_POWER = (0.55, 0.22, 1.0, 1.0)
PK_OVER = (1.0, 0.92, 0.62, 1.0)
PULSE = '(0.85 + 0.15*sin(Engine.Time*%s))'
PKD = 'clamp(Engine.Owner.LODDistance/4000.0, 1.0, 4.0)'
_SCL_STD = SCL
SCL = '(User.Scale*Engine.Owner.Scale.x*%s)' % PKD   # the idles only; restored below


def upright(e):
    """Turn a persistent mesh emitter's +X to the system's +Z (mesh renderer Velocity facing, a token velocity straight up)."""
    e['add'] = [(PS, 'Spawn/Velocity/AddVelocity')] + e['add']
    e['set'] += [(PS, 'AddVelocity', 'Velocity', (0.0, 0.0, 2.0))]
    e['rend']['FacingMode'] = 'Velocity'
    return e


def pk_common(halo=420):
    back = single('Back', 'M_TD_VFX_Backdrop', 380, 1.0, None)   # seen in play: over the nebula green / blue / violet added up to white
    back['rend']['SortOrderHint'] = -10
    return [back, single('Halo', 'M_TD_VFX_Glow', '(%s*%s)' % (halo, PULSE % 2.6), 0.45, None),
            shed('Motes', 'M_TD_VFX_Glow', 14.0, (0.8, 1.6), (4.0, 9.0), (10, 60), k=1.2, radius=120.0, drag=0.5, curl=120.0, local=True, power=1.2, fade_in=4.0)]


# Hull repair: a green plus (camera-facing, so it reads from every side), a hot centre, sparks rising.
S2('NS_TD_Pickup_Hull', U(PK_HULL), pk_common() + [
    single('Plus', 'M_TD_VFX_Plus', '(300*%s)' % (PULSE % 3.1), 1.3, None),
    single('Core', 'M_TD_VFX_Glow', 70, 1.2, 23.7),
    shed('Rise', 'M_TD_VFX_Spark', 10.0, (0.6, 1.1), (3.0, 6.0), (60, 150), k=1.4, radius=90.0, drag=0.0, local=True, stretch=7, cone=((0.0, 0.0, 1.0), 6.0), power=1.0, fade_in=5.0)])
# Shield cell: a hexagon ripple that keeps spreading inside a rim, a bright core.
S2('NS_TD_Pickup_Shield', U(PK_SHIELD), pk_common(400) + [
    looping(single('Hex', 'M_TD_VFX_Hex', 270, 1.3, None), 1.3),
    tint(single('Rim', 'M_TD_VFX_Ring', 320, 1.0, None), (0.20, 0.50, 1.0), 1.0),   # the ring material's white rim washed the first build white
    tint(single('Core', 'M_TD_VFX_Glow', 110, 1.0, 19.3), (0.30, 0.60, 1.0), 1.1),
    shed('Glints', 'M_TD_VFX_Glow', 10.0, (0.3, 0.6), (5.0, 11.0), (0, 10), k=1.4, radius=110.0, drag=0.0, local=True, power=1.0, fade_in=4.0)])   # first build (a sphere shell and a Shock rim) never activated
# Power cell: an upright violet crystal (the bolt spindle stood on end) inside a turning swirl, small arcs.
S2('NS_TD_Pickup_Power', U(PK_POWER), pk_common(400) + [
    upright(mesh_single('Crystal', 'SM_TD_VFX_Bolt', 'M_TD_VFX_Bolt', '3.2, 1.2, 1.2', 3.0, 41.3)),
    tint(looping(single('Swirl', 'M_TD_VFX_Swirl', 320, 1.0, None), 2.2), (0.45, 0.12, 1.0), 1.0),
    single('Core', 'M_TD_VFX_Glow', 90, 1.0, 41.3),
    arcs('Arcs', 6.0, (70, 140), 60, life=(0.08, 0.2), k=1.2)])
# Overcharge: a spiked gold-white star that throws sparks outward all the time.
S2('NS_TD_Pickup_Overcharge', U(PK_OVER), pk_common(440) + [
    looping(single('Star', 'M_TD_VFX_Flare', '(470*%s)' % (PULSE % 6.3), 1.2, None), 0.5),   # 330 / 210 / 100 read as a dot at 200 m beside the others
    looping(single('Star2', 'M_TD_VFX_Flare', 300, 1.4, None), 0.33),
    single('Core', 'M_TD_VFX_Glow', 160, 1.5, 67.1),
    shed('Sparks', 'M_TD_VFX_Spark', 34.0, (0.25, 0.5), (3.0, 6.5), (200, 520), k=1.6, radius=20.0, drag=1.0, local=True, stretch=11, power=1.0)])
SCL = _SCL_STD


def pk_collect(extra=()):
    # Played attached to the collecting ship (T2), seen from the chase camera 17 m behind: in play the first build was a thin
    # white ring round the ship with no colour. Now a coloured bloom and a coloured noise ring around the white shock ring.
    return [flash('Flash', 'M_TD_VFX_Flare', 520, 0.10, 1.3, growth='0.5 + Particles.NormalizedAge'),
            flash('Glow', 'M_TD_VFX_Glow', 950, 0.26, 0.95, growth='0.5 + Particles.NormalizedAge*0.8'),
            flash('Ring', 'M_TD_VFX_Shock', 620, 0.32, 0.9, growth='0.10 + pow(Particles.NormalizedAge, 0.5)*0.90', power=1.3),
            flash('ColorRing', 'M_TD_VFX_Ring', 900, 0.40, 1.6, growth='0.15 + pow(Particles.NormalizedAge, 0.6)*0.85', power=1.1),
            shed('Sparks', 'M_TD_VFX_Spark', 0, (0.20, 0.50), (2.5, 5.5), (500, 2200), k=1.7, radius=20.0, drag=2.6, stretch=12, burst=30, dur=0.6),
            shed('Motes', 'M_TD_VFX_Glow', 0, (0.5, 1.1), (4, 10), (80, 520), k=1.2, radius=40.0, drag=1.8, curl=500.0, burst=22, dur=1.2, power=1.6)] + list(extra)


# Collect bursts (a one-shot at the pickup, short: flash, ring, sparks, the type's own shape thrown out).
S2('NS_TD_Pickup_Hull_Collect', U(PK_HULL), pk_collect([
    flash('Plus', 'M_TD_VFX_Plus', 420, 0.35, 1.5, growth='0.5 + Particles.NormalizedAge*0.9', power=1.0)]))
S2('NS_TD_Pickup_Shield_Collect', U(PK_SHIELD), pk_collect([
    flash('Hex', 'M_TD_VFX_Hex', 420, 0.35, 1.2, growth='0.35 + Particles.NormalizedAge*0.65', power=0.8),
    shell_flash('Bubble', 60, 0.35, 'float3(1,1,1)*(1.0 + 2.2*pow(Particles.NormalizedAge, 0.6))', k=0.9, power=1.2)]))
S2('NS_TD_Pickup_Power_Collect', U(PK_POWER), pk_collect([
    arcs('Arcs', 0, (70, 150), 50, life=(0.1, 0.3), k=1.0, burst=10, dur=0.5, local=False),
    spin(flash('Swirl', 'M_TD_VFX_Swirl', 460, 0.4, 1.0, growth='0.3 + Particles.NormalizedAge*0.7', power=0.8))]))
S2('NS_TD_Pickup_Overcharge_Collect', U(PK_OVER), pk_collect([
    flash('Star', 'M_TD_VFX_Flare', 620, 0.16, 1.5, growth='0.4 + Particles.NormalizedAge*0.8'),
    shed('Burst', 'M_TD_VFX_Spark', 0, (0.25, 0.6), (3.0, 6.5), (1200, 3600), k=1.9, radius=10.0, drag=2.2, stretch=16, burst=40, dur=0.7)]))

# ============================================================================================= asteroids (round 8, for T6 / T1)
# Space rocks and junk are destructible (Ryan). Cheap and small: at most 4 emitters, bounded bursts, no loops. One-shots
# at the rock's centre; +X does not matter (everything is radial). Sizes: S ~ 4 m, M ~ 9 m, L ~ 20 m rocks (User.Scale
# 1 / 2.2 / 4.5); VFXScale scales on top. Slots: whatever T6 names them (a Break cue per size, a crystal-veined variant).
ROCK = (0.85, 0.93, 1.0, 1.0)   # 9g: the flash tint is ice white-blue (was dusty grey)
CRYSTAL = (0.45, 0.55, 1.0, 1.0)   # the belt's blue-violet crystals: clear of the squad's cyan and magenta


def rock_break(n_chunks, n_dust, n_grit, fl=1800):
    # 9q (Lead: still a light switching on): a smaller burst with short rays and a pale cyan-white core, and the read moved
    # into 8 tumbling ice shards and a quick pale dust puff flying outward, so a break looks like something shattering.
    return [spin(flash('Flash', 'M_TD_VFX_IceShatter', fl * 0.6, 0.10, 1.4, growth='0.5 + Particles.NormalizedAge*0.9')),
            chunks('Ice', 'SM_TD_VFX_Shard_A', 8, (0.7, 1.3), (0.25, 0.6), (900, 2600), drag=0.7, radius=60.0, dur=1.4, spin=(1.5, 5.0)),
            chunks('Chunks', 'SM_TD_VFX_Shard_C', n_chunks, (0.8, 1.6), (0.35, 0.9), (500, 2000), drag=0.6, radius=90.0, dur=1.8, spin=(0.8, 3.0)),
            # 9r (Lead: the puffs read as grey cotton wool): dust, not smoke - additive pale cyan-white, half size, alpha 0.2,
            # expanding fast and thinning, gone within 0.25 s
            shed('Dust', 'M_TD_VFX_Glow', 0, (0.15, 0.25), (60, 110), (1500, 3000), k=0.2, radius=60.0, drag=4.0, rgb=(0.75, 0.92, 1.0), alpha=1.0,
                 burst=8, dur=0.4, power=1.6, growth='0.5 + Particles.NormalizedAge*2.0'),
            shed('Shards', 'M_TD_VFX_Spark', 0, (0.3, 0.8), (7, 14), (1500, 4800), k=1.8, radius=60.0, drag=1.8, stretch=9, rgb=(0.80, 0.93, 1.0),
                 burst=n_grit, dur=0.9, power=1.2)]


S2('NS_TD_Asteroid_Break_S', U(ROCK, 1.0), rock_break(5, 2, 18))
S2('NS_TD_Asteroid_Break_M', U(ROCK, 2.2), rock_break(7, 2, 24, fl=1000))
def late_ring(e, delay):
    e['set'].append((EU, 'SpawnBurst_Instantaneous', 'Spawn Time', float(delay)))
    return e


# 10 (Lead MINOR): the L break carries more weight at 200 m - a second, slightly later ring of 5 bigger, slower ice shards.
S2('NS_TD_Asteroid_Break_L', U(ROCK, 4.5), rock_break(9, 3, 32, fl=670) + [
    late_ring(chunks('IceBig', 'SM_TD_VFX_Shard_A', 5, (1.2, 2.0), (0.6, 1.0), (500, 1300), drag=0.5, radius=90.0, dur=2.2, spin=(0.6, 2.0)), 0.12)])
# Crystal-veined rock: the crystals shatter in light. A flare, glowing shards streaking out, slow glints, a few rock chunks.
S2('NS_TD_Asteroid_Break_Crystal', U(CRYSTAL, 2.2), [
    flash('Flash', 'M_TD_VFX_Flare', 900, 0.16, 1.4, growth='0.4 + Particles.NormalizedAge*0.9'),
    shed('Shards', 'M_TD_VFX_Spark', 0, (0.35, 0.8), (10, 22), (1800, 5200), k=2.0, radius=80.0, drag=1.8, stretch=8, burst=26, dur=1.0, power=1.2),
    shed('Glints', 'M_TD_VFX_Glow', 0, (0.9, 1.8), (14, 30), (200, 1200), k=1.6, radius=150.0, drag=1.2, curl=300.0, burst=20, dur=2.0, power=1.6),
    chunks('Chunks', 'SM_TD_VFX_Shard_A', 6, (0.8, 1.5), (0.3, 0.8), (500, 1800), drag=0.6, radius=90.0, dur=1.7, spin=(0.8, 3.0))])
# A ship scraping or hitting a rock (T1 fires it at the contact point, +X along the contact normal, with User.Strength 0..1):
# a spray of sparks off the surface, grinding grit, a hot flash; everything grows with the strength.
S2('NS_TD_Ship_ImpactScrape', U((1.0, 0.72, 0.42, 1.0), 1.0, Strength=0.5), [
    flash('Flash', 'M_TD_VFX_Glow', '(160.0 + 520.0*User.Strength)', 0.10, 1.0, growth='0.6 + Particles.NormalizedAge*0.6', power=1.4),
    shed('Sparks', 'M_TD_VFX_Spark', 0, (0.2, 0.55), (4, 9), (1200, 4200), k=1.8, radius=20.0, drag=2.4, stretch=16, cone=((1.0, 0.0, 0.0), 70.0),
         burst='8.0 + 46.0*User.Strength', dur=0.6, power=1.3),
    spin(shed('Grit', 'M_TD_VFX_FlipSmoke', 0, (0.5, 1.0), (60, 140), (100, 500), radius=30.0, drag=2.5, rgb=(0.42, 0.38, 0.34), alpha=0.7, cone=((1.0, 0.0, 0.0), 60.0),
              burst='1.0 + 4.0*User.Strength', dur=1.0, power=0.8, growth='0.6 + Particles.NormalizedAge*1.2', sort=True)),
    shed('Embers', 'M_TD_VFX_Glow', 0, (0.3, 0.8), (5, 12), (300, 1500), k=1.4, radius=20.0, drag=1.5, cone=((1.0, 0.0, 0.0), 80.0),
         burst='4.0 + 14.0*User.Strength', dur=0.8, power=1.6)])

# Round 9 (Lead): from the chase camera the blade is hidden behind the body at the hit frame. A short swing trail: a
# hot thin ribbon and a soft wide one, 0.15 s of life, in the blade's colour. Slot wanted (T5): UTDBladeFX.SwingTrail,
# attached to the blade mesh at its tip socket, activated when a swing starts and deactivated when it ends.
S2('NS_TD_Blade_SwingTrail', U(BLADE), [
    trail('Core', 10, 0.15, 1.8, rate=240.0, taper=0.9, power=1.4),
    trail('Glow', 46, 0.12, 0.7, rate=240.0, taper=0.8, power=1.6)])

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
CHUNK = globals().get('CHUNK')   # (start, end): build only those emitters of each selected system (see tdvfx.system)
for _name, _user, _emitters in SYSTEMS2:
    _sys = await fx2.system(_name, _emitters, user=_user, chunk=CHUNK)
    for _e in _emitters:
        if _e.get('verbose_dump'):
            try:
                fx2.log.append(await fx2.dump(_sys, _e['name'], ['ShapeLocation']))
            except Exception as _ex:
                fx2.log.append('dump failed ' + str(_ex)[:200])
    await asyncio.sleep(0)
fx2.report()

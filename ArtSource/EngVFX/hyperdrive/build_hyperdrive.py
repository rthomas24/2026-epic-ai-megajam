# Triad Drift: hyperdrive look (Ryan, Nov 13: "a really nice animation with really sharp Niagara particles ... a portal
# that boosts you throughout the map"). T7. Original design: a jagged energy tear opens ahead of the nose while the drive
# spools, the ship punches through it into a tunnel of hair-thin light streaks and energy rings rushing past, with the
# destination rift glowing far ahead, and drops out through an exit tear with a shock ring and a spray of sparks.
#
# Run (the wrapper puts the effects builder's helpers in front of this file, building none of its systems):
#   bash ArtSource/EngVFX/hyperdrive/run.sh <port> build
#
# Slots (UTDTravelFX on DA_Travel_FX; the hyperdrive component plays them, +X is the nose):
#   HyperdriveSpool   NS_TD_Hyperdrive_Spool    attached loop on the ship root for the 1.2 s spool
#   HyperdriveTunnel  NS_TD_Hyperdrive_Tunnel   attached loop on the ship root for the whole jump
#   HyperdriveDropOut NS_TD_Hyperdrive_DropOut  one shot at the ship's location and rotation at the drop-out
# User parameters on all three: Color (linear, default electric blue-violet, away from the three squad colours so the
# jump reads as the drive, not as a weapon) and Scale (1 = authored for a ship of about 10 m, chase camera 10-15 m back).
# Particle budget: Spool about 60 live, Tunnel about 170 live, DropOut 150 in one burst. All CPU sprites/meshes.

HYPER_DIR = '/Game/TriadDrift/Gameplay/VFX/Hyperdrive'
HYPER = (0.42, 0.52, 1.0, 1.0)
RIFT_MAT = HYPER_DIR + '/Materials/M_TD_VFX_HyperRift'
RING_MAT = HYPER_DIR + '/Materials/M_TD_VFX_HyperRing'   # r5: thin hard ring (review: the shared ring read as a soft donut)


def ringed(e):
    """Point a meshed emitter at the hyperdrive ring material."""
    e['rend']['OverrideMaterials'] = [{'explicitMat': ref(RING_MAT)}]
    return e


def rifted(e):
    """Point a meshed disc emitter at the hyperdrive rift material (outside the shared Materials folder)."""
    e['rend']['OverrideMaterials'] = [{'explicitMat': ref(RIFT_MAT)}]
    return e


def mesh_scaled(e, scale):
    e['set'] += [(PS, INIT, 'Mesh Scale Mode', E('Non-Uniform')), (PS, INIT, 'Mesh Scale', X('float3(%s)*User.Scale' % scale))]
    return e


HSYS = []


def H(name, user, emitters):
    HSYS.append((name, user, emitters))


# ---- Spool: the tear opens 26 m ahead (0 -> full over 1.1 s), energy is pulled from the hull into it, the nose charges.
OPEN = '(saturate(Emitter.Age/1.1)*saturate(Emitter.Age/1.1)*(3.0 - 2.0*saturate(Emitter.Age/1.1)))'   # smoothstep (the function failed to compile in a Niagara expression)
H('NS_TD_Hyperdrive_Spool', U(HYPER), [
    # r3: bigger (r2 was about 90 px wide at 1600 x 900 from the chase camera)
    # r7 (review MINOR: small, on the nose, clipped by the ship glow): 70 m ahead, so from the chase camera it projects up
    # by the crosshair instead of onto the hull, and big enough to read there; a dimmer nose charge.
    rifted(mesh_single('Tear', 'SM_TD_VFX_Disc', 'M_TD_VFX_Ring', '1.0, 44.0, 66.0', 1.0, offset=(7000, 0, 0),
                       live_scale='float3(1.0, 0.15 + 0.85*%s, 0.05 + 0.95*%s)' % (OPEN, OPEN))),
    offset(single('TearGlow', 'M_TD_VFX_Glow', 7500, 0.40, 13.7), 7050),   # r2: the first capture read as a dark fan: light behind it
    offset(single('Charge', 'M_TD_VFX_Glow', 420, 0.7, 71.3), 320),
    shed('Inflow', 'M_TD_VFX_Spark', 0, (0.18, 0.32), (4, 9), (7000, 14000), k=1.8, radius=700.0, drag=0.0, local=True, stretch=22,
         cone=((1.0, 0.0, 0.0), 12.0), rate_expr='220.0*%s' % OPEN, fade_in=6.0),
    offset(shed('RimSparks', 'M_TD_VFX_Spark', 0, (0.10, 0.25), (3, 7), (1500, 5000), k=2.0, radius=500.0, drag=2.5, local=True, stretch=12,
                rate_expr='90.0*%s' % OPEN), 2600)])

# ---- Tunnel: the punch-through (the tear sweeps over the camera in 0.35 s), hair-thin streaks rushing back from 120 m
# ahead, rings of light flying past, the destination rift glowing 300 m ahead, a faint sheath round the hull.
H('NS_TD_Hyperdrive_Tunnel', U(HYPER), [
    # r5 (review BLOCKER: the punch filled 60% of the screen and hid the ship): grows 2.5x, not 4.5x, and fades faster
    rifted(mesh_flash('Punch', 'SM_TD_VFX_Disc', 'M_TD_VFX_Ring', '1.0, 8.0, 13.0', 0.30, 1.0, power=2.0,
                      growth='float3(1,1,1)*(1.0 + pow(Particles.NormalizedAge, 0.7)*1.5)')),
    flash('PunchFlash', 'M_TD_VFX_Glow', 2600, 0.20, 0.45, growth='0.6 + Particles.NormalizedAge*0.8'),
    offset(shed('Streaks', 'M_TD_VFX_Spark', 650.0, (0.12, 0.26), (9, 20), (60000, 120000), k=2.6, radius=2600.0, drag=0.0, local=True, stretch=60,
                cone=((-1.0, 0.0, 0.0), 1.5), fade_in=10.0), 12000),
    offset(shed('WhiteCore', 'M_TD_VFX_Spark', 140.0, (0.10, 0.20), (3, 6), (90000, 150000), radius=1600.0, drag=0.0, local=True, stretch=90,
                cone=((-1.0, 0.0, 0.0), 1.0), rgb=(1.4, 1.4, 1.6), fade_in=10.0), 12000),
    # r2: camera-facing ring sprites (r1's mesh-disc rings on the rate template left the whole system inactive)
    offset(shed('Rings', RING_MAT, 7.0, (0.42, 0.42), (4200, 4200), (50000, 50000), k=1.7, radius=10.0, drag=0.0, local=True,   # r7: brighter (faint at the frame edge)
                cone=((-1.0, 0.0, 0.0), 0.5), power=0.8), 18000),
    rifted(mesh_single('Destination', 'SM_TD_VFX_Disc', 'M_TD_VFX_Ring', '1.0, 70.0, 70.0', 0.7, offset=(30000, 0, 0))),
    offset(single('Sheath', 'M_TD_VFX_Glow', 1700, 0.22, 13.7), 200)])

# ---- Drop-out: the ship bursts out of an exit tear (behind it, closing in 0.7 s), a shock ring, a flare, sparks thrown
# forward with the ship and streaks left behind.
H('NS_TD_Hyperdrive_DropOut', U(HYPER), [
    rifted(offset(mesh_flash('ExitTear', 'SM_TD_VFX_Disc', 'M_TD_VFX_Ring', '1.0, 6.5, 10.0', 0.7, 1.0, power=1.4,   # r6: 30% smaller (review)
                             growth='float3(1.0, 1.0 - pow(Particles.NormalizedAge, 1.5), 1.0 - pow(Particles.NormalizedAge, 2.5))'), 1500)),
    # r4: 15 m AHEAD, not behind: behind the ship it sat at the chase camera and was never seen; ahead, the camera sees
    # the ship framed by the closing tear in the first frames, as if it had just burst out of it.
    # r5 (review MAJOR: a white haze washed the first frame; MINOR: the ring lingered): sharp ring, shorter, weaker flash
    ringed(mesh_flash('Shock', 'SM_TD_VFX_Disc', 'M_TD_VFX_Ring', '1.0, 10.0, 10.0', 0.35, 1.4, power=1.5,
                      growth='float3(1,1,1)*(0.2 + pow(Particles.NormalizedAge, 0.5)*4.0)')),
    flash('Flare', 'M_TD_VFX_Flare', 2200, 0.10, 0.8, growth='0.5 + Particles.NormalizedAge'),
    flash('Glow', 'M_TD_VFX_Glow', 2400, 0.20, 0.22, growth='0.6 + Particles.NormalizedAge*0.6'),
    shed('Spray', 'M_TD_VFX_Spark', 0, (0.30, 0.70), (5, 12), (8000, 26000), k=1.9, radius=300.0, drag=2.0, stretch=30,
         cone=((1.0, 0.0, 0.0), 28.0), burst=130, dur=0.9, power=1.3),
    # r7 (review MINOR: a white bloom wedge bottom-centre in the first frame): the wake streaks were born at the ship and
    # crossed the chase camera within a few frames; now fewer, dimmer, and born 10 m further forward.
    offset(shed('Wake', 'M_TD_VFX_Spark', 0, (0.20, 0.40), (3, 7), (10000, 22000), k=0.7, radius=400.0, drag=1.5, stretch=40,
                cone=((-1.0, 0.0, 0.0), 6.0), burst=30, dur=0.6, power=1.4), 1000)])

hx = tdvfx.Builder(call_tool)
ONLY_H = globals().get('ONLY_H')
for _name, _user, _emitters in HSYS:
    if ONLY_H and _name not in ONLY_H:
        continue
    await hx.system(_name, _emitters, user=_user, folder=HYPER_DIR)
    await asyncio.sleep(0)
hx.report()

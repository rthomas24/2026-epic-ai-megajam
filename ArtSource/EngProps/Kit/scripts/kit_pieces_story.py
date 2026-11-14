# SPDX-License-Identifier: MIT
# Kit, story props for the planets (Ryan: "planets should tell a story"; the environment builder composes the zones, T5
# places them through its Dress lists). Route C: code-built in the kit's style (planet trim, 1.5 cm bevels), NOT walkable;
# collision only on pieces larger than 1 m (one simple hull, so nothing snags the pilot). Pivot on the ground.
# Verdance - an abandoned field camp: shelter frame with a torn canopy, a dead lamp post, stacked sample cases, a toppled
#   antenna mast, survey stakes with flags, a drag-mark decal.
# Cinder - a mining camp: an ore cart, a short rail stub, a cable reel, a cracked heat-shield wall, a warning beacon, a
#   collapsed gantry section, scorch-line decals.
# Slots: Trim, Glow, Canopy (the trim tinted as weathered fabric), Flag, Beacon, Decal (raw UVs over a decal texture).
import math
import random

from mathutils import Matrix

from kit_lib import Piece, rotz


def _tube(p, a, b, r=4.0, reg="frame", mat="Trim"):
    """A square-section bar from a to b (cm): a thin box along the segment."""
    ax, ay, az = a
    bx, by, bz = b
    dx, dy, dz = bx - ax, by - ay, bz - az
    L = math.sqrt(dx * dx + dy * dy + dz * dz)
    yaw = math.degrees(math.atan2(dy, dx))
    pitch = -math.degrees(math.atan2(dz, math.hypot(dx, dy)))
    with p.at(Matrix.Translation(a) @ Matrix.Rotation(math.radians(yaw), 4, "Z") @ Matrix.Rotation(math.radians(pitch), 4, "Y")):
        p.box((0.0, -r, -r), (L, r, r), reg=reg, mat=mat)


# ------------------------------------------------------------------------------------------------ Verdance field camp
def shelter_frame():
    """A field shelter (400 long, 300 wide, 240 high): four tube hoops on ground plates, a ridge bar, and what is left of
    its canopy - two torn panels hanging on one side, the rest gone. Seen against the pilot it is a tent you could stand in."""
    p = Piece("SM_Kit_ShelterFrame", None)
    for x in (-200.0, -67.0, 67.0, 200.0):
        pts = [(x, 150.0 * math.cos(math.radians(a)), 240.0 * math.sin(math.radians(a))) for a in range(0, 181, 30)]
        for i in range(len(pts) - 1):
            _tube(p, pts[i], pts[i + 1], 4.0)
        for y in (-150.0, 150.0):
            p.box((x - 15, y - 15, 0.0), (x + 15, y + 15, 3.0), reg="sw_dark")
    _tube(p, (-205.0, 0.0, 240.0), (205.0, 0.0, 240.0), 4.0)
    # torn canopy: two panels between hoops on the +Y slope, ragged lower edges, both faces
    rnd = random.Random(5)
    for x0, x1 in ((-200.0, -67.0), (67.0, 200.0)):
        top = [(x0, 75.0, 207.0), (x1, 75.0, 207.0)]
        n = 6
        for k in range(n):
            xa, xb = x0 + (x1 - x0) * k / n, x0 + (x1 - x0) * (k + 1) / n
            ha = 60.0 + rnd.random() * 120.0
            hb = 60.0 + rnd.random() * 120.0
            ya, za = 75.0 + 75.0 * min(1.0, ha / 180.0), 207.0 - ha
            yb, zb = 75.0 + 75.0 * min(1.0, hb / 180.0), 207.0 - hb
            q = [(xa, 75.0, 207.0), (xb, 75.0, 207.0), (xb, yb, zb), (xa, ya, za)]
            p.poly(q, "wall", mat="Canopy", n=(0, 1, 0.5), uv=[(0, 0), (1, 0), (1, 1), (0, 1)])
            p.poly(q[::-1], "wall", mat="Canopy", n=(0, -1, -0.5), uv=[(0, 0), (1, 0), (1, 1), (0, 1)])
    p.chull([(x, y, z) for x in (-205.0, 205.0) for y in (-150.0, 150.0) for z in (0.0, 30.0)] +
            [(x, y, 240.0) for x in (-205.0, 205.0) for y in (-20.0, 20.0)])
    return p


def lamp_post_dead():
    """A dead camp lamp (320 tall): a post on a weighted base, a lamp head with a cracked dark lens, its cable cut."""
    p = Piece("SM_Kit_LampPostDead", None)
    p.box((-35.0, -35.0, 0.0), (35.0, 35.0, 18.0), reg="hazard")
    p.box((-7.0, -7.0, 18.0), (7.0, 7.0, 300.0), reg="pillar")
    p.box((-7.0, -7.0, 290.0), (60.0, 7.0, 300.0), reg="frame")
    p.box((40.0, -22.0, 268.0), (80.0, 22.0, 296.0), regs={"z": "sw_black"}, reg="frame")
    _tube(p, (0.0, 8.0, 60.0), (40.0, 30.0, 0.0), 2.0, reg="sw_dark")
    p.cbox((-35.0, -35.0, 0.0), (35.0, 35.0, 300.0))
    return p


def sample_cases():
    """Stacked sample cases (about 130 x 90 x 115): four hard cases with hazard-taped lids, one knocked off and open."""
    p = Piece("SM_Kit_SampleCases", None)
    for (x, y, z, w, d, h, yaw) in ((0.0, 0.0, 0.0, 70.0, 45.0, 38.0, 0.0), (8.0, 4.0, 38.0, 70.0, 45.0, 38.0, 6.0),
                                    (2.0, -2.0, 76.0, 60.0, 40.0, 36.0, -5.0), (-75.0, 30.0, 0.0, 60.0, 40.0, 30.0, 28.0)):
        with p.at(Matrix.Translation((x, y, z)) @ rotz(yaw)):
            p.box((-w / 2, -d / 2, 0.0), (w / 2, d / 2, h), regs={"Z": "hatch", "x": "frame", "X": "frame", "y": "wallB", "Y": "wallB"})
            p.box((-w / 2 - 1, -d / 2 - 1, h * 0.7), (w / 2 + 1, d / 2 + 1, h * 0.78), reg="hazard")
    p.chull([(x, y, z) for x in (-110.0, 45.0) for y in (-30.0, 55.0) for z in (0.0, 112.0)])
    return p


def antenna_toppled():
    """A survey antenna mast that came down (about 900 long on the ground): a lattice mast lying along +X from its tilted
    tripod base, a bent dish at the far end."""
    p = Piece("SM_Kit_AntennaToppled", None)
    for (a, b) in (((0.0, -40.0, 0.0), (-60.0, -90.0, 0.0)), ((0.0, 40.0, 0.0), (-60.0, 90.0, 0.0)), ((0.0, 0.0, 0.0), (80.0, 0.0, 0.0))):
        p.box((min(a[0], b[0]) - 5, min(a[1], b[1]) - 5, 0.0), (max(a[0], b[0]) + 5, max(a[1], b[1]) + 5, 8.0), reg="sw_dark")
    for y in (-15.0, 15.0):
        for z in (6.0, 36.0):
            _tube(p, (0.0, y, z), (820.0, y * 0.6, z * 0.8 + 4.0), 3.0)
    for x in range(40, 800, 60):
        _tube(p, (float(x), -15.0, 6.0), (float(x) + 30.0, 15.0, 34.0), 2.0, reg="sw_gun")
    with p.at(Matrix.Translation((850.0, 0.0, 40.0)) @ Matrix.Rotation(math.radians(70.0), 4, "Y")):
        p.lathe([(0.0, 0.0), (40.0, 8.0), (90.0, 28.0)], 12, regs=["sw_gun", "wallB"], a0=0.0, a1=300.0)
        p.lathe([(90.0, 30.0), (40.0, 10.0), (0.0, 2.0)], 12, regs=["sw_dark", "sw_dark"], a0=0.0, a1=300.0)
    p.chull([(x, y, z) for x in (-60.0, 900.0) for y in (-95.0, 95.0) for z in (0.0, 70.0)])
    return p


def survey_stake_flag():
    """A survey stake with a torn marker flag (140 tall, under 1 m wide: no collision)."""
    p = Piece("SM_Kit_SurveyStakeFlag", None)
    p.box((-2.0, -2.0, 0.0), (2.0, 2.0, 140.0), reg="pillar")
    q = [(0.0, 2.0, 138.0), (0.0, 38.0, 132.0), (0.0, 30.0, 118.0), (0.0, 36.0, 106.0), (0.0, 2.0, 104.0)]
    p.poly(q, mat="Flag", n=(1, 0, 0))
    p.poly(q[::-1], mat="Flag", n=(-1, 0, 0))
    return p


def drag_mark():
    """A drag-mark decal (600 x 140, 1 cm above the ground): two scuffed furrows where something heavy was dragged away
    from the camp. Slot Decal over T_Kit_DragMark (masked)."""
    p = Piece("SM_Kit_DragMark", None)
    N = 8
    for i in range(N):
        x0, x1 = -300.0 + 600.0 * i / N, -300.0 + 600.0 * (i + 1) / N
        y0, y1 = 25.0 * math.sin(i * 0.6), 25.0 * math.sin((i + 1) * 0.6)
        q = [(x0, y0 - 70.0, 1.0), (x1, y1 - 70.0, 1.0), (x1, y1 + 70.0, 1.0), (x0, y0 + 70.0, 1.0)]
        p.poly(q, mat="Decal", n=(0, 0, 1), uv=[(i / N, 0.0), ((i + 1) / N, 0.0), ((i + 1) / N, 1.0), (i / N, 1.0)])
    return p


# ------------------------------------------------------------------------------------------------ Cinder mining camp
def ore_cart():
    """An ore cart (190 x 120 x 130): a tapered steel bin on a wheeled chassis, heaped with dark ore, one wheel off."""
    p = Piece("SM_Kit_OreCart", None)
    p.box((-90.0, -55.0, 25.0), (90.0, 55.0, 40.0), reg="frame")
    for (x, y) in ((-60.0, -60.0), (60.0, -60.0), (-60.0, 60.0)):
        with p.at(Matrix.Translation((x, y, 22.0)) @ Matrix.Rotation(math.radians(90.0), 4, "X")):
            p.lathe([(0.0, -6.0), (22.0, -6.0), (22.0, 6.0), (0.0, 6.0)], 12, regs=["sw_dark", "sw_gun", "sw_dark"])
    with p.at(Matrix.Translation((60.0, 70.0, 22.0)) @ Matrix.Rotation(math.radians(70.0), 4, "Y")):       # the lost wheel
        p.lathe([(0.0, -6.0), (22.0, -6.0), (22.0, 6.0), (0.0, 6.0)], 12, regs=["sw_dark", "sw_gun", "sw_dark"])
    bot, top = [(-80.0, -48.0), (80.0, -48.0), (80.0, 48.0), (-80.0, 48.0)], [(-95.0, -60.0), (95.0, -60.0), (95.0, 60.0), (-95.0, 60.0)]
    for i in range(4):
        a0, a1, b0, b1 = bot[i], bot[(i + 1) % 4], top[i], top[(i + 1) % 4]
        q = [(a0[0], a0[1], 40.0), (a1[0], a1[1], 40.0), (b1[0], b1[1], 120.0), (b0[0], b0[1], 120.0)]
        n = ((a0[1] - a1[1]) * 1.0, (a1[0] - a0[0]) * 1.0, 0.0)
        p.poly(q, "wallB", n=(-n[0], -n[1], 0.2), uv=[(0, 0), (1, 0), (1, 1), (0, 1)])
    rnd = random.Random(8)
    for k in range(9):
        cx, cy = rnd.uniform(-60.0, 60.0), rnd.uniform(-35.0, 35.0)
        r = rnd.uniform(18.0, 30.0)
        p.box((cx - r, cy - r * 0.8, 100.0), (cx + r, cy + r * 0.8, 112.0 + rnd.random() * 18.0), reg="sw_black")
    for i in range(4):                                       # hazard-yellow rim, one ember strip on the long side
        b0, b1 = top[i], top[(i + 1) % 4]
        p.box((min(b0[0], b1[0]) - 2, min(b0[1], b1[1]) - 2, 116.0), (max(b0[0], b1[0]) + 2, max(b0[1], b1[1]) + 2, 124.0), reg="hazard")
    p.poly([(-70.0, -57.0, 70.0), (70.0, -57.0, 70.0), (70.0, -59.0, 76.0), (-70.0, -59.0, 76.0)], mat="Glow", n=(0, -1, 0.3))
    p.chull([(x, y, z) for x in (-95.0, 95.0) for y in (-62.0, 62.0) for z in (0.0, 130.0)])
    return p


def rail_stub():
    """A short rail stub (800 x 150): two rails on sleepers, the far end bent up where it was torn."""
    p = Piece("SM_Kit_RailStub", None)
    for x in range(-380, 400, 80):
        p.box((x - 12.0, -75.0, 0.0), (x + 12.0, 75.0, 10.0), reg="hazard" if x % 160 == 20 else "sw_dark")
    for y in (-45.0, 45.0):
        p.box((-400.0, y - 4.0, 10.0), (250.0, y + 4.0, 22.0), reg="sw_gun")
        _tube(p, (250.0, y, 16.0), (400.0, y * 1.3, 70.0), 5.0, reg="sw_gun")
    p.cbox((-400.0, -75.0, 0.0), (400.0, 75.0, 22.0))
    return p


def cable_reel():
    """A cable reel on its side stand (170 across, 140 high): a drum with heavy cable, a frayed end trailing on the ground."""
    p = Piece("SM_Kit_CableReel", None)
    with p.at(Matrix.Translation((0.0, 0.0, 85.0)) @ Matrix.Rotation(math.radians(90.0), 4, "X")):
        p.lathe([(0.0, -55.0), (85.0, -55.0), (85.0, -45.0), (55.0, -45.0), (55.0, 45.0), (85.0, 45.0), (85.0, 55.0), (0.0, 55.0)], 16,
                regs=["frame", "frame", "sw_gun", "sw_black", "sw_gun", "frame", "frame"])
    for y in (-60.0, 60.0):
        _tube(p, (-60.0, y, 0.0), (0.0, y, 85.0), 5.0)
        _tube(p, (60.0, y, 0.0), (0.0, y, 85.0), 5.0)
    for k in range(6):
        _tube(p, (55.0 + k * 40.0, -10.0 + 8.0 * math.sin(k), 30.0 * max(0.0, 1.0 - k / 2.0)),
              (95.0 + k * 40.0, -10.0 + 8.0 * math.sin(k + 1), 30.0 * max(0.0, 1.0 - (k + 1) / 2.0)), 5.0, reg="sw_black")
    p.chull([(x, y, z) for x in (-85.0, 85.0) for y in (-62.0, 62.0) for z in (0.0, 170.0)])
    return p


def heat_shield_cracked():
    """A cracked heat-shield wall (400 x 45 x 250): a heavy panel on two feet, split by a zigzag crack that glows ember
    where the heat got through."""
    p = Piece("SM_Kit_HeatShieldCracked", None)
    crack = [(20.0, 250.0), (-30.0, 190.0), (25.0, 130.0), (-15.0, 70.0), (10.0, 0.0)]
    left = [(-200.0, 0.0)] + [(x - 6.0, z) for x, z in crack[::-1]] + [(-200.0, 250.0)]
    right = [(200.0, 0.0), (200.0, 250.0)] + [(x + 6.0, z) for x, z in crack]
    p.prism(left, "xz", -18.0, 18.0, regs="wall", cap_reg="hazard")
    p.prism(right, "xz", -18.0, 18.0, regs="wallB", cap_reg="hazard")
    for i in range(len(crack) - 1):
        (x0, z0), (x1, z1) = crack[i], crack[i + 1]
        for s in (-1.0, 1.0):
            yy = s * 10.0
            p.poly([(x0 - 6.0, yy, z0), (x0 + 6.0, yy, z0), (x1 + 6.0, yy, z1), (x1 - 6.0, yy, z1)], mat="Glow", n=(0, s, 0))
    for x in (-150.0, 150.0):
        p.box((x - 30.0, -45.0, 0.0), (x + 30.0, 45.0, 25.0), reg="frame")
    p.cbox((-200.0, -45.0, 0.0), (200.0, 45.0, 250.0))
    return p


def warning_beacon():
    """A warning beacon (220 tall): a striped post on a weighted base with an ember lamp that pulses."""
    p = Piece("SM_Kit_WarningBeacon", None)
    p.box((-30.0, -30.0, 0.0), (30.0, 30.0, 15.0), reg="frame")
    p.box((-6.0, -6.0, 15.0), (6.0, 6.0, 190.0), reg="hazard")
    p.box((-14.0, -14.0, 190.0), (14.0, 14.0, 222.0), mat="Beacon")
    p.cbox((-30.0, -30.0, 0.0), (30.0, 30.0, 222.0))
    return p


def gantry_collapsed():
    """A collapsed gantry section (about 700 x 220 x 260): a box truss lying at an angle with one end propped on its
    broken leg, plates torn off the deck."""
    p = Piece("SM_Kit_GantryCollapsed", None)
    with p.at(Matrix.Translation((0.0, 0.0, 0.0)) @ Matrix.Rotation(math.radians(-14.0), 4, "Y")):
        for y in (-100.0, 100.0):
            for z in (20.0, 160.0):
                _tube(p, (-340.0, y, z), (340.0, y, z), 6.0)
        for x in range(-340, 341, 85):
            for y in (-100.0, 100.0):
                _tube(p, (float(x), y, 20.0), (float(x) + 42.0, y, 160.0), 4.0, reg="sw_gun")
            _tube(p, (float(x), -100.0, 160.0), (float(x), 100.0, 160.0), 4.0, reg="sw_gun")
        p.box((-340.0, -100.0, 160.0), (40.0, 100.0, 166.0), reg="tread")
        p.box((-340.0, -106.0, 150.0), (340.0, -100.0, 170.0), reg="hazard")
        p.box((-340.0, 100.0, 150.0), (340.0, 106.0, 170.0), reg="hazard")
        p.poly([(-300.0, -107.0, 40.0), (300.0, -107.0, 40.0), (300.0, -107.0, 48.0), (-300.0, -107.0, 48.0)], mat="Glow", n=(0, -1, 0))
        # the one working lamp, still lit on the raised end
        p.box((330.0, -20.0, 170.0), (346.0, 20.0, 230.0), reg="frame")
        p.box((322.0, -40.0, 230.0), (360.0, 40.0, 262.0), mat="Light")
    _tube(p, (300.0, -100.0, 0.0), (330.0, -100.0, 100.0), 6.0)
    p.chull([(x, y, z) for x in (-360.0, 360.0) for y in (-106.0, 106.0) for z in (0.0, 260.0 if x > 0 else 120.0)])
    return p


def scorch_lines():
    """Scorch-line decals (800 x 300, 1 cm above the ground): blast streaks fanning from a point - something burned here."""
    p = Piece("SM_Kit_ScorchLines", None)
    p.poly([(-400.0, -150.0, 1.0), (400.0, -150.0, 1.0), (400.0, 150.0, 1.0), (-400.0, 150.0, 1.0)], mat="Decal2", n=(0, 0, 1),
           uv=[(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)])
    return p


def _worn(fn):
    """Cinder pieces: every Trim face becomes slot Worn (lighter worn steel, a little self-lit: Cinder's lighting is dark)."""
    def wrapped():
        p = fn()
        p.faces = [(v, u, "Worn" if m == "Trim" else m) for v, u, m in p.faces]
        return p
    wrapped.__name__ = fn.__name__
    return wrapped


# ------------------------------------------------------------------------------------------------ landmarks
def relay_mast():
    """Verdance landmark r2: a leaning expedition relay mast (about 100 m, 5 degrees off vertical): a square lattice
    tower 9 m across at the base tapering to 1.4 m, two cross-arms, a 13 m dead dish two thirds up (its disc faces -X:
    yaw the mast so -X points at the landing pad), green-amber lit bands on three levels and a blinking light at the top,
    four guy lines (one snapped). Collision: the pad only."""
    p = Piece("SM_Kit_RelayMast", None)
    H = 10000.0
    W0, W1 = 450.0, 70.0                                   # half-width at the base and at the top

    def hw(z):
        return W0 + (W1 - W0) * z / H
    p.box((-650.0, -650.0, 0.0), (650.0, 650.0, 80.0), reg="floor")
    with p.at(Matrix.Rotation(math.radians(5.0), 4, "Y")):
        for (sx, sy) in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
            _tube(p, (sx * W0, sy * W0, 80.0), (sx * W1, sy * W1, H), 16.0)
        z = 80.0
        step = 500.0
        while z < H - step:
            w0, w1 = hw(z), hw(z + step)
            for (ax, ay, bx, by) in ((-1, -1, 1, -1), (1, -1, 1, 1), (1, 1, -1, 1), (-1, 1, -1, -1)):
                _tube(p, (ax * w0, ay * w0, z), (bx * w1, by * w1, z + step), 7.0, reg="sw_gun")
                _tube(p, (ax * w1, ay * w1, z + step), (bx * w1, by * w1, z + step), 7.0, reg="sw_gun")
            z += step
        # lit bands on three levels (steady green-amber), one strip on each face
        for t in (0.3, 0.55, 0.8):
            zb = H * t
            w = hw(zb) + 12.0
            for (nx, ny) in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                if nx:
                    q = [(nx * w, -w, zb), (nx * w, w, zb), (nx * w, w, zb + 60.0), (nx * w, -w, zb + 60.0)]
                else:
                    q = [(-w, ny * w, zb), (w, ny * w, zb), (w, ny * w, zb + 60.0), (-w, ny * w, zb + 60.0)]
                p.poly(q, mat="Band", n=(nx, ny, 0))
        # two cross-arms
        for (t, L) in ((0.55, 1300.0), (0.8, 900.0)):
            zc = H * t
            p.box((-L, -25.0, zc + 100.0), (L, 25.0, zc + 160.0), reg="frame")
            for sx in (-1, 1):
                _tube(p, (sx * L, 0.0, zc + 130.0), (sx * hw(zc), 0.0, zc - 400.0), 8.0, reg="sw_gun")
        # the dead dish, facing -X (toward the pad once the mast is yawed), tilted 15 degrees up
        with p.at(Matrix.Translation((-hw(H * 0.66) - 150.0, 0.0, H * 0.66)) @ Matrix.Rotation(math.radians(-75.0), 4, "Y")):
            p.lathe([(0.0, 0.0), (200.0, 40.0), (650.0, 220.0)], 20, regs=["sw_gun", "wallB"], a0=0.0, a1=360.0)
            p.lathe([(650.0, 222.0), (200.0, 42.0), (0.0, 2.0)], 20, regs=["sw_dark", "sw_dark"], a0=0.0, a1=360.0)
        _tube(p, (-hw(H * 0.66), 0.0, H * 0.66), (-hw(H * 0.66) - 150.0, 0.0, H * 0.66), 20.0)
        p.box((-90.0, -90.0, H), (90.0, 90.0, H + 160.0), reg="frame")
        p.box((-130.0, -130.0, H + 160.0), (130.0, 130.0, H + 420.0), mat="Nav")
    # guy lines (one snapped)
    top = (H * 0.8 * math.sin(math.radians(5.0)), 0.0, H * 0.8)
    for (gx, gy, snapped) in ((4200.0, 0.0, False), (-3800.0, 500.0, False), (0.0, 4000.0, False), (300.0, -4000.0, True)):
        if snapped:
            _tube(p, top, (top[0] * 0.95, top[1] - 200.0, top[2] - 3000.0), 5.0, reg="sw_dark")
            _tube(p, (gx, gy, 0.0), (gx * 0.6, gy * 0.6, 500.0), 5.0, reg="sw_dark")
        else:
            _tube(p, top, (gx, gy, 0.0), 5.0, reg="sw_dark")
        p.box((gx - 60.0, gy - 60.0, 0.0), (gx + 60.0, gy + 60.0, 60.0), reg="frame")
    p.cbox((-650.0, -650.0, 0.0), (650.0, 650.0, 600.0))
    return p


def light_pool():
    """A warm light pool on the ground (600 x 600, 1 cm up): an emissive decal under the gantry's working lamp, so the
    lamp seems to light the ground without a dynamic light. Slot Pool (M_Kit_Decal over T_Kit_LightPool)."""
    p = Piece("SM_Kit_LightPool", None)
    p.poly([(-300.0, -300.0, 1.0), (300.0, -300.0, 1.0), (300.0, 300.0, 1.0), (-300.0, 300.0, 1.0)], mat="Pool", n=(0, 0, 1),
           uv=[(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)])
    return p


def marker_pole():
    """A 6 m camp marker pole: a worn steel mast on a weighted foot, a hazard-yellow band, a working lamp on top - it
    gives the Cinder camp a lit silhouette from 30 m. Collision: one thin hull."""
    p = Piece("SM_Kit_MarkerPole", None)
    p.box((-45.0, -45.0, 0.0), (45.0, 45.0, 25.0), reg="frame")
    p.box((-10.0, -10.0, 25.0), (10.0, 10.0, 560.0), reg="pillar")
    p.box((-13.0, -13.0, 300.0), (13.0, 13.0, 420.0), reg="hazard")
    p.box((-28.0, -28.0, 560.0), (28.0, 28.0, 620.0), mat="Light")
    p.cbox((-45.0, -45.0, 0.0), (45.0, 45.0, 620.0))
    return p


def null_derrick():
    """Cinder landmark: a Null excavation derrick (about 80 m): a black A-frame tower over the pit on four splayed legs,
    a crown block with a red beacon, red seams up the legs, heavy cables running from the crown down into the pit (below
    the pivot, Z < 0). It marks the dig from the landing approach. Collision: the four leg feet only."""
    p = Piece("SM_Kit_NullDerrick", None)
    H = 8000.0
    for (x, y) in ((-1200.0, -1200.0), (1200.0, -1200.0), (1200.0, 1200.0), (-1200.0, 1200.0)):
        _tube(p, (x, y, 0.0), (x * 0.12, y * 0.12, H), 85.0, reg="wallB", mat="Dom")
        p.box((x - 160.0, y - 160.0, 0.0), (x + 160.0, y + 160.0, 120.0), reg="frame", mat="Dom")
        _tube(p, (x * 0.9, y * 0.9, 300.0), (x * 0.25, y * 0.25, H * 0.75), 14.0, mat="Glow")
    for z in (2000.0, 4000.0, 6000.0):
        w = 1200.0 * (1.0 - 0.88 * z / H)
        for (a, b) in (((-w, -w), (w, -w)), ((w, -w), (w, w)), ((w, w), (-w, w)), ((-w, w), (-w, -w))):
            _tube(p, (a[0], a[1], z), (b[0], b[1], z), 45.0, reg="frame", mat="Dom")
    p.box((-260.0, -260.0, H), (260.0, 260.0, H + 400.0), reg="wallB", mat="Dom")
    p.box((-160.0, -160.0, H + 400.0), (160.0, 160.0, H + 700.0), mat="Beacon")
    for (dx, dy) in ((-80.0, 0.0), (80.0, 0.0), (0.0, 80.0)):
        _tube(p, (dx, dy, H), (dx * 3.0, dy * 3.0, -2500.0), 12.0, reg="sw_black", mat="Dom")
    for (x, y) in ((-1200.0, -1200.0), (1200.0, -1200.0), (1200.0, 1200.0), (-1200.0, 1200.0)):
        p.cbox((x - 160.0, y - 160.0, 0.0), (x + 160.0, y + 160.0, 600.0))
    return p


VERDANCE = [shelter_frame, lamp_post_dead, sample_cases, antenna_toppled, survey_stake_flag, drag_mark]
CINDER = [_worn(f) for f in (ore_cart, rail_stub, cable_reel, heat_shield_cracked, warning_beacon, gantry_collapsed, scorch_lines,
                             marker_pole)] + [light_pool]
LANDMARKS = [relay_mast, null_derrick]
BUILDERS = VERDANCE + CINDER + LANDMARKS
NO_COLLISION = ("SM_Kit_SurveyStakeFlag", "SM_Kit_DragMark", "SM_Kit_ScorchLines", "SM_Kit_LightPool")

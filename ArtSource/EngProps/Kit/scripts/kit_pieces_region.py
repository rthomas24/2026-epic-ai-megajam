# SPDX-License-Identifier: MIT
# Kit, one modular set piece per space region (Ryan's map design, docs/eng/SPACE-DESIGN.md on eng/t6). Route C, the kit's
# hard-surface style and material set; seen from a ship at 300 m to 1.5 km. Non-walkable, one to three convex hulls,
# three LODs on import. Unreal cm, pivot at the centre of mass. Nothing carries a name, number or date.
#   Watchline       SM_KS_PicketRing        a broken picket-station ring segment, two dead sensor booms, a blinking amber light
#   Quiet Graveyard SM_KS_Hulk_Fore/_Mid/_Aft   three sections of one cut-up cruiser, dark, one cold-blue running light each
#   Survey Scar     SM_KS_ScarRig           a survey rig clamped across a split asteroid: gantry, drill arm, cable runs
#   Shattered Moons SM_KS_MoonShard_A/_B    two moon shards with exposed strata and a glowing fracture face
# Slots: Trim / Trim2 / Scorch (hull), Dom (the rig), Hazard is a trim region, Nav (amber blink), Cold (cold-blue light),
# Eye (white rig lights), Glow (fracture glow), Rock / Cut / Strata (stone).
import math
import random

from mathutils import Matrix

from kit_lib import Piece, rotz
from kit_pieces_space import _lathe_x, _rock, _ribs, hull_prof, loft
from kit_pieces_story import _tube


# ------------------------------------------------------------------------------------------------ Watchline
def picket_ring():
    """A broken picket-station ring segment (about 130 m across): a 140-degree arc of a 60 m ring, 12 x 8 m in section,
    lit window bands along the intact arc, a dark scorched torn end, two dead sensor booms (one bent) and a blinking
    amber light - the patrol line's station, ours, broken at First Contact. Pivot = the ring centre (the arc is 60 m
    out along +X)."""
    p = Piece("SM_KS_PicketRing", None)
    R_, A0, A1, N = 6000.0, -70.0, 70.0, 28
    T, Hh = 600.0, 400.0                                      # half radial thickness, half height
    step = (A1 - A0) / N
    seg = R_ * math.radians(step) * 0.5 + 8.0
    for k in range(N):
        a_ = A0 + step * (k + 0.5)
        with p.at(rotz(a_)):
            torn = k >= N - 2
            p.box((R_ - T, -seg, -Hh), (R_ + T, seg if k != N - 1 else seg * 0.2, Hh),
                  regs={"X": "wall" if k % 3 else "wallB", "x": "wallB", "Z": "roof", "z": "sw_dark", "y": "frame", "Y": "frame"},
                  mat="Scorch" if torn else ("Trim2" if k % 4 == 1 else "Trim"))
            if k % 2 == 0:
                p.box((R_ + T, -seg * 0.6, -120.0), (R_ + T + 50.0, seg * 0.6, 120.0), reg="frame")
            if not torn and k % 5 != 2:                          # two lit window bands on the outer face (one dark section)
                for zc in (-150.0, 150.0):
                    p.poly([(R_ + T + 2.0, -seg * 0.8, zc - 35.0), (R_ + T + 2.0, seg * 0.8, zc - 35.0),
                            (R_ + T + 2.0, seg * 0.8, zc + 35.0), (R_ + T + 2.0, -seg * 0.8, zc + 35.0)], mat="Light", n=(1, 0, 0))
            if k == N - 1:
                for (z0, z1, ext) in ((240.0, 400.0, 1000.0), (-400.0, -240.0, 650.0), (-80.0, 80.0, 380.0)):
                    p.box((R_ - T, seg * 0.2, z0), (R_ + T, seg * 0.2 + ext, z1), reg="wall", mat="Scorch")
    for (a_, L, bend) in ((-35.0, 4500.0, 0.0), (30.0, 3800.0, 35.0)):
        with p.at(rotz(a_)):
            end = (R_ + T + L * math.cos(math.radians(bend)), L * math.sin(math.radians(bend)) * 0.4, 0.0)
            for (dy, dz) in ((-110.0, -110.0), (110.0, -110.0), (0.0, 130.0)):
                _tube(p, (R_ + T, dy, dz), (end[0], end[1] + dy * 0.4, end[2] + dz * 0.4), 28.0, reg="sw_gun")
            p.box((end[0] - 250.0, end[1] - 250.0, -250.0), (end[0] + 400.0, end[1] + 250.0, 250.0), regs={"X": "sw_black"}, reg="frame")
    with p.at(rotz(5.0)):
        p.box((R_ - 60.0, -60.0, Hh), (R_ + 60.0, 60.0, 1400.0), reg="pillar")
        p.box((R_ - 160.0, -160.0, 1400.0), (R_ + 160.0, 160.0, 1720.0), mat="Nav")
    pts = []
    for a_ in (A0, 0.0, A1):
        for rr in (R_ - T, R_ + T + 4500.0 * (abs(a_) < 40)):
            for z in (-Hh, 1720.0 if a_ == 0.0 else Hh):
                pts.append((rr * math.cos(math.radians(a_)), rr * math.sin(math.radians(a_)), z))
    p.chull(pts)
    return p


# ------------------------------------------------------------------------------------------------ Quiet Graveyard
W2, H2 = 2400.0, 1600.0


def _torn_end(p, x, sign, w=W2, h=H2):
    """Staggered plates and frames where a section was cut from the next."""
    for (y0, y1, z0, z1, ext) in ((-w * 0.36, w * 0.36, h * 0.38, h * 0.46, 700.0), (-w / 2, -w * 0.4, -h * 0.1, h * 0.2, 420.0),
                                  (w * 0.4, w / 2, -h * 0.1, h * 0.25, 560.0), (-w * 0.28, w * 0.28, -h * 0.56, -h * 0.47, 250.0)):
        xa, xb = sorted((x, x + sign * ext))
        p.box((xa, y0, z0), (xb, y1, z1), reg="wall", mat="Scorch")
    _ribs(p, min(x, x + sign * 500.0), max(x, x + sign * 500.0), 250.0, hull_prof(w * 0.92, h * 0.92), t=60.0)


def _slits(p, x0, x1, k=1.0):
    """Rows of dead window slits along both flanks (dark, a few still faintly lit)."""
    rnd = random.Random(int(x0))
    for s_ in (-1.0, 1.0):
        y = s_ * (W2 / 2 * k + 4.0)
        for zc in (H2 * 0.18, H2 * 0.02):
            x = x0
            while x < x1:
                w = rnd.uniform(180.0, 320.0)
                lit = rnd.random() < 0.08
                p.poly([(x, y, zc - 22.0), (x + w, y, zc - 22.0), (x + w, y, zc + 22.0), (x, y, zc + 22.0)],
                       "sw_black", mat="Light" if lit else "Trim", n=(0, s_, 0), uv=[(0.5, 0.5)] * 4)
                x += w + rnd.uniform(60.0, 160.0)


def hulk_fore():
    """Cruiser bow section (about 75 m): the tapering bow and a dark command blister, cut off behind."""
    p = Piece("SM_KS_Hulk_Fore", None)
    loft(p, 0.0, hull_prof(W2, H2), 4000.0, hull_prof(W2, H2, 0.6), reg="wall", caps=(False, False))
    loft(p, 4000.0, hull_prof(W2, H2, 0.6), 6500.0, hull_prof(W2, H2, 0.08), reg="wallB", mat="Trim2", caps=(False, True))
    p.box((1500.0, -500.0, H2 * 0.45), (3500.0, 500.0, H2 * 0.45 + 450.0), regs={"Z": "roof", "X": "sw_black"}, reg="wall", mat="Trim2")
    _torn_end(p, 0.0, -1.0)
    _slits(p, 300.0, 3800.0, 1.0)
    p.box((6300.0, -80.0, -80.0), (6460.0, 80.0, 80.0), mat="Cold")
    p.chull([(x, y, z) for x in (-700.0, 6500.0) for y, z in hull_prof(W2, H2, 1.0 if x < 0 else 0.1)] +
            [(x, y, H2 * 0.45 + 450.0) for x in (1500.0, 3500.0) for y in (-500.0, 500.0)])
    return p


def hulk_mid():
    """Cruiser mid section (about 50 m): a long hull block cut at both ends, a row of dead hangar doors and a broken
    dorsal spine."""
    p = Piece("SM_KS_Hulk_Mid", None)
    prof = hull_prof(W2, H2)
    p.prism([(y, z) for y, z in prof][::-1], "yz", -2200.0, 2200.0, regs="wall", cap_reg="sw_dark")
    for k in range(4):
        x = -1700.0 + k * 1100.0
        p.box((x, -W2 / 2 - 8.0, -H2 * 0.05), (x + 700.0, -W2 / 2 + 4.0, H2 * 0.3), reg="hatch", mat="Trim2")
    p.box((-2200.0, -150.0, H2 * 0.45), (900.0, 150.0, H2 * 0.45 + 300.0), reg="frame")
    _torn_end(p, -2200.0, -1.0)
    _torn_end(p, 2200.0, 1.0)
    _slits(p, -2000.0, 2000.0, 1.0)
    # the hull breach: a torn hole on the +Y flank glowing cold, scorch round it
    p.poly([(-600.0, W2 / 2 + 12.0, -300.0), (500.0, W2 / 2 + 12.0, -250.0), (350.0, W2 / 2 + 12.0, 300.0), (-450.0, W2 / 2 + 12.0, 250.0)],
           mat="Breach", n=(0, 1, 0))
    p.poly([(-1000.0, W2 / 2 + 6.0, -600.0), (900.0, W2 / 2 + 6.0, -550.0), (750.0, W2 / 2 + 6.0, 600.0), (-850.0, W2 / 2 + 6.0, 550.0)],
           "wall", mat="Scorch", n=(0, 1, 0), uv=[(0, 0), (1, 0), (1, 1), (0, 1)])
    p.box((-80.0, W2 / 2 - 10.0, H2 * 0.2), (80.0, W2 / 2 + 150.0, H2 * 0.2 + 160.0), mat="Cold")
    p.chull([(x, y, z) for x in (-2900.0, 2900.0) for y, z in prof] + [(x, 0.0, H2 * 0.45 + 300.0) for x in (-2200.0, 900.0)])
    return p


def hulk_aft():
    """Cruiser engine section (about 40 m): the aft hull with three dead engine bells, cut off in front."""
    p = Piece("SM_KS_Hulk_Aft", None)
    loft(p, -2000.0, hull_prof(W2, H2, 0.85), 1500.0, hull_prof(W2, H2), reg="wall", caps=(True, False))
    _torn_end(p, 1500.0, 1.0)
    _slits(p, -1600.0, 1200.0, 1.0)
    for (y, z) in ((-650.0, -150.0), (650.0, -150.0), (0.0, 450.0)):
        _lathe_x(p, -2000.0, y, z, [(330.0, 0.0), (420.0, -400.0), (650.0, -1200.0), (600.0, -1200.0), (360.0, -400.0), (260.0, 0.0)],
                 14, regs=["sw_gun", "frame", "sw_dark", "sw_black", "sw_black"], mat="Trim2")
    p.box((-2100.0, -80.0, H2 * 0.4), (-1940.0, 80.0, H2 * 0.4 + 160.0), mat="Cold")
    p.chull([(x, y, z) for x in (-3300.0, 2200.0) for y in (-W2 / 2, W2 / 2) for z in (-H2 * 0.6, H2 * 0.5)])
    return p


# ------------------------------------------------------------------------------------------------ Survey Scar
def scar_rig():
    """A Null survey rig clamped across a split asteroid (about 90 m): the rock broken in two with a clean 14 m canyon
    (its two machined faces dark, their edges lit by work-light strips), a heavy truss gantry bridging it with
    hazard-yellow work lights, a drill arm down into the split with a slow white strobe, cable runs down both halves.
    Rock halves: the asteroid material on hard-shaded facets; the rig: the Dominion trim."""
    p = Piece("SM_KS_ScarRig", None)
    R = (3600.0, 3800.0, 3200.0)
    GAP = 700.0
    for (seed, yaw, dx) in ((31, 0.0, -GAP), (37, 180.0, GAP)):
        with p.at(Matrix.Translation((dx, 0.0, 0.0)) @ rotz(yaw)):
            _rock(p, R, seed, XC=0.0, BR=1.0, BD=0.0, NU=56, NV=30)
            # lit rim strips round the machined face (on the face plane, just proud)
            for (y0, y1, z0, z1) in ((-2600.0, 2600.0, 2300.0, 2360.0), (-2600.0, 2600.0, -2360.0, -2300.0),
                                     (-2700.0, -2640.0, -2200.0, 2200.0), (2640.0, 2700.0, -2200.0, 2200.0)):
                p.poly([(5.0, y0, z0), (5.0, y1, z0), (5.0, y1, z1), (5.0, y0, z1)], mat="Work", n=(1, 0, 0))
    p.faces = [(v, u, "Bore" if m == "Cut" else m) for v, u, m in p.faces]          # the canyon walls: dark
    p.smooth_deg = 22.0
    G = 3000.0
    for y in (-800.0, 800.0):
        for z in (G, G + 650.0):
            _tube(p, (-2700.0, y, z), (2700.0, y, z), 90.0, reg="hazard" if z == G else "frame", mat="Dom")
        for x in range(-2700, 2701, 900):
            _tube(p, (float(x), y, G), (float(x) + 450.0, y, G + 650.0), 45.0, reg="sw_gun", mat="Dom")
            _tube(p, (float(x), -800.0, G + 650.0), (float(x), 800.0, G + 650.0), 45.0, reg="sw_gun", mat="Dom")
        for x in (-2200.0, -900.0, 900.0, 2200.0):                  # hazard-yellow work lights along the gantry
            p.box((x - 70.0, y - 70.0, G - 200.0), (x + 70.0, y + 70.0, G - 90.0), mat="Work")
    for (x, y) in ((-2700.0, -800.0), (2700.0, -800.0), (-2700.0, 800.0), (2700.0, 800.0)):
        _tube(p, (x, y, G), (x * 1.05, y * 1.4, G - 1500.0), 90.0, reg="frame", mat="Dom")
        p.box((x * 1.05 - 220.0, y * 1.4 - 220.0, G - 1750.0), (x * 1.05 + 220.0, y * 1.4 + 220.0, G - 1450.0), reg="hazard", mat="Dom")
    p.box((-600.0, -600.0, G + 650.0), (600.0, 600.0, G + 1550.0), regs={"Z": "roof"}, reg="wallB", mat="Dom")
    p.box((-620.0, -620.0, G + 1150.0), (620.0, 620.0, G + 1230.0), reg="hazard", mat="Dom")
    _tube(p, (0.0, 0.0, G), (0.0, 0.0, -1800.0), 260.0, reg="frame", mat="Dom")
    p.box((-400.0, -400.0, -2200.0), (400.0, 400.0, -1800.0), reg="hazard", mat="Dom")
    p.box((-140.0, -140.0, 200.0), (140.0, 140.0, 480.0), mat="Strobe")          # the slow white strobe on the drill arm
    rnd = random.Random(6)
    for s_ in (-1.0, 1.0):
        prev = (s_ * 1900.0, -350.0, G)
        for k in range(5):
            nxt = (s_ * (2100.0 + 400.0 * k), -350.0 + rnd.uniform(-200, 200), G - 700.0 * (k + 1))
            _tube(p, prev, nxt, 45.0, reg="sw_black", mat="Dom")
            prev = nxt
    p.chull([(x, y, z) for x in (-4300.0, 4300.0) for y in (-3800.0, 3800.0) for z in (-3200.0, 4600.0)])
    return p


# ------------------------------------------------------------------------------------------------ Shattered Moons
def _shard(name, R, seed, seams, band=1100.0):
    """A moon shard: a hard-faceted rock (no smoothing across facets) cut by one planar fracture face (+X). The face is
    banded with strata (alternating Strata / Strata2 rows about 11 m thick) and crossed by ember seams (Seam, under 5 %
    of the face). Rock UVs tile 16 x 8 so the texture holds at 300 m."""
    p = Piece(name, None)
    XC = R[0] * 0.35
    _rock(p, R, seed, XC=XC, BR=1.0, BD=0.0, NU=96, NV=60)
    out = []
    for v, u, m in p.faces:
        if m == "Cut":
            zc = sum(q.z for q in v) / len(v)
            m = "Strata" if int((zc + 100000.0) // band) % 2 == 0 else "Strata2"
            u = [(q.y / 3000.0, q.z / 1200.0) for q in v]
        elif m == "Rock":
            u = [(a_ * 4.0, b_ * 4.0) for a_, b_ in u]
        out.append((v, u, m))
    p.faces = out
    p.smooth_deg = 18.0                                   # hard-shade the large facets and the break
    rnd = random.Random(seed + 1)
    for k in range(seams):
        z = -R[2] * 0.45 + R[2] * 0.9 * (k + 0.5) / seams + rnd.uniform(-300, 300)
        y = -R[1] * 0.55
        while y < R[1] * 0.55:
            y1 = y + rnd.uniform(1500, 3000)
            z1 = z + rnd.uniform(-600, 600)
            p.poly([(XC + 30.0, y, z - 90.0), (XC + 30.0, y1, z1 - 90.0), (XC + 30.0, y1, z1 + 90.0), (XC + 30.0, y, z + 90.0)],
                   mat="Seam", n=(1, 0, 0))
            y, z = y1, z1
    return p, XC


def moon_shard_a():
    """Moon shard A (about 300 m): a huge broken slab of moon, its fracture face flat and banded with exposed strata,
    ember seams glowing in the cracks - whatever stood near the Dominion was broken and swept aside."""
    p, XC = _shard("SM_KS_MoonShard_A", (18000.0, 15000.0, 9000.0), 41, 5)
    p.chull([(-18000.0, y, z) for y in (-15000.0, 15000.0) for z in (-9000.0, 9000.0)] +
            [(XC, y, z) for y in (-15000.0, 15000.0) for z in (-9000.0, 9000.0)])
    return p


def moon_shard_b():
    """Moon shard B (about 180 m): a smaller, chunkier shard from the same moon, the same strata and glowing fracture."""
    p, XC = _shard("SM_KS_MoonShard_B", (10000.0, 8000.0, 7000.0), 47, 4)
    p.chull([(-10000.0, y, z) for y in (-8000.0, 8000.0) for z in (-7000.0, 7000.0)] +
            [(XC, y, z) for y in (-8000.0, 8000.0) for z in (-7000.0, 7000.0)])
    return p


BUILDERS = [picket_ring, hulk_fore, hulk_mid, hulk_aft, scar_rig, moon_shard_a, moon_shard_b]

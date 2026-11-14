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
    """A broken picket-station ring segment (about 120 m across): a 140-degree arc of a 60 m ring, 6 x 4 m in section,
    torn at one end (staggered plates and frames), two dead sensor booms (one bent), a blinking amber light on a short
    mast - the patrol line's station, ours, broken at First Contact."""
    p = Piece("SM_KS_PicketRing", None)
    R, A0, A1, N = 6000.0, -70.0, 70.0, 28
    step = (A1 - A0) / N
    seg = R * math.radians(step) * 0.5 + 6.0
    for k in range(N):
        a = A0 + step * (k + 0.5)
        with p.at(rotz(a)):
            torn = k == N - 1
            p.box((R - 300.0, -seg, -200.0), (R + 300.0, seg if not torn else seg * 0.2, 200.0),
                  regs={"X": "wall" if k % 3 else "wallB", "x": "wallB", "Z": "roof", "z": "sw_dark", "y": "frame", "Y": "frame"},
                  mat="Trim2" if k % 4 == 1 else "Trim")
            if k % 2 == 0:
                p.box((R + 300.0, -seg * 0.6, -60.0), (R + 330.0, seg * 0.6, 60.0), reg="frame")      # outer rib
            if torn:                                                                                  # ragged end
                for (z0, z1, ext) in ((120.0, 200.0, 520.0), (-200.0, -120.0, 340.0), (-40.0, 40.0, 180.0)):
                    p.box((R - 300.0, seg * 0.2, z0), (R + 300.0, seg * 0.2 + ext, z1), reg="wall", mat="Scorch")
    # sensor booms: lattice arms out of the ring, a dead pod at the end (the second bent back)
    for (a, L, bend) in ((-35.0, 4000.0, 0.0), (30.0, 3400.0, 35.0)):
        with p.at(rotz(a)):
            end = (R + 300.0 + L * math.cos(math.radians(bend)), L * math.sin(math.radians(bend)) * 0.4, 0.0)
            for (dy, dz) in ((-60.0, -60.0), (60.0, -60.0), (0.0, 70.0)):
                _tube(p, (R + 300.0, dy, dz), (end[0], end[1] + dy * 0.4, end[2] + dz * 0.4), 14.0, reg="sw_gun")
            p.box((end[0] - 150.0, end[1] - 150.0, -150.0), (end[0] + 250.0, end[1] + 150.0, 150.0), regs={"X": "sw_black"}, reg="frame")
    # the amber light on a short mast
    with p.at(rotz(5.0)):
        p.box((R - 40.0, -40.0, 200.0), (R + 40.0, 40.0, 900.0), reg="pillar")
        p.box((R - 110.0, -110.0, 900.0), (R + 110.0, 110.0, 1120.0), mat="Nav")
    pts = []
    for a in (A0, 0.0, A1):
        for rr in (R - 300.0, R + 300.0 + 4000.0 * (abs(a) < 40)):
            for z in (-200.0, 1120.0 if a == 0.0 else 200.0):
                pts.append((rr * math.cos(math.radians(a)), rr * math.sin(math.radians(a)), z))
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


def hulk_fore():
    """Cruiser bow section (about 75 m): the tapering bow and a dark command blister, cut off behind."""
    p = Piece("SM_KS_Hulk_Fore", None)
    loft(p, 0.0, hull_prof(W2, H2), 4000.0, hull_prof(W2, H2, 0.6), reg="wall", caps=(False, False))
    loft(p, 4000.0, hull_prof(W2, H2, 0.6), 6500.0, hull_prof(W2, H2, 0.08), reg="wallB", mat="Trim2", caps=(False, True))
    p.box((1500.0, -500.0, H2 * 0.45), (3500.0, 500.0, H2 * 0.45 + 450.0), regs={"Z": "roof", "X": "sw_black"}, reg="wall", mat="Trim2")
    _torn_end(p, 0.0, -1.0)
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
    p.box((-80.0, W2 / 2 - 10.0, H2 * 0.2), (80.0, W2 / 2 + 150.0, H2 * 0.2 + 160.0), mat="Cold")
    p.chull([(x, y, z) for x in (-2900.0, 2900.0) for y, z in prof] + [(x, 0.0, H2 * 0.45 + 300.0) for x in (-2200.0, 900.0)])
    return p


def hulk_aft():
    """Cruiser engine section (about 40 m): the aft hull with three dead engine bells, cut off in front."""
    p = Piece("SM_KS_Hulk_Aft", None)
    loft(p, -2000.0, hull_prof(W2, H2, 0.85), 1500.0, hull_prof(W2, H2), reg="wall", caps=(True, False))
    _torn_end(p, 1500.0, 1.0)
    for (y, z) in ((-650.0, -150.0), (650.0, -150.0), (0.0, 450.0)):
        _lathe_x(p, -2000.0, y, z, [(330.0, 0.0), (420.0, -400.0), (650.0, -1200.0), (600.0, -1200.0), (360.0, -400.0), (260.0, 0.0)],
                 14, regs=["sw_gun", "frame", "sw_dark", "sw_black", "sw_black"], mat="Trim2")
    p.box((-2100.0, -80.0, H2 * 0.4), (-1940.0, 80.0, H2 * 0.4 + 160.0), mat="Cold")
    p.chull([(x, y, z) for x in (-3300.0, 2200.0) for y in (-W2 / 2, W2 / 2) for z in (-H2 * 0.6, H2 * 0.5)])
    return p


# ------------------------------------------------------------------------------------------------ Survey Scar
def scar_rig():
    """A Null survey rig clamped across a split asteroid (about 90 m): the rock broken in two with a 12 m gap (its two
    machined faces facing each other), a truss gantry bridging the gap with hazard-yellow edges, a drill arm hanging
    into the split, cable runs down both halves, white eye lights on the gantry head. The rock halves use the
    asteroid material (tiled UVs), the rig the Dominion trim."""
    p = Piece("SM_KS_ScarRig", None)
    R = (3600.0, 3800.0, 3200.0)
    for (seed, yaw, dx) in ((31, 0.0, -600.0), (37, 180.0, 600.0)):
        with p.at(Matrix.Translation((dx, 0.0, 0.0)) @ rotz(yaw)):
            _rock(p, R, seed, XC=0.0, BR=1.0, BD=0.0, NU=36, NV=20)
    G = 3000.0                                                   # gantry height
    for y in (-700.0, 700.0):
        for z in (G, G + 500.0):
            _tube(p, (-2600.0, y, z), (2600.0, y, z), 40.0, reg="hazard" if z == G else "frame", mat="Dom")
        for x in range(-2600, 2601, 650):
            _tube(p, (float(x), y, G), (float(x) + 325.0, y, G + 500.0), 22.0, reg="sw_gun", mat="Dom")
            _tube(p, (float(x), -700.0, G + 500.0), (float(x), 700.0, G + 500.0), 22.0, reg="sw_gun", mat="Dom")
    for (x, y) in ((-2600.0, -700.0), (2600.0, -700.0), (-2600.0, 700.0), (2600.0, 700.0)):       # clamp legs into the rock
        _tube(p, (x, y, G), (x * 1.05, y * 1.4, G - 1500.0), 45.0, reg="frame", mat="Dom")
        p.box((x * 1.05 - 150.0, y * 1.4 - 150.0, G - 1700.0), (x * 1.05 + 150.0, y * 1.4 + 150.0, G - 1450.0), reg="hazard", mat="Dom")
    # drill head on the gantry, drill arm down into the split
    p.box((-500.0, -500.0, G + 500.0), (500.0, 500.0, G + 1300.0), regs={"Z": "roof"}, reg="wallB", mat="Dom")
    for k in range(8):
        a = math.radians(22.5 + 45.0 * k)
        cx, cy = math.cos(a) * 505.0, math.sin(a) * 505.0
        if abs(cx) > 499 or abs(cy) > 499:
            nx, ny = (1.0 if cx > 0 else -1.0, 0.0) if abs(cx) > abs(cy) else (0.0, 1.0 if cy > 0 else -1.0)
            ox, oy = -ny * 50.0, nx * 50.0
            p.poly([(cx - ox, cy - oy, G + 1050.0), (cx + ox, cy + oy, G + 1050.0), (cx + ox, cy + oy, G + 1120.0), (cx - ox, cy - oy, G + 1120.0)],
                   mat="Eye", n=(nx, ny, 0))
    _tube(p, (0.0, 0.0, G), (0.0, 0.0, -1800.0), 120.0, reg="frame", mat="Dom")
    p.box((-250.0, -250.0, -2100.0), (250.0, 250.0, -1800.0), reg="hazard", mat="Dom")
    # cable runs down both halves
    rnd = random.Random(6)
    for s in (-1.0, 1.0):
        prev = (s * 1800.0, -300.0, G)
        for k in range(5):
            nxt = (s * (2000.0 + 400.0 * k), -300.0 + rnd.uniform(-200, 200), G - 700.0 * (k + 1))
            _tube(p, prev, nxt, 25.0, reg="sw_black", mat="Dom")
            prev = nxt
    p.chull([(x, y, z) for x in (-4200.0, 4200.0) for y in (-3800.0, 3800.0) for z in (-3200.0, 3200.0)])
    return p


# ------------------------------------------------------------------------------------------------ Shattered Moons
def _shard(name, R, seed, seams):
    p = Piece(name, None)
    XC = R[0] * 0.35
    _rock(p, R, seed, XC=XC, BR=1.0, BD=0.0, NU=48, NV=26)
    # the fracture face: strata (the cut faces re-slotted) and glowing seams across it
    p.faces = [(v, [((q.y) / 6000.0, (q.z) / 2500.0) for q in v] if m == "Cut" else u, "Strata" if m == "Cut" else m) for v, u, m in p.faces]
    rnd = random.Random(seed + 1)
    for k in range(seams):
        z = -R[2] * 0.5 + R[2] * (k + 0.5) / seams + rnd.uniform(-300, 300)
        y = -R[1] * 0.6
        while y < R[1] * 0.6:
            y1 = y + rnd.uniform(1500, 3000)
            z1 = z + rnd.uniform(-500, 500)
            p.poly([(XC + 30.0, y, z - 60.0), (XC + 30.0, y1, z1 - 60.0), (XC + 30.0, y1, z1 + 60.0), (XC + 30.0, y, z + 60.0)],
                   mat="Glow", n=(1, 0, 0))
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

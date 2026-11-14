# SPDX-License-Identifier: MIT
# Space set pieces for T6's regions (docs/eng/SPACE-DESIGN.md on eng/t6, the set-piece table). Route C: code-built in
# the kit's hard-surface style (bevelled, trim sheet), but read from a ship at 50 m to 2 km: strong silhouettes, big
# shapes, few small parts. Non-walkable; one to four convex hulls each; static meshes the space field can break.
# Unreal centimetres, +X forward (bow), pivot at the centre of mass (the rig: on the rock surface it clamps to).
# Slots: Trim (the planet kit trim; per-piece instances in Unreal: burnt patrol hull, weathered research hull),
# Dom (the Dominion trim), Glow (Null red), Eye (white "eye" lights), Light (lit windows), Rock / Cut / Bore (the rock).
# Nothing carries a name, number or date.
import math
import random

from mathutils import Matrix

from kit_lib import Piece, rotz

M = 100.0      # one metre in cm


def loft(p, x0, prof0, x1, prof1, reg="wall", mat="Trim", caps=(True, True), cap_reg="sw_dark", capmat=None):
    """A section that changes along X: prof0 at x0 to prof1 at x1 (same vertex count, (y, z) pairs, CCW seen from +X)."""
    n = len(prof0)
    for i in range(n):
        a0, b0 = prof0[i], prof0[(i + 1) % n]
        a1, b1 = prof1[i], prof1[(i + 1) % n]
        quad = [(x0, a0[0], a0[1]), (x0, b0[0], b0[1]), (x1, b1[0], b1[1]), (x1, a1[0], a1[1])]
        cy, cz = (a0[0] + b0[0]) * 0.5, (a0[1] + b0[1]) * 0.5
        p.poly(quad, reg, uv=[(0, 0), (1, 0), (1, 1), (0, 1)], mat=mat, n=(0, cy, cz))
    for x, prof, s, on in ((x0, prof0, -1.0, caps[0]), (x1, prof1, 1.0, caps[1])):
        if on:
            p.poly([(x, y, z) for y, z in prof], cap_reg, uv=[(0.5, 0.5)] * len(prof), mat=capmat or mat, n=(s, 0, 0))


def hull_prof(w, h, k=1.0):
    """The patrol corvette's section: a flattened hexagon with a keel, w wide, h tall (cm), scaled by k."""
    return [(y * k, z * k) for y, z in ((-w / 2, -h * 0.15), (-w * 0.36, h * 0.45), (w * 0.36, h * 0.45), (w / 2, -h * 0.15),
                                          (w * 0.28, -h * 0.55), (-w * 0.28, -h * 0.55))]


def _ribs(p, x0, x1, step, prof, t=40.0, reg="frame"):
    """Exposed frames across a torn-open section: thin rings following the hull section."""
    x = x0
    while x <= x1 + 1:
        loft(p, x - t / 2, prof, x + t / 2, prof, reg=reg, caps=(True, True), cap_reg="frame")
        x += step


def _hulls_box(p, lo, hi):
    p.cbox(lo, hi)


# ------------------------------------------------------------------------------------------------ the lost patrol
W_, H_ = 7.0 * M, 4.2 * M


def patrol_bow():
    """Bow section (about 15 m): a tapering wedge nose with a cockpit band, torn off at the back: plates of different
    lengths and exposed frames."""
    p = Piece("SM_SP_Patrol_Bow", None)
    loft(p, 0.0, hull_prof(W_, H_), 900.0, hull_prof(W_, H_, 0.55), reg="wall", caps=(False, False))
    loft(p, 900.0, hull_prof(W_, H_, 0.55), 1500.0, hull_prof(W_, H_, 0.08), reg="wallB", mat="Trim2", caps=(False, True))
    # cockpit band (dark glass)
    loft(p, 700.0, [(-150, 160), (150, 160), (110, 210), (-110, 210)], 1000.0, [(-90, 120), (90, 120), (60, 150), (-60, 150)],
         reg="sw_black", caps=(True, True))
    # torn back: staggered plates (top and the two flanks reach further back than the keel), frames between
    for (y0, y1, z0, z1, back) in ((-W_ * 0.36, W_ * 0.36, H_ * 0.40, H_ * 0.47, -380.0), (-W_ / 2 - 10, -W_ * 0.42, -60, 150, -240.0),
                                   (W_ * 0.42, W_ / 2 + 10, -60, 150, -300.0), (-W_ * 0.28, W_ * 0.28, -H_ * 0.56, -H_ * 0.48, -120.0)):
        p.box((back, y0, z0), (0.0, y1, z1), reg="wall", mat="Scorch")
    _band(p, 60.0, 800.0)
    # the failing distress strobe on the nose spine (white-blue, carries to 1 km)
    p.box((1050.0, -110.0, 150.0), (1270.0, 110.0, 370.0), mat="Strobe")
    _ribs(p, -300.0, 0.0, 150.0, hull_prof(W_ * 0.9, H_ * 0.9))
    # cold blue glow from the breached section, seen through the torn back
    p.poly([(-5.0, -W_ * 0.3, -H_ * 0.4), (-5.0, W_ * 0.3, -H_ * 0.4), (-5.0, W_ * 0.3, H_ * 0.3), (-5.0, -W_ * 0.3, H_ * 0.3)],
           mat="Breach", n=(-1, 0, 0))
    p.chull([(x, y, z) for x in (-380.0, 1500.0) for y, z in hull_prof(W_, H_, 1.0 if x < 0 else 0.1)])
    return p


def _band(p, x0, x1, w=W_, h=H_):
    """The squad's muted cyan band along both flanks (slot Stripe: the light trim tinted cyan) - it reads as ours."""
    for s in (-1.0, 1.0):
        y = s * (w / 2 + 6.0)
        p.poly([(x0, y, -h * 0.1), (x1, y, -h * 0.1), (x1, y, h * 0.05), (x0, y, h * 0.05)], "frame", mat="Stripe", n=(0, s, 0),
               uv=[(0, 0), (1, 0), (1, 1), (0, 1)])
        # r5: a steady dim cyan running glow under the band (the strobe alone is dark half the time)
        p.poly([(x0, y + s * 1.0, -h * 0.16), (x1, y + s * 1.0, -h * 0.16), (x1, y + s * 1.0, -h * 0.12), (x0, y + s * 1.0, -h * 0.12)],
               mat="CyanRun", n=(0, s, 0))


def patrol_mid():
    """Mid hull (about 18 m): the torn-open spine: an intact half-shell with frames standing out of the open side, a
    cargo bay with a hatch, a squad-colour stripe burnt to a dark band."""
    p = Piece("SM_SP_Patrol_Mid", None)
    half = [(0.0, -H_ * 0.55), (W_ * 0.28, -H_ * 0.55), (W_ / 2, -H_ * 0.15), (W_ * 0.36, H_ * 0.45), (0.0, H_ * 0.45),
            (0.0, H_ * 0.30), (W_ * 0.28, H_ * 0.30), (W_ * 0.36, -H_ * 0.12), (W_ * 0.2, -H_ * 0.42), (0.0, -H_ * 0.42)]
    p.prism([(y, z) for y, z in half], "yz", -900.0, 900.0, regs="wall", cap_reg="sw_dark")
    # the -Y side is open: frames standing out, broken at uneven heights
    rnd = random.Random(7)
    for k in range(7):
        x = -780.0 + k * 260.0
        top = H_ * (0.1 + 0.35 * rnd.random())
        p.box((x - 20, -W_ * 0.45, -H_ * 0.5), (x + 20, 0.0, -H_ * 0.42), reg="frame")
        p.box((x - 20, -W_ * 0.47, -H_ * 0.45), (x + 20, -W_ * 0.4, top), reg="frame", mat="Scorch")
    p.box((-900.0, -W_ * 0.3, -H_ * 0.55), (900.0, 0.0, -H_ * 0.45), reg="floor")      # keel deck, exposed
    p.box((-300.0, W_ * 0.30, -40), (300.0, W_ * 0.52, 140), reg="hatch", mat="Trim2")  # cargo hatch bulge
    # the breach: a cold blue glow on the inner wall of the open side, and scorch round it
    p.poly([(-700.0, -2.0, -H_ * 0.4), (700.0, -2.0, -H_ * 0.4), (700.0, -2.0, H_ * 0.25), (-700.0, -2.0, H_ * 0.25)], mat="Breach", n=(0, -1, 0))
    p.box((-900.0, -W_ * 0.05, H_ * 0.3), (900.0, 0.0, H_ * 0.45), reg="wall", mat="Scorch")
    _band(p, -880.0, 880.0)
    # outward breach on the intact +Y flank: a torn hole glowing cold blue, scorch round it
    p.poly([(-350.0, W_ / 2 + 4.0, -120.0), (350.0, W_ / 2 + 4.0, -120.0), (250.0, W_ / 2 + 4.0, 150.0), (-280.0, W_ / 2 + 4.0, 120.0)],
           mat="Breach", n=(0, 1, 0))
    p.poly([(-560.0, W_ / 2 + 2.0, -260.0), (560.0, W_ / 2 + 2.0, -260.0), (460.0, W_ / 2 + 2.0, 260.0), (-500.0, W_ / 2 + 2.0, 240.0)],
           "wall", mat="Scorch", n=(0, 1, 0), uv=[(0, 0), (1, 0), (1, 1), (0, 1)])
    p.chull([(x, y, z) for x in (-900.0, 900.0) for y, z in ((-W_ / 2, -H_ * 0.55), (W_ / 2, -H_ * 0.55), (W_ / 2, H_ * 0.45), (-W_ / 2, H_ * 0.45))])
    return p


def patrol_aft():
    """Engine section (about 14 m): the aft hull with two engine pods and dead nozzles, torn at the front."""
    p = Piece("SM_SP_Patrol_Aft", None)
    loft(p, -700.0, hull_prof(W_, H_, 0.9), 500.0, hull_prof(W_, H_), reg="wall", caps=(True, False))
    _ribs(p, 520.0, 700.0, 180.0, hull_prof(W_ * 0.95, H_ * 0.95))
    _band(p, -650.0, 450.0, W_, H_)
    p.poly([(505.0, -W_ * 0.3, -H_ * 0.4), (505.0, W_ * 0.3, -H_ * 0.4), (505.0, W_ * 0.3, H_ * 0.3), (505.0, -W_ * 0.3, H_ * 0.3)],
           mat="Breach", n=(1, 0, 0))
    for s in (-1.0, 1.0):
        with p.at(Matrix.Translation((-150.0, s * (W_ / 2 + 70.0), 20.0)) @ Matrix.Rotation(math.radians(-90.0), 4, "Y")):
            p.lathe([(0.0, -600.0), (105.0, -560.0), (120.0, -300.0), (120.0, 300.0), (105.0, 520.0), (80.0, 600.0)], 12,
                    regs=["sw_dark", "frame", "wall", "frame", "sw_gun"], mat="Trim2")
            p.lathe([(80.0, 600.0), (100.0, 700.0), (45.0, 700.0)], 12, regs=["sw_gun", "sw_black"])   # nozzle (rear = -X)
        p.box((-200.0, s * W_ * 0.4, -40.0), (200.0, s * (W_ / 2 + 60.0), 80.0) if s > 0 else (200.0, s * W_ * 0.4, 80.0), reg="frame")
    p.chull([(x, y, z) for x in (-1400.0, 700.0) for y in (-W_ / 2 - 300.0, W_ / 2 + 300.0) for z in (-H_ * 0.55, H_ * 0.45)])
    return p


def patrol_wing():
    """A sheared-off wing plate (about 12 m) with a broken trailing edge and a dead running light."""
    p = Piece("SM_SP_Patrol_Wing", None)
    plan = [(0.0, 0.0), (900.0, 0.0), (1150.0, 500.0), (700.0, 1200.0), (450.0, 1000.0), (300.0, 1250.0), (0.0, 600.0)]
    p.prism(plan, "xy", -35.0, 35.0, regs="wall", cap_reg="wallB", mat="Trim2")
    p.prism([(0.0, 0.0), (900.0, 0.0), (1000.0, 200.0), (0.0, 200.0)], "xy", 35.0, 45.0, regs="sw_dark", cap_reg="wall", mat="Scorch")
    p.box((850.0, -20.0, -45.0), (950.0, 20.0, 45.0), reg="sw_dark")
    p.chull([(x, y, z) for x, y in plan for z in (-35.0, 35.0)])
    return p


# ------------------------------------------------------------------------------------------------ research expedition
def _lathe_x(p, x, y, z, prof, n=16, regs="wall", mat="Trim"):
    """A body of revolution along +X (prof = (r, along) pairs, along measured +X from (x, y, z))."""
    with p.at(Matrix.Translation((x, y, z)) @ Matrix.Rotation(math.radians(90.0), 4, "Y")):
        # (r, -a) runs toward -Z in the lathe's frame, which would turn the normals inward: walk the profile backwards
        pr = [(r, -a) for r, a in prof][::-1]
        rg = regs if isinstance(regs, str) else list(regs)[::-1]
        p.lathe(pr, n, regs=rg, mat=mat)


def research_a():
    """Research hull A (about 160 m): an old long-haul expedition ship: a lattice spine with four round modules, a
    rotating-ring frame, a blunt command block with a few lit windows, three engine bells. Rounded and civilian next to
    the patrol's hard facets."""
    p = Piece("SM_SP_Research_A", None)
    L = 16000.0
    # spine: two parallel box beams with cross members
    for s in (-1.0, 1.0):
        p.box((-L / 2 + 1200, s * 350 - 60, -60), (L / 2 - 2200, s * 350 + 60, 60), reg="frame")
    for x in range(int(-L / 2 + 1400), int(L / 2 - 2200), 900):
        p.box((x - 40, -350, -40), (x + 40, 350, 40), reg="sw_gun")
    # four modules
    for i, x in enumerate((-4200.0, -1400.0, 1400.0, 4200.0)):
        if i == 2:
            # the blown-out module: a third of its shell gone, the rest scorched, its debris drifting out of the hole
            with p.at(Matrix.Translation((x - 1200.0, 0.0, 0.0)) @ Matrix.Rotation(math.radians(90.0), 4, "Y")):
                pr = [(r, -a) for r, a in [(0.0, 0.0), (650.0, 120.0), (1100.0, 550.0), (1100.0, 1850.0), (650.0, 2280.0), (0.0, 2400.0)]][::-1]
                p.lathe(pr, 16, regs=["sw_dark", "frame", "wallB", "frame", "sw_dark"], mat="Scorch", a0=120.0, a1=360.0)
                p.lathe(pr[::-1], 16, regs="sw_black", mat="Scorch", a0=120.0, a1=360.0)     # the inside, dark
            rnd = random.Random(19)
            for k in range(26):                                   # the venting haze: shards drifting out of the hole
                a = math.radians(40.0 + rnd.uniform(-35.0, 35.0))
                r = 1300.0 + rnd.random() ** 1.5 * 3200.0
                cx, cy, cz = x - 900.0 + rnd.uniform(0.0, 1800.0), math.cos(a) * r, math.sin(a) * r
                sz = rnd.uniform(40.0, 220.0) * (1.0 - (r - 1300.0) / 4500.0)
                with p.at(Matrix.Translation((cx, cy, cz)) @ Matrix.Rotation(rnd.uniform(0, 6.28), 4, (rnd.random(), rnd.random(), 1.0))):
                    p.box((-sz, -sz * 0.5, -sz * 0.15), (sz, sz * 0.5, sz * 0.15), reg="wallB", mat="Scorch" if k % 3 else "Trim")
            continue
        _lathe_x(p, x - 1200.0, 0.0, 0.0, [(0.0, 0.0), (650.0, 120.0), (1100.0, 550.0), (1100.0, 1850.0), (650.0, 2280.0), (0.0, 2400.0)],
                 16, regs=["sw_dark", "frame", "wall" if i % 2 else "wallB", "frame", "sw_dark"], mat="Trim2" if i % 2 else "Trim")
        # a window row round each module: lit (0), flickering (2), dark (1, 3: the dead sections)
        if i == 0:
            for k in range(0, 16, 2):
                a0, a1 = 2 * math.pi * k / 16 + 0.06, 2 * math.pi * (k + 1) / 16 - 0.06
                for xx in (x - 900.0, x + 300.0):
                    q = [(xx, 1112.0 * math.cos(a0), 1112.0 * math.sin(a0)), (xx + 450.0, 1112.0 * math.cos(a0), 1112.0 * math.sin(a0)),
                         (xx + 450.0, 1112.0 * math.cos(a1), 1112.0 * math.sin(a1)), (xx, 1112.0 * math.cos(a1), 1112.0 * math.sin(a1))]
                    if (k // 2 + (1 if xx > x else 0)) % 3:
                        p.poly(q, mat="Light" if i == 0 else "Flicker", n=(0, math.cos((a0 + a1) / 2), math.sin((a0 + a1) / 2)))
    # the ring frame (a flat octagonal ring round the spine, edge-on it reads as a halo)
    with p.at(Matrix.Translation((-2800.0, 0.0, 0.0)) @ Matrix.Rotation(math.radians(90.0), 4, "Y")):
        p.lathe([(2200.0, -150.0), (2200.0, 150.0), (1950.0, 150.0), (1950.0, -150.0)], 16, regs=["wall", "frame", "sw_dark", "frame"])
    for a in range(0, 360, 90):
        with p.at(Matrix.Translation((-2800.0, 0.0, 0.0)) @ Matrix.Rotation(math.radians(a), 4, "X")):
            p.box((-80.0, -60.0, 300.0), (80.0, 60.0, 1960.0), reg="frame")
    # command block at the bow with window strips
    p.box((L / 2 - 2200, -900, -700), (L / 2 - 200, 900, 800), regs={"Z": "roof", "z": "sw_dark", "x": "wall", "X": "wallB", "y": "wall", "Y": "wall"}, mat="Trim2")
    loft(p, L / 2 - 200, [(-900, -700), (900, -700), (900, 800), (-900, 800)], L / 2, [(-500, -400), (500, -400), (500, 500), (-500, 500)],
         reg="wallB")
    for x0, x1, z in ((L / 2 - 2000, L / 2 - 1400, 450.0), (L / 2 - 1100, L / 2 - 700, 450.0), (L / 2 - 1700, L / 2 - 900, 150.0)):
        for s in (-1.0, 1.0):
            yy = s * 902.0
            p.poly([(x0, yy, z), (x1, yy, z), (x1, yy, z + 60), (x0, yy, z + 60)], mat="Light", n=(0, s, 0))
    # running lights: bow, ring, stern; a dim amber nav light on the command block (carries to 1 km)
    for (x, y, z) in ((L / 2 + 5, 0.0, 0.0), (-2800.0, 0.0, 2215.0), (-2800.0, 0.0, -2215.0), (-L / 2 + 600, 0.0, 520.0)):
        p.box((x - 60, y - 60, z - 60), (x + 60, y + 60, z + 60), mat="Run")
    p.box((L / 2 - 1300, -100, 800), (L / 2 - 1100, 100, 1000), reg="frame")
    p.box((L / 2 - 1350, -150, 1000), (L / 2 - 1050, 150, 1300), mat="Nav")
    # engines: three bells on a thrust frame
    p.box((-L / 2 + 600, -900, -500), (-L / 2 + 1400, 900, 500), reg="frame")
    for (y, z) in ((-550.0, -150.0), (550.0, -150.0), (0.0, 380.0)):
        _lathe_x(p, -L / 2 + 600, y, z, [(250.0, 0.0), (330.0, -300.0), (520.0, -900.0), (470.0, -900.0), (280.0, -300.0), (200.0, 0.0)],
                 14, regs=["sw_gun", "frame", "sw_dark", "sw_black", "sw_black"])
    p.chull([(x, y, z) for x in (-L / 2 - 400, L / 2) for y in (-950, 950) for z in (-750, 850)])
    p.chull([(-2800.0 + dx, y, z) for dx in (-150, 150) for y, z in ((-2200, 0), (2200, 0), (0, -2200), (0, 2200),
                                                                    (-1556, -1556), (1556, 1556), (-1556, 1556), (1556, -1556))])
    return p


def research_b():
    """Research hull B (about 110 m): a survey tender with a broken back: a box hull torn through two thirds of the way
    aft (frames across the gap), a truss spine over it carrying stacked equipment blocks, side tanks, an antenna farm at
    the stern, a broken dish on a mast, a bridge with a lit row. Older and blunter than A, so the line of wrecks varies."""
    p = Piece("SM_SP_Research_B", None)
    hull = {"Z": "roof", "z": "sw_dark", "x": "frame", "X": "frame", "y": "wall", "Y": "wallB"}
    # r3: intact and abandoned at the front, torn only at the back (the stern section hangs on by the keel)
    p.box((-5000, -800, -600), (-3700, 800, 600), regs=hull)
    p.box((-2800, -800, -600), (-400, 800, 600), regs=hull, mat="Trim2")
    p.box((-400, -800, -600), (3800, 800, 600), regs=hull)
    for x in (-3600.0, -3250.0, -2900.0):                       # the torn gap: frames and a broken keel
        p.box((x - 40, -800, -600), (x + 40, 800, -500), reg="frame", mat="Scorch")
        p.box((x - 40, -820, -500), (x + 40, -740, 300 - (x + 3600) * 0.3), reg="frame", mat="Scorch")
        p.box((x - 40, 740, -500), (x + 40, 820, 450 + (x + 3600) * 0.2), reg="frame", mat="Scorch")
    p.box((3300, -100, 1100), (3500, 100, 1500), reg="frame")
    p.box((3250, -150, 1500), (3550, 150, 1800), mat="Nav")
    loft(p, 3800, [(-800, -600), (800, -600), (800, 600), (-800, 600)], 5000, [(-450, -350), (450, -350), (450, 250), (-450, 250)],
         reg="wall", mat="Trim2")
    # truss spine over the hull (it spans the break) with cross members
    for y in (-450.0, 450.0):
        p.box((-4600, y - 50, 1250), (3000, y + 50, 1350), reg="frame")
        p.box((-4600, y - 50, 650), (3000, y + 50, 750), reg="frame")
    for x in range(-4600, 3000, 500):
        for y in (-450.0, 450.0):
            p.box((x - 30, y - 30, 600), (x + 30, y + 30, 1350), reg="sw_gun")
    rnd = random.Random(3)
    x = -4400.0
    while x < 2600.0:
        w = 500.0 + rnd.random() * 700.0
        hgt = 300.0 + rnd.random() * 500.0
        if not (-3900.0 < x < -2700.0):
            p.box((x, -600.0, 1350.0), (x + w, 600.0, 1350.0 + hgt), regs={"Z": "vent", "x": "frame", "X": "frame", "y": "wall", "Y": "wallB"},
                  mat="Trim2" if rnd.random() < 0.4 else "Trim")
        x += w + 200.0
    # side tanks (three a side) on short struts
    for s_ in (-1.0, 1.0):
        for x0 in (-4400.0, -3200.0, 1200.0):
            _lathe_x(p, x0, s_ * 1100.0, -150.0, [(0.0, 0.0), (220.0, 80.0), (300.0, 300.0), (300.0, 800.0), (220.0, 1020.0), (0.0, 1100.0)],
                     12, regs=["sw_dark", "frame", "wallB", "frame", "sw_dark"], mat="Trim2")
            ya, yb = sorted((s_ * 790.0, s_ * 820.0 + s_ * 20.0))
            p.box((x0 + 450, ya, -200.0), (x0 + 650, yb, -100.0), reg="frame")
    # bridge with a lit row; a flickering row and a few lit cabins along the hull
    p.box((3000.0, -600.0, 600.0), (3900.0, 600.0, 1100.0), regs={"Z": "roof", "X": "wallB", "x": "wall", "y": "wall", "Y": "wall"})
    p.poly([(3901.0, -500.0, 850.0), (3901.0, 500.0, 850.0), (3901.0, 500.0, 950.0), (3901.0, -500.0, 950.0)], mat="Light", n=(1, 0, 0))
    p.poly([(2400.0, -802.0, 200.0), (3500.0, -802.0, 200.0), (3500.0, -802.0, 260.0), (2400.0, -802.0, 260.0)], mat="Flicker", n=(0, -1, 0))
    for xw in (-4300.0, -3500.0, -2600.0):
        p.poly([(xw, 802.0, 150.0), (xw + 500.0, 802.0, 150.0), (xw + 500.0, 802.0, 210.0), (xw, 802.0, 210.0)], mat="Light", n=(0, 1, 0))
    # mast and a broken dish (a tilted shallow cone, a third of it missing)
    p.box((-1500.0, -60.0, 1350.0), (-1380.0, 60.0, 2700.0), reg="frame")
    with p.at(Matrix.Translation((-1440.0, 0.0, 2800.0)) @ Matrix.Rotation(math.radians(-35.0), 4, "Y")):
        p.lathe([(0.0, 0.0), (300.0, 60.0), (900.0, 260.0)], 16, regs=["sw_gun", "wallB"], a0=0.0, a1=245.0, mat="Trim2")
        p.lathe([(900.0, 262.0), (300.0, 62.0), (0.0, 2.0)], 16, regs=["sw_dark", "sw_dark"], a0=0.0, a1=245.0)
    # antenna farm at the stern
    for i, (ax, ay) in enumerate(((-4500.0, -400.0), (-4200.0, 300.0), (-3900.0, -150.0), (-3600.0, 450.0), (-4700.0, 150.0))):
        top = 1350.0 + 900.0 + i * 260.0
        p.box((ax - 20, ay - 20, 1350.0), (ax + 20, ay + 20, top), reg="sw_gun")
        p.box((ax - 20, ay - 180, top - 300.0), (ax + 20, ay + 180, top - 260.0), reg="sw_gun")
        if i % 2 == 0:
            p.box((ax - 40, ay - 40, top), (ax + 40, ay + 40, top + 80), mat="Run")
    # engine block
    p.box((-5800.0, -700.0, -500.0), (-5000.0, 700.0, 500.0), reg="frame")
    for y in (-380.0, 380.0):
        _lathe_x(p, -5800.0, y, 0.0, [(220.0, 0.0), (420.0, -700.0), (380.0, -700.0), (200.0, 0.0)], 12, regs=["frame", "sw_black", "sw_black"])
    p.chull([(x, y, z) for x in (-6500.0, 5000.0) for y in (-1420.0, 1420.0) for z in (-620.0, 2200.0)])
    return p


# ------------------------------------------------------------------------------------------------ the Survey Scar
def null_rig():
    """A Null survey rig (about 40 m tall over the rock, 44 m across the clamps): a black armoured drill head on four
    clamp legs bitten into the rock (pivot = the rock surface, legs reach Z 0 and below), a drill string bored into the
    rock, red glow seams and a ring of white "eye" lights round the head - the enemy's colour code."""
    p = Piece("SM_SP_NullRig", None)
    H = 2600.0
    # head: an octagonal armoured block
    oct8 = [(math.cos(math.radians(22.5 + 45 * k)) * 900.0, math.sin(math.radians(22.5 + 45 * k)) * 900.0) for k in range(8)]
    oct8s = [(y * 0.7, z * 0.7) for y, z in oct8]
    with p.at(Matrix.Rotation(math.radians(-90.0), 4, "Y")):          # loft runs along +X; turned so it runs up +Z
        loft(p, H, oct8, H + 900.0, oct8, reg="wallB", mat="Dom", caps=(True, False), cap_reg="sw_dark")
        loft(p, H + 900.0, oct8, H + 1500.0, oct8s, reg="roof", mat="Dom", caps=(False, True), cap_reg="frame")
    # red seam round the head and white eyes above it
    for k in range(8):
        a0, a1 = math.radians(22.5 + 45 * k), math.radians(22.5 + 45 * (k + 1))
        p0 = (math.cos(a0) * 905.0, math.sin(a0) * 905.0)
        p1 = (math.cos(a1) * 905.0, math.sin(a1) * 905.0)
        am = (a0 + a1) * 0.5
        p.poly([(p0[0], p0[1], H + 340.0), (p1[0], p1[1], H + 340.0), (p1[0], p1[1], H + 560.0), (p0[0], p0[1], H + 560.0)],
               mat="Glow", n=(math.cos(am), math.sin(am), 0))
        ex, ey = math.cos(am) * 815.0, math.sin(am) * 815.0
        for d in (-120.0, 120.0):
            cx, cy = ex - math.sin(am) * d, ey + math.cos(am) * d
            tx, ty = -math.sin(am) * 45.0, math.cos(am) * 45.0
            p.poly([(cx - tx, cy - ty, H + 640.0), (cx + tx, cy + ty, H + 640.0), (cx + tx, cy + ty, H + 700.0), (cx - tx, cy - ty, H + 700.0)],
                   mat="Eye", n=(math.cos(am), math.sin(am), 0))
    # a slow red beacon on the crown
    p.box((-60.0, -60.0, H + 1500.0), (60.0, 60.0, H + 1800.0), reg="frame", mat="Dom")
    p.box((-260.0, -260.0, H + 1800.0), (260.0, 260.0, H + 2250.0), mat="Beacon")
    # drill string: from the head down into the rock
    p.lathe([(260.0, -1200.0), (260.0, H), (0.0, H)], 12, regs=["frame", "sw_dark"], mat="Dom")
    p.lathe([(0.0, -1500.0), (260.0, -1200.0)], 12, regs="sw_gun", mat="Dom")
    # four clamp legs: from under the head out and down to claws biting into the rock at r 2200
    for k in range(4):
        with p.at(rotz(45.0 + 90.0 * k)):
            prof = [(500.0, H + 200.0), (800.0, H + 300.0), (2350.0, 300.0), (2300.0, -250.0), (2050.0, -250.0), (2050.0, 300.0)]
            p.prism(prof, "xz", -110.0, 110.0, regs=["wallB", "wall", "sw_dark", "frame", "sw_dark", "wall"], cap_reg="frame", mat="Dom")
            p.poly([(1450.0, -112.0, 1400.0), (1550.0, -112.0, 1330.0), (1550.0, -112.0, 1380.0), (1450.0, -112.0, 1450.0)], mat="Glow", n=(0, -1, 0))
            p.box((2000.0, -200.0, -300.0), (2400.0, 200.0, 0.0), reg="hazard", mat="Dom")         # claw
    p.chull([(x, y, z) for x in (-900.0, 900.0) for y in (-900.0, 900.0) for z in (H, H + 1500.0)])
    p.chull([(r * math.cos(math.radians(45 + 90 * k)), r * math.sin(math.radians(45 + 90 * k)), z)
             for k in range(4) for (r, z) in ((2400.0, -300.0), (700.0, H + 300.0))])
    return p


def _rock(p, R, seed, XC=None, BR=950.0, BD=2600.0, ZT=None, NU=48, NV=26):
    """A lumpy asteroid on a lat / long grid. XC: a face cut flat at x = XC (slot Cut) with a bore of radius BR and depth
    BD into it (slot Bore). ZT: the top levelled at z = ZT (slot Cut: a pad the rigs flattened). Rock UVs tile 4 x 2 so
    the asteroid material's normal map keeps its scale."""
    rnd = random.Random(seed)
    ph = [rnd.random() * 6.28 for _ in range(6)]

    def rad(u, v):
        return 1.0 + 0.10 * math.sin(3 * u + ph[0]) * math.sin(2 * v + ph[1]) + 0.06 * math.sin(5 * u + ph[2]) * math.cos(4 * v + ph[3]) \
            + 0.04 * math.sin(9 * u + ph[4] + 3 * v)

    pts, kind = {}, {}
    for i in range(NU + 1):
        u = 2 * math.pi * i / NU
        for j in range(NV + 1):
            v = math.pi * j / NV
            k = rad(u, v) * (1.0 + (rnd.random() - 0.5) * 0.03 if 0 < j < NV else 1.0)
            x = R[0] * k * math.sin(v) * math.cos(u)
            y = R[1] * k * math.sin(v) * math.sin(u)
            z = R[2] * k * math.cos(v)
            t = "rock"
            if XC is not None and x > XC:
                x, t = XC, "cut"
                if math.hypot(y - 200.0, z + 300.0) < BR:
                    x, t = XC - BD, "bore"
            if ZT is not None and z > ZT:
                z, t = ZT, "pad"
            pts[i % NU, j], kind[i % NU, j] = (x, y, z), t
    for i in range(NU):
        for j in range(NV):
            q = [pts[i, j], pts[(i + 1) % NU, j], pts[(i + 1) % NU, j + 1], pts[i, j + 1]]
            ks = {kind[i, j], kind[(i + 1) % NU, j], kind[(i + 1) % NU, j + 1], kind[i, j + 1]}
            pad = ks == {"pad"}
            mat = "Bore" if "bore" in ks else ("Cut" if ks == {"cut"} or pad else "Rock")
            uvq = [(4.0 * i / NU, 2.0 * j / NV), (4.0 * (i + 1) / NU, 2.0 * j / NV), (4.0 * (i + 1) / NU, 2.0 * (j + 1) / NV), (4.0 * i / NU, 2.0 * (j + 1) / NV)]
            if mat != "Rock":
                uvq = [((q_[0] if pad else q_[1]) / 2000.0, (q_[1] if pad else q_[2]) / 2000.0) for q_ in q]
            if j == 0:
                q, uvq = [q[0], q[2], q[3]], [uvq[0], uvq[2], uvq[3]]
            elif j == NV - 1:
                q, uvq = q[:3], uvq[:3]
            cx = sum(a[0] for a in q) / len(q)
            cy = sum(a[1] for a in q) / len(q)
            cz = sum(a[2] for a in q) / len(q)
            if mat == "Rock":
                nrm = (cx, cy, cz)
            elif pad:
                nrm = (0.0, 0.0, 1.0)
            elif mat == "Cut":
                nrm = (1.0, 0.0, 0.0)
            else:
                nrm = (1.0, -(cy - 200.0), -(cz + 300.0))
                if all(abs(a[0] - (XC - BD)) < 1 for a in q):
                    nrm = (1.0, 0.0, 0.0)
            p.poly(q, uv=uvq, mat=mat, n=nrm)
    p.fix_normals = False
    p.smooth_deg = 50.0
    return list(pts.values())


def hollow_rock():
    """A mined-out rock (about 70 m): a lumpy asteroid with one face cut flat by the rigs (a lighter, machined face) and a
    bore driven 26 m into it, ringed by work lights, with rail stubs running across the face to its mouth - the yield
    taken, the pattern visible. Slots: Rock, Cut, Bore, Work, Dom."""
    p = Piece("SM_SP_HollowRock", None)
    XC, BR = 2500.0, 950.0
    allp = _rock(p, (4200.0, 3500.0, 3000.0), 11, XC=XC, BR=BR)
    for k in range(16):                                         # work lights round the bore mouth
        a = 2 * math.pi * k / 16
        cy, cz = 200.0 + (BR + 90.0) * math.cos(a), -300.0 + (BR + 90.0) * math.sin(a)
        p.box((XC, cy - 35.0, cz - 35.0), (XC + 25.0, cy + 35.0, cz + 35.0), mat="Work")
    # depth cues inside the bore: two rails along its floor, a parked hauler-sized block, a darker far wall
    XB = XC - 2600.0
    for yy in (-80.0, 480.0):
        p.box((XB + 50.0, yy - 30.0, -300.0 - BR + 40.0), (XC, yy + 30.0, -300.0 - BR + 110.0), reg="frame", mat="Dom")
    p.box((XB + 500.0, -120.0, -300.0 - BR + 110.0), (XB + 1300.0, 520.0, -300.0 - BR + 520.0), regs={"X": "hatch", "Z": "roof"}, reg="wallB", mat="Dom")
    p.box((XB + 1300.0, 0.0, -300.0 - BR + 450.0), (XB + 1330.0, 400.0, -300.0 - BR + 500.0), mat="Work")
    p.poly([(XB + 5.0, 200.0 - BR * 0.8, -300.0 - BR * 0.8), (XB + 5.0, 200.0 + BR * 0.8, -300.0 - BR * 0.8),
            (XB + 5.0, 200.0 + BR * 0.8, -300.0 + BR * 0.8), (XB + 5.0, 200.0 - BR * 0.8, -300.0 + BR * 0.8)], mat="BoreEnd", n=(1, 0, 0),
           uv=[(0, 0), (1, 0), (1, 1), (0, 1)])
    for z in (-420.0, -180.0):                                  # rail stubs from the bore toward the face edge
        p.box((XC, 200.0 + BR + 150.0, z - 30.0), (XC + 50.0, 200.0 + BR + 1900.0, z + 30.0), reg="frame", mat="Dom")
        for yy in range(int(200 + BR + 250), int(200 + BR + 1900), 300):
            p.box((XC, yy - 25.0, z - 60.0), (XC + 30.0, yy + 25.0, z + 60.0), reg="sw_gun", mat="Dom")
    p.chull([a for a in allp if a[0] < XC - 1] + [(XC + 50.0, a[1], a[2]) for a in allp if abs(a[0] - XC) < 1])
    return p


def rig_rock():
    """The rock a Null survey rig clamps to (about 60 m): the same asteroid look with its top levelled into a pad (slot
    Cut) about 26 m across at Z 0, so the rig's claws sit on it. Pivot = the pad centre."""
    p = Piece("SM_SP_RigRock", None)
    ZT = 1400.0
    with p.at(Matrix.Translation((0.0, 0.0, -ZT))):
        allp = _rock(p, (3200.0, 3000.0, 2600.0), 23, ZT=ZT, NU=40, NV=22)
    p.chull([(a[0], a[1], a[2] - ZT) for a in allp])
    return p


BUILDERS = [patrol_bow, patrol_mid, patrol_aft, patrol_wing, research_a, research_b, null_rig, hollow_rock, rig_rock]

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
    loft(p, 900.0, hull_prof(W_, H_, 0.55), 1500.0, hull_prof(W_, H_, 0.08), reg="wallB", caps=(False, True))
    # cockpit band (dark glass)
    loft(p, 700.0, [(-150, 160), (150, 160), (110, 210), (-110, 210)], 1000.0, [(-90, 120), (90, 120), (60, 150), (-60, 150)],
         reg="sw_black", caps=(True, True))
    # torn back: staggered plates (top and the two flanks reach further back than the keel), frames between
    for (y0, y1, z0, z1, back) in ((-W_ * 0.36, W_ * 0.36, H_ * 0.40, H_ * 0.47, -380.0), (-W_ / 2 - 10, -W_ * 0.42, -60, 150, -240.0),
                                   (W_ * 0.42, W_ / 2 + 10, -60, 150, -300.0), (-W_ * 0.28, W_ * 0.28, -H_ * 0.56, -H_ * 0.48, -120.0)):
        p.box((back, y0, z0), (0.0, y1, z1), reg="wall")
    _ribs(p, -300.0, 0.0, 150.0, hull_prof(W_ * 0.9, H_ * 0.9))
    p.chull([(x, y, z) for x in (-380.0, 1500.0) for y, z in hull_prof(W_, H_, 1.0 if x < 0 else 0.1)])
    return p


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
        p.box((x - 20, -W_ * 0.47, -H_ * 0.45), (x + 20, -W_ * 0.4, top), reg="frame")
    p.box((-900.0, -W_ * 0.3, -H_ * 0.55), (900.0, 0.0, -H_ * 0.45), reg="floor")      # keel deck, exposed
    p.box((-300.0, W_ * 0.30, -40), (300.0, W_ * 0.52, 140), reg="hatch")              # cargo hatch bulge
    p.chull([(x, y, z) for x in (-900.0, 900.0) for y, z in ((-W_ / 2, -H_ * 0.55), (W_ / 2, -H_ * 0.55), (W_ / 2, H_ * 0.45), (-W_ / 2, H_ * 0.45))])
    return p


def patrol_aft():
    """Engine section (about 14 m): the aft hull with two engine pods and dead nozzles, torn at the front."""
    p = Piece("SM_SP_Patrol_Aft", None)
    loft(p, -700.0, hull_prof(W_, H_, 0.9), 500.0, hull_prof(W_, H_), reg="wall", caps=(True, False))
    _ribs(p, 520.0, 700.0, 180.0, hull_prof(W_ * 0.95, H_ * 0.95))
    for s in (-1.0, 1.0):
        with p.at(Matrix.Translation((-150.0, s * (W_ / 2 + 70.0), 20.0)) @ Matrix.Rotation(math.radians(-90.0), 4, "Y")):
            p.lathe([(0.0, -600.0), (105.0, -560.0), (120.0, -300.0), (120.0, 300.0), (105.0, 520.0), (80.0, 600.0)], 12,
                    regs=["sw_dark", "frame", "wall", "frame", "sw_gun"])
            p.lathe([(80.0, 600.0), (100.0, 700.0), (45.0, 700.0)], 12, regs=["sw_gun", "sw_black"])   # nozzle (rear = -X)
        p.box((-200.0, s * W_ * 0.4, -40.0), (200.0, s * (W_ / 2 + 60.0), 80.0) if s > 0 else (200.0, s * W_ * 0.4, 80.0), reg="frame")
    p.chull([(x, y, z) for x in (-1400.0, 700.0) for y in (-W_ / 2 - 300.0, W_ / 2 + 300.0) for z in (-H_ * 0.55, H_ * 0.45)])
    return p


def patrol_wing():
    """A sheared-off wing plate (about 12 m) with a broken trailing edge and a dead running light."""
    p = Piece("SM_SP_Patrol_Wing", None)
    plan = [(0.0, 0.0), (900.0, 0.0), (1150.0, 500.0), (700.0, 1200.0), (450.0, 1000.0), (300.0, 1250.0), (0.0, 600.0)]
    p.prism(plan, "xy", -35.0, 35.0, regs="wall", cap_reg="wallB")
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
        _lathe_x(p, x - 1200.0, 0.0, 0.0, [(0.0, 0.0), (650.0, 120.0), (1100.0, 550.0), (1100.0, 1850.0), (650.0, 2280.0), (0.0, 2400.0)],
                 16, regs=["sw_dark", "frame", "wall" if i % 2 else "wallB", "frame", "sw_dark"])
    # the ring frame (a flat octagonal ring round the spine, edge-on it reads as a halo)
    with p.at(Matrix.Translation((-2800.0, 0.0, 0.0)) @ Matrix.Rotation(math.radians(90.0), 4, "Y")):
        p.lathe([(2200.0, -150.0), (2200.0, 150.0), (1950.0, 150.0), (1950.0, -150.0)], 16, regs=["wall", "frame", "sw_dark", "frame"])
    for a in range(0, 360, 90):
        with p.at(Matrix.Translation((-2800.0, 0.0, 0.0)) @ Matrix.Rotation(math.radians(a), 4, "X")):
            p.box((-80.0, -60.0, 300.0), (80.0, 60.0, 1960.0), reg="frame")
    # command block at the bow with window strips
    p.box((L / 2 - 2200, -900, -700), (L / 2 - 200, 900, 800), regs={"Z": "roof", "z": "sw_dark", "x": "wall", "X": "wallB", "y": "wall", "Y": "wall"})
    loft(p, L / 2 - 200, [(-900, -700), (900, -700), (900, 800), (-900, 800)], L / 2, [(-500, -400), (500, -400), (500, 500), (-500, 500)],
         reg="wallB")
    for x0, x1, z in ((L / 2 - 2000, L / 2 - 1400, 450.0), (L / 2 - 1100, L / 2 - 700, 450.0), (L / 2 - 1700, L / 2 - 900, 150.0)):
        for s in (-1.0, 1.0):
            yy = s * 902.0
            p.poly([(x0, yy, z), (x1, yy, z), (x1, yy, z + 60), (x0, yy, z + 60)], mat="Light", n=(0, s, 0))
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
    """Research hull B (about 100 m): a boxy survey tender: a long box hull with stacked equipment blocks, a broken
    sensor dish on a mast, a lit bridge row. Older and blunter than A, so the line of wrecks varies."""
    p = Piece("SM_SP_Research_B", None)
    p.box((-5000, -800, -600), (3800, 800, 600), regs={"Z": "roof", "z": "sw_dark", "x": "frame", "X": "frame", "y": "wall", "Y": "wallB"})
    loft(p, 3800, [(-800, -600), (800, -600), (800, 600), (-800, 600)], 5000, [(-450, -350), (450, -350), (450, 250), (-450, 250)], reg="wall")
    rnd = random.Random(3)
    x = -4600.0
    while x < 2800.0:
        w = 500.0 + rnd.random() * 900.0
        hgt = 250.0 + rnd.random() * 450.0
        y0 = -650.0 + rnd.random() * 300.0
        p.box((x, y0, 600.0), (x + w, y0 + 900.0 + rnd.random() * 300.0, 600.0 + hgt), regs={"Z": "vent", "x": "frame", "X": "frame", "y": "wall", "Y": "wall"})
        x += w + 150.0
    # bridge with a lit row
    p.box((3000.0, -600.0, 600.0), (3900.0, 600.0, 1100.0), regs={"Z": "roof", "X": "wallB", "x": "wall", "y": "wall", "Y": "wall"})
    p.poly([(3901.0, -500.0, 850.0), (3901.0, 500.0, 850.0), (3901.0, 500.0, 950.0), (3901.0, -500.0, 950.0)], mat="Light", n=(1, 0, 0))
    # mast and a broken dish (a tilted shallow cone, a third of it missing)
    p.box((-1500.0, -60.0, 600.0), (-1380.0, 60.0, 2200.0), reg="frame")
    with p.at(Matrix.Translation((-1440.0, 0.0, 2300.0)) @ Matrix.Rotation(math.radians(-35.0), 4, "Y")):
        p.lathe([(0.0, 0.0), (300.0, 60.0), (900.0, 260.0)], 16, regs=["sw_gun", "wallB"], a0=0.0, a1=245.0)
        p.lathe([(900.0, 262.0), (300.0, 62.0), (0.0, 2.0)], 16, regs=["sw_dark", "sw_dark"], a0=0.0, a1=245.0)
    # engine block
    p.box((-5800.0, -700.0, -500.0), (-5000.0, 700.0, 500.0), reg="frame")
    for y in (-380.0, 380.0):
        _lathe_x(p, -5800.0, y, 0.0, [(220.0, 0.0), (420.0, -700.0), (380.0, -700.0), (200.0, 0.0)], 12, regs=["frame", "sw_black", "sw_black"])
    p.chull([(x, y, z) for x in (-6500.0, 5000.0) for y in (-820.0, 820.0) for z in (-620.0, 1350.0)])
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
        p.poly([(p0[0], p0[1], H + 380.0), (p1[0], p1[1], H + 380.0), (p1[0], p1[1], H + 440.0), (p0[0], p0[1], H + 440.0)],
               mat="Glow", n=(math.cos(am), math.sin(am), 0))
        ex, ey = math.cos(am) * 815.0, math.sin(am) * 815.0
        for d in (-120.0, 120.0):
            cx, cy = ex - math.sin(am) * d, ey + math.cos(am) * d
            tx, ty = -math.sin(am) * 45.0, math.cos(am) * 45.0
            p.poly([(cx - tx, cy - ty, H + 640.0), (cx + tx, cy + ty, H + 640.0), (cx + tx, cy + ty, H + 700.0), (cx - tx, cy - ty, H + 700.0)],
                   mat="Eye", n=(math.cos(am), math.sin(am), 0))
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


def hollow_rock():
    """A mined-out rock (about 80 m): a lumpy asteroid with one face cut flat by the rigs and a bore driven into it -
    the yield taken, the pattern visible. Slots: Rock (the asteroid surface), Cut (the machined face), Bore (the hole)."""
    p = Piece("SM_SP_HollowRock", None)
    rnd = random.Random(11)
    NU, NV = 48, 26
    R = (4200.0, 3500.0, 3000.0)
    XC = 2500.0                    # the cut plane (x = XC)
    BR, BD = 950.0, 2600.0         # bore radius and depth
    ph = [rnd.random() * 6.28 for _ in range(6)]

    def rad(u, v):
        return 1.0 + 0.10 * math.sin(3 * u + ph[0]) * math.sin(2 * v + ph[1]) + 0.06 * math.sin(5 * u + ph[2]) * math.cos(4 * v + ph[3]) \
            + 0.04 * math.sin(9 * u + ph[4] + 3 * v)

    pts = {}
    kind = {}
    for i in range(NU + 1):
        u = 2 * math.pi * i / NU
        for j in range(NV + 1):
            v = math.pi * j / NV
            k = rad(u, v) * (1.0 + (rnd.random() - 0.5) * 0.03 if 0 < j < NV else 1.0)
            x = R[0] * k * math.sin(v) * math.cos(u)
            y = R[1] * k * math.sin(v) * math.sin(u)
            z = R[2] * k * math.cos(v)
            t = "rock"
            if x > XC:
                x, t = XC, "cut"
                if math.hypot(y - 200.0, z + 300.0) < BR:
                    x, t = XC - BD, "bore"
            pts[i % NU, j], kind[i % NU, j] = (x, y, z), t
    for i in range(NU):
        for j in range(NV):
            q = [pts[i, j], pts[(i + 1) % NU, j], pts[(i + 1) % NU, j + 1], pts[i, j + 1]]
            ks = {kind[i, j], kind[(i + 1) % NU, j], kind[(i + 1) % NU, j + 1], kind[i, j + 1]}
            mat = "Bore" if "bore" in ks else ("Cut" if ks == {"cut"} else "Rock")
            uvq = [(4.0 * i / NU, 2.0 * j / NV), (4.0 * (i + 1) / NU, 2.0 * j / NV), (4.0 * (i + 1) / NU, 2.0 * (j + 1) / NV), (4.0 * i / NU, 2.0 * (j + 1) / NV)]
            if mat != "Rock":
                uvq = [(q_[1] / 2000.0, q_[2] / 2000.0) for q_ in q]
            if j == 0:
                q, uvq = [q[0], q[2], q[3]], [uvq[0], uvq[2], uvq[3]]
            elif j == NV - 1:
                q, uvq = q[:3], uvq[:3]
            cx = sum(a[0] for a in q) / len(q)
            cy = sum(a[1] for a in q) / len(q)
            cz = sum(a[2] for a in q) / len(q)
            nrm = (cx, cy, cz) if mat == "Rock" else ((1.0, 0.0, 0.0) if mat == "Cut" else (1.0, -(cy - 200.0), -(cz + 300.0)))
            if mat == "Bore" and all(abs(a[0] - (XC - BD)) < 1 for a in q):
                nrm = (1.0, 0.0, 0.0)
            p.poly(q, uv=uvq, mat=mat, n=nrm)
    p.fix_normals = False
    p.smooth_deg = 50.0
    allp = list(pts.values())
    p.chull([a for a in allp if a[0] < XC - 1] + [(XC, a[1], a[2]) for a in allp if abs(a[0] - XC) < 1])
    return p


BUILDERS = [patrol_bow, patrol_mid, patrol_aft, patrol_wing, research_a, research_b, null_rig, hollow_rock]

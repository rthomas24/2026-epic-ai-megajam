# SPDX-License-Identifier: MIT
# Kit, planet camp buildings (#194, Ryan: the planets are far too empty; huts and the like filling the place).
# Route C, the modular hard-surface kit (R-043: nothing walkable is an AI mesh): the planet trim sheet, 1.5 cm bevels,
# exteriors only (doors shut, windows lit from inside: the Light slot), simple box collision matching the shell.
# Unreal centimetres, +X = the door side, pivot on the ground at the footprint centre. One palette per planet comes from
# the instances the import assigns by name (Verdance teal / white with warm windows; Cinder black / amber hazard).
# Slots: Trim (the planet's trim), Light (warm window glow), Glow (the planet's strip glow), Beacon (mast lamps).
import math

from mathutils import Matrix

from kit_lib import Piece


def _tube(p, a, b, r=5.0, reg="frame", mat="Trim"):
    ax, ay, az = a
    bx, by, bz = b
    dx, dy, dz = bx - ax, by - ay, bz - az
    L = math.sqrt(dx * dx + dy * dy + dz * dz)
    yaw = math.degrees(math.atan2(dy, dx))
    pitch = -math.degrees(math.atan2(dz, math.hypot(dx, dy)))
    with p.at(Matrix.Translation(a) @ Matrix.Rotation(math.radians(yaw), 4, "Z") @ Matrix.Rotation(math.radians(pitch), 4, "Y")):
        p.box((0.0, -r, -r), (L, r, r), reg=reg, mat=mat)


def _shell(p, x0, x1, y0, y1, h, plinth=30.0):
    """A prefab shell: a dark plinth, wall panels with a frame band, a frame at the top edge."""
    p.box((x0 - 10, y0 - 10, 0.0), (x1 + 10, y1 + 10, plinth), reg="sw_dark")
    p.box((x0, y0, plinth), (x1, y1, h), regs={"x": "wall", "X": "wall", "y": "wall", "Y": "wall", "Z": "roof", "z": "sw_dark"})
    p.box((x0 - 4, y0 - 4, h - 20.0), (x1 + 4, y1 + 4, h), reg="frame")


def _window(p, side, c, z0, z1, w, depth_axis="y"):
    """A lit window on a wall: a frame proud of the wall and a warm glass quad just inside it. side = +-1 on that axis
    at coordinate c (the wall face), centred on the other axis at its value."""
    (u, wall) = c
    if depth_axis == "y":
        y = wall + side * 6.0
        y0, y1 = sorted((wall, y))
        p.box((u - w / 2 - 8, y0, z0 - 8), (u + w / 2 + 8, y1, z0), reg="frame")
        p.box((u - w / 2 - 8, y0, z1), (u + w / 2 + 8, y1, z1 + 8), reg="frame")
        for e in (u - w / 2 - 8, u + w / 2):
            p.box((e, y0, z0), (e + 8, y1, z1), reg="frame")
        g0, g1 = sorted((wall + side * 1.0, wall + side * 2.0))
        p.box((u - w / 2, g0, z0), (u + w / 2, g1, z1), mat="Light")
    else:
        x = wall + side * 6.0
        x0, x1 = sorted((wall, x))
        p.box((x0, u - w / 2 - 8, z0 - 8), (x1, u + w / 2 + 8, z0), reg="frame")
        p.box((x0, u - w / 2 - 8, z1), (x1, u + w / 2 + 8, z1 + 8), reg="frame")
        for e in (u - w / 2 - 8, u + w / 2):
            p.box((x0, e, z0), (x1, e + 8, z1), reg="frame")
        g0, g1 = sorted((wall + side * 1.0, wall + side * 2.0))
        p.box((g0, u - w / 2, z0), (g1, u + w / 2, z1), mat="Light")


def _door(p, xwall, z0, w=100.0, h=210.0, shutter=False):
    """A shut door on the +X wall at x = xwall: a hazard-edged frame, the leaf (ribbed when a blast shutter), a lamp."""
    p.box((xwall, -w / 2 - 12, z0), (xwall + 8, w / 2 + 12, z0 + h + 12), reg="hazard")
    if shutter:
        for k in range(int(h // 20)):
            p.box((xwall + 2, -w / 2, z0 + k * 20.0), (xwall + 12, w / 2, z0 + k * 20.0 + 16.0), reg="sw_gun")
    else:
        p.box((xwall + 2, -w / 2, z0), (xwall + 10, w / 2, z0 + h), regs={"X": "hatch"}, reg="frame")
    p.box((xwall + 8, -16, z0 + h + 20), (xwall + 16, 16, z0 + h + 28), mat="Glow")


def _steps(p, x0, n=2, w=140.0):
    for k in range(n):
        p.box((x0 + k * 30.0, -w / 2, 0.0), (x0 + (k + 1) * 30.0 + 20, w / 2, 30.0 - k * 15.0 if k else 30.0), reg="grate")


# ------------------------------------------------------------------------------------------------ Verdance research camp
def v_prefab():
    """4 x 4 m prefab hut: plinth, panel walls, a low gable roof, door on +X with a lamp and steps, two windows a side."""
    p = Piece("SM_Hut_Verdance_Prefab", None)
    _shell(p, -200, 200, -200, 200, 280)
    p.prism([(-215.0, 276.0), (215.0, 276.0), (215.0, 290.0), (0.0, 350.0), (-215.0, 290.0)], "yz", -215.0, 215.0,
            regs=["sw_dark", "frame", "roof", "roof", "frame"], cap_reg="frame", fitu=True)
    _door(p, 200.0, 30.0)
    _steps(p, 210.0)
    for s in (-1.0, 1.0):
        for u in (-90.0, 90.0):
            _window(p, s, (u, s * 200.0), 130.0, 210.0, 80.0, "y")
    p.box((-200.0, -60.0, 200.0), (-190.0, 60.0, 250.0), reg="sw_gun")          # vent at the back
    p.cbox((-210, -210, 0), (210, 210, 290))
    p.cbox((210, -70, 0), (290, 70, 30))
    return p


def v_lab():
    """4 x 6 m lab hut: flat roof with a parapet, a window strip, a porch deck with posts and an awning at the door,
    a roof antenna with a dish."""
    p = Piece("SM_Hut_Verdance_Lab", None)
    _shell(p, -300, 300, -200, 200, 300)
    p.box((-305, -205, 300), (305, 205, 320), reg="frame")
    for s in (-1.0, 1.0):
        for u in (-180.0, -60.0, 60.0, 180.0):
            _window(p, s, (u, s * 200.0), 140.0, 220.0, 90.0, "y")
    _door(p, 300.0, 30.0)
    p.box((300.0, -200.0, 0.0), (450.0, 200.0, 30.0), regs={"Z": "grate"}, reg="sw_dark")         # porch deck
    for y in (-190.0, 190.0):
        p.box((436.0, y - 7, 30.0), (450.0, y + 7, 270.0), reg="pillar")
    p.box((300.0, -210.0, 270.0), (460.0, 210.0, 282.0), regs={"Z": "roof", "z": "sw_dark"}, reg="frame")   # awning
    p.box((440.0, -60.0, 0.0), (480.0, 60.0, 15.0), reg="grate")
    _tube(p, (-200.0, 120.0, 320.0), (-200.0, 120.0, 720.0), 5.0)               # antenna
    _tube(p, (-200.0, 120.0, 650.0), (-150.0, 120.0, 680.0), 3.0)
    p.box((-160.0, 70.0, 640.0), (-150.0, 170.0, 720.0), reg="sw_gun")           # dish plate
    p.box((-205.0, 115.0, 720.0), (-195.0, 125.0, 728.0), mat="Glow")
    p.cbox((-310, -210, 0), (310, 210, 320))
    p.cbox((300, -210, 0), (480, 210, 30))
    return p


def v_dome():
    """3 m dome tent on a frame: a fabric dome (Canopy-like wall region on the trim), four arched frame ribs, a ground
    ring, a shut door flap with a lamp, one lit window."""
    p = Piece("SM_Hut_Verdance_Dome", None)
    prof = [(150.0 * math.cos(math.radians(a)), 230.0 * math.sin(math.radians(a))) for a in range(0, 91, 15)]
    p.lathe(prof, 16, regs="wall", reps=4)
    for a in (0.0, 45.0, 90.0, 135.0):
        with p.at(Matrix.Rotation(math.radians(a), 4, "Z")):
            pts = [(155.0 * math.cos(math.radians(t)), 0.0, 235.0 * math.sin(math.radians(t))) for t in range(0, 181, 20)]
            for i in range(len(pts) - 1):
                _tube(p, pts[i], pts[i + 1], 4.0, reg="sw_dark")
    p.lathe([(150.0, 0.0), (160.0, 0.0), (160.0, 12.0), (150.0, 12.0)], 16, regs="sw_dark", closed=True)
    p.box((140.0, -45.0, 0.0), (152.0, 45.0, 170.0), regs={"X": "hatch"}, reg="frame")            # door flap
    p.box((146.0, -12.0, 182.0), (154.0, 12.0, 190.0), mat="Glow")
    p.box((-30.0, 121.0, 118.0), (30.0, 129.0, 140.0), mat="Light")             # flush with the dome skin (r 123-128 there)
    p.cbox((-150, -150, 0), (150, 150, 200))
    return p


def v_water_tank():
    """A water tank on legs: a 2.4 m cylinder 2 m tall on four 2.5 m legs with braces, a cap, a ladder, a level strip."""
    p = Piece("SM_Hut_Verdance_WaterTank", None)
    p.lathe([(0.0, 250.0), (120.0, 250.0), (120.0, 450.0), (100.0, 470.0), (0.0, 470.0)], 16, regs=["sw_dark", "wall", "frame", "roof"], reps=4)
    for (x, y) in ((85.0, 85.0), (-85.0, 85.0), (-85.0, -85.0), (85.0, -85.0)):
        _tube(p, (x * 1.2, y * 1.2, 0.0), (x, y, 250.0), 7.0, reg="pillar")
    for a, b in (((102.0, 102.0, 40.0), (-102.0, 102.0, 200.0)), ((-102.0, -102.0, 40.0), (102.0, -102.0, 200.0)),
                 ((102.0, -102.0, 40.0), (102.0, 102.0, 200.0)), ((-102.0, 102.0, 40.0), (-102.0, -102.0, 200.0))):
        _tube(p, a, b, 3.0, reg="frame")
    for y in (-22.0, 22.0):
        _tube(p, (126.0, y, 20.0), (126.0, y, 460.0), 3.0, reg="rail")
    for z in range(40, 460, 30):
        _tube(p, (126.0, -22.0, float(z)), (126.0, 22.0, float(z)), 2.0, reg="rail")
    p.box((119.0, -60.0, 300.0), (123.0, -50.0, 420.0), mat="Glow")
    p.cbox((-130, -130, 0), (130, 130, 470))
    return p


def v_mast():
    """A solar / relay mast: a 6 m pole on a footing, two tilted solar panels, a relay box, a beacon at the top."""
    p = Piece("SM_Hut_Verdance_Mast", None)
    p.box((-50.0, -50.0, 0.0), (50.0, 50.0, 25.0), reg="sw_dark")
    p.box((-9.0, -9.0, 25.0), (9.0, 9.0, 600.0), reg="pillar")
    for z, s in ((380.0, 1.0), (470.0, -1.0)):
        with p.at(Matrix.Translation((0.0, s * 70.0, z)) @ Matrix.Rotation(math.radians(s * 30.0), 4, "X")):
            p.box((-80.0, -60.0, -3.0), (80.0, 60.0, 3.0), regs={"Z": "console", "z": "sw_dark"}, reg="frame")
        _tube(p, (0.0, 0.0, z), (0.0, s * 60.0, z), 3.0)
    p.box((10.0, -25.0, 120.0), (45.0, 25.0, 190.0), regs={"X": "console"}, reg="frame")
    p.box((45.0, -8.0, 170.0), (47.0, 8.0, 180.0), mat="Glow")
    p.box((-14.0, -14.0, 600.0), (14.0, 14.0, 630.0), mat="Beacon")
    p.cbox((-50, -50, 0), (50, 50, 630))
    return p


def v_fence():
    """A short fence run, 4 m: posts every 1 m, top and mid rails, a mesh panel between."""
    p = Piece("SM_Hut_Verdance_Fence", None)
    for x in (-200.0, -100.0, 0.0, 100.0, 200.0):
        p.box((x - 5, -5, 0.0), (x + 5, 5, 140.0), reg="pillar")
    for z in (60.0, 132.0):
        p.box((-200.0, -3, z), (200.0, 3, z + 6), reg="rail")
    p.box((-200.0, -1.0, 10.0), (200.0, 1.0, 130.0), reg="grate")
    p.cbox((-205, -6, 0), (205, 6, 140))
    return p


def v_walkway():
    """A walkway slab, 4 x 1.5 m, 15 cm high: a grate deck in a dark frame, edge glow strips."""
    p = Piece("SM_Hut_Verdance_Walkway", None)
    p.box((-200.0, -75.0, 0.0), (200.0, 75.0, 15.0), regs={"Z": "grate"}, reg="sw_dark")
    for y0, y1 in ((-75.0, -70.0), (70.0, 75.0)):
        p.box((-200.0, y0, 15.0), (200.0, y1, 17.0), mat="Glow")
    p.cbox((-200, -75, 0), (200, 75, 15))
    return p


# ------------------------------------------------------------------------------------------------ Cinder mining camp
def c_shack():
    """A low armoured shack, 3.5 x 3.5 m, 2.4 m: thick walls, sloped armour skirts, a ribbed blast shutter on the door,
    slit windows, a roof hatch."""
    p = Piece("SM_Hut_Cinder_Shack", None)
    _shell(p, -175, 175, -175, 175, 240, plinth=20.0)
    for s in (-1.0, 1.0):
        p.prism([(s * 175.0, 20.0), (s * 205.0, 20.0), (s * 175.0, 110.0)], "yz", -175.0, 175.0, regs="hazard", cap_reg="sw_dark", fitu=True)
        p.prism([(s * 175.0, 20.0), (s * 205.0, 20.0), (s * 175.0, 110.0)], "xz", -175.0, 175.0, regs="sw_dark", cap_reg="sw_dark", fitu=True)
        _window(p, s, (0.0, s * 175.0), 170.0, 195.0, 120.0, "y")
    _door(p, 175.0, 20.0, w=110.0, h=190.0, shutter=True)
    p.box((-70.0, -70.0, 240.0), (70.0, 70.0, 262.0), regs={"Z": "hatch"}, reg="frame")
    p.cbox((-205, -205, 0), (205, 205, 262))
    return p


def c_workshop():
    """A 4 x 6 m workshop: a roll door on +X in a hazard frame, roof vents, side windows, a lamp over the door."""
    p = Piece("SM_Hut_Cinder_Workshop", None)
    _shell(p, -300, 300, -200, 200, 350, plinth=20.0)
    p.box((300.0, -150.0, 20.0), (312.0, 150.0, 300.0), reg="hazard")
    for k in range(14):
        p.box((304.0, -135.0, 22.0 + k * 19.0), (316.0, 135.0, 22.0 + k * 19.0 + 16.0), reg="sw_gun")
    p.box((312.0, -30.0, 310.0), (322.0, 30.0, 320.0), mat="Glow")
    for s in (-1.0, 1.0):
        for u in (-180.0, 0.0, 180.0):
            _window(p, s, (u, s * 200.0), 200.0, 260.0, 100.0, "y")
    for x in (-180.0, 0.0, 180.0):
        p.box((x - 40, -40, 350.0), (x + 40, 40, 390.0), regs={"Z": "grate"}, reg="frame")
    p.cbox((-305, -205, 0), (320, 205, 390))
    return p


def c_pump_house():
    """A pump house, 3 x 3 m: a box with a pipe gallery, two pipes out of each side with valve wheels, a tank."""
    p = Piece("SM_Hut_Cinder_PumpHouse", None)
    _shell(p, -150, 150, -150, 150, 260, plinth=20.0)
    _door(p, 150.0, 20.0, w=90.0, h=190.0)
    for s in (-1.0, 1.0):
        for z in (80.0, 150.0):
            _tube(p, (0.0, s * 150.0, z), (0.0, s * 320.0, z), 14.0, reg="pillar")
            _tube(p, (0.0, s * 320.0, z), (0.0, s * 320.0, 0.0), 14.0, reg="pillar")
        p.box((-30.0, s * 200.0 - 4, 120.0), (30.0, s * 200.0 + 4, 180.0), reg="hazard")       # valve wheel plate
    p.lathe([(0.0, 260.0), (70.0, 260.0), (70.0, 380.0), (40.0, 400.0), (0.0, 400.0)], 12, regs=["sw_dark", "wallB", "frame", "roof"], center=(-60.0, 0.0), reps=4)
    p.box((-152.0, -40.0, 120.0), (-148.0, 40.0, 140.0), mat="Glow")
    p.cbox((-150, -150, 0), (150, 150, 260))
    for s in (-1.0, 1.0):
        y0, y1 = sorted((s * 150.0, s * 335.0))
        p.cbox((-15, y0, 0), (15, y1, 165))
    return p


def c_ore_hopper():
    """An ore hopper: an inverted pyramid bin on four legs with a chute and a hazard band."""
    p = Piece("SM_Hut_Cinder_OreHopper", None)
    top = [(-200.0, -200.0), (200.0, -200.0), (200.0, 200.0), (-200.0, 200.0)]
    bot = [(-60.0, -60.0), (60.0, -60.0), (60.0, 60.0), (-60.0, 60.0)]
    for k in range(4):
        a, b = top[k], top[(k + 1) % 4]
        c, d = bot[(k + 1) % 4], bot[k]
        nrm = ((a[0] + b[0]) / 2.0, (a[1] + b[1]) / 2.0, -120.0)
        q = [(a[0], a[1], 480.0), (b[0], b[1], 480.0), (c[0], c[1], 240.0), (d[0], d[1], 240.0)]
        p.poly(q, "wall", uv=[(0, 0), (1, 0), (1, 1), (0, 1)], n=nrm)
        p.poly(q, "sw_dark", uv=[(0, 0), (1, 0), (1, 1), (0, 1)], n=(-nrm[0], -nrm[1], -nrm[2]))
    p.box((-204.0, -204.0, 470.0), (204.0, 204.0, 500.0), reg="hazard")
    for (x, y) in ((180.0, 180.0), (-180.0, 180.0), (-180.0, -180.0), (180.0, -180.0)):
        p.box((x - 12, y - 12, 0.0), (x + 12, y + 12, 480.0), reg="pillar")
    p.box((-60.0, -60.0, 200.0), (60.0, 60.0, 240.0), reg="frame")
    p.prism([(60.0, 240.0), (60.0, 200.0), (200.0, 120.0), (200.0, 160.0)], "xz", -50.0, 50.0, regs="sw_gun", cap_reg="sw_dark", fitu=True)
    p.box((196.0, -8.0, 470.0), (206.0, 8.0, 480.0), mat="Glow")
    p.cbox((-205, -205, 0), (205, 205, 500))
    return p


def c_drill_rig():
    """A drill rig frame, about 8 m: a tapered four-leg lattice tower on a base deck, the drill string and head in the
    middle, a crown block, a beacon. Non-walkable: collision only on the base and the legs."""
    p = Piece("SM_Hut_Cinder_DrillRig", None)
    p.box((-250.0, -250.0, 0.0), (250.0, 250.0, 40.0), regs={"Z": "grate"}, reg="sw_dark")
    H = 800.0
    feet = [(200.0, 200.0), (-200.0, 200.0), (-200.0, -200.0), (200.0, -200.0)]
    tops = [(60.0, 60.0), (-60.0, 60.0), (-60.0, -60.0), (60.0, -60.0)]
    for (a, b) in zip(feet, tops):
        _tube(p, (a[0], a[1], 40.0), (b[0], b[1], H), 9.0, reg="pillar")
    for lvl in range(4):
        t0, t1 = lvl / 4.0, (lvl + 1) / 4.0
        z0, z1 = 40.0 + (H - 40.0) * t0, 40.0 + (H - 40.0) * t1
        for k in range(4):
            a, b = feet[k], feet[(k + 1) % 4]
            ta, tb = tops[k], tops[(k + 1) % 4]
            pa = (a[0] + (ta[0] - a[0]) * t0, a[1] + (ta[1] - a[1]) * t0, z0)
            pb = (b[0] + (tb[0] - b[0]) * t1, b[1] + (tb[1] - b[1]) * t1, z1)
            _tube(p, pa, pb, 4.0, reg="hazard" if lvl == 0 else "frame")
            qa = (a[0] + (ta[0] - a[0]) * t1, a[1] + (ta[1] - a[1]) * t1, z1)
            qb = (b[0] + (tb[0] - b[0]) * t1, b[1] + (tb[1] - b[1]) * t1, z1)
            _tube(p, qa, qb, 4.0, reg="frame")
    p.box((-80.0, -80.0, H), (80.0, 80.0, H + 40.0), reg="sw_gun")                  # crown block
    _tube(p, (0.0, 0.0, 40.0), (0.0, 0.0, H), 8.0, reg="sw_dark")                  # drill string
    p.lathe([(0.0, 40.0), (40.0, 60.0), (40.0, 140.0), (0.0, 160.0)], 8, regs="hazard", center=(0.0, 0.0))
    p.box((-12.0, -12.0, H + 40.0), (12.0, 12.0, H + 64.0), mat="Beacon")
    p.box((180.0, -60.0, 40.0), (240.0, 60.0, 160.0), regs={"X": "console"}, reg="frame")   # control box
    p.box((240.0, -20.0, 120.0), (242.0, 20.0, 140.0), mat="Glow")
    p.cbox((-250, -250, 0), (250, 250, 40))
    for (a, b) in zip(feet, tops):
        p.chull([(a[0] - 10, a[1] - 10, 40.0), (a[0] + 10, a[1] + 10, 40.0), (b[0] - 10, b[1] - 10, H), (b[0] + 10, b[1] + 10, H)])
    return p


def c_pipe_run():
    """A pipe run, 6 m: two pipes on three saddle supports, flanges, a hazard band."""
    p = Piece("SM_Hut_Cinder_PipeRun", None)
    for y, r in ((-35.0, 16.0), (35.0, 12.0)):
        _tube(p, (-300.0, y, 80.0), (300.0, y, 80.0), r, reg="pillar")
        for x in (-150.0, 150.0):
            p.box((x - 6, y - r - 4, 80.0 - r - 4), (x + 6, y + r + 4, 80.0 + r + 4), reg="frame")
    for x in (-250.0, 0.0, 250.0):
        p.box((x - 10, -60.0, 0.0), (x + 10, 60.0, 64.0), reg="sw_dark")
    p.box((-20.0, -60.0, 64.0), (20.0, 60.0, 68.0), reg="hazard")
    p.cbox((-300, -60, 0), (300, 60, 100))
    return p


def c_flood_mast():
    """A flood-light mast, 7 m: a pole on a weighted base, a twin-lamp head (Light) angled down, a cable run."""
    p = Piece("SM_Hut_Cinder_FloodMast", None)
    p.box((-60.0, -60.0, 0.0), (60.0, 60.0, 30.0), reg="hazard")
    p.box((-10.0, -10.0, 30.0), (10.0, 10.0, 700.0), reg="pillar")
    p.box((-10.0, -90.0, 680.0), (10.0, 90.0, 700.0), reg="frame")
    for y in (-70.0, 70.0):
        with p.at(Matrix.Translation((10.0, y, 670.0)) @ Matrix.Rotation(math.radians(30.0), 4, "Y")):
            p.box((0.0, -30.0, -22.0), (40.0, 30.0, 22.0), reg="sw_dark")
            p.box((40.0, -26.0, -18.0), (42.0, 26.0, 18.0), mat="Light")
    _tube(p, (12.0, 0.0, 40.0), (12.0, 0.0, 660.0), 2.0, reg="sw_dark")
    p.cbox((-60, -60, 0), (60, 60, 700))
    return p


VERDANCE = [v_prefab, v_lab, v_dome, v_water_tank, v_mast, v_fence, v_walkway]
CINDER = [c_shack, c_workshop, c_pump_house, c_ore_hopper, c_drill_rig, c_pipe_run, c_flood_mast]
BUILDERS = VERDANCE + CINDER

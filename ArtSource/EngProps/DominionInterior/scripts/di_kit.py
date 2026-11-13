# SPDX-License-Identifier: MIT
# DominionInterior - the kit's pieces. Every builder returns a di_lib.Piece authored in cm: +X forward, +Z up,
# pivot at (0, 0, 0) = bottom centre of the footprint on the walkable plane. EXPECT is the size contract.
import math
import os
import random
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from di_lib import Piece, rotz  # noqa: E402

# name: ((min), (max)) in cm, relative to the pivot
EXPECT = {
    "SM_DI_Corridor_Straight": ((-200, -200, -20), (200, 200, 620)),
    "SM_DI_Corridor_Corner":   ((-200, -200, -20), (200, 200, 620)),
    "SM_DI_Corridor_T":        ((-200, -200, -20), (200, 200, 620)),
    "SM_DI_Doorway":           ((-30, -200, 0), (30, 200, 600)),
    "SM_DI_Door":              ((-15, -150, 0), (15, 150, 400)),
    "SM_DI_Room_Wall":         ((-30, -200, 0), (30, 200, 600)),
    "SM_DI_Room_Floor":        ((-200, -200, -20), (200, 200, 0)),
    "SM_DI_Room_Ceiling":      ((-200, -200, 600), (200, 200, 620)),
    "SM_DI_Cover_Low":         ((-30, -100, 0), (30, 100, 110)),
    "SM_DI_Hangar_Pad":        ((-600, -600, -40), (600, 600, 0)),
    "SM_DI_Core":              ((-400, -400, 0), (400, 400, 1200)),
    "SM_DI_Core_Shield":       ((-640, -640, 0), (640, 640, 1380)),
    "SM_DI_Conduit":           ((-60, -60, 0), (60, 60, 300)),
    "SM_DI_Walkway_Ring":      ((-200, -150, -20), (200, 150, 110)),
    "SM_DI_Console":           ((-30, -60, 0), (30, 60, 130)),
    "SM_DI_Shaft":             ((-200, -200, 0), (200, 200, 1200)),
    "SM_DI_Bulkhead_Rib":      ((-7, -12, 0), (7, 12, 600)),
    # debris: about 150 cm, checked by largest dimension
}
NO_COLLISION = ("SM_DI_Core_Ring_A", "SM_DI_Core_Ring_B")
DETAIL = True      # False = blockout (primary masses only)


# ------------------------------------------------------------------------------------------------ shared parts

def wall_face(p, yf=170.0, cove=True, lip=10.0, back=200.0, x0=-200.0, x1=200.0, caps=True):
    """A wall whose finished face looks toward -Y at y = yf (body toward +Y), running along X.
    Bands: kick 0-40 with the ankle strip, armour 40-190, head strip 190-200, armour 200-350, cable tray
    350-400, structure 400-500, then a 45-degree cove to the ceiling (corridors) or a top band (rooms)."""
    U = ("x", -200.0, 200.0)
    n = (0, -1, 0)

    def band(z0, z1, reg, vr=(0.0, 1.0), y=yf, mat="Atlas"):
        p.poly([(x0, y, z0), (x1, y, z0), (x1, y, z1), (x0, y, z1)], reg, U, ("z", z0, z1, vr[0], vr[1]), n=n, mat=mat)

    if not DETAIL:
        band(0, 500, "sw_gun")
    else:
        p.box((x0, yf - lip, 0), (x1, yf, 40), "kick", faces="yZxX", regs={"Z": "sw_gun", "x": "sw_gun", "X": "sw_gun"})
        band(17, 22, None, y=yf - lip - 0.5, mat="GlowRed")
        band(40, 190, "armourA")
        band(190, 200, "sw_gun")
        band(193, 197, None, y=yf - 0.5, mat="GlowRed")
        band(200, 350, "armourB")
        p.box((x0, yf - lip, 350), (x1, yf, 400), "cable", faces="yzZxX",
              regs={"z": "sw_gun", "Z": "sw_gun", "x": "sw_gun", "X": "sw_gun"})
        band(400, 500, "gunmetal")
    ua, ub = (x0 + 200) / 400.0, (x1 + 200) / 400.0
    if cove:
        p.poly([(x0, yf, 500), (x1, yf, 500), (x1, yf - 50, 600), (x0, yf - 50, 600)], "ceiling",
               uv=[(ua, 0), (ub, 0), (ub, 1), (ua, 1)], n=(0, -1, -0.5))
        sec = [(yf, 0), (back, 0), (back, 600), (yf - 50, 600), (yf, 500)]
    else:
        band(500, 600, "armourA", vr=(0.36, 1.0))
        sec = [(yf, 0), (back, 0), (back, 600), (yf, 600)]
    if caps:
        for x, s in ((x0, -1), (x1, 1)):
            p.poly([(x, a, b) for a, b in sec], "sw_gun", uv=[(0.5, 0.5)] * len(sec), n=(s, 0, 0))


def rib_half(p, beam=True):
    """Half a portal rib on the +Y wall at x = 0: pilaster, cove leg and (optionally) half the ceiling beam."""
    if not DETAIL:
        p.box((-12, 156, 0), (12, 170, 500), "sw_gun", faces="yxX")
        return
    p.box((-12, 156, 0), (12, 170, 60), "hazard", faces="yxX",
          uvo={"y": (("z", 0, 60, 0.0, 0.3125), ("x", -12, 12)), "x": (("z", 0, 60, 0.0, 0.3125), ("y", 156, 170)),
               "X": (("z", 0, 60, 0.0, 0.3125), ("y", 156, 170))})
    p.box((-12, 156, 60), (12, 170, 500), "kick", faces="yxX",
          uvo={"y": (("z", 60, 500), ("x", -12, 12)), "x": (("z", 60, 500), ("y", 156, 170, 0.0, 0.25)),
               "X": (("z", 60, 500), ("y", 156, 170, 0.0, 0.25))})
    p.poly([(-12, 156, 500), (12, 156, 500), (12, 106, 600), (-12, 106, 600)], "kick",
           uv=[(0, 0), (0, 1), (0.28, 1), (0.28, 0)], n=(0, -1, -0.5))
    for x, s in ((-12, -1), (12, 1)):
        p.poly([(x, 156, 500), (x, 170, 500), (x, 120, 600), (x, 106, 600)], "sw_gun", uv=[(0.5, 0.5)] * 4, n=(s, 0, 0))
    if beam:
        p.box((-12, 0, 586), (12, 113, 600), "kick", faces="zxX",
              uvo={"z": (("y", 0, 113, 0.0, 0.2825), ("x", -12, 12)), "x": (("y", 0, 113, 0.0, 0.2825), ("z", 586, 600, 0.0, 0.25)),
                   "X": (("y", 0, 113, 0.0, 0.2825), ("z", 586, 600, 0.0, 0.25))})


def light_bar(p, z=600.0, half=62.0):
    """A cold white work light under the ceiling, along X."""
    p.box((-half - 8, -12, z - 6), (half + 8, 12, z), "sw_gun", faces="zxXyY")
    p.poly([(-half, -7, z - 6.5), (half, -7, z - 6.5), (half, 7, z - 6.5), (-half, 7, z - 6.5)], mat="GlowWhite", n=(0, 0, -1))


def pillar(p, sy=1):
    """The structural column on a junction's open corner (hides the neighbours' wall ends). At (-200, sy * 200)."""
    y0, y1 = (120, 200) if sy > 0 else (-200, -120)
    fy = "y" if sy > 0 else "Y"
    yy = y0 if sy > 0 else y1
    if not DETAIL:
        p.box((-200, y0, 0), (-120, y1, 600), "sw_gun", faces="X" + fy)
        return
    for z0, z1, reg in ((0, 40, "kick"), (40, 100, "hazard"), (100, 600, "gunmetal")):
        if reg == "gunmetal":
            uvo = {"X": (("z", 100, 600), ("y", y0, y1)), fy: (("z", 100, 600), ("x", -200, -120))}
        elif reg == "hazard":
            uvo = {"X": (("y", y0, y0 + 400), ("z", z0, z1)), fy: (("x", -200, 200), ("z", z0, z1))}
        else:
            uvo = {}
        p.box((-200, y0, z0), (-120, y1, z1), reg, faces="X" + fy, uvo=uvo)
    for z0, z1 in ((17, 22), (193, 197)):
        p.poly([(-119.5, y0, z0), (-119.5, y1, z0), (-119.5, y1, z1), (-119.5, y0, z1)], mat="GlowRed", n=(1, 0, 0))
        e = yy - 0.5 * sy
        p.poly([(-200, e, z0), (-120, e, z0), (-120, e, z1), (-200, e, z1)], mat="GlowRed", n=(0, -sy, 0))
    p.cbox((-200, y0, 0), (-120, y1, 600))


def plates(p, z=0.0, x0=-200.0, y0=-200.0, nx=2, ny=2, special=(0, 1)):
    for i in range(nx):
        for j in range(ny):
            a, b = x0 + 200 * i, y0 + 200 * j
            reg = "plateB" if (i, j) == special else "plateA"
            p.poly([(a, b, z), (a + 200, b, z), (a + 200, b + 200, z), (a, b + 200, z)], reg,
                   ("x", a, a + 200), ("y", b, b + 200), n=(0, 0, 1))


# ------------------------------------------------------------------------------------------------ corridors

def corridor(name, walls, pillars, shell):
    p = Piece(name, EXPECT[name])
    straight = name.endswith("Straight")
    for deg in walls:
        with p.at(rotz(deg)):
            wall_face(p)
            p.cbox((-200, 170, 0), (200, 200, 600))
            if straight:
                rib_half(p, beam=True)
            else:
                rib_half(p, beam=False)
    for sy in pillars:
        pillar(p, sy)
    # floor
    if straight:
        p.poly([(-200, -100, 0), (200, -100, 0), (200, 100, 0), (-200, 100, 0)], "grating", ("x", -200, 200), ("y", -100, 100), n=(0, 0, 1))
        for a, b in ((100, 200), (-200, -100)):
            p.poly([(-200, a, 0), (200, a, 0), (200, b, 0), (-200, b, 0)], "kick", ("x", -200, 200), ("y", a, b), n=(0, 0, 1))
    else:
        plates(p)
    # ceiling
    p.poly([(-200, -200, 600), (200, -200, 600), (200, 200, 600), (-200, 200, 600)], "ceiling", ("x", -200, 200), ("y", -200, 200), n=(0, 0, -1))
    if DETAIL:
        light_bar(p)
    p.box((-200, -200, -20), (200, 200, 620), "sw_black", faces=shell)
    p.cbox((-200, -200, -20), (200, 200, 0))
    p.cbox((-200, -200, 600), (200, 200, 620))
    return p


def b_corridor_straight():
    return corridor("SM_DI_Corridor_Straight", (0, 180), (), "yYzZ")


def b_corridor_corner():      # open at -X and +Y: walls on -Y and +X
    return corridor("SM_DI_Corridor_Corner", (180, -90), (1,), "yXzZ")


def b_corridor_t():           # open at -X, +Y and -Y: wall on +X
    return corridor("SM_DI_Corridor_T", (-90,), (1, -1), "XzZ")


# ------------------------------------------------------------------------------------------------ doorway, door

def b_doorway():
    p = Piece("SM_DI_Doorway", EXPECT["SM_DI_Doorway"])
    F = 22.0      # wall face
    C = 29.5      # collar face
    for deg in (0, 180):
        with p.at(rotz(deg)):
            for s in (-1, 1):
                a, b = (175, 200) if s > 0 else (-200, -175)
                p.poly([(-F, a, 0), (-F, b, 0), (-F, b, 600), (-F, a, 600)], "gunmetal", ("z", 0, 600), ("y", a, b, 0.0, 0.25), n=(-1, 0, 0))
            p.poly([(-F, -175, 430), (-F, 175, 430), (-F, 175, 600), (-F, -175, 600)], "armourA", ("y", -200, 200), ("z", 430, 600, 0.36, 1.0), n=(-1, 0, 0))
            if DETAIL:
                p.poly([(-F - 0.6, -50, 468), (-F - 0.6, 50, 468), (-F - 0.6, 50, 568), (-F - 0.6, -50, 568)], "triad", ("y", 50, -50), ("z", 468, 568), n=(-1, 0, 0))
                for s in (-1, 1):
                    y = 162 * s
                    p.poly([(-30, y - 2, 75), (-30, y + 2, 75), (-30, y + 2, 385), (-30, y - 2, 385)], mat="GlowRed", n=(-1, 0, 0))
                p.poly([(-30, -45, 411), (-30, 45, 411), (-30, 45, 419), (-30, -45, 419)], mat="GlowWhite", n=(-1, 0, 0))
    # collars run through the wall (both faces)
    for s in (-1, 1):
        a, b = (150, 175) if s > 0 else (-175, -150)
        inner, outer = ("y", "Y") if s > 0 else ("Y", "y")
        if DETAIL:
            p.box((-C, a, 0), (C, b, 60), "hazard", faces="xX" + inner + outer, regs={inner: "sw_gun", outer: "sw_gun"},
                  uvo={"x": (("z", 0, 60, 0.0, 0.3125), ("y", a, b)), "X": (("z", 0, 60, 0.0, 0.3125), ("y", a, b))})
            z0 = 60
        else:
            z0 = 0
        p.box((-C, a, z0), (C, b, 430), "kick", faces="xX" + inner + outer, regs={inner: "gunmetal", outer: "sw_gun"},
              uvo={"x": (("z", z0, z0 + 400), ("y", a, b)), "X": (("z", z0, z0 + 400), ("y", a, b)),
                   inner: (("z", z0, z0 + 400), ("x", -C, C, 0.0, 0.25))})
        p.cbox((-30, 150 if s > 0 else -200, 0), (30, 200 if s > 0 else -150, 600))
    p.box((-C, -150, 400), (C, 150, 430), "kick", faces="xXz",
          uvo={"z": (("y", -200, 200), ("x", -C, C))})
    p.box((-F, -200, 0), (F, 200, 600), "sw_gun", faces="yYZ")
    p.cbox((-30, -150, 400), (30, 150, 600))
    return p


def b_door():
    p = Piece("SM_DI_Door", EXPECT["SM_DI_Door"])
    p.box((-10, -150, 0), (10, 150, 400), "sw_gun")
    if DETAIL:
        for x, n in ((-10.3, -1), (10.3, 1)):
            p.poly([(x, -150, 178), (x, 150, 178), (x, 150, 222), (x, -150, 222)], "hazard", ("y", -200, 200), ("z", 178, 222), n=(n, 0, 0))
        p.box((-14.5, -146, 8), (14.5, 146, 178), "armourA", regs={k: "sw_gun" for k in "yYzZ"})
        p.box((-14.5, -146, 222), (14.5, 146, 392), "armourB", regs={k: "sw_gun" for k in "yYzZ"})
        for x, n in ((-15, -1), (15, 1)):
            for a, b in ((-138, -100), (100, 138)):
                p.poly([(x, a, 228), (x, b, 228), (x, b, 234), (x, a, 234)], mat="GlowRed", n=(n, 0, 0))
    else:
        p.box((-15, -146, 8), (15, 146, 392), "sw_black")
    p.cbox((-15, -150, 0), (15, 150, 400))
    return p


# ------------------------------------------------------------------------------------------------ room pieces

def b_room_wall():
    p = Piece("SM_DI_Room_Wall", EXPECT["SM_DI_Room_Wall"])
    for deg in (90, -90):
        with p.at(rotz(deg)):
            wall_face(p, yf=-20.5, cove=False, lip=9.0, back=0.0)
    p.box((-20.5, -200, 0), (20.5, 200, 600), "sw_gun", faces="Z")
    if DETAIL:
        uvo = {k: (("z", 0, 600), ("y", -12, 12)) for k in "xX"}
        uvo.update({k: (("z", 0, 600), ("x", -29.5, 29.5, 0.0, 0.5)) for k in "yY"})
        p.box((-29.5, -12, 0), (29.5, 12, 600), "kick", faces="xXyY", uvo=uvo)
    else:
        p.box((-30, -12, 0), (30, 12, 600), "sw_gun", faces="xXyY")
    p.cbox((-30, -200, 0), (30, 200, 600))
    return p


def b_room_floor():
    p = Piece("SM_DI_Room_Floor", EXPECT["SM_DI_Room_Floor"])
    plates(p)
    p.box((-200, -200, -20), (200, 200, 0), "sw_gun", faces="xXyYz")
    p.cbox((-200, -200, -20), (200, 200, 0))
    return p


def b_room_ceiling():
    p = Piece("SM_DI_Room_Ceiling", EXPECT["SM_DI_Room_Ceiling"])
    U, V = ("x", -200, 200), ("y", -200, 200)

    def under(x0, x1, y0, y1):
        p.poly([(x0, y0, 600), (x1, y0, 600), (x1, y1, 600), (x0, y1, 600)], "ceiling", U, V, n=(0, 0, -1))
    if DETAIL:
        under(-200, 200, -200, -10)
        under(-200, 200, 10, 200)
        under(-200, -60, -10, 10)
        under(60, 200, -10, 10)
        p.poly([(-60, -10, 606), (60, -10, 606), (60, 10, 606), (-60, 10, 606)], mat="GlowWhite", n=(0, 0, -1))
        p.box((-60, -10, 600), (60, 10, 606), "sw_light", faces="xXyY")
        p.faces[-4:] = [(vs[::-1], uv[::-1], m) for vs, uv, m in p.faces[-4:]]     # the pocket is seen from inside
    else:
        under(-200, 200, -200, 200)
    p.box((-200, -200, 600), (200, 200, 620), "sw_black", faces="xXyYZ")
    p.cbox((-200, -200, 600), (200, 200, 620))
    return p


def b_cover():
    p = Piece("SM_DI_Cover_Low", EXPECT["SM_DI_Cover_Low"])
    if DETAIL:
        body = [(-27, 0), (27, 0), (27, 14), (14.5, 98), (14.5, 110), (-14.5, 110), (-14.5, 98), (-27, 14)]
        p.prism(body, "xz", -94, 94, ["sw_gun", "kick", "armourA", "sw_gun", "hazard", "sw_gun", "armourB", "kick"], caps=False)
        end = [(-30, 0), (30, 0), (30, 16), (15, 100), (15, 110), (-15, 110), (-15, 100), (-30, 16)]
        for a, b in ((-100, -94), (94, 100)):
            p.prism(end, "xz", a, b, "sw_gun", cap_reg="gunmetal")
        for x, n in ((-15, -1), (15, 1)):
            p.poly([(x, -88, 103), (x, 88, 103), (x, 88, 106), (x, -88, 106)], mat="GlowRed", n=(n, 0, 0))
    else:
        p.prism([(-30, 0), (30, 0), (30, 16), (15, 100), (15, 110), (-15, 110), (-15, 100), (-30, 16)], "xz", -100, 100, "sw_black")
    p.cbox((-30, -100, 0), (30, 100, 110))
    return p


def b_pad():
    p = Piece("SM_DI_Hangar_Pad", EXPECT["SM_DI_Hangar_Pad"])
    zt = -0.4 if DETAIL else 0.0
    for sx in (-1, 1):
        for sy in (-1, 1):
            vs = [(0, 0, zt), (600 * sx, 0, zt), (600 * sx, 600 * sy, zt), (0, 600 * sy, zt)]
            p.poly(vs, "pad", uv=[(abs(x) / 600.0, 1.0 - abs(y) / 600.0) for x, y, _ in vs], n=(0, 0, 1))
    for k in range(3):
        a = -600 + 400 * k
        p.box((-600, a, -12), (600, a + 400, zt), "kick", faces="xX")
        p.box((a, -600, -12), (a + 400, 600, zt), "kick", faces="yY")
        p.box((-600, a, -40), (600, a + 400, -12), "hazard", faces="xX")
        p.box((a, -600, -40), (a + 400, 600, -12), "hazard", faces="yY")
    p.box((-600, -600, -40), (600, 600, 0), "sw_gun", faces="z")
    if DETAIL:
        for i in range(12):
            t = math.radians(15 + 30 * i)
            with p.at(rotz(15 + 30 * i)):
                p.poly([(322, -22, 0), (338, -22, 0), (338, 22, 0), (322, 22, 0)], mat="GlowRed", n=(0, 0, 1))
        for sx in (-1, 1):
            for sy in (-1, 1):
                p.poly([(500 * sx - 10, 500 * sy - 10, 0), (500 * sx + 10, 500 * sy - 10, 0), (500 * sx + 10, 500 * sy + 10, 0),
                        (500 * sx - 10, 500 * sy + 10, 0)], mat="GlowWhite", n=(0, 0, 1))
    p.cbox((-600, -600, -40), (600, 600, 0))
    return p


# ------------------------------------------------------------------------------------------------ props

def b_console():
    p = Piece("SM_DI_Console", EXPECT["SM_DI_Console"])
    p.box((-30, -60, 0), (30, 60, 10), "kick", regs={"Z": "sw_gun", "z": "sw_gun"})
    p.box((-16, -42, 10), (22, 42, 86), "sw_black", faces="xXyY",
          regs={"x": "hatch", "X": "vent"},
          uvo={"x": (("y", 42, -42), ("z", 10, 86)), "X": (("y", -42, 42), ("z", 10, 86))})
    head = [(-30, 86), (30, 86), (30, 118), (20, 130), (8, 130), (-2, 102), (-30, 95)]
    p.prism(head, "xz", -60, 60, ["sw_gun", "gunmetal", "sw_gun", "sw_gun", "screen", "keys", "sw_gun"], fitu=True,
            vr=[None, (0.0, 0.5), None, None, (1.0, 0.0), (1.0, 0.0), None])
    if DETAIL:
        d = (-10 / 29.73, -28 / 29.73)
        n = (-0.942, 0.336)
        a = (8 + d[0] * 2.6 + n[0] * 0.3, 130 + d[1] * 2.6 + n[1] * 0.3)
        b = (-2 - d[0] * 2.6 + n[0] * 0.3, 102 - d[1] * 2.6 + n[1] * 0.3)
        p.poly([(a[0], -54, a[1]), (a[0], 54, a[1]), (b[0], 54, b[1]), (b[0], -54, b[1])], mat="Screen", n=(n[0], 0, n[1]))
        p.poly([(-16.5, -30, 14), (-16.5, 30, 14), (-16.5, 30, 18), (-16.5, -30, 18)], mat="GlowRed", n=(-1, 0, 0))
    p.cbox((-30, -60, 0), (30, 60, 130))
    return p


def b_conduit():
    p = Piece("SM_DI_Conduit", EXPECT["SM_DI_Conduit"])
    p.box((-60, -60, 0), (60, 60, 36), "hazard", regs={"Z": "sw_gun", "z": "sw_gun"})
    p.box((-60, -60, 264), (60, 60, 300), "kick", regs={"Z": "vent", "z": "sw_gun"},
          uvo={"Z": (("x", -60, 60), ("y", -60, 60))})
    for sx in (-1, 1):
        for sy in (-1, 1):
            p.box((sx * 53 - 7, sy * 53 - 7, 36), (sx * 53 + 7, sy * 53 + 7, 264), "sw_gun", faces="xXyY")
    p.lathe([(18, 40), (34, 54), (34, 246), (18, 260)], 8, mat="Cell", a0=22.5, a1=382.5)
    if DETAIL:
        for z in (92, 194):
            p.lathe([(34, z), (41, z), (41, z + 14), (34, z + 14)], 8, "sw_gun", a0=22.5, a1=382.5, closed=True)
        p.lathe([(12, 36), (22, 36), (22, 44), (12, 44)], 8, "sw_gun", a0=22.5, a1=382.5, closed=True)
    p.cbox((-60, -60, 0), (60, 60, 300))
    return p


def b_walkway():
    p = Piece("SM_DI_Walkway_Ring", EXPECT["SM_DI_Walkway_Ring"])
    U = ("x", -200, 200)
    for a, b, reg in ((-149.5, -110, "hazard"), (-110, 90, "grating"), (90, 150, "kick")):
        p.poly([(-200, a, 0), (200, a, 0), (200, b, 0), (-200, b, 0)], reg, U, ("y", a, b), n=(0, 0, 1))
    p.box((-200, -149.5, -20), (200, 150, 0), "kick", faces="xXyYz", regs={"x": "sw_gun", "X": "sw_gun", "z": "sw_gun"})
    p.poly([(-200, -150, -14), (200, -150, -14), (200, -150, -6), (-200, -150, -6)], mat="GlowRed", n=(0, -1, 0))
    for x in (-100, 100):
        p.box((x - 3, 143, 0), (x + 3, 149, 104), "sw_gun", faces="xXyY")
    p.box((-200, 141, 102), (200, 150, 110), "sw_gun")
    if DETAIL:
        p.box((-200, 144, 52), (200, 148, 57), "sw_gun")
        p.box((-200, 146, 0), (200, 149, 14), "kick", faces="xXyYZ")
    p.cbox((-200, -150, -20), (200, 150, 0))
    p.cbox((-200, 141, 0), (200, 150, 110))
    return p


def b_debris(variant):
    name = "SM_DI_Debris_" + variant
    p = Piece(name, None)
    rnd = random.Random({"A": 11, "B": 23, "C": 37}[variant])
    n = rnd.randint(8, 11)
    pts = []
    for i in range(n):
        t = 2 * math.pi * i / n + rnd.uniform(-0.18, 0.18)
        r = rnd.uniform(38, 78) * (1.25 if i % 3 == 0 else 0.85)
        pts.append((r * math.cos(t), r * math.sin(t)))
    th = rnd.uniform(9, 13)
    fold_x = rnd.uniform(-12, 12)
    ang = math.radians(rnd.uniform(22, 40))
    la, ha = min(a for a, _ in pts), max(a for a, _ in pts)
    lb, hb = min(b for _, b in pts), max(b for _, b in pts)

    def bend(x, y, z):
        if x > fold_x:
            dx = x - fold_x
            return (fold_x + dx * math.cos(ang) - z * math.sin(ang), y, dx * math.sin(ang) + z * math.cos(ang))
        return (x, y, z)

    faces = []
    # split the outline at the fold so both halves stay planar
    def clip(poly, keep_right):
        out = []
        for i in range(len(poly)):
            a, b = poly[i], poly[(i + 1) % len(poly)]
            ia, ib = (a[0] > fold_x) == keep_right, (b[0] > fold_x) == keep_right
            if ia:
                out.append(a)
            if ia != ib:
                t = (fold_x - a[0]) / (b[0] - a[0])
                out.append((fold_x, a[1] + t * (b[1] - a[1])))
        return out
    for half in (clip(pts, False), clip(pts, True)):
        if len(half) < 3:
            continue
        uvf = [((a - la) / (ha - la) * 0.5, 0.36 + 0.6 * (b - lb) / (hb - lb)) for a, b in half]
        faces.append(([bend(a, b, th) for a, b in half], "armourA", uvf, 1))
        uvb = [((a - la) / (ha - la), (b - lb) / (hb - lb)) for a, b in half]
        faces.append(([bend(a, b, 0) for a, b in half], "torn", uvb, -1))
        for i in range(len(half)):
            a, b = half[i], half[(i + 1) % len(half)]
            if abs(a[0] - fold_x) < 1e-6 and abs(b[0] - fold_x) < 1e-6:
                continue
            u0, u1 = rnd.uniform(0, 0.7), 0
            u1 = u0 + 0.25
            faces.append(([bend(a[0], a[1], 0), bend(b[0], b[1], 0), bend(b[0], b[1], th), bend(a[0], a[1], th)], "torn",
                          [(u0, 0.1), (u1, 0.1), (u1, 0.25), (u0, 0.25)], 0))
    # a snapped stiffener rib across the back
    ry = rnd.uniform(-15, 15)
    rib = ((la * 0.7, ry - 6, -12), (fold_x - 2, ry + 6, 0))
    # tumble it, then rest it on the floor with the pivot at the bottom centre of its bounds
    from mathutils import Euler, Vector
    R = Euler((math.radians(rnd.uniform(15, 50)), math.radians(rnd.uniform(-35, 35)), math.radians(rnd.uniform(0, 360)))).to_matrix().to_4x4()
    q = Piece("tmp")
    for vs, reg, uv, s in faces:
        q.poly(vs, reg, uv=uv)
    q.box(rib[0], rib[1], "gunmetal", uvo={k: (("x", rib[0][0], rib[0][0] + 160), ("z", -12, 0, 0.0, 0.2)) for k in "yY"})
    allp = [R @ Vector(v) for f in q.faces for v in f[0]]
    lo = Vector((min(v.x for v in allp), min(v.y for v in allp), min(v.z for v in allp)))
    hi = Vector((max(v.x for v in allp), max(v.y for v in allp), max(v.z for v in allp)))
    big = max(hi - lo)
    sc = 150.0 / big
    from mathutils import Matrix
    T = Matrix.Scale(sc, 4) @ Matrix.Translation((-(lo.x + hi.x) / 2, -(lo.y + hi.y) / 2, -lo.z)) @ R
    for vs, uv, m in q.faces:
        p.faces.append(([T @ v for v in vs], uv, m))
    p.notes = q.notes
    # outward winding: these are thin shells built without normal hints; fix them against the plate's centre plane
    p.fix_normals = True
    p.chull(p.all_points())
    return p


def b_rib():
    p = Piece("SM_DI_Bulkhead_Rib", EXPECT["SM_DI_Bulkhead_Rib"])
    uvz = lambda z0, z1, a, b, ax: (("z", z0, z1), (ax, a, b, 0.0, 0.5))
    p.box((-7, -12, 0), (7, 12, 60), "hazard", faces="xXyY",
          uvo={"x": (("z", 0, 60, 0.0, 0.3125), ("y", -12, 12)), "X": (("z", 0, 60, 0.0, 0.3125), ("y", -12, 12)),
               "y": (("z", 0, 60, 0.0, 0.3125), ("x", -7, 7)), "Y": (("z", 0, 60, 0.0, 0.3125), ("x", -7, 7))})
    p.box((-7, -12, 60), (7, 12, 600), "kick", faces="xXyYZ",
          uvo={"x": uvz(60, 600, -12, 12, "y"), "X": uvz(60, 600, -12, 12, "y"), "y": uvz(60, 600, -7, 7, "x"), "Y": uvz(60, 600, -7, 7, "x")})
    p.cbox((-7, -12, 0), (7, 12, 600))
    return p


def b_shaft():
    p = Piece("SM_DI_Shaft", EXPECT["SM_DI_Shaft"])
    U = ("x", -200, 200)
    for deg in (0, 90, 180, 270):
        with p.at(rotz(deg)):
            for k in range(3):
                z = 400 * k
                for z0, z1, reg in ((z, z + 150, "armourA"), (z + 150, z + 200, "cable"), (z + 200, z + 350, "armourB"), (z + 350, z + 400, "kick")):
                    p.poly([(-200, 170, z0), (200, 170, z0), (200, 170, z1), (-200, 170, z1)], reg, U, ("z", z0, z1), n=(0, -1, 0))
            # corner post with a white light strip (one per wall, at its +X end)
            p.box((140, 140, 0), (170, 170, 1200), "sw_gun", faces="xy")
            p.poly([(139.5, 150, 20), (139.5, 160, 20), (139.5, 160, 1180), (139.5, 150, 1180)], mat="GlowWhite", n=(-1, 0, 0))
            p.cbox((-200, 170, 0), (200, 200, 1200))
    p.box((-200, -200, 0), (200, 200, 1200), "sw_black", faces="xXyY")
    return p


# ------------------------------------------------------------------------------------------------ the core

def _smooth(prof, sub):
    """Catmull-Rom through (r, z) points, `sub` steps per span."""
    out = []
    P = [prof[0]] + list(prof) + [prof[-1]]
    for i in range(1, len(P) - 2):
        p0, p1, p2, p3 = P[i - 1], P[i], P[i + 1], P[i + 2]
        for s in range(sub):
            t = s / sub
            out.append(tuple(0.5 * ((2 * p1[k]) + (-p0[k] + p2[k]) * t + (2 * p0[k] - 5 * p1[k] + 4 * p2[k] - p3[k]) * t * t
                                    + (-p0[k] + 3 * p1[k] - 3 * p2[k] + p3[k]) * t ** 3) for k in (0, 1)))
    out.append(tuple(prof[-1]))
    return out


def _petal(p, prof, half_deg, taper, cols, thick=14.0, glow=(0.12, 0.8)):
    rows = _smooth(prof, 4 if DETAIL else 1)
    tap = _smooth([(t, i) for i, t in enumerate(taper)], 4 if DETAIL else 1)
    m = len(rows)

    def pt(j, c, inner=False):
        r, z = rows[j]
        a = math.radians(half_deg * tap[j][0] * (2.0 * c / cols - 1.0))
        r = r - (thick if inner else 0.0)
        return (r * math.cos(a), r * math.sin(a), z)

    for j in range(m - 1):
        v0, v1 = j / (m - 1), (j + 1) / (m - 1)
        lower = v1 <= 0.5 + 1e-6
        reg = "petal" if lower else "armourA"
        w0, w1 = ((2 * v0, 2 * v1) if lower else (0.36 + 0.64 * (2 * v0 - 1), 0.36 + 0.64 * (2 * v1 - 1)))
        for c in range(cols):
            u0, u1 = c / cols, (c + 1) / cols
            if not lower:
                u0, u1 = u0 * 0.5, u1 * 0.5
            nrm = (rows[j + 1][1] - rows[j][1], 0, -(rows[j + 1][0] - rows[j][0]))
            p.poly([pt(j, c), pt(j, c + 1), pt(j + 1, c + 1), pt(j + 1, c)], reg,
                   uv=[(u0, w0), (u1, w0), (u1, w1), (u0, w1)], n=nrm)
            p.poly([pt(j, c, True), pt(j, c + 1, True), pt(j + 1, c + 1, True), pt(j + 1, c, True)], "sw_black", uv=[(0.5, 0.5)] * 4, n=(-nrm[0], 0, -nrm[2]))
        hot = glow[0] <= v0 and v1 <= glow[1] and DETAIL
        for c, s in ((0, -1), (cols, 1)):
            p.poly([pt(j, c), pt(j, c, True), pt(j + 1, c, True), pt(j + 1, c)], "sw_gun", uv=[(0.5, 0.5)] * 4, n=(0, s, 0),
                   mat="GlowRed" if hot else "Atlas")
    p.poly([pt(m - 1, c) for c in range(cols + 1)] + [pt(m - 1, c, True) for c in range(cols, -1, -1)], "sw_gun",
           uv=[(0.5, 0.5)] * (2 * cols + 2), n=(0, 0, 1))


def b_core():
    p = Piece("SM_DI_Core", EXPECT["SM_DI_Core"])
    N = 24 if DETAIL else 12
    # stepped base drum
    p.lathe([(400, 0), (400, 34), (388, 40), (340, 40), (340, 104), (330, 110), (165, 110)], N,
            ["hazard", "sw_gun", "kick", "gunmetal", "sw_gun", "kick"], reps=6)
    # collars holding the core, the white-hot core, the lance to the crown
    p.lathe([(165, 110), (165, 235), (130, 250)], N, ["gunmetal", "sw_gun"], reps=3)
    p.lathe([(130, 950), (165, 965), (165, 1040), (60, 1056)], N, ["sw_gun", "gunmetal", "kick"], reps=3)
    p.lathe(_smooth([(100, 250), (128, 340), (138, 600), (128, 860), (100, 950)], 3 if DETAIL else 1), N, mat="CoreGlow")
    p.lathe([(38, 1050), (38, 1160)], 12, mat="CoreGlow")
    p.lathe([(108, 1146), (126, 1162), (126, 1188), (100, 1200), (0, 1200)], N, ["sw_gun", "kick", "sw_gun", "sw_black"], reps=3)
    p.lathe([(60, 1150), (108, 1146)], N, "sw_black")
    if DETAIL:
        for k in range(3):                                   # cage struts between the petal pairs
            with p.at(rotz(60 + 120 * k)):
                p.box((146, -6, 235), (162, 6, 965), "kick", faces="xXyY",
                      uvo={f: (("z", 235, 965), (a, lo, hi, 0.0, 0.3)) for f, a, lo, hi in (("x", "y", -6, 6), ("X", "y", -6, 6), ("y", "x", 146, 162), ("Y", "x", 146, 162))})
                for z in (420, 600, 780):
                    p.lathe([(140, z - 8), (150, z - 8), (150, z + 8), (140, z + 8)], 6, "sw_gun", a0=-60, a1=60, closed=True)
        for k in range(12):                                  # buttress fins round the drum
            with p.at(rotz(15 + 30 * k)):
                p.prism([(322, 40), (392, 40), (392, 52), (345, 118), (322, 118)], "xz", -9, 9, ["sw_gun", "sw_gun", "armourA", "sw_gun", None],
                        cap_reg="gunmetal", fitu=True)
        for k in range(3):                                   # the half-covered three-chevron mark, three times
            with p.at(rotz(7.5 + 120 * k)):
                x = 340 * math.cos(math.radians(7.5)) + 0.6
                p.poly([(x, -29, 44), (x, 29, 44), (x, 29, 102), (x, -29, 102)], "triad", ("y", -29, 29), ("z", 44, 102), n=(1, 0, 0))
    tall = [(338, 108), (384, 300), (394, 520), (362, 760), (286, 960), (176, 1130), (122, 1198)]
    short = [(338, 108), (376, 250), (386, 420), (360, 600), (304, 750), (246, 850), (214, 900)]
    tap_t = [1.0, 1.0, 1.0, 0.97, 0.88, 0.7, 0.5]
    tap_s = [1.0, 1.0, 1.0, 0.97, 0.9, 0.8, 0.65]
    for k in range(6):
        with p.at(rotz(60 * k)):
            if k % 2 == 0:
                _petal(p, tall, 24.0, tap_t, 5 if DETAIL else 2)
            else:
                _petal(p, short, 23.0, tap_s, 5 if DETAIL else 2)
            if DETAIL:      # a raised spine plate low on every petal
                sp = [(r + 4, z) for r, z in ((345, 130), (384, 300), (393, 470))]
                _petal(p, sp, 7.0, [1.0, 1.0, 0.8], 2, thick=10.0, glow=(2, 3))
    ring = [(400 * math.cos(math.radians(15 * i)), 400 * math.sin(math.radians(15 * i))) for i in range(24)]
    p.chull([(x, y, z) for x, y in ring for z in (0, 110)])
    p.chull([(x * 0.97, y * 0.97, z) for x, y in ring for z in (110, 700)] + [(x * 0.5, y * 0.5, 1200) for x, y in ring])
    return p


def _ring(name, R, sec, n, blocks, block_half, glow_mat):
    p = Piece(name, None)
    prof = [(R + a, b) for a, b in sec]
    regs = ["sw_black", "armourA", "gunmetal", "armourA", "sw_black", "sw_gun", None, "sw_gun"]
    p.lathe(prof, n, regs, reps=n // 8, closed=True, vr=[None, (0.36, 1.0), None, (0.36, 1.0), None, None, None, None])
    p.lathe([prof[6], prof[7]], n, mat="GlowRed")
    w, h = block_half
    rin, rout = R + sec[7][0] - 6, R + sec[2][0] + 8
    for k in range(blocks):
        with p.at(rotz(360.0 * k / blocks)):
            p.box((rin, -w, -h), (rout, w, h), "petal", regs={"z": "sw_gun", "Z": "hatch", "y": "gunmetal", "Y": "gunmetal"},
                  uvo={"X": (("y", -w, w), ("z", -h, h)), "x": (("y", -w, w), ("z", -h, h)), "Z": (("x", rin, rout), ("y", -w, w)),
                       "y": (("x", rin, rin + 160), ("z", -h, h)), "Y": (("x", rin, rin + 160), ("z", -h, h))})
            if DETAIL:
                p.poly([(rout + 0.5, -w * 0.6, -3), (rout + 0.5, w * 0.6, -3), (rout + 0.5, w * 0.6, 3), (rout + 0.5, -w * 0.6, 3)], mat=glow_mat, n=(1, 0, 0))
    return p


def b_ring_a():
    sec = [(-16, -22), (16, -22), (26, -6), (26, 6), (16, 22), (-16, 22), (-26, 5), (-26, -5)]
    return _ring("SM_DI_Core_Ring_A", 469.5, sec, 48, 3, (56, 30), "GlowWhite")


def b_ring_b():
    sec = [(-12, -16), (12, -16), (20, -5), (20, 5), (12, 16), (-12, 16), (-20, 4), (-20, -4)]
    return _ring("SM_DI_Core_Ring_B", 546.5, sec, 48, 2, (44, 24), "GlowWhite")


def b_shield():
    p = Piece("SM_DI_Core_Shield", EXPECT["SM_DI_Core_Shield"])
    p.lathe([(560, 0), (640, 330), (640, 760), (520, 1120), (270, 1320), (0, 1380)], 12, mat="Shield")
    p.smooth_deg = 1.0
    p.chull(p.all_points())
    return p


BUILDERS = [
    b_corridor_straight, b_corridor_corner, b_corridor_t, b_doorway, b_door, b_room_wall, b_room_floor, b_room_ceiling,
    b_cover, b_pad, b_core, b_ring_a, b_ring_b, b_shield, b_conduit, b_walkway,
    lambda: b_debris("A"), lambda: b_debris("B"), lambda: b_debris("C"), b_console, b_shaft, b_rib,
]

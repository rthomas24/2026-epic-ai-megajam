# SPDX-License-Identifier: MIT
# Kit, Null power conduit (batch AB, T7's Cinder play: the excavation gate's two blade-breakable conduits were plain
# orange cylinders). ATDShatterCrystal fits its art into the placeholder cylinder's box (Cinder: 100 x 100 x 300 cm,
# base on the ground; the reactor's: 120 x 120 x 300), so the intact piece fills exactly 100 x 100 x 300 with its pivot at
# the ground centre and fits either at scale 1 (Cinder) or 1.2 in X/Y (the reactor). On a break the actor hides the
# mesh; the stump is for T5 to show in its place at the same transform. Null trim (Dom slot), ember core (Glow).
import math
import random

from mathutils import Matrix

from kit_lib import Piece


def _oct(r, a0=22.5):
    return [(r * math.cos(math.radians(a0 + 45.0 * k)), r * math.sin(math.radians(a0 + 45.0 * k))) for k in range(8)]


def _base(p):
    """The footing: an octagonal plinth (100 across, 30 high) with a dark top ring and four bolt blocks."""
    R = 50.0 / math.cos(math.radians(22.5))                  # 100 cm across the flats
    p.prism(_oct(R), "xy", 0.0, 22.0, regs="sw_dark", cap_reg="frame", mat="Dom", fitu=True)
    p.prism(_oct(R * 0.84), "xy", 22.0, 30.0, regs="frame", cap_reg="sw_dark", mat="Dom", fitu=True)
    for a in (45.0, 135.0, 225.0, 315.0):
        c, s = math.cos(math.radians(a)) * 36.0, math.sin(math.radians(a)) * 36.0
        p.box((c - 6.0, s - 6.0, 22.0), (c + 6.0, s + 6.0, 27.0), reg="sw_gun", mat="Dom")


def _rib(p, a, z0, z1):
    """One armour rib on the 45-degree diagonals, 14 wide, standing 9 cm proud of the core."""
    with p.at(Matrix.Rotation(math.radians(a), 4, "Z")):
        p.box((22.0, -7.0, z0), (35.0, 7.0, z1), regs={"X": "wall", "x": "sw_dark", "y": "frame", "Y": "frame", "Z": "sw_dark", "z": "sw_dark"}, mat="Dom")


def conduit():
    """Intact (100 x 100 x 300): footing, an ember core column (36 across) caged by four armour ribs, two collar rings,
    a dark cap with an emitter lip. Hull: one octagonal prism of the whole."""
    p = Piece("SM_Kit_NullConduit", ((-50.0, -50.0, 0.0), (50.0, 50.0, 300.0)))
    _base(p)
    p.prism(_oct(19.5), "xy", 30.0, 262.0, regs="frame", cap_reg="frame", caps=False, mat="Glow", fitu=True)
    for a in (45.0, 135.0, 225.0, 315.0):
        _rib(p, a, 30.0, 262.0)
    for z in (110.0, 196.0):
        p.prism(_oct(38.0), "xy", z, z + 14.0, regs="frame", cap_reg="sw_dark", mat="Dom", fitu=True)
    R = 50.0 / math.cos(math.radians(22.5))
    p.prism(_oct(R * 0.9), "xy", 262.0, 284.0, regs="sw_dark", cap_reg="sw_dark", mat="Dom", fitu=True)
    p.prism(_oct(R * 0.62), "xy", 284.0, 296.0, regs="frame", cap_reg="sw_black", mat="Dom", fitu=True)
    p.prism(_oct(R * 0.4), "xy", 296.0, 300.0, regs="frame", cap_reg="frame", mat="Glow", fitu=True)
    p.chull([(x, y, z) for (x, y) in _oct(R) for z in (0.0, 300.0)])
    return p


def conduit_broken():
    """The stump left after the blade (100 x 100, about 130 high): the same footing, the ribs snapped at uneven
    heights and bent outward, the core shattered to a jagged dim-ember stub, the lower collar cracked. Pivot as intact."""
    p = Piece("SM_Kit_NullConduitBroken", None)
    rnd = random.Random(31)
    _base(p)
    for i, a in enumerate((45.0, 135.0, 225.0, 315.0)):
        h = (70.0, 128.0, 92.0, 112.0)[i]
        _rib(p, a, 30.0, h)
        with p.at(Matrix.Rotation(math.radians(a), 4, "Z") @ Matrix.Translation((28.0, 0.0, h)) @ Matrix.Rotation(math.radians(28.0), 4, "Y")):
            p.box((-6.0, -6.0, 0.0), (7.0, 6.0, rnd.uniform(16.0, 30.0)), reg="sw_dark", mat="Dom")
    for k, (a, b) in enumerate(zip(_oct(19.5), _oct(19.5)[1:] + _oct(19.5)[:1])):
        top = 30.0 + rnd.uniform(18.0, 62.0)
        p.prism([a, b, (b[0] * 0.3, b[1] * 0.3), (a[0] * 0.3, a[1] * 0.3)], "xy", 30.0, top, regs="frame", cap_reg="frame", mat="Glow", fitu=True)
    # the lower collar cracked off: two bent segments still hang on the tallest ribs, the rest lie at the foot
    oc = _oct(38.0)
    for k in (0, 1):
        a, b = oc[k], oc[k + 1]
        ai, bi = (a[0] * 0.78, a[1] * 0.78), (b[0] * 0.78, b[1] * 0.78)
        with p.at(Matrix.Translation((0.0, 0.0, 104.0)) @ Matrix.Rotation(math.radians(-12.0 * (k + 1)), 4, "X")):
            p.prism([a, b, bi, ai], "xy", 0.0, 14.0, regs="frame", cap_reg="sw_dark", mat="Dom", fitu=True)
    for k, (gx, gy, yaw) in enumerate(((-30.0, 40.0, 20.0), (38.0, -34.0, -50.0))):
        with p.at(Matrix.Translation((gx, gy, 30.0)) @ Matrix.Rotation(math.radians(yaw), 4, "Z")):
            p.box((-14.0, -5.0, 0.0), (14.0, 5.0, 10.0), regs={"Z": "frame"}, reg="sw_dark", mat="Dom")
    R = 50.0 / math.cos(math.radians(22.5))
    p.chull([(x, y, z) for (x, y) in _oct(R) for z in (0.0, 30.0)] + [(x, y, 95.0) for (x, y) in _oct(36.0)])
    return p


BUILDERS = [conduit, conduit_broken]

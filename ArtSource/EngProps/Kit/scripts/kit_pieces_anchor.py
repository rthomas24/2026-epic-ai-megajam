# SPDX-License-Identifier: MIT
# Kit, Cinder field anchor collapse (batch X, the Lead from T7's play): what the course did not have yet. The course
# already has SM_Cind_DebrisChunk (ATDCollapseRun's falling rock), Grok Bot's SM_CinderAnchorTower / _Emitter (the
# tower that sinks and leans) and placeholder cubes for the three charge sites. So: the planted charge, three broken
# tower sections that lie where the tower came down, and three falling-debris variants of tower shell.
# Null tech: the Dominion trim (Dom slot) with the Cinder ember Glow. Pivot on the ground (charge: on its back plate,
# +X out of the surface it is clamped to). One simple convex hull each; the 1-2 m debris are one hull too.
import math
import random

from mathutils import Matrix

from kit_lib import Piece


def _oct(r, a0=22.5):
    return [(r * math.cos(math.radians(a0 + 45.0 * k)), r * math.sin(math.radians(a0 + 45.0 * k))) for k in range(8)]


def charge():
    """The planted charge (60 x 40 x 30 cm): a clamp plate on the surface, the charge body with two hazard straps, an
    ember status strip and a stub antenna. +X = away from the tower face it is clamped to."""
    p = Piece("SM_Kit_AnchorCharge", None)
    p.box((0.0, -30.0, -20.0), (4.0, 30.0, 20.0), reg="sw_dark", mat="Dom")
    p.box((4.0, -24.0, -15.0), (28.0, 24.0, 15.0), regs={"X": "wall", "x": "sw_dark", "y": "frame", "Y": "frame", "z": "frame", "Z": "frame"}, mat="Dom")
    for y in (-14.0, 10.0):
        p.box((3.5, y, -16.0), (28.6, y + 4.0, 16.0), reg="hazard")
    p.box((28.0, -10.0, 4.0), (29.5, 10.0, 8.0), mat="Glow")
    p.box((10.0, 16.0, 15.0), (13.0, 19.0, 34.0), reg="sw_dark")
    p.cbox((0.0, -30.0, -20.0), (29.5, 30.0, 20.0))
    return p


def _section(name, L, R, seed, both_broken):
    """A tower section lying on one flat face along +X: an octagonal Null armour shaft (radius R, length L) with a
    jagged break at +X (and at -X if both_broken), an ember seam ring near each break, armour bands."""
    p = Piece(name, None)
    rnd = random.Random(seed)
    h = R * math.cos(math.radians(22.5))
    oc = _oct(R)
    rot = Matrix.Translation((0.0, 0.0, h)) @ Matrix.Rotation(math.radians(90.0), 4, "Y")    # shaft axis Z -> X
    with p.at(rot):
        # shaft: armour faces with a dark band every 150 cm
        z = 0.0
        k = 0
        while z < L - 1e-3:
            z1 = min(L, z + 150.0)
            p.prism(oc, "xy", -z1, -z, regs="wall" if k % 2 == 0 else "sw_dark", cap_reg="sw_dark", caps=False, mat="Dom", fitu=True)
            z, k = z1, k + 1
        ends = [(-L, -1.0)] + ([(0.0, 1.0)] if both_broken else [])
        if not both_broken:                     # the intact end: a capped flange
            p.prism(_oct(R + 12.0), "xy", 0.0, 18.0, regs="frame", cap_reg="sw_dark", mat="Dom", fitu=True)
        for (ze, s) in ends:
            # the break: torn armour teeth of random length past the end, an ember ring just inside it
            for kk in range(8):
                a, b = oc[kk], oc[(kk + 1) % 8]
                t = rnd.uniform(0.6 * R, 1.4 * R) if kk % 2 == 0 else rnd.uniform(8.0, 0.3 * R)      # torn: long and short teeth
                prof = [(a[0], a[1]), (b[0], b[1]), (b[0] * 0.82, b[1] * 0.82), (a[0] * 0.82, a[1] * 0.82)]
                z0, z1 = sorted((ze, ze + s * t))
                p.prism(prof, "xy", z0, z1, regs="sw_dark", cap_reg="sw_gun", mat="Dom", fitu=True)
            zr0, zr1 = sorted((ze - s * 30.0, ze - s * 42.0))
            p.prism(_oct(R + 1.5), "xy", zr0, zr1, regs="frame", cap_reg="frame", caps=False, mat="Glow", fitu=True)
            p.prism(_oct(R * 0.8), "xy", min(ze, ze - s * 2.0), max(ze, ze - s * 2.0), regs="sw_black", cap_reg="sw_black", mat="Dom", fitu=True)
        pts = [(x, y, zz) for (x, y) in _oct(R + (0.0 if both_broken else 12.0)) for zz in (-L, 18.0 if not both_broken else 0.0)]
        p.chull(pts)
    return p


def section_a():
    """The top section with the intact flange: 6 m long, 1.6 m across, broken at its base."""
    return _section("SM_Kit_AnchorSection_A", 600.0, 80.0, 11, False)


def section_b():
    """A middle section, broken at both ends: 4.5 m, 2 m across."""
    return _section("SM_Kit_AnchorSection_B", 450.0, 100.0, 12, True)


def section_c():
    """A short stub, broken at both ends: 2.5 m, 1.6 m across (a mantle-height obstacle on the run)."""
    return _section("SM_Kit_AnchorSection_C", 250.0, 80.0, 13, True)


def debris_a():
    """Falling shell plate (about 200 x 140 x 40 cm): an armour plate bent along one crease, ember-lit edge."""
    p = Piece("SM_Kit_AnchorDebris_A", None)
    p.box((-100.0, -70.0, 0.0), (0.0, 70.0, 12.0), regs={"Z": "wall", "z": "sw_dark"}, mat="Dom")
    with p.at(Matrix.Rotation(math.radians(-22.0), 4, "Y")):
        p.box((0.0, -70.0, 0.0), (100.0, 70.0, 12.0), regs={"Z": "wall", "z": "sw_dark"}, mat="Dom")
        p.box((96.0, -66.0, 12.0), (100.0, 66.0, 14.0), mat="Glow")
        p.chull([(x, y, z) for x in (0.0, 100.0) for y in (-70.0, 70.0) for z in (0.0, 14.0)])
    p.chull([(x, y, z) for x in (-100.0, 0.0) for y in (-70.0, 70.0) for z in (0.0, 12.0)])
    return p


def debris_b():
    """Falling girder (220 x 40 x 50 cm): an I-section of the tower's frame, snapped on a slant at +X."""
    p = Piece("SM_Kit_AnchorDebris_B", None)
    for (z0, z1) in ((0.0, 8.0), (42.0, 50.0)):
        p.box((-110.0, -20.0, z0), (110.0, 20.0, z1), reg="frame", mat="Dom")
    p.box((-110.0, -4.0, 8.0), (110.0, 4.0, 42.0), reg="sw_dark", mat="Dom")
    p.prism([(110.0, 0.0), (130.0, 0.0), (110.0, 50.0)], "xz", -20.0, 20.0, regs="sw_dark", cap_reg="sw_dark", mat="Dom", fitu=True)
    p.box((-112.0, -20.0, 0.0), (-110.0, 20.0, 50.0), mat="Glow")
    p.chull([(x, y, z) for x in (-112.0, 110.0) for y in (-20.0, 20.0) for z in (0.0, 50.0)] + [(130.0, -20.0, 0.0), (130.0, 20.0, 0.0)])
    return p


def debris_c():
    """Falling shell shard (about 170 x 110 x 60 cm): a curved slab of the octagonal shell, three facets, thick."""
    p = Piece("SM_Kit_AnchorDebris_C", None)
    oc_out, oc_in = _oct(110.0), _oct(86.0)
    pts = []
    lift = 110.0 * math.cos(math.radians(22.5))  # the bottom face on the ground
    for k in (4, 5, 6):                         # the three lowest faces: the slab lies concave side up
        a, b = oc_out[k], oc_out[(k + 1) % 8]
        ai, bi = oc_in[k], oc_in[(k + 1) % 8]
        prof = [(a[0], a[1] + lift), (b[0], b[1] + lift), (bi[0], bi[1] + lift), (ai[0], ai[1] + lift)]
        p.prism(prof, "yz", -85.0, 85.0, regs=["wall", "sw_dark", "sw_dark", "sw_dark"], cap_reg="sw_dark", mat="Dom", fitu=True)
        pts += [(x, y, z) for x in (-85.0, 85.0) for (y, z) in prof]
    p.box((-86.0, -30.0, 6.0), (-84.0, 30.0, 12.0), mat="Glow")
    p.chull(pts)
    return p


BUILDERS = [charge, section_a, section_b, section_c, debris_a, debris_b, debris_c]

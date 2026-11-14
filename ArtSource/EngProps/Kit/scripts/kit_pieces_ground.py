# SPDX-License-Identifier: MIT
# Kit, ground-break and camp perimeter pieces (#194, Ryan: the planets are flat and empty). Route C, code-built in the
# kit's style; nothing walkable is an AI mesh. Pivot on the ground, +X along the piece's main axis; simple collision
# (boxes / a few hulls); one LOD. Slots: Trim (the planet's hut trim), Stone (the planet's stone), Bark (Verdance
# wood), Glow (Verdance teal veins / Cinder strips), Ember (Cinder heat), Light (warm lamps), Beacon.
#   Verdance: SM_Site_Verdance_RootArch, _FallenTrunk (walk-under), _StoneRing, _OvergrownPylon
#   Cinder:   SM_Site_Cinder_BasaltArch, _CollapsedGantry, _VentChimney, _SlagHeap
#   Perimeter (both planets, the palette from the import): SM_Hut_<Planet>_WallLow, _Gate, _LampPost, _LightStrip
import math
import random

from mathutils import Matrix, Vector

from kit_lib import Piece


def _oct(r):
    return [(r * math.cos(math.radians(22.5 + 45.0 * k)), r * math.sin(math.radians(22.5 + 45.0 * k))) for k in range(8)]


def _seg(p, a, b, r0, r1, mat="Bark", reg="wallB"):
    """An eight-sided limb segment from a to b, radius r0 -> r1 (a frustum), along its own axis."""
    a, b = Vector(a), Vector(b)
    d = b - a
    L = d.length
    if L < 1e-3:
        return
    q = d.normalized().to_track_quat("X", "Z")
    with p.at(Matrix.Translation(a) @ q.to_matrix().to_4x4()):
        o0, o1 = _oct(r0), _oct(r1)
        for k in range(8):
            a0, a1 = o0[k], o0[(k + 1) % 8]
            b0, b1 = o1[k], o1[(k + 1) % 8]
            nx = ((a0[0] + a1[0]) / 2, (a0[1] + a1[1]) / 2)
            p.poly([(0.0, a0[0], a0[1]), (0.0, a1[0], a1[1]), (L, b1[0], b1[1]), (L, b0[0], b0[1])], reg,
                   uv=[(0, 0), (1, 0), (1, 1), (0, 1)], mat=mat, n=(0.0, nx[0], nx[1]))


def _limb(p, pts, r0, r1, mat="Bark", reg="wallB"):
    n = len(pts) - 1
    for i in range(n):
        _seg(p, pts[i], pts[i + 1], r0 + (r1 - r0) * i / n * 1.0, r0 + (r1 - r0) * (i + 1) / n, mat, reg)
        if i < n - 1:                                                   # a knuckle so the joints close
            _seg(p, pts[i + 1], Vector(pts[i + 1]) + (Vector(pts[i + 2]) - Vector(pts[i + 1])).normalized() * 2.0,
                 r0 + (r1 - r0) * (i + 1) / n * 1.06, r0 + (r1 - r0) * (i + 1) / n, mat, reg)


def _rock(p, c, r, seed, mat="Stone", flat=0.7, reg="wall"):
    """A faceted boulder: a jittered 8 x 5 lathe-like shell."""
    rnd = random.Random(seed)
    rings = []
    for j, t in enumerate((0.0, 0.3, 0.6, 0.85, 1.0)):
        z = c[2] + r * flat * t * 1.6 - r * 0.15
        rr = r * math.sin(math.pi * (0.15 + 0.85 * t) / 1.15 + 0.2) if t < 1.0 else 0.0
        rings.append([(c[0] + rr * math.cos(2 * math.pi * k / 8) * rnd.uniform(0.82, 1.15),
                       c[1] + rr * math.sin(2 * math.pi * k / 8) * rnd.uniform(0.82, 1.15), z + rnd.uniform(-0.08, 0.08) * r) for k in range(8)])
    for j in range(len(rings) - 1):
        for k in range(8):
            a, b = rings[j][k], rings[j][(k + 1) % 8]
            c1, d1 = rings[j + 1][(k + 1) % 8], rings[j + 1][k]
            mid = ((a[0] + b[0]) / 2 - c[0], (a[1] + b[1]) / 2 - c[1], 0.3 * r)
            if j == len(rings) - 2:
                p.poly([a, b, c1], reg, uv=[(0, 0), (1, 0), (0.5, 1)], mat=mat, n=mid)
            else:
                p.poly([a, b, c1, d1], reg, uv=[(0, 0), (1, 0), (1, 1), (0, 1)], mat=mat, n=mid)


# ------------------------------------------------------------------------------------------------ Verdance
def v_root_arch():
    """A root arch, about 6 m span, 4.5 m high: two thick roots rising out of mossy mounds and twisting into each other
    over the top, side roots, teal bioluminescent veins. Walk-under."""
    p = Piece("SM_Site_Verdance_RootArch", None)
    left = [(-300.0, 0.0, -20.0), (-260.0, 20.0, 150.0), (-170.0, -10.0, 330.0), (-40.0, 20.0, 440.0), (80.0, -10.0, 450.0)]
    right = [(300.0, 10.0, -20.0), (270.0, -20.0, 160.0), (180.0, 15.0, 340.0), (40.0, -15.0, 430.0), (-80.0, 10.0, 445.0)]
    _limb(p, left, 70.0, 30.0)
    _limb(p, right, 65.0, 28.0)
    for (a, b) in (((-300.0, 0.0, 20.0), (-430.0, 120.0, -10.0)), ((-300.0, 0.0, 20.0), (-420.0, -110.0, -10.0)),
                   ((300.0, 10.0, 20.0), (440.0, 100.0, -10.0)), ((300.0, 10.0, 20.0), (410.0, -130.0, -10.0))):
        _limb(p, [a, ((a[0] + b[0]) / 2, (a[1] + b[1]) / 2, 25.0), b], 35.0, 14.0)
    for pts in (left, right):                                          # teal veins along the inner faces
        for i in range(len(pts) - 1):
            a, b = Vector(pts[i]), Vector(pts[i + 1])
            _seg(p, a + Vector((0.0, 0.0, 6.0)), b + Vector((0.0, 0.0, 6.0)), 6.0, 5.0, mat="Glow", reg=None)
    _rock(p, (-300.0, 0.0, 0.0), 110.0, 3, flat=0.45)
    _rock(p, (300.0, 10.0, 0.0), 100.0, 4, flat=0.45)
    p.chull([(x, y, z) for x in (-380.0, -220.0) for y in (-90.0, 90.0) for z in (0.0, 300.0)])
    p.chull([(x, y, z) for x in (220.0, 380.0) for y in (-90.0, 90.0) for z in (0.0, 300.0)])
    p.chull([(x, y, z) for x in (-220.0, 220.0) for y in (-70.0, 70.0) for z in (360.0, 480.0)])
    return p


def v_fallen_trunk():
    """A fallen trunk, 9 m, lying across two boulders 2.3 m up so the pilot walks under it: bark limb, a broken stump
    end, a root plate at the other, moss tufts (Stone), a few teal fungus shelves."""
    p = Piece("SM_Site_Verdance_FallenTrunk", None)
    pts = [(-450.0, 0.0, 230.0), (-150.0, 15.0, 245.0), (150.0, -10.0, 255.0), (450.0, 5.0, 270.0)]
    _limb(p, pts, 85.0, 70.0)
    with p.at(Matrix.Translation((-450.0, 0.0, 230.0)) @ Matrix.Rotation(math.radians(90.0), 4, "Y")):   # root plate
        p.prism(_oct(150.0), "xy", -20.0, 20.0, regs="wallB", cap_reg="wall", mat="Bark", fitu=True)
    for k in range(6):                                                # torn end splinters
        a = math.radians(60.0 * k)
        _seg(p, (450.0, 60.0 * math.cos(a), 270.0 + 60.0 * math.sin(a)), (520.0 + 30 * (k % 2), 70.0 * math.cos(a), 270.0 + 70.0 * math.sin(a)), 14.0, 2.0)
    _rock(p, (-260.0, 0.0, 0.0), 120.0, 7, flat=1.0)
    _rock(p, (300.0, 0.0, 0.0), 130.0, 8, flat=1.0)
    for x in (-100.0, 60.0, 220.0):
        p.box((x - 30.0, 60.0, 250.0), (x + 30.0, 95.0, 262.0), mat="Glow")
    p.chull([(x, y, z) for x in (-470.0, 470.0) for y in (-85.0, 85.0) for z in (160.0, 340.0)])
    p.chull([(x, y, z) for x in (-380.0, -140.0) for y in (-120.0, 120.0) for z in (0.0, 170.0)])
    p.chull([(x, y, z) for x in (170.0, 430.0) for y in (-130.0, 130.0) for z in (0.0, 180.0)])
    return p


def v_stone_ring():
    """A stone ring, 7 m across: nine standing stones of uneven height round a low flat centre stone, faint teal glyphs."""
    p = Piece("SM_Site_Verdance_StoneRing", None)
    rnd = random.Random(21)
    for k in range(9):
        a = 2 * math.pi * k / 9
        h = rnd.uniform(140.0, 260.0)
        with p.at(Matrix.Translation((330.0 * math.cos(a), 330.0 * math.sin(a), 0.0)) @ Matrix.Rotation(a + rnd.uniform(-0.2, 0.2), 4, "Z")
                  @ Matrix.Rotation(math.radians(rnd.uniform(-6, 6)), 4, "Y")):
            p.prism([(-30.0, -45.0), (30.0, -40.0), (36.0, 40.0), (-28.0, 46.0)], "xy", 0.0, h, regs="wall", cap_reg="sw_dark", mat="Stone", fitu=True)
            p.box((-31.0, -14.0, h * 0.5), (-29.0, 14.0, h * 0.7), mat="Glow")
    p.prism(_oct(110.0), "xy", 0.0, 30.0, regs="wall", cap_reg="hatch", mat="Stone", fitu=True)
    for k in range(9):
        a = 2 * math.pi * k / 9
        p.cbox((330.0 * math.cos(a) - 40, 330.0 * math.sin(a) - 40, 0), (330.0 * math.cos(a) + 40, 330.0 * math.sin(a) + 40, 200))
    p.cbox((-110, -110, 0), (110, 110, 30))
    return p


def v_overgrown_pylon():
    """An overgrown pylon, 7 m: an old survey pylon (kit trim) leaning 8 degrees, roots climbing it, a dead lamp head,
    teal veins in the roots."""
    p = Piece("SM_Site_Verdance_OvergrownPylon", None)
    with p.at(Matrix.Rotation(math.radians(8.0), 4, "Y")):
        p.box((-60.0, -60.0, 0.0), (60.0, 60.0, 40.0), reg="sw_dark")
        p.prism(_oct(32.0), "xy", 40.0, 650.0, regs="pillar", cap_reg="frame", fitu=True)
        p.box((-50.0, -20.0, 620.0), (60.0, 20.0, 680.0), regs={"X": "console"}, reg="frame")
        p.box((60.0, -12.0, 640.0), (62.0, 12.0, 660.0), reg="sw_dark")
        rnd = random.Random(5)
        for k in range(4):
            a = math.pi / 2 * k + 0.4
            pts = [(70.0 * math.cos(a) + 60.0 * math.cos(a), 70.0 * math.sin(a), -10.0)]
            for j in range(1, 6):
                aa = a + j * 0.5
                pts.append((34.0 * math.cos(aa), 34.0 * math.sin(aa), j * 90.0 + rnd.uniform(-15, 15)))
            _limb(p, pts, 16.0, 6.0)
            _seg(p, Vector(pts[1]) + Vector((0, 0, 4)), Vector(pts[3]) + Vector((0, 0, 4)), 3.0, 2.5, mat="Glow", reg=None)
    _rock(p, (0.0, 0.0, 0.0), 90.0, 11, flat=0.35)
    p.chull([(x, y, z) for x in (-70.0, 70.0) for y in (-70.0, 70.0) for z in (0.0, 60.0)] + [(x + 90.0, y, 680.0) for x in (-40.0, 40.0) for y in (-40.0, 40.0)])
    return p


# ------------------------------------------------------------------------------------------------ Cinder
def c_basalt_arch():
    """A basalt arch, 6 m span, 4.5 m high: two clusters of hexagonal basalt columns and a broken lintel of fallen
    columns across them, ember in the cracks at the feet. Walk-under."""
    p = Piece("SM_Site_Cinder_BasaltArch", None)
    rnd = random.Random(31)
    hexa = [(math.cos(math.radians(60 * k)), math.sin(math.radians(60 * k))) for k in range(6)]
    for sx in (-1.0, 1.0):
        for k in range(7):
            cx = sx * (300.0 + rnd.uniform(-60, 60))
            cy = rnd.uniform(-90, 90)
            r = rnd.uniform(35, 55)
            h = rnd.uniform(300, 470) if k < 4 else rnd.uniform(120, 220)
            p.prism([(cx + r * u, cy + r * v) for (u, v) in hexa], "xy", 0.0, h, regs="wall", cap_reg="hatch", mat="Stone", fitu=True)
    for k in range(3):                                                 # the lintel: three columns lying across
        y = -60.0 + k * 55.0
        with p.at(Matrix.Translation((-360.0, y, 430.0 + k * 18.0)) @ Matrix.Rotation(math.radians(rnd.uniform(-4, 4)), 4, "Y")):
            p.prism([(0.0 + 40 * u, 40 * v) for (u, v) in hexa], "yz", 0.0, 720.0, regs="wall", cap_reg="hatch", mat="Stone", fitu=True) if False else \
                p.prism([(40 * u, 40 * v) for (u, v) in hexa], "yz", 0.0, 720.0, regs="wall", cap_reg="hatch", mat="Stone", fitu=True)
    for sx in (-1.0, 1.0):
        p.box((sx * 300.0 - 80.0, -100.0, 0.0), (sx * 300.0 + 80.0, 100.0, 3.0), mat="Ember")
    p.chull([(x, y, z) for x in (-380.0, -220.0) for y in (-150.0, 150.0) for z in (0.0, 470.0)])
    p.chull([(x, y, z) for x in (220.0, 380.0) for y in (-150.0, 150.0) for z in (0.0, 470.0)])
    p.chull([(x, y, z) for x in (-360.0, 360.0) for y in (-100.0, 100.0) for z in (390.0, 520.0)])
    return p


def c_collapsed_gantry():
    """A collapsed gantry, 10 m: a mining gantry truss fallen at an angle, one end on the ground, one on its broken
    leg 3 m up, hazard bands, a dead lamp. Walk-under at the high end."""
    p = Piece("SM_Site_Cinder_CollapsedGantry", None)
    with p.at(Matrix.Translation((-500.0, 0.0, 30.0)) @ Matrix.Rotation(math.radians(-17.0), 4, "Y")):
        for y in (-80.0, 80.0):
            for z in (0.0, 120.0):
                p.box((0.0, y - 8, z - 8), (1000.0, y + 8, z + 8), reg="pillar")
        for x in range(0, 1001, 125):
            for y in (-80.0, 80.0):
                p.box((x - 5.0, y - 5, 0.0), (x + 5.0, y + 5, 120.0), reg="frame")
            p.box((x - 5.0, -80.0, 115.0), (x + 5.0, 80.0, 125.0), reg="hazard" if (x // 125) % 2 else "frame")
        p.box((900.0, -60.0, 120.0), (960.0, 60.0, 160.0), reg="sw_dark")
    p.box((390.0, -100.0, 0.0), (430.0, 100.0, 330.0), reg="pillar")              # the broken leg
    p.prism([(370.0, 0.0), (450.0, 0.0), (430.0, 40.0), (390.0, 40.0)], "xz", -110.0, 110.0, regs="sw_dark", cap_reg="sw_dark", fitu=True)
    p.chull([(x, y, z) for (x, z) in ((-500.0, 0.0), (-500.0, 150.0), (480.0, 300.0), (480.0, 450.0)) for y in (-90.0, 90.0)])
    p.cbox((380, -110, 0), (440, 110, 330))
    return p


def c_vent_chimney():
    """A volcanic vent chimney, 5 m: a tapering stack of basalt rings with a cracked lip, ember glowing inside the top
    and in two side fissures."""
    p = Piece("SM_Site_Cinder_VentChimney", None)
    prof = [(0.0, 0.0), (190.0, 0.0), (160.0, 120.0), (130.0, 260.0), (105.0, 400.0), (95.0, 480.0), (60.0, 470.0), (40.0, 380.0)]
    p.lathe(prof, 10, regs=["sw_dark", "wall", "wall", "wall", "hatch", "sw_dark", "sw_dark"], mat="Stone", reps=2)
    p.lathe([(40.0, 380.0), (0.0, 375.0)], 10, regs="sw_dark", mat="Ember")
    for a in (40.0, 220.0):
        with p.at(Matrix.Rotation(math.radians(a), 4, "Z")):
            p.box((150.0, -10.0, 60.0), (172.0, 10.0, 230.0), mat="Ember")
    _rock(p, (180.0, 120.0, 0.0), 70.0, 41, flat=0.5)
    p.chull([(170.0 * math.cos(2 * math.pi * k / 8), 170.0 * math.sin(2 * math.pi * k / 8), 0.0) for k in range(8)] +
            [(95.0 * math.cos(2 * math.pi * k / 8), 95.0 * math.sin(2 * math.pi * k / 8), 480.0) for k in range(8)])
    return p


def c_slag_heap():
    """A slag heap, 8 x 6 m, 2.5 m high: a lumpy mound of tipped slag with a few glowing clinkers and a dark crust."""
    p = Piece("SM_Site_Cinder_SlagHeap", None)
    rnd = random.Random(51)
    for k, (x, y, r) in enumerate(((0.0, 0.0, 280.0), (-220.0, 80.0, 200.0), (230.0, -60.0, 210.0), (60.0, 180.0, 160.0), (-120.0, -170.0, 150.0))):
        _rock(p, (x, y, 0.0), r, 60 + k, flat=0.55, reg="sw_dark")
    for k in range(7):
        x, y = rnd.uniform(-300, 300), rnd.uniform(-200, 200)
        p.box((x - 12, y - 12, 0.0), (x + 12, y + 12, 18.0 + rnd.uniform(0, 30)), mat="Ember")
    p.chull([(x, y, 0.0) for x in (-420.0, 420.0) for y in (-300.0, 300.0)] + [(x, y, 240.0) for x in (-120.0, 120.0) for y in (-80.0, 80.0)])
    return p


# ------------------------------------------------------------------------------------------------ camp perimeter
def _wall_low(name):
    """A low perimeter wall segment, 4 m x 1.2 m: a dark footing, panel blocks with a frame cap, a glow strip."""
    p = Piece(name, None)
    p.box((-200.0, -30.0, 0.0), (200.0, 30.0, 20.0), reg="sw_dark")
    for x0 in (-200.0, -100.0, 0.0, 100.0):
        p.box((x0 + 3, -22.0, 20.0), (x0 + 97, 22.0, 110.0), regs={"y": "wall", "Y": "wall", "Z": "frame"}, reg="frame")
    p.box((-200.0, -26.0, 110.0), (200.0, 26.0, 122.0), reg="frame")
    p.box((-190.0, 22.0, 60.0), (190.0, 24.0, 64.0), mat="Glow")
    p.cbox((-200, -30, 0), (200, 30, 122))
    return p


def _gate(name):
    """A camp gate, 5 m: two posts with hazard bands and lamps, a crossbar with a sign plate, an open 3.6 m passage."""
    p = Piece(name, None)
    for y in (-230.0, 230.0):
        p.box((-30.0, y - 30, 0.0), (30.0, y + 30, 380.0), regs={"x": "pillar", "X": "pillar"}, reg="frame")
        p.box((-32.0, y - 32, 0.0), (32.0, y + 32, 60.0), reg="hazard")
        p.box((30.0, y - 10, 340.0), (36.0, y + 10, 360.0), mat="Light")
    p.box((-25.0, -260.0, 380.0), (25.0, 260.0, 430.0), reg="frame")
    p.box((26.0, -120.0, 388.0), (30.0, 120.0, 424.0), regs={"X": "console"}, reg="frame")
    p.box((30.0, -110.0, 400.0), (31.0, 110.0, 404.0), mat="Glow")
    for y in (-230.0, 230.0):
        p.cbox((-30, y - 30, 0), (30, y + 30, 380))
    p.cbox((-25, -260, 380), (25, 260, 430))
    return p


def _lamp_post(name):
    """A camp lamp post, 3.2 m: a weighted base, a post, a hooded warm lamp."""
    p = Piece(name, None)
    p.box((-30.0, -30.0, 0.0), (30.0, 30.0, 15.0), reg="hazard")
    p.box((-6.0, -6.0, 15.0), (6.0, 6.0, 300.0), reg="pillar")
    p.box((-6.0, -6.0, 290.0), (50.0, 6.0, 300.0), reg="frame")
    p.box((30.0, -18.0, 300.0), (62.0, 18.0, 318.0), reg="sw_dark")
    p.box((32.0, -14.0, 296.0), (60.0, 14.0, 300.0), mat="Light")
    p.cbox((-30, -30, 0), (30, 30, 318))
    return p


def _light_strip(name):
    """A landing-light strip, 4 m: a low rail with five recessed lamps, flush with the ground (no collision needed:
    a 6 cm step)."""
    p = Piece(name, None)
    p.box((-200.0, -15.0, 0.0), (200.0, 15.0, 6.0), regs={"Z": "frame"}, reg="sw_dark")
    for x in (-160.0, -80.0, 0.0, 80.0, 160.0):
        p.box((x - 10, -8.0, 6.0), (x + 10, 8.0, 8.0), mat="Beacon")
    return p


def perimeter(planet):
    return [lambda pl=planet: _wall_low("SM_Hut_%s_WallLow" % pl), lambda pl=planet: _gate("SM_Hut_%s_Gate" % pl),
            lambda pl=planet: _lamp_post("SM_Hut_%s_LampPost" % pl), lambda pl=planet: _light_strip("SM_Hut_%s_LightStrip" % pl)]


GROUND = [v_root_arch, v_fallen_trunk, v_stone_ring, v_overgrown_pylon, c_basalt_arch, c_collapsed_gantry, c_vent_chimney, c_slag_heap]
PERIMETER = perimeter("Verdance") + perimeter("Cinder")
NO_COLLISION = ("SM_Hut_Verdance_LightStrip", "SM_Hut_Cinder_LightStrip")

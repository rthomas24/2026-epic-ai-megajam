# SPDX-License-Identifier: MIT
# Kit, Dominion set - the walkable shell of the Dominion interior rebuilt with the kit's construction (1.5 cm bevels,
# authored collision whose tops equal the visual tops, one trim sheet: the Dominion palette T_KitD_Trim_*).
# Each piece keeps the NAME, size, pivot and opening of the DominionInterior piece it replaces (ArtSource/EngProps/
# DominionInterior/README.md), so T5's course (TDDominionInteriorCourse.inl) places them unchanged: they import to
# /Game/TriadDrift/Gameplay/Props/Kit/Dominion/ and the course switches by its folder string only.
# Slots: Trim (MI_KitD_Trim), Glow (red strips, MI_KitD_Glow), Light (white work lights, MI_KitD_Light).
from kit_lib import Piece

H = 600.0          # one storey


def _wallface_uv(z0, z1):
    """Wall faces tile the 300 cm wall trim twice per storey: V on the half-storey band the part sits in."""
    return ("z", z0, z1)


def _stacked(p, lo, hi, faces_wall, other_regs, ends="sw_gun"):
    """A wall slab from lo to hi, split at Z 300 so each half maps the whole 300 cm wall trim; faces_wall: which faces
    show the wall trim, e.g. 'xX' or 'y'."""
    for (z0, z1) in ((lo[2], min(hi[2], 300.0)), (max(lo[2], 300.0), hi[2])):
        if z1 - z0 < 0.5:
            continue
        regs = dict(other_regs)
        uvo = {}
        for f in faces_wall:
            regs[f] = "wall" if z0 < 1 else "wallB"
            axis = "y" if f in "xX" else "x"
            c0, c1 = (lo[1], hi[1]) if axis == "y" else (lo[0], hi[0])
            uvo[f] = ((axis, c0, c1), ("z", 0.0, 300.0) if z0 < 1 else ("z", 300.0, 600.0))
        p.box((lo[0], lo[1], z0), (hi[0], hi[1], z1), reg=ends, regs=regs, uvo=uvo)


def room_floor():
    p = Piece("SM_DI_Room_Floor", ((-200, -200, -20), (200, 200, 0)))
    p.box((-200, -200, -20), (200, 200, 0), regs={"Z": "floor", "z": "sw_dark"}, reg="kick",
          uvo={f: (("y" if f in "xX" else "x", -200, 200), ("z", -20, 0)) for f in "xXyY"})
    p.cbox((-200, -200, -20), (200, 200, 0))
    return p


def _ceiling(p, z0=600.0):
    for x0 in (-200.0, 0.0):
        for y0 in (-200.0, 0.0):
            p.box((x0, y0, z0), (x0 + 200, y0 + 200, z0 + 20), regs={"z": "roof", "Z": "sw_dark"}, reg="frame",
                  uvo={"z": (("x", x0, x0 + 200), ("y", y0, y0 + 200))})


def room_ceiling():
    """Place at the SAME transform as the floor below (pivot at floor level); underside at 600, one white work light."""
    p = Piece("SM_DI_Room_Ceiling", ((-200, -200, 565), (200, 200, 620)))
    _ceiling(p)
    p.box((-20, -200, 565), (20, 200, 600), reg="frame")              # transverse beam (review fix 2)
    p.box((-190, -6, 596), (-24, 6, 600), mat="Light", faces="xXyYz")
    p.box((24, -6, 596), (190, 6, 600), mat="Light", faces="xXyYz")
    p.cbox((-200, -200, 600), (200, 200, 620))
    return p


def _rib(p, s, y0, y1, z0=60.0, z1=560.0, depth=30.0):
    """A chamfered vertical rib on the face X = s*30, from y0 to y1 (a half rib when it stops at a seam)."""
    c = min(8.0, (y1 - y0) * 0.25)
    prof = [(s * (30.0 + dx), yy) for (dx, yy) in ((0.0, y0), (depth - c, y0), (depth, y0 + c), (depth, y1 - c), (depth - c, y1), (0.0, y1))]
    p.prism(prof, "xy", z0, z1, regs="frame", cap_reg="frame", fitu=True)
    p.cbox((min(s * 30, s * (30 + depth)), y0, z0), (max(s * 30, s * (30 + depth)), y1, z1))


def room_wall():
    """60 thick core, centred on its plane (X +-30), 400 along Y, one storey, the same on both faces. Brutal ribbed
    (review r1 fix 2): a 40 cm chamfered rib 30 deep at the centre and half ribs at both seams (a full rib where two
    walls meet), a sloped 60 cm base plinth and an angled cornice, all gunmetal; ONE red strip per storey at 236..244
    (review fix 1: the 536 band is gone). Ribs and plinth block the pawn (their own hulls)."""
    p = Piece("SM_DI_Room_Wall", ((-60, -200, 0), (60, 200, 600)))
    _stacked(p, (-30, -200, 0), (30, 200, 600), "xX", {"Z": "frame", "z": "sw_dark"})
    for s in (-1.0, 1.0):
        prof = [(s * 30.0, 0.0), (s * 55.0, 0.0), (s * 55.0, 15.0), (s * 30.0, 60.0)]
        p.prism(prof, "xz", -200.0, 200.0, regs=["sw_dark", "kick", "kick", "kick"], cap_reg="kick", fitu=True)
        p.chull([(x, y, z) for y in (-200.0, 200.0) for (x, z) in prof])
        prof = [(s * 30.0, 540.0), (s * 30.0, 600.0), (s * 52.0, 600.0)]
        p.prism(prof, "xz", -200.0, 200.0, regs="frame", cap_reg="frame", fitu=True)
        _rib(p, s, -20.0, 20.0)
        _rib(p, s, -200.0, -180.0)
        _rib(p, s, 180.0, 200.0)
        xa2, xb2 = sorted((s * 30, s * 33))
        p.box((xa2, -170, 236.0), (xb2, -28, 244.0), mat="Glow")
        p.box((xa2, 28, 236.0), (xb2, 170, 244.0), mat="Glow")
    p.cbox((-30, -200, 0), (30, 200, 600))
    return p


def corridor_straight():
    """400 cell open along X: floor -20..0, ceiling 600..620, inner wall faces at Y +-170 (walls 30 thick inside the
    footprint), a portal rib at X 0 (to +-156, crossing under the ceiling at 586), red strips, a white work light."""
    p = Piece("SM_DI_Corridor_Straight", ((-200, -200, -20), (200, 200, 620)))
    p.box((-200, -200, -20), (200, 200, 0), regs={"Z": "floor", "z": "sw_dark"}, reg="kick",
          uvo={f: (("y" if f in "xX" else "x", -200, 200), ("z", -20, 0)) for f in "xXyY"})
    for s in (-1.0, 1.0):
        ya, yb = sorted((s * 170, s * 200))
        inner = "y" if s > 0 else "Y"
        _stacked(p, (-200, ya, 0), (200, yb, 600), inner, {"Z": "frame", "z": "sw_dark"})
        ra, rb = sorted((s * 140, s * 170))
        prof = [(-20.0, s * 170.0), (-20.0, s * 148.0), (-12.0, s * 140.0), (12.0, s * 140.0), (20.0, s * 148.0), (20.0, s * 170.0)]
        p.prism(prof, "xy", 0.0, 600.0, regs="frame", cap_reg="frame", fitu=True)
        p.cbox((-20, ra, 0), (20, rb, 600))
        ka, kb = sorted((s * 166, s * 170))
        p.box((-200, ka, 0), (-12, kb, 40), reg="kick", faces="yYZ", uvo={f: (("x", -200, -12), ("z", 0, 40)) for f in "yY"})
        p.box((12, ka, 0), (200, kb, 40), reg="kick", faces="yYZ", uvo={f: (("x", 12, 200), ("z", 0, 40)) for f in "yY"})
        ga, gb = sorted((s * 167, s * 170))
        p.box((-190, ga, 236), (-16, gb, 244), mat="Glow")
        p.box((16, ga, 236), (190, gb, 244), mat="Glow")
    p.box((-20, -140, 560), (20, 140, 600), reg="frame")              # transverse beam on the portal
    _ceiling(p)
    p.box((-190, -6, 596), (-16, 6, 600), mat="Light", faces="xXyYz")
    p.box((16, -6, 596), (190, 6, 600), mat="Light", faces="xXyYz")
    # Grok Bot's route arrow (ROUTE_ARROW cell, 200 cm, chevrons pointing +X = deeper into the ship), 2 mm proud
    p.poly([(-100.0, -100.0, 0.2), (100.0, -100.0, 0.2), (100.0, 100.0, 0.2), (-100.0, 100.0, 0.2)], mat="Route", n=(0, 0, 1),
           uv=[(0.5, 0.0), (1.0, 0.0), (1.0, 0.5), (0.5, 0.5)])
    p.cbox((-200, -200, -20), (200, 200, 0))
    p.cbox((-200, -200, 600), (200, 200, 620))
    p.cbox((-200, 170, 0), (200, 200, 600))
    p.cbox((-200, -200, 0), (200, -170, 600))
    return p


def _eye(p, s, zc, r_lens=35.0, r_bezel=45.0, yc=0.0, x_face=30.0, ring=True, lens_mat="Light"):
    """A round lens on the face X = s*30, centred (Y 0, Z zc): a black bezel ring 4 cm proud, the white lens 2 cm proud."""
    import math
    n = 24
    x0 = s * x_face
    f1, f2 = x_face + 4.0, x_face + 2.0
    r_ring = r_lens + 0.35 * (r_bezel - r_lens)
    for i in range(n):
        a0, a1 = 2 * math.pi * i / n, 2 * math.pi * (i + 1) / n
        c0, s0, c1, s1 = math.cos(a0), math.sin(a0), math.cos(a1), math.sin(a1)
        Y = lambda r, c: yc + r * c
        p.poly([(s * f1, Y(r_ring, c0), zc + r_ring * s0), (s * f1, Y(r_bezel, c0), zc + r_bezel * s0),
                (s * f1, Y(r_bezel, c1), zc + r_bezel * s1), (s * f1, Y(r_ring, c1), zc + r_ring * s1)], "sw_black", n=(s, 0, 0),
               uv=[(0.5, 0.5)] * 4)
        p.poly([(s * f1, Y(r_lens, c0), zc + r_lens * s0), (s * f1, Y(r_ring, c0), zc + r_ring * s0),
                (s * f1, Y(r_ring, c1), zc + r_ring * s1), (s * f1, Y(r_lens, c1), zc + r_lens * s1)],
               "sw_black" if not ring else None, mat="Trim" if not ring else "Glow", n=(s, 0, 0), uv=[(0.5, 0.5)] * 4)
        p.poly([(x0, Y(r_bezel, c0), zc + r_bezel * s0), (s * f1, Y(r_bezel, c0), zc + r_bezel * s0),
                (s * f1, Y(r_bezel, c1), zc + r_bezel * s1), (x0, Y(r_bezel, c1), zc + r_bezel * s1)], "sw_black",
               n=(0, (c0 + c1) * 0.5, (s0 + s1) * 0.5), uv=[(0.5, 0.5)] * 4)
        p.poly([(s * f2, yc, zc), (s * f2, Y(r_lens, c0), zc + r_lens * s0), (s * f2, Y(r_lens, c1), zc + r_lens * s1)],
               mat=lens_mat, n=(s, 0, 0))


def doorway():
    """60 deep frame on the seam between two cells, 400 x 600; clear opening exactly 300 x 400 (Y +-150, Z 0..400)."""
    p = Piece("SM_DI_Doorway", ((-90, -200, 0), (90, 200, 600)))
    for s in (-1.0, 1.0):
        ya, yb = sorted((s * 150, s * 200))
        inner = "y" if s > 0 else "Y"
        p.box((-30, ya, 0), (30, yb, 600), reg="frame", regs={inner: "hazard"},
              uvo={inner: (("z", 0, 600), ("x", -30, 30)), "x": (("z", 0, 600), ("y", ya, yb)), "X": (("z", 0, 600), ("y", ya, yb))})
    p.box((-30, -150, 400), (30, 150, 600), reg="frame", regs={"z": "hazard"},
          uvo={"z": (("y", -150, 150), ("x", -30, 30)), "x": (("y", -150, 150), ("z", 400, 600)), "X": (("y", -150, 150), ("z", 400, 600))})
    for s in (-1.0, 1.0):
        xa, xb = sorted((s * 30, s * 32))
        p.box((xa, -140, 408), (xb, 140, 416), mat="Glow")
        p.box((xa, -192, 520), (xb, -184, 560), mat="Glow")         # 40 cm caps (review fix 1)
        p.box((xa, 184, 520), (xb, 192, 560), mat="Glow")
        # r9 (review r4): a 1.7 m black eye with a 44 cm red pupil instead of the white disc (read as a debug lamp)
        _eye(p, s, 500.0, r_lens=22.0, r_bezel=84.0, ring=False, lens_mat="Glow")
    # batch X (T7's play: a walker in the 3.4 m corridor caught the 20 cm lip of the jamb beside the 3 m opening):
    # a 60 cm lead-in chamfer on each jamb, both faces, from the corridor wall line (|y| 170) to the opening (|y| 150),
    # so a capsule sliding along the wall is turned into the opening instead of stopping on the frame edge
    for sx in (-1.0, 1.0):
        for sy in (-1.0, 1.0):
            prof = [(sx * 30.0, sy * 150.0), (sx * 30.0, sy * 170.0), (sx * 90.0, sy * 170.0)]
            if sx * sy < 0:
                prof = prof[::-1]
            p.prism(prof, "xy", 0.0, 400.0, regs="hazard", cap_reg="frame", fitu=True)
            p.chull([(x, y, z) for (x, y) in prof for z in (0.0, 400.0)])
    p.cbox((-30, -200, 0), (30, -150, 600))
    p.cbox((-30, 150, 0), (30, 200, 600))
    p.cbox((-30, -150, 400), (30, 150, 600))
    return p


def door():
    """The door leaf at the doorway's transform: 30 x 300 x 400. Gameplay slides it +Z by 400."""
    p = Piece("SM_DI_Door", ((-16.3, -150, 0), (16.3, 150, 400)))
    p.box((-15, -150, 0), (15, 150, 400), reg="frame", regs={"x": "wallB", "X": "wallB"},
          uvo={f: (("y", -150, 150), ("z", 0, 400)) for f in "xX"})
    for s in (-1.0, 1.0):
        xa, xb = sorted((s * 15, s * 16))
        p.box((xa, -3, 20), (xb, 3, 380), mat="Glow")
        p.box((xa, -140, 186), (xb, 140, 192), mat="Glow")
        # Grok Bot's trident emblem (T_DI_Decals, the TRIDENT cell), 160 cm, upright for a viewer on this side
        x = s * 16.3
        yl, yr = (-130.0, 130.0) if s > 0 else (130.0, -130.0)          # r8: 260 cm (review r3: read the mark)
        p.poly([(x, yl, 115.0), (x, yr, 115.0), (x, yr, 375.0), (x, yl, 375.0)], mat="Decal", n=(s, 0, 0),
               uv=[(0.0, 0.5), (0.5, 0.5), (0.5, 1.0), (0.0, 1.0)])
    p.cbox((-15, -150, 0), (15, 150, 400))
    return p


def cover_low():
    """60 deep, 200 long (Y), 110 high: the troopers' cover."""
    p = Piece("SM_DI_Cover_Low", ((-30.8, -100, 0), (30.8, 100, 110)))
    prof = [(-30.0, 8.0), (30.0, 8.0), (30.0, 88.0), (18.0, 110.0), (-18.0, 110.0), (-30.0, 88.0)]
    p.prism(prof, "xz", -100.0, 100.0, regs=["sw_dark", "pillar", "frame", "frame", "frame", "pillar"], cap_reg="frame", fitu=True,
            vr=[None, (0.0, 0.88), None, None, None, (0.0, 0.88)])
    p.box((-28, -98, 0), (28, 98, 8), reg="kick")
    for s in (-1.0, 1.0):
        xa, xb = sorted((s * 30, s * 30.8))
        p.box((xa, -90, 64), (xb, 90, 70), mat="Glow")
    p.cbox((-30, -100, 0), (30, 100, 110))
    return p


def hangar_pad():
    """12 m landing pad, top at 0 (collision exactly 0), a painted hazard border 0.3 cm proud (no emissive: review fix 1)."""
    p = Piece("SM_DI_Hangar_Pad", ((-600, -600, -40), (600, 600, 0.3)))
    for x0 in (-600.0, -200.0, 200.0):
        for y0 in (-600.0, -200.0, 200.0):
            p.box((x0, y0, -40), (x0 + 400, y0 + 400, 0), regs={"Z": "floor", "z": "sw_dark"}, reg="kick",
                  faces="xXyYzZ", uvo={"Z": (("x", x0, x0 + 400), ("y", y0, y0 + 400)),
                                        **{f: (("y" if f in "xX" else "x", -200, 200), ("z", -40, 0)) for f in "xXyY"}})
    for (lo, hi) in (((-572, -572, 0), (572, -560, 0.3)), ((-572, 560, 0), (572, 572, 0.3)),
                     ((-572, -560, 0), (-560, 560, 0.3)), ((560, -560, 0), (572, 560, 0.3))):
        p.box(lo, hi, reg="hazard", faces="xXyYZ")
    p.cbox((-600, -600, -40), (600, 600, 0))
    return p


BUILDERS = [room_floor, room_ceiling, room_wall, corridor_straight, doorway, door, cover_low, hangar_pad]
NO_COLLISION = ()
NO_CLAMP = ("SM_DI_Corridor_Straight",)    # bevel without the global clamp (see kit_lib.finish)

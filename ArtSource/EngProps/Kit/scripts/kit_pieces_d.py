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
    p = Piece("SM_DI_Room_Ceiling", ((-200, -200, 596), (200, 200, 620)))
    _ceiling(p)
    p.box((-190, -6, 596), (190, 6, 600), mat="Light", faces="xXyYz")
    p.cbox((-200, -200, 600), (200, 200, 620))
    return p


def room_wall():
    """60 thick, centred on its plane (X +-30), 400 along Y, one storey; the same on both faces."""
    p = Piece("SM_DI_Room_Wall", ((-30, -200, 0), (30, 200, 600)))
    _stacked(p, (-24, -200, 0), (24, 200, 600), "xX", {"Z": "frame", "z": "sw_dark"})
    for s in (-1.0, 1.0):
        xa, xb = sorted((s * 24, s * 30))
        p.box((xa, -200, 0), (xb, 200, 40), reg="kick", faces="xXZ", uvo={f: (("y", -200, 200), ("z", 0, 40)) for f in "xX"})
        p.box((xa, -14, 40), (xb, 14, 600), reg="frame", uvo={f: (("z", 40, 600), ("y", -14, 14)) for f in "xX"})   # centre rib
        for (z0, z1) in ((236.0, 244.0), (536.0, 544.0)):
            xa2, xb2 = sorted((s * 24, s * 27))
            p.box((xa2, -190, z0), (xb2, -16, z1), mat="Glow")
            p.box((xa2, 16, z0), (xb2, 190, z1), mat="Glow")
    p.box((-30, -200, 588), (30, 200, 600), reg="frame")
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
        ra, rb = sorted((s * 156, s * 170))
        p.box((-12, ra, 0), (12, rb, 600), reg="frame", uvo={f: (("z", 0, 600), ("x", -12, 12)) for f in "yY"})
        ka, kb = sorted((s * 166, s * 170))
        p.box((-200, ka, 0), (-12, kb, 40), reg="kick", faces="yYZ", uvo={f: (("x", -200, -12), ("z", 0, 40)) for f in "yY"})
        p.box((12, ka, 0), (200, kb, 40), reg="kick", faces="yYZ", uvo={f: (("x", 12, 200), ("z", 0, 40)) for f in "yY"})
        ga, gb = sorted((s * 167, s * 170))
        p.box((-190, ga, 236), (-16, gb, 244), mat="Glow")
        p.box((16, ga, 236), (190, gb, 244), mat="Glow")
    p.box((-12, -156, 586), (12, 156, 600), reg="frame")
    _ceiling(p)
    p.box((-190, -6, 596), (-16, 6, 600), mat="Light", faces="xXyYz")
    p.box((16, -6, 596), (190, 6, 600), mat="Light", faces="xXyYz")
    p.cbox((-200, -200, -20), (200, 200, 0))
    p.cbox((-200, -200, 600), (200, 200, 620))
    p.cbox((-200, 170, 0), (200, 200, 600))
    p.cbox((-200, -200, 0), (200, -170, 600))
    return p


def doorway():
    """60 deep frame on the seam between two cells, 400 x 600; clear opening exactly 300 x 400 (Y +-150, Z 0..400)."""
    p = Piece("SM_DI_Doorway", ((-32, -200, 0), (32, 200, 600)))
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
        p.box((xa, -192, 40), (xb, -184, 560), mat="Glow")
        p.box((xa, 184, 40), (xb, 192, 560), mat="Glow")
    p.cbox((-30, -200, 0), (30, -150, 600))
    p.cbox((-30, 150, 0), (30, 200, 600))
    p.cbox((-30, -150, 400), (30, 150, 600))
    return p


def door():
    """The door leaf at the doorway's transform: 30 x 300 x 400. Gameplay slides it +Z by 400."""
    p = Piece("SM_DI_Door", ((-16, -150, 0), (16, 150, 400)))
    p.box((-15, -150, 0), (15, 150, 400), reg="frame", regs={"x": "wallB", "X": "wallB"},
          uvo={f: (("y", -150, 150), ("z", 0, 400)) for f in "xX"})
    for s in (-1.0, 1.0):
        xa, xb = sorted((s * 15, s * 16))
        p.box((xa, -3, 20), (xb, 3, 380), mat="Glow")
        p.box((xa, -140, 186), (xb, 140, 192), mat="Glow")
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
    """12 m landing pad, top at 0 (collision exactly 0), red edge lights flush 0.3 cm proud."""
    p = Piece("SM_DI_Hangar_Pad", ((-600, -600, -40), (600, 600, 0.3)))
    for x0 in (-600.0, -200.0, 200.0):
        for y0 in (-600.0, -200.0, 200.0):
            p.box((x0, y0, -40), (x0 + 400, y0 + 400, 0), regs={"Z": "floor", "z": "sw_dark"}, reg="kick",
                  faces="xXyYzZ", uvo={"Z": (("x", x0, x0 + 400), ("y", y0, y0 + 400)),
                                        **{f: (("y" if f in "xX" else "x", -200, 200), ("z", -40, 0)) for f in "xXyY"}})
    for (lo, hi) in (((-572, -572, 0), (572, -560, 0.3)), ((-572, 560, 0), (572, 572, 0.3)),
                     ((-572, -560, 0), (-560, 560, 0.3)), ((560, -560, 0), (572, 560, 0.3))):
        p.box(lo, hi, mat="Glow", faces="xXyYZ")
    p.cbox((-600, -600, -40), (600, 600, 0))
    return p


BUILDERS = [room_floor, room_ceiling, room_wall, corridor_straight, doorway, door, cover_low, hangar_pad]
NO_COLLISION = ()
NO_CLAMP = ("SM_DI_Corridor_Straight",)    # bevel without the global clamp (see kit_lib.finish)

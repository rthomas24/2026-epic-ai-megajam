# SPDX-License-Identifier: MIT
# Kit (planet outposts) - the pieces. Unreal centimetres, +X forward, +Z up, pivot on the walkable plane (Z = 0) at
# the centre of the footprint, every size a multiple of the 50 cm grid. Slots: Trim (the shared sheet), Glow (light
# strips: teal on Verdance, ember on Cinder), Screen (console glass).
# Collision is authored, never generated: boxes / wedges whose walkable tops equal the visual tops exactly.
import math

from kit_lib import Piece

WALL_H, WALL_T = 300.0, 20.0
RAIL_H = 110.0
TAN_RAMP = 100.0 / 400.0          # 14.04 degrees: 100 cm rise over a 400 cm run


def _uv_wall(z0=0.0, z1=WALL_H):
    """Faces x / X of a wall part: U along Y on the 400 grid, V on the absolute wall height (so split parts line up)."""
    return {"x": (("y", -200.0, 200.0), ("z", z0, z1)), "X": (("y", -200.0, 200.0), ("z", z0, z1))}


def _wall_part(p, lo, hi, front="wall", back="wallB", ends="sw_gun", top="frame"):
    p.box(lo, hi, regs={"x": front, "X": back, "y": ends, "Y": ends, "Z": top, "z": "sw_dark"}, uvo=_uv_wall())


def _wall_dress(p, y0, y1, ribs=(0.0,), strip=True):
    """Kick plates, top cap, ribs and the light strip on both faces of a wall span y0..y1."""
    for s in (-1.0, 1.0):
        xa, xb = sorted((s * WALL_T / 2, s * (WALL_T / 2 + 4)))
        p.box((xa, y0, 0.0), (xb, y1, 30.0), reg="kick", uvo={f: (("y", y0, y1), ("z", 0, 30)) for f in "xX"})
        if strip:
            xa, xb = sorted((s * WALL_T / 2, s * (WALL_T / 2 + 3)))
            p.box((xa, y0 + 10, 236.0), (xb, y1 - 10, 244.0), mat="Glow")
        for y in ribs:
            if y0 + 8 < y < y1 - 8:
                xa, xb = sorted((s * WALL_T / 2, s * (WALL_T / 2 + 5)))
                p.box((xa, y - 6, 30.0), (xb, y + 6, 290.0), reg="frame", uvo={f: (("z", 30, 290), ("y", y - 6, y + 6)) for f in "xX"})
    p.box((-WALL_T / 2 - 4, y0, 290.0), (WALL_T / 2 + 4, y1, 300.0), reg="frame")


def floor_400():
    p = Piece("SM_Kit_Floor_400", ((-200, -200, -20), (200, 200, 0)))
    p.box((-200, -200, -20), (200, 200, 0), regs={"Z": "floor", "z": "sw_dark"}, reg="kick",
          uvo={f: (("y" if f in "xX" else "x", -200, 200), ("z", -20, 0)) for f in "xXyY"})
    p.cbox((-200, -200, -20), (200, 200, 0))
    return p


def platform_400(H=100.0, name="SM_Kit_Platform_400"):
    if H < 60:
        return _platform_low(H, name)
    """A 400 x 400 riser block, top at Z = H (100, or 200 for the x200 variant). Ramps and stairs climb 100 per 400."""
    p = Piece(name, ((-200, -200, 0), (200, 200, H)))
    B = 198.0      # the body is inset 2 cm so the dark plinth band at its foot sits flush with the 400 footprint
    for x0, x1 in ((-B, 0.0), (0.0, B)):
        for y0, y1 in ((-B, 0.0), (0.0, B)):
            p.box((x0, y0, 16.0), (x1, y1, H), regs={"z": "sw_dark"}, reg="pillar", faces="xXyYz",
                  uvo={"x": (("y", y0, y1), ("z", 0, H)), "X": (("y", y0, y1), ("z", 0, H)),
                       "y": (("x", x0, x1), ("z", 0, H)), "Y": (("x", x0, x1), ("z", 0, H))})
    p.box((-200, -200, 0), (200, 200, 16), reg="kick", faces="xXyYz",
          uvo={f: (("y" if f in "xX" else "x", -200, 200), ("z", 0, 16)) for f in "xXyY"})
    p.box((-200, -200, H - 0.01), (200, 200, H), regs={"Z": "floor"}, faces="Z")
    # a proud hazard nosing along the top edges (2 cm, flush with the top: no lip to trip on)
    for (lo, hi) in (((-200, -200, H - 8), (200, -198, H)), ((-200, 198, H - 8), (200, 200, H)),
                     ((-200, -198, H - 8), (-198, 198, H)), ((198, -198, H - 8), (200, 198, H))):
        p.box(lo, hi, reg="hazard", faces="xXyY", uvo={f: (("y" if f in "xX" else "x", -200, 200), ("z", H - 8, H)) for f in "xXyY"})
    p.cbox((-200, -200, 0), (200, 200, H))
    return p


def _platform_low(H, name):
    """A low deck tile (top H, e.g. 50): floor-trim top, a dark side band, a hazard nosing flush with the top."""
    p = Piece(name, ((-200, -200, 0), (200, 200, H)))
    p.box((-200, -200, 0), (200, 200, H - 8), reg="kick", faces="xXyYz",
          uvo={f: (("y" if f in "xX" else "x", -200, 200), ("z", 0, H - 8)) for f in "xXyY"})
    p.box((-200, -200, H - 8), (200, 200, H), reg="hazard", regs={"Z": "floor"},
          uvo={**{f: (("y" if f in "xX" else "x", -200, 200), ("z", H - 8, H)) for f in "xXyY"}, "Z": (("x", -200, 200), ("y", -200, 200))})
    p.cbox((-200, -200, 0), (200, 200, H))
    return p


def platform_400x200():
    return platform_400(200.0, "SM_Kit_Platform_400x200")


def platform_400x50():
    return platform_400(50.0, "SM_Kit_Platform_400x50")


def ramp_low_400():
    """A low approach onto a 50 cm deck: 400 wide (Y), 200 run (X), rising 50 toward +X (14.04 deg), no rails."""
    p = Piece("SM_Kit_RampLow_400", ((-100, -200, 0), (100, 200, 50)))
    prof = [(-100.0, 0.0), (100.0, 0.0), (100.0, 50.0)]
    p.prism(prof, "xz", -200.0, 200.0, regs=["sw_dark", "kick", "floor"], cap_reg="pillar", fitu=True)
    p.chull([(x, y, z) for y in (-200.0, 200.0) for (x, z) in ((-100, 0), (100, 0), (100, 50))])
    return p


def wall_400():
    p = Piece("SM_Kit_Wall_400", ((-15, -200, 0), (15, 200, 300)))
    _wall_part(p, (-WALL_T / 2, -200, 0), (WALL_T / 2, 200, WALL_H))
    _wall_dress(p, -200, 200)
    p.cbox((-WALL_T / 2, -200, 0), (WALL_T / 2, 200, WALL_H))
    return p


def wall_400_window():
    p = Piece("SM_Kit_Wall_400_Window", ((-16, -200, 0), (16, 200, 300)))
    W0, W1, Z0, Z1 = -120.0, 120.0, 110.0, 230.0
    _wall_part(p, (-WALL_T / 2, -200, 0), (WALL_T / 2, 200, Z0))
    _wall_part(p, (-WALL_T / 2, -200, Z1), (WALL_T / 2, 200, WALL_H))
    _wall_part(p, (-WALL_T / 2, -200, Z0), (WALL_T / 2, W0, Z1))
    _wall_part(p, (-WALL_T / 2, W1, Z0), (WALL_T / 2, 200, Z1))
    _wall_dress(p, -200, 200, ribs=())
    # frame: sill, head, jambs (gunmetal, proud of the wall by 6 cm), a dark glass pane recessed in the middle
    p.box((-16, W0 - 8, Z0 - 8), (16, W1 + 8, Z0), reg="frame")
    p.box((-16, W0 - 8, Z1), (16, W1 + 8, Z1 + 8), reg="frame")
    p.box((-16, W0 - 8, Z0), (16, W0, Z1), reg="frame")
    p.box((-16, W1, Z0), (16, W1 + 8, Z1), reg="frame")
    p.box((-2, W0, Z0), (2, W1, Z1), reg="sw_glass", faces="xX")
    p.box((-16, -4, Z0), (16, 4, Z1), reg="frame")                       # mullion
    p.cbox((-WALL_T / 2, -200, 0), (WALL_T / 2, 200, WALL_H))           # the glass blocks too
    return p


def doorframe_400():
    """Wall with a doorway: clear opening exactly 160 x 260 (Y -80..80, Z 0..260), collision included."""
    p = Piece("SM_Kit_DoorFrame_400", ((-19.5, -200, 0), (19.5, 200, 300)))
    D0, D1, DH = -80.0, 80.0, 260.0
    _wall_part(p, (-WALL_T / 2, -200, 0), (WALL_T / 2, D0 - 12, WALL_H))
    _wall_part(p, (-WALL_T / 2, D1 + 12, 0), (WALL_T / 2, 200, WALL_H))
    _wall_part(p, (-WALL_T / 2, D0 - 12, DH + 12), (WALL_T / 2, D1 + 12, WALL_H))
    _wall_dress(p, -200, D0 - 12, ribs=(-150.0,), strip=True)
    _wall_dress(p, D1 + 12, 200, ribs=(150.0,), strip=True)
    p.box((-WALL_T / 2 - 4, D0 - 12, 290.0), (WALL_T / 2 + 4, D1 + 12, 300.0), reg="frame")
    # jambs and head: heavy gunmetal frame, hazard chevrons on the inner faces, a light bar over the opening
    for (y0, y1, inner) in ((D0 - 12, D0, "Y"), (D1, D1 + 12, "y")):
        p.box((-18, y0, 0), (18, y1, DH + 12), reg="frame", regs={inner: "hazard"},
              uvo={inner: (("z", 0, DH + 12), ("x", -18, 18)), "x": (("z", 0, DH + 12), ("y", y0, y1)), "X": (("z", 0, DH + 12), ("y", y0, y1))})
    p.box((-18, D0 - 12, DH), (18, D1 + 12, DH + 12), reg="frame")
    for s in (-1.0, 1.0):
        xa, xb = sorted((s * 18, s * 19.5))
        p.box((xa, D0 + 6, DH + 3), (xb, D1 - 6, DH + 9), mat="Glow")
    p.cbox((-WALL_T / 2, -200, 0), (WALL_T / 2, D0, WALL_H))
    p.cbox((-WALL_T / 2, D1, 0), (WALL_T / 2, 200, WALL_H))
    p.cbox((-WALL_T / 2, D0, DH), (WALL_T / 2, D1, WALL_H))
    p.cbox((-18, D0 - 12, 0), (18, D0, DH + 12))
    p.cbox((-18, D1, 0), (18, D1 + 12, DH + 12))
    return p


def _rails(p, z_at, x0, x1, y_in, y_out, posts):
    """Posts, top rail and mid rail (yellow) following the walk surface z_at(x), on the strip y_in..y_out."""
    ya, yb = sorted((y_in, y_out))
    for x in posts:
        z = z_at(x)
        p.box((x - 3, ya, z), (x + 3, yb, z + RAIL_H), reg="rail", uvo={f: (("z", z, z + RAIL_H), ("x", x - 3, x + 3)) for f in "yY"})
    for (h0, h1) in ((RAIL_H - 6, RAIL_H), (52.0, 57.0)):
        prof = [(x0, z_at(x0) + h0), (x1, z_at(x1) + h0), (x1, z_at(x1) + h1), (x0, z_at(x0) + h1)]
        p.prism(prof, "xz", ya, yb, regs="rail", cap_reg="rail", fitu=True)


def ramp_400():
    """400 long (X), 200 wide, rising 100 toward +X at 14.04 degrees. Rails on both sides, 110 high."""
    p = Piece("SM_Kit_Ramp_400", ((-200, -100, 0), (200, 100, 210)))
    z_at = lambda x: (x + 200.0) * TAN_RAMP
    prof = [(-200.0, 0.0), (200.0, 0.0), (200.0, 100.0), (0.0, 50.0)]
    p.prism(prof, "xz", -100.0, 100.0, regs=["sw_dark", "frame", "grate", "grate"], cap_reg="pillar", fitu=True)
    _rails(p, z_at, -200.0, 200.0, 94.0, 100.0, (-196.0, -100.0, 0.0, 100.0, 196.0))
    _rails(p, z_at, -200.0, 200.0, -94.0, -100.0, (-196.0, -100.0, 0.0, 100.0, 196.0))
    p.chull([(x, y, z) for y in (-94.0, 94.0) for (x, z) in ((-200, 0), (200, 0), (200, 100))])
    for (ya, yb) in ((94.0, 100.0), (-100.0, -94.0)):
        p.chull([(x, y, z) for y in (ya, yb) for (x, z) in ((-200, 0), (200, 100), (200, 100 + RAIL_H), (-200, RAIL_H))])
    return p


def stairs_400():
    """8 steps, 50 cm level treads, 12.5 cm risers, 100 cm rise toward +X, 200 wide. Ramp collision through the
    middle of every tread (feet within 6.25 cm of each tread) plus rail walls."""
    p = Piece("SM_Kit_Stairs_400", None)
    for i in range(8):
        x0, x1, z1 = -200.0 + 50 * i, -150.0 + 50 * i, 12.5 * (i + 1)
        # riser block, set back 3 cm under the tread so each nosing throws a shadow line
        p.box((x0 + 3, -100, 0), (x1, 100, z1 - 4), regs={"x": "kick", "z": "sw_dark"}, reg="sw_gun", faces="xyYz" if i < 7 else "xXyYz",
              uvo={"x": (("y", -100, 100), ("z", 0, z1))})
        # tread slab 4 cm thick, its top exactly at the step height, overhanging the riser by 3 cm
        p.box((x0, -100, z1 - 4), (x1, 100, z1), regs={"Z": "tread", "x": "sw_yel", "z": "sw_dark"}, reg="sw_gun",
              uvo={"Z": (("y", -100, 100), ("x", x0, x1, 1.0, 0.5))}, faces="xZyYz" if i < 7 else "xXZyYz")
    for s_ in (-1.0, 1.0):        # stringers: a raking band along the nosing line, so the step profile shows below it
        ya, yb = sorted((s_ * 100, s_ * 108))
        nose = lambda x: 0.25 * x + 62.5
        prof = [(-200.0, 0.0), (-200.0 + 4 * 22.0, 0.0), (200.0, nose(200.0) - 22.0), (200.0, nose(200.0) + 8.0), (-200.0, nose(-200.0) + 8.0)]
        p.prism(prof, "xz", ya, yb, regs=["sw_dark", "frame", "frame", "frame", "frame"], cap_reg="frame", fitu=True)
        _rails(p, lambda x: 0.25 * x + 70.5, -196.0, 160.0, s_ * 101, s_ * 107, (-190.0, -100.0, 0.0, 100.0, 156.0))
    prof_c = [(-225.0, 0.0), (175.0, 100.0), (200.0, 100.0)]
    p.chull([(x, y, z) for y in (-100.0, 100.0) for (x, z) in prof_c + [(200.0, -10.0), (-225.0, -10.0)]])
    for (ya, yb) in ((100.0, 108.0), (-108.0, -100.0)):      # rail walls stop with the rails, 40 cm short of the top
        p.chull([(x, y, z) for y in (ya, yb) for (x, z) in ((-200, 0), (160, 0), (160, 0.25 * 160 + 70.5 + RAIL_H), (-200, 0.25 * -200 + 70.5 + RAIL_H))])
    return p


def catwalk_400():
    """Deck 400 x 200, top at Z = 0 (15 thick, beams below), railings 110 high on both long sides."""
    p = Piece("SM_Kit_Catwalk_400", ((-200, -100, -45), (200, 100, 110)))
    for x0 in (-200.0, 0.0):
        p.box((x0, -100, -15), (x0 + 200, 100, 0), regs={"Z": "grate", "z": "sw_dark"}, reg="frame",
              uvo={"Z": (("x", x0, x0 + 200), ("y", -100, 100)), "z": (("x", x0, x0 + 200), ("y", -100, 100))})
    for y in (-88.0, 88.0):
        p.box((-200, y - 7, -45), (200, y + 7, -15), reg="frame")
    for x in (-150.0, 0.0, 150.0):
        p.box((x - 6, -81, -40), (x + 6, 81, -15), reg="sw_gun")
    for s in (-1.0, 1.0):
        _rails(p, lambda x: 0.0, -200.0, 200.0, s * 94, s * 100, (-196.0, -100.0, 0.0, 100.0, 196.0))
        ya, yb = sorted((s * 92, s * 100))
        p.box((-200, ya, 0), (200, yb, 12), reg="hazard", uvo={f: (("x", -200, 200), ("z", 0, 12)) for f in "yY"})   # toe board
    p.cbox((-200, -100, -15), (200, 100, 0))
    p.cbox((-200, 92, 0), (200, 100, RAIL_H))
    p.cbox((-200, -100, 0), (200, -92, RAIL_H))
    return p


def railing_400():
    """Edge railing for platforms and landings: 400 long (X), posts on the line Y = 0..6, 110 high, toe board."""
    p = Piece("SM_Kit_Railing_400", ((-200, 0, 0), (200, 6, 110)))
    _rails(p, lambda x: 0.0, -200.0, 200.0, 0.0, 6.0, (-196.0, -100.0, 0.0, 100.0, 196.0))
    p.box((-200, 0, 0), (200, 6, 12), reg="hazard", uvo={f: (("x", -200, 200), ("z", 0, 12)) for f in "yY"})
    p.cbox((-200, 0, 0), (200, 6, RAIL_H))
    return p


def cover_200():
    """Low cover: 200 long (X), 40 deep, 110 high (waist), chamfered top edges, hazard band."""
    p = Piece("SM_Kit_Cover_200", ((-100, -21, 0), (100, 21, 110)))
    prof = [(-20.0, 8.0), (20.0, 8.0), (20.0, 92.0), (12.0, 110.0), (-12.0, 110.0), (-20.0, 92.0)]
    p.prism(prof, "yz", -100.0, 100.0, regs=["sw_dark", "pillar", "frame", "frame", "frame", "pillar"], cap_reg="frame", fitu=True,
            vr=[None, (0.08, 0.92), None, None, None, (0.08, 0.92)])
    p.box((-98, -18, 0), (98, 18, 8), reg="kick")
    for s_ in (-1.0, 1.0):
        ya, yb = sorted((s_ * 20.0, s_ * 21.0))
        p.box((-90, ya, 60), (90, yb, 70), reg="hazard", faces="yY", uvo={f: (("x", -100, 100), ("z", 60, 70)) for f in "yY"})
    p.cbox((-100, -20, 0), (100, 20, 110))
    return p


def beacon_mast():
    """A 16 m light mast so a site reads from the approach: a 40 cm lattice-look pole on a 120 cm footing, glow strips
    up two faces (Glow) and a lamp head (Beacon slot, much brighter). Collision: footing and pole."""
    H = 1600.0
    p = Piece("SM_Kit_BeaconMast", ((-80, -80, 0), (80, 80, H + 130)))
    p.box((-60, -60, 0), (60, 60, 30), reg="frame")
    p.box((-40, -40, 30), (40, 40, 60), reg="hazard", uvo={f: (("y" if f in "xX" else "x", -40, 40), ("z", 30, 60)) for f in "xXyY"})
    p.box((-20, -20, 60), (20, 20, H), reg="pillar", faces="xXyY",
          uvo={f: (("y" if f in "xX" else "x", -20, 20, 0.0, 0.2), ("z", 60, 260)) for f in "xXyY"})
    for z in range(260, int(H), 300):          # collars every 3 m
        p.box((-26, -26, z), (26, 26, z + 14), reg="frame")
    for s_ in (-1.0, 1.0):
        xa, xb = sorted((s_ * 20.0, s_ * 21.5))
        p.box((xa, -5, 120), (xb, 5, H - 40), mat="Glow", faces="xXyY")
    # lamp head (r4b: 1.4 m lantern, was 0.5 m): a gunmetal cradle, the lamp block, a cap
    p.box((-80, -80, H), (80, 80, H + 14), reg="frame")
    p.box((-70, -70, H + 14), (70, 70, H + 116), mat="Beacon")
    p.box((-80, -80, H + 116), (80, 80, H + 130), reg="frame")
    for (x, y) in ((-74, -74), (74, -74), (-74, 74), (74, 74)):
        p.box((x - 6, y - 6, H + 14), (x + 6, y + 6, H + 116), reg="frame")
    p.cbox((-60, -60, 0), (60, 60, 60))
    p.cbox((-26, -26, 60), (26, 26, H))
    p.cbox((-80, -80, H), (80, 80, H + 130))
    return p


def light_pillar():
    """A soft beam of light for reading a site from far away (no Niagara): an open cylinder 140 wide, 60 m tall, slot
    Pillar (M_Kit_LightPillar: additive, unlit, two-sided; brightest at the base, fading up, soft at the silhouette).
    UV V runs along the height. No collision."""
    p = Piece("SM_Kit_LightPillar", ((-70, -70, 0), (70, 70, 6000)))
    p.lathe([(70.0, 0.0), (70.0, 6000.0)], 16, mat="Pillar")
    return p


def pillar():
    p = Piece("SM_Kit_Pillar", ((-25, -25, 0), (25, 25, 300)))
    p.box((-20, -20, 20), (20, 20, 280), reg="pillar", faces="xXyY",
          uvo={f: (("y" if f in "xX" else "x", -20, 20, 0.0, 0.2), ("z", 20, 280)) for f in "xXyY"})
    p.box((-25, -25, 0), (25, 25, 20), reg="frame")
    p.box((-25, -25, 280), (25, 25, 300), reg="frame")
    p.box((-22, -22, 140), (22, 22, 150), reg="hazard")
    p.cbox((-25, -25, 0), (25, 25, 300))
    return p


def corner():
    """Outside-corner column for two walls meeting at 90 degrees (walls along -X and -Y of it): an octagonal post
    with one light strip on the outer (+X +Y) facet."""
    p = Piece("SM_Kit_Corner", ((-20, -20, 0), (20, 20, 300)))
    c = 8.3
    oct_ = [(-20, -c), (-c, -20), (c, -20), (20, -c), (20, c), (c, 20), (-c, 20), (-20, c)]
    p.prism(oct_, "xy", 10.0, 290.0, regs="pillar", cap_reg="frame", fitu=True, vr=None)
    p.box((-20, -20, 0), (20, 20, 10), reg="frame")
    p.box((-20, -20, 290), (20, 20, 300), reg="frame")
    a, b = (20.0, c), (c, 20.0)
    n = (0.7071, 0.7071)
    o = 0.8
    p.poly([(a[0] + n[0] * o - 2.0, a[1] + n[1] * o + 2.0, 40), (b[0] + n[0] * o + 2.0, b[1] + n[1] * o - 2.0, 40),
            (b[0] + n[0] * o + 2.0, b[1] + n[1] * o - 2.0, 260), (a[0] + n[0] * o - 2.0, a[1] + n[1] * o + 2.0, 260)],
           mat="Glow", n=(n[0], n[1], 0))
    p.cbox((-20, -20, 0), (20, 20, 300))
    return p


def roof_400():
    """Roof panel: place at the SAME transform as the floor below; occupies Z 300..320, two light strips under it."""
    p = Piece("SM_Kit_Roof_400", ((-200, -200, 296), (200, 200, 320)))
    for x0 in (-200.0, 0.0):
        for y0 in (-200.0, 0.0):
            p.box((x0, y0, 300), (x0 + 200, y0 + 200, 320), regs={"z": "roof", "Z": "sw_gun"}, reg="frame",
                  uvo={"z": (("x", x0, x0 + 200), ("y", y0, y0 + 200))})
    for y in (-100.0, 100.0):
        p.box((-190, y - 5, 296), (190, y + 5, 300), mat="Glow", faces="xXyYz")
    p.cbox((-200, -200, 300), (200, 200, 320))
    return p


def console_base():
    """Console 60 deep, 160 wide, 105 high; the screen slopes toward +X (the operator stands at +X, 150 cm clear)."""
    p = Piece("SM_Kit_Console", ((-30, -83, 0), (44, 83, 106.56)))
    prof = [(-30.0, 8.0), (30.0, 8.0), (30.0, 80.0), (10.0, 105.0), (-30.0, 105.0)]
    p.prism(prof, "xz", -80.0, 80.0, regs=["sw_dark", "console", "console", "sw_gun", "console"], cap_reg="console", fitu=True)
    p.box((-28, -78, 0), (28, 78, 8), reg="kick")
    # the screen: a quad 1 cm proud of the sloped face, inset 8 cm from its edges
    a, b = (30.0, 80.0), (10.0, 105.0)
    t = (b[0] - a[0], b[1] - a[1])
    L = math.hypot(*t)
    u = (t[0] / L, t[1] / L)
    nrm = (u[1], -u[0])
    if nrm[1] < 0:
        nrm = (-nrm[0], -nrm[1])
    nrm = (abs(nrm[0]), abs(nrm[1]))
    p0 = (a[0] + u[0] * 4 + nrm[0] * 1.0, a[1] + u[1] * 4 + nrm[1] * 1.0)
    p1 = (b[0] - u[0] * 4 + nrm[0] * 1.0, b[1] - u[1] * 4 + nrm[1] * 1.0)
    # UV 0..1 over the glass (U left -> right for the operator at +X, V bottom -> top): ScreenTexture shows whole
    p.poly([(p0[0], -70, p0[1]), (p0[0], 70, p0[1]), (p1[0], 70, p1[1]), (p1[0], -70, p1[1])], mat="Screen", n=(nrm[0], 0, nrm[1]),
           uv=[(0.0, 0.0), (1.0, 0.0), (1.0, 1.0), (0.0, 1.0)])
    # bezel around the screen
    for t0, t1 in ((0.0, 4.0), (L - 4.0, L)):
        q0 = (a[0] + u[0] * t0, a[1] + u[1] * t0)
        q1 = (a[0] + u[0] * t1, a[1] + u[1] * t1)
        p.prism([(q0[0], q0[1]), (q1[0], q1[1]), (q1[0] + nrm[0] * 2.5, q1[1] + nrm[1] * 2.5), (q0[0] + nrm[0] * 2.5, q0[1] + nrm[1] * 2.5)],
                "xz", -74.0, 74.0, regs="frame", cap_reg="frame", fitu=True)
    for y in (-74.0, 70.0):
        p.prism([(a[0], a[1]), (b[0], b[1]), (b[0] + nrm[0] * 2.5, b[1] + nrm[1] * 2.5), (a[0] + nrm[0] * 2.5, a[1] + nrm[1] * 2.5)],
                "xz", y, y + 4.0, regs="frame", cap_reg="frame", fitu=True)
    # keyboard ledge on the operator side, side ribs, a recessed kick
    p.box((30, -70, 70), (44, 70, 76), regs={"Z": "hatch"}, reg="sw_gun", uvo={"Z": (("y", -70, 70), ("x", 30, 44))})
    for y0, y1 in ((-83.0, -80.0), (80.0, 83.0)):
        p.box((-22, y0, 14), (22, y1, 96), reg="frame")
    p.cbox((-30, -80, 0), (30, 80, 105))
    return p


def light_strip_200():
    p = Piece("SM_Kit_LightStrip_200", ((-100, -5, 0), (100, 5, 6)))
    p.box((-100, -5, 0), (100, 5, 4), reg="frame")
    p.box((-96, -3, 4), (96, 3, 6), mat="Glow", faces="xXyYZ")
    return p


BUILDERS = [floor_400, platform_400, platform_400x200, platform_400x50, ramp_low_400, wall_400, wall_400_window, doorframe_400, ramp_400, stairs_400, catwalk_400,
            railing_400, cover_200, beacon_mast, light_pillar, pillar, corner, roof_400, console_base, light_strip_200]
import kit_pieces_art18  # noqa: E402  (ART-18 story props, same kit style)
BUILDERS += kit_pieces_art18.BUILDERS
NO_COLLISION = ("SM_Kit_LightStrip_200", "SM_Kit_LightPillar")

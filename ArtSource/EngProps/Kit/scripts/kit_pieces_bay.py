# SPDX-License-Identifier: MIT
# Kit, the Dominion docking bay mouth (#126 ART-15; batch AD). ATDDockingBay's placeholder mouth was four orange edge
# slabs at emissive 4 and 24 white spheres at 12: blown out near-white in play. These replace them:
#   SM_Kit_BayMouthFrame - a dark Null-trim portal around the 5000 x 3500 opening (DoorHalfWidth 2500, door height
#     0.7 x 2 x 2500 = 3500; Grok Bot's plan r1 mouth 5000 x 3500), three recessed steps with depth, two thin ember
#     strips (about 10 % of the face area), and a dark liner tunnel 1200 deep behind the opening. Pivot = the mouth
#     centre (the hull's SOCKET_Bay / ATDDockingBay's origin), +X out of the mouth, scale 1. Non-walkable, no collision.
#   SM_Kit_BayGuideLight - one approach light: a low housing with a hood and a lens facing +X (out along the approach),
#     sized like the placeholder sphere (600 cm). Pivot = the housing centre. No collision.
from kit_lib import Piece

HW, HH = 2500.0, 1750.0          # the opening's half width / half height


def _ring(p, y0, y1, z0, z1, x0, x1, regs, mat="Dom"):
    """A rectangular band between the inner (y0, z0) and outer (y1, z1) half sizes, from x0 to x1."""
    p.box((x0, -y1, z0), (x1, y1, z1), regs=regs, mat=mat)            # top
    p.box((x0, -y1, -z1), (x1, y1, -z0), regs=regs, mat=mat)          # bottom
    p.box((x0, y0, -z0), (x1, y1, z0), regs=regs, mat=mat)            # +Y jamb
    p.box((x0, -y1, -z0), (x1, -y0, z0), regs=regs, mat=mat)          # -Y jamb


def bay_mouth_frame():
    p = Piece("SM_Kit_BayMouthFrame", ((-1200.0, -HW - 800.0, -HH - 700.0), (302.0, HW + 800.0, HH + 700.0)))
    face = {"X": "wall", "x": "sw_dark", "y": "frame", "Y": "frame", "z": "frame", "Z": "frame"}
    # three recessed steps, outer to inner: each further back and closer to the opening
    _ring(p, HW + 400.0, HW + 800.0, HH + 350.0, HH + 700.0, -300.0, 300.0, face)
    _ring(p, HW + 200.0, HW + 400.0, HH + 175.0, HH + 350.0, -300.0, 120.0, {**face, "X": "sw_dark"})
    _ring(p, HW, HW + 200.0, HH, HH + 175.0, -1200.0, -60.0, {**face, "X": "frame"})
    # the liner tunnel behind the opening: dark plates on the four inner faces, ribbed every 300
    p.box((-1200.0, -HW, HH - 2.0), (-60.0, HW, HH), reg="sw_black", mat="Dom")
    p.box((-1200.0, -HW, -HH), (-60.0, HW, -HH + 2.0), reg="sw_black", mat="Dom")
    for s in (-1.0, 1.0):
        y0, y1 = sorted((s * HW, s * (HW - 2.0)))
        p.box((-1200.0, y0, -HH), (-60.0, y1, HH), reg="sw_black", mat="Dom")
    x = -1140.0
    while x < -100.0:
        _ring(p, HW - 40.0, HW, HH - 40.0, HH, x, x + 40.0, {f: "sw_dark" for f in "xXyYzZ"})
        x += 300.0
    # two thin ember strips on the step faces (about 10 % of the face area), proud by 2 cm
    for (yi, zi, w) in ((HW + 560.0, HH + 485.0, 40.0), (HW + 270.0, HH + 235.0, 40.0)):
        xf = 302.0 if yi > HW + 400.0 else 122.0
        _ring(p, yi, yi + w, zi, zi + w, xf - 2.0, xf, {}, mat="Ember")
    # sill lip lights: short ember dashes along the sill edge (the pilot's horizon on the glide)
    for i in range(-5, 6):
        p.box((-60.0, i * 440.0 - 120.0, -HH - 2.0), (0.0, i * 440.0 + 120.0, -HH + 6.0), mat="Ember")
    return p


def bay_guide_light():
    p = Piece("SM_Kit_BayGuideLight", ((-300.0, -300.0, -200.0), (300.0, 300.0, 200.0)))
    p.box((-300.0, -300.0, -200.0), (200.0, 300.0, -120.0), regs={"Z": "frame", "z": "sw_dark", "x": "sw_dark", "X": "sw_dark", "y": "sw_dark", "Y": "sw_dark"}, mat="Dom")
    p.box((-260.0, -220.0, -120.0), (200.0, 220.0, 160.0), regs={"Z": "wall", "x": "sw_dark", "X": "frame", "y": "frame", "Y": "frame"}, mat="Dom")
    p.box((200.0, -240.0, 140.0), (300.0, 240.0, 200.0), reg="sw_dark", mat="Dom")              # the hood over the lens
    p.box((200.0, -170.0, -80.0), (212.0, 170.0, 120.0), mat="Work")                           # the lens, facing +X
    p.box((200.0, -220.0, -120.0), (240.0, 220.0, -80.0), reg="sw_dark", mat="Dom")             # the lens sill
    return p


BUILDERS = [bay_mouth_frame, bay_guide_light]
NO_COLLISION = ("SM_Kit_BayMouthFrame", "SM_Kit_BayGuideLight")

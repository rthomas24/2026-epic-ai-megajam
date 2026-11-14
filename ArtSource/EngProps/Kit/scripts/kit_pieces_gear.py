# SPDX-License-Identifier: MIT
# Landing gear for the three squad ships (the hulls are single static meshes with no gear). One mesh per ship, in the
# deployed pose; its pivot is the plane where the struts meet the hull, so the landing code can scale it in on Z (0 -> 1)
# from inside the hull on ETDLandingStep::GearDown and out again on GearUp. A "Skid" socket marks the pad contact:
# UTDLandingComponent::HullBottomBelowOrigin already parks a ship on a visible mesh's "Skid" socket.
# Hull bounds measured in Unreal (review/ship_measure.json), ship frame = hull frame (identity, +X forward):
#   Rift    X +-250, Y +-208, Z -113..113      Lancer  X +-262, Y +-135, Z -75..75      Bastion X +-262, Y +-236, Z -70..70
# Slots: Trim (gunmetal struts, the kit trim), Accent (a squad-colour band), Pad (dark rubber-black pads).
import math

from kit_lib import Piece

CLEAR = 55.0          # visible gear below the hull's lowest point (cm): the ship parks this much higher than before


def _strut(p, top, foot, w=9.0, accent=True):
    """A strut from top (x, y, z) to foot: a square leg, a knee collar, an accent band, a pad at the foot."""
    tx, ty, tz = top
    fx, fy, fz = foot
    from kit_pieces_story import _tube
    _tube(p, top, (fx, fy, fz + 14.0), w, reg="frame")
    mx, my, mz = (tx + fx) / 2.0, (ty + fy) / 2.0, (tz + fz) / 2.0
    p.box((mx - w - 3, my - w - 3, mz - 8), (mx + w + 3, my + w + 3, mz + 8), reg="sw_gun")
    if accent:
        p.box((mx - w - 2, my - w - 2, mz + 10), (mx + w + 2, my + w + 2, mz + 18), mat="Accent")
    p.box((fx - 26.0, fy - 26.0, fz), (fx + 26.0, fy + 26.0, fz + 7.0), mat="Pad")
    p.box((fx - 16.0, fy - 16.0, fz + 7.0), (fx + 16.0, fy + 16.0, fz + 14.0), reg="sw_dark")


def _gear(name, hull_min_z, feet, attach_inset=20.0):
    """feet: (x, y, splay_x, splay_y): each strut runs from (x, y) at the attach plane down and outward to its pad."""
    p = Piece(name, None)
    top_z = 0.0                                    # the pivot plane (hull_min_z + attach_inset in the ship frame)
    foot_z = -(attach_inset + CLEAR)               # the pad bottom
    for (x, y, sx, sy) in feet:
        _strut(p, (x, y, top_z), (x + sx, y + sy, foot_z))
        p.box((x - 22.0, y - 22.0, -4.0), (x + 22.0, y + 22.0, 0.0), reg="sw_dark")      # the bay plate it hangs from
    p.sockets = [("Skid", (0.0, 0.0, foot_z))]
    p.chull([(x + sx + dx, y + sy + dy, z) for (x, y, sx, sy) in feet for dx in (-26.0, 26.0) for dy in (-26.0, 26.0)
             for z in (foot_z, foot_z + 14.0)])
    p.meta = {"attach_z": hull_min_z + attach_inset, "pad_contact_z": hull_min_z + attach_inset + foot_z}
    return p


def rift_gear():
    """Rift: three struts - a nose leg and two aft legs under the crescent wings, splayed out."""
    return _gear("SM_Rift_Gear", -113.0, [(150.0, 0.0, 20.0, 0.0), (-130.0, -120.0, -10.0, -25.0), (-130.0, 120.0, -10.0, 25.0)])


def lancer_gear():
    """Lancer: three struts - a long narrow interceptor, so a nose leg and two aft legs close in."""
    return _gear("SM_Lancer_Gear", -75.0, [(170.0, 0.0, 15.0, 0.0), (-150.0, -70.0, -10.0, -20.0), (-150.0, 70.0, -10.0, 20.0)])


def bastion_gear():
    """Bastion: four heavy struts at the corners of the wide hull."""
    return _gear("SM_Bastion_Gear", -70.0, [(150.0, -150.0, 10.0, -25.0), (150.0, 150.0, 10.0, 25.0),
                                            (-150.0, -150.0, -10.0, -25.0), (-150.0, 150.0, -10.0, 25.0)])


BUILDERS = [rift_gear, lancer_gear, bastion_gear]
NO_COLLISION = ("SM_Rift_Gear", "SM_Lancer_Gear", "SM_Bastion_Gear")       # the ship's own collision does the work

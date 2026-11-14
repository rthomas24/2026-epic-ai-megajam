# SPDX-License-Identifier: MIT
# Kit, Cinder magma bridge (T7's play, Nov 14: the conduit route crosses the magma river and kills the pilot).
# Walkable kit piece (R-043): 50 cm grid, bevelled hard surface, the planet trim sheet, authored collision whose walk
# tops equal the visual tops exactly. Unreal centimetres, +X along the span, pivot on the bank's ground plane at the
# span centre. T5 places it across the channel on the conduit route (yaw along the route).
#   SM_Kit_MagmaBridge - 2800 long overall: 200 cm ramps (14.04 degrees, the kit's ramp pitch) up to a 2400 cm deck at
#   +50 cm (the magma trigger ends 10 cm above the surface), 300 cm walk width between 110 cm rails, two piers
#   standing in the magma, side girders and hazard-striped ends.
from kit_lib import Piece

RAIL_H = 110.0                     # kit_pieces.RAIL_H (not imported: build_kit runs kit_pieces by path)


def _rails(p, z_at, x0, x1, y_in, y_out, posts):
    """kit_pieces._rails: posts, top rail and mid rail following the walk surface z_at(x)."""
    ya, yb = sorted((y_in, y_out))
    for x in posts:
        z = z_at(x)
        p.box((x - 3, ya, z), (x + 3, yb, z + RAIL_H), reg="rail", uvo={f: (("z", z, z + RAIL_H), ("x", x - 3, x + 3)) for f in "yY"})
    for (h0, h1) in ((RAIL_H - 6, RAIL_H), (52.0, 57.0)):
        prof = [(x0, z_at(x0) + h0), (x1, z_at(x1) + h0), (x1, z_at(x1) + h1), (x0, z_at(x0) + h1)]
        p.prism(prof, "xz", ya, yb, regs="rail", cap_reg="rail", fitu=True)

L_DECK, L_RAMP, H, W = 1200.0, 200.0, 50.0, 150.0      # half deck, ramp run, deck top, half walk width
X_END = L_DECK + L_RAMP


def _z(x):
    ax = abs(x)
    return H if ax <= L_DECK else max(0.0, H * (X_END - ax) / L_RAMP)


def magma_bridge():
    p = Piece("SM_Kit_MagmaBridge", ((-X_END, -W - 35, -150), (X_END, W + 35, H + RAIL_H)))
    # deck: grating top, dark structural edge; the ends hazard-striped where the ramps meet it
    p.box((-L_DECK, -W, 30.0), (L_DECK, W, H), regs={"Z": "grate", "z": "sw_dark", "x": "frame", "X": "frame", "y": "frame", "Y": "frame"})
    for s in (-1.0, 1.0):
        x0, x1 = sorted((s * L_DECK, s * (L_DECK - 50.0)))
        p.box((x0, -W, H), (x1, W, H + 0.5), reg="hazard", faces="Z")
    # ramps (solid wedges down to the bank) and abutment blocks under the deck ends
    for s in (-1.0, 1.0):
        prof = [(s * X_END, 0.0), (s * L_DECK, 0.0), (s * L_DECK, H)]
        if s < 0:
            prof = prof[::-1]
        p.prism(prof, "xz", -W, W, regs="grate", cap_reg="pillar", fitu=True)
        a0, a1 = sorted((s * L_DECK, s * (L_DECK - 200.0)))
        p.box((a0, -W - 25.0, -20.0), (a1, W + 25.0, 30.0), reg="sw_dark", faces="xXyY")
    # side girders outside the rails (a deep box section) and the piers in the magma
    for y0, y1 in ((W, W + 25.0), (-W - 25.0, -W)):
        p.box((-L_DECK, y0, -40.0), (L_DECK, y1, H + 10.0), regs={"y": "frame", "Y": "frame", "Z": "sw_gun", "z": "sw_dark", "x": "hazard", "X": "hazard"})
    for x in (-400.0, 400.0):
        p.box((x - 50.0, -W - 25.0, -150.0), (x + 50.0, W + 25.0, -40.0), regs={"x": "pillar", "X": "pillar", "y": "sw_dark", "Y": "sw_dark", "Z": "sw_dark"}, faces="xXyYZ")
        p.box((x - 60.0, -W - 35.0, -60.0), (x + 60.0, W + 35.0, -40.0), reg="hazard", faces="xXyYz")
    # rails on both sides, following the walk surface
    posts = [-X_END + 4.0] + [float(x) for x in range(-1200, 1201, 200)] + [X_END - 4.0]
    for ya, yb in ((W - 6.0, W), (-W, -W + 6.0)):
        _rails(p, _z, -X_END, -L_DECK, ya, yb, [posts[0], -L_DECK])
        _rails(p, _z, -L_DECK, L_DECK, ya, yb, posts[1:-1])
        _rails(p, _z, L_DECK, X_END, ya, yb, [L_DECK, posts[-1]])
    # collision: walk tops exactly the visual tops (deck at 50, ramps at 14.04 degrees), rails as walls
    p.cbox((-L_DECK, -W, 30.0), (L_DECK, W, H))
    for s in (-1.0, 1.0):
        p.chull([(x, y, z) for y in (-W, W) for (x, z) in ((s * X_END, 0.0), (s * L_DECK, 0.0), (s * L_DECK, H))])
        a0, a1 = sorted((s * L_DECK, s * (L_DECK - 200.0)))
        p.cbox((a0, -W - 25.0, -20.0), (a1, W + 25.0, 30.0))
    for (ya, yb), (ra, rb) in (((W - 6.0, W + 25.0), (W - 6.0, W)), ((-W - 25.0, -W + 6.0), (-W, -W + 6.0))):
        p.cbox((-L_DECK, ya, -40.0), (L_DECK, yb, H + RAIL_H))          # girder + deck rail as one wall
        for s in (-1.0, 1.0):
            xa, xb = s * X_END, s * L_DECK
            p.chull([(x, y, z) for y in (ra, rb) for (x, z) in ((xa, 0.0), (xb, H), (xb, H + RAIL_H), (xa, RAIL_H))])
    for x in (-400.0, 400.0):
        p.cbox((x - 60.0, -W - 35.0, -150.0), (x + 60.0, W + 35.0, -40.0))
    return p


BUILDERS = [magma_bridge]

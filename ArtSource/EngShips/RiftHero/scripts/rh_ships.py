# SPDX-License-Identifier: MIT
# Triad Drift - ship configs for the hybrid close-range pass (rh_hybrid.py --ship <Ship>, rh_closeparts.py).
# One dict per squad ship: where its Rodin source and its outputs are, its sockets (Unreal cm), its accent colour,
# and the measurements the close-range parts are built on. The Rift's values are the constants rh_closeparts.py and
# rh_hybrid.py carried until v11: its output must not change.
# Frame of every measurement: metres, nose +X, up +Z, Blender Y = -Unreal Y.
import os

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(os.path.dirname(HERE))                    # ArtSource/EngShips
REPO = os.path.abspath(os.path.join(KIT, "..", ".."))
AI = os.path.join(REPO, "ArtSource", "EngAI", "Ships")

# accent -> rh_parts.materials() colours (linear): accent paint, glow base / emission, hot emission, dim emission
ACCENTS = {
    "magenta": dict(paint=(0.80, 0.03, 0.46), glow=(0.55, 0.05, 0.42), glow_e=(1.0, 0.10, 0.78),
                    hot=(0.8, 0.5, 0.75), hot_e=(1.0, 0.72, 0.95), dim=(0.2, 0.02, 0.15), dim_e=(0.13, 0.008, 0.095),
                    glass=(0.03, 0.02, 0.06)),
    "cyan": dict(paint=(0.03, 0.52, 0.82), accent_e=(0.006, 0.10, 0.16), glow=(0.08, 0.42, 0.55), glow_e=(0.25, 0.91, 1.0),
                 hot=(0.55, 0.78, 0.82), hot_e=(0.72, 0.97, 1.0), dim=(0.02, 0.14, 0.2), dim_e=(0.02, 0.09, 0.13),
                 glass=(0.015, 0.045, 0.06)),
    "amber": dict(paint=(0.85, 0.40, 0.03), accent_e=(0.17, 0.08, 0.006), glow=(0.55, 0.33, 0.05), glow_e=(1.0, 0.70, 0.12),
                  hot=(0.82, 0.74, 0.5), hot_e=(1.0, 0.90, 0.6), dim=(0.2, 0.11, 0.02), dim_e=(0.13, 0.07, 0.01),
                  glass=(0.06, 0.04, 0.015)),
}

SHIPS = {
    "Rift": dict(
        ship="Rift", root=os.path.join(KIT, "RiftHero"), rodin=os.path.join(AI, "Rift", "mesh_r1"),
        heat=os.path.join(AI, "Rift", "heatmask_r1", "T_Rift_HeatMask.png"), accent="magenta",
        ring=True,                       # rh_parts.py: portal ring, engines, belly tail, portal disc
        sockets={   # Unreal cm, unchanged from the ship builder's export (ArtSource/EngShips/Rodin/scripts/rs_prep.py)
            "Muzzle_0": (252, -37, -60), "Muzzle_1": (252, 37, -60),
            "Engine": (-252, 0, -34), "Engine_0": (-252, -64, -34), "Engine_1": (-252, 64, -34),
            "Portal": (-170, 0, 55), "Cockpit": (60, 0, 0),
            "WingTip_L": (238, -176, -109), "WingTip_R": (238, 176, -109),
        },
        # canopy: socket Cockpit; the Rodin canopy's dark glass measured from above (y up to +-0.33); ox / ky place and
        # scale the seat, pilot and dash (designed on the Rift: 0 and 1)
        canopy=dict(cockpit=(0.60, 0.0, 0.0), x=(0.30, 1.50), wmax=0.33, deck=0.30, ox=0.0, ky=1.0, bows=(0.88, 1.28)),
        guns=dict(kind="rift", y=0.37, z=-0.565),
        nose=dict(kind="rift", x=(1.45, 2.44)),
        spine=dict(x=(-0.92, 0.27), edges=[-0.92, -0.45, 0.0, 0.27], hcap=0.26, hdef=0.2, tdef=0.15),
        sides=dict(x=(-0.6, 1.6), z=(-0.78, 0.10), ymin=0.14, sigma_k=2.6, sigma_r=0.42, iters=40, max_move=0.03),
        # (x0, x1, |y| max, |y| min, z0, z1): leftover Rodin chrome under the canopy sill and beside the nose
        repaint=[(0.95, 1.75, 0.42, 0.0, -0.42, 0.30), (1.40, 1.95, 0.36, 0.12, -0.48, -0.08)],
        repaint_accent=True,
    ),
    "Lancer": dict(
        ship="Lancer", root=os.path.join(KIT, "LancerHero"), rodin=os.path.join(AI, "Lancer", "mesh_r1"),
        heat=os.path.join(AI, "Lancer", "heatmask_r1", "T_Lancer_HeatMask.png"), accent="cyan",
        ring=False, nozzle_glow=True,    # the nozzle glow cores of rs_extras.py (slot M_Lancer_Glow), as before
        sockets={   # rs_prep.py, except Muzzle_1: the Lead's decision (Nov 13 night) is a clean symmetric pair, so it
                    # mirrors Muzzle_0 (it was (108, 45, -55), the tip of the Rodin mesh's short right-hand pod)
            "Muzzle_0": (152, -45, -59), "Muzzle_1": (152, 45, -59),
            "Engine": (-264, 0, -35), "Engine_0": (-264, 0, -35),
            "Engine_1": (-209, -55, -38), "Engine_2": (-209, 55, -38),
            "Ram": (262, 0, -58), "Cockpit": (25, 0, 0),
            "WingTip_L": (-188, -132, -50), "WingTip_R": (-188, 132, -50),
        },
        # measured on mesh_r1: the dark canopy runs x -0.72 .. 0.46 on the centre line, about 0.24 half-wide, its top
        # about 0.27 above the deck around it (the tub is 0.29 deep so the seat head clears the glass)
        canopy=dict(cockpit=(0.25, 0.0, 0.0), x=(-0.72, 0.46), wmax=0.24, deck=0.31, ox=-0.92, ky=0.74, bows=(-0.14, 0.24),
                    plan=(0.20, 0.80), dome="super", cutk=1.06, sill_w=0.06, skirt=(0.075, 0.05), seat_head=0, hk=1.2),
        # the long twin barrel is on Blender +Y (Unreal -Y, Muzzle_0), the short pod on the other side: both go
        guns=dict(kind="lancer", y=0.45, z=-0.59, x0=0.90, tip=1.52, cut_x=1.0),
        # the needle prow: stations (x, top z, bottom z, half width) measured on mesh_r1
        # Two fresh-eyes reviews judged the modelled prow (kind "needle", v01-v03) a regression: a smooth shell out of
        # style with the panelled hull. The Rodin prow stays (its maps cleaned like the rest of the hull); the needle
        # builder and these stations stay for a later, better-detailed attempt: nose=NEEDLE.
        nose=None, spine=None, needle=dict(kind="needle", table=[
            (1.20, -0.283, -0.620, 0.193), (1.30, -0.302, -0.604, 0.188), (1.40, -0.330, -0.588, 0.173),
            (1.50, -0.357, -0.573, 0.151), (1.60, -0.378, -0.563, 0.113), (1.70, -0.401, -0.551, 0.087),
            (1.80, -0.422, -0.548, 0.071), (1.90, -0.444, -0.543, 0.045), (2.00, -0.464, -0.546, 0.034),
            (2.10, -0.484, -0.552, 0.028), (2.20, -0.502, -0.552, 0.024), (2.30, -0.521, -0.558, 0.019),
            (2.40, -0.539, -0.562, 0.015), (2.50, -0.553, -0.573, 0.012), (2.62, -0.570, -0.578, 0.004)],
            x0=1.24, gun_y=0.45, gun_z=-0.59),
        deck=dict(x=(0.60, 1.20), edges=[0.60, 0.90, 1.20], hcap=0.20, hdef=0.18, tdef=-0.2),    # v01-v03 deck plates, unused
        sides=None,
        repaint=[(-0.82, 0.58, 0.31, 0.0, -0.10, 0.30)], repaint_accent=False,
    ),
    "Bastion": dict(
        ship="Bastion", root=os.path.join(KIT, "BastionHero"), rodin=os.path.join(AI, "Bastion", "mesh_r1"),
        heat=os.path.join(AI, "Bastion", "heatmask_r1", "T_Bastion_HeatMask.png"), accent="amber",
        ring=False, nozzle_glow=True,
        sockets={
            "Muzzle_0": (264, -44, -37), "Muzzle_1": (264, 44, -37),
            "Engine": (-264, 0, 0), "Engine_0": (-264, -77, 0), "Engine_1": (-264, 77, 0),
            "Dome": (-26, 0, 68), "Cockpit": (120, 0, 20),
            "MissilePod_L": (124, -165, -20), "MissilePod_R": (124, 165, -20),
        },
        canopy=None, guns=None, nose=None, spine=None, sides=None, repaint=[], repaint_accent=False,
    ),
}


def get(ship):
    return SHIPS[ship]

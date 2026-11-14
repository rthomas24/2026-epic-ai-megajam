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

ACCENTS["red"] = dict(paint=(0.70, 0.09, 0.03), glow=(0.6, 0.15, 0.03), glow_e=(1.0, 0.30, 0.08), hot=(0.9, 0.6, 0.4), hot_e=(1.0, 0.72, 0.45),
                      dim=(0.2, 0.04, 0.01), dim_e=(0.16, 0.035, 0.008), glass=(0.02, 0.0, 0.0))     # the Null Armada

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
        # v12: tested per texel (a straight end to the magenta side panel under the canopy, where whole faces left a
        # torn sawtooth), the first box a little wider and deeper (a stray magenta sliver ahead of the panel), and a
        # third box for the magenta shard between the panel and the canopy sill
        repaint=[(0.95, 1.75, 0.46, 0.0, -0.50, 0.30), (1.40, 1.95, 0.36, 0.12, -0.48, -0.08), (0.60, 0.95, 0.46, 0.0, -0.10, 0.30)],
        # v13: purple leftovers in the repainted area go too, a dark panel joint sits on the repaint's straight end, and
        # the nose slit is saturated accent paint (index 1 of rh_parts.materials()), it was a pale lilac glow
        repaint_tinted=True, repaint_seams=[(0.944, 0.956, 0.46, 0.0, -0.50, 0.30)], slit_mat=1,
        # v14 (review: pale blotches and white chips on the lower crescent wings): inside the wing boxes every light,
        # unsaturated texel becomes the wing's own magenta, the paint there is satin (roughness floor, no metal), and
        # three dark panel joints cross each wing
        accent_boxes=[(-2.4, 2.5, 2.2, 1.0, -1.2, -0.2)], accent_rough=0.55,
        accent_seams=[(-0.706, -0.694, 2.2, 1.0, -1.2, -0.2), (0.394, 0.406, 2.2, 1.0, -1.2, -0.2), (1.394, 1.406, 2.2, 1.0, -1.2, -0.2)],
        repaint_accent=True, repaint_texel=True, repaint_emissive=0,      # index of the box whose emissive texels go too
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
                    plan=(0.20, 0.80), dome="super", cutk=1.06, sill_w=0.06, skirt=(0.075, 0.05), seat_head=3, hk=1.2, soft=0.022,      # v08: the headrest in the shell's own value (the accent block read as a stray cube)
                    # v07 (Lead, Nov 14: the pilot did not read through the dark glass): a lighter seat shell and side
                    # consoles, a pale visor band, a thin cyan instrument glow, an accent headrest
                    seat_mat=3, visor_mat=6, instr=True),
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
        # The Rodin Bastion is the soundest of the three: blocky armour that reads well, a symmetric pair of cannons,
        # six-tube launchers. Only the lumpy barrel ends are replaced; the hull maps stay as they are (T.clean flattens
        # white paint, and this hull is mid-grey armour), and its hard edges keep a tighter smoothing angle.
        canopy=None, nose=None, spine=None, sides=None, repaint=[], repaint_accent=False, textures="raw", sharp_deg=30,
        # A glass cockpit was tried as v03 (Lead, Nov 14: keep the faceted windscreen, no bubble) and stopped in Blender:
        # the Rodin "windscreen" is not a flush pane. It is a raised cab block standing in a recess, with dark sloped
        # sides; cutting the dark faces into glass (rh_closeparts._canopy_facets) left the cab block standing inside brown
        # panes with tub walls around it. A real cockpit here means redesigning that block. To try again:
        # FACETS = dict(kind="facets", cockpit=(1.20, 0.0, 0.20), x=(1.30, 2.16), wmax=0.40, z=(0.0, 0.58), drop=0.035,
        #               depth=0.42, seat_x=1.50, head_z=0.22, dash_x=1.88, dash_z=0.06)   as canopy=
        guns=dict(kind="bastion", y=0.43, z=-0.41, x0=2.42, tip=2.62, r=0.068, cut_x=2.49),
        # v02 (review: the cockpit read as a black windscreen with a yellow glowing squiggle inside): the emissive map
        # is textures_r2's with the windscreen's texels cleared, so it reads as dark glass
        emissive_src=os.path.join(AI, "Bastion", "textures_r2", "T_Bastion_E.png"),
        emissive_clear=[(1.20, 2.14, 0.30, 0.0, 0.03, 0.56)],
    ),
}


def _null(ship, folder, sockets, **kw):
    """Null Armada ships: the Rodin hull, its normals and its maps as they are (dark low-poly armour: nothing to
    flatten or weld), maps capped at 1K in Unreal, no nozzle glow cores; parts as listed."""
    d = dict(ship=ship, root=os.path.join(KIT, ship + "Hero"), rodin=os.path.join(AI, "NullFleet", folder, kw.pop("rev", "mesh_r1")), heat=None,
             accent="red", ring=False, sockets=sockets, canopy=None, guns=None, nose=None, spine=None, sides=None, repaint=[],
             repaint_accent=False, textures="raw", sharp_deg=30, tex=1024, tri_budget=14000, keep_islands=True, weld=False)
    d.update(kw)
    return d


SHIPS["NullFighter"] = _null("NullFighter", "Fighter", {
    "Muzzle_0": (268, -57, -38), "Muzzle_1": (268, 57, -38),
    "Engine_0": (-277, -63, -14), "Engine_1": (-277, 63, -14), "Engine_2": (-275, 0, -14)},
    # the three engine bells, measured on mesh_r1: about 0.36 m in radius 0.3 m ahead of the exit, 0.27 m at the exit
    engines=dict(nozzles=[(0.63, -0.19), (0.0, -0.19), (-0.63, -0.19)], x0=-2.44, exit=-2.75, r_collar=0.375, r_exit=0.275, cut_x=-2.46))
SHIPS["NullGunship"] = _null("NullGunship", "Gunship", {
    "Muzzle_0": (748, -280, 295), "Muzzle_1": (748, -210, 300), "Muzzle_2": (748, 210, 300), "Muzzle_3": (748, 280, 295),
    "Engine_0": (-740, -270, 421), "Engine_1": (-740, -90, 421), "Engine_2": (-740, 75, 421), "Engine_3": (-740, 255, 421)},
    tri_budget=26000,
    # the four turret barrels, measured on mesh_r1: axes at |y| 2.08 and 2.78, z 2.92, about 0.14 m in radius, tips at x 7.5
    guns=dict(kind="barrels", barrels=[(2.78, 2.92), (2.08, 2.92), (-2.08, 2.92), (-2.78, 2.92)], x0=4.70, tip=7.50, r=0.135,
              cut_x=4.85, cut_y=(1.75, 3.08), cut_z=(2.68, 3.2)))


# The interceptor shares the fighter's crescent silhouette (same span to length, three engines): at 150 m the two
# could not be told apart. Cheapest change, no Rodin geometry cut: a 2.6 m lance forward from each wing tip (the ship
# reads as a fork, 8.9 m long where the fighter is 5.5 m) and twin canted tail fins (the fighter has one low fin).
SHIPS["NullInterceptor"] = _null("NullInterceptor", "Interceptor", {
    "Muzzle_0": (324, -74, -55), "Muzzle_1": (324, 74, -55), "Missile_0": (40, -150, -95), "Missile_1": (40, 150, -95),
    "Engine_0": (-332, -80, -13), "Engine_1": (-332, 80, -13), "Engine_2": (-332, 0, -15)}, rev="mesh_r2", tri_budget=13000,
    fins=dict(lances=[(2.30, 5.60, 2.62, -0.98, 0.20, 0.26)],
              fins=[([(-3.15, 0.05), (-1.55, 0.05), (-2.35, 1.75), (-3.05, 1.95)], 0.95, 0.07, 0.38)]))


# From behind the bomber was the fighter's recipe at twice the size (flat wedge, three engines in a row): it gets a
# twin-boom tail, two booms running 2.6 m aft of the engines with a fin on each (Lead, Nov 14).
SHIPS["NullBomber"] = _null("NullBomber", "Bomber", {
    "Muzzle_0": (-100, 0, -105), "Engine_0": (-563, -165, -10), "Engine_1": (-563, 165, -10), "Engine_2": (-592, 0, 15)},
    tri_budget=13000,
    fins=dict(lances=[(-8.5, -3.0, 3.3, 0.05, 0.34, 0.30)],
              fins=[([(-8.5, 0.15), (-6.9, 0.15), (-7.7, 1.9), (-8.5, 2.1)], 3.3, 0.09, 0.0)]))


def get(ship):
    return SHIPS[ship]

# SPDX-License-Identifier: MIT
# Kit - the walkable sites, rebuilt from the kit (Ryan, Nov 13 evening: nothing the pilot walks on, into or through
# is an AI mesh). Pure Python: writes sites/<Site>.manifest.json (the placement contract for T5 and the input of
# ue_make_sites.py). Unreal cm, site-relative, the site pivot on the ground; yaw in degrees.
#   python ArtSource/EngProps/Kit/scripts/kit_sites.py
import json
import os

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
K = "/Game/TriadDrift/Gameplay/Props/Kit/"
GLOW_V, GLOW_C = K + "MI_Kit_Glow_Verdance", K + "MI_Kit_Glow_Cinder"


def piece(name, mesh, x, y, z=0.0, yaw=0.0, **kw):
    d = {"name": name, "mesh": mesh if mesh.startswith("/Game/") else K + mesh, "location_cm": [x, y, z], "yaw_deg": yaw}
    d.update(kw)
    return d


# ------------------------------------------------------------------------------------------------ relay
def relay(awake):
    """Verdance objective 2: a 60-90 s data-transfer DEFENCE (three waves), sized to T5's arena needs (Nov 13 night):
    a flat 2800 x 2800 deck, top 50 above the ground, low ramps on the west, north and south sides; the device console
    4 m in from the WEST edge with 3+ m of open floor in front; five waist-high covers, none within 5 m of the console;
    the relay itself on a solid 2.5 m plinth at the EAST side (its top is out of mantle reach), with the beacon mast.
    Site X = east, +Y = north. Nothing walkable above 2.5 m within 10 m of the console; nothing solid off the deck."""
    R = "/Game/TriadDrift/Gameplay/Props/Relay/"
    D = 50.0                                   # deck top
    ps = []
    for x in (-1200, -800, -400, 0, 400, 800, 1200):
        for y in (-1200, -800, -400, 0, 400, 800, 1200):
            ps.append(piece("Deck_%d_%d" % (x, y), "SM_Kit_Platform_400x50", x, y))
    # approaches: west (two), north, south
    ps.append(piece("Ramp_W_S", "SM_Kit_RampLow_400", -1500, -800, 0, 0))
    ps.append(piece("Ramp_W_N", "SM_Kit_RampLow_400", -1500, 800, 0, 0))
    ps.append(piece("Ramp_N", "SM_Kit_RampLow_400", -600, 1500, 0, 270))
    ps.append(piece("Ramp_S", "SM_Kit_RampLow_400", -600, -1500, 0, 90))
    # the relay's plinth on the east side: 3 x 4 columns of 100 + 200 blocks, top at 300 (250 above the deck)
    for x in (400, 800, 1200):
        for y in (-600, -200, 200, 600):
            ps.append(piece("Plinth_%d_%d_lo" % (x, y), "SM_Kit_Platform_400", x, y, 0, slots={"Trim": K + "MI_Kit_Stone"}))
            ps.append(piece("Plinth_%d_%d_hi" % (x, y), "SM_Kit_Platform_400x200", x, y, 100, slots={"Trim": K + "MI_Kit_Stone"}))
    ps.append(piece("Relay", R + "SM_Relay", 800, 0, 300 - 90, 90, materials={"0": R + ("MI_Relay_Awake" if awake else "MI_Relay")},
                    note="Grok Bot's Rodin relay on the plinth, turned so its stairs face north (they stay over the plinth); its tier sits 90 cm into the plinth top. Not walkable: the plinth top is 250 above the deck (the pilot mantles at most 180)."))
    ps.append(piece("Beacon", "SM_Kit_BeaconMast", 1250, -750, 300, 0, slots={"Glow": GLOW_V, "Beacon": K + "MI_Kit_Beacon_Verdance"}))
    ps.append(piece("Beacon_Pillar", "SM_Kit_LightPillar", 1250, -750, 300 + 1730, 0, slots={"Pillar": K + "MI_Kit_LightPillar_Verdance"},
                    note="a 60 m soft light beam from the lamp head, so the site reads from the approach (no Niagara)"))
    # the device console: screen faces east, the pilot stands east of it facing west
    ps.append(piece("Device_Console", "SM_Kit_Console", -1000, 0, D, 0, slots={"Screen": K + "MI_Kit_Screen"}))
    # five covers off the ramp lanes (west Y +-800 at X < -1000; north / south X -800..-400)
    covers = [(-300, -650, 0), (-300, 650, 0), (0, 0, 90), (-150, -1100, 0), (-150, 1100, 0)]
    for i, (x, y, yaw) in enumerate(covers):
        ps.append(piece("Cover_%d" % i, "SM_Kit_Cover_200", x, y, D, yaw))
    stand = [-890.0, 0.0, D]
    return {
        "site": "Relay_Site", "actor": R + ("BP_Relay_Awake" if awake else "BP_Relay"),
        "about": "Verdance relay defence arena (T5's needs, Nov 13 night): deck 2800 x 2800, top Z 50, ramps W (2), N, S; relay on a 2.5 m plinth at the east side.",
        "pieces": ps,
        "device_interact": {"stand_at_site_cm": stand, "pilot_faces_yaw_deg": 180.0, "console_site_cm": [-1000.0, 0.0, D],
                            "device_on_console_site_cm": [-985.0, 0.0, D + 106.0], "open_floor_in_front_cm": 1090,
                            "note": "the pilot faces west at the console, his back to the deck; 1090 cm of open floor east of him to the first cover / the plinth at X 200"},
        "arena_cm": {"deck": {"x": [-1400, 1400], "y": [-1400, 1400], "z": D},
                     "approaches": {"west": "X -1600..-1400 at Y -800 and +800 (400 wide each)", "north": "Y 1400..1600 at X -600", "south": "Y -1600..-1400 at X -600"},
                     "covers_110_high": [[x, y, D, yaw] for (x, y, yaw) in covers],
                     "nearest_cover_to_console_cm": 955, "nearest_cover_to_stand_point_cm": 878,
                     "ramp_lanes_kept_clear": "the walk test (r4) found two covers on the north / south ramp lanes; moved off them in r4b",
                     "relay_plinth": {"x": [200, 1400], "y": [-800, 800], "top_z": 300, "note": "solid; the Rodin relay and the beacon mast (16 m, lamp at 1660 above the plinth) stand on it"},
                     "open_deck_west_of_plinth": {"x": [-1400, 200], "y": [-1400, 1400]},
                     "dormant_drone_suggested_site_cm": [700, 1100, D],
                     "drop_ring_cm": "13-14 m from the deck centre is the deck edge band; keep the 14 m of ground round the deck edge clear of solid props",
                     "highest_standable_within_10m_of_console": "the deck (50) and the covers (160 if vaulted)"},
        "walk_path_site_cm": [[-1800, -800, 0, "ground west of the south-west ramp"], [-1200, -800, 50, "deck"], [-890, 0, 50, "device point"],
                              [-600, 1800, 0, "ground north of the north ramp"], [-600, -1800, 0, "ground south of the south ramp"]],
        "placement_rules": ["Pivot on flat ground; the deck is 50 high, no piles.", "Keep 14 m of open ground round the deck edge (the drop ring).",
                            "Wake it with a dynamic instance of the Relay component's slot 0: RelayPower 0.15 -> 1 (emissive follows RelayPower squared)."],
    }


# ------------------------------------------------------------------------------------------------ wreck
def wreck():
    """A raised kit walkway straight through the ruptured midsection (top Z 200), walls inside the hull, ramps both sides."""
    W = "/Game/TriadDrift/Gameplay/Props/Wreck/"
    X = 75.0
    ps = [piece("Wreck", W + "SM_Wreck", 0, 0, -60, 0, materials={"0": W + "MI_Wreck"},
                note="Grok Bot's Rodin wreck as the exterior, sunk 60 cm (its belly rests in the ground). Its deck (now 182..198) and the r2 Rodin ramps are under the kit walkway; the hull section hulls still block the pawn outside.")]
    for y in (-600, -200, 200, 600):
        ps.append(piece("Walk_%d" % y, "SM_Kit_Platform_400x200", X, y))
    for y in (-200, 200):
        ps.append(piece("WallAft_%d" % y, "SM_Kit_Wall_400", X - 200, y, 200, 0))
        ps.append(piece("WallFwd_%d" % y, "SM_Kit_Wall_400_Window" if y > 0 else "SM_Kit_Wall_400", X + 200, y, 200, 180))
    for s in (-1, 1):
        ps.append(piece("Rail_Aft_%d" % s, "SM_Kit_Railing_400", X - 200, s * 600, 200, 270))
        ps.append(piece("Rail_Fwd_%d" % s, "SM_Kit_Railing_400", X + 200, s * 600, 200, 90))
        ps.append(piece("Step_%d" % s, "SM_Kit_Platform_400", X, s * 1000))
        ps.append(piece("RampHigh_%d" % s, "SM_Kit_Ramp_400", X, s * 1000, 100, 270 if s > 0 else 90))
        ps.append(piece("RampLow_%d" % s, "SM_Kit_Ramp_400", X, s * 1400, 0, 270 if s > 0 else 90))
    ps.append(piece("Console", "SM_Kit_Console", X + 145, 0, 200, 180, note="the wreck's core / log console on the walkway, screen facing -X"))
    return {
        "site": "Wreck_Site", "actor": W + "BP_Wreck",
        "about": "The Verdance wreck walk-through: a kit walkway (top Z 200, 400 wide, X -125..275) through the ruptured midsection along Y, ramps down both sides to Y +-1600.",
        "pieces": ps,
        "core_interact": {"stand_at_site_cm": [0.0, 0.0, 200.0], "pilot_faces_yaw_deg": 0.0, "console_site_cm": [220.0, 0.0, 200.0],
                          "moved_from": "(75, 0, 252) on the Rodin deck: now on the kit walkway, facing the console on the forward wall"},
        "walk_path_site_cm": [[X, -1750, 0, "ground off the -Y ramp toe (Y -1600)"], [X, -1400, 50, "low ramp"], [X, -1000, 150, "high ramp"],
                              [X, -600, 200, "walkway outside the hull"], [X, 0, 200, "inside the hull"], [X, 600, 200, "walkway, +Y side"],
                              [X, 1750, 0, "ground off the +Y ramp toe"]],
        "walkability_cm": {"walkway_top_z": 200, "walkway": {"x": [-125, 275], "y": [-800, 800]}, "inside_walls": "X -115 and 265 inner faces, Y -400..400",
                           "headroom": "hull roof collision at Z 488 (288 over the walkway); visual ceiling 475..523", "ramps": "200 wide, 14 deg, Y 800..1600 each side, on a 100 platform at Y 800..1200"},
        "placement_rules": ["Pivot on the ground; the hull rests 60 cm into it.", "Keep 3 m clear past each ramp toe (Y beyond +-1600, X -25..175).", "Never scale it."],
    }


# ------------------------------------------------------------------------------------------------ outpost
def outpost(variant):
    """The survey outpost: an 800 x 400 kit room on the ground, doorway on +X, windows on both long sides, a console
    against the back (-X) wall facing the door."""
    glow = GLOW_V if variant == "Verdance" else GLOW_C
    g = {"Glow": glow}
    ps = []
    for x in (-200, 200):
        ps.append(piece("Floor_%d" % x, "SM_Kit_Floor_400", x, 0))
        ps.append(piece("Roof_%d" % x, "SM_Kit_Roof_400", x, 0, 0, 0, slots=g))
    ps += [piece("Wall_S_A", "SM_Kit_Wall_400", -200, -200, 0, 90, slots=g), piece("Wall_S_B", "SM_Kit_Wall_400_Window", 200, -200, 0, 90, slots=g),
           piece("Wall_N_A", "SM_Kit_Wall_400_Window", -200, 200, 0, 270, slots=g), piece("Wall_N_B", "SM_Kit_Wall_400", 200, 200, 0, 270, slots=g),
           piece("Wall_Back", "SM_Kit_Wall_400", -400, 0, 0, 0, slots=g), piece("Door", "SM_Kit_DoorFrame_400", 400, 0, 0, 180, slots=g)]
    for (x, y, yaw) in ((400, 200, 0), (400, -200, 270), (-400, -200, 180), (-400, 200, 90)):
        ps.append(piece("Corner_%d_%d" % (x, y), "SM_Kit_Corner", x, y, 0, yaw, slots=g))
    ps.append(piece("Console", "SM_Kit_Console", -360, 0, 0, 0, slots={"Screen": K + "MI_Kit_Screen"}))
    ps.append(piece("Lamp_Door", "SM_Kit_LightStrip_200", 425, 0, 296, 90, slots=g))
    return {
        "site": "Outpost_Site", "actor": "/Game/TriadDrift/Gameplay/Props/Outpost/BP_Outpost_Site_" + variant,
        "about": "Survey outpost rebuilt from the kit: one 800 x 400 room, 300 to the roof, floor flush with the ground, doorway (160 x 260) on +X.",
        "pieces": ps,
        "console_interact": {"stand_at_site_cm": [-250.0, 0.0, 0.0], "pilot_faces_yaw_deg": 180.0, "console_site_cm": [-360.0, 0.0, 0.0],
                             "clear_in_front_cm": 316.0, "moved_from": "(76, 81.5, 153) yaw 180 on the Rodin cabin's bay floor"},
        "walk_path_site_cm": [[700, 0, 0, "ground in front of the door"], [400, 0, 0, "doorway, 160 x 260 clear"], [0, 0, 0, "room centre"], [-250, 0, 0, "console point"]],
        "walkability_cm": {"floor_top_z": 0, "room": {"x": [-390, 390], "y": [-190, 190]}, "door": {"y": [-80, 80], "z": [0, 260]}, "roof_underside_z": 296},
        "placement_rules": ["Put the pivot on flat ground: the floor top IS the ground (the 20 cm slab is buried).", "Keep 3 m clear in front of the door (X 400..700)."],
    }


# ------------------------------------------------------------------------------------------------ Cinder dig site
def digsite():
    """Cinder dig-site cabin (Grok Bot's #38/#69 cabin as the exterior reference): a raised 1200 x 1200 deck (top Z
    200) with a 400 x 800 cabin at its back, a wrap-around walkway with railings, and a straight front stair flight."""
    g = {"Glow": GLOW_C}
    ps = []
    for x in (-400, 0, 400):
        for y in (-400, 0, 400):
            ps.append(piece("Deck_%d_%d" % (x, y), "SM_Kit_Platform_400x200", x, y))
    for y in (-200, 200):
        ps.append(piece("Wall_Back_%d" % y, "SM_Kit_Wall_400", -600, y, 200, 0, slots=g))
        ps.append(piece("Roof_%d" % y, "SM_Kit_Roof_400", -400, y, 200, 0, slots=g))
    ps.append(piece("Wall_Side_S", "SM_Kit_Wall_400_Window", -400, -400, 200, 90, slots=g))
    ps.append(piece("Wall_Side_N", "SM_Kit_Wall_400_Window", -400, 400, 200, 270, slots=g))
    ps.append(piece("Door", "SM_Kit_DoorFrame_400", -200, -200, 200, 180, slots=g))
    ps.append(piece("Wall_Front", "SM_Kit_Wall_400_Window", -200, 200, 200, 180, slots=g))
    for (x, y, yaw) in ((-200, 400, 0), (-200, -400, 270), (-600, -400, 180), (-600, 400, 90)):
        ps.append(piece("Corner_%d_%d" % (x, y), "SM_Kit_Corner", x, y, 200, yaw, slots=g))
    ps.append(piece("Console", "SM_Kit_Console", -560, 0, 200, 0, slots={"Screen": K + "MI_Kit_Screen"}))
    ps.append(piece("Lamp_Door", "SM_Kit_LightStrip_200", -175, -200, 496, 90, slots=g))
    n = 0
    for y in (-400, 400):
        ps.append(piece("Rail_%02d" % n, "SM_Kit_Railing_400", 600, y, 200, 90)); n += 1
    for x in (-400, 0, 400):
        ps.append(piece("Rail_%02d" % n, "SM_Kit_Railing_400", x, 600, 200, 180)); n += 1
        ps.append(piece("Rail_%02d" % n, "SM_Kit_Railing_400", x, -600, 200, 0)); n += 1
    for y in (-400, 400):
        ps.append(piece("Rail_%02d" % n, "SM_Kit_Railing_400", -600, y, 200, 270)); n += 1
    ps.append(piece("Stair_Base", "SM_Kit_Platform_400", 800, 0))
    ps.append(piece("Stairs_Upper", "SM_Kit_Stairs_400", 800, 0, 100, 180))
    ps.append(piece("Stairs_Lower", "SM_Kit_Stairs_400", 1200, 0, 0, 180))
    # Grok Bot's drill rig (PR #69, 7.8 x 4.1 x 11 m, a box collision; import builder, ue_import_engai.py) as dressing
    # beside the cabin, off the +Y side: about 6 m of open ground from the deck edge, nothing in front of the stairs.
    ps.append(piece("DrillRig", "/Game/TriadDrift/Gameplay/Props/CinderDigSite/SM_CinderDrillRig", -100, 1450, 0, -15,
                    note="AI mesh as dressing (R-043): not walkable, its box only keeps the pilot out"))
    return {
        "site": "DigSite_Cabin", "actor": K + "Sites/BP_DigSite_Cabin",
        "about": "Cinder dig-site cabin from the kit: raised deck (top Z 200) 1200 x 1200, cabin 400 x 800 at the back (X -600..-200), walkway round it, front stairs X 600..1400.",
        "pieces": ps,
        "console_interact": {"stand_at_site_cm": [-430.0, 0.0, 200.0], "pilot_faces_yaw_deg": 180.0, "console_site_cm": [-560.0, 0.0, 200.0], "clear_in_front_cm": 300.0},
        "walk_path_site_cm": [[1550, 0, 0, "ground in front of the stairs"], [1200, 0, 50, "lower flight"], [800, 0, 150, "upper flight"], [300, 0, 200, "front deck"],
                              [-200, -200, 200, "cabin doorway (Y -280..-120)"], [-430, 0, 200, "console point"], [0, 500, 200, "side walkway"]],
        "walkability_cm": {"deck_top_z": 200, "cabin": {"x": [-585, -215], "y": [-385, 385]}, "door": {"y": [-280, -120], "z": [200, 460]},
                           "stairs": "two Stairs_400 in one flight, 14 deg, Y -100..100, X 600..1400", "railings": "every deck edge but the stairs opening (Y -200..200)"},
        "placement_rules": ["Pivot on flat ground.", "Keep 3 m clear in front of the stairs (X 1400..1700).",
                            "The drill rig stands at (-100, 1450): the footprint is X -600..1700, Y -600..1700."],
    }


# ------------------------------------------------------------------------------------------------ Dominion interior
def dominion_interior():
    """A preview of T5's course shell (TDDominionInteriorCourse.inl: Room / CorridorRun / Doorway, the same constants)
    built from the Dominion set, for the walk test only. The game builds the real interior in code; it switches to
    these meshes by the folder string /Game/TriadDrift/Gameplay/Props/Kit/Dominion/."""
    KD = K + "Dominion/"
    ps = []
    n = [0]

    def add(mesh, x, y, z=0.0, yaw=0.0):
        n[0] += 1
        ps.append(piece("P%04d_%s" % (n[0], mesh.replace("SM_DI_", "")), KD + mesh, x, y, z, yaw))

    def room(x0, x1, hy, storeys, door0, door1):
        x = x0 + 200
        while x < x1:
            y = -hy + 200
            while y < hy:
                add("SM_DI_Room_Floor", x, y)
                add("SM_DI_Room_Ceiling", x, y, (storeys - 1) * 600.0)
                y += 400
            x += 400
        for s in range(storeys):
            z = s * 600.0
            x = x0 + 200
            while x < x1:
                add("SM_DI_Room_Wall", x, -hy, z, 90)
                add("SM_DI_Room_Wall", x, hy, z, 90)
                x += 400
            y = -hy + 200
            while y < hy:
                middle = s == 0 and abs(y) < 1
                if not (middle and door0):
                    add("SM_DI_Room_Wall", x0, y, z, 0)
                if not (middle and door1):
                    add("SM_DI_Room_Wall", x1, y, z, 0)
                y += 400

    def corridor(x0, x1):
        x = x0 + 200
        while x < x1:
            add("SM_DI_Corridor_Straight", x, 0)
            x += 400
    HX0, HX1, HY = -1200.0, 1200.0, 1400.0
    CX1, AX1, AY, L1X1, WX1, WY, L2X1, KX1, KY = 5200.0, 7600.0, 1400.0, 8400.0, 10400.0, 1000.0, 11200.0, 14800.0, 1800.0
    room(HX0, HX1, HY, 2, False, True)
    add("SM_DI_Hangar_Pad", -200, 0, 0.5)
    corridor(HX1, CX1)
    room(CX1, AX1, AY, 1, True, True)
    corridor(AX1, L1X1)
    room(L1X1, WX1, WY, 1, True, True)
    corridor(WX1, L2X1)
    room(L2X1, KX1, KY, 3, True, False)
    for x in (HX1, CX1, AX1, L1X1, WX1, L2X1):
        add("SM_DI_Doorway", x, 0)
    add("SM_DI_Cover_Low", CX1 - 700, -70)
    return {"site": "DominionInterior_Preview", "actor": KD + "BP_KitD_InteriorPreview",
            "about": "Walk-test preview of T5's interior shell in the Dominion set (not used by the game).",
            "pieces": ps,
            "walk_path_site_cm": [[-600, 0, 0, "hangar start"], [1200, 0, 0, "hangar doorway"], [3200, 0, 0, "deflect corridor"], [6400, 0, 0, "arena"],
                                  [9400, 0, 0, "Warden room"], [13000, 0, 0, "core chamber"]]}


# ------------------------------------------------------------------------------------------------ docking bay
def docking_bay():
    """The bow docking bay's inside (#54): the pocket behind the 5000 x 3500 bay mouth, 6000 deep. Pivot = the centre
    of the bay mouth (hull-local (hull max X, 0, -2000), i.e. the pocket X 24000..30000, Z -3750..-250 on the r1
    hull); the pocket runs toward -X. Flown into, not walked: the Dominion set is placed at scale 2.5 (floor and
    ceiling tiles 10 m, walls 10 m x 17.5 m in two rows), so it reads at ship speed. Collision stays on (BlockAll)."""
    KD = K + "Dominion/"
    S = 2.5
    ps = []
    n = [0]

    def add(mesh, x, y, z, yaw=0.0, scale=(S, S, S)):
        n[0] += 1
        ps.append(piece("B%03d_%s" % (n[0], mesh.replace("SM_DI_", "")), KD + mesh, x, y, z, yaw, scale=list(scale)))
    FZ, CZ, HY, DX = -1750.0, 1750.0, 2500.0, 6000.0
    for i in range(6):                      # floor and ceiling, 10 m tiles
        x = -500.0 - 1000.0 * i
        for j in range(5):
            y = -2000.0 + 1000.0 * j
            add("SM_DI_Room_Floor", x, y, FZ)
            add("SM_DI_Room_Ceiling", x, y, CZ - 600.0 * S)
    wz = (S, S, (CZ - FZ) / 2.0 / 600.0)   # two rows of 1750
    for row in range(2):
        z = FZ + row * (CZ - FZ) / 2.0
        for i in range(6):
            x = -500.0 - 1000.0 * i
            add("SM_DI_Room_Wall", x, -HY, z, 90, wz)
            add("SM_DI_Room_Wall", x, HY, z, 90, wz)
        for j in range(5):
            add("SM_DI_Room_Wall", -DX, -2000.0 + 1000.0 * j, z, 0, wz)
    add("SM_DI_Hangar_Pad", -4600.0, 0.0, FZ + 1.0, 0, (2.0, 2.0, 1.0))
    return {"site": "DockingBay_Interior", "actor": KD + "BP_DockingBay_Interior",
            "about": "The inside of the Dominion's bow docking bay: a 6000 x 5000 x 3500 pocket behind the bay mouth, Dominion set at scale 2.5.",
            "pieces": ps,
            "attach": {"to": "the Dominion hull actor", "hull_local_cm": ["hull max X (30000 on r1)", 0, -2000], "yaw_deg": 0,
                       "note": "the pivot is the centre of the bay mouth; the pocket extends 6000 toward -X. Floor top Z -1750, ceiling underside +1750, walls at Y +-2500 (inner faces +-2425), back wall X -6000. A 24 m landing pad at X -4600 (the glide ends 6000 inside)."},
            "inner_clear_cm": {"x": [-5925, 0], "y": [-2425, 2425], "z": [-1750, 1750]}}


def main():
    out = os.path.join(KIT, "sites")
    os.makedirs(out, exist_ok=True)
    sites = {"Relay_Site": relay(False), "Relay_Site_Awake": relay(True), "Wreck_Site": wreck(),
             "Outpost_Site_Verdance": outpost("Verdance"), "Outpost_Site_Cinder": outpost("Cinder"), "DigSite_Cabin": digsite(),
             "DominionInterior_Preview": dominion_interior(), "DockingBay_Interior": docking_bay()}
    for k, v in sites.items():
        with open(os.path.join(out, k + ".manifest.json"), "w", encoding="utf-8") as fh:
            json.dump(v, fh, indent=1)
    print("wrote", sorted(sites))


if __name__ == "__main__":
    main()

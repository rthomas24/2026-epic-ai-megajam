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
    ps.append(piece("Beacon_Pillar", "SM_Kit_LightPillar", 1250, -750, 300 + 1730, 0, slots={"Pillar": K + "MI_Kit_LightPillar_Verdance"}, scale=[3.0, 3.0, 1.0],
                    note="a 60 m soft light beam (4.2 m wide) from the lamp head, so the site reads from the approach (no Niagara)"))
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
    # r7: a sealed bulkhead leaf on the core's far wall, so Grok Bot's trident decal (on every SM_DI_Door) is seen in place
    add("SM_DI_Door", KX1 - 64 - 16.3, 0)
    # r8: the reactor at T5's CoreAt ((L2X1 + KX1) / 2, 0, 0) with its rings at +600, DominionInterior's own pieces
    # and materials (the core glows), so the core chamber is lit by its reactor and not by work lights
    DI = "/Game/TriadDrift/Gameplay/Props/DominionInterior/"
    core_x = (L2X1 + KX1) * 0.5
    n[0] += 1
    ps.append(piece("P%04d_Core" % n[0], DI + "SM_DI_Core", core_x, 0, 0, 0))
    for rn in ("SM_DI_Core_Ring_A", "SM_DI_Core_Ring_B"):
        n[0] += 1
        ps.append(piece("P%04d_%s" % (n[0], rn.replace("SM_DI_", "")), DI + rn, core_x, 0, 600, 0))
    # r5: work lights that really light (point lights, no shadows), so the rooms read without the course's lights:
    # a grid in every room under the ceiling, one per 800 cm of corridor. Cool white (the red strips stay ambience).
    lights = []

    def grid(x0, x1, hy, z, step=800.0):
        x = x0 + step / 2
        while x < x1:
            y = -hy + step / 2
            while y < hy:
                lights.append({"name": "L%03d" % len(lights), "location_cm": [x, y, z], "intensity": 1800.0, "radius": 1600.0,
                               "color": [0.8, 0.88, 1.0]})
                y += step
            x += step
    grid(HX0, HX1, HY, 1100.0)
    grid(CX1, AX1, AY, 540.0)
    grid(L1X1, WX1, WY, 540.0)
    for lt in lights[9:]:
        lt["intensity"] = 900.0          # arena and Warden: half (review r2: washed out)
    grid(L2X1, KX1, KY, 1650.0, 1200.0)
    for x in [HX1 + 400 + 800 * k for k in range(5)] + [AX1 + 400, WX1 + 400]:
        lights.append({"name": "L%03d" % len(lights), "location_cm": [x, 0.0, 540.0], "intensity": 900.0, "radius": 1000.0, "color": [0.8, 0.88, 1.0]})
    # r8 (review r3, Lead): the white work lights at 35 % and slightly warm; the strips keep their budget
    for lt in lights:
        lt["intensity"] = round(lt["intensity"] * 0.35, 1)
        lt["color"] = [1.0, 0.9, 0.78]
    core_x = (L2X1 + KX1) * 0.5
    lights.append({"name": "L%03d_Reactor" % len(lights), "location_cm": [core_x, 0.0, 1500.0], "intensity": 3600.0, "radius": 2600.0,
                   "color": [1.0, 0.32, 0.16]})          # r9: -40 %, deeper
    for yy in (-900.0, 900.0):
        lights.append({"name": "L%03d_ReactorFill" % len(lights), "location_cm": [core_x, yy, 300.0], "intensity": 900.0,
                       "radius": 1400.0, "color": [0.75, 0.03, 0.015]})
    lights.append({"name": "L%03d_Bulkhead" % len(lights), "location_cm": [KX1 - 64 - 180.0, 0.0, 470.0], "intensity": 350.0,
                   "radius": 700.0, "color": [1.0, 0.05, 0.03]})   # r9: lower so the door shape reads
    return {"site": "DominionInterior_Preview", "lights": lights, "actor": KD + "BP_KitD_InteriorPreview",
            "about": "Walk-test preview of T5's interior shell in the Dominion set (not used by the game).",
            "pieces": ps,
            "walk_path_site_cm": [[-600, 0, 0, "hangar start"], [1200, 0, 0, "hangar doorway"], [3200, 0, 0, "deflect corridor"], [6400, 0, 0, "arena"],
                                  [9400, 0, 0, "Warden room"], [13000, 0, 0, "core chamber"]]}


# ------------------------------------------------------------------------------------------------ docking bay
# r5: Grok Bot's measured plan (PR #77, ArtSource/EngAI/Props/DominionDockingBay/plans_r1/dockbay_layout.json: 799 kit
# placements on the 400 grid in T5's course frame) replaces my r4 scale-2.5 shell: it is walkable at scale 1, joins
# the hangar, has the pads and the gantry, and its pocket matches the r2 hull exactly (hull = (22800 - x, -y, z - 3750)).
# The pieces it names come from the kit's Dominion set where the set has them, else from DominionInterior.
DOMINION_SET = ("SM_DI_Room_Floor", "SM_DI_Room_Ceiling", "SM_DI_Room_Wall", "SM_DI_Corridor_Straight", "SM_DI_Doorway",
                "SM_DI_Door", "SM_DI_Cover_Low", "SM_DI_Hangar_Pad")
BAY_PLAN = os.path.join(os.path.dirname(os.path.dirname(KIT)), "EngAI", "Props", "DominionDockingBay", "plans_r1", "dockbay_layout.json")


def _bay_pieces(to_hull):
    with open(BAY_PLAN, encoding="utf-8") as fh:
        plan = json.load(fh)
    ps = []
    for i, pl in enumerate(plan["placements"]):
        if pl["group"] == "hangar":
            continue                     # T5's hangar, built by the course
        if pl["group"] == "pads" and abs(pl["x"] + 200) < 1 and abs(pl["y"]) < 1:
            continue                     # pad A is T5's, in the hangar
        mesh = pl["mesh"]
        path = (K + "Dominion/" + mesh) if mesh in DOMINION_SET else ("/Game/TriadDrift/Gameplay/Props/DominionInterior/" + mesh)
        x, y, z, yaw = float(pl["x"]), float(pl["y"]), float(pl["z"]), float(pl.get("yaw", 0.0))
        if to_hull:                      # pivot = SOCKET_Bay (hull 30000, 0, -2000), +X out of the mouth
            x, y, z, yaw = -7200.0 - x, -y, z - 1750.0, (yaw + 180.0) % 360.0
        ps.append(piece("%s_%03d_%s" % (pl["group"], i, mesh.replace("SM_DI_", "")), path, x, y, z, yaw))
    # work lights under the pocket's ceiling (r5): a 1200 grid at 3300, cool white, no shadows
    lights = []
    for x in (-6600.0, -5400.0, -4200.0, -3000.0, -1800.0):
        for y in (-1800.0, -600.0, 600.0, 1800.0):
            lx, ly, lz = (-7200.0 - x, -y, 3300.0 - 1750.0) if to_hull else (x, y, 3300.0)
            lights.append({"name": "BL%02d" % len(lights), "location_cm": [lx, ly, lz], "intensity": 6000.0, "radius": 3000.0, "color": [0.8, 0.88, 1.0]})
    # a low red fill along both pocket walls so the lower storeys and the mouth are not dead black (review r2)
    for x in (-6400.0, -4400.0, -2400.0):
        for y in (-2300.0, 2300.0):
            lx, ly, lz = (-7200.0 - x, -y, 250.0 - 1750.0) if to_hull else (x, y, 250.0)
            lights.append({"name": "BR%02d" % len(lights), "location_cm": [lx, ly, lz], "intensity": 600.0, "radius": 1800.0, "color": [1.0, 0.12, 0.05]})
    return plan, ps, lights


def docking_bay():
    """The bow docking bay's inside for the hull (space): pivot = the hull's SOCKET_Bay, the pocket toward -X."""
    plan, ps, lights = _bay_pieces(True)
    return {"site": "DockingBay_Interior", "actor": K + "Dominion/BP_DockingBay_Interior",
            "about": "The Dominion bow docking bay's pocket from Grok Bot's measured plan r1 (6000 deep x 5200 x 3600, six storeys of the Dominion set at scale 1, two dressing pads, the gantry).",
            "pieces": ps, "lights": lights,
            "attach": {"to": "the Dominion hull mesh's SOCKET_Bay (hull-local (30000, 0, -2000)), no extra rotation",
                       "note": "the pocket extends 6000 toward -X; floor top Z -1750 (the mouth sill), 3600 clear; the doors and tractor emitter are Grok Bot's exterior meshes (T4); ATDDominion spawns ATDDockingBay at (Bounds.Max.X, 0, ...) = (33400, 0, -2933) on hull r2, 34 m in front of the mouth: use SOCKET_Bay (plan SOURCE.md, mismatch 1)"},
            "plan": {"source": "ArtSource/EngAI/Props/DominionDockingBay/plans_r1", "pocket": plan["pocket"], "mouth": plan["mouth"]}}


def docking_bay_course():
    """The same pocket in T5's interior course frame (L_Dominion_Interior): place at the course origin, yaw 0. It opens
    onto the hangar once the course drops the hangar's X0 end wall (plan: hangar_opening 2800 x 1200)."""
    plan, ps, lights = _bay_pieces(False)
    return {"site": "DockingBay_Course", "actor": K + "Dominion/BP_DockingBay_Course",
            "about": "Grok Bot's docking bay pocket (plan r1) in T5's course frame: x -7200..-1200, floor z 0; spawn at the course origin.",
            "pieces": ps, "lights": lights,
            "for_T5": {"place_at": "the course actor's transform", "hangar_change": plan["hangar"]["note"], "force_field": plan["force_field"],
                       "glide_end_recommended": plan["glide"]["end_recommended"], "pads_B_C": [plan["pads"]["B"], plan["pads"]["C"]]}}


def docking_bay_galaxy_test():
    """Capture only: the bay turned yaw 90 (the Dominion's heading in L_Galaxy: at (0, -520000, 20000) facing the combat
    zone centre), so it can be spawned with no rotation at the hull's Bay socket. The hull's -2.2 deg pitch is ignored."""
    import math
    d = docking_bay()
    for lt in d.get("lights", []):
        x, y, z = lt["location_cm"]
        lt["location_cm"] = [-y, x, z]
    for pc in d["pieces"]:
        x, y, z = pc["location_cm"]
        pc["location_cm"] = [-y, x, z]
        pc["yaw_deg"] = (pc.get("yaw_deg", 0.0) + 90.0) % 360.0
    d["site"] = "DockingBay_GalaxyTest"
    d["actor"] = K + "Dominion/BP_DockingBay_GalaxyTest"
    return d


def main():
    out = os.path.join(KIT, "sites")
    os.makedirs(out, exist_ok=True)
    sites = {"Relay_Site": relay(False), "Relay_Site_Awake": relay(True), "Wreck_Site": wreck(),
             "Outpost_Site_Verdance": outpost("Verdance"), "Outpost_Site_Cinder": outpost("Cinder"), "DigSite_Cabin": digsite(),
             "DominionInterior_Preview": dominion_interior(), "DockingBay_Interior": docking_bay(),
             "DockingBay_GalaxyTest": docking_bay_galaxy_test(), "DockingBay_Course": docking_bay_course()}
    for k, v in sites.items():
        with open(os.path.join(out, k + ".manifest.json"), "w", encoding="utf-8") as fh:
            json.dump(v, fh, indent=1)
    print("wrote", sorted(sites))


if __name__ == "__main__":
    main()

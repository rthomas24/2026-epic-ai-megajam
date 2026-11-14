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
    """Verdance objective 2 (Ryan, Nov 13 evening): a 60-90 s data-transfer DEFENCE at the relay, three waves. A raised
    1600 x 1600 kit deck (top Z 100) round the sunk Rodin relay, three ramps up (+X front, +Y, -Y), railings on the
    other edges, six low covers, the device console in front of the ring, open sightlines (no posts)."""
    R = "/Game/TriadDrift/Gameplay/Props/Relay/"
    ps = [piece("Relay", R + "SM_Relay", 0, 0, -50, 0, materials={"0": R + ("MI_Relay_Awake" if awake else "MI_Relay")},
                note="Grok Bot's Rodin relay as dressing, sunk 50 cm so its own plinth, court, landing and stairs (tops 85..96 after the sink) sit under the kit floor at 100. Its ring, cross and pylon hulls still block the pawn.")]
    for x in (-600, -200, 200, 600):
        for y in (-600, -200, 200, 600):
            ps.append(piece("Deck_%d_%d" % (x, y), "SM_Kit_Platform_400", x, y))
    ps.append(piece("Ramp_Front", "SM_Kit_Ramp_400", 1000, 0, 0, 180))       # +X, rises toward -X
    ps.append(piece("Ramp_North", "SM_Kit_Ramp_400", 200, 1000, 0, 270))     # +Y, rises toward -Y
    ps.append(piece("Ramp_South", "SM_Kit_Ramp_400", 200, -1000, 0, 90))     # -Y, rises toward +Y
    n = 0
    rails = [(800, -600, 90), (800, 600, 90),                                   # +X edge, opening Y -400..400
             (-800, -600, 270), (-800, -200, 270), (-800, 200, 270), (-800, 600, 270),
             (-600, 800, 180), (-200, 800, 180), (600, 800, 180),               # +Y edge, opening X 0..400
             (-600, -800, 0), (-200, -800, 0), (600, -800, 0)]                  # -Y edge, opening X 0..400
    for (x, y, yaw) in rails:
        ps.append(piece("Rail_%02d" % n, "SM_Kit_Railing_400", x, y, 100, yaw)); n += 1
    ps.append(piece("Device_Console", "SM_Kit_Console", 80, 0, 100, 0, slots={"Screen": K + "MI_Kit_Screen"},
                    note="the transfer device sits on this console; the pilot works it from +X"))
    # six low covers; the three ramp heads (front Y -100..100 at X 800, north / south X 100..300) stay clear
    covers = [(420, -320, 0), (420, 320, 0), (560, -250, 90), (560, 250, 90), (-300, -560, 0), (-300, 560, 0)]
    for i, (x, y, yaw) in enumerate(covers):
        ps.append(piece("Cover_%d" % i, "SM_Kit_Cover_200", x, y, 100, yaw))
    return {
        "site": "Relay_Site", "actor": R + ("BP_Relay_Awake" if awake else "BP_Relay"),
        "about": "Verdance relay defence arena: kit deck 1600 x 1600, top Z 100, over the sunk Rodin relay (ring faces +X); ramps +X, +Y, -Y.",
        "pieces": ps,
        "device_interact": {"stand_at_site_cm": [190.0, 0.0, 100.0], "pilot_faces_yaw_deg": 180.0, "console_site_cm": [80.0, 0.0, 100.0],
                            "device_on_console_site_cm": [95.0, 0.0, 206.0], "clear_in_front_cm": 300.0},
        "wake_interact": {"stand_at_site_cm": [190.0, 0.0, 100.0], "pilot_faces_yaw_deg": 180.0, "look_at_core_site_cm": [-190.0, -2.0, 443.0],
                          "moved_from": "(150, -2, 135) on the r2 Rodin court"},
        "arena_cm": {"deck": {"x": [-800, 800], "y": [-800, 800], "z": 100},
                     "ramps": {"front": "X 800..1200, Y -100..100", "north": "X 100..300, Y 800..1200", "south": "X 100..300, Y -1200..-800"},
                     "cover_low_100_high": [[x, y, 100, yaw] for (x, y, yaw) in covers],
                     "suggested_enemy_spawns_ground": [[2200, 0, 0], [1600, 1800, 0], [1600, -1800, 0], [-1800, 1600, 0], [-1800, -1600, 0]],
                     "blocked_on_deck": "the ring and its lower cross arm (X -316..-67), three pylons (X 190..309 Y 152..266; X 190..305 Y -395..-286; X -451..-318 Y 87..278)"},
        "walk_path_site_cm": [[1350, 0, 0, "ground in front of the front ramp"], [1000, 0, 50, "front ramp"], [500, 0, 100, "deck"],
                              [190, 0, 100, "device point"], [200, 1350, 0, "foot of the north ramp"], [200, -1350, 0, "foot of the south ramp"]],
        "placement_rules": ["Pivot on flat ground (the deck is flat 0..100, no piles).", "Keep 3 m clear beyond each ramp toe.",
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


def main():
    out = os.path.join(KIT, "sites")
    os.makedirs(out, exist_ok=True)
    sites = {"Relay_Site": relay(False), "Relay_Site_Awake": relay(True), "Wreck_Site": wreck(),
             "Outpost_Site_Verdance": outpost("Verdance"), "Outpost_Site_Cinder": outpost("Cinder"), "DigSite_Cabin": digsite()}
    for k, v in sites.items():
        with open(os.path.join(out, k + ".manifest.json"), "w", encoding="utf-8") as fh:
            json.dump(v, fh, indent=1)
    print("wrote", sorted(sites))


if __name__ == "__main__":
    main()

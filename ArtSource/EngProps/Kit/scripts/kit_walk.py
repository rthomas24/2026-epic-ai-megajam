# SPDX-License-Identifier: MIT
# Kit - in-game walk test of one site with the real pawn (system Python, drives a -game session started with
# Tools/td-play-mcp.ps1 through ../../Outpost/scripts/op_game.py's helpers).
#   python kit_walk.py <port> <Site> <ox> <oy> <oz>
# Spawns the site's actor class at (ox, oy, oz) (transient, nothing saved), then runs that site's script below:
# teleports, real key presses (TD.Key W), positions after every step (feet = capsule centre - 90), third-person
# captures at 1280 x 720 into review/game/<Site>/. Writes review/game/<Site>/walk.json.
import importlib.util
import json
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
PORT, SITE = sys.argv[1], sys.argv[2]
O = [float(v) for v in sys.argv[3:6]]
OUT = os.path.join(KIT, "review", "game", SITE)
os.makedirs(OUT, exist_ok=True)
os.environ["TD_SHOT_DIR"] = OUT
sys.argv = [sys.argv[0], PORT, "noop"]
spec = importlib.util.spec_from_file_location("op_game", os.path.join(os.path.dirname(KIT), "Outpost", "scripts", "op_game.py"))
G = importlib.util.module_from_spec(spec)
try:
    spec.loader.exec_module(G)        # its CLI block sees "noop" and does nothing
except Exception:
    pass
with open(os.path.join(KIT, "sites", SITE + ".manifest.json"), encoding="utf-8") as fh:
    MAN = json.load(fh)
log = []


def W(x, y, z):
    return O[0] + x, O[1] + y, O[2] + z


def feet():
    p = G.where()
    if not isinstance(p, list):
        return None
    return [round(p[0] - O[0]), round(p[1] - O[1]), round(p[2] - 90 - O[2]), round(p[3])]


def tp(x, y, z, yaw, pitch=-10, settle=1.5):
    wx, wy, wz = W(x, y, z + 92)
    G.cmd("TD.Ground.Teleport %.0f %.0f %.0f" % (wx, wy, wz))
    G.cmd("TD.Look %.1f %.1f" % (yaw, pitch))
    time.sleep(settle)
    f = feet()
    log.append({"teleport": [x, y, z], "feet": f})
    return f


def walk(yaw, secs, label=""):
    G.cmd("TD.Look %.1f -8" % yaw)
    G.cmd("TD.Key W %.2f" % secs)
    time.sleep(secs + 0.7)
    f = feet()
    log.append({"walk": [yaw, secs], "feet": f, "at": label})
    print("walk %s yaw %.0f %.1fs -> feet %s" % (label, yaw, secs, f))
    return f


def shot(name, yaw=None, pitch=None):
    if yaw is not None:
        G.cmd("TD.Look %.1f %.1f" % (yaw, pitch if pitch is not None else -10))
        time.sleep(0.8)
    p = G.shot(name)
    log.append({"shot": p})
    print("shot", p)
    return p


def relay():
    tp(-1800, -800, 0, 0)
    shot("01_west_ramp", 0, -6)
    for i in range(3):
        walk(0, 0.45, "west ramp onto the deck %d" % i)
    tp(-890, 0, 50, 180, settle=2.0)
    shot("02_device_point", 180, -8)
    tp(-890, 0, 50, 0, settle=1.0)
    shot("03_device_point_view_east", 0, -4)
    tp(-600, 1800, 0, 270)
    for i in range(3):
        walk(270, 0.45, "north ramp %d" % i)
    tp(-600, -1800, 0, 90)
    for i in range(3):
        walk(90, 0.45, "south ramp %d" % i)
    tp(-200, 0, 50, 0)
    for i in range(4):
        walk(0, 0.6, "into the relay plinth (must stay on the deck) %d" % i)
    tp(-1700, -1700, 600, 45, settle=0.4)
    shot("04_deck_overview", 45, -25)
    tp(-2900, -1900, 0, 33, settle=2.0)
    shot("05_15m", 33, 6)
    G.cmd("TD.Ground.ThirdPerson 0")
    tp(-29000, 0, 5800, 0, pitch=-8, settle=0.3)
    shot("06_300m", 0, -8)
    G.cmd("TD.Ground.ThirdPerson 1")


def dominion():
    tp(-600, 0, 0, 0)
    shot("01_hangar", 0, -4)
    xs = []
    for i in range(30):
        f = walk(0, 0.8, "east %d" % i)
        if f:
            xs.append(f[0])
        if len(xs) > 2 and abs(xs[-1] - xs[-2]) < 5:
            break
    for (name, x) in (("02_corridor", 3000), ("03_arena", 6200), ("04_warden", 9200), ("05_core", 12000)):
        tp(x, 0, 0, 0, settle=1.5)
        shot(name, 0, 2)
    tp(1100, -600, 0, 330, settle=1.5)
    shot("06_hangar_doorway", 20, 0)
    tp(13600, 300, 0, 0, settle=1.5)
    shot("07_core_bulkhead_trident", 0, 4)
    tp(1700, 0, 0, 0, settle=1.5)
    shot("08_corridor_route_arrows", 0, -18)


def dock():
    """The bay is spawned 1750 up so its floor is the ground; the pilot stands in it for scale."""
    tp(1800, 0, -1750, 180, settle=2.0)
    shot("01_mouth_from_outside", 180, 4)
    tp(-1000, 0, -1750, 180, settle=2.0)
    shot("02_inside_to_the_back", 180, 8)
    tp(-5200, 1500, -1750, 15, settle=2.0)
    shot("03_inside_to_the_mouth", 15, 6)


def wreck():
    tp(75, -1800, 0, 90)
    shot("01_ramp_foot", 90, -8)
    for i in range(10):
        walk(90, 0.45, "through %d" % i)
    tp(75, -600, 200, 90, settle=2.0)
    shot("02_walkway_into_hull", 90, -5)
    tp(0, 0, 200, 0, settle=2.0)
    shot("03_inside_console", 0, -5)
    tp(75, 150, 200, 270, settle=2.0)
    shot("04_inside_back", 270, -5)
    tp(1700, -2100, 0, 128, settle=2.0)
    shot("05_15m", 128, 4)


def outpost():
    tp(800, 0, 0, 180)
    shot("01_door", 180, -6)
    for i in range(4):
        walk(180, 0.45, "in %d" % i)
    tp(-250, 0, 0, 180, settle=2.0)
    shot("02_console", 180, -8)
    tp(300, 120, 0, 200, settle=2.0)
    shot("03_room", 200, -6)
    tp(1500, -900, 0, 148, settle=2.0)
    shot("04_15m", 148, 2)


def digsite():
    tp(1750, 0, 0, 180)
    shot("01_stairs_foot", 180, -10)
    for i in range(6):
        walk(180, 0.45, "stairs %d" % i)
    tp(300, -200, 200, 180, settle=2.0)
    for i in range(3):
        walk(180, 0.45, "door %d" % i)
    tp(-430, 0, 200, 180, settle=2.0)
    shot("02_console", 180, -8)
    tp(0, 500, 200, 180, settle=2.0)
    shot("03_side_walkway", 180, -8)
    for i in range(4):
        walk(180, 0.45, "walkway to the back rail %d" % i)
    tp(2200, -1300, 0, 148, settle=2.0)
    shot("04_15m", 148, 4)


def survey():
    tp(-450, 350, 0, -25, settle=2.0)
    shot("01_line", -25, -6)
    tp(1200 - 250, -120, 0, 25, settle=2.0)
    shot("02_tripod_close", 25, -12)
    tp(300, -500, 0, 90)                       # walk across the line between the probe (0) and the pod (600)
    for i in range(5):
        walk(90, 0.45, "across %d" % i)
    tp(1200, -400, 0, 90)                      # walk straight into the tripod: the hull stops the pilot, no snag
    for i in range(3):
        walk(90, 0.45, "into tripod %d" % i)


def artefact_site():
    tp(-450, -150, 0, 10, settle=2.0)
    shot("01_close", 10, 8)
    tp(-1800, -900, 0, 27, settle=2.0)
    shot("02_20m", 27, 4)
    tp(-400, 0, 0, 0)
    for i in range(3):
        walk(0, 0.45, "into artefact %d" % i)
    tp(400, 300, 0, 200, settle=2.0)
    shot("03_back", 200, 6)


def story_set():
    """Story sets: 5 m and 30 m from the set, first person from the pilot's eye height, looking at it."""
    G.cmd("TD.Ground.ThirdPerson 0")
    tp(-700.0, -650.0, 0.0, 40.0, settle=2.0)
    shot("01_5m", 40.0, -6.0)
    tp(-950.0, -880.0, 0.0, 40.0, settle=2.0)
    shot("01b_8m", 40.0, -5.0)
    tp(-2600.0, -1900.0, 0.0, 37.0, settle=2.0)
    shot("02_30m", 37.0, -2.0)
    G.cmd("TD.Ground.ThirdPerson 1")


def bridge():
    """Cinder magma bridge: from 2 m before the -X ramp foot, walk the whole span with real key presses (feet logged:
    the deck must read 50, the ramps 0..50), push into each rail, then frames at the foot, mid-deck and 15 m."""
    G.cmd("TD.Ground.ThirdPerson 1")
    tp(-1600.0, 0.0, 0.0, 0.0, settle=2.0)
    shot("01_ramp_foot", 0.0, -8.0)
    for i in range(14):
        walk(0.0, 0.6, "span %d" % i)
    tp(0.0, 0.0, 50.0, 90.0, settle=1.5)
    for i in range(2):
        walk(90.0, 0.6, "into the +Y rail %d (must stay on the deck)" % i)
    tp(0.0, 0.0, 50.0, 270.0, settle=1.5)
    for i in range(2):
        walk(270.0, 0.6, "into the -Y rail %d (must stay on the deck)" % i)
    tp(0.0, 0.0, 50.0, 0.0, settle=1.5)
    shot("02_mid_deck", 0.0, -10.0)
    tp(-1500.0, -1100.0, 0.0, 36.0, settle=2.0)
    shot("03_15m", 36.0, -4.0)


def station():
    """ART-17 survey station: up the ramp(s) from the foot to the console, real key presses, feet logged."""
    foot = MAN["stair_foot_site_cm"]
    stand = MAN["console_stand_site_cm"][0]
    G.cmd("TD.Ground.ThirdPerson 1")
    tp(foot[0] + 250.0, 0.0, 0.0, 180.0, settle=2.0)
    shot("01_foot", 180.0, -6.0)
    for i in range(10):
        walk(180.0, 0.45, "up %d" % i)
    tp(stand[0], stand[1], stand[2], stand[3], settle=2.0)
    shot("02_console", stand[3], -10.0)
    tp(foot[0] + 900.0, -700.0, 0.0, 150.0, settle=2.0)
    shot("03_10m", 150.0, -4.0)


def main():
    G.cmd("TD.Ground.ThirdPerson 1")
    G.cmd("TD.God 1")
    cls = MAN["actor"]
    G.cmd("TD.Dev.SpawnClass %s KitWalk %.0f %.0f %.0f" % (cls, O[0], O[1], O[2]))
    time.sleep(3.0)
    print(G.log_tail("TDDevSpawn", 1))
    {"DockingBay_Interior": dock, "DominionInterior_Preview": dominion, "Relay_Site": relay, "Relay_Site_Awake": relay, "Wreck_Site": wreck, "Outpost_Site_Verdance": outpost,
     "Outpost_Site_Cinder": outpost, "DigSite_Cabin": digsite,
     "SurveyLine_Verdance": survey, "Artefact": artefact_site,
     "StorySet_VerdanceCamp": story_set, "StorySet_CinderMine": story_set,
     "SurveyStation_Verdance": station, "SurveyStation_Cinder": station, "Bridge_Magma": bridge}[SITE]()
    G.cmd("TD.Input.ReleaseAll")
    with open(os.path.join(OUT, "walk.json"), "w", encoding="utf-8") as fh:
        json.dump({"site": SITE, "origin": O, "steps": log}, fh, indent=1)


main()

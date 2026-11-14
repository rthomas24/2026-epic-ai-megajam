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
    tp(1450, 0, 0, 180)
    shot("01_front_ramp_foot", 180, -10)
    for i in range(5):
        walk(180, 0.45, "front ramp / deck %d" % i)
    tp(190, 0, 100, 180, settle=2.0)
    shot("02_device_point", 180, 15)
    tp(200, 1450, 0, 270)
    for i in range(4):
        walk(270, 0.45, "north ramp %d" % i)
    tp(200, -1450, 0, 90)
    for i in range(4):
        walk(90, 0.45, "south ramp %d" % i)
    tp(700, 700, 100, 225, settle=2.0)
    shot("03_deck_covers", 225, -12)
    tp(420, -460, 100, 90, settle=2.0)
    walk(90, 0.4, "into cover (blocked)")
    shot("04_behind_cover", 90, -5)
    tp(2000, -1400, 0, 145, settle=2.0)
    shot("05_15m", 145, 3)
    G.cmd("TD.Ground.ThirdPerson 0")
    tp(29000, 0, 5800, 180, pitch=-11, settle=0.3)
    shot("06_300m", 180, -11)
    G.cmd("TD.Ground.ThirdPerson 1")


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


def main():
    G.cmd("TD.Ground.ThirdPerson 1")
    G.cmd("TD.God 1")
    cls = MAN["actor"]
    G.cmd("TD.Dev.SpawnClass %s KitWalk %.0f %.0f %.0f" % (cls, O[0], O[1], O[2]))
    time.sleep(3.0)
    print(G.log_tail("TDDevSpawn", 1))
    {"Relay_Site": relay, "Relay_Site_Awake": relay, "Wreck_Site": wreck, "Outpost_Site_Verdance": outpost,
     "Outpost_Site_Cinder": outpost, "DigSite_Cabin": digsite}[SITE]()
    G.cmd("TD.Input.ReleaseAll")
    with open(os.path.join(OUT, "walk.json"), "w", encoding="utf-8") as fh:
        json.dump({"site": SITE, "origin": O, "steps": log}, fh, indent=1)


main()

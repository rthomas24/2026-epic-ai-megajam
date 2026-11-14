# SPDX-License-Identifier: MIT
# Crew readability captures (Lead, Nov 13 night): a Grunt and a Trooper (and the Warden once) in front of the pilot at
# 5 / 15 / 30 m in the Dominion interior preview (reactor room, deflect corridor, arena) or outdoors on Verdance at dusk.
#   python kit_crew_read.py <port> <tag> interior     (L_Dominion_Interior; spawns BP_KitD_InteriorPreview at y +30000)
#   python kit_crew_read.py <port> <tag> verdance     (after TD.Stage VerdanceOnFoot)
# Writes review/game/CrewRead/<tag>_<place>_<d>m.png. The crew AI is frozen with slomo for the shot.
import importlib.util
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
PORT, TAG, WHERE = sys.argv[1], sys.argv[2], sys.argv[3]
OUT = os.path.join(KIT, "review", "game", "CrewRead")
os.makedirs(OUT, exist_ok=True)
os.environ["TD_SHOT_DIR"] = OUT
sys.argv = [sys.argv[0], PORT, "noop"]
spec = importlib.util.spec_from_file_location("op_game", os.path.join(os.path.dirname(KIT), "Outpost", "scripts", "op_game.py"))
G = importlib.util.module_from_spec(spec)
try:
    spec.loader.exec_module(G)
except SystemExit:
    pass

OY = 30000.0
# place: (pilot x, pilot y, yaw toward the crew, frame offset degrees per distance)
PLACES = {
    "interior": {"reactor": (11650.0, OY + 1300.0, 0.0), "corridor": (1550.0, OY + 60.0, 0.0), "arena": (5450.0, OY - 1150.0, 50.0)},
    "verdance": {"dusk": (600.0, -1600.0, 0.0)},
}
DIST = {"reactor": (500, 1500, 2700), "corridor": (500, 1500, 2900), "arena": (500, 1500, 2900), "dusk": (500, 1500, 3000)}


def clear():
    for c in ("KillAll TDCrewCharacter", "DestroyAll TDCrewCharacter"):
        G.cmd(c)
    time.sleep(0.3)


def frame(place, x, y, yaw, d, warden=False):
    G.cmd("slomo 1")
    clear()
    G.cmd("TD.Ground.Teleport %.0f %.0f 92" % (x, y))
    off = 3.0 if d >= 2500 else (5.0 if d >= 1500 else 14.0)
    G.cmd("TD.Look %.1f 0" % (yaw - off))
    time.sleep(0.5)
    G.cmd("TD.Ground.Spawn %s %d" % ("Warden" if warden else "Grunt", d))
    G.cmd("TD.Look %.1f 0" % (yaw + off))
    time.sleep(0.25)
    G.cmd("TD.Ground.Spawn Trooper %d" % d)
    G.cmd("TD.Look %.1f 1" % yaw)
    time.sleep(0.25)
    G.cmd("slomo 0.02")
    time.sleep(1.2)
    name = "%s_%s_%s%dm" % (TAG, place, "warden_" if warden else "", round(d / 100))
    print(G.shot(name))


def main():
    G.cmd("TD.Ground.ThirdPerson 1")
    G.cmd("TD.God 1")
    if WHERE == "interior":
        G.cmd("TD.Dev.SpawnClass /Game/TriadDrift/Gameplay/Props/Kit/Dominion/BP_KitD_InteriorPreview Preview 0 %.0f 0" % OY)
        time.sleep(4.0)
    # warm-up: one of each type in view so their shaders are compiled before the first capture (a fresh material
    # renders with the default grey material while "Preparing Shaders" runs)
    x, y, yaw = list(PLACES[WHERE].values())[0]
    G.cmd("TD.Ground.Teleport %.0f %.0f 92" % (x, y))
    G.cmd("TD.Look %.1f 0" % yaw)
    for t in ("Grunt", "Trooper", "Heavy", "Warden"):
        G.cmd("TD.Ground.Spawn %s 4000" % t)
    time.sleep(15.0)
    for place, (x, y, yaw) in PLACES[WHERE].items():
        for d in DIST[place]:
            frame(place, x, y, yaw, d)
    x, y, yaw = list(PLACES[WHERE].values())[0]
    frame(list(PLACES[WHERE])[0], x, y, yaw, 1500, warden=True)
    G.cmd("slomo 1")
    clear()


main()

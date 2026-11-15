# SPDX-License-Identifier: MIT
# T7's Cinder blockers: walk the outpost doorway (course (3300, 0), door on the course's -X side: the outpost is placed
# at (3700, 0) yaw 180) and the magma bridge's near ramp (foot about (4449, 3823), toe (4251, 3686)) with real key
# presses, in both directions, feet logged; a frame at each. python struct_cinder_walk.py <port>
import importlib.util
import json
import math
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
PORT = sys.argv[1]
OUT = os.path.join(KIT, "review", "game", "CinderWalkT7")
os.environ["TD_SHOT_DIR"] = OUT
os.makedirs(OUT, exist_ok=True)
sys.argv = [sys.argv[0], PORT, "noop"]
spec = importlib.util.spec_from_file_location("op_game", os.path.join(os.path.dirname(KIT), "Outpost", "scripts", "op_game.py"))
G = importlib.util.module_from_spec(spec)
try:
    spec.loader.exec_module(G)
except SystemExit:
    pass
log = []


def feet():
    p = G.where()
    return [round(p[0]), round(p[1]), round(p[2] - 90), round(p[3])] if isinstance(p, list) else None


def tp(x, y, yaw):
    G.cmd("TD.Ground.Teleport %.0f %.0f %.0f" % (x, y, 400.0))     # drop onto whatever ground is there
    G.cmd("TD.Look %.1f -6" % yaw)
    time.sleep(2.5)
    f = feet()
    log.append({"teleport": [x, y], "feet": f})
    print("start", f)
    return f


def walk_to(tx, ty, steps, label):
    path = []
    for i in range(steps):
        f = feet()
        if not f:
            break
        if math.hypot(tx - f[0], ty - f[1]) < 80.0:
            break
        yaw = math.degrees(math.atan2(ty - f[1], tx - f[0]))
        G.cmd("TD.Look %.1f -6" % yaw)
        G.cmd("TD.Key W 0.5")
        time.sleep(1.1)
        g = feet()
        path.append(g)
        print("%s %d -> %s" % (label, i, g))
    log.append({"walk": label, "path": path})
    return path


G.cmd("TD.God 1")
G.cmd("DisableAllScreenMessages")
G.cmd("TD.Stage CinderOnFoot")
time.sleep(28.0)
G.cmd("TD.Ground.ThirdPerson 1")
# 1. the outpost door: outside (2700, 0) -> in through (3300, 0) -> room centre (3700, 0), then back out
tp(2700.0, 0.0, 0.0)
walk_to(3700.0, 0.0, 14, "door in")
print(G.shot("door_inside"))
walk_to(2700.0, 0.0, 14, "door out")
G.cmd("TD.Look 0 -4")
time.sleep(0.8)
print(G.shot("door_from_outside"))
# 2. the bridge's near ramp: from 6 m before the toe up onto the deck, then back down
YAW = 34.7
ax, ay = math.cos(math.radians(YAW)), math.sin(math.radians(YAW))
bx, by = 5600.0, 4620.0
start = (bx - ax * 2200.0, by - ay * 2200.0)
deck = (bx - ax * 600.0, by - ay * 600.0)
tp(start[0], start[1], YAW)
walk_to(deck[0], deck[1], 16, "ramp up")
walk_to(start[0], start[1], 16, "ramp down")
G.cmd("TD.Look %.1f -6" % YAW)
time.sleep(0.8)
print(G.shot("bridge_ramp_foot"))
with open(os.path.join(OUT, "walk.json"), "w", encoding="utf-8") as fh:
    json.dump(log, fh, indent=1)
G.cmd("TD.Input.ReleaseAll")

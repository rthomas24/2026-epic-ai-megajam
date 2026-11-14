# SPDX-License-Identifier: MIT
# Planet landmarks: captures from the landing approach and from the ground (a -game session on <port>).
#   python kit_landmark_shots.py <port> <Verdance|Cinder> <lx> <ly> <lz>
# Stages <Planet>Arrival (the ship flies down to the pad), spawns the landmark class at (lx, ly, lz) and captures every
# 2.5 s of the descent, logging the distance to the landmark; then stages <Planet>OnFoot and captures from the ground at
# about 150 m and from the pad. Writes review/game/Landmarks/<Planet>_*.png and <Planet>_approach.json.
import importlib.util
import json
import math
import os
import re
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
OUT = os.path.join(KIT, "review", "game", "Landmarks")
os.makedirs(OUT, exist_ok=True)
os.environ["TD_SHOT_DIR"] = OUT
PORT, PLANET = sys.argv[1], sys.argv[2]
L = [float(v) for v in sys.argv[3:6]]
sys.argv = [sys.argv[0], PORT, "noop"]
spec = importlib.util.spec_from_file_location("op_game", os.path.join(os.path.dirname(KIT), "Outpost", "scripts", "op_game.py"))
G = importlib.util.module_from_spec(spec)
try:
    spec.loader.exec_module(G)
except SystemExit:
    pass
CLS = {"Verdance": "/Game/TriadDrift/Gameplay/Props/Kit/Sites/BP_Landmark_RelayMast",
       "Cinder": "/Game/TriadDrift/Gameplay/Props/Kit/Sites/BP_Landmark_NullDerrick"}[PLANET]


def where():
    G.cmd("TD.Ground.Where")
    time.sleep(0.4)
    line = (G.log_tail(r"TD\.Where pos=") or ["?"])[-1]
    m = re.search(r"pos=\(([-\d.]+) ([-\d.]+) ([-\d.]+)\)", line)
    return [float(x) for x in m.groups()] if m else None


log = []
G.cmd("TD.God 1")
G.cmd("TD.Stage %sArrival" % PLANET)
time.sleep(3.0)
G.cmd("TD.Dev.SpawnClass %s Landmark %.0f %.0f %.0f" % (CLS, L[0], L[1], L[2]))
for k in range(16):
    time.sleep(2.5)
    w = where()
    d = math.dist(w, L) / 100.0 if w else -1
    name = "%s_approach_%02d" % (PLANET, k)
    G.shot(name)
    log.append({"shot": name, "pos": w, "dist_m": round(d)})
    print(name, round(d), "m")
G.cmd("TD.Stage %sOnFoot" % PLANET)
time.sleep(20.0)
G.cmd("TD.Dev.SpawnClass %s Landmark %.0f %.0f %.0f" % (CLS, L[0], L[1], L[2]))
time.sleep(3.0)
pad = where()
yaw = math.degrees(math.atan2(L[1] - pad[1], L[0] - pad[0]))
G.cmd("TD.Look %.1f 12" % yaw)
time.sleep(1.5)
G.shot("%s_ground_from_pad" % PLANET)
log.append({"shot": "%s_ground_from_pad" % PLANET, "pos": pad, "dist_m": round(math.dist(pad, L) / 100.0)})
gx, gy = L[0] - math.cos(math.radians(yaw)) * 15000.0, L[1] - math.sin(math.radians(yaw)) * 15000.0
G.cmd("TD.Ground.Teleport %.0f %.0f %.0f" % (gx, gy, L[2] + 400.0))
G.cmd("TD.Look %.1f 18" % yaw)
time.sleep(2.5)
G.shot("%s_ground_150m" % PLANET)
log.append({"shot": "%s_ground_150m" % PLANET, "pos": where(), "dist_m": 150})
with open(os.path.join(OUT, "%s_approach.json" % PLANET), "w", encoding="utf-8") as fh:
    json.dump(log, fh, indent=1)

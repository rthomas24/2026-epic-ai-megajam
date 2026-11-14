# SPDX-License-Identifier: MIT
# Space set pieces: in-game captures at 1 km and 100 m against the nebula (a -game session in L_Galaxy on <port>).
# For each piece: TD.World.Goto <place> (a fresh view direction), read the ship's position and yaw, spawn the class
# 1 km ahead, capture, spawn a second copy 100 m ahead, capture.  python kit_space_shots.py <port>
import importlib.util
import math
import os
import re
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
OUT = os.path.join(KIT, "review", "game", "Space")
os.makedirs(OUT, exist_ok=True)
os.environ["TD_SHOT_DIR"] = OUT
PORT = sys.argv[1]
sys.argv = [sys.argv[0], PORT, "noop"]
spec = importlib.util.spec_from_file_location("op_game", os.path.join(os.path.dirname(KIT), "Outpost", "scripts", "op_game.py"))
G = importlib.util.module_from_spec(spec)
try:
    spec.loader.exec_module(G)
except SystemExit:
    pass
SPS = "/Game/TriadDrift/Gameplay/Props/Space/SetPieces/"
# (class, place, extra distance for the near shot so the whole piece is in frame, height offset)
PIECES = [("BP_SP_PatrolWreck", "Start", 10000, 0), ("BP_SP_ResearchA", "Gate", 16000, 0), ("BP_SP_ResearchB", "Dominion", 12000, 0),
          ("BP_SP_NullRig", "Cinder", 11000, -1500), ("BP_SP_HollowRock", "Verdance", 12000, 0)]
PLACE = os.environ.get("TD_SPACE_PLACE")          # one place for every piece (a run per piece: a fresh session each)
ONLY = os.environ.get("TD_SPACE_ONLY")


def where():
    G.cmd("TD.Ground.Where")
    time.sleep(0.5)
    line = (G.log_tail(r"TD\.Where pos=") or ["?"])[-1]
    m = re.search(r"pos=\(([-\d.]+) ([-\d.]+) ([-\d.]+)\) yaw=([-\d.]+)", line)
    return [float(x) for x in m.groups()] if m else None


G.cmd("TD.God 1")
for cls, place, near, dz in PIECES:
    if ONLY and ONLY not in cls:
        continue
    G.cmd("slomo 1")
    G.cmd("TD.World.Goto %s" % (PLACE or place))
    time.sleep(4.0)
    a = where()
    time.sleep(1.0)
    b = where()
    if not a or not b:
        print("no position at", place)
        continue
    v = [b[i] - a[i] for i in range(3)]
    n = math.sqrt(sum(c * c for c in v)) or 1.0
    f = [c / n for c in v]                      # the ship's travel direction = where the chase camera looks
    G.cmd("slomo 0.01")
    time.sleep(0.5)
    x, y, z, _ = where()
    for d, tag in [(int(t), "%sm" % (int(t) // 100)) for t in os.environ.get("TD_SPACE_D", "").split(",") if t] or ((100000, "1km"), (near, "100m")):
        G.cmd("TD.Dev.SpawnClass %s%s Space %.0f %.0f %.0f" % (SPS, cls, x + f[0] * d, y + f[1] * d, z + f[2] * d + dz))
        time.sleep(3.0)
        print(G.shot("%s_%s" % (cls.replace("BP_SP_", ""), tag)))
    G.cmd("slomo 1")

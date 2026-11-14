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
# r2 (Lead): the patrol wreck away from the station ruins; every piece at 1 km, 400 m and 100 m
PIECES = [("BP_SP_PatrolWreck", os.environ.get("TD_SPACE_PATROL", "Gate"), 15000, 0), ("BP_SP_ResearchA", os.environ.get("TD_SPACE_RA", "Belt"), 16000, 0), ("BP_SP_ResearchB", os.environ.get("TD_SPACE_RB", "Verdance"), 13000, 0),
          ("BP_SP_NullRig", os.environ.get("TD_SPACE_RIG", "Cinder"), 11000, -1500), ("BP_SP_HollowRock", os.environ.get("TD_SPACE_ROCK", "Reactor"), 12000, 0)]
if os.environ.get("TD_SPACE_SET") == "region":          # the region set pieces: (class, place, near distance, dz)
    SPS = "/Game/TriadDrift/Gameplay/Props/Kit/Space/"
    # r3: the near shot stands off so the whole piece is in frame (the 261 m shard at 600 m)
    PIECES = [("BP_KS_PicketRing", "Gate", 30000, 0), ("BP_KS_Hulk", "Verdance", 35000, 0), ("BP_KS_ScarRig", "Cinder", 30000, 0),
              ("BP_KS_MoonShards", "Belt", 60000, 0)]
AIM = {"BP_KS_PicketRing": (6600.0, 0.0)}
PLACE = os.environ.get("TD_SPACE_PLACE")          # one place for every piece (a run per piece: a fresh session each)
ONLY = os.environ.get("TD_SPACE_ONLY")


def where():
    G.cmd("TD.Ground.Where")
    time.sleep(0.5)
    line = (G.log_tail(r"TD\.Where pos=") or ["?"])[-1]
    m = re.search(r"pos=\(([-\d.]+) ([-\d.]+) ([-\d.]+)\) yaw=([-\d.]+)", line)
    return [float(x) for x in m.groups()] if m else None


G.cmd("TD.God 1")
G.cmd("DisableAllScreenMessages")
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
    for d, tag in [(int(t), "%sm" % (int(t) // 100)) for t in os.environ.get("TD_SPACE_D", "").split(",") if t] or (((150000, "1500m"), (near, "near")) if os.environ.get("TD_SPACE_SET") == "region" else ((100000, "1km"), (40000, "400m"), (near, "100m"))):
        # beside and above the ship, so the player's own hull does not hide the piece (about 14 deg right, 7 up)
        rx, ry = -f[1], f[0]
        ax, ay = AIM.get(cls, (0.0, 0.0))                # aim at the piece, not its pivot (the picket ring's arc is 66 m out)
        G.cmd("TD.Dev.SpawnClass %s%s Space %.0f %.0f %.0f" % (SPS, cls, x + f[0] * d + rx * d * 0.25 - ax, y + f[1] * d + ry * d * 0.25 - ay,
                                                                 z + f[2] * d + dz + d * 0.12))
        time.sleep(3.0)
        print(G.shot("%s_%s" % (cls.replace("BP_SP_", "").replace("BP_", ""), tag)))
        if os.environ.get("TD_SPACE_PHASES"):        # the strobe's other phase: let half a period (0.38 s) run, shoot again
            G.cmd("slomo 1")
            time.sleep(0.38)
            G.cmd("slomo 0.01")
            time.sleep(0.8)
            print(G.shot("%s_%s_b" % (cls.replace("BP_SP_", ""), tag)))
    G.cmd("slomo 1")

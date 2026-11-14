# SPDX-License-Identifier: MIT
# Batch AE, one launch, one lock turn: (#119) the anchor collapse watched from the shore route at (11200, -7600),
# facing the tower and the run box together, frames every 3 s, then TD.Ground.Near on the run box to log what fell;
# (#126) the Dominion bay mouth from space on the approach (TD.Stage DominionAssault, TD.Travel.ToDock 1500 / 600 / 250).
#   python kit_batch_ae.py <port>
import importlib.util
import json
import math
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
PORT = sys.argv[1]
OUT = os.path.join(KIT, "review", "game", "BatchAE")
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


def shot(name, yaw=None, pitch=None):
    if yaw is not None:
        G.cmd("TD.Look %.1f %.1f" % (yaw, pitch if pitch is not None else -4.0))
        time.sleep(0.5)
    p = G.shot(name)
    log.append({"shot": p})
    print("shot", p)


G.cmd("TD.God 1")
G.cmd("DisableAllScreenMessages")
# 1. the collapse from the shore route
G.cmd("TD.Stage CinderOnFoot")
time.sleep(28.0)
G.cmd("TD.Ground.ThirdPerson 1")
G.cmd("TD.Ground.SkipTo Jammer 2")
time.sleep(0.5)
VX, VY = 11200.0, -7600.0
G.cmd("TD.Ground.Teleport %.0f %.0f %.0f" % (VX, VY, 92.0))
yaw = math.degrees(math.atan2(-5000.0 - VY, 12600.0 - VX)) - 35.0     # the tower at the right of frame, the run box across
for i in range(7):
    shot("collapse_shore_%02ds" % (2 + i * 3), yaw, 10.0)
    time.sleep(2.4)
G.cmd("TD.Ground.Near 10200 -7400 0 4500")
G.cmd("TD.Ground.Near 12600 -5000 0 1500")
time.sleep(1.0)
near = G.log_tail(r"TD\.Near ", 200)
log.append({"near": near})
print("pieces near the run box / tower:", [n for n in near if "Anchor" in n or "Debris" in n or "Section" in n][:20])
# 2. the bay mouth from space
G.cmd("TD.Stage DominionAssault")
time.sleep(25.0)
for d in (1500, 600, 250):
    G.cmd("TD.Travel.ToDock %d 0 0" % d)
    time.sleep(3.0)
    shot("bay_mouth_%dm" % d)
print(G.log_tail(r"TD\.Travel\.ToDock", 1))
with open(os.path.join(OUT, "walk.json"), "w", encoding="utf-8") as fh:
    json.dump(log, fh, indent=1)
G.cmd("TD.Input.ReleaseAll")

# SPDX-License-Identifier: MIT
# Batch AF, one launch, one lock turn: (#119) the anchor collapse from the shore route after T5's DropPiece fix
# (sections at native size, no lifespan) with TD.Ground.Near on what fell; (#126) one guide-light close-up in the
# DominionDocking stage; and, when TD_BREAKUP_CMD is set (T4's command or stage), the Dominion breakup from T4's
# camera every 2 s to judge SM_KS_DomHullChunk_A/B/C.   python kit_batch_af.py <port>
import importlib.util
import json
import math
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
PORT = sys.argv[1]
OUT = os.path.join(KIT, "review", "game", "BatchAF")
os.environ["TD_SHOT_DIR"] = OUT
os.makedirs(OUT, exist_ok=True)
BREAKUP = os.environ.get("TD_BREAKUP_CMD", "")
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
yaw = math.degrees(math.atan2(-5000.0 - VY, 12600.0 - VX)) - 30.0
for i in range(7):
    shot("collapse_shore_%02ds" % (2 + i * 3), yaw, 10.0)
    time.sleep(2.4)
G.cmd("TD.Ground.Near 12600 -5000 0 3000")
G.cmd("TD.Ground.Near 10200 -7400 0 4500")
time.sleep(1.0)
near = [n for n in G.log_tail(r"TD\.Near ", 300) if "Anchor" in n and "Charge" not in n and "Tower" not in n and "Emitter" not in n]
log.append({"fallen": near})
print("fallen kit pieces:", len(near))
for n in near[:12]:
    print("  ", n[n.find("TD.Near"):][:220])
if os.environ.get("TD_AF_ONLY") == "collapse":
    with open(os.path.join(OUT, "walk.json"), "w", encoding="utf-8") as fh:
        json.dump(log, fh, indent=1)
    G.cmd("TD.Input.ReleaseAll")
    sys.exit(0)
# 2. a guide light close-up (DominionDocking: the bay exists there)
G.cmd("TD.Stage DominionDocking")
time.sleep(22.0)
G.cmd("TD.Travel.ToDock 120 30 1")
time.sleep(3.0)
shot("guide_light_close")
print(G.log_tail(r"TD\.Travel\.ToDock", 1))
# 3. the breakup, when T4's trigger is given
if BREAKUP:
    # T4: kill shot about 1 s, the outside camera cuts in, the final blast at 7 s, the chunks fly; frames 6-12 s after the kill
    for c in BREAKUP.split(";"):
        c = c.strip()
        if c.lower().startswith("sleep "):
            time.sleep(float(c.split()[1]))
        elif c:
            G.cmd(c)
            time.sleep(0.5)
    time.sleep(5.5)
    for i in range(7):
        shot("breakup_%02ds" % (6 + i))
        time.sleep(0.5)
    print(G.log_tail(r"breakup camera on|hull chunks thrown", 2))
with open(os.path.join(OUT, "walk.json"), "w", encoding="utf-8") as fh:
    json.dump(log, fh, indent=1)
G.cmd("TD.Input.ReleaseAll")

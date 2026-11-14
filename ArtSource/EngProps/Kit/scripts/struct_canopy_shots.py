# SPDX-License-Identifier: MIT
# #149 canopies: the intro's three Squad pilot shots (Wren 10.7 s, Kato 13.3 s, Moss 16.0 s, held) and each ship in
# flight from the chase camera. python struct_canopy_shots.py <port> <tag: before|after>
import importlib.util
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
PORT, TAG = sys.argv[1], sys.argv[2]
OUT = os.path.join(KIT, "review", "game", "Canopy149")
os.environ["TD_SHOT_DIR"] = OUT
os.makedirs(OUT, exist_ok=True)
sys.argv = [sys.argv[0], PORT, "noop"]
spec = importlib.util.spec_from_file_location("op_game", os.path.join(os.path.dirname(KIT), "Outpost", "scripts", "op_game.py"))
G = importlib.util.module_from_spec(spec)
try:
    spec.loader.exec_module(G)
except SystemExit:
    pass

G.cmd("DisableAllScreenMessages")
time.sleep(8.0)
G.cmd("TD.Intro.Play")
time.sleep(3.0)
ONLY = os.environ.get("TD_CAN_ONLY", "")
SHOTS = [(10.7, "wren"), (13.3, "kato"), (16.0, "moss")]
if os.environ.get("TD_CAN_TIMES"):          # e.g. "moss:14.8,15.4,16.0,16.6": several holds of one shot (its pose varies)
    who_, ts = os.environ["TD_CAN_TIMES"].split(":")
    SHOTS = [(float(t_), "%s_%s" % (who_, t_.replace(".", "p"))) for t_ in ts.split(",")]
for t, who in SHOTS:
    if ONLY and who.split("_")[0] not in ONLY:
        continue
    G.cmd("TD.Intro.At %.1f 1" % t)
    time.sleep(2.5)
    print(G.shot("%s_intro_%s" % (TAG, who), 1920))
if ONLY:
    sys.exit(0)
G.cmd("TD.Intro.Skip")
time.sleep(8.0)
G.cmd("TD.God 1")
G.cmd("TD.Squad.Passive 1")
for ship in ("Rift", "Lancer", "Bastion"):
    G.cmd("TD.Squad.Switch %s" % ship)
    time.sleep(2.5)
    G.cmd("TD.World.Hold 3 120")
    time.sleep(1.5)
    print(G.shot("%s_chase_%s" % (TAG, ship.lower())))
G.cmd("TD.World.Hold 0")

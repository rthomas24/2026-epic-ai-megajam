# SPDX-License-Identifier: MIT
# Batch AC, one launch, one 15-minute lock turn (about 6 minutes of play): Cinder only.
#   1. The Null conduit intact and broken (BP_ConduitCheck, capture only) at 4 m.
#   2. The anchor collapse watched whole: TD.Ground.SkipTo Jammer 2, the pilot at the far bank facing the tower,
#      frames every 3.5 s while the tower sinks and the kit sections / debris fall; the run times out (a CUT is expected).
#   3. The escape run walked along T5's shore route (11200, -8300) -> exit (6400, -8400) after a second SkipTo,
#      re-aiming every step; stalls and the pilot's state logged; a frame looking back at the fallen pieces.
#   python kit_batch_ac.py <port>
import importlib.util
import json
import math
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
PORT = sys.argv[1]
OUT = os.path.join(KIT, "review", "game", "BatchAC")
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
TOWER = (12600.0, -5000.0)


def feet():
    p = G.where()
    return [round(p[0]), round(p[1]), round(p[2] - 90), round(p[3])] if isinstance(p, list) else None


def tp(x, y, z, yaw, pitch=-8.0, settle=2.0):
    G.cmd("TD.Ground.Teleport %.0f %.0f %.0f" % (x, y, z + 92))
    G.cmd("TD.Look %.1f %.1f" % (yaw, pitch))
    time.sleep(settle)
    f = feet()
    log.append({"teleport": [x, y, z], "feet": f})
    return f


def walk(yaw, secs, label):
    G.cmd("TD.Look %.1f -8" % yaw)
    G.cmd("TD.Key W %.2f" % secs)
    time.sleep(secs + 0.7)
    f = feet()
    log.append({"walk": [yaw, secs], "feet": f, "at": label})
    print("walk %-30s -> %s" % (label, f))
    return f


def shot(name, yaw=None, pitch=None):
    if yaw is not None:
        G.cmd("TD.Look %.1f %.1f" % (yaw, pitch if pitch is not None else -8.0))
        time.sleep(0.6)
    p = G.shot(name)
    log.append({"shot": p})
    print("shot", p)


def to_tower(f):
    return math.degrees(math.atan2(TOWER[1] - f[1], TOWER[0] - f[0])) if f else 0.0


G.cmd("TD.God 1")
G.cmd("DisableAllScreenMessages")
G.cmd("TD.Stage CinderOnFoot")
time.sleep(28.0)
G.cmd("TD.Ground.ThirdPerson 1")
# 1. the conduit
G.cmd("TD.Dev.SpawnClass /Game/TriadDrift/Gameplay/Props/Kit/Sites/BP_ConduitCheck ConduitCheck -3000 -3000 0")
time.sleep(3.0)
tp(-3400.0, -3000.0, 0.0, 0.0)
shot("conduit_intact_broken_4m", 0.0, -6.0)
# 2. the collapse, watched
G.cmd("TD.Ground.SkipTo Jammer 2")
time.sleep(0.5)
print(G.log_tail(r"TD\.Ground\.SkipTo", 1))
f = feet()
log.append({"run_start": f})
for i in range(7):
    shot("collapse_%02ds" % int(1 + i * 3.5), to_tower(feet()), 6.0)
    time.sleep(2.6)
print(G.log_tail(r"Pilot down|CUT!", 2))
time.sleep(4.0)
# 3. the run, walked along the shore route
G.cmd("TD.Ground.SkipTo Jammer 2")
time.sleep(1.0)
route = [(11200.0, -8300.0), (6400.0, -8400.0)]
leg = 0
for i in range(30):
    f = feet()
    if not f:
        break
    tx, ty = route[leg]
    if math.hypot(tx - f[0], ty - f[1]) < 500.0:
        leg += 1
        if leg >= len(route):
            break
        tx, ty = route[leg]
    walk(math.degrees(math.atan2(ty - f[1], tx - f[0])), 0.8, "run leg %d step %d" % (leg, i))
print(G.log_tail(r"Pilot down|CUT!|Jammer", 3))
f = feet()
shot("run_end_looking_back", to_tower(f), 2.0)
with open(os.path.join(OUT, "walk.json"), "w", encoding="utf-8") as fh:
    json.dump(log, fh, indent=1)
G.cmd("TD.Input.ReleaseAll")

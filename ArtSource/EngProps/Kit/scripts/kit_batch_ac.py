# SPDX-License-Identifier: MIT
# Batch AC, one launch, one 15-minute lock turn (about 6 minutes of play): Cinder only.
#   1. The Null conduit intact and broken (BP_ConduitCheck, capture only) at 4 m.
#   2. The anchor collapse watched whole: TD.Ground.SkipTo Jammer 2, the pilot at the far bank facing the tower,
#      frames every 3.5 s while the tower sinks and the kit sections / debris fall; the run times out (a CUT is expected).
#   3. The charge at scale 1 on the tower face; 4. the docking bay mouth on the approach (DominionDocking stage).
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
import re


def items(pattern):
    G.cmd("TD.Ground.CourseBounds")
    time.sleep(1.5)
    out = []
    for line in G.log_tail(r"TD\.CourseItem", 4000):
        if re.search(pattern, line):
            m = re.search(r"world=V\(X=([-\d.]+), Y=([-\d.]+), Z=([-\d.]+)\)", line)
            if m:
                out.append([float(v) for v in m.groups()])
    return out


# 1. the conduits at the excavation gate (#118, #117): the gate's two shatter actors, frames from 5 m
con = [c for c in items(r"TDShatterCrystal") if c[1] > 3000.0]
log.append({"gate_conduits": con})
print("gate conduits", con)
if len(con) >= 2:
    mx, my = (con[0][0] + con[1][0]) * 0.5, (con[0][1] + con[1][1]) * 0.5
    tp(mx - 700.0, my, 0.0, 0.0)
    shot("gate_conduits_7m", 0.0, -4.0)
    tp(con[0][0] - 450.0, con[0][1], 0.0, 0.0)
    shot("gate_conduit_4m", 0.0, -6.0)
# 2. the charge at scale 1 on the tower face (#117)
G.cmd("TD.Ground.SkipTo Jammer 1")
time.sleep(3.0)
sites = sorted(items(r"TDInteractableActor"), key=lambda s_: math.hypot(s_[0] - TOWER[0], s_[1] - TOWER[1]))[:3]
log.append({"charge_sites": sites})
if sites:
    s0 = sites[0]
    d = math.atan2(s0[1] - TOWER[1], s0[0] - TOWER[0])
    tp(s0[0] + math.cos(d) * 300.0, s0[1] + math.sin(d) * 300.0, 0.0, math.degrees(d) + 180.0)
    shot("charge_on_face_3m", math.degrees(d) + 180.0, 4.0)
# 3. the collapse watched from the far bank (#119)
G.cmd("TD.Ground.SkipTo Jammer 2")
time.sleep(0.5)
print(G.log_tail(r"TD\.Ground\.SkipTo", 1))
for i in range(7):
    shot("collapse_%02ds" % int(1 + i * 3.5), to_tower(feet()), 8.0)
    time.sleep(2.6)
print(G.log_tail(r"Pilot down|CUT!", 2))
# 4. the docking bay mouth (#126): the docking stage, frames on the approach
G.cmd("TD.Stage DominionDocking")
time.sleep(20.0)
for i, wait in enumerate((0.0, 4.0, 4.0, 4.0)):
    time.sleep(wait)
    shot("bay_mouth_%d" % i)
with open(os.path.join(OUT, "walk.json"), "w", encoding="utf-8") as fh:
    json.dump(log, fh, indent=1)
G.cmd("TD.Input.ReleaseAll")

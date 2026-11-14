# SPDX-License-Identifier: MIT
# Batch AB, one -game session (one render-slot lock turn): the Shattered Moons shard at 1 km (per-plate strata UVs),
# the Cinder anchor's three charge sites (T5: scale 1 on the tower face), the collapse run with the kit debris and
# sections falling (TD.Ground.SkipTo Jammer 2), and the Dominion hangar start to the first door.
#   python kit_batch_ab.py <port>
import importlib.util
import json
import math
import os
import re
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
PORT = sys.argv[1]
OUT = os.path.join(KIT, "review", "game", "BatchAB")
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
    print("walk %-36s -> %s" % (label, f))
    return f


def shot(name, yaw=None, pitch=None):
    if yaw is not None:
        G.cmd("TD.Look %.1f %.1f" % (yaw, pitch if pitch is not None else -8.0))
        time.sleep(0.8)
    p = G.shot(name)
    log.append({"shot": p})
    print("shot", p)


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


G.cmd("TD.God 1")
G.cmd("DisableAllScreenMessages")
# 1. the shard at 1 km
G.cmd("TD.SpaceField.ViewRegion 6 1")
time.sleep(6.0)
shot("shard_region6_1km")
# 2. the charges
G.cmd("TD.Stage CinderOnFoot")
time.sleep(28.0)
G.cmd("TD.Ground.ThirdPerson 1")
G.cmd("TD.Ground.SkipTo Jammer 1")
time.sleep(4.0)
print(G.log_tail(r"TD\.Ground\.SkipTo", 1))
tower = (12600.0, -5000.0)
sites = sorted(items(r"TDInteractableActor"), key=lambda s: math.hypot(s[0] - tower[0], s[1] - tower[1]))[:3]
log.append({"charge_sites": sites})
print("charge sites", sites)
for i, s in enumerate(sites):
    d = math.atan2(s[1] - tower[1], s[0] - tower[0])
    tp(s[0] + math.cos(d) * 280.0, s[1] + math.sin(d) * 280.0, 0.0, math.degrees(d) + 180.0)
    shot("charge_site_%d" % i, math.degrees(d) + 180.0, -10.0)
# 3. the collapse run
G.cmd("TD.Ground.SkipTo Jammer 2")
time.sleep(1.0)
print(G.log_tail(r"TD\.Ground\.SkipTo", 1))
f = feet()
log.append({"run_start": f})
for t in range(5):
    time.sleep(2.5)
    G.cmd("TD.Look %.1f -4" % 60.0)
    shot("collapse_t%02d" % (3 + t * 3))
exit_at = (6400.0, -8400.0)
for i in range(24):
    f = feet()
    if not f:
        break
    if math.hypot(f[0] - exit_at[0], f[1] - exit_at[1]) < 700.0:
        break
    yaw = math.degrees(math.atan2(exit_at[1] - f[1], exit_at[0] - f[0]))
    walk(yaw, 0.6, "escape run %d" % i)
print(G.log_tail(r"CUT!|Pilot down|Jammer|collapse", 3))
shot("collapse_run_end", 60.0, -6.0)
# 4. the hangar
G.cmd("TD.Stage DominionInterior")
time.sleep(28.0)
f = feet()
log.append({"hangar_start": f})
print("hangar start", f)
shot("hangar_start", f[3] if f else 0.0, -6.0)
for i in range(16):
    f = feet()
    if not f or f[0] > 1300.0:
        break
    yaw = math.degrees(math.atan2(0.0 - f[1], 1450.0 - f[0])) if f[0] > -200.0 or abs(f[1]) > 450.0 else (90.0 if f[1] >= 0 else -90.0)
    walk(yaw, 0.6, "hangar to door %d" % i)
shot("hangar_door", 0.0, -6.0)
with open(os.path.join(OUT, "walk.json"), "w", encoding="utf-8") as fh:
    json.dump(log, fh, indent=1)
G.cmd("TD.Input.ReleaseAll")

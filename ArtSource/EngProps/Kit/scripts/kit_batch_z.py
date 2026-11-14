# SPDX-License-Identifier: MIT
# Batch Z, one -game session (course-local coordinates; both courses sit at the world origin unrotated):
#   1. Cinder: off the far end of T5's magma bridge toward the moved geyser row (must not be thrown).
#   2. Cinder: the three charge sites on the anchor tower (frames), then BP_AnchorCheck (the collapse pieces at rest)
#      spawned on the escape shore: frames, and a walk through and around them (nothing traps the pilot).
#   3. Dominion interior: from T5's hangar start, walk to the first door (x 1200), re-aiming every step; stalls logged.
#   python kit_batch_z.py <port>
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
OUT = os.path.join(KIT, "review", "game", "BatchZ")
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
    print("walk %-40s -> %s" % (label, f))
    return f


def shot(name, yaw, pitch):
    G.cmd("TD.Look %.1f %.1f" % (yaw, pitch))
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
G.cmd("TD.Stage CinderOnFoot")
time.sleep(28.0)
G.cmd("TD.Ground.ThirdPerson 1")
ONLY = os.environ.get("TD_BATCHZ_ONLY", "")
# 1. off the far end of the bridge
YAW = 34.7
ax, ay = math.cos(math.radians(YAW)), math.sin(math.radians(YAW))
if ONLY != "charges":
    tp(5600.0 + ax * 1200.0, 4620.0 + ay * 1200.0, 60.0, YAW)
    for i in range(8):
        walk(YAW, 0.6, "off the far ramp %d" % i)
    shot("bridge_far_side", YAW, -6.0)
# 2. the charge sites and the collapse pieces
sites = items(r"TDInteractableActor")
log.append({"interactables": sites})
print("interactables", sites)
tower = (12600.0, -5000.0)
near = sorted(sites, key=lambda s: math.hypot(s[0] - tower[0], s[1] - tower[1]))[:3]
for i, s in enumerate(near):
    d = math.atan2(s[1] - tower[1], s[0] - tower[0])
    x, y = s[0] + math.cos(d) * 300.0, s[1] + math.sin(d) * 300.0
    tp(x, y, 0.0, math.degrees(d) + 180.0)
    shot("charge_site_%d" % i, math.degrees(d) + 180.0, -12.0)
if ONLY == "charges":
    with open(os.path.join(OUT, "walk_charges.json"), "w", encoding="utf-8") as fh:
        json.dump(log, fh, indent=1)
    sys.exit(0)
sx, sy = 6400.0, -8400.0                                   # the escape shore, west of the lake, short of the exit
G.cmd("TD.Dev.SpawnClass /Game/TriadDrift/Gameplay/Props/Kit/Sites/BP_AnchorCheck AnchorCheck %.0f %.0f 0" % (sx, sy))
time.sleep(3.0)
print(G.log_tail("TDDevSpawn", 1))
tp(sx - 1500.0, sy - 1100.0, 0.0, 36.0)
shot("collapse_pieces_15m", 36.0, -8.0)
tp(sx - 600.0, sy - 300.0, 0.0, 30.0)
shot("collapse_pieces_6m", 30.0, -14.0)
for (y0, tag) in ((-250.0, "between A and B"), (220.0, "between B and C")):
    tp(sx - 1100.0, sy + y0, 0.0, 0.0)
    for i in range(5):
        walk(0.0, 0.6, "escape run %s %d" % (tag, i))
tp(sx + 300.0, sy, 400.0, 0.0, settle=3.0)
for yaw in (0.0, 90.0, 180.0, 270.0):
    walk(yaw, 0.6, "off the top of Section B yaw %d" % yaw)
# 3. the hangar
G.cmd("TD.Stage DominionInterior")
time.sleep(28.0)
f = feet()
log.append({"hangar_start": f})
print("hangar start", f)
shot("hangar_start", f[3] if f else 0.0, -6.0)
for i in range(14):
    f = feet()
    if not f or f[0] > 1350.0:
        break
    yaw = math.degrees(math.atan2(0.0 - f[1], 1400.0 - f[0]))
    walk(yaw, 0.6, "hangar to door %d" % i)
shot("hangar_door_reached", 0.0, -6.0)
with open(os.path.join(OUT, "walk.json"), "w", encoding="utf-8") as fh:
    json.dump(log, fh, indent=1)
G.cmd("TD.Input.ReleaseAll")

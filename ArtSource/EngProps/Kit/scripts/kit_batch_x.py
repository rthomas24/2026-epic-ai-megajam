# SPDX-License-Identifier: MIT
# Batch X, one -game session, real key presses, feet logged (course-local coordinates):
#   1. Cinder: T5's placed magma bridge on the real course (course-local (5600, 4620), yaw 34.7): approach the near ramp
#      foot, walk the whole span on the centre line, push into both rails mid-deck, walk past the far ramp toward the
#      geyser row; frames at the near foot, mid-deck, the far foot.
#   2. Dominion interior: the hangar exit doorway (course-local x 1200) walked from the corridor side twice - on the
#      centre line and hugging the corridor wall (y 130, the capsule against the wall) - to check the jamb chamfer.
#   python kit_batch_x.py <port>
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
OUT = os.path.join(KIT, "review", "game", "BatchX")
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
XF = [0.0, 0.0, 0.0, 0.0]


def course():
    G.cmd("TD.Ground.CourseBounds")
    time.sleep(1.0)
    lines = G.log_tail(r"TD\.Course \S+ at ", 6)
    m = re.search(r"at X=([-\d.]+) Y=([-\d.]+) Z=([-\d.]+) yaw ([-\d.]+)", lines[-1] if lines else "")
    XF[:] = [float(v) for v in m.groups()] if m else [0.0, 0.0, 0.0, 0.0]
    log.append({"course": lines[-1] if lines else None, "xf": list(XF)})
    print("course", XF)


def to_world(x, y, z):
    c, s = math.cos(math.radians(XF[3])), math.sin(math.radians(XF[3]))
    return XF[0] + c * x - s * y, XF[1] + s * x + c * y, XF[2] + z


def to_local(wx, wy, wz):
    c, s = math.cos(math.radians(XF[3])), math.sin(math.radians(XF[3]))
    dx, dy = wx - XF[0], wy - XF[1]
    return round(c * dx + s * dy), round(-s * dx + c * dy), round(wz - XF[2] - 90)


def feet():
    p = G.where()
    return list(to_local(p[0], p[1], p[2])) if isinstance(p, list) else None


def tp(x, y, z, yaw, pitch=-8.0, settle=2.0):
    wx, wy, wz = to_world(x, y, z + 92)
    G.cmd("TD.Ground.Teleport %.0f %.0f %.0f" % (wx, wy, wz))
    G.cmd("TD.Look %.1f %.1f" % (yaw + XF[3], pitch))
    time.sleep(settle)
    f = feet()
    log.append({"teleport": [x, y, z], "feet": f})
    return f


def walk(yaw, secs, label):
    G.cmd("TD.Look %.1f -8" % (yaw + XF[3]))
    G.cmd("TD.Key W %.2f" % secs)
    time.sleep(secs + 0.7)
    f = feet()
    log.append({"walk": [yaw, secs], "feet": f, "at": label})
    print("walk %-34s -> %s" % (label, f))
    return f


def shot(name, yaw, pitch):
    G.cmd("TD.Look %.1f %.1f" % (yaw + XF[3], pitch))
    time.sleep(0.8)
    p = G.shot(name)
    log.append({"shot": p})
    print("shot", p)


G.cmd("TD.God 1")
G.cmd("DisableAllScreenMessages")
# ---- 1. the bridge on the Cinder course
G.cmd("TD.Stage CinderOnFoot")
time.sleep(28.0)
course()
G.cmd("TD.Ground.ThirdPerson 1")
YAW = 34.7
ax, ay = math.cos(math.radians(YAW)), math.sin(math.radians(YAW))
cx, cy = 5600.0, 4620.0


def on_span(d, side=0.0):
    return cx + ax * d - ay * side, cy + ay * d + ax * side


x0, y0 = on_span(-1800.0)
tp(x0, y0, 0.0, YAW)
shot("bridge_01_near_foot", YAW, -8.0)
for i in range(16):
    f = walk(YAW, 0.6, "span %d" % i)
mx, my = on_span(0.0)
tp(mx, my, 60.0, YAW + 90.0)
for i in range(2):
    walk(YAW + 90.0, 0.6, "into the left rail %d" % i)
tp(mx, my, 60.0, YAW - 90.0)
for i in range(2):
    walk(YAW - 90.0, 0.6, "into the right rail %d" % i)
tp(mx, my, 60.0, YAW)
shot("bridge_02_mid_deck", YAW, -10.0)
fx, fy = on_span(1500.0, 300.0)
tp(fx, fy, 0.0, YAW + 160.0)
shot("bridge_03_far_foot_back", YAW + 160.0, -6.0)
gx, gy = on_span(1250.0)
tp(gx, gy, 60.0, YAW)
for i in range(4):
    walk(YAW, 0.6, "off the far ramp toward the geysers %d" % i)
shot("bridge_04_far_side_geysers", YAW, -6.0)
# ---- 2. the Dominion interior doorway
G.cmd("TD.Stage DominionInterior")
time.sleep(28.0)
course()
G.cmd("TD.Ground.ThirdPerson 1")
for side, tag in ((0.0, "centre"), (130.0, "wall")):
    tp(2000.0, side, 0.0, 180.0)
    for i in range(6):
        walk(180.0, 0.6, "door %s %d" % (tag, i))
tp(1900.0, 130.0, 0.0, 180.0)
shot("door_01_wall_line", 180.0, -6.0)
with open(os.path.join(OUT, "walk.json"), "w", encoding="utf-8") as fh:
    json.dump(log, fh, indent=1)
G.cmd("TD.Input.ReleaseAll")

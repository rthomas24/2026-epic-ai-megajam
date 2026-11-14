# SPDX-License-Identifier: MIT
# #149: the intro's squad shots in motion, for the canopy reflections and framing: from 9 s, the intro runs at
# slomo 0.2 while frames are grabbed as fast as the capture allows; each frame is named by its intro time so a 10 fps
# sequence of normal-speed time can be assembled (struct_clip_assemble picks the nearest frame per 0.1 s).
#   python struct_intro_clip.py <port>
import importlib.util
import os
import re
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
PORT = sys.argv[1]
OUT = os.path.join(KIT, "review", "game", "Canopy149", "clip_raw")
os.environ["TD_SHOT_DIR"] = OUT
os.makedirs(OUT, exist_ok=True)
sys.argv = [sys.argv[0], PORT, "noop"]
spec = importlib.util.spec_from_file_location("op_game", os.path.join(os.path.dirname(KIT), "Outpost", "scripts", "op_game.py"))
G = importlib.util.module_from_spec(spec)
try:
    spec.loader.exec_module(G)
except SystemExit:
    pass
SLOMO = 0.2
G.cmd("DisableAllScreenMessages")
time.sleep(8.0)
G.cmd("TD.Intro.Play")
time.sleep(2.0)
G.cmd("TD.Intro.At 9.0 1")
time.sleep(2.0)
G.cmd("slomo %.2f" % SLOMO)
G.cmd("TD.Intro.Run")
t0 = time.time()
n = 0
while True:
    t_intro = 9.0 + (time.time() - t0) * SLOMO
    if t_intro > 29.0:
        break
    G.shot("f_%06.2f" % t_intro, 1280)
    n += 1
G.cmd("slomo 1")
print("frames", n, "over", round(time.time() - t0, 1), "s")

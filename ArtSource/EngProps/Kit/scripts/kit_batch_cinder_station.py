# SPDX-License-Identifier: MIT
# One -game session: the ART-17 Cinder survey station walk on clear ground (batch U placed it over the magma river).
#   python kit_batch_cinder_station.py <port>
import importlib.util
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
PORT = sys.argv[1]
sys.argv = [sys.argv[0], PORT, "noop"]
spec = importlib.util.spec_from_file_location("op_game", os.path.join(os.path.dirname(KIT), "Outpost", "scripts", "op_game.py"))
G = importlib.util.module_from_spec(spec)
try:
    spec.loader.exec_module(G)
except SystemExit:
    pass
G.cmd("TD.Stage CinderOnFoot")
time.sleep(25.0)
r = subprocess.run([sys.executable, "kit_walk.py", PORT, "SurveyStation_Cinder", "-3500", "2000", "2"], cwd=HERE, capture_output=True, text=True, timeout=900)
print((r.stdout.strip().splitlines() or [r.stderr[-300:]])[-1])

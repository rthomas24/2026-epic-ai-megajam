# SPDX-License-Identifier: MIT
# Batch W, one -game session: the moon shard strata at 1 km (T6's HISM field), the Verdance camp at 5 / 8 / 30 m,
# the Cinder survey station walk with the rack moved off the deck.   python kit_batch_w.py <port>
import importlib.util
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
PORT = sys.argv[1]
os.environ["TD_SHOT_DIR"] = os.path.join(KIT, "review", "game", "BatchW")
os.makedirs(os.environ["TD_SHOT_DIR"], exist_ok=True)
sys.argv = [sys.argv[0], PORT, "noop"]
spec = importlib.util.spec_from_file_location("op_game", os.path.join(os.path.dirname(KIT), "Outpost", "scripts", "op_game.py"))
G = importlib.util.module_from_spec(spec)
try:
    spec.loader.exec_module(G)
except SystemExit:
    pass


def run(args):
    r = subprocess.run([sys.executable] + args, cwd=HERE, capture_output=True, text=True, timeout=900)
    print(args, "->", (r.stdout.strip().splitlines() or [r.stderr[-300:]])[-1])


G.cmd("TD.God 1")
G.cmd("DisableAllScreenMessages")
G.cmd("TD.SpaceField.ViewRegion 6 1")
time.sleep(6.0)
print(G.shot("Shard_HISM_region6_1km_strataW"))
G.cmd("TD.Stage VerdanceOnFoot")
time.sleep(25.0)
run(["kit_walk.py", PORT, "StorySet_VerdanceCamp", "-2500", "2500", "2"])
G.cmd("TD.Stage CinderOnFoot")
time.sleep(25.0)
run(["kit_walk.py", PORT, "SurveyStation_Cinder", "-3500", "2000", "2"])

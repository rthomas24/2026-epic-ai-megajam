# SPDX-License-Identifier: MIT
# One batched -game session for batch R checks (L_Galaxy start; stages planets itself):
#   region set pieces r4 near / 1.5 km and 150 m on the 2x ring and rig (kit_space_shots, TD_SPACE_SET=region),
#   the outpost walk test on Verdance and Cinder at T5's seated outposts (spawned copies here), the Verdance landmark
#   approach and the Cinder camp at 30 m at T5's spots.     python kit_batch_r.py <port>
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PORT = sys.argv[1]
env = dict(os.environ, MSYS_NO_PATHCONV="1")


def run(args, extra=None):
    e = dict(env, **(extra or {}))
    r = subprocess.run([sys.executable] + args, cwd=HERE, env=e, capture_output=True, text=True, timeout=900)
    out = r.stdout.strip().splitlines()
    print(args[0], args[2:], "->", len([l for l in out if ".png" in l or "shot" in l]), "shots;", (out[-1] if out else r.stderr[-300:]))


run(["kit_space_shots.py", PORT], {"TD_SPACE_SET": "region"})
run(["kit_space_shots.py", PORT], {"TD_SPACE_SET": "region", "TD_SPACE_ONLY": "PicketRing", "TD_SPACE_D": "15000"})
run(["kit_space_shots.py", PORT], {"TD_SPACE_SET": "region", "TD_SPACE_ONLY": "ScarRig", "TD_SPACE_D": "15000"})
# Verdance: landmark approach (T5's mast spot), the outpost walk (a spawned copy on clear ground near the pad)
run(["kit_landmark_shots.py", PORT, "Verdance", "13200", "6800", "0"])
run(["kit_walk.py", PORT, "Outpost_Site_Verdance", "1500", "-1500", "2"])
# Cinder: the camp at 30 m (T5's spot), the outpost walk
run(["kit_landmark_shots.py", PORT, "Cinder", "19300", "6000", "0"])
run(["kit_walk.py", PORT, "StorySet_CinderMine", "13500", "2400", "2"])
run(["kit_walk.py", PORT, "Outpost_Site_Cinder", "3000", "-2700", "2"])

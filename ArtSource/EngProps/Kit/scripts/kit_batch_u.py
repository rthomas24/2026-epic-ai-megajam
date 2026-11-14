# SPDX-License-Identifier: MIT
# One -game session: ART-17 station walks (Verdance, Cinder) and each squad ship parked on its gear (fixed spot on
# Verdance near the pad, third person), plus the debris shroud at 100 m.   python kit_batch_u.py <port>
import importlib.util
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
PORT = sys.argv[1]
env = dict(os.environ, MSYS_NO_PATHCONV="1")


def run(args, extra=None):
    r = subprocess.run([sys.executable] + args, cwd=HERE, env=dict(env, **(extra or {})), capture_output=True, text=True, timeout=900)
    out = r.stdout.strip().splitlines()
    print(args, "->", (out[-1] if out else r.stderr[-300:]))


os.environ["TD_SHOT_DIR"] = os.path.join(KIT, "review", "game", "Gear")
os.makedirs(os.environ["TD_SHOT_DIR"], exist_ok=True)
sys.argv = [sys.argv[0], PORT, "noop"]
spec = importlib.util.spec_from_file_location("op_game", os.path.join(os.path.dirname(KIT), "Outpost", "scripts", "op_game.py"))
G = importlib.util.module_from_spec(spec)
try:
    spec.loader.exec_module(G)
except SystemExit:
    pass

# the moon shards as T6's field places them (HISM, the usage-flag fix 83a231c9): region 6 at 1 km and 150 m
G.cmd("TD.God 1")
G.cmd("DisableAllScreenMessages")
for rng, tag in ((1, "1km"), (2, "150m")):
    G.cmd("TD.SpaceField.ViewRegion 6 %d" % rng)
    time.sleep(6.0)
    print(G.shot("Shard_HISM_region6_%s" % tag))
run(["kit_space_shots.py", PORT], {"TD_SPACE_SET": "region", "TD_SPACE_ONLY": "DomDebris", "TD_SPACE_D": "10000"})
G.cmd("TD.Stage VerdanceOnFoot")
time.sleep(25.0)
run(["kit_walk.py", PORT, "StorySet_VerdanceCamp", "-2500", "2500", "2"])
run(["kit_walk.py", PORT, "SurveyStation_Verdance", "2500", "-2500", "2"])
G.cmd("TD.Ground.ThirdPerson 0")
G.cmd("DisableAllScreenMessages")
for i, ship in enumerate(("Rift", "Lancer", "Bastion")):
    x, y = 1800.0 + i * 900.0, 1500.0
    G.cmd("TD.Dev.SpawnClass /Game/TriadDrift/Gameplay/Props/Kit/Sites/BP_ShipGear_%s Gear %.0f %.0f 2" % (ship, x, y))
    time.sleep(2.5)
    G.cmd("TD.Ground.Teleport %.0f %.0f 92" % (x - 520.0, y - 420.0))
    G.cmd("TD.Look 38 -6")
    time.sleep(2.0)
    print(G.shot("%s_parked_on_gear" % ship))
G.cmd("TD.Ground.ThirdPerson 1")
G.cmd("TD.Stage CinderOnFoot")
time.sleep(25.0)
run(["kit_walk.py", PORT, "SurveyStation_Cinder", "4000", "3000", "2"])

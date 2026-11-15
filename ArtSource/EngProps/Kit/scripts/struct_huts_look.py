# SPDX-License-Identifier: MIT
# #194: each planet's camp pieces in a row (BP_HutsCheck_<Planet>, capture only) beside the pilot, third person, plus a
# night-ish close-up of a lit window. python struct_huts_look.py <port>
import importlib.util
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
PORT = sys.argv[1]
OUT = os.path.join(KIT, "review", "game", "Huts194")
os.environ["TD_SHOT_DIR"] = OUT
os.makedirs(OUT, exist_ok=True)
sys.argv = [sys.argv[0], PORT, "noop"]
spec = importlib.util.spec_from_file_location("op_game", os.path.join(os.path.dirname(KIT), "Outpost", "scripts", "op_game.py"))
G = importlib.util.module_from_spec(spec)
try:
    spec.loader.exec_module(G)
except SystemExit:
    pass
G.cmd("TD.God 1")
G.cmd("DisableAllScreenMessages")
for planet, stage, (ox, oy) in (("Verdance", "VerdanceOnFoot", (-6000.0, -6000.0)), ("Cinder", "CinderOnFoot", (-6000.0, -9000.0))):
    G.cmd("TD.Stage %s" % stage)
    time.sleep(28.0)
    G.cmd("TD.Ground.ThirdPerson 1")
    G.cmd("TD.Dev.SpawnClass /Game/TriadDrift/Gameplay/Props/Kit/Sites/BP_HutsCheck_%s Huts %.0f %.0f 0" % (planet, ox, oy))
    time.sleep(3.0)
    # the row runs along +X from the origin (9 m apart); stand in front of the first three, facing them
    for i, (dx, look) in enumerate(((900.0, 90.0), (3600.0, 90.0), (6300.0, 90.0))):
        G.cmd("TD.Ground.Teleport %.0f %.0f %.0f" % (ox + dx, oy - 1100.0, 92.0))
        G.cmd("TD.Look %.1f -4" % look)
        time.sleep(2.0)
        print(G.shot("%s_row_%d" % (planet.lower(), i)))
    G.cmd("TD.Ground.Teleport %.0f %.0f %.0f" % (ox + 450.0, oy - 600.0, 92.0))
    G.cmd("TD.Look 135 -2")
    time.sleep(2.0)
    print(G.shot("%s_hut_close" % planet.lower()))

# Kit r10 - reactor-room readability captures: a Grunt, a Trooper and the pilot at 30 / 15 / 5 m, and the artefact at its
# core-chamber spot (drives a -game session on port 21014 in L_Dominion_Interior after kit_walk.py has spawned the preview).
import importlib.util, os, sys, time
OUT = r"C:\Users\ryant\Documents\myRepos\game-jam\.wt\struct\ArtSource\EngProps\Kit\review\game\DominionInterior_Preview"
os.environ["TD_SHOT_DIR"] = OUT
sys.argv = [sys.argv[0], "21014", "noop"]
spec = importlib.util.spec_from_file_location("op_game", r"C:\Users\ryant\Documents\myRepos\game-jam\.wt\struct\ArtSource\EngProps\Outpost\scripts\op_game.py")
G = importlib.util.module_from_spec(spec)
try:
    spec.loader.exec_module(G)
except SystemExit:
    pass
OY = 30000.0
G.cmd("TD.Ground.ThirdPerson 1")
G.cmd("TD.God 1")
for d, name in ((3000, "11_core_crew_30m"), (1500, "12_core_crew_15m"), (500, "13_core_crew_5m")):
    G.cmd("slomo 1")
    G.cmd("KillAll TDCrewCharacter")
    time.sleep(0.5)
    px = 14500.0 - d - 150.0 if d < 3000 else 11550.0
    G.cmd("TD.Ground.Teleport %.0f %.0f 92" % (px, OY + 1300))
    off = 4.0 if d >= 1500 else 14.0
    G.cmd("TD.Look %.1f 0" % -off)
    time.sleep(0.6)
    G.cmd("TD.Ground.Spawn Grunt %d" % d)
    G.cmd("TD.Look %.1f 0" % off)
    time.sleep(0.3)
    G.cmd("TD.Ground.Spawn Trooper %d" % d)
    G.cmd("TD.Look 0 2")
    time.sleep(0.4)
    G.cmd("slomo 0.02")
    time.sleep(1.5)
    G.cmd("TD.Ground.Crew")
    time.sleep(0.3)
    print(G.log_tail(r"TD\.Ground\.Crew", 1))
    print(G.shot(name))
G.cmd("slomo 1")
G.cmd("KillAll TDCrewCharacter")
# the artefact at its core-chamber spot, from the core door
G.cmd("TD.Ground.Teleport 11700 %.0f 92" % (OY + 250))
G.cmd("TD.Look 24 4")
time.sleep(2.0)
print(G.shot("14_core_artefact_from_door"))
G.cmd("TD.Ground.Teleport 13700 %.0f 92" % (OY + 800))
G.cmd("TD.Look 50 6")
time.sleep(2.0)
print(G.shot("15_core_artefact_close"))

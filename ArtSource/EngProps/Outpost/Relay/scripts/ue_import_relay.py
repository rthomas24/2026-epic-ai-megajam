# SPDX-License-Identifier: MIT
# Triad Drift - import the Ancient Relay (Grok Bot's mesh + the collision from relay_build.py).
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Outpost\Relay\scripts\ue_import_relay.py
# Writes only /Game/TriadDrift/Gameplay/Props/Relay: SM_Relay, Materials/M_Relay, MI_Relay (dormant, RelayPower 0.15),
# MI_Relay_Awake (RelayPower 1), Textures/T_Relay_BC, _N (2K), _ORM, _E (1K), BP_Relay, BP_Relay_Awake.
# The game wakes it with a dynamic instance of slot 0: SetScalarParameterValue("RelayPower", 0.15 .. 1).
# Emissive = T_Relay_E x GlowColor x GlowStrength (40) x RelayPower^2: dormant 0.9, awake 40 (r2; r1 was linear x 10,
# and dormant and awake looked the same in the game).
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
sys.path.insert(0, os.path.join(os.path.dirname(KIT), "scripts"))
import ue_import_gb_prop as G
REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(KIT))))
SRC = os.path.join(REPO, "ArtSource", "EngAI", "Props", "AncientRelay", "mesh_r1")
G.run({
    "dest": "/Game/TriadDrift/Gameplay/Props/Relay", "name": "Relay",
    "fbx": os.path.join(KIT, "export", "SM_Relay.fbx"), "manifest": os.path.join(KIT, "export", "SM_Relay.manifest.json"),
    "textures": {"BC": (os.path.join(SRC, "T_AncientRelay_BC.png"), "color"), "N": (os.path.join(SRC, "T_AncientRelay_N.png"), "normal"),
                 "ORM": (os.path.join(KIT, "textures", "T_Relay_ORM.png"), "masks"), "E": (os.path.join(KIT, "textures", "T_Relay_E.png"), "color")},
    "power_param": "RelayPower", "power_squared": True,
    "instances": {"MI_Relay": {"scalars": {"RelayPower": 0.15, "GlowStrength": 40.0}},
                  "MI_Relay_Awake": {"scalars": {"RelayPower": 1.0, "GlowStrength": 40.0}}},
    "actors": {"BP_Relay": "MI_Relay", "BP_Relay_Awake": "MI_Relay_Awake"},
    "report": os.path.join(KIT, "review", "ue_import_report.json"),
})

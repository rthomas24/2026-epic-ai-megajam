# SPDX-License-Identifier: MIT
# Triad Drift - import the Verdance wreck (Grok Bot's mesh + ramp, lights and collision from wreck_build.py).
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Outpost\Wreck\scripts\ue_import_wreck.py
# Writes only /Game/TriadDrift/Gameplay/Props/Wreck: SM_Wreck, Materials/M_Wreck, MI_Wreck, Textures/T_Wreck_BC, _N
# (2K), _ORM, _E (1K), BP_Wreck. The failing lights: scalar LightPower (0 = off, 1 = on) x GlowStrength, GlowColor.
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
sys.path.insert(0, os.path.join(os.path.dirname(KIT), "scripts"))
import ue_import_gb_prop as G
REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(KIT))))
SRC = os.path.join(REPO, "ArtSource", "EngAI", "Props", "VerdanceWreck", "mesh_r1")
G.run({
    "dest": "/Game/TriadDrift/Gameplay/Props/Wreck", "name": "Wreck",
    "fbx": os.path.join(KIT, "export", "SM_Wreck.fbx"), "manifest": os.path.join(KIT, "export", "SM_Wreck.manifest.json"),
    "textures": {"BC": (os.path.join(SRC, "T_VerdanceWreck_BC.png"), "color"), "N": (os.path.join(SRC, "T_VerdanceWreck_N.png"), "normal"),
                 "ORM": (os.path.join(KIT, "textures", "T_Wreck_ORM.png"), "masks"), "E": (os.path.join(KIT, "textures", "T_Wreck_E.png"), "color")},
    "power_param": "LightPower",
    "instances": {"MI_Wreck": {"scalars": {"LightPower": 1.0, "GlowStrength": 14.0}, "vectors": {"GlowColor": (1.0, 0.5, 0.16)}}},
    "actors": {"BP_Wreck": "MI_Wreck"},
    "report": os.path.join(KIT, "review", "ue_import_report.json"),
})

# SPDX-License-Identifier: MIT
# Triad Drift - run the relay and the wreck imports in ONE headless editor session (each editor start waits on the
# shared UnrealBuildTool mutex, which other worktrees hold for minutes at a time).
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Outpost\scripts\ue_import_relay_wreck.py
import os, runpy
KIT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
for rel in ("Relay/scripts/ue_import_relay.py", "Wreck/scripts/ue_import_wreck.py"):
    runpy.run_path(os.path.join(KIT, rel), run_name="__main__")

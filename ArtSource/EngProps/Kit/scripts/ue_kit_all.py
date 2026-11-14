# SPDX-License-Identifier: MIT
# Triad Drift - everything for the kit in ONE headless editor session (each editor start waits on the shared
# UnrealBuildTool mutex): import the kit, re-import the relay (RelayPower^2 emissive), rebuild the site classes.
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Kit\scripts\ue_kit_all.py
import os
import runpy

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
PROPS = os.path.dirname(os.path.dirname(HERE))
runpy.run_path(os.path.join(HERE, "ue_import_kit.py"), run_name="__main__")
runpy.run_path(os.path.join(PROPS, "Outpost", "Relay", "scripts", "ue_import_relay.py"), run_name="__main__")
runpy.run_path(os.path.join(HERE, "ue_make_sites.py"), run_name="__main__")
unreal.log("TDPY: kit_all finished (each step logs its own OK / FAILED above)")

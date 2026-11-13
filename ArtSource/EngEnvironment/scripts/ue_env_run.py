# SPDX-License-Identifier: MIT
# Run several environment build scripts in one editor session (TD_ENV_RUN = comma-separated script names in this
# folder; default: terrain, atmosphere fade, cloud deck).
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngEnvironment\scripts\ue_env_run.py
import os
import runpy

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
names = os.environ.get("TD_ENV_RUN", "ue_terrain_r2_build.py,ue_atmo_fade_build.py,ue_cloud_deck_build.py").split(",")
for n in names:
    n = n.strip()
    if not n:
        continue
    unreal.log("TDPY: --- " + n)
    try:
        runpy.run_path(os.path.join(HERE, n), run_name="__main__")
    except Exception as e:  # noqa: BLE001
        unreal.log("TDPY: FAILED %s: %s" % (n, e))

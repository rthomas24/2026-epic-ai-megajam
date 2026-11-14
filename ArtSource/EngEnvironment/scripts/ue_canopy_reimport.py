# SPDX-License-Identifier: MIT
# ue_env_run.py entry for the canopy clumps' re-import (canopy/ue_import_clumps.py): lets one editor session rebuild
# materials and re-import the clumps together (one rendering turn).
import os
import runpy

runpy.run_path(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "canopy", "ue_import_clumps.py"), run_name="__main__")

"""Re-export the #53 hero crystal cluster r2 FBX from its editable .blend (Blender 4.5.3 LTS).

    blender -b SM_CrystalCacheCrystal.blend --python rebuild.py

Uses the repo's export_for_unreal.py: live Bevel + WeightedNormal are applied on a temporary copy, SRC_* is skipped,
UCX_* becomes collision, SOCKET_* becomes a static-mesh socket, FBX UnitScaleFactor 100 (metres -> Unreal cm).
"""
import runpy
from pathlib import Path
import bpy

here = Path(bpy.data.filepath).resolve().parent
repo = next(p for p in here.parents if (p / ".claude").is_dir())
exporter = runpy.run_path(str(repo / ".claude/skills/blender-unreal-export/scripts/export_for_unreal.py"))
root = Path(bpy.data.filepath).stem  # SM_CrystalCacheCrystal
print(exporter["main"](root=root, out_dir=str(here), front_axis="+X", collision="auto"))

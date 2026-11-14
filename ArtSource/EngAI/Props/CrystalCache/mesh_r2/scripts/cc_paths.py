"""Paths and constants for #53 hero crystal cluster r2 (build-time /tmp paths)."""
from pathlib import Path
import bpy
from mathutils import Matrix, Vector

REPO = Path("/tmp/wt-crystal-cluster-r2")
R1 = REPO / "ArtSource/EngAI/Props/CrystalCache/mesh_r1"
OUT = REPO / "ArtSource/EngAI/Props/CrystalCache/mesh_r2"
WORK = Path("/tmp/cc2")
TEX = WORK / "tex"
RENDERS = WORK / "renders"
EXPORT_PY = REPO / ".claude/skills/blender-unreal-export/scripts/export_for_unreal.py"
HARDSURFACE_PY = REPO / ".claude/skills/blender-game-asset/scripts/hardsurface.py"
HDRI = Path("/home/box/.local/blender-4.5.3-linux-x64/4.5/datafiles/studiolights/world/studio.exr")
REF_PNG = OUT / "refs/Crystal_Cache_Modeling_Reference.png"
for d in (TEX, RENDERS, OUT):
    d.mkdir(parents=True, exist_ok=True)

SOCKET_Z = 0.3005          # SOCKET_Crystal on the r1 cradle floor (measured)
CAGE_RAIL_Z = 1.678        # underside of the cradle's top rails / cross bars (ray-probed)
MAX_TOP_LOCAL = 1.32       # cluster top <= 1.32 m -> 162 cm assembled, >= 5.8 cm under the rails
CRADLE_LEN = 3.60
CRADLE_WID = 2.00
CRADLE_HGT = 1.80
PILOT_H = 1.80
TEX_SIZE = 2048
BAKE_SAMPLES = 16
CAGE_EXTRUDE = 0.040
CAGE_RAY = 0.140

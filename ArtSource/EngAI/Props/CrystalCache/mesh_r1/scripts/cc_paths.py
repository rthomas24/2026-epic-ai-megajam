from pathlib import Path
import bpy
from mathutils import Matrix, Vector

REPO = Path("/tmp/wt-crystal-cache-r1")
OUT = REPO / "ArtSource/EngAI/Props/CrystalCache/mesh_r1"
SRC_CRADLE_FBX = OUT / "source/cradle/base_basic_pbr.fbx"
SRC_CRADLE_TEX = OUT / "source/cradle"
WORK = Path("/tmp/crystal-cache-build")
TEX = WORK / "tex"
RENDERS = WORK / "renders"
EXPORT_PY = REPO / ".claude/skills/blender-unreal-export/scripts/export_for_unreal.py"
HDRI = Path("/home/box/.local/blender-4.5.3-linux-x64/4.5/datafiles/studiolights/world/studio.exr")
REF_PNG = OUT / "refs/Crystal_Cache_Modeling_Reference.png"
for d in (TEX, RENDERS, OUT):
    d.mkdir(parents=True, exist_ok=True)

CRADLE_LEN = 3.60
CRADLE_WID = 2.00
CRADLE_HGT = 1.80
CLUSTER_HGT = 1.40
PILOT_H = 1.80
DISSOLVE_DEG = 12.0
SHARP_DEG = 30.0
BEVEL_WIDTH = 0.006
BEVEL_SEGS = 2
BEVEL_ANGLE = 70.0
TEX_SIZE = 2048
BAKE_SAMPLES = 16
CAGE_EXTRUDE = 0.040
CAGE_RAY = 0.140

# Triad Drift - Verdance wreck: derived textures (system Python + Pillow). Run after wreck_build.py.
#   T_Wreck_ORM.png  1K copy of Grok Bot's 2K ORM
#   T_Wreck_E.png    1K, black except the texel block the three added light boxes are mapped to
import json, os
from PIL import Image, ImageDraw
HERE = os.path.dirname(os.path.abspath(__file__)); KIT = os.path.dirname(HERE)
REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(KIT))))
SRC = os.path.join(REPO, "ArtSource", "EngAI", "Props", "VerdanceWreck", "mesh_r1")
os.makedirs(os.path.join(KIT, "textures"), exist_ok=True)
Image.open(os.path.join(SRC, "T_VerdanceWreck_ORM.png")).convert("RGB").resize((1024, 1024), Image.LANCZOS).save(os.path.join(KIT, "textures", "T_Wreck_ORM.png"))
with open(os.path.join(KIT, "review", "build_r1.json")) as fh:
    b = json.load(fh)
u, v = b["glow_uv"]; half = b["glow_block_uv"] * 0.5 * 0.75
e = Image.new("RGB", (1024, 1024), (0, 0, 0))
ImageDraw.Draw(e).rectangle([(u - half) * 1024, (1 - v - half) * 1024, (u + half) * 1024, (1 - v + half) * 1024], fill=(255, 255, 255))
e.save(os.path.join(KIT, "textures", "T_Wreck_E.png"))
print("ok", u, v)

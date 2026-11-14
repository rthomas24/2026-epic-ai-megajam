# SPDX-License-Identifier: MIT
# Downscale Grok Bot's 2K terrain sets (ArtSource/EngAI/Terrain, read-only) to 1K BC + N PNGs for import and write
# each base colour's mean (linear) to terrain_means.json. Output goes to Saved/EnvTerrainTex (not in git: it is
# rebuilt from the sources by this script).  Needs Pillow:  python ArtSource/EngEnvironment/scripts/prep_terrain_textures.py
import json
import os

from PIL import Image, ImageStat

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
SRC = os.path.join(REPO, "ArtSource", "EngAI", "Terrain")
OUT = os.path.join(REPO, "Saved", "EnvTerrainTex")
SIZE = 1024
SETS = ["Verdance_Moss", "Verdance_Soil", "Verdance_CliffRock", "Verdance_PeakLichenFrost",
        "Cinder_Ash", "Cinder_Basalt", "Cinder_CliffRock", "Cinder_LavaCrust", "Cinder_Obsidian"]


def lin(c):
    c /= 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def main():
    os.makedirs(OUT, exist_ok=True)
    means = {}
    for s in SETS:
        for suffix in ("BC", "N"):
            src = os.path.join(SRC, s, "T_%s_%s.png" % (s, suffix))
            im = Image.open(src).convert("RGB").resize((SIZE, SIZE), Image.LANCZOS)
            im.save(os.path.join(OUT, "T_Terr_%s_%s.png" % (s, suffix)))
            if suffix == "BC":
                means[s] = [round(lin(v), 4) for v in ImageStat.Stat(im).mean]
    with open(os.path.join(HERE, "terrain_means.json"), "w") as f:
        json.dump(means, f, indent=1)
    print(json.dumps(means))


main()

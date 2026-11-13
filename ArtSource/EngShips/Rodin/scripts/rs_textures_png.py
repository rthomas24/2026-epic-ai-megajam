# SPDX-License-Identifier: MIT
# Wraps the raw RGB8 dumps written by rs_textures.py as PNG (system Python with Pillow) and removes the dumps.
import glob
import os
import re

from PIL import Image

OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "textures")
for raw in glob.glob(os.path.join(OUT, "*.rgb8")):
    name, w, h = re.match(r"(.+)_(\d+)x(\d+)\.rgb8$", os.path.basename(raw)).groups()
    with open(raw, "rb") as fh:
        img = Image.frombytes("RGB", (int(w), int(h)), fh.read())
    img.save(os.path.join(OUT, name + ".png"), optimize=True)
    os.remove(raw)
    print(name, img.size, img.getextrema())

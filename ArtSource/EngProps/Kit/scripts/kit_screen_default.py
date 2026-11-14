# SPDX-License-Identifier: MIT
# Kit - the console screen's default image (T_Kit_ScreenDefault.png, 512 x 256): a dim telemetry readout (grid,
# waveform, level bars), original work. Gameplay replaces it through M_Kit_Screen's ScreenTexture.
#   python ArtSource/EngProps/Kit/scripts/kit_screen_default.py
import math
import os

from PIL import Image, ImageDraw

KIT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W, H = 512, 256
im = Image.new("RGB", (W, H), (4, 14, 16))
d = ImageDraw.Draw(im)
for x in range(0, W, 32):
    d.line([(x, 0), (x, H)], fill=(8, 34, 38))
for y in range(0, H, 32):
    d.line([(0, y), (W, y)], fill=(8, 34, 38))
pts = [(x, 96 + 40 * math.sin(x / 23.0) * math.exp(-((x - 200) / 160.0) ** 2) + 8 * math.sin(x / 5.0)) for x in range(16, 330)]
d.line(pts, fill=(60, 230, 210), width=3)
for i, h in enumerate((60, 92, 40, 120, 76, 104)):
    x = 360 + i * 22
    d.rectangle([x, 220 - h, x + 14, 220], fill=(30, 160, 150))
d.rectangle([12, 182, 330, 188], fill=(20, 90, 90))
d.rectangle([12, 182, 230, 188], fill=(70, 240, 220))
d.rectangle([2, 2, W - 3, H - 3], outline=(40, 140, 140), width=2)
out = os.path.join(KIT, "textures", "T_Kit_ScreenDefault.png")
im.save(out)
print("wrote", out)

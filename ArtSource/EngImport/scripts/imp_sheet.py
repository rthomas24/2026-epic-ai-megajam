# SPDX-License-Identifier: MIT
# Triad Drift - import builder: one contact sheet from in-game captures.
#   python imp_sheet.py <out.png> "<title>" <img1> "<label1>" [<img2> "<label2>" ...]
import sys

from PIL import Image, ImageDraw, ImageFont

out, title, rest = sys.argv[1], sys.argv[2], sys.argv[3:]
pairs = list(zip(rest[0::2], rest[1::2]))
W = 960                      # each tile
cols = 2 if len(pairs) > 1 else 1
tiles = []
for path, label in pairs:
    im = Image.open(path).convert("RGB")
    im = im.resize((W, int(im.height * W / im.width)))
    tiles.append((im, label))
th = max(t[0].height for t in tiles)
rows = (len(tiles) + cols - 1) // cols
pad, head, cap = 12, 56, 34
sheet = Image.new("RGB", (cols * W + (cols + 1) * pad, head + rows * (th + cap + pad) + pad), (24, 25, 29))
d = ImageDraw.Draw(sheet)
try:
    f1 = ImageFont.truetype("arialbd.ttf", 26)
    f2 = ImageFont.truetype("arial.ttf", 18)
except OSError:
    f1 = f2 = ImageFont.load_default()
d.text((pad, 14), title, fill=(240, 240, 240), font=f1)
for i, (im, label) in enumerate(tiles):
    x = pad + (i % cols) * (W + pad)
    y = head + (i // cols) * (th + cap + pad)
    sheet.paste(im, (x, y))
    d.text((x + 4, y + im.height + 6), label, fill=(210, 210, 210), font=f2)
sheet.save(out, optimize=True)
print(out, sheet.size)

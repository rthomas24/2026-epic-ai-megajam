# Stack Blender contact strips (Saved/EngAnim/strips/<clip>_strip.png: 6 frames, front row over right-side row) into
# one labelled review sheet.  Usage: python compose.py <out.png> <title> <clip> <clip> ...   (env SCALE, default 0.5)
import os, sys
from PIL import Image, ImageDraw, ImageFont
out, title, clips = sys.argv[1], sys.argv[2], sys.argv[3:]
SC = float(os.environ.get("SCALE", "0.5")); D = "Saved/EngAnim/strips/"
try:
    font = ImageFont.truetype("arialbd.ttf", 15)
except Exception:
    font = ImageFont.load_default()
ims = []
for c in clips:
    p = D + c + "_strip.png"
    if not os.path.isfile(p):
        print("missing", c); continue
    im = Image.open(p).convert("RGB")
    ims.append((c, im.resize((int(im.size[0] * SC), int(im.size[1] * SC)), Image.LANCZOS)))
COLS = int(os.environ.get("COLS", "2"))
w = max(i.size[0] for _, i in ims); h = max(i.size[1] for _, i in ims) + 20
rows = (len(ims) + COLS - 1) // COLS
sheet = Image.new("RGB", (COLS * (w + 6) + 6, 30 + rows * (h + 4)), (18, 20, 26))
d = ImageDraw.Draw(sheet); d.text((8, 6), title, fill=(255, 255, 255), font=font)
for k, (c, im) in enumerate(ims):
    x = 6 + (k % COLS) * (w + 6); y = 30 + (k // COLS) * (h + 4)
    d.text((x + 2, y + 2), c, fill=(255, 220, 120), font=font); sheet.paste(im, (x, y + 20))
os.makedirs(os.path.dirname(out), exist_ok=True)
sheet.convert("P", palette=Image.ADAPTIVE, colors=128).save(out, optimize=True)
print("SHEET", out, sheet.size, os.path.getsize(out) // 1024, "KB")

# Animation audit: assemble capture bursts into one labelled sheet (one row per move).
# Usage: python sheet.py <out.png> <shots dir> <title> <label>=<prefix>[@crop] ...
#   every file <prefix>_NN.png in the shots dir is one cell; crop is "x0,y0,x1,y1" as fractions (default: the
#   character area of the third-person view). Rows keep at most 8 cells (evenly picked).
import glob, os, sys
from PIL import Image, ImageDraw, ImageFont

out, shots, title = sys.argv[1], sys.argv[2], sys.argv[3]
CELL_H = int(os.environ.get("SHEET_CELL_H", "250"))
MAXC = int(os.environ.get("SHEET_MAX", "8"))
try:
    font = ImageFont.truetype("arial.ttf", 16); big = ImageFont.truetype("arialbd.ttf", 22)
except Exception:
    font = big = ImageFont.load_default()
rows = []
for spec in sys.argv[4:]:
    label, rest = spec.split("=", 1)
    prefix, crop = (rest.split("@") + ["0.12,0.1,0.88,1.0"])[:2]
    c = [float(v) for v in crop.split(",")]
    files = sorted(glob.glob(os.path.join(shots, prefix + "_[0-9][0-9].png")))
    if not files and os.path.isfile(os.path.join(shots, prefix + ".png")):
        files = [os.path.join(shots, prefix + ".png")]
    if len(files) > MAXC:
        files = [files[int(round(i * (len(files) - 1) / (MAXC - 1)))] for i in range(MAXC)]
    cells = []
    for f in files:
        im = Image.open(f).convert("RGB")
        w, h = im.size
        im = im.crop((int(c[0] * w), int(c[1] * h), int(c[2] * w), int(c[3] * h)))
        im = im.resize((int(im.size[0] * CELL_H / im.size[1]), CELL_H), Image.LANCZOS)
        cells.append(im)
    rows.append((label, cells))
W = max([sum(c.size[0] + 4 for c in cells) for _, cells in rows] + [600]) + 8
H = 40 + sum(CELL_H + 26 for _ in rows) + 8
sheet = Image.new("RGB", (W, H), (18, 20, 26))
d = ImageDraw.Draw(sheet)
d.text((8, 8), title, fill=(255, 255, 255), font=big)
y = 40
for label, cells in rows:
    d.text((8, y + 3), label, fill=(255, 220, 120), font=font)
    x = 4
    for c in cells:
        sheet.paste(c, (x, y + 24)); x += c.size[0] + 4
    y += CELL_H + 26
os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
sheet.save(out, optimize=True)
print("SHEET", out, sheet.size)

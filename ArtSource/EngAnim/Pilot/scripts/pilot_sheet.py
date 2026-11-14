# Triad Drift - one review sheet per move from pilot_capture.py frames: a row per view, up to N evenly picked cells,
# each labelled with its frame index. python pilot_sheet.py <frames dir> <out dir> [cells=10] [first=0] [last=-1]
# Animation builder (eng/anim), Nov 14.
import glob
import os
import re
import sys
from PIL import Image, ImageDraw, ImageFont

src, out = sys.argv[1], sys.argv[2]
N = int(sys.argv[3]) if len(sys.argv) > 3 else 10
os.makedirs(out, exist_ok=True)
try:
    font = ImageFont.truetype("arial.ttf", 14)
    big = ImageFont.truetype("arialbd.ttf", 18)
except Exception:  # noqa: BLE001
    font = big = ImageFont.load_default()
moves = {}
for f in glob.glob(os.path.join(src, "*.png")):
    m = re.match(r"(.+)_(chase|side|front)_(\d+)\.png$", os.path.basename(f))
    if m:
        moves.setdefault(m.group(1), {}).setdefault(m.group(2), []).append((int(m.group(3)), f))
for move, views in sorted(moves.items()):
    rows = []
    for view in ("chase", "side", "front"):
        if view not in views:
            continue
        fr = sorted(views[view])
        pick = [fr[round(i * (len(fr) - 1) / max(1, N - 1))] for i in range(min(N, len(fr)))]
        cells = []
        for idx, f in pick:
            im = Image.open(f).convert("RGB")
            w, h = im.size
            im = im.crop((int(w * 0.2), int(h * 0.05), int(w * 0.8), h)) if view == "chase" else im.crop((int(w * 0.25), 0, int(w * 0.75), h))
            im = im.resize((int(im.width * 300 / im.height), 300))
            ImageDraw.Draw(im).text((4, 4), "%s %d" % (view, idx), fill=(255, 255, 0), font=font)
            cells.append(im)
        rows.append(cells)
    W = max(sum(c.width for c in r) for r in rows)
    sheet = Image.new("RGB", (W, 30 + 300 * len(rows)), (20, 20, 20))
    ImageDraw.Draw(sheet).text((6, 5), move, fill=(255, 255, 255), font=big)
    for ri, r in enumerate(rows):
        x = 0
        for c in r:
            sheet.paste(c, (x, 30 + ri * 300))
            x += c.width
    sheet.save(os.path.join(out, "sheet_%s.png" % move))
    print("sheet", move, [len(v) for v in views.values()])

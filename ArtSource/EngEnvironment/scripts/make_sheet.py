# SPDX-License-Identifier: MIT
# Contact sheet of captures with labels:  python make_sheet.py <out.jpg> <cols> <cell width> <img>[=label] ...
import os
import sys

from PIL import Image, ImageDraw, ImageFont


def main():
    out, cols, cw = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
    items = []
    for a in sys.argv[4:]:
        p, _, label = a.partition("=")
        items.append((p, label or os.path.splitext(os.path.basename(p))[0]))
    ims = [Image.open(p).convert("RGB") for p, _ in items]
    ch = int(cw * ims[0].height / ims[0].width)
    rows = (len(ims) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * cw, rows * (ch + 26)), (18, 18, 22))
    d = ImageDraw.Draw(sheet)
    try:
        font = ImageFont.truetype("arial.ttf", 18)
    except OSError:
        font = ImageFont.load_default()
    for i, (im, (_, label)) in enumerate(zip(ims, items)):
        x, y = (i % cols) * cw, (i // cols) * (ch + 26)
        sheet.paste(im.resize((cw, ch), Image.LANCZOS), (x, y + 26))
        d.text((x + 6, y + 3), label, fill=(235, 235, 235), font=font)
    sheet.save(out, quality=88)
    print(out)


if __name__ == "__main__":
    main()

# Triad Drift - before / after contact sheet of the pilot's hand captures (grip_capture.py output).
# python contact_sheet.py <before_dir> <after_dir> <out.png> <pose:view> [<pose:view> ...]
# One row per pose: before close-up | after close-up | before chase camera | after chase camera.
# Animation builder (eng/anim), Nov 13.
import os
import sys

from PIL import Image, ImageDraw, ImageFont


def main():
    before, after, out = sys.argv[1:4]
    rows = [a.split(":") for a in sys.argv[4:]] or [["idle", "front"], ["walk", "front"], ["light1_mid", "behind"], ["block", "front"]]
    tw, th = 640, 360
    pad, head = 6, 30
    cols = [(before, "before", None, "BEFORE close-up"), (after, "after", None, "AFTER close-up"),
            (before, "before", "chase", "BEFORE chase camera"), (after, "after", "chase", "AFTER chase camera")]
    sheet = Image.new("RGB", (pad + len(cols) * (tw + pad), head + len(rows) * (th + pad) + pad), (18, 18, 22))
    d = ImageDraw.Draw(sheet)
    try:
        font = ImageFont.truetype("arial.ttf", 18)
    except OSError:
        font = ImageFont.load_default()
    for j, (_, tag, _, label) in enumerate(cols):
        d.text((pad + j * (tw + pad), 5), label, fill=(255, 140, 120) if tag == "before" else (140, 255, 160), font=font)
    for i, (pose, view) in enumerate(rows):
        y = head + i * (th + pad)
        for j, (folder, tag, fixed, _) in enumerate(cols):
            v = fixed or view
            p = os.path.join(folder, "%s_%s_%s.png" % (tag, pose, v))
            x = pad + j * (tw + pad)
            if os.path.exists(p):
                im = Image.open(p).convert("RGB")
                im.thumbnail((tw, th))
                sheet.paste(im, (x, y))
            else:
                d.rectangle([x, y, x + tw, y + th], outline=(90, 90, 90))
                d.text((x + 10, y + 30), "missing " + os.path.basename(p), fill=(200, 200, 200), font=font)
            d.text((x + 8, y + 6), "%s / %s" % (pose, v), fill=(255, 255, 0), font=font)
    sheet.save(out)
    print("sheet", out, sheet.size)


if __name__ == "__main__":
    main()

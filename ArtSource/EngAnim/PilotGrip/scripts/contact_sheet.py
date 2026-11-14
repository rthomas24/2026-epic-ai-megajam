# Triad Drift - before / after contact sheet of the pilot's hand captures (grip_capture.py output).
# python contact_sheet.py <before_dir> <after_dir> <out.png> <view> [pose ...]
# One row per pose: before | after, labelled. Animation builder (eng/anim), Nov 13.
import os
import sys

from PIL import Image, ImageDraw, ImageFont


def main():
    before, after, out, view = sys.argv[1:5]
    poses = sys.argv[5:] or ["idle", "walk", "light1_mid", "block"]
    tw, th = 800, 450
    pad, head = 6, 28
    sheet = Image.new("RGB", (pad + 2 * (tw + pad), head + len(poses) * (th + pad) + pad), (18, 18, 22))
    d = ImageDraw.Draw(sheet)
    try:
        font = ImageFont.truetype("arial.ttf", 18)
    except OSError:
        font = ImageFont.load_default()
    d.text((pad, 4), "BEFORE (open hand, by-eye offset on hand_r)", fill=(255, 140, 120), font=font)
    d.text((pad * 2 + tw, 4), "AFTER (A_Pilot_Grip_R layer, BladeSocket)", fill=(140, 255, 160), font=font)
    for i, pose in enumerate(poses):
        y = head + i * (th + pad)
        for j, (folder, tag) in enumerate(((before, "before"), (after, "after"))):
            p = os.path.join(folder, "%s_%s_%s.png" % (tag, pose, view))
            x = pad + j * (tw + pad)
            if os.path.exists(p):
                im = Image.open(p).convert("RGB")
                im.thumbnail((tw, th))
                sheet.paste(im, (x, y))
            else:
                d.rectangle([x, y, x + tw, y + th], outline=(90, 90, 90))
                d.text((x + 10, y + 10), "missing " + os.path.basename(p), fill=(200, 200, 200), font=font)
            d.text((x + 8, y + 6), "%s / %s" % (pose, view), fill=(255, 255, 0), font=font)
    sheet.save(out)
    print("sheet", out, sheet.size)


if __name__ == "__main__":
    main()

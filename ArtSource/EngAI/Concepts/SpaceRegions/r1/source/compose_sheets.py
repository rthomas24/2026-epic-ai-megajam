#!/usr/bin/env python3
"""Compose labelled mood / silhouette / palette boards for SpaceRegions r1.

Reads the untouched 4K Nano Banana Pro keyframes in source/ and writes the
three labelled sheets plus a stacked contact sheet. Labels, scale bars and
palette swatches are authored here (not generated) so measurements stay exact.
"""
from __future__ import annotations

from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent
# When run from the repo, ROOT is .../SpaceRegions/r1/source; sheets go up one.
# When run from /workspace/space_regions_r1, ROOT is that folder.
if (ROOT / "source").is_dir() and not (ROOT / "shepherd_mood_01.png").exists():
    SRC = ROOT / "source"
    OUT = ROOT
else:
    SRC = ROOT
    OUT = ROOT.parent

FONT_DIR = Path("/usr/share/fonts/truetype/sand-box/google")
BARLOW = FONT_DIR / "Barlow"
COND = FONT_DIR / "Barlow Condensed"

BG = (10, 11, 16, 255)
INK = (236, 238, 242, 255)
INK_DIM = (168, 174, 184, 255)
INK_MUTED = (118, 124, 134, 255)
RULE = (42, 46, 56, 255)
PANEL = (16, 18, 24, 255)

W, H = 5504, 3280
MARGIN = 96
HEADER_H = 300
FOOTER_H = 72
PALETTE_H = 248
GAP = 36
COL_GAP = 40


def font(path: Path, size: int) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(str(path), size=size)


def F(name: str, size: int) -> ImageFont.FreeTypeFont:
    mapping = {
        "title": COND / "BarlowCondensed-Bold.ttf",
        "cond": COND / "BarlowCondensed-Medium.ttf",
        "cond_reg": COND / "BarlowCondensed-Regular.ttf",
        "cond_light": COND / "BarlowCondensed-Light.ttf",
        "sans": BARLOW / "Barlow-Regular.ttf",
        "sans_med": BARLOW / "Barlow-Medium.ttf",
        "sans_sb": BARLOW / "Barlow-SemiBold.ttf",
        "sans_light": BARLOW / "Barlow-Light.ttf",
        "sans_bold": BARLOW / "Barlow-Bold.ttf",
    }
    return font(mapping[name], size)


def hex_rgb(h: str) -> tuple[int, int, int]:
    h = h.lstrip("#")
    return tuple(int(h[i : i + 2], 16) for i in (0, 2, 4))  # type: ignore[return-value]


def crop_letterbox(im: Image.Image, thresh: float = 8.0) -> Image.Image:
    a = np.asarray(im.convert("RGB"), dtype=np.float32)
    lum = a.mean(axis=(1, 2))
    keep = np.where(lum >= thresh)[0]
    if keep.size < 64:
        return im
    top, bot = int(keep[0]), int(keep[-1]) + 1
    # Ignore near-full-frame crops
    if top < 16 and (im.height - bot) < 16:
        return im
    if (bot - top) < im.height * 0.5:
        return im
    return im.crop((0, top, im.width, bot))


def cover(im: Image.Image, tw: int, th: int) -> Image.Image:
    im = im.convert("RGB")
    scale = max(tw / im.width, th / im.height)
    nw, nh = int(round(im.width * scale)), int(round(im.height * scale))
    im = im.resize((nw, nh), Image.Resampling.LANCZOS)
    left = max(0, (nw - tw) // 2)
    top = max(0, (nh - th) // 2)
    return im.crop((left, top, left + tw, top + th))


def text_size(draw: ImageDraw.ImageDraw, text: str, fnt: ImageFont.FreeTypeFont) -> tuple[int, int]:
    box = draw.textbbox((0, 0), text, font=fnt)
    return box[2] - box[0], box[3] - box[1]


def wrap(draw: ImageDraw.ImageDraw, text: str, fnt: ImageFont.FreeTypeFont, width: int) -> list[str]:
    words = text.split()
    lines: list[str] = []
    cur = ""
    for w in words:
        trial = w if not cur else f"{cur} {w}"
        if text_size(draw, trial, fnt)[0] <= width:
            cur = trial
        else:
            if cur:
                lines.append(cur)
            cur = w
    if cur:
        lines.append(cur)
    return lines


REGIONS = [
    {
        "id": "shepherd",
        "filename": "sheet_ShepherdBelt.png",
        "eyebrow_right": "REFERENCE ONLY  ·  NOT A SHIPPED ASSET",
        "title": "THE SHEPHERD BELT",
        "story": "The system's old belt: the reason this was a patrol station. Calm, ordered, Keplerian.",
        "silhouette_from_5km": "Silhouette from 5 km: a bright band across the sky from anywhere in the system.",
        "where": "A true ring around the Watchline, radius 2.0–2.6 km. One thin plane (±60 m), tilted 8°, cut by natural gaps where the lanes cross.",
        "accent": "#C49A5A",
        "mood": "shepherd-mood-nbpro_s421001_PICK.png",
        "sil": "shepherd-sil-nbpro_s421002_PICK.png",
        "scale_intro": "The player ship is the scale unit. Rift's approved mesh is 5.00 m long. A 180 cm figure is omitted: at a 300 m moonlet it is 0.6% of the diameter and would not read.",
        "facts": [
            "Ring radius 2.0–2.6 km  ·  band width 600 m",
            "Plane thickness ±60 m (120 m total)  ·  tilt 8°",
            "Inner-edge rocks grade out to gravel  ·  four shepherd moonlets (250–400 m) mark gap edges",
            "Tutorial withdrawal crosses the Verdance gap  ·  skirmishes on the edge",
        ],
        "bars": [
            ("Rift (player ship)", 5.0, "5.00 m"),
            ("Belt thickness", 120.0, "120 m"),
            ("Moonlet (min)", 250.0, "250 m"),
            ("Moonlet (max)", 400.0, "400 m"),
            ("Band width", 600.0, "600 m"),
        ],
        "bar_max": 600.0,
        "palette": [
            ("Deep nebula", "#2A0F4F"),
            ("Warm gold light", "#D4A05A"),
            ("Inner-edge ochre", "#C49A5A"),
            ("Mid rock grey", "#8B8680"),
            ("Outer gravel", "#D0CBC3"),
            ("Moonlet grey", "#9A948C"),
            ("Rift violet", "#6B2BD9"),
            ("Engine magenta", "#FF2BD6"),
        ],
    },
    {
        "id": "graveyard",
        "filename": "sheet_QuietGraveyard.png",
        "eyebrow_right": "REFERENCE ONLY  ·  NOT A SHIPPED ASSET",
        "title": "THE QUIET GRAVEYARD",
        "story": "The research expedition that went quiet: the place the signal device was taken from. Still, cold, no rocks.",
        "silhouette_from_5km": "Silhouette from 5 km: a line of dark hulls lit by one blinking beacon.",
        "where": "Beside the transponder off Verdance's orbit, 3.2–3.8 km out, clear of the tutorial plane at 2.5 km. None of the fights happen here: the calm before the descent.",
        "accent": "#19C3B1",
        "mood": "graveyard-mood-nbpro_s421003_PICK.png",
        "sil": "graveyard-sil-nbpro_s421004_PICK.png",
        "scale_intro": "The player ship is the scale unit. Rift's approved mesh is 5.00 m long. A 180 cm figure is omitted: it is about 1/50 of the smallest 80 m hull and would not read.",
        "facts": [
            "Sparse: five or six old civilian research hulls, 80–200 m, in a loose line",
            "Spilled debris and marker buoys  ·  no rocks, no asteroids",
            "Transponder beacon on a small dish platform, 15–25 m",
            "Verdance (teal jungle world) sits in the distance  ·  cool, still lighting",
        ],
        "bars": [
            ("Rift (player ship)", 5.0, "5.00 m"),
            ("Dish platform", 20.0, "15–25 m"),
            ("Hull (min)", 80.0, "80 m"),
            ("Hull (typical)", 120.0, "120 m"),
            ("Hull (max)", 200.0, "200 m"),
        ],
        "bar_max": 200.0,
        "palette": [
            ("Void teal-black", "#071318"),
            ("Deep blue-green", "#0F3B3A"),
            ("Verdance teal", "#19C3B1"),
            ("Atmosphere cyan", "#3CFFE0"),
            ("Dark hull", "#2A3034"),
            ("Weathered metal", "#6A7378"),
            ("Beacon / ice white", "#E8F4FF"),
            ("Engine magenta", "#FF2BD6"),
        ],
    },
    {
        "id": "surveyscar",
        "filename": "sheet_SurveyScar.png",
        "eyebrow_right": "REFERENCE ONLY  ·  NOT A SHIPPED ASSET",
        "title": "THE SURVEY SCAR",
        "story": "Null survey rigs have mined these rocks hollow and haul the yield to Cinder: the search follows the same pattern as Verdance.",
        "silhouette_from_5km": "Silhouette from 5 km: fractured metal and violet crystal rocks, rig lights.",
        "where": "The Cinder approach, 3–4.5 km out, with a rubble stream trailing toward the planet. Medium-high cluster with cut channels kept clear for transports. Gameplay 04's interception.",
        "accent": "#C3122E",
        "mood": "surveyscar-mood-nbpro_s421005_PICK.png",
        "sil": "surveyscar-sil-nbpro_s421006_PICK.png",
        "scale_intro": "The player ship is the scale unit. Rift's approved mesh is 5.00 m long. A 180 cm figure is omitted: it is 3–6% of a 30–60 m rig and would barely read at this distance.",
        "facts": [
            "Null survey rigs, 30–60 m, anchored to rocks: legs, drill, crimson seams, white 'eyes'",
            "Hollowed rocks, 40–120 m, with a cut face and a bore hole",
            "Fractured metal-laced stone and violet crystal  ·  traces of cyan crystal",
            "Rubble stream toward Cinder (basalt, ash, amber magma)  ·  clear transport channels",
        ],
        "bars": [
            ("Rift (player ship)", 5.0, "5.00 m"),
            ("Survey rig (min)", 30.0, "30 m"),
            ("Survey rig (max)", 60.0, "60 m"),
            ("Hollowed rock (min)", 40.0, "40 m"),
            ("Hollowed rock (max)", 120.0, "120 m"),
        ],
        "bar_max": 120.0,
        "palette": [
            ("Basalt", "#1A1718"),
            ("Ash grey", "#4A4446"),
            ("Violet crystal", "#7B3FE4"),
            ("Cyan crystal", "#2EE6FF"),
            ("Null gloss black", "#0E0E12"),
            ("Crimson seam", "#C3122E"),
            ("White eye", "#F4F7FF"),
            ("Amber magma", "#FF5A1F"),
        ],
    },
]



def fit_16x9(im: Image.Image, width: int) -> Image.Image:
    """Letterbox-crop, then cover-fit to 16:9 so aspect is never stretched."""
    im = crop_letterbox(im.convert("RGB"))
    height = int(round(width * 9 / 16))
    return cover(im, width, height)


def compose(region: dict) -> Image.Image:
    canvas = Image.new("RGBA", (W, H), BG)
    draw = ImageDraw.Draw(canvas)
    accent = hex_rgb(region["accent"])

    x0 = MARGIN
    y = MARGIN

    f_eye = F("cond", 36)
    draw.text((x0, y), "TRIAD DRIFT    ·    SPACE REGIONS  r1", font=f_eye, fill=INK_DIM)
    right = region["eyebrow_right"]
    rw, _ = text_size(draw, right, f_eye)
    draw.text((W - MARGIN - rw, y), right, font=f_eye, fill=accent + (255,))
    y += 52
    draw.rectangle((x0, y, W - MARGIN, y + 3), fill=accent + (255,))
    y += 16

    draw.text((x0, y - 8), region["title"], font=F("title", 124), fill=INK)
    y += 116

    f_story = F("sans_med", 34)
    for line in wrap(draw, region["story"], f_story, W - 2 * MARGIN):
        draw.text((x0, y), line, font=f_story, fill=INK)
        y += 44
    draw.text((x0, y + 2), region["silhouette_from_5km"], font=F("sans", 28), fill=INK_DIM)

    palette_top = H - MARGIN - FOOTER_H - PALETTE_H
    images_top = MARGIN + HEADER_H
    content_w = W - 2 * MARGIN
    mood_w = 3320
    right_w = content_w - mood_w - COL_GAP
    label_h = 44

    # Labels above the frames
    draw.text((x0, images_top), "KEY MOOD", font=F("cond", 32), fill=accent + (255,))
    rx = x0 + mood_w + COL_GAP
    draw.text((rx, images_top), "SILHOUETTE  &  SCALE", font=F("cond", 32), fill=accent + (255,))

    mood = fit_16x9(Image.open(SRC / region["mood"]), mood_w)
    sil = fit_16x9(Image.open(SRC / region["sil"]), right_w)
    img_y = images_top + label_h
    canvas.paste(mood, (x0, img_y))
    canvas.paste(sil, (rx, img_y))
    # accent underlines
    draw.rectangle((x0, img_y + mood.height, x0 + mood_w, img_y + mood.height + 3), fill=accent + (255,))
    draw.rectangle((rx, img_y + sil.height, rx + right_w, img_y + sil.height + 3), fill=accent + (255,))

    # LEFT under mood: region notes + where
    notes_top = img_y + mood.height + 28
    notes_bot = palette_top - 20
    draw.rectangle((x0, notes_top, x0 + mood_w, notes_bot), fill=PANEL)
    nx, ny = x0 + 32, notes_top + 22
    draw.text((nx, ny), "REGION NOTES", font=F("cond", 32), fill=INK)
    ny += 48
    f_note = F("sans", 28)
    for fact in region["facts"]:
        lines = wrap(draw, fact, f_note, mood_w - 80)
        draw.text((nx, ny), "·", font=f_note, fill=accent + (255,))
        for line in lines:
            draw.text((nx + 24, ny), line, font=f_note, fill=INK)
            ny += 36
        ny += 8
    ny += 8
    draw.rectangle((nx, ny, x0 + mood_w - 32, ny + 1), fill=RULE)
    ny += 16
    for line in wrap(draw, region["where"], F("sans_light", 26), mood_w - 64):
        if ny > notes_bot - 32:
            break
        draw.text((nx, ny), line, font=F("sans_light", 26), fill=INK_DIM)
        ny += 34

    # RIGHT under silhouette: scale intro + bars
    scale_top = img_y + sil.height + 28
    scale_bot = palette_top - 20
    draw.rectangle((rx, scale_top, rx + right_w, scale_bot), fill=PANEL)
    sx, sy = rx + 28, scale_top + 22
    draw.text((sx, sy), "MEASURED SCALE", font=F("cond", 32), fill=INK)
    sy += 46
    f_body = F("sans", 26)
    for line in wrap(draw, region["scale_intro"], f_body, right_w - 56):
        draw.text((sx, sy), line, font=f_body, fill=INK_DIM)
        sy += 34
    sy += 18

    # 180 cm callout as a faint tick note already in intro; bars:
    bar_label_w = 360
    bar_x = sx + bar_label_w
    bar_max_w = right_w - 56 - bar_label_w - 150
    f_label = F("sans", 24)
    f_val = F("sans_sb", 24)
    row_h = 52
    for name, metres, label in region["bars"]:
        draw.text((sx, sy + 4), name, font=f_label, fill=INK)
        bw = max(6, int(bar_max_w * (metres / region["bar_max"])))
        draw.rectangle((bar_x, sy + 14, bar_x + bar_max_w, sy + 30), fill=RULE)
        draw.rectangle((bar_x, sy + 14, bar_x + bw, sy + 30), fill=accent + (255,))
        draw.text((bar_x + bar_max_w + 16, sy + 4), label, font=f_val, fill=INK)
        sy += row_h

    # Palette
    draw.rectangle((x0, palette_top, W - MARGIN, palette_top + PALETTE_H), fill=PANEL)
    draw.rectangle((x0, palette_top, W - MARGIN, palette_top + 3), fill=accent + (255,))
    py = palette_top + 16
    draw.text((x0 + 24, py), "PALETTE    ·    sRGB", font=F("cond", 32), fill=INK)
    draw.text(
        (x0 + 420, py + 6),
        "Official Triad Drift swatches where a faction or world colour applies; belt / nebula / hull values chosen to match the key mood.",
        font=F("sans", 22),
        fill=INK_MUTED,
    )
    swatches = region["palette"]
    n = len(swatches)
    inner_w = content_w - 48
    slot = inner_w / n
    sw_h = 96
    sw_y = palette_top + 70
    sw_w = slot - 16
    for i, (name, hx) in enumerate(swatches):
        sx0 = x0 + 24 + int(i * slot)
        col = hex_rgb(hx)
        draw.rectangle((sx0, sw_y, sx0 + int(sw_w), sw_y + sw_h), fill=col + (255,))
        draw.rectangle((sx0, sw_y, sx0 + int(sw_w), sw_y + sw_h), outline=RULE, width=1)
        draw.text((sx0, sw_y + sw_h + 10), name, font=F("sans_med", 22), fill=INK)
        draw.text((sx0, sw_y + sw_h + 38), hx.upper(), font=F("sans", 20), fill=INK_DIM)

    fy = H - MARGIN - FOOTER_H + 22
    draw.rectangle((x0, fy - 14, W - MARGIN, fy - 13), fill=RULE)
    foot_l = "Source: docs/eng/SPACE-DESIGN.md  ·  Palettes: docs/visual/Art-Direction.md and the Blender art-direction reference  ·  R-042 ask 3"
    foot_r = "Keyframes: fal-ai/nano-banana-pro 4K  ·  Board composed in Python  ·  Grok Bot"
    draw.text((x0, fy), foot_l, font=F("sans", 22), fill=INK_MUTED)
    r_w, _ = text_size(draw, foot_r, F("sans", 22))
    draw.text((W - MARGIN - r_w, fy), foot_r, font=F("sans", 22), fill=INK_MUTED)

    return canvas.convert("RGB")


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    sheets: list[Image.Image] = []
    for region in REGIONS:
        print(f"composing {region['filename']} …")
        sheet = compose(region)
        dest = OUT / region["filename"]
        sheet.save(dest, "PNG", optimize=True)
        print(f"  wrote {dest}  {sheet.size}  {dest.stat().st_size} bytes")
        sheets.append(sheet)

    # Stacked contact sheet at half resolution, with 24 px gaps and a slim header
    gap = 24
    header = 80
    half = [s.resize((s.width // 2, s.height // 2), Image.Resampling.LANCZOS) for s in sheets]
    cw = half[0].width
    ch = header + gap + sum(im.height for im in half) + gap * (len(half) - 1) + gap
    contact = Image.new("RGB", (cw + 2 * gap, ch), (10, 11, 16))
    d = ImageDraw.Draw(contact)
    d.text((gap, 22), "TRIAD DRIFT    ·    SPACE REGIONS r1    ·    MOOD AND SILHOUETTE REFERENCE    ·    CONTACT SHEET", font=F("cond", 36), fill=INK)
    y = header + gap
    for im in half:
        contact.paste(im, (gap, y))
        y += im.height + gap
    cdest = OUT / "contact_sheet.png"
    contact.save(cdest, "PNG", optimize=True)
    print(f"wrote {cdest}  {contact.size}  {cdest.stat().st_size} bytes")

    # 1920-wide JPEG previews for review
    prev = OUT / "previews"
    prev.mkdir(exist_ok=True)
    for region, sheet in zip(REGIONS, sheets):
        w = 1920
        h = int(sheet.height * w / sheet.width)
        p = prev / (region["filename"].replace(".png", "_prev.jpg"))
        sheet.resize((w, h), Image.Resampling.LANCZOS).save(p, "JPEG", quality=90, optimize=True)
    w = 1600
    h = int(contact.height * w / contact.width)
    contact.resize((w, h), Image.Resampling.LANCZOS).save(prev / "contact_sheet_prev.jpg", "JPEG", quality=88, optimize=True)
    print("previews written")


if __name__ == "__main__":
    main()

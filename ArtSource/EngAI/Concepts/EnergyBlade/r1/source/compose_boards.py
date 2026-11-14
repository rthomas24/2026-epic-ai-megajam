#!/usr/bin/env python3
"""Compose three labelled concept boards for concepts r2 (Energy Blade, Null Mine, Tractor Beam).

Typography and chrome match ArtSource/EngAI/Concepts/SpaceRegions/r1.
16:9 keyframes are crop-letterboxed then cover-fitted (never stretched).
Measured scale panels (300 m mine, 35° tractor cone) are drawn in Python
and are the authority for numbers; generated frames are mood.
"""
from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path("/workspace/concepts_r2")
SRC = ROOT / "source"
OUT = ROOT / "boards"
OUT.mkdir(parents=True, exist_ok=True)

FONT_DIR = Path("/usr/share/fonts/truetype/sand-box/google")
FONT_REG = FONT_DIR / "Barlow/Barlow-Regular.ttf"
FONT_MED = FONT_DIR / "Barlow/Barlow-Medium.ttf"
FONT_SEM = FONT_DIR / "Barlow/Barlow-SemiBold.ttf"
FONT_BOLD = FONT_DIR / "Barlow/Barlow-Bold.ttf"
FONT_CBOLD = FONT_DIR / "Barlow_Condensed/BarlowCondensed-Bold.ttf"
FONT_CMED = FONT_DIR / "Barlow_Condensed/BarlowCondensed-Medium.ttf"
FONT_CLIGHT = FONT_DIR / "Barlow_Condensed/BarlowCondensed-Light.ttf"

BG = (10, 10, 14, 255)
PANEL = (16, 16, 20, 255)
RULE = (48, 48, 56, 255)
RULE_SOFT = (32, 32, 38, 255)
INK = (232, 232, 236, 255)
INK_DIM = (168, 168, 176, 255)
INK_MUTE = (112, 112, 122, 255)
ACCENT = (63, 233, 255, 255)  # Kato cyan, same as r1

MAGENTA = (255, 43, 214)
CYAN = (63, 233, 255)
AMBER = (255, 178, 30)
CRIMSON = (195, 18, 46)
RED_ARM = (255, 30, 60)
HOT_WHITE = (248, 250, 255)
CERAMIC = (232, 232, 236)
GUNMETAL = (72, 76, 84)
WRAP = (36, 36, 40)
NULL_BLACK = (14, 14, 18)

W = 5504


def font(path: Path, size: int) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(str(path), size)


def text_w(draw: ImageDraw.ImageDraw, text: str, fnt: ImageFont.FreeTypeFont) -> int:
    return int(draw.textlength(text, font=fnt))


def crop_letterbox(im: Image.Image, max_bar_frac: float = 0.18) -> Image.Image:
    """Trim uniform letterbox/pillarbox bars, then return the picture."""
    rgb = im.convert("RGB")
    w, h = rgb.size
    px = rgb.load()

    def is_bar(x0, y0, x1, y1) -> bool:
        # sample a strip; treat near-black as a bar
        n = 0
        acc = 0
        step = max(1, (x1 - x0) // 40)
        stepy = max(1, (y1 - y0) // 8)
        for y in range(y0, y1, stepy):
            for x in range(x0, x1, step):
                r, g, b = px[x, y]
                acc += r + g + b
                n += 1
        return (acc / max(n, 1)) < 24 * 3

    top = 0
    bot = h
    # scan from top
    band = max(4, h // 80)
    while top < int(h * max_bar_frac) and is_bar(0, top, w, min(h, top + band)):
        top += band
    while bot > int(h * (1 - max_bar_frac)) and is_bar(0, max(0, bot - band), w, bot):
        bot -= band
    left = 0
    right = w
    while left < int(w * max_bar_frac) and is_bar(left, top, min(w, left + band), bot):
        left += band
    while right > int(w * (1 - max_bar_frac)) and is_bar(max(0, right - band), top, right, bot):
        right -= band
    if bot - top < h * 0.5 or right - left < w * 0.5:
        return im
    return im.crop((left, top, right, bot))


def cover_fit(im: Image.Image, tw: int, th: int) -> Image.Image:
    """Scale to cover (tw, th) then center-crop. Never stretch."""
    im = im.convert("RGBA")
    w, h = im.size
    scale = max(tw / w, th / h)
    nw, nh = max(1, int(round(w * scale))), max(1, int(round(h * scale)))
    im = im.resize((nw, nh), Image.Resampling.LANCZOS)
    x0 = (nw - tw) // 2
    y0 = (nh - th) // 2
    return im.crop((x0, y0, x0 + tw, y0 + th))


def contain_fit(im: Image.Image, tw: int, th: int, bg=BG) -> Image.Image:
    """Scale to fit inside (tw, th), letterbox with bg. Never stretch."""
    im = im.convert("RGBA")
    w, h = im.size
    scale = min(tw / w, th / h)
    nw, nh = max(1, int(round(w * scale))), max(1, int(round(h * scale)))
    im = im.resize((nw, nh), Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", (tw, th), bg)
    canvas.paste(im, ((tw - nw) // 2, (th - nh) // 2), im)
    return canvas


def frame16x9(path: Path, tw: int) -> Image.Image:
    im = Image.open(path)
    im = crop_letterbox(im)
    th = int(round(tw * 9 / 16))
    return cover_fit(im, tw, th)


def paste(dst: Image.Image, src: Image.Image, x: int, y: int) -> None:
    dst.paste(src, (x, y), src if src.mode == "RGBA" else None)


def draw_header(canvas, draw, y, kicker, title, subtitle, meta_right):
    f_kicker = font(FONT_CMED, 36)
    f_title = font(FONT_CBOLD, 112)
    f_sub = font(FONT_MED, 36)
    f_meta = font(FONT_MED, 28)
    mx = 96
    draw.text((mx, y), kicker, font=f_kicker, fill=ACCENT)
    y += 48
    draw.text((mx, y), title, font=f_title, fill=INK)
    # right meta
    mw = text_w(draw, meta_right, f_meta)
    draw.text((W - 96 - mw, y + 28), meta_right, font=f_meta, fill=INK_DIM)
    y += 118
    # wrap subtitle
    max_w = W - 192 - 720  # leave the right meta clear; wrap to 2–3 lines
    words = subtitle.split()
    lines, cur = [], ""
    for word in words:
        trial = (cur + " " + word).strip()
        if text_w(draw, trial, f_sub) <= max_w:
            cur = trial
        else:
            if cur:
                lines.append(cur)
            cur = word
    if cur:
        lines.append(cur)
    for line in lines:
        draw.text((mx, y), line, font=f_sub, fill=INK_DIM)
        y += 48
    y += 16
    draw.rectangle((mx, y, W - 96, y + 2), fill=RULE)
    return y + 24


def label_above(draw, x, y, text):
    f = font(FONT_CMED, 32)
    draw.text((x, y), text, font=f, fill=ACCENT)
    return y + 40


def caption_below(draw, x, y, w, text):
    f = font(FONT_REG, 26)
    # wrap
    words = text.split()
    lines, cur = [], ""
    for word in words:
        trial = (cur + " " + word).strip()
        if text_w(draw, trial, f) <= w:
            cur = trial
        else:
            if cur:
                lines.append(cur)
            cur = word
    if cur:
        lines.append(cur)
    for line in lines:
        draw.text((x, y), line, font=f, fill=INK_MUTE)
        y += 34
    return y


def palette_strip(canvas, draw, x, y, w, swatches, title="PALETTE  ·  sRGB"):
    f_t = font(FONT_CMED, 26)
    f_n = font(FONT_MED, 22)
    f_h = font(FONT_REG, 20)
    draw.text((x, y), title, font=f_t, fill=INK_DIM)
    y += 36
    n = len(swatches)
    gap = 16
    sw = (w - gap * (n - 1)) // n
    sh = 56
    for i, (name, hexcol, rgb) in enumerate(swatches):
        sx = x + i * (sw + gap)
        draw.rounded_rectangle((sx, y, sx + sw, y + sh), radius=6, fill=rgb + (255,))
        # name + hex under
        draw.text((sx, y + sh + 8), name, font=f_n, fill=INK)
        draw.text((sx, y + sh + 34), hexcol, font=f_h, fill=INK_MUTE)
    return y + sh + 64


def footer(canvas, draw, y, text):
    f = font(FONT_REG, 24)
    draw.rectangle((96, y, W - 96, y + 2), fill=RULE)
    y += 16
    draw.text((96, y), text, font=f, fill=INK_MUTE)
    return y + 40


def scale_bar(draw, x, y, px_len, label, color=INK):
    f = font(FONT_MED, 22)
    draw.rectangle((x, y, x + px_len, y + 6), fill=color)
    draw.rectangle((x, y - 8, x + 3, y + 14), fill=color)
    draw.rectangle((x + px_len - 3, y - 8, x + px_len, y + 14), fill=color)
    draw.text((x, y + 16), label, font=f, fill=INK_DIM)


# ---------------------------------------------------------------------------
# Measured panels
# ---------------------------------------------------------------------------

def make_mine_readability_panel(w: int, h: int) -> Image.Image:
    """Angular-size test of a 2.8 m mine at 10 / 50 / 300 m, 90° HFOV."""
    panel = Image.new("RGBA", (w, h), (8, 8, 12, 255))
    draw = ImageDraw.Draw(panel)
    # faint starfield
    import random
    rng = random.Random(432003)
    for _ in range(280):
        sx = rng.randint(0, w - 1)
        sy = rng.randint(40, h - 1)
        b = rng.randint(80, 200)
        r = rng.choice((1, 1, 1, 2))
        draw.ellipse((sx, sy, sx + r, sy + r), fill=(b, b, b + 10, 255))

    # title
    f_t = font(FONT_CMED, 30)
    f_b = font(FONT_REG, 22)
    draw.text((24, 16), "300 m READABILITY TEST  ·  MEASURED  ·  90° HFOV", font=f_t, fill=ACCENT)
    draw.text(
        (24, 52),
        "Body Ø 2.8 m  (CollisionRadius 140 cm). Pixel size = (2·atan(r/D) / 90°) · frame width. "
        "The cinematic frame is mood; this panel is the authority.",
        font=f_b,
        fill=INK_MUTE,
    )

    # load mine hero, cut a roughly circular crop from center
    hero = Image.open(SRC / "mine_hero_01.png").convert("RGBA")
    # the mine is fairly centered; square-crop around the middle 70%
    hw, hh = hero.size
    side = int(min(hw, hh) * 0.72)
    cx, cy = hw // 2, hh // 2 + int(hh * 0.02)
    cut = hero.crop((cx - side // 2, cy - side // 2, cx + side // 2, cy + side // 2))

    # distances and layout
    # Use a local frame width of the cell, but quote sizes against a 16:9 1920-wide HUD
    # so numbers are readable. Also show px at 5504-wide 4K 16:9.
    # Angular: 2*atan(1.4/D) degrees
    # At 4K 16:9 W=5504, 90° HFOV: px = ang/90 * 5504
    distances = [10, 50, 300]
    labels_extra = [
        "close inspection",
        "mid dogfight",
        "art-direction readability range",
    ]
    # three columns
    col_w = (w - 48) // 3
    base_y = 110
    cell_h = h - base_y - 24
    fov = 90.0
    ref_w = 5504  # reported 4K frame

    def ang_deg(D):
        return math.degrees(2 * math.atan(1.4 / D))

    def px_at(D, frame_w):
        return ang_deg(D) / fov * frame_w

    for i, D in enumerate(distances):
        x0 = 24 + i * col_w
        # cell background
        draw.rounded_rectangle((x0, base_y, x0 + col_w - 16, base_y + cell_h), radius=10, fill=(14, 14, 20, 255))
        f_h = font(FONT_CBOLD, 36)
        f_s = font(FONT_REG, 22)
        draw.text((x0 + 16, base_y + 12), f"{D} m", font=f_h, fill=INK)
        ang = ang_deg(D)
        px4k = px_at(D, ref_w)
        draw.text(
            (x0 + 16, base_y + 54),
            f"{ang:.2f}°  ·  {px4k:.0f} px on 4K 16:9",
            font=f_s,
            fill=INK_DIM,
        )
        draw.text((x0 + 16, base_y + 82), labels_extra[i], font=f_s, fill=INK_MUTE)

        # size the mine relative to the cell so 10m is large, 300m is a spark
        # Fit 10 m mine to ~58% of cell width, then scale others by ratio of angular sizes
        px_10 = px_at(10, ref_w)
        px_this = px_at(D, ref_w)
        max_show = int((col_w - 48) * 0.62)
        show = max(3, int(round(max_show * (px_this / px_10))))
        # at 300 m, also overlay a glow so the arming light is the readable cue
        mine = cut.resize((show, show), Image.Resampling.LANCZOS)
        mx = x0 + (col_w - 16 - show) // 2
        my = base_y + 130 + (cell_h - 160 - show) // 2
        if D >= 50:
            # red bloom behind
            bloom_r = max(6, show // 2 + (18 if D == 300 else 10))
            bloom = Image.new("RGBA", (bloom_r * 2, bloom_r * 2), (0, 0, 0, 0))
            bd = ImageDraw.Draw(bloom)
            bd.ellipse((0, 0, bloom_r * 2 - 1, bloom_r * 2 - 1), fill=(255, 30, 60, 90 if D == 300 else 50))
            bloom = bloom.filter(ImageFilter.GaussianBlur(radius=bloom_r * 0.45))
            bx = mx + show // 2 - bloom_r
            by = my + show // 2 - bloom_r
            panel.alpha_composite(bloom, (bx, by))
        panel.alpha_composite(mine, (mx, my))
        if D == 300:
            # extra arming-light spark so the cue is visible at this scale
            spark_r = 5
            sx = mx + show // 2
            sy = my + show // 2
            draw.ellipse((sx - spark_r * 3, sy - spark_r * 3, sx + spark_r * 3, sy + spark_r * 3), fill=(255, 60, 80, 70))
            draw.ellipse((sx - spark_r, sy - spark_r, sx + spark_r, sy + spark_r), fill=(255, 240, 245, 255))
            note = font(FONT_MED, 20)
            draw.text((x0 + 16, base_y + cell_h - 36), "cue: red arming light, not body silhouette", font=note, fill=RED_ARM + (255,))

    return panel


def make_tractor_diagram(w: int, h: int) -> Image.Image:
    """Accurate 35° half-angle cone, near-field + full 3 km inset."""
    panel = Image.new("RGBA", (w, h), (8, 8, 12, 255))
    draw = ImageDraw.Draw(panel)
    f_t = font(FONT_CMED, 30)
    f_b = font(FONT_REG, 22)
    f_n = font(FONT_MED, 20)
    f_s = font(FONT_REG, 18)
    draw.text((24, 16), "CONE GEOMETRY  ·  MEASURED  ·  35° HALF-ANGLE", font=f_t, fill=ACCENT)
    draw.text(
        (24, 52),
        "Constants from TDDockingBay.h: ConeDepth 300 000 cm (3 km), ConeHalfAngle 35°, "
        "MouthDistance 30 000 cm, MouthRadius 6 000 cm, emitter 900 × 186 cm.",
        font=f_b,
        fill=INK_MUTE,
    )

    half = math.radians(35)
    tan_h = math.tan(half)  # ≈ 0.7002

    # ---- Near-field: 0–400 m so the 5 m ship and 9 m emitter are visible ----
    # Layout: emitter on the left, +X to the right (beam out of the bay).
    plot_x0, plot_y0 = 40, 110
    plot_w, plot_h = int(w * 0.62) - 40, h - 140
    # world: x 0–400 m, y ± (400*tan35) ≈ ±280 m. Use a little padding.
    world_x = 400.0
    world_y = 400.0 * tan_h * 1.15  # ±
    # y=0 is vertical center of plot
    def wx(xm):  # metres from emitter → px
        return plot_x0 + int(xm / world_x * plot_w)
    def wy(ym):  # metres off axis → px (up is +)
        return plot_y0 + plot_h // 2 - int(ym / world_y * (plot_h / 2))

    # plot frame
    draw.rounded_rectangle((plot_x0 - 8, plot_y0 - 8, plot_x0 + plot_w + 8, plot_y0 + plot_h + 8),
                           radius=8, fill=(14, 14, 20, 255))

    # cone fill
    pts = [
        (wx(0), wy(0)),
        (wx(world_x), wy(world_x * tan_h)),
        (wx(world_x), wy(-world_x * tan_h)),
    ]
    # denser polygon along the rays
    cone = []
    for t in range(0, 21):
        xm = world_x * t / 20
        cone.append((wx(xm), wy(xm * tan_h)))
    for t in range(20, -1, -1):
        xm = world_x * t / 20
        cone.append((wx(xm), wy(-xm * tan_h)))
    draw.polygon(cone, fill=(63, 233, 255, 36), outline=None)
    # ray lines
    draw.line([(wx(0), wy(0)), (wx(world_x), wy(world_x * tan_h))], fill=CYAN + (200,), width=3)
    draw.line([(wx(0), wy(0)), (wx(world_x), wy(-world_x * tan_h))], fill=CYAN + (200,), width=3)
    # axis
    draw.line([(wx(0), wy(0)), (wx(world_x), wy(0))], fill=(80, 90, 100, 180), width=1)

    # angle ticks at emitter
    # a small arc at 40 m along rays
    arc_r_m = 48
    bbox = [wx(0) - int(arc_r_m / world_x * plot_w), wy(0) - int(arc_r_m / world_y * (plot_h / 2)),
            wx(0) + int(arc_r_m / world_x * plot_w), wy(0) + int(arc_r_m / world_y * (plot_h / 2))]
    # start/end in PIL: 0 is 3 o'clock, clockwise. Our +x is right, +y is up.
    # upper ray is at -35° from +x, lower at +35° (image y flipped already in wy)
    draw.arc(bbox, start=-35, end=35, fill=CYAN + (255,), width=3)
    draw.text((wx(arc_r_m * 0.7) + 8, wy(0) - 12), "35°", font=font(FONT_CBOLD, 28), fill=CYAN + (255,))
    draw.text((wx(arc_r_m * 0.55), wy(arc_r_m * tan_h) - 28), "half-angle", font=f_s, fill=INK_DIM)

    # MouthDistance 300 m plane + MouthRadius 60 m
    mx = 300.0
    if mx <= world_x:
        r_cone = mx * tan_h  # 210.06
        r_lock = 60.0
        x = wx(mx)
        draw.line([(x, wy(r_cone)), (x, wy(-r_cone))], fill=(200, 200, 210, 120), width=2)
        # lock radius ticks
        draw.line([(x - 6, wy(r_lock)), (x + 6, wy(r_lock))], fill=AMBER + (255,), width=3)
        draw.line([(x - 6, wy(-r_lock)), (x + 6, wy(-r_lock))], fill=AMBER + (255,), width=3)
        draw.text((x + 10, wy(r_cone) - 2), f"300 m  ·  cone r {r_cone:.0f} m", font=f_n, fill=INK)
        draw.text((x + 10, wy(r_lock) - 22), f"MouthRadius 60 m (lock)", font=f_n, fill=AMBER + (255,))

    # emitter 9 m disk (900 cm dia) — show as a short vertical bar at x=0
    em_r = 4.5
    # exaggerate slightly so it's visible, but label true size
    draw.rectangle((wx(0) - 5, wy(em_r * 4), wx(0) + 5, wy(-em_r * 4)), fill=HOT_WHITE + (255,))
    # cyan glow
    draw.ellipse((wx(0) - 14, wy(0) - 14, wx(0) + 14, wy(0) + 14), fill=(180, 250, 255, 200))
    draw.text((wx(0) + 12, wy(0) + 18), "emitter  9.00 m dia  ×  1.86 m deep", font=f_n, fill=INK)

    # Rift 5.00 m at 180 m (inside the mouth, being pulled)
    ship_d = 180.0
    ship_len = 5.00
    # draw a small chevron / plan silhouette, length 5 m, beam ~2.3 m (from extents)
    sl = 5.00
    swd = 2.26  # half-ish of 4.17? width 4.17, height 2.26 — use 4.17 as span
    span = 4.17
    # simple crescent-ish triangle pointing toward emitter (nose +X? wait cone is +X from bay
    # so ship is flying toward the bay, nose toward emitter = toward -X? Behind-the-ship
    # camera looks at the bay, so nose is toward emitter.
    sx = wx(ship_d)
    # triangle nose at ship_d - sl/2 (toward emitter), tail at ship_d+sl/2
    nose = (wx(ship_d - sl / 2), wy(0))
    tail_t = (wx(ship_d + sl / 2), wy(span / 2))
    tail_b = (wx(ship_d + sl / 2), wy(-span / 2))
    draw.polygon([nose, tail_t, tail_b], fill=(140, 90, 200, 255), outline=(220, 200, 255, 255))
    draw.text((wx(ship_d + sl / 2) + 8, wy(0) - 10), "Rift  5.00 m  at 180 m", font=f_n, fill=INK)

    # scale bar 50 m
    bar_m = 50.0
    bx0 = wx(20)
    by0 = plot_y0 + plot_h - 28
    draw.rectangle((wx(20), by0, wx(20 + bar_m), by0 + 5), fill=INK)
    draw.rectangle((wx(20), by0 - 6, wx(20) + 2, by0 + 11), fill=INK)
    draw.rectangle((wx(20 + bar_m) - 2, by0 - 6, wx(20 + bar_m), by0 + 11), fill=INK)
    draw.text((wx(20), by0 - 26), "50 m", font=f_n, fill=INK_DIM)
    draw.text((plot_x0, plot_y0 + 4), "NEAR FIELD  ·  0–400 m   (emitter and ship to scale)", font=f_n, fill=INK_DIM)

    # ---- Full 3 km inset on the right ----
    ix0 = int(w * 0.64)
    iy0 = 110
    iw = w - ix0 - 32
    ih = h - 150
    draw.rounded_rectangle((ix0, iy0, ix0 + iw, iy0 + ih), radius=8, fill=(14, 14, 20, 255))
    draw.text((ix0 + 16, iy0 + 10), "FULL CONE  ·  3 km depth", font=f_n, fill=INK_DIM)

    world_x2 = 3000.0
    world_y2 = 3000.0 * tan_h * 1.12
    def wx2(xm):
        return ix0 + 16 + int(xm / world_x2 * (iw - 32))
    def wy2(ym):
        return iy0 + 48 + (ih - 80) // 2 - int(ym / world_y2 * ((ih - 80) / 2))

    cone2 = []
    for t in range(0, 21):
        xm = world_x2 * t / 20
        cone2.append((wx2(xm), wy2(xm * tan_h)))
    for t in range(20, -1, -1):
        xm = world_x2 * t / 20
        cone2.append((wx2(xm), wy2(-xm * tan_h)))
    draw.polygon(cone2, fill=(63, 233, 255, 40))
    draw.line([(wx2(0), wy2(0)), (wx2(world_x2), wy2(world_x2 * tan_h))], fill=CYAN + (200,), width=2)
    draw.line([(wx2(0), wy2(0)), (wx2(world_x2), wy2(-world_x2 * tan_h))], fill=CYAN + (200,), width=2)
    # 3 km plane
    r3 = 3000 * tan_h  # 2100.6
    draw.line([(wx2(3000), wy2(r3)), (wx2(3000), wy2(-r3))], fill=INK, width=2)
    draw.text((wx2(3000) - 150, wy2(r3) - 22), f"3 km  ·  r {r3:.0f} m", font=f_s, fill=INK)
    # 300 m tick (will be very close to emitter)
    draw.line([(wx2(300), wy2(300 * tan_h)), (wx2(300), wy2(-300 * tan_h))], fill=AMBER + (200,), width=2)
    # emitter
    draw.ellipse((wx2(0) - 4, wy2(0) - 4, wx2(0) + 4, wy2(0) + 4), fill=HOT_WHITE + (255,))
    # note: ship and emitter are sub-pixel at this scale
    draw.text((ix0 + 16, iy0 + ih - 56), "Rift 5 m and emitter 9 m are", font=f_s, fill=INK_MUTE)
    draw.text((ix0 + 16, iy0 + ih - 36), "sub-pixel at this scale — see near field.", font=f_s, fill=INK_MUTE)
    # formula
    draw.text((ix0 + 16, iy0 + 36), "r(D) = D · tan(35°) ≈ D · 0.700", font=f_s, fill=CYAN + (255,))

    return panel


# ---------------------------------------------------------------------------
# Boards
# ---------------------------------------------------------------------------

def compose_blade():
    # 5 frames + palette + scale. 5504 x ~5000
    hero = frame16x9(SRC / "blade_hero_01.png", 3320)
    colors = frame16x9(SRC / "blade_colors_01.png", 1904)
    ignite = frame16x9(SRC / "blade_ignite_01.png", 3320)
    xsec = frame16x9(SRC / "blade_xsec_01.png", 1904)
    swing = frame16x9(SRC / "blade_swing_01.png", 3320)

    H = 8000
    canvas = Image.new("RGBA", (W, H), BG)
    draw = ImageDraw.Draw(canvas)
    y = 72
    y = draw_header(
        canvas, draw, y,
        "TRIAD DRIFT  ·  CONCEPT  ·  REFERENCE ONLY  ·  ISSUE #163",
        "ENERGY BLADE",
        "Handle is approved Direction B (mesh r1) and is not redesigned. This sheet is the energy-beam remake: a hot near-white core, coloured glow, soft bloom and flicker, riding a thin single-edged ribbon on the existing rail — not a cylindrical lightsaber tube and not a baked volume. Colours are per-pilot; the hilt stays the same.",
        "r1   ·   180 cm figure   ·   39.7 cm tip-to-pommel",
    )

    # ROW 1
    y = label_above(draw, 96, y, "KEY  ·  180 cm PILOT  ·  WREN MAGENTA")
    paste(canvas, hero, 96, y)
    # colors stacked to the right, top-aligned with hero
    ry = y
    label_above(draw, 96 + 3320 + 32, ry - 40, "CORE AND GLOW  ·  PER PILOT")
    paste(canvas, colors, 96 + 3320 + 32, ry)
    # notes under colors (hero is taller)
    nx, ny = 96 + 3320 + 32, ry + colors.size[1] + 16
    f_n = font(FONT_MED, 26)
    f_b = font(FONT_REG, 24)
    notes = [
        "Same Direction B hilt on every variant.",
        "Only the beam and the status pips change colour.",
        "Wren / Rift   magenta  #FF2BD6",
        "Kato / Lancer  cyan    #3FE9FF",
        "Moss / Bastion amber   #FFB21E",
        "Core is near-white #F8FAFF; glow is the pilot colour.",
        "Energy is a thin wedge on the rail, not a tube.",
    ]
    for line in notes:
        draw.text((nx, ny), line, font=f_b, fill=INK_DIM)
        ny += 34
    y = y + hero.size[1] + 16
    y = caption_below(
        draw, 96, y, 3320,
        "Hangar key is for scale and bloom in context. The beam may read longer than the rail here; "
        "the colour and ignite product shots are the authority for length: energy sits on the 39.7 cm Direction B rail, a short one-handed tool.",
    )
    y += 20

    # ROW 2
    y = label_above(draw, 96, y, "IGNITE  ·  FOLDED  →  SPARK  →  CRAWL  →  FULL")
    paste(canvas, ignite, 96, y)
    label_above(draw, 96 + 3320 + 32, y - 40, "BEAM CROSS-SECTION  ·  WEDGE, NOT TUBE")
    paste(canvas, xsec, 96 + 3320 + 32, y)
    y = y + ignite.size[1] + 16
    y = caption_below(
        draw, 96, y, 3320,
        "Ignite is a crawl along the rail, not a lengthening cylinder. Folded (off) → white spark at the hinge → energy walks the edge → full ribbon with bloom. Retract is the sequence in reverse.",
    )
    # xsec caption on the right, same baseline-ish
    caption_below(
        draw, 96 + 3320 + 32, y - 34, 1904,
        "Hot core, coloured sheath, soft outer bloom. Crescent / single-edged wedge. Illustrated callouts; hardware from the product shots.",
    )
    y += 20

    # ROW 3  swing + measured scale card
    y = label_above(draw, 96, y, "SWING TRAIL  ·  RIBBON, SPARKS, HIT")
    paste(canvas, swing, 96, y)
    # scale card to the right of swing
    cx, cy = 96 + 3320 + 32, y
    cw, ch = 1904, swing.size[1]
    draw.rounded_rectangle((cx, cy, cx + cw, cy + ch), radius=8, fill=PANEL)
    f_h = font(FONT_CMED, 30)
    f_b = font(FONT_REG, 26)
    f_t = font(FONT_MED, 28)
    tx, ty = cx + 28, cy + 24
    draw.text((tx, ty), "SCALE  ·  DIRECTION B RAIL", font=f_h, fill=ACCENT)
    ty += 48
    metrics = [
        ("180 cm figure", "required on this human-scale sheet"),
        ("Tip to pommel", "39.7 cm  (deployed)"),
        ("Pommel to guard", "24.6 cm"),
        ("Barrel diameter", "3.70 cm"),
        ("Extents, deployed", "24.9 × 4.2 × 35.9 cm"),
        ("Extents, folded", "12.0 × 4.5 × 23.7 cm"),
        ("Energy", "VFX edge on the rail, not a baked volume"),
        ("Handle", "locked — white ceramic, dark wrap, knuckle spine"),
    ]
    for k, v in metrics:
        draw.text((tx, ty), k, font=f_t, fill=INK)
        tw = text_w(draw, k, f_t)
        draw.text((tx + 320, ty), v, font=f_b, fill=INK_DIM)
        ty += 40
    ty += 8
    # 39.7 cm bar next to a 180 cm bar, same px/cm
    # card width 1904-56 = 1848 usable. 180 cm should fit: 8 px/cm → 1440 px. Use 7 px/cm.
    ppm = 7.0
    scale_bar(draw, tx, ty, int(180 * ppm), "180 cm figure", INK)
    ty += 52
    scale_bar(draw, tx, ty, int(39.7 * ppm), "39.7 cm tip-to-pommel  (same scale)", MAGENTA + (255,))
    ty += 58
    for line in [
        "Swing panel is effect-led (ribbon trail, sparks shed",
        "on the arc, hit spark on contact). Hardware in that",
        "frame is looser than the product shots; use colours",
        "and ignite as the hilt authority.",
        "Original design only — no lightsaber likeness.",
    ]:
        draw.text((tx, ty), line, font=f_b, fill=INK_MUTE)
        ty += 32

    y = y + swing.size[1] + 20

    swatches = [
        ("Wren magenta", "#FF2BD6", MAGENTA),
        ("Kato cyan", "#3FE9FF", CYAN),
        ("Moss amber", "#FFB21E", AMBER),
        ("Hot core", "#F8FAFF", HOT_WHITE),
        ("Ceramic", "#E8E8EC", CERAMIC),
        ("Gunmetal", "#484C54", GUNMETAL),
        ("Wrap", "#242428", WRAP),
    ]
    y = palette_strip(canvas, draw, 96, y, W - 192, swatches)
    footer(
        canvas, draw, y,
        "ArtSource/EngAI/Concepts/EnergyBlade/r1   ·   fal-ai/nano-banana-pro 4K  ·   $0.30 / image (model page; 4K is 2× standard)   ·   5 images  ·   $1.50   ·   reference only, never shipped   ·   see SOURCE.md",
    )

    out = canvas.crop((0, 0, W, min(H, y + 96)))
    path = OUT / "sheet_EnergyBlade.png"
    out.convert("RGB").save(path, "PNG", optimize=True)
    print("wrote", path, out.size, path.stat().st_size)
    return path


def compose_mine():
    hero = frame16x9(SRC / "mine_hero_01.png", 3320)
    trail = frame16x9(SRC / "mine_trail_01.png", 3320)
    cine = frame16x9(SRC / "mine_300m_01.png", 3320)
    # measured panel full width
    meas_w = W - 192
    meas_h = 1180
    measured = make_mine_readability_panel(meas_w, meas_h)

    # layout estimate
    H = 8000
    canvas = Image.new("RGBA", (W, H), BG)
    draw = ImageDraw.Draw(canvas)
    y = 72
    y = draw_header(
        canvas, draw, y,
        "TRIAD DRIFT  ·  CONCEPT  ·  REFERENCE ONLY  ·  NULL BOMBER ORDNANCE",
        "NULL PLASMA MINE",
        "Dark spiked core and a red arming light, readable at 300 m. Body visual diameter ~2.8 m from CollisionRadius 140 cm. The bomber drops a trail of four every 5 s (FuseArmDelay 0.75 s, SplashRadius 800 cm, ProximityFuseRadius 500 cm). 180 cm figure omitted: a person is 1.8 / 2.8 of the body and unreadable at the 300 m test.",
        "r1   ·   body Ø 2.8 m   ·   bomber 11.80 m",
    )

    y = label_above(draw, 96, y, "KEY  ·  SPIKED CORE  ·  RED ARMING LIGHT")
    paste(canvas, hero, 96, y)
    # specs card
    cx, cy = 96 + 3320 + 32, y
    cw, ch = 1904, hero.size[1]
    draw.rounded_rectangle((cx, cy, cx + cw, cy + ch), radius=8, fill=PANEL)
    f_h = font(FONT_CMED, 30)
    f_t = font(FONT_MED, 28)
    f_b = font(FONT_REG, 26)
    tx, ty = cx + 28, cy + 24
    draw.text((tx, ty), "SPEC  ·  TDWeaponLibrary.cpp", font=f_h, fill=ACCENT)
    ty += 52
    specs = [
        ("Visual diameter", "~2.8 m  (CollisionRadius 140 cm)"),
        ("PlaceholderScale", "1.6, 1.6, 1.6"),
        ("Damage", "60"),
        ("SplashRadius", "800 cm  (8.0 m)"),
        ("ProximityFuseRadius", "500 cm  (5.0 m)"),
        ("FuseArmDelay", "0.75 s"),
        ("Speed / Lifetime", "300  /  20 s"),
        ("Drop pattern", "trail of 4, every 5 s"),
        ("Bomber (Maul)", "11.80 m  (1180 × 1035 × 216 cm)"),
        ("Rift (scale)", "5.00 m  — art-direction “~7 m” is stale"),
        ("Readability", "red arming light at 300 m"),
        ("Palette", "gloss black #0E0E12  ·  crimson #C3122E"),
        ("", "arming #FF1E3C  ·  white-hot core #F4F7FF"),
    ]
    for k, v in specs:
        if k:
            draw.text((tx, ty), k, font=f_t, fill=INK)
            draw.text((tx + 420, ty), v, font=f_b, fill=INK_DIM)
        else:
            draw.text((tx + 420, ty), v, font=f_b, fill=INK_DIM)
        ty += 40
    ty += 16
    # scale bars: 2.8 m and 11.8 m at 18 px/m → 50 and 212 px (small). Use 40 px/m for 2.8m = 112, 11.8=472, 5.00=200
    ppm = 40
    scale_bar(draw, tx, ty, int(2.8 * ppm), "2.8 m  mine body", RED_ARM + (255,))
    ty += 52
    scale_bar(draw, tx, ty, int(5.00 * ppm), "5.00 m  Rift", (140, 90, 200, 255))
    ty += 52
    scale_bar(draw, tx, ty, int(11.80 * ppm), "11.80 m  Null Maul bomber", INK)

    y = y + hero.size[1] + 16
    y = caption_below(
        draw, 96, y, 3320,
        "Gloss-black faceted core, sharp Null spikes, crimson seam glow, one dominant white-hot arming eye with a soft red bloom — the long-range cue. Not a planet-scale object; a 2.8 m munition.",
    )
    y += 20

    y = label_above(draw, 96, y, "SILHOUETTE / SCALE  ·  BOMBER 11.80 m  ·  RIFT 5.00 m  ·  TRAIL OF FOUR")
    paste(canvas, trail, 96, y)
    # second cine stacked? We'll put cine under trail, full 3320, and leave right for notes? 
    # Actually place cine to the right? cine is also 16:9 — same width as trail if I used 3320.
    # Use a smaller cine on the right? Trail at 3320 and cine would be 1904.
    cine_r = frame16x9(SRC / "mine_300m_01.png", 1904)
    # wait I already made cine at 3320. I'll paste trail only in this row and put a note
    # that the 300m cinematic sits in the next row together with the measured panel.
    # To use the right column of the trail row, paste the smaller cine there and add a short note under it.
    label_above(draw, 96 + 3320 + 32, y - 40, "CINEMATIC  ·  300 m  ·  MOOD")
    paste(canvas, cine_r, 96 + 3320 + 32, y)
    y = y + trail.size[1] + 16
    y = caption_below(
        draw, 96, y, 3320,
        "Bomber laying a trail of four; Rift at left for scale. Apparent sizes in the painting are mood — use the measured bars and the 300 m test below. The cinematic crop is mood as well; lights remain the readable cue at range.",
    )
    y += 12

    y = label_above(draw, 96, y, "300 m READABILITY TEST  ·  MEASURED ANGULAR SIZE  ·  AUTHORITY")
    paste(canvas, measured, 96, y)
    y = y + meas_h + 20

    swatches = [
        ("Gloss black", "#0E0E12", NULL_BLACK),
        ("Crimson seam", "#C3122E", CRIMSON),
        ("Arming light", "#FF1E3C", RED_ARM),
        ("White eye", "#F4F7FF", (244, 247, 255)),
        ("Rift magenta", "#FF2BD6", MAGENTA),
    ]
    y = palette_strip(canvas, draw, 96, y, W - 192, swatches)
    footer(
        canvas, draw, y,
        "ArtSource/EngAI/Concepts/NullMine/r1   ·   fal-ai/nano-banana-pro 4K  ·   $0.30 / image (model page; 4K is 2× standard)   ·   3 images  ·   $0.90   ·   reference only, never shipped   ·   see SOURCE.md",
    )
    out = canvas.crop((0, 0, W, min(H, y + 96)))
    path = OUT / "sheet_NullMine.png"
    out.convert("RGB").save(path, "PNG", optimize=True)
    print("wrote", path, out.size, path.stat().st_size)
    return path


def compose_tractor():
    behind = frame16x9(SRC / "tractor_behind_01.png", 3320)
    wide = frame16x9(SRC / "tractor_wide_01.png", 1904)
    side = frame16x9(SRC / "tractor_side_01.png", 3320)
    meas_w = W - 192
    meas_h = 1280
    measured = make_tractor_diagram(meas_w, meas_h)

    H = 8000
    canvas = Image.new("RGBA", (W, H), BG)
    draw = ImageDraw.Draw(canvas)
    y = 72
    y = draw_header(
        canvas, draw, y,
        "TRIAD DRIFT  ·  CONCEPT  ·  REFERENCE ONLY  ·  DOMINION DOCKING  ·  ENG-29 / ENG-37",
        "TRACTOR-BEAM CONE",
        "Seen from behind the ship, looking toward the bay. A wide volumetric cyan/white cone, not a dotted laser. Apex at the 9.00 m emitter in the bay mouth; the cone narrows ahead of the camera toward that mouth. Half-angle 35°, depth 3 km. Generated frames are mood; the measured diagram is the authority for the angle and radii.",
        "r1   ·   35° half-angle   ·   Rift 5.00 m",
    )

    y = label_above(draw, 96, y, "KEY  ·  FROM BEHIND THE SHIP  ·  LOOKING TOWARD THE BAY")
    paste(canvas, behind, 96, y)
    label_above(draw, 96 + 3320 + 32, y - 40, "ESTABLISHING  ·  CONE VOLUME")
    paste(canvas, wide, 96 + 3320 + 32, y)
    # notes under the wide frame
    nx, ny = 96 + 3320 + 32, y + wide.size[1] + 16
    f_b = font(FONT_REG, 24)
    for line in [
        "Cue attaches at the bay mouth, +X out.",
        "User.Length = bay-to-ship distance (cm).",
        "User.Intensity = 0..1.",
        "GlowColor is cyan.",
        "The wide frame is mood — the ship may sit",
        "outside the volume here. Gameplay has the",
        "hull inside the cone, being drawn in.",
        "Emitter SM_DominionBay_TractorEmitter",
        "at hull (31200, 0, 1400).",
    ]:
        draw.text((nx, ny), line, font=f_b, fill=INK_DIM)
        ny += 32

    y = y + behind.size[1] + 16
    y = caption_below(
        draw, 96, y, 3320,
        "Camera sits aft of the Rift, engines toward us, nose toward the bay. The volumetric cone wraps the hull and tightens onto the 9 m emitter. Soft cyan falloff, brighter core on axis, no hard laser ticks.",
    )
    y += 20

    y = label_above(draw, 96, y, "SIDE SILHOUETTE  ·  SCALE MOOD")
    paste(canvas, side, 96, y)
    # specs card
    cx, cy = 96 + 3320 + 32, y
    cw, ch = 1904, side.size[1]
    draw.rounded_rectangle((cx, cy, cx + cw, cy + ch), radius=8, fill=PANEL)
    f_h = font(FONT_CMED, 30)
    f_t = font(FONT_MED, 28)
    f_bb = font(FONT_REG, 26)
    tx, ty = cx + 28, cy + 24
    draw.text((tx, ty), "SPEC  ·  TDDockingBay.h", font=f_h, fill=ACCENT)
    ty += 52
    specs = [
        ("ConeHalfAngle", "35°"),
        ("ConeDepth", "300 000 cm   (3.00 km)"),
        ("MouthDistance", "30 000 cm   (300 m)"),
        ("MouthRadius", "6 000 cm   (60 m)  lock"),
        ("Radius at 300 m", "300 · tan 35°  =  210 m  (cone)"),
        ("Radius at 3 km", "3 000 · tan 35°  =  2 101 m"),
        ("Emitter diameter", "9.00 m  (900 cm)"),
        ("Emitter depth", "1.86 m  (186 cm)"),
        ("Rift length", "5.00 m  (approved mesh)"),
        ("Lancer / Bastion", "~8 m  /  ~6 × 9 m"),
        ("Formula", "r(D) = D · tan(35°)  ≈  D · 0.700"),
    ]
    for k, v in specs:
        draw.text((tx, ty), k, font=f_t, fill=INK)
        draw.text((tx + 400, ty), v, font=f_bb, fill=INK_DIM)
        ty += 40
    ty += 8
    ppm = 8  # px per metre — 9 m emitter = 72 px, 60 m = 480, 210 m = 1680 (fits 1848)
    scale_bar(draw, tx, ty, int(9.00 * ppm), "9.00 m  emitter  (same scale as bars below)", HOT_WHITE + (255,))
    ty += 52
    scale_bar(draw, tx, ty, int(60 * ppm), "60 m  MouthRadius (lock)", AMBER + (255,))
    ty += 52
    scale_bar(draw, tx, ty, min(int(210 * ppm), 1848 - 40), "210 m  cone radius at 300 m", CYAN + (255,))

    y = y + side.size[1] + 16
    y = caption_below(
        draw, 96, y, 3320,
        "Side view is mood for the volumetric shape. Half-angle in the painting is approximate — use the measured diagram below. 180 cm figure omitted: a person is invisible against a 9 m emitter and a kilometre-scale cone.",
    )
    y += 12

    y = label_above(draw, 96, y, "CONE GEOMETRY  ·  MEASURED  ·  AUTHORITY")
    paste(canvas, measured, 96, y)
    y = y + meas_h + 20

    swatches = [
        ("Cone cyan", "#3FE9FF", CYAN),
        ("Hot core", "#F8FAFF", HOT_WHITE),
        ("Deep cyan", "#0A3A44", (10, 58, 68)),
        ("Rift violet", "#6B2BD9", (107, 43, 217)),
        ("Dominion hull", "#14141A", (20, 20, 26)),
        ("Lock amber (diag.)", "#FFB21E", AMBER),
    ]
    y = palette_strip(canvas, draw, 96, y, W - 192, swatches)
    footer(
        canvas, draw, y,
        "ArtSource/EngAI/Concepts/TractorBeam/r1   ·   fal-ai/nano-banana-pro 4K  ·   $0.30 / image (model page; 4K is 2× standard)   ·   3 images  ·   $0.90   ·   reference only, never shipped   ·   see SOURCE.md",
    )
    out = canvas.crop((0, 0, W, min(H, y + 96)))
    path = OUT / "sheet_TractorBeam.png"
    out.convert("RGB").save(path, "PNG", optimize=True)
    print("wrote", path, out.size, path.stat().st_size)
    return path


if __name__ == "__main__":
    compose_blade()
    compose_mine()
    compose_tractor()
    print("done")

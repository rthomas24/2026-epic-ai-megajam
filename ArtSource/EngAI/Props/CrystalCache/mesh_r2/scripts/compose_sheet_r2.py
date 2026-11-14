#!/usr/bin/env python3
"""Compose the #53 crystal cluster r2 contact sheet (3840 x 2400) from the render_r2.py panels."""
import json
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

WORK = Path("/tmp/cc2"); RENDERS = WORK / "renders"
OUT = Path("/tmp/wt-crystal-cluster-r2/ArtSource/EngAI/Props/CrystalCache/mesh_r2")
REF = OUT / "refs/Crystal_Cache_Modeling_Reference.png"
W, H = 3840, 2400
BG, BAR, ACCENT, FG, MUTED, CYAN = (18, 20, 24), (12, 14, 18), (255, 140, 40), (236, 238, 242), (160, 166, 176), (80, 210, 230)

def font(size, bold=False):
    p = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
    return ImageFont.truetype(p, size) if Path(p).exists() else ImageFont.load_default()

def fit(im, box, fill=(28, 30, 34)):
    bw, bh = box
    c = Image.new("RGB", (bw, bh), fill)
    im = im.convert("RGB")
    s = min(bw / im.width, bh / im.height)
    nw, nh = max(1, int(im.width * s)), max(1, int(im.height * s))
    c.paste(im.resize((nw, nh), Image.Resampling.LANCZOS), ((bw - nw) // 2, (bh - nh) // 2))
    return c

def panel(dst, draw, img, x, y, w, h, title, sub=""):
    draw.rectangle([x, y, x + w, y + h], fill=(28, 30, 34), outline=(48, 52, 60), width=2)
    cap = 42
    dst.paste(fit(img, (w - 4, h - 4 - cap)), (x + 2, y + 2))
    draw.rectangle([x, y + h - cap, x + w, y + h], fill=BAR)
    draw.rectangle([x, y + h - cap, x + 6, y + h], fill=ACCENT)
    draw.text((x + 16, y + h - cap + 8), title, font=font(20, True), fill=FG)
    if sub:
        tw = draw.textlength(title, font=font(20, True))
        draw.text((x + 24 + tw, y + h - cap + 11), sub, font=font(16), fill=MUTED)

def triptych(imgs, labels, h=900):
    tiles = []
    for im, lab in zip(imgs, labels):
        t = fit(im, (h, h))
        d = ImageDraw.Draw(t)
        d.rectangle([0, 0, h, 46], fill=BAR)
        d.text((14, 10), lab, font=font(24, True), fill=CYAN if "r2" in lab else FG)
        tiles.append(t)
    out = Image.new("RGB", (h * len(tiles) + 8 * (len(tiles) - 1), h), BG)
    for i, t in enumerate(tiles):
        out.paste(t, (i * (h + 8), 0))
    return out

def label(im, text):
    im = im.convert("RGB").copy(); d = ImageDraw.Draw(im)
    d.rectangle([0, 0, im.width, 50], fill=BAR)
    d.text((16, 10), text, font=font(26, True), fill=FG)
    return im

def maps(prefix):
    tiles = [Image.open(OUT / f"{prefix}_{k}.png").convert("RGB").resize((512, 512)) for k in ("BC", "N", "ORM", "E")]
    m = Image.new("RGB", (1024, 1024))
    for i, t in enumerate(tiles):
        m.paste(t, ((i % 2) * 512, (i // 2) * 512))
    return m

def main():
    info = json.load(open(WORK / "build_info.json")); fitj = json.load(open(WORK / "fit.json"))
    tris = info["export"]["tris"]; size = info["export"]["size_cm"]; c = info["counts"]
    dst = Image.new("RGB", (W, H), BG); draw = ImageDraw.Draw(dst)
    draw.rectangle([0, 0, W, 110], fill=BAR); draw.rectangle([0, 110, W, 114], fill=ACCENT)
    draw.text((48, 22), "HERO CRYSTAL CLUSTER  r2", font=font(42, True), fill=FG)
    draw.text((48, 70), f"Issue #53  ·  mesh_r2  ·  Blender 4.5.3 LTS  ·  R-042 queue 4 / PR #75 merge note  ·  SM_CrystalCacheCrystal replaces r1 on import  ·  "
              f"{c['cyan']} cyan + {c['amber']} amber crystals on a {c['rocks']}-chunk basalt nest", font=font(20), fill=MUTED)
    draw.text((W - 48, 28), "TRIAD DRIFT", font=font(22, True), fill=ACCENT, anchor="ra")
    draw.text((W - 48, 70), "ArtSource/EngAI/Props/CrystalCache/mesh_r2", font=font(16), fill=MUTED, anchor="ra")
    fy = H - 86
    draw.rectangle([0, fy, W, H], fill=BAR); draw.rectangle([0, fy, W, fy + 3], fill=ACCENT)
    draw.text((48, fy + 18), (f"KEY SPECS   Cluster {size[0]:.1f} × {size[1]:.1f} × {size[2]:.1f} cm   ·   {tris:,} tris (r1 1,384)   ·   "
               f"top {fitj['top_assembled_cm']} cm assembled, {fitj['room_under_rails_cm']} cm under the 167.8 cm rail underside, 180 cm cage   ·   "
               f"crystals ≥ {fitj['min_clearance_crystals_above_20cm_cm']} cm from the cage   ·   {len(info['ucx'])} UCX hulls   ·   180 cm pilot"), font=font(18), fill=FG)
    draw.text((48, fy + 50), "Pivot = base centre (min Z 0) → SOCKET_Crystal (0, 0, 30.05 cm)  ·  +Z up  ·  +X forward  ·  Route C procedural, live Bevel 1 seg + WeightedNormal  ·  "
              "UnitScaleFactor 100  ·  2K BC / N (DirectX) / ORM / E  ·  neutral studio HDRI", font=font(16), fill=MUTED)
    top, bottom, left, right, gap, cols, rows = 130, fy - 16, 24, W - 24, 16, 4, 3
    cw = (right - left - gap * (cols - 1)) // cols; ch = (bottom - top - gap * (rows - 1)) // rows
    cell = lambda cc, rr: (left + cc * (cw + gap), top + rr * (ch + gap), cw, ch)
    R = lambda n: Image.open(RENDERS / n)
    ref = Image.open(REF)
    refcrop = ref.convert("RGB").crop((1270, 20, 1690, 395))
    files = {
        (0, 0): (ref, "01  #53 MODELLING REFERENCE", "dense 10-15 cyan cluster, amber at the base, ~1.4 m"),
        (1, 0): (triptych([refcrop, R("r1_front.png"), R("r2_front.png")], ["reference", "r1 (merged #75)", "r2 (this PR)"]),
                 "02  REFERENCE  ·  r1  ·  r2", "front, same scale for r1 / r2"),
        (2, 0): (label(R("r2_seated_top.png"), "r2 seated in the r1 cradle"), "03  r2 IN THE CAGE", "on SOCKET_Crystal, open top"),
        (3, 0): (label(R("r1_seated_top.png"), "r1 (before), same camera"), "04  r1 IN THE CAGE", "the thin cluster the Lead flagged"),
        (0, 1): (R("r1_r2_pilot.png"), "05  r1 vs r2  +  180 cm PILOT", "front ortho, alone"),
        (1, 1): (R("r2_end_ortho.png"), "06  END ORTHO  +  PILOT", f"top {fitj['top_assembled_cm']} cm, rails 167.8, cage 180"),
        (2, 1): (R("r2_front_ortho.png"), "07  FRONT ORTHO (+X)  +  PILOT", "seated, emblem side"),
        (3, 1): (R("r2_hero.png"), "08  SM_CrystalCacheCrystal r2", "hero 3/4, textured, live modifiers"),
        (0, 2): (R("r2_closeup.png"), "09  CLOSE-UP  BASE", "amber crystals + glow, hard facets, bevel"),
        (1, 2): (R("r2_clay_wire.png"), "10  CLAY  +  HARD EDGES", "feature edges ≥ 20°"),
        (2, 2): (R("r2_ucx.png"), "11  UCX HULLS", f"{len(info['ucx'])} convex hulls: nest + one per cyan shard"),
        (3, 2): (maps("T_CrystalCacheCrystal"), "12  CRYSTAL 2K MAPS", "BC / N / ORM / E"),
    }
    for (cc, rr), (im, t, s) in files.items():
        x, y, w, h = cell(cc, rr)
        panel(dst, draw, im, x, y, w, h, t, s)
    dst.save(OUT / "contact_sheet.png", optimize=True)
    print("wrote", OUT / "contact_sheet.png", dst.size)

if __name__ == "__main__":
    main()

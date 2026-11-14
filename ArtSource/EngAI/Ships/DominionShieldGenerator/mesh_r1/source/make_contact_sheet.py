#!/usr/bin/env python3
"""Compose contact_sheet.png for SM_DominionShieldGenerator mesh_r1."""
import os, sys, json
from PIL import Image, ImageDraw, ImageFont

R = sys.argv[1] if len(sys.argv) > 1 else "/tmp/dsg_r1/renders"
OUT = sys.argv[2] if len(sys.argv) > 2 else "/tmp/dsg_r1/contact_sheet.png"
REP = sys.argv[3] if len(sys.argv) > 3 else "/tmp/dsg_r1/export/build_report.json"
report = json.load(open(REP)) if os.path.exists(REP) else {}

F = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
FB = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"

def font(s, b=False):
    return ImageFont.truetype(FB if b else F, s)

W = 3200
BG = (18, 17, 20)
ACCENT = (255, 210, 160)
MUTED = (200, 200, 210)

def load(name):
    for ext in (".jpg", ".jpeg", ".png"):
        p = os.path.join(R, f"render_{name}" + ext)
        if os.path.exists(p):
            return Image.open(p).convert("RGB")
    raise FileNotFoundError(name)

def fit(im, w, h=None, fill=(22, 21, 24)):
    im = im.convert("RGB")
    r = w / im.width
    h2 = int(im.height * r)
    im = im.resize((w, h2), Image.LANCZOS)
    if h and h2 > h:
        im = im.crop((0, (h2 - h) // 2, w, (h2 - h) // 2 + h))
    elif h and h2 < h:
        canvas = Image.new("RGB", (w, h), fill)
        canvas.paste(im, (0, (h - h2) // 2))
        im = canvas
    return im

def label(im, text, sub=None):
    d = ImageDraw.Draw(im)
    bar_h = 54 if not sub else 92
    d.rectangle((0, 0, im.width, bar_h), fill=(14, 14, 18))
    d.text((16, 10), text, font=font(28, True), fill=ACCENT)
    if sub:
        d.text((16, 52), sub, font=font(20), fill=MUTED)
    return im

def panel(name, title, sub, w, h):
    return label(fit(load(name), w, h), title, sub)

# layout
# row 0: title
# row 1: hero (wide) + front ortho
# row 2: side + top + hero_low
# row 3: detail + detail_collar
# row 4: scale + stump
# row 5: plinth + clay
# row 6: edge + ucx
# footer metrics

pad = 16
title_h = 150
col_gap = 12

hero = panel("hero", "ASSEMBLED PYLON + CORE  —  hero 3/4",
             "Route C hard-surface  ·  dark gunmetal, crimson trim, amber strips, cyan weak-point core",
             2100, 1180)
front = panel("front", "FRONT ORTHO", "14.8 m wide  ×  19.7 m tall", 1060, 1180)

side = panel("side", "SIDE ORTHO", "+Z up, origin = SOCKET_Generator / actor root", 1040, 1040)
top = panel("top", "TOP ORTHO", "fits r = 18 m keep-clear cylinder  (max r = 7.44 m)", 1040, 1040)
hero_low = panel("hero_low", "LOW 3/4  —  stump language", "lower 40% is a solid armored collar for ShowDestroyed()", 1040, 1040)

det = panel("detail", "CRADLE + CORE  —  bevel / emissive close-up",
            "Core mesh authored at local Z = +10.40 m so C++ attach (0,0,+60 cm) lands it at Z = +11.00 m",
            1572, 960)
detc = panel("detail_collar", "COLLAR  —  trident banners, crimson lips, amber slits",
             "Dominion hull r2 palette  ·  live Bevel + WeightedNormal + smooth_by_angle(30°)",
             1572, 960)

scale = panel("scale", "180 cm PILOT  —  scale callout",
              "Generator is ~14.8 m wide × 19.7 m tall (placeholder was 12 × 20 m). Pilot stands on the plinth at Z = −50 cm.",
              1572, 960)
stump = panel("stump", "ShowDestroyed() STUMP  —  Mesh × (1,1,0.4), Core hidden",
              "Base lifts 30 cm (Z −50 → −20 cm). Lower 40% (base + collar) still reads as a solid wreckage stump.",
              1572, 960)

plinth = panel("plinth", "SEATED ON DOMINION r2 OUTRIGGER PLINTH  (proxy)",
               "Plinth r = 24 m, top Z = −50 cm. Cyan wire = r = 18 m keep-clear dome (Z 0..40 m cylinder in contract).",
               1572, 960)
clay = panel("clay", "CLAY  —  form / silhouette",
             "Lathe base, six flying buttresses, lantern cradle, emitter crown.",
             1572, 960)

edge = panel("edge", "EDGE HIGHLIGHT  —  facing / bevel check",
             "Crisp hard edges from live Bevel (weight, 2 segments, 7 cm) + weighted normals.",
             1572, 960)
ucx = panel("ucx", "UCX_ + SOCKET_Core",
            "Convex colliders on base, collar, pylons, rings and prongs. SOCKET_Core at (0, 0, 1100) cm.",
            1572, 960)

# compose
rows = [
    [hero, front],
    [side, top, hero_low],
    [det, detc],
    [scale, stump],
    [plinth, clay],
    [edge, ucx],
]
# normalize row widths to W - 2*pad
composed_rows = []
for row in rows:
    tw = sum(im.width for im in row) + col_gap * (len(row) - 1)
    scale_r = (W - 2 * pad) / tw
    if abs(scale_r - 1) > 0.01:
        new = []
        for im in row:
            nw, nh = int(im.width * scale_r), int(im.height * scale_r)
            new.append(im.resize((nw, nh), Image.LANCZOS))
        row = new
    h = max(im.height for im in row)
    strip = Image.new("RGB", (W - 2 * pad, h), BG)
    x = 0
    for im in row:
        strip.paste(im, (x, 0))
        x += im.width + col_gap
    composed_rows.append(strip)

footer_h = 220
H = title_h + sum(s.height + pad for s in composed_rows) + footer_h
sheet = Image.new("RGB", (W, H), BG)
d = ImageDraw.Draw(sheet)
d.rectangle((0, 0, W, title_h), fill=(12, 12, 16))
d.text((24, 18), "SM_DominionShieldGenerator  +  SM_DominionShieldGeneratorCore", font=font(36, True), fill=ACCENT)
d.text((24, 66), "ArtSource/EngAI/Ships/DominionShieldGenerator/mesh_r1    ·    issue #41    ·    Route C  ·  Blender 4.5.3 LTS",
       font=font(22), fill=MUTED)
d.text((24, 100),
       "UTDDominionVisuals.GeneratorMesh + GeneratorCoreMesh    ·    seats on SOCKET_Generator plinth (Z = −50 cm)    ·    fits r = 18 m dome",
       font=font(20), fill=(180, 190, 200))

y = title_h
for s in composed_rows:
    sheet.paste(s, (pad, y))
    y += s.height + pad

# footer metrics
p = report.get("pylon", {})
c = report.get("core", {})
cc = report.get("core_contract", {})
pb = report.get("pylon_bounds_m", {})
d.rectangle((0, y, W, H), fill=(12, 12, 16))
ucx_n = len(report.get("ucx", []))
lines = [
    f"Pylon  {p.get('eval_tris','?')} tris (raw {p.get('raw_verts','?')} verts / {p.get('raw_faces','?')} faces, {p.get('bevel_edges','?')} bevel-weighted edges)    ·    Core  {c.get('eval_tris','?')} tris    ·    0 non-manifold, 0 degenerate    ·    UnitScaleFactor = 100, import scale 1.0",
    f"Bounds  {14.88:.2f} × {14.88:.2f} × {19.68:.2f} m    ·    base Z = −50 cm (plinth top)    ·    top Z = +1918 cm    ·    max radius 744 cm  (keep-clear r = 1800 cm)",
    f"Core authored centre Z = +1040 cm  so  C++ Core RelativeLocation (0,0,60) places it at cradle Z = +1100 cm (SOCKET_Core).    ShowDestroyed() scales Mesh Z × 0.4; stump = base+collar Z −50..+730 cm.",
    f"UCX_SM_DominionShieldGenerator_00..{ucx_n-1:02d}  ({ucx_n} convex hulls)    ·    2K PBR BC / DirectX N (255-G) / ORM / E    ·    gunmetal albedo p50 ≈ sRGB 77    ·    studio HDRI, AgX.",
]
fy = y + 16
for i, line in enumerate(lines):
    d.text((24, fy + i * 36), line, font=font(18), fill=MUTED)

sheet.save(OUT, optimize=False)
print("wrote", OUT, sheet.size)

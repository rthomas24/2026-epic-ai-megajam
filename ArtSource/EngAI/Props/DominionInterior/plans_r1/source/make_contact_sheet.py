#!/usr/bin/env python3
"""contact_sheet.png for the Dominion interior plans r1 (R-042 item 3): blockout renders (neutral studio HDRI + the C++
lights, 180 cm figures), measured plans and sections, trim sheet + decals + layout guide, C++ mismatches.
python3 make_contact_sheet.py <plans_r1_dir>"""
import os, sys, json
from PIL import Image, ImageDraw, ImageFont
Image.MAX_IMAGE_PIXELS = None
PL = sys.argv[1]; RV, PN, TX = (os.path.join(PL, d) for d in ("review", "plans", "textures"))
F, FB = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
def font(s, b=False): return ImageFont.truetype(FB if b else F, s)
W, PAD, BG, ACC, MUT = 3200, 16, (18, 17, 20), (255, 210, 160), (205, 205, 215)
L = json.load(open(os.path.join(PL, "interior_layout.json"))); MM = json.load(open(os.path.join(PL, "mismatches.json")))
def fit(p, w, h, bg=(24, 23, 27)):
    im = Image.open(p)
    if im.mode == "RGBA": b = Image.new("RGBA", im.size, (120, 120, 125, 255)); b.alpha_composite(im); im = b
    im = im.convert("RGB"); r = min(w / im.width, h / im.height)
    im = im.resize((int(im.width * r), int(im.height * r)), Image.LANCZOS)
    c = Image.new("RGB", (w, h), bg); c.paste(im, ((w - im.width) // 2, (h - im.height) // 2)); return c
def label(im, t, sub=None):
    d = ImageDraw.Draw(im); h = 46 if not sub else 78
    d.rectangle((0, 0, im.width, h), fill=(12, 12, 16)); d.text((12, 8), t, font=font(26, True), fill=ACC)
    if sub: d.text((12, 44), sub, font=font(19), fill=MUT)
    return im
def row(items, h, widths=None):
    n = len(items); tot = W - PAD * (n + 1); ws = [int(tot * f) for f in widths] if widths else [tot // n] * n
    r = Image.new("RGB", (W, h), BG); x = PAD
    for (p, t, sub), w in zip(items, ws):
        r.paste(label(fit(p, w, h), t, sub), (x, 0)); x += w + PAD
    return r
rows = []
hd = Image.new("RGB", (W, 290), BG); d = ImageDraw.Draw(hd)
d.text((PAD, 14), "Dominion Interior r1  ·  measured plans + trim sheet + decals  ·  R-042 queue item 3 (R-043: builders build it from the kit)", font=font(42, True), fill=(255, 255, 255))
rm = {r["name"]: r for r in L["rooms"]}
d.text((PAD, 74), "Route (cm, TDDominionInteriorCourse.inl @188acc50): pocket 6000x5200x3600 (PR #77) > hangar 2400x2800x1200 > corridor 4000 (340x600 clear) > "
       "flight deck 2400x2800x600 > link 800 > Warden 2000x2000x600 > link 800 > core 3600x3600x1800", font=font(21), fill=MUT)
d.text((PAD, 108), "400 cm kit grid, pieces by pivot at scale 1 (1282 placements, 15 kit meshes). All floors z 0: no stairs/ramps on the route. 6 doors 300 x 400 clear (R-042 >= 140 x 240). "
       "Ceilings 586 (ribs) to 3600.", font=font(21), fill=MUT)
d.text((PAD, 142), "Trim T_DI_InteriorTrim 2K (BC/N DirectX/ORM/E), 12 bands, 5.12 px/cm, BC median sRGB 87 (gunmetal bands 87-101). Decals T_DI_Decals 2K RGBA: trident, Triad mark, hazard frame, route arrow.",
       font=font(21), fill=MUT)
d.text((PAD, 180), "C++ mismatches: %d called out (bottom). Biggest: the hangar's X0 wall vs PR #77's open pocket, and the corridor cover gap 130 cm < 140." % len(MM), font=font(24, True), fill=ACC)
d.text((PAD, 222), "Renders: Cycles, Blender's neutral studio.exr HDRI + the C++ point lights, AgX. Yellow = 180 cm figures; red capsules = trooper (r40 h180) / Warden (r60 h260) at their C++ spawns; grey box = 524 x 488 x 230 ship footprint.",
       font=font(21), fill=MUT)
rows.append(hd)
R = lambda n: os.path.join(RV, n)
rows.append(row([(R("bo_cutaway_hangar_corridor.png"), "Cutaway · hangar + deflect corridor + flight deck", "kit at scale 1, ceilings + +Y walls hidden"),
                 (R("bo_cutaway_warden_core.png"), "Cutaway · Warden's gate + reactor core", "core, rings, shield, 4 conduits, heart")], 900))
rows.append(row([(R("bo_top.png"), "Blockout top · mouth (left) to core (right), 228 m", None)], 760))
rows.append(row([(R("eye_hangar.png"), "Eye 170 · hangar, pilot start side", "ship proxy on Pad A, HangarDoor"), (R("eye_corridor.png"), "Eye 170 · deflect corridor", "troopers at x 4750 behind cover"),
                 (R("eye_arena.png"), "Eye 170 · flight deck (arena)", "4 x Cover_Low, consoles")], 600))
rows.append(row([(R("eye_warden.png"), "Eye 170 · Warden's gate", "Warden capsule 260 at (9900, 0)"), (R("eye_core.png"), "Eye 170 · reactor core", "core 1200, shield 1380, 18 m room"),
                 (R("trim_preview.png"), "Trim preview · one 400 x 600 wall cell", "T_DI_InteriorTrim + T_DI_Decals, 180 cm figure")], 600))
rows.append(row([(os.path.join(PN, "DI_Plan_Top.png"), "Plan · top, kit pieces", "piece names, dims cm, consoles 150 clear"),
                 (os.path.join(PN, "DI_Plan_Gameplay.png"), "Plan · gameplay overlay", "volumes, spawns, checkpoints, sightlines")], 1300))
rows.append(row([(os.path.join(PN, "DI_Section_Long.png"), "Section A-A · long (y = 0)", "level floor, ceilings, doors"),
                 (os.path.join(PN, "DI_Sections_Cross.png"), "Sections B-H + walkability standard", "cover gap, doorway, rooms, ramp/stair rule")], 1000))
rows.append(row([(os.path.join(TX, "T_DI_InteriorTrim_Layout.png"), "Trim layout guide · UV bands in px", "T_DI_InteriorTrim + T_DI_Decals"),
                 (R("trim_preview_close.png"), "Trim close-up", "kick plate, strips, trident")], 900, (0.62, 0.38)))
mm = Image.new("RGB", (W, 120 + 46 * len(MM)), BG); d = ImageDraw.Draw(mm)
d.text((PAD, 10), "C++ / brief mismatches (TDDominionInteriorCourse.inl at 188acc50) — nothing in Source/ was edited", font=font(32, True), fill=ACC)
y = 64
for i, m in enumerate(MM):
    d.text((PAD, y), "%d. %s" % (i + 1, m), font=font(21), fill=(255, 255, 255) if i < 3 else MUT); y += 46
rows.append(mm)
H = sum(r.height for r in rows) + PAD * (len(rows) + 1)
out = Image.new("RGB", (W, H), BG); yy = PAD
for r in rows: out.paste(r, (0, yy)); yy += r.height + PAD
out.save(os.path.join(PL, "contact_sheet.png"), optimize=True); print("contact sheet", out.size)

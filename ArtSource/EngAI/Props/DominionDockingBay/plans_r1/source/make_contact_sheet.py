#!/usr/bin/env python3
"""contact_sheet.png for the Dominion docking bay r1 (#54): renders (neutral studio HDRI, 180 cm figures), the measured
plans, and the concept board vs the reference. python3 make_contact_sheet.py <plans_dir> <reference_png>"""
import os, sys, json
from PIL import Image, ImageDraw, ImageFont
PL = sys.argv[1]; REF = sys.argv[2]
RV, PN = os.path.join(PL, "review"), os.path.join(PL, "plans")
F, FB = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
def font(s, b=False): return ImageFont.truetype(FB if b else F, s)
W, PAD, BG, ACC, MUT = 3200, 16, (18, 17, 20), (255, 210, 160), (205, 205, 215)
rep = json.load(open(os.path.join(PL, "mesh", "exterior_report.json")))

def fit(p, w, h):
    im = Image.open(p).convert("RGB"); r = min(w / im.width, h / im.height)
    im = im.resize((int(im.width * r), int(im.height * r)), Image.LANCZOS)
    c = Image.new("RGB", (w, h), (24, 23, 27)); c.paste(im, ((w - im.width) // 2, (h - im.height) // 2)); return c
def label(im, t, sub=None):
    d = ImageDraw.Draw(im); h = 46 if not sub else 78
    d.rectangle((0, 0, im.width, h), fill=(12, 12, 16)); d.text((12, 8), t, font=font(26, True), fill=ACC)
    if sub: d.text((12, 44), sub, font=font(19), fill=MUT)
    return im
def row(items, h):
    n = len(items); w = (W - PAD * (n + 1)) // n
    r = Image.new("RGB", (W, h), BG)
    for i, (p, t, sub) in enumerate(items):
        r.paste(label(fit(p, w, h), t, sub), (PAD + i * (w + PAD), 0))
    return r

rows = []
# header
hd = Image.new("RGB", (W, 250), BG); d = ImageDraw.Draw(hd)
d.text((PAD, 14), "Dominion Docking Bay r1  ·  measured plans + bay-mouth exterior pieces  ·  issue #54 (R-043)", font=font(44, True), fill=(255, 255, 255))
d.text((PAD, 76), "Mouth 5000 x 3500 cm at hull r2 SOCKET_Bay (30000, 0, -2000) facing +X  ·  glide pocket 6000 x 5200 x 3600 cm on the 400 cm DominionInterior kit grid  ·  "
       "doors 300 x 2500 x 3500 cm (= ATDDockingBay DoorHalfWidth 2500)", font=font(24), fill=MUT)
d.text((PAD, 112), "Doors %d tris each, emitter %d tris (FBX, live Bevel + WeightedNormal applied at export; hidden SRC_ + UCX_ in DominionBay_Exterior.blend; UnitScaleFactor 100)"
       % (rep["SM_DominionBay_Door_L"]["tris"], rep["SM_DominionBay_TractorEmitter"]["tris"]), font=font(24), fill=MUT)
d.text((PAD, 148), "Interior is a BLOCKOUT of the real kit pieces (builders own the walkable level). BAY THRESHOLD (Lead N-061) marked: 400 cm hazard-chevron floor band + emissive jamb/header strips.",
       font=font(24), fill=ACC)
d.text((PAD, 186), "Renders: Cycles, Blender's neutral studio.exr HDRI, AgX. Yellow 180 cm figures for scale. Ships on the pads are render-only stand-ins (squad Lancer, Null fighters).",
       font=font(22), fill=MUT)
rows.append(hd)
R = lambda n: os.path.join(RV, n)
rows.append(row([(R("ext_closed_34.png"), "Exterior · closed", "hull r2 + doors at SOCKET_Bay"),
                 (R("ext_open_34.png"), "Exterior · open", "slabs at |Y| 2500..5000 (Open01 = 1)")], 900))
rows.append(row([(R("ext_closed_front.png"), "Front · closed", None), (R("ext_open_front.png"), "Front · open", None),
                 (R("ext_emitter.png"), "Tractor emitter on the brow", "(31200, 0, 1400), cyan GlowColor")], 600))
rows.append(row([(R("pc_pieces_34.png"), "Pieces · studio", "doors closed + emitter, 180 cm figure at the sill"),
                 (R("pc_door_detail.png"), "Door seam + hazard sill", "live 5 cm bevel, weighted normals"),
                 (R("pc_emitter_detail.png"), "Emitter detail", "900 x 900 x 186 cm, live 4 cm bevel")], 600))
rows.append(row([(R("bo_cutaway.png"), "Blockout cutaway · pocket + hangar", "kit pieces at scale 1, +Y wall/ceiling hidden"),
                 (R("bo_cutaway_mouth.png"), "Blockout cutaway · from the mouth", "threshold band + glow frame at x -7200")], 900))
rows.append(row([(R("bo_eye_start.png"), "Eye level · pilot start, looking out", "180 cm camera"),
                 (R("bo_eye_mouth.png"), "Eye level · at the threshold, looking in", "180 cm camera"),
                 (R("bo_top.png"), "Blockout top (ceilings off)", None)], 600))
rows.append(row([(os.path.join(PN, "DB_Plan_Top.png"), "Plan · top (floor z 0)", "piece names, 400 grid, hull X on top"),
                 (os.path.join(PN, "DB_Section_Side.png"), "Section A-A · side", None)], 900))
rows.append(row([(os.path.join(PN, "DB_Section_Front.png"), "Section B-B · front", "mouth = 2 x DoorHalfWidth"),
                 (os.path.join(PN, "DB_Hull_Mouth_Cone.png"), "Hull r2 mouth + approach cone (C++)", "mismatches called out")], 900))
# concept board vs reference
cb = Image.new("RGB", (W, 900), BG)
cb.paste(label(fit(REF, 1560, 900), "Reference board (#54)", "Docking Bay Dominion Modeling Reference.png"), (PAD, 0))
d = ImageDraw.Draw(cb); x0 = 1600; y = 10
d.text((x0, y), "Concept board vs reference", font=font(34, True), fill=ACC); y += 60
lines = [
 ("Kept from the board", True),
 ("• Retractable blast doors, closed and open states (slide sideways into the hull)", False),
 ("• Ring tractor-beam emitter above the mouth, cyan lens + glow ring, four clamp fins", False),
 ("• Dominion chevron split across the door seam, crimson band, hazard sill, panel/bolt wear", False),
 ("• Deck interface + gantry (P/S) inside: kit pads, walkway ring, consoles (blockout)", False),
 ("Changed for the game (hull r2 + C++ win)", True),
 ("• Opening 50 x 35 m, not the board's 28 x 12 m: hull r2 bow mouth + DoorHalfWidth 2500", False),
 ("• No separate 36 x 18 x 20 m module: the mouth is cut into hull r2 (merged PR #72)", False),
 ("• Emitter 9.0 m dia (board 8 m), 34 m above the mouth centre on the brow (board 3.5 m)", False),
 ("• Board's orange strip lights -> red-orange GlowColor on doors (Dominion palette)", False),
 ("• Board LOD/Nanite targets (millions of tris) not used: game-res trims, ~2.4k / 4.6k tris", False),
 ("Threshold (Lead N-061)", True),
 ("• Hazard-chevron floor band x -7200..-6800, full 5000 width; emissive edge strip at", False),
 ("  x -6800; emissive jamb + header strips; door sill hazard band on both slabs", False),
]
for t, h in lines:
    d.text((x0 + (0 if h else 10), y), t, font=font(26 if h else 23, h), fill=(255, 255, 255) if h else MUT); y += 46 if h else 38
rows.append(cb)
H = sum(r.height for r in rows) + PAD * (len(rows) + 1)
out = Image.new("RGB", (W, H), BG); yy = PAD
for r in rows: out.paste(r, (0, yy)); yy += r.height + PAD
out.save(os.path.join(PL, "contact_sheet.png"), optimize=True)
print("contact sheet", out.size)

#!/usr/bin/env python3
"""Compose contact_sheet.png for SM_DominionMothership mesh_r2 (renders + blueprint overlays)."""
import json, sys, math
from PIL import Image, ImageDraw, ImageFont
from scipy.spatial import ConvexHull
import numpy as np
B = "/tmp/dm_r2"; R = B + "/renders"
CONCEPT = sys.argv[1]; OUT = sys.argv[2]
V = json.load(open(B + "/verify.json"))
for _s in V["sockets"].values(): _s["ue_cm"] = [c + 0.0 if abs(c) > 0.04 else 0.0 for c in _s["ue_cm"]]
U = json.load(open(B + "/ucx_pts.json")); BR = json.load(open(B + "/export/build_report.json"))
F = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"; FB = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
def font(s, b=False): return ImageFont.truetype(FB if b else F, s)
W = 3200
def fit(im, w, h=None):
    im = im.convert("RGB"); r = w / im.width; h2 = int(im.height * r)
    im = im.resize((w, h2), Image.LANCZOS)
    if h and h2 > h: im = im.crop((0, (h2 - h) // 2, w, (h2 - h) // 2 + h))
    return im
def label(im, text, sub=None):
    d = ImageDraw.Draw(im); d.rectangle((0, 0, im.width, 54 if not sub else 92), fill=(14, 14, 18))
    d.text((18, 10), text, font=font(30, True), fill=(255, 210, 160))
    if sub: d.text((18, 54), sub, font=font(24), fill=(220, 220, 230))
    return im
# ---------- top blueprint overlay (UE coords; image right = +X bow, image down = +Y starboard)
top = Image.open(R + "/render_top.png").convert("RGB"); S = 760.0; P = top.width
top = Image.blend(top, Image.new("RGB", top.size, (10, 20, 40)), 0.45)
d = ImageDraw.Draw(top, "RGBA")
def tp(x, y): return ((x / S + 0.5) * P, (y / S + 0.5) * P)
for i in range(-350, 351, 50):
    d.line([tp(i, -380), tp(i, 380)], fill=(80, 140, 200, 60)); d.line([tp(-380, i), tp(380, i)], fill=(80, 140, 200, 60))
for name, pts in U.items():
    a = np.array(pts)[:, :2]
    try: h = ConvexHull(a); poly = [tp(*a[k]) for k in h.vertices]
    except Exception: continue
    d.polygon(poly, outline=(60, 255, 120, 230), fill=(60, 255, 120, 28))
d.rectangle([tp(-300, -80), tp(300, 80)], outline=(0, 220, 255, 255), width=4)
d.rectangle([tp(-180, -25), tp(240, 25)], outline=(255, 240, 0, 255), width=3)
d.rectangle([tp(240, -25), tp(300, 25)], outline=(255, 120, 0, 255), width=3)
f22 = font(22, True); f18 = font(18)
d.text(tp(-300, 84), "600 m (X -30000..+30000)  x  160 m (Y -8000..+8000)  x  100 m (Z -5000..+5000) cm", font=f22, fill=(0, 230, 255))
d.text(tp(-175, -24), "flyable trench |Y|<2500, X -18000..+24000", font=f18, fill=(255, 240, 0))
for s in V["sockets"].values(): pass
for n, s in V["sockets"].items():
    x, y, z = [c / 100 for c in s["ue_cm"]]; c = tp(x, y)
    d.ellipse([c[0] - 9, c[1] - 9, c[0] + 9, c[1] + 9], outline=(255, 40, 60, 255), width=4)
    yaw = math.radians(s["ue_yaw_deg"]); e = tp(x + math.cos(yaw) * 28, y + math.sin(yaw) * 28)
    if "Generator" in n or "Bay" in n: d.line([c, e], fill=(255, 40, 60, 255), width=4)
    lab = n.replace("SOCKET_", "") + f"\n({s['ue_cm'][0]:.0f}, {s['ue_cm'][1]:.0f}, {s['ue_cm'][2]:.0f})"
    off = (14, -44) if "Turret" not in n else (14, 6 if s["ue_cm"][1] > 0 else -50)
    if "Bay" in n: off = (-120, 20)
    d.multiline_text((c[0] + off[0], c[1] + off[1]), lab, font=f18, fill=(255, 255, 255, 255), stroke_width=2, stroke_fill=(0, 0, 0))
for (gx, gy) in ((275, 275), (-275, 275), (-275, -275), (275, -275)):
    c = tp(gx, gy); r = 18 / S * P; d.ellipse([c[0] - r, c[1] - r, c[0] + r, c[1] + r], outline=(80, 160, 255, 255), width=3)
d.text((20, P - 120), "green = UCX_ convex hulls (%d)   cyan = 600x160 m hull envelope   yellow = trench corridor   orange = bay pocket\nred = 12 SOCKET_ empties (arrows = local +X)   blue = r18 m generator-dome keep-clear (Z>0)   +X bow ->   +Y starboard = down" % V["ucx_count"], font=font(20), fill=(230, 230, 240))
top = label(top, "TOP BLUEPRINT (UE cm) - envelope, trench, outriggers, 12 sockets, UCX")
# ---------- side blueprint (camera at -Y looking +Y; image right = +X)
side = Image.open(R + "/render_side.png").convert("RGB"); S2 = 720.0; PW, PH = side.size
side = Image.blend(side, Image.new("RGB", side.size, (10, 20, 40)), 0.45); d2 = ImageDraw.Draw(side, "RGBA")
def sp(x, z): return ((x / S2 + 0.5) * PW, PH / 2 - (z - 5) / S2 * PW)
for name, pts in U.items():
    a = np.array(pts)[:, [0, 2]]
    try: h = ConvexHull(a); d2.polygon([sp(*a[k]) for k in h.vertices], outline=(60, 255, 120, 230), fill=(60, 255, 120, 22))
    except Exception: pass
d2.rectangle([sp(-300, 50), sp(300, -50)], outline=(0, 220, 255, 255), width=4)
d2.rectangle([sp(-232.5, 110), sp(-157.5, 50)], outline=(255, 0, 255, 255), width=3)
d2.text(sp(-300, 128), "bridge base 7500 x 4000 x 6000 cm, Z 5000..11000, centred (-19500, 0, 8000); spire mast to Z 19800", font=f22, fill=(255, 120, 255))
d2.rectangle([sp(-180, 80), sp(240, 50)], outline=(255, 240, 0, 255), width=3)
d2.rectangle([sp(240, -2.5), sp(300, -37.5)], outline=(255, 120, 0, 255), width=3)
d2.text(sp(150, -60), "bay mouth 5000 x 3500 cm\nZ -3750..-250, pocket X 24000..30000", font=f18, fill=(255, 160, 60))
for n, s in V["sockets"].items():
    x, y, z = [c / 100 for c in s["ue_cm"]]; c = sp(x, z)
    d2.ellipse([c[0] - 7, c[1] - 7, c[0] + 7, c[1] + 7], outline=(255, 40, 60, 255), width=3)
c = sp(165, 65); r = 14 / S2 * PW; d2.ellipse([c[0] - r, c[1] - r, c[0] + r, c[1] + r], outline=(0, 255, 255, 255), width=2)
side = label(side, "SIDE BLUEPRINT (starboard view, +X bow right) - core, bridge, trench, bay, UCX")
# ---------- assemble
rows = []
c = fit(Image.open(CONCEPT), 1600, 900); rows.append([label(c, "APPROVED v3 CONCEPT (Nicholas, as-is)"), label(fit(Image.open(R + "/render_hero.png"), 1600, 900), "mesh_r2 Cycles beauty - 3/4 hero")])
rows.append([label(fit(Image.open(R + f"/render_{k}.png"), 1066, 600), t) for k, t in (("side", "side ortho (starboard)"), ("front", "front ortho (bow)"), ("rear", "rear ortho (engine banks)"))])
rows.append([label(fit(Image.open(R + "/render_trench.png"), 1600, 900), "INSIDE THE 50 m FLYABLE TRENCH", "walls |Y| 2500..3400, Z 5000..8000 | cyan = 14 m reactor keep-clear | red = trench turret sockets"),
             label(fit(Image.open(R + "/render_bay.png"), 1600, 900), "BOW DOCKING BAY 5000 x 3500 cm + 60 m GLIDE POCKET", "green = launch point (33000,0,-2000): zero tris in X>30000, |Y|<2800, Z -4000..0")])
sc = Image.new("RGB", (1600, 900)); sc.paste(fit(Image.open(R + "/render_scale.png"), 1000, 563), (0, 100))
sc.paste(fit(Image.open(R + "/render_scale_close.png"), 600, 600), (1000, 80))
dd = ImageDraw.Draw(sc); dd.text((20, 700), "180 cm reference pilot (light-blue figure) standing on the bay deck just inside the 50 m x 35 m mouth.\nLeft: whole doorway (figure lower centre). Right: close-up of the same figure.", font=font(24), fill=(230, 230, 240))
rows.append([label(sc, "HUMAN SCALE - 180 cm FIGURE AT THE BAY MOUTH (R-042)"), label(fit(Image.open(R + "/render_generator.png"), 1600, 900), "OUTRIGGER PLINTH @ SOCKET_Generator_3 (27500,-27500,0)", "blue = r18 m dome keep-clear (Z>0) | red = guard perches +/-3000 tangent, Z+1500")])
rows.append([label(fit(Image.open(R + "/render_detail_super.png"), 1600, 900), "CLOSE-UP: STEPPED SUPERSTRUCTURE + SPIRES", "bevel highlights, window rows, amber courses along the aft city"),
             label(fit(Image.open(R + "/render_detail_armor.png"), 1600, 900), "CLOSE-UP: FLANK ARMOUR, RIBS, CRIMSON BAND", "layered plates, vertical ribs, original trident emblem")])
clay = fit(Image.open(R + "/render_clay.png"), 3200)
rows.append([label(clay, "CLAY / EDGE-HIGHLIGHT CHECK (mid-grey, no textures)", "live Bevel + WeightedNormal + smooth-by-angle 30deg; studio HDRI so edges read without a dark void")])

topf = fit(top, 1500); sidef = fit(side, 1700)
# stats panel under the side blueprint
st = Image.new("RGB", (1700, topf.height - sidef.height), (16, 16, 22)); ds = ImageDraw.Draw(st)
rm = V["render_mesh_clearance"]; uc = V["ucx_clearance"]
lines = [f"SM_DominionMothership  mesh_r2   |   {V['tris']:,} tris evaluated (budget <= 150,000)   |   {V['ucx_count']} UCX (r1 59 kept + r2 extras; all closed + convex: {V['ucx_all_convex_closed']})",
         f"FBX UnitScaleFactor {V['fbx_UnitScaleFactor']:.0f}, import scale 1.0  |  hull core {V['hull_core_extent_cm'][0]:.0f} x {V['hull_core_extent_cm'][1]:.0f} x {V['hull_core_extent_cm'][2]:.0f} cm  |  overall bounds X {V['bounds_cm']['min'][0]:.0f}..{V['bounds_cm']['max'][0]:.0f}, Y +/-{V['bounds_cm']['max'][1]:.0f}, Z {V['bounds_cm']['min'][2]:.0f}..{V['bounds_cm']['max'][2]:.0f}",
         f"Trench corridor |Y|<2350, Z 5000..8500: {rm[list(rm)[0]]['verts_inside']} verts / {rm[list(rm)[0]]['tri_box_overlaps']} tris; UCX in |Y|<2450: {len(uc[list(uc)[0]])}  |  forward entry: {rm[list(rm)[1]]['tri_box_overlaps']} tris",
         f"Launch channel X>30000 |Y|<2800 Z -4000..0: {rm[list(rm)[2]]['tri_box_overlaps']} tris, {len(uc[list(uc)[2]])} UCX  |  glide pocket: {rm[list(rm)[3]]['tri_box_overlaps']} tris, {len(uc[list(uc)[3]])} UCX",
         f"Reactor clearance: nearest render tri {V['reactor_min_clearance_cm']:.0f} cm, nearest UCX {V['reactor_ucx_min_clearance_cm']:.0f} cm (need >= 1400)  |  dome volumes r1800 Z 0..4000: 0 tris, 0 UCX",
         f"Emissive area {V['emissive_area_share_pct']}% of surface (<= 5%)  |  textures 2048 BC/N(DirectX)/ORM/E from 4096 masters (Lanczos) + tiling Detail_N / Detail_ORM 2048  |  .blend ships live Bevel+WN",
         "SOCKETS (UE cm, yaw of local +X):"]
it = list(V["sockets"].items())
for a in range(0, len(it), 2):
    l = "  "
    for n, s in it[a:a + 2]: l += f"{n.replace('SOCKET_', ''):<15}({s['ue_cm'][0]:>7.0f},{s['ue_cm'][1]:>7.0f},{s['ue_cm'][2]:>6.0f}) {s['ue_yaw_deg']:>4.0f}deg    "
    lines.append(l)
fm = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 20)
y = 16
for i, l in enumerate(lines): ds.text((20, y), l, font=fm if l.startswith("  ") else font(20, i == 0), fill=(240, 240, 245) if i else (255, 210, 160)); y += 28
right = Image.new("RGB", (1700, topf.height)); right.paste(sidef, (0, 0)); right.paste(st, (0, sidef.height))
rows.append([topf, right])
H = 110 + sum(max(im.height for im in r) for r in rows) + 10 * len(rows)
sheet = Image.new("RGB", (W, H), (22, 20, 26)); dh = ImageDraw.Draw(sheet)
dh.text((24, 18), "#41 Dominion Mothership Hull r2 - SM_DominionMothership - contact sheet (Blender 4.5.3, procedural, from approved v3)", font=font(42, True), fill=(255, 220, 180))
dh.text((24, 72), "Unreal local space: +X bow, +Y starboard, +Z up, origin = hull centre, cm. Cycles + studio HDRI previews (UE look will differ). Gameplay sockets/trench/bay/UCX-r1 unchanged.", font=font(24), fill=(210, 210, 220))
y = 110
for r in rows:
    x = 0
    for im in r: sheet.paste(im, (x, y)); x += im.width
    y += max(im.height for im in r) + 10
sheet.save(OUT, optimize=True); print(sheet.size)

# SPDX-License-Identifier: MIT
# Triad Drift - Outpost: derived textures (system Python + Pillow; no Blender).
#   python scripts/op_textures.py [src_dir] [tag]
# Writes into textures/:
#   T_Outpost_Cabin_M.png     1K accent mask for the material variants: R = accent paint (the yellow hazard marks of
#                             the delivered base colour + the two ramp handrails, from review/rail_uv_<tag>.json),
#                             G = light painted panels (bright, unsaturated), B = 0.
#   T_Outpost_Cabin_E.png     1K, black except the block the added lights (beacon, awning bars) are mapped to.
#   T_Outpost_Cabin_ORM.png   1K copy of T_VerdanceOutpost_ORM (roughness / metallic are low-frequency).
#   T_Outpost_Console_N.png, _ORM.png, _E.png   1K copies of Grok Bot's 2K console maps.
#   T_Outpost_Console_M.png   1K: R = yellow / orange accents of the console, G = light panels.
# The 2K maps that are used as delivered (cabin BC, N; console BC) are imported straight from Grok Bot's folder.
import json, os, sys
from PIL import Image, ImageChops, ImageDraw, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
REPO = os.path.dirname(os.path.dirname(os.path.dirname(KIT)))
SRC = (sys.argv[1] if len(sys.argv) > 1 else "") or os.environ.get("OUTPOST_SRC") or os.path.join(REPO, "ArtSource", "EngAI", "Props", "VerdanceOutpost", "mesh_r1")
TAG = sys.argv[2] if len(sys.argv) > 2 else "r1"
OUT = os.path.join(KIT, "textures")
os.makedirs(OUT, exist_ok=True)


def masks(bc_path, hue_lo, hue_hi):
    im = Image.open(bc_path).convert("RGB")
    h, s, v = im.convert("HSV").split()
    lo, hi = hue_lo * 255.0 / 360.0, hue_hi * 255.0 / 360.0
    accent = ImageChops.multiply(ImageChops.multiply(h.point(lambda x: 255 if lo <= x <= hi else 0), s.point(lambda x: 255 if x > 115 else 0)),
                                 v.point(lambda x: 255 if x > 90 else 0))
    panel = ImageChops.multiply(v.point(lambda x: max(0, min(255, int((x - 127) * 5)))), s.point(lambda x: max(0, min(255, int((56 - x) * 10)))))
    return im.size, accent, panel


def save_mask(name, size, accent, panel, extra_tris=None):
    if extra_tris:
        d = ImageDraw.Draw(accent)
        w, h = size
        for tri in extra_tris:
            d.polygon([(u * w, (1.0 - v) * h) for u, v in tri], fill=255, outline=255)
    accent = accent.filter(ImageFilter.MaxFilter(5)).filter(ImageFilter.GaussianBlur(1.2))
    out = Image.merge("RGB", (accent, panel, Image.new("L", size, 0))).resize((1024, 1024), Image.LANCZOS)
    out.save(os.path.join(OUT, name))
    r, g, _ = out.split()
    n = float(out.size[0] * out.size[1])
    return {"file": name, "accent_pct": round(sum(r.histogram()[128:]) / n * 100, 2), "panel_pct": round(sum(g.histogram()[128:]) / n * 100, 2)}


res = []
with open(os.path.join(KIT, "review", "rail_uv_%s.json" % TAG)) as fh:
    rail = json.load(fh)
size, acc, pan = masks(os.path.join(SRC, "T_VerdanceOutpost_BC.png"), 30, 62)
res.append(save_mask("T_Outpost_Cabin_M.png", size, acc, pan, rail))
size, acc, pan = masks(os.path.join(SRC, "T_ResearchConsole_BC.png"), 12, 62)
res.append(save_mask("T_Outpost_Console_M.png", size, acc, pan))
for src, dst in (("T_VerdanceOutpost_ORM", "T_Outpost_Cabin_ORM"), ("T_ResearchConsole_N", "T_Outpost_Console_N"),
                 ("T_ResearchConsole_ORM", "T_Outpost_Console_ORM"), ("T_ResearchConsole_E", "T_Outpost_Console_E")):
    im = Image.open(os.path.join(SRC, src + ".png")).convert("RGB").resize((1024, 1024), Image.LANCZOS)
    im.save(os.path.join(OUT, dst + ".png"))
    res.append({"file": dst + ".png"})
# cabin emissive: black, with one lit block where op_build.py parked the beacon and awning-light UVs
with open(os.path.join(KIT, "review", "build_%s.json" % TAG)) as fh:
    build = json.load(fh)
u, v = build["glow_uv"]
half = build["glow_block_uv"] * 0.5 * 0.8
e = Image.new("RGB", (1024, 1024), (0, 0, 0))
ImageDraw.Draw(e).rectangle([(u - half) * 1024, (1 - v - half) * 1024, (u + half) * 1024, (1 - v + half) * 1024], fill=(255, 255, 255))
e.save(os.path.join(OUT, "T_Outpost_Cabin_E.png"))
res.append({"file": "T_Outpost_Cabin_E.png", "glow_uv": [u, v]})
print(json.dumps(res, indent=1))

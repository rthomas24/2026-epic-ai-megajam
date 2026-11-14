# SPDX-License-Identifier: MIT
# Kit, Dominion set (r7) - the Dominion trim re-composed from Grok Bot's interior trim sheet (PR #82,
# ArtSource/EngAI/Props/DominionInterior/plans_r1/textures/T_DI_InteriorTrim_{BC,N,ORM}.png: 12 horizontal bands,
# 5.12 px/cm, tiling every 400 cm in U) into the kit's own atlas layout (kit_regions.py, 2.56 px/cm), so the kit's
# pieces, UVs, ribs and lighting stay as they are and only the look changes. The swatches and the console screen /
# hatch keep the kit's painted Dominion values (read from the current T_KitD_Trim_*). Writes T_KitD_Trim_{BC,N,ORM}.png.
#   python ArtSource/EngProps/Kit/scripts/kit_atlas_from_grok.py
import json
import os
import sys

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
from kit_regions import ATLAS, REGIONS  # noqa: E402

SRC = os.path.join(os.path.dirname(os.path.dirname(KIT)), "EngAI", "Props", "DominionInterior", "plans_r1", "textures")
with open(os.path.join(SRC, "trim_layout.json"), encoding="utf-8") as fh:
    BANDS = {b["key"]: b for b in json.load(fh)["trim"]["bands"]}
PX = 2.56                                       # the kit atlas: px per cm

# region -> a stack of (band, cm) rows from the region's top, and the region's width in cm
STACKS = {
    "wall":    (400, [("TRIM_CAP", 25), ("WALL_PANEL", 100), ("CRIMSON_BAND", 25), ("WALL_PANEL", 100), ("KICK_PLATE", 25), ("KICK_PLATE", 25)]),
    "wallB":   (400, [("TRIM_CAP", 25), ("WALL_PANEL", 100), ("VENT", 25), ("WALL_PANEL", 100), ("KICK_PLATE", 25), ("KICK_PLATE", 25)]),
    "floor":   (400, [("DECK_PLATE", 50)] * 8),
    "grate":   (200, [("VENT", 25)] * 8),
    "roof":    (200, [("WALL_PANEL", 100)] * 2),
    "frame":   (200, [("TRIM_CAP", 50)]),
    "hazard":  (200, [("HAZARD", 25)] * 2),
    "kick":    (200, [("KICK_PLATE", 25)] * 2),
    "rail":    (200, [("CRIMSON_BAND", 25)] * 2),
    "tread":   (200, [("DECK_PLATE", 50)] * 2),
    "pillar":  (200, [("WALL_PANEL", 100)]),
    "console": (200, [("TRIM_CAP", 50)] * 2),
    "vent":    (100, [("VENT", 25)] * 4),
}


def band_strip(img, key, width_cm, height_cm):
    """Band `key` cropped to width_cm of its 400 cm tile and height_cm of its own height, resampled to the kit's 2.56 px/cm."""
    b = BANDS[key]
    y0, y1 = b["v_px"]
    take_h = int(round(min(height_cm, b["height_cm"]) * 5.12))
    crop = img.crop((0, y0, int(round(width_cm * 5.12)), y0 + take_h))
    return crop.resize((int(round(width_cm * PX)), int(round(height_cm * PX))), Image.LANCZOS)


# Grok Bot's sheet is mid-grey (wall BC ~0.34 sRGB); the kit's lighting budget and review round 2 ("gloss black armour")
# were set on a near-black Dominion (wall ~0.11, floor ~0.19). So the BC is graded back to those values in linear light:
# neutral pixels take the region's factor, saturated ones (the crimson band, hazard orange) keep most of their value.
TARGET = {"floor": 0.15, "tread": 0.15, "grate": 0.13, "hazard": None, "rail": None}
TARGET_DEFAULT = 0.13
SAT_KEEP = 0.6


def _lin(v):
    v /= 255.0
    return v / 12.92 if v <= 0.04045 else ((v + 0.055) / 1.055) ** 2.4


def _srgb(v):
    v = max(0.0, min(1.0, v))
    return int(round(255 * (v * 12.92 if v <= 0.0031308 else 1.055 * v ** (1 / 2.4) - 0.055)))


def _lut(k):
    t = [_srgb(_lin(float(i)) * k) for i in range(256)]
    return t * 3


def grade(tile, reg):
    target = TARGET.get(reg, TARGET_DEFAULT)
    if target is None:
        return tile
    from PIL import ImageStat
    mean = sum(ImageStat.Stat(tile.convert("L")).mean) / 1.0
    k = min(1.0, (_lin(target * 255.0) / max(_lin(mean), 1e-4)))
    dark = tile.point(_lut(k))
    keep = tile.point(_lut(max(k, SAT_KEEP)))
    sat = tile.convert("HSV").split()[1].point(lambda v: min(255, v * 2))
    return Image.composite(keep, dark, sat)


def build(kind):
    src = Image.open(os.path.join(SRC, "T_DI_InteriorTrim_%s.png" % kind)).convert("RGB")
    base = Image.open(os.path.join(KIT, "textures", "T_KitD_Trim_%s.png" % kind)).convert("RGB")
    out = base.copy()
    for reg, (w_cm, rows) in STACKS.items():
        x, y, w, h = REGIONS[reg]
        tile = Image.new("RGB", (w, h))
        yy = 0
        for key, cm in rows:
            s = band_strip(src, key, w_cm, cm)
            tile.paste(s, (0, yy))
            yy += s.size[1]
        tile = tile.resize((w, h), Image.LANCZOS) if tile.size != (w, h) else tile
        if kind == "BC":
            tile = grade(tile, reg)
        out.paste(tile, (x, y))
    return out


def main():
    for kind in ("BC", "N", "ORM"):
        build(kind).save(os.path.join(KIT, "textures", "T_KitD_Trim_%s.png" % kind))
    print("wrote T_KitD_Trim_{BC,N,ORM} from", SRC)


if __name__ == "__main__":
    main()

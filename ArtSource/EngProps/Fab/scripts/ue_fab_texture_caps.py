# SPDX-License-Identifier: MIT
# Triad Drift - cook-size caps for every texture under /Game/TriadDrift/Gameplay/Props/Fab/** (Lead, Nov 14: the Shipping
# zip is at 931 MB of 1 GB). Sets Maximum Texture Size only: no re-import, no source change. TD_FAB_DRY=1 reports only.
#   tiling ground sets (Props/Fab/Textures/<Set>/): base colour 1024, normal 1024, ORM / masks 512
#   decal masks (ScorchMark, AshDrift): 512
#   everything else (scatter and dressing props, Mechanic Base, PipeBeam, plants): colour 1024, normal 512, ORM / masks 512
# Estimates the cooked size before and after from the cooked dimensions and the format, with the full mip chain:
#   DXT1 0.5 B/px (colour without alpha, masks), DXT5 / BC5 / BC7 / G8 1 B/px (colour with alpha, normals, grayscale).
import json
import os

import unreal

FAB = "/Game/TriadDrift/Gameplay/Props/Fab"
DRY = os.environ.get("TD_FAB_DRY", "") == "1"
eas = unreal.EditorAssetLibrary
TCS = unreal.TextureCompressionSettings


def log(m):
    unreal.log("TDPY: " + m)


def role(t):
    cs = t.get_editor_property("compression_settings")
    if cs == TCS.TC_NORMALMAP:
        return "normal"
    if cs in (TCS.TC_DEFAULT, TCS.TC_BC7) and t.get_editor_property("srgb"):
        return "colour"
    return "mask"


def cap_for(path, r):
    if "/Fab/ScorchMark/" in path or "/Fab/AshDrift/" in path:
        return 512
    if "/Fab/Textures/" in path:
        return {"colour": 1024, "normal": 1024, "mask": 512}[r]
    return {"colour": 1024, "normal": 512, "mask": 512}[r]


def bpp(t, r):
    cs = t.get_editor_property("compression_settings")
    if r == "normal" or cs in (TCS.TC_GRAYSCALE, TCS.TC_BC7, TCS.TC_ALPHA):
        return 1.0
    if r == "colour" and not t.get_editor_property("compression_no_alpha") and t.get_name().endswith("VerdancePlants_BC"):
        return 1.0                                      # the plant atlas carries its opacity in alpha (DXT5)
    return 0.5


def mb(src_w, src_h, cap, b):
    big = max(src_w, src_h)
    s = min(big, cap) / float(big) if big else 1.0
    return src_w * s * src_h * s * b * 4.0 / 3.0 / (1024 * 1024)


rows, before_t, after_t = [], 0.0, 0.0
for p in eas.list_assets(FAB, recursive=True, include_folder=False):
    p = p.split(".")[0]
    if str(eas.find_asset_data(p).asset_class_path.asset_name) != "Texture2D":
        continue
    t = eas.load_asset(p)
    w, h = t.blueprint_get_size_x(), t.blueprint_get_size_y()     # editor: source dimensions
    src = t.source.get_size_x() if hasattr(t, "source") else None
    r = role(t)
    old = t.get_editor_property("max_texture_size") or 16384
    new = min(old, cap_for(p, r))
    b = bpp(t, r)
    before, after = mb(w, h, old, b), mb(w, h, new, b)
    before_t += before
    after_t += after
    if new != old and not DRY:
        t.set_editor_property("max_texture_size", new)
        eas.save_loaded_asset(t, False)
    rows.append((p[len(FAB) + 1:], r, "%dx%d" % (w, h), old if old != 16384 else 0, new, round(before, 2), round(after, 2)))
for row in sorted(rows):
    log("CAP %s %s src %s max %s -> %s  %.2f -> %.2f MB" % row)
log("TOTAL %d textures: %.1f MB -> %.1f MB (saved %.1f MB, uncompressed estimate before the pak's compression)%s" % (
    len(rows), before_t, after_t, before_t - after_t, " DRY" if DRY else ""))
log("OK")

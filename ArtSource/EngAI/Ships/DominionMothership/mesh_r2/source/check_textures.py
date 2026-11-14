#!/usr/bin/env python3
"""Texture QA for T_DominionMothership_* (2K ship set + tiling detail)."""
import sys, json, os
import numpy as np
from PIL import Image
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_textures as mt
D = sys.argv[1]; R = {}
def L(n): return np.asarray(Image.open(os.path.join(D, n)).convert("RGB")).astype(np.int32)
for n in ("BC", "N", "ORM", "E", "Detail_N", "Detail_ORM"):
    im = Image.open(os.path.join(D, f"T_DominionMothership_{n}.png")); R[n] = {"size": list(im.size), "mode": im.mode, "bytes": os.path.getsize(os.path.join(D, f"T_DominionMothership_{n}.png"))}
for n in ("N", "Detail_N"):
    a = L(f"T_DominionMothership_{n}.png"); flat = np.all(np.abs(a - [128, 128, 255]) <= 2, -1)
    v = a / 255.0 * 2 - 1; ln = np.linalg.norm(v, axis=-1)
    R[n].update({"flat_pixels_pct_within2_of_128_128_255": round(100 * flat.mean(), 1), "median_rgb": np.median(a.reshape(-1, 3), 0).tolist(), "unit_length_mean": round(float(ln.mean()), 3)})
# DirectX convention proof: a bump's upper edge (faces image-up = +V) must encode G < 128 (DirectX inverts OpenGL green)
yy, xx = np.mgrid[0:64, 0:64]; bump = np.exp(-((yy - 32) ** 2 + (xx - 32) ** 2) / 60.0).astype(np.float32)
n = mt.normal_dx(bump, 6.0); R["directx_test"] = {"upper_edge_G": round(float(n[26, 32, 1] * 255), 1), "lower_edge_G": round(float(n[38, 32, 1] * 255), 1), "right_edge_R": round(float(n[32, 38, 0] * 255), 1), "pass": bool(n[26, 32, 1] < 0.5 < n[38, 32, 1] and n[32, 38, 0] > 0.5)}
o = L("T_DominionMothership_ORM.png"); R["ORM"].update({"R_AO_mean": round(o[..., 0].mean() / 255, 3), "G_rough_mean": round(o[..., 1].mean() / 255, 3), "B_metal_mean": round(o[..., 2].mean() / 255, 3)})
e = L("T_DominionMothership_E.png"); mx = e.max(-1)
R["E"].update({"lit_pixels_pct": round(100 * (mx > 0).mean(), 2), "pixels_with_max_1_to_18": int(((mx > 0) & (mx <= 18)).sum()), "max_rgb": e.reshape(-1, 3).max(0).tolist()})
for n in ("Detail_N", "Detail_ORM"):
    a = L(f"T_DominionMothership_{n}.png").astype(np.float32)
    inner_c = np.abs(np.diff(a, axis=1)).mean(); inner_r = np.abs(np.diff(a, axis=0)).mean()
    wrap_c = np.abs(a[:, 0] - a[:, -1]).mean(); wrap_r = np.abs(a[0] - a[-1]).mean()
    R[n].update({"wrap_seam_diff_cols": round(float(wrap_c), 2), "interior_adjacent_diff_cols": round(float(inner_c), 2), "wrap_seam_diff_rows": round(float(wrap_r), 2), "interior_adjacent_diff_rows": round(float(inner_r), 2), "tileable": bool(wrap_c < 3 * inner_c and wrap_r < 3 * inner_r)})
dorm = L("T_DominionMothership_Detail_ORM.png"); R["Detail_ORM"].update({"R_AO_mean": round(dorm[..., 0].mean() / 255, 3), "G_rough_mean": round(dorm[..., 1].mean() / 255, 3), "B_max": int(dorm[..., 2].max())})
print(json.dumps(R, indent=1)); json.dump(R, open(sys.argv[2], "w"), indent=1)

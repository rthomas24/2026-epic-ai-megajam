"""Trim-layout guide for builders: T_DI_InteriorTrim bands (px, UV v, cm) and T_DI_Decals rectangles.
python3 make_trim_guide.py <plans_r1_dir>  ->  textures/T_DI_InteriorTrim_Layout.png"""
import sys, os, json, textwrap
import numpy as np
import matplotlib; matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
from PIL import Image
PL = sys.argv[1]; T = os.path.join(PL, "textures"); J = json.load(open(os.path.join(T, "trim_layout.json")))
BG = "#f4f2ec"
fig = plt.figure(figsize=(26, 16), dpi=110); fig.patch.set_facecolor(BG)
ax = fig.add_axes([0.02, 0.06, 0.36, 0.86]); ax.imshow(Image.open(os.path.join(T, "T_DI_InteriorTrim_BC.png")), extent=(0, 2048, 2048, 0))
ax.set_title("T_DI_InteriorTrim_BC  (2048 x 2048, tiles in U; 5.12 px/cm, 1 U tile = 400 cm = 1 kit cell)", loc="left", fontsize=12, fontweight="bold")
for b in J["trim"]["bands"]:
    y0, y1 = b["v_px"]; ax.axhline(y0, color="#00e0ff", lw=1); ax.text(2060, (y0 + y1) / 2, b["key"], va="center", fontsize=9, fontweight="bold", clip_on=False)
ax.axhline(2048, color="#00e0ff", lw=1)
for x in range(0, 2049, 512): ax.axvline(x, color="#ffffff", lw=0.5, ls=":")
ax.set_xticks(range(0, 2049, 256)); ax.set_yticks([b["v_px"][0] for b in J["trim"]["bands"]] + [2048]); ax.tick_params(labelsize=7)
ax.set_xlabel("U px (512 px = 100 cm)"); ax.set_ylabel("V px from the top")
tx = fig.add_axes([0.47, 0.40, 0.52, 0.52]); tx.axis("off")
tx.set_title("UV BANDS · V px from the top (Unreal/Blender UV v = 1 - px/2048) · world height at 5.12 px/cm", loc="left", fontsize=12, fontweight="bold")
cols = ("band", "V px", "UV v (min..max)", "px", "cm", "use")
xs = (0.0, 0.14, 0.23, 0.37, 0.42, 0.48)
for j, c in enumerate(cols): tx.text(xs[j], 0.97, c, fontsize=9.5, fontweight="bold", transform=tx.transAxes)
for i, b in enumerate(J["trim"]["bands"]):
    y = 0.92 - i * 0.068
    row = (b["key"], "%d-%d" % tuple(b["v_px"]), "%.4f..%.4f" % tuple(b["uv_v"]), str(b["height_px"]), "%.1f" % b["height_cm"], b["desc"])
    for j, t in enumerate(row): tx.text(xs[j], y, t, fontsize=8.6, transform=tx.transAxes, fontweight="bold" if j == 0 else None)
dx = fig.add_axes([0.47, 0.04, 0.24, 0.33])
d = Image.open(os.path.join(T, "T_DI_Decals_BC.png")); bgi = Image.new("RGBA", d.size, (120, 120, 125, 255)); bgi.alpha_composite(d)
dx.imshow(bgi.convert("RGB"), extent=(0, 2048, 2048, 0)); dx.set_title("T_DI_Decals_BC (RGBA, alpha = opacity) on grey", loc="left", fontsize=11, fontweight="bold")
for k in J["decals"]:
    x0, y0, x1, y1 = k["px"]; dx.add_patch(Rectangle((x0 + 4, y0 + 4), x1 - x0 - 8, y1 - y0 - 8, fc="none", ec="#00e0ff", lw=1.2))
    dx.text(x0 + 20, y0 + 70, k["key"], color="#00e0ff", fontsize=9, fontweight="bold")
dx.tick_params(labelsize=7)
ex = fig.add_axes([0.73, 0.04, 0.26, 0.33]); ex.axis("off")
ex.set_title("DECAL RECTANGLES (px, UV u0 v0 u1 v1) · suggested world size", loc="left", fontsize=11, fontweight="bold")
for i, k in enumerate(J["decals"]):
    ex.text(0, 0.9 - i * 0.22, "%s  px %s\nUV (%.2f, %.2f)-(%.2f, %.2f)\n%s" % (k["key"], k["px"], *k["uv"], textwrap.fill(k["desc"], 62)), fontsize=8.3, transform=ex.transAxes, va="top", wrap=True)
fig.text(0.02, 0.015, "Material: one trim material (R-043): BC sRGB, N DirectX (Unreal green, no flip), ORM linear (R AO, G roughness, B metallic), "
         "E sRGB colour x EmissiveStrength (strips already coloured: red-orange / amber / cyan / white). Decals: DBuffer decal or a 0.5 cm offset quad, "
         "same N/ORM/E layout. Keep UV U at 1 tile per 400 cm so the panel seams land on the kit's 400 cm ribs.", fontsize=9.5)
fig.savefig(os.path.join(T, "T_DI_InteriorTrim_Layout.png"), facecolor=BG); print("guide ok")

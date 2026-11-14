# SPDX-License-Identifier: MIT
# Triad Drift - Quixel Megascans "Assorted Plants" (ocskq2, a leaf atlas, Fab Standard) -> Verdance ground-cover card plants:
#   blender --background --factory-startup --python prep_fab_plants.py -- <folder with the 4K atlas maps> <out dir>
# 1. Atlas at 1024: T_Fab_VerdancePlants_BC (RGBA: colour with the opacity in alpha; greens shifted to Verdance teal, the
#    dark red-brown leaves to violet) and _N (convention measured against the displacement map).
# 2. Finds the leaf islands in the opacity mask and sorts them: ferns (sparse fronds), broad leaves (monstera-like) and long
#    leaves (tall, narrow; the two lying horizontally are skipped).
# 3. Builds three clumps from bent quads (4 triangles a card) mapped on those islands, base at the origin, no collision:
#    SM_Fab_VerdancePlant_Fern, _Broad, _Long. One material M_Fab_VerdancePlants.
import json
import math
import os
import sys
from collections import deque

import bpy
import numpy as np

SRC, OUT = sys.argv[sys.argv.index("--") + 1:][:2]
EXP = os.path.join(OUT, "export")
os.makedirs(EXP, exist_ok=True)
P = os.path.join(SRC, "Assorted_Plants_ocskq2_4K_")
N = 1024
rep = {}


def load(name, size=N):
    im = bpy.data.images.load(P + name + ".jpg")
    im.colorspace_settings.name = "Non-Color"
    im.scale(size, size)
    x = np.empty(size * size * 4, np.float32)
    im.pixels.foreach_get(x)
    bpy.data.images.remove(im)
    return x.reshape(size, size, 4)                    # rows bottom-up = V up


def save(arr, name, alpha=False):
    im = bpy.data.images.new(name, N, N, alpha=alpha)
    im.colorspace_settings.name = "Non-Color"
    o = arr.copy()
    if not alpha:
        o[..., 3] = 1.0
    im.pixels.foreach_set(np.clip(o, 0, 1).ravel())
    im.filepath_raw = os.path.join(EXP, name + ".png")
    im.file_format = "PNG"
    if alpha:
        im.alpha_mode = "STRAIGHT"
        bpy.context.scene.render.image_settings.color_mode = "RGBA"
    im.save()
    return im


def hsv(rgb):
    mx, mn = rgb.max(-1), rgb.min(-1)
    d = mx - mn
    s = np.where(mx > 1e-5, d / np.maximum(mx, 1e-5), 0)
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    dd = np.maximum(d, 1e-5)
    h = np.where(mx == r, ((g - b) / dd) % 6, np.where(mx == g, (b - r) / dd + 2, (r - g) / dd + 4)) / 6.0
    return np.where(d > 1e-5, h, 0), s, mx


def from_hsv(h, s, v):
    i = np.floor(h * 6).astype(int) % 6
    f = h * 6 - np.floor(h * 6)
    p, q, t = v * (1 - s), v * (1 - f * s), v * (1 - (1 - f) * s)
    return np.stack([np.choose(i, [v, q, p, p, t, v]), np.choose(i, [t, v, v, q, p, p]), np.choose(i, [p, p, t, v, v, q])], -1)


# ---- atlas
bc = load("BaseColor")
op = load("Opacity")[..., 0]
h, s, v = hsv(bc[..., :3])
green = (h > 0.1) & (h < 0.45)
red = ((h < 0.06) | (h > 0.9)) & (s > 0.2)
h2 = np.where(green, 0.47 + (h - 0.27) * 0.15, np.where(red, 0.80, h))
s2 = np.where(green | red, np.clip(s * 1.05, 0, 1), s)
bc[..., :3] = from_hsv(h2 % 1.0, s2, v)
bc[..., 3] = op
save(bc, "T_Fab_VerdancePlants_BC", alpha=True)
n = load("Normal")
hm = load("Displacement")[..., 0]
dv = np.zeros_like(hm)
dv[1:-1] = hm[2:] - hm[:-2]
sel = op[8:-8:2, 8:-8:2] > 0.5
c = float(np.corrcoef((n[8:-8:2, 8:-8:2, 1] - 0.5)[sel], dv[8:-8:2, 8:-8:2][sel])[0, 1])
if c < 0:
    n[..., 1] = 1.0 - n[..., 1]
save(n, "T_Fab_VerdancePlants_N")
rep["normal"] = "%s (%.3f)" % ("OpenGL, flipped" if c < 0 else "DirectX, kept", c)
rep["green_share"] = round(float((green & (op > 0.5)).sum() / max((op > 0.5).sum(), 1)), 3)

# ---- islands in a 256 mask (dilated so a fern's fronds stay one island)
M = 256
small = op.reshape(M, N // M, M, N // M).max(axis=(1, 3)) > 0.4
grow = small.copy()
for _ in range(3):
    g2 = grow.copy()
    g2[1:] |= grow[:-1]
    g2[:-1] |= grow[1:]
    g2[:, 1:] |= grow[:, :-1]
    g2[:, :-1] |= grow[:, 1:]
    grow = g2
seen = np.zeros_like(grow)
islands = []
for y0 in range(M):
    for x0 in range(M):
        if grow[y0, x0] and not seen[y0, x0]:
            q = deque([(y0, x0)])
            seen[y0, x0] = True
            pts = []
            while q:
                y, x = q.popleft()
                pts.append((y, x))
                for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    yy, xx = y + dy, x + dx
                    if 0 <= yy < M and 0 <= xx < M and grow[yy, xx] and not seen[yy, xx]:
                        seen[yy, xx] = True
                        q.append((yy, xx))
            if len(pts) > 150:
                ys = [p[0] for p in pts]
                xs = [p[1] for p in pts]
                bx = (min(xs), min(ys), max(xs) + 1, max(ys) + 1)
                fill = float(small[bx[1]:bx[3], bx[0]:bx[2]].mean())
                islands.append({"bbox": bx, "w": bx[2] - bx[0], "h": bx[3] - bx[1], "fill": round(fill, 2)})
rep["detected_islands"] = len(islands)
# The dilation above merges neighbouring leaves (the monstera swallowed a long leaf), so the cards use a hand-measured
# table of the nine upright leaves instead (pixel boxes on a 768 px view of the mask, top-down; stems at the bottom).
# The two leaves lying horizontally along the top are not used.
LEAVES = {  # name: (x0, y0, x1, y1) top-down on 768, kind
    "fern_a": ((37, 85, 242, 385), "fern"), "fern_b": ((260, 130, 410, 330), "fern"),
    "oak": ((437, 152, 592, 312), "broad"), "monstera": ((17, 428, 267, 700), "broad"), "split": ((587, 510, 732, 695), "broad"),
    "long_right": ((627, 85, 694, 485), "long"), "oval_1": ((282, 360, 367, 745), "long"), "oval_2": ((397, 342, 492, 745), "long"),
    "narrow": ((517, 405, 582, 730), "long"),
}


def to_isl(box):
    x0, y0, x1, y1 = box
    b = (x0 / 768 * M, (1 - y1 / 768) * M, x1 / 768 * M, (1 - y0 / 768) * M)
    return {"bbox": b, "w": b[2] - b[0], "h": b[3] - b[1]}


ferns = [to_isl(b) for b, k in LEAVES.values() if k == "fern"]
long_ = [to_isl(b) for b, k in LEAVES.values() if k == "long"]
broad = [to_isl(LEAVES["monstera"][0]), to_isl(LEAVES["split"][0]), to_isl(LEAVES["oak"][0])]
rep["islands"] = {"fern": len(ferns), "long": len(long_), "broad": len(broad)}


# ---- card plants
def card(verts, faces, uvs, isl, height, yaw, tilt, bend, offset=0.0):
    x0, y0, x1, y1 = [c / M for c in isl["bbox"]]
    width = height * (isl["w"] / isl["h"])
    base = len(verts)
    rows = ((0.0, tilt), (0.5, tilt + bend * 0.5), (1.0, tilt + bend))
    p = np.array([math.cos(yaw) * offset, math.sin(yaw) * offset, 0.0])
    fwd = np.array([math.cos(yaw), math.sin(yaw), 0.0])
    side = np.array([-math.sin(yaw), math.cos(yaw), 0.0])
    prev = (0.0, p.copy())
    pos = [p.copy()]
    for t, a in rows[1:]:
        seg = height * 0.5
        d = fwd * math.sin(a) + np.array([0, 0, 1.0]) * math.cos(a)
        pos.append(pos[-1] + d * seg)
    for (t, a), c in zip(rows, pos):
        for sgn, u in ((-1, x0), (1, x1)):
            verts.append(tuple(c + side * sgn * width * 0.5))
            uvs.append((u, y0 + (y1 - y0) * t))
    for r in range(2):
        a0 = base + r * 2
        faces.append((a0, a0 + 1, a0 + 3, a0 + 2))


def build(name, pick, count, height, tilt, bend, offset, seed):
    rnd = np.random.default_rng(seed)
    verts, faces, uvs = [], [], []
    for k in range(count):
        isl = pick[k % len(pick)]
        hh = height * (0.8 + 0.4 * rnd.random())
        card(verts, faces, uvs, isl, hh, 2 * math.pi * k / count + rnd.uniform(-0.3, 0.3),
             math.radians(tilt + rnd.uniform(-8, 8)), math.radians(bend), offset * rnd.random())
    me = bpy.data.meshes.new(name)
    me.from_pydata(verts, [], faces)
    uv = me.uv_layers.new(name="UVMap")
    for poly in me.polygons:
        for li, vi in zip(poly.loop_indices, poly.vertices):
            uv.data[li].uv = uvs[vi]
    me.update()
    o = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(o)
    me.materials.append(mat)
    me.shade_smooth()
    me.calc_loop_triangles()
    co = np.array([vv.co for vv in me.vertices])
    rep[name] = {"cards": count, "tris": len(me.loop_triangles), "size_cm": [round(float(x) * 100) for x in co.max(0) - co.min(0)]}


for o in list(bpy.data.objects):
    bpy.data.objects.remove(o, do_unlink=True)
mat = bpy.data.materials.new("M_Fab_VerdancePlants")
nt = mat.node_tree
b = next(x for x in nt.nodes if x.type == "BSDF_PRINCIPLED")
t = nt.nodes.new("ShaderNodeTexImage")
t.image = bpy.data.images.load(os.path.join(EXP, "T_Fab_VerdancePlants_BC.png"))
nt.links.new(t.outputs["Color"], b.inputs["Base Color"])
nt.links.new(t.outputs["Alpha"], b.inputs["Alpha"])
mat.surface_render_method = "DITHERED"
if ferns:
    build("SM_Fab_VerdancePlant_Fern", ferns, 7, 0.55, 25, 25, 0.04, 1)
if broad:
    build("SM_Fab_VerdancePlant_Broad", broad, 6, 0.6, 35, 20, 0.05, 2)
if long_:
    build("SM_Fab_VerdancePlant_Long", long_, 8, 0.85, 15, 20, 0.06, 3)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "SM_Fab_VerdancePlants.blend"), compress=True, relative_remap=True)
json.dump(rep, open(os.path.join(OUT, "prep_report.json"), "w"), indent=1)
print("__RESULT__" + json.dumps(rep))

# SPDX-License-Identifier: MIT
# Triad Drift - Quixel Megascans 3D assets (glTF "ue_high" tier 1 + 4K B/N/ORM/H) -> non-walkable dressing meshes, headless:
#   blender --background --factory-startup --python prep_fab_quixel_mesh.py -- <inner folder> <out dir> <Name> <mode> <tris>
#   Name: CinderRock (Icelandic Sandy Volcanic Rock, slnv6) or VerdanceStump (Mossy Pine Stump, rhiuf)
#   mode: basalt (desaturate / cool, mean brightness matched to T_Fab_TundraVolcanic_BC) or teal (moss greens toward teal)
# Stands the mesh up (the files carry the height on Y; the larger, upward-facing side goes to +Z), puts the origin on the
# floor centre, decimates to <tris>, writes T_Fab_<Name>_BC (2048, seller colour kept as _BC_orig.jpg), _N and _ORM
# (1024; the normal's convention measured against the height map), one material M_Fab_<Name>, and saves SM_Fab_<Name>.blend
# for export_for_unreal.py (one convex hull).
import json
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix

SRC, OUT, NAME, MODE, TRIS = sys.argv[sys.argv.index("--") + 1:][:5]
TRIS = int(TRIS)
EXP = os.path.join(OUT, "export")
os.makedirs(EXP, exist_ok=True)
LUMA = np.array([0.2126, 0.7152, 0.0722])
REF = os.path.join(os.path.dirname(os.path.dirname(OUT)), "Fab", "Textures", "TundraVolcanic", "export", "T_Fab_TundraVolcanic_BC.png")
rep = {"name": NAME}

for o in list(bpy.data.objects):
    bpy.data.objects.remove(o, do_unlink=True)
gltf = [f for f in os.listdir(SRC) if f.endswith(".gltf")][0]
bpy.ops.import_scene.gltf(filepath=os.path.join(SRC, gltf))
meshes = [o for o in bpy.data.objects if o.type == "MESH"]
for o in list(bpy.data.objects):
    if o.type != "MESH":
        bpy.data.objects.remove(o, do_unlink=True)
for o in meshes:
    o.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
if len(meshes) > 1:
    bpy.ops.object.join()
obj = bpy.context.view_layer.objects.active
for o in meshes:
    if o.name in bpy.data.objects and o.parent:
        o.parent = None
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
me = obj.data
me.calc_loop_triangles()
rep["source_tris"] = len(me.loop_triangles)
# stand up: rotate the thinnest axis to Z, sign chosen so the area-weighted normal points up
dims = np.array(obj.dimensions)
thin = int(np.argmin(dims))
area_n = np.zeros(3)
for p in me.polygons:
    area_n += np.array(p.normal) * p.area
rot = Matrix.Identity(4)
if thin == 1:
    rot = Matrix.Rotation(math.radians(90 if area_n[1] > 0 else -90), 4, "X")
elif thin == 0:
    rot = Matrix.Rotation(math.radians(-90 if area_n[0] > 0 else 90), 4, "Y")
elif area_n[2] < 0:
    rot = Matrix.Rotation(math.radians(180), 4, "X")
me.transform(rot)
me.update()
co = np.empty(len(me.vertices) * 3)
me.vertices.foreach_get("co", co)
co = co.reshape(-1, 3)
lo, hi = co.min(0), co.max(0)
co -= np.array([(lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, lo[2]])
me.vertices.foreach_set("co", co.ravel())
me.update()
rep["size_cm"] = [round(float(v) * 100, 1) for v in hi - lo]
rep["stood_up_from_axis"] = "XYZ"[thin]
if rep["source_tris"] > TRIS:
    mod = obj.modifiers.new("Decimate", "DECIMATE")
    mod.ratio = TRIS / rep["source_tris"]
    mod.use_collapse_triangulate = True
    with bpy.context.temp_override(object=obj, active_object=obj):
        bpy.ops.object.modifier_apply(modifier=mod.name)
me.calc_loop_triangles()
rep["tris"] = len(me.loop_triangles)
obj.name = me.name = "SM_Fab_" + NAME

tex = os.path.join(SRC, "Textures")
key = [f for f in os.listdir(tex) if f.endswith("_B.png")][0][:-len("_B.png")]


def load(path, size):
    im = bpy.data.images.load(path)
    im.colorspace_settings.name = "Non-Color"
    if tuple(im.size) != (size, size):
        im.scale(size, size)
    x = np.empty(size * size * 4, np.float32)
    im.pixels.foreach_get(x)
    bpy.data.images.remove(im)
    return x.reshape(size, size, 4)


def save(arr, name, fmt="PNG"):
    s = arr.shape[0]
    im = bpy.data.images.new(name, s, s)
    im.colorspace_settings.name = "Non-Color"
    o = arr.copy()
    o[..., 3] = 1.0
    im.pixels.foreach_set(np.clip(o, 0, 1).ravel())
    im.filepath_raw = os.path.join(EXP, name)
    im.file_format = fmt
    im.save()
    bpy.data.images.remove(im)
    return os.path.join(EXP, name)


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


bc = load(os.path.join(tex, key + "_B.png"), 2048)
save(bc, "T_Fab_%s_BC_orig.jpg" % NAME, "JPEG")
lum = bc[..., :3] @ LUMA
rep["source_mean_luma"] = round(float(lum.mean()), 3)
if MODE == "basalt":
    ref = load(REF, 2048)
    target = float((ref[..., :3] @ LUMA).mean())
    rgb = (bc[..., :3] * 0.4 + lum[..., None] * 0.6) * np.array([0.95, 0.97, 1.03])
    k = target / max(float((rgb @ LUMA).mean()), 1e-4)
    bc[..., :3] = rgb * k
    rep["basalt"] = {"target_mean_luma": round(target, 3), "scale": round(k, 3)}
else:
    h, s, v = hsv(bc[..., :3])
    moss = ((h > 0.13) & (h < 0.42) & (s > 0.15)).astype(np.float32)
    # moss to Verdance teal-green (hue ~0.46), a little less saturated (a first pass at +20% read as lurid lime)
    h2 = np.where(moss > 0, 0.46 + (np.clip(h, 0.13, 0.42) - 0.27) * 0.1, h)
    bc[..., :3] = from_hsv(h2, np.where(moss > 0, s * 0.85, s), v)
    rep["moss_share"] = round(float(moss.mean()), 3)
bc_path = save(bc, "T_Fab_%s_BC.png" % NAME)
n = load(os.path.join(tex, key + "_N.png"), 1024)
hm = load(os.path.join(tex, key + "_H.png"), 1024)[..., 0]
dv = np.zeros_like(hm)
dv[1:-1] = hm[2:] - hm[:-2]
c = float(np.corrcoef((n[8:-8:2, 8:-8:2, 1] - 0.5).ravel(), dv[8:-8:2, 8:-8:2].ravel())[0, 1])
preview = n.copy()
if c < 0:
    n[..., 1] = 1.0 - n[..., 1]
else:
    preview[..., 1] = 1.0 - preview[..., 1]
rep["normal"] = "%s (%.3f)" % ("OpenGL, flipped" if c < 0 else "DirectX, kept", c)
save(n, "T_Fab_%s_N.png" % NAME)
orm = load(os.path.join(tex, key + "_ORM.png"), 1024)
orm_path = save(orm, "T_Fab_%s_ORM.png" % NAME)
rep["orm_mean"] = [round(float(orm[..., i].mean()), 3) for i in range(3)]

# one Blender material on the exported maps (review renders)
mat = bpy.data.materials.new("M_Fab_" + NAME)
nt = mat.node_tree
b = next(x for x in nt.nodes if x.type == "BSDF_PRINCIPLED")


def tnode(img):
    t = nt.nodes.new("ShaderNodeTexImage")
    t.image = img
    return t


nt.links.new(tnode(bpy.data.images.load(bc_path)).outputs["Color"], b.inputs["Base Color"])
oimg = bpy.data.images.load(orm_path)
oimg.colorspace_settings.name = "Non-Color"
sep = nt.nodes.new("ShaderNodeSeparateColor")
nt.links.new(tnode(oimg).outputs["Color"], sep.inputs[0])
nt.links.new(sep.outputs["Green"], b.inputs["Roughness"])
nt.links.new(sep.outputs["Blue"], b.inputs["Metallic"])
pimg = bpy.data.images.new("prevN", 1024, 1024)
pimg.colorspace_settings.name = "Non-Color"
pimg.pixels.foreach_set(preview.ravel())
pimg.pack()
nm = nt.nodes.new("ShaderNodeNormalMap")
nt.links.new(tnode(pimg).outputs["Color"], nm.inputs["Color"])
nt.links.new(nm.outputs["Normal"], b.inputs["Normal"])
me.materials.clear()
me.materials.append(mat)
for m in list(bpy.data.materials):
    if m.users == 0:
        bpy.data.materials.remove(m)
for im in list(bpy.data.images):
    if im.users == 0:
        bpy.data.images.remove(im)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "SM_Fab_%s.blend" % NAME), compress=True, relative_remap=True)
json.dump(rep, open(os.path.join(OUT, "prep_report.json"), "w"), indent=1)
print("__RESULT__" + json.dumps(rep))

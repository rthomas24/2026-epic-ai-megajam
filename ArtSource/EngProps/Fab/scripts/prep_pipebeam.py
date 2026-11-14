# SPDX-License-Identifier: MIT
# Triad Drift - Fab "Sci-Fi Kit Free 08 Tubes Cables" (REX Hsu, SciFi_Prop_PipeBeam) -> SM_Fab_PipeBeam, headless:
#   blender --background --factory-startup --disable-autoexec <Sci-Fi_08_Free.blend> --python prep_pipebeam.py -- \
#       --src <folder with the seller PNGs> --out <repo>/ArtSource/EngProps/Fab/PipeBeam
# One mesh, two slots: M_Fab_PipeBeam_Truss (TU_03_A_01 maps) and M_Fab_PipeBeam_Cable (cable_02 maps). Origin on the
# floor centre, front kept, extra UV channels dropped. Textures at 2048: the seller's colours as *_orig.png, and the
# approved Null recolour (Lead, Nov 14): orange paint -> black armour keeping its wear, orange light strips and the
# emissive -> Null red, the grey service box darkened to gunmetal. SKY_002.exr is never touched.
import json
import math
import os
import sys

import bpy
import numpy as np

a = sys.argv[sys.argv.index("--") + 1:]
SRC = a[a.index("--src") + 1]
OUT = a[a.index("--out") + 1]
EXP = os.path.join(OUT, "export")
os.makedirs(EXP, exist_ok=True)
SIZE = 2048
NAME = "SM_Fab_PipeBeam"
RED = np.array([0.85, 0.05, 0.03])
ARMOUR = np.array([0.05, 0.048, 0.052])
rep = {"notes": []}

meshes = [o for o in bpy.data.objects if o.type == "MESH"]
rep["source_tris"] = 0
for o in meshes:
    o.data.calc_loop_triangles()
    rep["source_tris"] += len(o.data.loop_triangles)
for o in list(bpy.data.objects):
    if o.type != "MESH":
        bpy.data.objects.remove(o, do_unlink=True)
for o in meshes:
    while len(o.data.uv_layers) > 1:
        o.data.uv_layers.remove(o.data.uv_layers[-1])
    o.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
bpy.ops.object.join()
obj = bpy.context.view_layer.objects.active
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
co = np.empty(len(obj.data.vertices) * 3)
obj.data.vertices.foreach_get("co", co)
co = co.reshape(-1, 3)
lo, hi = co.min(0), co.max(0)
co -= np.array([(lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, lo[2]])
obj.data.vertices.foreach_set("co", co.ravel())
obj.data.update()
obj.name = obj.data.name = NAME
rep["size_cm"] = [round(float(v) * 100, 1) for v in hi - lo]
obj.data.calc_loop_triangles()
rep["tris"] = len(obj.data.loop_triangles)


def load(name):
    im = bpy.data.images.load(os.path.join(SRC, name))
    im.colorspace_settings.name = "Non-Color"
    im.scale(SIZE, SIZE)
    x = np.empty(SIZE * SIZE * 4, np.float32)
    im.pixels.foreach_get(x)
    bpy.data.images.remove(im)
    return x.reshape(SIZE, SIZE, 4)


def save(arr, name, srgb):
    im = bpy.data.images.new(name, SIZE, SIZE)
    im.colorspace_settings.name = "sRGB" if srgb else "Non-Color"
    o = arr.copy()
    o[..., 3] = 1.0
    im.pixels.foreach_set(np.clip(o, 0, 1).ravel())
    im.filepath_raw = os.path.join(EXP, name + ".png")
    im.file_format = "PNG"
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


# normal convention from the truss height map (rows of Blender's pixel array run bottom-to-top)
n = load("TU_03_A_01_Normal.png")
h = load("TU_03_A_01_Height.png")[..., 0]
dv = np.zeros_like(h)
dv[1:-1] = h[2:] - h[:-2]
corr = float(np.corrcoef((n[8:-8:2, 8:-8:2, 1] - 0.5).ravel(), dv[8:-8:2, 8:-8:2].ravel())[0, 1])
opengl = corr < 0
rep["normal"] = "%s (green vs height slope %.3f)%s" % ("OpenGL" if opengl else "DirectX", corr, ", flipped to DirectX" if opengl else "")
imgs = {}
for part, prefix, has_e in (("Truss", "TU_03_A_01", True), ("Cable", "cable_02", False)):
    bc = load(prefix + "_BaseColor.png")
    save(bc, "T_Fab_PipeBeam_%s_BC_orig" % part, True)
    nn = load(prefix + "_Normal.png")
    prev = nn.copy()
    if opengl:
        nn[..., 1] = 1.0 - nn[..., 1]
    else:
        prev[..., 1] = 1.0 - prev[..., 1]
    save(nn, "T_Fab_PipeBeam_%s_N" % part, False)
    orm = np.ones((SIZE, SIZE, 4), np.float32)
    orm[..., 1] = load(prefix + "_Roughness.png")[..., 0]
    orm[..., 2] = load(prefix + "_Metallic.png")[..., 0]
    save(orm, "T_Fab_PipeBeam_%s_ORM" % part, False)
    rgb = bc[..., :3]
    hh, s, v = hsv(rgb)
    lum = rgb @ np.array([0.2126, 0.7152, 0.0722])
    out = rgb.copy()
    if part == "Truss":
        e = load(prefix + "_Emissive.png")
        save(e, "T_Fab_PipeBeam_Truss_E_orig", True)
        glow = e[..., :3].max(-1)
        lamp = glow > 0.2
        paint = (hh > 0.04) & (hh < 0.15) & (s > 0.35) & ~lamp
        detail = np.clip(lum / max(np.percentile(lum[paint], 90), 1e-3), 0, 1.2)
        out[paint] = (ARMOUR[None] * (0.6 + 0.8 * detail[paint][:, None]))
        grey = (s < 0.2) & (v > 0.3)
        out[grey] = rgb[grey] * 0.45                                       # service box: gunmetal
        out[lamp] = RED * 0.9
        reddish = (hh < 0.04) | (hh > 0.95)
        stripe = reddish & (s > 0.35) & ~paint
        out[stripe] = RED[None] * (0.4 + 0.5 * v[stripe][:, None])
        en = np.zeros_like(e)
        en[..., :3] = RED[None, None] * glow[..., None]
        imgs["Truss_E"] = save(en, "T_Fab_PipeBeam_Truss_E", True)
        rep["truss_paint_share"] = round(float(paint.mean()), 3)
    else:
        out = rgb * 0.55                                                   # cables: darker, still readable
    bc[..., :3] = out
    imgs[part + "_BC"] = save(bc, "T_Fab_PipeBeam_%s_BC" % part, True)
    imgs[part + "_ORM"] = bpy.data.images.load(os.path.join(EXP, "T_Fab_PipeBeam_%s_ORM.png" % part))
    p = bpy.data.images.new("prevN_" + part, SIZE, SIZE)
    p.colorspace_settings.name = "Non-Color"
    p.pixels.foreach_set(prev.ravel())
    p.pack()
    imgs[part + "_N"] = p

# materials: rename the seller's two slots and rebuild them on the exported maps (Blender preview)
for slot in obj.material_slots:
    m = slot.material
    part = "Cable" if "cable" in m.name.lower() else "Truss"
    nm = bpy.data.materials.new("M_Fab_PipeBeam_" + part)
    nt = nm.node_tree
    b = next(x for x in nt.nodes if x.type == "BSDF_PRINCIPLED")

    def tex(img):
        t = nt.nodes.new("ShaderNodeTexImage")
        t.image = img
        return t
    nt.links.new(tex(imgs[part + "_BC"]).outputs["Color"], b.inputs["Base Color"])
    sep = nt.nodes.new("ShaderNodeSeparateColor")
    nt.links.new(tex(imgs[part + "_ORM"]).outputs["Color"], sep.inputs[0])
    nt.links.new(sep.outputs["Green"], b.inputs["Roughness"])
    nt.links.new(sep.outputs["Blue"], b.inputs["Metallic"])
    nmap = nt.nodes.new("ShaderNodeNormalMap")
    nt.links.new(tex(imgs[part + "_N"]).outputs["Color"], nmap.inputs["Color"])
    nt.links.new(nmap.outputs["Normal"], b.inputs["Normal"])
    if part == "Truss":
        nt.links.new(tex(imgs["Truss_E"]).outputs["Color"], b.inputs["Emission Color"])
        b.inputs["Emission Strength"].default_value = 4.0
    slot.material = nm
for m in list(bpy.data.materials):
    if m.users == 0:
        bpy.data.materials.remove(m)
for im in list(bpy.data.images):
    if im.users == 0:
        bpy.data.images.remove(im)
if not obj.data.has_custom_normals:
    obj.data.shade_smooth()
    obj.data.set_sharp_from_angle(angle=math.radians(35))
rep["slots"] = [s.material.name for s in obj.material_slots]
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, NAME + ".blend"), compress=True, relative_remap=True)
json.dump(rep, open(os.path.join(OUT, "prep_report.json"), "w"), indent=1)
print("__RESULT__" + json.dumps(rep))

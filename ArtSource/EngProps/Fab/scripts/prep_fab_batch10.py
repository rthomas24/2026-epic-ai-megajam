# SPDX-License-Identifier: MIT
# Triad Drift - Quixel priority batch (#207, Fab Standard), headless, one asset group per run:
#   blender --background --factory-startup --python prep_fab_batch10.py -- <kind> <Name> <inner dir> <out dir> <json options>
# kind "scan":    glTF scan pieces -> SM_Fab_<Name>_<A,B..> sharing T_Fab_<Name>_BC (1024) / _N (512) / _ORM (512);
#                 options: pieces (tri counts to keep), tris (decimate target, 0 = keep), mode (basalt | teal), collision
# kind "foliage": Megascans foliage FBX variants (LOD0) -> SM_Fab_<Name>_<A,B..> sharing T_Fab_<Name>_BC (1024, RGBA with the
#                 opacity in alpha; greens to Verdance teal) / _N (512), for M_TD_FabFoliageCard; options: variants, prefix
# kind "decal":   an opacity map -> T_Fab_<Name>_Mask (512, grayscale) and a preview on a dark Dominion metal swatch
# Each mesh: origin on the floor centre, one material M_Fab_<Name>; export_for_unreal.py writes the FBX afterwards.
import json
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix

KIND, NAME, SRC, OUT, OPT = sys.argv[sys.argv.index("--") + 1:][:5]
OPT = json.loads(OPT)
EXP = os.path.join(OUT, "export")
REV = os.path.join(OUT, "review")
os.makedirs(EXP, exist_ok=True)
os.makedirs(REV, exist_ok=True)
LUMA = np.array([0.2126, 0.7152, 0.0722])
FAB = os.path.dirname(OUT)
rep = {"name": NAME, "kind": KIND}


def load(path, size):
    im = bpy.data.images.load(path)
    im.colorspace_settings.name = "Non-Color"
    if tuple(im.size) != (size, size):
        im.scale(size, size)
    x = np.empty(size * size * 4, np.float32)
    im.pixels.foreach_get(x)
    bpy.data.images.remove(im)
    return x.reshape(size, size, 4)


def save(arr, name, alpha=False):
    s = arr.shape[0]
    im = bpy.data.images.new(name, s, s, alpha=alpha)
    im.colorspace_settings.name = "Non-Color"
    o = arr.copy()
    if not alpha:
        o[..., 3] = 1.0
    im.pixels.foreach_set(np.clip(o, 0, 1).ravel())
    im.filepath_raw = os.path.join(EXP, name + ".png")
    im.file_format = "PNG"
    if alpha:
        im.alpha_mode = "STRAIGHT"
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


def teal(rgb, strength=1.0):
    h, s, v = hsv(rgb)
    g = (h > 0.1) & (h < 0.45) & (s > 0.12)
    h2 = np.where(g, 0.46 + (np.clip(h, 0.1, 0.45) - 0.27) * 0.1, h)
    return from_hsv(h2, np.where(g, s * 0.9, s), v), float(g.mean())


def basalt(rgb):
    ref = load(os.path.join(FAB, "Textures", "TundraVolcanic", "export", "T_Fab_TundraVolcanic_BC.png"), 1024)
    target = float((ref[..., :3] @ LUMA).mean())
    lum = rgb @ LUMA
    out = (rgb * 0.4 + lum[..., None] * 0.6) * np.array([0.95, 0.97, 1.03])
    k = target / max(float((out @ LUMA).mean()), 1e-4)
    return out * k, round(k, 3)


def normal_fix(n, h):
    dv = np.zeros_like(h)
    dv[1:-1] = h[2:] - h[:-2]
    c = float(np.corrcoef((n[8:-8:2, 8:-8:2, 1] - 0.5).ravel(), dv[8:-8:2, 8:-8:2].ravel())[0, 1])
    if c < 0:
        n[..., 1] = 1.0 - n[..., 1]
    return n, "%s (%.3f)" % ("OpenGL, flipped" if c < 0 else "DirectX, kept", c)


def floor_centre(obj):
    me = obj.data
    co = np.empty(len(me.vertices) * 3)
    me.vertices.foreach_get("co", co)
    co = co.reshape(-1, 3)
    lo, hi = co.min(0), co.max(0)
    co -= np.array([(lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, lo[2]])
    me.vertices.foreach_set("co", co.ravel())
    me.update()
    obj.location = (0, 0, 0)
    return [round(float(v) * 100, 1) for v in hi - lo]


def stand_up(obj):
    me = obj.data
    dims = np.array(obj.dimensions)
    thin = int(np.argmin(dims))
    an = np.zeros(3)
    for p in me.polygons:
        an += np.array(p.normal) * p.area
    rot = Matrix.Identity(4)
    if thin == 1:
        rot = Matrix.Rotation(math.radians(90 if an[1] > 0 else -90), 4, "X")
    elif thin == 0:
        rot = Matrix.Rotation(math.radians(-90 if an[0] > 0 else 90), 4, "Y")
    elif an[2] < 0:
        rot = Matrix.Rotation(math.radians(180), 4, "X")
    me.transform(rot)
    me.update()


def decimate(obj, target):
    obj.data.calc_loop_triangles()
    n = len(obj.data.loop_triangles)
    if target and n > target:
        m = obj.modifiers.new("Decimate", "DECIMATE")
        m.ratio = target / n
        m.use_collapse_triangulate = True
        with bpy.context.temp_override(object=obj, active_object=obj):
            bpy.ops.object.modifier_apply(modifier=m.name)
    obj.data.calc_loop_triangles()
    return n, len(obj.data.loop_triangles)


def material(bc_img, n_img, orm_img=None, alpha=False):
    mat = bpy.data.materials.new("M_Fab_" + NAME)
    nt = mat.node_tree
    b = next(x for x in nt.nodes if x.type == "BSDF_PRINCIPLED")
    t = nt.nodes.new("ShaderNodeTexImage")
    t.image = bc_img
    nt.links.new(t.outputs["Color"], b.inputs["Base Color"])
    if alpha:
        nt.links.new(t.outputs["Alpha"], b.inputs["Alpha"])
        mat.surface_render_method = "DITHERED"
    nm = nt.nodes.new("ShaderNodeNormalMap")
    tn = nt.nodes.new("ShaderNodeTexImage")
    prev = bpy.data.images.new("prevN", n_img.shape[1], n_img.shape[0])
    prev.colorspace_settings.name = "Non-Color"
    p = n_img.copy()
    p[..., 1] = 1.0 - p[..., 1]                       # the exported map is DirectX; Blender previews OpenGL
    prev.pixels.foreach_set(p.ravel())
    prev.pack()
    tn.image = prev
    nt.links.new(tn.outputs["Color"], nm.inputs["Color"])
    nt.links.new(nm.outputs["Normal"], b.inputs["Normal"])
    if orm_img is not None:
        to = nt.nodes.new("ShaderNodeTexImage")
        to.image = orm_img
        sep = nt.nodes.new("ShaderNodeSeparateColor")
        nt.links.new(to.outputs["Color"], sep.inputs[0])
        nt.links.new(sep.outputs["Green"], b.inputs["Roughness"])
        nt.links.new(sep.outputs["Blue"], b.inputs["Metallic"])
    return mat


for o in list(bpy.data.objects):
    bpy.data.objects.remove(o, do_unlink=True)
letters = "ABCDEFG"
pieces_out = []
if KIND == "scan":
    gltf = [f for f in os.listdir(SRC) if f.endswith(".gltf")][0]
    bpy.ops.import_scene.gltf(filepath=os.path.join(SRC, gltf))
    meshes = [o for o in bpy.data.objects if o.type == "MESH"]
    for o in list(bpy.data.objects):
        if o.type != "MESH":
            bpy.data.objects.remove(o, do_unlink=True)
    for o in meshes:
        o.parent = None
    tex = os.path.join(SRC, "Textures")
    key = [f for f in os.listdir(tex) if f.endswith("_B.png")][0][:-len("_B.png")]
    bc = load(os.path.join(tex, key + "_B.png"), 1024)
    if OPT["mode"] == "basalt":
        bc[..., :3], k = basalt(bc[..., :3])
        bc[..., :3] *= OPT.get("dim", 1.0)              # the cliff's pale dolerite still read light grey at the matched mean
        rep["basalt_scale"] = k
        rep["dim"] = OPT.get("dim", 1.0)
    else:
        bc[..., :3], share = teal(bc[..., :3])
        rep["moss_share"] = round(share, 3)
    bc_img = save(bc, "T_Fab_%s_BC" % NAME)
    hpath = os.path.join(tex, key + "_H.png")
    n = load(os.path.join(tex, key + "_N.png"), 512)
    if os.path.isfile(hpath):
        n, rep["normal"] = normal_fix(n, load(hpath, 512)[..., 0])
    else:
        n[..., 1] = 1.0 - n[..., 1]                    # Megascans glTF normals measured OpenGL on every earlier set
        rep["normal"] = "no height map: flipped as OpenGL like the other Megascans glTF sets"
    save(n, "T_Fab_%s_N" % NAME)
    orm = load(os.path.join(tex, key + "_ORM.png"), 512)
    orm_img = save(orm, "T_Fab_%s_ORM" % NAME)
    mat = material(bc_img, n, orm_img)
    tris = [(o, None) for o in meshes]
    for o in meshes:
        o.data.calc_loop_triangles()
    keep = OPT.get("pieces")
    chosen = []
    for want in keep:
        best = min((o for o in meshes if o not in chosen), key=lambda o: abs(len(o.data.loop_triangles) - want))
        chosen.append(best)
    for o in meshes:
        if o not in chosen:
            bpy.data.objects.remove(o, do_unlink=True)
    for i, o in enumerate(chosen):
        bpy.context.view_layer.objects.active = o
        for x in bpy.context.view_layer.objects:
            x.select_set(x == o)
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        # no stand_up here: Blender's glTF importer already turns these files Z-up (the thinnest-axis rule laid the
        # cliff wall flat and turned the log over)
        src_t, new_t = decimate(o, OPT.get("tris", 0))
        size = floor_centre(o)
        o.name = o.data.name = "SM_Fab_%s_%s" % (NAME, letters[i])
        o.data.materials.clear()
        o.data.materials.append(mat)
        pieces_out.append({"mesh": o.name, "source_tris": src_t, "tris": new_t, "size_cm": size})
elif KIND == "foliage":
    pre = OPT["prefix"]
    bc = load(os.path.join(SRC, pre + "BaseColor.jpg"), 1024)
    op = load(os.path.join(SRC, pre + "Opacity.jpg"), 1024)[..., 0]
    bc[..., :3], share = teal(bc[..., :3])
    bc[..., 3] = op
    bc_img = save(bc, "T_Fab_%s_BC" % NAME, alpha=True)
    n = load(os.path.join(SRC, pre + "Normal.jpg"), 512)
    n, rep["normal"] = normal_fix(n, load(os.path.join(SRC, pre + "Displacement.jpg"), 512)[..., 0])
    save(n, "T_Fab_%s_N" % NAME)
    rep["green_share"] = round(share, 3)
    mat = material(bc_img, n, None, alpha=True)
    for i, v in enumerate(OPT["variants"]):
        before = set(bpy.data.objects)
        f = [x for x in os.listdir(SRC) if x.endswith("_Var%s_LOD0.fbx" % v)][0]
        bpy.ops.import_scene.fbx(filepath=os.path.join(SRC, f))
        new = [o for o in bpy.data.objects if o not in before]
        ms = [o for o in new if o.type == "MESH"]
        for o in new:
            if o.type != "MESH":
                bpy.data.objects.remove(o, do_unlink=True)
        o = ms[0]
        o.parent = None
        for x in bpy.context.view_layer.objects:
            x.select_set(x == o)
        bpy.context.view_layer.objects.active = o
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        o.data.calc_loop_triangles()
        t = len(o.data.loop_triangles)
        size = floor_centre(o)
        o.name = o.data.name = "SM_Fab_%s_%s" % (NAME, letters[i])
        o.data.materials.clear()
        o.data.materials.append(mat)
        pieces_out.append({"mesh": o.name, "variant": v, "tris": t, "size_cm": size})
elif KIND == "decal":
    m = load(os.path.join(SRC, OPT["opacity"]), 512)[..., 0]
    floor = float(np.percentile(m, 5))
    m = np.clip((m - floor) / max(1 - floor, 1e-3), 0, 1)
    out = np.zeros((512, 512, 4), np.float32)
    out[..., 0] = out[..., 1] = out[..., 2] = m
    save(out, "T_Fab_%s_Mask" % NAME)
    look = OPT["look"]
    metal = np.full((512, 512, 3), 0.018)              # Null black armour (linear)
    metal[200:230, :] = (0.25, 0.01, 0.01)               # a crimson accent band
    a = np.clip((m - look["Bias"]) * look["Contrast"], 0, 1)[..., None] * look["Opacity"]
    res = metal * (1 - a) + np.array(look["CharColor"]) * a

    def srgb(x):
        x = np.clip(x, 0, 1)
        return np.where(x <= 0.0031308, x * 12.92, 1.055 * np.power(x, 1 / 2.4) - 0.055)
    sheet = np.concatenate([srgb(metal), np.full((512, 8, 3), 0.08), srgb(res)], 1)
    s = np.ones(sheet.shape[:2] + (4,), np.float32)
    s[..., :3] = sheet
    im = bpy.data.images.new("prev", s.shape[1], s.shape[0])
    im.pixels.foreach_set(s.ravel())
    im.filepath_raw = os.path.join(REV, "Fab_%s_on_metal.png" % NAME)
    im.file_format = "PNG"
    im.save()
    rep["look"] = look
rep["meshes"] = pieces_out
for m in list(bpy.data.materials):
    if m.users == 0:
        bpy.data.materials.remove(m)
if KIND != "decal":
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "SM_Fab_%s.blend" % NAME), compress=True, relative_remap=True)
json.dump(rep, open(os.path.join(OUT, "prep_report.json"), "w"), indent=1)
print("__RESULT__" + json.dumps(rep))

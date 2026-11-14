# SPDX-License-Identifier: MIT
# Triad Drift - asset intake: turn one of Nicholas's Fab downloads into a game-ready static mesh, headless.
#
#   blender --background --factory-startup --disable-autoexec [source.blend] --python prep_fab.py -- \
#       --asset PowerCore --out <repo>/ArtSource/EngProps/Fab/PowerCore [--fbx source.fbx] [--tex-dir dir] [--max-tris 15000]
#
# The sources are untrusted third-party files: a .blend is opened with --disable-autoexec (no embedded scripts run),
# an .fbx is imported into an empty factory scene. Nothing is written next to the source.
#
# What it does:
#   1. joins every mesh into one object SM_Fab_<Asset>, applies transforms, real scale in metres (Unreal cm on export),
#      origin on the floor centre (bounding-box centre in X/Y, lowest point in Z) at the world origin, front stays -Y;
#   2. decimates (collapse) only when the mesh is over --max-tris;
#   3. writes the Unreal texture set at 2048 next to the FBX: T_Fab_<Asset>_BC (sRGB), _N (DirectX: the green channel
#      is flipped when the source map is OpenGL, as every Blender Normal Map node source is), _ORM (R occlusion,
#      G roughness, B metallic: the project's M_TD_PropTextured packing) and _E when the source has an emission map;
#   4. one material M_Fab_<Asset> (becomes the Unreal slot) that previews those maps in Blender;
#   5. saves <out>/SM_Fab_<Asset>.blend (textures referenced, not packed); export_for_unreal.py then writes the FBX
#      with UCX_ convex collision.
import json
import math
import os
import sys

import bpy
import numpy as np

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []


def arg(name, default=None):
    return argv[argv.index(name) + 1] if name in argv else default


ASSET = arg("--asset")
OUT = arg("--out")
FBX = arg("--fbx")
TEX_DIR = arg("--tex-dir")
MAX_TRIS = int(arg("--max-tris", "15000"))
SIZE = 2048
NAME = "SM_Fab_" + ASSET
EXPORT = os.path.join(OUT, "export")
os.makedirs(EXPORT, exist_ok=True)
report = {"asset": NAME, "source": FBX or bpy.data.filepath, "notes": []}

# ---------------------------------------------------------------- load
if FBX:
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    bpy.ops.import_scene.fbx(filepath=FBX)
meshes = [o for o in bpy.data.objects if o.type == "MESH" and o.name in bpy.context.view_layer.objects]
report["source_objects"] = [o.name for o in meshes]
dg = bpy.context.evaluated_depsgraph_get()


def tri_count(o):
    e = o.evaluated_get(bpy.context.evaluated_depsgraph_get())
    me = e.to_mesh()
    me.calc_loop_triangles()
    n = len(me.loop_triangles)
    e.to_mesh_clear()
    return n


report["tris_before"] = sum(tri_count(o) for o in meshes)

# source material maps, read before anything is joined
src_mat = None
for o in meshes:
    for s in o.material_slots:
        if s.material and s.material.node_tree and any(n.type == "TEX_IMAGE" for n in s.material.node_tree.nodes):
            src_mat = s.material
            break


def linked_image(socket):
    """Follow a Principled input back to its image (through Separate Color / Normal Map); return (image, channel)."""
    if not socket.is_linked:
        return None, None
    node = socket.links[0].from_node
    chan = None
    if node.type == "SEPARATE_COLOR":
        chan = {"Red": 0, "Green": 1, "Blue": 2}[socket.links[0].from_socket.name]
        node = node.inputs[0].links[0].from_node if node.inputs[0].is_linked else None
    elif node.type == "NORMAL_MAP":
        node = node.inputs["Color"].links[0].from_node if node.inputs["Color"].is_linked else None
    if node is not None and node.type == "TEX_IMAGE":
        return node.image, chan
    return None, None


maps = {}
if src_mat is not None:
    bsdf = next(n for n in src_mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    for key, sock in (("BC", "Base Color"), ("R", "Roughness"), ("M", "Metallic"), ("N", "Normal"), ("E", "Emission Color")):
        img, chan = linked_image(bsdf.inputs[sock])
        if img is not None:
            maps[key] = (img, chan)
if TEX_DIR:                                   # loose PNG set (the oil barrel)
    names = {"BC": "BaseColor", "R": "Roughness", "M": "Metallic", "N": "Normal", "AO": "AmbientOcclusion", "H": "Height"}
    for key, word in names.items():
        p = next((os.path.join(TEX_DIR, f) for f in os.listdir(TEX_DIR) if f.lower().endswith(word.lower() + ".png")), None)
        if p:
            img = bpy.data.images.load(p)
            img.colorspace_settings.name = "sRGB" if key == "BC" else "Non-Color"
            maps[key] = (img, 0 if key != "BC" and key != "N" else None)
report["maps"] = {k: "%s %dx%d ch%s" % (v[0].name, v[0].size[0], v[0].size[1], v[1]) for k, v in maps.items()}

# ---------------------------------------------------------------- mesh
for o in bpy.data.objects:
    o.select_set(False)
for o in meshes:
    o.hide_set(False)
    o.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
for o in meshes:                               # clear parents, keep the world transform
    if o.parent is not None:
        mw = o.matrix_world.copy()
        o.parent = None
        o.matrix_world = mw
if len(meshes) > 1:
    bpy.ops.object.join()
obj = bpy.context.view_layer.objects.active
for o in list(bpy.data.objects):               # cameras, lights, empties of the seller's scene
    if o != obj:
        bpy.data.objects.remove(o, do_unlink=True)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
co = np.empty(len(obj.data.vertices) * 3, dtype=np.float64)
obj.data.vertices.foreach_get("co", co)
co = co.reshape(-1, 3)
lo, hi = co.min(0), co.max(0)
pivot = np.array([(lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, lo[2]])
co -= pivot
obj.data.vertices.foreach_set("co", co.ravel())
obj.data.update()
obj.location = (0, 0, 0)
obj.name = NAME
obj.data.name = NAME
tris = tri_count(obj)
if tris > MAX_TRIS:
    mod = obj.modifiers.new("Decimate", "DECIMATE")
    mod.ratio = MAX_TRIS / tris
    mod.use_collapse_triangulate = True
    with bpy.context.temp_override(object=obj, active_object=obj):
        bpy.ops.object.modifier_apply(modifier=mod.name)
    report["notes"].append("decimated %d -> %d tris" % (tris, tri_count(obj)))
# keep the authored split normals of the source; set sharp edges only where there are none
if not any(a.name == "sharp_edge" for a in obj.data.attributes) and not obj.data.has_custom_normals:
    obj.data.shade_smooth()
    obj.data.set_sharp_from_angle(angle=math.radians(40))
report["tris_after"] = tri_count(obj)
report["size_cm"] = [round(float(v) * 100, 1) for v in (hi - lo)]

# ---------------------------------------------------------------- textures


def pixels(img):
    """float32 HxWx4 of the image at SIZE (scaled on a copy)."""
    c = img.copy()
    if c.size[0] != SIZE or c.size[1] != SIZE:
        c.scale(SIZE, SIZE)
    a = np.empty(SIZE * SIZE * 4, dtype=np.float32)
    c.pixels.foreach_get(a)
    bpy.data.images.remove(c)
    return a.reshape(SIZE, SIZE, 4)


def write(name, arr, srgb):
    img = bpy.data.images.new(name, SIZE, SIZE, alpha=False, float_buffer=False)
    img.colorspace_settings.name = "sRGB" if srgb else "Non-Color"
    a = arr.copy()
    a[..., 3] = 1.0
    img.pixels.foreach_set(a.ravel())
    img.filepath_raw = os.path.join(EXPORT, name + ".png")
    img.file_format = "PNG"
    img.save()
    return img


def normal_is_opengl(n, h):
    """Compare the normal map's green with the height map's slope along V (Blender's pixel rows run bottom-to-top,
    so row index grows with V). A normal is (-dH/du, -dH/dv, 1): OpenGL green (+V) anti-correlates with dH/dv."""
    g = n[..., 1] - 0.5
    dh = np.zeros_like(h)
    dh[1:-1] = h[2:] - h[:-2]            # rows of Blender's pixel array run bottom-to-top
    corr = float(np.corrcoef(g[8:-8:2, 8:-8:2].ravel(), dh[8:-8:2, 8:-8:2].ravel())[0, 1])
    return corr < 0, corr


out_imgs = {}
if "BC" in maps:
    out_imgs["BC"] = write("T_Fab_%s_BC" % ASSET, pixels(maps["BC"][0]), True)
if "N" in maps:
    n = pixels(maps["N"][0])
    opengl, how = True, "Blender Normal Map node source (OpenGL)"
    if "H" in maps:
        opengl, corr = normal_is_opengl(n, pixels(maps["H"][0])[..., 0])
        how = "height-slope correlation %.3f" % corr
    report["normal_source"] = ("OpenGL, green flipped to DirectX" if opengl else "DirectX, kept") + " (%s)" % how
    preview = n.copy()
    if opengl:
        n[..., 1] = 1.0 - n[..., 1]
    else:
        preview[..., 1] = 1.0 - preview[..., 1]       # Blender previews OpenGL
    out_imgs["N"] = write("T_Fab_%s_N" % ASSET, n, False)
    prev = bpy.data.images.new("preview_N", SIZE, SIZE, alpha=False, float_buffer=False)
    prev.colorspace_settings.name = "Non-Color"
    prev.pixels.foreach_set(preview.ravel())
    prev.pack()
    out_imgs["N_preview"] = prev
orm = np.ones((SIZE, SIZE, 4), dtype=np.float32)
if "AO" in maps:
    orm[..., 0] = pixels(maps["AO"][0])[..., 0]
if "R" in maps:
    img, ch = maps["R"]
    orm[..., 1] = pixels(img)[..., ch or 0]
else:
    orm[..., 1] = 0.6
if "M" in maps:
    img, ch = maps["M"]
    orm[..., 2] = pixels(img)[..., ch or 0]
else:
    orm[..., 2] = 0.0
out_imgs["ORM"] = write("T_Fab_%s_ORM" % ASSET, orm, False)
report["orm_means"] = [round(float(orm[..., i].mean()), 3) for i in range(3)]
if "E" in maps:
    out_imgs["E"] = write("T_Fab_%s_E" % ASSET, pixels(maps["E"][0]), True)
    if src_mat is not None:
        bsdf = next(n for n in src_mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
        report["source_emission_strength"] = bsdf.inputs["Emission Strength"].default_value

# ---------------------------------------------------------------- material (Blender preview of the exported set)
mat = bpy.data.materials.new("M_Fab_" + ASSET)
nt = mat.node_tree
bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")


def tex(img, y):
    t = nt.nodes.new("ShaderNodeTexImage")
    t.image = img
    t.location = (-700, y)
    return t


if "BC" in out_imgs:
    nt.links.new(tex(out_imgs["BC"], 300).outputs["Color"], bsdf.inputs["Base Color"])
sep = nt.nodes.new("ShaderNodeSeparateColor")
sep.location = (-400, 0)
nt.links.new(tex(out_imgs["ORM"], 0).outputs["Color"], sep.inputs[0])
nt.links.new(sep.outputs["Green"], bsdf.inputs["Roughness"])
nt.links.new(sep.outputs["Blue"], bsdf.inputs["Metallic"])
if "N_preview" in out_imgs:
    nm = nt.nodes.new("ShaderNodeNormalMap")
    nm.location = (-400, -300)
    nt.links.new(tex(out_imgs["N_preview"], -300).outputs["Color"], nm.inputs["Color"])
    nt.links.new(nm.outputs["Normal"], bsdf.inputs["Normal"])
if "E" in out_imgs:
    nt.links.new(tex(out_imgs["E"], -600).outputs["Color"], bsdf.inputs["Emission Color"])
    bsdf.inputs["Emission Strength"].default_value = report.get("source_emission_strength", 1.0)
obj.data.materials.clear()
obj.data.materials.append(mat)
for m in list(bpy.data.materials):
    if m.users == 0:
        bpy.data.materials.remove(m)
keep = {i.name for i in out_imgs.values()}
for im in list(bpy.data.images):              # drop the seller's packed 4K maps from the saved file
    if im.name not in keep and im.name != "Render Result":
        bpy.data.images.remove(im)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, NAME + ".blend"), compress=True, relative_remap=True)
with open(os.path.join(OUT, "prep_report.json"), "w", encoding="utf-8") as fh:
    json.dump(report, fh, indent=1)
print("__RESULT__" + json.dumps(report))

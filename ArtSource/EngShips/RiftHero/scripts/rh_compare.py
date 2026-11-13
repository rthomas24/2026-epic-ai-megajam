# SPDX-License-Identifier: MIT
# RiftHero: before / after sheets. Renders the Rodin original with its textures (once) and stacks
# "original over rebuild" for a clay pair and a material pair.
#   blender --background --factory-startup --python rh_compare.py -- <rebuild label, e.g. 03_rebuild_v03> [out name]
import os, sys
import bpy, numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rh_common as C

args = sys.argv[sys.argv.index("--") + 1:]
label = args[0]
out_name = args[1] if len(args) > 1 else label
src_mat = os.path.join(C.REVIEW, "00_rodin_original_material_sheet.png")
if not os.path.isfile(src_mat):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ob = C.import_rodin("SM_RiftRodin", textured=True)
    C.skill("blender-mcp/scripts/review_renders.py")["main"](targets=[ob.name], out_dir=C.REVIEW, label="00_rodin_original",
                                                             mode="material", views=C.VIEWS, resolution=800)


def load(path):
    im = bpy.data.images.load(path)
    w, h = im.size
    a = np.empty(w * h * 4, np.float32)
    im.pixels.foreach_get(a)
    return a.reshape(h, w, 4)


def stack(top, bottom, out):
    a, b = load(top), load(bottom)
    w = max(a.shape[1], b.shape[1])
    bar = np.zeros((14, w, 4), np.float32); bar[..., 0] = 1.0; bar[..., 1] = 0.75; bar[..., 3] = 1.0
    def pad(x):
        if x.shape[1] == w:
            return x
        p = np.zeros((x.shape[0], w, 4), np.float32); p[..., 3] = 1; p[:, :x.shape[1]] = x
        return p
    img = np.vstack([pad(b), bar, pad(a)])           # image rows are bottom-up: original ends on top
    o = bpy.data.images.new("out", img.shape[1], img.shape[0], alpha=False)
    o.pixels.foreach_set(img.ravel())
    o.filepath_raw = out; o.file_format = "PNG"; o.save()
    return out

res = {}
for mode in ("clay", "material"):
    new = os.path.join(C.REVIEW, "%s_%s_sheet.png" % (label, mode))
    old = os.path.join(C.REVIEW, "00_rodin_original_%s_sheet.png" % mode)
    if os.path.isfile(new):
        res[mode] = stack(old, new, os.path.join(C.REVIEW, "%s_BEFORE_AFTER_%s.png" % (out_name, mode)))
C.out(res)

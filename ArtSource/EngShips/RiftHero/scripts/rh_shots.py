# SPDX-License-Identifier: MIT
# RiftHero: game-like review shots (chase camera and "8 m on foot" framing) of a set of objects, rendered in
# a temporary scene with EEVEE, and a grid helper to put original and hybrid side by side.
import math, os
import bpy, numpy as np
from mathutils import Vector

# name: (camera position m, look-at m, lens mm). Ship: nose +X; ground when landed is about z = -1.15.
CAMS = {
    "chase": ((-7.2, 0.0, 2.5), (0.3, 0.0, 0.0), 40),
    "chase_3q": ((-6.2, -3.6, 2.3), (0.0, 0.0, 0.0), 40),
    "foot_rear_8m": ((-8.0, 0.0, 0.55), (0.0, 0.0, -0.05), 40),
    "foot_3q_8m": ((-6.6, -4.5, 0.55), (0.0, 0.0, -0.05), 40),
    "rear_close": ((-5.0, -1.3, 0.75), (-1.9, 0.0, 0.05), 50),
}


def render(targets, out_dir, label, mode="material", cams=None, res=(1000, 640)):
    cams = cams or list(CAMS)
    sc = bpy.data.scenes.new("__shots__")
    w = bpy.data.worlds.new("__shots_w__")
    bg = w.node_tree.nodes["Background"]
    bg.inputs["Color"].default_value = (0.30, 0.33, 0.38, 1)
    bg.inputs["Strength"].default_value = 0.9
    sc.world = w
    objs = [bpy.data.objects[n] for n in targets]
    for o in objs:
        sc.collection.objects.link(o)
    lights = []
    for name, rot, energy, col in (("key", (math.radians(50), 0, math.radians(-150)), 3.2, (1, 0.97, 0.92)),
                                   ("fill", (math.radians(65), 0, math.radians(60)), 1.2, (0.8, 0.88, 1.0)),
                                   ("rim", (math.radians(110), 0, math.radians(-60)), 0.8, (1, 1, 1))):
        ld = bpy.data.lights.new("__shots_" + name, "SUN")
        ld.energy = energy
        ld.color = col
        lo = bpy.data.objects.new("__shots_" + name, ld)
        lo.rotation_euler = rot
        sc.collection.objects.link(lo)
        lights.append(lo)
    cd = bpy.data.cameras.new("__shots_cam__")
    co = bpy.data.objects.new("__shots_cam__", cd)
    sc.collection.objects.link(co)
    sc.camera = co
    sc.render.engine = "BLENDER_EEVEE"
    sc.render.resolution_x, sc.render.resolution_y = res
    sc.render.image_settings.file_format = "PNG"
    sc.view_settings.view_transform = "Standard"
    try:
        sc.eevee.taa_render_samples = 32
        sc.eevee.use_raytracing = True
    except Exception:
        pass
    saved = []
    if mode == "clay":
        clay = bpy.data.materials.new("__shots_clay__")
        b = next(n for n in clay.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
        b.inputs["Base Color"].default_value = (0.55, 0.55, 0.56, 1)
        b.inputs["Roughness"].default_value = 0.42
        b.inputs["Metallic"].default_value = 0.0
        for o in objs:
            saved.append((o, [s.material for s in o.material_slots]))
            for s in o.material_slots:
                s.material = clay
    out = {}
    win = bpy.context.window
    prev = win.scene
    win.scene = sc
    try:
        for cam in cams:
            pos, look, lens = CAMS[cam]
            co.location = pos
            d = Vector(look) - Vector(pos)
            co.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
            cd.lens = lens
            path = os.path.join(out_dir, "%s_%s_%s.png" % (label, mode, cam))
            sc.render.filepath = path
            bpy.ops.render.render(write_still=True, scene=sc.name)
            out[cam] = path
    finally:
        win.scene = prev
        for o, ms in saved:
            for s, m in zip(o.material_slots, ms):
                s.material = m
        for lo in lights + [co]:
            bpy.data.objects.remove(lo)
        bpy.data.scenes.remove(sc)
        bpy.data.worlds.remove(w)
    return out


def _load(path):
    im = bpy.data.images.load(path, check_existing=False)
    w, h = im.size
    a = np.empty(w * h * 4, np.float32)
    im.pixels.foreach_get(a)
    bpy.data.images.remove(im)
    return a.reshape(h, w, 4)


def grid(rows, out, gap=8):
    """rows: list of lists of image paths (same size). Row 0 ends up on top."""
    imgs = [[_load(p) for p in r] for r in rows]
    h, w = imgs[0][0].shape[:2]
    H = len(rows) * h + (len(rows) - 1) * gap
    W = len(rows[0]) * w + (len(rows[0]) - 1) * gap
    canvas = np.zeros((H, W, 4), np.float32)
    canvas[..., 0], canvas[..., 1], canvas[..., 3] = 1.0, 0.75, 1.0
    for r, row in enumerate(imgs):
        y0 = H - (r + 1) * h - r * gap
        for c, im in enumerate(row):
            x0 = c * (w + gap)
            canvas[y0:y0 + h, x0:x0 + w] = im
    o = bpy.data.images.new("__grid__", W, H, alpha=False)
    o.pixels.foreach_set(canvas.ravel())
    o.filepath_raw = out
    o.file_format = "PNG"
    o.save()
    bpy.data.images.remove(o)
    return out

# SPDX-License-Identifier: MIT
"""
review_renders.py - render an asset from fixed angles and assemble ONE contact-sheet
image, so an agent can judge silhouette, proportion, shading and topology at a glance.

The user's scene is never modified: the targets are linked into a temporary scene
(with its own camera, lights and world), rendered, and everything temporary is removed.

Inside Blender (Blender Lab MCP `execute_blender_code` / `execute_blender_code_for_cli`):

    import runpy
    m = runpy.run_path(r"<repo>/.claude/skills/blender-mcp/scripts/review_renders.py")
    result = m["main"](targets=["SM_Ship_Rift"], out_dir=r"<repo>/ArtSource/Space/Ships/Rift/review",
                       label="rift_v03", mode="material")

From a shell (no MCP needed):

    blender --background asset.blend --python review_renders.py -- \
        --targets SM_Ship_Rift --out-dir C:/tmp/review --label rift_v03 --mode clay

Modes:
    clay       Workbench, grey studio clay + cavity + outline. Fast. Judge form/silhouette.
    material   EEVEE (falls back to Cycles) with studio HDRI + key/rim lights. Judge materials/emission.
    wireframe  Workbench clay with the evaluated topology drawn on top. Judge edge flow/density.

Views (azimuth, elevation in degrees; azimuth 0 = camera on -Y looking at +Y = Blender "Front"):
    front right back left top bottom (orthographic), front_3q back_3q low_3q (perspective).
Contact-sheet cells are laid out left-to-right, top-to-bottom in the order of `views`.
"""

import json
import math
import os
import sys
import time

import bpy
import numpy as np
from mathutils import Matrix, Vector

HELPER_PREFIXES = ("UCX_", "UBX_", "USP_", "UCP_", "SOCKET_", "CUT_", "SRC_")

VIEW_ANGLES = {
    "front": (0.0, 0.0),
    "right": (90.0, 0.0),
    "back": (180.0, 0.0),
    "left": (270.0, 0.0),
    "top": (0.0, 90.0),
    "bottom": (0.0, -90.0),
    "front_3q": (35.0, 22.0),
    "back_3q": (215.0, 22.0),
    "low_3q": (325.0, -18.0),
}
ORTHO_VIEWS = {"front", "right", "back", "left", "top", "bottom"}
DEFAULT_VIEWS = ("front_3q", "front", "right", "top", "back_3q", "low_3q")

_TMP = "__review__"


def _engine_ids():
    return set(bpy.types.RenderSettings.bl_rna.properties["engine"].enum_items.keys())


def _pick_engine(mode):
    ids = _engine_ids()
    if mode in {"clay", "wireframe"}:
        return "BLENDER_WORKBENCH"
    for candidate in ("BLENDER_EEVEE", "BLENDER_EEVEE_NEXT", "CYCLES"):
        if candidate in ids:
            return candidate
    return "CYCLES"


def _gather_targets(targets, collection):
    objs = []
    if collection:
        coll = bpy.data.collections.get(collection)
        if coll is None:
            raise ValueError("Collection {!r} not found".format(collection))
        objs.extend(coll.all_objects)
    for name in targets or []:
        obj = bpy.data.objects.get(name)
        if obj is None:
            raise ValueError("Object {!r} not found".format(name))
        objs.append(obj)
        objs.extend(obj.children_recursive)
    if not targets and not collection:
        objs = [o for o in bpy.context.scene.objects if o.visible_get()]
    seen, out = set(), []
    for obj in objs:
        if obj.name in seen or obj.name.startswith(HELPER_PREFIXES):
            continue
        if obj.type not in {"MESH", "CURVE", "SURFACE", "META", "FONT", "CURVES", "POINTCLOUD", "VOLUME"}:
            continue
        seen.add(obj.name)
        out.append(obj)
    return out


def _world_bbox(objs):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    lo = Vector((math.inf,) * 3)
    hi = Vector((-math.inf,) * 3)
    for obj in objs:
        ev = obj.evaluated_get(depsgraph)
        for corner in ev.bound_box:
            p = ev.matrix_world @ Vector(corner)
            lo = Vector(map(min, lo, p))
            hi = Vector(map(max, hi, p))
    if lo.x == math.inf:
        raise ValueError("Targets have no renderable geometry")
    return lo, hi


def _look_matrix(forward, world_up=Vector((0.0, 0.0, 1.0))):
    """3x3 rotation whose -Z axis points along `forward` (camera/light convention)."""
    f = forward.normalized()
    if abs(f.dot(world_up)) > 0.999:
        world_up = Vector((0.0, 1.0, 0.0))
    right = f.cross(world_up).normalized()
    up = right.cross(f).normalized()
    return Matrix((
        (right.x, up.x, -f.x),
        (right.y, up.y, -f.y),
        (right.z, up.z, -f.z),
    ))


def _view_dir(azimuth, elevation):
    a, e = math.radians(azimuth), math.radians(elevation)
    return Vector((math.sin(a) * math.cos(e), -math.cos(a) * math.cos(e), math.sin(e))).normalized()


def _studio_hdri():
    try:
        folder = bpy.utils.system_resource("DATAFILES", path="studiolights/world")
    except Exception:
        return None
    if not folder or not os.path.isdir(folder):
        return None
    for name in ("studio.exr", "city.exr", "courtyard.exr", "interior.exr", "forest.exr"):
        path = os.path.join(folder, name)
        if os.path.exists(path):
            return path
    files = [f for f in os.listdir(folder) if f.lower().endswith((".exr", ".hdr"))]
    return os.path.join(folder, files[0]) if files else None


def _setup_world(scene, strength):
    world = bpy.data.worlds.new(_TMP + "world")
    scene.world = world
    try:
        world.use_nodes = True  # deprecated/always-on in newer versions; harmless if so
    except Exception:
        pass
    tree = world.node_tree
    if tree is None:
        world.color = (0.05, 0.05, 0.05)
        return world, None
    tree.nodes.clear()
    out = tree.nodes.new("ShaderNodeOutputWorld")
    bg = tree.nodes.new("ShaderNodeBackground")
    bg.inputs["Strength"].default_value = strength
    tree.links.new(bg.outputs["Background"], out.inputs["Surface"])
    image = None
    hdri = _studio_hdri()
    if hdri:
        image = bpy.data.images.load(hdri, check_existing=True)
        env = tree.nodes.new("ShaderNodeTexEnvironment")
        env.image = image
        tree.links.new(env.outputs["Color"], bg.inputs["Color"])
    else:
        bg.inputs["Color"].default_value = (0.4, 0.4, 0.42, 1.0)
    return world, image


def _configure_engine(scene, engine, mode, samples):
    scene.render.engine = engine
    if engine == "BLENDER_WORKBENCH":
        shading = scene.display.shading
        shading.light = "STUDIO"
        shading.color_type = "OBJECT" if mode == "wireframe" else "SINGLE"
        shading.single_color = (0.72, 0.72, 0.74)
        shading.show_cavity = True
        shading.cavity_type = "BOTH"
        shading.show_object_outline = True
        shading.show_specular_highlight = True
        try:
            scene.display.render_aa = "16"
        except Exception:
            pass
    elif engine.startswith("BLENDER_EEVEE"):
        try:
            scene.eevee.taa_render_samples = samples
        except Exception:
            pass
    else:
        scene.cycles.samples = max(8, samples)
        scene.cycles.use_denoising = True
        scene.cycles.device = "CPU"


def _png_to_array(path):
    img = bpy.data.images.load(path, check_existing=False)
    w, h = img.size
    px = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(px)
    bpy.data.images.remove(img)
    return px.reshape(h, w, 4)


def _coverage(arr):
    alpha = arr[:, :, 3] > 0.01
    if not alpha.any():
        return 0.0, False
    rows = np.where(alpha.any(axis=1))[0]
    cols = np.where(alpha.any(axis=0))[0]
    h, w = alpha.shape
    touches = rows[0] == 0 or cols[0] == 0 or rows[-1] == h - 1 or cols[-1] == w - 1
    return float(alpha.mean()), bool(touches)


def _autocrop_square(arr, pad=0.07):
    """Crop to the non-transparent content as a padded square (keeps the asset centred)."""
    alpha = arr[:, :, 3] > 0.01
    if not alpha.any():
        return arr
    h, w = alpha.shape
    rows = np.where(alpha.any(axis=1))[0]
    cols = np.where(alpha.any(axis=0))[0]
    cy = (rows[0] + rows[-1]) * 0.5
    cx = (cols[0] + cols[-1]) * 0.5
    side = max(int(round(max(rows[-1] - rows[0], cols[-1] - cols[0]) * (1.0 + 2.0 * pad))), 8)
    y0 = int(round(cy - side * 0.5))
    x0 = int(round(cx - side * 0.5))
    out = np.zeros((side, side, 4), dtype=np.float32)
    sy0, sx0 = max(y0, 0), max(x0, 0)
    sy1, sx1 = min(y0 + side, h), min(x0 + side, w)
    out[sy0 - y0:sy1 - y0, sx0 - x0:sx1 - x0] = arr[sy0:sy1, sx0:sx1]
    return out


def _resize(arr, size):
    """Bilinear resize of an (h, w, 4) array to (size, size, 4)."""
    h, w = arr.shape[:2]
    if h == size and w == size:
        return arr
    ys = (np.arange(size) + 0.5) * h / size - 0.5
    xs = (np.arange(size) + 0.5) * w / size - 0.5
    y0 = np.clip(np.floor(ys).astype(int), 0, h - 1)
    x0 = np.clip(np.floor(xs).astype(int), 0, w - 1)
    y1 = np.clip(y0 + 1, 0, h - 1)
    x1 = np.clip(x0 + 1, 0, w - 1)
    wy = np.clip(ys - y0, 0.0, 1.0)[:, None, None]
    wx = np.clip(xs - x0, 0.0, 1.0)[None, :, None]
    top = arr[y0][:, x0] * (1.0 - wx) + arr[y0][:, x1] * wx
    bottom = arr[y1][:, x0] * (1.0 - wx) + arr[y1][:, x1] * wx
    return (top * (1.0 - wy) + bottom * wy).astype(np.float32)


def _write_sheet(cells, path, bg, cols, gap=6):
    n = len(cells)
    cols = max(1, min(cols, n))
    rows = math.ceil(n / cols)
    h, w = cells[0].shape[:2]
    sheet_h = rows * h + (rows + 1) * gap
    sheet_w = cols * w + (cols + 1) * gap
    sheet = np.empty((sheet_h, sheet_w, 4), dtype=np.float32)
    sheet[:, :] = (bg[0] * 0.6, bg[1] * 0.6, bg[2] * 0.6, 1.0)
    bg_arr = np.array(bg[:3], dtype=np.float32)
    for i, cell in enumerate(cells):
        r, c = divmod(i, cols)
        # Image rows run bottom-to-top in Blender, so row 0 of the grid goes at the top.
        y0 = sheet_h - gap - (r + 1) * h - r * gap
        x0 = gap + c * (w + gap)
        a = cell[:, :, 3:4]
        rgb = cell[:, :, :3] * a + bg_arr * (1.0 - a)
        sheet[y0:y0 + h, x0:x0 + w, :3] = rgb
        sheet[y0:y0 + h, x0:x0 + w, 3] = 1.0
    img = bpy.data.images.new(_TMP + "sheet", width=sheet_w, height=sheet_h, alpha=False)
    img.pixels.foreach_set(sheet.ravel())
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


def main(targets=None, collection=None, out_dir=None, label="review", mode="material",
         views=DEFAULT_VIEWS, resolution=512, samples=16, margin=1.12,
         background=(0.16, 0.17, 0.19), world_strength=0.6, keep_views=False, columns=3,
         view_transform=None):
    t0 = time.time()
    if mode not in {"clay", "material", "wireframe"}:
        raise ValueError("mode must be clay, material or wireframe")
    views = list(views or DEFAULT_VIEWS)
    unknown = [v for v in views if v not in VIEW_ANGLES]
    if unknown:
        raise ValueError("Unknown views {}; choose from {}".format(unknown, sorted(VIEW_ANGLES)))
    out_dir = os.path.abspath(out_dir or os.path.join(bpy.app.tempdir, "review_renders"))
    os.makedirs(out_dir, exist_ok=True)

    objs = _gather_targets(targets, collection)
    if not objs:
        raise ValueError("No renderable target objects found")
    warnings = [o.name + " has hide_render=True and was skipped" for o in objs if o.hide_render]
    objs = [o for o in objs if not o.hide_render]
    lo, hi = _world_bbox(objs)
    center = (lo + hi) * 0.5
    size = hi - lo
    radius = max(size.length * 0.5, 1e-4)

    engine = _pick_engine(mode)
    scene = bpy.data.scenes.new(_TMP + "scene")
    temp_objects, temp_data, temp_images = [], [], []
    files = []
    try:
        root = scene.collection
        if mode == "wireframe":
            for obj in objs:
                base = obj.copy()  # shares mesh data; the user's object is untouched
                base.color = (0.72, 0.72, 0.74, 1.0)
                wire = obj.copy()
                wire.color = (0.03, 0.03, 0.035, 1.0)
                mod = wire.modifiers.new(_TMP + "wire", "WIREFRAME")
                mod.thickness = radius * 0.0035
                mod.use_replace = True
                mod.use_even_offset = True
                for o in (base, wire):
                    root.objects.link(o)
                    temp_objects.append(o)
        else:
            for obj in objs:
                root.objects.link(obj)

        cam_data = bpy.data.cameras.new(_TMP + "cam")
        cam = bpy.data.objects.new(_TMP + "cam", cam_data)
        root.objects.link(cam)
        temp_objects.append(cam)
        temp_data.append(cam_data)
        scene.camera = cam

        key_data = bpy.data.lights.new(_TMP + "key", "SUN")
        key_data.energy = 3.0
        key_data.angle = math.radians(8)
        rim_data = bpy.data.lights.new(_TMP + "rim", "SUN")
        rim_data.energy = 2.0
        key = bpy.data.objects.new(_TMP + "key", key_data)
        rim = bpy.data.objects.new(_TMP + "rim", rim_data)
        for o in (key, rim):
            root.objects.link(o)
            temp_objects.append(o)
        temp_data.extend([key_data, rim_data])

        world, hdri_img = _setup_world(scene, world_strength)
        if hdri_img is not None:
            temp_images.append(hdri_img)

        render = scene.render
        render.resolution_x = render.resolution_y = int(resolution)
        render.resolution_percentage = 100
        render.film_transparent = True
        render.image_settings.file_format = "PNG"
        render.image_settings.color_mode = "RGBA"
        render.image_settings.color_depth = "8"
        if view_transform:
            scene.view_settings.view_transform = view_transform
        _configure_engine(scene, engine, mode, samples)

        lens = 50.0
        cam_data.lens = lens
        cam_data.sensor_fit = "AUTO"
        tan_half_fov = math.tan(math.atan((cam_data.sensor_width * 0.5) / lens))
        corners = [Vector((x, y, z)) for x in (lo.x, hi.x) for y in (lo.y, hi.y) for z in (lo.z, hi.z)]

        # Orthographic views share ONE scale (blueprint style) so proportions can be
        # compared between front/side/top and measured with `ortho_cm_per_px`.
        ortho_scale = 1e-4
        for view in views:
            if view in ORTHO_VIEWS:
                rot = _look_matrix(-_view_dir(*VIEW_ANGLES[view]))
                xs = [(c - center).dot(rot.col[0]) for c in corners]
                ys = [(c - center).dot(rot.col[1]) for c in corners]
                ortho_scale = max(ortho_scale, max(xs) - min(xs), max(ys) - min(ys))
        ortho_scale *= margin

        cells, view_info = [], []
        for view in views:
            az, el = VIEW_ANGLES[view]
            d = _view_dir(az, el)
            forward = -d
            rot = _look_matrix(forward)
            right, up = rot.col[0], rot.col[1]
            if view in ORTHO_VIEWS:
                cam_data.type = "ORTHO"
                cam_data.ortho_scale = ortho_scale
                distance = radius * 4.0
                render.resolution_x = render.resolution_y = int(resolution)
            else:
                # Perspective: fit every bbox corner, render larger, then auto-crop to the asset.
                cam_data.type = "PERSP"
                distance = radius * 1.05
                for c in corners:
                    off = c - center
                    lateral = max(abs(off.dot(right)), abs(off.dot(up))) * margin
                    distance = max(distance, lateral / tan_half_fov + off.dot(d))
                render.resolution_x = render.resolution_y = int(resolution * 1.5)
            cam.matrix_world = Matrix.Translation(center + d * distance) @ rot.to_4x4()
            cam_data.clip_start = max(distance * 0.001, 1e-4)
            cam_data.clip_end = distance + radius * 8.0

            # Key light from the camera's upper-left, rim light from behind.
            key.matrix_world = _look_matrix(forward + rot.col[1] * -0.7 + rot.col[0] * 0.6).to_4x4()
            rim.matrix_world = _look_matrix(-forward + rot.col[1] * -0.4).to_4x4()

            path = os.path.join(out_dir, "{}_{}_{}.png".format(label, mode, view))
            render.filepath = path
            bpy.ops.render.render(write_still=True, scene=scene.name)
            arr = _png_to_array(path)
            cov, touches = _coverage(arr)
            if view not in ORTHO_VIEWS:
                arr = _resize(_autocrop_square(arr), int(resolution))
                cov, _unused = _coverage(arr)
            cells.append(arr)
            info = {"view": view, "coverage": round(cov, 3), "touches_edge": touches}
            if keep_views:
                info["path"] = path
            else:
                os.remove(path)
            files.append(info)
            view_info.append(view)

        sheet_path = os.path.join(out_dir, "{}_{}_sheet.png".format(label, mode))
        _write_sheet(cells, sheet_path, background, columns)
    finally:
        for o in temp_objects:
            if bpy.data.objects.get(o.name) is o:
                bpy.data.objects.remove(o, do_unlink=True)
        for d in temp_data:
            try:
                if isinstance(d, bpy.types.Camera):
                    bpy.data.cameras.remove(d)
                elif isinstance(d, bpy.types.Light):
                    bpy.data.lights.remove(d)
            except ReferenceError:
                pass
        if scene.world and scene.world.name.startswith(_TMP):
            bpy.data.worlds.remove(scene.world)
        for img in temp_images:
            if img.users == 0:
                bpy.data.images.remove(img)
        bpy.data.scenes.remove(scene)

    for info in files:
        if info["touches_edge"]:
            warnings.append("View {} touches the frame edge; increase margin".format(info["view"]))
    scale_length = bpy.context.scene.unit_settings.scale_length
    return {
        "status": "ok",
        "sheet": sheet_path,
        "mode": mode,
        "engine": engine,
        "layout": {"columns": min(columns, len(view_info)), "order": view_info},
        "ortho_cm_per_px": round(ortho_scale * 100.0 * scale_length / float(resolution), 4),
        "views": files,
        "bbox_min_m": [round(v, 4) for v in lo],
        "bbox_max_m": [round(v, 4) for v in hi],
        "size_cm": [round(v * 100.0 * scale_length, 2) for v in size],
        "objects": [o.name for o in objs],
        "warnings": warnings,
        "seconds": round(time.time() - t0, 2),
    }


def _cli():
    import argparse
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser(description="Multi-view review renders + contact sheet")
    p.add_argument("--targets", nargs="*")
    p.add_argument("--collection")
    p.add_argument("--out-dir", required=True)
    p.add_argument("--label", default="review")
    p.add_argument("--mode", default="material", choices=["clay", "material", "wireframe"])
    p.add_argument("--views", nargs="*")
    p.add_argument("--resolution", type=int, default=512)
    p.add_argument("--samples", type=int, default=16)
    p.add_argument("--keep-views", action="store_true")
    a = p.parse_args(argv)
    res = main(targets=a.targets, collection=a.collection, out_dir=a.out_dir, label=a.label, mode=a.mode,
               views=a.views or DEFAULT_VIEWS, resolution=a.resolution, samples=a.samples, keep_views=a.keep_views)
    print("__RESULT__" + json.dumps(res))


if __name__ == "__main__":
    _cli()

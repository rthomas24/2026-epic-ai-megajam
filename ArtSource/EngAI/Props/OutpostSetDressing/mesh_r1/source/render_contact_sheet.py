#!/usr/bin/env python3
"""Studio-HDRI contact sheet for #40 Outpost set dressing.

    blender -b --factory-startup --python render_contact_sheet.py -- <mesh_r1_dir>
    python3 render_contact_sheet.py <mesh_r1_dir>   # re-composite from source/shots only
"""
from __future__ import annotations

import math
import os
import sys

try:
    import bpy
    from mathutils import Vector, Euler, Matrix
except ImportError:  # system python: composite only
    bpy = None

# Pillow lives in the system Python, not Blender's bundled interpreter.
import sys as _sys
for _p in ("/usr/local/lib/python3.13/dist-packages", "/usr/lib/python3/dist-packages",
           "/usr/local/lib/python3.11/dist-packages", "/usr/lib/python3.12/dist-packages"):
    if _p not in _sys.path:
        _sys.path.append(_p)
try:
    from PIL import Image, ImageDraw, ImageFont
    _HAS_PIL = True
except ImportError:
    Image = ImageDraw = ImageFont = None
    _HAS_PIL = False

PROPS = [
    ("SM_Outpost_CargoCrate", "Cargo Crate"),
    ("SM_Outpost_SupplyCrate", "Supply Crate"),
    ("SM_Outpost_FieldTerminal", "Field Terminal"),
    ("SM_Outpost_DataKiosk", "Data Kiosk"),
    ("SM_Outpost_CommAntenna", "Comm Antenna"),
    ("SM_Outpost_CommsDish", "Comms Dish"),
    ("SM_Outpost_Floodlight", "Floodlight"),
    ("SM_Outpost_BollardLight", "Bollard Light"),
    ("SM_Outpost_Generator", "Generator"),
    ("SM_Outpost_PowerJunction", "Power Junction"),
]
HEROES = ["SM_Outpost_FieldTerminal", "SM_Outpost_DataKiosk", "SM_Outpost_Generator", "SM_Outpost_CommsDish"]
CONCEPTS = [
    "01_cargo_crate.png", "02_supply_crate.png", "03_field_terminal.png", "04_data_kiosk.png",
    "05_comm_antenna.png", "06_comms_dish.png", "07_floodlight.png", "08_bollard_light.png",
    "09_generator.png", "10_power_junction.png",
]
HDRI = "/home/box/.local/blender-4.5.3-linux-x64/4.5/datafiles/studiolights/world/studio.exr"
CONCEPT_DIR = "/workspace/concepts/fal/40_props"


def argv_dir():
    a = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    here = os.path.dirname(os.path.abspath(__file__))
    return a[0] if a else os.path.dirname(here)


def setup_scene(hdri=True, samples=32):
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = samples
    scene.cycles.use_denoising = True
    scene.cycles.denoiser = "OPENIMAGEDENOISE"
    scene.cycles.max_bounces = 6
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.look = "AgX - Base Contrast"
    scene.view_settings.exposure = 0.35
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    # world
    world = bpy.data.worlds.new("Studio") if "Studio" not in bpy.data.worlds else bpy.data.worlds["Studio"]
    scene.world = world
    world.use_nodes = True
    nt = world.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputWorld")
    bg = nt.nodes.new("ShaderNodeBackground")
    if hdri and os.path.isfile(HDRI):
        env = nt.nodes.new("ShaderNodeTexEnvironment")
        env.image = bpy.data.images.load(HDRI)
        nt.links.new(env.outputs["Color"], bg.inputs["Color"])
        bg.inputs["Strength"].default_value = 1.15
    else:
        bg.inputs["Color"].default_value = (0.62, 0.64, 0.66, 1)
        bg.inputs["Strength"].default_value = 1.0
    nt.links.new(bg.outputs["Background"], out.inputs["Surface"])
    # soft ground
    bpy.ops.mesh.primitive_plane_add(size=40, location=(0, 0, -0.002))
    ground = bpy.context.active_object
    ground.name = "GROUND"
    mat = bpy.data.materials.new("M_Ground")
    mat.use_nodes = True
    princ = mat.node_tree.nodes["Principled BSDF"]
    princ.inputs["Base Color"].default_value = (0.48, 0.49, 0.51, 1)
    princ.inputs["Roughness"].default_value = 0.55
    ground.data.materials.append(mat)
    # extra fill so it is not a void
    bpy.ops.object.light_add(type="AREA", location=(4, -6, 5))
    fill = bpy.context.active_object
    fill.data.energy = 250
    fill.data.size = 6
    fill.rotation_euler = (math.radians(55), 0, math.radians(20))
    bpy.ops.object.light_add(type="AREA", location=(-5, 4, 3))
    rim = bpy.context.active_object
    rim.data.energy = 80
    rim.data.size = 4
    rim.rotation_euler = (math.radians(70), 0, math.radians(-40))
    return scene


def make_material(tex_dir):
    mat = bpy.data.materials.new("M_OutpostProps")
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])

    def load(name, noncolor):
        path = os.path.join(tex_dir, name)
        if name.endswith("_N.png"):
            alt = os.path.join(tex_dir, "source", "preview", "T_OutpostProps_N_OpenGL.png")
            path = alt if os.path.isfile(alt) else path
        img = bpy.data.images.load(path)
        img.colorspace_settings.name = "Non-Color" if noncolor else "sRGB"
        n = nt.nodes.new("ShaderNodeTexImage")
        n.image = img
        return n, path

    bc, _ = load("T_OutpostProps_BC.png", False)
    orm, _ = load("T_OutpostProps_ORM.png", True)
    emi, _ = load("T_OutpostProps_E.png", False)
    nnode, npath = load("T_OutpostProps_N.png", True)
    nrm = nt.nodes.new("ShaderNodeNormalMap")
    if "OpenGL" not in os.path.basename(npath):
        sep = nt.nodes.new("ShaderNodeSeparateColor")
        comb = nt.nodes.new("ShaderNodeCombineColor")
        inv = nt.nodes.new("ShaderNodeMath")
        inv.operation = "SUBTRACT"
        inv.inputs[0].default_value = 1.0
        nt.links.new(nnode.outputs["Color"], sep.inputs["Color"])
        nt.links.new(sep.outputs["Red"], comb.inputs["Red"])
        nt.links.new(sep.outputs["Green"], inv.inputs[1])
        nt.links.new(inv.outputs["Value"], comb.inputs["Green"])
        nt.links.new(sep.outputs["Blue"], comb.inputs["Blue"])
        nt.links.new(comb.outputs["Color"], nrm.inputs["Color"])
    else:
        nt.links.new(nnode.outputs["Color"], nrm.inputs["Color"])
    nt.links.new(nrm.outputs["Normal"], bsdf.inputs["Normal"])
    nt.links.new(bc.outputs["Color"], bsdf.inputs["Base Color"])
    sep_o = nt.nodes.new("ShaderNodeSeparateColor")
    nt.links.new(orm.outputs["Color"], sep_o.inputs["Color"])
    nt.links.new(sep_o.outputs["Green"], bsdf.inputs["Roughness"])
    nt.links.new(sep_o.outputs["Blue"], bsdf.inputs["Metallic"])
    nt.links.new(emi.outputs["Color"], bsdf.inputs["Emission Color"])
    bsdf.inputs["Emission Strength"].default_value = 8.0
    return mat


def clay_material():
    mat = bpy.data.materials.new("M_Clay")
    mat.use_nodes = True
    p = mat.node_tree.nodes["Principled BSDF"]
    p.inputs["Base Color"].default_value = (0.72, 0.70, 0.66, 1)
    p.inputs["Roughness"].default_value = 0.55
    p.inputs["Metallic"].default_value = 0.0
    return mat


def make_pilot():
    """180 cm reference figure, matte signal-orange."""
    mat = bpy.data.materials.new("M_Pilot")
    mat.use_nodes = True
    p = mat.node_tree.nodes["Principled BSDF"]
    p.inputs["Base Color"].default_value = (0.85, 0.38, 0.12, 1)
    p.inputs["Roughness"].default_value = 0.6
    parts = []
    def box(sz, loc):
        bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
        o = bpy.context.active_object
        o.scale = sz
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        o.data.materials.append(mat)
        parts.append(o)
        return o
    box((0.32, 0.22, 0.58), (0, 0, 1.07))          # torso
    box((0.18, 0.18, 0.18), (0, 0, 1.45))          # neck/head base
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.12, location=(0, 0, 1.64), segments=16, ring_count=8)
    head = bpy.context.active_object
    head.data.materials.append(mat)
    parts.append(head)
    for y in (-0.20, 0.20):                         # legs
        box((0.12, 0.12, 0.76), (0, y, 0.40))
    for y in (-0.28, 0.28):                         # arms
        box((0.10, 0.10, 0.58), (0.02, y, 1.10))
    # join
    for o in bpy.context.selected_objects:
        o.select_set(False)
    for o in parts:
        o.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    fig = bpy.context.active_object
    fig.name = "REF_Pilot_180cm"
    # origin at feet
    bpy.ops.object.origin_set(type="ORIGIN_GEOMETRY", center="BOUNDS")
    # snap z to 0
    zs = [v.co.z for v in fig.data.vertices]
    off = min(zs)
    for v in fig.data.vertices:
        v.co.z -= off
    fig.location = (0, 0, 0)
    return fig


def import_prop(mesh_dir, name, mat):
    path = os.path.join(mesh_dir, name + ".fbx")
    existing = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=path)
    new = [o for o in bpy.data.objects if o not in existing]
    vis = [o for o in new if o.type == "MESH" and not o.name.startswith("UCX_")]
    ucx = [o for o in new if o.name.startswith("UCX_")]
    root = vis[0]
    # drop UCX from the default view (keep them, hide)
    for u in ucx:
        u.hide_render = True
        u.hide_set(True)
        u.display_type = "WIRE"
    for o in vis:
        o.data.materials.clear()
        o.data.materials.append(mat)
        # ensure scale is 1 and sitting on z=0 (already)
    return root, vis, ucx


def arrange_lineup(roots, spacing=2.15, start_y=0.0):
    # place along +Y, bottoms at 0, facing +X (our front)
    ys = []
    y = start_y
    for root in roots:
        # use world bbox
        xs, ys_, zs = [], [], []
        family = [root] + [c for c in root.children_recursive if c.type == "MESH" and not c.name.startswith("UCX_")]
        for o in family:
            for v in o.data.vertices:
                w = o.matrix_world @ v.co
                xs.append(w.x); ys_.append(w.y); zs.append(w.z)
        width = max(ys_) - min(ys_)
        # center at y
        cy = 0.5 * (max(ys_) + min(ys_))
        root.location.y = y - cy
        # keep x=0, z so min z = 0
        root.location.z -= min(zs)
        ys.append(y)
        y += max(width, 0.6) + 0.55
    return ys


def add_cam(loc, look, lens=50, name="CAM"):
    cam_d = bpy.data.cameras.new(name)
    cam_d.lens = lens
    cam = bpy.data.objects.new(name, cam_d)
    bpy.context.scene.collection.objects.link(cam)
    cam.location = loc
    direction = Vector(look) - Vector(loc)
    cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    bpy.context.scene.camera = cam
    return cam


def render_to(path, res_x, res_y):
    scene = bpy.context.scene
    scene.render.resolution_x = res_x
    scene.render.resolution_y = res_y
    scene.render.filepath = path
    if "--resume" in sys.argv and os.path.isfile(path) and os.path.getsize(path) > 100000:
        print("SKIP (resume)", path)
        return
    bpy.ops.render.render(write_still=True)
    print("RENDERED", path)


def font(size):
    for p in ("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
              "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"):
        if os.path.isfile(p):
            return ImageFont.truetype(p, size)
    return ImageFont.load_default()


def font_bold(size):
    for p in ("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
              "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf"):
        if os.path.isfile(p):
            return ImageFont.truetype(p, size)
    return font(size)


ROWS = [list(range(0, 5)), list(range(5, 10))]
ROW_X = [0.0, -4.0]          # row A at x=0, row B 4 m behind (top view shows both)


def _family(root):
    return [root] + [c for c in root.children_recursive if c.type == "MESH" and not c.name.startswith("UCX_")]


def _world_pts(objs):
    return [o.matrix_world @ v.co for o in objs for v in o.data.vertices]


def _bbox_corners(pts):
    mn = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    mx = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    return mn, mx, [Vector((x, y, z)) for x in (mn.x, mx.x) for y in (mn.y, mx.y) for z in (mn.z, mx.z)]


def fit_cam(pts, direction, lens, res_x, res_y, name, margin=0.04, look_z_bias=0.0):
    """Perspective camera on `direction` from the bbox centre, pulled back until every point fits."""
    from bpy_extras.object_utils import world_to_camera_view
    scene = bpy.context.scene
    scene.render.resolution_x = res_x
    scene.render.resolution_y = res_y
    mn, mx, corners = _bbox_corners(pts)
    centre = (mn + mx) * 0.5
    centre.z += look_z_bias
    d = Vector(direction).normalized()
    cam = add_cam(centre + d * 5.0, centre, lens=lens, name=name)
    test = corners + list(pts[:: max(1, len(pts) // 4000)])
    dist = 1.0
    for _ in range(400):
        cam.location = centre + d * dist
        cam.rotation_euler = (centre - cam.location).to_track_quat("-Z", "Y").to_euler()
        bpy.context.view_layer.update()
        ok = True
        for p in test:
            c = world_to_camera_view(scene, cam, p)
            if c.z <= 0 or not (margin <= c.x <= 1 - margin and margin <= c.y <= 1 - margin):
                ok = False
                break
        if ok:
            break
        dist *= 1.03
    return cam


def proj_rect(cam, objs, res_x, res_y):
    from bpy_extras.object_utils import world_to_camera_view
    scene = bpy.context.scene
    xs, ys = [], []
    for p in _world_pts(objs):
        c = world_to_camera_view(scene, cam, p)
        xs.append(c.x * res_x); ys.append((1 - c.y) * res_y)
    return [int(min(xs)), int(min(ys)), int(max(xs)) + 1, int(max(ys)) + 1]


def set_visible(objs, flag):
    for o in objs:
        o.hide_render = not flag
        o.hide_set(not flag)


def main():
    import json
    mesh_dir = argv_dir()
    shots = os.path.join(mesh_dir, "source", "shots")
    os.makedirs(shots, exist_ok=True)
    composite_only = bpy is None or "--composite-only" in sys.argv
    if not composite_only:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        setup_scene(hdri=True, samples=28)
        mat = make_material(mesh_dir)
        clay = clay_material()
        roots, ucx_of = [], {}
        for name, _ in PROPS:
            root, vis, ucx = import_prop(mesh_dir, name, mat)
            roots.append(root); ucx_of[root.name] = ucx
        # two rows of five, each with its own 180 cm pilot at the +Y end
        pilots = []
        for r, idx in enumerate(ROWS):
            rr = [roots[i] for i in idx]
            arrange_lineup(rr, spacing=2.2, start_y=-4.5)
            for o in rr:
                o.location.x = ROW_X[r]
            bpy.context.view_layer.update()
            ymax = max(p.y for p in _world_pts(sum((_family(o) for o in rr), [])))
            pil = make_pilot()
            pil.name = "REF_Pilot_180cm_row%d" % r
            pil.location = (ROW_X[r], ymax + 0.75, 0.0)
            pilots.append(pil)
        bpy.context.view_layer.update()
        RX, RY = 3200, 1300
        crops = {"res": [RX, RY], "rows": {}}
        all_vis = [o for r in roots for o in _family(r)]
        def row_objs(r):
            return [o for i in ROWS[r] for o in _family(roots[i])] + [pilots[r]]
        def show_row(r, ucx=False):
            for rr in range(2):
                set_visible(row_objs(rr), rr == r)
                for i in ROWS[rr]:
                    set_visible(ucx_of[roots[i].name], ucx and rr == r)
        for r in range(2):
            show_row(r)
            pts = _world_pts(row_objs(r))
            cam = fit_cam(pts, (1.0, -0.34, 0.36), 45, RX, RY, "CAM_34_%d" % r, margin=0.03)
            crops["rows"][str(r)] = {PROPS[i][0]: proj_rect(cam, _family(roots[i]), RX, RY) for i in ROWS[r]}
            crops["rows"][str(r)]["pilot"] = proj_rect(cam, [pilots[r]], RX, RY)
            render_to(os.path.join(shots, "row%d_34.png" % r), RX, RY)
            fit_cam(pts, (-1.0, 0.34, 0.40), 45, RX, RY, "CAM_BACK_%d" % r, margin=0.03)
            render_to(os.path.join(shots, "row%d_back.png" % r), RX, RY)
        # top: both rows
        for rr in range(2):
            set_visible(row_objs(rr), True)
        pts = _world_pts(row_objs(0) + row_objs(1))
        fit_cam(pts, (0.0, 0.0001, 1.0), 50, RX, 1500, "CAM_TOP", margin=0.03)
        render_to(os.path.join(shots, "top.png"), RX, 1500)
        # hero close-ups, each prop isolated with its own pilot hidden
        for name in HEROES:
            root = next(r for r in roots if r.name == name or r.name.startswith(name))
            for rr in range(2):
                set_visible(row_objs(rr), False)
            set_visible(_family(root), True)
            fit_cam(_world_pts(_family(root)), (1.0, -0.75, 0.42), 50, 1200, 1200, "CAM_H_" + name, margin=0.06)
            render_to(os.path.join(shots, "hero_" + name + ".png"), 1200, 1200)
        # clay (bevel highlight) and UCX, same 3/4 cameras
        for o in all_vis:
            o.data.materials.clear(); o.data.materials.append(clay)
        for r in range(2):
            show_row(r)
            bpy.context.scene.camera = bpy.data.objects["CAM_34_%d" % r]
            render_to(os.path.join(shots, "row%d_clay.png" % r), RX, RY)
        ucx_mat = bpy.data.materials.new("M_UCX")
        ucx_mat.use_nodes = True
        ucx_mat.blend_method = "BLEND"
        pb = ucx_mat.node_tree.nodes["Principled BSDF"]
        pb.inputs["Base Color"].default_value = (0.15, 0.75, 0.95, 1)
        pb.inputs["Roughness"].default_value = 0.25
        pb.inputs["Alpha"].default_value = 0.35
        pb.inputs["Emission Color"].default_value = (0.1, 0.6, 0.85, 1)
        pb.inputs["Emission Strength"].default_value = 0.6
        for r in roots:
            for u in ucx_of[r.name]:
                u.data.materials.clear(); u.data.materials.append(ucx_mat)
        clay.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.55, 0.54, 0.52, 1)
        for r in range(2):
            show_row(r, ucx=True)
            bpy.context.scene.camera = bpy.data.objects["CAM_34_%d" % r]
            render_to(os.path.join(shots, "row%d_ucx.png" % r), RX, RY)
        with open(os.path.join(shots, "crops.json"), "w") as f:
            json.dump(crops, f, indent=1)
        print("RENDERS_DONE", flush=True)

    # -------- composite
    if not _HAS_PIL:
        print("NO_PIL - shots written; composite with: python3 render_contact_sheet.py <mesh_r1_dir>")
        return
    def open_(fn):
        return Image.open(os.path.join(shots, fn)).convert("RGB")
    crops = json.load(open(os.path.join(shots, "crops.json")))
    W, pad, gaps = 3600, 28, 18
    bg = (36, 38, 42); accent = (230, 200, 80); white = (240, 242, 245); midc = (180, 184, 190)
    f_title, f_h, f_s = font_bold(44), font_bold(28), font(20)
    inner = W - 2 * pad
    def sw(im, w):
        return im.resize((w, int(im.height * w / im.width)), Image.Resampling.LANCZOS)
    blocks = []   # (title, image)
    for kind, title in (("34", "FRONT 3/4"), ("back", "BACK 3/4")):
        for r in range(2):
            blocks.append(("%s  ·  row %s  ·  180 cm PILOT (orange) at the right" % (title, "AB"[r]), sw(open_("row%d_%s.png" % (r, kind)), inner)))
    blocks.append(("TOP  (row A front, row B behind; each with its 180 cm pilot)", sw(open_("top.png"), inner)))
    hw = (inner - 3 * 16) // 4
    hero_row = Image.new("RGB", (inner, hw + 8), bg)
    for i, n in enumerate(HEROES):
        hero_row.paste(open_("hero_" + n + ".png").resize((hw, hw), Image.Resampling.LANCZOS), (i * (hw + 16), 8))
    blocks.append(("HERO CLOSE-UPS    " + "  ·  ".join(dict(PROPS)[n] for n in HEROES), hero_row))
    # concept vs build pairs, build tile cropped from the row 3/4 render by projected bbox
    cell_w = (inner - 4 * 12) // 5
    cell_h = int(cell_w * 0.55)
    con = Image.new("RGB", (inner, (cell_h + 36) * 2 + 12), bg)
    dcon = ImageDraw.Draw(con)
    row_img = {r: open_("row%d_34.png" % r) for r in range(2)}
    def fit(im, w, h):
        s_ = min(w / im.width, h / im.height)
        im2 = im.resize((max(1, int(im.width * s_)), max(1, int(im.height * s_))), Image.Resampling.LANCZOS)
        c = Image.new("RGB", (w, h), (58, 60, 64)); c.paste(im2, ((w - im2.width) // 2, (h - im2.height) // 2)); return c
    cw = (cell_w - 8) // 2
    for i, (name, label) in enumerate(PROPS):
        r = 0 if i < 5 else 1
        x0, y0, x1, y1 = crops["rows"][str(r)][name]
        m = 14
        tile = row_img[r].crop((max(0, x0 - m), max(0, y0 - m), min(RXW(row_img[r]), x1 + m), min(row_img[r].height, y1 + m)))
        concept = Image.open(os.path.join(CONCEPT_DIR, CONCEPTS[i])).convert("RGB")
        x = (i % 5) * (cell_w + 12); y = (i // 5) * (cell_h + 36 + 12)
        dcon.text((x, y), label, font=f_s, fill=white)
        dcon.text((x + cw + 8, y), "build", font=font(16), fill=midc)
        con.paste(fit(concept, cw, cell_h), (x, y + 30))
        con.paste(fit(tile, cw, cell_h), (x + cw + 8, y + 30))
    blocks.append(("CONCEPT (ChatGPT/fal, visual reference only)  vs  ROUTE C BUILD", con))
    for r in range(2):
        blocks.append(("CLAY / BEVEL HIGHLIGHT CHECK  ·  row %s  (smooth_by_angle 30°  ·  live Bevel  ·  WeightedNormal)" % "AB"[r], sw(open_("row%d_clay.png" % r), inner)))
    for r in range(2):
        blocks.append(("UCX CONVEX COLLIDERS (cyan, over clay)  ·  row %s" % "AB"[r], sw(open_("row%d_ucx.png" % r), inner)))
    header_h, section_h = 150, 44
    total_h = header_h + sum(section_h + im.height + gaps for _, im in blocks) + 70
    canvas = Image.new("RGB", (W, total_h), bg)
    draw = ImageDraw.Draw(canvas)
    y = 24
    draw.text((pad, y), "OUTPOST & LANDING-PAD SET DRESSING   ·   #40   ·   Route C  mesh_r1", font=f_title, fill=white)
    y += 58
    draw.text((pad, y), "Blender 4.5.3 LTS  ·  procedural hard-surface (no AI meshes)  ·  shared trim T_OutpostProps  ·  180 cm pilot  ·  neutral studio.exr  ·  AgX  ·  Cycles",
              font=f_s, fill=midc)
    y += 36
    draw.rectangle((pad, y, W - pad, y + 3), fill=accent)
    y += 16
    for title, im in blocks:
        draw.text((pad, y + 6), title, font=f_h, fill=accent); y += section_h
        canvas.paste(im, (pad, y)); y += im.height + gaps
    draw.text((pad, y + 10), "ArtSource/EngAI/Props/OutpostSetDressing/mesh_r1   ·   Grok Bot   ·   for Lead review, not merged",
              font=f_s, fill=midc)
    out = os.path.join(mesh_dir, "contact_sheet.png")
    canvas.save(out, compress_level=6)
    print("WROTE", out, canvas.size)


def RXW(im):
    return im.width


if __name__ == "__main__":
    main()

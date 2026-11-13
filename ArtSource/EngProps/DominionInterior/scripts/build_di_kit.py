# SPDX-License-Identifier: MIT
# DominionInterior - build the whole kit reproducibly (Blender 5.2 headless):
#   blender --background --factory-startup --python scripts/build_di_kit.py -- --tag r1 [--blockout] [--render] [--export]
# Always: builds every piece, measures it against the size contract (di_kit.EXPECT) and writes
# review/size_check_<tag>.json. --render: review/kit_<tag>_clay.png, kit_<tag>_material.png, kit_<tag>_assembly.png.
# --export: DominionInterior.blend + export/SM_DI_*.fbx and manifests (blender-unreal-export skill).
import argparse
import json
import math
import os
import runpy
import sys

import bpy
import numpy as np
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
REPO = os.path.dirname(os.path.dirname(os.path.dirname(KIT)))
sys.path.insert(0, HERE)
import di_kit  # noqa: E402

TEX = os.path.join(KIT, "textures")
REVIEW = os.path.join(KIT, "review")
EXPORT = os.path.join(KIT, "export")
SKILLS = os.path.join(REPO, ".claude", "skills")

GLOWS = {   # name: (colour, strength in Blender)
    "GlowRed": ((1.0, 0.14, 0.02), 6.0),
    "GlowWhite": ((0.9, 0.95, 1.0), 8.0),
    "Screen": ((0.1, 0.5, 0.6), 0.0),
    "Cell": ((1.0, 0.42, 0.08), 7.0),
    "CoreGlow": ((1.0, 0.86, 0.7), 14.0),
}


def make_materials():
    mats = {}
    m = bpy.data.materials.new("M_DI_Atlas")
    nt = m.node_tree
    bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")

    def tex(name, srgb, y):
        n = nt.nodes.new("ShaderNodeTexImage")
        n.image = bpy.data.images.load(os.path.join(TEX, name))
        n.image.colorspace_settings.name = "sRGB" if srgb else "Non-Color"
        n.location = (-700, y)
        return n
    bc, orm, nm = tex("T_DI_Atlas_BC.png", True, 300), tex("T_DI_Atlas_ORM.png", False, 0), tex("T_DI_Atlas_N.png", False, -300)
    nt.links.new(bc.outputs["Color"], bsdf.inputs["Base Color"])
    sep = nt.nodes.new("ShaderNodeSeparateColor")
    nt.links.new(orm.outputs["Color"], sep.inputs["Color"])
    nt.links.new(sep.outputs["Green"], bsdf.inputs["Roughness"])
    nt.links.new(sep.outputs["Blue"], bsdf.inputs["Metallic"])
    s2 = nt.nodes.new("ShaderNodeSeparateColor")
    inv = nt.nodes.new("ShaderNodeMath")
    inv.operation = "SUBTRACT"
    inv.inputs[0].default_value = 1.0
    comb = nt.nodes.new("ShaderNodeCombineColor")
    nmap = nt.nodes.new("ShaderNodeNormalMap")
    nt.links.new(nm.outputs["Color"], s2.inputs["Color"])
    nt.links.new(s2.outputs["Red"], comb.inputs["Red"])
    nt.links.new(s2.outputs["Green"], inv.inputs[1])          # the map is DirectX; Blender wants OpenGL
    nt.links.new(inv.outputs[0], comb.inputs["Green"])
    nt.links.new(s2.outputs["Blue"], comb.inputs["Blue"])
    nt.links.new(comb.outputs["Color"], nmap.inputs["Color"])
    nt.links.new(nmap.outputs["Normal"], bsdf.inputs["Normal"])
    m.use_backface_culling = True
    mats["Atlas"] = m
    for key, (col, strength) in GLOWS.items():
        g = bpy.data.materials.new("M_DI_" + key)
        b = next(n for n in g.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
        b.inputs["Base Color"].default_value = (0.01, 0.012, 0.016, 1.0)
        b.inputs["Roughness"].default_value = 0.15
        b.inputs["Emission Color"].default_value = (col[0], col[1], col[2], 1.0)
        b.inputs["Emission Strength"].default_value = strength
        g.use_backface_culling = True
        mats[key] = g
    s = bpy.data.materials.new("M_DI_Shield")
    b = next(n for n in s.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    b.inputs["Base Color"].default_value = (1.0, 0.25, 0.1, 1.0)
    b.inputs["Alpha"].default_value = 0.16
    b.inputs["Roughness"].default_value = 0.1
    b.inputs["Emission Color"].default_value = (1.0, 0.2, 0.05, 1.0)
    b.inputs["Emission Strength"].default_value = 0.6
    try:
        s.surface_render_method = "BLENDED"
    except Exception:
        pass
    mats["Shield"] = s
    return mats


def build_all(blockout):
    di_kit.DETAIL = not blockout
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    coll = bpy.data.collections.new("DominionInterior")
    bpy.context.scene.collection.children.link(coll)
    mats = make_materials()
    rows, objs = [], {}
    for fn in di_kit.BUILDERS:
        p = fn()
        lo, hi = p.bounds()
        size = tuple(round(hi[i] - lo[i], 2) for i in range(3))
        if p.expect:
            ok = all(abs(lo[i] - p.expect[0][i]) < 0.011 and abs(hi[i] - p.expect[1][i]) < 0.011 for i in range(3))
            want = [list(p.expect[0]), list(p.expect[1])]
        elif "Debris" in p.name:
            ok = abs(max(size) - 150.0) < 0.5 and abs(lo[2]) < 0.01
            want = "largest dimension 150, resting on Z = 0"
        else:
            ok = True
            want = "measured (pivot at the ring centre)"
        obj = p.build(mats, coll)
        objs[p.name] = obj
        tris = sum(len(f.vertices) - 2 for f in obj.data.polygons)
        row = {"asset": p.name, "min_cm": list(lo), "max_cm": list(hi), "size_cm": list(size), "expected": want,
               "check": "PASS" if ok else "FAIL", "tris": tris, "materials": [m.name for m in obj.data.materials],
               "collision_hulls": len(p.hulls), "uv_notes": sorted(set(p.notes))[:6]}
        rows.append(row)
        print("SIZE %-26s %s  min %s max %s size %s tris %d mats %d hulls %d %s" % (
            p.name, row["check"], lo, hi, size, tris, len(row["materials"]), len(p.hulls), row["uv_notes"] or ""))
    return objs, rows, mats


# ------------------------------------------------------------------------------------------------ rendering

def look_at(cam, eye, target):
    cam.location = eye
    d = Vector(target) - Vector(eye)
    cam.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()


def setup_render(scene, w, h, world_strength=1.0):
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x, scene.render.resolution_y = w, h
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = False
    try:
        scene.eevee.taa_render_samples = 24
        scene.eevee.use_raytracing = True
    except Exception:
        pass
    world = scene.world or bpy.data.worlds.new("W")
    scene.world = world
    bg = next((n for n in world.node_tree.nodes if n.type == "BACKGROUND"), None) if world.node_tree else None
    if bg is None:
        world.use_nodes = True
        bg = next(n for n in world.node_tree.nodes if n.type == "BACKGROUND")
    bg.inputs["Color"].default_value = (0.09, 0.095, 0.11, 1.0)
    bg.inputs["Strength"].default_value = world_strength


def render_to(scene, path):
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)


def load_px(path):
    img = bpy.data.images.load(path)
    w, h = img.size
    a = np.empty(w * h * 4, np.float32)
    img.pixels.foreach_get(a)
    bpy.data.images.remove(img)
    return a.reshape(h, w, 4)[::-1, :, :3]


def save_px(arr, path):
    h, w, _ = arr.shape
    img = bpy.data.images.new("sheet", w, h, alpha=False)
    rgba = np.ones((h, w, 4), np.float32)
    rgba[..., :3] = arr[::-1]
    img.pixels.foreach_set(rgba.ravel())
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


def add_light(kind, loc, energy, color=(1, 1, 1), size=1.0, rot=None):
    d = bpy.data.lights.new("L", kind)
    d.energy = energy
    d.color = color
    if kind == "POINT":
        d.shadow_soft_size = size
    o = bpy.data.objects.new("L", d)
    o.location = loc
    if rot:
        o.rotation_euler = rot
    bpy.context.scene.collection.objects.link(o)
    return o


VIEWS = {   # per-piece camera direction (from the piece toward the camera) and what to hide; default 3/4 front-left
    "SM_DI_Corridor_Straight": "inside", "SM_DI_Corridor_Corner": "inside", "SM_DI_Corridor_T": "inside",
    "SM_DI_Room_Ceiling": "below", "SM_DI_Shaft": "above",
}


def contact_sheet(objs, tag, clay, cell=320, cols=5):
    scene = bpy.context.scene
    setup_render(scene, cell, cell, 1.6 if clay else 1.0)
    cam_d = bpy.data.cameras.new("C")
    cam = bpy.data.objects.new("C", cam_d)
    scene.collection.objects.link(cam)
    scene.camera = cam
    sun = add_light("SUN", (0, 0, 10), 3.0 if clay else 2.2, rot=(math.radians(50), 0, math.radians(-35)))
    fill = add_light("SUN", (0, 0, 10), 1.0, rot=(math.radians(60), 0, math.radians(140)))
    head = add_light("POINT", (0, 0, 0), 0, size=0.5)
    claymat = bpy.data.materials.new("Clay")
    b = next(n for n in claymat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    b.inputs["Base Color"].default_value = (0.55, 0.55, 0.56, 1)
    b.inputs["Roughness"].default_value = 0.6
    claymat.use_backface_culling = True
    names = list(objs)
    for o in objs.values():
        o.hide_render = True
    cells = []
    tmp = os.path.join(REVIEW, "_cell.png")
    for name in names:
        o = objs[name]
        o.hide_render = False
        saved = list(o.data.materials)
        if clay:
            for i in range(len(o.data.materials)):
                o.data.materials[i] = claymat
        bb = [o.matrix_world @ Vector(c) for c in o.bound_box]
        lo = Vector((min(v.x for v in bb), min(v.y for v in bb), min(v.z for v in bb)))
        hi = Vector((max(v.x for v in bb), max(v.y for v in bb), max(v.z for v in bb)))
        c = (lo + hi) / 2
        rad = (hi - lo).length / 2
        mode = VIEWS.get(name, "3q")
        cam_d.lens = 35
        head.data.energy = 0
        if mode == "inside":
            cam_d.lens = 16
            look_at(cam, c + Vector((-4.6, 0.5, -0.4)), c + Vector((0, 0, -0.3)))
            head.location = c + Vector((-0.5, 0, 1.5))
            head.data.energy = 900
        elif mode == "below":
            look_at(cam, c + Vector((-0.9, -0.7, -1.4)) * rad * 1.5, c)
            head.location = c + Vector((0, 0, -3))
            head.data.energy = 1500
        elif mode == "above":
            cam_d.lens = 18
            look_at(cam, c + Vector((0.3, 0.2, 7.5)), c + Vector((0, 0, -3)))
            head.location = c
            head.data.energy = 1500
        else:
            look_at(cam, c + Vector((-1.0, -0.8, 0.62)).normalized() * rad * 2.9, c)
        render_to(scene, tmp)
        cells.append(load_px(tmp))
        if clay:
            for i, m in enumerate(saved):
                o.data.materials[i] = m
        o.hide_render = True
    rowsn = int(math.ceil(len(cells) / cols))
    sheet = np.zeros((rowsn * cell, cols * cell, 3), np.float32)
    for i, a in enumerate(cells):
        r, cc = divmod(i, cols)
        sheet[r * cell:(r + 1) * cell, cc * cell:(cc + 1) * cell] = a
    out = os.path.join(REVIEW, "kit_%s_%s.png" % (tag, "clay" if clay else "material"))
    save_px(sheet, out)
    if os.path.exists(tmp):
        os.remove(tmp)
    for o in (cam, sun, fill, head):
        bpy.data.objects.remove(o, do_unlink=True)
    for o in objs.values():
        o.hide_render = False
    print("SHEET", out, "order:", ", ".join(names))
    return out


def figure(coll, at, yaw=0.0):
    """A 185 cm stand-in for scale (render only)."""
    m = bpy.data.materials.new("Fig")
    b = next(n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    b.inputs["Base Color"].default_value = (0.75, 0.72, 0.62, 1)
    parts = [((0, -0.1, 0.45), (0.16, 0.16, 0.9)), ((0, 0.1, 0.45), (0.16, 0.16, 0.9)), ((0, 0, 1.2), (0.26, 0.42, 0.62)),
             ((0, 0, 1.72), (0.22, 0.2, 0.26)), ((0, -0.29, 1.15), (0.12, 0.12, 0.66)), ((0, 0.29, 1.15), (0.12, 0.12, 0.66))]
    root = bpy.data.objects.new("Figure", None)
    coll.objects.link(root)
    root.location = at
    root.rotation_euler = (0, 0, yaw)
    for c, s in parts:
        me = bpy.data.meshes.new("fig")
        import bmesh
        bm = bmesh.new()
        bmesh.ops.create_cube(bm, size=1.0)
        for v in bm.verts:
            v.co = Vector((v.co.x * s[0] + c[0], v.co.y * s[1] + c[1], v.co.z * s[2] + c[2]))
        bm.to_mesh(me)
        bm.free()
        me.materials.append(m)
        o = bpy.data.objects.new("fig", me)
        coll.objects.link(o)
        o.parent = root
    return root


def assembly(objs, tag):
    """3 corridor pieces -> doorway (door half raised) -> 3 x 3 room with a console and low cover, a 185 cm figure."""
    scene = bpy.context.scene
    for o in objs.values():
        o.hide_render = True
    coll = bpy.data.collections.new("Assembly")
    scene.collection.children.link(coll)

    def put(name, x, y, z=0.0, yaw=0.0):
        o = bpy.data.objects.new("A_" + name, objs[name].data)
        coll.objects.link(o)
        o.location = (x / 100.0, y / 100.0, z / 100.0)
        o.rotation_euler = (0, 0, math.radians(yaw))
        return o
    for x in (-1000, -600, -200):
        put("SM_DI_Corridor_Straight", x, 0)
    put("SM_DI_Doorway", 0, 0)
    put("SM_DI_Door", 0, 0, 250)
    ceil = []
    for i in range(3):
        for j in range(3):
            put("SM_DI_Room_Floor", 200 + 400 * i, -400 + 400 * j)
            ceil.append(put("SM_DI_Room_Ceiling", 200 + 400 * i, -400 + 400 * j))
    for y in (-400, 400):
        put("SM_DI_Room_Wall", 0, y)
    for y in (-400, 0, 400):
        put("SM_DI_Room_Wall", 1200, y)
    for x in (200, 600, 1000):
        put("SM_DI_Room_Wall", x, -600, yaw=90)
        put("SM_DI_Room_Wall", x, 600, yaw=90)
    put("SM_DI_Console", 1120, 150)
    put("SM_DI_Cover_Low", 520, -180)
    put("SM_DI_Cover_Low", 760, 220, yaw=20)
    put("SM_DI_Conduit", 1080, -480)
    put("SM_DI_Debris_A", 380, 380, yaw=40)
    fig1 = figure(coll, (-5.2, 0.3, 0))
    fig2 = figure(coll, (4.0, -0.4, 0))
    lights = []
    for x in (-1000, -600, -200):
        lights.append(add_light("POINT", (x / 100.0, 0, 5.4), 450, (0.85, 0.92, 1.0), 0.6))
    for i in range(3):
        for j in range(3):
            lights.append(add_light("POINT", ((200 + 400 * i) / 100.0, (-400 + 400 * j) / 100.0, 5.5), 380, (0.85, 0.92, 1.0), 0.6))
    cam_d = bpy.data.cameras.new("C")
    cam_d.lens = 18
    cam = bpy.data.objects.new("C", cam_d)
    scene.collection.objects.link(cam)
    scene.camera = cam
    setup_render(scene, 1600, 800, 0.15)
    shots = []
    tmp = os.path.join(REVIEW, "_asm.png")
    # third person in the corridor, toward the doorway
    look_at(cam, (-9.6, 0.9, 2.5), (-1.0, 0.0, 1.5))
    render_to(scene, tmp)
    shots.append(load_px(tmp))
    # third person in the room, toward the console and cover
    look_at(cam, (0.9, -1.6, 2.5), (9.0, 0.6, 1.2))
    render_to(scene, tmp)
    shots.append(load_px(tmp))
    # cutaway from above (ceilings hidden)
    for c in ceil:
        c.hide_render = True
    cam_d.lens = 28
    look_at(cam, (-6.0, -13.0, 16.0), (2.0, 0.0, 1.0))
    for l in lights:
        l.data.energy *= 1.6
    render_to(scene, tmp)
    shots.append(load_px(tmp))
    sheet = np.concatenate(shots, 0)
    out = os.path.join(REVIEW, "kit_%s_assembly.png" % tag)
    save_px(sheet, out)
    os.remove(tmp)
    for o in list(coll.objects) + lights + [cam]:
        bpy.data.objects.remove(o, do_unlink=True)
    bpy.data.collections.remove(coll)
    for o in objs.values():
        o.hide_render = False
    print("SHEET", out)
    return out


# ------------------------------------------------------------------------------------------------ main

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--tag", default="r0")
    ap.add_argument("--blockout", action="store_true")
    ap.add_argument("--render", action="store_true")
    ap.add_argument("--export", action="store_true")
    a = ap.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
    os.makedirs(REVIEW, exist_ok=True)
    objs, rows, mats = build_all(a.blockout)
    with open(os.path.join(REVIEW, "size_check_%s.json" % a.tag), "w", encoding="utf-8") as fh:
        json.dump(rows, fh, indent=1)
    fails = [r["asset"] for r in rows if r["check"] != "PASS"]
    print("SIZECHECK %d pieces, %d FAIL %s, total tris %d" % (len(rows), len(fails), fails, sum(r["tris"] for r in rows)))
    if a.render:
        contact_sheet(objs, a.tag, True)
        if not a.blockout:
            contact_sheet(objs, a.tag, False)
        assembly(objs, a.tag)
    if a.export and not fails:
        rep = runpy.run_path(os.path.join(SKILLS, "blender-mcp", "scripts", "asset_report.py"))["main"](targets=list(objs))
        with open(os.path.join(REVIEW, "health_%s.json" % a.tag), "w", encoding="utf-8") as fh:
            json.dump(rep, fh, indent=1, default=str)
        ex = runpy.run_path(os.path.join(SKILLS, "blender-unreal-export", "scripts", "export_for_unreal.py"))["main"]
        os.makedirs(EXPORT, exist_ok=True)
        for name in objs:
            r = ex(root=name, out_dir=EXPORT, front_axis="+X", collision="auto" if name not in di_kit.NO_COLLISION else "none")
            print("EXPORT %s tris %s size %s collision %s warn %s" % (name, r.get("tris"), r.get("size_cm"), r.get("collision"), r.get("warnings")))
        # lay the pieces out for anyone opening the file, then save
        x = 0.0
        for name, o in objs.items():
            w = o.dimensions.y
            o.location = (0, x + w / 2, 0)
            x += w + 1.0
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(KIT, "DominionInterior.blend"), compress=True)
    print("__RESULT__ done")


main()

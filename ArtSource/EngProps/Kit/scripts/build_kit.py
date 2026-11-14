# SPDX-License-Identifier: MIT
# Kit (planet outposts) - build the whole kit reproducibly (Blender 5.2 headless):
#   blender --background --factory-startup --python scripts/build_kit.py -- --tag r1 [--render] [--export]
# Always: builds every piece (kit_pieces.BUILDERS), adds the edge finish (1.5 cm bevel, weighted normals), measures
# each against its size contract and writes review/size_check_<tag>.json.
# --render: review/kit_<tag>_clay.png, kit_<tag>_material.png, kit_<tag>_assembly.png (a test room with a 180 cm
# figure). --export: Kit.blend + export/SM_Kit_*.fbx and manifests (blender-unreal-export skill).
# Forked from DominionInterior/scripts/build_di_kit.py.
import argparse
import json
import math
import os
import runpy
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
REPO = os.path.dirname(os.path.dirname(os.path.dirname(KIT)))
sys.path.insert(0, HERE)
import kit_lib  # noqa: E402
import kit_pieces  # noqa: E402
import kit_pieces_d  # noqa: E402
import kit_pieces_space  # noqa: E402

SET = "planet"          # --set dominion: the Dominion shell (kit_pieces_d), T_KitD_Trim, red strips

TEX = os.path.join(KIT, "textures")
REVIEW = os.path.join(KIT, "review")
EXPORT = os.path.join(KIT, "export")
SKILLS = os.path.join(REPO, ".claude", "skills")
GLOW_TEAL = (0.05, 0.85, 0.75)
GLOW_EMBER = (1.0, 0.36, 0.06)


def make_materials(glow=GLOW_TEAL):
    mats = {}
    stem = "T_KitD_Trim" if SET == "dominion" else "T_Kit_Trim"
    if SET in ("dominion", "space"):
        glow = (1.0, 0.1, 0.03)
    m = bpy.data.materials.new("M_Kit_Trim")
    nt = m.node_tree
    bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")

    def tex(name, srgb, y):
        n = nt.nodes.new("ShaderNodeTexImage")
        n.image = bpy.data.images.load(os.path.join(TEX, name), check_existing=True)
        n.image.colorspace_settings.name = "sRGB" if srgb else "Non-Color"
        n.location = (-700, y)
        return n
    bc, orm, nm = tex(stem + "_BC.png", True, 300), tex(stem + "_ORM.png", False, 0), tex(stem + "_N.png", False, -300)
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
    mats["Trim"] = m
    if SET == "space":                          # the rig's Dominion trim, the rock, the eyes
        d = m.copy()
        d.name = "M_Kit_Dom"
        for nd in d.node_tree.nodes:
            if nd.type == "TEX_IMAGE" and nd.image and nd.image.name.startswith("T_Kit_Trim"):
                nd.image = bpy.data.images.load(os.path.join(TEX, nd.image.name.replace("T_Kit_Trim", "T_KitD_Trim")), check_existing=True)
                nd.image.colorspace_settings.name = "sRGB" if "_BC" in nd.image.name else "Non-Color"
        mats["Dom"] = d
        for key, tint in (("Trim2", (0.55, 0.56, 0.6)), ("Scorch", (0.08, 0.06, 0.05))):
            t2 = m.copy()
            t2.name = "M_Kit_" + key
            bs = next(n for n in t2.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
            mix = t2.node_tree.nodes.new("ShaderNodeMix")
            mix.data_type = "RGBA"
            mix.blend_type = "MULTIPLY"
            mix.inputs["Factor"].default_value = 1.0
            src = bs.inputs["Base Color"].links[0].from_socket
            t2.node_tree.links.new(src, mix.inputs["A"])
            mix.inputs["B"].default_value = (tint[0], tint[1], tint[2], 1.0)
            t2.node_tree.links.new(mix.outputs["Result"], bs.inputs["Base Color"])
            mats[key] = t2
        for key, col, rough in (("Rock", (0.16, 0.14, 0.13), 0.9), ("Cut", (0.24, 0.23, 0.22), 0.6), ("Bore", (0.03, 0.03, 0.035), 0.9)):
            r_ = bpy.data.materials.new("M_Kit_" + key)
            b_ = next(n for n in r_.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
            b_.inputs["Base Color"].default_value = (col[0], col[1], col[2], 1.0)
            b_.inputs["Roughness"].default_value = rough
            mats[key] = r_
    if SET != "dominion":                       # ART-18: the artefact's ancient stone (T_KitS_Trim, same regions)
        st = m.copy()
        st.name = "M_Kit_Stone"
        for nd in st.node_tree.nodes:
            if nd.type == "TEX_IMAGE" and nd.image and nd.image.name.startswith("T_Kit_Trim"):
                nd.image = bpy.data.images.load(os.path.join(TEX, nd.image.name.replace("T_Kit_Trim", "T_KitS_Trim")), check_existing=True)
                nd.image.colorspace_settings.name = "sRGB" if "_BC" in nd.image.name else "Non-Color"
        mats["Stone"] = st
    for key, col, strength in (("Glow", glow, 6.0), ("Screen", (0.06, 0.4, 0.45), 1.5), ("Light", (0.9, 0.95, 1.0), 8.0),
                               ("Beacon", glow, 20.0), ("Pillar", glow, 3.0), ("Decal", (1.0, 0.45, 0.1), 0.5), ("Route", (1.0, 0.05, 0.03), 0.5),
                               ("Eye", (1.0, 1.0, 1.0), 12.0), ("Breach", (0.2, 0.5, 1.0), 6.0), ("Run", (1.0, 0.2, 0.15), 6.0),
                               ("Flicker", (1.0, 0.72, 0.42), 4.0), ("Work", (1.0, 0.9, 0.7), 8.0)):
        g = bpy.data.materials.new("M_Kit_" + key)
        b = next(n for n in g.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
        b.inputs["Base Color"].default_value = (0.02, 0.025, 0.03, 1.0)
        b.inputs["Roughness"].default_value = 0.15
        b.inputs["Emission Color"].default_value = (col[0], col[1], col[2], 1.0)
        b.inputs["Emission Strength"].default_value = strength
        g.use_backface_culling = True
        mats[key] = g
    return mats


def build_all():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    coll = bpy.data.collections.new("Kit")
    bpy.context.scene.collection.children.link(coll)
    mats = make_materials()
    rows, objs, pieces = [], {}, {}
    for fn in {"dominion": kit_pieces_d.BUILDERS, "space": kit_pieces_space.BUILDERS}.get(SET, kit_pieces.BUILDERS):
        p = fn()
        lo, hi = p.bounds()
        size = tuple(round(hi[i] - lo[i], 2) for i in range(3))
        ok = all(abs(lo[i] - p.expect[0][i]) < 0.6 and abs(hi[i] - p.expect[1][i]) < 0.6 for i in range(3)) if p.expect else True
        obj = p.build(mats, coll)
        if SET == "space":                       # seen from 50 m to 2 km: a 15 cm bevel, no global clamp
            kit_lib.finish(obj, bevel_cm=15.0, clamp=False)
        else:
            kit_lib.finish(obj, clamp=p.name not in getattr(kit_pieces_d, "NO_CLAMP", ()))
        clamped = kit_lib.bake_and_clamp(obj, lo, hi)
        objs[p.name], pieces[p.name] = obj, p
        dg = bpy.context.evaluated_depsgraph_get()
        ev = obj.evaluated_get(dg)
        tris = sum(len(f.vertices) - 2 for f in ev.data.polygons)
        row = {"asset": p.name, "min_cm": list(lo), "max_cm": list(hi), "size_cm": list(size),
               "expected": [list(p.expect[0]), list(p.expect[1])] if p.expect else "measured",
               "check": "PASS" if ok else "FAIL", "tris_beveled": tris, "materials": [m.name for m in obj.data.materials],
               "collision_hulls": len(p.hulls), "bevel_verts_clamped": clamped, "uv_notes": sorted(set(p.notes))[:6]}
        rows.append(row)
        print("SIZE %-24s %s  min %s max %s tris %d mats %d hulls %d %s" % (
            p.name, row["check"], lo, hi, tris, len(row["materials"]), len(p.hulls), row["uv_notes"] or ""))
    return objs, rows, mats, pieces


# ------------------------------------------------------------------------------------------------ rendering
def look_at(cam, eye, target):
    cam.location = eye
    d = Vector(target) - Vector(eye)
    cam.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()


def setup_render(scene, w, h, world_strength=1.0, sky=(0.09, 0.095, 0.11)):
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x, scene.render.resolution_y = w, h
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.film_transparent = False
    scene.view_settings.view_transform = "AgX"
    try:
        scene.eevee.taa_render_samples = 24
        scene.eevee.use_raytracing = True
    except Exception:
        pass
    world = scene.world or bpy.data.worlds.new("W")
    scene.world = world
    world.use_nodes = True
    bg = next(n for n in world.node_tree.nodes if n.type == "BACKGROUND")
    bg.inputs["Color"].default_value = (sky[0], sky[1], sky[2], 1.0)
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
    if kind in ("POINT", "AREA"):
        d.shadow_soft_size = size
    o = bpy.data.objects.new("L", d)
    o.location = loc
    if rot:
        o.rotation_euler = rot
    bpy.context.scene.collection.objects.link(o)
    return o


def contact_sheet(objs, tag, clay, cell=420, cols=5):
    scene = bpy.context.scene
    setup_render(scene, cell, cell, 1.4 if clay else 0.9, (0.32, 0.36, 0.42))
    cam_d = bpy.data.cameras.new("C")
    cam = bpy.data.objects.new("C", cam_d)
    scene.collection.objects.link(cam)
    scene.camera = cam
    sun = add_light("SUN", (0, 0, 10), 3.2 if clay else 3.0, rot=(math.radians(50), 0, math.radians(-35)))
    fill = add_light("SUN", (0, 0, 10), 1.0, rot=(math.radians(60), 0, math.radians(140)))
    claymat = bpy.data.materials.new("Clay")
    b = next(n for n in claymat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    b.inputs["Base Color"].default_value = (0.55, 0.55, 0.56, 1)
    b.inputs["Roughness"].default_value = 0.55
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
        rad = max((hi - lo).length / 2, 0.6)
        cam_d.lens = 35
        if name == "SM_Kit_Roof_400":
            look_at(cam, c + Vector((-1.0, -0.8, -0.9)).normalized() * rad * 2.6, c)
        else:
            look_at(cam, c + Vector((1.0, -0.85, 0.62)).normalized() * rad * 2.7, c)
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
    out = os.path.join(REVIEW, "kit%s_%s_%s.png" % ({"dominion": "D", "space": "S"}.get(SET, ""), tag, "clay" if clay else "material"))
    save_px(sheet, out)
    if os.path.exists(tmp):
        os.remove(tmp)
    for o in (cam, sun, fill):
        bpy.data.objects.remove(o, do_unlink=True)
    for o in objs.values():
        o.hide_render = False
    print("SHEET", out, "order:", ", ".join(names))
    return out


def figure(coll, at, yaw=0.0):
    """A 180 cm stand-in (render only)."""
    m = bpy.data.materials.new("Fig")
    b = next(n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    b.inputs["Base Color"].default_value = (0.85, 0.45, 0.2, 1)
    parts = [((0, -0.1, 0.44), (0.16, 0.16, 0.88)), ((0, 0.1, 0.44), (0.16, 0.16, 0.88)), ((0, 0, 1.17), (0.26, 0.42, 0.6)),
             ((0, 0, 1.68), (0.22, 0.2, 0.24)), ((0, -0.29, 1.12), (0.12, 0.12, 0.64)), ((0, 0.29, 1.12), (0.12, 0.12, 0.64))]
    root = bpy.data.objects.new("Figure", None)
    coll.objects.link(root)
    root.location = at
    root.rotation_euler = (0, 0, yaw)
    for c, s in parts:
        me = bpy.data.meshes.new("fig")
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
    """A 2 x 2 room (doorway, window, corner, roof, console), a 100 cm platform reached by the ramp and the stairs, and a
    catwalk off the platform: the kit's seams and the pilot's scale in one test. Unreal cm in, Blender Y mirrored."""
    scene = bpy.context.scene
    for o in objs.values():
        o.hide_render = True
    coll = bpy.data.collections.new("Assembly")
    scene.collection.children.link(coll)

    def put(name, x, y, z=0.0, yaw=0.0):
        o = bpy.data.objects.new("A_" + name, objs[name].data)
        coll.objects.link(o)
        o.location = (x / 100.0, -y / 100.0, z / 100.0)
        o.rotation_euler = (0, 0, math.radians(-yaw))
        return o
    roofs = []
    for x in (-200, 200):
        for y in (-200, 200):
            put("SM_Kit_Floor_400", x, y)
            roofs.append(put("SM_Kit_Roof_400", x, y))
    put("SM_Kit_DoorFrame_400", 400, -200)          # front (+X) wall: doorway and a plain wall
    put("SM_Kit_Wall_400", 400, 200)
    put("SM_Kit_Wall_400", -400, -200)
    put("SM_Kit_Wall_400_Window", -400, 200)
    put("SM_Kit_Wall_400_Window", -200, -400, yaw=90)
    put("SM_Kit_Wall_400", 200, -400, yaw=90)
    put("SM_Kit_Wall_400", -200, 400, yaw=90)
    put("SM_Kit_Wall_400", 200, 400, yaw=90)
    for (cx, cy) in ((400, -400), (400, 400), (-400, -400), (-400, 400)):
        put("SM_Kit_Corner", cx, cy)
    put("SM_Kit_Console", -340, -100, yaw=0)
    put("SM_Kit_Pillar", 0, 0)
    # outside: platform with ramp and stairs, catwalk
    put("SM_Kit_Platform_400", 1000, -200)
    put("SM_Kit_Ramp_400", 1000, -600, yaw=90)        # rises toward +Y onto the platform's -Y edge
    put("SM_Kit_Stairs_400", 600, -200)                # rises toward +X onto the platform's -X edge
    put("SM_Kit_Catwalk_400", 1000, 200, 100, yaw=90)  # off the platform's +Y edge
    put("SM_Kit_LightStrip_200", 400 + 14, 200, 250, yaw=90)
    figs = [figure(coll, (6.0, 2.0, 0.25), 0.0), figure(coll, (-1.4, 1.0, 0.0), 0.4), figure(coll, (10.0, -2.5, 1.0), 1.2)]
    lights = [add_light("POINT", (x / 100.0, -y / 100.0, 2.7), 300, (0.85, 0.95, 1.0), 0.4) for x in (-200, 200) for y in (-200, 200)]
    sun = add_light("SUN", (0, 0, 20), 3.2, rot=(math.radians(55), 0, math.radians(-125)))
    cam_d = bpy.data.cameras.new("C")
    cam_d.lens = 20
    cam = bpy.data.objects.new("C", cam_d)
    scene.collection.objects.link(cam)
    scene.camera = cam
    setup_render(scene, 1600, 900, 0.9, (0.32, 0.38, 0.45))
    shots = []
    tmp = os.path.join(REVIEW, "_asm.png")
    look_at(cam, (13.5, 6.0, 4.2), (4.0, 0.5, 1.0))           # outside: platform, ramp, stairs, doorway
    render_to(scene, tmp)
    shots.append(load_px(tmp))
    look_at(cam, (3.2, 2.6, 1.7), (-3.6, -0.6, 1.2))          # inside, third person height, toward the window wall
    render_to(scene, tmp)
    shots.append(load_px(tmp))
    for r in roofs:
        r.hide_render = True
    cam_d.lens = 24
    look_at(cam, (-9.0, 12.0, 14.0), (3.0, 0.0, 0.5))         # cutaway from above
    render_to(scene, tmp)
    shots.append(load_px(tmp))
    sheet = np.concatenate(shots, 0)
    out = os.path.join(REVIEW, "kit_%s_assembly.png" % tag)
    save_px(sheet, out)
    os.remove(tmp)
    for o in list(coll.objects) + lights + [cam, sun]:
        bpy.data.objects.remove(o, do_unlink=True)
    bpy.data.collections.remove(coll)
    for o in objs.values():
        o.hide_render = False
    print("SHEET", out)
    return out


def assembly_d(objs, tag):
    """A slice of T5's course: hangar floor + pad + walls, the hangar doorway (door half raised), three corridor cells,
    a doorway and a 2 x 3 room with cover, ceilings; a 180 cm figure. Unreal cm in (Blender Y mirrored)."""
    scene = bpy.context.scene
    for o in objs.values():
        o.hide_render = True
    coll = bpy.data.collections.new("AssemblyD")
    scene.collection.children.link(coll)

    def put(name, x, y, z=0.0, yaw=0.0):
        o = bpy.data.objects.new("A_" + name, objs[name].data)
        coll.objects.link(o)
        o.location = (x / 100.0, -y / 100.0, z / 100.0)
        o.rotation_euler = (0, 0, math.radians(-yaw))
        return o
    ceils = []
    for x in (-600, -200):                         # hangar slice X -800..0, Y -600..600
        for y in (-400, 0, 400):
            put("SM_DI_Room_Floor", x, y)
            ceils.append(put("SM_DI_Room_Ceiling", x, y))
    put("SM_DI_Hangar_Pad", -600, 0, 0.5)
    for y in (-400, 400):
        put("SM_DI_Room_Wall", 0, y)
    for x in (-600, -200):
        put("SM_DI_Room_Wall", x, -600, yaw=90)
        put("SM_DI_Room_Wall", x, 600, yaw=90)
    put("SM_DI_Doorway", 0, 0)
    put("SM_DI_Door", 0, 0, 250)
    for x in (200, 600, 1000):
        put("SM_DI_Corridor_Straight", x, 0)
    put("SM_DI_Doorway", 1200, 0)
    for x in (1400, 1800):
        for y in (-400, 0, 400):
            put("SM_DI_Room_Floor", x, y)
            ceils.append(put("SM_DI_Room_Ceiling", x, y))
    for y in (-400, 400):
        put("SM_DI_Room_Wall", 1200, y)
        put("SM_DI_Room_Wall", 2000, y)
    put("SM_DI_Room_Wall", 2000, 0)
    for x in (1400, 1800):
        put("SM_DI_Room_Wall", x, -600, yaw=90)
        put("SM_DI_Room_Wall", x, 600, yaw=90)
    put("SM_DI_Cover_Low", 1600, -250)
    put("SM_DI_Cover_Low", 1750, 250)
    figure(coll, (-2.0, 0.6, 0.0), 0.0)
    figure(coll, (6.0, 0.4, 0.0), 0.0)
    figure(coll, (16.0, -1.0, 0.0), 0.3)
    lights = [add_light("POINT", (x / 100.0, 0, 5.4), 600, (1.0, 0.35, 0.25), 0.5) for x in (-400, 200, 600, 1000, 1600)]
    cam_d = bpy.data.cameras.new("C")
    cam_d.lens = 18
    cam = bpy.data.objects.new("C", cam_d)
    scene.collection.objects.link(cam)
    scene.camera = cam
    setup_render(scene, 1600, 900, 0.25, (0.05, 0.05, 0.06))
    shots = []
    tmp = os.path.join(REVIEW, "_asm.png")
    look_at(cam, (-7.0, 3.5, 2.6), (2.0, 0.0, 1.6))          # hangar toward the doorway
    render_to(scene, tmp)
    shots.append(load_px(tmp))
    look_at(cam, (2.0, 0.9, 1.9), (12.0, 0.0, 1.5))           # in the corridor (third-person height)
    render_to(scene, tmp)
    shots.append(load_px(tmp))
    for c in ceils:
        c.hide_render = True
    cam_d.lens = 24
    look_at(cam, (-6.0, 14.0, 16.0), (7.0, 0.0, 0.5))         # cutaway from above
    for l in lights:
        l.data.energy *= 2.0
    render_to(scene, tmp)
    shots.append(load_px(tmp))
    out = os.path.join(REVIEW, "kit%s_assembly.png" % tag)
    save_px(np.concatenate(shots, 0), out)
    os.remove(tmp)
    for o in list(coll.objects) + lights + [cam]:
        bpy.data.objects.remove(o, do_unlink=True)
    bpy.data.collections.remove(coll)
    for o in objs.values():
        o.hide_render = False
    print("SHEET", out)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--tag", default="r0")
    ap.add_argument("--render", action="store_true")
    ap.add_argument("--export", action="store_true")
    ap.add_argument("--set", default="planet", choices=("planet", "dominion", "space"))
    a = ap.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
    global SET, EXPORT
    SET = a.set
    if SET == "dominion":
        EXPORT = os.path.join(KIT, "export_dominion")
        a.tag = "D_" + a.tag
    elif SET == "space":
        EXPORT = os.path.join(KIT, "export_space")
        a.tag = "S_" + a.tag
    os.makedirs(REVIEW, exist_ok=True)
    objs, rows, mats, pieces = build_all()
    with open(os.path.join(REVIEW, "size_check_%s.json" % a.tag), "w", encoding="utf-8") as fh:
        json.dump(rows, fh, indent=1)
    fails = [r["asset"] for r in rows if r["check"] != "PASS"]
    print("SIZECHECK %d pieces, %d FAIL %s, total tris %d" % (len(rows), len(fails), fails, sum(r["tris_beveled"] for r in rows)))
    if a.render:
        contact_sheet(objs, a.tag, True)
        contact_sheet(objs, a.tag, False)
        if SET == "planet":
            assembly(objs, a.tag)
        elif SET == "dominion":
            assembly_d(objs, a.tag)
    if a.export and not fails:
        rep = runpy.run_path(os.path.join(SKILLS, "blender-mcp", "scripts", "asset_report.py"))["main"](targets=list(objs))
        with open(os.path.join(REVIEW, "health_%s.json" % a.tag), "w", encoding="utf-8") as fh:
            json.dump(rep, fh, indent=1, default=str)
        ex = runpy.run_path(os.path.join(SKILLS, "blender-unreal-export", "scripts", "export_for_unreal.py"))["main"]
        os.makedirs(EXPORT, exist_ok=True)
        for name in objs:
            try:
                r = ex(root=name, out_dir=EXPORT, front_axis="+X", collision="auto" if name not in kit_pieces.NO_COLLISION else "none")
            except AttributeError:
                with open(os.path.join(EXPORT, name + ".manifest.json")) as fh:
                    r = json.load(fh)
            print("EXPORT %s tris %s size %s collision %d warn %s" % (name, r.get("tris"), r.get("size_cm"), len(r.get("collision") or []), r.get("warnings")))
        x = 0.0
        for name, o in objs.items():
            w = o.dimensions.y
            o.location = (0, x + w / 2, 0)
            x += w + 1.0
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(KIT, {"dominion": "KitDominion.blend", "space": "KitSpace.blend"}.get(SET, "Kit.blend")), compress=True)
    print("__RESULT__ done")


main()

# SPDX-License-Identifier: MIT
"""
sky_sphere_review.py - builds SM_TD_SkySphere (inverted UV sphere, radius 100 cm, equirect UVs), exports it,
and reviews T_TD_NebulaSky from INSIDE that sphere: six 90-degree-FOV renders (Unreal yaw 0 / 90 / 180 / -90
at pitch 0, then straight up and straight down) through the mesh's own UVs, so the views prove the mesh UVs,
the texture and the documented sun direction together.

    blender --background --factory-startup --python sky_sphere_review.py -- --sky <T_TD_NebulaSky.png> --tag r3 [--export]

UV convention (same as the texture):  u = Yaw/360 + 0.5,  Blender v = 0.5 + Pitch/180 (FBX flips it, so in
Unreal V = 0.5 - Pitch/180, top row = straight up). Seam at Yaw = +-180 (duplicated vertices).
"""

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
SKYDIR = os.path.dirname(HERE)
REPO = os.path.abspath(os.path.join(SKYDIR, "..", "..", ".."))
SK = os.path.join(REPO, ".claude", "skills")


def ue_dir(yaw, pitch):
    y, p = math.radians(yaw), math.radians(pitch)
    return Vector((math.cos(p) * math.cos(y), -math.cos(p) * math.sin(y), math.sin(p)))


def build_sphere(n_lon=128, n_lat=64, radius=1.0):
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    rows = []
    for j in range(1, n_lat):  # skip poles
        pitch = -90.0 + 180.0 * j / n_lat
        row = []
        for i in range(n_lon + 1):
            yaw = -180.0 + 360.0 * i / n_lon
            row.append((bm.verts.new(ue_dir(yaw, pitch) * radius), i / n_lon, 0.5 + pitch / 180.0))
        rows.append(row)
    south = [bm.verts.new((0, 0, -radius)) for _ in range(n_lon)]
    north = [bm.verts.new((0, 0, radius)) for _ in range(n_lon)]

    def face(vs):
        # vs = [(vert, u, v)] given counter-clockwise seen from OUTSIDE; reverse -> normals point inward
        vs = list(reversed(vs))
        f = bm.faces.new([x[0] for x in vs])
        for loop, (_, u, v) in zip(f.loops, vs):
            loop[uvl].uv = (u, v)
        return f

    for j in range(len(rows) - 1):
        a, b = rows[j], rows[j + 1]
        for i in range(n_lon):
            # yaw increases toward -Blender Y (clockwise seen from +Z), so outward CCW order is a[i+1], a[i], b[i], b[i+1]
            face([a[i + 1], a[i], b[i], b[i + 1]])
    for i in range(n_lon):
        uc = (i + 0.5) / n_lon
        face([rows[0][i], rows[0][i + 1], (south[i], uc, 0.0)])
        face([rows[-1][i + 1], rows[-1][i], (north[i], uc, 1.0)])
    me = bpy.data.meshes.new("SM_TD_SkySphere")
    bm.to_mesh(me)
    bm.free()
    for p in me.polygons:
        p.use_smooth = True
    obj = bpy.data.objects.new("SM_TD_SkySphere", me)
    coll = bpy.data.collections.new("Sky")
    bpy.context.scene.collection.children.link(coll)
    coll.objects.link(obj)
    # check orientation: every face normal must point at the centre
    inward = sum(1 for p in me.polygons if p.normal.dot(p.center) < 0)
    return obj, inward, len(me.polygons)


def sky_material(obj, png):
    m = bpy.data.materials.new("M_NebulaSky_BlenderPreview")
    nt = m.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    em = nt.nodes.new("ShaderNodeEmission")
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.interpolation = "Linear"
    tex.extension = "REPEAT"
    if png:
        tex.image = bpy.data.images.load(png)
    uv = nt.nodes.new("ShaderNodeUVMap")
    uv.uv_map = "UVMap"
    nt.links.new(uv.outputs[0], tex.inputs[0])
    nt.links.new(tex.outputs[0], em.inputs[0])
    em.inputs[1].default_value = 1.0
    nt.links.new(em.outputs[0], out.inputs[0])
    obj.data.materials.append(m)
    return m


def render_views(obj, out_png, cell=720):
    sc = bpy.context.scene
    sc.render.engine = "CYCLES"
    sc.cycles.samples = 6
    sc.cycles.use_denoising = False
    sc.cycles.max_bounces = 0
    sc.view_settings.view_transform = "Standard"
    sc.view_settings.look = "None"
    sc.render.resolution_x = sc.render.resolution_y = cell
    sc.render.image_settings.file_format = "PNG"
    w = bpy.data.worlds.new("black")
    sc.world = w
    w.node_tree.nodes["Background"].inputs[1].default_value = 0.0
    obj.scale = (50.0, 50.0, 50.0)
    cd = bpy.data.cameras.new("cam")
    cd.lens_unit = "FOV"
    cd.angle = math.radians(90.0)
    cd.clip_start = 0.1
    cd.clip_end = 1000.0
    cam = bpy.data.objects.new("cam", cd)
    sc.collection.objects.link(cam)
    sc.camera = cam
    views = [("yaw0", 0, 0), ("yaw90", 90, 0), ("yaw180", 180, 0), ("yaw-90", -90, 0), ("up", 0, 89.999), ("down", 0, -89.999)]
    tiles = []
    for name, yaw, pitch in views:
        d = ue_dir(yaw, pitch)
        upv = "Y" if abs(pitch) < 80 else "Y"
        cam.rotation_euler = d.to_track_quat("-Z", upv).to_euler()
        if abs(pitch) > 80:
            # look up/down with Unreal yaw 0 (Blender +X) at the top of the frame
            cam.rotation_euler = (math.pi, 0.0, math.radians(90.0)) if pitch > 0 else (0.0, 0.0, math.radians(-90.0))
        p = out_png[:-4] + "_" + name + ".png"
        sc.render.filepath = p
        bpy.ops.render.render(write_still=True)
        img = bpy.data.images.load(p)
        a = np.empty(cell * cell * 4, dtype=np.float32)
        img.pixels.foreach_get(a)
        bpy.data.images.remove(img)
        os.remove(p)
        tiles.append(a.reshape(cell, cell, 4))
    gap = 8
    W = 3 * cell + 4 * gap
    H = 2 * cell + 3 * gap
    sheet = np.full((H, W, 4), 0.15, dtype=np.float32)
    sheet[:, :, 3] = 1
    for k, t in enumerate(tiles):
        r, c = divmod(k, 3)
        y0 = H - gap - (r + 1) * cell - r * gap  # Blender rows bottom-up: first row on top
        x0 = gap + c * (cell + gap)
        sheet[y0:y0 + cell, x0:x0 + cell] = t
    img = bpy.data.images.new("sheet", W, H)
    img.pixels.foreach_set(sheet.ravel())
    img.filepath_raw = out_png
    img.file_format = "PNG"
    img.save()
    obj.scale = (1, 1, 1)
    return [v[0] for v in views]


def main():
    import argparse
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    ap = argparse.ArgumentParser()
    ap.add_argument("--sky", default="")
    ap.add_argument("--tag", default="r1")
    ap.add_argument("--export", action="store_true")
    ap.add_argument("--noviews", action="store_true")
    a = ap.parse_args(argv)
    bpy.ops.wm.read_homefile(use_empty=True)
    obj, inward, nfaces = build_sphere()
    bpy.context.view_layer.update()
    sky_material(obj, a.sky)
    res = {"faces": nfaces, "inward_faces": inward, "tris": sum(len(p.vertices) - 2 for p in obj.data.polygons)}
    review = os.path.join(SKYDIR, "review")
    os.makedirs(review, exist_ok=True)
    if a.export:
        ar = runpy.run_path(os.path.join(SK, "blender-mcp", "scripts", "asset_report.py"))
        rep = ar["main"](targets=[obj.name], tri_budget=20000)
        res["asset_report_totals"] = rep["totals"]
        res["asset_report_issues"] = rep["issues"]
        exp_dir = os.path.join(SKYDIR, "SM_TD_SkySphere")
        os.makedirs(exp_dir, exist_ok=True)
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(exp_dir, "SM_TD_SkySphere.blend"), compress=True)
        ex = runpy.run_path(os.path.join(SK, "blender-unreal-export", "scripts", "export_for_unreal.py"))
        try:
            man = ex["main"](root=obj.name, out_dir=exp_dir, front_axis="+X", collision="none")
        except AttributeError as err:
            # export_for_unreal.py restores the selection in a finally block and trips over a stale view-layer
            # base (None) after its temporary objects are removed. The FBX and manifest are already written by
            # then, so read the manifest back.
            print("note: export cleanup error ignored:", err)
            with open(os.path.join(exp_dir, "SM_TD_SkySphere.manifest.json"), encoding="utf-8") as fh:
                man = json.load(fh)
            man["status"] = "ok" if not man.get("errors") else "error"
        res["export"] = {k: man.get(k) for k in ("status", "tris", "size_cm", "warnings", "errors")}
        with open(os.path.join(review, "SM_TD_SkySphere_health.json"), "w") as fh:
            json.dump(res, fh, indent=2, default=str)
    if a.sky and not a.noviews:
        res["views"] = render_views(obj, os.path.join(review, "T_TD_NebulaSky_{}_inside_views.png".format(a.tag)))
    print("__RESULT__" + json.dumps(res, default=str))


main()

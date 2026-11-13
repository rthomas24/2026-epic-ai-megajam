# SPDX-License-Identifier: MIT
"""
dm_build.py - builds the Dominion exterior kit headless, reviews it assembled, exports each mesh.

    blender --background --factory-startup --python dm_build.py -- --tag r1 [--stage blockout|full] [--no-export]

Writes review/Dominion_<tag>_clay_sheet.png (assembled, clay) and review/Dominion_<tag>_views.png (EEVEE, game FOV
90 deg, nebula: 2 km approach, 700 m side, bow 250 m, trench run, flank skim 60 m, stern 400 m), and for --stage full
Dominion.blend, SM_Dominion_*/SM_Dominion_*.fbx + .manifest.json, review/Dominion_<tag>_health.json.
"""

import json
import math
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import bmesh  # noqa: E402
import bpy  # noqa: E402
import numpy as np  # noqa: E402
from mathutils import Matrix, Vector  # noqa: E402

import dm_lib as L  # noqa: E402
import dm_ships as S  # noqa: E402

T = S.UE_TO_BL


def clear_scene():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)


def box_uv(obj, tile=8.0):
    me = obj.data
    bm = bmesh.new()
    bm.from_mesh(me)
    uv = bm.loops.layers.uv.verify()
    for f in bm.faces:
        n = f.normal
        ax = max(range(3), key=lambda i: abs(n[i]))
        a, b = [(1, 2), (0, 2), (0, 1)][ax]
        for lp in f.loops:
            co = lp.vert.co
            lp[uv].uv = (co[a] / tile, co[b] / tile)
    bm.to_mesh(me)
    bm.free()


def ue_place(p, mirror_y=False):
    A = Matrix.Translation(Vector(p))
    if mirror_y:
        A = A @ Matrix.Diagonal((1.0, -1.0, 1.0, 1.0))
    return T @ A @ T


def standins(coll, mats):
    out = []
    bm = bmesh.new()
    c, r = S.REACTOR
    bmesh.ops.create_uvsphere(bm, u_segments=32, v_segments=16, radius=r, matrix=Matrix.Translation(c))
    o = S.to_bl(bm, coll)
    o.data.materials.append(mats["GlowEngine"])
    o.name = "STANDIN_Reactor"
    out.append(o)
    for i, p in enumerate(S.TURRETS):
        o = S.to_bl(S.cyl_bm(3.0, 4.0, 16, "Z", p), coll)
        o.data.materials.append(mats["Trim"])
        o.name = "STANDIN_Turret_%d" % i
        out.append(o)
    # a 6 m enemy fighter at the hangar launch point, for scale
    h = S.HANGAR
    bm = S.loft3_bm([[(h.x - 3, h.y - 2.5, h.z), (h.x - 3, h.y + 2.5, h.z), (h.x - 3, h.y, h.z + 1.0)],
                     [(h.x + 3, h.y - 0.2, h.z), (h.x + 3, h.y + 0.2, h.z), (h.x + 3, h.y, h.z + 0.3)]])
    o = S.to_bl(bm, coll)
    o.data.materials.append(mats["GlowRed"])
    o.name = "STANDIN_Fighter6m"
    out.append(o)
    return out


VIEWS = [  # name, camera (UE m), target (UE m)
    ("approach_2km", (1500.0, 1150.0, 650.0), (0.0, 0.0, 0.0)),
    ("side_700m", (-120.0, 690.0, 160.0), (0.0, 0.0, 10.0)),
    ("bow_250m", (470.0, 170.0, 40.0), (290.0, 0.0, 0.0)),
    ("trench_run", (-150.0, 0.0, 66.0), (200.0, 0.0, 60.0)),
    ("flank_skim_60m", (-70.0, 128.0, 30.0), (90.0, 80.0, 12.0)),
    ("stern_400m", (-640.0, 270.0, 160.0), (-260.0, 0.0, 0.0)),
]


LOWBOW_VIEWS = [  # name, camera (UE m), target (UE m), lens mm (36 mm sensor): the intro's framings + a belly view
    ("bow_135mm_low_3q", (2350.0, 1350.0, -760.0), (150.0, 0.0, -20.0), 135.0),
    ("under_bow_24mm", (440.0, 120.0, -150.0), (180.0, 0.0, -50.0), 24.0),
    ("belly_400m", (-40.0, 260.0, -420.0), (0.0, 0.0, -30.0), 18.0),
]


def render_views(path, cell=(960, 540), views=None):
    scene = bpy.context.scene
    world = scene.world or bpy.data.worlds.new("World")
    scene.world = world
    L._nebula_world(world)
    cam_d = bpy.data.cameras.new("__cam")
    cam = bpy.data.objects.new("__cam", cam_d)
    scene.collection.objects.link(cam)
    scene.camera = cam
    cam_d.sensor_fit = "HORIZONTAL"
    cam_d.angle = math.radians(90.0)
    cam_d.clip_start = 0.5
    cam_d.clip_end = 8000.0
    sun_d = bpy.data.lights.new("__sun", "SUN")
    sun_d.energy = 4.0
    sun = bpy.data.objects.new("__sun", sun_d)
    scene.collection.objects.link(sun)
    sun.rotation_euler = (T @ Vector((-0.5, 0.6, -0.62))).to_track_quat("-Z", "Y").to_euler()
    r = scene.render
    r.engine = "BLENDER_EEVEE"
    try:
        scene.eevee.taa_render_samples = 24
    except Exception:
        pass
    r.resolution_x, r.resolution_y = cell
    r.resolution_percentage = 100
    r.film_transparent = False
    r.image_settings.file_format = "PNG"
    try:
        scene.view_settings.view_transform = "Standard"     # AgX washes the saturated glow out; Unreal's tonemapper keeps it
    except Exception:
        pass
    cells = []
    for v in (views or VIEWS):
        name, cp, tp = v[:3]
        if len(v) > 3:
            cam_d.lens_unit = "MILLIMETERS"
            cam_d.sensor_width = 36.0
            cam_d.lens = v[3]
        else:
            cam_d.angle = math.radians(90.0)
        c, t = T @ Vector(cp), T @ Vector(tp)
        cam.location = c
        cam.rotation_euler = (t - c).to_track_quat("-Z", "Y").to_euler()
        p = path[:-4] + "_" + name + ".png"
        r.filepath = p
        bpy.ops.render.render(write_still=True)
        cells.append(L._load_rgba(p))
        os.remove(p)
    w, h = cell
    gap = 6
    rows = (len(cells) + 2) // 3
    sheet = np.full((rows * h + (rows + 1) * gap, 3 * w + 4 * gap, 4), 0.06, np.float32)
    sheet[..., 3] = 1.0
    for i, px in enumerate(cells):
        row, col = divmod(i, 3)
        y0 = gap + (rows - 1 - row) * (h + gap)       # Blender rows run bottom-up: first row on top
        x0 = gap + col * (w + gap)
        sheet[y0:y0 + h, x0:x0 + w] = px
    L._save_rgba(sheet, path)
    bpy.data.objects.remove(cam, do_unlink=True)
    bpy.data.objects.remove(sun, do_unlink=True)
    return path


def main():
    import argparse
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser()
    p.add_argument("--tag", default="r1")
    p.add_argument("--stage", default="full", choices=("blockout", "full"))
    p.add_argument("--assets", nargs="*", default=sorted(S.BUILDERS))
    p.add_argument("--no-export", action="store_true")
    p.add_argument("--no-views", action="store_true")
    p.add_argument("--views", default="game", choices=("game", "lowbow", "both"))
    a = p.parse_args(argv)
    t0 = time.time()
    clear_scene()
    mats = S.materials()
    roots, info = {}, {}
    review_coll = L.ensure_collection("Dominion_Review")
    review_objs = []
    for asset in a.assets:
        fn, budget, placements, mirror = S.BUILDERS[asset]
        K = fn(asset, mats, a.stage)
        obj = K.P.join(asset)
        box_uv(obj)
        # drop empty material slots left by the join anchor (remap face indices)
        me = obj.data
        mats_ = list(me.materials)
        if any(m is None for m in mats_):
            keep = [i for i, m in enumerate(mats_) if m is not None]
            remap = {old: new for new, old in enumerate(keep)}
            idx = [remap.get(pl.material_index, 0) for pl in me.polygons]
            me.materials.clear()
            for i in keep:
                me.materials.append(mats_[i])
            me.polygons.foreach_set("material_index", idx)
            me.update()
        used = set(pl.material_index for pl in obj.data.polygons)
        slots = [(i, m.name if m else None, i in used) for i, m in enumerate(obj.data.materials)]
        roots[asset] = obj
        pts = L.mesh_points(obj.data)
        lo, hi = pts.min(axis=0), pts.max(axis=0)
        # report in Unreal cm (x = -bl_y, y = -bl_x)
        ue_lo = [-hi[1] * 100, -hi[0] * 100, lo[2] * 100]
        ue_hi = [-lo[1] * 100, -lo[0] * 100, hi[2] * 100]
        info[asset] = {"tris": L.tri_count(obj.data), "budget": budget,
                       "ue_min_cm": [round(float(v), 1) for v in ue_lo], "ue_max_cm": [round(float(v), 1) for v in ue_hi],
                       "ucx": len(K.ucx), "slots": slots, "glow_area_m2": round(getattr(K, "glow_area", 0.0), 1)}
        hull_names = L.add_hulls(obj, K.ucx, obj.users_collection[0], max_points=48) if a.stage == "full" else []
        info[asset]["ucx_objects"] = len(hull_names)
        for i, pl in enumerate(placements):
            d = bpy.data.objects.new("REV_%s_%d" % (asset, i), obj.data)
            review_coll.objects.link(d)
            d.matrix_world = ue_place(pl, mirror_y=(mirror and i == 1))
            review_objs.append(d)
        obj.hide_render = True
        obj.hide_viewport = True
        for ch in obj.children:
            ch.hide_render = True
    review_objs += standins(review_coll, mats)
    os.makedirs(L.REVIEW_DIR, exist_ok=True)
    label = "Dominion_%s" % a.tag
    clay = L.rv["main"](targets=[o.name for o in review_objs], out_dir=L.REVIEW_DIR, label=label, mode="clay",
                        views=("front_3q", "right", "top", "back_3q", "low_3q", "front"), columns=3, resolution=520)
    res = {"tag": a.tag, "stage": a.stage, "assets": info, "clay": clay.get("sheet")}
    if not a.no_views:
        if a.views in ("game", "both"):
            res["views"] = render_views(os.path.join(L.REVIEW_DIR, label + "_views.png"))
        if a.views in ("lowbow", "both"):
            res["lowbow"] = render_views(os.path.join(L.REVIEW_DIR, label + "_lowbow.png"), views=LOWBOW_VIEWS)
    if a.stage == "full":
        blend = os.path.join(L.KIT, "Dominion.blend")
        for o in roots.values():
            o.hide_viewport = False
            o.hide_render = False
        for o in review_objs:
            o.hide_render = True
        bpy.ops.wm.save_as_mainfile(filepath=blend, compress=True)
        if os.path.exists(blend + "1"):
            os.remove(blend + "1")
        for asset, obj in roots.items():
            rep = L.ar["main"](targets=[asset], tri_budget=S.BUILDERS[asset][1][1])
            asm = L.ac["main"](targets=[asset])
            info[asset]["report_errors"] = rep["totals"]["errors"]
            info[asset]["report_warnings"] = rep["totals"]["warnings"]
            info[asset]["issues"] = rep["issues"][:8]
            info[asset]["assembly_ok"] = asm.get("ok")
            info[asset]["floating"] = (asm.get("floating") or [])[:6]
            if not a.no_export:
                out_dir = os.path.join(L.KIT, asset)
                os.makedirs(out_dir, exist_ok=True)
                man = L.ex["main"](root=asset, out_dir=out_dir, collision="auto")
                info[asset]["export"] = {k: man.get(k) for k in ("status", "tris", "size_cm", "collision", "materials",
                                                                  "warnings", "errors")}
                mp = os.path.join(out_dir, asset + ".manifest.json")
                if os.path.exists(mp):
                    with open(mp, encoding="utf-8") as fh:
                        m = json.load(fh)
                    m["nanite"] = True
                    m["fbx_relative"] = asset + ".fbx"
                    m["ue_extent_cm"] = {"min": info[asset]["ue_min_cm"], "max": info[asset]["ue_max_cm"]}
                    with open(mp, "w", encoding="utf-8") as fh:
                        json.dump(m, fh, indent=2)
        with open(os.path.join(L.REVIEW_DIR, label + "_health.json"), "w", encoding="utf-8") as fh:
            json.dump(res, fh, indent=2, default=str)
    res["seconds"] = round(time.time() - t0, 1)
    print("__RESULT__" + json.dumps(res, default=str))


if __name__ == "__main__":
    main()

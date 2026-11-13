# SPDX-License-Identifier: MIT
"""
dm_build_parts.py - builds the Dominion finale objectives (generator, generator core, reactor) headless, reviews them
in place on the Dominion (hull + trench walls appended from ../Dominion.blend) against the nebula at gameplay
distances, and exports each mesh. The hull kit (dm_build.py, Dominion.blend) is read, never written.

    blender --background --factory-startup --python dm_build_parts.py -- --tag p1 [--stage blockout|full] [--no-export]

Writes review/Parts_<tag>_{generator,reactor}_clay.png and _views.png, and for --stage full
DominionParts.blend, SM_Dominion_<Part>/SM_Dominion_<Part>.fbx + .manifest.json, review/Parts_<tag>_health.json.
"""

import json
import math
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

import bpy  # noqa: E402
from mathutils import Vector  # noqa: E402

import dm_build as B  # noqa: E402
import dm_lib as L  # noqa: E402
import dm_parts as P  # noqa: E402

GEN_POS = Vector((math.cos(math.radians(45.0)) * 388.8, math.sin(math.radians(45.0)) * 388.8, 0.0))
REACTOR_POS = Vector((165.0, 0.0, 65.0))
CORE_ATTACH = Vector((0.0, 0.0, P.CORE_ATTACH_Z))

G = GEN_POS
R = REACTOR_POS
GEN_VIEWS = [   # name, camera (UE m, Dominion frame), target
    ("gen_45m_3q", tuple(G + Vector((30.0, 30.0, 9.0))), tuple(G + Vector((0.0, 0.0, 3.0)))),
    ("gen_120m_low", tuple(G + Vector((105.0, -40.0, -45.0))), tuple(G)),
    ("gen_300m", tuple(G + Vector((230.0, 170.0, 60.0))), tuple(G)),
    ("gen_1000m", tuple(G + Vector((720.0, 650.0, 220.0))), tuple(G)),
    ("gen_200m_hull_bg", tuple(G + Vector((150.0, 130.0, 25.0))), tuple(G + Vector((-40.0, -40.0, 0.0)))),
    ("gen_30m_above", tuple(G + Vector((14.0, -18.0, 30.0))), tuple(G + Vector((0.0, 0.0, 10.0)))),
]
REACTOR_VIEWS = [
    ("trench_run_300m", (-135.0, 0.0, 88.0), tuple(R)),
    ("above_150m", (70.0, 70.0, 170.0), tuple(R)),
    ("approach_60m", (115.0, 14.0, 95.0), tuple(R)),
    ("top_100m", (185.0, 6.0, 165.0), tuple(R)),
    ("trench_side_45m", (128.0, 18.0, 72.0), tuple(R)),
    ("low_front_80m", (240.0, -10.0, 72.0), tuple(R)),
]


def append_context(coll):
    """Hull and trench walls from the kit's Dominion.blend, placed as in the game (read-only)."""
    path = os.path.join(L.KIT, "Dominion.blend")
    want = ["SM_Dominion_Hull", "SM_Dominion_TrenchWall", "SM_Dominion_Bridge", "SM_Dominion_ShieldEmitter"]
    with bpy.data.libraries.load(path, link=False) as (src, dst):
        dst.objects = [n for n in src.objects if n in want]
    import dm_ships as S
    out = []
    place = {"SM_Dominion_Hull": [(Vector((0, 0, 0)), False)],
             "SM_Dominion_TrenchWall": [(S.WALL_C, False), (Vector((S.WALL_C.x, -S.WALL_C.y, S.WALL_C.z)), True)],
             "SM_Dominion_Bridge": [(S.BRIDGE_C, False)],
             "SM_Dominion_ShieldEmitter": [(e, False) for e in S.EMITTERS]}
    for o in dst.objects:
        if o is None:
            continue
        for i, (p, mir) in enumerate(place.get(o.name, [])):
            d = bpy.data.objects.new("CTX_%s_%d" % (o.name, i), o.data)
            coll.objects.link(d)
            d.matrix_world = B.ue_place(p, mirror_y=mir)
            out.append(d)
    return out


def main():
    import argparse
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser()
    p.add_argument("--tag", default="p1")
    p.add_argument("--stage", default="full", choices=("blockout", "full"))
    p.add_argument("--assets", nargs="*", default=sorted(P.BUILDERS))
    p.add_argument("--no-export", action="store_true")
    p.add_argument("--no-views", action="store_true")
    a = p.parse_args(argv)
    t0 = time.time()
    B.clear_scene()
    mats = P.materials()
    roots, info = {}, {}
    review_coll = L.ensure_collection("Parts_Review")
    rev = {"generator": [], "reactor": []}
    for asset in a.assets:
        fn, budget = P.BUILDERS[asset]
        K = fn(asset, mats, a.stage)
        obj = K.P.join(asset)
        B.box_uv(obj)
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
        roots[asset] = obj
        pts = L.mesh_points(obj.data)
        lo, hi = pts.min(axis=0), pts.max(axis=0)
        ue_lo = [-hi[1] * 100, -hi[0] * 100, lo[2] * 100]
        ue_hi = [-lo[1] * 100, -lo[0] * 100, hi[2] * 100]
        # material area shares
        area = {}
        for pl in me.polygons:
            n = me.materials[pl.material_index].name if me.materials else "?"
            area[n] = area.get(n, 0.0) + pl.area
        tot = sum(area.values()) or 1.0
        info[asset] = {"tris": L.tri_count(me), "budget": budget,
                       "ue_min_cm": [round(float(v), 1) for v in ue_lo], "ue_max_cm": [round(float(v), 1) for v in ue_hi],
                       "ucx": len(K.ucx), "slots": [m.name for m in me.materials],
                       "area_share": {k: round(v / tot, 3) for k, v in sorted(area.items())}}
        hull_names = L.add_hulls(obj, K.ucx, obj.users_collection[0], max_points=48) if (a.stage == "full" and K.ucx) else []
        info[asset]["ucx_objects"] = len(hull_names)
        if asset == "SM_Dominion_Reactor":
            places, key = [REACTOR_POS], "reactor"
        elif asset == "SM_Dominion_GeneratorCore":
            places, key = [GEN_POS + CORE_ATTACH], "generator"
        else:
            places, key = [GEN_POS], "generator"
        for i, pl in enumerate(places):
            d = bpy.data.objects.new("REV_%s_%d" % (asset, i), obj.data)
            review_coll.objects.link(d)
            d.matrix_world = B.ue_place(pl)
            rev[key].append(d)
        obj.hide_render = True
        obj.hide_viewport = True
        for ch in obj.children:
            ch.hide_render = True
    os.makedirs(L.REVIEW_DIR, exist_ok=True)
    res = {"tag": a.tag, "stage": a.stage, "assets": info}
    for key, objs in rev.items():
        if not objs:
            continue
        label = "Parts_%s_%s" % (a.tag, key)
        clay = L.rv["main"](targets=[o.name for o in objs], out_dir=L.REVIEW_DIR, label=label, mode="clay",
                            views=("front_3q", "right", "top", "back_3q", "low_3q", "front"), columns=3, resolution=520)
        res[key + "_clay"] = clay.get("sheet")
    if not a.no_views:
        ctx = append_context(review_coll)
        if rev["generator"]:
            res["generator_views"] = B.render_views(os.path.join(L.REVIEW_DIR, "Parts_%s_generator_views.png" % a.tag),
                                                    views=GEN_VIEWS)
        if rev["reactor"]:
            res["reactor_views"] = B.render_views(os.path.join(L.REVIEW_DIR, "Parts_%s_reactor_views.png" % a.tag),
                                                  views=REACTOR_VIEWS)
        for o in ctx:
            bpy.data.objects.remove(o, do_unlink=True)
    if a.stage == "full":
        for o in roots.values():
            o.hide_viewport = False
            o.hide_render = False
        for objs in rev.values():
            for o in objs:
                o.hide_render = True
        blend = os.path.join(L.KIT, "DominionParts.blend")
        bpy.ops.wm.save_as_mainfile(filepath=blend, compress=True)
        if os.path.exists(blend + "1"):
            os.remove(blend + "1")
        for asset, obj in roots.items():
            rep = L.ar["main"](targets=[asset], tri_budget=P.BUILDERS[asset][1][1])
            asm = L.ac["main"](targets=[asset])
            info[asset]["report_errors"] = rep["totals"]["errors"]
            info[asset]["report_warnings"] = rep["totals"]["warnings"]
            info[asset]["issues"] = rep["issues"][:8]
            info[asset]["assembly_ok"] = asm.get("ok")
            info[asset]["floating"] = (asm.get("floating") or [])[:6]
            if not a.no_export:
                out_dir = os.path.join(L.KIT, asset)
                os.makedirs(out_dir, exist_ok=True)
                man = L.ex["main"](root=asset, out_dir=out_dir,
                                   collision="none" if asset == "SM_Dominion_GeneratorCore" else "auto")
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
        with open(os.path.join(L.REVIEW_DIR, "Parts_%s_health.json" % a.tag), "w", encoding="utf-8") as fh:
            json.dump(res, fh, indent=2, default=str)
    res["seconds"] = round(time.time() - t0, 1)
    print("__RESULT__" + json.dumps(res, default=str))


if __name__ == "__main__":
    main()

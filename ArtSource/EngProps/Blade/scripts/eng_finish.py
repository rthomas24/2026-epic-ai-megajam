# SPDX-License-Identifier: MIT
"""
eng_finish.py - finishing pipeline for single engineering props (blade hilt, landing pad).

Uses eng_lib.py (a snapshot of ArtSource/EngProps/SpaceDressing/scripts/sd_lib.py, Nov 13) for the
role materials, UVs, bake, cavity tint and review sheets, but keeps the AUTHORED pivot (no centre-of-mass
recentre) and passes the authoring front axis to the exporter.
"""

import json
import os
import sys
import time

import bpy

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
import eng_lib as L  # noqa: E402


def use_folder(folder):
    """Point the library's output folders at <folder>/<asset> and <folder>/review."""
    L.KIT = folder
    L.REVIEW_DIR = os.path.join(folder, "review")
    os.makedirs(L.REVIEW_DIR, exist_ok=True)


def export(asset, out_dir, front_axis):
    try:
        man = L.ex["main"](root=asset, out_dir=out_dir, collision="auto", front_axis=front_axis)
    except AttributeError as err:
        # export_for_unreal.py's selection restore can trip over a stale view-layer base after it removes its
        # temporary objects; the FBX and manifest are already written by then.
        L.log("note: exporter cleanup error ignored:", err)
        with open(os.path.join(out_dir, asset + ".manifest.json"), encoding="utf-8") as fh:
            man = json.load(fh)
        man["status"] = "ok" if not man.get("errors") else "error"
    return man


def finish(part, asset, tex, tag, budget, front_axis, ao_distance=0.2, tint=(0.62, 0.6, 0.58),
           tint_strength=0.6, sockets=(), hulls=None, extra_manifest=None, samples=48):
    t0 = time.time()
    obj = part.join()
    info = {}
    info["edge_face_share"] = L.mark_edge_faces(obj.data, part.max_bevel or 0.002)
    info["uv_unwrap"] = L.unwrap(obj, tex)
    info["uv"] = L.uv_stats(obj.data, 1024)
    asset_dir = os.path.join(L.KIT, asset)
    os.makedirs(asset_dir, exist_ok=True)
    info["bake"] = L.bake(obj, asset_dir, tex, sources=None, ao_distance=ao_distance, samples=samples)
    info["cavity"] = L.cavity_tint(asset[3:], tint=tint, strength=tint_strength)
    obj.data.materials[0].name = "M_{}".format(asset[3:])
    coll = obj.users_collection[0]
    hull_names = L.add_hulls(obj, hulls if hulls is not None else part.hulls, coll)
    for name, loc in sockets:
        e = bpy.data.objects.new(name, None)
        e.empty_display_type = "ARROWS"
        e.empty_display_size = 0.05
        coll.objects.link(e)
        e.location = loc
        e.parent = obj
    blend_path = os.path.join(asset_dir, asset + ".blend")
    bpy.ops.wm.save_as_mainfile(filepath=blend_path, compress=True)
    if os.path.exists(blend_path + "1"):
        os.remove(blend_path + "1")
    report = L.ar["main"](targets=[asset], tri_budget=budget[1])
    assembly = L.ac["main"](targets=[asset])
    rev = L.review(asset, tag)
    manifest = export(asset, asset_dir, front_axis)
    man_path = os.path.join(asset_dir, asset + ".manifest.json")
    with open(man_path, encoding="utf-8") as fh:
        man = json.load(fh)
    man["fbx_relative"] = asset + ".fbx"
    man["texture_size"] = tex
    man["emission_strength"] = info.get("bake", {}).get("emission_strength", 0.0)
    if extra_manifest:
        man.update(extra_manifest)
    with open(man_path, "w", encoding="utf-8") as fh:
        json.dump(man, fh, indent=2)
    me = obj.data
    pts = L.mesh_points(me)
    size_cm = [round(float(v) * 100.0, 1) for v in (pts.max(axis=0) - pts.min(axis=0))]
    checks = {
        "tris": L.tri_count(me), "tris_in_budget": budget[0] <= L.tri_count(me) <= budget[1],
        "material_slots": len(me.materials), "no_floating_parts": bool(assembly.get("ok")),
        "uv_inside_0_1": info["uv"].get("inside_0_1"), "uv_overlap_pixels": info["uv"].get("overlap_pixels"),
        "asset_report_errors": report["totals"]["errors"], "asset_report_warnings": report["totals"]["warnings"],
        "collision_hulls": len(hull_names), "size_cm_blender_xyz": size_cm, "export_status": manifest.get("status"),
        "bbox_min_m": [round(float(v), 4) for v in pts.min(axis=0)], "bbox_max_m": [round(float(v), 4) for v in pts.max(axis=0)],
    }
    health = {"asset": asset, "tag": tag, "checks": checks, "info": info, "textures": L.image_stats(asset[3:]),
              "emissive_surface_share": L.emissive_surface_share(me, asset[3:]),
              "asset_report_issues": report["issues"],
              "assembly": {k: assembly.get(k) for k in ("ok", "islands", "groups", "floating", "tolerance_m")},
              "review": rev, "export": {k: man.get(k) for k in ("status", "tris", "size_cm", "collision", "sockets",
                                                               "materials", "textures", "warnings", "errors")},
              "seconds": round(time.time() - t0, 1)}
    with open(os.path.join(L.REVIEW_DIR, asset + "_health.json"), "w", encoding="utf-8") as fh:
        json.dump(health, fh, indent=2, default=str)
    L.log("CHECKS", asset, json.dumps(checks), "emissive share", health["emissive_surface_share"])
    L.log("ISSUES", json.dumps(report["issues"][:12]))
    return obj, health

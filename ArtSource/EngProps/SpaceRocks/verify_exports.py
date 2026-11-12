# SPDX-License-Identifier: MIT
"""
verify_exports.py - re-import every exported FBX of the SpaceRocks kit into an empty Blender scene
and MEASURE what an importer will get (nothing is taken from the .blend):

    blender --background --factory-startup --python verify_exports.py

Per asset: triangle count, all faces triangles, closed manifold, bounding-sphere diameter about the
pivot (the sizing contract: 100 cm), pivot at the world origin, object scale, UV layer / range,
material slots, hard edges, the UCX_ hull (name, vertex count, convexity, how far the render mesh
pokes out of it), and the texture files with their pixel sizes read from the PNG headers.
Writes review/export_verification.json and prints one line per asset.
"""

import json
import os
import struct
import sys

import bmesh
import bpy
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ASSETS = ["SM_Asteroid_01", "SM_Asteroid_02", "SM_Asteroid_03", "SM_Asteroid_04",
          "SM_SpaceDebris_01", "SM_SpaceDebris_02", "SM_SpaceDebris_03"]
BUDGET = {"SM_Asteroid": (1500, 3500), "SM_SpaceDebris": (200, 600)}


def png_size(path):
    with open(path, "rb") as fh:
        head = fh.read(24)
    if head[:8] != b"\x89PNG\r\n\x1a\n":
        return None
    return list(struct.unpack(">II", head[16:24]))


def world_points(obj):
    return np.array([obj.matrix_world @ v.co for v in obj.data.vertices], dtype=np.float64)


def verify(asset):
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    folder = os.path.join(HERE, asset)
    fbx = os.path.join(folder, asset + ".fbx")
    bpy.ops.import_scene.fbx(filepath=fbx, use_custom_normals=True)
    meshes = [o for o in bpy.data.objects if o.type == "MESH"]
    render = [o for o in meshes if not o.name.startswith(("UCX_", "UBX_", "USP_", "UCP_"))]
    hulls = [o for o in meshes if o.name.startswith("UCX_")]
    out = {"asset": asset, "fbx_bytes": os.path.getsize(fbx), "objects": sorted(o.name for o in bpy.data.objects)}
    if len(render) != 1:
        out["error"] = "expected exactly one render mesh, found {}".format([o.name for o in render])
        return out
    obj = render[0]
    me = obj.data
    pts = world_points(obj)
    pivot = np.array(obj.matrix_world.translation)
    radius = float(np.sqrt(((pts - pivot) ** 2).sum(axis=1).max()))
    bm = bmesh.new()
    bm.from_mesh(me)
    boundary = sum(1 for e in bm.edges if e.is_boundary)
    nonman = sum(1 for e in bm.edges if not e.is_manifold and not e.is_boundary)
    flipped = sum(1 for e in bm.edges if e.is_manifold and not e.is_contiguous)
    sharp = sum(1 for e in bm.edges if not e.smooth)
    volume = bm.calc_volume(signed=True) if (boundary == 0 and nonman == 0) else None
    loose = sum(1 for v in bm.verts if not v.link_faces)
    # connected pieces (a floating part would be a second piece)
    seen, pieces = set(), 0
    for v in bm.verts:
        if v.index in seen:
            continue
        pieces += 1
        stack = [v]
        while stack:
            w = stack.pop()
            if w.index in seen:
                continue
            seen.add(w.index)
            stack.extend(e.other_vert(w) for e in w.link_edges)
    bm.free()
    uv = np.empty(len(me.loops) * 2, dtype=np.float32)
    if me.uv_layers:
        me.uv_layers.active.data.foreach_get("uv", uv)
    kind = "SM_Asteroid" if asset.startswith("SM_Asteroid") else "SM_SpaceDebris"
    tris = sum(len(p.vertices) - 2 for p in me.polygons)
    textures = {}
    for suffix in ("BC", "N", "ORM", "E"):
        path = os.path.join(folder, "T_{}_{}.png".format(asset[3:], suffix))
        if os.path.exists(path):
            textures[suffix] = {"size": png_size(path), "bytes": os.path.getsize(path)}
    out.update({
        "mesh_name": obj.name,
        "tris": tris,
        "tris_in_budget": BUDGET[kind][0] <= tris <= BUDGET[kind][1],
        "all_triangles": all(len(p.vertices) == 3 for p in me.polygons),
        "closed_manifold": boundary == 0 and nonman == 0,
        "flipped_winding_edges": flipped,
        "outward_normals": bool(volume is not None and volume > 0),
        "loose_verts": loose,
        "connected_pieces": pieces,
        "bounding_sphere_diameter_cm": round(radius * 200.0, 3),
        "pivot_world_m": [round(float(v), 6) for v in pivot],
        "object_scale": [round(float(v), 5) for v in obj.scale],
        "aabb_size_cm": [round(float(v) * 100.0, 2) for v in (pts.max(axis=0) - pts.min(axis=0))],
        "uv_layers": [u.name for u in me.uv_layers],
        "uv_range": [round(float(uv.min()), 4), round(float(uv.max()), 4)] if me.uv_layers else None,
        "material_slots": [s.material.name if s.material else None for s in obj.material_slots],
        "hard_edges": sharp,
        "has_custom_normals": bool(me.has_custom_normals),
        "textures": textures,
        "manifest_present": os.path.exists(os.path.join(folder, asset + ".manifest.json")),
        "blend_present": os.path.exists(os.path.join(folder, asset + ".blend")),
    })
    hull_info = []
    for h in hulls:
        hp = world_points(h)
        hb = bmesh.new()
        hb.from_mesh(h.data)
        hb.transform(h.matrix_world)
        hb.normal_update()
        centre = hp.mean(axis=0)
        worst_concave, protrude = 0.0, 0.0
        for f in hb.faces:
            n = np.array(f.normal)
            p0 = np.array(f.verts[0].co)
            if np.dot(n, p0 - centre) < 0:
                n = -n
            worst_concave = max(worst_concave, float(((hp - p0) @ n).max()))
            protrude = max(protrude, float(((pts - p0) @ n).max()))
        closed = all(e.is_manifold for e in hb.edges)
        hb.free()
        hull_info.append({"name": h.name, "verts": len(h.data.vertices), "closed": closed,
                          "convex": worst_concave < 1e-4, "max_vertex_outside_a_face_plane_cm": round(worst_concave * 100, 4),
                          "render_mesh_pokes_out_by_cm": round(protrude * 100, 2)})
    out["collision"] = hull_info
    want_hull = kind == "SM_Asteroid"
    out["collision_ok"] = (len(hull_info) == 1 and hull_info[0]["convex"] and hull_info[0]["closed"]
                           and hull_info[0]["name"] == "UCX_{}_00".format(asset)) if want_hull else len(hull_info) == 0
    out["pass"] = bool(
        out["tris_in_budget"] and out["all_triangles"] and out["closed_manifold"] and out["outward_normals"]
        and out["flipped_winding_edges"] == 0 and out["loose_verts"] == 0 and out["connected_pieces"] == 1
        and abs(out["bounding_sphere_diameter_cm"] - 100.0) < 0.05 and max(abs(v) for v in out["pivot_world_m"]) < 1e-5
        and len(out["uv_layers"]) == 1 and out["uv_range"][0] >= 0.0 and out["uv_range"][1] <= 1.0
        and len(out["material_slots"]) == 1 and out["collision_ok"] and out["mesh_name"] == asset
        and all(s in textures for s in ("BC", "N", "ORM")))
    return out


def main():
    results = [verify(a) for a in ASSETS]
    path = os.path.join(HERE, "review", "export_verification.json")
    with open(path, "w", encoding="utf-8") as fh:
        json.dump({"blender": bpy.app.version_string, "assets": results}, fh, indent=2)
    for r in results:
        print("VERIFY {asset}: pass={p} tris={t} sphere={d}cm closed={c} pieces={k} uv={uv} mats={m} hull={h} tex={x}".format(
            asset=r["asset"], p=r.get("pass"), t=r.get("tris"), d=r.get("bounding_sphere_diameter_cm"),
            c=r.get("closed_manifold"), k=r.get("connected_pieces"), uv=r.get("uv_range"),
            m=r.get("material_slots"), h=[(c["name"], c["verts"], c["convex"], c["render_mesh_pokes_out_by_cm"])
                                          for c in r.get("collision", [])],
            x={k: v["size"] for k, v in r.get("textures", {}).items()}))
    print("__RESULT__" + json.dumps({"all_pass": all(r.get("pass") for r in results), "file": path}))


if __name__ == "__main__":
    main()

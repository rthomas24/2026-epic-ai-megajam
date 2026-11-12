# SPDX-License-Identifier: MIT
"""
assembly_check.py - find FLOATING parts in a multi-part asset (read-only).

The most common visual failure of code-built assets is "simplistic, floating primitives": a ring,
fin, engine or greeble that looks attached in one view but hovers in space. Renders hide this;
geometry doesn't. This script splits every evaluated mesh (modifiers applied) into connected
islands, builds a world-space BVH per island, and links two islands when they intersect or come
within `tolerance` of each other. Everything not connected to the main body is reported.

Inside Blender (MCP `execute_blender_code` / `execute_blender_code_for_cli`):

    import runpy
    m = runpy.run_path(r"<repo>/.claude/skills/blender-mcp/scripts/assembly_check.py")
    result = m["main"](targets=["SM_Ship_Rift"])          # root object (children included) or collection=

From a shell:

    blender --background asset.blend --python assembly_check.py -- --targets SM_Ship_Rift

Result: {"status", "islands", "groups", "floating": [{"object", "island", "tris", "center", "gap_m"}],
         "ok": bool}. `gap_m` is the distance to the nearest island of the main body.
"""

import json
import sys

import bmesh
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

SKIP_PREFIXES = ("UCX_", "UBX_", "USP_", "UCP_", "SOCKET_", "CUT_")


def _gather(targets, collection):
    objs = []
    if collection:
        objs.extend(bpy.data.collections[collection].all_objects)
    for name in targets or []:
        obj = bpy.data.objects[name]
        objs.append(obj)
        objs.extend(obj.children_recursive)
    seen, out = set(), []
    for o in objs:
        if o.name in seen or o.type != "MESH" or o.name.startswith(SKIP_PREFIXES) or o.hide_render:
            continue
        seen.add(o.name)
        out.append(o)
    return out


def _islands(obj, depsgraph):
    ev = obj.evaluated_get(depsgraph)
    mesh = ev.to_mesh()
    bm = bmesh.new()
    bm.from_mesh(mesh)
    ev.to_mesh_clear()
    bm.transform(obj.matrix_world)
    bm.faces.ensure_lookup_table()
    seen, out = set(), []
    for f in bm.faces:
        if f.index in seen:
            continue
        stack, comp = [f], []
        while stack:
            g = stack.pop()
            if g.index in seen:
                continue
            seen.add(g.index)
            comp.append(g)
            for v in g.verts:
                for h in v.link_faces:
                    if h.index not in seen:
                        stack.append(h)
        vmap, verts, polys = {}, [], []
        for face in comp:
            idx = []
            for v in face.verts:
                if v.index not in vmap:
                    vmap[v.index] = len(verts)
                    verts.append(v.co.copy())
                idx.append(vmap[v.index])
            polys.append(idx)
        lo = Vector((min(c.x for c in verts), min(c.y for c in verts), min(c.z for c in verts)))
        hi = Vector((max(c.x for c in verts), max(c.y for c in verts), max(c.z for c in verts)))
        out.append({"object": obj.name, "island": len(out), "verts": verts,
                    "tris": sum(len(p) - 2 for p in polys), "bvh": BVHTree.FromPolygons(verts, polys),
                    "lo": lo, "hi": hi})
    bm.free()
    return out


def _gap(a, b, tolerance, samples=400):
    # Quick reject on bounding boxes expanded by tolerance.
    for i in range(3):
        if a["lo"][i] - tolerance > b["hi"][i] or b["lo"][i] - tolerance > a["hi"][i]:
            return float("inf")
    if a["bvh"].overlap(b["bvh"]):
        return 0.0
    best = float("inf")
    for src, dst in ((a, b), (b, a)):
        step = max(1, len(src["verts"]) // samples)
        for co in src["verts"][::step]:
            hit = dst["bvh"].find_nearest(co)
            if hit[0] is not None and hit[3] < best:
                best = hit[3]
                if best <= tolerance:
                    return best
    return best


def main(targets=None, collection=None, tolerance=None, min_island_tris=1):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    objs = _gather(targets, collection)
    islands = [isl for o in objs for isl in _islands(o, depsgraph) if isl["tris"] >= min_island_tris]
    if not islands:
        return {"status": "error", "message": "No mesh islands found"}
    lo = Vector((min(i["lo"].x for i in islands), min(i["lo"].y for i in islands), min(i["lo"].z for i in islands)))
    hi = Vector((max(i["hi"].x for i in islands), max(i["hi"].y for i in islands), max(i["hi"].z for i in islands)))
    size = (hi - lo).length
    tol = tolerance if tolerance is not None else max(0.001, size * 0.0015)

    n = len(islands)
    parent = list(range(n))

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i

    for i in range(n):
        for j in range(i + 1, n):
            if find(i) != find(j) and _gap(islands[i], islands[j], tol) <= tol:
                parent[find(i)] = find(j)
    groups = {}
    for i in range(n):
        groups.setdefault(find(i), []).append(i)
    main_group = max(groups.values(), key=lambda g: sum(islands[k]["tris"] for k in g))
    floating = []
    for g in groups.values():
        if g is main_group:
            continue
        for k in g:
            isl = islands[k]
            gap = min(_gap(isl, islands[m], float("inf")) for m in main_group)
            floating.append({"object": isl["object"], "island": isl["island"], "tris": isl["tris"],
                             "center": [round(v, 3) for v in (isl["lo"] + isl["hi"]) * 0.5],
                             "gap_m": round(gap, 4)})
    floating.sort(key=lambda f: -f["tris"])
    return {
        "status": "ok",
        "ok": not floating,
        "tolerance_m": round(tol, 5),
        "objects": [o.name for o in objs],
        "islands": n,
        "groups": len(groups),
        "main_body_tris": sum(islands[k]["tris"] for k in main_group),
        "floating": floating,
        "hint": "Connect each floating island to the body (strut, pylon, overlap it slightly) or delete it."
        if floating else "All parts are connected.",
    }


def _cli():
    import argparse
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser(description="Detect floating parts")
    p.add_argument("--targets", nargs="*")
    p.add_argument("--collection")
    p.add_argument("--tolerance", type=float)
    a = p.parse_args(argv)
    print("__RESULT__" + json.dumps(main(targets=a.targets, collection=a.collection, tolerance=a.tolerance)))


if __name__ == "__main__":
    _cli()

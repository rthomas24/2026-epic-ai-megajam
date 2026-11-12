# SPDX-License-Identifier: MIT
"""
asset_report.py - structured health report for mesh assets (read-only).

Reports, per object, the EVALUATED result (what an exporter will see, modifiers included):
triangle/vertex counts, n-gons, non-manifold / boundary / wire edges, faces with flipped
winding, inside-out closed meshes, zero-area faces, duplicate vertices, loose geometry,
applied/unapplied transforms, origin placement, size in centimetres, UV layers, material
slots, smooth/flat shading, custom normals, modifiers and naming problems.

Inside Blender (MCP `execute_blender_code` / `execute_blender_code_for_cli`):

    import runpy
    m = runpy.run_path(r"<repo>/.claude/skills/blender-mcp/scripts/asset_report.py")
    result = m["main"](targets=["SM_Ship_Rift"], tri_budget=12000)

From a shell:

    blender --background asset.blend --python asset_report.py -- --targets SM_Ship_Rift --tri-budget 12000

Returns a JSON-serialisable dict: {"status", "objects": [...], "totals": {...}, "issues": [...]}.
Each issue is {"severity": "error"|"warn"|"info", "object", "message"}.
"""

import json
import re
import sys

import bmesh
import bpy
from mathutils import Vector

HELPER_PREFIXES = ("UCX_", "UBX_", "USP_", "UCP_", "SOCKET_", "CUT_", "SRC_")
_NUMBER_SUFFIX = re.compile(r"\.\d{3}$")


def _gather(targets, collection, include_helpers):
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
        objs = list(bpy.context.scene.objects)
    seen, out = set(), []
    for obj in objs:
        if obj.name in seen:
            continue
        if not include_helpers and obj.name.startswith(HELPER_PREFIXES):
            continue
        seen.add(obj.name)
        out.append(obj)
    return out


def _origin_placement(lo, hi, tol):
    center = (lo + hi) * 0.5
    size = hi - lo
    t = max(size.length * tol, 1e-6)
    if abs(center.x) < t and abs(center.y) < t and abs(lo.z) < t:
        return "bottom_center"
    if center.length < t:
        return "center"
    if lo.length < t:
        return "min_corner"
    return "custom"


def _mesh_stats(obj, depsgraph, merge_distance):
    ev = obj.evaluated_get(depsgraph)
    mesh = ev.to_mesh()
    try:
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bm.verts.ensure_lookup_table()
        faces = bm.faces
        tris = sum(len(f.verts) - 2 for f in faces)
        ngons = sum(1 for f in faces if len(f.verts) > 4)
        quads = sum(1 for f in faces if len(f.verts) == 4)
        boundary = sum(1 for e in bm.edges if e.is_boundary)
        wire = sum(1 for e in bm.edges if e.is_wire)
        non_manifold = sum(1 for e in bm.edges if not e.is_manifold and not e.is_boundary and not e.is_wire)
        flipped = sum(1 for e in bm.edges if e.is_manifold and not e.is_contiguous)
        loose_verts = sum(1 for v in bm.verts if not v.link_edges)
        zero_area = sum(1 for f in faces if f.calc_area() < 1e-10)
        doubles = len(bmesh.ops.find_doubles(bm, verts=list(bm.verts), dist=merge_distance)["targetmap"])
        closed = boundary == 0 and non_manifold == 0 and wire == 0 and len(faces) > 0
        signed_volume = bm.calc_volume(signed=True) if closed else None
        smooth_faces = sum(1 for f in faces if f.smooth)
        lo = Vector((min(v.co.x for v in bm.verts), min(v.co.y for v in bm.verts), min(v.co.z for v in bm.verts))) if bm.verts else Vector()
        hi = Vector((max(v.co.x for v in bm.verts), max(v.co.y for v in bm.verts), max(v.co.z for v in bm.verts))) if bm.verts else Vector()
        bm.free()
        uv_layers = [uv.name for uv in mesh.uv_layers]
        mat_usage = {}
        for poly in mesh.polygons:
            mat_usage[poly.material_index] = mat_usage.get(poly.material_index, 0) + 1
        return {
            "verts": len(mesh.vertices),
            "faces": len(mesh.polygons),
            "tris": tris,
            "quads": quads,
            "ngons": ngons,
            "boundary_edges": boundary,
            "non_manifold_edges": non_manifold,
            "wire_edges": wire,
            "flipped_winding_edges": flipped,
            "loose_verts": loose_verts,
            "zero_area_faces": zero_area,
            "duplicate_verts": doubles,
            "closed": closed,
            "inside_out": bool(signed_volume is not None and signed_volume < 0),
            "smooth_face_ratio": round(smooth_faces / len(faces), 3) if len(faces) else 0.0,
            "has_custom_normals": bool(getattr(mesh, "has_custom_normals", False)),
            "uv_layers": uv_layers,
            "faces_per_material_index": {str(k): v for k, v in sorted(mat_usage.items())},
            "local_bbox_min": list(lo),
            "local_bbox_max": list(hi),
        }
    finally:
        ev.to_mesh_clear()


def main(targets=None, collection=None, include_helpers=True, tri_budget=None,
         merge_distance=0.0001, origin_tolerance=0.02):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    scale_length = bpy.context.scene.unit_settings.scale_length
    objs = _gather(targets, collection, include_helpers)
    issues, reports = [], []
    total_tris = 0

    def issue(sev, obj, msg):
        issues.append({"severity": sev, "object": obj.name if obj else None, "message": msg})

    for obj in objs:
        rep = {
            "name": obj.name,
            "type": obj.type,
            "parent": obj.parent.name if obj.parent else None,
            "collections": [c.name for c in obj.users_collection],
            "location": [round(v, 5) for v in obj.location],
            "rotation_mode": obj.rotation_mode,
            "rotation_euler_deg": [round(v * 57.29578, 3) for v in obj.rotation_euler],
            "scale": [round(v, 5) for v in obj.scale],
            "dimensions_cm": [round(v * 100.0 * scale_length, 2) for v in obj.dimensions],
            "hide_render": obj.hide_render,
            "modifiers": [{"name": m.name, "type": m.type, "show_render": m.show_render} for m in obj.modifiers],
            "materials": [s.material.name if s.material else None for s in obj.material_slots],
        }
        if _NUMBER_SUFFIX.search(obj.name):
            issue("warn", obj, "Name ends in a .### duplicate suffix; give it a deliberate name")
        if " " in obj.name or not obj.name.isascii():
            issue("warn", obj, "Name contains spaces or non-ASCII characters (bad for engine import)")
        if obj.type == "MESH":
            if any(abs(s - 1.0) > 1e-4 for s in obj.scale):
                issue("warn", obj, "Scale is not applied ({}); apply scale before bevels/booleans/export".format(rep["scale"]))
            if any(s < 0 for s in obj.scale):
                issue("error", obj, "Negative scale flips normals on export; apply it and recalculate normals")
            if obj.parent is None and any(abs(r) > 1e-4 for r in obj.rotation_euler):
                issue("info", obj, "Rotation is not applied; fine if intentional, otherwise apply rotation")
            stats = _mesh_stats(obj, depsgraph, merge_distance)
            rep.update(stats)
            total_tris += stats["tris"]
            lo, hi = Vector(stats["local_bbox_min"]), Vector(stats["local_bbox_max"])
            rep["origin_placement"] = _origin_placement(lo, hi, origin_tolerance)
            rep["local_bbox_min"] = [round(v, 5) for v in lo]
            rep["local_bbox_max"] = [round(v, 5) for v in hi]
            helper = obj.name.startswith(HELPER_PREFIXES)
            if stats["faces"] == 0:
                issue("error", obj, "Mesh has no faces")
            if stats["non_manifold_edges"]:
                issue("warn", obj, "{} non-manifold edges (edges shared by 3+ faces)".format(stats["non_manifold_edges"]))
            if stats["flipped_winding_edges"]:
                issue("error", obj, "{} edges between faces with opposite winding; recalculate normals outside".format(
                    stats["flipped_winding_edges"]))
            if stats["inside_out"]:
                issue("error", obj, "Closed mesh is inside-out (normals point inward); flip/recalculate normals")
            if stats["duplicate_verts"]:
                issue("warn", obj, "{} duplicate vertices within {} m; merge by distance".format(
                    stats["duplicate_verts"], merge_distance))
            if stats["loose_verts"] or stats["wire_edges"]:
                issue("warn", obj, "Loose geometry: {} verts, {} wire edges; delete loose".format(
                    stats["loose_verts"], stats["wire_edges"]))
            if stats["zero_area_faces"]:
                issue("warn", obj, "{} zero-area faces; dissolve degenerate".format(stats["zero_area_faces"]))
            if not helper:
                if stats["ngons"]:
                    issue("info", obj, "{} n-gons; fine if planar, they get triangulated on export".format(stats["ngons"]))
                if not stats["uv_layers"]:
                    issue("error", obj, "No UV map; textures and baking need UVs")
                if not rep["materials"] or None in rep["materials"]:
                    issue("warn", obj, "Missing material slot(s); engine will show the default material")
                if stats["boundary_edges"] and stats["faces"]:
                    issue("info", obj, "{} open boundary edges (mesh is not closed); OK for planes/cards".format(
                        stats["boundary_edges"]))
        reports.append(rep)

    if tri_budget is not None and total_tris > tri_budget:
        issues.append({"severity": "warn", "object": None,
                       "message": "Total {} tris exceeds budget {}".format(total_tris, tri_budget)})
    unit = bpy.context.scene.unit_settings
    return {
        "status": "ok",
        "scene_units": {"system": unit.system, "scale_length": unit.scale_length, "length_unit": unit.length_unit},
        "objects": reports,
        "totals": {"objects": len(reports), "tris": total_tris,
                   "errors": sum(1 for i in issues if i["severity"] == "error"),
                   "warnings": sum(1 for i in issues if i["severity"] == "warn")},
        "issues": issues,
    }


def _cli():
    import argparse
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser(description="Mesh asset health report")
    p.add_argument("--targets", nargs="*")
    p.add_argument("--collection")
    p.add_argument("--tri-budget", type=int)
    a = p.parse_args(argv)
    print("__RESULT__" + json.dumps(main(targets=a.targets, collection=a.collection, tri_budget=a.tri_budget)))


if __name__ == "__main__":
    _cli()

# SPDX-License-Identifier: MIT
"""
cleanup_ai_mesh.py - turn an AI-generated or downloaded model (Hyper3D Rodin, Tripo, Meshy,
Hunyuan3D, Sketchfab, Fab, ...) into a game-ready static mesh for Unreal.

Pipeline:
  1. import GLB/GLTF/FBX/OBJ (or take existing objects) and merge every imported mesh into one
     world-space SOURCE object  SRC_<asset>  (kept hidden, used as the bake source)
  2. working copy: weld duplicate verts (triangle soup), delete loose/degenerate geometry,
     recalculate normals outward
  3. optional up-axis fix, uniform scale to a target height/size in cm, origin at bottom centre
  4. reduce: voxel remesh (optional, fixes holes/self-intersections) then decimate to a tri budget
  5. smooth shading + sharp edges by angle, fresh Smart UV unwrap
  6. bake the source's look (base colour, normal detail, roughness/metal, emission) onto the clean
     mesh with bake_texture_set.py (selected-to-active), and build a preview material

Inside Blender (MCP `execute_blender_code`):

    import runpy
    m = runpy.run_path(r"<repo>/.claude/skills/blender-ai-mesh-cleanup/scripts/cleanup_ai_mesh.py")
    result = m["main"](path=r"C:/downloads/rock.glb", name="SM_Rock_Cinder_A", target_height_cm=180,
                       target_tris=4000, out_dir=r"<repo>/ArtSource/Environment/Rocks/Rock_Cinder_A/export")

From a shell:

    blender --background --factory-startup --python cleanup_ai_mesh.py -- --path rock.glb \
        --name SM_Rock_A --height-cm 180 --tris 4000 --out-dir ./export --save-blend ./Rock_A.blend

Log every AI-generated or downloaded asset (tool or source + licence) in CREDITS.md.
"""

import json
import math
import os
import sys
import time

import bmesh
import bpy
from mathutils import Matrix, Vector

_HERE = os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else os.getcwd()
_DEFAULT_BAKE = os.path.normpath(os.path.join(_HERE, "..", "..", "blender-texturing", "scripts", "bake_texture_set.py"))


def _import(path):
    before = set(bpy.data.objects)
    ext = os.path.splitext(path)[1].lower()
    if ext in (".glb", ".gltf"):
        bpy.ops.import_scene.gltf(filepath=path)
    elif ext == ".fbx":
        bpy.ops.import_scene.fbx(filepath=path)
    elif ext == ".obj":
        bpy.ops.wm.obj_import(filepath=path)
    elif ext == ".stl":
        bpy.ops.wm.stl_import(filepath=path)
    else:
        raise ValueError("Unsupported format {!r}".format(ext))
    return [o for o in bpy.data.objects if o not in before]


def _apply_all(obj):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    ev = obj.evaluated_get(depsgraph)
    mesh = bpy.data.meshes.new_from_object(ev, preserve_all_data_layers=True, depsgraph=depsgraph)
    mesh.transform(obj.matrix_world)
    old = obj.data
    obj.modifiers.clear()
    obj.parent = None
    obj.matrix_world = Matrix.Identity(4)
    obj.data = mesh
    if old.users == 0:
        bpy.data.meshes.remove(old)


def _join(objs, name, collection):
    copies = []
    for o in objs:
        c = o.copy()
        c.data = o.data.copy()
        collection.objects.link(c)
        c.matrix_world = o.matrix_world.copy()
        _apply_all(c)
        copies.append(c)
    target = copies[0]
    if len(copies) > 1:
        with bpy.context.temp_override(active_object=target, object=target, selected_objects=copies,
                                       selected_editable_objects=copies):
            bpy.ops.object.join()
    target.name = name
    target.data.name = name
    return target


def _tris(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def _bbox(obj):
    vs = [v.co for v in obj.data.vertices]
    lo = Vector((min(v.x for v in vs), min(v.y for v in vs), min(v.z for v in vs)))
    hi = Vector((max(v.x for v in vs), max(v.y for v in vs), max(v.z for v in vs)))
    return lo, hi


def _clean(obj, merge_distance):
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    before = len(bm.verts)
    bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=merge_distance)
    welded = before - len(bm.verts)
    bmesh.ops.dissolve_degenerate(bm, edges=list(bm.edges), dist=merge_distance)
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context="VERTS")
    wire = [e for e in bm.edges if not e.link_faces]
    bmesh.ops.delete(bm, geom=wire, context="EDGES")
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(obj.data)
    bm.free()
    return {"welded_verts": welded, "deleted_loose_verts": len(loose), "deleted_wire_edges": len(wire)}


def _smart_uv(obj, angle_deg=66.0, margin=0.01):
    """Edit-mode operators read the REAL view-layer active object (temp_override of active_object
    is not enough for mesh.select_all's poll), so set it for real and restore afterwards."""
    vl = bpy.context.view_layer
    prev_active = vl.objects.active
    prev_sel = [o for o in vl.objects if o.select_get()]
    for o in prev_sel:
        o.select_set(False)
    obj.select_set(True)
    vl.objects.active = obj
    try:
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.uv.smart_project(angle_limit=math.radians(angle_deg), island_margin=margin)
    finally:
        if obj.mode != "OBJECT":
            bpy.ops.object.mode_set(mode="OBJECT")
        obj.select_set(False)
        for o in prev_sel:
            o.select_set(True)
        vl.objects.active = prev_active


def main(path=None, source_objects=None, name="SM_Asset", target_height_cm=None, target_size_cm=None,
         up_fix_deg=None, target_tris=6000, voxel_size=None, merge_distance=0.0005,
         sharp_angle_deg=60.0, out_dir=None, bake=True, bake_size=1024, bake_script=None,
         collection=None, keep_source=True):
    t0 = time.time()
    coll_name = collection or name
    coll = bpy.data.collections.get(coll_name) or bpy.data.collections.new(coll_name)
    if coll.name not in bpy.context.scene.collection.children:
        bpy.context.scene.collection.children.link(coll)
    warnings = []

    if path:
        imported = _import(os.path.abspath(path))
        meshes = [o for o in imported if o.type == "MESH"]
    else:
        imported = []
        meshes = [bpy.data.objects[n] for n in source_objects or []]
    if not meshes:
        raise ValueError("No mesh objects to clean up")
    src_materials = sorted({s.material.name for o in meshes for s in o.material_slots if s.material})

    src = _join(meshes, "SRC_" + name[3:] if name.startswith("SM_") else "SRC_" + name, coll)
    for o in imported:  # remove the original import hierarchy (empties + meshes)
        if bpy.data.objects.get(o.name) is o:
            bpy.data.objects.remove(o, do_unlink=True)
    tris_in = _tris(src)

    # Orientation + scale + origin are applied to the SOURCE too, so source and result stay aligned.
    if up_fix_deg:
        src.data.transform(Matrix.Rotation(math.radians(up_fix_deg), 4, "X"))
    lo, hi = _bbox(src)
    size_m = hi - lo
    scale = 1.0
    if target_height_cm:
        scale = (target_height_cm / 100.0) / max(size_m.z, 1e-9)
    elif target_size_cm:
        scale = (target_size_cm / 100.0) / max(max(size_m), 1e-9)
    center = (lo + hi) * 0.5
    src.data.transform(Matrix.Diagonal((scale, scale, scale, 1.0)) @ Matrix.Translation((-center.x, -center.y, -lo.z)))
    if scale > 50 or scale < 0.02:
        warnings.append("Rescaled by {:.4g}x - the import was far off real-world size".format(scale))

    low = src.copy()
    low.data = src.data.copy()
    coll.objects.link(low)
    low.name = name
    low.data.name = name
    cleaned = _clean(low, merge_distance)

    if voxel_size:
        rm = low.modifiers.new("Remesh", "REMESH")
        rm.mode = "VOXEL"
        rm.voxel_size = voxel_size
        _apply_all(low)
    tris_clean = _tris(low)
    if target_tris and tris_clean > target_tris:
        dec = low.modifiers.new("Decimate", "DECIMATE")
        dec.decimate_type = "COLLAPSE"
        dec.ratio = max(0.001, target_tris / float(tris_clean))
        dec.use_collapse_triangulate = True
        _apply_all(low)
    low.data.shade_smooth()
    if sharp_angle_deg:
        low.data.set_sharp_from_angle(angle=math.radians(sharp_angle_deg))
    while low.data.uv_layers:
        low.data.uv_layers.remove(low.data.uv_layers[0])
    low.data.uv_layers.new(name="UVMap")
    _smart_uv(low)
    low.data.materials.clear()
    target_mat = bpy.data.materials.new("M_{}".format(name[3:] if name.startswith("SM_") else name))
    low.data.materials.append(target_mat)

    bake_result = None
    if bake and out_dir:
        lo2, hi2 = _bbox(low)
        diag = (hi2 - lo2).length
        bk = runpy_bake(bake_script or _DEFAULT_BAKE)
        bake_result = bk["main"](target=low.name, out_dir=out_dir, size=bake_size, source_objects=[src.name],
                                 cage_extrusion=diag * 0.01, max_ray_distance=diag * 0.03,
                                 build_preview_material=True)
    if keep_source:
        src.hide_render = True
        src.hide_set(True)
    else:
        bpy.data.objects.remove(src, do_unlink=True)

    lo3, hi3 = _bbox(low)
    return {
        "status": "ok",
        "object": low.name,
        "source_object": src.name if keep_source else None,
        "source_materials": src_materials,
        "tris_in": tris_in,
        "tris_after_clean": tris_clean,
        "tris_out": _tris(low),
        "cleanup": cleaned,
        "scale_applied": round(scale, 6),
        "size_cm": [round(v * 100.0, 1) for v in (hi3 - lo3)],
        "bake": bake_result,
        "warnings": warnings + (bake_result or {}).get("warnings", []),
        "next": "Review renders, check for baked-in lighting/shadows in BC, then export. Log the source in CREDITS.md.",
        "seconds": round(time.time() - t0, 2),
    }


def runpy_bake(path):
    import runpy
    if not os.path.exists(path):
        raise FileNotFoundError("bake_texture_set.py not found at {}; pass bake_script=".format(path))
    return runpy.run_path(path)


def _cli():
    import argparse
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser(description="Clean up an AI-generated/downloaded mesh for Unreal")
    p.add_argument("--path", required=True)
    p.add_argument("--name", required=True)
    p.add_argument("--height-cm", type=float)
    p.add_argument("--size-cm", type=float)
    p.add_argument("--up-fix-deg", type=float)
    p.add_argument("--tris", type=int, default=6000)
    p.add_argument("--voxel-size", type=float)
    p.add_argument("--out-dir")
    p.add_argument("--bake-size", type=int, default=1024)
    p.add_argument("--save-blend")
    a = p.parse_args(argv)
    res = main(path=a.path, name=a.name, target_height_cm=a.height_cm, target_size_cm=a.size_cm,
               up_fix_deg=a.up_fix_deg, target_tris=a.tris, voxel_size=a.voxel_size, out_dir=a.out_dir,
               bake=bool(a.out_dir), bake_size=a.bake_size)
    if a.save_blend:
        bpy.ops.wm.save_as_mainfile(filepath=os.path.abspath(a.save_blend))
    print("__RESULT__" + json.dumps(res))


if __name__ == "__main__":
    _cli()

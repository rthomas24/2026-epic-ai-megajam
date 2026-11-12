# SPDX-License-Identifier: MIT
"""
export_for_unreal.py - export ONE game-ready FBX per asset for Unreal Engine 5 (FBX via Interchange).

Works on TEMPORARY copies; the user's scene is left exactly as it was.
  1. Collect the asset = root object + children. Mesh parts are evaluated (modifiers applied) and
     joined into ONE render mesh named after the root (SM_<Asset>). Helpers named CUT_* / SRC_* are
     skipped. UCX_/UBX_/USP_/UCP_ objects become collision renamed <PREFIX>_<SM name>_##.
     SOCKET_* empties become static-mesh sockets.
  2. collision="auto": no authored collision -> convex hulls (<= 64 support points each) of the
     largest parts become UCX_ pieces.
  3. Re-base on the root's origin (the pivot) and rotate the authoring front (-Y = Blender "Front"
     view) to Unreal's forward (+X). Pass front_axis="+X" if the asset was modelled nose-on-+X.
  4. Weld microscopic doubles, triangulate (beauty), sanity-check.
  5. Write <out_dir>/SM_<Asset>.fbx (Apply Unit + FBX Units Scale, Face smoothing, tangents,
     triangulated, no leaf bones/animation) and SM_<Asset>.manifest.json (materials, textures
     found next to it, tris, size, collision, sockets, Nanite hint, recommended import settings).

Inside Blender (MCP `execute_blender_code`):

    import runpy
    m = runpy.run_path(r"<repo>/.claude/skills/blender-unreal-export/scripts/export_for_unreal.py")
    result = m["main"](root="SM_Ship_Rift", out_dir=r"<repo>/ArtSource/Space/Ships/Rift/export")

From a shell:

    blender --background Rift.blend --python export_for_unreal.py -- --root SM_Ship_Rift --out-dir ./export

One asset per FBX on purpose: an FBX that carries sockets may hold only one render mesh, and with
several meshes only the first one's custom collision is imported.
"""

import glob
import json
import math
import os
import sys
import time

import bmesh
import bpy
from mathutils import Matrix, Vector

COLLISION_PREFIXES = ("UCX_", "UBX_", "USP_", "UCP_")
SKIP_PREFIXES = ("CUT_", "SRC_")
FRONT_TO_UE = {  # rotation about Z that turns the authoring front into +X
    "-Y": math.radians(90.0), "+Y": math.radians(-90.0), "+X": 0.0, "-X": math.radians(180.0),
}


def _fib_dirs(n):
    golden = math.pi * (3.0 - math.sqrt(5.0))
    out = []
    for i in range(n):
        y = 1.0 - (i / float(n - 1)) * 2.0
        r = math.sqrt(max(0.0, 1.0 - y * y))
        out.append(Vector((math.cos(golden * i) * r, y, math.sin(golden * i) * r)))
    return out


def _hull_object(name, world_points, collection, max_points=64):
    """Convex hull from the support points of `world_points` in max_points directions."""
    pts = {}
    for d in _fib_dirs(max_points):
        best = max(world_points, key=lambda p: p.dot(d))
        pts[tuple(round(c, 6) for c in best)] = best
    bm = bmesh.new()
    for p in pts.values():
        bm.verts.new(p)
    bmesh.ops.convex_hull(bm, input=list(bm.verts))
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context="VERTS")
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    return obj


def _evaluated_world_mesh(obj, depsgraph):
    ev = obj.evaluated_get(depsgraph)
    mesh = bpy.data.meshes.new_from_object(ev, preserve_all_data_layers=True, depsgraph=depsgraph)
    mesh.transform(obj.matrix_world)
    return mesh


def _material_params(mat):
    """Principled BSDF values for rebuilding the material in Unreal as a material instance.
    `textured` is True when Base Color / Roughness / Metallic / Emission are driven by nodes (bake them)."""
    out = {"name": mat.name, "textured": False}
    tree = mat.node_tree
    bsdf = next((n for n in tree.nodes if n.type == "BSDF_PRINCIPLED"), None) if tree else None
    if bsdf is None:
        out["textured"] = True
        return out

    def val(name):
        sock = bsdf.inputs[name]
        if sock.is_linked:
            out["textured"] = True
            return None
        v = sock.default_value
        return [round(c, 4) for c in v[:3]] if hasattr(v, "__len__") else round(float(v), 4)

    out.update({"base_color": val("Base Color"), "metallic": val("Metallic"), "roughness": val("Roughness"),
                "emission_color": val("Emission Color"), "emission_strength": val("Emission Strength")})
    return out


def _select_only(objs, active):
    vl = bpy.context.view_layer
    for o in vl.objects:
        o.select_set(False)
    for o in objs:
        o.select_set(True)
    vl.objects.active = active


def main(root, out_dir, front_axis="-Y", collision="auto", max_hulls=6, triangulate=True,
         weld_distance=1e-5, nanite_tris=20000, textures_dir=None, file_name=None, dry_run=False):
    t0 = time.time()
    root_obj = bpy.data.objects.get(root)
    if root_obj is None:
        raise ValueError("Root object {!r} not found".format(root))
    if front_axis not in FRONT_TO_UE:
        raise ValueError("front_axis must be one of {}".format(sorted(FRONT_TO_UE)))
    asset_name = file_name or root_obj.name
    warnings, errors = [], []
    if not asset_name.startswith("SM_"):
        warnings.append("Static meshes should be named SM_<Asset> (got {!r})".format(asset_name))

    family = [root_obj] + list(root_obj.children_recursive)
    parts = [o for o in family if o.type == "MESH" and not o.name.startswith(SKIP_PREFIXES + COLLISION_PREFIXES)
             and not o.hide_render]
    authored_collision = [o for o in family if o.type == "MESH" and o.name.startswith(COLLISION_PREFIXES)]
    sockets = [o for o in family if o.type == "EMPTY" and o.name.startswith("SOCKET_")]
    if not parts:
        raise ValueError("No render meshes under {!r}".format(root))

    depsgraph = bpy.context.evaluated_depsgraph_get()
    rebase = Matrix.Rotation(FRONT_TO_UE[front_axis], 4, "Z") @ root_obj.matrix_world.inverted()

    scene_coll = bpy.context.scene.collection
    tmp_coll = bpy.data.collections.new("__ue_export__")
    scene_coll.children.link(tmp_coll)
    vl = bpy.context.view_layer
    prev_active = vl.objects.active
    prev_sel = [o for o in vl.objects if o.select_get()]
    temp_objects, created_meshes = [], []
    manifest = {}
    # The exported objects must carry the EXACT names (SM_x, UCX_SM_x_00, SOCKET_y), but the originals
    # still own those names; Blender would silently call the copies "SM_x.001" and Unreal would then
    # fail to pair collision with the mesh. Park the originals under temporary names, restore after.
    renamed = []
    for o in family:
        renamed.append((o, o.name))
        o.name = o.name[:58] + "~exp"
        if o.data is not None and o.type == "MESH" and o.data.users == 1:
            renamed.append((o.data, o.data.name))
            o.data.name = o.data.name[:58] + "~exp"
    try:
        # ---- render mesh: evaluate every part in world space, then join.
        meshes = [_evaluated_world_mesh(p, depsgraph) for p in parts]
        created_meshes.extend(meshes)
        part_objs = []
        for p, me in zip(parts, meshes):
            o = bpy.data.objects.new("__part__" + p.name, me)
            tmp_coll.objects.link(o)
            part_objs.append(o)
        render = part_objs[0]
        if len(part_objs) > 1:
            _select_only(part_objs, render)
            bpy.ops.object.join()  # removes every part object except `render`
        temp_objects = [render]
        render.name = asset_name
        render.data.name = asset_name
        render.data.transform(rebase)
        try:
            _select_only([render], render)
            bpy.ops.object.material_slot_remove_unused()
        except Exception:
            pass

        bm = bmesh.new()
        bm.from_mesh(render.data)
        welded = len(bm.verts)
        if weld_distance:
            bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=weld_distance)
        welded -= len(bm.verts)
        if triangulate:
            bmesh.ops.triangulate(bm, faces=list(bm.faces), quad_method="BEAUTY", ngon_method="BEAUTY")
        bm.to_mesh(render.data)
        bm.free()
        tris = sum(len(p.vertices) - 2 for p in render.data.polygons)
        if not render.data.uv_layers:
            errors.append("Render mesh has no UV map")
        mats = [s.material.name if s.material else None for s in render.material_slots]
        if not mats or None in mats:
            errors.append("Render mesh has empty material slots: {}".format(mats))
        if len(mats) > 8:
            warnings.append("{} materials = {} draw calls; consider merging".format(len(mats), len(mats)))
        vs = [v.co for v in render.data.vertices]
        lo = Vector((min(v.x for v in vs), min(v.y for v in vs), min(v.z for v in vs)))
        hi = Vector((max(v.x for v in vs), max(v.y for v in vs), max(v.z for v in vs)))
        size_cm = [round(v * 100.0 * bpy.context.scene.unit_settings.scale_length, 1) for v in (hi - lo)]
        if max(size_cm) < 1.0 or max(size_cm) > 100000.0:
            warnings.append("Suspicious size {} cm - check scale".format(size_cm))

        # ---- collision
        col_objs = []
        if authored_collision:
            counters = {}
            for src in authored_collision:
                prefix = src.name[:4]
                idx = counters.get(prefix, 0)
                counters[prefix] = idx + 1
                me = _evaluated_world_mesh(src, depsgraph)
                created_meshes.append(me)
                me.transform(rebase)
                o = bpy.data.objects.new("{}{}_{:02d}".format(prefix, asset_name, idx), me)
                tmp_coll.objects.link(o)
                col_objs.append(o)
        elif collision == "auto":
            ranked = sorted(zip(parts, meshes), key=lambda pm: -(Vector(pm[0].dimensions).length))
            for idx, (p, _me) in enumerate(ranked[:max_hulls]):
                me = _evaluated_world_mesh(p, depsgraph)
                me.transform(rebase)
                pts = [v.co.copy() for v in me.vertices]
                bpy.data.meshes.remove(me)
                if len(pts) >= 4:
                    hull = _hull_object("UCX_{}_{:02d}".format(asset_name, idx), pts, tmp_coll)
                    created_meshes.append(hull.data)
                    col_objs.append(hull)
        temp_objects.extend(col_objs)

        # ---- sockets
        sock_objs = []
        original_names = dict((id(o), n) for o, n in renamed)
        for s in sockets:
            e = bpy.data.objects.new(original_names.get(id(s), s.name), None)
            e.empty_display_type = "ARROWS"
            tmp_coll.objects.link(e)
            e.matrix_world = rebase @ s.matrix_world
            sock_objs.append(e)
        temp_objects.extend(sock_objs)

        os.makedirs(out_dir, exist_ok=True)
        fbx_path = os.path.join(os.path.abspath(out_dir), asset_name + ".fbx")
        if errors and not dry_run:
            raise RuntimeError("Not exporting {}: {}".format(asset_name, "; ".join(errors)))
        if not dry_run:
            _select_only([render] + col_objs + sock_objs, render)
            bpy.ops.export_scene.fbx(
                filepath=fbx_path, use_selection=True, object_types={"MESH", "EMPTY"},
                global_scale=1.0, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
                axis_forward="-Z", axis_up="Y", use_space_transform=True, bake_space_transform=False,
                use_mesh_modifiers=True, mesh_smooth_type="FACE", use_triangles=True, use_tspace=True,
                use_custom_props=False, add_leaf_bones=False, bake_anim=False, colors_type="NONE",
                path_mode="AUTO", embed_textures=False)

        tex_dir = textures_dir or out_dir
        short = asset_name[3:] if asset_name.startswith("SM_") else asset_name
        textures = sorted(os.path.basename(p) for p in glob.glob(os.path.join(tex_dir, "T_{}_*.png".format(short))))
        nanite = tris >= nanite_tris
        manifest = {
            "asset": asset_name,
            "fbx": fbx_path,
            "tris": tris,
            "size_cm": size_cm,
            "pivot": "root object origin ({})".format(root_obj.name),
            "forward": "+X (authoring front {} rotated)".format(front_axis),
            "materials": mats,
            "material_params": [_material_params(s.material) for s in render.material_slots if s.material],
            "textures": textures,
            "collision": [o.name for o in col_objs],
            "collision_source": "authored" if authored_collision else ("auto convex hulls" if col_objs else "none"),
            "sockets": [o.name for o in sock_objs],
            "welded_verts": welded,
            "nanite_recommended": nanite,
            "unreal_import": {
                "uniform_scale": 1.0,
                "generate_lightmap_uvs": False,
                "normals": "Import Normals (keep the authored hard edges/custom normals)",
                "combine_meshes": True,
                "nanite": nanite,
                "collision": "use UCX_ from file" if col_objs else "none / add simple collision in UE",
                "textures": {"_BC": "sRGB on, Default", "_N": "Normalmap (already DirectX - do NOT flip green)",
                             "_ORM": "sRGB off, Masks", "_E": "sRGB on, Default"},
                "note": "Nanite only for opaque/masked materials; keep glass/translucent/emissive-blade parts as "
                        "separate non-Nanite meshes.",
            },
            "warnings": warnings,
            "errors": errors,
        }
        if not dry_run:
            with open(os.path.join(os.path.abspath(out_dir), asset_name + ".manifest.json"), "w",
                      encoding="utf-8") as fh:
                json.dump(manifest, fh, indent=2)
    finally:
        for o in temp_objects:
            try:
                bpy.data.objects.remove(o, do_unlink=True)
            except ReferenceError:
                pass
        bpy.data.collections.remove(tmp_coll)
        for me in created_meshes:
            try:
                if me.users == 0:
                    bpy.data.meshes.remove(me)
            except ReferenceError:
                pass
        for item, old_name in renamed:
            item.name = old_name
        for o in vl.objects:
            o.select_set(False)
        for o in prev_sel:
            if o.name in vl.objects:
                o.select_set(True)
        vl.objects.active = prev_active

    manifest["status"] = "ok" if not errors else "error"
    manifest["seconds"] = round(time.time() - t0, 2)
    return manifest


def export_calibration_arrow(out_dir):
    """One-time pipeline check. Exports SM_TD_CalibrationArrow: 100 cm long, nose authored on -Y,
    a fin on +Z. Imported into Unreal at the origin it must be 100 units long, point +X and have the
    fin up. If not, fix the import settings (or front_axis) BEFORE exporting real assets.
    Builds and removes its own temporary objects; the user's scene is untouched."""
    coll = bpy.data.collections.new("__calibration__")
    bpy.context.scene.collection.children.link(coll)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    del uv
    bmesh.ops.create_cube(bm, size=1.0, calc_uvs=True,
                          matrix=Matrix.Translation((0.0, 0.1, 0.05)) @ Matrix.Diagonal((0.1, 0.8, 0.1, 1.0)))
    bmesh.ops.create_cone(bm, cap_ends=True, segments=4, radius1=0.15, radius2=0.0, depth=0.2, calc_uvs=True,
                          matrix=Matrix.Translation((0.0, -0.4, 0.05)) @ Matrix.Rotation(math.radians(90), 4, "X"))
    bmesh.ops.create_cube(bm, size=1.0, calc_uvs=True,
                          matrix=Matrix.Translation((0.0, 0.3, 0.2)) @ Matrix.Diagonal((0.02, 0.2, 0.2, 1.0)))
    mesh = bpy.data.meshes.new("SM_TD_CalibrationArrow")
    bm.to_mesh(mesh)
    bm.free()
    mat = bpy.data.materials.new("M_TD_Calibration")
    mesh.materials.append(mat)
    obj = bpy.data.objects.new("SM_TD_CalibrationArrow", mesh)
    coll.objects.link(obj)
    try:
        return main(root=obj.name, out_dir=out_dir, collision="none")
    finally:
        bpy.data.objects.remove(obj, do_unlink=True)
        bpy.data.meshes.remove(mesh)
        bpy.data.materials.remove(mat)
        bpy.data.collections.remove(coll)


def _cli():
    import argparse
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser(description="Export one asset to FBX for Unreal")
    p.add_argument("--root", nargs="+", help="one or more asset root objects")
    p.add_argument("--out-dir", required=True)
    p.add_argument("--front-axis", default="-Y")
    p.add_argument("--collision", default="auto", choices=["auto", "none"])
    p.add_argument("--calibration", action="store_true", help="export SM_TD_CalibrationArrow only")
    a = p.parse_args(argv)
    if a.calibration:
        print("__RESULT__" + json.dumps(export_calibration_arrow(a.out_dir)))
        return
    if not a.root:
        p.error("--root is required unless --calibration")
    results = [main(root=r, out_dir=a.out_dir, front_axis=a.front_axis, collision=a.collision) for r in a.root]
    print("__RESULT__" + json.dumps(results if len(results) > 1 else results[0]))


if __name__ == "__main__":
    _cli()

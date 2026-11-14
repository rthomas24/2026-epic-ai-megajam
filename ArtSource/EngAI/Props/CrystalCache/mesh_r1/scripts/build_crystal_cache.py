
#!/usr/bin/env python3
"""Build #53 Crystal cache + hero crystal (Blender 4.5.3 LTS)."""
from __future__ import annotations
import json, os, sys
from pathlib import Path
from mathutils import Vector
import bpy

# Allow "from cc_*.py import" when launched via blender --python
sys.path.insert(0, str(Path(__file__).resolve().parent))

from cc_paths import *
from cc_utils import *
from cc_crystal import build_crystal_cluster, paint_crystal_attributes, make_attr_material
from cc_cradle import import_fbx, scale_orient_cradle, measure_floor_z, polish_cradle
from cc_bake import (
    load_image, make_src_emit_material, ensure_bake_image, bake_selected_to_active,
    bake_self, save_image, make_bevel_normal_material, make_emit_from_attribute,
    assign_pbr_material,
)
from cc_scene import (
    make_box, make_convex_hull, make_socket, make_pilot, make_clay_material,
    setup_hdri, add_studio_lights, render_view, export_fbx_unreal, export_fbx_direct,
    export_glb,
)

def copy_tex(name):
    srcp, dstp = TEX / name, OUT / name
    dstp.write_bytes(srcp.read_bytes())
    log(f"    copied {dstp}")

def main():
    log("=== #53 Crystal cache build (Blender 4.5.3) ===")
    reset_scene()
    col_src = new_collection("SRC")
    col_del = new_collection("Delivery")
    col_col = new_collection("Collision")
    col_ref = new_collection("Reference")

    log("-- building procedural hero crystal --")
    crystal = build_crystal_cluster()
    link_obj(crystal, col_del)
    paint_crystal_attributes(crystal)
    make_attr_material("M_CrystalCacheCrystal_attr", crystal)
    smart_uv(crystal, angle=46.0, island_margin=0.004)
    rec_crystal = mesh_report_dict(crystal, "crystal procedural (pre-bake)")
    if rec_crystal["wobble_pct"] >= 3.0:
        log(f"WARNING crystal wobble {rec_crystal['wobble_pct']:.1f}% (want ~0)")

    log("-- importing and polishing cradle --")
    cradle_raw = import_fbx(SRC_CRADLE_FBX)
    cradle_raw.name = "SRC_import"
    scale_orient_cradle(cradle_raw)
    floor_z = measure_floor_z(cradle_raw)

    src = duplicate_obj(cradle_raw, "SRC_CrystalCacheCradle")
    link_obj(src, col_src)
    src.hide_render = True
    src.hide_viewport = True

    src_bc = load_image(SRC_CRADLE_TEX / "texture_diffuse.png", "sRGB")
    src_e = load_image(SRC_CRADLE_TEX / "texture_emissive.png", "sRGB")
    src_m = load_image(SRC_CRADLE_TEX / "texture_metallic.png", "sRGB")
    src_r = load_image(SRC_CRADLE_TEX / "texture_roughness.png", "sRGB")

    cradle = duplicate_obj(cradle_raw, "SM_CrystalCacheCradle")
    link_obj(cradle, col_del)
    bpy.data.objects.remove(cradle_raw, do_unlink=True)

    recs_cradle = polish_cradle(cradle)
    log(f"  polish non-manifold (evaluated)={recs_cradle[-1]['nonmanifold']}")

    log("-- baking cradle textures --")
    bake_cradle = duplicate_obj(cradle, "BAKE_CrystalCacheCradle")
    link_obj(bake_cradle, col_del)
    select_only(bake_cradle)
    for mod in list(bake_cradle.modifiers):
        try:
            bpy.ops.object.modifier_apply(modifier=mod.name)
        except Exception as e:
            log(f"  modifier apply {mod.name}: {e}")
    # Clean bevel leftovers
    select_only(bake_cradle)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.dissolve_degenerate(threshold=0.0001)
    bpy.ops.mesh.delete_loose(use_verts=True, use_edges=True, use_faces=False)
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    smart_uv(bake_cradle, angle=66.0, island_margin=0.002)
    mat_b = bpy.data.materials.new("M_CradleBake")
    mat_b.use_nodes = True
    bake_cradle.data.materials.clear()
    bake_cradle.data.materials.append(mat_b)

    src.hide_render = False
    src.hide_viewport = False
    src.hide_set(False)

    def bake_chan(src_img, colorspace, filename, noncolor):
        make_src_emit_material(src, src_img, colorspace)
        img, _ = ensure_bake_image(bake_cradle, filename, TEX_SIZE, noncolor=noncolor)
        bake_selected_to_active(src, bake_cradle, img)
        save_image(img, TEX / filename, noncolor=noncolor)

    bake_chan(src_bc, "sRGB", "T_CrystalCacheCradle_BC_raw.png", False)
    bake_chan(src_e, "sRGB", "T_CrystalCacheCradle_E_raw.png", False)
    bake_chan(src_r, "sRGB", "T_CrystalCacheCradle_R_raw.png", False)
    bake_chan(src_m, "sRGB", "T_CrystalCacheCradle_M_raw.png", False)

    img_ao, _ = ensure_bake_image(bake_cradle, "AO", TEX_SIZE, noncolor=True)
    bake_self(bake_cradle, "AO", img_ao, samples=32)
    save_image(img_ao, TEX / "T_CrystalCacheCradle_AO_raw.png", noncolor=True)

    make_bevel_normal_material(bake_cradle, radius=0.006, noise_scale=550.0, bump_strength=0.16, bump_dist=0.0012)
    img_n, _ = ensure_bake_image(bake_cradle, "Nogl", TEX_SIZE, noncolor=True)
    bake_self(bake_cradle, "NORMAL", img_n, samples=16)
    save_image(img_n, TEX / "T_CrystalCacheCradle_N_ogl.png", noncolor=True)

    src.hide_render = True
    src.hide_viewport = True
    rec_cradle_applied = mesh_report_dict(bake_cradle, "cradle applied (bake/export mesh)")

    log("-- baking crystal textures --")
    make_emit_from_attribute(crystal, "Col")
    img, _ = ensure_bake_image(crystal, "cBC", TEX_SIZE, False)
    bake_self(crystal, "EMIT", img)
    save_image(img, TEX / "T_CrystalCacheCrystal_BC_raw.png", False)

    make_emit_from_attribute(crystal, "Emi")
    img, _ = ensure_bake_image(crystal, "cE", TEX_SIZE, False)
    bake_self(crystal, "EMIT", img)
    save_image(img, TEX / "T_CrystalCacheCrystal_E_raw.png", False)

    make_emit_from_attribute(crystal, "RM")
    img, _ = ensure_bake_image(crystal, "cRM", TEX_SIZE, True)
    bake_self(crystal, "EMIT", img)
    save_image(img, TEX / "T_CrystalCacheCrystal_RM_raw.png", True)

    img_ao, _ = ensure_bake_image(crystal, "cAO", TEX_SIZE, True)
    bake_self(crystal, "AO", img_ao, samples=32)
    save_image(img_ao, TEX / "T_CrystalCacheCrystal_AO_raw.png", True)

    make_bevel_normal_material(crystal, radius=0.0018, noise_scale=400.0, bump_strength=0.08, bump_dist=0.0006)
    img_n, _ = ensure_bake_image(crystal, "cN", TEX_SIZE, True)
    bake_self(crystal, "NORMAL", img_n, samples=16)
    save_image(img_n, TEX / "T_CrystalCacheCrystal_N_ogl.png", True)

    log("-- processing textures --")
    hook = Path(__file__).resolve().parent / "process_tex.py"
    rc = os.system(f"python3 {hook} {TEX}")
    if rc != 0:
        raise SystemExit(f"texture post failed rc={rc}")
    for name in [
        "T_CrystalCacheCradle_BC.png", "T_CrystalCacheCradle_N.png",
        "T_CrystalCacheCradle_ORM.png", "T_CrystalCacheCradle_E.png",
        "T_CrystalCacheCrystal_BC.png", "T_CrystalCacheCrystal_N.png",
        "T_CrystalCacheCrystal_ORM.png", "T_CrystalCacheCrystal_E.png",
    ]:
        copy_tex(name)

    assign_pbr_material(bake_cradle, "T_CrystalCacheCradle", OUT, emission_strength=2.4)
    assign_pbr_material(crystal, "T_CrystalCacheCrystal", OUT, emission_strength=2.0)
    cradle.data.materials.clear()
    cradle.data.materials.append(bake_cradle.data.materials[0])

    log("-- sockets and collision --")
    socket = make_socket("SOCKET_Crystal", (0.0, 0.0, floor_z), col_del)
    parent_keep_world(socket, cradle)

    mins, maxs, size = world_bbox(bake_cradle)
    center = (mins + maxs) / 2.0
    ucx0 = make_box("UCX_SM_CrystalCacheCradle_00", center, size, col_col)
    parent_keep_world(ucx0, cradle)
    ucx0.hide_render = True

    ucx_c = make_convex_hull("UCX_SM_CrystalCacheCrystal_00", crystal, col_col, max_points=40)
    parent_keep_world(ucx_c, crystal)
    ucx_c.hide_render = True

    bake_cradle.hide_render = True
    bake_cradle.hide_viewport = True
    src.hide_render = True
    src.hide_viewport = True
    bpy.ops.wm.save_as_mainfile(filepath=str(WORK / "assembly.blend"))

    log("-- exporting FBX / GLB --")
    cradle_edit = cradle
    parent_keep_world(socket, bake_cradle)
    parent_keep_world(ucx0, bake_cradle)
    bake_cradle.hide_render = False
    bake_cradle.hide_viewport = False
    bake_cradle.name = "SM_CrystalCacheCradle"
    cradle_edit.name = "EDIT_CrystalCacheCradle"
    try:
        export_fbx_unreal("SM_CrystalCacheCradle", OUT)
    except Exception as e:
        log(f"  export_for_unreal cradle failed ({e}); fallback")
        export_fbx_direct([bake_cradle, ucx0, socket], OUT / "SM_CrystalCacheCradle.fbx")
    export_glb(bake_cradle, OUT / "SM_CrystalCacheCradle.glb")

    crystal.hide_render = False
    try:
        export_fbx_unreal("SM_CrystalCacheCrystal", OUT)
    except Exception as e:
        log(f"  export_for_unreal crystal failed ({e}); fallback")
        export_fbx_direct([crystal, ucx_c], OUT / "SM_CrystalCacheCrystal.fbx")
    export_glb(crystal, OUT / "SM_CrystalCacheCrystal.glb")

    bake_cradle.name = "BAKE_CrystalCacheCradle"
    cradle_edit.name = "SM_CrystalCacheCradle"
    parent_keep_world(socket, cradle_edit)
    parent_keep_world(ucx0, cradle_edit)
    bake_cradle.hide_render = True
    bake_cradle.hide_viewport = True

    log("-- rendering contact-sheet panels --")
    crystal.location = Vector((0.0, 0.0, floor_z))
    pilot = make_pilot()
    link_obj(pilot, col_ref)
    mins_c, maxs_c, size_c = world_bbox(bake_cradle)
    pilot.location = Vector((maxs_c.x + 0.35, -0.55, 0.0))

    setup_hdri(strength=1.05)
    add_studio_lights()
    bpy.ops.mesh.primitive_plane_add(size=12.0, location=(0, 0, -0.001))
    ground = bpy.context.object
    ground.name = "GROUND"
    gmat = bpy.data.materials.new("M_Ground")
    gmat.use_nodes = True
    gmat.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.22, 0.22, 0.23, 1)
    gmat.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value = 0.65
    ground.data.materials.append(gmat)
    try:
        ground.is_shadow_catcher = True
    except Exception:
        pass

    cradle_edit.hide_render = True
    cradle_edit.hide_viewport = True
    bake_cradle.hide_render = False
    bake_cradle.hide_viewport = False
    src.hide_render = True
    ucx0.hide_render = True
    ucx_c.hide_render = True

    render_view(RENDERS / "assembled_hero.png",
                loc=Vector((4.6, -5.2, 2.6)), target=Vector((0.1, 0.0, 0.7)), lens=45)
    hide_for_render(crystal, pilot, hide=True)
    render_view(RENDERS / "cradle_alone.png",
                loc=Vector((4.4, -5.0, 2.5)), target=Vector((0.0, 0.0, 0.85)), lens=45)
    hide_for_render(bake_cradle, hide=True)
    hide_for_render(crystal, hide=False)
    saved_loc = crystal.location.copy()
    crystal.location = Vector((0, 0, 0))
    render_view(RENDERS / "crystal_alone.png",
                loc=Vector((2.2, -2.4, 1.5)), target=Vector((0.0, 0.0, 0.7)), lens=50)
    crystal.location = saved_loc
    hide_for_render(bake_cradle, crystal, hide=False)
    hide_for_render(pilot, hide=True)
    render_view(RENDERS / "closeup.png",
                loc=Vector((1.7, -1.1, 1.15)), target=Vector((0.15, 0.0, 0.85)), lens=50)
    hide_for_render(pilot, hide=False)
    render_view(RENDERS / "front_ortho.png",
                loc=Vector((6.0, 0.0, 0.95)), target=Vector((0.0, 0.0, 0.9)), ortho=4.6)
    render_view(RENDERS / "end_ortho.png",
                loc=Vector((0.0, -6.0, 0.95)), target=Vector((0.0, 0.0, 0.9)), ortho=2.6)

    clay = make_clay_material()
    for o in (bake_cradle, crystal, pilot):
        o.data.materials.clear()
        o.data.materials.append(clay)
    hide_for_render(pilot, bake_cradle, crystal, hide=False)
    render_view(RENDERS / "clay.png",
                loc=Vector((4.6, -5.2, 2.6)), target=Vector((0.1, 0.0, 0.7)), lens=45)

    def wire_copy(o, name):
        w = o.copy()
        w.data = o.data.copy()
        w.name = name
        bpy.context.scene.collection.objects.link(w)
        mod = w.modifiers.new("Wire", "WIREFRAME")
        mod.thickness = 0.004
        mod.use_replace = True
        wmat = bpy.data.materials.new(name + "_w")
        wmat.use_nodes = True
        wmat.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.02, 0.02, 0.02, 1)
        wmat.node_tree.nodes["Principled BSDF"].inputs["Emission Color"].default_value = (0.05, 0.05, 0.06, 1)
        wmat.node_tree.nodes["Principled BSDF"].inputs["Emission Strength"].default_value = 0.3
        w.data.materials.clear()
        w.data.materials.append(wmat)
        return w
    w1 = wire_copy(bake_cradle, "W_cradle")
    w2 = wire_copy(crystal, "W_crystal")
    render_view(RENDERS / "clay_wire.png",
                loc=Vector((4.6, -5.2, 2.6)), target=Vector((0.1, 0.0, 0.7)), lens=45)
    w1.hide_render = True
    w2.hide_render = True

    def axis_gizmo(loc, size=0.28):
        import bmesh
        from mathutils import Matrix
        bm = bmesh.new()
        for axis, L in enumerate([(size, 0.012, 0.012), (0.012, size, 0.012), (0.012, 0.012, size)]):
            bmesh.ops.create_cube(bm, size=1.0,
                                  matrix=Matrix.Translation((L[0]/2 if axis==0 else 0,
                                                             L[1]/2 if axis==1 else 0,
                                                             L[2]/2 if axis==2 else 0))
                                  @ Matrix.Diagonal((L[0], L[1], L[2], 1)))
        me = bpy.data.meshes.new("Gizmo")
        bm.to_mesh(me); bm.free()
        o = bpy.data.objects.new("GIZMO_SOCKET", me)
        o.location = loc
        bpy.context.scene.collection.objects.link(o)
        mat = bpy.data.materials.new("M_Gizmo")
        mat.use_nodes = True
        mat.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.9, 0.2, 0.1, 1)
        mat.node_tree.nodes["Principled BSDF"].inputs["Emission Color"].default_value = (0.9, 0.25, 0.05, 1)
        mat.node_tree.nodes["Principled BSDF"].inputs["Emission Strength"].default_value = 2.0
        o.data.materials.append(mat)
        return o
    giz = axis_gizmo(Vector((0, 0, floor_z)))
    def vis_collision(o, color):
        o.hide_render = False
        o.hide_viewport = False
        mat = bpy.data.materials.new(o.name + "_vis")
        mat.use_nodes = True
        mat.blend_method = "BLEND"
        try:
            mat.surface_render_method = "BLENDED"
        except Exception:
            pass
        bsdf = mat.node_tree.nodes["Principled BSDF"]
        bsdf.inputs["Base Color"].default_value = (*color, 0.25)
        bsdf.inputs["Alpha"].default_value = 0.28
        bsdf.inputs["Roughness"].default_value = 0.4
        o.data.materials.clear()
        o.data.materials.append(mat)
    vis_collision(ucx0, (0.15, 0.85, 0.25))
    vis_collision(ucx_c, (0.15, 0.55, 0.95))
    hide_for_render(pilot, hide=False)
    render_view(RENDERS / "collision_socket.png",
                loc=Vector((4.8, -5.4, 2.8)), target=Vector((0.1, 0.0, 0.7)), lens=45)

    ucx0.hide_render = True
    ucx_c.hide_render = True
    giz.hide_render = True
    assign_pbr_material(bake_cradle, "T_CrystalCacheCradle", OUT, emission_strength=2.4)
    assign_pbr_material(crystal, "T_CrystalCacheCrystal", OUT, emission_strength=2.0)
    cradle_edit.data.materials.clear()
    cradle_edit.data.materials.append(bake_cradle.data.materials[0])

    bake_cradle.hide_render = True
    bake_cradle.hide_viewport = True
    cradle_edit.hide_render = False
    cradle_edit.hide_viewport = False
    src.hide_render = True
    src.hide_viewport = True
    crystal.location = Vector((0.0, 0.0, floor_z))
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "SM_CrystalCache_assembly.blend"))
    log("  saved assembly blend")

    meta = {
        "floor_z": floor_z,
        "cradle_reports": recs_cradle + [rec_cradle_applied],
        "crystal_report": rec_crystal,
    }
    (WORK / "build_meta.json").write_text(json.dumps(meta, indent=2, default=str))
    log("=== build geometry/textures/renders complete ===")
    return meta

if __name__ == "__main__":
    main()

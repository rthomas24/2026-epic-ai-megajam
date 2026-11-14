"""Rebuild sharp-facet crystal (no root taper) and re-render under mid-grey studio lighting."""
from __future__ import annotations
import math, os, sys
from pathlib import Path
import bpy, bmesh
from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
from cc_paths import *
from cc_utils import *
from cc_bake import (ensure_bake_image, bake_self, save_image, make_bevel_normal_material,
                     make_emit_from_attribute, assign_pbr_material)
from cc_scene import (make_convex_hull, make_pilot, make_clay_material,
                      add_studio_lights, render_view, export_fbx_unreal, export_fbx_direct, export_glb)
from repair_crystal import build_new_crystal, ASSEMBLY, FLOOR_Z


def setup_neutral_studio():
    sc = bpy.context.scene
    world = bpy.data.worlds.new("WorldNeutral")
    sc.world = world
    world.use_nodes = True
    nt = world.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    bg = nt.nodes.new("ShaderNodeBackground")
    # mid-grey, slightly cool, no sun disk
    bg.inputs["Color"].default_value = (0.22, 0.225, 0.23, 1)
    bg.inputs["Strength"].default_value = 1.0
    out = nt.nodes.new("ShaderNodeOutputWorld")
    nt.links.new(bg.outputs["Background"], out.inputs["Surface"])
    # refresh area lights (drop old)
    for name in ("key", "fill", "rim", "ground_fill"):
        if name in bpy.data.objects:
            bpy.data.objects.remove(bpy.data.objects[name], do_unlink=True)
    add_studio_lights()
    # extra soft fill so gunmetal doesn't crush
    bpy.ops.object.light_add(type="AREA", location=(0.0, -0.5, 4.5), rotation=(0, 0, 0))
    L = bpy.context.object
    L.name = "ground_fill"
    L.data.energy = 18
    L.data.size = 6.0
    L.data.color = (1.0, 1.0, 1.0)
    L.data.shape = "DISK"
    sc.view_settings.view_transform = "Standard"
    sc.view_settings.exposure = 0.15
    sc.view_settings.look = "None"
    return world


def hide_helpers():
    for n in list(bpy.data.objects.keys()):
        if n.startswith("W_") or n.startswith("GIZMO") or n.startswith("FIG_") or n.startswith("UCX_"):
            bpy.data.objects[n].hide_render = True
            bpy.data.objects[n].hide_viewport = n.startswith("W_") or n.startswith("GIZMO")


def wire_copy(o, name, thickness=0.0035):
    # triangulate a copy so the Wireframe modifier doesn't spike on n-gons
    w = o.copy()
    w.data = o.data.copy()
    w.name = name
    w.parent = None
    w.matrix_world = o.matrix_world.copy()
    bpy.context.scene.collection.objects.link(w)
    bm = bmesh.new()
    bm.from_mesh(w.data)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    loose = [v for v in bm.verts if not v.link_faces]
    if loose:
        bmesh.ops.delete(bm, geom=loose, context="VERTS")
    bm.to_mesh(w.data)
    bm.free()
    mod = w.modifiers.new("Wire", "WIREFRAME")
    mod.thickness = thickness
    mod.use_replace = True
    mod.use_even_offset = True
    mod.use_boundary = True
    wmat = bpy.data.materials.new(name + "_w")
    wmat.use_nodes = True
    bsdf = wmat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (0.04, 0.04, 0.045, 1)
    bsdf.inputs["Emission Color"].default_value = (0.05, 0.05, 0.055, 1)
    bsdf.inputs["Emission Strength"].default_value = 0.25
    bsdf.inputs["Roughness"].default_value = 1.0
    w.data.materials.clear()
    w.data.materials.append(wmat)
    w.hide_render = False
    return w


def make_socket_gizmo(location):
    bm = bmesh.new()
    # RGB axis bars + a small sphere at the origin
    axes = [
        ((0.45, 0.016, 0.016), (0.225, 0, 0), (1.0, 0.15, 0.05)),
        ((0.016, 0.45, 0.016), (0, 0.225, 0), (0.15, 0.85, 0.20)),
        ((0.016, 0.016, 0.45), (0, 0, 0.225), (0.15, 0.45, 1.0)),
    ]
    # we'll paint one material (orange) — color-coded emission objects instead
    bmesh.ops.create_uvsphere(bm, u_segments=16, v_segments=10, radius=0.06)
    me = bpy.data.meshes.new("GizmoSphere")
    bm.to_mesh(me)
    bm.free()
    sphere = bpy.data.objects.new("GIZMO_SOCKET", me)
    sphere.location = location
    bpy.context.scene.collection.objects.link(sphere)
    gm = bpy.data.materials.new("M_Gizmo")
    gm.use_nodes = True
    bsdf = gm.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (1.0, 0.35, 0.05, 1)
    bsdf.inputs["Emission Color"].default_value = (1.0, 0.4, 0.05, 1)
    bsdf.inputs["Emission Strength"].default_value = 6.0
    bsdf.inputs["Roughness"].default_value = 0.4
    sphere.data.materials.append(gm)

    bars = []
    for size, offset, color in axes:
        bm = bmesh.new()
        bmesh.ops.create_cube(bm, size=1.0)
        for v in bm.verts:
            v.co.x *= size[0]
            v.co.y *= size[1]
            v.co.z *= size[2]
            v.co += Vector(offset)
        me = bpy.data.meshes.new("GizmoBar")
        bm.to_mesh(me)
        bm.free()
        bar = bpy.data.objects.new("GIZMO_AXIS", me)
        bar.location = location
        bpy.context.scene.collection.objects.link(bar)
        mat = bpy.data.materials.new("M_GizmoAxis")
        mat.use_nodes = True
        b = mat.node_tree.nodes["Principled BSDF"]
        b.inputs["Base Color"].default_value = (*color, 1)
        b.inputs["Emission Color"].default_value = (*color, 1)
        b.inputs["Emission Strength"].default_value = 5.0
        bar.data.materials.append(mat)
        bars.append(bar)
    return [sphere] + bars


def vis_wire_collision(obj, name, color):
    w = obj.copy()
    w.data = obj.data.copy()
    w.name = name
    w.parent = None
    w.matrix_world = obj.matrix_world.copy()
    bpy.context.scene.collection.objects.link(w)
    mod = w.modifiers.new("Wire", "WIREFRAME")
    mod.thickness = 0.018
    mod.use_replace = True
    mod.use_even_offset = True
    mat = bpy.data.materials.new(name + "_m")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (*color, 1)
    bsdf.inputs["Emission Color"].default_value = (*color, 1)
    bsdf.inputs["Emission Strength"].default_value = 3.5
    bsdf.inputs["Roughness"].default_value = 1.0
    w.data.materials.clear()
    w.data.materials.append(mat)
    w.hide_render = False
    return w


def main():
    log("=== fix crystal wobble + studio re-render ===")
    bpy.ops.wm.open_mainfile(filepath=str(ASSEMBLY))

    # Drop previous crystal, collision, pilot, gizmos, wires
    drop = []
    for obj in list(bpy.data.objects):
        n = obj.name
        if n.startswith("SM_CrystalCacheCrystal") or n.startswith("UCX_SM_CrystalCacheCrystal") \
           or n.startswith("FIG_") or n.startswith("GIZMO") or n.startswith("W_") \
           or n.startswith("Gizmo"):
            drop.append(obj)
    for obj in drop:
        bpy.data.objects.remove(obj, do_unlink=True)

    crystal = build_new_crystal()
    rec = mesh_report_dict(crystal)
    log(f"  REPORT repaired crystal: tris={rec['tris']} islands={rec['islands']} "
        f"nonman={rec['nonmanifold']} degen={rec['degenerate']} "
        f"flat={rec['flat_pct']:.1f}% wobble={rec['wobble_pct']:.1f}% hard={rec['hard_pct']:.1f}% "
        f"size_cm={[round(c*100,1) for c in rec['size_m']]}")

    # UV + bake (signatures: make_emit_from_attribute(obj, layer), bake_self(obj, type, img))
    if "smart_uv" in dir():
        smart_uv(crystal, angle=46.0, island_margin=0.004)
    else:
        select_only(crystal)
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.uv.smart_project(angle_limit=math.radians(46), island_margin=0.004)
        bpy.ops.object.mode_set(mode="OBJECT")

    log("color attrs: " + str([a.name for a in crystal.data.color_attributes]))
    for layer, fname in (("Col", "T_CrystalCacheCrystal_BC_raw.png"),
                         ("Emi", "T_CrystalCacheCrystal_E_raw.png"),
                         ("RM", "T_CrystalCacheCrystal_RM_raw.png")):
        make_emit_from_attribute(crystal, layer)
        img, _ = ensure_bake_image(crystal, "r"+layer, TEX_SIZE, False)
        bake_self(crystal, "EMIT", img)
        save_image(img, TEX / fname, False)
    img, _ = ensure_bake_image(crystal, "rAO", TEX_SIZE, False)
    bake_self(crystal, "AO", img, samples=32)
    save_image(img, TEX / "T_CrystalCacheCrystal_AO_raw.png", False)
    make_bevel_normal_material(crystal, radius=0.0018, noise_scale=400.0, bump_strength=0.08, bump_dist=0.0006)
    img, _ = ensure_bake_image(crystal, "rN", TEX_SIZE, False)
    bake_self(crystal, "NORMAL", img, samples=16)
    save_image(img, TEX / "T_CrystalCacheCrystal_N_ogl.png", False)

    # process crystal only — leave cradle maps alone
    rc = os.system(f"python3 {Path(__file__).resolve().parent / 'process_tex.py'} --crystal-only {TEX}")
    log(f"process_tex exit {rc}")
    for name in ("T_CrystalCacheCrystal_BC.png", "T_CrystalCacheCrystal_N.png",
                 "T_CrystalCacheCrystal_ORM.png", "T_CrystalCacheCrystal_E.png"):
        if (TEX/name).exists():
            (OUT/name).write_bytes((TEX/name).read_bytes())

    assign_pbr_material(crystal, "T_CrystalCacheCrystal", OUT, emission_strength=2.2)

    # collision
    ucx_c = make_convex_hull("UCX_SM_CrystalCacheCrystal_00", crystal)
    ucx_c.display_type = "WIRE"
    parent_keep_world(ucx_c, crystal)

    # export crystal
    try:
        export_fbx_unreal("SM_CrystalCacheCrystal", OUT)
    except Exception as e:
        log(f"export_for_unreal failed: {e}")
        export_fbx_direct([crystal, ucx_c], OUT / "SM_CrystalCacheCrystal.fbx")
    try:
        export_glb(crystal, OUT / "SM_CrystalCacheCrystal.glb")
    except Exception as e:
        log(f"glb: {e}")
        select_only(crystal)
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.mesh.quads_convert_to_tris(quad_method="BEAUTY", ngon_method="BEAUTY")
        bpy.ops.object.mode_set(mode="OBJECT")
        export_glb(crystal, OUT / "SM_CrystalCacheCrystal.glb")

    # Lighting
    setup_neutral_studio()
    hide_helpers()

    cradle_bake = bpy.data.objects.get("BAKE_CrystalCacheCradle")
    cradle_edit = bpy.data.objects.get("SM_CrystalCacheCradle")
    src = bpy.data.objects.get("SRC_CrystalCacheCradle")
    ucx0 = bpy.data.objects.get("UCX_SM_CrystalCacheCradle_00")
    if src:
        src.hide_render = True
        src.hide_viewport = True
    if cradle_edit:
        cradle_edit.hide_render = True
        cradle_edit.hide_viewport = True
    if cradle_bake:
        cradle_bake.hide_render = False
        cradle_bake.hide_viewport = False
        assign_pbr_material(cradle_bake, "T_CrystalCacheCradle", OUT, emission_strength=2.4)
    if ucx0:
        ucx0.hide_render = True

    crystal.location = Vector((0.0, 0.0, FLOOR_Z))
    # Pilot beside the -Y tunnel mouth, to the +X side, clear of the emblem and crystal
    pilot = make_pilot()
    pilot.location = Vector((1.35, -2.25, 0.0))

    # 02 assembled — look into the open bay so the cluster reads
    hide_for_render(pilot, cradle_bake, crystal, hide=False)
    render_view(RENDERS / "assembled_hero.png",
                loc=Vector((4.2, -5.6, 2.35)), target=Vector((0.0, 0.15, 0.75)), lens=45)
    hide_for_render(crystal, pilot, hide=True)
    render_view(RENDERS / "cradle_alone.png",
                loc=Vector((4.2, -5.4, 2.35)), target=Vector((0.0, 0.0, 0.85)), lens=45)
    hide_for_render(cradle_bake, hide=True)
    hide_for_render(crystal, hide=False)
    saved = crystal.location.copy()
    crystal.location = Vector((0, 0, 0))
    render_view(RENDERS / "crystal_alone.png",
                loc=Vector((2.15, -2.35, 1.45)), target=Vector((0.0, 0.0, 0.65)), lens=50)
    crystal.location = saved
    hide_for_render(cradle_bake, crystal, hide=False)
    hide_for_render(pilot, hide=True)
    # close-up of -Y rim bevels + crystal facets
    render_view(RENDERS / "closeup.png",
                loc=Vector((1.15, -2.05, 1.45)), target=Vector((0.05, -0.15, 0.95)), lens=50)
    hide_for_render(pilot, hide=False)
    render_view(RENDERS / "front_ortho.png",
                loc=Vector((6.0, 0.0, 0.95)), target=Vector((0.0, 0.0, 0.9)), ortho=5.2)
    render_view(RENDERS / "end_ortho.png",
                loc=Vector((0.0, -6.0, 0.95)), target=Vector((0.0, 0.0, 0.9)), ortho=2.8)

    # clay
    clay = make_clay_material()
    for o in (cradle_bake, crystal, pilot):
        if o:
            o.data.materials.clear()
            o.data.materials.append(clay)
    hide_for_render(pilot, cradle_bake, crystal, hide=False)
    render_view(RENDERS / "clay.png",
                loc=Vector((4.2, -5.6, 2.35)), target=Vector((0.0, 0.15, 0.75)), lens=45)
    w1 = wire_copy(cradle_bake, "W_cradle")
    w2 = wire_copy(crystal, "W_crystal", thickness=0.0025)
    render_view(RENDERS / "clay_wire.png",
                loc=Vector((4.2, -5.6, 2.35)), target=Vector((0.0, 0.15, 0.75)), lens=45)
    w1.hide_render = True
    w2.hide_render = True

    # collision / socket callout on clay meshes + wire UCX + orange gizmo
    vis = []
    if ucx0:
        vis.append(vis_wire_collision(ucx0, "VIS_UCX_cradle", (0.20, 0.90, 0.25)))
    vis.append(vis_wire_collision(ucx_c, "VIS_UCX_crystal", (0.20, 0.55, 1.0)))
    gizmos = make_socket_gizmo(Vector((0, 0, FLOOR_Z)))
    hide_for_render(pilot, hide=False)
    # slightly higher/closer so the socket inside the bay is readable
    render_view(RENDERS / "collision_socket.png",
                loc=Vector((3.6, -5.2, 2.8)), target=Vector((0.0, 0.1, 0.55)), lens=40)
    for o in vis + gizmos:
        o.hide_render = True

    # restore PBR, keep editable assembly clean
    assign_pbr_material(cradle_bake, "T_CrystalCacheCradle", OUT, emission_strength=2.4)
    assign_pbr_material(crystal, "T_CrystalCacheCrystal", OUT, emission_strength=2.2)
    if cradle_edit:
        cradle_edit.data.materials.clear()
        if cradle_bake.data.materials:
            cradle_edit.data.materials.append(cradle_bake.data.materials[0])
        cradle_edit.hide_render = False
        cradle_edit.hide_viewport = False
    if cradle_bake:
        cradle_bake.hide_render = True
        cradle_bake.hide_viewport = True
    crystal.location = Vector((0.0, 0.0, FLOOR_Z))
    if ucx0:
        ucx0.hide_render = True
        ucx0.hide_viewport = False
    ucx_c.hide_render = True
    # leave socket empty visible in viewport
    sock = bpy.data.objects.get("SOCKET_Crystal")
    if sock:
        sock.hide_render = True
        sock.hide_viewport = False

    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "SM_CrystalCache_assembly.blend"))
    log("saved assembly")
    log(f"=== fix complete  wobble={rec['wobble_pct']:.1f}% tris={rec['tris']} ===")


if __name__ == "__main__":
    main()

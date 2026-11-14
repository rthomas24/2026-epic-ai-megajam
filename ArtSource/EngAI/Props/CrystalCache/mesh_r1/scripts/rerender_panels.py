"""Re-render closeup, clay+feature-wire, and socket/collision callout only. Does not save the blend."""
from __future__ import annotations
import math, sys
from pathlib import Path
import bpy, bmesh
from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
from cc_paths import *
from cc_utils import *
from cc_bake import assign_pbr_material
from cc_scene import make_clay_material, render_view
from repair_crystal import ASSEMBLY, FLOOR_Z


def feature_wire(src, name, angle=28.0, radius=0.0030, collection=None):
    """Curve-bevel overlay of hard/boundary edges only — no Wireframe-modifier spikes."""
    dg = bpy.context.evaluated_depsgraph_get()
    ev = src.evaluated_get(dg)
    me = ev.to_mesh()
    mw = src.matrix_world.copy()
    bm = bmesh.new()
    bm.from_mesh(me)
    ev.to_mesh_clear()
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bm.edges.ensure_lookup_table()
    keep = []
    for e in bm.edges:
        if len(e.link_faces) < 2:
            keep.append(e)
        elif math.degrees(e.calc_face_angle(0)) >= angle:
            keep.append(e)
    # build a 3D curve from those edges
    curve = bpy.data.curves.new(name, "CURVE")
    curve.dimensions = "3D"
    curve.bevel_depth = radius
    curve.bevel_resolution = 1
    curve.fill_mode = "FULL"
    used = set()
    # walk polylines so we don't make one spline per edge (faster, cleaner)
    adj = {}
    for e in keep:
        a = e.vert0 if hasattr(e, "vert0") else e.verts[0]
        b = e.verts[1]
        adj.setdefault(a, []).append(b)
        adj.setdefault(b, []).append(a)
    visited_e = set()
    def eid(u, v):
        return tuple(sorted((u.index, v.index)))
    for start, nbs in adj.items():
        for nb in nbs:
            if eid(start, nb) in visited_e:
                continue
            # grow a chain
            chain = [start, nb]
            visited_e.add(eid(start, nb))
            prev, cur = start, nb
            while True:
                nxts = [x for x in adj.get(cur, []) if eid(cur, x) not in visited_e]
                if len(nxts) != 1:
                    break
                visited_e.add(eid(cur, nxts[0]))
                chain.append(nxts[0])
                prev, cur = cur, nxts[0]
            spl = curve.splines.new("POLY")
            spl.points.add(len(chain) - 1)
            for i, v in enumerate(chain):
                p = mw @ v.co
                spl.points[i].co = (p.x, p.y, p.z, 1.0)
    bm.free()
    obj = bpy.data.objects.new(name, curve)
    (collection or bpy.context.scene.collection).objects.link(obj)
    mat = bpy.data.materials.new(name + "_m")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (0.03, 0.03, 0.035, 1)
    bsdf.inputs["Emission Color"].default_value = (0.02, 0.02, 0.025, 1)
    bsdf.inputs["Emission Strength"].default_value = 0.15
    bsdf.inputs["Roughness"].default_value = 1.0
    obj.data.materials.append(mat)
    return obj


def socket_gizmo(loc):
    objs = []
    # floor ring
    bm = bmesh.new()
    bmesh.ops.create_circle(bm, cap_ends=False, cap_tris=False, segments=48, radius=0.28)
    # extrude a thin torus-like ribbon by spinning? just a disk ring via annulus
    # simpler: curve circle
    bm.free()
    cu = bpy.data.curves.new("SOCK_RING", "CURVE")
    cu.dimensions = "3D"
    cu.bevel_depth = 0.018
    cu.bevel_resolution = 3
    cu.fill_mode = "FULL"
    spl = cu.splines.new("NURBS")
    n = 48
    spl.points.add(n - 1)
    for i in range(n):
        a = (i / n) * math.tau
        spl.points[i].co = (0.32 * math.cos(a), 0.32 * math.sin(a), 0.0, 1.0)
    spl.use_cyclic_u = True
    ring = bpy.data.objects.new("GIZMO_SOCKET_RING", cu)
    ring.location = loc
    bpy.context.scene.collection.objects.link(ring)
    objs.append(ring)
    # sphere
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=20, v_segments=12, radius=0.08)
    me = bpy.data.meshes.new("SOCK_SPH")
    bm.to_mesh(me); bm.free()
    sph = bpy.data.objects.new("GIZMO_SOCKET", me)
    sph.location = loc
    bpy.context.scene.collection.objects.link(sph)
    objs.append(sph)
    # axis bars
    for size, offset, color, name in (
        ((0.70, 0.022, 0.022), (0.35, 0, 0), (1.0, 0.25, 0.05), "GX"),
        ((0.022, 0.70, 0.022), (0, 0.35, 0), (0.15, 0.90, 0.25), "GY"),
        ((0.022, 0.022, 0.90), (0, 0, 0.45), (0.20, 0.45, 1.0), "GZ"),
    ):
        bm = bmesh.new()
        bmesh.ops.create_cube(bm, size=1.0)
        for v in bm.verts:
            v.co.x *= size[0]; v.co.y *= size[1]; v.co.z *= size[2]
            v.co += Vector(offset)
        me = bpy.data.meshes.new(name)
        bm.to_mesh(me); bm.free()
        bar = bpy.data.objects.new("GIZMO_AXIS_"+name, me)
        bar.location = loc
        bpy.context.scene.collection.objects.link(bar)
        mat = bpy.data.materials.new("M_"+name)
        mat.use_nodes = True
        b = mat.node_tree.nodes["Principled BSDF"]
        b.inputs["Base Color"].default_value = (*color, 1)
        b.inputs["Emission Color"].default_value = (*color, 1)
        b.inputs["Emission Strength"].default_value = 8.0
        bar.data.materials.append(mat)
        objs.append(bar)
    mat = bpy.data.materials.new("M_SockOrange")
    mat.use_nodes = True
    b = mat.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (1.0, 0.35, 0.05, 1)
    b.inputs["Emission Color"].default_value = (1.0, 0.4, 0.05, 1)
    b.inputs["Emission Strength"].default_value = 8.0
    ring.data.materials.append(mat)
    sph.data.materials.append(mat)
    return objs


def vis_wire_box(obj, name, color, thickness=0.016):
    w = obj.copy(); w.data = obj.data.copy(); w.name = name
    w.parent = None
    w.matrix_world = obj.matrix_world.copy()
    bpy.context.scene.collection.objects.link(w)
    mod = w.modifiers.new("Wire", "WIREFRAME")
    mod.thickness = thickness
    mod.use_replace = True
    mod.use_even_offset = True
    mat = bpy.data.materials.new(name+"_m")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (*color, 1)
    bsdf.inputs["Emission Color"].default_value = (*color, 1)
    bsdf.inputs["Emission Strength"].default_value = 4.0
    w.data.materials.clear(); w.data.materials.append(mat)
    w.hide_render = False
    return w


def main():
    log("=== re-render closeup / wire / socket ===")
    bpy.ops.wm.open_mainfile(filepath=str(ASSEMBLY))
    # world should already be mid-grey from last save... but last save was after restore,
    # and setup_neutral was in the same session before save, so world should be mid-grey.
    # Confirm:
    sc = bpy.context.scene
    sc.view_settings.view_transform = "Standard"
    sc.view_settings.exposure = 0.15

    cradle_bake = bpy.data.objects.get("BAKE_CrystalCacheCradle")
    cradle_edit = bpy.data.objects.get("SM_CrystalCacheCradle")
    crystal = bpy.data.objects.get("SM_CrystalCacheCrystal")
    pilot = bpy.data.objects.get("FIG_Pilot_180")
    ucx0 = bpy.data.objects.get("UCX_SM_CrystalCacheCradle_00")
    ucx_c = bpy.data.objects.get("UCX_SM_CrystalCacheCrystal_00")
    src = bpy.data.objects.get("SRC_CrystalCacheCradle")
    if src:
        src.hide_render = True; src.hide_viewport = True
    if cradle_edit:
        cradle_edit.hide_render = True; cradle_edit.hide_viewport = True
    if cradle_bake:
        cradle_bake.hide_render = False; cradle_bake.hide_viewport = False
        assign_pbr_material(cradle_bake, "T_CrystalCacheCradle", OUT, emission_strength=2.4)
    if crystal:
        crystal.hide_render = False
        assign_pbr_material(crystal, "T_CrystalCacheCrystal", OUT, emission_strength=2.2)
        crystal.location = Vector((0.0, 0.0, FLOOR_Z))
    if ucx0: ucx0.hide_render = True
    if ucx_c: ucx_c.hide_render = True
    for o in list(bpy.data.objects):
        if o.name.startswith("W_") or o.name.startswith("GIZMO") or o.name.startswith("VIS_"):
            o.hide_render = True

    # CLOSEUP: pulled back to show -Y rim bevels AND crystal facets
    if pilot: pilot.hide_render = True
    render_view(RENDERS / "closeup.png",
                loc=Vector((1.85, -2.55, 1.70)), target=Vector((0.05, -0.05, 1.05)), lens=45)

    # CLAY + feature wire
    clay = make_clay_material()
    for o in (cradle_bake, crystal, pilot):
        if o:
            o.data.materials.clear()
            o.data.materials.append(clay)
            o.hide_render = False
    if pilot:
        pilot.hide_render = False
        pilot.location = Vector((1.35, -2.25, 0.0))
    render_view(RENDERS / "clay.png",
                loc=Vector((4.2, -5.6, 2.35)), target=Vector((0.0, 0.15, 0.75)), lens=45)
    w1 = feature_wire(cradle_bake, "W_cradle", angle=28.0, radius=0.0028)
    w2 = feature_wire(crystal, "W_crystal", angle=20.0, radius=0.0022)
    render_view(RENDERS / "clay_wire.png",
                loc=Vector((4.2, -5.6, 2.35)), target=Vector((0.0, 0.15, 0.75)), lens=45)
    w1.hide_render = True
    w2.hide_render = True

    # SOCKET + collision: look down into the bay, pilot off to the +X side
    if pilot:
        pilot.location = Vector((1.85, -2.40, 0.0))
        pilot.hide_render = False
    vis = []
    if ucx0:
        vis.append(vis_wire_box(ucx0, "VIS_UCX_cradle", (0.25, 0.95, 0.30), 0.020))
    if ucx_c:
        vis.append(vis_wire_box(ucx_c, "VIS_UCX_crystal", (0.25, 0.55, 1.0), 0.014))
    gizmos = socket_gizmo(Vector((0, 0, FLOOR_Z)))
    render_view(RENDERS / "collision_socket.png",
                loc=Vector((3.0, -4.6, 4.2)), target=Vector((0.0, 0.15, 0.35)), lens=35)
    log("=== panels done (blend not saved) ===")


if __name__ == "__main__":
    main()

"""Render helpers carried over from mesh_r1 (feature-edge overlay, UCX wire, HDRI rotation)."""
import math
import bpy, bmesh
from mathutils import Vector


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
    mod.use_even_offset = False  # even offset spikes on sliver hull triangles
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


def rotate_hdri(degrees=125.0):
    world = bpy.context.scene.world
    if not world or not world.node_tree:
        return
    nt = world.node_tree
    env = next((n for n in nt.nodes if n.type == "TEX_ENVIRONMENT"), None)
    if env is None:
        return
    maps = nt.nodes.new("ShaderNodeMapping")
    coord = nt.nodes.new("ShaderNodeTexCoord")
    maps.inputs["Rotation"].default_value = (0, 0, math.radians(degrees))
    # unlink previous
    for l in list(env.inputs["Vector"].links):
        nt.links.remove(l)
    nt.links.new(coord.outputs["Generated"], maps.inputs["Vector"])
    nt.links.new(maps.outputs["Vector"], env.inputs["Vector"])
    # slightly dim the env so the sun is less of a blown disk; area lights carry shape
    bg = next((n for n in nt.nodes if n.type == "BACKGROUND" and n.inputs[0].is_linked), None)
    if bg:
        bg.inputs["Strength"].default_value = 0.7


def ucx_edge_overlay(ucx_objs, name="VIS_UCX", color=(0.20, 0.55, 1.0), radius=0.004):
    """UCX hull edges as thin emissive curve tubes (a Wireframe modifier spikes on sliver hull triangles)."""
    out = []
    for u in ucx_objs:
        w = feature_wire(u, name + "_" + u.name[-2:], angle=0.0, radius=radius)
        m = w.data.materials[0]
        b = m.node_tree.nodes["Principled BSDF"]
        b.inputs["Base Color"].default_value = (*color, 1)
        b.inputs["Emission Color"].default_value = (*color, 1)
        b.inputs["Emission Strength"].default_value = 2.5
        out.append(w)
    return out

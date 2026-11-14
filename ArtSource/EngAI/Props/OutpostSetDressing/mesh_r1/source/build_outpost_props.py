#!/usr/bin/env python3
"""Route C procedural hard-surface builder for #40 Outpost / landing-pad set dressing.

Authored in metres, +X forward, +Z up, pivot at bottom centre (Z=0).
Exported with apply_unit_scale + FBX_SCALE_UNITS so UnitScaleFactor=100
(Unreal import scale 1.0, sizes in cm).

    blender -b --factory-startup --python build_outpost_props.py -- <tex_dir> <out_dir>
"""
from __future__ import annotations

import math
import os
import sys
from mathutils import Matrix, Vector, Euler

import bmesh
import bpy

# --------------------------------------------------------------------------- atlas (must match make_textures.py)
ATLAS = {
    "gun":         {"u": (0.00, 1.00), "v": (0.875, 1.000), "tile": True},
    "gun2":        {"u": (0.00, 1.00), "v": (0.750, 0.875), "tile": True},
    "yellow":      {"u": (0.00, 1.00), "v": (0.625, 0.750), "tile": True},
    "strap":       {"u": (0.00, 1.00), "v": (0.500, 0.625), "tile": True},
    "hazard":      {"u": (0.00, 1.00), "v": (0.4375, 0.500), "tile": True},
    "metal":       {"u": (0.00, 1.00), "v": (0.375, 0.4375), "tile": True},
    "vent":        {"u": (0.00, 1.00), "v": (0.3125, 0.375), "tile": True},
    "plastic":     {"u": (0.00, 1.00), "v": (0.250, 0.3125), "tile": True},
    "screen_term": {"u": (0.00, 0.25), "v": (0.00, 0.25), "tile": False},
    "screen_kiosk":{"u": (0.25, 0.50), "v": (0.00, 0.25), "tile": False},
    "keyboard":    {"u": (0.50, 0.75), "v": (0.125, 0.25), "tile": False},
    "warn":        {"u": (0.50, 0.75), "v": (0.00, 0.125), "tile": False},
    "flood":       {"u": (0.75, 0.875), "v": (0.125, 0.25), "tile": False},
    "bollard":     {"u": (0.75, 0.875), "v": (0.00, 0.125), "tile": False},
    "amber":       {"u": (0.875, 1.00), "v": (0.125, 0.25), "tile": False},
    "led":         {"u": (0.875, 1.00), "v": (0.00, 0.125), "tile": False},
}

TEXEL = 512.0  # px / metre at the 1K atlas
ATLAS_RES = 1024.0
SEG_CYL = 6  # 60° dihedral; 1-seg bevel at 35° stays hard (0% wobble)


def _argv():
    if "--" in sys.argv:
        a = sys.argv[sys.argv.index("--") + 1:]
    else:
        a = []
    here = os.path.dirname(os.path.abspath(__file__))
    mesh = os.path.dirname(here)
    tex = a[0] if len(a) > 0 else mesh
    out = a[1] if len(a) > 1 else mesh
    return tex, out


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    scene.unit_settings.length_unit = "METERS"
    # hide default, start clean
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    for mesh in list(bpy.data.meshes):
        bpy.data.meshes.remove(mesh)
    for mat in list(bpy.data.materials):
        bpy.data.materials.remove(mat)
    for coll in list(bpy.data.collections):
        bpy.data.collections.remove(coll)


def ensure_coll(name, parent=None):
    coll = bpy.data.collections.get(name)
    if coll is None:
        coll = bpy.data.collections.new(name)
        (parent or bpy.context.scene.collection).children.link(coll)
    return coll


def new_obj(name, bm, collection):
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    if not me.uv_layers:
        me.uv_layers.new(name="UVMap")
    obj = bpy.data.objects.new(name, me)
    ensure_coll(collection).objects.link(obj)
    return obj


def _frac(x):
    return x - math.floor(x)


def map_uv(su, sv, region, td=TEXEL, res=ATLAS_RES):
    u0, u1 = region["u"]
    v0, v1 = region["v"]
    du, dv = u1 - u0, v1 - v0
    if region.get("tile", True):
        fu = _frac(su * td / (res * du))
        fv = _frac(sv * td / (res * dv))
        return u0 + fu * du, v0 + fv * dv
    return u0 + su * du, v0 + sv * dv


def apply_box_uv(obj, region_name, unique=False, unique_axis=None):
    """Box-project UVs into an atlas region. unique=True fits the dominant face into the island."""
    region = ATLAS[region_name]
    me = obj.data
    uv = me.uv_layers.get("UVMap") or me.uv_layers.new(name="UVMap")
    # world-space (object is already at the identity after we bake transforms)
    if unique:
        # find faces whose normal is closest to unique_axis (default: the largest face)
        if unique_axis is None:
            unique_axis = max(me.polygons, key=lambda p: p.area).normal.copy()
        unique_axis = Vector(unique_axis).normalized()
        # collect loops of matching faces and fit 0-1
        idxs = [p.index for p in me.polygons if p.normal.dot(unique_axis) > 0.55]
        if not idxs:
            idxs = [max(me.polygons, key=lambda p: p.area).index]
        # 2D basis
        n = unique_axis
        aux = Vector((0, 0, 1)) if abs(n.z) < 0.8 else Vector((1, 0, 0))
        t = n.cross(aux)
        if t.length < 1e-8:
            t = n.cross(Vector((0, 1, 0)))
        t.normalize()
        b = n.cross(t).normalized()
        pts = []
        for pi in idxs:
            for li in me.polygons[pi].loop_indices:
                co = me.vertices[me.loops[li].vertex_index].co
                pts.append((co.dot(t), co.dot(b), li))
        us = [p[0] for p in pts]
        vs = [p[1] for p in pts]
        umin, umax = min(us), max(us)
        vmin, vmax = min(vs), max(vs)
        duw = max(umax - umin, 1e-6)
        dvw = max(vmax - vmin, 1e-6)
        # pad 2%
        for su, sv, li in pts:
            uu = (su - umin) / duw
            vv = (sv - vmin) / dvw
            uu = 0.03 + 0.94 * uu
            vv = 0.03 + 0.94 * vv
            uv.data[li].uv = (region["u"][0] + uu * (region["u"][1] - region["u"][0]),
                              region["v"][0] + vv * (region["v"][1] - region["v"][0]))
        # remaining faces: tile gun
        gun = ATLAS["gun"]
        used = set()
        for pi in idxs:
            for li in me.polygons[pi].loop_indices:
                used.add(li)
        for poly in me.polygons:
            axis = max(range(3), key=lambda i: abs(poly.normal[i]))
            for li in poly.loop_indices:
                if li in used:
                    continue
                co = me.vertices[me.loops[li].vertex_index].co
                su, sv = ((co.y, co.z), (co.x, co.z), (co.x, co.y))[axis]
                uv.data[li].uv = map_uv(su, sv, gun)
        return

    for poly in me.polygons:
        axis = max(range(3), key=lambda i: abs(poly.normal[i]))
        for li in poly.loop_indices:
            co = me.vertices[me.loops[li].vertex_index].co
            su, sv = ((co.y, co.z), (co.x, co.z), (co.x, co.y))[axis]
            uv.data[li].uv = map_uv(su, sv, region)


def apply_cyl_uv(obj, region_name, axis="Z"):
    region = ATLAS[region_name]
    me = obj.data
    uv = me.uv_layers.get("UVMap") or me.uv_layers.new(name="UVMap")
    ax = axis.upper()
    for poly in me.polygons:
        for li in poly.loop_indices:
            co = me.vertices[me.loops[li].vertex_index].co
            if ax == "Z":
                ang = math.atan2(co.y, co.x)
                along = co.z
                rad = math.hypot(co.x, co.y)
            elif ax == "X":
                ang = math.atan2(co.z, co.y)
                along = co.x
                rad = math.hypot(co.y, co.z)
            else:
                ang = math.atan2(co.z, co.x)
                along = co.y
                rad = math.hypot(co.x, co.z)
            su = (ang / (2.0 * math.pi)) * (2.0 * math.pi * max(rad, 1e-4))
            sv = along
            # caps: radial
            if abs(poly.normal.dot({"Z": Vector((0, 0, 1)), "X": Vector((1, 0, 0)), "Y": Vector((0, 1, 0))}[ax])) > 0.8:
                if ax == "Z":
                    su, sv = co.x, co.y
                elif ax == "X":
                    su, sv = co.y, co.z
                else:
                    su, sv = co.x, co.z
            uv.data[li].uv = map_uv(su, sv, region)


def add_box(name, size, center, region="gun", collection="Visual", unique=False,
            unique_axis=None, rotation=None):
    bm = bmesh.new()
    mx = Matrix.Translation(Vector(center))
    if rotation is not None:
        if isinstance(rotation, Euler):
            mx = mx @ rotation.to_matrix().to_4x4()
        else:
            mx = mx @ rotation
    mx = mx @ Matrix.Diagonal((*size, 1.0))
    bmesh.ops.create_cube(bm, size=1.0, matrix=mx, calc_uvs=True)
    obj = new_obj(name, bm, collection)
    apply_box_uv(obj, region, unique=unique, unique_axis=unique_axis)
    return obj


def add_cyl(name, radius, depth, center, region="gun", collection="Visual",
            segments=SEG_CYL, axis="Z", radius_top=None, rotation=None):
    bm = bmesh.new()
    rot = {"X": Matrix.Rotation(math.radians(90), 4, "Y"),
           "Y": Matrix.Rotation(math.radians(-90), 4, "X"),
           "Z": Matrix.Identity(4)}[axis.upper()]
    mx = Matrix.Translation(Vector(center))
    if rotation is not None:
        mx = mx @ (rotation.to_matrix().to_4x4() if isinstance(rotation, Euler) else rotation)
    mx = mx @ rot
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=segments,
                          radius1=radius, radius2=radius if radius_top is None else radius_top,
                          depth=depth, matrix=mx, calc_uvs=True)
    obj = new_obj(name, bm, collection)
    apply_cyl_uv(obj, region, axis=axis)
    return obj


def add_bar(name, p0, p1, radius, region="metal", collection="Visual", segments=6):
    p0, p1 = Vector(p0), Vector(p1)
    d = p1 - p0
    length = d.length
    if length < 1e-8:
        return None
    mid = (p0 + p1) * 0.5
    z = Vector((0, 0, 1))
    direction = d.normalized()
    # create_cone is along +Z; rotation_difference(Z, Z) is identity (do NOT special-case
    # with X->direction, that lays vertical posts over on X and inflates the bbox).
    if direction.dot(z) < -0.999:
        rot = Matrix.Rotation(math.pi, 4, "X")
    else:
        rot = z.rotation_difference(direction).to_matrix().to_4x4()
    bm = bmesh.new()
    mx = Matrix.Translation(mid) @ rot
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=segments,
                          radius1=radius, radius2=radius, depth=length, matrix=mx, calc_uvs=True)
    obj = new_obj(name, bm, collection)
    apply_cyl_uv(obj, region, axis="Z")  # local Z after transform is baked into verts... 
    # verts are already in world; cylindrical UV about the bar axis is nicer
    # reuse box-ish via world
    apply_box_uv(obj, region)
    return obj


def add_elbow(name, center, bend_r, tube_r, axis_from, axis_to, region="metal",
              collection="Visual", segments=6, sweep_seg=6):
    """Quarter-torus elbow. axis_from / axis_to are unit vectors pointing along the incoming/outgoing legs."""
    a = Vector(axis_from).normalized()
    b = Vector(axis_to).normalized()
    # torus centre is offset from the corner along -a and -b
    # The corner point is `center`. The torus sits at center - a*bend_r - b*bend_r
    origin = Vector(center) - a * bend_r - b * bend_r
    n = a.cross(b)
    if n.length < 1e-6:
        return None
    n.normalize()
    # build in a local frame: X=a, Y=b, Z=n  (sweep from +X to +Y)
    bm = bmesh.new()
    # manual torus quarter
    for i in range(sweep_seg + 1):
        ang = (math.pi * 0.5) * (i / sweep_seg)
        ring_c = origin + a * bend_r * math.cos(ang) + b * bend_r * math.sin(ang)
        # ring normal is the tangent (-a sin + b cos)
        tang = (-a * math.sin(ang) + b * math.cos(ang)).normalized()
        # ring plane perp to tangent
        ref = n
        x = ref
        y = tang.cross(ref).normalized()
        for k in range(segments):
            th = 2 * math.pi * k / segments
            p = ring_c + (x * math.cos(th) + y * math.sin(th)) * tube_r
            bm.verts.new(p)
    bm.verts.ensure_lookup_table()
    for i in range(sweep_seg):
        for k in range(segments):
            v0 = bm.verts[i * segments + k]
            v1 = bm.verts[i * segments + (k + 1) % segments]
            v2 = bm.verts[(i + 1) * segments + (k + 1) % segments]
            v3 = bm.verts[(i + 1) * segments + k]
            bm.faces.new((v0, v1, v2, v3))
    # caps
    bm.faces.new([bm.verts[k] for k in range(segments)])
    bm.faces.new([bm.verts[sweep_seg * segments + k] for k in range(segments - 1, -1, -1)])
    obj = new_obj(name, bm, collection)
    apply_box_uv(obj, region)
    return obj


def add_dish(name, radius, depth, thickness, center, region="gun2", collection="Visual",
             segments=6, rings=2, rotation=None):
    """Parabolic dish (open front toward local +X after rotation). Comms dish uses 16 x 5: smooth-shaded bowl, hard rim."""
    bm = bmesh.new()
    # parabola z_local = depth * (r/radius)^2, dish opens toward +X in local (rotate later)
    # We'll build in local: axis +X is the dish look, parabola in the YZ plane.
    def point(i, j, r_scale, x_off):
        ang = 2 * math.pi * i / segments
        t = j / rings
        r = radius * t * r_scale
        x = x_off + depth * (t * t)
        y = r * math.cos(ang)
        z = r * math.sin(ang)
        return Vector((x, y, z))

    # outer surface (front, concave when viewed from +X? 
    # We want the dish to receive signal from +X, so the concave side faces +X.
    # vertex X should be smaller at the rim and larger at the centre (centre is further +X? 
    # No: centre (vertex) is the back. Rim is toward the signal.
    # Signal from +X: rim has larger X, vertex has smaller X.
    # x = -depth * t^2  with t=0 at vertex? Then rim at t=1 is x=-depth, vertex at 0.
    # Then we offset so the back is at 0 and rim at +depth? Let's put the BACK of the dish
    # (vertex exterior) at local X=0, rim at local X=+depth, concave facing +X.
    # Front surface (inner, concave): x = depth - depth*(1 - (r/R)^2) = depth * (r/R)^2
    # Wait: vertex r=0 should be at the back (x=0+thickness?). Inner vertex at x=thickness?
    # Simpler: inner surface x = depth * (r/radius)^2, so vertex at 0, rim at depth — that
    # is CONVEX toward +X. We want CONCAVE toward +X, so x = depth * (1 - (r/R)^2):
    # vertex r=0 at x=depth (furthest back? if +X is forward, back is -X).
    #
    # Let's define local +X = dish pointing direction (toward the sky/target).
    # Rim is at the front (max X), vertex is at the back (min X).
    # inner: x = depth * (r/R)^2   → vertex x=0 (back), rim x=depth (front). That's concave
    # when viewed from +X (the front looks "into" a bowl whose bottom is at smaller X). YES.

    verts_inner = []
    verts_outer = []
    for j in range(rings + 1):
        row_i, row_o = [], []
        t = j / rings
        r = radius * t
        # Two-band dish (rings=2): mid ring at 0.28*depth so the outer cone is steep
        # and the meridional dihedral stays above 25° (wobble-free).
        if rings == 2:
            x_i = depth * (0.28 if j == 1 else (0.0 if j == 0 else 1.0))
        else:
            x_i = depth * (t * t)
        x_o = x_i - thickness
        for i in range(segments):
            ang = 2 * math.pi * i / segments
            y = r * math.cos(ang)
            z = r * math.sin(ang)
            row_i.append(bm.verts.new((x_i, y, z)))
            row_o.append(bm.verts.new((x_o, y, z)))
        verts_inner.append(row_i)
        verts_outer.append(row_o)
    # faces
    for j in range(rings):
        for i in range(segments):
            i2 = (i + 1) % segments
            # inner (facing +X roughly at rim, normals should point toward +X / inward)
            bm.faces.new((verts_inner[j][i], verts_inner[j][i2], verts_inner[j + 1][i2], verts_inner[j + 1][i]))
            # outer
            bm.faces.new((verts_outer[j][i2], verts_outer[j][i], verts_outer[j + 1][i], verts_outer[j + 1][i2]))
    # rim join
    for i in range(segments):
        i2 = (i + 1) % segments
        bm.faces.new((verts_inner[rings][i], verts_outer[rings][i], verts_outer[rings][i2], verts_inner[rings][i2]))
    # back cap (vertex ring is a point if we want, but j=0 is the vertex circle of r=0 — all verts coincide?
    # r=0: all i share the same (x,0,0). That's degenerate. Start rings from j=1 and cap with a single vert.
    # I already created 12 stacked verts at the same point. That will be non-manifold / degenerate.
    # Fix: I'll dissolve later... better rebuild with a single vertex at the poles.
    # Let's fix by collapsing ring 0.
    # Actually I'll skip this mesh and rebuild properly below — I'll free and redo.

    bm.free()
    bm = bmesh.new()
    # single inner / outer vertex
    v_i0 = bm.verts.new((0.0, 0.0, 0.0))
    v_o0 = bm.verts.new((-thickness, 0.0, 0.0))
    inner_rows = [[v_i0]]
    outer_rows = [[v_o0]]
    for j in range(1, rings + 1):
        t = j / rings
        r = radius * t
        x_i = depth * (t * t)
        x_o = x_i - thickness
        row_i, row_o = [], []
        for i in range(segments):
            ang = 2 * math.pi * i / segments
            y = r * math.cos(ang)
            z = r * math.sin(ang)
            row_i.append(bm.verts.new((x_i, y, z)))
            row_o.append(bm.verts.new((x_o, y, z)))
        inner_rows.append(row_i)
        outer_rows.append(row_o)
    # first ring fans
    for i in range(segments):
        i2 = (i + 1) % segments
        bm.faces.new((inner_rows[0][0], inner_rows[1][i2], inner_rows[1][i]))
        bm.faces.new((outer_rows[0][0], outer_rows[1][i], outer_rows[1][i2]))
    for j in range(1, rings):
        for i in range(segments):
            i2 = (i + 1) % segments
            bm.faces.new((inner_rows[j][i], inner_rows[j][i2], inner_rows[j + 1][i2], inner_rows[j + 1][i]))
            bm.faces.new((outer_rows[j][i2], outer_rows[j][i], outer_rows[j + 1][i], outer_rows[j + 1][i2]))
    for i in range(segments):
        i2 = (i + 1) % segments
        bm.faces.new((inner_rows[rings][i], outer_rows[rings][i], outer_rows[rings][i2], inner_rows[rings][i2]))

    mx = Matrix.Translation(Vector(center))
    if rotation is not None:
        mx = mx @ (rotation.to_matrix().to_4x4() if isinstance(rotation, Euler) else rotation)
    bmesh.ops.transform(bm, matrix=mx, verts=list(bm.verts))
    obj = new_obj(name, bm, collection)
    apply_box_uv(obj, region)
    return obj


def join_parts(parts, name, collection="Visual"):
    parts = [p for p in parts if p is not None]
    if not parts:
        raise RuntimeError("no parts to join for " + name)
    # bake any leftover transforms
    for o in parts:
        if o.matrix_world != Matrix.Identity(4):
            o.data.transform(o.matrix_world)
            o.matrix_world = Matrix.Identity(4)
    vl = bpy.context.view_layer
    for o in list(vl.objects):
        o.select_set(False)
    for o in parts:
        o.select_set(True)
    vl.objects.active = parts[0]
    if len(parts) > 1:
        bpy.ops.object.join()
    obj = vl.objects.active
    obj.name = name
    obj.data.name = name
    # Do NOT weld overlapping kit parts — welding stacked solids creates non-manifold edges.
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(obj.data)
    bm.free()
    # origin at bottom centre
    set_origin_bottom(obj)
    # move to collection
    for c in list(obj.users_collection):
        c.objects.unlink(obj)
    ensure_coll(collection).objects.link(obj)
    return obj


def set_origin_bottom(obj):
    me = obj.data
    xs = [v.co.x for v in me.vertices]
    ys = [v.co.y for v in me.vertices]
    zs = [v.co.z for v in me.vertices]
    cx = 0.5 * (min(xs) + max(xs))
    cy = 0.5 * (min(ys) + max(ys))
    cz = min(zs)
    off = Vector((cx, cy, cz))
    for v in me.vertices:
        v.co -= off
    obj.location = Vector((0, 0, 0))
    obj.rotation_euler = (0, 0, 0)
    obj.scale = (1, 1, 1)
    return (max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs))


def finish_shading(obj, bevel_width=0.005, bevel_segs=1):
    """smooth_by_angle(30°) + live unapplied Bevel(35°) + WeightedNormal."""
    obj.data.shade_smooth()
    obj.data.set_sharp_from_angle(angle=math.radians(30.0))
    # drop leftover modifiers
    for m in list(obj.modifiers):
        obj.modifiers.remove(m)
    bev = obj.modifiers.new("Bevel", "BEVEL")
    bev.width = bevel_width
    bev.segments = bevel_segs
    bev.limit_method = "ANGLE"
    bev.angle_limit = math.radians(35.0)
    bev.harden_normals = True
    bev.profile = 0.5
    bev.use_clamp_overlap = True
    bev.miter_outer = "MITER_SHARP"
    bev.miter_inner = "MITER_SHARP"
    wn = obj.modifiers.new("WeightedNormal", "WEIGHTED_NORMAL")
    wn.keep_sharp = True
    wn.weight = 50
    wn.mode = "FACE_AREA"
    return obj


def add_ucx(name, size, center, parent, collection="Collision"):
    obj = add_box(name, size, center, region="gun", collection=collection)
    # collision doesn't need fancy UV; hide from render
    obj.hide_render = True
    obj.display_type = "WIRE"
    obj.parent = parent
    # keep world position after parenting: we created in world, so unparent inverse
    obj.matrix_parent_inverse = parent.matrix_world.inverted()
    return obj


def make_material(tex_dir, opengl_normal=False):
    name = "M_OutpostProps"
    if name in bpy.data.materials:
        return bpy.data.materials[name]
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    out.location = (700, 0)
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.location = (350, 0)
    nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])

    def img(fname, noncolor=False):
        path = os.path.join(tex_dir, fname)
        if not os.path.isfile(path):
            # fallback: OpenGL preview path
            alt = os.path.join(os.path.dirname(tex_dir) if False else tex_dir, fname)
            path = path if os.path.isfile(path) else alt
        im = bpy.data.images.load(path)
        im.colorspace_settings.name = "Non-Color" if noncolor else "sRGB"
        node = nt.nodes.new("ShaderNodeTexImage")
        node.image = im
        node.location = (-400, 0)
        return node

    n_name = "T_OutpostProps_N_OpenGL.png" if opengl_normal else "T_OutpostProps_N.png"
    # try preview folder for OpenGL
    npath_candidates = [
        os.path.join(tex_dir, n_name),
        os.path.join(tex_dir, "source", "preview", "T_OutpostProps_N_OpenGL.png"),
        os.path.join(tex_dir, "T_OutpostProps_N.png"),
    ]
    bc = img("T_OutpostProps_BC.png", False)
    orm = img("T_OutpostProps_ORM.png", True)
    emi = img("T_OutpostProps_E.png", False)
    npath = next((p for p in npath_candidates if os.path.isfile(p)), npath_candidates[-1])
    nnode = nt.nodes.new("ShaderNodeTexImage")
    nnode.image = bpy.data.images.load(npath)
    nnode.image.colorspace_settings.name = "Non-Color"
    nnode.location = (-400, -400)
    # If we loaded DirectX, flip G for Blender
    nrm = nt.nodes.new("ShaderNodeNormalMap")
    nrm.location = (50, -350)
    if "OpenGL" not in os.path.basename(npath):
        sep = nt.nodes.new("ShaderNodeSeparateColor")
        comb = nt.nodes.new("ShaderNodeCombineColor")
        inv = nt.nodes.new("ShaderNodeMath")
        inv.operation = "SUBTRACT"
        inv.inputs[0].default_value = 1.0
        sep.location = (-150, -400)
        inv.location = (20, -480)
        comb.location = (180, -400)
        nt.links.new(nnode.outputs["Color"], sep.inputs["Color"])
        nt.links.new(sep.outputs["Red"], comb.inputs["Red"])
        nt.links.new(sep.outputs["Green"], inv.inputs[1])
        nt.links.new(inv.outputs["Value"], comb.inputs["Green"])
        nt.links.new(sep.outputs["Blue"], comb.inputs["Blue"])
        nt.links.new(comb.outputs["Color"], nrm.inputs["Color"])
    else:
        nt.links.new(nnode.outputs["Color"], nrm.inputs["Color"])
    nrm.inputs["Strength"].default_value = 1.0
    nt.links.new(nrm.outputs["Normal"], bsdf.inputs["Normal"])
    nt.links.new(bc.outputs["Color"], bsdf.inputs["Base Color"])
    # split ORM
    sep_o = nt.nodes.new("ShaderNodeSeparateColor")
    sep_o.location = (-80, -80)
    nt.links.new(orm.outputs["Color"], sep_o.inputs["Color"])
    nt.links.new(sep_o.outputs["Green"], bsdf.inputs["Roughness"])
    nt.links.new(sep_o.outputs["Blue"], bsdf.inputs["Metallic"])
    # emission
    nt.links.new(emi.outputs["Color"], bsdf.inputs["Emission Color"])
    bsdf.inputs["Emission Strength"].default_value = 6.0
    return mat


def assign_mat(obj, mat):
    obj.data.materials.clear()
    obj.data.materials.append(mat)


def bbox_size(obj):
    xs = [v.co.x for v in obj.data.vertices]
    ys = [v.co.y for v in obj.data.vertices]
    zs = [v.co.z for v in obj.data.vertices]
    return (max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs), min(xs), min(ys), min(zs), max(xs), max(ys), max(zs))


def tri_count(obj, apply_mods=True):
    if apply_mods:
        deps = bpy.context.evaluated_depsgraph_get()
        ev = obj.evaluated_get(deps)
        me = ev.to_mesh()
        n = sum(len(p.vertices) - 2 for p in me.polygons)
        ev.to_mesh_clear()
        return n
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


# =========================================================================== props
def build_cargo_crate(P):
    """Dark gunmetal crate ~120 x 100 x 100 cm, gold-ish corner armour, front latches + light bar."""
    sx, sy, sz = 1.20, 1.00, 1.00
    hx, hy = sx * 0.5, sy * 0.5
    parts = []
    n = 0

    def N(tag):
        nonlocal n
        n += 1
        return f"p{n}_{tag}"

    # main body slightly inset so rails/corners define the silhouette
    parts.append(add_box(N("body"), (sx - 0.08, sy - 0.08, sz - 0.08), (0, 0, sz * 0.5), "gun"))
    # recessed lid panel
    parts.append(add_box(N("lid"), (sx - 0.36, sy - 0.36, 0.04), (0, 0, sz - 0.03), "gun2"))
    # tiny lid emblem
    parts.append(add_box(N("emblem"), (0.16, 0.05, 0.02), (0.10, 0, sz - 0.008), "metal"))
    # corner armour
    c = 0.16
    for x in (-1, 1):
        for y in (-1, 1):
            for z in (0, 1):
                cz = c * 0.5 if z == 0 else sz - c * 0.5
                parts.append(add_box(N("cap"), (c, c, c), (x * (hx - c * 0.5), y * (hy - c * 0.5), cz), "metal"))
    # edge rails (12 edges minus the corners already covered)
    rw, rh = 0.06, 0.06
    # horizontal X at top/bottom, front/back
    for z in (0.03, sz - 0.03):
        for y in (-hy + 0.03, hy - 0.03):
            parts.append(add_box(N("rail"), (sx - 2 * c, rw, rh), (0, y, z), "metal"))
        for x in (-hx + 0.03, hx - 0.03):
            parts.append(add_box(N("rail"), (rw, sy - 2 * c, rh), (x, 0, z), "metal"))
    # vertical rails
    for x in (-hx + 0.03, hx - 0.03):
        for y in (-hy + 0.03, hy - 0.03):
            parts.append(add_box(N("vrail"), (rw, rh, sz - 2 * c), (x, y, sz * 0.5), "metal"))
    # recessed end panel (-X)
    parts.append(add_box(N("end"), (0.03, sy - 0.36, sz - 0.40), (-hx + 0.05, 0, sz * 0.48), "strap"))
    parts.append(add_box(N("endlbl"), (0.02, 0.16, 0.06), (-hx + 0.03, 0, sz * 0.62), "plastic"))
    # front (+X) panel details
    parts.append(add_box(N("frontp"), (0.025, sy - 0.38, sz - 0.42), (hx - 0.05, 0, sz * 0.48), "gun2"))
    # two vertical latches
    for y in (-0.18, 0.18):
        parts.append(add_box(N("latch"), (0.04, 0.06, 0.22), (hx - 0.03, y, sz * 0.58), "metal"))
        parts.append(add_box(N("latchb"), (0.035, 0.08, 0.05), (hx - 0.02, y, sz * 0.48), "metal"))
    # light bar (top-right of front)
    parts.append(add_box(N("lbar"), (0.02, 0.36, 0.04), (hx - 0.025, 0.16, sz * 0.78), "amber",
                         unique=True, unique_axis=(1, 0, 0)))
    # bottom feet
    for x in (-1, 1):
        for y in (-1, 1):
            parts.append(add_box(N("foot"), (0.14, 0.14, 0.03), (x * (hx - 0.14), y * (hy - 0.14), 0.015), "strap"))
    # gold edge ticks (thin yellow strips near corners on long front)
    for y in (-hy + 0.22, hy - 0.22):
        parts.append(add_box(N("tick"), (0.01, 0.03, 0.10), (hx - 0.015, y, 0.18), "yellow"))
    obj = join_parts(parts, "SM_Outpost_CargoCrate")
    finish_shading(obj, bevel_width=0.006)
    # UCX: one box matching silhouette
    add_ucx("UCX_SM_Outpost_CargoCrate_00", (sx, sy, sz), (0, 0, sz * 0.5), obj)
    return obj, (sx, sy, sz), 1


def build_supply_crate(P):
    """Safety-yellow crate ~110 x 90 x 90 cm with dark straps and a recessed end panel."""
    sx, sy, sz = 1.10, 0.90, 0.90
    hx, hy = sx * 0.5, sy * 0.5
    parts, n = [], 0

    def N(tag):
        nonlocal n
        n += 1
        return f"p{n}_{tag}"

    parts.append(add_box(N("body"), (sx - 0.08, sy - 0.08, sz - 0.08), (0, 0, sz * 0.5), "yellow"))
    parts.append(add_box(N("lid"), (sx - 0.32, sy - 0.32, 0.035), (0, 0, sz - 0.028), "yellow"))
    parts.append(add_box(N("lbl"), (0.18, 0.06, 0.015), (0.16, 0, sz - 0.008), "strap"))
    c = 0.14
    for x in (-1, 1):
        for y in (-1, 1):
            for z in (0, 1):
                cz = c * 0.5 if z == 0 else sz - c * 0.5
                parts.append(add_box(N("cap"), (c, c, c), (x * (hx - c * 0.5), y * (hy - c * 0.5), cz), "strap"))
    # dark straps wrapping over the lid (two bands along Y, across X=const)
    for x in (-0.22, 0.22):
        # top run
        parts.append(add_box(N("strapT"), (0.08, sy - 0.04, 0.03), (x, 0, sz - 0.005), "strap"))
        # front / back drops
        for y in (-hy + 0.02, hy - 0.02):
            parts.append(add_box(N("strapS"), (0.08, 0.04, sz - 0.10), (x, y, sz * 0.5 - 0.02), "strap"))
    # recessed end (-X)
    parts.append(add_box(N("end"), (0.04, sy - 0.32, sz - 0.36), (-hx + 0.04, 0, sz * 0.48), "strap"))
    parts.append(add_box(N("endl"), (0.02, 0.14, 0.05), (-hx + 0.02, 0, sz * 0.62), "plastic"))
    # latches on +X
    for y in (-0.16, 0.16):
        parts.append(add_box(N("latch"), (0.04, 0.055, 0.20), (hx - 0.03, y, sz * 0.58), "metal"))
        parts.append(add_box(N("latchb"), (0.03, 0.07, 0.045), (hx - 0.02, y, sz * 0.47), "metal"))
    # bottom lip
    parts.append(add_box(N("lip"), (sx - 0.10, sy - 0.10, 0.04), (0, 0, 0.02), "strap"))
    obj = join_parts(parts, "SM_Outpost_SupplyCrate")
    finish_shading(obj, bevel_width=0.0055)
    add_ucx("UCX_SM_Outpost_SupplyCrate_00", (sx, sy, sz), (0, 0, sz * 0.5), obj)
    return obj, (sx, sy, sz), 1


print("helpers+crates defined")


def build_field_terminal(P):
    """Sloped field console ~100 x 70 x 125 cm; keyboard ~100 cm, screen centre ~110 cm."""
    sx, sy, sz = 1.00, 0.70, 1.25
    hx, hy = sx * 0.5, sy * 0.5
    parts, n = [], 0

    def N(tag):
        nonlocal n
        n += 1
        return f"p{n}_{tag}"

    # plinth / cabinet
    cab_h = 0.64
    parts.append(add_box(N("cab"), (sx - 0.06, sy - 0.06, cab_h), (0, 0, cab_h * 0.5), "gun"))
    # two front doors
    for y in (-0.16, 0.16):
        parts.append(add_box(N("door"), (0.03, 0.28, 0.42), (hx - 0.04, y, 0.28), "gun2"))
        parts.append(add_box(N("knob"), (0.02, 0.03, 0.03), (hx - 0.022, y - 0.08, 0.30), "metal"))
    # side access + gold tick
    parts.append(add_box(N("side"), (0.10, 0.03, 0.22), (-0.22, -hy + 0.02, 0.34), "plastic"))
    parts.append(add_box(N("gold"), (0.08, 0.02, 0.10), (0.18, -hy + 0.015, 0.50), "yellow"))
    # desk slab (work surface ~0.95-1.02 m)
    desk_z = 0.98
    parts.append(add_box(N("desk"), (sx + 0.04, sy + 0.02, 0.08), (0.02, 0, desk_z), "plastic"))
    # keyboard (unique)
    parts.append(add_box(N("keys"), (0.22, 0.36, 0.025), (0.18, -0.04, desk_z + 0.05), "keyboard",
                         unique=True, unique_axis=(0, 0, 1)))
    # right-side trackball / buttons
    parts.append(add_box(N("pad"), (0.16, 0.18, 0.03), (0.20, 0.22, desk_z + 0.05), "gun2"))
    parts.append(add_cyl(N("ball"), 0.035, 0.03, (0.18, 0.20, desk_z + 0.07), "led", segments=6))
    parts.append(add_box(N("btn"), (0.04, 0.04, 0.02), (0.26, 0.26, desk_z + 0.06), "led",
                         unique=True, unique_axis=(0, 0, 1)))
    # hood / screen housing: a stepped stack approximating the slant
    # lower hood lip
    parts.append(add_box(N("lip"), (0.10, sy - 0.08, 0.06), (-0.28, 0, desk_z + 0.08), "gun"))
    # sloped screen box: rotate around Y so the face looks +X and slightly up
    tilt = math.radians(-25.0)  # negative Y rot pitches the +X face upward
    screen_c = Vector((-0.08, 0.0, 1.06))
    # housing
    rot = Euler((0.0, tilt, 0.0), "XYZ")
    parts.append(add_box(N("hood"), (0.14, sy - 0.10, 0.36), screen_c + Vector((-0.04, 0, 0.02)), "gun",
                         rotation=rot))
    # bezel slightly larger
    parts.append(add_box(N("bezel"), (0.05, 0.52, 0.30), screen_c + Vector((0.05, 0, 0.01)), "plastic",
                         rotation=rot))
    # actual screen (unique)
    parts.append(add_box(N("scr"), (0.03, 0.46, 0.26), screen_c + Vector((0.08, 0, 0.01)), "screen_term",
                         unique=True, unique_axis=rot.to_matrix() @ Vector((1, 0, 0)), rotation=rot))
    # top cap of hood
    parts.append(add_box(N("cap"), (0.22, sy - 0.08, 0.05), (-0.10, 0, sz - 0.04), "gun2"))
    # rear spine
    parts.append(add_box(N("spine"), (0.16, sy - 0.16, 0.50), (-0.38, 0, 0.90), "gun"))
    # bottom skids
    for y in (-hy + 0.08, hy - 0.08):
        parts.append(add_box(N("skid"), (sx - 0.10, 0.08, 0.03), (0, y, 0.015), "strap"))
    obj = join_parts(parts, "SM_Outpost_FieldTerminal")
    finish_shading(obj, bevel_width=0.0045)
    # UCX: base + upper hood
    add_ucx("UCX_SM_Outpost_FieldTerminal_00", (sx, sy, cab_h + 0.12), (0, 0, (cab_h + 0.12) * 0.5), obj)
    add_ucx("UCX_SM_Outpost_FieldTerminal_01", (0.70, sy - 0.04, 0.52), (-0.08, 0, 1.00), obj)
    return obj, (sx, sy, sz), 2


def build_data_kiosk(P):
    """Upright kiosk ~110 x 85 x 145 cm, screen centre ~110 cm."""
    sx, sy, sz = 1.10, 0.85, 1.45
    hx, hy = sx * 0.5, sy * 0.5
    parts, n = [], 0

    def N(tag):
        nonlocal n
        n += 1
        return f"p{n}_{tag}"

    # main body (slightly inset)
    parts.append(add_box(N("body"), (sx - 0.10, sy - 0.06, sz - 0.08), (0, 0, (sz - 0.08) * 0.5 + 0.02), "gun"))
    # chamfered / stepped shoulders at the top
    parts.append(add_box(N("shldr"), (sx - 0.22, sy - 0.06, 0.18), (0, 0, sz - 0.10), "plastic"))
    parts.append(add_box(N("top"), (sx - 0.30, sy - 0.14, 0.08), (0, 0, sz - 0.03), "gun2"))
    # gold corner ticks on the shoulders
    for y in (-hy + 0.10, hy - 0.10):
        for x in (-hx + 0.20, hx - 0.20):
            parts.append(add_box(N("gold"), (0.08, 0.04, 0.08), (x, y, sz - 0.12), "yellow"))
    # front upper recess + screen (centre ~1.10)
    parts.append(add_box(N("recess"), (0.06, 0.58, 0.40), (hx - 0.08, 0, 1.12), "strap"))
    parts.append(add_box(N("bezel"), (0.04, 0.50, 0.34), (hx - 0.05, 0, 1.12), "plastic"))
    parts.append(add_box(N("scr"), (0.025, 0.44, 0.28), (hx - 0.03, 0, 1.12), "screen_kiosk",
                         unique=True, unique_axis=(1, 0, 0)))
    # mid keypad / card reader
    parts.append(add_box(N("mid"), (0.05, 0.50, 0.16), (hx - 0.04, 0, 0.82), "gun2"))
    parts.append(add_box(N("keys"), (0.02, 0.22, 0.10), (hx - 0.02, -0.08, 0.82), "keyboard",
                         unique=True, unique_axis=(1, 0, 0)))
    parts.append(add_box(N("card"), (0.04, 0.10, 0.08), (hx - 0.02, 0.18, 0.82), "plastic"))
    parts.append(add_box(N("slot"), (0.03, 0.08, 0.012), (hx - 0.005, 0.18, 0.80), "strap"))
    # lower door
    parts.append(add_box(N("door"), (0.03, 0.62, 0.58), (hx - 0.04, 0, 0.40), "gun2"))
    # gold warning triangle on the door
    parts.append(add_box(N("tri"), (0.02, 0.12, 0.12), (hx - 0.02, 0, 0.28), "warn",
                         unique=True, unique_axis=(1, 0, 0)))
    # side panels (slightly darker)
    for y in (-hy + 0.03, hy - 0.03):
        parts.append(add_box(N("side"), (sx - 0.22, 0.03, sz - 0.30), (0, y, sz * 0.48), "gun2"))
    # base plinth
    parts.append(add_box(N("plinth"), (sx - 0.04, sy - 0.04, 0.08), (0, 0, 0.04), "metal"))
    # vertical edge rails
    for x in (-hx + 0.03, hx - 0.03):
        for y in (-hy + 0.03, hy - 0.03):
            parts.append(add_box(N("rail"), (0.06, 0.06, sz - 0.10), (x, y, sz * 0.5), "metal"))
    obj = join_parts(parts, "SM_Outpost_DataKiosk")
    finish_shading(obj, bevel_width=0.005)
    add_ucx("UCX_SM_Outpost_DataKiosk_00", (sx, sy, 0.72), (0, 0, 0.36), obj)
    add_ucx("UCX_SM_Outpost_DataKiosk_01", (sx - 0.16, sy - 0.04, 0.76), (0, 0, 1.07), obj)
    return obj, (sx, sy, sz), 2


def build_comm_antenna(P):
    """Tall comm mast ~90 x 90 x 260 cm: drum base, twin canisters, stepped whip."""
    sx = sy = 0.90
    sz = 2.60
    parts, n = [], 0

    def N(tag):
        nonlocal n
        n += 1
        return f"p{n}_{tag}"

    # drum base
    parts.append(add_box(N("plinth"), (0.90, 0.90, 0.04), (0, 0, 0.02), "strap"))
    parts.append(add_cyl(N("base"), 0.42, 0.14, (0, 0, 0.10), "metal", segments=6))
    parts.append(add_cyl(N("base2"), 0.37, 0.08, (0, 0, 0.18), "gun", segments=6))
    # yellow markings on the drum (thin boxes sitting on the side)
    for ang in (0, 120, 240):
        a = math.radians(ang)
        parts.append(add_box(N("ymark"), (0.04, 0.12, 0.08),
                             (math.cos(a) * 0.43, math.sin(a) * 0.43, 0.10), "yellow"))
    # neck / instrument stack
    parts.append(add_cyl(N("neck"), 0.09, 0.22, (0, 0, 0.32), "strap", segments=6))
    parts.append(add_cyl(N("col"), 0.11, 0.50, (0, 0, 0.66), "metal", segments=6))
    parts.append(add_cyl(N("col2"), 0.08, 0.28, (0, 0, 1.02), "gun", segments=6))
    # greeble boxes on the column
    parts.append(add_box(N("box"), (0.10, 0.08, 0.14), (0.12, 0, 0.70), "gun2"))
    parts.append(add_box(N("box2"), (0.06, 0.06, 0.10), (0.11, 0.08, 0.58), "yellow"))
    # side canisters (left / right in Y)
    for y, zoff in ((-0.28, 0.0), (0.28, 0.06)):
        parts.append(add_cyl(N("can"), 0.09, 0.46, (0.0, y, 0.92 + zoff), "metal", segments=6))
        parts.append(add_cyl(N("cant"), 0.07, 0.04, (0.0, y, 1.16 + zoff), "plastic", segments=6))
        # support arm
        parts.append(add_bar(N("arm"), (0.0, y * 0.35, 0.70), (0.0, y, 0.74), 0.025, "metal", segments=6))
        parts.append(add_box(N("armb"), (0.06, 0.06, 0.06), (0.0, y * 0.4, 0.70), "gun"))
    # mast, stepped
    parts.append(add_cyl(N("mast1"), 0.045, 0.70, (0, 0, 1.50), "metal", segments=6))
    parts.append(add_cyl(N("collar"), 0.055, 0.06, (0, 0, 1.86), "strap", segments=6))
    parts.append(add_cyl(N("mast2"), 0.032, 0.62, (0, 0, 2.20), "metal", segments=6))
    parts.append(add_cyl(N("tip"), 0.028, 0.10, (0, 0, 2.55), "plastic", segments=6))
    obj = join_parts(parts, "SM_Outpost_CommAntenna")
    finish_shading(obj, bevel_width=0.004)
    add_ucx("UCX_SM_Outpost_CommAntenna_00", (sx, sy, 0.24), (0, 0, 0.12), obj)
    add_ucx("UCX_SM_Outpost_CommAntenna_01", (0.70, 0.70, 1.20), (0, 0, 0.84), obj)
    add_ucx("UCX_SM_Outpost_CommAntenna_02", (0.16, 0.16, 1.20), (0, 0, 2.00), obj)
    return obj, (sx, sy, sz), 3


def build_comms_dish(P):
    """Parabolic comms dish ~160 x 160 x 230 cm on a marked pedestal."""
    sx = sy = 1.60
    sz = 2.30
    parts, n = [], 0

    def N(tag):
        nonlocal n
        n += 1
        return f"p{n}_{tag}"

    # base plinth with corner feet
    parts.append(add_box(N("base"), (1.10, 1.10, 0.16), (0, 0, 0.10), "gun"))
    parts.append(add_box(N("base2"), (0.86, 0.86, 0.14), (0, 0, 0.22), "gun2"))
    for x in (-1, 1):
        for y in (-1, 1):
            parts.append(add_box(N("foot"), (0.28, 0.28, 0.10), (x * 0.66, y * 0.66, 0.05), "metal"))
            parts.append(add_box(N("pad"), (0.20, 0.14, 0.04), (x * 0.66, y * 0.66, 0.12), "yellow"))
    # hazard patches on the plinth faces
    for y in (-0.55, 0.55):
        parts.append(add_box(N("haz"), (0.16, 0.03, 0.10), (0.20, y, 0.16), "hazard",
                             unique=True, unique_axis=(0, 1 if y > 0 else -1, 0)))
    parts.append(add_box(N("haz2"), (0.03, 0.14, 0.10), (0.55, 0.0, 0.16), "hazard",
                         unique=True, unique_axis=(1, 0, 0)))
    # warning triangles
    for y in (-0.30, 0.30):
        parts.append(add_box(N("tri"), (0.03, 0.10, 0.10), (0.44, y, 0.22), "warn",
                             unique=True, unique_axis=(1, 0, 0)))
    # tall stepped pedestal (needed so a 160 cm dish can reach the 230 cm overall height)
    parts.append(add_cyl(N("ring"), 0.24, 0.10, (0, 0, 0.36), "metal", segments=6))
    parts.append(add_cyl(N("col"), 0.16, 0.50, (0, 0, 0.66), "gun", segments=6))
    parts.append(add_cyl(N("col2"), 0.13, 0.28, (0, 0, 1.04), "metal", segments=6))
    # elevation joint (a short cylinder on Y)
    parts.append(add_cyl(N("elv"), 0.11, 0.30, (0.02, 0, 1.22), "metal", segments=6, axis="Y"))
    parts.append(add_box(N("fork"), (0.18, 0.24, 0.18), (0.02, 0, 1.22), "gun2"))
    # dish: radius 0.78 (156 cm), depth 0.42, tilted up toward +X
    # z_top ≈ dish_back.z + depth*sin(θ) + radius*cos(θ) = 1.42 + 0.42*sin(38) + 0.78*cos(38) ≈ 2.29
    tilt = Euler((0.0, math.radians(-38.0), 0.0), "XYZ")
    dish_back = Vector((0.08, 0.0, 1.42))
    parts.append(add_dish(N("dish"), radius=0.78, depth=0.42, thickness=0.028,
                          center=dish_back, region="gun2", segments=16, rings=5, rotation=tilt))
    # pale reflector liner inside the bowl (same parabola, 4 mm proud) so the concave face reads
    # against the dark shell; the 4 cm of shell left outside it is the dark rim lip
    tilt_m = tilt.to_matrix()
    parts.append(add_dish(N("liner"), radius=0.74, depth=0.42 * (0.74 / 0.78) ** 2, thickness=0.010,
                          center=dish_back + tilt_m @ Vector((0.014, 0.0, 0.0)), region="plastic",
                          segments=16, rings=5, rotation=tilt))
    # radial ribs: polyline hugging the back (outer) surface, so nothing pokes through the bowl
    R = tilt.to_matrix()
    for i in range(6):
        ang = math.pi * i / 6.0 + math.pi / 12.0
        ts = (0.10, 0.40, 0.70, 0.97)
        pts = [dish_back + R @ Vector((0.42 * t * t - 0.028 - 0.016, 0.78 * t * math.cos(ang),
                                       0.78 * t * math.sin(ang))) for t in ts]
        for k in range(len(pts) - 1):
            parts.append(add_bar(N("rib"), pts[k], pts[k + 1], 0.014, "metal", segments=6))
    # feed horn + 3 struts
    feed = dish_back + R @ Vector((0.42 + 0.22, 0.0, 0.0))
    parts.append(add_cyl(N("feed"), 0.04, 0.16, feed, "metal", segments=6,
                         rotation=tilt, axis="X"))
    parts.append(add_cyl(N("feed2"), 0.055, 0.05, feed + R @ Vector((0.08, 0, 0)), "plastic",
                         segments=6, rotation=tilt, axis="X"))
    # three struts from rim to feed
    for i in range(3):
        ang = math.radians(90 + i * 120)
        rim = dish_back + R @ Vector((0.42 * 0.87 * 0.87 + 0.03, 0.68 * math.cos(ang), 0.68 * math.sin(ang)))
        parts.append(add_bar(N("strut"), rim, feed, 0.012, "metal", segments=6))
    obj = join_parts(parts, "SM_Outpost_CommsDish")
    finish_shading(obj, bevel_width=0.0045)
    add_ucx("UCX_SM_Outpost_CommsDish_00", (1.20, 1.20, 0.40), (0, 0, 0.20), obj)
    add_ucx("UCX_SM_Outpost_CommsDish_01", (1.50, 1.50, 1.20), (0.15, 0, 1.35), obj)
    return obj, (sx, sy, sz), 2


def build_floodlight(P):
    """Six-head worklight tower ~95 x 85 x 220 cm on a yellow tripod."""
    sx, sy, sz = 0.95, 0.85, 2.20
    parts, n = [], 0

    def N(tag):
        nonlocal n
        n += 1
        return f"p{n}_{tag}"

    # central mast
    parts.append(add_cyl(N("pole1"), 0.045, 1.30, (0, 0, 0.90), "metal", segments=6))
    parts.append(add_cyl(N("collar"), 0.055, 0.08, (0, 0, 1.56), "strap", segments=6))
    parts.append(add_cyl(N("pole2"), 0.035, 0.42, (0, 0, 1.80), "metal", segments=6))
    # head yoke
    parts.append(add_box(N("yoke"), (0.10, 0.70, 0.08), (0.0, 0, 2.04), "gun"))
    parts.append(add_cyl(N("hjoint"), 0.04, 0.12, (0, 0, 2.00), "metal", segments=6))
    # 6 lamp heads: 2 rows x 3, facing +X, slightly down
    tilt = Euler((0.0, math.radians(-8.0), 0.0), "XYZ")
    xs = 0.08
    for row, z in enumerate((2.10, 1.86)):
        for i, y in enumerate((-0.26, 0.0, 0.26)):
            c = Vector((xs, y, z))
            parts.append(add_box(N("hous"), (0.10, 0.22, 0.18), c, "gun", rotation=tilt))
            # lamp face (unique flood)
            parts.append(add_box(N("lamp"), (0.03, 0.18, 0.14), c + Vector((0.065, 0, 0)), "flood",
                                 unique=True, unique_axis=tilt.to_matrix() @ Vector((1, 0, 0)),
                                 rotation=tilt))
            # side ear
            if i != 1:
                parts.append(add_box(N("ear"), (0.06, 0.03, 0.08), c + Vector((0.0, 0.12 if y > 0 else -0.12, 0)), "strap"))
    # tripod: 3 legs splayed, yellow upper + metal lower, dark feet
    # footprint 0.95 x 0.85: place feet on an ellipse
    foot_r = 0.50
    # angles so one leg is back (-X) and two are forward-ish, matching the concept
    for i, ang in enumerate((math.radians(180), math.radians(55), math.radians(-55))):
        fx = math.cos(ang) * foot_r * (0.95 / 0.90)
        fy = math.sin(ang) * foot_r * (0.85 / 0.90)
        foot = Vector((fx, fy, 0.04))
        hip = Vector((0.0, 0.0, 0.70))
        # yellow upper half
        mid = hip.lerp(foot, 0.45)
        parts.append(add_bar(N("legY"), hip, mid, 0.028, "yellow", segments=6))
        parts.append(add_bar(N("legM"), mid, foot + Vector((0, 0, 0.06)), 0.022, "metal", segments=6))
        parts.append(add_box(N("foot"), (0.10, 0.10, 0.05), (fx, fy, 0.025), "strap"))
        # knee joint
        parts.append(add_cyl(N("knee"), 0.035, 0.06, mid, "metal", segments=6))
    # hip collar
    parts.append(add_cyl(N("hip"), 0.07, 0.08, (0, 0, 0.70), "gun", segments=6))
    # small cross braces near the hip
    for ang in (math.radians(180), math.radians(55), math.radians(-55)):
        p = Vector((math.cos(ang) * 0.16, math.sin(ang) * 0.16, 0.55))
        parts.append(add_bar(N("brace"), (0, 0, 0.68), p, 0.012, "metal", segments=6))
    obj = join_parts(parts, "SM_Outpost_Floodlight")
    finish_shading(obj, bevel_width=0.0035)
    add_ucx("UCX_SM_Outpost_Floodlight_00", (0.36, 0.36, 1.50), (0, 0, 0.75), obj)
    add_ucx("UCX_SM_Outpost_Floodlight_01", (0.30, 0.80, 0.50), (0.05, 0, 1.88), obj)
    add_ucx("UCX_SM_Outpost_Floodlight_02", (sx, sy, 0.12), (0, 0, 0.06), obj)
    return obj, (sx, sy, sz), 3


def build_bollard_light(P):
    """Landing-pad perimeter bollard ~45 x 45 x 110 cm with a tall amber window."""
    sx = sy = 0.45
    sz = 1.10
    parts, n = [], 0

    def N(tag):
        nonlocal n
        n += 1
        return f"p{n}_{tag}"

    # octagonal-ish 12-sided post
    parts.append(add_cyl(N("post"), 0.125, 0.78, (0, 0, 0.62), "gun", segments=6))
    # cap
    parts.append(add_cyl(N("cap"), 0.135, 0.10, (0, 0, 1.05), "plastic", segments=6))
    parts.append(add_cyl(N("cap2"), 0.10, 0.03, (0, 0, 1.09), "gun2", segments=6))
    # small red indicator on the cap
    parts.append(add_box(N("ind"), (0.03, 0.04, 0.02), (0.12, 0.0, 1.06), "led",
                         unique=True, unique_axis=(1, 0, 0)))
    # tall front light panel
    parts.append(add_box(N("win"), (0.04, 0.14, 0.52), (0.12, 0.0, 0.68), "bollard",
                         unique=True, unique_axis=(1, 0, 0)))
    # bezel around the window
    parts.append(add_box(N("bez"), (0.03, 0.17, 0.56), (0.105, 0.0, 0.68), "metal"))
    # mid latch
    parts.append(add_box(N("latch"), (0.03, 0.05, 0.08), (0.13, 0.0, 0.40), "metal"))
    # drum base with arched recesses and rim lights
    parts.append(add_cyl(N("drum"), 0.22, 0.16, (0, 0, 0.08), "metal", segments=6))
    parts.append(add_cyl(N("drum2"), 0.20, 0.04, (0, 0, 0.18), "gun", segments=6))
    # arched recesses (boxes cut visually as inset straps)
    for i in range(6):
        ang = math.radians(i * 60 + 30)
        parts.append(add_box(N("arch"), (0.05, 0.08, 0.08),
                             (math.cos(ang) * 0.20, math.sin(ang) * 0.20, 0.07), "gun2",
                             rotation=Euler((0, 0, ang), "XYZ")))
    # rim amber lights (4)
    for i in range(4):
        ang = math.radians(i * 90 + 45)
        parts.append(add_box(N("rim"), (0.03, 0.08, 0.02),
                             (math.cos(ang) * 0.18, math.sin(ang) * 0.18, 0.17), "amber",
                             unique=True, unique_axis=(0, 0, 1),
                             rotation=Euler((0, 0, ang), "XYZ")))
    obj = join_parts(parts, "SM_Outpost_BollardLight")
    finish_shading(obj, bevel_width=0.0035)
    add_ucx("UCX_SM_Outpost_BollardLight_00", (sx, sy, sz), (0, 0, sz * 0.5), obj)
    return obj, (sx, sy, sz), 1


def build_generator(P):
    """Field generator ~170 x 110 x 125 cm in a tubular roll cage."""
    sx, sy, sz = 1.70, 1.10, 1.25
    parts, n = [], 0

    def N(tag):
        nonlocal n
        n += 1
        return f"p{n}_{tag}"

    # body (yellow) sitting inside the cage
    bx, by, bz = 1.38, 0.78, 0.78
    body_z = 0.18 + bz * 0.5
    parts.append(add_box(N("body"), (bx, by, bz), (0, 0, body_z), "yellow"))
    # slightly raised lid
    parts.append(add_box(N("lid"), (bx - 0.16, by - 0.16, 0.06), (0, 0, body_z + bz * 0.5 + 0.01), "yellow"))
    # top handle
    parts.append(add_bar(N("hdl"), (-0.16, 0, body_z + bz * 0.5 + 0.08),
                         (0.16, 0, body_z + bz * 0.5 + 0.08), 0.018, "metal", segments=6))
    for x in (-0.16, 0.16):
        parts.append(add_box(N("hdlb"), (0.05, 0.04, 0.05), (x, 0, body_z + bz * 0.5 + 0.04), "metal"))
    # front (+X): left control panel, right vents
    parts.append(add_box(N("panel"), (0.04, 0.30, 0.42), (bx * 0.5 + 0.01, -0.16, body_z + 0.02), "strap"))
    parts.append(add_box(N("knob"), (0.03, 0.06, 0.06), (bx * 0.5 + 0.03, -0.16, body_z + 0.12), "metal"))
    parts.append(add_box(N("led"), (0.02, 0.05, 0.03), (bx * 0.5 + 0.03, -0.22, body_z + 0.18), "led",
                         unique=True, unique_axis=(1, 0, 0)))
    parts.append(add_box(N("vent"), (0.04, 0.32, 0.40), (bx * 0.5 + 0.01, 0.20, body_z + 0.00), "vent",
                         unique=True, unique_axis=(1, 0, 0)))
    # side dark access (-Y)
    parts.append(add_box(N("side"), (0.36, 0.04, 0.50), (-0.20, -by * 0.5 - 0.01, body_z), "strap"))
    parts.append(add_box(N("sidew"), (0.12, 0.03, 0.16), (-0.20, -by * 0.5 - 0.03, body_z + 0.08), "plastic"))
    # nameplate
    parts.append(add_box(N("plate"), (0.22, 0.04, 0.06), (0.10, -by * 0.5 - 0.02, body_z + 0.22), "strap"))
    # roll cage — clean square-corner tubular frame (no torus elbows: those blew the bbox
    # and created non-manifold welds). Extents hit 170 x 110 x 125 cm including tube radius.
    r_tube = 0.032
    post_x, post_y = 0.85 - r_tube, 0.55 - r_tube  # centreline so outer skin is 1.70 x 1.10
    z0, z1 = r_tube, 1.25 - r_tube
    for x in (-post_x, post_x):
        for y in (-post_y, post_y):
            parts.append(add_bar(N("post"), (x, y, z0), (x, y, z1), r_tube, "metal", segments=6))
    for z in (z0, z1):
        for y in (-post_y, post_y):
            parts.append(add_bar(N("hx"), (-post_x, y, z), (post_x, y, z), r_tube, "metal", segments=6))
        for x in (-post_x, post_x):
            parts.append(add_bar(N("hy"), (x, -post_y, z), (x, post_y, z), r_tube, "metal", segments=6))
    zmid = 0.62
    for y in (-post_y, post_y):
        parts.append(add_bar(N("mid"), (-post_x, y, zmid), (post_x, y, zmid), r_tube * 0.9, "metal", segments=6))
    # feet
    for x in (-post_x, post_x):
        for y in (-post_y, post_y):
            parts.append(add_cyl(N("foot"), 0.045, 0.04, (x, y, 0.02), "strap", segments=6))
    obj = join_parts(parts, "SM_Outpost_Generator")
    finish_shading(obj, bevel_width=0.004)
    add_ucx("UCX_SM_Outpost_Generator_00", (sx, sy, sz), (0, 0, sz * 0.5), obj)
    return obj, (sx, sy, sz), 1


def build_power_junction(P):
    """Power junction cabinet ~120 x 75 x 135 cm, lightning warning, chunky corners."""
    sx, sy, sz = 1.20, 0.75, 1.35
    hx, hy = sx * 0.5, sy * 0.5
    parts, n = [], 0

    def N(tag):
        nonlocal n
        n += 1
        return f"p{n}_{tag}"

    # body
    parts.append(add_box(N("body"), (sx - 0.12, sy - 0.10, sz - 0.16), (0, 0, (sz - 0.16) * 0.5 + 0.06), "gun"))
    # chunky corner armour
    c = 0.18
    for x in (-1, 1):
        for y in (-1, 1):
            for z in (0, 1):
                cz = c * 0.55 if z == 0 else sz - c * 0.55
                parts.append(add_box(N("cap"), (c, c * 0.85, c), (x * (hx - c * 0.5), y * (hy - c * 0.42), cz), "metal"))
    # edge rails
    for z in (0.06, sz - 0.06):
        for y in (-hy + 0.04, hy - 0.04):
            parts.append(add_box(N("rail"), (sx - 2 * c, 0.06, 0.07), (0, y, z), "metal"))
        for x in (-hx + 0.04, hx - 0.04):
            parts.append(add_box(N("rail"), (0.06, sy - 2 * c + 0.04, 0.07), (x, 0, z), "metal"))
    for x in (-hx + 0.04, hx - 0.04):
        for y in (-hy + 0.04, hy - 0.04):
            parts.append(add_box(N("vrail"), (0.07, 0.06, sz - 2 * c), (x, y, sz * 0.5), "metal"))
    # dark top cap
    parts.append(add_box(N("top"), (sx - 0.28, sy - 0.22, 0.10), (0, 0, sz - 0.08), "strap"))
    parts.append(add_box(N("topl"), (sx - 0.40, sy - 0.32, 0.04), (0, 0, sz - 0.03), "gun2"))
    # front door
    parts.append(add_box(N("door"), (0.03, sy - 0.28, sz - 0.50), (hx - 0.06, 0, sz * 0.48), "gun2"))
    # warning
    parts.append(add_box(N("warn"), (0.02, 0.22, 0.22), (hx - 0.04, 0.04, 0.85), "warn",
                         unique=True, unique_axis=(1, 0, 0)))
    # latch
    parts.append(add_box(N("latch"), (0.04, 0.06, 0.14), (hx - 0.03, -0.16, 0.72), "metal"))
    # gold ticks on the door corners
    for y, z in ((-0.18, 0.32), (0.18, 0.32), (-0.18, 0.95), (0.18, 0.95)):
        parts.append(add_box(N("tick"), (0.02, 0.05, 0.05), (hx - 0.04, y, z), "yellow"))
    # side yellow recessed panels
    for y in (-hy + 0.03, hy - 0.03):
        parts.append(add_box(N("side"), (0.28, 0.03, 0.36), (0.05, y, 0.70), "yellow"))
        parts.append(add_box(N("sidei"), (0.16, 0.02, 0.16), (0.05, y + (0.01 if y > 0 else -0.01), 0.70), "hazard",
                             unique=True, unique_axis=(0, 1 if y > 0 else -1, 0)))
    # bottom plinth
    parts.append(add_box(N("plinth"), (sx - 0.08, sy - 0.06, 0.08), (0, 0, 0.04), "strap"))
    obj = join_parts(parts, "SM_Outpost_PowerJunction")
    finish_shading(obj, bevel_width=0.0055)
    add_ucx("UCX_SM_Outpost_PowerJunction_00", (sx, sy, sz), (0, 0, sz * 0.5), obj)
    return obj, (sx, sy, sz), 1


BUILDERS = [
    ("SM_Outpost_CargoCrate", build_cargo_crate),
    ("SM_Outpost_SupplyCrate", build_supply_crate),
    ("SM_Outpost_FieldTerminal", build_field_terminal),
    ("SM_Outpost_DataKiosk", build_data_kiosk),
    ("SM_Outpost_CommAntenna", build_comm_antenna),
    ("SM_Outpost_CommsDish", build_comms_dish),
    ("SM_Outpost_Floodlight", build_floodlight),
    ("SM_Outpost_BollardLight", build_bollard_light),
    ("SM_Outpost_Generator", build_generator),
    ("SM_Outpost_PowerJunction", build_power_junction),
]


def export_fbx(obj, out_path):
    """Repo convention: apply_unit_scale + FBX_SCALE_UNITS => UnitScaleFactor=100."""
    # select visual + children (UCX)
    family = [obj] + list(obj.children_recursive)
    vl = bpy.context.view_layer
    for o in list(vl.objects):
        o.select_set(False)
    for o in family:
        o.select_set(True)
    vl.objects.active = obj
    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    bpy.ops.export_scene.fbx(
        filepath=out_path,
        use_selection=True,
        object_types={"MESH", "EMPTY"},
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_UNITS",
        global_scale=1.0,
        axis_forward="-Z",
        axis_up="Y",
        mesh_smooth_type="OFF",
        use_tspace=True,
        use_triangles=True,
        use_mesh_modifiers=True,
        add_leaf_bones=False,
        bake_anim=False,
        path_mode="AUTO",
        embed_textures=False,
    )


def export_glb(obj, out_path, tex_dir):
    family = [obj] + [c for c in obj.children_recursive if not c.name.startswith("UCX_")]
    # temp triangulate so the GLB gets tangents (n-gon cylinder caps otherwise fail)
    tri_mod = obj.modifiers.new("Triangulate_export", "TRIANGULATE")
    tri_mod.quad_method = "BEAUTY"
    vl = bpy.context.view_layer
    for o in list(vl.objects):
        o.select_set(False)
    for o in family:
        o.select_set(True)
    vl.objects.active = obj
    try:
        bpy.ops.export_scene.gltf(
        filepath=out_path,
        export_format="GLB",
        use_selection=True,
        export_apply=True,
        export_texcoords=True,
        export_normals=True,
        export_tangents=True,
        export_materials="EXPORT",
        export_cameras=False,
        export_lights=False,
        export_animations=False,
        export_yup=True,
        )
    finally:
        obj.modifiers.remove(tri_mod)


def add_src_copy(obj):
    """Hidden, modifier-free SRC_ copy of the editable cage (R-043 audit). Not exported."""
    src = obj.copy()
    src.data = obj.data.copy()
    src.name = "SRC_" + obj.name
    src.data.name = "SRC_" + obj.data.name
    for m in list(src.modifiers):
        src.modifiers.remove(m)
    src.parent = None
    src.matrix_world = obj.matrix_world.copy()
    coll = bpy.data.collections.get("Source") or bpy.data.collections.new("Source")
    if coll.name not in bpy.context.scene.collection.children:
        bpy.context.scene.collection.children.link(coll)
    coll.objects.link(src)
    src.hide_set(True)
    src.hide_viewport = True
    src.hide_render = True
    src.select_set(False)
    return src


def save_blend(path):
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=path, compress=True, copy=True)


def build_all(tex_dir, out_dir):
    results = []
    for name, fn in BUILDERS:
        print("=" * 60, "\nBUILD", name)
        reset_scene()
        vis = ensure_coll("Visual")
        col = ensure_coll("Collision")
        mat = make_material(tex_dir, opengl_normal=True)
        obj, size, ucx_count = fn(None)
        assign_mat(obj, mat)
        # ensure collections
        for c in list(obj.users_collection):
            if c.name != "Visual":
                c.objects.unlink(obj)
        if obj.name not in vis.objects:
            vis.objects.link(obj)
        sx, sy, sz, *rest = bbox_size(obj)
        print("  bbox m", round(sx, 4), round(sy, 4), round(sz, 4),
              "target", size, "tris_eval", tri_count(obj), "ucx", ucx_count)
        blend_path = os.path.join(out_dir, f"{name}.blend")
        fbx_path = os.path.join(out_dir, f"{name}.fbx")
        glb_path = os.path.join(out_dir, f"{name}.glb")
        add_src_copy(obj)
        save_blend(blend_path)
        export_fbx(obj, fbx_path)
        export_glb(obj, glb_path, tex_dir)
        results.append({
            "name": name,
            "size_m": (sx, sy, sz),
            "size_cm": [round(sx * 100, 1), round(sy * 100, 1), round(sz * 100, 1)],
            "target_cm": [round(size[0] * 100, 1), round(size[1] * 100, 1), round(size[2] * 100, 1)],
            "tris": tri_count(obj),
            "ucx_count": ucx_count,
            "blend": blend_path,
            "fbx": fbx_path,
            "glb": glb_path,
        })
    import json
    meta = os.path.join(out_dir, "source", "build_results.json")
    os.makedirs(os.path.dirname(meta), exist_ok=True)
    with open(meta, "w") as f:
        json.dump(results, f, indent=2)
    print("WROTE", meta)
    return results


if __name__ == "__main__":
    tex, out = _argv()
    print("tex", tex, "out", out)
    build_all(tex, out)

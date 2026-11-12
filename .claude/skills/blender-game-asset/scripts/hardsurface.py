# SPDX-License-Identifier: MIT
"""
hardsurface.py - parametric hard-surface building blocks for stylized sci-fi game assets.

Agents left alone tend to stack boxes into "simplistic, floating primitives". These helpers give
better starting forms (lofted hulls, swept wings, lathed nacelles, canopies), the standard clean
shading stack (bevel + harden normals + weighted normals), detail passes (inset glow panels,
greebles), and context-free utilities (join, origin, apply). Everything uses the data API or
context-overridden operators, so it runs the same in the live session and headless.

    import runpy
    hs = runpy.run_path(r"<repo>/.claude/skills/blender-game-asset/scripts/hardsurface.py")
    hull = hs["loft"]("SM_Ship_Rift", [
        {"y": -3.2, "w": 0.0, "h": 0.0},             # nose tip (w/h 0 = point)
        {"y": -2.2, "w": 0.8, "h": 0.5},
        {"y": 0.4,  "w": 1.5, "h": 0.9, "n": 3.0},   # n: 2 = ellipse, 3-4 = rounded box
        {"y": 2.4,  "w": 0.9, "h": 0.55},
    ], collection="Ship_Rift")
    hs["add_bevel"](hull, width=0.03)
    hs["add_weighted_normal"](hull)
    result = {"hull": hull.name}

Conventions: metres; the asset's front faces -Y (Blender "Front" view), up is +Z; object scale is
kept at 1 (all sizing happens in mesh data). Recommended modifier order:
Mirror -> Boolean(s) -> Bevel -> Weighted Normal (add_* helpers append in call order).
Cutter objects are named CUT_* and hidden from render; exporters and review renders skip them.
"""

import math
import random

import bmesh
import bpy
from mathutils import Matrix, Vector


# ============================================================================ data helpers

def ensure_collection(name, parent=None):
    coll = bpy.data.collections.get(name)
    if coll is None:
        coll = bpy.data.collections.new(name)
        (parent or bpy.context.scene.collection).children.link(coll)
    return coll


def _finish(name, bm, collection, location=(0.0, 0.0, 0.0), smart_uv_angle=66.0):
    if bpy.data.objects.get(name) is not None:
        raise ValueError("Object {!r} already exists; choose another name or delete it first".format(name))
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    ensure_collection(collection).objects.link(obj)
    obj.location = location
    if not mesh.uv_layers:
        smart_uv(obj, angle_deg=smart_uv_angle)
    return obj


def _uv_layer(bm):
    return bm.loops.layers.uv.get("UVMap") or bm.loops.layers.uv.new("UVMap")


def smart_uv(obj, angle_deg=66.0, island_margin=0.02):
    """Smart UV Project. Edit-mode operators read the REAL view-layer active object (a temp_override
    of active_object is not enough for mesh.select_all's poll), so set it for real, then restore.
    Falls back to a box projection only if the operators are unavailable."""
    vl = bpy.context.view_layer
    prev_active = vl.objects.active
    prev_sel = [o for o in vl.objects if o.select_get()]
    try:
        for o in prev_sel:
            o.select_set(False)
        obj.select_set(True)
        vl.objects.active = obj
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.uv.smart_project(angle_limit=math.radians(angle_deg), island_margin=island_margin)
        bpy.ops.object.mode_set(mode="OBJECT")
        method = "smart_project"
    except Exception as ex:
        if obj.mode != "OBJECT":
            bpy.ops.object.mode_set(mode="OBJECT")
        _box_uv(obj)
        method = "box_uv ({})".format(ex.__class__.__name__)
    finally:
        obj.select_set(False)
        for o in prev_sel:
            o.select_set(True)
        vl.objects.active = prev_active
    return method


def _box_uv(obj, scale=0.5):
    mesh = obj.data
    uv = mesh.uv_layers.get("UVMap") or mesh.uv_layers.new(name="UVMap")
    for poly in mesh.polygons:
        axis = max(range(3), key=lambda i: abs(poly.normal[i]))
        for li in poly.loop_indices:
            co = mesh.vertices[mesh.loops[li].vertex_index].co
            u, v = [(co.y, co.z), (co.x, co.z), (co.x, co.y)][axis]
            uv.data[li].uv = (u * scale, v * scale)


# ============================================================================ primitives

def box(name, size=(1.0, 1.0, 1.0), location=(0.0, 0.0, 0.0), collection="Asset"):
    bm = bmesh.new()
    _uv_layer(bm)
    bmesh.ops.create_cube(bm, size=1.0, matrix=Matrix.Diagonal((*size, 1.0)), calc_uvs=True)
    return _finish(name, bm, collection, location)


def cylinder(name, radius=0.5, depth=1.0, segments=24, axis="Z", location=(0.0, 0.0, 0.0),
             collection="Asset", radius_top=None):
    bm = bmesh.new()
    _uv_layer(bm)
    rot = {"X": Matrix.Rotation(math.radians(90), 4, "Y"), "Y": Matrix.Rotation(math.radians(-90), 4, "X"),
           "Z": Matrix.Identity(4)}[axis.upper()]
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=segments, radius1=radius,
                          radius2=radius if radius_top is None else radius_top, depth=depth, matrix=rot,
                          calc_uvs=True)
    return _finish(name, bm, collection, location)


def _ring(w, h, n, verts, y, x0, z0):
    pts = []
    for i in range(verts):
        t = 2.0 * math.pi * i / verts
        c, s = math.cos(t), math.sin(t)
        x = (w * 0.5) * math.copysign(abs(c) ** (2.0 / n), c)
        z = (h * 0.5) * math.copysign(abs(s) ** (2.0 / n), s)
        pts.append(Vector((x + x0, y, z + z0)))
    return pts


def loft(name, sections, ring_verts=16, collection="Asset", location=(0.0, 0.0, 0.0), cap=True):
    """Hull/fuselage from cross-sections along +Y (front of the asset = -Y).

    sections: [{"y": float, "w": width, "h": height, "n": squareness (2 ellipse .. 6 boxy),
                "x": x offset, "z": z offset, "groove": depth, "groove_width": width}, ...] ordered by y.
    w or h of 0 collapses that section to a point (nose/tail tip). "groove" cuts a circumferential
    panel line (e.g. 0.01 deep, 0.02 wide) at that station - the cheapest way to break up a hull
    into readable plates. Produces clean quad rings that bevel and subdivide well."""
    bm = bmesh.new()
    expanded = []
    for s in sections:
        g = s.get("groove", 0.0)
        if g and s["w"] > 1e-6 and s["h"] > 1e-6:
            gw = s.get("groove_width", max(g * 2.0, 0.01))
            expanded.append(dict(s, y=s["y"] - gw * 0.5))
            expanded.append(dict(s, w=s["w"] - 2.0 * g, h=s["h"] - 2.0 * g))
            expanded.append(dict(s, y=s["y"] + gw * 0.5))
        else:
            expanded.append(s)
    rings = []
    for s in expanded:
        x0, z0 = s.get("x", 0.0), s.get("z", 0.0)
        if s["w"] <= 1e-6 or s["h"] <= 1e-6:
            rings.append([bm.verts.new((x0, s["y"], z0))])
        else:
            rings.append([bm.verts.new(p) for p in _ring(s["w"], s["h"], s.get("n", 2.5), ring_verts,
                                                         s["y"], x0, z0)])
    for a, b in zip(rings, rings[1:]):
        if len(a) == 1 and len(b) == 1:
            continue
        if len(a) == 1:
            for i in range(len(b)):
                bm.faces.new((a[0], b[(i + 1) % len(b)], b[i]))
        elif len(b) == 1:
            for i in range(len(a)):
                bm.faces.new((a[i], a[(i + 1) % len(a)], b[0]))
        else:
            for i in range(len(a)):
                bm.faces.new((a[i], a[(i + 1) % len(a)], b[(i + 1) % len(b)], b[i]))
    if cap:
        for ring in (rings[0], rings[-1]):
            if len(ring) > 2:
                bm.faces.new(ring)
    return _finish(name, bm, collection, location)


def lathe(name, profile, segments=24, collection="Asset", location=(0.0, 0.0, 0.0), axis="Y"):
    """Round part (engine nacelle, nozzle, turret barrel) from (position_along_axis, radius) pairs.
    radius 0 closes the end to a point. Built along Y, then rotated for axis X or Z."""
    sections = [{"y": p, "w": 2.0 * r, "h": 2.0 * r, "n": 2.0} for p, r in profile]
    obj = loft(name, sections, ring_verts=segments, collection=collection, location=location)
    if axis.upper() == "X":
        obj.data.transform(Matrix.Rotation(math.radians(-90), 4, "Z"))
    elif axis.upper() == "Z":
        obj.data.transform(Matrix.Rotation(math.radians(90), 4, "X"))
    return obj


def wing(name, root_chord=3.0, tip_chord=1.2, span=4.0, sweep=1.5, thickness=0.2, tip_thickness=None,
         dihedral_deg=0.0, root_x=0.0, root_y=0.0, root_z=0.0, collection="Asset", location=(0.0, 0.0, 0.0)):
    """Tapered, swept slab wing extending toward +X (add_mirror(axis="X") for the other side).
    Leading edge faces -Y; sweep moves the tip backwards (+Y)."""
    tip_t = thickness * 0.5 if tip_thickness is None else tip_thickness

    def airfoil(x, y_le, chord, t, z):
        pts = [(0.0, 0.0), (0.28, 0.5), (0.72, 0.36), (1.0, 0.0), (0.72, -0.36), (0.28, -0.5)]
        return [Vector((x, y_le + px * chord, z + pz * t)) for px, pz in pts]

    tip_z = root_z + span * math.tan(math.radians(dihedral_deg))
    sections = [airfoil(root_x, root_y, root_chord, thickness, root_z),
                airfoil(root_x + span, root_y + sweep, tip_chord, tip_t, tip_z)]
    bm = bmesh.new()
    rings = [[bm.verts.new(p) for p in sec] for sec in sections]
    a, b = rings
    for i in range(len(a)):
        bm.faces.new((a[i], a[(i + 1) % len(a)], b[(i + 1) % len(b)], b[i]))
    bm.faces.new(a)
    bm.faces.new(b)
    return _finish(name, bm, collection, location)


# ============================================================================ modifiers & shading

def smooth_by_angle(obj, angle_deg=30.0):
    """Blender 4.1+ replacement for 'auto smooth': smooth shading + sharp edges above the angle."""
    obj.data.shade_smooth()
    obj.data.set_sharp_from_angle(angle=math.radians(angle_deg))


def add_bevel(obj, width=0.02, segments=3, angle_deg=30.0, harden=True, profile=0.5):
    """Rounded, light-catching edges. harden=True keeps big faces flat-shaded (needs smooth shading)."""
    if harden:
        smooth_by_angle(obj, angle_deg)
    mod = obj.modifiers.new("Bevel", "BEVEL")
    mod.width = width
    mod.segments = segments
    mod.limit_method = "ANGLE"
    mod.angle_limit = math.radians(angle_deg)
    mod.harden_normals = harden
    mod.profile = profile
    mod.use_clamp_overlap = True
    mod.miter_outer = "MITER_ARC"
    return mod


def add_weighted_normal(obj, keep_sharp=True, weight=50, mode="FACE_AREA"):
    mod = obj.modifiers.new("WeightedNormal", "WEIGHTED_NORMAL")
    mod.keep_sharp = keep_sharp
    mod.weight = weight
    mod.mode = mode
    return mod


def add_mirror(obj, axis="X", bisect=True, clip=True, mirror_object=None):
    mod = obj.modifiers.new("Mirror", "MIRROR")
    idx = "XYZ".index(axis.upper())
    mod.use_axis = [i == idx for i in range(3)]
    mod.use_bisect_axis = [bisect and i == idx for i in range(3)]
    mod.use_clip = clip
    if mirror_object is not None:
        mod.mirror_object = mirror_object
    return mod


def add_solidify(obj, thickness=0.05, offset=-1.0):
    mod = obj.modifiers.new("Solidify", "SOLIDIFY")
    mod.thickness = thickness
    mod.offset = offset
    return mod


def add_array(obj, count=4, relative_offset=(1.0, 0.0, 0.0)):
    mod = obj.modifiers.new("Array", "ARRAY")
    mod.count = count
    mod.use_relative_offset = True
    mod.relative_offset_displace = relative_offset
    return mod


def boolean(obj, cutter, operation="DIFFERENCE", solver="EXACT"):
    """Non-destructive boolean. Cutter is renamed CUT_*, shown as wire and hidden from render.
    Solvers in 5.x: 'EXACT' (robust, coplanar-safe), 'MANIFOLD' (fastest, closed meshes only), 'FLOAT'."""
    if not cutter.name.startswith("CUT_"):
        cutter.name = "CUT_" + cutter.name
    cutter.display_type = "WIRE"
    cutter.hide_render = True
    # The faces a cutter creates take the CUTTER's material; give it the target's material so the
    # result has no empty material slot.
    if not cutter.data.materials and obj.active_material is not None:
        cutter.data.materials.append(obj.active_material)
    mod = obj.modifiers.new("Bool_" + cutter.name[4:], "BOOLEAN")
    mod.object = cutter
    mod.operation = operation
    mod.solver = solver
    return mod


def apply_modifiers(obj):
    """Bake the evaluated modifier stack into the mesh, context-free (keeps custom normals)."""
    depsgraph = bpy.context.evaluated_depsgraph_get()
    ev = obj.evaluated_get(depsgraph)
    new_mesh = bpy.data.meshes.new_from_object(ev, preserve_all_data_layers=True, depsgraph=depsgraph)
    old = obj.data
    obj.modifiers.clear()
    obj.data = new_mesh
    if old.users == 0:
        bpy.data.meshes.remove(old)
    new_mesh.name = obj.name
    return obj


def apply_transform(obj, location=False, rotation=True, scale=True):
    """Bake rotation/scale (and optionally location) into mesh data, context-free."""
    loc, rot, sca = obj.matrix_basis.decompose()
    m = Matrix.Identity(4)
    if location:
        m = Matrix.Translation(loc) @ m
    if rotation:
        m = m @ rot.to_matrix().to_4x4()
    if scale:
        m = m @ Matrix.Diagonal((*sca, 1.0))
    obj.data.transform(m)
    for child in obj.children:
        child.matrix_parent_inverse = m @ child.matrix_parent_inverse
    if location:
        obj.location = (0.0, 0.0, 0.0)
    if rotation:
        obj.rotation_mode = "XYZ"
        obj.rotation_euler = (0.0, 0.0, 0.0)
    if scale:
        obj.scale = (1.0, 1.0, 1.0)
    return obj


def set_origin(obj, where="BOTTOM_CENTER"):
    """Move the origin without moving the geometry in world space.
    where: 'BOTTOM_CENTER' (props/kit pieces standing on the floor), 'CENTER', 'MIN_CORNER'
    (grid-snapped modular pieces) or an (x, y, z) point in object space."""
    vs = [v.co for v in obj.data.vertices]
    lo = Vector((min(v.x for v in vs), min(v.y for v in vs), min(v.z for v in vs)))
    hi = Vector((max(v.x for v in vs), max(v.y for v in vs), max(v.z for v in vs)))
    if isinstance(where, str):
        c = (lo + hi) * 0.5
        target = {"BOTTOM_CENTER": Vector((c.x, c.y, lo.z)), "CENTER": c, "MIN_CORNER": lo}[where.upper()]
    else:
        target = Vector(where)
    obj.data.transform(Matrix.Translation(-target))
    obj.matrix_world = obj.matrix_world @ Matrix.Translation(target)
    return [round(v, 5) for v in target]


def join(objects, name=None, apply_mods=True):
    """Join meshes into the first object (materials and UVs merged). Applies modifiers first by default."""
    objects = [o for o in objects if o.type == "MESH"]
    if apply_mods:
        for o in objects:
            if o.modifiers:
                apply_modifiers(o)
    target = objects[0]
    with bpy.context.temp_override(active_object=target, object=target, selected_objects=objects,
                                   selected_editable_objects=objects):
        bpy.ops.object.join()
    try:  # drop empty/unused slots (e.g. from material-less boolean cutters)
        with bpy.context.temp_override(active_object=target, object=target):
            bpy.ops.object.material_slot_remove_unused()
    except Exception:
        pass
    if name:
        target.name = name
        target.data.name = name
    return target


# ============================================================================ materials

def hex_color(value):
    """'#FF2BD6' (sRGB, as in the art-direction palette) -> linear RGB tuple for Blender inputs."""
    value = value.lstrip("#")
    out = []
    for i in (0, 2, 4):
        c = int(value[i:i + 2], 16) / 255.0
        out.append(c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4)
    return tuple(out)


def _mat(name):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    try:
        mat.use_nodes = True
    except Exception:
        pass
    return mat, mat.node_tree.nodes.get("Principled BSDF")


def paint(name, color, metallic=0.35, roughness=0.38, coat=0.0):
    """Saturated painted panel (squad colours). Metallic ~0.3 keeps colour vivid in Unreal."""
    mat, bsdf = _mat(name)
    bsdf.inputs["Base Color"].default_value = (*color, 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Coat Weight"].default_value = coat
    mat.diffuse_color = (*color, 1.0)
    return mat


def glow(name, color, strength=4.0):
    """Emissive trim/engine glow. Base colour dark so the emission carries the colour."""
    mat, bsdf = _mat(name)
    bsdf.inputs["Base Color"].default_value = (0.02, 0.02, 0.02, 1.0)
    bsdf.inputs["Emission Color"].default_value = (*color, 1.0)
    bsdf.inputs["Emission Strength"].default_value = strength
    mat.diffuse_color = (*color, 1.0)
    return mat


def glass(name, color=(0.02, 0.12, 0.16), roughness=0.05):
    """Opaque stylised canopy glass (dark, glossy). Real transparency is costly in Unreal."""
    mat, bsdf = _mat(name)
    bsdf.inputs["Base Color"].default_value = (*color, 1.0)
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Coat Weight"].default_value = 1.0
    mat.diffuse_color = (*color, 1.0)
    return mat


def assign_material(obj, mat):
    """Append (if needed) and return the slot index of `mat` on obj."""
    for i, slot in enumerate(obj.material_slots):
        if slot.material == mat:
            return i
    obj.data.materials.append(mat)
    return len(obj.material_slots) - 1


# ============================================================================ detail passes

def _face_filter(faces, normal, min_dot):
    if normal is None:
        return list(faces)
    n = Vector(normal).normalized()
    return [f for f in faces if f.normal.dot(n) >= min_dot]


def inset_panels(obj, normal=(1.0, 0.0, 0.0), min_dot=0.6, fraction=0.5, thickness=0.03, depth=-0.012,
                 material=None, seed=0, min_area=None):
    """Inset (and recess) a random subset of faces facing `normal` - vents/panels/glow strips.
    Run on the base mesh BEFORE bevel. material: e.g. glow(...) to light up the recessed faces.
    Faces smaller than min_area (default: 16 x thickness^2) are skipped - insetting tiny faces
    (e.g. a nose cone's tip triangles) collapses them into duplicate vertices."""
    rng = random.Random(seed)
    if min_area is None:
        min_area = 16.0 * thickness * thickness
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    faces = [f for f in _face_filter(bm.faces, normal, min_dot) if f.calc_area() >= min_area]
    faces = [f for f in faces if rng.random() < fraction]
    if faces:
        bmesh.ops.inset_individual(bm, faces=faces, thickness=thickness, depth=depth, use_even_offset=True)
        if material is not None:
            idx = assign_material(obj, material)
            for f in faces:
                f.material_index = idx
    bm.to_mesh(obj.data)
    bm.free()
    return len(faces)


def plates(target, normal=(0.0, 0.0, 1.0), min_dot=0.6, region=None, thickness=0.025, gap=0.012,
           lift=0.004, bevel=0.004, material=None, name=None, collection=None):
    """Layered armour plates: take the target's (evaluated) faces that face `normal` inside `region`,
    shrink each connected patch by `gap` (the visible panel seam), lift it off the surface and give it
    `thickness`. Returns a separate object parented to the target. Layering 2-3 plate passes with
    different regions/normals is what makes a hull read as built rather than primitive."""
    depsgraph = bpy.context.evaluated_depsgraph_get()
    ev = target.evaluated_get(depsgraph)
    mesh = ev.to_mesh()
    bm = bmesh.new()
    bm.from_mesh(mesh)
    ev.to_mesh_clear()
    n = Vector(normal).normalized() if normal is not None else None

    def keep(f):
        if n is not None and f.normal.dot(n) < min_dot:
            return False
        if region is None:
            return True
        c = f.calc_center_median()
        (x0, y0, z0), (x1, y1, z1) = region
        return x0 <= c.x <= x1 and y0 <= c.y <= y1 and z0 <= c.z <= z1

    lo = Vector((min(v.co.x for v in bm.verts), min(v.co.y for v in bm.verts), min(v.co.z for v in bm.verts)))
    hi = Vector((max(v.co.x for v in bm.verts), max(v.co.y for v in bm.verts), max(v.co.z for v in bm.verts)))

    def thin(f):  # groove walls / slivers: insetting across them folds the patch into spikes
        return min(e.calc_length() for e in f.edges) < 2.0 * gap

    drop = [f for f in bm.faces if not keep(f) or thin(f)]
    bmesh.ops.delete(bm, geom=drop, context="FACES")
    if not bm.faces:
        bm.free()
        return None
    # Shrink each patch inward to open a seam, then discard the seam ring. Uniform (not "even")
    # offset, and a bounds check: if the inset ever explodes, keep the un-inset patch instead.
    trial = bm.copy()
    ret = bmesh.ops.inset_region(trial, faces=list(trial.faces), thickness=gap, depth=0.0,
                                 use_even_offset=False, use_boundary=True)
    bmesh.ops.delete(trial, geom=ret["faces"], context="FACES")
    loose = [v for v in trial.verts if not v.link_faces]
    bmesh.ops.delete(trial, geom=loose, context="VERTS")
    pad = gap + lift + 1e-3
    inside = all(lo.x - pad <= v.co.x <= hi.x + pad and lo.y - pad <= v.co.y <= hi.y + pad
                 and lo.z - pad <= v.co.z <= hi.z + pad for v in trial.verts)
    if inside and trial.faces:
        bm.free()
        bm = trial
    else:
        trial.free()
    bm.normal_update()
    for v in bm.verts:
        v.co += v.normal * lift
    _uv_layer(bm)
    pname = name or (target.name + "_Plates")
    obj = _finish(pname, bm, collection or target.users_collection[0].name)
    obj.parent = target
    obj.matrix_parent_inverse = Matrix.Identity(4)
    sol = obj.modifiers.new("Solidify", "SOLIDIFY")
    sol.thickness = thickness
    sol.offset = 1.0
    sol.use_even_offset = True
    if bevel:
        add_bevel(obj, width=bevel, segments=2, angle_deg=40)
        add_weighted_normal(obj)
    mat = material or (target.material_slots[0].material if target.material_slots else None)
    if mat is not None:
        obj.data.materials.append(mat)
    return obj


def greebles(target, count=40, seed=0, size=(0.06, 0.22), height=(0.02, 0.09), normal=(0.0, 0.0, 1.0),
             min_dot=0.75, name=None, material=None, collection=None, region=None):
    """Scatter small boxes on target faces that face `normal` (area-weighted). Returns a separate
    object parented to the target, so it can be toggled, tuned or joined later.
    region=((xmin, ymin, zmin), (xmax, ymax, zmax)) in the target's object space confines them to a
    zone (e.g. the rear deck) - clustered greebles read as designed detail, scattered ones as debris."""
    rng = random.Random(seed)
    depsgraph = bpy.context.evaluated_depsgraph_get()
    ev = target.evaluated_get(depsgraph)
    mesh = ev.to_mesh()

    def inside(c):
        if region is None:
            return True
        (x0, y0, z0), (x1, y1, z1) = region
        return x0 <= c.x <= x1 and y0 <= c.y <= y1 and z0 <= c.z <= z1

    faces = [(p.center.copy(), p.normal.copy(), p.area) for p in mesh.polygons
             if (normal is None or p.normal.dot(Vector(normal).normalized()) >= min_dot) and inside(p.center)]
    ev.to_mesh_clear()
    if not faces:
        return None
    total = sum(a for _c, _n, a in faces)
    bm = bmesh.new()
    _uv_layer(bm)
    for _ in range(count):
        r = rng.uniform(0.0, total)
        center, nrm, area = faces[-1]
        for center, nrm, area in faces:
            r -= area
            if r <= 0:
                break
        sx, sy, sz = rng.uniform(*size), rng.uniform(*size), rng.uniform(*height)
        rot = nrm.to_track_quat("Z", "Y").to_matrix().to_4x4()
        spin = Matrix.Rotation(rng.choice([0.0, math.pi / 2]), 4, "Z")
        jitter = Vector((rng.uniform(-0.3, 0.3), rng.uniform(-0.3, 0.3), 0.0)) * math.sqrt(area)
        m = Matrix.Translation(center) @ rot @ spin @ Matrix.Translation(jitter + Vector((0, 0, sz * 0.5))) \
            @ Matrix.Diagonal((sx, sy, sz, 1.0))
        bmesh.ops.create_cube(bm, size=1.0, matrix=m, calc_uvs=True)
    gname = name or (target.name + "_Greebles")
    obj = _finish(gname, bm, collection or target.users_collection[0].name)
    obj.parent = target
    obj.matrix_parent_inverse = Matrix.Identity(4)
    if material is not None:
        obj.data.materials.append(material)
    elif target.material_slots and target.material_slots[0].material:
        obj.data.materials.append(target.material_slots[0].material)
    return obj

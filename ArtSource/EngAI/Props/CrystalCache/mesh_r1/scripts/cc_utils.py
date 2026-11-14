
from __future__ import annotations
import math
import bpy
import bmesh
from mathutils import Vector
from cc_paths import *

def log(msg):
    print(msg, flush=True)

def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    sc = bpy.context.scene
    sc.unit_settings.system = "METRIC"
    sc.unit_settings.scale_length = 1.0
    sc.unit_settings.length_unit = "METERS"
    sc.render.engine = "BLENDER_EEVEE_NEXT"
    sc.render.resolution_x = 1280
    sc.render.resolution_y = 960
    sc.render.image_settings.file_format = "PNG"
    sc.render.image_settings.color_mode = "RGB"
    sc.render.film_transparent = False
    sc.view_settings.view_transform = "Standard"
    sc.view_settings.look = "None"
    sc.view_settings.exposure = 0.10
    sc.view_settings.gamma = 1.0
    return sc

def select_only(*objs):
    bpy.ops.object.select_all(action="DESELECT")
    active = None
    for o in objs:
        if o is None:
            continue
        o.select_set(True)
        o.hide_set(False)
        o.hide_viewport = False
        active = o
    if active is not None:
        bpy.context.view_layer.objects.active = active

def object_mode():
    if bpy.context.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")

def world_bbox(obj):
    mw = obj.matrix_world
    xs, ys, zs = [], [], []
    dg = bpy.context.evaluated_depsgraph_get()
    ev = obj.evaluated_get(dg)
    me = ev.to_mesh()
    try:
        for v in me.vertices:
            p = mw @ v.co
            xs.append(p.x); ys.append(p.y); zs.append(p.z)
    finally:
        ev.to_mesh_clear()
    mins = Vector((min(xs), min(ys), min(zs)))
    maxs = Vector((max(xs), max(ys), max(zs)))
    return mins, maxs, maxs - mins

def apply_world_transform(obj):
    object_mode()
    select_only(obj)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

def new_collection(name):
    col = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(col)
    return col

def link_obj(obj, col):
    for c in list(obj.users_collection):
        c.objects.unlink(obj)
    col.objects.link(obj)

def mesh_report_dict(obj, label=""):
    me = obj.data
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=5e-5)
    bm.faces.ensure_lookup_table()
    tris = sum(len(f.verts) - 2 for f in bm.faces)
    nonman = sum(1 for e in bm.edges if not e.is_manifold)
    degen = sum(1 for f in bm.faces if f.calc_area() < 1e-10)
    seen, islands = set(), 0
    for v in bm.verts:
        if v.index in seen:
            continue
        islands += 1
        stack = [v]
        seen.add(v.index)
        while stack:
            c = stack.pop()
            for e in c.link_edges:
                w = e.other_vert(c)
                if w.index not in seen:
                    seen.add(w.index)
                    stack.append(w)
    flat = wob = hard = tot = 0
    for e in bm.edges:
        if len(e.link_faces) == 2:
            tot += 1
            a = math.degrees(e.calc_face_angle(0))
            if a < 1.0:
                flat += 1
            elif a < 25.0:
                wob += 1
            else:
                hard += 1
    ngons = sum(1 for f in bm.faces if len(f.verts) > 4)
    bm.free()
    mins, maxs, size = world_bbox(obj)
    rec = {
        "label": label or obj.name,
        "name": obj.name,
        "tris": tris,
        "verts": len(me.vertices),
        "faces": len(me.polygons),
        "ngons": ngons,
        "islands": islands,
        "nonmanifold": nonman,
        "degenerate": degen,
        "flat_pct": 100.0 * flat / tot if tot else 0,
        "wobble_pct": 100.0 * wob / tot if tot else 0,
        "hard_pct": 100.0 * hard / tot if tot else 0,
        "bbox_min": [round(c, 4) for c in mins],
        "bbox_max": [round(c, 4) for c in maxs],
        "size_m": [round(c, 4) for c in size],
        "size_cm": [round(c * 100.0, 1) for c in size],
    }
    log("  REPORT {label}: tris={tris} islands={islands} nonman={nonmanifold} degen={degenerate} "
        "flat={flat_pct:.1f}% wobble={wobble_pct:.1f}% hard={hard_pct:.1f}% size_cm={size_cm}".format(**rec))
    return rec

def metrics_on_evaluated(obj, label):
    dg = bpy.context.evaluated_depsgraph_get()
    ev = obj.evaluated_get(dg)
    me = bpy.data.meshes.new_from_object(ev)
    tmp = bpy.data.objects.new("__metrics__", me)
    bpy.context.scene.collection.objects.link(tmp)
    rec = mesh_report_dict(tmp, label)
    bpy.data.objects.remove(tmp, do_unlink=True)
    bpy.data.meshes.remove(me)
    return rec

def duplicate_obj(obj, name):
    me = obj.data.copy()
    dup = obj.copy()
    dup.data = me
    dup.name = name
    dup.data.name = name
    bpy.context.scene.collection.objects.link(dup)
    return dup

def smart_uv(obj, angle=66.0, island_margin=0.003):
    object_mode()
    select_only(obj)
    while len(obj.data.uv_layers) > 1:
        obj.data.uv_layers.remove(obj.data.uv_layers[-1])
    if not obj.data.uv_layers:
        obj.data.uv_layers.new(name="UVMap")
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(angle), island_margin=island_margin,
                             correct_aspect=True, scale_to_bounds=False)
    bpy.ops.object.mode_set(mode="OBJECT")

def hide_for_render(*objs, hide=True):
    for o in objs:
        if o is None:
            continue
        o.hide_render = hide
        o.hide_viewport = hide

def parent_keep_world(child, parent):
    mw = child.matrix_world.copy()
    child.parent = parent
    child.matrix_parent_inverse = parent.matrix_world.inverted()
    child.matrix_world = mw

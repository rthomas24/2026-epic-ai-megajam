
from __future__ import annotations
import math
import bpy
from mathutils import Vector
from cc_paths import *
from cc_utils import *

def import_fbx(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=str(path))
    new = [o for o in bpy.data.objects if o not in before and o.type == "MESH"]
    if not new:
        new = [o for o in bpy.data.objects if o.type == "MESH"]
    return new[0]

def scale_orient_cradle(obj):
    apply_world_transform(obj)
    mins, maxs, size = world_bbox(obj)
    obj.scale = (CRADLE_WID / size.x, CRADLE_LEN / size.y, CRADLE_HGT / size.z)
    apply_world_transform(obj)
    mins, maxs, size = world_bbox(obj)
    obj.location.x -= (mins.x + maxs.x) * 0.5
    obj.location.y -= (mins.y + maxs.y) * 0.5
    obj.location.z -= mins.z
    apply_world_transform(obj)
    return world_bbox(obj)

def measure_floor_z(obj):
    dg = bpy.context.evaluated_depsgraph_get()
    ev = obj.evaluated_get(dg)
    zs = []
    for y in [i * 0.08 for i in range(-10, 11)]:
        origin = Vector((0.0, y, CRADLE_HGT * 0.7))
        inv = ev.matrix_world.inverted()
        res, loc, nrm, idx = ev.ray_cast(inv @ origin, Vector((0, 0, -1)))
        if res:
            w = ev.matrix_world @ loc
            if 0.05 < w.z < 0.8:
                zs.append(w.z)
    if not zs:
        return 0.30
    zs.sort()
    mid = zs[len(zs) // 2]
    log(f"  cradle interior floor z approx {mid:.4f} m  (n={len(zs)})")
    return mid

def polish_cradle(obj, dissolve_deg=DISSOLVE_DEG, sharp_deg=SHARP_DEG,
                  bevel_w=BEVEL_WIDTH, bevel_ang=BEVEL_ANGLE):
    object_mode()
    select_only(obj)
    recs = [mesh_report_dict(obj, "cradle raw (scaled)")]
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.remove_doubles(threshold=0.0002)
    bpy.ops.mesh.delete_loose(use_verts=True, use_edges=True, use_faces=False)
    bpy.ops.mesh.dissolve_degenerate(threshold=0.0001)
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    recs.append(mesh_report_dict(obj, "after weld+clean"))
    while obj.data.uv_layers:
        obj.data.uv_layers.remove(obj.data.uv_layers[0])
    try:
        bpy.ops.mesh.customdata_custom_splitnormals_clear()
    except Exception as e:
        log(f"  (split-normal clear: {e})")
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.dissolve_limited(angle_limit=math.radians(dissolve_deg),
                                  use_dissolve_boundaries=False, delimit={"MATERIAL"})
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.object.mode_set(mode="OBJECT")
    recs.append(mesh_report_dict(obj, f"after planar dissolve {dissolve_deg} deg"))
    smart_uv(obj, angle=66.0, island_margin=0.002)
    try:
        obj.data.set_sharp_from_angle(angle=math.radians(sharp_deg))
    except Exception:
        pass
    select_only(obj)
    try:
        bpy.ops.object.shade_smooth_by_angle(angle=math.radians(sharp_deg), keep_sharp_edges=True)
    except TypeError:
        bpy.ops.object.shade_smooth()
    for m in list(obj.modifiers):
        obj.modifiers.remove(m)
    bv = obj.modifiers.new("Bevel", "BEVEL")
    bv.width = bevel_w
    bv.segments = BEVEL_SEGS
    bv.limit_method = "ANGLE"
    bv.angle_limit = math.radians(bevel_ang)
    bv.harden_normals = True
    bv.profile = 0.50
    bv.miter_outer = "MITER_ARC"
    bv.use_clamp_overlap = True
    wn = obj.modifiers.new("WeightedNormal", "WEIGHTED_NORMAL")
    wn.keep_sharp = True
    wn.weight = 50
    wn.mode = "FACE_AREA"
    recs.append(metrics_on_evaluated(obj, f"after bevel {bevel_w*1000:.0f}mm + WN (evaluated)"))
    return recs

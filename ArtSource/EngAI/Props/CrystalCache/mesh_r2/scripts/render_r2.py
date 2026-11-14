"""Render the #53 crystal cluster r2 contact-sheet panels and measure the seat clearance (Blender 4.5.3).

    blender -b --python render_r2.py
Scene: the merged r1 cradle (mesh_r1/SM_CrystalCacheCradle.blend, live modifiers), r2 cluster appended from
mesh_r2/SM_CrystalCacheCrystal.blend on SOCKET_Crystal, r1 cluster appended for the before/after panels,
a measured 180 cm pilot, neutral studio HDRI + soft key/fill/rim, Standard view transform.
"""
import json, math, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import bpy, bmesh
from mathutils import Vector, Matrix
from mathutils.bvhtree import BVHTree
from cc_paths import *
from cc_utils import log, world_bbox, hide_for_render
from cc_scene import setup_hdri, add_studio_lights, render_view, make_pilot, make_clay_material
from vis_helpers import feature_wire, ucx_edge_overlay, rotate_hdri

NAME = "SM_CrystalCacheCrystal"
bpy.ops.wm.open_mainfile(filepath=str(R1 / "SM_CrystalCacheCradle.blend"))
O = bpy.data.objects
cradle = O["SM_CrystalCacheCradle"]
for n in ("SRC_CrystalCacheCradle", "UCX_SM_CrystalCacheCradle_00"):
    O[n].hide_render = True; O[n].hide_viewport = True
sock = O["SOCKET_Crystal"]

def append_all(path, rename_prefix=None):
    with bpy.data.libraries.load(str(path), link=False) as (src, dst):
        dst.objects = [n for n in src.objects]
    out = {}
    for ob in dst.objects:
        if ob is None:
            continue
        bpy.context.scene.collection.objects.link(ob)
        if rename_prefix:
            ob.name = rename_prefix + ob.name
        out[ob.name] = ob
    return out

r2objs = append_all(OUT / f"{NAME}.blend")
r2 = r2objs[NAME]
r2_ucx = [o for n, o in r2objs.items() if n.startswith("UCX_")]
for n, o in r2objs.items():
    if n.startswith(("SRC_", "UCX_")):
        o.hide_render = True; o.hide_viewport = True
r1objs = append_all(R1 / f"{NAME}.blend", rename_prefix="R1_")
r1 = next(o for n, o in r1objs.items() if n.startswith("R1_SM_"))
for n, o in r1objs.items():
    if not n.startswith("R1_SM_"):
        o.hide_render = True; o.hide_viewport = True
r2.location = sock.matrix_world.translation.copy()
r1.location = sock.matrix_world.translation.copy()
bpy.context.view_layer.update()

# ---- clearance in the cage (evaluated meshes, live modifiers)
dg = bpy.context.evaluated_depsgraph_get()
bvh_c = BVHTree.FromObject(cradle, dg)
ev = r2.evaluated_get(dg); me = ev.to_mesh()
bm = bmesh.new(); bm.from_mesh(me); bm.transform(r2.matrix_world); ev.to_mesh_clear()
fz = sock.matrix_world.translation.z
clear = (9.0, None); top = max(v.co.z for v in bm.verts)
for v in bm.verts:
    if v.co.z > fz + 0.05:
        hit = bvh_c.find_nearest(v.co)
        if hit[0] is not None and hit[3] < clear[0]:
            clear = (hit[3], tuple(round(c, 3) for c in v.co))
clear20 = (9.0, None)
for v in bm.verts:
    if v.co.z > fz + 0.20:
        hit = bvh_c.find_nearest(v.co)
        if hit[0] is not None and hit[3] < clear20[0]:
            clear20 = (hit[3], tuple(round(c, 3) for c in v.co))
up = bm.copy()
bmesh.ops.delete(up, geom=[f for f in up.faces if min(v.co.z for v in f.verts) < fz + 0.03], context="FACES")
overlaps = len(bvh_c.overlap(BVHTree.FromBMesh(up)))
# vertical room above the cluster's own top (ray straight up from the tallest tip)
tipv = max(bm.verts, key=lambda v: v.co.z).co.copy()
upray = bvh_c.ray_cast(tipv + Vector((0, 0, 0.001)), Vector((0, 0, 1)), 5.0)
mins = Vector((min(v.co.x for v in bm.verts), min(v.co.y for v in bm.verts), min(v.co.z for v in bm.verts)))
maxs = Vector((max(v.co.x for v in bm.verts), max(v.co.y for v in bm.verts), top))
bm.free(); up.free()
fit = {"socket_z_cm": round(fz * 100, 2), "top_assembled_cm": round(top * 100, 1),
       "cage_top_cm": 180.0, "rail_underside_cm": round(CAGE_RAIL_Z * 100, 1),
       "room_under_rails_cm": round((CAGE_RAIL_Z - top) * 100, 1),
       "tip_xy_cm": [round(tipv.x * 100, 1), round(tipv.y * 100, 1)],
       "open_above_tip": upray[0] is None,
       "min_clearance_to_cradle_cm": round(clear[0] * 100, 1), "min_clearance_at_m": clear[1],
       "face_overlaps_above_floor": overlaps,
       "min_clearance_crystals_above_20cm_cm": round(clear20[0] * 100, 1), "min_clearance_crystals_at_m": clear20[1],
       "assembled_bbox_cm": [[round(c * 100, 1) for c in mins], [round(c * 100, 1) for c in maxs]]}
log("FIT " + json.dumps(fit))
json.dump(fit, open(WORK / "fit.json", "w"), indent=1)

# ---- staging
setup_hdri(1.0); rotate_hdri(125.0); add_studio_lights()
bpy.ops.mesh.primitive_plane_add(size=14.0, location=(0, 0, -0.001))
ground = bpy.context.object; ground.name = "GROUND"
gm = bpy.data.materials.new("M_Ground"); gm.use_nodes = True
gb = gm.node_tree.nodes["Principled BSDF"]; gb.inputs["Base Color"].default_value = (0.20, 0.20, 0.21, 1); gb.inputs["Roughness"].default_value = 0.8
ground.data.materials.append(gm)
pilot = make_pilot(); pilot.location = (1.35, -2.25, 0.0)
R = RENDERS
def show(*objs):
    for o in (cradle, r1, r2, pilot):
        hide_for_render(o, hide=o not in objs)

# 02 r2 seated + pilot ; 03 r1 seated, same camera
cam_asm = dict(loc=Vector((4.2, -5.6, 2.35)), target=Vector((0.0, 0.15, 0.80)), lens=45)
show(cradle, r2, pilot); render_view(R / "r2_seated.png", **cam_asm)
show(cradle, r1, pilot); render_view(R / "r1_seated.png", **cam_asm)
# looking down into the open top, so the density reads
cam_top = dict(loc=Vector((2.6, -2.9, 3.3)), target=Vector((0.0, 0.0, 0.95)), lens=40)
show(cradle, r2); render_view(R / "r2_seated_top.png", **cam_top)
show(cradle, r1); render_view(R / "r1_seated_top.png", **cam_top)
# 04 side by side, alone, front view from +X: r1 left (-Y), r2 right (+Y), pilot far left
r1.location = Vector((0, -0.95, 0)); r2.location = Vector((0, 0.95, 0)); pilot.location = Vector((0, -2.35, 0))
show(r1, r2, pilot); render_view(R / "r1_r2_pilot.png", loc=Vector((7.0, 0.0, 0.95)), target=Vector((0, 0.0, 0.95)), ortho=5.2, res=(1600, 960))
# 05 r2 hero alone 3/4 ; 06 r2 alone front ortho (reference framing)
r2.location = Vector((0, 0, 0))
show(r2); render_view(R / "r2_hero.png", loc=Vector((2.35, -2.25, 1.35)), target=Vector((0, 0, 0.62)), lens=50, res=(1280, 1280))
render_view(R / "r2_front.png", loc=Vector((6.0, 0, 0.66)), target=Vector((0, 0, 0.66)), ortho=1.62, res=(1280, 1280))
r1.location = Vector((0, 0, 0)); show(r1)
render_view(R / "r1_front.png", loc=Vector((6.0, 0, 0.66)), target=Vector((0, 0, 0.66)), ortho=1.62, res=(1280, 1280))
# 07 end ortho seated + pilot, 08 front ortho seated + pilot
r2.location = sock.matrix_world.translation.copy(); r1.location = r2.location.copy()
pilot.location = Vector((1.35, -2.25, 0.0))
show(cradle, r2, pilot); render_view(R / "r2_end_ortho.png", loc=Vector((0.0, -6.0, 0.95)), target=Vector((0, 0, 0.92)), ortho=2.9, res=(1280, 1100))
pilot.location = Vector((0.0, -2.35, 0.0))
render_view(R / "r2_front_ortho.png", loc=Vector((6.0, 0.0, 0.95)), target=Vector((0, -0.35, 0.92)), ortho=5.0, res=(1600, 960))
# 09 close-up of the base: amber crystals, glow, facets, bevel highlights
r2.location = Vector((0, 0, 0)); show(r2)
render_view(R / "r2_closeup.png", loc=Vector((0.95, -0.80, 0.55)), target=Vector((0.0, -0.05, 0.30)), lens=50, res=(1280, 1100))
# 10 clay + hard edges ; 11 UCX hulls
clay = make_clay_material()
mats = list(r2.data.materials); r2.data.materials.clear(); r2.data.materials.append(clay)
w = feature_wire(r2, "W_r2", angle=20.0, radius=0.0018)
render_view(R / "r2_clay_wire.png", loc=Vector((2.35, -2.25, 1.35)), target=Vector((0, 0, 0.62)), lens=50, res=(1280, 1280))
hide_for_render(w, hide=True)
for u in r2_ucx:
    u.hide_viewport = False
bpy.context.view_layer.update()
vis = ucx_edge_overlay(r2_ucx)
render_view(R / "r2_ucx.png", loc=Vector((2.35, -2.25, 1.35)), target=Vector((0, 0, 0.62)), lens=50, res=(1280, 1280))
log("RENDER DONE")

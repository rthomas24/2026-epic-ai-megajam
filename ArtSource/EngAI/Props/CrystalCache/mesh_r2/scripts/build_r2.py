"""#53 hero crystal cluster r2: build -> bake -> 2K maps -> editable .blend -> FBX/GLB (Blender 4.5.3 LTS).

    blender -b --python build_r2.py
"""
import json, math, os, runpy, subprocess, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import bpy, bmesh
from mathutils import Matrix, Vector
from cc_paths import *
from cc_utils import log, world_bbox, mesh_report_dict, metrics_on_evaluated, smart_uv, select_only, parent_keep_world
from cc_bake import ensure_bake_image, bake_self, save_image, make_bevel_normal_material, make_emit_from_attribute, assign_pbr_material
import cluster_r2

HS = runpy.run_path(str(HARDSURFACE_PY))
NAME = "SM_CrystalCacheCrystal"
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.unit_settings.system = "METRIC"; sc.unit_settings.scale_length = 1.0
world = bpy.data.worlds.new("BakeWorld"); sc.world = world
world.light_settings.distance = 0.12   # AO distance for the bake

obj, n_cyan, n_amber, n_rocks = cluster_r2.build_cluster(NAME)
raw = mesh_report_dict(obj, "SRC (pre-bevel)")
mins, maxs, size = world_bbox(obj)
assert maxs.z <= MAX_TOP_LOCAL + 1e-4, f"cluster top {maxs.z:.3f} m above the {MAX_TOP_LOCAL} m budget"
smart_uv(obj, angle=46.0, island_margin=0.004)

# ---- bakes on the un-bevelled mesh (UVs carry through the live Bevel)
for layer, fname in (("Col", "_BC_raw"), ("Emi", "_E_raw"), ("RM", "_RM_raw")):
    make_emit_from_attribute(obj, layer)
    img, _ = ensure_bake_image(obj, "b" + layer, TEX_SIZE)
    bake_self(obj, "EMIT", img)
    save_image(img, TEX / f"T_CrystalCacheCrystal{fname}.png")
# coverage mask (margin 0) for the dilation
mat = bpy.data.materials.new("mask"); mat.use_nodes = True
nt = mat.node_tree; [nt.nodes.remove(n) for n in list(nt.nodes)]
em = nt.nodes.new("ShaderNodeEmission"); em.inputs["Color"].default_value = (1, 1, 1, 1)
o = nt.nodes.new("ShaderNodeOutputMaterial"); nt.links.new(em.outputs[0], o.inputs[0])
obj.data.materials.clear(); obj.data.materials.append(mat)
img, _ = ensure_bake_image(obj, "bMask", TEX_SIZE)
select_only(obj); sc.cycles.bake_type = "EMIT"
bpy.ops.object.bake(type="EMIT", use_clear=True, margin=0)
save_image(img, TEX / "T_CrystalCacheCrystal_MASK_raw.png")
make_emit_from_attribute(obj, "Col")
img, _ = ensure_bake_image(obj, "bAO", TEX_SIZE)
bake_self(obj, "AO", img, samples=96)
save_image(img, TEX / "T_CrystalCacheCrystal_AO_raw.png")
make_bevel_normal_material(obj, radius=0.0018, noise_scale=400.0, bump_strength=0.06, bump_dist=0.0005)
img, _ = ensure_bake_image(obj, "bN", TEX_SIZE)
bake_self(obj, "NORMAL", img, samples=16)
save_image(img, TEX / "T_CrystalCacheCrystal_N_ogl.png")
r = subprocess.run(["python3", str(Path(__file__).resolve().parent / "tex_r2.py"), str(TEX), str(OUT)], capture_output=True, text=True)
log(r.stdout + r.stderr)
if r.returncode:
    raise SystemExit("tex_r2 failed")

# ---- material, SRC_, live modifiers
assign_pbr_material(obj, "T_CrystalCacheCrystal", OUT, emission_strength=2.2)
obj.data.materials[0].name = "M_CrystalCacheCrystal"
for m in list(bpy.data.materials):
    if m.users == 0:
        bpy.data.materials.remove(m)
for im in list(bpy.data.images):
    if im.users == 0:
        bpy.data.images.remove(im)
src_col = bpy.data.collections.new("SRC"); sc.collection.children.link(src_col)
deliv = bpy.data.collections.new("Delivery"); sc.collection.children.link(deliv)
colc = bpy.data.collections.new("Collision"); sc.collection.children.link(colc)
src = obj.copy(); src.data = obj.data.copy(); src.name = "SRC_CrystalCacheCrystal"; src.data.name = "SRC_CrystalCacheCrystal"
src_col.objects.link(src); src.hide_set(True); src.hide_viewport = True; src.hide_render = True
sc.collection.objects.unlink(obj); deliv.objects.link(obj)
HS["add_bevel"](obj, width=0.0025, segments=1, angle_deg=52.0, harden=True)
HS["add_weighted_normal"](obj, keep_sharp=True, weight=50, mode="FACE_AREA")

# ---- UCX: one convex hull per cyan shard (above the nest) + one for the nest and amber crystals
me = obj.data
part = me.attributes["part"]
groups = {}
for p in me.polygons:
    groups.setdefault(part.data[p.index].value, set()).update(p.vertices)
hulls = []
for k in sorted(groups):
    pts = [me.vertices[i].co.copy() for i in groups[k]]
    if k:  # trim the root buried in the nest so the shard hull doesn't double the nest hull
        pts = [p for p in pts if p.z > 0.10] or pts
    # drop points lying on a segment between two others (the colour-ramp ring stations are collinear
    # along each shaft edge and would leave zero-area slivers in the hull)
    def on_segment(p, a, b):
        ab = b - a
        if ab.length < 1e-6:
            return False
        t = (p - a).dot(ab) / ab.length_squared
        return 1e-3 < t < 1 - 1e-3 and (a + ab * t - p).length < 2e-4
    pts = [p for i, p in enumerate(pts)
           if not any(on_segment(p, a, b) for j, a in enumerate(pts) for l, b in enumerate(pts) if j < l and i not in (j, l))]
    bm = bmesh.new()
    vs = [bm.verts.new(p) for p in pts]
    bmesh.ops.convex_hull(bm, input=vs)
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context="VERTS")
    bmesh.ops.dissolve_degenerate(bm, dist=1e-4, edges=bm.edges[:])
    hm = bpy.data.meshes.new("UCX"); bm.to_mesh(hm); bm.free()
    h = bpy.data.objects.new("UCX_%s_%02d" % (NAME, len(hulls)), hm)
    colc.objects.link(h); h.display_type = "WIRE"; h.hide_render = True
    parent_keep_world(h, obj)
    hulls.append((h.name, len(hm.vertices)))
log(f"UCX hulls {hulls}")
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / f"{NAME}.blend"), relative_remap=True)
bpy.ops.file.make_paths_relative(); bpy.ops.wm.save_mainfile()
ev = metrics_on_evaluated(obj, "SM (live Bevel+WN)")

# ---- exports
exp = runpy.run_path(str(EXPORT_PY))
res = exp["main"](root=NAME, out_dir=str(OUT), front_axis="+X", collision="auto")
select_only(obj)
bpy.ops.export_scene.gltf(filepath=str(OUT / f"{NAME}.glb"), use_selection=True, export_apply=True, export_format="GLB")
info = {"counts": {"cyan": n_cyan, "amber": n_amber, "rocks": n_rocks}, "raw": raw, "evaluated": ev,
        "ucx": hulls, "export": {k: res.get(k) for k in ("tris", "size_cm", "collision", "warnings", "errors")} if isinstance(res, dict) else str(res)}
json.dump(info, open(WORK / "build_info.json", "w"), indent=1, default=str)
log("BUILD DONE " + json.dumps(info["export"], default=str))

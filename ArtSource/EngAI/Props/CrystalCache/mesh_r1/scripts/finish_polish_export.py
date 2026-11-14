import bpy, math, runpy, json, sys
from pathlib import Path
from mathutils import Vector, Matrix
REPO=Path("/tmp/wt-crystal-cache-r1")
OUT=REPO/"ArtSource/EngAI/Props/CrystalCache/mesh_r1"
HS=runpy.run_path(str(REPO/".claude/skills/blender-game-asset/scripts/hardsurface.py"))
EXP=REPO/".claude/skills/blender-unreal-export/scripts/export_for_unreal.py"
FLOOR_Z=0.3005485534667969
ASM=OUT/"SM_CrystalCache_assembly.blend"
log=lambda *a: print("[finish]",*a, flush=True)
O=bpy.data.objects

def coll(name):
    c=bpy.data.collections.get(name)
    if not c:
        c=bpy.data.collections.new(name); bpy.context.scene.collection.children.link(c)
    return c
def move_to(o,c):
    for u in list(o.users_collection): u.objects.unlink(o)
    c.objects.link(o)
def clean_mat(o, base):
    m=o.data.materials[0]
    if m.name!=base:
        old=bpy.data.materials.get(base)
        if old and old!=m: old.name=base+"_old"
        m.name=base

cr=O["SM_CrystalCacheCradle"]; cy=O["SM_CrystalCacheCrystal"]; sock=O["SOCKET_Crystal"]
deliv=coll("Delivery"); srcc=coll("SRC"); colc=coll("Collision")
# --- socket at the cradle floor (was left at the origin)
sock.matrix_world=Matrix.Translation((0,0,FLOOR_Z))
sock.empty_display_type='ARROWS'; sock.empty_display_size=0.3
# --- crystal: hidden SRC_ ref + live Bevel/WeightedNormal via hardsurface.py
if "SRC_CrystalCacheCrystal" not in O:
    s=cy.copy(); s.data=cy.data.copy(); s.name="SRC_CrystalCacheCrystal"; s.data.name="SRC_CrystalCacheCrystal"
    s.modifiers.clear(); s.parent=None; srcc.objects.link(s)
s=O["SRC_CrystalCacheCrystal"]; s.matrix_world=cy.matrix_world.copy(); s.hide_set(True); s.hide_viewport=True; s.hide_render=True
cy.modifiers.clear()
import os
SEG=int(os.environ.get("SEGS","1"))
HS["add_bevel"](cy, width=0.0025, segments=SEG, angle_deg=30.0, harden=True)
HS["add_weighted_normal"](cy, keep_sharp=True, weight=50, mode="FACE_AREA")
move_to(cy, deliv)
ucxc=O["UCX_SM_CrystalCacheCrystal_00"]; move_to(ucxc, colc); ucxc.display_type='WIRE'
O["UCX_SM_CrystalCacheCradle_00"].display_type='WIRE'
cr.modifiers["Bevel"].segments=SEG
cr.modifiers["Bevel"].harden_normals=True
u=O["UCX_SM_CrystalCacheCradle_00"]; u.data.materials.clear()
# cradle: confirm live mods
log("cradle mods", [(m.type, round(getattr(m,'width',0),4)) for m in cr.modifiers])
log("crystal mods", [(m.type, round(getattr(m,'width',0),4)) for m in cy.modifiers])
for o in (cr, O["SRC_CrystalCacheCradle"]):
    pass
O["SRC_CrystalCacheCradle"].hide_set(True)
clean_mat(cr,"M_CrystalCacheCradle"); clean_mat(cy,"M_CrystalCacheCrystal")
if O.get("BAKE_CrystalCacheCradle"):
    b=O["BAKE_CrystalCacheCradle"]; b.data.materials[0]=cr.data.materials[0]
cr.hide_render=False; cr.hide_viewport=False; cy.hide_render=False
# remove stale helper objects from previous panel runs
for n in [o.name for o in O if o.name.startswith(("W_","VIS_UCX","GIZMO"))]:
    bpy.data.objects.remove(O[n], do_unlink=True)
bpy.ops.outliner.orphans_purge(do_recursive=True)
bpy.ops.wm.save_as_mainfile(filepath=str(ASM), relative_remap=True)
bpy.ops.file.make_paths_relative(); bpy.ops.wm.save_mainfile()
log("saved assembly")

# --- export FBX (+manifest) from the editable meshes (modifiers applied by the exporter)
m=runpy.run_path(str(EXP))
res={}
for root in ("SM_CrystalCacheCradle","SM_CrystalCacheCrystal"):
    r=m["main"](root=root, out_dir=str(OUT), front_axis="+X", collision="auto")
    res[root]={k:r.get(k) for k in ("tris","size_cm","sockets","collision","warnings","errors")} if isinstance(r,dict) else str(r)
    log("export", root, res[root])
# --- GLB (metres, modifiers applied)
for root in ("SM_CrystalCacheCradle","SM_CrystalCacheCrystal"):
    bpy.ops.object.select_all(action='DESELECT')
    o=O[root]; o.hide_set(False); o.select_set(True); bpy.context.view_layer.objects.active=o
    saved=o.matrix_world.copy(); o.matrix_world=Matrix.Identity(4)
    bpy.ops.export_scene.gltf(filepath=str(OUT/(root+".glb")), use_selection=True, export_apply=True, export_format='GLB')
    o.matrix_world=saved
json.dump(res, open("/tmp/cc-finish/export_res.json","w"), indent=1, default=str)

# --- per-asset editable blends
def per_asset(keep, out, zero=None):
    bpy.ops.wm.open_mainfile(filepath=str(ASM))
    for o in list(bpy.data.objects):
        if o.name not in keep: bpy.data.objects.remove(o, do_unlink=True)
    if zero:
        for n in zero: bpy.data.objects[n].location.z=0.0
    for c in list(bpy.data.collections):
        if not c.objects and not c.children: bpy.data.collections.remove(c)
    bpy.ops.outliner.orphans_purge(do_recursive=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(out), relative_remap=True)
    log("saved", out, sorted(o.name for o in bpy.data.objects))
per_asset({"SM_CrystalCacheCradle","SRC_CrystalCacheCradle","UCX_SM_CrystalCacheCradle_00","SOCKET_Crystal"}, OUT/"SM_CrystalCacheCradle.blend")
per_asset({"SM_CrystalCacheCrystal","SRC_CrystalCacheCrystal","UCX_SM_CrystalCacheCrystal_00"}, OUT/"SM_CrystalCacheCrystal.blend", zero=["SM_CrystalCacheCrystal","SRC_CrystalCacheCrystal"])
log("DONE")

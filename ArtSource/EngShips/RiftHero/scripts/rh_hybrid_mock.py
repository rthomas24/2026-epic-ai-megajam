# SPDX-License-Identifier: MIT
# RiftHero: quick MOCK of the hybrid route (not a deliverable): the Rodin hull with its ring and nozzles cut away
# and the clean rebuilt ring, saddle and nozzles from Rift_v03.blend put in their place. Review render only.
#   blender --background --factory-startup --python rh_hybrid_mock.py
import math, os, sys
import bmesh, bpy
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rh_common as C
import rh_geo as G

bpy.ops.wm.read_factory_settings(use_empty=True)
ob = C.import_rodin("SM_RiftHybrid", textured=True)
bm = bmesh.new(); bm.from_mesh(ob.data)
kill = []
for f in bm.faces:
    c = f.calc_center_median()
    r = math.hypot(c.y, c.z - 0.55)
    if (-1.84 <= c.x <= -1.32 and 0.40 <= r <= 0.68 and c.z > 0.13) or c.x < -2.30:
        kill.append(f)
n_kill = len(kill)
bmesh.ops.delete(bm, geom=kill, context="FACES")
bm.to_mesh(ob.data); bm.free()
names = ["SM_Rift_Ring", "SM_Rift_Saddle", "SM_Rift_NozzleC", "SM_Rift_NozzleO"]
with bpy.data.libraries.load(os.path.join(C.ROOT, "Rift_v03.blend")) as (src, dst):
    dst.objects = [n for n in names if n in src.objects]
for o in dst.objects:
    o.parent = None
    bpy.context.scene.collection.objects.link(o)
mats = [bpy.data.materials.get(n) for n in ("M_Rift_Hull", "M_Rift_Accent", "M_Rift_Mech", "M_Rift_Glass", "M_Rift_Glow")]
plate = G.box("SM_Rift_BackPlate", (-2.325, -0.83, -0.69), (-2.27, 0.83, -0.07), mats, 2)
targets = [ob.name, plate.name] + [o.name for o in dst.objects]
rr = C.skill("blender-mcp/scripts/review_renders.py")
res = {"deleted_faces": n_kill}
for mode in ("material", "clay"):
    res[mode] = rr["main"](targets=targets, out_dir=C.REVIEW, label="04_hybrid_mock", mode=mode,
                           views=["back_3q", "left", "front_3q", "top"], resolution=900, columns=2).get("sheet")
C.out(res)

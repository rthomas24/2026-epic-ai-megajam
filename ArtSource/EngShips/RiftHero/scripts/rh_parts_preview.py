# SPDX-License-Identifier: MIT
# RiftHero hybrid: quick look at the new parts on the cut Rodin hull (review renders only, nothing saved).
#   blender --background --factory-startup --python rh_parts_preview.py -- <label>
import os, sys
import bpy
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rh_common as C, rh_geo as G, rh_parts as P, rh_shots as S
label = sys.argv[sys.argv.index("--") + 1]
bpy.ops.wm.read_factory_settings(use_empty=True)
ob = C.import_rodin("SM_Rift", textured=True)
n = P.cut_rodin(ob)
mats, parts, disc = P.build_all()
names = [ob.name] + [p.name for p in parts]
res = {"cut": n, "tris": G.tri_count(parts), "per": {p.name: G.tri_count([p]) for p in parts}}
a = S.render(names, C.REVIEW, label, "material", ["chase_3q", "foot_rear_8m", "rear_close", "chase"])
res["sheet"] = S.grid([[a["chase_3q"], a["foot_rear_8m"]], [a["rear_close"], a["chase"]]], os.path.join(C.REVIEW, label + "_shots.png"))
for p in a.values():
    os.remove(p)
C.out(res)

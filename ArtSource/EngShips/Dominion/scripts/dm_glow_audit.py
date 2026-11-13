# SPDX-License-Identifier: MIT
"""
dm_glow_audit.py - emissive surface audit of the Dominion hull (which glow faces face down / sit under the belly).

    blender --background ArtSource/EngShips/Dominion/Dominion.blend --python dm_glow_audit.py

Prints __RESULT__{...}: per glow material the total area (m2), the area facing down (normal z < -0.3) and the area
below Z -30 m (the belly), grouped into 50 m bins along Unreal X so the source can be located.
"""
import json

import bpy

obj = bpy.data.objects["SM_Dominion_Hull"]
me = obj.data
out = {}
for poly in me.polygons:
    mat = me.materials[poly.material_index]
    if mat is None or "Glow" not in mat.name:
        continue
    d = out.setdefault(mat.name, {"area": 0.0, "down": 0.0, "belly": 0.0, "belly_bins_ue_x": {}})
    a = poly.area
    d["area"] += a
    n = poly.normal
    c = poly.center
    if n.z < -0.3:
        d["down"] += a
    if c.z < -30.0:
        d["belly"] += a
        ue_x = int(-c.y // 50 * 50)
        d["belly_bins_ue_x"][ue_x] = round(d["belly_bins_ue_x"].get(ue_x, 0.0) + a, 1)
for d in out.values():
    for k in ("area", "down", "belly"):
        d[k] = round(d[k], 1)
total = sum(p.area for p in me.polygons)
print("__RESULT__" + json.dumps({"hull_area_m2": round(total, 1), "glow": out}))

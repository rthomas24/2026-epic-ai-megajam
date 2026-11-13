# SPDX-License-Identifier: MIT
# RiftHero: round-trip check of the exported FBX files (re-import into an empty scene and measure).
import os, sys
import bpy
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rh_common as C
out = {}
for name in ("SM_Rift", "SM_Rift_PortalDisc"):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=os.path.join(C.EXPORT, name + ".fbx"))
    r = {"objects": sorted(o.name + ":" + o.type for o in bpy.data.objects)}
    for o in bpy.data.objects:
        if o.type == "MESH" and not o.name.startswith("UCX_"):
            me = o.data
            me.calc_loop_triangles()
            vs = [o.matrix_world @ v.co for v in me.vertices]
            r["mesh"] = {"name": o.name, "tris": len(me.loop_triangles), "slots": [m.name for m in me.materials],
                         "uv_layers": [u.name for u in me.uv_layers],
                         "lo_cm": [round(min(v[i] for v in vs) * 100, 1) for i in range(3)],
                         "hi_cm": [round(max(v[i] for v in vs) * 100, 1) for i in range(3)],
                         "scale": [round(s, 4) for s in o.scale],
                         "tris_per_slot": {me.materials[i].name: sum(1 for t in me.loop_triangles if t.material_index == i)
                                           for i in range(len(me.materials))}}
    r["sockets_blender_cm"] = {o.name: [round(c * 100, 1) for c in o.matrix_world.translation]
                               for o in bpy.data.objects if o.name.startswith("SOCKET_")}
    out[name] = r
C.out(out)

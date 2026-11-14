# Triad Drift - Outpost: look at Grok Bot's source meshes as delivered (read only).
import bpy, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import op_common as C
C.clear_scene()
o = C.import_fbx("SM_VerdanceOutpost")
o.data.materials.clear(); o.data.materials.append(C.preview_material("M_prev_outpost", "T_VerdanceOutpost"))
p = os.path.join(C.KIT, "review", "src")
C.render_views(p, [
    ("front34", (11, -8, 6), (0, 0, 2.6), 35),
    ("bay", (5.5, -0.9, 2.9), (0, -0.9, 2.5), 28),
    ("ramp_side", (3.1, -9, 1.2), (3.1, 0, 1.0), ("ortho", 5.0)),
    ("under", (7, -6, 0.3), (0, 0, 1.2), 30),
    ("back34", (-10, 8, 5), (0, 0, 2.6), 35),
    ("top", (0, 0, 30), (0, 0, 0), ("ortho", 9.5)),
])
print("__RESULT__ ok")

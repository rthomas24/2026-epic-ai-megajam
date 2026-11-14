# SPDX-License-Identifier: MIT
# Triad Drift - import builder: refit the Cinder anchor tower's plinth collision. Grok Bot's UCX_SM_CinderAnchorTower_00
# is an axis-aligned 13.8 m box: the three charge sockets sit inside it and its corners stand up to 2.2 m outside the
# visible octagonal plinth. This opens Grok Bot's .blend (never modified, never saved), replaces that one hull with a
# octagonal prism fitted to the visible plinth (each face as far out as the mesh reaches, less INSET; the three port
# faces stop 10 cm short of their charge socket), and exports the same objects with Grok Bot's
# own export settings (mesh_r1/rebuild.py) to ArtSource/EngImport/CinderAnchor/SM_CinderAnchorTower.fbx.
#   blender --background --factory-startup --python anchor_collision.py
import json
import math
import os
import runpy

import bmesh
import bpy
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
SRC = os.path.join(REPO, "ArtSource", "EngAI", "Props", "CinderFieldAnchor", "mesh_r1")
OUT = os.path.join(REPO, "ArtSource", "EngImport", "CinderAnchor")
INSET = float(os.environ.get("TD_ANCHOR_INSET", "0.05"))     # m, pulled in along the radius
PORT_CLEAR = 0.10                                            # m, each charge socket ends up this far outside the hull

bpy.ops.wm.open_mainfile(filepath=os.path.join(SRC, "SM_CinderAnchorTower.blend"))
root = bpy.data.objects["SM_CinderAnchorTower"]
old = bpy.data.objects["UCX_SM_CinderAnchorTower_00"]
ob = [old.matrix_world @ Vector(c) for c in old.bound_box]
z0, z1 = min(v.z for v in ob), max(v.z for v in ob)
old_half = max(max(abs(v.x), abs(v.y)) for v in ob)
dg = bpy.context.evaluated_depsgraph_get()
me = root.evaluated_get(dg).to_mesh()
verts = [root.matrix_world @ v.co for v in me.vertices if (root.matrix_world @ v.co).z <= z1 + 1e-4]
socks_in = {o.name: o.matrix_world.translation.copy() for o in bpy.data.objects if o.name.startswith("SOCKET_Charge")}
# An octagonal prism: eight faces at 45 degree steps (the plinth's own faces), each as far out as the visible plinth
# reaches in that direction, less INSET; the three port faces stop PORT_CLEAR short of their charge socket (the ports
# are recessed 25-40 cm, a hull on the face would bury the socket). 16 vertices: nothing for the cooker to trip on
# (a 47-vertex fitted hull imported, drew in "show Collision", and blocked nothing).
faces, profile = [], []
for k in range(8):
    a = math.radians(45.0 * k)
    n = Vector((math.cos(a), math.sin(a), 0.0))
    d = max(v.x * n.x + v.y * n.y for v in verts) - INSET
    port = None
    for name, p in socks_in.items():
        if Vector((p.x, p.y, 0.0)).normalized().dot(n) > math.cos(math.radians(22.5)):
            d = min(d, p.x * n.x + p.y * n.y - PORT_CLEAR)
            port = name
    faces.append((n, d))
    profile.append({"face_deg": 45 * k, "apothem_cm": round(d * 100, 1), "port": port})
pts = []
for k in range(8):
    (n1, d1), (n2, d2) = faces[k], faces[(k + 1) % 8]
    det = n1.x * n2.y - n1.y * n2.x
    x, y = (d1 * n2.y - d2 * n1.y) / det, (n1.x * d2 - n2.x * d1) / det
    pts += [Vector((x, y, z0)), Vector((x, y, z1))]
bm = bmesh.new()
for p in pts:
    bm.verts.new(old.matrix_world.inverted() @ p)
bmesh.ops.convex_hull(bm, input=bm.verts)
bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
bm.normal_update()
planes = [(old.matrix_world.to_3x3() @ f.normal, old.matrix_world @ f.calc_center_median()) for f in bm.faces]


def depth_inside(p):
    """> 0: metres inside the hull; < 0: outside by that much (largest plane distance)."""
    return -max((p - c).dot(n) for n, c in planes)


socks = {}
for o in bpy.data.objects:
    if o.name.startswith("SOCKET_Charge"):
        p = o.matrix_world.translation
        socks[o.name] = {"blender_m": [round(p.x, 3), round(p.y, 3), round(p.z, 3)], "r_cm": round(math.hypot(p.x, p.y) * 100, 1),
                         "outside_cm": round(-depth_inside(p) * 100, 1)}
nverts, nfaces = len(bm.verts), len(bm.faces)
new_r = max(math.hypot((old.matrix_world @ v.co).x, (old.matrix_world @ v.co).y) for v in bm.verts)
bm.to_mesh(old.data)
bm.free()
old.data.update()
reb = runpy.run_path(os.path.join(SRC, "rebuild.py"), run_name="td_anchor")
extras = [o for o in bpy.data.objects if o.name.startswith("UCX_") or o.name.startswith("SOCKET_")]
os.makedirs(OUT, exist_ok=True)
reb["export_fbx"]([root] + extras, os.path.join(OUT, "SM_CinderAnchorTower.fbx"))
report = {"old_box_half_cm": round(old_half * 100, 1), "z_cm": [round(z0 * 100, 1), round(z1 * 100, 1)], "inset_cm": INSET * 100,
          "hull_verts": nverts, "hull_faces": nfaces, "hull_max_radius_cm": round(new_r * 100, 1),
          "faces": profile, "charge_sockets": socks,
          "exported": [root.name] + [o.name for o in extras]}
with open(os.path.join(OUT, "collision_report.json"), "w", encoding="utf-8") as fh:
    json.dump(report, fh, indent=1)
print("TDANCHOR " + json.dumps(report))

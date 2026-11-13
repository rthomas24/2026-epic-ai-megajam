# Contact strip for one Unreal-exported animation FBX (mesh + skeleton + one take):
# 5 evenly spaced frames, front row and right-side row, clay (Workbench), floor at z=0, fixed camera.
# Also prints motion metrics (root and pelvis travel, foot heights) as METRIC lines.
# Run: blender --background --factory-startup --python b_strip.py -- <in.fbx> <out.png>
import bpy, sys, os, math, json
import numpy as np
from mathutils import Vector

argv = sys.argv[sys.argv.index("--") + 1:]
SRC, OUT = argv[0], argv[1]
NFR = 6
CELL_W, CELL_H = 230, 322

for o in list(bpy.data.objects):
    bpy.data.objects.remove(o)
bpy.ops.import_scene.fbx(filepath=SRC, use_anim=True, automatic_bone_orientation=False)
scene = bpy.context.scene
arm = next(o for o in scene.objects if o.type == 'ARMATURE')
meshes = [o for o in scene.objects if o.type == 'MESH']
act = arm.animation_data.action if arm.animation_data else None
f0, f1 = (int(round(act.frame_range[0])), int(round(act.frame_range[1]))) if act else (0, 0)
print("INFO action", act.name if act else None, "range", f0, f1, "fps", scene.render.fps, "meshes", [m.name for m in meshes])
pb = arm.pose.bones


def bw(name):
    b = pb.get(name)
    return (arm.matrix_world @ b.head) if b else None


def bwtail(name):
    b = pb.get(name)
    return (arm.matrix_world @ b.tail) if b else None


# metrics over every frame
rows = []
for f in range(f0, f1 + 1):
    scene.frame_set(f)
    r = {"f": f}
    for n in ("root", "pelvis", "foot_l", "foot_r", "ball_l", "ball_r", "hand_r", "hand_l", "head"):
        v = bw(n)
        if v is not None:
            r[n] = [round(v.x, 4), round(v.y, 4), round(v.z, 4)]
    rows.append(r)
scale_hint = (bw("head") - bw("foot_l")).length if bw("head") else 1.0
scene.frame_set(f0)
p0 = Vector(rows[0]["pelvis"]); p1 = Vector(rows[-1]["pelvis"])
root_travel = max((Vector(r["root"]) - Vector(rows[0]["root"])).length for r in rows) if "root" in rows[0] else None
pel_h = max(math.hypot(r["pelvis"][0] - rows[0]["pelvis"][0], r["pelvis"][1] - rows[0]["pelvis"][1]) for r in rows)
print("METRIC " + json.dumps({"src": os.path.basename(SRC), "frames": f1 - f0 + 1, "root_travel_max": root_travel,
      "pelvis_horiz_travel_max": round(pel_h, 4), "pelvis_end_minus_start": [round(v, 4) for v in (p1 - p0)],
      "pelvis_z_min": min(r["pelvis"][2] for r in rows), "pelvis_z_max": max(r["pelvis"][2] for r in rows),
      "foot_l_z_min": min(r["foot_l"][2] for r in rows), "foot_r_z_min": min(r["foot_r"][2] for r in rows),
      "head_z_min": min(r["head"][2] for r in rows), "scale_head_to_foot": round(scale_hint, 4),
      "hand_r_path": round(sum((Vector(b["hand_r"]) - Vector(a["hand_r"])).length for a, b in zip(rows, rows[1:])), 3),
      "hand_l_path": round(sum((Vector(b["hand_l"]) - Vector(a["hand_l"])).length for a, b in zip(rows, rows[1:])), 3),
      "foot_l_horiz_slide_when_low": round(max([0] + [math.hypot(b["foot_l"][0] - a["foot_l"][0], b["foot_l"][1] - a["foot_l"][1]) for a, b in zip(rows, rows[1:]) if a["foot_l"][2] < 0.14 and b["foot_l"][2] < 0.14]), 4),
      "foot_r_horiz_slide_when_low": round(max([0] + [math.hypot(b["foot_r"][0] - a["foot_r"][0], b["foot_r"][1] - a["foot_r"][1]) for a, b in zip(rows, rows[1:]) if a["foot_r"][2] < 0.14 and b["foot_r"][2] < 0.14]), 4)}))

# facing from rest-ish frame: lateral = left minus right shoulder
L = (bw("upperarm_l") - bw("upperarm_r")); L.z = 0; L.normalize()
Z = Vector((0, 0, 1))
F = L.cross(Z); F.normalize()
print("INFO lateral", tuple(round(v, 3) for v in L), "forward", tuple(round(v, 3) for v in F))

frames = [int(round(f0 + (f1 - f0) * i / (NFR - 1))) for i in range(NFR)]
# bounds over chosen frames from evaluated meshes
mn = Vector((1e9, 1e9, 1e9)); mx = Vector((-1e9, -1e9, -1e9))
dg = bpy.context.evaluated_depsgraph_get()
for f in frames:
    scene.frame_set(f)
    dg = bpy.context.evaluated_depsgraph_get()
    for m in meshes:
        e = m.evaluated_get(dg)
        me = e.to_mesh()
        co = np.empty(len(me.vertices) * 3, dtype=np.float32)
        me.vertices.foreach_get("co", co)
        co = co.reshape(-1, 3)
        M = np.array(e.matrix_world)
        w = co @ M[:3, :3].T + M[:3, 3]
        mn = Vector(np.minimum(np.array(mn), w.min(0))); mx = Vector(np.maximum(np.array(mx), w.max(0)))
        e.to_mesh_clear()
ground = 0.0
mn.z = min(mn.z, ground)
center = (mn + mx) / 2
size = mx - mn
print("INFO bounds", tuple(round(v, 3) for v in mn), tuple(round(v, 3) for v in mx))

# floor
bpy.ops.mesh.primitive_plane_add(size=1, location=(center.x, center.y, ground))
floor = bpy.context.active_object
floor.scale = (max(size.x, size.y) * 3, max(size.x, size.y) * 3, 1)
fm = bpy.data.materials.new("floor"); fm.diffuse_color = (0.25, 0.3, 0.4, 1)
floor.data.materials.append(fm)
# origin marker (small cross) to judge travel
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0, ground))
mk = bpy.context.active_object; mk.scale = (scale_hint * 0.02, scale_hint * 0.02, scale_hint * 0.004)
mm = bpy.data.materials.new("mark"); mm.diffuse_color = (1, 0.2, 0.1, 1); mk.data.materials.append(mm)

scene.render.engine = 'BLENDER_WORKBENCH'
sh = scene.display.shading
sh.light = 'STUDIO'; sh.color_type = 'MATERIAL'; sh.show_cavity = True; sh.show_shadows = True
for m in meshes:
    mat = bpy.data.materials.new("clay_" + m.name); mat.diffuse_color = (0.85, 0.82, 0.75, 1)
    m.data.materials.clear(); m.data.materials.append(mat)
scene.render.resolution_x = CELL_W; scene.render.resolution_y = CELL_H
scene.render.film_transparent = False
scene.world = bpy.data.worlds.new("w") if scene.world is None else scene.world
scene.render.image_settings.file_format = 'PNG'

cam_data = bpy.data.cameras.new("cam"); cam_data.type = 'ORTHO'
extent = max(size.z * (CELL_W / CELL_H) * 1.0, size.x, size.y) * 1.1
cam_data.ortho_scale = max(size.z * 1.1 * CELL_W / CELL_H, max(size.x, size.y) * 1.1) * (CELL_H / CELL_W) if False else max(size.z * 1.08, max(size.x, size.y) * 1.08 * CELL_H / CELL_W)
cam_data.clip_end = 1e5
cam = bpy.data.objects.new("cam", cam_data); scene.collection.objects.link(cam); scene.camera = cam
dist = max(size) * 5 + 10


def place(dirv):
    cam.location = center + dirv * dist
    cam.rotation_euler = (-dirv).to_track_quat('-Z', 'Y').to_euler()


tmp = os.path.join(os.path.dirname(OUT), "_tmp_strip")
os.makedirs(tmp, exist_ok=True)
cells = []
for vi, dirv in enumerate((F, -L)):   # front view, then character's right side
    row = []
    for f in frames:
        scene.frame_set(f)
        place(dirv)
        p = os.path.join(tmp, "c_%d_%03d.png" % (vi, f))
        scene.render.filepath = p
        bpy.ops.render.render(write_still=True)
        img = bpy.data.images.load(p)
        a = np.array(img.pixels[:], dtype=np.float32).reshape(img.size[1], img.size[0], 4)
        bpy.data.images.remove(img)
        # thin separator
        a[:, :2, :3] = 0.05
        row.append(a)
    cells.append(np.concatenate(row, axis=1))
# Blender pixel rows are bottom-up; row 0 (front) must be on top
full = np.concatenate([cells[1], cells[0]], axis=0)
H, W = full.shape[:2]
out = bpy.data.images.new("strip", W, H, alpha=True)
out.pixels = full.ravel().tolist()
out.filepath_raw = OUT; out.file_format = 'PNG'; out.save()
for fn in os.listdir(tmp):
    os.remove(os.path.join(tmp, fn))
os.rmdir(tmp)
print("INFO frames", frames)
print("STRIP_OK", OUT)

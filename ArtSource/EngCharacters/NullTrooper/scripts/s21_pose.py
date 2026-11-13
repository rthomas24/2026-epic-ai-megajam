# Stage 21: pose test renders (walk-like, arms raised). -- <label> [mode]
import bpy, os, sys, math, runpy
from mathutils import Matrix, Vector
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
a = sys.argv[sys.argv.index("--") + 1:]
label = a[0]; mode = a[1] if len(a) > 1 else "clay"
arm = bpy.data.objects["root"]
rr = runpy.run_path(os.path.join(ROOT, "..", "..", "..", ".claude", "skills", "blender-mcp", "scripts", "review_renders.py"))


def rot(bone, axis, deg):
    pb = arm.pose.bones[bone]
    bpy.context.view_layer.update()
    head = pb.matrix.to_translation()
    R = Matrix.Translation(head) @ Matrix.Rotation(math.radians(deg), 4, Vector(axis)) @ Matrix.Translation(-head)
    pb.matrix = R @ pb.matrix
    bpy.context.view_layer.update()


def reset():
    for pb in arm.pose.bones: pb.matrix_basis = Matrix.Identity(4)
    bpy.context.view_layer.update()


POSES = {
    "walk": [("thigh_l", (1, 0, 0), 38), ("calf_l", (1, 0, 0), -55), ("foot_l", (1, 0, 0), 12), ("thigh_r", (1, 0, 0), -22), ("calf_r", (1, 0, 0), -18),
             ("ball_r", (1, 0, 0), 25), ("spine_03", (0, 0, 1), 8), ("upperarm_l", (1, 0, 0), -30), ("lowerarm_l", (1, 0, 0), 35),
             ("upperarm_r", (1, 0, 0), 35), ("lowerarm_r", (1, 0, 0), 50), ("head", (0, 0, 1), -25)],
    "armsup": [("clavicle_l", (0, 1, 0), -14), ("clavicle_r", (0, 1, 0), 14), ("upperarm_l", (0, 1, 0), -95), ("upperarm_r", (0, 1, 0), 95),
               ("lowerarm_l", (0, 1, 0), -30), ("lowerarm_r", (0, 1, 0), 30), ("head", (1, 0, 0), -15), ("spine_03", (1, 0, 0), -10)],
}
ONLY = a[2].split(",") if len(a) > 2 else list(POSES)
for name, steps in POSES.items():
    if name not in ONLY: continue
    reset()
    for b, ax, deg in steps: rot(b, ax, deg)
    r = rr["main"](targets=[next(o.name for o in bpy.data.objects if o.name.startswith("SK_Null"))], out_dir=os.path.join(ROOT, "review"), label="%s_%s" % (label, name), mode=mode,
                   views=["front", "left", "front_3q", "back_3q"], resolution=800, columns=4)
    print("POSE", name, r.get("sheet"))
reset()

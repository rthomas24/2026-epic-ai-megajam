# Pilot: review renders. One sheet of the rest pose (front, back, side, three-quarter; textured) and one of the middle of the run cycle.
# Run: blender --background work/Pilot_v01.blend --python scripts/p20_review.py
import bpy, os, runpy
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REPO = os.path.abspath(os.path.join(ROOT, "..", "..", ".."))
m = runpy.run_path(os.path.join(REPO, ".claude", "skills", "blender-mcp", "scripts", "review_renders.py"))
out = os.path.join(ROOT, "review")
arm = bpy.data.objects["root"]; arm.hide_render = True
print("RESULT", m["main"](targets=["SK_Pilot"], out_dir=out, label="pilot_rest", mode="material", views=("front", "back", "right", "front_3q"), resolution=768, columns=4))
print("RESULT", m["main"](targets=["SK_Pilot"], out_dir=out, label="pilot_rest_clay", mode="clay", views=("front", "back", "right", "front_3q"), resolution=768, columns=4))
def pose(action, frame, label, views):
    a = bpy.data.actions[action]
    arm.animation_data.action = a; arm.animation_data.action_slot = a.slots[0]
    bpy.context.scene.frame_set(frame); bpy.context.view_layer.update()
    print("RESULT", m["main"](targets=["SK_Pilot"], out_dir=out, label=label, mode="material", views=views, resolution=768, columns=len(views)))
pose("Running", 10, "pilot_run_f10", ("right", "front_3q", "back_3q"))
pose("Walking", 8, "pilot_walk_f08", ("right", "front_3q"))
pose("Idle_02", 28, "pilot_idle02_f28", ("front", "front_3q"))
pose("Idle_6", 110, "pilot_idle6_f110", ("front", "front_3q"))
pose("Idle_3", 150, "pilot_idle3_f150", ("front", "front_3q"))

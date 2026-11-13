# matsheet.py -- <label> <res> <views,comma> : material review on a dark background (like the concept)
import bpy, sys, os, runpy
a = sys.argv[sys.argv.index("--") + 1:]
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
m = runpy.run_path(os.path.join(ROOT, "..", "..", "..", ".claude", "skills", "blender-mcp", "scripts", "review_renders.py"))
views = a[2].split(",")
r = m["main"](targets=[next(o.name for o in bpy.data.objects if o.name.startswith("SK_Null"))], out_dir=os.path.join(ROOT, "review"), label=a[0], mode="material", views=views, resolution=int(a[1]),
              columns=min(len(views), 4), background=(0.035, 0.035, 0.04), samples=32)
print("MATSHEET", r.get("sheet"), r.get("engine"))

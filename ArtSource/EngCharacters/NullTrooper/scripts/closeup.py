# closeup.py -- <label> <mode> <res> <views comma> <prefix,prefix,...> [zmin zmax]
import bpy, sys, runpy, os, json
a = sys.argv[sys.argv.index("--")+1:]
label, mode, res, views, pre = a[0], a[1], int(a[2]), a[3].split(","), tuple(a[4].split(","))
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
names = [o.name for o in bpy.data.collections["NullTrooper"].objects if o.type == 'MESH' and o.name.startswith(pre)]
if len(a) > 6:
    zmin, zmax = float(a[5]), float(a[6])
    names = []
    for o in bpy.data.collections["NullTrooper"].objects:
        if o.type != 'MESH': continue
        zs = [(o.matrix_world @ v.co).z for v in o.data.vertices]
        if min(zs) >= zmin and max(zs) <= zmax: names.append(o.name)
m = runpy.run_path(os.path.join(ROOT, "..", "..", "..", ".claude", "skills", "blender-mcp", "scripts", "review_renders.py"))
r = m["main"](targets=names, out_dir=os.path.join(ROOT, "review"), label=label, mode=mode, views=views, resolution=res, columns=len(views) if len(views) < 4 else 3)
print("CLOSEUP", r.get("sheet"), len(names))

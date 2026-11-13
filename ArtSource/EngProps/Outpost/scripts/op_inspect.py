# Triad Drift - inspect Grok Bot's outpost / console FBX (read only): bounds, deck, ramp slope, bay, islands, open edges.
# blender --background --factory-startup --python op_inspect.py -- <src_dir> <out_json>
import bpy, bmesh, sys, json, math
from mathutils import Vector
argv = sys.argv[sys.argv.index("--") + 1:]
SRC, OUT = argv[0], argv[1]
res = {}
for name in ("SM_VerdanceOutpost", "SM_ResearchConsole"):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=SRC + "/" + name + ".fbx")
    objs = [o for o in bpy.data.objects if o not in before]
    r = {"objects": [(o.name, o.type, [round(v, 4) for v in o.scale], [round(math.degrees(v), 2) for v in o.rotation_euler]) for o in objs]}
    o = [x for x in objs if x.type == "MESH"][0]
    me = o.data
    mw = o.matrix_world
    bm = bmesh.new(); bm.from_mesh(me); bm.transform(mw)
    bm.normal_update()
    co = [v.co for v in bm.verts]
    lo = Vector((min(c.x for c in co), min(c.y for c in co), min(c.z for c in co)))
    hi = Vector((max(c.x for c in co), max(c.y for c in co), max(c.z for c in co)))
    r["lo_cm"] = [round(v * 100, 1) for v in lo]; r["hi_cm"] = [round(v * 100, 1) for v in hi]
    r["tris"] = sum(len(f.verts) - 2 for f in bm.faces); r["verts"] = len(bm.verts)
    r["materials"] = [m.name if m else None for m in me.materials]; r["uv"] = [u.name for u in me.uv_layers]
    # weld copy for topology
    bw = bm.copy(); bmesh.ops.remove_doubles(bw, verts=bw.verts, dist=1e-5)
    r["welded_verts"] = len(bw.verts)
    r["boundary_edges"] = sum(1 for e in bw.edges if len(e.link_faces) == 1)
    r["nonmanifold_edges"] = sum(1 for e in bw.edges if len(e.link_faces) > 2)
    # islands
    seen = set(); isl = []
    for v in bw.verts:
        if v.index in seen: continue
        stack = [v]; seen.add(v.index); vs = []
        while stack:
            a = stack.pop(); vs.append(a)
            for e in a.link_edges:
                b = e.other_vert(a)
                if b.index not in seen: seen.add(b.index); stack.append(b)
        l = [min(q.co[i] for q in vs) * 100 for i in range(3)]; h = [max(q.co[i] for q in vs) * 100 for i in range(3)]
        isl.append({"verts": len(vs), "lo": [round(x) for x in l], "hi": [round(x) for x in h]})
    isl.sort(key=lambda d: -d["verts"]); r["islands"] = len(isl); r["island_list"] = isl[:40]
    # up-facing area histogram by Z (5 cm bins)
    hist = {}
    for f in bm.faces:
        if f.normal.z > 0.97:
            z = round(f.calc_center_median().z * 100 / 2) * 2
            hist[z] = hist.get(z, 0) + f.calc_area() * 1e4
    r["up_area_by_z_cm2"] = sorted(((z, round(a)) for z, a in hist.items() if a > 2000), key=lambda t: -t[1])[:25]
    # sloped up faces (ramp candidates)
    slopes = []
    for f in bm.faces:
        if 0.3 < f.normal.z <= 0.97 and f.calc_area() * 1e4 > 300:
            c = f.calc_center_median() * 100
            slopes.append((round(math.degrees(math.acos(f.normal.z)), 1), round(f.calc_area() * 1e4), [round(c.x), round(c.y), round(c.z)], [round(n, 2) for n in f.normal]))
    slopes.sort(key=lambda t: -t[1]); r["sloped_faces"] = slopes[:40]
    res[name] = r
    bm.free(); bw.free()
json.dump(res, open(OUT, "w"), indent=1)
print("__RESULT__ ok")

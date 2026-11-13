import bpy, bmesh, collections
o = bpy.data.objects["Manny_Body"]; me = o.data
vg = {g.index: g.name for g in o.vertex_groups}
dom = []
for v in me.vertices:
    best = max(v.groups, key=lambda g: g.weight) if v.groups else None
    dom.append(vg[best.group] if best else None)
cnt = collections.Counter()
me.calc_loop_triangles()
for t in me.loop_triangles:
    cnt[dom[t.vertices[0]]] += 1
print("TRIS", len(me.loop_triangles))
for k, v in sorted(cnt.items(), key=lambda kv: -kv[1]): print("  ", k, v)
bm = bmesh.new(); bm.from_mesh(me)
seen = set(); isl = []
for v in bm.verts:
    if v.index in seen: continue
    st = [v]; seen.add(v.index); n = 0; zs = []
    while st:
        a = st.pop(); n += 1; zs.append(a.co.z)
        for e in a.link_edges:
            b = e.other_vert(a)
            if b.index not in seen: seen.add(b.index); st.append(b)
    isl.append((n, min(zs), max(zs)))
print("ISLANDS", len(isl), sorted(isl, reverse=True)[:12])
print("boundary edges", sum(1 for e in bm.edges if len(e.link_faces) == 1))
# cross-sections
import math
for z in (1.70,1.62,1.56,1.50,1.42,1.30,1.20,1.10,1.02,0.96,0.90,0.80,0.60,0.50,0.40,0.20,0.10,0.04):
    pts = [v.co for v in bm.verts if abs(v.co.z - z) < 0.012 and abs(v.co.x) < 0.22]
    if pts: print("z %.2f x[%.3f %.3f] y[%.3f %.3f] n%d" % (z, min(p.x for p in pts), max(p.x for p in pts), min(p.y for p in pts), max(p.y for p in pts), len(pts)))

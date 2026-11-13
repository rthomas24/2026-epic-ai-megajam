# Pilot: second test of the normal map's green convention. Across a UV seam the same 3D edge is textured twice; with the
# right green sign the two decoded surface normals agree, with the wrong sign they diverge. Uses work/Pilot_v01.blend.
# Run: blender --background work/Pilot_v01.blend --python scripts/p12_normal_seam_test.py
import bpy, os, numpy as np
from mathutils import Vector
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
me = bpy.data.objects["SK_Pilot"].data
img = bpy.data.images.load(os.path.join(ROOT, "export", "T_Pilot_N_source_opengl.png")); img.colorspace_settings.name = "Non-Color"
w, h = img.size
px = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)
me.calc_tangents(uvmap=me.uv_layers[0].name)
uv = me.uv_layers[0].data
edges = {}
def pos(i):
    return tuple(round(c, 5) for c in me.vertices[i].co)
for poly in me.polygons:
    ls = list(poly.loop_indices)
    cuv = sum((Vector(uv[l].uv) for l in ls), Vector((0, 0))) / len(ls)
    for i, l0 in enumerate(ls):
        l1 = ls[(i + 1) % len(ls)]
        # the glTF importer splits vertices at UV seams, so match edges by position, not by vertex index
        v0, v1 = pos(me.loops[l0].vertex_index), pos(me.loops[l1].vertex_index)
        edges.setdefault((min(v0, v1), max(v0, v1)), []).append((l0, l1, cuv))
def decode(l0, l1, cuv, sign):
    m = (Vector(uv[l0].uv) + Vector(uv[l1].uv)) / 2
    d = cuv - m
    if d.length > 1e-9: m = m + d.normalized() * (2.0 / w)       # 2 texels inside the island
    x = min(max(int(m.x * w), 0), w - 1); y = min(max(int(m.y * h), 0), h - 1)
    c = px[y, x]; n = Vector((c[0] * 2 - 1, (c[1] * 2 - 1) * sign, c[2] * 2 - 1))
    A, B = me.loops[l0], me.loops[l1]
    T = (Vector(A.tangent) + Vector(B.tangent)).normalized(); N = (Vector(A.normal) + Vector(B.normal)).normalized()
    Bt = N.cross(T) * A.bitangent_sign
    return (T * n.x + Bt * n.y + N * n.z).normalized(), N
agree = {1: [], -1: []}; base = []
for key, sides in edges.items():
    if len(sides) != 2: continue
    (a0, a1, ca), (b0, b1, cb) = sides
    ua = {pos(me.loops[a0].vertex_index): Vector(uv[a0].uv), pos(me.loops[a1].vertex_index): Vector(uv[a1].uv)}
    ub = {pos(me.loops[b0].vertex_index): Vector(uv[b0].uv), pos(me.loops[b1].vertex_index): Vector(uv[b1].uv)}
    if all((ua[v] - ub[v]).length < 1e-5 for v in ua): continue          # not a seam
    for s in (1, -1):
        na, Na = decode(a0, a1, ca, s); nb, Nb = decode(b0, b1, cb, s)
        agree[s].append(na.dot(nb))
    base.append(Na.dot(Nb))
for s, name in ((1, "OpenGL (+Y, as glTF specifies)"), (-1, "DirectX (-Y)")):
    a = np.array(agree[s]); print("SEAM read as %-32s seam edges %d  mean dot %.4f  median %.4f  share > 0.95: %.3f" % (name, a.size, a.mean(), np.median(a), (a > 0.95).mean()))
print("SEAM vertex normals only (no map)                 mean dot %.4f" % float(np.mean(base)))

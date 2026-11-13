# SPDX-License-Identifier: MIT
# Triad Drift - RiftHero geometry kit: clean quad surfaces from analytic functions, with real recessed
# panel lines / pockets cut into the grid (no booleans), lathes around any axis, mirroring and materials.
# Units: metres. Ship frame: nose +X, up +Z, Blender Y = -Unreal Y.
import math

import bmesh
import bpy
import numpy as np
from mathutils import Vector

COLL = "Rift"


def collection():
    c = bpy.data.collections.get(COLL)
    if c is None:
        c = bpy.data.collections.new(COLL)
        bpy.context.scene.collection.children.link(c)
    return c


# ------------------------------------------------------------------ interpolation
def pchip(xk, yk, x):
    """Monotone cubic (Fritsch-Carlson) interpolation; yk may be (n,) or (n, m). xk ascending."""
    xk = np.asarray(xk, float)
    yk = np.asarray(yk, float)
    one = yk.ndim == 1
    if one:
        yk = yk[:, None]
    h = np.diff(xk)
    d = np.diff(yk, axis=0) / h[:, None]
    m = np.zeros_like(yk)
    m[0], m[-1] = d[0], d[-1]
    for i in range(1, len(xk) - 1):
        for c in range(yk.shape[1]):
            if d[i - 1, c] * d[i, c] > 0:
                w1, w2 = 2 * h[i] + h[i - 1], h[i] + 2 * h[i - 1]
                m[i, c] = (w1 + w2) / (w1 / d[i - 1, c] + w2 / d[i, c])
    x = np.clip(np.asarray(x, float), xk[0], xk[-1])
    i = np.clip(np.searchsorted(xk, x, side="right") - 1, 0, len(xk) - 2)
    t = ((x - xk[i]) / h[i])[..., None]
    h00, h10 = 2 * t ** 3 - 3 * t ** 2 + 1, t ** 3 - 2 * t ** 2 + t
    h01, h11 = -2 * t ** 3 + 3 * t ** 2, t ** 3 - t ** 2
    out = h00 * yk[i] + h10 * h[i][..., None] * m[i] + h01 * yk[i + 1] + h11 * h[i][..., None] * m[i + 1]
    return out[..., 0] if one else out


def spow(c, n):
    return math.copysign(abs(c) ** (2.0 / n), c)


# ------------------------------------------------------------------ materials
def mat(name, color, metallic=0.0, roughness=0.45, emission=None, strength=0.0, alpha=1.0):
    m = bpy.data.materials.get(name)
    if m is None:
        m = bpy.data.materials.new(name)
    b = next(n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    b.inputs["Base Color"].default_value = (*color, 1.0)
    b.inputs["Metallic"].default_value = metallic
    b.inputs["Roughness"].default_value = roughness
    if emission:
        b.inputs["Emission Color"].default_value = (*emission, 1.0)
        b.inputs["Emission Strength"].default_value = strength
    else:                                   # the default emission colour is white: it would bake as a white E map
        b.inputs["Emission Color"].default_value = (0.0, 0.0, 0.0, 1.0)
        b.inputs["Emission Strength"].default_value = 0.0
    m.diffuse_color = (*color, 1.0)
    return m


def _finish(name, bm, mats, smooth_deg=38.0, parent=None):
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
    bmesh.ops.dissolve_degenerate(bm, dist=1e-7, edges=bm.edges)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    for m in mats:
        me.materials.append(m)
    me.shade_smooth()
    me.set_sharp_from_angle(angle=math.radians(smooth_deg))
    ob = bpy.data.objects.new(name, me)
    collection().objects.link(ob)
    if parent is not None:
        ob.parent = parent
    return ob


# ------------------------------------------------------------------ grid surface with pockets
def _axis(base, pockets, lo_key, hi_key, e_key, closed_span=None):
    """Sorted sample coordinates: base samples plus pocket lips and floor edges; base samples that crowd a
    pocket edge are dropped so no sliver quads appear."""
    special = []
    for p in pockets:
        a, b, e = p[lo_key], p[hi_key], p[e_key]
        s = p.get("support", 0.0) * (e / max(p["e_m"], 1e-9)) if p.get("support") else 0.0
        special += [a, a + e, b - e, b]
        if s:
            special += [a - s, b + s]
    special = sorted(set(round(v, 9) for v in special))
    out = list(special)
    if special:
        sp = np.array(special)
        gap = min(p[e_key] for p in pockets) * 1.5
        for v in base:
            if np.min(np.abs(sp - v)) > gap:
                out.append(float(v))
    else:
        out = [float(v) for v in base]
    return sorted(set(round(v, 9) for v in out))


def grid_surface(name, f, us, vs, mats, pockets=(), closed_u=True, cap0="ngon", cap1="ngon",
                 mat_fn=None, flip=False, smooth_deg=38.0, parent=None, uv_scale=(1.0, 1.0),
                 simplify_deg=0.35):
    """Quad surface P = f(u, v) -> (x, y, z). us / vs: base sample coordinates (us without the wrap duplicate
    when closed_u). pockets: dicts {u0, u1, v0, v1, depth (m, >0 recessed, <0 raised), eu, ev (wall widths in
    param units), mat (material index for the floor, optional), wall_mat}. Pocket lips and floors become real
    edges of the grid, so panel lines follow the form exactly. cap0/cap1: 'ngon' | 'point' | 'none' at
    vs[0] / vs[-1]. mat_fn(u, v) -> material index for faces outside pockets."""
    pockets = [dict(p) for p in pockets]
    for p in pockets:
        p.setdefault("e_m", 1.0)
    U = _axis(us, pockets, "u0", "u1", "eu")
    V = _axis(vs, pockets, "v0", "v1", "ev")
    V = [v for v in V if vs[0] - 1e-9 <= v <= vs[-1] + 1e-9]
    nu, nv = len(U), len(V)
    P = np.zeros((nu, nv, 3))
    for i, u in enumerate(U):
        for j, v in enumerate(V):
            P[i, j] = f(u, v)
    # normals by central differences on the base surface
    du, dv = 1e-4, 1e-4
    N = np.zeros_like(P)
    need = np.zeros((nu, nv), bool)
    D = np.zeros((nu, nv))
    tol = 1e-8
    for p in pockets:
        for i, u in enumerate(U):
            if not (p["u0"] + p["eu"] - tol <= u <= p["u1"] - p["eu"] + tol):
                continue
            for j, v in enumerate(V):
                if p["v0"] + p["ev"] - tol <= v <= p["v1"] - p["ev"] + tol:
                    if abs(p["depth"]) > abs(D[i, j]):
                        D[i, j] = p["depth"]
                    need[i, j] = True
    for i, u in enumerate(U):
        for j, v in enumerate(V):
            if not need[i, j]:
                continue
            v0, v1 = max(vs[0], v - dv), min(vs[-1], v + dv)
            a = np.array(f(u + du, v)) - np.array(f(u - du, v))
            b = np.array(f(u, v1)) - np.array(f(u, v0))
            n = np.cross(a, b)
            ln = np.linalg.norm(n)
            N[i, j] = n / ln if ln > 1e-12 else 0.0
    if flip:
        N = -N
    # orient: outward should point away from the ring centroid (sampled once)
    P2 = P - N * D[..., None] * _outward_sign(P, N, need)

    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    point0 = cap0 == "point"
    point1 = cap1 == "point"
    vert = {}
    for j in range(nv):
        if (j == 0 and point0) or (j == nv - 1 and point1):
            vert[("p", j)] = bm.verts.new(P2[:, j].mean(axis=0))
            continue
        for i in range(nu):
            vert[(i, j)] = bm.verts.new(P2[i, j])

    def V_(i, j):
        return vert.get(("p", j)) or vert[(i % nu, j)]

    def pocket_mat(u, v):
        best = None
        for p in pockets:
            if p["u0"] < u < p["u1"] and p["v0"] < v < p["v1"]:
                inside = p["u0"] + p["eu"] < u < p["u1"] - p["eu"] and p["v0"] + p["ev"] < v < p["v1"] - p["ev"]
                m = p.get("mat") if inside else p.get("wall_mat", p.get("mat"))
                if m is not None:
                    best = m
        return best

    span_u = (U[-1] - U[0]) or 1.0
    span_v = (V[-1] - V[0]) or 1.0
    period = None
    if closed_u:
        period = 2.0 * math.pi
    ni = nu if closed_u else nu - 1
    for j in range(nv - 1):
        for i in range(ni):
            i1 = i + 1
            quad = [V_(i, j), V_(i1, j), V_(i1, j + 1), V_(i, j + 1)]
            uniq = []
            for q in quad:
                if q not in uniq:
                    uniq.append(q)
            if len(uniq) < 3:
                continue
            if flip:
                uniq.reverse()
            try:
                face = bm.faces.new(uniq)
            except ValueError:
                continue
            ua = U[i]
            ub = U[i1 % nu] if i1 < nu else U[0] + period
            if closed_u and i1 == nu:
                ub = U[0] + period
            um, vm = 0.5 * (ua + ub), 0.5 * (V[j] + V[j + 1])
            pm = pocket_mat(um, vm)
            face.material_index = pm if pm is not None else (mat_fn(um, vm) if mat_fn else 0)
            face.smooth = True
            corner = {id(V_(i, j)): (ua, V[j]), id(V_(i1, j)): (ub, V[j]),
                      id(V_(i1, j + 1)): (ub, V[j + 1]), id(V_(i, j + 1)): (ua, V[j + 1])}
            for loop in face.loops:
                cu, cv = corner[id(loop.vert)]
                loop[uvl].uv = ((cu - U[0]) / span_u * uv_scale[0], (cv - V[0]) / span_v * uv_scale[1])
    for j, cap in ((0, cap0), (nv - 1, cap1)):
        if cap == "ngon" and closed_u:
            ring = [vert[(i, j)] for i in range(nu)]
            if (j == 0) != flip:
                ring.reverse()
            try:
                face = bm.faces.new(ring)
                face.material_index = mat_fn(U[0], V[j]) if mat_fn else 0
                for loop in face.loops:
                    loop[uvl].uv = (0.5 + 0.4 * loop.vert.co.y, 0.5 + 0.4 * loop.vert.co.z)
            except ValueError:
                pass
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    if pockets and simplify_deg:
        # the pocket lips run across the whole grid; dissolve the parts of those lines that carry no shape
        bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
        bmesh.ops.dissolve_limit(bm, angle_limit=math.radians(simplify_deg), use_dissolve_boundaries=False,
                                 verts=bm.verts[:], edges=bm.edges[:], delimit={"MATERIAL", "UV"})
        ng = [f for f in bm.faces if len(f.verts) > 4]
        if ng:
            bmesh.ops.triangulate(bm, faces=ng, quad_method="BEAUTY", ngon_method="BEAUTY")
    return _finish(name, bm, mats, smooth_deg, parent)


def _outward_sign(P, N, need):
    """+1 if N points outward (away from the surface's centroid) on average, else -1."""
    if not need.any():
        return 1.0
    c = P.reshape(-1, 3).mean(axis=0)
    d = ((P - c) * N)[need].sum()
    return 1.0 if d >= 0 else -1.0


# ------------------------------------------------------------------ lathe (solid of revolution about X)
def lathe_x(name, profile, centre, mats, segments=48, mat_idx=None, smooth_deg=30.0, parent=None, arc=None):
    """profile: closed polygon [(x, r), ...] in metres relative to `centre` (x along the ship, r from the
    axis), wound so the outside is on the left when walking it... simply: recalculated afterwards.
    mat_idx: material index per profile edge k (edge k joins point k and k+1). r = 0 points collapse."""
    cx, cy, cz = centre
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    n = len(profile)
    rings = []
    for (x, r) in profile:
        if r <= 1e-7:
            rings.append([bm.verts.new((cx + x, cy, cz))] * segments)
        else:
            rings.append([bm.verts.new((cx + x, cy + r * math.cos(2 * math.pi * s / segments),
                                        cz + r * math.sin(2 * math.pi * s / segments))) for s in range(segments)])
    lens = [0.0]
    for k in range(n):
        a, b = profile[k], profile[(k + 1) % n]
        lens.append(lens[-1] + math.hypot(b[0] - a[0], b[1] - a[1]))
    total = lens[-1] or 1.0
    for k in range(n):
        a, b = rings[k], rings[(k + 1) % n]
        for s in range(segments):
            s1 = (s + 1) % segments
            quad = []
            for q in (a[s], a[s1], b[s1], b[s]):
                if q not in quad:
                    quad.append(q)
            if len(quad) < 3:
                continue
            try:
                face = bm.faces.new(quad)
            except ValueError:
                continue
            face.material_index = mat_idx[k] if mat_idx else 0
            face.smooth = True
            uvs = {id(a[s]): (s / segments, lens[k] / total), id(a[s1]): ((s + 1) / segments, lens[k] / total),
                   id(b[s1]): ((s + 1) / segments, lens[k + 1] / total), id(b[s]): (s / segments, lens[k + 1] / total)}
            for loop in face.loops:
                loop[uvl].uv = uvs[id(loop.vert)]
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return _finish(name, bm, mats, smooth_deg, parent)


# ------------------------------------------------------------------ utilities
def mirror_y(ob):
    """Add the exact mirror image across Y = 0 into the same mesh (data level; no modifier, no weld)."""
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    geom = bm.verts[:] + bm.edges[:] + bm.faces[:]
    ret = bmesh.ops.duplicate(bm, geom=geom)
    new_v = [g for g in ret["geom"] if isinstance(g, bmesh.types.BMVert)]
    new_f = [g for g in ret["geom"] if isinstance(g, bmesh.types.BMFace)]
    for v in new_v:
        v.co.y = -v.co.y
    bmesh.ops.reverse_faces(bm, faces=new_f)
    bm.to_mesh(ob.data)
    bm.free()
    ob.data.update()
    return ob


def bevel(ob, width=0.006, segments=2, angle_deg=32.0, weighted=True):
    m = ob.modifiers.new("Bevel", "BEVEL")
    m.width = width
    m.segments = segments
    m.limit_method = "ANGLE"
    m.angle_limit = math.radians(angle_deg)
    m.harden_normals = True
    m.use_clamp_overlap = True
    m.miter_outer = "MITER_ARC"
    if weighted:
        w = ob.modifiers.new("WeightedNormal", "WEIGHTED_NORMAL")
        w.keep_sharp = True
        w.weight = 50
    return m


def box(name, lo, hi, mats, mat_index=0, parent=None, taper=None):
    """Axis-aligned box from two corners (metres). taper=(sy, sz) scales the +X face about its centre."""
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    lo, hi = Vector(lo), Vector(hi)
    c = [(x, y, z) for x in (lo.x, hi.x) for y in (lo.y, hi.y) for z in (lo.z, hi.z)]
    vs = []
    for (x, y, z) in c:
        if taper and x == hi.x:
            cy, cz = (lo.y + hi.y) / 2, (lo.z + hi.z) / 2
            y, z = cy + (y - cy) * taper[0], cz + (z - cz) * taper[1]
        vs.append(bm.verts.new((x, y, z)))
    for idx in ((0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)):
        face = bm.faces.new([vs[i] for i in idx])
        face.material_index = mat_index
        for k, loop in enumerate(face.loops):
            loop[uvl].uv = ((0, 0), (1, 0), (1, 1), (0, 1))[k]
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return _finish(name, bm, mats, 30.0, parent)


def transform(ob, matrix):
    ob.data.transform(matrix)
    ob.data.update()
    return ob


def tri_count(objs):
    dg = bpy.context.evaluated_depsgraph_get()
    n = 0
    for o in objs:
        ev = o.evaluated_get(dg)
        me = ev.to_mesh()
        me.calc_loop_triangles()
        n += len(me.loop_triangles)
        ev.to_mesh_clear()
    return n

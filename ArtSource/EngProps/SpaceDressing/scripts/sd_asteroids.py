# SPDX-License-Identifier: MIT
"""
sd_asteroids.py - ice, metal and crystal asteroids for the SpaceDressing kit.

Each rock is a pure function of direction (lumps, craters, exact facet planes in stretched space,
then family detail: ice cracks, metal thumbprint dents + raised ore veins, flake scars), evaluated on
two icospheres:
  - game mesh  (subdiv 6, lowpass: no fine detail) -> mild collapse decimation to the budget
  - bake source (subdiv 8, ~330k tris, all detail + per-vertex masks for the material)
so the low and high agree everywhere except the fine detail, which goes into the normal map.
Crystal asteroids add faceted crystal clusters (identical on both meshes) grown from the surface.
"""

import math
import random

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector, noise
from mathutils.bvhtree import BVHTree

import sd_lib as L

# --------------------------------------------------------------------------- specs
# size = longest dimension (m) of the finished asset; tris = game-mesh budget of the rock body.
SPECS = {
    "SM_Ast_Ice_01": dict(family="ice", seed=101, size=72.0, tris=13000, tex=2048,
                          stretch=(1.35, 0.92, 0.80), lump=(0.13, 1.1), craters=dict(large=1, medium=2, small=3),
                          facets=dict(n=6, frac=(0.78, 0.9)), cracks=dict(amp=0.05, width=0.08, freq=1.5, mask=0.55),
                          glow="#5CCBF2"),
    "SM_Ast_Ice_02": dict(family="ice", seed=113, size=46.0, tris=11000, tex=2048,
                          stretch=(1.3, 0.95, 0.78), lump=(0.10, 1.3), craters=dict(large=1, medium=3, small=4),
                          facets=dict(n=7, frac=(0.76, 0.9)),
                          chasm=dict(dir=(0.2, 1.0, 0.35), width=0.085, depth=0.22),
                          cracks=dict(amp=0.042, width=0.07, freq=1.9, mask=0.55), glow="#5CCBF2"),
    "SM_Ast_Ice_03": dict(family="ice", seed=127, size=26.0, tris=6500, tex=1024,
                          stretch=(1.3, 0.7, 0.62), lump=(0.08, 1.5), craters=dict(small=2),
                          facets=dict(n=11, frac=(0.62, 0.82)), cracks=dict(amp=0.04, width=0.08, freq=2.1, mask=0.6),
                          glow="#5CCBF2"),
    "SM_Ast_Metal_01": dict(family="metal", seed=211, size=55.0, tris=12000, tex=2048,
                            stretch=(1.25, 1.0, 0.82), lump=(0.13, 1.0), craters=dict(medium=2, small=3),
                            facets=dict(n=4, frac=(0.82, 0.92)), dents=(0.04, 5.0),
                            veins=dict(freq=1.4, width=0.09, relief=0.012, gate=0.5)),
    "SM_Ast_Metal_02": dict(family="metal", seed=223, size=34.0, tris=9000, tex=2048,
                            stretch=(1.6, 0.78, 0.7), lump=(0.10, 1.3), craters=dict(small=2),
                            facets=dict(n=8, frac=(0.7, 0.86)), dents=(0.032, 6.0),
                            veins=dict(freq=1.8, width=0.1, relief=0.014, gate=0.45)),
    "SM_Ast_Metal_03": dict(family="metal", seed=239, size=17.0, tris=5000, tex=1024,
                            stretch=(1.25, 0.95, 0.8), lump=(0.11, 1.2), craters=dict(),
                            facets=dict(n=7, frac=(0.7, 0.85)), dents=(0.04, 4.5),
                            veins=dict(freq=2.0, width=0.1, relief=0.016, gate=0.45)),
    "SM_Ast_Crystal_01": dict(family="crystal", seed=307, size=46.0, tris=9500, tex=2048, body=0.66,
                              stretch=(1.2, 0.95, 0.85), lump=(0.11, 1.2), craters=dict(small=3),
                              facets=dict(n=7, frac=(0.74, 0.88)), flakes=(0.022, 4.5),
                              crystal=dict(base="#2E0C7A", light="#9A4DFF", glow="#8F4BFF",
                                           clusters=[((0.55, -0.6, 0.62), 11, 0.42), ((-0.7, 0.3, 0.55), 8, 0.3),
                                                     ((0.15, 0.85, -0.45), 6, 0.24), ((-0.2, -0.3, -0.95), 5, 0.18)])),
    "SM_Ast_Crystal_02": dict(family="crystal", seed=331, size=28.0, tris=6500, tex=2048, body=0.68,
                              stretch=(1.1, 1.0, 0.8), lump=(0.10, 1.4), craters=dict(small=2),
                              facets=dict(n=8, frac=(0.72, 0.86)), flakes=(0.025, 5.0),
                              crystal=dict(base="#03504C", light="#12D9BE", glow="#1FF0D2",
                                           clusters=[((-0.4, -0.7, 0.6), 10, 0.4), ((0.8, 0.4, 0.2), 7, 0.28),
                                                     ((0.0, 0.5, -0.9), 5, 0.2)])),
}

PAL = {
    "ice": dict(white="#EEF8FC", pale="#CBE8F4", mid="#A2D2EA", blue="#3D8ED6", deep="#1A4C9C"),
    "metal": dict(dark="#2A292E", mid="#45434A", light="#6A6668", rust1="#5A3020", rust2="#86452A",
                  rust3="#A3603A", gold="#F2BC45", halo="#2A2116"),
    "crystal": dict(dark="#1C1924", mid="#2E2A39", light="#474158", patch="#5E5574", vein="#8E86A6"),
}


# --------------------------------------------------------------------------- shape function

def _spread_dirs(n, rng, jitter=0.25):
    golden = math.pi * (3.0 - math.sqrt(5.0))
    rot = (Matrix.Rotation(rng.uniform(0, math.tau), 3, "X") @ Matrix.Rotation(rng.uniform(0, math.tau), 3, "Y")
           @ Matrix.Rotation(rng.uniform(0, math.tau), 3, "Z"))
    out = []
    for i in range(n):
        z = 1.0 - 2.0 * (i + 0.5) / n
        r = math.sqrt(max(0.0, 1.0 - z * z))
        d = rot @ Vector((math.cos(golden * i) * r, math.sin(golden * i) * r, z))
        d += Vector((rng.gauss(0, jitter), rng.gauss(0, jitter), rng.gauss(0, jitter)))
        out.append(d.normalized())
    return out


def _craters(counts, rng):
    out = []

    def add(n, radius, depth, power):
        made = tries = 0
        while made < n and tries < 300:
            tries += 1
            d = Vector((rng.gauss(0, 1), rng.gauss(0, 1), rng.gauss(0, 1))).normalized()
            r = rng.uniform(*radius)
            if any(d.angle(o[0]) < (r + o[1]) * 0.85 for o in out):
                continue
            out.append((d, r, rng.uniform(*depth), power))
            made += 1
    add(counts.get("large", 0), (0.36, 0.48), (0.10, 0.13), 3.0)
    add(counts.get("medium", 0), (0.17, 0.25), (0.06, 0.08), 2.5)
    add(counts.get("small", 0), (0.08, 0.12), (0.03, 0.045), 2.0)
    return out


class Shape:
    def __init__(self, spec):
        self.spec = spec
        rng = random.Random(spec["seed"])
        self.rng = rng
        self.off = Vector((rng.uniform(-90, 90), rng.uniform(-90, 90), rng.uniform(-90, 90)))
        self.off2 = Vector((rng.uniform(-90, 90), rng.uniform(-90, 90), rng.uniform(-90, 90)))
        self.off3 = Vector((rng.uniform(-90, 90), rng.uniform(-90, 90), rng.uniform(-90, 90)))
        self.craters = _craters(spec.get("craters", {}), rng)
        for d, _count, frac in spec.get("crystal", {}).get("clusters", []):
            self.craters.append((Vector(d).normalized(), 0.22 + frac * 0.6, 0.07 + frac * 0.12, 2.0))
        sx, sy, sz = spec["stretch"]
        self.stretch = Vector(spec["stretch"])
        f = spec.get("facets")
        self.planes = []
        if f:
            for d in _spread_dirs(f["n"], rng):
                # plane normal mapped onto the ellipsoid, offset as a fraction of the support
                u = Vector((d.x / sx, d.y / sy, d.z / sz)).normalized()
                support = math.sqrt((sx * u.x) ** 2 + (sy * u.y) ** 2 + (sz * u.z) ** 2)
                self.planes.append((u, support * rng.uniform(*f["frac"])))
        self.rot1 = Matrix.Rotation(rng.uniform(0, 6.28), 3, Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), 1)).normalized())
        self.rot2 = Matrix.Rotation(rng.uniform(0, 6.28), 3, Vector((1, rng.uniform(-1, 1), rng.uniform(-1, 1))).normalized())

    def point(self, n, detail):
        sp = self.spec
        amp, freq = sp["lump"]
        h = noise.fractal(n * freq + self.off, 1.0, 2.0, 3, noise_basis="PERLIN_ORIGINAL") * amp
        for c_dir, c_r, c_depth, c_pow in self.craters:
            x = n.angle(c_dir) / c_r
            if x < 2.0:
                rim = 0.3 * math.exp(-((x - 1.0) / 0.25) ** 2)
                bowl = (x ** c_pow - 1.0) if x < 1.0 else 0.0
                h += c_depth * (bowl + rim)
        P = Vector((n.x * self.stretch.x, n.y * self.stretch.y, n.z * self.stretch.z)) * (1.0 + h)
        attrs = {"facet": 0.0, "crack": 0.0, "vein": 0.0}
        # facets: radial projection onto each plane, with a soft min so the crease is rounded a little
        if self.planes:
            k = 0.018
            terms = [math.exp(-1.0 / k)]
            tmin = 1.0
            for u, c in self.planes:
                s = P.dot(u)
                if s > 1e-6:
                    t = c / s
                    if t < 1.6:
                        terms.append(math.exp(-t / k))
                        tmin = min(tmin, t)
            t = -k * math.log(sum(terms))
            t = min(t, 1.0)
            P = P * t
            attrs["facet"] = 1.0 if tmin < 0.995 else 0.0
        chasm = sp.get("chasm")
        if chasm:
            axis = Vector(chasm["dir"]).normalized()
            d = abs(n.dot(axis)) + 0.03 * noise.noise(n * 6.0 + self.off2)
            if d < chasm["width"]:
                m = 1.0 - d / chasm["width"]
                P = P * (1.0 - chasm["depth"] * m ** 1.2)
                attrs["crack"] = max(attrs["crack"], m)
        if not detail:
            return P, attrs
        fam = sp["family"]
        radial = P.normalized()
        if fam == "ice":
            # Fracture network: Voronoi cell borders (F2 - F1 small) give straight, branching,
            # polygonal cracks - like crazed ice - at two scales; a broad mask leaves calm areas.
            cr = sp["cracks"]
            nm = (noise.noise(P * 0.9 + self.off3) + 1.0) * 0.5          # 0..1, ~0.5 average
            t = max(0.0, min(1.0, (nm - (1.0 - cr["mask"]) + 0.1) / 0.2))
            mask = t * t * (3.0 - 2.0 * t)                               # cr["mask"] ~ share of the surface cracked
            warp = Vector((noise.noise(P * 3.0 + self.off), noise.noise(P * 3.0 + self.off2),
                           noise.noise(P * 3.0 + self.off3))) * 0.06
            depth = 0.0
            crack = 0.0
            for oct_, (s, w, a, g) in enumerate(((1.0, cr["width"] * 0.8, 1.0, 1.0), (2.6, cr["width"] * 0.5, 0.35, 0.55))):
                q = (self.rot1 if oct_ else self.rot2) @ (P + warp) * cr["freq"] * s + (self.off2 if oct_ else self.off)
                dd, _p = noise.voronoi(q, distance_metric="DISTANCE")
                edge = dd[1] - dd[0]
                m = max(0.0, 1.0 - edge / w)
                gm = mask * g if oct_ == 0 else (0.25 + 0.75 * mask) * g
                depth += cr["amp"] * a * (m ** 1.4) * gm / s ** 0.4
                crack = max(crack, (m ** 0.8) * gm)
            P = P - radial * depth
            P = P + radial * noise.fractal(P * 9.0 + self.off3, 1.0, 2.0, 3) * 0.0035
            attrs["crack"] = max(attrs["crack"], crack)
        elif fam == "metal":
            amp2, f2 = sp["dents"]
            dd, _p = noise.voronoi(self.rot1 @ P * f2 + self.off2, distance_metric="DISTANCE")
            P = P + radial * (dd[0] - 0.45) * amp2
            ve = sp["veins"]
            gate = max(0.0, min(1.0, (noise.noise(P * 1.1 + self.off3) + 1.0) * 0.5 / ve["gate"] - 0.6))
            warp = Vector((noise.noise(P * 2.5 + self.off), noise.noise(P * 2.5 + self.off2),
                           noise.noise(P * 2.5 + self.off3))) * 0.12
            dv, _p = noise.voronoi(self.rot2 @ (P + warp) * ve["freq"] + self.off, distance_metric="DISTANCE")
            m = max(0.0, 1.0 - (dv[1] - dv[0]) / ve["width"]) * gate
            P = P + radial * ve["relief"] * m ** 0.7
            P = P + radial * noise.fractal(P * 14.0 + self.off3, 1.0, 2.0, 3) * 0.0025
            attrs["vein"] = m
        else:
            amp2, f2 = sp["flakes"]
            dd, _p = noise.voronoi(self.rot1 @ P * f2 + self.off2, distance_metric="MANHATTAN")
            P = P + radial * (dd[0] - 0.55) * amp2
            P = P + radial * noise.fractal(P * 12.0 + self.off3, 1.0, 2.0, 3) * 0.003
        return P, attrs


def build_shape_mesh(shape, subdiv, detail, name, coll):
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=subdiv, radius=1.0)
    layers = {k: bm.verts.layers.float.new(k) for k in ("facet", "crack", "vein")}
    for v in bm.verts:
        P, a = shape.point(v.co.normalized(), detail)
        v.co = P
        for k, lay in layers.items():
            v[lay] = a[k]
    bm.normal_update()
    return L.bm_to_object(name, bm, coll)


def cleanup_mesh(me, weld=2e-5):
    bm = bmesh.new()
    bm.from_mesh(me)
    bmesh.ops.remove_doubles(bm, verts=list(bm.verts), dist=weld)
    bmesh.ops.dissolve_degenerate(bm, edges=list(bm.edges), dist=weld)
    loose = [v for v in bm.verts if not v.link_faces]
    if loose:
        bmesh.ops.delete(bm, geom=loose, context="VERTS")
    bmesh.ops.triangulate(bm, faces=list(bm.faces), quad_method="BEAUTY", ngon_method="BEAUTY")
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bm.to_mesh(me)
    bm.free()
    me.update()


def decimate(obj, target):
    tris = L.tri_count(obj.data)
    if tris > target:
        mod = obj.modifiers.new("Decimate", "DECIMATE")
        mod.decimate_type = "COLLAPSE"
        mod.ratio = target / float(tris)
        mod.use_collapse_triangulate = True
        L.hs["apply_modifiers"](obj)
    cleanup_mesh(obj.data)


def store_curvature(me, iterations=2):
    """Per-vertex convexity (+ ridges, - creases), smoothed, as attribute 'curv' (from the SpaceRocks kit)."""
    n = len(me.vertices)
    co = L.mesh_points(me)
    nor = np.empty(n * 3, dtype=np.float64)
    me.vertices.foreach_get("normal", nor)
    nor = nor.reshape(-1, 3)
    ed = np.empty(len(me.edges) * 2, dtype=np.int32)
    me.edges.foreach_get("vertices", ed)
    ed = ed.reshape(-1, 2)
    cnt = np.zeros(n)
    np.add.at(cnt, ed[:, 0], 1.0)
    np.add.at(cnt, ed[:, 1], 1.0)
    cnt = np.maximum(cnt, 1.0)
    acc = np.zeros((n, 3))
    np.add.at(acc, ed[:, 0], co[ed[:, 1]])
    np.add.at(acc, ed[:, 1], co[ed[:, 0]])
    edge_len = float(np.linalg.norm(co[ed[:, 0]] - co[ed[:, 1]], axis=1).mean())
    curv = -((acc / cnt[:, None] - co) * nor).sum(axis=1) / edge_len
    for _ in range(iterations):
        a = np.zeros(n)
        np.add.at(a, ed[:, 0], curv[ed[:, 1]])
        np.add.at(a, ed[:, 1], curv[ed[:, 0]])
        curv = 0.5 * curv + 0.5 * a / cnt
    attr = me.attributes.get("curv") or me.attributes.new("curv", "FLOAT", "POINT")
    attr.data.foreach_set("value", curv.astype(np.float32))
    return {"p05": float(np.percentile(curv, 5)), "p50": float(np.percentile(curv, 50)), "p95": float(np.percentile(curv, 95))}


def split_uv_at_sharp(me):
    """Detach UV islands along every sharp edge (from the SpaceRocks kit), then pack re-fits them."""
    bm = bmesh.new()
    bm.from_mesh(me)
    uv = bm.loops.layers.uv.verify()
    bm.faces.ensure_lookup_table()

    def loop_uv(face, vert):
        for lp in face.loops:
            if lp.vert is vert:
                return lp[uv].uv
        return None
    comp = {}
    n_comp = 0
    for f in bm.faces:
        if f.index in comp:
            continue
        comp[f.index] = n_comp
        stack = [f]
        while stack:
            g = stack.pop()
            for e in g.edges:
                if not e.smooth or len(e.link_faces) != 2:
                    continue
                o = e.link_faces[0] if e.link_faces[1] is g else e.link_faces[1]
                if o.index in comp:
                    continue
                if all((loop_uv(g, v) - loop_uv(o, v)).length < 1e-6 for v in e.verts):
                    comp[o.index] = n_comp
                    stack.append(o)
        n_comp += 1
    cols = int(math.ceil(math.sqrt(n_comp)))
    for f in bm.faces:
        k = comp[f.index]
        shift = Vector((2.0 * (k % cols), 2.0 * (k // cols)))
        for lp in f.loops:
            lp[uv].uv = lp[uv].uv + shift
    bm.to_mesh(me)
    bm.free()
    return n_comp


def deviation(low, sources):
    def bvh_of(objs):
        bm = bmesh.new()
        for o in objs:
            bm.from_mesh(o.data)
        return BVHTree.FromBMesh(bm), bm
    low_tree, low_bm = bvh_of([low])
    d = []
    for o in sources:
        for co in L.mesh_points(o.data)[::3]:
            hit = low_tree.find_nearest(Vector(co))
            if hit[0] is not None:
                d.append(hit[3])
    low_bm.free()
    src_tree, src_bm = bvh_of(sources)
    back = [src_tree.find_nearest(v.co)[3] for v in low.data.vertices]
    back += [src_tree.find_nearest(f.center)[3] for f in low.data.polygons]
    src_bm.free()
    d, b = np.array(d), np.array(back)
    return {"p99_m": float(np.percentile(d, 99.0)), "max_m": float(d.max()),
            "game_to_source_p995_m": float(np.percentile(b, 99.5)), "game_to_source_max_m": float(b.max())}


# --------------------------------------------------------------------------- crystals

def crystal_bm(clusters_world, bvh, rng, scale):
    """Clusters of faceted hexagonal crystals grown out of the surface (bases embedded ~30%).
    clusters_world: [(direction, count, size_fraction)] ; scale = asset size (m).
    Returns (bm with 'cglow' vertex layer, anchors, hull point lists)."""
    bm = bmesh.new()
    lay = bm.verts.layers.float.new("cglow")
    anchors, hulls = [], []
    for direction, count, frac in clusters_world:
        d = Vector(direction).normalized()
        hit, nrm, _i, _dist = bvh.ray_cast(d * scale * 3.0, -d)
        if hit is None:
            continue
        anchors.append(hit.copy())
        t1 = nrm.orthogonal().normalized()
        t2 = nrm.cross(t1).normalized()
        H = frac * scale
        cluster_pts = []
        for k in range(count):
            if k == 0:
                p, n, h = hit, nrm, H
                dirc = (nrm * 0.85 + d * 0.15 + Vector((rng.uniform(-0.12, 0.12), rng.uniform(-0.12, 0.12),
                                                         rng.uniform(-0.12, 0.12)))).normalized()
            else:
                a = rng.uniform(0, math.tau)
                rr = H * rng.uniform(0.16, 0.32)
                q = hit + (t1 * math.cos(a) + t2 * math.sin(a)) * rr + nrm * H * 0.4
                h2 = bvh.ray_cast(q, -nrm)
                if h2[0] is None:
                    h2 = bvh.find_nearest(q)
                p, n = h2[0], h2[1]
                h = H * rng.uniform(0.35, 0.72)
                outward = (p - hit)
                outward = outward.normalized() if outward.length > 1e-6 else t1
                dirc = (n * 0.45 + nrm * 0.35 + outward * rng.uniform(0.35, 0.7)).normalized()
            radius = h * rng.uniform(0.17, 0.25)
            base = p - dirc * (h * 0.3)
            before = set(bm.verts)
            L.rocks["_crystal"](bm, base, dirc, radius, h, 6, rng.uniform(1.0, 1.5), rng)
            for v in bm.verts:
                if v not in before:
                    t = max(0.0, min(1.0, (v.co - base).dot(dirc) / (h * 1.25)))
                    v[lay] = 1.0 - 0.75 * t
                    cluster_pts.append(v.co.copy())
        hulls.append(cluster_pts)
    bmesh.ops.triangulate(bm, faces=list(bm.faces))
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    return bm, anchors, hulls


# --------------------------------------------------------------------------- materials (bake source)

def ice_material(name, S, glow_hex):
    p = {k: L.H(v) for k, v in PAL["ice"].items()}
    s = L.Shader(name)
    o = s.obj
    broad = s.noise(o, 0.9 / S, 3.0, 0.55, offset=(3, 1, 7))
    # frost (snowy white / pale cyan) everywhere ...
    col = s.ramp(broad, [(0.3, p["white"]), (0.55, p["pale"]), (0.75, L.H("#B5DDF0"))])
    band = s.ramp(s.noise(o, 0.6 / S, 2.0, 0.5, offset=(8, 2, 1), stretch=(0.35, 0.35, 4.0)), [(0.48, 0.0), (0.58, 1.0)])
    col = s.mix(s.m("MULTIPLY", band, 0.4), col, p["pale"])
    # ... with exposed glacial blue ice on the fracture facets and in broad wind-scoured patches
    facet = s.attr("facet")
    patch = s.ramp(s.noise(o, 0.55 / S, 3.0, 0.6, offset=(-3, 4, 9)), [(0.58, 0.0), (0.66, 1.0)])
    glacial = s.maprange(s.m("MAXIMUM", s.m("MULTIPLY", facet, 0.85), s.m("MULTIPLY", patch, 0.7)), 0.0, 1.0, 0.0, 1.0)
    ice_col = s.ramp(s.noise(o, 2.0 / S, 3.0, 0.55, offset=(1, 1, 1)),
                     [(0.3, L.H("#5FB4E6")), (0.55, L.H("#8CCBEE")), (0.8, L.H("#C2E6F6"))])
    col = s.mix(glacial, col, ice_col)
    crack = s.attr("crack")
    curv = s.attr("curv")
    crease = s.maprange(s.m("MULTIPLY", curv, -1.0), 0.004, 0.05, 0.0, 1.0)
    col = s.mix(s.m("MULTIPLY", crease, 0.65), col, p["blue"])
    col = s.mix(s.maprange(crack, 0.05, 0.45, 0.0, 1.0), col, p["blue"])
    col = s.mix(s.maprange(crack, 0.45, 0.9, 0.0, 1.0), col, p["deep"])
    ridge = s.maprange(curv, 0.02, 0.09, 0.0, 1.0)
    col = s.mix(s.m("MULTIPLY", ridge, 0.6), col, p["white"])
    # frost sparkle: sparse bright cells, very glossy
    cellc = s.voronoi(o, 2.6, "Color")
    sp = s.m("GREATER_THAN", s.m("MULTIPLY", s.sep(cellc)[0], 1.0), 0.93)
    sp = s.m("MULTIPLY", sp, s.m("SUBTRACT", 1.0, s.maprange(crack, 0.1, 0.4, 0.0, 1.0)))
    col = s.mix(s.m("MULTIPLY", sp, 0.8), col, (1.0, 1.0, 1.0))
    rough = s.maprange(s.noise(o, 1.4 / S, 2.0, 0.5, offset=(2, 9, 4)), 0.3, 0.7, 0.48, 0.68)
    rough = s.mix(glacial, rough, (0.14, 0.14, 0.14))
    rough = s.mix(sp, rough, (0.04, 0.04, 0.04))
    em = s.mix(s.maprange(crack, 0.72, 0.98, 0.0, 1.0), (0.0, 0.0, 0.0), tuple(c * 0.5 for c in L.H(glow_hex)))
    h = s.m("ADD", s.m("MULTIPLY", s.noise(o, 1.8, 4.0, 0.6, offset=(5, 5, 1)), 0.6),
            s.m("MULTIPLY", s.voronoi(o, 2.6, "Distance"), 0.35))
    return s.finish(col, rough, 0.0, height=h, bump_strength=0.28, bump_distance=0.05,
                    emission=em, emission_strength=L.EMISSION_STRENGTH)


def metal_material(name, S):
    p = {k: L.H(v) for k, v in PAL["metal"].items()}
    s = L.Shader(name)
    o = s.obj
    col = s.ramp(s.noise(o, 1.1 / S, 3.0, 0.55, offset=(1, 2, 3)), [(0.32, p["dark"]), (0.5, p["mid"]), (0.7, p["light"])])
    curv = s.attr("curv")
    crease = s.maprange(s.m("MULTIPLY", curv, -1.0), 0.003, 0.04, 0.0, 1.0)
    ridge = s.maprange(curv, 0.015, 0.07, 0.0, 1.0)
    # rust: broad patches, strongest in creases and dent floors
    patch = s.ramp(s.noise(o, 0.75 / S, 3.0, 0.6, offset=(7, 3, 9)), [(0.6, 0.0), (0.7, 1.0)])
    speck = s.ramp(s.noise(o, 6.0 / S, 3.0, 0.7, offset=(2, 8, 2)), [(0.55, 0.0), (0.65, 1.0)])
    rust_m = s.m("MINIMUM", s.m("ADD", s.m("MULTIPLY", patch, 0.75),
                                s.m("MULTIPLY", crease, s.m("ADD", 0.25, s.m("MULTIPLY", speck, 0.6)))), 1.0)
    rust_c = s.ramp(s.noise(o, 3.0 / S, 4.0, 0.6, offset=(4, 4, 4)), [(0.3, p["rust1"]), (0.55, p["rust2"]), (0.75, p["rust3"])])
    col = s.mix(rust_m, col, rust_c)
    col = s.mix(s.m("MULTIPLY", ridge, 0.7), col, tuple(min(1.0, c * 2.0) for c in p["light"]))
    vein = s.attr("vein")
    halo = s.maprange(vein, 0.02, 0.25, 0.0, 1.0)
    core = s.maprange(vein, 0.3, 0.55, 0.0, 1.0)
    col = s.mix(s.m("MULTIPLY", halo, 0.7), col, p["halo"])
    gold = s.mix(s.noise(o, 4.0 / S, 2.0, 0.5), p["gold"], tuple(c * 0.75 for c in p["gold"]))
    col = s.mix(core, col, gold)
    metal = s.mix(rust_m, (0.6, 0.6, 0.6), (0.05, 0.05, 0.05))
    metal = s.mix(core, metal, (1.0, 1.0, 1.0))
    rough = s.maprange(s.noise(o, 2.0 / S, 2.0, 0.5, offset=(9, 1, 1)), 0.3, 0.7, 0.36, 0.56)
    rough = s.mix(s.m("MULTIPLY", ridge, 0.8), rough, (0.24, 0.24, 0.24))
    rough = s.mix(rust_m, rough, (0.88, 0.88, 0.88))
    rough = s.mix(core, rough, (0.2, 0.2, 0.2))
    h = s.m("ADD", s.m("MULTIPLY", s.noise(o, 2.2, 4.0, 0.6, offset=(2, 6, 3)), 0.5),
            s.m("MULTIPLY", s.m("MULTIPLY", rust_m, s.noise(o, 6.0, 3.0, 0.7)), 0.4))
    return s.finish(col, rough, metal, height=h, bump_strength=0.3, bump_distance=0.04)


def crystal_rock_material(name, S, glow_hex, anchors, glow_radius):
    p = {k: L.H(v) for k, v in PAL["crystal"].items()}
    s = L.Shader(name)
    o = s.obj
    col = s.ramp(s.noise(o, 1.0 / S, 3.0, 0.55, offset=(2, 7, 1)), [(0.36, p["dark"]), (0.5, p["mid"]), (0.64, p["light"])])
    strata = s.ramp(s.noise(o, 0.7 / S, 2.0, 0.5, offset=(5, 1, 3), stretch=(0.5, 0.7, 5.0)), [(0.45, 0.0), (0.56, 1.0)])
    col = s.mix(s.m("MULTIPLY", strata, 0.35), col, p["dark"])
    patch = s.ramp(s.noise(o, 0.9 / S, 2.0, 0.5, offset=(-4, 9, 5)), [(0.62, 0.0), (0.66, 1.0)])
    col = s.mix(s.m("MULTIPLY", patch, 0.35), col, p["patch"])
    curv = s.attr("curv")
    col = s.mix(s.m("MULTIPLY", s.maprange(curv, 0.02, 0.1, 0.0, 1.0), 0.5), col, p["vein"])
    col = s.mix(s.m("MULTIPLY", s.maprange(s.m("MULTIPLY", curv, -1.0), 0.005, 0.05, 0.0, 1.0), 0.6), col, tuple(c * 0.6 for c in p["dark"]))
    # crystal-tinted dust near the clusters
    g = L.H(glow_hex)
    total = None
    for a in anchors:
        dist = s.vm("DISTANCE", o, tuple(a))
        f = s.m("POWER", s.maprange(dist, 0.0, glow_radius, 1.0, 0.0), 2.0)
        total = f if total is None else s.m("ADD", total, f, clamp=True)
    col = s.mix(s.m("MULTIPLY", total, 0.45), col, tuple(c * 0.55 for c in g))
    fis = s.voronoi(s.mapping(o, scale=(1.0, 1.0, 1.0)), 0.45 / S, "Distance", feature="DISTANCE_TO_EDGE")
    fis = s.m("MULTIPLY", s.maprange(fis, 0.025, 0.004, 0.0, 1.0), s.maprange(total, 0.15, 0.5, 0.0, 1.0))
    col = s.mix(fis, col, tuple(c * 0.8 for c in g))
    em = s.mix(s.m("ADD", s.m("MULTIPLY", total, 0.06), s.m("MULTIPLY", fis, 0.7)), (0.0, 0.0, 0.0), g)
    rough = s.maprange(s.noise(o, 2.0 / S, 2.0, 0.5, offset=(3, 3, 3)), 0.3, 0.7, 0.74, 0.92)
    h = s.m("ADD", s.m("MULTIPLY", s.noise(o, 1.6, 4.0, 0.55, offset=(1, 2, 3)), 0.5),
            s.m("MULTIPLY", s.noise(o, 6.0, 3.0, 0.5), 0.2))
    return s.finish(col, rough, 0.0, height=h, bump_strength=0.32, bump_distance=0.05,
                    emission=em, emission_strength=L.EMISSION_STRENGTH)


def crystal_material(name, c):
    s = L.Shader(name)
    o = s.obj
    glow = s.attr("cglow")
    base, light = L.H(c["base"]), L.H(c["light"])
    col = s.mix(s.maprange(glow, 0.3, 0.95, 1.0, 0.0, smooth=True), base, light)
    col = s.mix(s.m("MULTIPLY", s.noise(o, 0.35, 2.0, 0.5), 0.45), col, tuple(x * 0.55 for x in base))
    core = s.maprange(glow, 0.86, 0.98, 0.0, 1.0)
    em = s.mix(s.m("ADD", s.maprange(glow, 0.25, 1.0, 0.0, 0.07), s.m("MULTIPLY", core, 0.6)),
               (0.0, 0.0, 0.0), L.H(c["glow"]))
    rough = s.maprange(s.noise(o, 0.8, 2.0, 0.5), 0.3, 0.7, 0.06, 0.18)
    h = s.m("MULTIPLY", s.noise(o, 3.0, 3.0, 0.5), 0.15)
    return s.finish(col, rough, 0.0, height=h, bump_strength=0.12, bump_distance=0.02,
                    emission=em, emission_strength=L.EMISSION_STRENGTH)


# --------------------------------------------------------------------------- build

def build(asset, tag="r1"):
    import os
    import time
    t0 = time.time()
    spec = dict(SPECS[asset])
    short = asset[3:]
    fam = spec["family"]
    asset_dir = os.path.join(L.KIT, asset)
    os.makedirs(asset_dir, exist_ok=True)
    coll = L.ensure_collection("SpaceDressing")
    src_coll = L.ensure_collection("SpaceDressing_BakeSource")
    shape = Shape(spec)
    low = build_shape_mesh(shape, 6, False, asset, coll)
    high = build_shape_mesh(shape, 8, True, "SRC_{}_High".format(short), src_coll)
    # size: the rock body is scaled so the finished asset's longest side is spec["size"]
    pts = L.mesh_points(low.data)
    ext = float((pts.max(axis=0) - pts.min(axis=0)).max())
    body_size = spec["size"] * spec.get("body", 1.0)
    k = body_size / ext
    for o in (low, high):
        o.data.transform(Matrix.Diagonal((k, k, k, 1.0)))
        o.data.update()
    decimate(low, spec["tris"])
    dev = deviation(low, [high])
    rng = random.Random(spec["seed"] * 7 + 1)
    sources = [high]
    hulls = [[v.co.copy() for v in low.data.vertices]]
    anchors = []
    if fam == "crystal":
        bml = bmesh.new()
        bml.from_mesh(low.data)
        bvh = BVHTree.FromBMesh(bml)
        cbm, anchors, chulls = crystal_bm(spec["crystal"]["clusters"], bvh, rng, body_size)
        bml.free()
        hulls.extend(chulls)
        cobj = L.bm_to_object("SRC_{}_Crystals".format(short), cbm, src_coll)
        cobj.data.shade_flat()
        sources.append(cobj)
        # game mesh = rock + an identical copy of the crystals (separate, embedded islands)
        bm = bmesh.new()
        bm.from_mesh(low.data)
        bm.from_mesh(cobj.data)
        bm.to_mesh(low.data)
        bm.free()
        low.data.update()
        # the size spec covers the whole asset: rescale everything so the longest side matches
        pts = L.mesh_points(low.data)
        k2 = spec["size"] / float((pts.max(axis=0) - pts.min(axis=0)).max())
        for o in (low, high, cobj):
            o.data.transform(Matrix.Diagonal((k2, k2, k2, 1.0)))
            o.data.update()
        anchors = [a * k2 for a in anchors]
        hulls = [[p * k2 for p in h] for h in hulls]
    # pivot: centre of mass of the game mesh
    com = L.center_of_mass(low.data)
    for o in [low] + sources:
        o.data.transform(Matrix.Translation(-com))
        o.data.update()
    anchors = [a - com for a in anchors]
    hulls = [[p - com for p in h] for h in hulls]
    # shading, UVs
    low.data.shade_smooth()
    low.data.set_sharp_from_angle(angle=math.radians(38.0))
    L.smart_uv(low, angle=60.0, margin=0.003)
    islands = split_uv_at_sharp(low.data)
    margin = 0.0035 if spec["tex"] >= 2048 else 0.006
    L.pack_uv(low, margin)
    fixed = L.fix_uv_overlaps(low, margin)
    L.hs["add_weighted_normal"](low, keep_sharp=True)
    L.hs["apply_modifiers"](low)
    low.data.name = asset
    high.data.shade_smooth()
    # materials
    S = spec["size"] / 10.0
    log_curv = store_curvature(high.data)
    if fam == "ice":
        hm = ice_material("M_{}_SrcIce".format(short), S, spec["glow"])
    elif fam == "metal":
        hm = metal_material("M_{}_SrcMetal".format(short), S)
    else:
        hm = crystal_rock_material("M_{}_SrcRock".format(short), S, spec["crystal"]["glow"], anchors,
                                   glow_radius=spec["size"] * 0.16)
        sources[1].data.materials.append(crystal_material("M_{}_SrcCrystal".format(short), spec["crystal"]))
    high.data.materials.append(hm)
    low.data.materials.clear()
    low.data.materials.append(bpy.data.materials.new("M_{}_BakeTarget".format(short)))
    cage = max(dev["max_m"], dev["game_to_source_max_m"]) * 1.15 + 0.004 * S
    info = {"family": fam, "deviation": dev, "cage_m": cage, "uv_islands": islands, "uv_faces_detached": fixed,
            "uv": L.uv_stats(low.data, 1024), "curvature": log_curv, "source_tris": L.tri_count(high.data)}
    info["bake"] = L.bake(low, asset_dir, spec["tex"], sources=[o.name for o in sources], cage=cage,
                          ao_distance=spec["size"] * 0.12, samples=32, ao_samples=48)
    info["bake_misses"] = L.fill_bake_misses(short)
    tint = {"ice": (0.62, 0.78, 1.0), "metal": (0.55, 0.5, 0.48), "crystal": (0.62, 0.55, 0.85)}[fam]
    info["cavity"] = L.cavity_tint(short, tint=tint, strength=0.6)
    low.data.materials[0].name = "M_{}".format(short)
    info["build_seconds"] = round(time.time() - t0, 1)
    budget = (3000, 15000)
    return L.finish_asset(asset, low, info, spec["tex"], tag, budget, hulls, sources=sources,
                          extra_manifest={"family": fam, "kit": "SpaceDressing"})

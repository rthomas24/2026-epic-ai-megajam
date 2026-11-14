"""#53 hero crystal cluster r2 - Route C procedural geometry (Blender 4.5.3, bmesh).

A dense cluster of 14 sharp hexagonal cyan crystals of varied height, tilt and girth radiating from a
basalt nest, 5 amber crystals packed in at the base, and an amber-to-cyan glow ramp up each cyan shaft.
The shard generator is the procedural_rocks.py `_crystal` idea (hex prism + pyramid tip) re-implemented
so every facet is planar, with an off-axis apex per shard (still planar triangles) and two extra
coplanar ring stations that carry the amber->cyan colour ramp. Rocks are convex hulls of a few seeded
points (planar triangle facets). Pivot = base centre, min Z = 0 (sits on SOCKET_Crystal).
"""
from __future__ import annotations
import math, random
import bpy, bmesh
from mathutils import Vector

# kind, base(x,y,z), lean_deg, azimuth_deg (0 = +X), length, radius, flatten, rot0_deg, embed
CYAN = [
    ((0.00,  0.00, 0.12),  3.0,  30.0, 1.185, 0.130, 0.95,  8.0, 0.10),   # hero spire
    ((0.07,  0.15, 0.10), 13.0,  70.0, 0.860, 0.102, 0.90, 21.0, 0.10),
    ((0.05, -0.16, 0.10), 12.0, 290.0, 0.920, 0.104, 0.92,  2.0, 0.10),
    ((-0.10, 0.05, 0.10), 15.0, 165.0, 0.780, 0.094, 1.00, 14.0, 0.10),
    ((0.12,  0.00, 0.07), 19.0,   5.0, 0.760, 0.086, 0.88, 27.0, 0.11),
    ((-0.07,-0.11, 0.07), 18.0, 215.0, 0.820, 0.088, 0.94, 33.0, 0.11),
    ((0.08,  0.30, 0.06), 27.0,  78.0, 0.700, 0.080, 0.90, 11.0, 0.12),
    ((0.02, -0.32, 0.06), 28.0, 272.0, 0.740, 0.082, 0.86,  5.0, 0.12),
    ((-0.12, 0.27, 0.05), 30.0, 128.0, 0.600, 0.070, 0.95, 40.0, 0.12),
    ((-0.10,-0.29, 0.05), 31.0, 238.0, 0.580, 0.068, 0.92, 17.0, 0.12),
    ((0.17, -0.15, 0.05), 35.0, 328.0, 0.500, 0.062, 0.90, 23.0, 0.13),
    ((0.16,  0.19, 0.05), 38.0,  42.0, 0.460, 0.058, 0.95,  9.0, 0.13),
    ((-0.18,-0.01, 0.05), 36.0, 182.0, 0.480, 0.062, 0.90, 29.0, 0.13),
    ((0.02,  0.44, 0.04), 47.0,  92.0, 0.400, 0.054, 0.92, 15.0, 0.14),
]
AMBER = [
    ((0.13,  0.08, 0.06), 34.0,  25.0, 0.320, 0.050, 0.90, 12.0, 0.16),
    ((-0.04, 0.17, 0.06), 30.0, 115.0, 0.280, 0.046, 0.95, 31.0, 0.16),
    ((0.10, -0.24, 0.05), 40.0, 300.0, 0.300, 0.046, 0.90,  3.0, 0.16),
    ((-0.15,-0.17, 0.05), 38.0, 205.0, 0.260, 0.044, 0.92, 22.0, 0.16),
    ((-0.13, 0.15, 0.05), 44.0, 150.0, 0.240, 0.040, 0.95, 37.0, 0.16),
]
# rock nest: centre(x,y,z), radii(x,y,z), seed, n_points
ROCKS = [
    ((0.00,  0.00, 0.09), (0.26, 0.32, 0.15), 1, 12),
    ((0.20,  0.08, 0.06), (0.14, 0.15, 0.11), 2, 9),
    ((-0.20, 0.10, 0.06), (0.14, 0.14, 0.11), 3, 9),
    ((0.16, -0.20, 0.06), (0.14, 0.13, 0.10), 4, 9),
    ((-0.16,-0.22, 0.06), (0.13, 0.14, 0.10), 5, 9),
    ((0.02,  0.34, 0.05), (0.13, 0.12, 0.07), 6, 9),
    ((0.00, -0.38, 0.05), (0.12, 0.12, 0.07), 7, 9),
    ((0.26, -0.04, 0.04), (0.09, 0.10, 0.06), 8, 8),
    ((-0.27,-0.03, 0.04), (0.09, 0.11, 0.06), 9, 8),
    ((0.17,  0.31, 0.035), (0.08, 0.08, 0.05), 10, 8),
    ((-0.16, 0.33, 0.035), (0.08, 0.09, 0.05), 11, 8),
    ((0.15, -0.40, 0.035), (0.08, 0.08, 0.05), 12, 8),
    ((-0.15,-0.41, 0.035), (0.08, 0.08, 0.05), 13, 8),
    ((0.05,  0.52, 0.03), (0.07, 0.07, 0.045), 14, 8),
    ((-0.04,-0.53, 0.03), (0.07, 0.07, 0.045), 15, 8),
    ((0.30,  0.20, 0.03), (0.06, 0.07, 0.04), 16, 7),
    ((-0.30, 0.20, 0.03), (0.06, 0.06, 0.04), 17, 7),
    ((0.30, -0.26, 0.03), (0.06, 0.06, 0.04), 18, 7),
    ((-0.31,-0.25, 0.03), (0.06, 0.06, 0.04), 19, 7),
]

CYAN_DEEP, CYAN_BC, CYAN_GLOW = Vector((0.02, 0.17, 0.27)), Vector((0.09, 0.44, 0.58)), Vector((0.12, 0.80, 1.00))
CYAN_TIP = Vector((0.48, 0.80, 0.90))
AMBER_DEEP, AMBER_BC, AMBER_GLOW = Vector((0.36, 0.11, 0.02)), Vector((0.80, 0.42, 0.07)), Vector((1.00, 0.55, 0.10))
ROCK_BC, ROCK_ASH = Vector((0.085, 0.082, 0.085)), Vector((0.20, 0.19, 0.18))


def _frame(d):
    d = Vector(d).normalized()
    helper = Vector((0, 0, 1)) if abs(d.z) < 0.92 else Vector((1, 0, 0))
    side = d.cross(helper).normalized()
    return d, side, d.cross(side).normalized()


def add_shard(bm, base, lean, az, length, radius, flatten, rot0, embed, rng, sides=6):
    """Hex prism + off-axis pyramid tip. Returns (faces, base, axis, length)."""
    la, aa = math.radians(lean), math.radians(az)
    d, side, bit = _frame((math.sin(la) * math.cos(aa), math.sin(la) * math.sin(aa), math.cos(la)))
    base = Vector(base)
    r0 = math.radians(rot0)
    # tip: shaft-to-tip crease kept >= ~34 deg on every face so it reads (and counts) as a hard edge
    tip_len = min(rng.uniform(1.15, 1.45) * radius, length * 0.35)
    off = (side * rng.uniform(-0.22, 0.22) + bit * rng.uniform(-0.22, 0.22)) * radius
    # bury the root in the nest, but never below the nest's flat underside (z = 0.01 m at the lowest ring corner)
    z0 = max(-embed * length, (0.01 - base.z + radius * math.sin(la)) / max(d.z, 0.2))
    z_tip = length - tip_len
    stations = [z0, z0 + (z_tip - z0) * 0.14, z0 + (z_tip - z0) * 0.40, z_tip]

    def ring(z):
        out = []
        for k in range(sides):
            a = r0 + math.tau * k / sides
            out.append(bm.verts.new(base + d * z + side * (radius * math.cos(a)) + bit * (radius * math.sin(a) * flatten)))
        return out
    rings = [ring(z) for z in stations]
    faces = [bm.faces.new(list(reversed(rings[0])))]
    for a, b in zip(rings, rings[1:]):
        for i in range(sides):
            j = (i + 1) % sides
            faces.append(bm.faces.new((a[i], a[j], b[j], b[i])))
    apex = bm.verts.new(base + d * length + off)
    for i in range(sides):
        j = (i + 1) % sides
        faces.append(bm.faces.new((rings[-1][i], rings[-1][j], apex)))
    return faces, base, d, length, z0


def add_rock(bm, center, radii, seed, n):
    """Faceted basalt chunk: a yawed box with n-4 random planar corner cuts (every facet planar, cuts
    kept >= 30 deg off the box faces so the chunk reads with hard edges). Flat underside at z = 0."""
    rng = random.Random(1000 + seed)
    t = bmesh.new()
    bmesh.ops.create_cube(t, size=1.0)
    yaw = rng.uniform(0, math.pi)
    for v in t.verts:
        v.co = Vector((v.co.x * 2 * radii[0], v.co.y * 2 * radii[1], (v.co.z + 0.5) * (center[2] + radii[2])))
    cz = (center[2] + radii[2]) * 0.5
    cuts = 0
    while cuts < max(3, n - 4):
        dv = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-0.15, 1)))
        if dv.length < 1e-3:
            continue
        dv.normalize()
        if max(abs(dv.x), abs(dv.y), abs(dv.z)) > 0.84 or min(abs(dv.x), abs(dv.y), abs(dv.z)) < 0.12:
            continue
        ext = max(v.co.dot(dv) for v in t.verts) - Vector((0, 0, cz)).dot(dv)
        co = Vector((0, 0, cz)) + dv * ext * rng.uniform(0.55, 0.80)
        geom = list(t.verts) + list(t.edges) + list(t.faces)
        res = bmesh.ops.bisect_plane(t, geom=geom, plane_co=co, plane_no=dv, clear_outer=True)
        cut_edges = [e for e in res["geom_cut"] if isinstance(e, bmesh.types.BMEdge)]
        if cut_edges:
            bmesh.ops.holes_fill(t, edges=cut_edges, sides=0)
        cuts += 1
    bmesh.ops.recalc_face_normals(t, faces=t.faces)
    c, s_ = math.cos(yaw), math.sin(yaw)
    vmap = {}
    for v in t.verts:
        x, y, z = v.co
        vmap[v] = bm.verts.new(Vector((center[0] + x * c - y * s_, center[1] + x * s_ + y * c, z)))
    faces = []
    for f in t.faces:
        try:
            faces.append(bm.faces.new([vmap[v] for v in f.verts]))
        except ValueError:
            pass
    t.free()
    return faces


def build_cluster(name="SM_CrystalCacheCrystal"):
    bm = bmesh.new()
    col = bm.loops.layers.float_color.new("Col")
    emi = bm.loops.layers.float_color.new("Emi")
    rmx = bm.loops.layers.float_color.new("RM")
    kind_l = bm.faces.layers.int.new("kind")
    part_l = bm.faces.layers.int.new("part")   # 1..14 = cyan shard index (per-shard UCX), 0 = nest
    rng = random.Random(5302)
    roots_cyan, roots_amber = [], []

    def paint_shard(faces, base, d, length, z0, amber):
        frng = random.Random(len(faces) * 7919 + int(length * 1000))
        for f in faces:
            f[kind_l] = 1 if amber else 0
            facet = 0.55 + 0.45 * frng.random()
            for lp in f.loops:
                h = max(0.0, min(1.0, ((lp.vert.co - base).dot(d) - z0) / max(length - z0, 1e-4)))
                if amber:
                    heat = (1.0 - min(1.0, h / 0.8)) ** 0.8
                    bc = AMBER_DEEP.lerp(AMBER_BC, 0.45 + 0.4 * facet)
                    em = AMBER_GLOW * (0.95 * (0.35 + 0.65 * heat))
                    rough = 0.14
                else:
                    warm = max(0.0, 1.0 - h / 0.30) ** 1.3          # amber at the root -> cyan by ~30%
                    tip = max(0.0, (h - 0.70) / 0.30)               # pale icy tip
                    bc = CYAN_DEEP.lerp(CYAN_BC, 0.15 + 0.85 * facet).lerp(CYAN_TIP, 0.40 * tip)
                    bc = bc.lerp(AMBER_BC, 0.80 * warm)
                    em = CYAN_GLOW * (0.13 * facet * (0.30 + 0.70 * (1.0 - h)))
                    em = em.lerp(AMBER_GLOW * 0.95, warm)
                    rough = 0.08 + 0.05 * (1.0 - facet)
                lp[col] = (bc.x, bc.y, bc.z, 1.0)
                lp[emi] = (em.x, em.y, em.z, 1.0)
                lp[rmx] = (rough, 0.02, 0.0, 1.0)

    for i, spec in enumerate(CYAN):
        faces, base, d, length, z0 = add_shard(bm, *spec, rng)
        paint_shard(faces, base, d, length, z0, False)
        for f in faces:
            f[part_l] = i + 1
        roots_cyan.append(base)
    for spec in AMBER:
        faces, base, d, length, z0 = add_shard(bm, *spec, rng)
        paint_shard(faces, base, d, length, z0, True)
        roots_amber.append(base)
    for center, radii, seed, n in ROCKS:
        faces = add_rock(bm, center, radii, seed, n)
        for f in faces:
            f[kind_l] = 2
            for lp in f.loops:
                v = lp.vert.co
                mix = 0.2 + 0.4 * (0.5 + 0.5 * math.sin(v.x * 21 + v.y * 17 + v.z * 9))
                bc = ROCK_BC.lerp(ROCK_ASH, mix)
                em = Vector((0, 0, 0))
                da = min((v - r).length for r in roots_amber)
                dc = min((v - r).length for r in roots_cyan)
                if da < 0.20:
                    w = (1 - da / 0.20) ** 2
                    em = em + AMBER_GLOW * 0.55 * w
                    bc = bc.lerp(AMBER_BC, 0.30 * w)
                if dc < 0.14:
                    w = (1 - dc / 0.14) ** 2
                    em = em + AMBER_GLOW * 0.30 * w
                lp[col] = (bc.x, bc.y, bc.z, 1.0)
                lp[emi] = (em.x, em.y, em.z, 1.0)
                lp[rmx] = (0.72 + 0.12 * mix, 0.06, 0.0, 1.0)

    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    obj = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(obj)
    me.shade_flat()
    # pivot: XY centre of the nest footprint (= origin), min Z = 0
    zmin = min(v.co.z for v in me.vertices)
    for v in me.vertices:
        v.co.z -= zmin
    me.update()
    return obj, len(CYAN), len(AMBER), len(ROCKS)


def vertex_color_material(obj, name="M_CrystalCacheCrystal_vc", emission=2.2):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    a = nt.nodes.new("ShaderNodeVertexColor"); a.layer_name = "Col"
    e = nt.nodes.new("ShaderNodeVertexColor"); e.layer_name = "Emi"
    r = nt.nodes.new("ShaderNodeVertexColor"); r.layer_name = "RM"
    s = nt.nodes.new("ShaderNodeSeparateColor")
    nt.links.new(a.outputs["Color"], bsdf.inputs["Base Color"])
    nt.links.new(e.outputs["Color"], bsdf.inputs["Emission Color"])
    nt.links.new(r.outputs["Color"], s.inputs["Color"])
    nt.links.new(s.outputs["Red"], bsdf.inputs["Roughness"])
    nt.links.new(s.outputs["Green"], bsdf.inputs["Metallic"])
    bsdf.inputs["Emission Strength"].default_value = emission
    nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    return mat

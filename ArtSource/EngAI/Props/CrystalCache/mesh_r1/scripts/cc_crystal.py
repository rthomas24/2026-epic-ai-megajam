
from __future__ import annotations
import math, random
import bpy, bmesh
from mathutils import Matrix, Vector
from cc_paths import CLUSTER_HGT
from cc_utils import log, world_bbox, apply_world_transform, mesh_report_dict

def _frame(direction):
    d = Vector(direction).normalized()
    helper = Vector((0, 0, 1)) if abs(d.z) < 0.92 else Vector((1, 0, 0))
    side = d.cross(helper).normalized()
    bitan = d.cross(side).normalized()
    return d, side, bitan

def add_crystal(bm, base, direction, length, radius, sides=6, tip=0.20, taper=0.78,
                embed=0.08, rot0=0.0, flatten=1.0, step=None):
    d, side, bitan = _frame(direction)
    base = Vector(base)
    def ring(scale, z):
        vs = []
        for k in range(sides):
            a = rot0 + (math.tau * k / sides)
            x = radius * scale * math.cos(a)
            y = radius * scale * math.sin(a) * flatten
            vs.append(bm.verts.new(base + d * z + side * x + bitan * y))
        return vs
    def bridge(a, b):
        faces = []
        for i in range(sides):
            j = (i + 1) % sides
            faces.append(bm.faces.new((a[i], a[j], b[j], b[i])))
        return faces
    z0 = -embed * length
    # Straight hex prism (constant radius, including the buried root) plus a
    # pyramid tip. The shaft-to-tip crease is forced past 32° so it counts as
    # a hard edge, not the 1-25° wobble band. A long 0.76 taper into a
    # shallower pyramid was exactly 25% wobble (the six tip-shaft edges).
    max_tip = (radius) / math.tan(math.radians(32.0))
    tip_len = min(max(tip * length, 0.01), max_tip * 0.95)
    z_tip = max(z0 + 0.02, length - tip_len)
    stations = [(z0, 1.00)]
    if step:
        t, sc = step
        z_ledge = min(length * t, z_tip - 0.01)
        stations.append((z_ledge, 1.00))
        stations.append((z_ledge + 1e-4, sc))
        stations.append((z_tip, sc))
    else:
        stations.append((z_tip, 1.00))
    rings = [ring(sc, z) for z, sc in stations]
    faces = [bm.faces.new(list(reversed(rings[0])))]
    for a, b in zip(rings, rings[1:]):
        faces.extend(bridge(a, b))
    pointed = tip > 0 and (length - stations[-1][0]) > 1e-5
    if pointed:
        tip_v = bm.verts.new(base + d * length)
        for i in range(sides):
            j = (i + 1) % sides
            faces.append(bm.faces.new((rings[-1][i], rings[-1][j], tip_v)))
    else:
        # flat planar cap (used for basalt stubs — a short fat pyramid
        # made adjacent tip faces fall in the 1-25° wobble band)
        try:
            faces.append(bm.faces.new(list(rings[-1])))
        except ValueError:
            faces.append(bm.faces.new(list(reversed(rings[-1]))))
    return faces, (base, d, length)

def add_rock_chunk(bm, center, size, seed, npts=8):
    """Planar-faced basalt chunk: a rotated/tapered box (all faces stay planar)."""
    rng = random.Random(seed)
    yaw = math.radians(rng.uniform(-25, 25))
    tilt = math.radians(rng.uniform(-8, 8))
    taper = rng.uniform(0.72, 0.92)
    sx, sy, sz = size.x, size.y, size.z
    # 4 bottom + 4 top, top uniformly scaled (taper) so sides stay planar
    c, s = math.cos(yaw), math.sin(yaw)
    ct, st = math.cos(tilt), math.sin(tilt)
    def xf(x, y, z):
        # tilt about X, then yaw about Z
        y, z = y*ct - z*st, y*st + z*ct
        x, y = x*c - y*s, x*s + y*c
        return center + Vector((x, y, z))
    zs = (-0.32*sz, 0.55*sz)
    ring = [(-0.5*sx, -0.5*sy), (0.5*sx, -0.5*sy), (0.5*sx, 0.5*sy), (-0.5*sx, 0.5*sy)]
    bot = [bm.verts.new(xf(x, y, zs[0])) for x, y in ring]
    top = [bm.verts.new(xf(x*taper, y*taper, zs[1])) for x, y in ring]
    bm.faces.new(bot)
    bm.faces.new(list(reversed(top)))
    for i in range(4):
        j = (i+1) % 4
        bm.faces.new((bot[i], bot[j], top[j], top[i]))
    return bot + top

def build_crystal_cluster():
    bm = bmesh.new()
    # kind, base_xyz, lean_deg, az_deg (0=+X front), length, radius, sides, rot0, flatten, step, embed
    shards = [
        ("cyan",  (0.00,  0.02, 0.10),  5.0,  25.0, 1.26, 0.108, 6, 8.0,  1.00, (0.55, 0.88), 0.10),
        ("cyan",  (0.03,  0.20, 0.08), 20.0,  95.0, 0.78, 0.088, 6, 14.0, 0.92, (0.50, 0.86), 0.10),
        ("cyan",  (0.04, -0.20, 0.08), 18.0, 260.0, 0.92, 0.092, 6, 4.0,  0.95, (0.48, 0.86), 0.10),
        ("cyan",  (0.16,  0.12, 0.06), 26.0,  40.0, 0.52, 0.068, 6, 20.0, 1.00, None, 0.12),
        ("cyan",  (0.14, -0.14, 0.06), 28.0, 310.0, 0.44, 0.058, 6, 10.0, 1.00, None, 0.12),
        ("cyan",  (-0.14, 0.08, 0.06), 16.0, 155.0, 0.46, 0.055, 6, 0.0,  1.00, None, 0.12),
        ("cyan",  (-0.06, 0.22, 0.05), 30.0, 110.0, 0.32, 0.042, 6, 22.0, 1.05, None, 0.14),
        ("cyan",  (0.02, -0.26, 0.05), 32.0, 250.0, 0.28, 0.038, 6, 6.0,  1.00, None, 0.14),
        ("amber", (0.07,  0.00, 0.07), 10.0,  10.0, 0.56, 0.070, 6, 16.0, 0.90, (0.45, 0.85), 0.12),
        ("amber", (0.05,  0.11, 0.06), 18.0,  70.0, 0.34, 0.046, 6, 0.0,  1.00, None, 0.14),
        ("amber", (0.10, -0.09, 0.05), 22.0, 300.0, 0.30, 0.042, 6, 12.0, 1.00, None, 0.14),
        ("amber", (-0.02,-0.04, 0.05),  8.0, 200.0, 0.22, 0.034, 6, 28.0, 1.00, None, 0.16),
    ]
    face_kinds = []
    for kind, base, lean, az, length, radius, sides, rot0, flatten, step, embed in shards:
        la, aa = math.radians(lean), math.radians(az)
        direction = Vector((math.sin(la) * math.cos(aa), math.sin(la) * math.sin(aa), math.cos(la)))
        faces, meta = add_crystal(bm, Vector(base), direction, length, radius,
                                  sides=sides, tip=0.20, taper=0.76, embed=embed,
                                  rot0=math.radians(rot0), flatten=flatten, step=step)
        face_kinds.extend((f, kind, meta) for f in faces)
    rocks = [
        (Vector((0.00,  0.00, 0.10)), Vector((0.62, 0.70, 0.28)), 11),
        (Vector((0.16,  0.10, 0.08)), Vector((0.36, 0.34, 0.20)), 21),
        (Vector((-0.14, 0.12, 0.07)), Vector((0.32, 0.30, 0.18)), 31),
        (Vector((0.10, -0.18, 0.07)), Vector((0.30, 0.32, 0.16)), 41),
        (Vector((-0.10,-0.14, 0.06)), Vector((0.28, 0.28, 0.15)), 51),
        (Vector((0.00,  0.22, 0.06)), Vector((0.24, 0.26, 0.14)), 61),
    ]
    for center, size, seed in rocks:
        add_rock_chunk(bm, center, size, seed)
    bm.faces.ensure_lookup_table()
    for f in bm.faces:
        if f not in [x[0] for x in face_kinds]:
            face_kinds.append((f, "rock", None))
    kind_layer = bm.faces.layers.int.new("kind")
    kind_code = {"cyan": 0, "amber": 1, "rock": 2}
    for f, kind, meta in face_kinds:
        if f.is_valid:
            f[kind_layer] = kind_code[kind]
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new("SM_CrystalCacheCrystal")
    bm.to_mesh(me)
    bm.free()
    obj = bpy.data.objects.new("SM_CrystalCacheCrystal", me)
    bpy.context.scene.collection.objects.link(obj)
    obj.data.shade_flat()
    mins, maxs, size = world_bbox(obj)
    obj.location.x -= (mins.x + maxs.x) * 0.5
    obj.location.y -= (mins.y + maxs.y) * 0.5
    obj.location.z -= mins.z
    apply_world_transform(obj)
    mins, maxs, size = world_bbox(obj)
    if abs(size.z - CLUSTER_HGT) > 0.01:
        s = CLUSTER_HGT / size.z
        obj.scale = (s, s, s)
        apply_world_transform(obj)
        mins, maxs, size = world_bbox(obj)
        obj.location.z -= mins.z
        apply_world_transform(obj)
    return obj

def paint_crystal_attributes(obj):
    me = obj.data
    kind_attr = me.attributes.get("kind")
    col_attr = me.color_attributes.new("Col", "FLOAT_COLOR", "CORNER")
    emi_attr = me.color_attributes.new("Emi", "FLOAT_COLOR", "CORNER")
    rm_attr = me.color_attributes.new("RM", "FLOAT_COLOR", "CORNER")
    kinds = [2] * len(me.polygons)
    if kind_attr and kind_attr.domain == "FACE":
        for i in range(len(me.polygons)):
            kinds[i] = int(kind_attr.data[i].value)
    zs = [v.co.z for v in me.vertices]
    zmin, zmax = min(zs), max(zs)
    rng = random.Random(53)
    CYAN_DEEP = Vector((0.02, 0.22, 0.32))
    CYAN_GLOW = Vector((0.12, 0.85, 1.00))
    CYAN_BC   = Vector((0.10, 0.45, 0.58))
    AMBER_DEEP = Vector((0.28, 0.08, 0.01))
    AMBER_GLOW = Vector((1.00, 0.48, 0.06))
    AMBER_BC   = Vector((0.62, 0.28, 0.05))
    ROCK_BC    = Vector((0.10, 0.10, 0.11))
    ROCK_ASH   = Vector((0.22, 0.21, 0.20))
    roots = []
    for p, k in zip(me.polygons, kinds):
        if k in (0, 1) and p.center.z < 0.35:
            roots.append((p.center.copy(), k))
    for p in me.polygons:
        k = kinds[p.index]
        facet = 0.7 + 0.3 * rng.random()
        tone = 0.85 + 0.25 * rng.random()
        for li in p.loop_indices:
            loop = me.loops[li]
            v = me.vertices[loop.vertex_index]
            h = (v.co.z - zmin) / max(1e-6, (zmax - zmin))
            if k == 0:
                heat = 1.0 - max(0.0, min(1.0, (h - 0.02) / 0.70))
                heat = heat * heat
                bc = CYAN_DEEP.lerp(CYAN_BC, 0.35 + 0.4 * facet) * tone
                bc = bc.lerp(CYAN_GLOW * 0.25, 0.15 * facet)
                em = CYAN_GLOW * (0.55 * facet * (0.15 + 0.85 * heat))
                rough, metal = 0.10 + 0.06 * (1.0 - facet), 0.02
            elif k == 1:
                heat = 1.0 - max(0.0, min(1.0, (h - 0.0) / 0.65))
                heat = heat ** 1.2
                bc = AMBER_DEEP.lerp(AMBER_BC, 0.45 + 0.3 * facet) * tone
                em = AMBER_GLOW * (0.85 * facet * (0.25 + 0.75 * heat))
                rough, metal = 0.12 + 0.05 * (1.0 - facet), 0.02
            else:
                mix = 0.15 + 0.35 * (0.5 + 0.5 * math.sin(v.co.x * 18.0 + v.co.y * 13.0))
                bc = ROCK_BC.lerp(ROCK_ASH, mix)
                em = Vector((0, 0, 0))
                if roots:
                    dmin_c = min((v.co - r).length for r, kk in roots if kk == 0) if any(kk == 0 for _, kk in roots) else 9
                    dmin_a = min((v.co - r).length for r, kk in roots if kk == 1) if any(kk == 1 for _, kk in roots) else 9
                    if dmin_c < 0.22 and v.co.z < 0.28:
                        w = (1.0 - dmin_c / 0.22) ** 2 * 0.22
                        em = em + CYAN_GLOW * w
                        bc = bc.lerp(CYAN_BC, w * 0.35)
                    if dmin_a < 0.20 and v.co.z < 0.28:
                        w = (1.0 - dmin_a / 0.20) ** 2 * 0.28
                        em = em + AMBER_GLOW * w
                        bc = bc.lerp(AMBER_BC, w * 0.40)
                rough, metal = 0.68 + 0.12 * mix, 0.08 + 0.06 * (1.0 - mix)
            col_attr.data[li].color = (bc.x, bc.y, bc.z, 1.0)
            emi_attr.data[li].color = (em.x, em.y, em.z, 1.0)
            rm_attr.data[li].color = (rough, metal, 0.0, 1.0)

def make_attr_material(name, obj):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    out.location = (600, 0)
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    bsdf.location = (250, 0)
    col = nt.nodes.new("ShaderNodeVertexColor"); col.layer_name = "Col"; col.location = (-200, 120)
    emi = nt.nodes.new("ShaderNodeVertexColor"); emi.layer_name = "Emi"; emi.location = (-200, -80)
    rm = nt.nodes.new("ShaderNodeVertexColor"); rm.layer_name = "RM"; rm.location = (-200, -240)
    sep = nt.nodes.new("ShaderNodeSeparateColor"); sep.location = (0, -240)
    nt.links.new(col.outputs["Color"], bsdf.inputs["Base Color"])
    nt.links.new(emi.outputs["Color"], bsdf.inputs["Emission Color"])
    nt.links.new(rm.outputs["Color"], sep.inputs["Color"])
    nt.links.new(sep.outputs["Red"], bsdf.inputs["Roughness"])
    nt.links.new(sep.outputs["Green"], bsdf.inputs["Metallic"])
    bsdf.inputs["Emission Strength"].default_value = 1.0
    bsdf.inputs["Specular IOR Level"].default_value = 0.5
    nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    return mat

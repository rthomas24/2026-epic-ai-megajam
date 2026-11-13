# Stage 30 (step E): pack the authored UVs, procedural materials, bake to one 2048 set (T_NullTrooper_BC/_N/_ORM/_E).
# blender --background --factory-startup NullTrooper_v09.blend --python s30_tex.py -- --out NullTrooper_v10.blend [--size 2048]
import bpy, bmesh, os, sys, math, runpy, json
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
SK = os.path.join(ROOT, "..", "..", "..", ".claude", "skills")
argv = sys.argv[sys.argv.index("--") + 1:]
OUT = argv[argv.index("--out") + 1]
SIZE = int(argv[argv.index("--size") + 1]) if "--size" in argv else 2048
ASSET = argv[argv.index("--asset") + 1] if "--asset" in argv else "NullTrooper"
hs = runpy.run_path(os.path.join(SK, "blender-game-asset", "scripts", "hardsurface.py"))
bk = runpy.run_path(os.path.join(SK, "blender-texturing", "scripts", "bake_texture_set.py"))
hexc = hs["hex_color"]
EXPORT = os.path.join(ROOT, "export") if ASSET == "NullTrooper" else os.path.join(ROOT, "export", ASSET)
ob = bpy.data.objects["SK_" + ASSET]
arm = bpy.data.objects["root"]
me = ob.data
B = lambda n: arm.data.bones[n].head_local.copy()

# ---------------------------------------------------------------- UVs: islands were authored per part; scale + pack
gi = {g.name: g.index for g in ob.vertex_groups}
hero_groups = {gi.get("head"): 1.45, gi.get("spine_05"): 1.3}
vhero = []
for v in me.vertices:
    f = 1.0
    if len(v.groups) == 1 and v.groups[0].group in hero_groups: f = hero_groups[v.groups[0].group]
    vhero.append(f)
suit_idx = [i for i, m in enumerate(me.materials) if m.name == "M_NT_Suit"]
for o in bpy.context.selected_objects: o.select_set(False)
ob.select_set(True); bpy.context.view_layer.objects.active = ob
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.select_all(action='SELECT')
bpy.ops.uv.average_islands_scale()
bpy.ops.object.mode_set(mode='OBJECT')
uvd = me.uv_layers["UVMap"].data
for poly in me.polygons:
    f = min(vhero[v] for v in poly.vertices)
    if poly.material_index in suit_idx: f = 0.8      # the undersuit is mostly hidden under plates
    if f != 1.0:
        for li in poly.loop_indices: uvd[li].uv = uvd[li].uv * f
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.select_all(action='SELECT')
try:
    bpy.ops.uv.pack_islands(rotate=True, margin=0.0016, shape_method='CONCAVE')
except TypeError:
    bpy.ops.uv.pack_islands(rotate=True, margin=0.0016)
bpy.ops.object.mode_set(mode='OBJECT')
bm = bmesh.new(); bm.from_mesh(me)
uv = bm.loops.layers.uv.active
area3 = uva = 0.0
by_mat = {}; hero3 = herouv = 0.0
for f in bm.faces:
    a = f.calc_area(); area3 += a
    by_mat[f.material_index] = by_mat.get(f.material_index, 0.0) + a
    l = f.loops; fa = 0.0
    for k in range(1, len(l) - 1):
        p0, p1, p2 = l[0][uv].uv, l[k][uv].uv, l[k + 1][uv].uv
        fa += abs((p1.x - p0.x) * (p2.y - p0.y) - (p2.x - p0.x) * (p1.y - p0.y)) / 2
    uva += fa
    if min(vhero[v.index] for v in f.verts) > 1.0: hero3 += a; herouv += fa
bm.free()
uv_per_m = math.sqrt(uva / area3)
names = [m.name for m in me.materials]
share = {names[k]: round(v / area3, 4) for k, v in by_mat.items()}
print("TEX uv coverage %.3f  surface %.3f m2  mean texel density %.0f px/m  helmet+chest %.0f px/m  area share %s" % (
    uva, area3, uv_per_m * SIZE, math.sqrt(herouv / max(hero3, 1e-9)) * SIZE, share))


# ---------------------------------------------------------------- materials
def nodes_of(name):
    m = bpy.data.materials[name]
    nt = m.node_tree
    for n in list(nt.nodes): nt.nodes.remove(n)
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    b = nt.nodes.new("ShaderNodeBsdfPrincipled")
    b.inputs["Emission Color"].default_value = (0, 0, 0, 1); b.inputs["Emission Strength"].default_value = 0.0
    nt.links.new(b.outputs[0], out.inputs[0])
    return nt, b


def N(nt, kind, **props):
    n = nt.nodes.new(kind)
    for k, v in props.items():
        if k.startswith("in_"):
            n.inputs[k[3:].replace("_", " ")].default_value = v
        else:
            setattr(n, k, v)
    return n


def ramp(nt, src, p0, p1, c0=(0, 0, 0, 1), c1=(1, 1, 1, 1)):
    r = nt.nodes.new("ShaderNodeValToRGB")
    r.color_ramp.elements[0].position = p0; r.color_ramp.elements[0].color = c0
    r.color_ramp.elements[1].position = p1; r.color_ramp.elements[1].color = c1
    nt.links.new(src, r.inputs[0])
    return r.outputs[0]


def math_(nt, op, a, b=None, clamp=False):
    n = nt.nodes.new("ShaderNodeMath"); n.operation = op; n.use_clamp = clamp
    for i, v in enumerate((a, b)):
        if v is None: continue
        if isinstance(v, (int, float)): n.inputs[i].default_value = v
        else: nt.links.new(v, n.inputs[i])
    return n.outputs[0]


def mixc(nt, fac, a, b):
    n = nt.nodes.new("ShaderNodeMix"); n.data_type = 'RGBA'
    for key, v in (("Factor", fac), ("A", a), ("B", b)):
        sock = [s for s in n.inputs if s.name == key and (key == "Factor" or s.type == 'RGBA')][0]
        if hasattr(v, "is_linked"): nt.links.new(v, sock)
        else: sock.default_value = v
    return [s for s in n.outputs if s.type == 'RGBA'][0]


def armor(name, c_lo, c_hi, c_wear, metal, r_base, wear_amt=1.0):
    """Calm charcoal plate: broad tonal drift, worn edges (curvature), a few scribed panel lines. No speckle."""
    nt, b = nodes_of(name)
    tc = N(nt, "ShaderNodeTexCoord")
    big = N(nt, "ShaderNodeTexNoise", in_Scale=4.0, in_Detail=2.0)
    nt.links.new(tc.outputs["Object"], big.inputs["Vector"])
    base = mixc(nt, ramp(nt, big.outputs["Fac"], 0.35, 0.7), c_lo, c_hi)
    geo = N(nt, "ShaderNodeNewGeometry")
    edge = ramp(nt, geo.outputs["Pointiness"], 0.506, 0.545)
    brk = N(nt, "ShaderNodeTexNoise", in_Scale=24.0, in_Detail=3.0)
    nt.links.new(tc.outputs["Object"], brk.inputs["Vector"])
    wear = math_(nt, 'MULTIPLY', math_(nt, 'MULTIPLY', edge, ramp(nt, brk.outputs["Fac"], 0.15, 0.5)), wear_amt, clamp=True)
    # scribed panel lines: edges of large Voronoi cells
    # scribed lines that follow each plate: an outline inset 1.5 cm from the plate's edges, plus one cross break on the big plates
    pc = N(nt, "ShaderNodeAttribute"); pc.attribute_name = "PlateCoord"
    sepc = N(nt, "ShaderNodeSeparateColor"); nt.links.new(pc.outputs["Color"], sepc.inputs[0])

    def near(sock, at, hw=0.0016):
        dlt = math_(nt, 'ABSOLUTE', math_(nt, 'SUBTRACT', sock, at))
        return ramp(nt, dlt, hw * 0.6, hw * 1.4, c0=(1, 1, 1, 1), c1=(0, 0, 0, 1))
    ins_s = math_(nt, 'MULTIPLY', near(sepc.outputs[0], 0.015), ramp(nt, sepc.outputs[1], 0.0135, 0.0145))
    ins_t = math_(nt, 'MULTIPLY', near(sepc.outputs[1], 0.015), ramp(nt, sepc.outputs[0], 0.0135, 0.0145))
    crs = math_(nt, 'MULTIPLY', math_(nt, 'MULTIPLY', near(sepc.outputs[2], 0.0), pc.outputs["Alpha"]), ramp(nt, sepc.outputs[0], 0.0135, 0.0145))
    line = math_(nt, 'MAXIMUM', math_(nt, 'MAXIMUM', ins_s, ins_t), crs)
    col = mixc(nt, wear, base, c_wear)
    col = mixc(nt, math_(nt, 'MULTIPLY', line, 0.55), col, (0.004, 0.004, 0.005, 1))
    nt.links.new(col, b.inputs["Base Color"])
    b.inputs["Metallic"].default_value = metal
    rn = N(nt, "ShaderNodeTexNoise", in_Scale=9.0, in_Detail=2.0)
    nt.links.new(tc.outputs["Object"], rn.inputs["Vector"])
    r0 = math_(nt, 'ADD', r_base - 0.04, math_(nt, 'MULTIPLY', rn.outputs["Fac"], 0.09))
    nt.links.new(math_(nt, 'ADD', r0, math_(nt, 'MULTIPLY', wear, -0.12), clamp=True), b.inputs["Roughness"])
    bp = N(nt, "ShaderNodeBump"); bp.inputs["Strength"].default_value = 0.8; bp.inputs["Distance"].default_value = 0.003
    nt.links.new(math_(nt, 'MULTIPLY', line, -1.0), bp.inputs["Height"]); nt.links.new(bp.outputs[0], b.inputs["Normal"])


def suit(name):
    """Matte near-black undersuit, ribbed only at the joints (neck, elbows, waist, knees)."""
    nt, b = nodes_of(name)
    tc = N(nt, "ShaderNodeTexCoord")
    sep = N(nt, "ShaderNodeSeparateXYZ"); nt.links.new(tc.outputs["Object"], sep.inputs[0])
    mask = None
    for bone, rad in (("lowerarm_l", 0.13), ("lowerarm_r", 0.13), ("calf_l", 0.14), ("calf_r", 0.14)):
        d = N(nt, "ShaderNodeVectorMath", operation='DISTANCE'); d.inputs[1].default_value = B(bone)
        nt.links.new(tc.outputs["Object"], d.inputs[0])
        m = ramp(nt, d.outputs["Value"], rad * 0.6, rad, c0=(1, 1, 1, 1), c1=(0, 0, 0, 1))
        mask = m if mask is None else math_(nt, 'MAXIMUM', mask, m)
    ax = math_(nt, 'ABSOLUTE', sep.outputs["X"])
    core = ramp(nt, ax, 0.2, 0.24, c0=(1, 1, 1, 1), c1=(0, 0, 0, 1))
    waist = math_(nt, 'MULTIPLY', ramp(nt, sep.outputs["Z"], 1.03, 1.06), ramp(nt, sep.outputs["Z"], 1.25, 1.29, c0=(1, 1, 1, 1), c1=(0, 0, 0, 1)))
    neck = ramp(nt, sep.outputs["Z"], 1.5, 1.53)
    mask = math_(nt, 'MAXIMUM', mask, math_(nt, 'MULTIPLY', core, math_(nt, 'MAXIMUM', waist, neck)))
    s = math_(nt, 'SINE', math_(nt, 'MULTIPLY', sep.outputs["Z"], 2 * math.pi / 0.024))
    rib = math_(nt, 'MULTIPLY', ramp(nt, s, 0.35, 0.75), mask)
    col = mixc(nt, rib, hexc("#101113") + (1,), hexc("#1B1C20") + (1,))
    nt.links.new(col, b.inputs["Base Color"])
    b.inputs["Metallic"].default_value = 0.0
    nt.links.new(math_(nt, 'ADD', 0.82, math_(nt, 'MULTIPLY', rib, -0.08)), b.inputs["Roughness"])
    bp = N(nt, "ShaderNodeBump"); bp.inputs["Strength"].default_value = 0.8; bp.inputs["Distance"].default_value = 0.004
    nt.links.new(rib, bp.inputs["Height"]); nt.links.new(bp.outputs[0], b.inputs["Normal"])


def emissive(name, level=1.0, hex_cell_m=None):
    nt, b = nodes_of(name)
    a = hexc("#FF4A1E")
    acc = (a[0] * level, a[1] * level, a[2] * level, 1)
    if hex_cell_m:
        uvn = N(nt, "ShaderNodeUVMap"); uvn.uv_map = "UVMap"
        sc = N(nt, "ShaderNodeVectorMath", operation='SCALE'); sc.inputs["Scale"].default_value = 1.0 / (hex_cell_m * uv_per_m)
        nt.links.new(uvn.outputs[0], sc.inputs[0])
        S = (1.0, 1.7320508, 1.0)

        def cell(offset):
            ad = N(nt, "ShaderNodeVectorMath", operation='ADD'); ad.inputs[1].default_value = offset
            nt.links.new(sc.outputs[0], ad.inputs[0])
            m = N(nt, "ShaderNodeVectorMath", operation='MODULO'); m.inputs[1].default_value = S
            nt.links.new(ad.outputs[0], m.inputs[0])
            c = N(nt, "ShaderNodeVectorMath", operation='SUBTRACT'); c.inputs[1].default_value = (0.5, 0.8660254, 0.5)
            nt.links.new(m.outputs[0], c.inputs[0])
            ab = N(nt, "ShaderNodeVectorMath", operation='ABSOLUTE'); nt.links.new(c.outputs[0], ab.inputs[0])
            dt = N(nt, "ShaderNodeVectorMath", operation='DOT_PRODUCT'); dt.inputs[1].default_value = (0.5, 0.8660254, 0.0)
            nt.links.new(ab.outputs[0], dt.inputs[0])
            sx = N(nt, "ShaderNodeSeparateXYZ"); nt.links.new(ab.outputs[0], sx.inputs[0])
            return math_(nt, 'MAXIMUM', sx.outputs["X"], dt.outputs["Value"])
        d = math_(nt, 'MINIMUM', cell((100.0, 100.0, 0.0)), cell((100.5, 100.8660254, 0.0)))
        col = mixc(nt, ramp(nt, d, 0.40, 0.47), acc, (acc[0] * 0.14, acc[1] * 0.14, acc[2] * 0.14, 1))
        nt.links.new(col, b.inputs["Emission Color"])
    else:
        b.inputs["Emission Color"].default_value = acc
    b.inputs["Emission Strength"].default_value = 3.0
    b.inputs["Base Color"].default_value = (0.05 * level + 0.01, 0.006, 0.004, 1)
    b.inputs["Roughness"].default_value = 0.35
    b.inputs["Metallic"].default_value = 0.0


L4 = lambda h: hexc(h) + (1,)
have = set(names)
armor("M_NT_Armor", L4("#303032"), L4("#3F3F42"), L4("#7A7B7E"), 0.9, 0.54)
armor("M_NT_ArmorDark", L4("#222325"), L4("#2E2E31"), L4("#606266"), 0.75, 0.58, wear_amt=0.85)
suit("M_NT_Suit")
if "M_NT_ArmorFlat" in have: armor("M_NT_ArmorFlat", L4("#2A2B2D"), L4("#353538"), L4("#6A6C70"), 0.9, 0.55, wear_amt=0.0)
if "M_NT_Emissive" in have: emissive("M_NT_Emissive", 1.0)
if "M_NT_EmissiveCore" in have: emissive("M_NT_EmissiveCore", 0.6)
if "M_NT_EmissiveLine" in have: emissive("M_NT_EmissiveLine", 0.1)
if "M_NT_Rubber" in have:
    nt_, b_ = nodes_of("M_NT_Rubber")
    b_.inputs["Base Color"].default_value = hexc("#17181A") + (1,); b_.inputs["Roughness"].default_value = 0.7; b_.inputs["Metallic"].default_value = 0.0
if "M_NT_EmissiveDim" in have: emissive("M_NT_EmissiveDim", 0.42)
if "M_NT_EmissiveHex" in have: emissive("M_NT_EmissiveHex", 0.7, hex_cell_m=0.011)
if "M_NT_EmissiveGlow" in have: emissive("M_NT_EmissiveGlow", 0.2)
if "M_NT_Stripe" in have: emissive("M_NT_Stripe", 0.5)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(ROOT, OUT.replace(".blend", "_procedural.blend")))

# ---------------------------------------------------------------- bake
r = bk["main"](target="SK_" + ASSET, out_dir=EXPORT, asset_name=ASSET, size=SIZE, samples=8, ao_samples=64,
               margin=6, build_preview_material=True)
print("TEX bake", json.dumps({k: v for k, v in r.items() if k != "files"}), list(r["files"]))
# cavity darkening: multiply the base colour by the baked ambient occlusion (kept in ORM.R as well)
bc = bpy.data.images["T_%s_BC" % ASSET]; orm = bpy.data.images["T_%s_ORM" % ASSET]
pb = np.empty(len(bc.pixels), dtype=np.float32); bc.pixels.foreach_get(pb); pb = pb.reshape(-1, 4)
po = np.empty(len(orm.pixels), dtype=np.float32); orm.pixels.foreach_get(po); po = po.reshape(-1, 4)
ao = np.clip(po[:, 0], 0, 1)
pb[:, :3] *= (0.68 + 0.32 * ao ** 1.5)[:, None]
bc.pixels.foreach_set(pb.ravel()); bc.update()
bc.filepath_raw = os.path.join(EXPORT, "T_%s_BC.png" % ASSET); bc.file_format = 'PNG'; bc.save()
em = bpy.data.images.get("T_%s_E" % ASSET)
if em:
    pe = np.empty(len(em.pixels), dtype=np.float32); em.pixels.foreach_get(pe); pe = pe.reshape(-1, 4)
    print("TEX emissive texels lit %.4f of the atlas" % float((pe[:, :3].max(axis=1) > 0.05).mean()))
m = ob.data.materials[0]
m.name = "M_" + ASSET
print("TEX slots", [mm.name for mm in ob.data.materials])
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(ROOT, OUT))

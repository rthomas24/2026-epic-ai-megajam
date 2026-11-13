# Stage 30 (step E): UVs, procedural materials, bake to one 2048 set (T_NullTrooper_BC/_N/_ORM/_E).
# blender --background --factory-startup NullTrooper_v06.blend --python s30_tex.py -- --out NullTrooper_v07.blend [--size 2048]
import bpy, bmesh, os, sys, math, runpy, json
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
SK = os.path.join(ROOT, "..", "..", "..", ".claude", "skills")
argv = sys.argv[sys.argv.index("--") + 1:]
OUT = argv[argv.index("--out") + 1]
SIZE = int(argv[argv.index("--size") + 1]) if "--size" in argv else 2048
hs = runpy.run_path(os.path.join(SK, "blender-game-asset", "scripts", "hardsurface.py"))
bk = runpy.run_path(os.path.join(SK, "blender-texturing", "scripts", "bake_texture_set.py"))
hexc = hs["hex_color"]
ob = bpy.data.objects["SK_NullTrooper"]
me = ob.data

# ---------------------------------------------------------------- UVs
while me.uv_layers: me.uv_layers.remove(me.uv_layers[0])
me.uv_layers.new(name="UVMap")
hs["smart_uv"](ob, angle_deg=75, island_margin=0.0015)
for o in bpy.context.selected_objects: o.select_set(False)
ob.select_set(True); bpy.context.view_layer.objects.active = ob
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.uv.select_all(action='SELECT')
bpy.ops.uv.average_islands_scale()
try:
    bpy.ops.uv.pack_islands(rotate=True, margin=0.0011, shape_method='CONCAVE')
except TypeError:
    bpy.ops.uv.pack_islands(rotate=True, margin=0.0011)
bpy.ops.object.mode_set(mode='OBJECT')
# areas: emissive share, texel density
bm = bmesh.new(); bm.from_mesh(me)
uv = bm.loops.layers.uv.active
area3 = uva = 0.0
by_mat = {}
for f in bm.faces:
    a = f.calc_area(); area3 += a
    by_mat[f.material_index] = by_mat.get(f.material_index, 0.0) + a
    l = f.loops
    for k in range(1, len(l) - 1):
        p0, p1, p2 = l[0][uv].uv, l[k][uv].uv, l[k + 1][uv].uv
        uva += abs((p1.x - p0.x) * (p2.y - p0.y) - (p2.x - p0.x) * (p1.y - p0.y)) / 2
bm.free()
uv_per_m = math.sqrt(uva / area3)
names = [m.name for m in me.materials]
share = {names[k]: round(v / area3, 4) for k, v in by_mat.items()}
print("TEX uv coverage %.3f  surface %.3f m2  texel density %.0f px/m  area share %s" % (uva, area3, uv_per_m * SIZE, share))


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
            key = k[3:]
            key = int(key) if key.isdigit() else key.replace("_", " ")
            n.inputs[key].default_value = v
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
    nt, b = nodes_of(name)
    tc = N(nt, "ShaderNodeTexCoord")
    big = N(nt, "ShaderNodeTexNoise", in_Scale=7.0, in_Detail=3.0)
    nt.links.new(tc.outputs["Object"], big.inputs["Vector"])
    tone = ramp(nt, big.outputs["Fac"], 0.35, 0.7)
    base = mixc(nt, tone, c_lo, c_hi)
    # worn edges: pointiness, broken up by a mid-frequency noise
    geo = N(nt, "ShaderNodeNewGeometry")
    edge = ramp(nt, geo.outputs["Pointiness"], 0.505, 0.545)
    chip = N(nt, "ShaderNodeTexNoise", in_Scale=55.0, in_Detail=4.0)
    nt.links.new(tc.outputs["Object"], chip.inputs["Vector"])
    chipm = ramp(nt, chip.outputs["Fac"], 0.22, 0.5)
    wear_e = math_(nt, 'MULTIPLY', edge, chipm)
    # fine scratches: thin streaks from a stretched noise
    mp = N(nt, "ShaderNodeMapping"); mp.inputs["Scale"].default_value = (30.0, 30.0, 420.0); mp.inputs["Rotation"].default_value = (0.5, 0.3, 0.2)
    nt.links.new(tc.outputs["Object"], mp.inputs["Vector"])
    sc = N(nt, "ShaderNodeTexNoise", in_Scale=1.0, in_Detail=2.0)
    nt.links.new(mp.outputs[0], sc.inputs["Vector"])
    scr = ramp(nt, sc.outputs["Fac"], 0.66, 0.72)
    mp2 = N(nt, "ShaderNodeMapping"); mp2.inputs["Scale"].default_value = (380.0, 26.0, 26.0); mp2.inputs["Rotation"].default_value = (0.2, 0.9, 0.4)
    nt.links.new(tc.outputs["Object"], mp2.inputs["Vector"])
    sc2 = N(nt, "ShaderNodeTexNoise", in_Scale=1.0, in_Detail=2.0)
    nt.links.new(mp2.outputs[0], sc2.inputs["Vector"])
    scr2 = ramp(nt, sc2.outputs["Fac"], 0.67, 0.73)
    scratches = math_(nt, 'MAXIMUM', scr, scr2)
    wear = math_(nt, 'MAXIMUM', wear_e, math_(nt, 'MULTIPLY', scratches, 0.45))
    wear = math_(nt, 'MULTIPLY', wear, wear_amt, clamp=True)
    col = mixc(nt, wear, base, c_wear)
    nt.links.new(col, b.inputs["Base Color"])
    b.inputs["Metallic"].default_value = metal
    rn = N(nt, "ShaderNodeTexNoise", in_Scale=18.0, in_Detail=3.0)
    nt.links.new(tc.outputs["Object"], rn.inputs["Vector"])
    r0 = math_(nt, 'ADD', r_base - 0.07, math_(nt, 'MULTIPLY', rn.outputs["Fac"], 0.16))
    rough = math_(nt, 'ADD', r0, math_(nt, 'MULTIPLY', wear, -0.14), clamp=True)
    nt.links.new(rough, b.inputs["Roughness"])
    fine = N(nt, "ShaderNodeTexNoise", in_Scale=320.0, in_Detail=2.0)
    nt.links.new(tc.outputs["Object"], fine.inputs["Vector"])
    h = math_(nt, 'ADD', math_(nt, 'MULTIPLY', fine.outputs["Fac"], 0.5), math_(nt, 'MULTIPLY', scratches, -0.8))
    bp = N(nt, "ShaderNodeBump"); bp.inputs["Strength"].default_value = 0.35; bp.inputs["Distance"].default_value = 0.002
    nt.links.new(h, bp.inputs["Height"]); nt.links.new(bp.outputs[0], b.inputs["Normal"])


def suit(name):
    nt, b = nodes_of(name)
    tc = N(nt, "ShaderNodeTexCoord")
    sep = N(nt, "ShaderNodeSeparateXYZ"); nt.links.new(tc.outputs["Object"], sep.inputs[0])
    wob = N(nt, "ShaderNodeTexNoise", in_Scale=9.0, in_Detail=1.0); nt.links.new(tc.outputs["Object"], wob.inputs["Vector"])
    z = math_(nt, 'ADD', sep.outputs["Z"], math_(nt, 'MULTIPLY', wob.outputs["Fac"], 0.006))
    s = math_(nt, 'SINE', math_(nt, 'MULTIPLY', z, 2 * math.pi / 0.021))
    rib = ramp(nt, s, 0.35, 0.75)           # raised band
    weave = N(nt, "ShaderNodeTexNoise", in_Scale=900.0, in_Detail=1.0); nt.links.new(tc.outputs["Object"], weave.inputs["Vector"])
    col = mixc(nt, rib, hexc("#0C0D0F") + (1,), hexc("#191A1D") + (1,))
    nt.links.new(col, b.inputs["Base Color"])
    b.inputs["Metallic"].default_value = 0.0
    nt.links.new(math_(nt, 'ADD', 0.86, math_(nt, 'MULTIPLY', rib, -0.12)), b.inputs["Roughness"])
    h = math_(nt, 'ADD', rib, math_(nt, 'MULTIPLY', weave.outputs["Fac"], 0.12))
    bp = N(nt, "ShaderNodeBump"); bp.inputs["Strength"].default_value = 0.7; bp.inputs["Distance"].default_value = 0.004
    nt.links.new(h, bp.inputs["Height"]); nt.links.new(bp.outputs[0], b.inputs["Normal"])


def emissive(name, cell_m):
    nt, b = nodes_of(name)
    acc = hexc("#FF4A1E") + (1,)
    uvn = N(nt, "ShaderNodeUVMap"); uvn.uv_map = "UVMap"
    sc = N(nt, "ShaderNodeVectorMath", operation='SCALE'); sc.inputs["Scale"].default_value = 1.0 / (cell_m * uv_per_m)
    nt.links.new(uvn.outputs[0], sc.inputs[0])
    S = (1.0, 1.7320508, 1.0)

    def cell(offset):
        a = N(nt, "ShaderNodeVectorMath", operation='ADD'); a.inputs[1].default_value = offset
        nt.links.new(sc.outputs[0], a.inputs[0])
        m = N(nt, "ShaderNodeVectorMath", operation='MODULO'); m.inputs[1].default_value = S
        nt.links.new(a.outputs[0], m.inputs[0])
        c = N(nt, "ShaderNodeVectorMath", operation='SUBTRACT'); c.inputs[1].default_value = (0.5, 0.8660254, 0.5)
        nt.links.new(m.outputs[0], c.inputs[0])
        ab = N(nt, "ShaderNodeVectorMath", operation='ABSOLUTE'); nt.links.new(c.outputs[0], ab.inputs[0])
        dt = N(nt, "ShaderNodeVectorMath", operation='DOT_PRODUCT'); dt.inputs[1].default_value = (0.5, 0.8660254, 0.0)
        nt.links.new(ab.outputs[0], dt.inputs[0])
        sx = N(nt, "ShaderNodeSeparateXYZ"); nt.links.new(ab.outputs[0], sx.inputs[0])
        return math_(nt, 'MAXIMUM', sx.outputs["X"], dt.outputs["Value"])
    d = math_(nt, 'MINIMUM', cell((100.0, 100.0, 0.0)), cell((100.5, 100.8660254, 0.0)))
    line = ramp(nt, d, 0.39, 0.46)
    col = mixc(nt, line, acc, (acc[0] * 0.16, acc[1] * 0.16, acc[2] * 0.16, 1))
    nt.links.new(col, b.inputs["Emission Color"])
    b.inputs["Emission Strength"].default_value = 3.0
    b.inputs["Base Color"].default_value = (0.05, 0.008, 0.004, 1)
    b.inputs["Roughness"].default_value = 0.35
    b.inputs["Metallic"].default_value = 0.0


def stripe(name):
    nt, b = nodes_of(name)
    acc = hexc("#FF4A1E")
    b.inputs["Base Color"].default_value = (acc[0] * 0.55, acc[1] * 0.5, acc[2] * 0.5, 1)
    b.inputs["Emission Color"].default_value = (acc[0] * 0.3, acc[1] * 0.3, acc[2] * 0.3, 1)
    b.inputs["Emission Strength"].default_value = 3.0
    b.inputs["Roughness"].default_value = 0.4
    b.inputs["Metallic"].default_value = 0.0


L4 = lambda h: hexc(h) + (1,)
armor("M_NT_Armor", L4("#2E3034"), L4("#40434A"), L4("#8A8C92"), 0.9, 0.5)
armor("M_NT_ArmorDark", L4("#1D1E21"), L4("#2A2C30"), L4("#6A6C70"), 0.75, 0.56, wear_amt=0.85)
suit("M_NT_Suit")
emissive("M_NT_Emissive", 0.0075)
stripe("M_NT_Stripe")
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(ROOT, OUT.replace(".blend", "_procedural.blend")))

# ---------------------------------------------------------------- bake
r = bk["main"](target="SK_NullTrooper", out_dir=os.path.join(ROOT, "export"), asset_name="NullTrooper", size=SIZE, samples=8, ao_samples=48,
               margin=6, build_preview_material=True)
print("TEX bake", json.dumps({k: v for k, v in r.items() if k != "files"}), list(r["files"]))
m = ob.data.materials[0]
m.name = "M_NullTrooper"
print("TEX slots", [mm.name for mm in ob.data.materials])
bpy.ops.file.pack_all() if False else None
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(ROOT, OUT))

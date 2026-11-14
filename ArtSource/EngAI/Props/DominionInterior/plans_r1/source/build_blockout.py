"""Assemble the Dominion interior blockout from the ACTUAL kit meshes (ArtSource/EngProps/DominionInterior/export/*.fbx),
placed by pivot at scale 1 from interior_layout.json (the same numbers as the plans and the C++). Linked duplicates of the
kit meshes, so the .blend stays small and editable; re-run after changing interior_layout.py. Adds 180 cm figures, a ship
footprint proxy on Pad A, trooper / Warden capsule proxies at their C++ spawns, the C++ lights, and a TrimPreview bay
(off the route, y +6000) showing T_DI_InteriorTrim + T_DI_Decals on a 400 x 600 wall cell. No AI mesh anywhere.
(Material/import helpers reused from PR #77's DominionDockingBay/plans_r1/source/build_blockout.py.)

blender -b --factory-startup --python build_blockout.py -- <repo_root> <plans_r1_dir>
Blender frame: metres, Blender Y = -Unreal Y (kit FBX convention): (x, y, z, yaw) -> (x/100, -y/100, z/100), rot -yaw.
"""
import bpy, bmesh, sys, os, json, math
from mathutils import Vector, Matrix

argv = sys.argv[sys.argv.index('--') + 1:]
REPO, PLANS = os.path.abspath(argv[0]), os.path.abspath(argv[1])
KIT = os.path.join(REPO, "ArtSource/EngProps/DominionInterior")
L = json.load(open(os.path.join(PLANS, "interior_layout.json")))
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene

def coll(name, parent=None):
    c = bpy.data.collections.new(name); (parent or sc.collection).children.link(c); return c

C_SRC = coll("KitSource (do not move)"); C_SRC.hide_render = True; C_SRC.hide_viewport = True
C_BAY = coll("DominionInterior_Blockout")
groups = {}

# --- kit materials (preview of MI_DI_*; the atlas is DirectX normal -> invert green) -------------------------------
def atlas_mat(m):
    m.use_nodes = True; nt = m.node_tree; N = nt.nodes; Lk = nt.links
    for n in list(N):
        if n.type != 'OUTPUT_MATERIAL': N.remove(n)
    out = [n for n in N if n.type == 'OUTPUT_MATERIAL'][0]
    b = N.new("ShaderNodeBsdfPrincipled"); Lk.new(b.outputs[0], out.inputs[0])
    def img(n, cs):
        t = N.new("ShaderNodeTexImage"); t.image = bpy.data.images.load(os.path.join(KIT, "textures", n), check_existing=True)
        t.image.colorspace_settings.name = cs; return t
    bc, orm, nrm = img("T_DI_Atlas_BC.png", "sRGB"), img("T_DI_Atlas_ORM.png", "Non-Color"), img("T_DI_Atlas_N.png", "Non-Color")
    Lk.new(bc.outputs[0], b.inputs["Base Color"])
    s = N.new("ShaderNodeSeparateColor"); Lk.new(orm.outputs[0], s.inputs[0])
    Lk.new(s.outputs[1], b.inputs["Roughness"]); Lk.new(s.outputs[2], b.inputs["Metallic"])
    s2 = N.new("ShaderNodeSeparateColor"); inv = N.new("ShaderNodeMath"); inv.operation = 'SUBTRACT'; inv.inputs[0].default_value = 1
    cm = N.new("ShaderNodeCombineColor"); Lk.new(nrm.outputs[0], s2.inputs[0]); Lk.new(s2.outputs[0], cm.inputs[0])
    Lk.new(s2.outputs[1], inv.inputs[1]); Lk.new(inv.outputs[0], cm.inputs[1]); Lk.new(s2.outputs[2], cm.inputs[2])
    nm = N.new("ShaderNodeNormalMap"); Lk.new(cm.outputs[0], nm.inputs["Color"]); Lk.new(nm.outputs[0], b.inputs["Normal"])
GLOW = {"M_DI_GlowRed": ((1.0, 0.14, 0.02), 8.0), "M_DI_GlowWhite": ((0.9, 0.95, 1.0), 14.0),
        "M_DI_Screen": ((0.25, 0.85, 1.0), 4.0), "M_DI_Cell": ((1.0, 0.42, 0.08), 14.0), "M_DI_CoreGlow": ((1.0, 0.86, 0.7), 30.0)}
def glow_mat(m, col, strength):
    m.use_nodes = True; b = m.node_tree.nodes.get("Principled BSDF")
    b.inputs["Base Color"].default_value = (0.01, 0.012, 0.016, 1); b.inputs["Roughness"].default_value = 0.2
    b.inputs["Emission Color"].default_value = (*col, 1); b.inputs["Emission Strength"].default_value = strength
def shield_mat(m):
    m.use_nodes = True; nt = m.node_tree; b = nt.nodes.get("Principled BSDF")
    b.inputs["Base Color"].default_value = (1, 0.5, 0.2, 1); b.inputs["Alpha"].default_value = 0.12
    b.inputs["Emission Color"].default_value = (1, 0.45, 0.15, 1); b.inputs["Emission Strength"].default_value = 0.6
done_mats = set()
def fix_materials(o):
    for slot in o.material_slots:
        m = slot.material
        if not m or m.name in done_mats: continue
        base = m.name.split(".")[0]
        if base == "M_DI_Atlas": atlas_mat(m)
        elif base in GLOW: glow_mat(m, *GLOW[base])
        elif base == "M_DI_Shield": shield_mat(m)
        done_mats.add(m.name)

# --- import each kit mesh once ------------------------------------------------------------------------------------
src = {}
for name in sorted(set(p["mesh"] for p in L["placements"])):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=os.path.join(KIT, "export", name + ".fbx"))
    new = [o for o in bpy.data.objects if o not in before]
    for o in new:
        for c in o.users_collection: c.objects.unlink(o)
        C_SRC.objects.link(o)
    body = [o for o in new if o.type == 'MESH' and not o.name.startswith("UCX_")][0]
    fix_materials(body)
    src[name] = body

def place(mesh_obj, x, y, z, yaw, coll_, name=None):
    o = bpy.data.objects.new(name or mesh_obj.name, mesh_obj.data)
    o.location = (x / 100.0, -y / 100.0, z / 100.0); o.rotation_euler = (0, 0, math.radians(-yaw))
    coll_.objects.link(o); return o

for p in L["placements"]:
    g = p["group"]
    if g not in groups: groups[g] = coll("G_" + g, C_BAY)
    o = place(src[p["mesh"]], p["x"], p["y"], p["z"], p["yaw"], groups[g])
    o["kit_mesh"] = p["mesh"]; o["ue_xyz_yaw"] = [p["x"], p["y"], p["z"], p["yaw"]]
    if p["note"]: o["note"] = p["note"]
    if p["mesh"] == "SM_DI_Room_Ceiling": o["is_ceiling"] = 1

def solid_mat(name, col, emit=0.0):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name); m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]; b.inputs["Base Color"].default_value = (*col, 1); b.inputs["Roughness"].default_value = 0.5
    if emit: b.inputs["Emission Color"].default_value = (*col, 1); b.inputs["Emission Strength"].default_value = emit
    return m
C_FIG = coll("G_scale_figures_and_actors", C_BAY)
def capsule(name, x, y, r_cm, h_cm, col, emit=0.0, z0=0.0):
    bm = bmesh.new(); r, h = r_cm / 100, h_cm / 100
    bmesh.ops.create_cone(bm, cap_ends=True, segments=16, radius1=r, radius2=r, depth=h - 2 * r, matrix=Matrix.Translation((0, 0, h / 2)))
    for zz in (r, h - r): bmesh.ops.create_uvsphere(bm, u_segments=16, v_segments=8, radius=r, matrix=Matrix.Translation((0, 0, zz)))
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free(); me.materials.append(solid_mat("M_" + name.split("_")[0], col, emit))
    o = bpy.data.objects.new(name, me); C_FIG.objects.link(o); o.location = (x / 100, -y / 100, z0 / 100); return o
def figure(x, y, yaw=0.0, name="Figure_180cm"):
    bm = bmesh.new()
    def cap(cx, cy, z0, z1, r):
        bmesh.ops.create_cone(bm, cap_ends=True, segments=16, radius1=r, radius2=r, depth=(z1 - z0), matrix=Matrix.Translation((cx, cy, (z0 + z1) / 2)))
    cap(0, -0.11, 0.0, 0.86, 0.075); cap(0, 0.11, 0.0, 0.86, 0.075); cap(0, 0, 0.84, 1.46, 0.19)
    cap(0, -0.27, 0.80, 1.44, 0.055); cap(0, 0.27, 0.80, 1.44, 0.055)
    bmesh.ops.create_uvsphere(bm, u_segments=16, v_segments=10, radius=0.11, matrix=Matrix.Translation((0, 0, 1.69)))
    bmesh.ops.create_cone(bm, cap_ends=True, segments=8, radius1=0.05, radius2=0.05, depth=0.12, matrix=Matrix.Translation((0, 0, 1.52)))
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free(); me.materials.append(solid_mat("M_Figure", (0.95, 0.75, 0.1)))
    o = bpy.data.objects.new(name, me); C_FIG.objects.link(o)
    o.location = (x / 100, -y / 100, 0); o.rotation_euler = (0, 0, math.radians(-yaw)); return o
G = L["game"]
for (n, x, y, yaw) in (("PilotStart", -600, 0, 0), ("HangarDoor", 1000, 120, 0), ("Corridor", 3000, 60, 0), ("ArenaIn", 5600, 200, 0),
                       ("Warden", 8800, -200, 0), ("CoreDoor", 11500, 150, 0), ("CoreRing", 12100, -900, 30), ("Pocket", -6200, 600, 0)):
    figure(x, y, yaw, "Figure_180cm_" + n)
for n, (x, y) in G["spawns"].items():
    if "Warden" in n: capsule("Warden_capsule_r60_h260", x, y, 60, 260, (0.45, 0.08, 0.12), 0.4, 0)
    else: capsule("Trooper_capsule_r40_h180_" + n[-1], x, y, 40, 180, (0.6, 0.12, 0.1), 0.3)
# heart sphere (the C++ glowing sphere until its art lands)
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.8, location=(G["core"]["heart"][0] / 100, 0, 1.6)); h = bpy.context.object; h.name = "Heart_placeholder_r80"
h.data.materials.append(solid_mat("M_Heart", (1.0, 0.55, 0.25), 6.0))
for c in list(h.users_collection): c.objects.unlink(h)
C_FIG.objects.link(h)
# ship footprint proxy on Pad A
C_SHIP = coll("G_ship_proxy", C_BAY); sl, sw, sh = L["ship_footprint"]["size"]
bm = bmesh.new(); bmesh.ops.create_cube(bm, size=1.0)
bmesh.ops.transform(bm, matrix=Matrix.Diagonal((sl / 100, sw / 100, sh / 100, 1)), verts=bm.verts); bmesh.ops.translate(bm, vec=(0, 0, sh / 200 + 0.005), verts=bm.verts)
me = bpy.data.meshes.new("ShipProxy_PadA"); bm.to_mesh(me); bm.free(); me.materials.append(solid_mat("M_ShipProxy", (0.75, 0.78, 0.82)))
o = bpy.data.objects.new("ShipProxy_PadA_524x488x230", me); C_SHIP.objects.link(o); o.location = (-2.0, 0, 0)
# lights as T5 places them (watts are a Cycles guess for the read, not Unreal candelas)
C_L = coll("G_lights_cpp", C_BAY)
COLS = {"white": (0.85, 0.9, 1.0), "red": (1.0, 0.25, 0.12), "warm red": (1.0, 0.6, 0.5), "core orange": (1.0, 0.45, 0.2)}
WATT = {"white": 60000, "red": 1200, "warm red": 30000, "core orange": 120000}
for i, (pos, rad, col) in enumerate(G["lights"]):
    l = bpy.data.lights.new("L_%02d_%s" % (i, col), 'POINT'); l.energy = WATT[col]; l.color = COLS[col]; l.shadow_soft_size = 0.5
    lo = bpy.data.objects.new(l.name, l); lo.location = (pos[0] / 100, -pos[1] / 100, pos[2] / 100); C_L.objects.link(lo)
for i, x in enumerate((-6000, -4400, -2800)):
    l = bpy.data.lights.new("Pocket_Work_%d" % i, 'POINT'); l.energy = 90000; l.color = (0.85, 0.9, 1.0)
    lo = bpy.data.objects.new(l.name, l); lo.location = (x / 100, 0, 33); C_L.objects.link(lo)

# --- TrimPreview: one 400 x 600 wall cell + 400 x 400 floor with the NEW trim sheet and decals (off the route) ---------
C_TP = coll("TrimPreview (T_DI_InteriorTrim + T_DI_Decals)", C_BAY)
TX = os.path.join(PLANS, "textures")
def trim_mat(name, prefix, alpha=False):
    m = bpy.data.materials.new(name); m.use_nodes = True; nt = m.node_tree; N_ = nt.nodes; Lk = nt.links
    b = N_["Principled BSDF"]
    def img(n, cs):
        t = N_.new("ShaderNodeTexImage"); t.image = bpy.data.images.load(os.path.join(TX, n), check_existing=True); t.image.colorspace_settings.name = cs; return t
    bc, orm, nrm, em = img(prefix + "_BC.png", "sRGB"), img(prefix + "_ORM.png", "Non-Color"), img(prefix + "_N.png", "Non-Color"), img(prefix + "_E.png", "sRGB")
    Lk.new(bc.outputs[0], b.inputs["Base Color"])
    if alpha: Lk.new(bc.outputs[1], b.inputs["Alpha"]); m.blend_method = 'BLEND' if hasattr(m, "blend_method") else None
    s = N_.new("ShaderNodeSeparateColor"); Lk.new(orm.outputs[0], s.inputs[0]); Lk.new(s.outputs[1], b.inputs["Roughness"]); Lk.new(s.outputs[2], b.inputs["Metallic"])
    s2 = N_.new("ShaderNodeSeparateColor"); inv = N_.new("ShaderNodeMath"); inv.operation = 'SUBTRACT'; inv.inputs[0].default_value = 1
    cm = N_.new("ShaderNodeCombineColor"); Lk.new(nrm.outputs[0], s2.inputs[0]); Lk.new(s2.outputs[0], cm.inputs[0])
    Lk.new(s2.outputs[1], inv.inputs[1]); Lk.new(inv.outputs[0], cm.inputs[1]); Lk.new(s2.outputs[2], cm.inputs[2])
    nm = N_.new("ShaderNodeNormalMap"); Lk.new(cm.outputs[0], nm.inputs["Color"]); Lk.new(nm.outputs[0], b.inputs["Normal"])
    Lk.new(em.outputs[0], b.inputs["Emission Color"]); b.inputs["Emission Strength"].default_value = 4.0
    return m
MT = trim_mat("M_DI_InteriorTrim_preview", "T_DI_InteriorTrim"); MD = trim_mat("M_DI_Decals_preview", "T_DI_Decals", alpha=True)
BANDS = {b["key"]: b for b in json.load(open(os.path.join(TX, "trim_layout.json")))["trim"]["bands"]}
DEC = {d["key"]: d for d in json.load(open(os.path.join(TX, "trim_layout.json")))["decals"]}
def strip_quad(name, p0, p1, z0, z1, key, mat, nrm_off=(0, 0, 0), u_per_cm=1 / 400.0):
    """vertical quad from p0 to p1 (UE xy) between z0..z1, UV'd to band `key` (V) and 1 tile / 400 cm (U)."""
    v0, v1 = BANDS[key]["uv_v"]; Lu = math.hypot(p1[0] - p0[0], p1[1] - p0[1]) * u_per_cm
    pts = [(p0[0], p0[1], z0), (p1[0], p1[1], z0), (p1[0], p1[1], z1), (p0[0], p0[1], z1)]
    uvs = [(0, v0), (Lu, v0), (Lu, v1), (0, v1)]
    bm = bmesh.new(); vs = [bm.verts.new(((x + nrm_off[0]) / 100, -(y + nrm_off[1]) / 100, (z + nrm_off[2]) / 100)) for (x, y, z) in pts]
    f = bm.faces.new(vs); lay = bm.loops.layers.uv.new("UVMap")
    for l, uv in zip(f.loops, uvs): l[lay].uv = uv
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free(); me.materials.append(mat)
    o = bpy.data.objects.new(name, me); C_TP.objects.link(o); return o
def floor_quad(name, x0, y0, x1, y1, z, uvrect, mat):
    u0, v0, u1, v1 = uvrect
    bm = bmesh.new(); vs = [bm.verts.new((x / 100, -y / 100, z / 100)) for (x, y) in ((x0, y0), (x1, y0), (x1, y1), (x0, y1))]
    f = bm.faces.new(vs); lay = bm.loops.layers.uv.new("UVMap")
    for l, uv in zip(f.loops, ((u0, v0), (u1, v0), (u1, v1), (u0, v1))): l[lay].uv = uv
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free(); me.materials.append(mat)
    o = bpy.data.objects.new(name, me); C_TP.objects.link(o); return o
TPX, TPY = 0.0, 6000.0      # preview bay origin (UE), wall plane at y = TPY + 200 facing -Y
W0, W1 = (TPX - 400, TPY + 200), (TPX + 400, TPY + 200)
stack = [("KICK_PLATE", 0, 25), ("STRIP_RED", 25, 37.5), ("WALL_PANEL", 37.5, 137.5), ("STRIP_AMBER", 137.5, 150), ("WALL_PANEL", 150, 250),
         ("CRIMSON_BAND", 250, 275), ("WALL_PANEL", 275, 375), ("VENT", 375, 400), ("WALL_PANEL", 400, 500), ("STRIP_CYAN", 500, 512.5),
         ("CABLE_TRAY", 512.5, 562.5), ("TRIM_CAP", 562.5, 600)]
for i, (k, z0, z1) in enumerate(stack):
    strip_quad("TP_Wall_%02d_%s" % (i, k), W0, W1, z0, z1, k, MT)
for sgn in (-1, 1):   # side walls (returns) so the corner reads
    x = TPX + sgn * 400
    for i, (k, z0, z1) in enumerate(stack):
        strip_quad("TP_Side%+d_%02d_%s" % (sgn, i, k), (x, TPY - 400), (x, TPY + 200), z0, z1, k, MT)
fv0, fv1 = BANDS["DECK_PLATE"]["uv_v"]
for j in range(12):   # floor: 50 cm deck strips, 800 x 600
    floor_quad("TP_Floor_%02d" % j, TPX - 400, TPY - 400 + j * 50, TPX + 400, TPY - 350 + j * 50, 0, (0, fv0, 2, fv1), MT)
hv0, hv1 = BANDS["HAZARD"]["uv_v"]
floor_quad("TP_Floor_HazardEdge", TPX - 400, TPY + 175, TPX + 400, TPY + 200, 0.3, (0, hv0, 2, hv1), MT)
def dec(key, uv): d = DEC[key]["uv"]; return (d[0], d[1], d[2], d[3])
# trident on the wall (160 cm), triad mark (120 cm), route arrow + hazard frame on the floor
def wall_decal(name, key, cx, cz, size):
    u0, v0, u1, v1 = DEC[key]["uv"]; y = TPY + 199.4
    bm = bmesh.new(); vs = [bm.verts.new((x / 100, -y / 100, z / 100)) for (x, z) in ((cx - size / 2, cz - size / 2), (cx + size / 2, cz - size / 2), (cx + size / 2, cz + size / 2), (cx - size / 2, cz + size / 2))]
    f = bm.faces.new(vs); lay = bm.loops.layers.uv.new("UVMap")
    for l, uv in zip(f.loops, ((u0, v0), (u1, v0), (u1, v1), (u0, v1))): l[lay].uv = uv
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free(); me.materials.append(MD); o = bpy.data.objects.new(name, me); C_TP.objects.link(o)
wall_decal("TP_Decal_Trident", "TRIDENT", TPX - 150, 330, 160); wall_decal("TP_Decal_TriadMark", "TRIAD_MARK", TPX + 220, 330, 120)
u0, v0, u1, v1 = DEC["ROUTE_ARROW"]["uv"]; floor_quad("TP_Decal_RouteArrow", TPX - 100, TPY - 300, TPX + 100, TPY - 100, 0.5, (u0, v1, u1, v0), MD)
u0, v0, u1, v1 = DEC["HAZARD_FRAME"]["uv"]; floor_quad("TP_Decal_HazardFrame", TPX + 150, TPY - 350, TPX + 350, TPY - 150, 0.5, (u0, v1, u1, v0), MD)
figure(TPX - 280, TPY - 120, 0, "Figure_180cm_TrimPreview")

bpy.ops.wm.save_as_mainfile(filepath=os.path.join(PLANS, "blockout", "DominionInterior_Blockout.blend"), compress=True)
bpy.ops.file.make_paths_relative(); bpy.ops.wm.save_mainfile(compress=True)
print("BLOCKOUT_OK objects", sum(1 for o in C_BAY.all_objects), "kit sources", len(src))

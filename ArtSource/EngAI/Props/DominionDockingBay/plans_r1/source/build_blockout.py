"""Assemble the docking-bay blockout from the ACTUAL kit meshes (ArtSource/EngProps/DominionInterior/export/*.fbx),
placed by pivot at scale 1 from dockbay_layout.json (the same numbers as the plans). Every placed object is a linked
duplicate of the kit mesh, so the .blend stays small and editable: move or delete pieces, or re-run this script after
changing dockbay_layout.py. Also places the exterior bay pieces (open state) at the mouth, a force-field plane, a
180 cm figure and a ship-footprint proxy box on each pad. No AI mesh anywhere.

blender -b --factory-startup --python build_blockout.py -- <repo_root> <plans_dir>
Blender frame: metres, Blender Y = -Unreal Y (the kit FBX import convention), so a placement (x, y, z, yaw) goes to
(x/100, -y/100, z/100) with rotation -yaw.
"""
import bpy, bmesh, sys, os, json, math
from mathutils import Vector, Matrix

argv = sys.argv[sys.argv.index('--') + 1:]
REPO, PLANS = os.path.abspath(argv[0]), os.path.abspath(argv[1])
KIT = os.path.join(REPO, "ArtSource/EngProps/DominionInterior")
L = json.load(open(os.path.join(PLANS, "dockbay_layout.json")))

bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene

def coll(name, parent=None):
    c = bpy.data.collections.new(name); (parent or sc.collection).children.link(c); return c

C_SRC = coll("KitSource (do not move)"); C_SRC.hide_render = True; C_SRC.hide_viewport = True
C_BAY = coll("DockingBay_Blockout")
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
        "M_DI_Screen": ((0.25, 0.85, 1.0), 4.0), "M_DI_Cell": ((1.0, 0.42, 0.08), 14.0)}
def glow_mat(m, col, strength):
    m.use_nodes = True; b = m.node_tree.nodes.get("Principled BSDF")
    b.inputs["Base Color"].default_value = (0.01, 0.012, 0.016, 1); b.inputs["Roughness"].default_value = 0.2
    b.inputs["Emission Color"].default_value = (*col, 1); b.inputs["Emission Strength"].default_value = strength
done_mats = set()
def fix_materials(o):
    for slot in o.material_slots:
        m = slot.material
        if not m or m.name in done_mats: continue
        base = m.name.split(".")[0]
        if base == "M_DI_Atlas": atlas_mat(m)
        elif base in GLOW: glow_mat(m, *GLOW[base])
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

# --- exterior bay pieces (open) at the mouth: yaw 180 so their local +X (out) points down -X here --------------------
C_EXT = coll("G_mouth_exterior_pieces (reference, hidden in renders)", C_BAY)
C_EXT.hide_render = True   # in the interior map the mouth looks out to space; these belong to the hull
ext_blend = os.path.join(PLANS, "mesh", "DominionBay_Exterior.blend")
with bpy.data.libraries.load(ext_blend, link=False) as (src_d, dst_d):
    dst_d.objects = [n for n in src_d.objects if n.startswith("SM_DominionBay_")]
mx, my, mz = L["mouth"]["centre"]
for o in dst_d.objects:
    C_EXT.objects.link(o)
    if o.name == "SM_DominionBay_Door_L":   # hull port (-Y) = +Y here; open centre |y| 3750
        o.location = (mx / 100, -(+3750) / 100, mz / 100)
    elif o.name == "SM_DominionBay_Door_R":
        o.location = (mx / 100, -(-3750) / 100, mz / 100)
    else:   # the emitter lives on the hull's brow, outside the interior map: park it beside the mouth for reference
        o.location = ((mx - 1200) / 100, 0, (mz + 3400) / 100)
    o.rotation_euler = (0, 0, math.pi)
# force field (pawn-only blocking plane) just inside the mouth
ff = L["force_field"]
bm = bmesh.new()
y0, y1 = ff["y"]; z0, z1 = ff["z"]; x = ff["x"]
vs = [bm.verts.new((x / 100, -y / 100, z / 100)) for (y, z) in ((y0, z0), (y1, z0), (y1, z1), (y0, z1))]
bm.faces.new(vs); me = bpy.data.meshes.new("FX_ForceField"); bm.to_mesh(me)
ffo = bpy.data.objects.new("FX_ForceField (BlockingVolume, pawn only)", me); C_EXT.objects.link(ffo)
fm = bpy.data.materials.new("M_ForceField_preview"); fm.use_nodes = True
fb = fm.node_tree.nodes["Principled BSDF"]; fb.inputs["Base Color"].default_value = (0.3, 0.7, 1, 1)
fb.inputs["Alpha"].default_value = 0.08; fb.inputs["Emission Color"].default_value = (0.3, 0.7, 1, 1); fb.inputs["Emission Strength"].default_value = 0.3
fm.blend_method = 'BLEND' if hasattr(fm, "blend_method") else None
me.materials.append(fm)

# --- BAY THRESHOLD marking (Lead N-061: the threshold read as 'unmarked') ----------------------------------------------
# A 400 cm (one kit cell) hazard-chevron floor band across the full 5000 cm mouth, x -7200..-6800, using the HAZARD band of
# T_DominionBay_Trim_BC (v 652..884 px), an emissive red-orange edge strip on its inner edge (x -6800) and emissive jamb +
# header strips framing the 5000 x 3500 opening. Builders: a DBuffer decal or a 1 cm plane with M_DominionBay_Trim.
C_TH = coll("G_threshold_marking", C_BAY)
tm = bpy.data.materials.new("M_Threshold_Hazard_preview"); tm.use_nodes = True
tn = tm.node_tree.nodes; tb = tn["Principled BSDF"]
ti = tn.new("ShaderNodeTexImage"); ti.image = bpy.data.images.load(os.path.join(PLANS, "textures", "T_DominionBay_Trim_BC.png"), check_existing=True)
tm.node_tree.links.new(ti.outputs[0], tb.inputs["Base Color"]); tb.inputs["Roughness"].default_value = 0.55
gm = bpy.data.materials.new("M_Threshold_Glow_preview"); gm.use_nodes = True
gb = gm.node_tree.nodes["Principled BSDF"]; gb.inputs["Base Color"].default_value = (0.05, 0.02, 0.01, 1)
gb.inputs["Emission Color"].default_value = (1.0, 0.14, 0.02, 1); gb.inputs["Emission Strength"].default_value = 10.0
def quad(name, pts_ue, mat, uvs=None):
    bm = bmesh.new(); vs = [bm.verts.new((x / 100, -y / 100, z / 100)) for (x, y, z) in pts_ue]
    f = bm.faces.new(vs); f.normal_update()
    if uvs:
        lay = bm.loops.layers.uv.new("UVMap")
        for l, uv in zip(f.loops, uvs): l[lay].uv = uv
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free(); me.materials.append(mat)
    o = bpy.data.objects.new(name, me); C_TH.objects.link(o); o["threshold"] = 1; return o
v0, v1 = 1 - 884 / 2048, 1 - 652 / 2048
ul = 5000.0 / 400.0 * (232 / 2048)          # keep the texel aspect: 232 px band <-> 400 cm
quad("Threshold_HazardBand_Floor", [(-7200, -2500, 0.6), (-6800, -2500, 0.6), (-6800, 2500, 0.6), (-7200, 2500, 0.6)], tm,
     [(0, v1), (0, v0), (ul, v0), (ul, v1)])
quad("Threshold_GlowStrip_Floor", [(-6800, -2500, 0.8), (-6770, -2500, 0.8), (-6770, 2500, 0.8), (-6800, 2500, 0.8)], gm)
for sgn in (-1, 1):   # jambs on the mouth edges, facing into the bay
    quad("Threshold_GlowStrip_Jamb_%s" % ("L" if sgn < 0 else "R"),
         [(-7190, sgn * 2500, 0), (-7190, sgn * 2500, 3500), (-7190, sgn * 2460, 3500), (-7190, sgn * 2460, 0)], gm)
quad("Threshold_GlowStrip_Header", [(-7190, -2500, 3460), (-7190, 2500, 3460), (-7190, 2500, 3500), (-7190, -2500, 3500)], gm)

# --- 180 cm figure (procedural mannequin) ---------------------------------------------------------------------------
C_FIG = coll("G_scale_figures", C_BAY)
def figure(x, y, yaw=0.0, name="Figure_180cm"):
    bm = bmesh.new()
    def cap(cx, cy, z0, z1, r):
        bmesh.ops.create_cone(bm, cap_ends=True, segments=16, radius1=r, radius2=r, depth=(z1 - z0),
                              matrix=Matrix.Translation((cx, cy, (z0 + z1) / 2)))
    cap(0, -0.11, 0.0, 0.86, 0.075); cap(0, 0.11, 0.0, 0.86, 0.075)          # legs
    cap(0, 0, 0.84, 1.46, 0.19)                                             # torso
    cap(0, -0.27, 0.80, 1.44, 0.055); cap(0, 0.27, 0.80, 1.44, 0.055)        # arms
    bmesh.ops.create_uvsphere(bm, u_segments=16, v_segments=10, radius=0.11, matrix=Matrix.Translation((0, 0, 1.69)))
    bmesh.ops.create_cone(bm, cap_ends=True, segments=8, radius1=0.05, radius2=0.05, depth=0.12, matrix=Matrix.Translation((0, 0, 1.52)))
    # trim to exactly 180 cm: head top = 1.80
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new(name, me); C_FIG.objects.link(o)
    o.location = (x / 100, -y / 100, 0); o.rotation_euler = (0, 0, math.radians(-yaw))
    m = bpy.data.materials.get("M_Figure") or bpy.data.materials.new("M_Figure"); m.use_nodes = True
    m.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.95, 0.75, 0.1, 1)
    m.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value = 0.5
    me.materials.append(m)
    return o
sx, sy, _ = L["start"]
figure(sx, sy, 180, "Figure_180cm_PilotStart")
figure(-6200, 600, 0, "Figure_180cm_AtMouth")
figure(-1700, -1800, 0, "Figure_180cm_DockConsole")

# --- ship footprint proxies on the pads -----------------------------------------------------------------------------
C_SHIP = coll("G_ship_proxies", C_BAY)
sl, sw, sh = L["ship_footprint"]["size"]
for key, pos in L["pads"].items():
    if key == "size": continue
    px, py = pos
    bm = bmesh.new(); bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.transform(bm, matrix=Matrix.Diagonal((sl / 100, sw / 100, sh / 100, 1)), verts=bm.verts)
    bmesh.ops.translate(bm, vec=(0, 0, sh / 200 + 0.005), verts=bm.verts)
    me = bpy.data.meshes.new("ShipProxy_" + key); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new("ShipProxy_Pad" + key, me); C_SHIP.objects.link(o)
    o.location = (px / 100, -py / 100, 0); o.display_type = 'WIRE'
    o["note"] = "largest squad ship footprint 524 x 488 x 230 cm, nose +X"

# --- interior lights as T5 places them, plus pocket work lights ------------------------------------------------------
C_L = coll("G_lights", C_BAY)
def plight(name, x, y, z, radius_cm, col, watts):
    l = bpy.data.lights.new(name, 'POINT'); l.energy = watts; l.color = col; l.shadow_soft_size = 0.5
    o = bpy.data.objects.new(name, l); o.location = (x / 100, -y / 100, z / 100); C_L.objects.link(o)
plight("T5_HangarOverhead", 0, 0, 1000, 3200, (0.8, 0.85, 1.0), 60000)
for i, x in enumerate((-6000, -4400, -2800)):
    plight("Pocket_Work_%d" % i, x, 0, 3300, 2600, (0.85, 0.9, 1.0), 90000)

bpy.ops.wm.save_as_mainfile(filepath=os.path.join(PLANS, "blockout", "DominionDockingBay_Blockout.blend"), compress=True)
bpy.ops.file.make_paths_relative(); bpy.ops.wm.save_mainfile(compress=True)
n = sum(1 for o in C_BAY.all_objects)
print("BLOCKOUT_OK objects", n, "kit sources", len(src))

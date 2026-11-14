"""Renders for the contact sheet. Neutral studio HDRI (Blender's bundled studio.exr), Cycles CPU, AgX.
  mode 'interior': opens blockout/DominionDockingBay_Blockout.blend; render-only ship stand-ins are imported from the
                   repo's squad / Null GLBs (Lancer on Pad A, Null fighters on B and C) and NOT saved.
  mode 'exterior': appends hull r2 (PR #72 branch) or r1 and places the bay pieces at SOCKET_Bay, closed and open.
blender -b --factory-startup --python render_views.py -- <mode> <repo_root> <plans_dir> <out_dir> [hull_blend_or_fbx] [samples]
"""
import bpy, sys, os, math, json
from mathutils import Vector
argv = sys.argv[sys.argv.index('--') + 1:]
MODE, REPO, PLANS, OUT = argv[0], argv[1], argv[2], argv[3]
HULL = argv[4] if len(argv) > 4 else ""
SAMPLES = int(argv[5]) if len(argv) > 5 else 64
sys.path.insert(0, os.path.join(PLANS, "source")); import render_common as R
os.makedirs(OUT, exist_ok=True)

def ue(x, y, z): return (x / 100.0, -y / 100.0, z / 100.0)

def import_glb(path, length_cm, at, yaw_deg, name):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    new = [o for o in bpy.data.objects if o not in before]
    meshes = [o for o in new if o.type == 'MESH']
    pts = [o.matrix_world @ Vector(c) for o in meshes for c in o.bound_box]
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    ext = hi - lo; L = max(ext.x, ext.y)
    root = bpy.data.objects.new(name, None); bpy.context.scene.collection.objects.link(root)
    for o in new:
        if o.parent is None: o.parent = root
    s = (length_cm / 100.0) / L
    centre = (lo + hi) / 2
    root.scale = (s, s, s)
    # move so the footprint centre sits on the target and the lowest point on the pad
    bpy.context.view_layer.update()
    root.location = Vector(ue(*at)) - Vector((centre.x * s, centre.y * s, lo.z * s)) + Vector((0, 0, 0.02))
    root.rotation_euler = (0, 0, math.radians(-yaw_deg))
    return root

if MODE == "interior":
    bpy.ops.wm.open_mainfile(filepath=os.path.join(PLANS, "blockout", "DominionDockingBay_Blockout.blend"))
    R.world(0.6, 0.8, bg=(0.02, 0.022, 0.03)); R.engine(SAMPLES, (1600, 900))
    sc = bpy.context.scene
    ships = os.path.join(REPO, "ArtSource/EngAI/Ships")
    lan = os.path.join(ships, "Lancer/mesh_r1/SM_Lancer.glb"); nf = os.path.join(ships, "NullFleet/Fighter/mesh_r1/SM_NullFighter.glb")
    if os.path.exists(lan) and os.path.getsize(lan) > 1000: import_glb(lan, 524, (-200, 0, 0), 0, "RenderOnly_Lancer_PadA")
    if os.path.exists(nf) and os.path.getsize(nf) > 1000:
        import_glb(nf, 700, (-2600, -1600, 0), 180, "RenderOnly_NullFighter_PadB")
        import_glb(nf, 700, (-2600, 1600, 0), 180, "RenderOnly_NullFighter_PadC")
    for o in bpy.data.objects:
        if o.name.startswith("ShipProxy_PadA") or o.name.startswith("ShipProxy_PadB") or o.name.startswith("ShipProxy_PadC"):
            o.hide_render = True
    def cutaway(on):
        for o in bpy.data.objects:
            k = o.get("ue_xyz_yaw")
            if not k: continue
            if o.get("is_ceiling"): o.hide_render = on
            # walls on the +Y (Unreal) side and the gantry there
            if o.get("kit_mesh") in ("SM_DI_Room_Wall", "SM_DI_Walkway_Ring") and k[1] > 1000 and abs(k[3]) in (90, 180): o.hide_render = on
    cutaway(True)
    c = R.camera("Cam_Cutaway", ue(-1200, 7600, 5200), ue(-3600, -300, 300), lens=24, clip_end=500)
    R.shot(c, os.path.join(OUT, "bo_cutaway.png"))
    c = R.camera("Cam_Cutaway2", ue(-9500, 5200, 4600), ue(-2500, -400, 200), lens=24, clip_end=500)
    R.shot(c, os.path.join(OUT, "bo_cutaway_mouth.png"))
    for o in bpy.data.objects:
        if o.get("is_ceiling"): o.hide_render = True
    c = R.camera("Cam_Top", ue(-3000, 0, 9000), ue(-3000, 0, 0), ortho=96, clip_end=500)
    c.rotation_euler = (0, 0, 0)
    R.shot(c, os.path.join(OUT, "bo_top.png"), (1800, 900))
    cutaway(False)
    sc.render.resolution_x, sc.render.resolution_y = 1600, 900
    c = R.camera("Cam_Eye_Start", ue(400, 650, 170), ue(-7200, -300, 900), lens=22, clip_end=500)
    R.shot(c, os.path.join(OUT, "bo_eye_start.png"))
    c = R.camera("Cam_Eye_Mouth", ue(-6900, -1500, 170), ue(-200, 300, 300), lens=22, clip_end=500)
    R.shot(c, os.path.join(OUT, "bo_eye_mouth.png"))

elif MODE == "exterior":
    bpy.ops.wm.read_factory_settings(use_empty=True)
    R.world(0.45, 2.4, bg=(0.03, 0.03, 0.04)); R.engine(SAMPLES, (1600, 900))
    if HULL.endswith(".blend"):
        with bpy.data.libraries.load(HULL, link=False) as (s, d):
            d.objects = ["SM_DominionMothership"]
        for o in d.objects: bpy.context.scene.collection.objects.link(o)
        tex = os.path.dirname(HULL)
        for im in bpy.data.images:
            base = os.path.basename(im.filepath).replace("_N_OpenGL", "_N")
            p = os.path.join(tex, base)
            if os.path.exists(p): im.filepath = p; im.reload()
    else:
        bpy.ops.import_scene.fbx(filepath=HULL)
        for o in list(bpy.data.objects):
            if o.name.startswith("UCX_"): bpy.data.objects.remove(o)
    with bpy.data.libraries.load(os.path.join(PLANS, "mesh", "DominionBay_Exterior.blend"), link=False) as (s, d):
        d.objects = [n for n in s.objects if n.startswith("SM_DominionBay_")]
    for o in d.objects: bpy.context.scene.collection.objects.link(o)
    L_, R_, E_ = (bpy.data.objects[n] for n in ("SM_DominionBay_Door_L", "SM_DominionBay_Door_R", "SM_DominionBay_TractorEmitter"))
    E_.location = ue(31200, 0, 1400); E_.rotation_euler = (0, 0, 0)
    # a 180 cm figure on the sill for scale
    bpy.ops.mesh.primitive_cylinder_add(radius=0.2, depth=1.8, location=ue(29600, 900, -3750 + 90))
    fig = bpy.context.object; fig.name = "Figure_180cm"
    m = bpy.data.materials.new("Fig"); m.use_nodes = True; m.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.95, 0.75, 0.1, 1)
    m.node_tree.nodes["Principled BSDF"].inputs["Emission Color"].default_value = (0.95, 0.75, 0.1, 1); m.node_tree.nodes["Principled BSDF"].inputs["Emission Strength"].default_value = 2
    fig.data.materials.append(m)
    # light the bay interior a little so the open state reads
    l = bpy.data.lights.new("BayFill", 'AREA'); l.energy = 2.5e5; l.size = 40; l.color = (1, 0.85, 0.75)
    lo = bpy.data.objects.new("BayFill", l); lo.location = ue(26000, 0, -400); lo.rotation_euler = (0, 0, 0); bpy.context.scene.collection.objects.link(lo)
    sun = bpy.data.lights.new("Key", 'SUN'); sun.energy = 3.0; so = bpy.data.objects.new("Key", sun)
    so.rotation_euler = (math.radians(55), 0, math.radians(140)); bpy.context.scene.collection.objects.link(so)
    for state, open01 in (("closed", 0.0), ("open", 1.0)):
        L_.location = ue(30000, -2500 * (0.5 + open01), -2000); R_.location = ue(30000, 2500 * (0.5 + open01), -2000)
        c = R.camera("Cam_34_" + state, ue(52000, 17000, 6000), ue(29000, 0, -1500), lens=50, clip_end=3000)
        R.shot(c, os.path.join(OUT, "ext_%s_34.png" % state))
        c = R.camera("Cam_front_" + state, ue(60000, 0, -1500), ue(30000, 0, -1200), lens=60, clip_end=3000)
        R.shot(c, os.path.join(OUT, "ext_%s_front.png" % state))
    c = R.camera("Cam_emitter", ue(36000, 3500, 1600), ue(31200, 0, 1300), lens=50, clip_end=3000)
    R.shot(c, os.path.join(OUT, "ext_emitter.png"))
elif MODE == "pieces":
    # studio turnaround of the exported pieces alone (live modifiers), neutral studio HDRI, 180 cm figure for scale
    bpy.ops.wm.read_factory_settings(use_empty=True)
    R.world(1.0, 0.6, bg=(0.16, 0.165, 0.175)); R.engine(SAMPLES, (1600, 900))
    with bpy.data.libraries.load(os.path.join(PLANS, "mesh", "DominionBay_Exterior.blend"), link=False) as (s, d):
        d.objects = [n for n in s.objects if n.startswith("SM_DominionBay_")]
    for o in d.objects: bpy.context.scene.collection.objects.link(o)
    L_, R_, E_ = (bpy.data.objects[n] for n in ("SM_DominionBay_Door_L", "SM_DominionBay_Door_R", "SM_DominionBay_TractorEmitter"))
    # doors closed side by side, outer face toward +X (Blender +X = Unreal +X), bottom on the floor
    L_.location = ue(0, -1250, 1750); R_.location = ue(0, 1250, 1750); L_.rotation_euler = R_.rotation_euler = (0, 0, 0)
    E_.location = ue(400, 0, 4400); E_.rotation_euler = (0, 0, 0)
    bpy.ops.mesh.primitive_plane_add(size=200, location=(0, 0, 0)); fl = bpy.context.object
    fm = bpy.data.materials.new("Floor"); fm.use_nodes = True; fb = fm.node_tree.nodes["Principled BSDF"]
    fb.inputs["Base Color"].default_value = (0.18, 0.18, 0.19, 1); fb.inputs["Roughness"].default_value = 0.8; fl.data.materials.append(fm)
    # 180 cm figure: capsule body + head (yellow), at the door foot
    fig_m = bpy.data.materials.new("Fig"); fig_m.use_nodes = True
    fig_m.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.95, 0.72, 0.08, 1)
    bpy.ops.mesh.primitive_cylinder_add(radius=0.2, depth=1.55, location=ue(700, -2200, 77.5)); f1 = bpy.context.object
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.12, location=ue(700, -2200, 168)); f2 = bpy.context.object
    for f in (f1, f2): f.data.materials.append(fig_m); f.name = "Figure_180cm"
    sun = bpy.data.lights.new("Key", 'SUN'); sun.energy = 2.5; so = bpy.data.objects.new("Key", sun)
    so.rotation_euler = (math.radians(50), 0, math.radians(60)); bpy.context.scene.collection.objects.link(so)
    c = R.camera("Cam_pieces_34", ue(8200, -6000, 1800), ue(0, -300, 2000), lens=32, clip_end=500)
    R.shot(c, os.path.join(OUT, "pc_pieces_34.png"))
    if os.environ.get("PIECES_ONLY_34"): print("RENDER_DONE", MODE); sys.exit(0)
    c = R.camera("Cam_pieces_detail", ue(900, 300, 400), ue(0, 900, 150), lens=35, clip_end=500)
    R.shot(c, os.path.join(OUT, "pc_door_detail.png"))
    c = R.camera("Cam_emitter_detail", ue(1700, -1100, 5200), ue(400, 0, 4400), lens=40, clip_end=500)
    R.shot(c, os.path.join(OUT, "pc_emitter_detail.png"))
print("RENDER_DONE", MODE)

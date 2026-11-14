"""Cycles verification renders for SM_DominionMothership mesh_r1.
usage: blender -b BUILD.blend --python render_verify.py -- TEX2K_DIR OUT_DIR SAMPLES shot[,shot...]"""
import bpy, sys, os, math
from mathutils import Vector
argv = sys.argv[sys.argv.index("--") + 1:]
TEX, OUT, SAMPLES, SHOTS = argv[0], argv[1], int(argv[2]), argv[3].split(",")
os.makedirs(OUT, exist_ok=True)
sc = bpy.context.scene
obj = bpy.data.objects["SM_DominionMothership"]
for o in sc.objects:
    if o.name.startswith("UCX_"): o.hide_render = True
# --- preview material: atlas + world-space tiling detail layer (Detail_ORM cavity as bump, 8 m tile)
mat = obj.data.materials[0]; nt = mat.node_tree
bsdf = [n for n in nt.nodes if n.type == 'BSDF_PRINCIPLED'][0]
nm = [n for n in nt.nodes if n.type == 'NORMAL_MAP'][0]
bsdf.inputs["Emission Strength"].default_value = 4.0
tc = nt.nodes.new("ShaderNodeTexCoord"); mp = nt.nodes.new("ShaderNodeMapping"); mp.inputs["Scale"].default_value = (1 / 8.0,) * 3
nt.links.new(tc.outputs["Object"], mp.inputs[0])
dimg = nt.nodes.new("ShaderNodeTexImage"); dimg.image = bpy.data.images.load(os.path.join(TEX, "T_DominionMothership_Detail_ORM.png"))
dimg.image.colorspace_settings.name = "Non-Color"; dimg.projection = 'BOX'; dimg.projection_blend = 0.25
nt.links.new(mp.outputs[0], dimg.inputs[0])
sep = nt.nodes.new("ShaderNodeSeparateColor"); nt.links.new(dimg.outputs[0], sep.inputs[0])
bump = nt.nodes.new("ShaderNodeBump"); bump.inputs["Strength"].default_value = 0.35; bump.inputs["Distance"].default_value = 0.04
nt.links.new(sep.outputs[0], bump.inputs["Height"]); nt.links.new(nm.outputs[0], bump.inputs["Normal"]); nt.links.new(bump.outputs[0], bsdf.inputs["Normal"])
bcn = [l.from_node for l in nt.links if l.to_socket == bsdf.inputs["Base Color"]][0]
mix = nt.nodes.new("ShaderNodeMix"); mix.data_type = 'RGBA'; mix.blend_type = 'MULTIPLY'; mix.inputs["Factor"].default_value = 0.45
nt.links.new(bcn.outputs[0], mix.inputs[6]); nt.links.new(sep.outputs[0], mix.inputs[7]); nt.links.new(mix.outputs[2], bsdf.inputs["Base Color"])
# --- world & lights (v3-like mauve studio backdrop)
w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True
w.node_tree.nodes["Background"].inputs[0].default_value = (0.075, 0.062, 0.072, 1); w.node_tree.nodes["Background"].inputs[1].default_value = 1.0
def light(name, kind, rot, energy, color=(1, 1, 1), size=None, loc=(0, 0, 0)):
    d = bpy.data.lights.new(name, kind); d.energy = energy; d.color = color
    if size is not None and kind == 'SUN': d.angle = size
    o = bpy.data.objects.new(name, d); sc.collection.objects.link(o); o.rotation_euler = [math.radians(a) for a in rot]; o.location = loc; return o
light("key", 'SUN', (50, 0, 150), 4.0, (1.0, 0.96, 0.92), 0.05)
light("fill", 'SUN', (65, 0, -40), 0.8, (0.75, 0.8, 1.0), 0.2)
light("rim", 'SUN', (110, 0, 20), 1.2, (1.0, 0.85, 0.8), 0.1)
sc.render.engine = 'CYCLES'; sc.cycles.device = 'CPU'; sc.cycles.samples = SAMPLES; sc.cycles.use_denoising = True
sc.cycles.max_bounces = 4; sc.render.film_transparent = False
sc.view_settings.view_transform = 'AgX'; sc.view_settings.look = 'None'
# glare for emissive glow
sc.use_nodes = True; cnt = sc.node_tree; cnt.nodes.clear()
rl = cnt.nodes.new("CompositorNodeRLayers"); gl = cnt.nodes.new("CompositorNodeGlare"); co = cnt.nodes.new("CompositorNodeComposite")
gl.glare_type = 'FOG_GLOW'; gl.quality = 'HIGH'; gl.threshold = 1.0; gl.size = 6
cnt.links.new(rl.outputs[0], gl.inputs[0]); cnt.links.new(gl.outputs[0], co.inputs[0])
# --- helper ghosts for clearance shots (render only)
def ghost_sphere(name, loc, r, color, wire=0.15):
    bpy.ops.mesh.primitive_uv_sphere_add(radius=r, location=loc, segments=32, ring_count=16)
    o = bpy.context.active_object; o.name = name
    m = o.modifiers.new("w", 'WIREFRAME'); m.thickness = wire
    mt = bpy.data.materials.new(name); mt.use_nodes = True
    b = mt.node_tree.nodes["Principled BSDF"]; b.inputs["Base Color"].default_value = color; b.inputs["Emission Color"].default_value = color; b.inputs["Emission Strength"].default_value = 3
    o.data.materials.append(mt); return o
def cam(name, loc, target, lens=35, ortho=None, res=(1600, 900)):
    c = bpy.data.cameras.new(name); c.clip_start = 1; c.clip_end = 5000
    if ortho: c.type = 'ORTHO'; c.ortho_scale = ortho
    else: c.lens = lens
    o = bpy.data.objects.new(name, c); sc.collection.objects.link(o); o.location = loc
    d = Vector(target) - Vector(loc); o.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
    sc.camera = o; sc.render.resolution_x, sc.render.resolution_y = res; sc.render.resolution_percentage = 100
    return o
SH = {
    "hero":   dict(loc=(520, 760, 330), target=(-10, 0, -20), lens=40),
    "side":   dict(loc=(0, -1200, 5), target=(0, 0, 5), ortho=720),
    "top":    dict(loc=(0, 0, 1200), target=(0, 0.001, 0), ortho=760, res=(1400, 1400)),
    "front":  dict(loc=(1300, 0, 20), target=(0, 0, 20), ortho=700),
    "rear":   dict(loc=(-1300, 0, 20), target=(0, 0, 20), ortho=700),
    "trench": dict(loc=(-30, 14, 72), target=(170, -2, 60), lens=16),
    "bay":    dict(loc=(395, -60, 2), target=(272, 0, -20), lens=28),
    "scale":  dict(loc=(352, -38, -24), target=(296, 0, -21), lens=34),
    "scale_close": dict(loc=(303.5, -9.5, -35.9), target=(299, -4.6, -36.7), lens=30, res=(900, 900)),
    "generator": dict(loc=(390, -150, 60), target=(275, -275, -5), lens=30),
}
ghosts = []
for shot in SHOTS:
    for g in ghosts: g.hide_render = True
    if shot == "trench":
        ghosts.append(ghost_sphere("reactor_clear", (165, 0, 65), 14, (0.2, 0.9, 1.0, 1)))
        for (x, y, z) in ((-150, 17, 77), (-60, -17, 77), (30, 17, 77), (120, -17, 77)):   # Blender Y = -UE Y
            ghosts.append(ghost_sphere("tt", (x, y, z), 2.5, (1, 0.1, 0.1, 1), 0.4))
    if shot == "generator":
        for (x, y, z) in ((275, -275, 0),):
            ghosts.append(ghost_sphere("dome_clear", (x, y, 12), 18, (0.2, 0.6, 1.0, 1), 0.2))
        import math as _m
        T = Vector((-_m.sin(_m.radians(-45)), _m.cos(_m.radians(-45)), 0))
        for k in (-1, 1):
            p = Vector((275, -275, 15)) + T * 30 * k; ghosts.append(ghost_sphere("gt", p, 2.5, (1, 0.1, 0.1, 1), 0.4))
    if shot in ("scale", "scale_close") and not bpy.data.objects.get("pilot_180cm"):
        fm = bpy.data.materials.new("pilot"); fm.use_nodes = True; pb = fm.node_tree.nodes["Principled BSDF"]
        pb.inputs["Base Color"].default_value = (0.85, 0.9, 1.0, 1); pb.inputs["Emission Color"].default_value = (0.4, 0.8, 1.0, 1); pb.inputs["Emission Strength"].default_value = 1.5
        parts = []
        X0, Y0, Z0 = 299.0, -4.6, -37.25           # standing on the bay deck, just inside the 50 x 35 m mouth
        for (lo, hi) in (((-0.09, -0.2, 0), (0.09, -0.04, 0.85)), ((-0.09, 0.04, 0), (0.09, 0.2, 0.85)), ((-0.12, -0.22, 0.85), (0.12, 0.22, 1.5)),
                         ((-0.07, -0.33, 0.8), (0.07, -0.23, 1.45)), ((-0.07, 0.23, 0.8), (0.07, 0.33, 1.45)), ((-0.05, -0.05, 1.5), (0.05, 0.05, 1.56))):
            bpy.ops.mesh.primitive_cube_add(size=1, location=(X0 + (lo[0] + hi[0]) / 2, Y0 + (lo[1] + hi[1]) / 2, Z0 + (lo[2] + hi[2]) / 2))
            c = bpy.context.active_object; c.scale = (hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]); parts.append(c)
        bpy.ops.mesh.primitive_uv_sphere_add(radius=0.12, location=(X0, Y0, Z0 + 1.68)); parts.append(bpy.context.active_object)   # head top = 1.80 m
        for p in parts: p.data.materials.append(fm)
        bpy.ops.object.select_all(action='DESELECT')
        for p in parts: p.select_set(True)
        bpy.context.view_layer.objects.active = parts[0]; bpy.ops.object.join(); parts[0].name = "pilot_180cm"
        dims = parts[0].dimensions; print("PILOT_HEIGHT_M", round(dims.z, 3))
    if shot == "bay":
        ghosts.append(ghost_sphere("launch", (330, 0, -20), 2.5, (0.2, 1.0, 0.3, 1), 0.4))
    kw = SH[shot]; cam(shot, **kw)
    sc.render.filepath = os.path.join(OUT, f"render_{shot}.png")
    bpy.ops.render.render(write_still=True)
    print("RENDERED", shot)

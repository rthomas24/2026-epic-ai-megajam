"""Studio-HDRI verification renders for SM_DominionShieldGenerator + Core.
usage: blender -b WORKSHOP.blend --python render_verify.py -- OUT_DIR SAMPLES
"""
import bpy, sys, os, math, bmesh
from mathutils import Vector, Matrix

argv = sys.argv[sys.argv.index("--") + 1:]
OUT = argv[0] if argv else "/tmp/dsg_r1/renders"
SAMPLES = int(argv[1]) if len(argv) > 1 else 32
SHOT_FILTER = set(argv[2].split(",")) if len(argv) > 2 else None
os.makedirs(OUT, exist_ok=True)
sc = bpy.context.scene

pylon = bpy.data.objects["SM_DominionShieldGenerator"]
core = bpy.data.objects["SM_DominionShieldGeneratorCore"]
for o in sc.objects:
    if o.name.startswith("UCX_"):
        o.hide_render = True
        o.hide_set(True)

# bump emission a little for stills
for obj, strength in ((pylon, 4.5), (core, 8.0)):
    mat = obj.data.materials[0]
    bsdf = [n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED"][0]
    bsdf.inputs["Emission Strength"].default_value = strength

HDRI = "/home/box/.local/blender-4.5.3-linux-x64/4.5/datafiles/studiolights/world/studio.exr"
w = bpy.data.worlds.new("W"); sc.world = w; w.use_nodes = True
wnt = w.node_tree; wnt.nodes.clear()
env = wnt.nodes.new("ShaderNodeTexEnvironment"); env.image = bpy.data.images.load(HDRI)
bg = wnt.nodes.new("ShaderNodeBackground"); bg.inputs["Strength"].default_value = 1.2
wout = wnt.nodes.new("ShaderNodeOutputWorld")
texc = wnt.nodes.new("ShaderNodeTexCoord"); mp = wnt.nodes.new("ShaderNodeMapping")
mp.inputs["Rotation"].default_value = (0, 0, math.radians(35))
wnt.links.new(texc.outputs["Generated"], mp.inputs[0])
wnt.links.new(mp.outputs[0], env.inputs[0])
wnt.links.new(env.outputs[0], bg.inputs[0])
bg_cam = wnt.nodes.new("ShaderNodeBackground")
bg_cam.inputs["Color"].default_value = (0.16, 0.155, 0.165, 1)
bg_cam.inputs["Strength"].default_value = 1.0
lp = wnt.nodes.new("ShaderNodeLightPath")
mixw = wnt.nodes.new("ShaderNodeMixShader")
wnt.links.new(lp.outputs["Is Camera Ray"], mixw.inputs["Fac"])
wnt.links.new(bg.outputs[0], mixw.inputs[1])
wnt.links.new(bg_cam.outputs[0], mixw.inputs[2])
wnt.links.new(mixw.outputs[0], wout.inputs[0])

def light(name, kind, rot, energy, color=(1, 1, 1), size=None, loc=(0, 0, 0)):
    d = bpy.data.lights.new(name, kind); d.energy = energy; d.color = color
    if size is not None and kind == "SUN":
        d.angle = size
    o = bpy.data.objects.new(name, d); sc.collection.objects.link(o)
    o.rotation_euler = [math.radians(a) for a in rot]; o.location = loc
    return o

light("key", "SUN", (50, 0, 140), 1.7, (1.0, 0.97, 0.94), 0.08)
light("fill", "SUN", (70, 0, -40), 0.45, (0.85, 0.9, 1.0), 0.25)
light("rim", "SUN", (110, 0, 20), 1.0, (1.0, 0.88, 0.82), 0.12)

sc.render.engine = "CYCLES"; sc.cycles.device = "CPU"
sc.cycles.samples = SAMPLES; sc.cycles.use_denoising = True
sc.cycles.max_bounces = 4; sc.render.film_transparent = False
sc.view_settings.view_transform = "AgX"; sc.view_settings.look = "None"
sc.render.image_settings.file_format = "JPEG"
sc.render.image_settings.quality = 95
sc.use_nodes = True; cnt = sc.node_tree; cnt.nodes.clear()
rl = cnt.nodes.new("CompositorNodeRLayers")
gl = cnt.nodes.new("CompositorNodeGlare")
co = cnt.nodes.new("CompositorNodeComposite")
gl.glare_type = "FOG_GLOW"; gl.quality = "HIGH"; gl.threshold = 0.85; gl.size = 7
cnt.links.new(rl.outputs[0], gl.inputs[0]); cnt.links.new(gl.outputs[0], co.inputs[0])

# --- helpers ---
def new_mat(name, color, emission=0.0, rough=0.45, metal=0.0):
    m = bpy.data.materials.new(name); m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (*color[:3], 1)
    if emission:
        b.inputs["Emission Color"].default_value = (*color[:3], 1)
        b.inputs["Emission Strength"].default_value = emission
    b.inputs["Roughness"].default_value = rough
    b.inputs["Metallic"].default_value = metal
    return m

def add_cyl(name, loc, r, depth, mat, segs=48):
    bpy.ops.mesh.primitive_cylinder_add(radius=r, depth=depth, location=loc, vertices=segs)
    o = bpy.context.active_object; o.name = name
    o.data.materials.append(mat)
    return o

# Proxy outrigger plinth matching mesh_r2: r=24 m, top Z=-0.5, height 8.5, bottom Z=-9.0
plinth_mat = new_mat("plinth", (0.18, 0.19, 0.21), 0, 0.5, 0.55)
lip_mat = new_mat("plinth_lip", (0.12, 0.12, 0.14), 0, 0.4, 0.7)
amber_mat = new_mat("plinth_amber", (1.0, 0.35, 0.08), 6.0, 0.3, 0.0)
red_mat = new_mat("plinth_red", (0.55, 0.08, 0.1), 0, 0.4, 0.35)
plinth = add_cyl("PROXY_Plinth", (0, 0, -9 + 8.5 / 2), 24.0, 8.5, plinth_mat, 64)
lip = add_cyl("PROXY_PlinthLip", (0, 0, -4.2 + 1.1), 24.7, 2.2, lip_mat, 64)
# amber lights around the rim (match r2: 20 lights at r 24.75, Z -1.6)
for k in range(20):
    a = 2 * math.pi * k / 20
    bpy.ops.mesh.primitive_cube_add(size=1, location=(math.cos(a) * 24.75, math.sin(a) * 24.75, -1.6))
    o = bpy.context.active_object; o.scale = (1.6, 0.25, 0.4)
    o.rotation_euler = (0, 0, a + math.pi / 2)
    o.data.materials.append(amber_mat)
    o.name = f"PROXY_Amber_{k}"
# short stub of outrigger arm (visual only, coming in from -X)
arm_mat = plinth_mat
bpy.ops.mesh.primitive_cube_add(size=1, location=(-20, 0, -6))
arm = bpy.context.active_object; arm.name = "PROXY_Arm"
arm.scale = (16, 6, 5); arm.rotation_euler = (0, math.radians(8), 0)
arm.data.materials.append(arm_mat)
# keep-clear dome ghost (r=18 m, Z 0..18 shown as a wire hemisphere-ish sphere)
bpy.ops.mesh.primitive_uv_sphere_add(radius=18, location=(0, 0, 0), segments=32, ring_count=16)
dome = bpy.context.active_object; dome.name = "PROXY_Dome"
wm = dome.modifiers.new("w", "WIREFRAME"); wm.thickness = 0.08
dome_mat = new_mat("dome", (0.25, 0.55, 1.0), 2.0, 0.3, 0)
dome.data.materials.append(dome_mat)
dome.hide_render = True

# 180 cm pilot standing on the plinth, just outside the foot ring (r=7.44)
def make_pilot(loc):
    fm = new_mat("pilot", (0.85, 0.9, 1.0), 1.6, 0.4, 0)
    parts = []
    X0, Y0, Z0 = loc
    boxes = [
        ((-0.09, -0.20, 0), (0.09, -0.04, 0.85)),
        ((-0.09, 0.04, 0), (0.09, 0.20, 0.85)),
        ((-0.12, -0.22, 0.85), (0.12, 0.22, 1.50)),
        ((-0.07, -0.33, 0.80), (0.07, -0.23, 1.45)),
        ((-0.07, 0.23, 0.80), (0.07, 0.33, 1.45)),
        ((-0.05, -0.05, 1.50), (0.05, 0.05, 1.56)),
    ]
    for lo, hi in boxes:
        bpy.ops.mesh.primitive_cube_add(
            size=1,
            location=(X0 + (lo[0] + hi[0]) / 2, Y0 + (lo[1] + hi[1]) / 2, Z0 + (lo[2] + hi[2]) / 2),
        )
        c = bpy.context.active_object
        c.scale = (hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2])
        c.data.materials.append(fm)
        parts.append(c)
    bpy.ops.mesh.primitive_uv_sphere_add(radius=0.12, location=(X0, Y0, Z0 + 1.68))
    h = bpy.context.active_object; h.data.materials.append(fm); parts.append(h)
    bpy.ops.object.select_all(action="DESELECT")
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    p = bpy.context.active_object; p.name = "pilot_180cm"
    return p

pilot = make_pilot((8.6, 0.0, -0.50))  # feet on plinth top, head top = 1.30 m world
pilot.hide_render = True

# Destroyed stump: duplicate pylon, scale Z * 0.4, hide core
stump = pylon.copy(); stump.data = pylon.data  # same mesh, visual only
sc.collection.objects.link(stump)
stump.name = "STUMP_ShowDestroyed"
stump.scale = (1, 1, 0.4)
stump.hide_render = True

# Clay / edge materials (override)
clay_mat = new_mat("clay", (0.62, 0.60, 0.58), 0, 0.55, 0)
edge_mat = bpy.data.materials.new("edge"); edge_mat.use_nodes = True
nt = edge_mat.node_tree; nt.nodes.clear()
out = nt.nodes.new("ShaderNodeOutputMaterial")
em = nt.nodes.new("ShaderNodeEmission")
lw = nt.nodes.new("ShaderNodeLayerWeight"); lw.inputs["Blend"].default_value = 0.35
cr = nt.nodes.new("ShaderNodeValToRGB")
cr.color_ramp.elements[0].position = 0.0
cr.color_ramp.elements[0].color = (0.08, 0.08, 0.09, 1)
cr.color_ramp.elements[1].position = 0.85
cr.color_ramp.elements[1].color = (0.95, 0.92, 0.85, 1)
nt.links.new(lw.outputs["Facing"], cr.inputs[0])
nt.links.new(cr.outputs[0], em.inputs[0])
em.inputs["Strength"].default_value = 1.2
nt.links.new(em.outputs[0], out.inputs[0])

# UCX preview material
ucx_mat = new_mat("ucx", (0.15, 0.95, 0.4), 3.0, 0.3, 0)

def cam(name, loc, target, lens=35, ortho=None, res=(1600, 900)):
    c = bpy.data.cameras.new(name); c.clip_start = 0.05; c.clip_end = 400
    if ortho:
        c.type = "ORTHO"; c.ortho_scale = ortho
    else:
        c.lens = lens
    o = bpy.data.objects.new(name, c); sc.collection.objects.link(o); o.location = loc
    d = Vector(target) - Vector(loc)
    o.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
    sc.camera = o
    sc.render.resolution_x, sc.render.resolution_y = res
    sc.render.resolution_percentage = 100
    return o

def hide_proxy(hide=True):
    for o in sc.objects:
        if o.name.startswith("PROXY_"):
            o.hide_render = hide

def set_override(objs, mat_or_none):
    for o in objs:
        if mat_or_none is None:
            o.data.materials.clear()
            # cannot easily restore; we only use copies? Instead swap slot 0
        else:
            # add as override via object slots? just append and set use...
            pass

SHOTS = {
    "hero": dict(loc=(24, 28, 13), target=(0, 0, 7.5), lens=32, res=(1600, 1200)),
    "hero_low": dict(loc=(16, 18, 3.2), target=(0, 0, 6), lens=32, res=(1600, 1000)),
    "front": dict(loc=(40, 0, 9.3), target=(0, 0, 9.3), ortho=22, res=(1200, 1600)),
    "side": dict(loc=(0, -40, 9.3), target=(0, 0, 9.3), ortho=22, res=(1200, 1600)),
    "top": dict(loc=(0, 0, 40), target=(0, 0.001, 0), ortho=22, res=(1400, 1400)),
    "detail": dict(loc=(6.5, 8.5, 12.2), target=(0, 0, 11.0), lens=50, res=(1600, 1000)),
    "detail_collar": dict(loc=(10, 8, 3.2), target=(5, 0, 2.6), lens=50, res=(1600, 1000)),
    "scale": dict(loc=(14, 10, 2.4), target=(5, 0, 1.2), lens=35, res=(1600, 1000)),
    "stump": dict(loc=(16, 20, 6), target=(0, 0, 3.2), lens=40, res=(1600, 1000)),
    "plinth": dict(loc=(38, 32, 14), target=(0, 0, 4), lens=35, res=(1600, 1000)),
    "clay": dict(loc=(24, 28, 13), target=(0, 0, 7.5), lens=32, res=(1600, 1200)),
    "edge": dict(loc=(24, 28, 13), target=(0, 0, 7.5), lens=32, res=(1600, 1200)),
    "ucx": dict(loc=(24, 28, 13), target=(0, 0, 7.5), lens=32, res=(1600, 1200)),
}

# store original materials
orig = {o.name: [s.material for s in o.material_slots] for o in (pylon, core)}

def restore_mats():
    for o, mats in orig.items():
        obj = bpy.data.objects[o]
        obj.data.materials.clear()
        for m in mats:
            obj.data.materials.append(m)

def render_shot(name, **kw):
    loc = kw.pop; 
    # handled below

for name, kw in SHOTS.items():
    if SHOT_FILTER and name not in SHOT_FILTER:
        continue
    # reset visibility
    pylon.hide_render = False
    core.hide_render = False
    stump.hide_render = True
    pilot.hide_render = True
    dome.hide_render = True
    hide_proxy(hide=(name not in ("plinth", "scale")))
    if name == "scale":
        # show a cropped bit of plinth under the feet, not the huge 48 m pad dominating
        hide_proxy(True)
        plinth.hide_render = False
        lip.hide_render = False
        pilot.hide_render = False
    if name == "plinth":
        hide_proxy(False)
        dome.hide_render = False
        pilot.hide_render = False
    if name == "stump":
        pylon.hide_render = True
        core.hide_render = True
        stump.hide_render = False
        hide_proxy(True)
        plinth.hide_render = False
        lip.hide_render = False
    if name == "ucx":
        core.hide_render = True
        pylon.hide_render = False
        # show UCX as wire-ish solids
        for o in sc.objects:
            if o.name.startswith("UCX_"):
                o.hide_render = False
                o.hide_set(False)
                if not o.data.materials:
                    o.data.materials.append(ucx_mat)
                # thin solid, slightly transparent feel via emission
        hide_proxy(True)
    else:
        for o in sc.objects:
            if o.name.startswith("UCX_"):
                o.hide_render = True
                o.hide_set(True)

    restore_mats()
    if name == "clay":
        pylon.data.materials.clear(); pylon.data.materials.append(clay_mat)
        core.data.materials.clear(); core.data.materials.append(clay_mat)
    if name == "edge":
        pylon.data.materials.clear(); pylon.data.materials.append(edge_mat)
        core.data.materials.clear(); core.data.materials.append(edge_mat)

    cam(name, kw["loc"], kw["target"], lens=kw.get("lens", 35),
        ortho=kw.get("ortho"), res=kw.get("res", (1600, 900)))
    path = os.path.join(OUT, f"render_{name}.jpg")
    sc.render.filepath = path
    print("RENDER", name, flush=True)
    bpy.ops.render.render(write_still=True)
    print("wrote", path, flush=True)

restore_mats()
print("ALL RENDERS DONE", flush=True)

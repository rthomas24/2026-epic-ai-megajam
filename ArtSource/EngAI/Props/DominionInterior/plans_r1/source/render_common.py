"""Shared render setup: neutral studio HDRI (Blender's bundled studio.exr, rotated, strength 1), Cycles CPU, filmic AgX."""
import bpy, math, os
HDRI = os.path.join(os.path.dirname(bpy.app.binary_path), "%d.%d" % bpy.app.version[:2], "datafiles", "studiolights", "world", "studio.exr")
def world(strength=1.0, rot=0.0, bg=None):
    sc = bpy.context.scene
    w = bpy.data.worlds.new("W_NeutralStudio"); sc.world = w; w.use_nodes = True
    N = w.node_tree.nodes; L = w.node_tree.links
    env = N.new("ShaderNodeTexEnvironment"); env.image = bpy.data.images.load(HDRI)
    mp = N.new("ShaderNodeMapping"); tc = N.new("ShaderNodeTexCoord"); mp.inputs["Rotation"].default_value[2] = rot
    L.new(tc.outputs["Generated"], mp.inputs[0]); L.new(mp.outputs[0], env.inputs[0])
    bgn = N["Background"]; L.new(env.outputs[0], bgn.inputs[0]); bgn.inputs[1].default_value = strength
    if bg is not None:   # camera sees a flat grey, lighting stays the HDRI
        lp = N.new("ShaderNodeLightPath"); mix = N.new("ShaderNodeMixShader"); flat = N.new("ShaderNodeBackground")
        flat.inputs[0].default_value = (*bg, 1); flat.inputs[1].default_value = 1
        out = N["World Output"]
        L.new(lp.outputs["Is Camera Ray"], mix.inputs[0]); L.new(bgn.outputs[0], mix.inputs[1]); L.new(flat.outputs[0], mix.inputs[2])
        L.new(mix.outputs[0], out.inputs[0])
def engine(samples=48, res=(1600, 900)):
    sc = bpy.context.scene
    sc.render.engine = 'CYCLES'; sc.cycles.device = 'CPU'; sc.cycles.samples = samples; sc.cycles.use_denoising = True
    sc.cycles.max_bounces = 6
    sc.render.resolution_x, sc.render.resolution_y = res; sc.render.resolution_percentage = 100
    sc.view_settings.view_transform = 'AgX'; sc.view_settings.look = 'AgX - Medium High Contrast'
    sc.render.film_transparent = False
def camera(name, loc, target, lens=35, ortho=None, clip_end=2000):
    from mathutils import Vector
    c = bpy.data.cameras.new(name); c.lens = lens; c.clip_end = clip_end; c.clip_start = 0.05
    if ortho: c.type = 'ORTHO'; c.ortho_scale = ortho
    o = bpy.data.objects.new(name, c); bpy.context.scene.collection.objects.link(o)
    o.location = loc; d = Vector(target) - Vector(loc); o.rotation_euler = d.to_track_quat('-Z', 'Y').to_euler()
    return o
def shot(cam, path, res=None):
    only = os.environ.get("ONLY_SHOTS")          # optional: re-render just these file names (comma list)
    if only and os.path.basename(path) not in only.split(","): return
    sc = bpy.context.scene; sc.camera = cam
    if res: sc.render.resolution_x, sc.render.resolution_y = res
    sc.render.filepath = path; bpy.ops.render.render(write_still=True)

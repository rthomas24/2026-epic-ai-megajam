# Triad Drift - Outpost: shared helpers (source paths, textured preview materials, camera renders).
import bpy, os, math
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
REPO = os.path.dirname(os.path.dirname(os.path.dirname(KIT)))
SRC_REL = os.path.join("ArtSource", "EngAI", "Props", "VerdanceOutpost", "mesh_r1")


def src_dir():
    """Grok Bot's delivery. Read only. OUTPOST_SRC overrides (used while the folder was not yet in the worktree)."""
    p = os.environ.get("OUTPOST_SRC") or os.path.join(REPO, SRC_REL)
    if not os.path.isdir(p):
        raise RuntimeError("source folder not found: %s (set OUTPOST_SRC)" % p)
    return p


def clear_scene():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o)


def import_fbx(name):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=os.path.join(src_dir(), name + ".fbx"))
    new = [o for o in bpy.data.objects if o not in before and o.type == "MESH"]
    return new[0]


def preview_material(name, tex_prefix, tex_dir=None, emissive=False, tint=None):
    """Image-based Principled material equal to what Unreal shows (N is DirectX: green flipped back for Blender)."""
    tex_dir = tex_dir or src_dir()
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])

    def tex(suffix, colorspace):
        n = nt.nodes.new("ShaderNodeTexImage")
        n.image = bpy.data.images.load(os.path.join(tex_dir, "%s_%s.png" % (tex_prefix, suffix)), check_existing=True)
        n.image.colorspace_settings.name = colorspace
        return n
    bc = tex("BC", "sRGB")
    nt.links.new(bc.outputs["Color"], bsdf.inputs["Base Color"])
    orm = tex("ORM", "Non-Color")
    sep = nt.nodes.new("ShaderNodeSeparateColor")
    nt.links.new(orm.outputs["Color"], sep.inputs["Color"])
    nt.links.new(sep.outputs["Green"], bsdf.inputs["Roughness"])
    nt.links.new(sep.outputs["Blue"], bsdf.inputs["Metallic"])
    n = tex("N", "Non-Color")
    s2 = nt.nodes.new("ShaderNodeSeparateColor")
    inv = nt.nodes.new("ShaderNodeMath"); inv.operation = "SUBTRACT"; inv.inputs[0].default_value = 1.0
    c2 = nt.nodes.new("ShaderNodeCombineColor")
    nm = nt.nodes.new("ShaderNodeNormalMap")
    nt.links.new(n.outputs["Color"], s2.inputs["Color"])
    nt.links.new(s2.outputs["Red"], c2.inputs["Red"]); nt.links.new(s2.outputs["Green"], inv.inputs[1])
    nt.links.new(inv.outputs[0], c2.inputs["Green"]); nt.links.new(s2.outputs["Blue"], c2.inputs["Blue"])
    nt.links.new(c2.outputs["Color"], nm.inputs["Color"]); nt.links.new(nm.outputs["Normal"], bsdf.inputs["Normal"])
    if emissive:
        e = tex("E", "sRGB")
        nt.links.new(e.outputs["Color"], bsdf.inputs["Emission Color"])
        bsdf.inputs["Emission Strength"].default_value = 2.0
    return mat


def render_views(path_prefix, views, res=(1100, 800), hide=()):
    """views: list of (label, cam_xyz_m, target_xyz_m, lens_mm or ('ortho', scale_m)). EEVEE, sun + world light."""
    sc = bpy.context.scene
    sc.render.engine = "BLENDER_EEVEE"
    sc.render.resolution_x, sc.render.resolution_y = res
    sc.render.image_settings.file_format = "PNG"
    sc.view_settings.view_transform = "Standard"
    if sc.world is None:
        sc.world = bpy.data.worlds.new("W")
    sc.world.use_nodes = True
    bg = sc.world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs[0].default_value = (0.55, 0.6, 0.68, 1); bg.inputs[1].default_value = 0.9
    if not bpy.data.objects.get("OP_Sun"):
        sd = bpy.data.lights.new("OP_Sun", "SUN"); sd.energy = 3.0
        so = bpy.data.objects.new("OP_Sun", sd); sc.collection.objects.link(so)
        so.rotation_euler = (math.radians(50), 0, math.radians(-60))
    cd = bpy.data.cameras.new("OP_Cam"); co = bpy.data.objects.new("OP_Cam", cd); sc.collection.objects.link(co)
    sc.camera = co
    for h in hide:
        h.hide_render = True
    out = []
    for label, pos, tgt, lens in views:
        co.location = Vector(pos)
        d = Vector(tgt) - Vector(pos)
        co.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
        if isinstance(lens, tuple):
            cd.type = "ORTHO"; cd.ortho_scale = lens[1]
        else:
            cd.type = "PERSP"; cd.lens = lens
        cd.clip_start = 0.02; cd.clip_end = 2000
        sc.render.filepath = "%s_%s.png" % (path_prefix, label)
        bpy.ops.render.render(write_still=True)
        out.append(sc.render.filepath)
    for h in hide:
        h.hide_render = False
    bpy.data.objects.remove(co)
    return out


from __future__ import annotations
from pathlib import Path
import bpy
from cc_paths import *
from cc_utils import log, select_only

def load_image(path, colorspace="sRGB"):
    img = bpy.data.images.load(str(path))
    img.colorspace_settings.name = colorspace
    return img

def make_src_emit_material(obj, image, colorspace="sRGB"):
    mat = bpy.data.materials.new(obj.name + "_emit")
    mat.use_nodes = True
    nt = mat.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    em = nt.nodes.new("ShaderNodeEmission")
    em.inputs["Strength"].default_value = 1.0
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = image
    image.colorspace_settings.name = colorspace
    nt.links.new(tex.outputs["Color"], em.inputs["Color"])
    nt.links.new(em.outputs["Emission"], out.inputs["Surface"])
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    return mat

def ensure_bake_image(obj, name, size=TEX_SIZE, noncolor=False):
    # Always bake into an sRGB image. Blender 4.5.3 writes all-black PNGs when the
    # bake target is tagged Non-Color (AO, NORMAL, and EMIT-of-data all failed).
    img = bpy.data.images.new(name, size, size, alpha=False, float_buffer=False)
    img.generated_color = (0, 0, 0, 1)
    img.colorspace_settings.name = "sRGB"
    mat = obj.data.materials[0] if obj.data.materials else None
    if mat is None:
        mat = bpy.data.materials.new(name + "_bake")
        mat.use_nodes = True
        obj.data.materials.append(mat)
    nt = mat.node_tree
    node = nt.nodes.new("ShaderNodeTexImage")
    node.image = img
    node.image.colorspace_settings.name = "sRGB"
    for n in nt.nodes:
        n.select = False
    node.select = True
    nt.nodes.active = node
    return img, node

def setup_cycles(samples=BAKE_SAMPLES):
    sc = bpy.context.scene
    sc.render.engine = "CYCLES"
    sc.cycles.device = "CPU"
    sc.cycles.samples = samples
    sc.cycles.bake_type = "EMIT"
    sc.render.bake.use_pass_direct = False
    sc.render.bake.use_pass_indirect = False
    sc.render.bake.use_pass_color = True
    sc.render.bake.margin = 8
    sc.render.bake.margin_type = "EXTEND"
    try:
        sc.cycles.use_denoising = False
    except Exception:
        pass

def bake_selected_to_active(src, dst, img, cage=CAGE_EXTRUDE, ray=CAGE_RAY):
    setup_cycles()
    sc = bpy.context.scene
    sc.cycles.bake_type = "EMIT"
    sc.render.bake.use_selected_to_active = True
    sc.render.bake.use_cage = True
    sc.render.bake.cage_extrusion = cage
    sc.render.bake.max_ray_distance = ray
    sc.render.bake.use_clear = True
    src.hide_set(False); src.hide_render = False; src.hide_viewport = False
    dst.hide_set(False)
    select_only(src, dst)
    bpy.context.view_layer.objects.active = dst
    log(f"    bake EMIT selected-to-active -> {img.name}")
    bpy.ops.object.bake(type="EMIT", use_clear=True, margin=8, use_selected_to_active=True,
                        cage_extrusion=cage, max_ray_distance=ray)
    return img

def bake_self(obj, bake_type, img, samples=BAKE_SAMPLES):
    setup_cycles(samples)
    sc = bpy.context.scene
    sc.cycles.bake_type = bake_type
    sc.render.bake.use_selected_to_active = False
    sc.render.bake.use_cage = False
    sc.render.bake.use_clear = True
    if bake_type == "NORMAL":
        sc.render.bake.normal_space = "TANGENT"
        sc.render.bake.normal_r = "POS_X"
        sc.render.bake.normal_g = "POS_Y"
        sc.render.bake.normal_b = "POS_Z"
    select_only(obj)
    log(f"    bake {bake_type} self -> {img.name}")
    kwargs = dict(type=bake_type, use_clear=True, margin=8, use_selected_to_active=False)
    if bake_type == "NORMAL":
        kwargs.update(normal_space="TANGENT")
    bpy.ops.object.bake(**kwargs)
    return img

def save_image(img, path, noncolor=False):
    """Write baked pixels without Blender's Image.save() (which writes the
    generated black plate when source is GENERATED, even if pixels are filled)."""
    import numpy as np
    path = Path(path)
    w, h = img.size
    ch = img.channels
    px = np.array(img.pixels, dtype=np.float32).reshape(h, w, ch)
    px = np.clip(px[::-1, :, :3], 0.0, 1.0)
    arr = (px * 255.0 + 0.5).astype(np.uint8)
    lo, hi = float(px.min()), float(px.max())
    try:
        from PIL import Image as PILImage
        PILImage.fromarray(arr, "RGB").save(str(path))
    except ImportError:
        # PPM then no; use a tiny PNG writer via zlib
        import struct, zlib
        def png(arr, dest):
            h, w, _ = arr.shape
            def chunk(tag, data):
                return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag+data) & 0xffffffff)
            raw = b"".join(b"\x00" + row.tobytes() for row in arr)
            with open(dest, "wb") as f:
                f.write(b"\x89PNG\r\n\x1a\n")
                f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)))
                f.write(chunk(b"IDAT", zlib.compress(raw, 9)))
                f.write(chunk(b"IEND", b""))
        png(arr, str(path))
    log(f"    saved {path}  pixels {lo:.3f}..{hi:.3f}  mean {float(px.mean()):.3f}")
    if hi <= 0.0:
        log(f"    WARNING {path} is all black")
    return path

def make_bevel_normal_material(obj, radius=0.006, noise_scale=700.0, bump_strength=0.18, bump_dist=0.0015):
    mat = bpy.data.materials.new(obj.name + "_nbake")
    mat.use_nodes = True
    nt = mat.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    bev = nt.nodes.new("ShaderNodeBevel")
    bev.inputs["Radius"].default_value = radius
    bev.samples = 12
    noise = nt.nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = noise_scale
    noise.inputs["Detail"].default_value = 6.0
    noise.inputs["Roughness"].default_value = 0.45
    texcoord = nt.nodes.new("ShaderNodeTexCoord")
    nt.links.new(texcoord.outputs["Object"], noise.inputs["Vector"])
    bump = nt.nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = bump_strength
    bump.inputs["Distance"].default_value = bump_dist
    nt.links.new(noise.outputs["Fac"], bump.inputs["Height"])
    nt.links.new(bev.outputs["Normal"], bump.inputs["Normal"])
    nt.links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    bsdf.inputs["Base Color"].default_value = (0.18, 0.18, 0.18, 1)
    bsdf.inputs["Roughness"].default_value = 0.45
    bsdf.inputs["Metallic"].default_value = 0.7
    nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    return mat

def make_emit_from_attribute(obj, layer):
    mat = bpy.data.materials.new(obj.name + "_attr_" + layer)
    mat.use_nodes = True
    nt = mat.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    em = nt.nodes.new("ShaderNodeEmission")
    em.inputs["Strength"].default_value = 1.0
    vc = nt.nodes.new("ShaderNodeVertexColor")
    vc.layer_name = layer
    nt.links.new(vc.outputs["Color"], em.inputs["Color"])
    nt.links.new(em.outputs["Emission"], out.inputs["Surface"])
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    return mat

def assign_pbr_material(obj, prefix, tex_dir, emission_strength=1.0, flip_dx=True):
    tex_dir = Path(tex_dir)
    bc = load_image(tex_dir / f"{prefix}_BC.png", "sRGB")
    n = load_image(tex_dir / f"{prefix}_N.png", "Non-Color")
    orm = load_image(tex_dir / f"{prefix}_ORM.png", "Non-Color")
    e = load_image(tex_dir / f"{prefix}_E.png", "sRGB")
    mat = bpy.data.materials.new("M_" + (prefix[2:] if prefix.startswith("T_") else prefix))
    mat.use_nodes = True
    nt = mat.node_tree
    for nd in list(nt.nodes):
        nt.nodes.remove(nd)
    out = nt.nodes.new("ShaderNodeOutputMaterial"); out.location = (800, 0)
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled"); bsdf.location = (450, 0)
    bsdf.inputs["Emission Strength"].default_value = emission_strength
    def texnode(img, loc, noncolor=False):
        nd = nt.nodes.new("ShaderNodeTexImage")
        nd.image = img
        nd.location = loc
        if noncolor:
            nd.image.colorspace_settings.name = "Non-Color"
        return nd
    n_bc = texnode(bc, (-300, 250))
    n_e = texnode(e, (-300, -250))
    n_orm = texnode(orm, (-300, 40), True)
    n_n = texnode(n, (-300, -80), True)
    sep = nt.nodes.new("ShaderNodeSeparateColor"); sep.location = (0, 40)
    nmap = nt.nodes.new("ShaderNodeNormalMap"); nmap.location = (150, -80)
    nmap.space = "TANGENT"
    nmap.inputs["Strength"].default_value = 1.0
    if flip_dx:
        sep_n = nt.nodes.new("ShaderNodeSeparateColor"); sep_n.location = (-50, -80)
        comb_n = nt.nodes.new("ShaderNodeCombineColor"); comb_n.location = (200, -80)
        inv = nt.nodes.new("ShaderNodeInvert"); inv.location = (80, -140)
        nt.links.new(n_n.outputs["Color"], sep_n.inputs["Color"])
        nt.links.new(sep_n.outputs["Red"], comb_n.inputs["Red"])
        nt.links.new(sep_n.outputs["Green"], inv.inputs["Color"])
        nt.links.new(inv.outputs["Color"], comb_n.inputs["Green"])
        nt.links.new(sep_n.outputs["Blue"], comb_n.inputs["Blue"])
        nt.links.new(comb_n.outputs["Color"], nmap.inputs["Color"])
    else:
        nt.links.new(n_n.outputs["Color"], nmap.inputs["Color"])
    nt.links.new(n_bc.outputs["Color"], bsdf.inputs["Base Color"])
    nt.links.new(n_e.outputs["Color"], bsdf.inputs["Emission Color"])
    nt.links.new(n_orm.outputs["Color"], sep.inputs["Color"])
    nt.links.new(sep.outputs["Green"], bsdf.inputs["Roughness"])
    nt.links.new(sep.outputs["Blue"], bsdf.inputs["Metallic"])
    nt.links.new(nmap.outputs["Normal"], bsdf.inputs["Normal"])
    nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    return mat

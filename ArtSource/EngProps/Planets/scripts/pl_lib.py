# Shared helpers for the planet and ground bakes (Blender 5.2, headless).
# Node builder, colour helpers, Cycles EMIT bakes into float images, numpy post-processing
# (normal from height, cavity AO, sRGB encode) and PNG writing.
import math
import os

import bpy
import numpy as np


# ---------------------------------------------------------------- colour
def srgb_to_lin(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def hexc(h, a=1.0):
    h = h.lstrip("#")
    return (srgb_to_lin(int(h[0:2], 16) / 255.0), srgb_to_lin(int(h[2:4], 16) / 255.0),
            srgb_to_lin(int(h[4:6], 16) / 255.0), a)


def lin_to_srgb_np(x):
    x = np.clip(x, 0.0, 1.0)
    return np.where(x <= 0.0031308, x * 12.92, 1.055 * np.power(x, 1.0 / 2.4) - 0.055)


# ---------------------------------------------------------------- node builder
class NB:
    """Tiny node-graph DSL. Every helper returns an output socket."""

    def __init__(self, nt):
        self.nt = nt
        self.n_count = 0

    def node(self, kind, **props):
        n = self.nt.nodes.new(kind)
        n.location = ((self.n_count % 20) * 220, -(self.n_count // 20) * 260)
        self.n_count += 1
        for k, v in props.items():
            setattr(n, k, v)
        return n

    def put(self, sock, val):
        if isinstance(val, bpy.types.NodeSocket):
            self.nt.links.new(val, sock)
        elif val is not None:
            try:
                sock.default_value = val
            except (ValueError, TypeError):
                n = len(sock.default_value)
                sock.default_value = (tuple(val) + (1.0,) * 4)[:n]

    def inp(self, n, key, val):
        if isinstance(key, int):
            self.put(n.inputs[key], val)
            return
        for s in n.inputs:
            if (s.identifier == key or s.name == key) and s.enabled:
                self.put(s, val)
                return
        for s in n.inputs:
            if s.identifier == key or s.name == key:
                self.put(s, val)
                return
        raise KeyError("%s has no input %s" % (n.bl_idname, key))

    def out(self, n, key):
        for s in n.outputs:
            if (s.identifier == key or s.name == key) and s.enabled:
                return s
        for s in n.outputs:
            if s.identifier == key or s.name == key:
                return s
        raise KeyError("%s has no output %s" % (n.bl_idname, key))

    # --- coordinates
    def objpos_normalized(self):
        tc = self.node("ShaderNodeTexCoord")
        vm = self.node("ShaderNodeVectorMath", operation="NORMALIZE")
        self.inp(vm, 0, self.out(tc, "Object"))
        return self.out(vm, "Vector")

    def uv(self):
        tc = self.node("ShaderNodeTexCoord")
        return self.out(tc, "UV")

    # --- maths
    def m(self, op, a, b=None, c=None, clamp=False):
        n = self.node("ShaderNodeMath", operation=op, use_clamp=clamp)
        self.put(n.inputs[0], a)
        if b is not None:
            self.put(n.inputs[1], b)
        if c is not None:
            self.put(n.inputs[2], c)
        return n.outputs[0]

    def add(self, a, b): return self.m("ADD", a, b)
    def sub(self, a, b): return self.m("SUBTRACT", a, b)
    def mul(self, a, b): return self.m("MULTIPLY", a, b)
    def mx(self, a, b): return self.m("MAXIMUM", a, b)
    def mn(self, a, b): return self.m("MINIMUM", a, b)
    def absv(self, a): return self.m("ABSOLUTE", a)
    def powv(self, a, b): return self.m("POWER", a, b)
    def one_minus(self, a): return self.m("SUBTRACT", 1.0, a)
    def clamp01(self, a): return self.m("ADD", a, 0.0, clamp=True)

    def ridge(self, a):
        """1 - |2a - 1|: peaks where a == 0.5."""
        return self.one_minus(self.absv(self.m("MULTIPLY_ADD", a, 2.0, -1.0)))

    def smooth(self, x, lo, hi):
        n = self.node("ShaderNodeMapRange", interpolation_type="SMOOTHSTEP", clamp=True)
        self.inp(n, "Value", x)
        self.inp(n, "From Min", lo)
        self.inp(n, "From Max", hi)
        self.inp(n, "To Min", 0.0)
        self.inp(n, "To Max", 1.0)
        return self.out(n, "Result")

    def remap(self, x, lo, hi, a=0.0, b=1.0, clamp=True):
        n = self.node("ShaderNodeMapRange", interpolation_type="LINEAR", clamp=clamp)
        self.inp(n, "Value", x)
        self.inp(n, "From Min", lo)
        self.inp(n, "From Max", hi)
        self.inp(n, "To Min", a)
        self.inp(n, "To Max", b)
        return self.out(n, "Result")

    # --- vectors
    def vadd(self, a, b):
        n = self.node("ShaderNodeVectorMath", operation="ADD")
        self.put(n.inputs[0], a); self.put(n.inputs[1], b)
        return n.outputs[0]

    def vscale(self, a, s):
        n = self.node("ShaderNodeVectorMath", operation="SCALE")
        self.put(n.inputs[0], a); self.inp(n, "Scale", s)
        return n.outputs[0]

    def vmul(self, a, b):
        n = self.node("ShaderNodeVectorMath", operation="MULTIPLY")
        self.put(n.inputs[0], a); self.put(n.inputs[1], b)
        return n.outputs[0]

    def sep(self, v):
        n = self.node("ShaderNodeSeparateXYZ")
        self.put(n.inputs[0], v)
        return n.outputs[0], n.outputs[1], n.outputs[2]

    def comb(self, x, y, z):
        n = self.node("ShaderNodeCombineXYZ")
        self.put(n.inputs[0], x); self.put(n.inputs[1], y); self.put(n.inputs[2], z)
        return n.outputs[0]

    def rgb(self, r, g, b):
        n = self.node("ShaderNodeCombineColor", mode="RGB")
        self.put(n.inputs[0], r); self.put(n.inputs[1], g); self.put(n.inputs[2], b)
        return n.outputs[0]

    def warp(self, v, w, amount):
        """v + (w - 0.5) * amount, w a colour/vector in 0..1."""
        off = self.vadd(w, (-0.5, -0.5, -0.5))
        return self.vadd(v, self.vscale(off, amount))

    # --- textures
    def noise(self, vec, scale, detail=6.0, rough=0.55, lac=2.0, distortion=0.0, w=None, kind="FBM", out="Fac"):
        n = self.node("ShaderNodeTexNoise", noise_dimensions="4D" if w is not None else "3D", noise_type=kind)
        try:
            n.normalize = True
        except Exception:
            pass
        self.inp(n, "Vector", vec)
        if w is not None:
            self.inp(n, "W", w)
        self.inp(n, "Scale", scale)
        self.inp(n, "Detail", detail)
        self.inp(n, "Roughness", rough)
        self.inp(n, "Lacunarity", lac)
        self.inp(n, "Distortion", distortion)
        return self.out(n, out)

    def voronoi(self, vec, scale, feature="F1", w=None, randomness=1.0, out="Distance", detail=0.0, smooth=0.0):
        n = self.node("ShaderNodeTexVoronoi", voronoi_dimensions="4D" if w is not None else "3D", feature=feature)
        self.inp(n, "Vector", vec)
        if w is not None:
            self.inp(n, "W", w)
        self.inp(n, "Scale", scale)
        self.inp(n, "Randomness", randomness)
        try:
            self.inp(n, "Detail", detail)
        except KeyError:
            pass
        if feature == "SMOOTH_F1":
            self.inp(n, "Smoothness", smooth)
        return self.out(n, out)

    def ramp(self, x, stops, interp="LINEAR"):
        n = self.node("ShaderNodeValToRGB")
        cr = n.color_ramp
        cr.interpolation = interp
        while len(cr.elements) > 1:
            cr.elements.remove(cr.elements[-1])
        cr.elements[0].position = stops[0][0]
        cr.elements[0].color = hexc(stops[0][1]) if isinstance(stops[0][1], str) else stops[0][1]
        for pos, col in stops[1:]:
            e = cr.elements.new(pos)
            e.color = hexc(col) if isinstance(col, str) else col
        self.put(n.inputs[0], x)
        return n.outputs[0]

    def mix(self, fac, a, b):
        n = self.node("ShaderNodeMix", data_type="RGBA", blend_type="MIX", clamp_factor=True)
        self.inp(n, "Factor_Float", fac)
        self.inp(n, "A_Color", a if not isinstance(a, str) else hexc(a))
        self.inp(n, "B_Color", b if not isinstance(b, str) else hexc(b))
        return self.out(n, "Result_Color")

    def cmul(self, a, b):
        n = self.node("ShaderNodeMix", data_type="RGBA", blend_type="MULTIPLY", clamp_factor=True)
        self.inp(n, "Factor_Float", 1.0)
        self.inp(n, "A_Color", a)
        self.inp(n, "B_Color", b if not isinstance(b, str) else hexc(b))
        return self.out(n, "Result_Color")

    def cadd(self, a, b):
        n = self.node("ShaderNodeMix", data_type="RGBA", blend_type="ADD", clamp_factor=True)
        self.inp(n, "Factor_Float", 1.0)
        self.inp(n, "A_Color", a)
        self.inp(n, "B_Color", b if not isinstance(b, str) else hexc(b))
        return self.out(n, "Result_Color")

    def cscale(self, c, s):
        """colour * scalar (as a vector scale)."""
        return self.vscale(c, s)


def torus_coords(nb, tile_m):
    """Seamless tiling coordinates: UV 0..1 mapped onto a flat (Clifford) torus in 4D, in metres.
    Returns (vec3, w). Any noise or Voronoi evaluated on these is periodic over one UV tile."""
    uvv = nb.uv()
    u, v, _ = nb.sep(uvv)
    r = tile_m / (2.0 * math.pi)
    tau = 2.0 * math.pi
    au = nb.mul(u, tau)
    av = nb.mul(v, tau)
    x = nb.mul(nb.m("COSINE", au), r)
    y = nb.mul(nb.m("SINE", au), r)
    z = nb.mul(nb.m("COSINE", av), r)
    w = nb.mul(nb.m("SINE", av), r)
    return nb.comb(x, y, z), w


def torus_offset(nb, vec, w, offset):
    """Shift a torus coordinate (for decorrelated noise layers)."""
    return nb.vadd(vec, (offset, offset * 0.7, offset * 1.3)), nb.add(w, offset * 0.37)


# ---------------------------------------------------------------- bake
def setup_cycles_bake(scene, samples=1):
    scene.render.engine = "CYCLES"
    scene.cycles.samples = samples
    scene.cycles.use_denoising = False
    try:
        scene.cycles.device = "CPU"
    except Exception:
        pass
    scene.render.bake.margin = 16
    scene.render.bake.use_clear = True
    scene.render.bake.target = "IMAGE_TEXTURES"


def bake_socket(obj, mat, socket, w, h, name, scene):
    """Bake an arbitrary shader socket (float or colour) via EMIT into a float image -> (h, w, 4) numpy, rows bottom-up."""
    nt = mat.node_tree
    img = bpy.data.images.new(name, w, h, alpha=True, float_buffer=True)
    img.colorspace_settings.name = "Non-Color"
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = img
    for n in nt.nodes:
        n.select = False
    tex.select = True
    nt.nodes.active = tex
    emis = nt.nodes.new("ShaderNodeEmission")
    nt.links.new(socket, emis.inputs["Color"])
    emis.inputs["Strength"].default_value = 1.0
    outn = [n for n in nt.nodes if n.bl_idname == "ShaderNodeOutputMaterial"][0]
    nt.links.new(emis.outputs[0], outn.inputs["Surface"])
    for o in scene.objects:
        o.select_set(False)
    obj.hide_render = False
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.bake(type="EMIT", margin=16, use_clear=True)
    arr = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(arr)
    arr = arr.reshape(h, w, 4)
    nt.nodes.remove(tex)
    nt.nodes.remove(emis)
    bpy.data.images.remove(img)
    return arr


# ---------------------------------------------------------------- post
def box_blur(a, r, wrap_y=True):
    """Separable box blur, wrapping in x (and optionally y)."""
    out = a.copy()
    k = 2 * r + 1
    acc = np.zeros_like(out)
    for d in range(-r, r + 1):
        acc += np.roll(out, d, axis=1)
    out = acc / k
    acc = np.zeros_like(out)
    for d in range(-r, r + 1):
        if wrap_y:
            acc += np.roll(out, d, axis=0)
        else:
            idx = np.clip(np.arange(out.shape[0]) + d, 0, out.shape[0] - 1)
            acc += out[idx]
    return acc / k


def normal_from_height(hgt, px_w, px_h, strength, wrap_y=True):
    """hgt (H, W) rows bottom-up; px_w/px_h world size of a pixel (scalar or (H,1) array).
    Returns DirectX tangent-space normal (H, W, 3) in 0..1 (green = +down the image)."""
    dx = (np.roll(hgt, -1, axis=1) - np.roll(hgt, 1, axis=1)) / (2.0 * px_w)
    if wrap_y:
        dy = (np.roll(hgt, -1, axis=0) - np.roll(hgt, 1, axis=0)) / (2.0 * px_h)
    else:
        up = np.vstack([hgt[1:], hgt[-1:]])
        dn = np.vstack([hgt[:1], hgt[:-1]])
        dy = (up - dn) / (2.0 * px_h)
    nx = -dx * strength
    ny_gl = -dy * strength           # OpenGL: +Y towards the image top (rows increase upward here)
    nz = np.ones_like(hgt)
    ln = np.sqrt(nx * nx + ny_gl * ny_gl + nz * nz)
    nx, ny_gl, nz = nx / ln, ny_gl / ln, nz / ln
    ny_dx = -ny_gl                   # DirectX flips green
    return np.stack([nx * 0.5 + 0.5, ny_dx * 0.5 + 0.5, nz * 0.5 + 0.5], axis=-1)


def save_png(path, arr, srgb_encode=False):
    """arr (H, W, C) float 0..1, rows bottom-up (Blender order). Writes an 8-bit PNG (RGB or RGBA)."""
    h, w, c = arr.shape
    data = np.clip(arr, 0.0, 1.0).astype(np.float32)
    if srgb_encode:
        data = data.copy()
        data[..., :3] = lin_to_srgb_np(data[..., :3])
    rgba = np.ones((h, w, 4), dtype=np.float32)
    rgba[..., :c] = data if c <= 4 else data[..., :4]
    if c == 1:
        rgba[..., 1] = data[..., 0]
        rgba[..., 2] = data[..., 0]
    name = os.path.basename(path)
    img = bpy.data.images.new(name, w, h, alpha=(c == 4), float_buffer=False)
    img.colorspace_settings.name = "Non-Color"
    img.pixels.foreach_set(rgba.ravel())
    img.filepath_raw = path
    img.file_format = "PNG"
    os.makedirs(os.path.dirname(path), exist_ok=True)
    img.save()
    bpy.data.images.remove(img)
    return path


def load_img(path, noncolor):
    img = bpy.data.images.load(path, check_existing=False)
    img.colorspace_settings.name = "Non-Color" if noncolor else "sRGB"
    return img


def fresh_scene():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    for m in list(bpy.data.materials):
        bpy.data.materials.remove(m)
    for m in list(bpy.data.meshes):
        bpy.data.meshes.remove(m)
    for i in list(bpy.data.images):
        bpy.data.images.remove(i)
    return bpy.context.scene


def set_view(scene, exposure=0.0, look="AgX - Punchy"):
    vs = scene.view_settings
    try:
        vs.view_transform = "AgX"
    except Exception:
        pass
    try:
        vs.look = look
    except Exception:
        pass
    vs.exposure = exposure


def render_to(scene, path, res=(1600, 900), engine="BLENDER_EEVEE", samples=64):
    scene.render.engine = engine
    scene.render.resolution_x, scene.render.resolution_y = res
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    if engine == "CYCLES":
        scene.cycles.samples = samples
        scene.cycles.use_denoising = True
    else:
        try:
            scene.eevee.taa_render_samples = samples
        except Exception:
            pass
    scene.render.filepath = path
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.render.render(write_still=True)
    return path

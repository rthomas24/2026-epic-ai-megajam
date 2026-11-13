# SPDX-License-Identifier: MIT
"""
build_sky.py - Triad Drift nebula sky (T_TD_NebulaSky), baked in Blender 5.2 from procedural shaders.

    blender --background --factory-startup --python build_sky.py -- --width 8192 --samples 24 --out <dir> [--tag r3]

Everything is procedural and seeded:
  * world shader (evaluated per view direction): very dark indigo space, a faint galactic band,
    three domain-warped nebula complexes (magenta/pink + violet, teal/cyan, violet with a gold core)
    with ridged filaments, bright rims and dark dust lanes, four star layers with size / colour /
    brightness variance, the sun disc with an analytic corona
  * real geometry: a banded, ringed gas giant (sphere + ring annulus) lit by a Sun lamp that points
    exactly along the painted sun direction, so the terminator and the ring shadow agree with the sun
  * a Cycles PANO / EQUIRECTANGULAR camera at the origin renders the whole sphere of directions
  * numpy post: soft glow (and short cross-flare) on the brightest stars, then
      <out>/T_TD_NebulaSky.exr  (half float, linear, HDR)
      <out>/T_TD_NebulaSky.png  (8-bit sRGB, soft shoulder tone curve)

Mapping (verified by probe_equirect.py): image u = 0.5 at Blender +X, u = 0.25 at Blender +Y, top row = +Z.
In Unreal axes (X fwd, Y right, Z up; Unreal Y = -Blender Y):  u = Yaw/360 + 0.5,  v(top-down) = 0.5 - Pitch/180.
"""

import json
import math
import os
import sys
import time

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

# --------------------------------------------------------------------------- layout (Unreal yaw/pitch, deg)
SUN = (35.0, 18.0)            # the sun, also the Sun lamp direction
PLANET = (126.0, 4.0, 10.0)    # yaw, pitch, angular radius
CLOUDS = [
    # name, yaw, pitch, radius deg, noise scale, palette, strength, seed
    ("magenta", -18.0, 6.0, 52.0, 2.1, "magenta", 1.0, 1),
    ("violet", -52.0, 30.0, 26.0, 3.0, "violet", 0.7, 2),
    ("teal", 166.0, -10.0, 54.0, 1.9, "teal", 1.0, 3),
    ("gold", 78.0, 46.0, 27.0, 2.8, "violetgold", 0.85, 4),
]
GALAXY_NORMAL = (100.0, 58.0)   # normal of the faint galactic band's great circle

PALETTES = {
    # (deep, main, hot, rim) linear RGB
    "magenta": ((0.14, 0.0, 0.13), (1.0, 0.03, 0.62), (1.0, 0.36, 0.80), (1.0, 0.30, 0.85)),
    "violet": ((0.035, 0.008, 0.12), (0.32, 0.07, 0.85), (0.70, 0.45, 1.0), (0.85, 0.40, 1.0)),
    "teal": ((0.0, 0.06, 0.12), (0.0, 0.72, 0.85), (0.40, 1.0, 0.92), (0.30, 0.70, 1.0)),
    "violetgold": ((0.08, 0.0, 0.20), (0.55, 0.08, 1.0), (1.0, 0.52, 0.04), (1.0, 0.50, 0.02)),
}


def ue_dir(yaw, pitch):
    """Unreal yaw/pitch -> Blender world direction."""
    y, p = math.radians(yaw), math.radians(pitch)
    return Vector((math.cos(p) * math.cos(y), -math.cos(p) * math.sin(y), math.sin(p)))


def log(*a):
    print("[sky]", *a)
    sys.stdout.flush()


# --------------------------------------------------------------------------- node builder
class G:
    def __init__(self, nt):
        self.nt = nt
        self.x = 0

    def node(self, kind, **props):
        n = self.nt.nodes.new(kind)
        for k, v in props.items():
            setattr(n, k, v)
        self.x += 1
        n.location = ((self.x % 40) * 220, -(self.x // 40) * 300)
        return n

    def _set(self, sock, val):
        if isinstance(val, bpy.types.NodeSocket):
            self.nt.links.new(val, sock)
        elif isinstance(val, (tuple, list, Vector)):
            n = len(sock.default_value)
            v = list(val)[:n] + [1.0] * max(0, n - len(val))
            sock.default_value = v
        else:
            sock.default_value = val

    def m(self, op, a, b=None, c=None, clamp=False):
        n = self.node("ShaderNodeMath", operation=op, use_clamp=clamp)
        self._set(n.inputs[0], a)
        if b is not None:
            self._set(n.inputs[1], b)
        if c is not None:
            self._set(n.inputs[2], c)
        return n.outputs[0]

    def v(self, op, a, b=None, scale=None):
        n = self.node("ShaderNodeVectorMath", operation=op)
        self._set(n.inputs[0], a)
        if b is not None:
            self._set(n.inputs[1], b)
        if scale is not None:
            self._set(n.inputs["Scale"], scale)
        return n.outputs["Value"] if op in ("DOT_PRODUCT", "LENGTH", "DISTANCE") else n.outputs["Vector"]

    def mr(self, val, a, b, c=0.0, d=1.0, smooth=True, clamp=True):
        n = self.node("ShaderNodeMapRange", clamp=clamp)
        if smooth:
            n.interpolation_type = "SMOOTHSTEP"
        self._set(n.inputs[0], val)
        for i, x in zip((1, 2, 3, 4), (a, b, c, d)):
            self._set(n.inputs[i], x)
        return n.outputs[0]

    def noise(self, vec, scale, detail=6.0, rough=0.55, dist=0.0, lac=2.0, color=False):
        n = self.node("ShaderNodeTexNoise")
        n.noise_dimensions = "3D"
        try:
            n.noise_type = "FBM"
        except Exception:
            pass
        self._set(n.inputs["Vector"], vec)
        n.inputs["Scale"].default_value = scale
        n.inputs["Detail"].default_value = detail
        n.inputs["Roughness"].default_value = rough
        n.inputs["Lacunarity"].default_value = lac
        n.inputs["Distortion"].default_value = dist
        if color:
            return n.outputs["Color"]
        return n.outputs.get("Fac") or n.outputs.get("Factor")

    def voronoi(self, vec, scale):
        n = self.node("ShaderNodeTexVoronoi")
        n.voronoi_dimensions = "3D"
        n.feature = "F1"
        n.distance = "EUCLIDEAN"
        self._set(n.inputs["Vector"], vec)
        n.inputs["Scale"].default_value = scale
        n.inputs["Randomness"].default_value = 1.0
        return n.outputs["Distance"], n.outputs["Color"]

    def sep(self, vec):
        n = self.node("ShaderNodeSeparateXYZ")
        self._set(n.inputs[0], vec)
        return n.outputs[0], n.outputs[1], n.outputs[2]

    def ramp(self, fac, stops):
        n = self.node("ShaderNodeValToRGB")
        cr = n.color_ramp
        cr.interpolation = "B_SPLINE" if len(stops) > 2 else "LINEAR"
        while len(cr.elements) < len(stops):
            cr.elements.new(0.5)
        for el, (pos, col) in zip(cr.elements, stops):
            el.position = pos
            el.color = (*col, 1.0)
        self._set(n.inputs[0], fac)
        return n.outputs[0]

    def col(self, c):
        return tuple(c)

    def tint(self, vec_or_col, scalar):
        """colour * scalar socket"""
        return self.v("SCALE", vec_or_col, scale=scalar)

    def ridge(self, fac, power):
        # 1 - |2f - 1|  ->  ^power : thin bright filaments where the fBm crosses its mean
        r = self.m("SUBTRACT", 1.0, self.m("ABSOLUTE", self.m("SUBTRACT", self.m("MULTIPLY", fac, 2.0), 1.0)))
        return self.m("POWER", self.m("MAXIMUM", r, 0.0), power)


# --------------------------------------------------------------------------- world shader
PIX = None  # angular size of one final-resolution pixel (rad), set in main()


def build_world(width):
    global PIX
    # star sizes are fixed in ANGLE (one texel of a 4096-wide map), so the look does not change with --width
    PIX = 2.0 * math.pi / 4096.0
    w = bpy.data.worlds.new("W_TD_NebulaSky")
    bpy.context.scene.world = w
    nt = w.node_tree
    nt.nodes.clear()
    g = G(nt)
    out = g.node("ShaderNodeOutputWorld")
    bg = g.node("ShaderNodeBackground")
    tc = g.node("ShaderNodeTexCoord")
    D = g.v("NORMALIZE", tc.outputs["Generated"])

    # domain warp (two levels): wispy, folded structure instead of round blobs
    w1 = g.v("SUBTRACT", g.noise(D, 1.3, 4.0, 0.55, color=True), (0.5, 0.5, 0.5))
    Dw = g.v("ADD", D, g.v("SCALE", w1, scale=0.42))
    w2 = g.v("SUBTRACT", g.noise(Dw, 4.2, 5.0, 0.6, color=True), (0.5, 0.5, 0.5))
    Dw2 = g.v("ADD", Dw, g.v("SCALE", w2, scale=0.10))

    # --- dust (absorbs nebula light and stars): ridged lanes inside a patchy mask
    dfb = g.noise(g.v("ADD", Dw2, (3.1, 7.7, 1.9)), 3.4, 8.0, 0.6, dist=0.3)
    lanes = g.ridge(dfb, 3.0)
    dmask = g.mr(g.noise(g.v("ADD", Dw, (9.2, 1.4, 5.5)), 1.9, 4.0, 0.5), 0.42, 0.62)
    dust_big = g.mr(g.noise(g.v("ADD", Dw2, (1.7, 4.4, 8.8)), 2.6, 7.0, 0.62), 0.50, 0.66)
    dust = g.m("MINIMUM", g.m("ADD", g.m("MULTIPLY", lanes, dmask), g.m("MULTIPLY", dust_big, 0.55)), 1.0)
    trans = g.m("SUBTRACT", 1.0, g.m("MULTIPLY", dust, 0.93))

    # --- galactic band (very faint diffuse light, more stars)
    GN = ue_dir(*GALAXY_NORMAL)
    gb = g.mr(g.m("ABSOLUTE", g.v("DOT_PRODUCT", Dw, tuple(GN))), 0.30, 0.0)
    gb = g.m("MULTIPLY", gb, g.mr(g.noise(Dw, 6.0, 6.0, 0.6), 0.3, 0.75))

    # --- nebula complexes
    neb = (0.0, 0.0, 0.0)
    neb_mask_total = 0.0
    for name, yaw, pitch, rad, sc, pal, strength, seed in CLOUDS:
        C = ue_dir(yaw, pitch)
        deep, main, hot, rim_c = PALETTES[pal]
        off = (seed * 3.71, seed * 1.93, seed * 5.17)
        q = g.v("DOT_PRODUCT", Dw, tuple(C))
        r0 = 1.0 - math.cos(math.radians(rad))
        mask = g.mr(g.m("SUBTRACT", 1.0, q), r0, 0.0)
        edge = g.mr(g.noise(g.v("ADD", Dw, off), 2.2, 5.0, 0.6), 0.30, 0.70, 0.25, 1.35)
        mask = g.m("MINIMUM", g.m("MULTIPLY", mask, edge), 1.0)
        mask = g.m("MULTIPLY", mask, mask)
        billow_f = g.noise(g.v("ADD", Dw, off), sc, 10.0, 0.62, dist=0.15)
        billow = g.mr(billow_f, 0.38, 0.80)
        fil_f = g.noise(g.v("ADD", Dw2, (off[1], off[2], off[0])), sc * 1.7, 9.0, 0.58)
        fil = g.ridge(fil_f, 6.0)
        fil_soft = g.ridge(fil_f, 2.0)
        dens = g.m("ADD", g.m("MULTIPLY", g.m("POWER", billow, 1.6), 0.62),
                   g.m("MULTIPLY", g.m("MULTIPLY", fil, 1.05), g.m("ADD", billow, 0.25)))
        dens = g.m("ADD", dens, g.m("MULTIPLY", fil_soft, 0.10))
        dens = g.m("MULTIPLY", dens, mask)
        colr = g.ramp(g.m("MINIMUM", dens, 1.0), [(0.0, (0.0, 0.0, 0.0)), (0.14, deep), (0.42, main),
                                                    (0.80, hot), (1.0, tuple(min(1.0, c * 0.7 + 0.3) for c in hot))])
        em = g.tint(colr, g.m("MULTIPLY", g.m("ADD", dens, g.m("MULTIPLY", dens, dens)), strength * 1.6))
        # bright rims: filament crests near the cloud's outer edge
        rim = g.m("MULTIPLY", g.m("MULTIPLY", g.m("MULTIPLY", mask, g.m("SUBTRACT", 1.0, mask)), 4.0), fil)
        rim = g.m("MULTIPLY", rim, g.mr(billow_f, 0.45, 0.7))
        em = g.v("ADD", em, g.tint(tuple(rim_c), g.m("MULTIPLY", rim, 0.9 * strength)))
        neb = g.v("ADD", neb, em)
        neb_mask_total = g.m("ADD", neb_mask_total, mask)

    # dust lanes read as dark silhouettes against the glow; a little warm scattering on their edges
    neb_t = g.tint(neb, trans)
    dust_edge = g.m("MULTIPLY", g.m("MULTIPLY", dust, g.m("SUBTRACT", 1.0, dust)), 4.0)
    neb_t = g.v("ADD", neb_t, g.tint((0.05, 0.004, 0.06), g.m("MULTIPLY", dust_edge, g.m("MINIMUM", neb_mask_total, 1.0))))

    # --- deep background: very dark indigo, slow variation, faint band light
    bgv = g.mr(g.noise(D, 0.9, 3.0, 0.5), 0.3, 0.7)
    base = g.v("ADD", (0.0022, 0.0016, 0.0075), g.tint((0.003, 0.0015, 0.009), bgv))
    base = g.v("ADD", base, g.tint((0.012, 0.008, 0.024), gb))
    # faint wide veils between the complexes: the sky is never an empty black void, yet stays dark
    vf = g.noise(g.v("ADD", Dw, (5.5, 2.2, 9.1)), 1.15, 8.0, 0.62, dist=0.2)
    veil = g.m("ADD", g.m("MULTIPLY", g.mr(vf, 0.54, 0.84), 0.7), g.m("MULTIPLY", g.ridge(vf, 5.0), 0.45))
    veil = g.m("MULTIPLY", veil, g.mr(g.noise(g.v("ADD", D, (1.0, 8.0, 3.0)), 0.7, 2.0, 0.5), 0.44, 0.70, 0.0, 1.0))
    hue = g.noise(g.v("ADD", D, (4.4, 4.4, 0.3)), 0.55, 2.0, 0.5)
    vcol = g.ramp(hue, [(0.0, (0.01, 0.20, 0.26)), (0.30, (0.04, 0.08, 0.36)), (0.5, (0.16, 0.03, 0.36)),
                        (0.72, (0.32, 0.02, 0.24)), (1.0, (0.02, 0.14, 0.30))])
    base = g.v("ADD", base, g.tint(g.tint(vcol, veil), g.m("MULTIPLY", trans, 0.13)))

    # --- stars (scale, keep share, radius px min/max, brightness lo/hi, offset, density boost from band/nebula)
    layers = [
        (170.0, 0.16, 0.55, 1.1, 0.10, 1.0, (11.0, 3.0, 7.0), 1.0),
        (95.0, 0.10, 0.7, 1.4, 0.25, 2.4, (2.0, 13.0, 5.0), 0.6),
        (34.0, 0.16, 0.9, 1.8, 0.8, 6.0, (7.0, 5.0, 17.0), 0.3),
        (11.0, 0.13, 1.3, 2.2, 3.0, 40.0, (19.0, 2.0, 3.0), 0.0),
    ]
    stars = (0.0, 0.0, 0.0)
    boost = g.m("ADD", 1.0, g.m("ADD", g.m("MULTIPLY", gb, 1.6), g.m("MULTIPLY", g.m("MINIMUM", neb_mask_total, 1.0), 0.6)))
    for (S, keep, rmin, rmax, blo, bhi, off, dens_boost) in layers:
        dist, rc = g.voronoi(g.v("ADD", D, (off[0] / S, off[1] / S, off[2] / S)), S)
        r1, r2, r3 = g.sep(rc)
        thresh = 1.0 - keep
        if dens_boost > 0.0:
            # denser where the band and the nebulae are: lower the survival threshold there
            thresh = g.m("MAXIMUM", g.m("SUBTRACT", thresh, g.m("MULTIPLY", g.m("SUBTRACT", boost, 1.0), keep * dens_boost)), 0.0)
        alive = g.m("GREATER_THAN", r1, thresh)
        radius = g.m("MULTIPLY", g.m("ADD", rmin, g.m("MULTIPLY", g.m("POWER", r2, 3.0), rmax - rmin)), PIX * S)
        x = g.m("DIVIDE", dist, radius)
        prof = g.m("EXPONENT", g.m("MULTIPLY", g.m("MULTIPLY", x, x), -2.2))
        bright = g.m("MULTIPLY", blo, g.m("POWER", bhi / blo, g.m("POWER", r3, 5.0)))
        ckey = g.m("FRACT", g.m("MULTIPLY", r1, 13.7))
        scol = g.ramp(ckey, [(0.0, (1.0, 0.62, 0.38)), (0.18, (1.0, 0.85, 0.65)), (0.45, (1.0, 0.97, 0.92)),
                             (0.75, (0.78, 0.86, 1.0)), (1.0, (0.62, 0.74, 1.0))])
        s = g.tint(scol, g.m("MULTIPLY", g.m("MULTIPLY", alive, prof), bright))
        stars = g.v("ADD", stars, s)
    stars = g.tint(stars, g.m("ADD", g.m("MULTIPLY", trans, 0.92), 0.08))

    # --- sun: disc + corona (analytic, exact direction)
    SD = ue_dir(*SUN)
    cs = g.v("DOT_PRODUCT", D, tuple(SD))
    ang = g.m("ARCCOSINE", g.m("MINIMUM", cs, 1.0))  # radians from the sun
    disc = g.mr(ang, math.radians(0.62), math.radians(0.50))
    c1 = g.m("EXPONENT", g.m("MULTIPLY", ang, -1.0 / math.radians(1.2)))
    c2 = g.m("EXPONENT", g.m("MULTIPLY", ang, -1.0 / math.radians(5.0)))
    c3 = g.m("EXPONENT", g.m("MULTIPLY", ang, -1.0 / math.radians(18.0)))
    sun = g.tint((1.0, 0.95, 0.86), g.m("MULTIPLY", disc, 60.0))
    sun = g.v("ADD", sun, g.tint((1.0, 0.86, 0.62), g.m("MULTIPLY", c1, 3.2)))
    sun = g.v("ADD", sun, g.tint((1.0, 0.62, 0.42), g.m("MULTIPLY", c2, 0.55)))
    sun = g.v("ADD", sun, g.tint((0.80, 0.50, 0.45), g.m("MULTIPLY", c3, 0.04)))

    dbg = os.environ.get("SKY_DEBUG", "")
    if "nostars" in dbg:
        stars = (0.0, 0.0, 0.0)
    if "onlybase" in dbg:
        neb_t, stars, sun = (0.0, 0.0, 0.0), (0.0, 0.0, 0.0), (0.0, 0.0, 0.0)
    total = g.v("ADD", g.v("ADD", g.v("ADD", base, neb_t), stars), sun)
    # camera rays only: the planet and rings are lit by the Sun lamp alone (no noisy world lighting)
    lp = g.node("ShaderNodeLightPath")
    total = g.v("SCALE", total, scale=lp.outputs["Is Camera Ray"])
    g._set(bg.inputs["Color"], total)
    bg.inputs["Strength"].default_value = 1.0
    nt.links.new(bg.outputs[0], out.inputs["Surface"])
    log("world nodes", len(nt.nodes))
    return w


# --------------------------------------------------------------------------- gas giant
def build_planet():
    yaw, pitch, arad = PLANET
    P = ue_dir(yaw, pitch)
    dist = 1000.0
    R = dist * math.tan(math.radians(arad))
    # spin axis: screen-up tilted by -24 deg about the view direction, then 18 deg toward the viewer (ring opening)
    up = Vector((0, 0, 1))
    A = (up - P * up.dot(P)).normalized()
    A = (Matrix.Rotation(math.radians(-24.0), 3, P) @ A).normalized()
    A = (A * math.cos(math.radians(18.0)) - P * math.sin(math.radians(18.0))).normalized()
    X = A.cross(P).normalized()
    Y = A.cross(X).normalized()
    rot = Matrix((X, Y, A)).transposed().to_4x4()
    loc = P * dist

    coll = bpy.context.scene.collection
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=128, v_segments=64, radius=R)
    me = bpy.data.meshes.new("GasGiant")
    bm.to_mesh(me)
    bm.free()
    for p in me.polygons:
        p.use_smooth = True
    planet = bpy.data.objects.new("GasGiant", me)
    coll.objects.link(planet)
    planet.matrix_world = Matrix.Translation(loc) @ rot

    m = bpy.data.materials.new("M_GasGiant")
    nt = m.node_tree
    g = G(nt)
    bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
    tc = g.node("ShaderNodeTexCoord")
    o = g.v("SCALE", tc.outputs["Object"], scale=1.0 / R)
    warp = g.v("SUBTRACT", g.noise(o, 2.2, 6.0, 0.6, color=True), (0.5, 0.5, 0.5))
    ow = g.v("ADD", o, g.v("MULTIPLY", warp, (0.05, 0.05, 0.11)))
    _, _, z = g.sep(ow)
    zf = g.noise(g.v("MULTIPLY", ow, (0.6, 0.6, 4.0)), 1.0, 3.0, 0.45, dist=0.3)
    band = g.m("ADD", g.m("MULTIPLY", z, 1.6), g.m("MULTIPLY", zf, 0.9))
    # one great storm oval, cream with a rust ring
    sp = g.v("SUBTRACT", ow, (0.55, -0.55, -0.32))
    sd = g.v("LENGTH", g.v("MULTIPLY", sp, (1.0, 1.0, 2.4)))
    storm = g.mr(sd, 0.20, 0.10)
    storm_ring = g.m("MULTIPLY", g.mr(sd, 0.26, 0.20), g.mr(sd, 0.12, 0.20))
    mix1 = g.node("ShaderNodeMix", data_type="RGBA", clamp_factor=True)   # band colour linked in later
    g._set(mix1.inputs[0], storm_ring)
    mix1.inputs[6].default_value = (1, 1, 1, 1)
    mix1.inputs[7].default_value = (0.55, 0.16, 0.06, 1.0)
    return planet, m, g, bsdf, o, (P, R, A, loc, rot, band, storm, storm_ring, mix1)


def build_planet_full():
    planet, m, g, bsdf, o, extra = build_planet()
    P, R, A, loc, rot, band, storm, storm_ring, mix1 = extra
    nt = m.node_tree
    band_col = g.ramp(g.m("FRACT", g.m("MULTIPLY", band, 0.55)), [
        (0.0, (1.0, 0.62, 0.26)), (0.16, (0.88, 0.26, 0.06)), (0.30, (1.0, 0.86, 0.58)),
        (0.47, (0.62, 0.13, 0.05)), (0.62, (1.0, 0.70, 0.34)), (0.78, (0.95, 0.42, 0.16)),
        (1.0, (1.0, 0.62, 0.26))])
    nt.links.new(band_col, mix1.inputs[6])
    mix2 = g.node("ShaderNodeMix", data_type="RGBA", clamp_factor=True)
    g._set(mix2.inputs[0], storm)
    nt.links.new(mix1.outputs[2], mix2.inputs[6])
    mix2.inputs[7].default_value = (0.98, 0.90, 0.76, 1.0)
    nt.links.new(mix2.outputs[2], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.85
    bsdf.inputs["Specular IOR Level"].default_value = 0.1
    # thin atmosphere: soft warm limb
    lw = g.node("ShaderNodeLayerWeight")
    lw.inputs["Blend"].default_value = 0.25
    bsdf.inputs["Emission Color"].default_value = (1.0, 0.62, 0.40, 1.0)
    nt.links.new(g.m("MULTIPLY", g.m("POWER", lw.outputs["Facing"], 3.0), 0.06), bsdf.inputs["Emission Strength"])
    planet.data.materials.append(m)

    # rings in the equatorial plane
    bm = bmesh.new()
    segs = 256
    r_in, r_out = 1.32 * R, 2.35 * R
    inner = [bm.verts.new((r_in * math.cos(2 * math.pi * i / segs), r_in * math.sin(2 * math.pi * i / segs), 0)) for i in range(segs)]
    outer = [bm.verts.new((r_out * math.cos(2 * math.pi * i / segs), r_out * math.sin(2 * math.pi * i / segs), 0)) for i in range(segs)]
    for i in range(segs):
        j = (i + 1) % segs
        bm.faces.new((inner[i], inner[j], outer[j], outer[i]))
    me = bpy.data.meshes.new("Rings")
    bm.to_mesh(me)
    bm.free()
    rings = bpy.data.objects.new("Rings", me)
    bpy.context.scene.collection.objects.link(rings)
    rings.matrix_world = Matrix.Translation(loc) @ rot
    rm = bpy.data.materials.new("M_Rings")
    rnt = rm.node_tree
    rnt.nodes.clear()
    rg = G(rnt)
    outn = rg.node("ShaderNodeOutputMaterial")
    tc = rg.node("ShaderNodeTexCoord")
    rr = rg.m("DIVIDE", rg.v("LENGTH", tc.outputs["Object"]), R)  # 1.32 .. 2.35
    t = rg.m("DIVIDE", rg.m("SUBTRACT", rr, 1.32), 1.03)
    comb = rg.node("ShaderNodeCombineXYZ")   # 1D coordinate along the ring radius
    rg._set(comb.inputs[0], rg.m("MULTIPLY", rr, 9.0))
    fine = rg.noise(comb.outputs[0], 1.0, 8.0, 0.7)
    dens = rg.mr(fine, 0.30, 0.72, 0.25, 1.0, smooth=False)
    edge_fade = rg.m("MULTIPLY", rg.mr(t, 0.0, 0.06), rg.mr(t, 1.0, 0.86))
    gap = rg.m("SUBTRACT", 1.0, rg.m("MULTIPLY", rg.mr(rg.m("ABSOLUTE", rg.m("SUBTRACT", t, 0.62)), 0.035, 0.012), 0.92))
    alpha = rg.m("MULTIPLY", rg.m("MULTIPLY", rg.m("MULTIPLY", dens, edge_fade), gap), 0.9)
    rcol = rg.ramp(t, [(0.0, (0.75, 0.55, 0.42)), (0.35, (1.0, 0.88, 0.70)), (0.6, (0.70, 0.88, 1.0)),
                       (0.8, (1.0, 0.80, 0.58)), (1.0, (0.85, 0.62, 0.50))])
    diff = rg.node("ShaderNodeBsdfDiffuse")
    rnt.links.new(rcol, diff.inputs["Color"])
    trl = rg.node("ShaderNodeBsdfTranslucent")
    rnt.links.new(rcol, trl.inputs["Color"])
    em = rg.node("ShaderNodeEmission")
    rnt.links.new(rcol, em.inputs["Color"])
    em.inputs["Strength"].default_value = 0.10
    add1 = rg.node("ShaderNodeMixShader")
    add1.inputs[0].default_value = 0.3
    rnt.links.new(diff.outputs[0], add1.inputs[1])
    rnt.links.new(trl.outputs[0], add1.inputs[2])
    add2 = rg.node("ShaderNodeAddShader")
    rnt.links.new(add1.outputs[0], add2.inputs[0])
    rnt.links.new(em.outputs[0], add2.inputs[1])
    tr = rg.node("ShaderNodeBsdfTransparent")
    mixs = rg.node("ShaderNodeMixShader")
    rnt.links.new(alpha, mixs.inputs[0])
    rnt.links.new(tr.outputs[0], mixs.inputs[1])
    rnt.links.new(add2.outputs[0], mixs.inputs[2])
    rnt.links.new(mixs.outputs[0], outn.inputs["Surface"])
    me.materials.append(rm)
    return planet, rings


# --------------------------------------------------------------------------- post
def box_blur(a, r, axis):
    """box blur radius r (px) along axis; wraps horizontally (axis 1), clamps vertically."""
    if r < 1:
        return a
    mode = "wrap" if axis == 1 else "edge"
    pad = [(0, 0)] * a.ndim
    pad[axis] = (r + 1, r)
    p = np.pad(a, pad, mode=mode)
    c = np.cumsum(p, axis=axis, dtype=np.float32)
    n = a.shape[axis]
    hi = np.take(c, np.arange(2 * r + 1, 2 * r + 1 + n), axis=axis)
    lo = np.take(c, np.arange(0, n), axis=axis)
    return (hi - lo) / float(2 * r + 1)


def hblur_lat(a, r, band=32):
    """horizontal box blur whose radius grows with 1/cos(latitude): round glows on the sphere, also near the poles."""
    h, w = a.shape[:2]
    out = np.empty_like(a)
    for y0 in range(0, h, band):
        y1 = min(h, y0 + band)
        lat = math.radians(-90.0 + 180.0 * ((y0 + y1) * 0.5) / h)
        rr = int(round(r / max(math.cos(lat), 0.02)))
        out[y0:y1] = box_blur(a[y0:y1], min(rr, w // 4), 1)
    return out


def gauss(a, r):
    for _ in range(3):
        a = hblur_lat(a, r)
        a = box_blur(a, r, 0)
    return a


def post(px, width):
    """px: HxWx3 linear. Soft glow + faint cross flare on bright stars (not the sun, it has its corona)."""
    k = width / 8192.0
    lum = px.max(axis=2)
    hot = np.where(lum[:, :, None] > 1.2, px - 1.2 * px / np.maximum(lum[:, :, None], 1e-6), 0.0).astype(np.float32)
    glow = 0.55 * gauss(hot, max(1, int(round(2 * k)))) + 0.30 * gauss(hot, max(1, int(round(6 * k)))) + \
        0.18 * gauss(hot, max(1, int(round(16 * k))))
    very = np.where(lum[:, :, None] > 6.0, hot, 0.0).astype(np.float32)
    flare = np.zeros_like(px)
    if very.any():
        L = max(2, int(round(22 * k)))
        hx = hblur_lat(hblur_lat(very, L), max(1, L // 3))
        vy = box_blur(box_blur(very, L, 0), max(1, L // 3), 0)
        flare = 0.06 * (hx + vy)
    return px + glow * 9.0 + flare * 8.0


def tonemap_srgb(px):
    x = np.maximum(px, 0.0)
    knee = 0.72
    y = np.where(x < knee, x, knee + (1.0 - knee) * (1.0 - np.exp(-(x - knee) / (1.0 - knee))))
    y = np.clip(y, 0.0, 1.0)
    return np.where(y <= 0.0031308, 12.92 * y, 1.055 * np.power(y, 1.0 / 2.4) - 0.055)


def save_float_exr(px, path):
    h, w = px.shape[:2]
    img = bpy.data.images.new("__sky_exr", width=w, height=h, alpha=False, float_buffer=True)
    rgba = np.ones((h, w, 4), dtype=np.float32)
    rgba[:, :, :3] = px
    img.pixels.foreach_set(rgba.ravel())
    sc = bpy.context.scene
    s = sc.render.image_settings
    s.file_format = "OPEN_EXR"
    s.color_depth = "16"
    s.exr_codec = "PIZ"
    s.color_mode = "RGB"
    img.save_render(path, scene=sc)
    bpy.data.images.remove(img)


def save_png8(px, path):
    h, w = px.shape[:2]
    img = bpy.data.images.new("__sky_png", width=w, height=h, alpha=False, float_buffer=False)
    rgba = np.ones((h, w, 4), dtype=np.float32)
    rgba[:, :, :3] = tonemap_srgb(px)
    img.pixels.foreach_set(rgba.ravel())
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


# --------------------------------------------------------------------------- main
def main():
    import argparse
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    ap = argparse.ArgumentParser()
    ap.add_argument("--width", type=int, default=8192)
    ap.add_argument("--samples", type=int, default=24)
    ap.add_argument("--out", required=True)
    ap.add_argument("--tag", default="r1")
    ap.add_argument("--threads", type=int, default=0)
    a = ap.parse_args(argv)
    os.makedirs(a.out, exist_ok=True)
    t0 = time.time()
    sc = bpy.context.scene
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)
    sc.render.engine = "CYCLES"
    sc.cycles.device = "CPU"
    sc.cycles.samples = a.samples
    sc.cycles.use_adaptive_sampling = False
    sc.cycles.use_denoising = False
    sc.cycles.max_bounces = 0
    sc.cycles.transparent_max_bounces = 8
    sc.cycles.sample_clamp_indirect = 2.0
    sc.cycles.seed = 7
    if a.threads:
        sc.render.threads_mode = "FIXED"
        sc.render.threads = a.threads
    sc.render.resolution_x, sc.render.resolution_y = a.width, a.width // 2
    sc.render.resolution_percentage = 100
    sc.render.use_compositing = False
    sc.render.use_sequencer = False
    sc.view_settings.view_transform = "Standard"
    sc.view_settings.look = "None"

    build_world(a.width)
    build_planet_full()

    sun_d = bpy.data.lights.new("Sun", "SUN")
    sun_d.energy = 2.6
    sun_d.angle = math.radians(1.0)
    sun_d.color = (1.0, 0.95, 0.86)
    sun = bpy.data.objects.new("Sun", sun_d)
    sc.collection.objects.link(sun)
    SD = ue_dir(*SUN)
    sun.rotation_euler = (-SD).to_track_quat("-Z", "Y").to_euler()

    cam_d = bpy.data.cameras.new("PanoCam")
    cam_d.type = "PANO"
    cam_d.panorama_type = "EQUIRECTANGULAR"
    cam_d.clip_start = 0.1
    cam_d.clip_end = 100000.0
    cam = bpy.data.objects.new("PanoCam", cam_d)
    sc.collection.objects.link(cam)
    sc.camera = cam
    cam.rotation_euler = (math.radians(90), 0.0, math.radians(-90))

    raw = os.path.join(a.out, "_raw_{}.exr".format(a.tag))
    sc.render.filepath = raw
    s = sc.render.image_settings
    s.file_format = "OPEN_EXR"
    s.color_depth = "32"
    s.exr_codec = "ZIP"
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(a.out, "NebulaSky_{}.blend".format(a.tag)), compress=True)
    log("rendering", a.width, "x", a.width // 2, "samples", a.samples)
    bpy.ops.render.render(write_still=True)
    t_render = time.time() - t0
    log("render done in %.0fs" % t_render)

    img = bpy.data.images.load(raw)
    w, h = img.size
    arr = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(arr)
    bpy.data.images.remove(img)
    px = arr.reshape(h, w, 4)[:, :, :3].copy()
    del arr
    px = post(px, w).astype(np.float32)
    exr = os.path.join(a.out, "T_TD_NebulaSky.exr")
    png = os.path.join(a.out, "T_TD_NebulaSky.png")
    save_float_exr(px, exr)
    save_png8(px, png)
    os.remove(raw)
    lum = px.max(axis=2)
    stats = {
        "size": [w, h], "render_s": round(t_render), "total_s": round(time.time() - t0),
        "mean_rgb": [round(float(c), 4) for c in px.reshape(-1, 3).mean(axis=0)],
        "share_lum_below_0.05": round(float((lum < 0.05).mean()), 3),
        "share_lum_below_0.15": round(float((lum < 0.15).mean()), 3),
        "share_lum_above_1": round(float((lum > 1.0).mean()), 5),
        "max": round(float(lum.max()), 2),
        "exr_mb": round(os.path.getsize(exr) / 1e6, 1), "png_mb": round(os.path.getsize(png) / 1e6, 1),
        "sun_yaw_pitch_ue": SUN, "planet_ue": PLANET,
    }
    # where is the brightest pixel? (should be the sun)
    yy, xx = np.unravel_index(np.argmax(lum), lum.shape)  # row 0 = bottom in Blender's buffer
    u = (xx + 0.5) / w
    v_top = 1.0 - (yy + 0.5) / h
    stats["brightest_px_yaw_pitch_ue"] = [round((u - 0.5) * 360.0, 2), round((0.5 - v_top) * 180.0, 2)]
    with open(os.path.join(a.out, "sky_stats_{}.json".format(a.tag)), "w") as fh:
        json.dump(stats, fh, indent=2)
    print("__RESULT__" + json.dumps(stats))


main()

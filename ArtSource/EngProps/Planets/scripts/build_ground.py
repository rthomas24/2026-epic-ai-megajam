# Tileable ground texture sets (2048, one tile = 4 m) for walking on Verdance and Cinder,
# baked from procedural Blender materials on a flat-torus (4D) coordinate so every layer tiles
# seamlessly, plus eye-height review renders that mimic M_TD_Ground (world-aligned UVs + macro variation).
# Run: blender --background --factory-startup --python build_ground.py -- --ground all --size 2048 --tag v1
import argparse
import json
import math
import os
import sys
import time

import bpy
import numpy as np
from mathutils import Vector, Matrix

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import pl_lib as L   # noqa: E402

ROOT = os.path.dirname(HERE)
TEX = os.path.join(ROOT, "textures")
REVIEW = os.path.join(ROOT, "review")
REPO = os.path.abspath(os.path.join(ROOT, "..", "..", ".."))
SAVED = os.path.join(REPO, "Saved", "PlanetsKit")
TILE_M = 4.0
# Mirrored by MI_Ground_* in ue_import_planets.py
MACRO = dict(scale1=7.3, scale2=31.7, angle1=37.0, angle2=-61.0, strength=0.6, mip=6,
             b_scale=1.37, b_offset=(0.31, 0.57), blend_contrast=1.5, glow_macro=0.8)


def log(*a):
    print("[ground]", *a, flush=True)


def verdance(nb):
    v, w = L.torus_coords(nb, TILE_M)
    off = lambda o: L.torus_offset(nb, v, w, o)                      # noqa: E731
    v1, w1 = off(3.0)
    v2, w2 = off(7.0)
    v3, w3 = off(11.0)
    v4, w4 = off(17.0)
    v5, w5 = off(23.0)

    soil_n = nb.noise(v, 1.6, detail=8, rough=0.6, w=w)
    grain = nb.noise(v1, 28.0, detail=6, rough=0.6, w=w1)
    soil = nb.mix(nb.smooth(soil_n, 0.38, 0.62), "#2A2218", "#1E3020")
    soil = nb.mix(nb.mul(nb.smooth(grain, 0.5, 0.8), 0.5), soil, "#2E261C")

    moss_n = nb.noise(v2, 1.3, detail=7, rough=0.62, w=w2, distortion=0.2)
    moss = nb.smooth(moss_n, 0.44, 0.56)
    fuzz = nb.noise(v3, 55.0, detail=5, rough=0.6, w=w3)
    moss_tone = nb.noise(v3, 2.2, detail=3, w=w3)
    moss_col = nb.mix(nb.smooth(fuzz, 0.35, 0.7), "#1A4A26", "#3A9A40")
    moss_col = nb.mix(nb.mul(nb.smooth(moss_tone, 0.5, 0.7), 0.6), moss_col, "#1E6F5A")

    # pebbles (raised domes) mostly outside the moss, plus fine grit
    pw = nb.noise(v3, 6.0, detail=3, w=w3, out="Color")
    pv = nb.warp(v4, pw, 0.06)
    pd = nb.voronoi(pv, 8.0, feature="F1", w=w4, randomness=0.9)
    pc = nb.voronoi(pv, 8.0, feature="F1", w=w4, randomness=0.9, out="Color")
    pr, pg, _ = nb.sep(pc)
    chosen = nb.mul(nb.smooth(pr, 0.76, 0.80), nb.one_minus(nb.mul(moss, 0.9)))
    dome = nb.powv(nb.clamp01(nb.sub(1.0, nb.mul(pd, 2.6))), 0.8)
    peb = nb.mul(dome, chosen)
    gd = nb.voronoi(v5, 36.0, feature="F1", w=w5)
    gc = nb.voronoi(v5, 36.0, feature="F1", w=w5, out="Color")
    gr, _, _ = nb.sep(gc)
    grit = nb.mul(nb.mul(nb.powv(nb.clamp01(nb.sub(1.0, nb.mul(gd, 3.0))), 0.6), nb.smooth(gr, 0.80, 0.84)),
                  nb.one_minus(moss))
    peb_col = nb.mix(nb.smooth(pg, 0.2, 0.8), "#343A37", "#57534A")

    # faint roots: thin ridge lines of a low-frequency warped noise, half buried
    rn = nb.noise(v1, 0.7, detail=2, rough=0.5, w=w1, distortion=0.35)
    root = nb.smooth(nb.ridge(rn), 0.972, 0.992)
    rn2 = nb.noise(v5, 1.7, detail=2, rough=0.5, w=w5, distortion=0.3)
    root = nb.mx(root, nb.mul(nb.smooth(nb.ridge(rn2), 0.982, 0.995), 0.7))
    root = nb.mul(root, nb.smooth(nb.noise(v2, 0.9, detail=2, w=w2), 0.42, 0.55))

    # bioluminescent moss: glowing dots + a soft glow in some moss patches
    gpatch = nb.mul(nb.smooth(nb.noise(v4, 0.8, detail=3, w=w4), 0.45, 0.58), moss)
    dd = nb.voronoi(v2, 22.0, feature="F1", w=w2)
    dc = nb.voronoi(v2, 22.0, feature="F1", w=w2, out="Color")
    dr, dg, _ = nb.sep(dc)
    dots = nb.mul(nb.one_minus(nb.smooth(dd, 0.12, 0.30)), nb.smooth(dr, 0.45, 0.52))
    glow_col = nb.mix(nb.smooth(dg, 0.75, 0.8), "#3CFFE0", "#B45CFF")
    emis = nb.cadd(nb.cscale(glow_col, nb.mul(dots, gpatch)),
                   nb.cscale(L.hexc("#2AE8C0"), nb.mul(nb.mul(gpatch, nb.smooth(fuzz, 0.5, 0.8)), 0.35)))

    albedo = nb.mix(moss, soil, moss_col)
    albedo = nb.mix(nb.mul(root, 0.5), albedo, "#3A2A1C")
    albedo = nb.mix(nb.clamp01(nb.mul(peb, 3.0)), albedo, peb_col)
    albedo = nb.mix(nb.clamp01(nb.mul(grit, 3.0)), albedo, "#5A5852")
    albedo = nb.mix(nb.mul(nb.mul(dots, gpatch), 0.8), albedo, "#7FFFE8")

    height = nb.add(nb.mul(soil_n, 0.010), nb.mul(grain, 0.004))                   # metres
    height = nb.add(height, nb.mul(moss, nb.add(0.008, nb.mul(fuzz, 0.006))))
    height = nb.add(height, nb.mul(peb, 0.022))
    height = nb.add(height, nb.mul(grit, 0.006))
    height = nb.add(height, nb.mul(root, 0.005))
    rough = nb.add(0.93, nb.mul(moss, -0.08))
    rough = nb.add(rough, nb.mul(nb.clamp01(nb.mul(peb, 3.0)), -0.35))
    return dict(albedo=albedo, emis=emis, height=height, rough=rough)


def cinder(nb):
    v, w = L.torus_coords(nb, TILE_M)
    off = lambda o: L.torus_offset(nb, v, w, o)                      # noqa: E731
    v1, w1 = off(5.0)
    v2, w2 = off(9.0)
    v3, w3 = off(13.0)
    wc = nb.noise(v1, 1.2, detail=2, w=w1, out="Color")
    vw = nb.warp(v, wc, 0.18)
    ced = nb.voronoi(vw, 2.3, feature="DISTANCE_TO_EDGE", w=w)
    ced2 = nb.voronoi(vw, 7.5, feature="DISTANCE_TO_EDGE", w=w, randomness=0.9)
    surf = nb.noise(v2, 14.0, detail=8, rough=0.62, w=w2)
    pits = nb.noise(v3, 40.0, detail=4, w=w3)

    crack = nb.one_minus(nb.smooth(ced, 0.0, 0.035))
    crack2 = nb.mul(nb.one_minus(nb.smooth(ced2, 0.0, 0.022)), 0.7)
    bevel = nb.smooth(ced, 0.0, 0.14)
    lava_mask = nb.smooth(nb.noise(v2, 0.32, detail=3, w=w2), 0.47, 0.56)
    seam = nb.mul(nb.one_minus(nb.smooth(ced, 0.0, 0.016)), lava_mask)
    core = nb.mul(nb.one_minus(nb.smooth(ced, 0.0, 0.007)), lava_mask)
    heat = nb.mul(nb.one_minus(nb.smooth(ced, 0.0, 0.11)), lava_mask)

    ash_n = nb.noise(v3, 0.6, detail=6, rough=0.6, w=w3)
    ash = nb.smooth(nb.add(ash_n, nb.mul(nb.one_minus(bevel), 0.08)), 0.55, 0.66)
    ash = nb.mul(ash, nb.one_minus(nb.mul(lava_mask, 0.75)))
    dust = nb.mul(nb.smooth(nb.noise(v1, 9.0, detail=6, w=w1), 0.55, 0.75), 0.35)

    basalt = nb.mix(nb.smooth(surf, 0.4, 0.65), "#141112", "#272022")
    basalt = nb.mix(nb.mul(nb.smooth(pits, 0.62, 0.8), 0.5), basalt, "#0C0A0A")
    basalt = nb.mix(nb.mul(heat, 0.75), basalt, "#3E130B")
    basalt = nb.mix(nb.clamp01(nb.add(crack, crack2)), basalt, "#090707")
    ash_col = nb.mix(nb.smooth(surf, 0.3, 0.7), "#5A5455", "#7A7273")
    albedo = nb.mix(nb.clamp01(nb.add(ash, dust)), basalt, ash_col)
    albedo = nb.mix(seam, albedo, "#FF6A1A")

    lava_col = nb.mix(core, "#FF4A12", "#FFC060")
    emis = nb.cadd(nb.cscale(lava_col, seam), nb.cscale(L.hexc("#FF3A0A"), nb.mul(heat, 0.10)))

    height = nb.add(nb.mul(bevel, 0.025), nb.mul(surf, 0.008))
    height = nb.sub(height, nb.mul(crack2, 0.006))
    height = nb.sub(height, nb.mul(nb.smooth(pits, 0.62, 0.8), 0.003))
    height = nb.add(height, nb.mul(ash, 0.006))
    rough = nb.add(0.72, nb.mul(surf, 0.15))
    rough = nb.add(rough, nb.mul(ash, 0.12))
    rough = nb.add(rough, nb.mul(seam, -0.4))
    return dict(albedo=albedo, emis=emis, height=height, rough=rough)


GROUNDS = {
    "Verdance": dict(graph=verdance, macro_tint_a=(0.80, 0.80, 0.72, 1.0), macro_tint_b=(1.0, 1.15, 1.05, 1.0), normal_strength=1.0, emissive_strength=4.0, sky="#0E2A33", sun=3.0,
                     sun_color="#E8FFF4", sun_elev=28.0),
    "Cinder": dict(graph=cinder, macro_tint_a=(0.85, 0.82, 0.82, 1.0), macro_tint_b=(1.1, 1.0, 0.95, 1.0), normal_strength=1.0, emissive_strength=6.0, sky="#2A120C", sun=3.0,
                   sun_color="#FFD2B0", sun_elev=24.0),
}


def make_plane(name, size):
    me = bpy.data.meshes.new(name)
    h = size / 2.0
    me.from_pydata([(-h, -h, 0), (h, -h, 0), (h, h, 0), (-h, h, 0)], [], [(0, 1, 2, 3)])
    uvl = me.uv_layers.new(name="UVMap")
    for li, uv in zip(range(4), ((0, 0), (1, 0), (1, 1), (0, 1))):
        uvl.data[li].uv = uv
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    return ob


def stage_bake(name, size):
    cfg = GROUNDS[name]
    scene = L.fresh_scene()
    L.setup_cycles_bake(scene)
    ob = make_plane("BAKE_" + name, TILE_M)
    mat = bpy.data.materials.new("PROC_G_" + name)
    ob.data.materials.append(mat)
    nb = L.NB(mat.node_tree)
    s = cfg["graph"](nb)
    t0 = time.time()
    albedo = L.bake_socket(ob, mat, s["albedo"], size, size, "albedo", scene)
    emis = L.bake_socket(ob, mat, s["emis"], size, size, "emis", scene)
    pack = L.bake_socket(ob, mat, nb.rgb(s["height"], s["rough"], 0.0), size, size, "pack", scene)
    log(name, "baked %d in %.1fs" % (size, time.time() - t0))
    hgt = pack[..., 0].astype(np.float64)
    px = TILE_M / size
    nrm = L.normal_from_height(hgt, px, px, cfg["normal_strength"], wrap_y=True)
    blur = L.box_blur(hgt, 6, wrap_y=True)
    ao = np.clip(1.0 - np.maximum(blur - hgt, 0.0) * 40.0, 0.45, 1.0)
    orm = np.stack([ao, np.clip(pack[..., 1], 0.05, 1.0), np.zeros_like(hgt)], axis=-1)
    short = "Ground_" + name
    paths = dict(
        BC=L.save_png(os.path.join(TEX, "T_%s_BC.png" % short), albedo[..., :3], srgb_encode=True),
        N=L.save_png(os.path.join(TEX, "T_%s_N.png" % short), nrm),
        ORM=L.save_png(os.path.join(TEX, "T_%s_ORM.png" % short), orm),
        E=L.save_png(os.path.join(TEX, "T_%s_E.png" % short), emis[..., :3], srgb_encode=True))
    lum = albedo[..., 0] * 0.2126 + albedo[..., 1] * 0.7152 + albedo[..., 2] * 0.0722
    # seam check: wrap difference vs interior neighbour difference
    seam_x = float(np.abs(albedo[:, 0, :3] - albedo[:, -1, :3]).mean())
    inner_x = float(np.abs(albedo[:, 1, :3] - albedo[:, 0, :3]).mean())
    small_l = lum.reshape(32, size // 32, 32, size // 32).mean(axis=(1, 3))
    stats = dict(macro_mean_lum=float(lum.mean()), macro_std_rel=float(small_l.std() / small_l.mean()), emissive_cover=float((emis[..., :3].max(-1) > 0.1).mean()),
                 height_range_m=[float(hgt.min()), float(hgt.max())], seam_diff=seam_x, interior_diff=inner_x)
    with open(os.path.join(TEX, "T_%s_stats.json" % short), "w") as f:
        json.dump(stats, f, indent=1)
    # blurred copy standing in for the macro sample's forced mip level in Unreal (review only)
    small = albedo[..., :3].reshape(64, size // 64, 64, size // 64, 3).mean(axis=(1, 3))
    L.save_png(os.path.join(SAVED, "T_%s_BC_mip.png" % short), small, srgb_encode=True)
    log(name, paths, stats)
    return stats


def preview_ground_material(name, cfg, stats):
    short = "Ground_" + name
    mat = bpy.data.materials.new("PREVIEW_G_" + name)
    nt = mat.node_tree
    nb = L.NB(nt)
    bsdf = [n for n in nt.nodes if n.bl_idname == "ShaderNodeBsdfPrincipled"][0]
    tc = nb.node("ShaderNodeTexCoord")
    pos = nb.out(tc, "Object")                         # plane at the origin: object == world (metres)
    uv = nb.vscale(pos, 1.0 / TILE_M)

    def tex(sfx, noncolor, vec, path=None):
        n = nb.node("ShaderNodeTexImage")
        n.image = L.load_img(path or os.path.join(TEX, "T_%s_%s.png" % (short, sfx)), noncolor)
        nb.inp(n, "Vector", vec)
        return n

    def rot_uv(angle_deg, scale):
        a = math.radians(angle_deg)
        x, y, _ = nb.sep(pos)
        u = nb.add(nb.mul(x, math.cos(a)), nb.mul(y, -math.sin(a)))
        v = nb.add(nb.mul(x, math.sin(a)), nb.mul(y, math.cos(a)))
        return nb.vscale(nb.comb(u, v, 0.0), 1.0 / (TILE_M * scale))
    uvb = nb.vadd(nb.vscale(pos, 1.0 / (TILE_M * MACRO["b_scale"])), (MACRO["b_offset"][0], MACRO["b_offset"][1], 0.0))
    A = [tex(k, nc, uv) for k, nc in (("BC", False), ("N", True), ("ORM", True), ("E", False))]
    Bs = [tex(k, nc, uvb) for k, nc in (("BC", False), ("N", True), ("ORM", True), ("E", False))]
    mip = os.path.join(SAVED, "T_%s_BC_mip.png" % short)
    m1 = tex(None, False, rot_uv(MACRO["angle1"], MACRO["scale1"]), mip)
    m2 = tex(None, False, rot_uv(MACRO["angle2"], MACRO["scale2"]), mip)

    def lum(sock):
        d = nb.node("ShaderNodeVectorMath", operation="DOT_PRODUCT")
        nb.put(d.inputs[0], sock)
        d.inputs[1].default_value = (0.2126, 0.7152, 0.0722)
        return nb.out(d, "Value")
    k = 1.0 / (2.0 * stats["macro_std_rel"] * stats["macro_mean_lum"])
    d1 = nb.mul(nb.sub(lum(nb.out(m1, "Color")), stats["macro_mean_lum"]), k)     # ~ -1..1
    d2 = nb.mul(nb.sub(lum(nb.out(m2, "Color")), stats["macro_mean_lum"]), k)
    blend = nb.clamp01(nb.add(nb.mul(d2, MACRO["blend_contrast"]), 0.5))
    bc, nn, orm, em = [nb.mix(blend, nb.out(a_, "Color"), nb.out(b_, "Color")) for a_, b_ in zip(A, Bs)]
    factor = nb.add(1.0, nb.mul(d1, MACRO["strength"] * 0.35))
    gm = nb.clamp01(nb.add(nb.mul(d1, 1.2), 0.6))
    glowmask = nb.add(1.0, nb.mul(nb.sub(gm, 1.0), MACRO["glow_macro"]))
    base = nb.cscale(bc, factor)
    tint = nb.mix(nb.clamp01(nb.add(nb.mul(d1, 0.5), 0.5)), cfg["macro_tint_a"], cfg["macro_tint_b"])
    base = nb.cmul(base, tint)
    r, g, b = nb.sep(nn)
    nmap = nb.node("ShaderNodeNormalMap")
    nb.inp(nmap, "Color", nb.rgb(r, nb.one_minus(g), b))
    ao, ro, _ = nb.sep(orm)
    nb.put(bsdf.inputs["Base Color"], nb.cscale(base, ao))
    nb.put(bsdf.inputs["Roughness"], ro)
    nb.put(bsdf.inputs["Normal"], nb.out(nmap, "Normal"))
    glow = nb.mul(glowmask, cfg["emissive_strength"] * 0.4)
    nb.put(bsdf.inputs["Emission Color"], nb.cscale(em, glow))
    bsdf.inputs["Emission Strength"].default_value = 1.0
    return mat


def stage_review(name, tag, res=(1600, 900)):
    cfg = GROUNDS[name]
    with open(os.path.join(TEX, "T_Ground_%s_stats.json" % name)) as f:
        stats = json.load(f)
    scene = L.fresh_scene()
    world = bpy.data.worlds.new("sky")
    bg = world.node_tree.nodes.get("Background")
    bg.inputs["Color"].default_value = L.hexc(cfg["sky"])
    bg.inputs["Strength"].default_value = 1.0
    scene.world = world
    L.set_view(scene, exposure=0.0, look="None")
    plane = make_plane("ground", 1200.0)
    plane.data.materials.append(preview_ground_material(name, cfg, stats))
    sd = bpy.data.lights.new("sun", "SUN")
    sd.energy = cfg["sun"]
    sd.color = L.hexc(cfg["sun_color"])[:3]
    sd.angle = math.radians(1.0)
    sun = bpy.data.objects.new("sun", sd)
    scene.collection.objects.link(sun)
    sun.rotation_euler = (math.radians(90 - cfg["sun_elev"]), 0.0, math.radians(140))
    cd = bpy.data.cameras.new("cam")
    cd.sensor_fit = "HORIZONTAL"
    cd.sensor_width = 36.0
    cd.lens = 18.0                                       # 90 degree horizontal FOV, like the game
    cd.clip_end = 2000.0
    cam = bpy.data.objects.new("cam", cd)
    scene.collection.objects.link(cam)
    scene.camera = cam
    paths = []
    for label, pitch in (("eye", 78.0), ("down", 45.0)):
        cam.location = (3.1, -7.3, 1.8)
        cam.rotation_euler = (math.radians(pitch), 0.0, math.radians(20))
        paths.append(L.render_to(scene, os.path.join(REVIEW, "Ground_%s_%s_%s.png" % (name, tag, label)), res,
                                 samples=32))
    log(name, "review", paths)
    return paths


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    ap = argparse.ArgumentParser()
    ap.add_argument("--ground", default="all")
    ap.add_argument("--size", type=int, default=2048)
    ap.add_argument("--tag", default="v1")
    ap.add_argument("--stage", default="all")
    a = ap.parse_args(argv)
    names = list(GROUNDS) if a.ground == "all" else [a.ground]
    out = {}
    for n in names:
        if a.stage in ("bake", "all"):
            out[n] = stage_bake(n, a.size)
        if a.stage in ("review", "all"):
            stage_review(n, a.tag)
    print("__RESULT__", out, flush=True)


main()

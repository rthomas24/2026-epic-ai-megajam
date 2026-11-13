# Planets seen from space: meshes, baked equirectangular texture sets, review renders, FBX export.
# Run (headless):
#   blender --background --factory-startup --python build_planets.py -- --stage all --planet all --width 4096
# Stages: meshes (build + export FBX), bake (texture sets), review (renders), all.
import argparse
import math
import os
import sys
import time

import bmesh
import bpy
import numpy as np
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import pl_lib as L            # noqa: E402
import planet_graphs as G     # noqa: E402

ROOT = os.path.dirname(HERE)                      # ArtSource/EngProps/Planets
TEX = os.path.join(ROOT, "textures")
MESH = os.path.join(ROOT, "meshes")
REVIEW = os.path.join(ROOT, "review")

# Look parameters, mirrored by the Unreal material instances (ue_import_planets.py reads PLANETS too).
PLANETS = {
    "Verdance": dict(graph=G.verdance, height_amp=0.020, cloud_color="#F4FBFF", cloud_opacity=0.92,
                     atmo_color="#3CF2D0", atmo_intensity=3.0, rim_power=4.0, surface_rim="#40E8FF",
                     emissive_strength=6.0, ring=True),
    "Cinder": dict(graph=G.cinder, height_amp=0.022, cloud_color="#8C8086", cloud_opacity=0.80,
                   atmo_color="#FF7A2A", atmo_intensity=2.6, rim_power=4.5, surface_rim="#FF6A2A", surface_rim_strength=0.25,
                   emissive_strength=8.0, ring=False),
}
SUN_DIR = Vector((0.8, -0.45, 0.35)).normalized()      # towards the sun, used by the review lighting
RING_INNER, RING_OUTER = 1.4, 2.3
ATMO_RADIUS = 1.025


def log(*a):
    print("[planets]", *a, flush=True)


# ------------------------------------------------------------------ meshes
def make_sphere(name, radius, useg, vseg):
    me = bpy.data.meshes.new(name)
    bm = bmesh.new()
    bm.loops.layers.uv.new("UVMap")
    bmesh.ops.create_uvsphere(bm, u_segments=useg, v_segments=vseg, radius=radius, calc_uvs=True)
    bm.to_mesh(me)
    bm.free()
    me.shade_smooth()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    return ob


def make_ring(name, inner, outer, seg=256):
    me = bpy.data.meshes.new(name)
    verts, faces, uvs = [], [], []
    for i in range(seg + 1):
        a = 2.0 * math.pi * i / seg
        c, s = math.cos(a), math.sin(a)
        verts.append((inner * c, inner * s, 0.0))
        verts.append((outer * c, outer * s, 0.0))
    for i in range(seg):
        a, b, c, d = 2 * i, 2 * i + 1, 2 * i + 3, 2 * i + 2
        faces.append((a, b, c, d))
    me.from_pydata(verts, [], faces)
    uvl = me.uv_layers.new(name="UVMap")
    for poly in me.polygons:
        for li in poly.loop_indices:
            vi = me.loops[li].vertex_index
            i = vi // 2
            uvl.data[li].uv = (float(vi % 2), i / seg)     # U radial (0 inner -> 1 outer), V around
    me.update()
    # +Z facing; the material is two-sided
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    return ob


def tri_count(ob):
    me = ob.data
    me.calc_loop_triangles()
    return len(me.loop_triangles)


def export_fbx(ob, path):
    for o in bpy.context.scene.objects:
        o.select_set(False)
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.export_scene.fbx(
        filepath=path, use_selection=True, object_types={"MESH"},
        global_scale=1.0, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
        axis_forward="-Z", axis_up="Y", use_space_transform=True, bake_space_transform=False,
        use_mesh_modifiers=True, mesh_smooth_type="FACE", use_triangles=True, use_tspace=True,
        use_custom_props=False, add_leaf_bones=False, bake_anim=False, colors_type="NONE",
        path_mode="AUTO", embed_textures=False)


def stage_meshes():
    L.fresh_scene()
    sph = make_sphere("SM_Planet_Sphere", 1.0, 192, 96)
    atm = make_sphere("SM_Planet_Atmosphere", ATMO_RADIUS, 128, 64)
    ring = make_ring("SM_Planet_Ring", RING_INNER, RING_OUTER, 256)
    info = {}
    for ob in (sph, atm, ring):
        export_fbx(ob, os.path.join(MESH, ob.name + ".fbx"))
        info[ob.name] = tri_count(ob)
    # check the sphere UVs are equirectangular (u along longitude, v along latitude)
    me = sph.data
    uvl = me.uv_layers[0].data
    err = 0.0
    for poly in me.polygons[:4000]:
        for li in poly.loop_indices:
            co = me.vertices[me.loops[li].vertex_index].co
            v_expect = 0.5 + math.asin(max(-1.0, min(1.0, co.z))) / math.pi
            err = max(err, abs(uvl[li].uv[1] - v_expect))
    info["uv_v_max_error"] = round(err, 5)
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(ROOT, "Planets_meshes.blend"))
    log("meshes", info)
    return info


# ------------------------------------------------------------------ bake
def stage_bake(name, width):
    cfg = PLANETS[name]
    scene = L.fresh_scene()
    L.setup_cycles_bake(scene, samples=1)
    ob = make_sphere("BAKE_" + name, 1.0, 192, 96)
    mat = bpy.data.materials.new("PROC_" + name)
    ob.data.materials.append(mat)
    nb = L.NB(mat.node_tree)
    s = cfg["graph"](nb)
    W, H = width, width // 2
    t0 = time.time()
    albedo = L.bake_socket(ob, mat, s["albedo"], W, H, "albedo", scene)
    emis = L.bake_socket(ob, mat, s["emis"], W, H, "emis", scene)
    pack = L.bake_socket(ob, mat, nb.rgb(s["height"], s["rough"], s["cloud"]), W, H, "pack", scene)
    log(name, "baked %dx%d in %.1fs" % (W, H, time.time() - t0))
    hgt = pack[..., 0].astype(np.float64)
    rough = pack[..., 1]
    cloud = pack[..., 2]
    # per-row pixel size on the unit sphere (rows bottom-up = latitude -90..90)
    lat = ((np.arange(H) + 0.5) / H - 0.5) * math.pi
    coslat = np.maximum(np.cos(lat), 0.03)[:, None]
    px_w = 2.0 * math.pi * coslat / W
    px_h = math.pi / H
    nrm = L.normal_from_height(hgt * cfg["height_amp"], px_w, px_h, 1.0, wrap_y=False)
    # fade the normal to flat at the poles (texel stretch)
    polefade = np.clip((coslat - 0.03) / 0.12, 0.0, 1.0)[..., None]
    nrm = nrm * polefade + np.array([0.5, 0.5, 1.0]) * (1.0 - polefade)
    blur = L.box_blur(hgt, max(2, W // 512), wrap_y=False)
    ao = np.clip(1.0 - np.maximum(blur - hgt, 0.0) * 6.0, 0.6, 1.0)
    orm = np.stack([ao, rough, np.zeros_like(rough)], axis=-1)
    short = "Planet_" + name
    out = {}
    out["BC"] = L.save_png(os.path.join(TEX, "T_%s_BC.png" % short), albedo[..., :3], srgb_encode=True)
    out["N"] = L.save_png(os.path.join(TEX, "T_%s_N.png" % short), nrm)
    out["ORM"] = L.save_png(os.path.join(TEX, "T_%s_ORM.png" % short), orm)
    out["E"] = L.save_png(os.path.join(TEX, "T_%s_E.png" % short), emis[..., :3], srgb_encode=True)
    out["Clouds"] = L.save_png(os.path.join(TEX, "T_%s_Clouds.png" % short), cloud[..., None])
    stats = dict(land_or_lava_emissive_cover=float((emis[..., :3].max(axis=-1) > 0.15).mean()),
                 cloud_cover=float((cloud > 0.5).mean()), albedo_mean=albedo[..., :3].mean(axis=(0, 1)).round(3).tolist(),
                 height_mean=float(hgt.mean()))
    log(name, "textures", out, stats)
    return stats


# ------------------------------------------------------------------ review
def preview_planet_material(name, cfg):
    short = "Planet_" + name
    mat = bpy.data.materials.new("PREVIEW_" + name)
    nt = mat.node_tree
    nb = L.NB(nt)
    bsdf = [n for n in nt.nodes if n.bl_idname == "ShaderNodeBsdfPrincipled"][0]

    def tex(sfx, noncolor, vec=None):
        n = nb.node("ShaderNodeTexImage")
        n.image = L.load_img(os.path.join(TEX, "T_%s_%s.png" % (short, sfx)), noncolor)
        n.interpolation = "Cubic" if sfx == "Clouds" else "Linear"
        if vec is not None:
            nb.inp(n, "Vector", vec)
        return n
    bc, nn, orm, em = tex("BC", False), tex("N", True), tex("ORM", True), tex("E", False)
    # clouds: static offset in U stands in for the time-driven pan in Unreal
    uv = nb.uv()
    cl = tex("Clouds", True, nb.vadd(uv, (0.013, 0.0, 0.0)))
    cloud = nb.mul(nb.out(cl, "Color"), cfg["cloud_opacity"])
    # DirectX -> OpenGL for Blender: flip green
    r, g, b = nb.sep(nb.out(nn, "Color"))
    nmap = nb.node("ShaderNodeNormalMap")
    nb.inp(nmap, "Color", nb.rgb(r, nb.one_minus(g), b))
    nb.inp(nmap, "Strength", 1.0)
    base = nb.mix(cloud, nb.out(bc, "Color"), cfg["cloud_color"])
    ro, rg, rb = nb.sep(nb.out(orm, "Color"))
    rough = nb.add(nb.mul(rg, nb.one_minus(cloud)), nb.mul(cloud, 0.9))
    # night-side mask for the glow (Unreal: SunDirection param, NightGlowMask)
    geo = nb.node("ShaderNodeNewGeometry")
    dotn = nb.node("ShaderNodeVectorMath", operation="DOT_PRODUCT")
    nb.put(dotn.inputs[0], nb.out(geo, "Normal"))
    dotn.inputs[1].default_value = tuple(SUN_DIR)
    ndl = nb.out(dotn, "Value")
    night = nb.clamp01(nb.mul(nb.sub(0.15, ndl), 3.0))
    glowmask = nb.add(0.15, nb.mul(night, 0.85))
    e = nb.cscale(nb.out(em, "Color"), nb.mul(nb.mul(glowmask, nb.one_minus(cloud)), cfg["emissive_strength"] * 0.25))
    # day-side limb in-scatter
    lw = nb.node("ShaderNodeLayerWeight")
    nb.inp(lw, "Blend", 0.5)
    fres = nb.powv(nb.out(lw, "Facing"), 4.0)
    day = nb.clamp01(nb.add(nb.mul(ndl, 1.2), 0.25))
    rim = nb.cscale(L.hexc(cfg["surface_rim"]), nb.mul(nb.mul(fres, day), cfg.get("surface_rim_strength", 0.5)))
    nb.put(bsdf.inputs["Base Color"], base)
    nb.put(bsdf.inputs["Roughness"], rough)
    nb.put(bsdf.inputs["Normal"], nb.out(nmap, "Normal"))
    nb.put(bsdf.inputs["Emission Color"], nb.cadd(e, rim))
    bsdf.inputs["Emission Strength"].default_value = 1.0
    bsdf.inputs["Specular IOR Level"].default_value = 0.5
    return mat


def preview_atmo_material(name, cfg):
    mat = bpy.data.materials.new("PREVIEW_ATMO_" + name)
    nt = mat.node_tree
    for n in list(nt.nodes):
        if n.bl_idname != "ShaderNodeOutputMaterial":
            nt.nodes.remove(n)
    out = nt.nodes[0]
    nb = L.NB(nt)
    lw = nb.node("ShaderNodeLayerWeight")
    nb.inp(lw, "Blend", 0.5)
    x = nb.one_minus(nb.out(lw, "Facing"))         # ~ N.V : 1 centre, 0 at the shell silhouette
    edge = nb.clamp01(nb.mul(x, cfg.get("edge_soft", 4.0)))
    glow = nb.mul(nb.powv(nb.one_minus(x), cfg["rim_power"]), nb.powv(edge, 3.0))
    geo = nb.node("ShaderNodeNewGeometry")
    dotn = nb.node("ShaderNodeVectorMath", operation="DOT_PRODUCT")
    nb.put(dotn.inputs[0], nb.out(geo, "Normal"))
    dotn.inputs[1].default_value = tuple(SUN_DIR)
    day = nb.clamp01(nb.add(nb.mul(nb.out(dotn, "Value"), 0.8), 0.45))
    em = nb.node("ShaderNodeEmission")
    nb.inp(em, "Color", L.hexc(cfg["atmo_color"]))
    nb.inp(em, "Strength", nb.mul(nb.mul(glow, day), cfg["atmo_intensity"]))
    tr = nb.node("ShaderNodeBsdfTransparent")
    add = nb.node("ShaderNodeAddShader")
    nt.links.new(tr.outputs[0], add.inputs[0])
    nt.links.new(em.outputs[0], add.inputs[1])
    nt.links.new(add.outputs[0], out.inputs["Surface"])
    try:
        mat.surface_render_method = "BLENDED"
    except Exception:
        pass
    mat.use_backface_culling = True
    return mat


def preview_ring_material():
    mat = bpy.data.materials.new("PREVIEW_RING")
    nt = mat.node_tree
    for n in list(nt.nodes):
        if n.bl_idname != "ShaderNodeOutputMaterial":
            nt.nodes.remove(n)
    out = nt.nodes[0]
    nb = L.NB(nt)
    t = nb.node("ShaderNodeTexImage")
    t.image = L.load_img(os.path.join(TEX, "T_PlanetRing_Verdance.png"), False)
    t.image.alpha_mode = "STRAIGHT"
    em = nb.node("ShaderNodeEmission")
    nb.inp(em, "Color", nb.out(t, "Color"))
    nb.inp(em, "Strength", 1.2)
    diff = nb.node("ShaderNodeBsdfDiffuse")
    nb.inp(diff, "Color", nb.out(t, "Color"))
    add = nb.node("ShaderNodeAddShader")
    nt.links.new(diff.outputs[0], add.inputs[0])
    nt.links.new(em.outputs[0], add.inputs[1])
    tr = nb.node("ShaderNodeBsdfTransparent")
    mix = nb.node("ShaderNodeMixShader")
    nb.put(mix.inputs[0], nb.mul(nb.out(t, "Alpha"), 0.85))
    nt.links.new(tr.outputs[0], mix.inputs[1])
    nt.links.new(add.outputs[0], mix.inputs[2])
    nt.links.new(mix.outputs[0], out.inputs["Surface"])
    try:
        mat.surface_render_method = "BLENDED"
    except Exception:
        pass
    mat.use_backface_culling = False
    return mat


def make_ring_texture():
    """Banded ring: 2048 along the radius x 16 around. Pale gold and teal bands, gaps, soft edges."""
    W, H = 2048, 16
    u = (np.arange(W) + 0.5) / W
    rng = np.random.default_rng(7)
    dens = np.zeros(W)
    for f, a in ((1.3, 0.30), (3.7, 0.22), (8.0, 0.14), (19.0, 0.08), (47.0, 0.05), (113.0, 0.03)):
        dens += a * np.sin(2 * math.pi * (f * u + rng.random()))
    dens = 0.55 + dens
    # three divisions: one wide dark gap, two thin ones
    for c, wdt, depth in ((0.36, 0.030, 0.95), (0.62, 0.010, 0.7), (0.80, 0.007, 0.6)):
        dens *= 1.0 - depth * np.exp(-((u - c) / wdt) ** 2)
    edge = np.clip(u / 0.06, 0, 1) * np.clip((1 - u) / 0.10, 0, 1)
    alpha = 0.85 * np.clip(dens, 0, 1) ** 1.5 * edge
    gold = np.array(L.hexc("#F2DDA0")[:3])
    teal = np.array(L.hexc("#5ED8C8")[:3])
    mixf = 0.5 + 0.5 * np.sin(2 * math.pi * (2.2 * u + 0.15)) * np.clip(np.sin(2 * math.pi * 9 * u) + 0.4, 0, 1)
    mixf = np.clip(mixf, 0, 1)[:, None]
    col = gold * (1 - mixf) + teal * mixf
    col *= (0.75 + 0.25 * np.clip(dens, 0, 1))[:, None]
    img = np.zeros((H, W, 4))
    img[..., :3] = col[None, :, :]
    img[..., 3] = alpha[None, :]
    return L.save_png(os.path.join(TEX, "T_PlanetRing_Verdance.png"), img, srgb_encode=True)


def look_at(cam, target, up=(0.0, 0.0, 1.0)):
    from mathutils import Matrix
    f = (Vector(target) - Vector(cam.location)).normalized()
    r = f.cross(Vector(up)).normalized()
    u = r.cross(f).normalized()
    m = Matrix((r, u, -f)).transposed()          # columns: camera X, Y, Z (camera looks down -Z)
    cam.rotation_euler = m.to_euler()


def stage_review(name, tag, res=(1600, 900), samples=48):
    cfg = PLANETS[name]
    scene = L.fresh_scene()
    world = bpy.data.worlds.new("black")
    world.color = (0, 0, 0)
    bg = world.node_tree.nodes.get("Background")
    if bg:
        bg.inputs["Color"].default_value = (0.0, 0.0, 0.0, 1.0)
    scene.world = world
    L.set_view(scene, exposure=-0.3, look="None")
    sph = make_sphere("P", 1.0, 192, 96)
    sph.data.materials.append(preview_planet_material(name, cfg))
    atm = make_sphere("A", ATMO_RADIUS, 128, 64)
    atm.data.materials.append(preview_atmo_material(name, cfg))
    root = bpy.data.objects.new("root", None)
    scene.collection.objects.link(root)
    sph.parent = root
    atm.parent = root
    if cfg["ring"]:
        ring = make_ring("R", RING_INNER, RING_OUTER, 256)
        ring.data.materials.append(preview_ring_material())
        ring.parent = root
    root.rotation_euler = (math.radians(14), math.radians(-24), 0.0)       # tilted planet; the ring is equatorial
    sun_d = bpy.data.lights.new("sun", "SUN")
    sun_d.energy = 4.5
    sun_d.angle = math.radians(0.6)
    sun = bpy.data.objects.new("sun", sun_d)
    scene.collection.objects.link(sun)
    sun.rotation_euler = (-SUN_DIR).to_track_quat("-Z", "Y").to_euler()
    cam_d = bpy.data.cameras.new("cam")
    cam_d.sensor_fit = "HORIZONTAL"
    cam_d.sensor_width = 36.0
    cam_d.clip_start = 0.001
    cam = bpy.data.objects.new("cam", cam_d)
    scene.collection.objects.link(cam)
    scene.camera = cam
    paths = []
    # 1. whole disc, 60% of frame height
    cam_d.lens = 50.0
    vfov = 2 * math.atan((36.0 * res[1] / res[0]) / 2 / 50.0)
    dist = 1.0 / math.sin(0.6 * vfov / 2)
    cam.location = (0.0, -dist, 0.0)
    look_at(cam, (0, 0, 0))
    paths.append(L.render_to(scene, os.path.join(REVIEW, "%s_%s_1_disc.png" % (name, tag)), res, samples=samples))
    # 2. limb close-up where the terminator meets the limb
    cam_d.lens = 35.0
    term = Vector((-0.40, 0.0, 0.916)).normalized()
    cam.location = Vector((0.0, -1.0, 0.0)) * 1.25 + term * 0.55
    look_at(cam, term * 0.92 + Vector((0.12, 0.0, -0.05)))
    paths.append(L.render_to(scene, os.path.join(REVIEW, "%s_%s_2_limb.png" % (name, tag)), res, samples=samples))
    # 3. grazing view just above the surface near the terminator, looking along the ground
    t = SUN_DIR.cross(Vector((0, 0, 1))).normalized()
    p0 = (SUN_DIR * 0.22 + t * math.sqrt(1 - 0.22 ** 2)).normalized()
    cam_d.lens = 22.0
    cam.location = p0 * 1.045
    fwd = (-SUN_DIR - p0 * (-SUN_DIR).dot(p0)).normalized()          # towards the night side
    side = fwd.cross(p0).normalized()
    fwd = (fwd.lerp(side, 0.35)).normalized()
    target = cam.location + fwd * 0.3 - p0 * 0.11
    look_at(cam, target, up=p0)
    paths.append(L.render_to(scene, os.path.join(REVIEW, "%s_%s_3_grazing.png" % (name, tag)), res, samples=samples))
    log(name, "review", paths)
    return paths


def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    ap = argparse.ArgumentParser()
    ap.add_argument("--stage", default="all")
    ap.add_argument("--planet", default="all")
    ap.add_argument("--width", type=int, default=4096)
    ap.add_argument("--tag", default="v1")
    a = ap.parse_args(argv)
    names = list(PLANETS) if a.planet == "all" else [a.planet]
    res = {}
    if a.stage in ("meshes", "all"):
        res["meshes"] = stage_meshes()
    if a.stage in ("bake", "all", "bakereview"):
        make_ring_texture()
        for n in names:
            res[n] = stage_bake(n, a.width)
    if a.stage in ("review", "all", "bakereview"):
        for n in names:
            stage_review(n, a.tag)
    print("__RESULT__", res, flush=True)


main()

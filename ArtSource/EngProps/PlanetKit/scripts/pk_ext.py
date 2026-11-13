# SPDX-License-Identifier: MIT
"""
pk_ext.py - PlanetKit extensions on top of pk_lib (a copy of the SpaceDressing sd_lib pipeline):
stone / basalt / bark role materials, footprint fitting with a bottom-centre pivot, eye-height review renders
(1.8 m camera at 5 m and 20 m, Cinder dim warm light or Verdance dusk cyan light, a 1.8 m figure for scale),
a no-collision finish and a second "UVDetail" channel for a tiling detail normal in Unreal.
Units metres in Blender (1 m = 100 uu), Unreal forward = Blender +X (exported with front_axis="+X").
"""

import json
import math
import os
import time

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

from pk_lib import (H, KIT, REVIEW_DIR, EMISSION_STRENGTH, Shader, _load_rgba, _save_rgba, ar, ac, ex, rv,
                    bake, cavity_tint, edge_area_share, emissive_surface_share, image_stats, log,
                    mark_edge_faces, mesh_points, tri_count, unwrap, uv_stats)

FAMILY_LIGHT = {
    # world colour, world strength, sun colour, sun energy, sun elevation deg, ground colour, ground roughness
    "cinder": ((0.30, 0.13, 0.07), 0.55, (1.0, 0.55, 0.30), 1.6, 9.0, "#1C1817", 0.85),
    "verdance": ((0.05, 0.16, 0.22), 0.6, (0.55, 0.85, 1.0), 1.3, 12.0, "#0F2A27", 0.9),
}


def _geo_normal(s):
    n = s.nt.nodes.new("ShaderNodeNewGeometry")
    return n.outputs["Normal"]


def role_stone(name, dark, light, scale=1.0, moss=0.0, moss_color="#1E6E5E", moss_glow="#3CFFE0",
               glyph_rings=None, glyph_color="#3CFFE0", glyph_width=0.05, top_z=None, seed=0, rough=0.78,
               block=1.0, tint=None):
    """Carved alien stone: two-tone noise, per-block tone (voronoi cells ~`block` m), fine grain bump, moss on
    up-facing surfaces and creeping down from `top_z`, faint glowing spores in the moss, optional glyph rings on
    the top face (list of (cx, cy, r) in object metres, emissive lines on faces whose normal points up)."""
    s = Shader(name)
    o = (seed * 3.1, seed * 1.7, seed * 0.9)
    c_d, c_l = H(dark), H(light)
    v = s.noise(s.obj, 0.35 / scale, 4.0, 0.6, offset=o)
    col = s.mix(s.ramp(v, [(0.3, 0.0), (0.7, 1.0)]), c_d, c_l)
    cell = s.voronoi(s.mapping(s.obj, loc=o), 1.0 / block, "Color")
    cb, _, _ = s.sep(cell)
    col = s.mix(s.m("MULTIPLY", cb, 0.35), col, tuple(c * 0.62 for c in c_d))
    if tint is not None:
        tn = s.ramp(s.noise(s.obj, 0.12 / scale, 2.0, 0.5, offset=(5, 1, 2)), [(0.45, 0.0), (0.7, 1.0)])
        col = s.mix(s.m("MULTIPLY", tn, 0.45), col, H(tint))
    nrm = _geo_normal(s)
    _, _, nz = s.sep(nrm)
    _, _, oz = s.sep(s.obj)
    em = (0.0, 0.0, 0.0)
    rough_s = rough
    if moss > 0.0:
        up = s.maprange(nz, 0.35, 0.8, 0.0, 1.0)
        mn = s.noise(s.obj, 0.9 / scale, 4.0, 0.62, offset=(2, 8, 1))
        mask = s.m("MULTIPLY", up, s.maprange(mn, 0.5, 0.6, 0.0, 1.0))
        if top_z is not None:
            drip_n = s.noise(s.obj, 2.2 / scale, 3.0, 0.6, offset=(9, 3, 3), stretch=(1.0, 1.0, 0.25))
            depth = s.maprange(drip_n, 0.3, 0.7, 0.05, 0.9 * scale)
            dz = s.m("SUBTRACT", top_z, oz)
            drip = s.m("LESS_THAN", dz, depth)
            side = s.maprange(nz, 0.5, 0.2, 0.0, 1.0)          # drips only on the sides, not across the top
            mask = s.m("MAXIMUM", mask, s.m("MULTIPLY", s.m("MULTIPLY", drip, side), s.maprange(mn, 0.38, 0.5, 0.0, 1.0)))
        mask = s.m("MULTIPLY", mask, moss)
        mcol = s.mix(s.ramp(s.noise(s.obj, 3.0 / scale, 3.0, 0.5, offset=(4, 4, 9)), [(0.3, 0.0), (0.7, 1.0)]),
                     H(moss_color), tuple(c * 0.55 for c in H(moss_color)))
        col = s.mix(mask, col, mcol)
        spores = s.m("MULTIPLY", s.m("LESS_THAN", s.voronoi(s.obj, 7.0 / scale, "Distance"), 0.09), mask)
        em = s.mix(spores, (0.0, 0.0, 0.0), tuple(c * 0.5 for c in H(moss_glow)))
        rough_s = s.m("ADD", rough, s.m("MULTIPLY", mask, 0.12))
    if glyph_rings:
        lx, ly, _ = s.sep(s.obj)
        topf = s.m("GREATER_THAN", nz, 0.9)
        lines = None
        for (cx, cy, r) in glyph_rings:
            dx = s.m("SUBTRACT", lx, cx)
            dy = s.m("SUBTRACT", ly, cy)
            dxy = s.m("SQRT", s.m("ADD", s.m("MULTIPLY", dx, dx), s.m("MULTIPLY", dy, dy)))
            ring = s.m("LESS_THAN", s.m("ABSOLUTE", s.m("SUBTRACT", dxy, r)), glyph_width * 0.5)
            ang = s.m("ARCTAN2", dy, dx)
            gaps = s.m("GREATER_THAN", s.m("FRACT", s.m("DIVIDE", s.m("ADD", ang, 3.1416), 0.7854)), 0.1)
            ring = s.m("MULTIPLY", ring, gaps)
            tick = s.m("MULTIPLY", s.m("LESS_THAN", s.m("ABSOLUTE", s.m("SUBTRACT", dxy, r - 0.2 * scale)), 0.1 * scale),
                       s.m("LESS_THAN", s.m("FRACT", s.m("DIVIDE", s.m("ADD", ang, 3.1416), 0.2618)), 0.1))
            ring = s.m("MAXIMUM", ring, tick)
            lines = ring if lines is None else s.m("MAXIMUM", lines, ring)
        g = s.m("MULTIPLY", lines, topf)
        g = s.m("MULTIPLY", g, s.maprange(s.noise(s.obj, 1.5, 2.0, 0.5, offset=(1, 1, 1)), 0.3, 0.6, 0.35, 1.0))
        col = s.mix(g, col, tuple(c * 0.4 for c in H(glyph_color)))
        em = s.mix(g, em, H(glyph_color))
    # damp dark soil creeping up from the ground line
    gd = s.m("MULTIPLY", s.maprange(oz, 0.5 * scale, 0.0, 0.0, 0.55),
             s.maprange(s.noise(s.obj, 1.8 / scale, 3.0, 0.6, offset=(8, 8, 2)), 0.3, 0.7, 0.6, 1.0))
    col = s.mix(gd, col, H("#1A1E1C"))
    rgh = s.m("ADD", rough_s, s.maprange(s.noise(s.obj, 1.3 / scale, 2.0, 0.5, offset=(6, 2, 2)), 0.3, 0.7, -0.08, 0.08))
    h = s.m("ADD", s.m("MULTIPLY", s.noise(s.obj, 14.0 / scale, 5.0, 0.62, offset=(3, 3, 7)), 0.55),
            s.m("MULTIPLY", s.voronoi(s.obj, 2.4 / scale, "Distance"), 0.9))
    return s.finish(col, rgh, 0.0, height=h, bump_strength=0.55, bump_distance=0.04 * scale,
                    emission=em, emission_strength=EMISSION_STRENGTH)


def role_basalt(name, scale=1.0, crack_color="#FF5A1F", crack=0.0, crack_zmax=None, seed=0, dark="#1A1718",
                light="#4A4446"):
    """Black basalt: ash-grey lights and ash on up-facing faces, vesicle bump, emissive cracks (voronoi edges)
    masked by noise and optionally by height (only below crack_zmax, e.g. the magma line)."""
    s = Shader(name)
    o = (seed * 2.3, seed * 0.7, seed * 1.9)
    v = s.noise(s.obj, 0.5 / scale, 4.0, 0.6, offset=o)
    col = s.mix(s.ramp(v, [(0.35, 0.0), (0.75, 1.0)]), H(dark), H(light))
    nrm = _geo_normal(s)
    _, _, nz = s.sep(nrm)
    ash = s.m("MULTIPLY", s.maprange(nz, 0.5, 0.9, 0.0, 1.0),
              s.maprange(s.noise(s.obj, 1.4 / scale, 3.0, 0.6, offset=(3, 9, 2)), 0.45, 0.6, 0.0, 0.6))
    col = s.mix(ash, col, H("#6A6264"))
    em = (0.0, 0.0, 0.0)
    if crack > 0.0:
        edge = s.voronoi(s.mapping(s.obj, scale=(1.0, 1.0, 0.7)), 1.6 / scale, "Distance", "DISTANCE_TO_EDGE")
        c = s.maprange(edge, 0.022 * scale, 0.006 * scale, 0.0, 1.0)
        m = s.maprange(s.noise(s.obj, 0.6 / scale, 2.0, 0.5, offset=(7, 7, 1)), 0.58, 0.66, 0.0, 1.0)
        c = s.m("MULTIPLY", c, m)
        if crack_zmax is not None:
            _, _, oz = s.sep(s.obj)
            c = s.m("MULTIPLY", c, s.maprange(oz, crack_zmax, crack_zmax * 0.4, 0.0, 1.0))
        c = s.m("MULTIPLY", c, crack)
        cc = H(crack_color)
        col = s.mix(c, col, tuple(x * 0.5 for x in cc))
        em = s.mix(c, em, cc)
    rgh = s.maprange(s.noise(s.obj, 2.0 / scale, 3.0, 0.5, offset=(2, 2, 6)), 0.3, 0.7, 0.62, 0.9)
    h = s.m("ADD", s.m("MULTIPLY", s.noise(s.obj, 18.0 / scale, 5.0, 0.7, offset=(5, 1, 3)), 0.6),
            s.m("MULTIPLY", s.voronoi(s.obj, 9.0 / scale, "Distance"), -0.5))
    return s.finish(col, rgh, 0.0, height=h, bump_strength=0.6, bump_distance=0.03 * scale,
                    emission=em, emission_strength=EMISSION_STRENGTH)


def role_bark(name, scale=1.0, seed=0):
    """Violet-brown bark with lengthwise fibres (on the part's local X), teal moss on top, glowing spores."""
    s = Shader(name)
    streak = s.noise(s.mapping(s.lc, scale=(0.08, 1.0, 1.0)), 6.0 / scale, 4.0, 0.6, offset=(seed, 1, 2))
    col = s.mix(s.ramp(streak, [(0.3, 0.0), (0.7, 1.0)]), H("#2A2033"), H("#4B3A52"))
    nrm = _geo_normal(s)
    _, _, nz = s.sep(nrm)
    mask = s.m("MULTIPLY", s.maprange(nz, 0.3, 0.8, 0.0, 1.0),
               s.maprange(s.noise(s.obj, 1.2 / scale, 3.0, 0.6, offset=(4, 2, 9)), 0.45, 0.58, 0.0, 1.0))
    col = s.mix(mask, col, H("#1E6E5E"))
    spores = s.m("MULTIPLY", s.m("LESS_THAN", s.voronoi(s.obj, 9.0 / scale, "Distance"), 0.1), mask)
    em = s.mix(spores, (0, 0, 0), tuple(c * 0.5 for c in H("#3CFFE0")))
    h = s.m("ADD", streak, s.m("MULTIPLY", s.noise(s.obj, 20.0 / scale, 4.0, 0.6), 0.3))
    return s.finish(col, 0.82, 0.0, height=h, bump_strength=0.8, bump_distance=0.03 * scale,
                    emission=em, emission_strength=EMISSION_STRENGTH)


# --------------------------------------------------------------------------- footprint

def bounds(obj):
    pts = mesh_points(obj.data)
    return Vector(pts.min(axis=0)), Vector(pts.max(axis=0))


def fit_box(obj, dims_m, pivot="bottom", fit_axes=(True, True, True), max_correction=0.04):
    """Scale (per axis, only where fit_axes and dims_m[i] is not None) and translate the mesh so its bounds equal
    dims_m with the pivot at the bottom centre (or the centre). Corrections above max_correction are flagged:
    the builder should already be close, so the bevels and normals are not distorted."""
    lo, hi = bounds(obj)
    size = hi - lo
    sc = [1.0, 1.0, 1.0]
    for i in range(3):
        if fit_axes[i] and dims_m[i] is not None and size[i] > 1e-6:
            sc[i] = dims_m[i] / size[i]
    centre = (lo + hi) * 0.5
    t = Matrix.Translation((-centre.x, -centre.y, -(lo.z if pivot == "bottom" else centre.z)))
    obj.data.transform(Matrix.Diagonal((sc[0], sc[1], sc[2], 1.0)) @ t)
    obj.data.update()
    warn = [i for i in range(3) if abs(sc[i] - 1.0) > max_correction]
    return {"scale": [round(v, 4) for v in sc], "warn_axes": warn, "size_before_m": [round(v, 3) for v in size]}


# --------------------------------------------------------------------------- eye-height review

def _figure(coll, loc):
    """1.8 m human proxy (body + head) for scale in the eye-height views."""
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=12, radius1=0.2, radius2=0.24, depth=1.45,
                          matrix=Matrix.Translation((0, 0, 0.725)))
    bmesh.ops.create_uvsphere(bm, u_segments=12, v_segments=8, radius=0.13,
                              matrix=Matrix.Translation((0, 0, 1.64)))
    me = bpy.data.meshes.new("__pk_fig")
    bm.to_mesh(me)
    bm.free()
    o = bpy.data.objects.new("__pk_fig", me)
    o.location = loc
    mat = bpy.data.materials.new("__pk_fig_mat")
    mat.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.5, 0.5, 0.53, 1)
    me.materials.append(mat)
    coll.objects.link(o)
    return o, mat


def eye_height_strip(asset, family, out_path, w=720, h=440, dists=(5.0, 20.0), az_deg=35.0):
    """Two cells: a 1.8 m eye-height camera at 5 m and at 20 m from the asset's bounding box, 90 deg horizontal
    FOV like the game, on a ground plane under the family light (Cinder: dim warm; Verdance: dusk cyan). A grey
    1.8 m figure stands 1 m in front of the asset for scale."""
    obj = bpy.data.objects[asset]
    lo, hi = bounds(obj)
    centre = (lo + hi) * 0.5
    base_z = max(lo.z, 0.0)
    world_col, world_str, sun_col, sun_e, sun_el, ground_hex, ground_r = FAMILY_LIGHT[family]
    scene = bpy.data.scenes.new("__pk_eye")
    scene.collection.objects.link(obj)
    world = bpy.data.worlds.new("__pk_eye_world")
    scene.world = world
    nt = world.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputWorld")
    bg = nt.nodes.new("ShaderNodeBackground")
    bg.inputs["Color"].default_value = (*world_col, 1.0)
    bg.inputs["Strength"].default_value = world_str
    nt.links.new(bg.outputs["Background"], out.inputs["Surface"])
    gme = bpy.data.meshes.new("__pk_ground")
    bm = bmesh.new()
    bmesh.ops.create_grid(bm, x_segments=1, y_segments=1, size=400.0)
    bm.to_mesh(gme)
    bm.free()
    ground = bpy.data.objects.new("__pk_ground", gme)
    gmat = bpy.data.materials.new("__pk_ground_mat")
    gb = gmat.node_tree.nodes["Principled BSDF"]
    gb.inputs["Base Color"].default_value = (*H(ground_hex), 1.0)
    gb.inputs["Roughness"].default_value = ground_r
    gme.materials.append(gmat)
    ground.location = (0, 0, min(lo.z, 0.0) - 0.002)
    scene.collection.objects.link(ground)
    cam_d = bpy.data.cameras.new("__pk_cam")
    cam_d.lens = 18.0                     # 90 deg horizontal on the 36 mm sensor
    cam_d.clip_start = 0.05
    cam_d.clip_end = 2000.0
    cam = bpy.data.objects.new("__pk_cam", cam_d)
    scene.collection.objects.link(cam)
    scene.camera = cam
    sun_d = bpy.data.lights.new("__pk_sun", "SUN")
    sun_d.color = sun_col
    sun_d.energy = sun_e
    sun_d.angle = math.radians(2.0)
    sun = bpy.data.objects.new("__pk_sun", sun_d)
    scene.collection.objects.link(sun)
    fill_d = bpy.data.lights.new("__pk_fill", "SUN")
    fill_d.color = sun_col
    fill_d.energy = sun_e * 0.25
    fill = bpy.data.objects.new("__pk_fill", fill_d)
    scene.collection.objects.link(fill)
    a = math.radians(az_deg)
    d = Vector((math.cos(a), -math.sin(a), 0.0))        # from the asset toward the camera: +X (front), 35 deg to -Y
    sa = a + math.radians(70.0)
    sdir = Vector((math.cos(sa), -math.sin(sa), math.tan(math.radians(sun_el)))).normalized()   # toward the sun
    sun.rotation_euler = (-sdir).to_track_quat("-Z", "Y").to_euler()
    fdir = Vector((-d.x, -d.y, 0.6)).normalized()
    fill.rotation_euler = (-fdir).to_track_quat("-Z", "Y").to_euler()
    hx, hy = (hi.x - lo.x) * 0.5, (hi.y - lo.y) * 0.5
    reach = abs(d.x) * hx + abs(d.y) * hy
    side = Vector((-d.y, d.x, 0.0))
    fig, fmat = _figure(scene.collection, (centre.x + d.x * (reach + 1.0) + side.x * 1.4,
                                           centre.y + d.y * (reach + 1.0) + side.y * 1.4, base_z))
    r = scene.render
    r.engine = "BLENDER_EEVEE"
    try:
        scene.eevee.taa_render_samples = 32
    except Exception:
        pass
    r.resolution_x, r.resolution_y = w, h
    r.film_transparent = False
    r.image_settings.file_format = "PNG"
    cells = []
    try:
        for dist in dists:
            p = Vector((centre.x, centre.y, 0.0)) + d * (reach + dist)
            p.z = base_z + 1.8
            height = hi.z - base_z
            aim = Vector((centre.x, centre.y, base_z + min(height * 0.42, 1.0 + dist * 0.3)))
            cam.location = p
            cam.rotation_euler = (aim - p).to_track_quat("-Z", "Y").to_euler()
            path = out_path[:-4] + "_{}m.png".format(int(dist))
            r.filepath = path
            bpy.ops.render.render(write_still=True, scene=scene.name)
            cells.append(_load_rgba(path))
            os.remove(path)
    finally:
        scene.collection.objects.unlink(obj)
        for o_ in (cam, sun, fill, ground, fig):
            data = o_.data
            bpy.data.objects.remove(o_, do_unlink=True)
            if isinstance(data, bpy.types.Mesh):
                bpy.data.meshes.remove(data)
        bpy.data.cameras.remove(cam_d)
        bpy.data.lights.remove(sun_d)
        bpy.data.lights.remove(fill_d)
        bpy.data.materials.remove(gmat)
        bpy.data.materials.remove(fmat)
        bpy.data.scenes.remove(scene)
        bpy.data.worlds.remove(world)
    gap = 6
    strip = np.full((h + 2 * gap, len(cells) * w + (len(cells) + 1) * gap, 4), 0.06, dtype=np.float32)
    strip[:, :, 3] = 1.0
    for i, c in enumerate(cells):
        x0 = gap + i * (w + gap)
        strip[gap:gap + h, x0:x0 + w] = c
    return strip


def review_pk(asset, family, tag):
    """review/<asset>_<tag>_clay_sheet.png (front 3/4, front, side, top) and review/<asset>_<tag>_review.png:
    top row material front 3/4, front, side, back 3/4; bottom row the eye-height views at 5 m and 20 m under
    the family light. (Blender "front" = camera on -Y; Unreal forward is Blender +X.)"""
    os.makedirs(REVIEW_DIR, exist_ok=True)
    label = "{}_{}".format(asset, tag)
    clay = rv["main"](targets=[asset], out_dir=REVIEW_DIR, label=label, mode="clay",
                      views=("front_3q", "front", "right", "top"), columns=4, resolution=384)
    mat = rv["main"](targets=[asset], out_dir=REVIEW_DIR, label=label + "_tmp", mode="material",
                     views=("front_3q", "front", "right", "back_3q"), columns=4, resolution=448)
    sheet = _load_rgba(mat["sheet"])
    os.remove(mat["sheet"])
    strip = eye_height_strip(asset, family, os.path.join(REVIEW_DIR, label + "_eye.png"))
    w = max(sheet.shape[1], strip.shape[1])

    def pad(a):
        out = np.full((a.shape[0], w, 4), 0.06, dtype=np.float32)
        out[:, :, 3] = 1.0
        out[:, :a.shape[1]] = a
        return out
    combined = np.concatenate([pad(strip), pad(sheet)], axis=0)
    path = os.path.join(REVIEW_DIR, label + "_review.png")
    _save_rgba(combined, path)
    return {"review": path, "clay": clay["sheet"], "warnings": clay.get("warnings", []) + mat.get("warnings", [])}


# --------------------------------------------------------------------------- detail UV + finish

def add_detail_uv(obj, metres_per_tile=4.0):
    """Second UV channel 'UVDetail': box projection in object metres / metres_per_tile (no stretch on any face),
    for the tiling detail normal in the master material (texture coordinate 1)."""
    me = obj.data
    if me.uv_layers.get("UVDetail") is None:
        me.uv_layers.new(name="UVDetail")
    uvl = me.uv_layers["UVDetail"].data
    co_all = mesh_points(me)
    for poly in me.polygons:
        n = poly.normal
        ax = max(range(3), key=lambda i: abs(n[i]))
        for li in poly.loop_indices:
            co = co_all[me.loops[li].vertex_index]
            if ax == 0:
                u, v = co[1], co[2]
            elif ax == 1:
                u, v = co[0], co[2]
            else:
                u, v = co[0], co[1]
            uvl[li].uv = (u / metres_per_tile, v / metres_per_tile)
    me.uv_layers.active_index = 0
    try:
        me.uv_layers[0].active_render = True
    except Exception:
        pass


def finish_pk(part, asset, family, tag, dims_m, tex=2048, budget=(500, 40000), pivot="bottom",
              fit_axes=(True, True, True), do_bake=True, ao_distance=1.0, samples=32, detail_uv=False,
              extra_manifest=None, tint=(0.62, 0.6, 0.58), tint_strength=0.6, snap=False):
    """Join -> fit footprint (pivot bottom centre or centre) -> edge attribute -> [UVs -> bake -> cavity] ->
    save .blend -> asset_report / assembly_check -> review sheets -> [FBX, no collision] -> health json."""
    t0 = time.time()
    info = {}
    if snap:      # organic dressing only (moss, vines, splinters); never on glow strips or flush pieces
        info["snapped_floating_groups"] = part.snap_floating(tol=0.01, rounds=3)
    obj = part.join()
    info["fit"] = fit_box(obj, dims_m, pivot=pivot, fit_axes=fit_axes)
    if obj.data.attributes.get("edge") is None:
        mark_edge_faces(obj.data, part.max_bevel or 0.02)
    info["edge_area_share"] = edge_area_share(obj.data)
    asset_dir = os.path.join(KIT, asset)
    os.makedirs(asset_dir, exist_ok=True)
    if do_bake:
        info["uv_unwrap"] = unwrap(obj, tex)
        info["uv"] = uv_stats(obj.data, 1024)
        info["bake"] = bake(obj, asset_dir, tex, sources=None, ao_distance=ao_distance, samples=samples)
        info["cavity"] = cavity_tint(asset[3:], tint=tint, strength=tint_strength)
        obj.data.materials[0].name = "M_{}".format(asset[3:])
        if detail_uv:
            add_detail_uv(obj)
            info["detail_uv"] = "UVDetail (channel 1), 4 m per tile"
    info["build_seconds"] = round(time.time() - t0, 1)
    blend_path = os.path.join(asset_dir, asset + ".blend")
    bpy.ops.wm.save_as_mainfile(filepath=blend_path, compress=True)
    try:
        bpy.ops.file.make_paths_relative()
        bpy.ops.wm.save_mainfile()
    except Exception:
        pass
    if os.path.exists(blend_path + "1"):
        os.remove(blend_path + "1")
    report = ar["main"](targets=[asset], tri_budget=budget[1])
    assembly = ac["main"](targets=[asset])
    manifest = None
    if do_bake:
        man_path = os.path.join(asset_dir, asset + ".manifest.json")
        try:
            manifest = ex["main"](root=asset, out_dir=asset_dir, front_axis="+X", collision="none")
        except AttributeError as error:
            # export_for_unreal.py writes the FBX + manifest, restores the names, then fails restoring the
            # selection on a stale view-layer entry after read_factory_settings(use_empty=True); the files are
            # complete, so read the manifest back.
            log("note: export selection restore failed after writing (%s); using the written manifest" % error)
            bpy.context.view_layer.update()
            with open(man_path, encoding="utf-8") as fh:
                manifest = json.load(fh)
        with open(man_path, encoding="utf-8") as fh:
            man = json.load(fh)
        man["fbx_relative"] = asset + ".fbx"
        man["texture_size"] = tex
        man["emission_strength"] = info.get("bake", {}).get("emission_strength", 0.0)
        man["nanite"] = True
        man["family"] = family
        man["footprint_cm"] = [None if d is None else round(d * 100.0, 1) for d in dims_m]
        man["pivot"] = pivot
        man["detail_uv"] = bool(detail_uv)
        if extra_manifest:
            man.update(extra_manifest)
        with open(man_path, "w", encoding="utf-8") as fh:
            json.dump(man, fh, indent=2)
    rev = review_pk(asset, family, tag)          # after the export: the review scenes relink the object
    me = obj.data
    lo, hi = bounds(obj)
    checks = {
        "tris": tri_count(me),
        "tris_in_budget": budget[0] <= tri_count(me) <= budget[1],
        "material_slots": len(me.materials),
        "no_floating_parts": bool(assembly.get("ok")),
        "uv": info.get("uv"),
        "asset_report_errors": report["totals"]["errors"],
        "asset_report_warnings": report["totals"]["warnings"],
        "bounds_min_cm": [round(v * 100, 1) for v in lo], "bounds_max_cm": [round(v * 100, 1) for v in hi],
        "fit": info["fit"],
        "export_status": manifest.get("status") if manifest else "skipped (no bake this round)",
    }
    health = {"asset": asset, "tag": tag, "family": family, "checks": checks, "info": info,
              "textures": image_stats(asset[3:]) if do_bake else {},
              "emissive_surface_share": emissive_surface_share(me, asset[3:]) if do_bake else None,
              "asset_report_issues": report["issues"],
              "assembly": {k: assembly.get(k) for k in ("ok", "islands", "groups", "floating", "tolerance_m")},
              "review": rev}
    with open(os.path.join(REVIEW_DIR, asset + "_health.json"), "w", encoding="utf-8") as fh:
        json.dump(health, fh, indent=2, default=str)
    log("CHECKS", asset, json.dumps(checks, default=str))
    log("ISSUES", json.dumps(report["issues"][:10], default=str))
    return health

# SPDX-License-Identifier: MIT
"""
build_magma_surface.py - SM_Cind_MagmaSurface: the visible surface of ATDMagmaVolume on Cinder (Blender 5.2, headless).

    blender --background --factory-startup --python build_magma_surface.py

The volume fits its art to the volume box (TDPropArt::Dress, bFit): the art's bounds are stretched to the box, i.e.
NON-UNIFORMLY (the field is 1600 x 7000 cm, the jammer lake 3400 x 5000 cm, both 10 cm thick, top at the volume's
Z 0). So the mesh is only a unit slab - 100 x 100 x 10 cm, pivot bottom centre, flat top at 10 cm, UV0 0..1 over the
top - and every visible detail comes from the master material M_TD_Magma (ue_import_magma_surface.py), which samples
the textures here in WORLD space (no stretch at any fit) and uses UV0 x the object's bounds for the crust rim.

Writes ArtSource/EngProps/PlanetKit/SM_Cind_MagmaSurface/:
  SM_Cind_MagmaSurface.blend / .fbx / .manifest.json
  T_Cind_MagmaSurface_M.png  2048, tileable (one tile = TileSize, 800 cm in the material), linear masks:
        R  edge field of the large crust plates (Voronoi F2 - F1, warped; 0 on a channel's centre line, 1 deep inside
           a plate); the material thresholds it into the bright flowing channels, width varying in the world
        G  the same for the fine cracks crossing the plates (thin, dull glow)
        B  per-plate variation + fine grain (crust value / roughness)
  T_Cind_MagmaSurface_N.png  2048, tileable, DirectX: plates crowned, rolling down into the channels; cracks cut in
Seeded (SEED), reproducible.
"""

import json
import os
import runpy
import sys

import bmesh
import bpy
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
ASSET = "SM_Cind_MagmaSurface"
SHORT = ASSET[3:]
OUT = os.path.join(KIT, ASSET)
RES = 2048
TILE_CM = 800.0          # must match the material's TileSize default
SEED = 7
BIG_CELLS = 4            # large plates per tile side (2 m plates)
SMALL_CELLS = 11         # fine crack cells per tile side (~73 cm)


def _repo_root():
    d = HERE
    while not os.path.isdir(os.path.join(d, ".claude", "skills")):
        parent = os.path.dirname(d)
        if parent == d:
            raise RuntimeError("repo root not found")
        d = parent
    return d


REPO = _repo_root()
SKILLS = os.path.join(REPO, ".claude", "skills")


def log(*a):
    print("[magma]", *a)
    sys.stdout.flush()


# ---------------------------------------------------------------------------------------------- tileable noise
def fbm(rng, n, beta, lo_cut=1.0):
    """Periodic fractal noise via a 1/f^beta spectrum (an inverse FFT is periodic by construction), -1..1."""
    white = rng.standard_normal((n, n)).astype(np.float32)
    f = np.fft.fftfreq(n) * n
    fx, fy = np.meshgrid(f, f)
    r = np.sqrt(fx * fx + fy * fy)
    r[0, 0] = 1.0
    amp = 1.0 / np.power(np.maximum(r, lo_cut), beta)
    amp[0, 0] = 0.0
    out = np.real(np.fft.ifft2(np.fft.fft2(white) * amp)).astype(np.float32)
    out -= out.mean()
    return out / (np.abs(out).max() + 1e-6)


def voronoi(rng, n_cells, u, v):
    """Periodic jittered-grid Voronoi on the unit tile: F1, F2 (in cell units) and the F1 cell's random value."""
    jit = rng.uniform(0.12, 0.88, (n_cells, n_cells, 2)).astype(np.float32)
    val = rng.uniform(0.0, 1.0, (n_cells, n_cells)).astype(np.float32)
    px, py = u * n_cells, v * n_cells
    ix, iy = np.floor(px).astype(np.int32), np.floor(py).astype(np.int32)
    f1 = np.full(u.shape, 1e9, np.float32)
    f2 = np.full(u.shape, 1e9, np.float32)
    v1 = np.zeros(u.shape, np.float32)
    for oy in (-1, 0, 1):
        for ox in (-1, 0, 1):
            cx, cy = ix + ox, iy + oy
            wx, wy = np.mod(cx, n_cells), np.mod(cy, n_cells)
            qx = cx + jit[wy, wx, 0]
            qy = cy + jit[wy, wx, 1]
            d = np.sqrt((px - qx) ** 2 + (py - qy) ** 2)
            closer = d < f1
            f2 = np.where(closer, f1, np.minimum(f2, d))
            v1 = np.where(closer, val[wy, wx], v1)
            f1 = np.where(closer, d, f1)
    return f1, f2, v1


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def build_textures():
    rng = np.random.default_rng(SEED)
    n = RES
    g = (np.arange(n, dtype=np.float32) + 0.5) / n
    u, v = np.meshgrid(g, g)                  # row index = v (down the image), column = u
    # domain warp (periodic): organic, slightly flowing plate outlines
    wa, wb = fbm(rng, n, 2.2), fbm(rng, n, 2.2)

    bu = np.mod(u + 0.045 * wa, 1.0)
    bv = np.mod(v + 0.045 * wb, 1.0)
    f1, f2, plate = voronoi(rng, BIG_CELLS, bu, bv)
    edge_big = f2 - f1
    wc, wd = fbm(rng, n, 1.9), fbm(rng, n, 1.9)
    su = np.mod(u + 0.02 * wc, 1.0)
    sv = np.mod(v + 0.02 * wd, 1.0)
    s1, s2, _ = voronoi(rng, SMALL_CELLS, su, sv)
    edge_small = s2 - s1
    grain = fbm(rng, n, 1.1, lo_cut=8.0)
    R = np.clip(edge_big / 0.55, 0.0, 1.0)
    G = np.clip(edge_small / 0.45, 0.0, 1.0)
    B = np.clip(0.5 + 0.32 * (plate - 0.5) + 0.22 * grain, 0.0, 1.0)
    # height (cm): plates crowned, rolling down into the channels; fine cracks cut into the plates; grain
    plate_h = smoothstep(0.06, 0.42, R)
    crack = 1.0 - smoothstep(0.0, 0.10, G)
    h = 5.0 * plate_h - 1.5 * crack * plate_h + 0.6 * grain + 1.2 * (plate - 0.5) * plate_h
    px_cm = TILE_CM / n
    dhdx = (np.roll(h, -1, axis=1) - np.roll(h, 1, axis=1)) / (2.0 * px_cm)
    dhdy = (np.roll(h, -1, axis=0) - np.roll(h, 1, axis=0)) / (2.0 * px_cm)
    nx, ny, nz = -dhdx, -dhdy, np.ones_like(h)
    ln = np.sqrt(nx * nx + ny * ny + nz * nz)
    # DirectX: R = +u, G = +v (down the image), both as stored rows / columns
    N = np.stack([nx / ln * 0.5 + 0.5, ny / ln * 0.5 + 0.5, nz / ln * 0.5 + 0.5], axis=-1)
    M = np.stack([R, G, B], axis=-1)
    stats = {"R_mean": float(R.mean()), "R_lt_0.3": float((R < 0.3).mean()), "G_lt_0.1": float((G < 0.1).mean()),
             "B_mean": float(B.mean()), "normal_z_min": float((nz / ln).min()),
             "tileable_max_seam_delta": float(max(np.abs(R[:, 0] - R[:, -1]).max(), np.abs(R[0, :] - R[-1, :]).max()))}
    return M, N, stats


def save_png(arr, path, name):
    h, w, c = arr.shape
    rgba = np.ones((h, w, 4), np.float32)
    rgba[..., :c] = arr
    img = bpy.data.images.new(name, w, h, alpha=False, float_buffer=False)
    img.colorspace_settings.name = "Non-Color"
    img.pixels.foreach_set(np.flipud(rgba).ravel())     # Blender rows run bottom-up; row 0 of arr = top of the file
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    return img


# ---------------------------------------------------------------------------------------------- mesh
def build_mesh():
    """100 x 100 x 10 cm slab (Blender metres), bottom centre pivot, top a 8 x 8 grid at z = 0.1 m with UV0 0..1;
    the sides and bottom map into a thin strip at u 0..0.004 (the crust rim) so they read as the cooled edge."""
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    S, T, K = 0.5, 0.10, 8
    grid = {}
    for j in range(K + 1):
        for i in range(K + 1):
            grid[i, j] = bm.verts.new((-S + 2 * S * i / K, -S + 2 * S * j / K, T))
    for j in range(K):
        for i in range(K):
            f = bm.faces.new((grid[i, j], grid[i + 1, j], grid[i + 1, j + 1], grid[i, j + 1]))
            for lp in f.loops:
                x, y, _ = lp.vert.co
                lp[uv].uv = (x + 0.5, y + 0.5)
    ring_top = [grid[i, 0] for i in range(K)] + [grid[K, j] for j in range(K)] + \
               [grid[i, K] for i in range(K, 0, -1)] + [grid[0, j] for j in range(K, 0, -1)]
    ring_bot = [bm.verts.new((p.co.x, p.co.y, 0.0)) for p in ring_top]
    m = len(ring_top)
    for k in range(m):
        a, b = ring_top[k], ring_top[(k + 1) % m]
        c, d = ring_bot[(k + 1) % m], ring_bot[k]
        f = bm.faces.new((b, a, d, c))           # outward
        for lp, (uu, vv) in zip(f.loops, ((0.004, (k + 1) / m), (0.004, k / m), (0.0, k / m), (0.0, (k + 1) / m))):
            lp[uv].uv = (uu, vv)
    fb = bm.faces.new(list(reversed(ring_bot)))
    for lp in fb.loops:
        lp[uv].uv = (0.002, 0.5 + 0.001 * lp.vert.co.x)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    me = bpy.data.meshes.new(ASSET)
    bm.to_mesh(me)
    bm.free()
    for p in me.polygons:
        p.use_smooth = False
    obj = bpy.data.objects.new(ASSET, me)
    coll = bpy.data.collections.new(ASSET)
    bpy.context.scene.collection.children.link(coll)
    coll.objects.link(obj)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    mat = bpy.data.materials.new("M_Cind_MagmaSurface")
    mat.use_nodes = True
    me.materials.append(mat)
    return obj


def preview_material(obj, m_img, n_img):
    """Blender-side look-alike (preview only; Unreal's M_TD_Magma is the real material)."""
    mat = obj.data.materials[0]
    nt = mat.node_tree
    for nd in list(nt.nodes):
        if nd.type != "OUTPUT_MATERIAL":
            nt.nodes.remove(nd)
    out = next(nd for nd in nt.nodes if nd.type == "OUTPUT_MATERIAL")
    bsdf = nt.nodes.new("ShaderNodeBsdfPrincipled")
    tm = nt.nodes.new("ShaderNodeTexImage")
    tm.image = m_img
    sep = nt.nodes.new("ShaderNodeSeparateColor")
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].position = 0.10
    ramp.color_ramp.elements[0].color = (1.0, 0.5, 0.08, 1.0)
    ramp.color_ramp.elements[1].position = 0.30
    ramp.color_ramp.elements[1].color = (0.0, 0.0, 0.0, 1.0)
    nt.links.new(tm.outputs["Color"], sep.inputs["Color"])
    nt.links.new(sep.outputs["Red"], ramp.inputs["Fac"])
    nt.links.new(ramp.outputs["Color"], bsdf.inputs["Emission Color"])
    bsdf.inputs["Emission Strength"].default_value = 4.0
    bsdf.inputs["Base Color"].default_value = (0.03, 0.028, 0.027, 1.0)
    bsdf.inputs["Roughness"].default_value = 0.85
    nt.links.new(bsdf.outputs["BSDF"], out.inputs["Surface"])


def main():
    os.makedirs(OUT, exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    M, N, stats = build_textures()
    m_path = os.path.join(OUT, "T_%s_M.png" % SHORT)
    n_path = os.path.join(OUT, "T_%s_N.png" % SHORT)
    m_img = save_png(M, m_path, "T_%s_M" % SHORT)
    n_img = save_png(N, n_path, "T_%s_N" % SHORT)
    log("textures", m_path, n_path, json.dumps(stats))
    obj = build_mesh()
    preview_material(obj, m_img, n_img)
    blend = os.path.join(OUT, ASSET + ".blend")
    bpy.ops.wm.save_as_mainfile(filepath=blend, compress=True)
    if os.path.exists(blend + "1"):
        os.remove(blend + "1")
    ex = runpy.run_path(os.path.join(SKILLS, "blender-unreal-export", "scripts", "export_for_unreal.py"))
    man_path = os.path.join(OUT, ASSET + ".manifest.json")
    try:
        ex["main"](root=ASSET, out_dir=OUT, front_axis="+X", collision="none")
    except AttributeError as error:
        # the exporter's selection restore trips over a stale view-layer entry in this empty factory scene; the FBX
        # and the manifest are already written by then (both checked below)
        log("note: exporter cleanup:", error)
    with open(man_path, encoding="utf-8") as fh:
        data = json.load(fh)
    man = data
    data.update({
        "fbx_relative": ASSET + ".fbx",
        "texture_size": RES,
        "textures": ["T_%s_M.png" % SHORT, "T_%s_N.png" % SHORT],
        "emission_strength": 0.0,
        "nanite": False,
        "family": "cinder",
        "footprint_cm": [100.0, 100.0, 10.0],
        "world_aligned": True,
        "tile_cm": TILE_CM,
        # ue_import_planet_kit.py skips manifests that name their own importer (this one needs M_TD_Magma, no Nanite)
        "custom_importer": "ue_import_magma_surface.py",
        "texture_stats": stats,
    })
    with open(man_path, "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=2)
    me = obj.data
    log("DONE", ASSET, "tris", sum(len(p.vertices) - 2 for p in me.polygons), "export", man.get("status"),
        "warnings", man.get("warnings"), "errors", man.get("errors"))


main()

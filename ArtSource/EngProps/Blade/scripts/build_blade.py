# SPDX-License-Identifier: MIT
"""
build_blade.py - the on-foot energy blade: SM_TD_BladeHilt (baked 1K set) + SM_TD_BladeBeam (emissive shell).

    blender --background --factory-startup --python build_blade.py -- --tag r1

Authored along Blender -Y (exported with front_axis "-Y", so -Y becomes Unreal +X):
  hilt: grip centre at the origin, pommel at Unreal X = -8.5 cm, emitter face at X = +19.3 cm
        (socket SOCKET_BladeEmitter there), total 27.8 cm, held in the right hand, button on top (+Z)
  beam: origin at the emitter, 100 cm along +X, 4.0 cm wide (Unreal Y) x 2.0 cm thick (Z), lens-shaped tapered
        cross-section; UV0 U = 0 at the emitter -> 1 at the tip, V = around the blade
Writes ArtSource/EngProps/Blade/<asset>/..., review sheets in ArtSource/EngProps/Blade/review/.
"""

import json
import math
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import eng_finish as F  # noqa: E402

L = F.L
FOLDER = os.path.dirname(HERE)
F.use_folder(FOLDER)

GRIP = 0.085  # s (distance from the pommel end) of the grip centre = the pivot


def y_of(s):
    return GRIP - s


def lathe_s(part, profile, mat, segments=32, smooth=35.0):
    """profile in (s, radius) from the pommel end; revolved around the hilt axis (Blender Y)."""
    prof = [(y_of(s), r) for s, r in profile]
    return part.add(L.lathe(part.coll, prof, segments=segments, axis="Y"), mat, smooth=smooth)


def build_hilt(tag):
    part = L.Part("SM_TD_BladeHilt", "Blade")
    sc = 0.03
    gun = L.role_paint("M_hilt_gun", "#3A414C", color2="#323944", metallic=0.6, rough=0.36, wear=0.55,
                       wear_color="#A9B2BC", grime=0.12, scale=sc, bump=0.12, seed=3)
    grip = L.role_paint("M_hilt_grip", "#17191D", color2="#1E2126", metallic=0.0, rough=0.74, wear=0.12,
                        grime=0.08, scale=sc, bump=0.25, seed=5)
    blue = L.role_paint("M_hilt_blue", "#2E4A5E", color2="#273F52", metallic=0.55, rough=0.34, wear=0.6,
                        wear_color="#A9B2BC", grime=0.1, scale=sc, bump=0.12, seed=7)
    steel = L.role_metal("M_hilt_steel", "#8D96A1", rough=0.28, metallic=1.0, scale=sc, grime=0.08)
    glow = L.role_glow("M_hilt_glow", "#3FE9FF")

    # pommel: domed cap with a steel ring and a cyan accent band
    lathe_s(part, [(0.0, 0.0), (0.0, 0.008), (0.002, 0.013), (0.006, 0.0168), (0.012, 0.0178), (0.020, 0.0178),
                   (0.023, 0.0160), (0.023, 0.0)], gun)
    lathe_s(part, [(0.0105, 0.0175), (0.0110, 0.0184), (0.0135, 0.0184), (0.0140, 0.0175)], glow, smooth=10)
    # grip core + 8 rounded ribs
    lathe_s(part, [(0.021, 0.0), (0.021, 0.0142), (0.150, 0.0142), (0.150, 0.0)], grip)
    for k in range(8):
        c = 0.034 + k * 0.0145
        lathe_s(part, [(c - 0.0034, 0.0136), (c - 0.0030, 0.0152), (c - 0.0016, 0.0163), (c, 0.0166),
                       (c + 0.0016, 0.0163), (c + 0.0030, 0.0152), (c + 0.0034, 0.0136)], grip, segments=28)
    # grip ferrules (steel) at both ends of the grip
    lathe_s(part, [(0.020, 0.0150), (0.021, 0.0165), (0.027, 0.0165), (0.028, 0.0150)], steel, smooth=20)
    lathe_s(part, [(0.142, 0.0150), (0.143, 0.0168), (0.150, 0.0168), (0.151, 0.0150)], steel, smooth=20)
    # guard collar: flared, with a cyan band at its widest point
    lathe_s(part, [(0.149, 0.0), (0.149, 0.0160), (0.156, 0.0215), (0.166, 0.0240), (0.176, 0.0240),
                   (0.183, 0.0200), (0.187, 0.0165), (0.187, 0.0)], blue)
    lathe_s(part, [(0.1685, 0.0238), (0.1690, 0.0247), (0.1735, 0.0247), (0.1740, 0.0238)], glow, smooth=10)
    # swept collar fins (sideways, raked toward the emitter): the silhouette signature
    for side in (1.0, -1.0):
        prof = [(0.016, 0.0), (0.016, -0.030), (0.029, -0.041), (0.036, -0.040), (0.031, -0.026), (0.022, -0.004)]
        prof = [(x * side, y) for x, y in prof]
        if side < 0:
            prof = list(reversed(prof))
        fin = L.extrude_profile(part.coll, prof, 0.0042, axis="Z")
        part.add(fin, gun, bevel=0.0007, segments=2, matrix=Matrix.Translation((0.0, y_of(0.150), 0.0)))
    # neck with three vent rings, then the emitter head
    lathe_s(part, [(0.186, 0.0), (0.186, 0.0150), (0.232, 0.0158), (0.232, 0.0)], steel)
    for k in range(3):
        c = 0.195 + k * 0.0115
        lathe_s(part, [(c - 0.0028, 0.0150), (c - 0.0022, 0.0172), (c + 0.0022, 0.0172), (c + 0.0028, 0.0150)], gun,
                segments=28, smooth=20)
    lathe_s(part, [(0.230, 0.0), (0.230, 0.0158), (0.236, 0.0172), (0.250, 0.0176), (0.252, 0.0168),
                   (0.256, 0.0168), (0.258, 0.0180), (0.268, 0.0180), (0.272, 0.0170), (0.272, 0.0)], blue)
    # emitter: recessed dark lens housing, glowing lens, and the cyan emitter ring
    lathe_s(part, [(0.270, 0.0), (0.270, 0.0128), (0.2765, 0.0128), (0.2765, 0.0072), (0.275, 0.0)], steel, smooth=20)
    lathe_s(part, [(0.2745, 0.0), (0.2745, 0.0074), (0.2772, 0.0060), (0.2778, 0.0)], glow)
    lathe_s(part, [(0.2715, 0.0142), (0.2715, 0.0166), (0.2780, 0.0160), (0.2780, 0.0146), (0.2715, 0.0142)], glow,
            smooth=20)
    # activation slide on top (+Z), at the thumb near the collar, with a cyan pip
    slide = L.box(part.coll, (0.0085, 0.020, 0.0045), center=(0.0, y_of(0.128), 0.0158))
    part.add(slide, gun, bevel=0.0009, segments=2)
    pip = L.box(part.coll, (0.0034, 0.0075, 0.0016), center=(0.0, y_of(0.125), 0.0185))
    part.add(pip, glow, bevel=0.0004, segments=1)

    # one convex hull of everything (a hand-held prop: cheap collision)
    hull_pts = []
    for o in part.parts:
        hull_pts.extend(o.matrix_world @ v.co for v in o.data.vertices)
    obj, health = F.finish(part, "SM_TD_BladeHilt", 1024, tag, (1500, 9000), "-Y", ao_distance=0.03,
                           sockets=[("SOCKET_BladeEmitter", (0.0, y_of(0.278), 0.0))], hulls=[hull_pts],
                           extra_manifest={"nanite": False, "pivot_note": "grip centre; emitter socket at +19.3 cm X"})
    return obj, health


def build_beam():
    """Lens cross-section shell, tapered to a point; UVs: U along the blade, V around it."""
    coll = L.ensure_collection("BladeBeam")
    seg = 32
    prof = [(0.0, 0.0105), (0.012, 0.0175), (0.05, 0.0198), (0.30, 0.0200), (0.60, 0.0192), (0.80, 0.0170),
            (0.90, 0.0130), (0.955, 0.0080), (0.985, 0.0035), (1.0, 0.0)]
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    rings = []
    for s, r in prof[:-1]:
        ring = []
        for i in range(seg + 1):
            a = 2.0 * math.pi * i / seg
            # lens section: full width along X, half thickness along Z
            ring.append((bm.verts.new((r * math.cos(a), -s, 0.5 * r * math.sin(a))), s, i / seg))
        rings.append(ring)
    tip = [bm.verts.new((0.0, -1.0, 0.0)) for _ in range(seg)]
    base = [bm.verts.new((0.0, 0.0, 0.0)) for _ in range(seg)]

    def face(vs):
        f = bm.faces.new([v[0] for v in vs])
        for loop, (_, u, v) in zip(f.loops, vs):
            loop[uvl].uv = (u, v)

    for a, b in zip(rings, rings[1:]):
        for i in range(seg):
            face([a[i], a[i + 1], b[i + 1], b[i]])
    for i in range(seg):
        face([rings[-1][i], rings[-1][i + 1], (tip[i], 1.0, (i + 0.5) / seg)])
        face([(base[i], 0.0, (i + 0.5) / seg), rings[0][i + 1], rings[0][i]])
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    me = bpy.data.meshes.new("SM_TD_BladeBeam")
    bm.to_mesh(me)
    bm.free()
    for p in me.polygons:
        p.use_smooth = True
    obj = bpy.data.objects.new("SM_TD_BladeBeam", me)
    coll.objects.link(obj)
    outward = sum(1 for p in me.polygons if p.normal.dot(p.center - Vector((0, p.center.y, 0))) > 0)

    # Blender preview of M_TD_BladeBeam (the Unreal graph is the same idea): white-hot facing core, cyan edges
    m = bpy.data.materials.new("M_BladeBeam_BlenderPreview")
    nt = m.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    lw = nt.nodes.new("ShaderNodeLayerWeight")
    lw.inputs["Blend"].default_value = 0.5
    inv = nt.nodes.new("ShaderNodeMath")
    inv.operation = "SUBTRACT"
    inv.inputs[0].default_value = 1.0
    nt.links.new(lw.outputs["Facing"], inv.inputs[1])
    core = nt.nodes.new("ShaderNodeMath")
    core.operation = "POWER"
    nt.links.new(inv.outputs[0], core.inputs[0])
    core.inputs[1].default_value = 2.5
    mix = nt.nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    nt.links.new(core.outputs[0], mix.inputs[0])
    mix.inputs[6].default_value = (0.1, 0.65, 1.0, 1.0)
    mix.inputs[7].default_value = (1.0, 1.0, 1.0, 1.0)
    em = nt.nodes.new("ShaderNodeEmission")
    nt.links.new(mix.outputs[2], em.inputs[0])
    bright = nt.nodes.new("ShaderNodeMath")
    bright.operation = "MULTIPLY_ADD"
    nt.links.new(inv.outputs[0], bright.inputs[0])
    bright.inputs[1].default_value = 5.0
    bright.inputs[2].default_value = 1.2
    nt.links.new(bright.outputs[0], em.inputs[1])
    tr = nt.nodes.new("ShaderNodeBsdfTransparent")
    add = nt.nodes.new("ShaderNodeAddShader")
    nt.links.new(tr.outputs[0], add.inputs[0])
    nt.links.new(em.outputs[0], add.inputs[1])
    nt.links.new(add.outputs[0], out.inputs[0])
    m.diffuse_color = (0.1, 0.65, 1.0, 1.0)
    me.materials.append(m)
    return obj, outward


def blade_review(hilt, beam, tag):
    """Hilt + beam assembled, against black and against mid grey, two angles each (Cycles)."""
    sc = bpy.data.scenes.new("__blade_review")
    for o in (hilt, beam):
        sc.collection.objects.link(o)
    beam.location = (0.0, y_of(0.278), 0.0)  # mounted on SOCKET_BladeEmitter for the review
    w = bpy.data.worlds.new("__bw")
    sc.world = w
    sc.render.engine = "CYCLES"
    sc.cycles.samples = 64
    sc.cycles.use_denoising = False
    sc.view_settings.view_transform = "AgX"
    sc.render.resolution_x, sc.render.resolution_y = 960, 540
    cd = bpy.data.cameras.new("__bc")
    cam = bpy.data.objects.new("__bc", cd)
    sc.collection.objects.link(cam)
    sc.camera = cam
    cd.lens = 50
    kd = bpy.data.lights.new("__key", "AREA")
    kd.energy = 30.0
    kd.size = 0.6
    key = bpy.data.objects.new("__key", kd)
    sc.collection.objects.link(key)
    key.location = (0.6, 0.3, 0.8)
    key.rotation_euler = (Vector((0.0, -0.4, 0.0)) - key.location).to_track_quat("-Z", "Y").to_euler()
    tiles = []
    centre = Vector((0.0, -0.40, 0.0))
    shots = [((0.55, 0.55, 0.30), centre, 1.0), ((-0.25, 0.12, 0.10), Vector((0.0, -0.08, 0.0)), 0.35)]
    for bgcol in ((0.0, 0.0, 0.0), (0.18, 0.18, 0.18)):
        bgn = w.node_tree.nodes["Background"]
        bgn.inputs[0].default_value = (*bgcol, 1.0)
        bgn.inputs[1].default_value = 1.0
        for loc, target, zoom in shots:
            cam.location = target + Vector(loc) * (1.6 if zoom == 1.0 else 1.0)
            cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()
            p = os.path.join(L.REVIEW_DIR, "__tile.png")
            sc.render.filepath = p
            bpy.ops.render.render(write_still=True, scene=sc.name)
            img = bpy.data.images.load(p)
            a = np.empty(960 * 540 * 4, dtype=np.float32)
            img.pixels.foreach_get(a)
            bpy.data.images.remove(img)
            os.remove(p)
            tiles.append(a.reshape(540, 960, 4))
    g = 6
    sheet = np.full((2 * 540 + 3 * g, 2 * 960 + 3 * g, 4), 0.3, dtype=np.float32)
    sheet[:, :, 3] = 1
    H = sheet.shape[0]
    for k, t in enumerate(tiles):
        r, c = divmod(k, 2)
        y0 = H - g - (r + 1) * 540 - r * g
        x0 = g + c * (960 + g)
        sheet[y0:y0 + 540, x0:x0 + 960] = t
    path = os.path.join(L.REVIEW_DIR, "Blade_{}_black_grey.png".format(tag))
    L._save_rgba(sheet, path)
    beam.location = (0.0, 0.0, 0.0)
    for o in (hilt, beam):
        sc.collection.objects.unlink(o)
    bpy.data.scenes.remove(sc)
    return path


def main():
    import argparse
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    ap = argparse.ArgumentParser()
    ap.add_argument("--tag", default="r1")
    a = ap.parse_args(argv)
    bpy.ops.wm.read_homefile(use_empty=True)
    hilt, health = build_hilt(a.tag)
    beam, outward = build_beam()
    bdir = os.path.join(FOLDER, "SM_TD_BladeBeam")
    os.makedirs(bdir, exist_ok=True)
    rep = L.ar["main"](targets=[beam.name], tri_budget=1500)
    bpy.context.view_layer.update()
    # the beam has no collision: export it alone
    try:
        man = L.ex["main"](root=beam.name, out_dir=bdir, collision="none", front_axis="-Y")
    except AttributeError:
        with open(os.path.join(bdir, "SM_TD_BladeBeam.manifest.json"), encoding="utf-8") as fh:
            man = json.load(fh)
        man["status"] = "ok" if not man.get("errors") else "error"
    sheet = blade_review(hilt, beam, a.tag)
    pts = L.mesh_points(beam.data)
    beam_health = {"tris": L.tri_count(beam.data), "outward_faces": outward, "faces": len(beam.data.polygons),
                   "bbox_min_m": [round(float(v), 4) for v in pts.min(axis=0)],
                   "bbox_max_m": [round(float(v), 4) for v in pts.max(axis=0)],
                   "asset_report": rep["totals"], "issues": rep["issues"],
                   "export": {k: man.get(k) for k in ("status", "tris", "size_cm", "warnings", "errors")}}
    with open(os.path.join(L.REVIEW_DIR, "SM_TD_BladeBeam_health.json"), "w") as fh:
        json.dump(beam_health, fh, indent=2, default=str)
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(bdir, "SM_TD_BladeBeam.blend"), compress=True)
    print("__RESULT__" + json.dumps({"hilt": health["checks"], "hilt_emissive_share": health["emissive_surface_share"],
                                     "hilt_review": health["review"].get("review"), "beam": beam_health,
                                     "sheet": sheet}, default=str))


main()

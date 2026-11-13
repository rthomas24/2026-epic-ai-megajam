# SPDX-License-Identifier: MIT
"""
build_pad.py - SM_LandingPad: a 10 m sci-fi landing pad for the planets (one material, baked 2048 set).

    blender --background --factory-startup --python build_pad.py -- --tag r1

Diameter 1000 cm, height 40 cm, flat walkable top at Z = 40 cm, origin at the centre of the base (Z = 0).
Top: radial panel lines, concentric seams, a hazard-striped rim band, an original target-style landing mark
(two rings + four bars) in warm paint; dark gunmetal centre field and a warm-grey outer deck.
Rim: 24 cyan guide lights set into the top chamfer; 8 buttress feet; two step blocks (+-Unreal Y).
Writes ArtSource/EngProps/Pad/SM_LandingPad/..., review sheets in ArtSource/EngProps/Pad/review/.
"""

import json
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
FOLDER = os.path.dirname(HERE)
BLADE_SCRIPTS = os.path.join(os.path.dirname(FOLDER), "Blade", "scripts")
sys.path.insert(0, BLADE_SCRIPTS)
import eng_finish as F  # noqa: E402

L = F.L
F.use_folder(FOLDER)

R = 5.0
TOP = 0.40


def deck_material():
    """Procedural top-deck paint driven by object-space polar coordinates."""
    s = L.Shader("M_pad_deck")
    H = L.H
    x, y, z = s.sep(s.obj)
    r = s.m("SQRT", s.m("ADD", s.m("MULTIPLY", x, x), s.m("MULTIPLY", y, y)))
    th = s.m("ARCTAN2", y, x)  # -pi..pi

    def ring(r0, r1, soft=0.006):
        return s.m("MULTIPLY", s.maprange(r, r0 - soft, r0 + soft, 0.0, 1.0), s.maprange(r, r1 + soft, r1 - soft, 0.0, 1.0))

    # zones
    outer = s.maprange(r, 2.98, 3.02, 0.0, 1.0)
    var = s.noise(s.obj, 0.35, 3.0, 0.55)
    dark = s.mix(s.ramp(var, [(0.35, 0.0), (0.7, 1.0)]), H("#2B2E34"), H("#24272C"))
    warm = s.mix(s.ramp(var, [(0.35, 0.0), (0.7, 1.0)]), H("#8E877D"), H("#7D776E"))
    col = s.mix(outer, dark, warm)
    metal = s.mix(outer, (0.55,) * 3, (0.25,) * 3)
    rough = s.mix(outer, (0.40,) * 3, (0.52,) * 3)
    # hazard band (diagonal stripes) 4.42 .. 4.74
    band = ring(4.42, 4.74)
    stripe = s.m("GREATER_THAN", s.m("FRACT", s.m("DIVIDE", s.m("ADD", s.m("MULTIPLY", th, 4.58), r), 0.42)), 0.5)
    hz = s.mix(stripe, H("#18181A"), H("#F2A01E"))
    col = s.mix(band, col, hz)
    rough = s.mix(band, rough, (0.5,) * 3)
    metal = s.mix(band, metal, (0.2,) * 3)
    # landing mark: rings 1.05-1.20 and 0.28-0.36, four bars between them, a small centre dot
    tau = 2.0 * math.pi
    ang4 = s.m("MULTIPLY", s.m("ABSOLUTE", s.m("SUBTRACT", s.m("FRACT", s.m("ADD", s.m("DIVIDE", th, tau / 4.0), 0.5)), 0.5)),
               s.m("MULTIPLY", r, tau / 4.0))  # metres from the nearest of 4 radial axes
    bars = s.m("MULTIPLY", s.maprange(ang4, 0.075, 0.065, 0.0, 1.0), ring(0.46, 0.92))
    mark = s.m("MAXIMUM", s.m("MAXIMUM", ring(1.05, 1.20), ring(0.28, 0.36)), s.m("MAXIMUM", bars, s.maprange(r, 0.11, 0.10, 0.0, 1.0)))
    # an outer dashed ring of warm paint at 2.55
    dash = s.m("GREATER_THAN", s.m("FRACT", s.m("DIVIDE", th, tau / 36.0)), 0.45)
    mark = s.m("MAXIMUM", mark, s.m("MULTIPLY", ring(2.52, 2.60), dash))
    wearn = s.ramp(s.noise(s.obj, 3.0, 4.0, 0.6, offset=(3, 1, 7)), [(0.30, 0.0), (0.42, 1.0)])
    mark = s.m("MULTIPLY", mark, s.m("ADD", s.m("MULTIPLY", wearn, 0.85), 0.15))
    col = s.mix(mark, col, H("#F3E6C8"))
    rough = s.mix(mark, rough, (0.55,) * 3)
    metal = s.mix(mark, metal, (0.1,) * 3)
    # seams: 24 radial (outside r 1.4), concentric at 1.4, 3.0, 4.40, 4.76
    rad = s.m("MULTIPLY", s.m("ABSOLUTE", s.m("SUBTRACT", s.m("FRACT", s.m("ADD", s.m("DIVIDE", th, tau / 24.0), 0.5)), 0.5)),
              s.m("MULTIPLY", r, tau / 24.0))
    seam_r = s.m("MULTIPLY", s.maprange(rad, 0.014, 0.006, 0.0, 1.0), s.maprange(r, 1.38, 1.42, 0.0, 1.0))
    # extra radial seams in the outer deck (48 sectors)
    rad48 = s.m("MULTIPLY", s.m("ABSOLUTE", s.m("SUBTRACT", s.m("FRACT", s.m("ADD", s.m("DIVIDE", th, tau / 48.0), 0.5)), 0.5)),
                s.m("MULTIPLY", r, tau / 48.0))
    seam_r = s.m("MAXIMUM", seam_r, s.m("MULTIPLY", s.maprange(rad48, 0.012, 0.005, 0.0, 1.0), s.maprange(r, 2.98, 3.02, 0.0, 1.0)))
    seam_c = 0.0
    for rc in (1.4, 3.0, 4.40, 4.76):
        seam_c = s.m("MAXIMUM", seam_c, s.maprange(s.m("ABSOLUTE", s.m("SUBTRACT", r, rc)), 0.016, 0.007, 0.0, 1.0))
    seam = s.m("MULTIPLY", s.m("MAXIMUM", seam_r, seam_c), s.maprange(z, TOP - 0.02, TOP - 0.005, 0.0, 1.0))
    col = s.mix(s.m("MULTIPLY", seam, 0.85), col, H("#0E0F11"))
    rough = s.m("ADD", rough, s.m("MULTIPLY", seam, 0.2))
    # sides (below the top): plain dark gunmetal
    side = s.maprange(z, TOP - 0.01, TOP - 0.03, 0.0, 1.0)
    col = s.mix(side, col, H("#2E3238"))
    # grime streaks + scuffs on the deck, edge wear on chamfers
    g = s.ramp(s.noise(s.obj, 0.9, 4.0, 0.6, offset=(2, 5, 8)), [(0.5, 0.0), (0.8, 1.0)])
    col = s.mix(s.m("MULTIPLY", g, 0.35), col, H("#3B362F"))
    scuff = s.ramp(s.noise(s.obj, 7.0, 3.0, 0.7, offset=(9, 9, 1), stretch=(1.0, 4.0, 1.0)), [(0.62, 0.0), (0.75, 1.0)])
    col = s.mix(s.m("MULTIPLY", scuff, 0.25), col, H("#A6A39C"))
    chips = s.ramp(s.noise(s.obj, 18.0, 3.0, 0.6, offset=(1, 9, 3)), [(0.42, 0.0), (0.55, 1.0)])
    e = s.m("MINIMUM", s.m("MULTIPLY", s.m("MULTIPLY", s.attr("edge"), chips), 1.4), 1.0)
    col = s.mix(e, col, H("#A9B0B8"))
    metal = s.mix(e, metal, (0.95,) * 3)
    rough = s.mix(e, rough, (0.28,) * 3)
    rnd = s.maprange(s.noise(s.obj, 1.6, 2.0, 0.5, offset=(6, 2, 2)), 0.3, 0.7, -0.06, 0.06)
    rough = s.m("ADD", rough, rnd)
    h = s.m("SUBTRACT", s.m("MULTIPLY", s.noise(s.obj, 60.0, 4.0, 0.6, offset=(3, 3, 7)), 0.25), s.m("MULTIPLY", seam, 1.0))
    return s.finish(col, rough, metal, height=h, bump_strength=0.35, bump_distance=0.006)


def build(tag):
    part = L.Part("SM_LandingPad", "Pad")
    deck = deck_material()
    foot_m = L.role_paint("M_pad_foot", "#2E3238", color2="#262A30", metallic=0.5, rough=0.42, wear=0.5,
                          grime=0.3, panel=(0.5, 0.012), scale=0.5, seed=2)
    step_m = L.role_paint("M_pad_step", "#2E3238", metallic=0.4, rough=0.5, wear=0.5, grime=0.3,
                          stripes=("#18181A", "#F2A01E", 0.16, (1.0, 0.0, 1.0)), scale=0.5, seed=4)
    lens = L.role_glow("M_pad_light", "#3FE9FF")
    housing = L.role_metal("M_pad_housing", "#5E6670", rough=0.32, scale=0.2)

    # main body: lathe about Z with a chamfered top edge and a slight skirt
    prof = [(0.0, 0.0), (0.0, 4.985), (0.03, 5.0), (0.30, 4.93), (0.34, 4.90), (0.375, 4.86), (0.40, 4.82),
            (0.40, 3.0), (0.40, 1.4), (0.40, 0.0)]
    body = part.add(L.lathe(part.coll, prof, segments=96, axis="Z"), deck, smooth=20.0)
    part.hulls.append([v.co.copy() for v in body.data.vertices])

    # 24 rim lights on the chamfer: steel housing + cyan lens, half sunk into the chamfer
    for k in range(24):
        a = 2.0 * math.pi * (k + 0.5) / 24.0
        rr, zz = 4.875, 0.368
        tilt = math.atan2(0.04, 0.035)  # chamfer slope
        m = Matrix.Translation((rr * math.cos(a), rr * math.sin(a), zz)) @ Matrix.Rotation(a, 4, "Z") @ \
            Matrix.Rotation(-tilt, 4, "Y")
        hb = L.box(part.coll, (0.07, 0.30, 0.05))
        part.add(hb, housing, bevel=0.008, segments=2, matrix=m)
        lb = L.box(part.coll, (0.035, 0.24, 0.03), center=(0.0, 0.0, 0.018))
        part.add(lb, lens, bevel=0.006, segments=2, matrix=m)

    # 8 buttress feet around the skirt
    for k in range(8):
        a = 2.0 * math.pi * k / 8.0 + math.radians(22.5)
        prof2 = [(-0.0, 0.0), (0.42, 0.0), (0.42, 0.06), (0.12, 0.30), (0.0, 0.30)]
        ft = L.extrude_profile(part.coll, prof2, 0.34, axis="Y")  # profile (x, z) extruded along Y
        m = Matrix.Translation((4.86 * math.cos(a), 4.86 * math.sin(a), 0.0)) @ Matrix.Rotation(a, 4, "Z")
        part.add(ft, foot_m, bevel=0.012, segments=2, matrix=m)

    # two step blocks (Blender +-X, which become Unreal +-Y after the -Y front rotation)
    for side in (1.0, -1.0):
        st = L.box(part.coll, (0.50, 1.40, 0.20), center=(0.0, 0.0, 0.10))
        m = Matrix.Translation((side * 5.12, 0.0, 0.0))
        o = part.add(st, step_m, bevel=0.015, segments=2, matrix=m)
        part.hulls.append([o.matrix_world @ v.co for v in o.data.vertices])

    obj, health = F.finish(part, "SM_LandingPad", 2048, tag, (2000, 16000), "-Y", ao_distance=0.6,
                           tint=(0.6, 0.58, 0.56), tint_strength=0.55,
                           extra_manifest={"nanite": False, "pivot_note": "centre of the base; walkable top at Z = 40 cm"})
    return obj, health


def pad_review(obj, tag):
    """Standing eye height (1.7 m, 3 m outside the rim) and straight down from above; sun + soft sky."""
    sc = bpy.data.scenes.new("__pad_review")
    sc.collection.objects.link(obj)
    w = bpy.data.worlds.new("__pw")
    sc.world = w
    bgn = w.node_tree.nodes["Background"]
    bgn.inputs[0].default_value = (0.35, 0.42, 0.55, 1.0)
    bgn.inputs[1].default_value = 0.6
    sc.render.engine = "CYCLES"
    sc.cycles.samples = 48
    sc.cycles.use_denoising = False
    sc.view_settings.view_transform = "AgX"
    sc.render.resolution_x, sc.render.resolution_y = 960, 640
    # ground
    import bmesh
    bm = bmesh.new()
    bmesh.ops.create_grid(bm, x_segments=1, y_segments=1, size=40.0)
    gme = bpy.data.meshes.new("__ground")
    bm.to_mesh(gme)
    bm.free()
    ground = bpy.data.objects.new("__ground", gme)
    gm = bpy.data.materials.new("__gm")
    gm.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.09, 0.075, 0.07, 1.0)
    gm.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value = 0.9
    gme.materials.append(gm)
    sc.collection.objects.link(ground)
    sd = bpy.data.lights.new("__sun", "SUN")
    sd.energy = 4.0
    sd.angle = math.radians(2.0)
    sun = bpy.data.objects.new("__sun", sd)
    sc.collection.objects.link(sun)
    sun.rotation_euler = Vector((-0.5, 0.7, -0.6)).to_track_quat("-Z", "Y").to_euler()
    cd = bpy.data.cameras.new("__pc")
    cam = bpy.data.objects.new("__pc", cd)
    sc.collection.objects.link(cam)
    sc.camera = cam
    shots = [("eye", (0.0, -8.0, 1.7), (0.0, 0.0, 0.3), 24.0, "PERSP"),
             ("eye_close", (3.2, -5.6, 1.7), (0.6, -2.0, 0.3), 24.0, "PERSP"),
             ("above", (0.0, 0.0, 30.0), (0.0, 0.0, 0.0), 0.0, "ORTHO"),
             ("approach", (-14.0, -14.0, 9.0), (0.0, 0.0, 0.0), 40.0, "PERSP")]
    tiles = []
    for name, loc, tgt, lens, kind in shots:
        cd.type = kind
        if kind == "ORTHO":
            cd.ortho_scale = 11.5
        else:
            cd.lens = lens
        cam.location = loc
        d = Vector(tgt) - Vector(loc)
        cam.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
        p = os.path.join(L.REVIEW_DIR, "__tile.png")
        sc.render.filepath = p
        bpy.ops.render.render(write_still=True, scene=sc.name)
        img = bpy.data.images.load(p)
        a = np.empty(960 * 640 * 4, dtype=np.float32)
        img.pixels.foreach_get(a)
        bpy.data.images.remove(img)
        os.remove(p)
        tiles.append(a.reshape(640, 960, 4))
    g = 6
    sheet = np.full((2 * 640 + 3 * g, 2 * 960 + 3 * g, 4), 0.1, dtype=np.float32)
    sheet[:, :, 3] = 1
    Hh = sheet.shape[0]
    for k, t in enumerate(tiles):
        r_, c = divmod(k, 2)
        y0 = Hh - g - (r_ + 1) * 640 - r_ * g
        x0 = g + c * (960 + g)
        sheet[y0:y0 + 640, x0:x0 + 960] = t
    path = os.path.join(L.REVIEW_DIR, "SM_LandingPad_{}_eye_above.png".format(tag))
    L._save_rgba(sheet, path)
    sc.collection.objects.unlink(obj)
    bpy.data.scenes.remove(sc)
    return path


def main():
    import argparse
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    ap = argparse.ArgumentParser()
    ap.add_argument("--tag", default="r1")
    a = ap.parse_args(argv)
    bpy.ops.wm.read_homefile(use_empty=True)
    obj, health = build(a.tag)
    sheet = pad_review(obj, a.tag)
    print("__RESULT__" + json.dumps({"checks": health["checks"], "emissive_share": health["emissive_surface_share"],
                                     "review": health["review"].get("review"), "sheet": sheet}, default=str))


main()

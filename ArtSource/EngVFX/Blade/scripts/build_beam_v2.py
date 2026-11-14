# SPDX-License-Identifier: MIT
"""
build_beam_v2.py - the energy blade's new beam (issue #163): SM_TD_BladeBeam_V2. Original design.

    blender --background --factory-startup --python ArtSource/EngVFX/Blade/scripts/build_beam_v2.py

Same conventions as SM_TD_BladeBeam (so the code's length scaling keeps working): origin at the emitter, 100 cm along
Unreal +X (authored along Blender -Y, exported with front_axis "-Y"), UV0 U = 0 at the emitter -> 1 at the tip,
V = around the blade. Not Nanite (translucent), no collision.

Two material slots:
  0 Core  - a slightly flattened elliptical blade 2.6 cm wide (Y) x 1.7 cm thick (Z) that swells just above the
            emitter, runs even, then closes in a rounded tip at 96.5 cm.  -> M_TD_BladeBeamCore_V2
  1 Glow  - a soft shell round it, about 3x wider, rounded past the core tip to 100 cm. Additive with a
            facing-ratio falloff so it has no hard silhouette: it is the "bloom you can see" that keeps the
            colour saturated when the white core clips.                   -> M_TD_BladeBeamGlow_V2
Writes ArtSource/EngVFX/Blade/meshes/SM_TD_BladeBeam_V2/{.fbx,.manifest.json,.blend}.
"""
import json
import math
import os
import sys

import bmesh
import bpy

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
sys.path.insert(0, os.path.join(REPO, "ArtSource", "EngProps", "Blade", "scripts"))
import eng_finish as F  # noqa: E402

L = F.L
OUT = os.path.join(os.path.dirname(HERE), "meshes", "SM_TD_BladeBeam_V2")
F.use_folder(os.path.dirname(HERE))

SEG = 24


def core_radius(s):
    """Half-width (m) of the core at s (0 emitter .. 1)."""
    tip0, tip1 = 0.84, 0.965
    if s <= 0.0:
        return 0.0095
    if s < 0.03:                      # a short swell out of the emitter
        return 0.0095 + (0.0140 - 0.0095) * math.sin(0.5 * math.pi * s / 0.03)
    if s < 0.10:
        return 0.0140 - 0.0010 * (s - 0.03) / 0.07
    if s < tip0:                      # almost even, a very slight taper
        return 0.0130 - 0.0012 * (s - 0.10) / (tip0 - 0.10)
    t = min(1.0, (s - tip0) / (tip1 - tip0))
    return 0.0118 * math.sqrt(max(0.0, 1.0 - t * t))   # rounded (elliptic) tip


def glow_radius(s):
    tip0, tip1 = 0.80, 1.0
    if s < 0.02:
        return 0.022 + (0.040 - 0.022) * s / 0.02
    if s < tip0:
        return 0.040 - 0.004 * (s - 0.02) / (tip0 - 0.02)
    t = min(1.0, (s - tip0) / (tip1 - tip0))
    return 0.036 * math.sqrt(max(0.0, 1.0 - t * t))


def stations(tip1, n_body=26, n_tip=12, tip0=0.84):
    out = [0.0, 0.006, 0.015, 0.03, 0.05, 0.08]
    for i in range(1, n_body + 1):
        out.append(0.08 + (tip0 - 0.08) * i / n_body)
    for i in range(1, n_tip):
        # cosine spacing: dense near the tip
        out.append(tip0 + (tip1 - tip0) * math.sin(0.5 * math.pi * i / n_tip))
    return sorted(set(round(s, 5) for s in out if s < tip1))


def tube(bm, uvl, radius, tip1, flat, mat_index, tip0):
    """A closed tube along Blender -Y. flat = thickness / width ratio of the ellipse. Returns faces."""
    ss = stations(tip1, tip0=tip0)
    rings = []
    for s in ss:
        r = radius(s)
        ring = []
        for i in range(SEG + 1):
            a = 2.0 * math.pi * i / SEG
            # width along Blender X (-> Unreal Y), thickness along Z
            ring.append((bm.verts.new((r * math.cos(a), -s, flat * r * math.sin(a))), s, i / SEG))
        rings.append(ring)
    tip = bm.verts.new((0.0, -tip1, 0.0))
    base = bm.verts.new((0.0, 0.0, 0.0)) if radius(0.0) > 0 else None
    faces = []

    def face(vs):
        f = bm.faces.new([v[0] for v in vs])
        f.material_index = mat_index
        for loop, (_, u, v) in zip(f.loops, vs):
            loop[uvl].uv = (u, v)
        faces.append(f)

    for a, b in zip(rings, rings[1:]):
        for i in range(SEG):
            face([a[i], a[i + 1], b[i + 1], b[i]])
    for i in range(SEG):
        face([rings[-1][i], rings[-1][i + 1], (tip, 1.0, (i + 0.5) / SEG)])
        if base is not None:
            face([(base, 0.0, (i + 0.5) / SEG), rings[0][i + 1], rings[0][i]])
    return faces


def build():
    coll = L.ensure_collection("BladeBeamV2")
    bm = bmesh.new()
    uvl = bm.loops.layers.uv.new("UVMap")
    tube(bm, uvl, core_radius, 0.965, 0.65, 0, 0.84)
    tube(bm, uvl, glow_radius, 1.0, 0.80, 1, 0.80)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    me = bpy.data.meshes.new("SM_TD_BladeBeam_V2")
    bm.to_mesh(me)
    bm.free()
    for p in me.polygons:
        p.use_smooth = True
    for nm, rgb in (("BladeCore", (1.0, 1.0, 1.0, 1.0)), ("BladeGlow", (1.0, 0.2, 0.85, 1.0))):
        m = bpy.data.materials.new(nm)
        m.diffuse_color = rgb
        me.materials.append(m)
    obj = bpy.data.objects.new("SM_TD_BladeBeam_V2", me)
    coll.objects.link(obj)
    return obj


def main():
    bpy.ops.wm.read_homefile(use_empty=True)
    obj = build()
    bpy.context.view_layer.update()
    os.makedirs(OUT, exist_ok=True)
    rep = L.ar["main"](targets=[obj.name], tri_budget=6000)
    try:
        man = L.ex["main"](root=obj.name, out_dir=OUT, collision="none", front_axis="-Y")
    except AttributeError:   # the exporter's selection cleanup trips after writing (same as build_blade.py)
        with open(os.path.join(OUT, "SM_TD_BladeBeam_V2.manifest.json"), encoding="utf-8") as fh:
            man = json.load(fh)
        man["status"] = "ok" if not man.get("errors") else "error"
    obj = bpy.data.objects["SM_TD_BladeBeam_V2"]
    pts = L.mesh_points(obj.data)
    health = {"tris": L.tri_count(obj.data), "slots": [m.name for m in obj.data.materials],
              "bbox_min_m": [round(float(v), 4) for v in pts.min(axis=0)],
              "bbox_max_m": [round(float(v), 4) for v in pts.max(axis=0)],
              "asset_report": rep.get("totals"), "issues": rep.get("issues"),
              "export": {k: man.get(k) for k in ("status", "tris", "size_cm", "warnings", "errors")}}
    os.makedirs(os.path.join(os.path.dirname(HERE), "review"), exist_ok=True)
    with open(os.path.join(os.path.dirname(HERE), "review", "SM_TD_BladeBeam_V2_health.json"), "w") as fh:
        json.dump(health, fh, indent=2, default=str)
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "SM_TD_BladeBeam_V2.blend"), compress=True)
    print("__RESULT__" + json.dumps(health, default=str))


main()

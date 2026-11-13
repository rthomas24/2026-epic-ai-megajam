# SPDX-License-Identifier: MIT
# Triad Drift - prepare one of Grok Bot's Rodin ships for Unreal: import the derived FBX (read-only), light
# clean-up, SOCKET_ empties at the real muzzles and nozzles, review sheet with socket markers, .blend, FBX and
# manifest in ArtSource/EngShips/<Ship>/.
#   blender --background --factory-startup --python rs_prep.py -- <Ship> [--tag r1] [--no-export]
import json
import os
import runpy
import sys

import bmesh
import bpy

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rs_common as C

SKILLS = os.path.join(C.REPO, ".claude", "skills")

# Sockets in UNREAL cm relative to the pivot (bounding-box centre): +X forward, +Y right, +Z up.
# Gameplay reads Muzzle_0..N (contiguous) and Engine / Engine_0..3; the rest are FX and ability points.
SOCKETS = {
    "Rift": {
        "Muzzle_0": (252, -37, -60), "Muzzle_1": (252, 37, -60),
        "Engine": (-252, 0, -34), "Engine_0": (-252, -64, -34), "Engine_1": (-252, 64, -34),
        "Portal": (-170, 0, 55), "Cockpit": (60, 0, 0),
        "WingTip_L": (238, -176, -109), "WingTip_R": (238, 176, -109),
    },
    "Lancer": {
        "Muzzle_0": (152, -45, -59), "Muzzle_1": (108, 45, -55),      # the Rodin mesh has one long and one short gun
        "Engine": (-264, 0, -35), "Engine_0": (-264, 0, -35),
        "Engine_1": (-209, -55, -38), "Engine_2": (-209, 55, -38),
        "Ram": (262, 0, -58), "Cockpit": (25, 0, 0),
        "WingTip_L": (-188, -132, -50), "WingTip_R": (-188, 132, -50),
    },
    "Bastion": {
        "Muzzle_0": (264, -44, -37), "Muzzle_1": (264, 44, -37),
        "Engine": (-264, 0, 0), "Engine_0": (-264, -77, 0), "Engine_1": (-264, 77, 0),
        "Dome": (-26, 0, 68), "Cockpit": (120, 0, 20),
        "MissilePod_L": (124, -165, -20), "MissilePod_R": (124, 165, -20),
    },
    "NullFighter": {
        "Muzzle_0": (268, -57, -38), "Muzzle_1": (268, 57, -38),
        "Engine_0": (-277, -63, -14), "Engine_1": (-277, 63, -14), "Engine_2": (-275, 0, -14),
    },
    "NullInterceptor": {       # mesh_r2 (textured)
        "Muzzle_0": (320, -72, -55), "Muzzle_1": (320, 72, -55),      # chin guns; this mesh has no missile pods
        "Engine_0": (-332, -80, -13), "Engine_1": (-332, 80, -13), "Engine_2": (-332, 0, -15),
    },
    "NullBomber": {
        "Muzzle_0": (-100, 0, -105),
        "Engine_0": (-563, -165, -10), "Engine_1": (-563, 165, -10), "Engine_2": (-592, 0, 15),
    },
    "NullCarrier": {
        "Engine_0": (-1514, -805, 50), "Engine_1": (-1514, 805, 50),
        "Engine_2": (-370, -1040, -290), "Engine_3": (-370, 1040, -290),
    },
    "NullTurret": {
        "Muzzle_0": (110, -11, 9), "Muzzle_1": (108, 23, 12),
    },
    "NullGunship": {
        "Muzzle_0": (748, -280, 295), "Muzzle_1": (748, -210, 300),
        "Muzzle_2": (748, 210, 300), "Muzzle_3": (748, 280, 295),
        "Engine_0": (-740, -270, 421), "Engine_1": (-740, -90, 421),
        "Engine_2": (-740, 75, 421), "Engine_3": (-740, 255, 421),
    },
}
MARK_COLORS = {"Muzzle": (0.0, 1.0, 0.1), "Engine": (0.0, 0.4, 1.0)}


def arg(flag, default=None):
    a = sys.argv[sys.argv.index("--") + 1:]
    if flag in a:
        return a[a.index(flag) + 1]
    return default


def has(flag):
    return flag in sys.argv[sys.argv.index("--") + 1:]


def image_node(nt, path, colorspace, x, y):
    n = nt.nodes.new("ShaderNodeTexImage")
    n.image = bpy.data.images.load(path, check_existing=True)
    n.image.colorspace_settings.name = colorspace
    n.location = (x, y)
    return n


def build_material(ship, s):
    """Preview material (the review sheet) = the same maps Unreal gets."""
    mat = bpy.data.materials.new("M_%s" % ship)
    nt = mat.node_tree
    bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
    stem = os.path.join(s["src"], "T_%s_" % s["stem"])
    if os.path.isfile(stem + "BC.png"):
        nt.links.new(image_node(nt, stem + "BC.png", "sRGB", -700, 300).outputs["Color"], bsdf.inputs["Base Color"])
        e = image_node(nt, stem + "E.png", "sRGB", -700, -300)
        nt.links.new(e.outputs["Color"], bsdf.inputs["Emission Color"])
        bsdf.inputs["Emission Strength"].default_value = 2.0
        orm = image_node(nt, stem + "ORM.png", "Non-Color", -700, 0)
        sep = nt.nodes.new("ShaderNodeSeparateColor")
        sep.location = (-400, 0)
        nt.links.new(orm.outputs["Color"], sep.inputs["Color"])
        nt.links.new(sep.outputs["Green"], bsdf.inputs["Roughness"])
        nt.links.new(sep.outputs["Blue"], bsdf.inputs["Metallic"])
    else:
        bsdf.inputs["Base Color"].default_value = (0.03, 0.03, 0.035, 1.0)
        bsdf.inputs["Roughness"].default_value = 0.45
        bsdf.inputs["Metallic"].default_value = 0.8
    return mat


def main():
    ship = sys.argv[sys.argv.index("--") + 1]
    tag = arg("--tag", "r1")
    s = C.SHIPS[ship]
    asset = "SM_" + ship
    out_dir = os.path.join(C.OUT, ship)
    review = os.path.join(C.OUT, "Rodin", "review")
    os.makedirs(out_dir, exist_ok=True)
    os.makedirs(review, exist_ok=True)

    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=os.path.join(s["src"], "SM_%s.fbx" % s["stem"]))
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    assert len(meshes) == 1, "expected one mesh, got %d" % len(meshes)
    ob = meshes[0]
    for o in list(bpy.context.scene.objects):
        if o is not ob:
            bpy.data.objects.remove(o)
    ob.name = asset
    ob.data.name = asset
    # the FBX arrives nose on +X, Z up, metres (1 m = 100 uu), object scale 1; bake any import transform
    ob.data.transform(ob.matrix_world)
    ob.matrix_world.identity()

    # light clean-up: loose bits and degenerate faces only. No weld across UV seams is needed (shared verts
    # already), custom normals from Rodin are kept, UVs are untouched (the heat mask is baked in this layout).
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    before = (len(bm.verts), len(bm.faces))
    bmesh.ops.dissolve_degenerate(bm, dist=1e-6, edges=bm.edges)
    loose = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose, context="VERTS")
    after = (len(bm.verts), len(bm.faces))
    bm.to_mesh(ob.data)
    bm.free()
    ob.data.update()

    ob.data.materials.clear()
    ob.data.materials.append(build_material(ship, s))

    xs = [v.co for v in ob.data.vertices]
    mn = [min(c[i] for c in xs) for i in range(3)]
    mx = [max(c[i] for c in xs) for i in range(3)]
    size_cm = [round((mx[i] - mn[i]) * 100, 1) for i in range(3)]
    centre_cm = [round((mx[i] + mn[i]) * 50, 1) for i in range(3)]

    marks = []
    for name, (x, y, z) in SOCKETS[ship].items():
        e = bpy.data.objects.new("SOCKET_" + name, None)
        e.empty_display_size = 0.1
        e.location = (x / 100.0, -y / 100.0, z / 100.0)     # Unreal +Y right = Blender -Y with the nose on +X
        e.parent = ob
        bpy.context.scene.collection.objects.link(e)
        col = next((c for k, c in MARK_COLORS.items() if name.startswith(k)), (1.0, 0.9, 0.0))
        bpy.ops.mesh.primitive_uv_sphere_add(radius=0.045, location=e.location, segments=12, ring_count=6)
        m = bpy.context.active_object
        m.name = "MARK_" + name
        mm = bpy.data.materials.new("MK_" + name)
        b = next(n for n in mm.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
        b.inputs["Base Color"].default_value = (0, 0, 0, 1)
        b.inputs["Emission Color"].default_value = (*col, 1.0)
        b.inputs["Emission Strength"].default_value = 8.0
        m.data.materials.append(mm)
        marks.append(m)

    sheet = {}
    if not has("--no-review"):
        rr = runpy.run_path(os.path.join(SKILLS, "blender-mcp", "scripts", "review_renders.py"))
        sheet = rr["main"](targets=[asset] + [m.name for m in marks], out_dir=review, label="%s_%s_sockets" % (ship, tag),
                           mode="material", views=["left", "right", "top", "front", "back_3q", "front_3q"], resolution=900)
    for m in marks:
        bpy.data.objects.remove(m)
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    bpy.context.view_layer.objects.active = ob

    rep = runpy.run_path(os.path.join(SKILLS, "blender-mcp", "scripts", "asset_report.py"))["main"](targets=[asset])
    asm = runpy.run_path(os.path.join(SKILLS, "blender-mcp", "scripts", "assembly_check.py"))["main"](targets=[asset])
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out_dir, "%s.blend" % asset), compress=True)

    result = {"ship": ship, "verts_faces_before": before, "after": after, "size_cm": size_cm,
              "centre_cm": centre_cm, "sheet": sheet.get("sheet") or sheet.get("contact_sheet") or str(sheet)[:300],
              "report": {k: rep.get(k) for k in ("ok", "errors", "warnings", "totals") if k in rep},
              "assembly": {k: asm.get(k) for k in list(asm)[:6]}}
    if not has("--no-export"):
        ex = runpy.run_path(os.path.join(SKILLS, "blender-unreal-export", "scripts", "export_for_unreal.py"))
        man_path = os.path.join(out_dir, asset + ".manifest.json")
        for stale in (man_path, os.path.join(out_dir, asset + ".fbx")):
            if os.path.isfile(stale):
                os.remove(stale)
        try:
            ex["main"](root=asset, out_dir=out_dir, front_axis="+X", collision="auto")
        except AttributeError as error:
            # the exporter's selection restore trips over a stale view layer in a scene started with
            # use_empty=True; the FBX and manifest are already written by then (the .blend was saved above)
            result["export_note"] = "exporter cleanup: %s" % error
        if os.path.isfile(man_path):
            with open(man_path, encoding="utf-8") as fh:
                man = json.load(fh)
            result["export"] = {k: man.get(k) for k in ("tris", "size_cm", "collision", "welded_verts", "errors", "warnings")}
            man["ue_sockets"] = [{"name": n, "ue_cm": list(p)} for n, p in SOCKETS[ship].items()]
            man["source"] = os.path.relpath(s.get("rel", s["src"]), C.REPO).replace("\\", "/")
            man["texture_dir"] = man["source"]
            man["heat_mask"] = os.path.relpath(s["heat"], C.REPO).replace("\\", "/") if s["heat"] else None
            man["max_texture_size"] = s["tex"]
            with open(man_path, "w", encoding="utf-8") as fh:
                json.dump(man, fh, indent=2)
    print("__RESULT__" + json.dumps(result, default=str))


main()

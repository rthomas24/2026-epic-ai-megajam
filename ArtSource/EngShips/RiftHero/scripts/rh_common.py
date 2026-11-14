# SPDX-License-Identifier: MIT
# Triad Drift - RiftHero: shared paths and helpers. Convention: metres, nose +X, +Z up, Blender Y = -Unreal Y.
import json, os, runpy, sys
import bpy

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)                                   # ArtSource/EngShips/RiftHero
REPO = os.path.abspath(os.path.join(ROOT, "..", "..", ".."))
SKILLS = os.path.join(REPO, ".claude", "skills")
RODIN = os.path.join(REPO, "ArtSource", "EngAI", "Ships", "Rift", "mesh_r1")
REVIEW = os.path.join(ROOT, "review")
EXPORT = os.path.join(ROOT, "export")
ANALYSIS = os.path.join(ROOT, "analysis")
SHIP = "Rift"


def use(cfg, out_root=None):
    """Point the shared paths at a ship config (rh_ships.py). out_root redirects everything that is written (a dry
    run that leaves the repo alone)."""
    global SHIP, ROOT, RODIN, REVIEW, EXPORT, ANALYSIS
    SHIP = cfg["ship"]
    ROOT = out_root or cfg["root"]
    RODIN = cfg["rodin"]
    REVIEW, EXPORT, ANALYSIS = (os.path.join(ROOT, d) for d in ("review", "export", "analysis"))
    for d in (REVIEW, EXPORT):
        os.makedirs(d, exist_ok=True)


VIEWS = ["front_3q", "front", "top", "back_3q", "left", "right"]   # 3/4 nose, side, top, chase 3/4, rear, nose-on


def skill(rel):
    return runpy.run_path(os.path.join(SKILLS, *rel.split("/")))


def import_rodin(name="SRC_Rift_Rodin", textured=False):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=os.path.join(RODIN, "SM_%s.fbx" % SHIP))
    new = [o for o in bpy.data.objects if o not in before]
    ob = next(o for o in new if o.type == "MESH")
    for o in new:
        if o is not ob:
            bpy.data.objects.remove(o)
    ob.data.transform(ob.matrix_world)
    ob.matrix_world.identity()
    ob.name = name
    ob.data.name = name
    if textured:
        ob.data.materials.clear()
        mat = bpy.data.materials.new("M_SRC_Rodin")
        nt = mat.node_tree
        bsdf = next(n for n in nt.nodes if n.type == "BSDF_PRINCIPLED")
        def img(file, cs):
            n = nt.nodes.new("ShaderNodeTexImage")
            n.image = bpy.data.images.load(os.path.join(RODIN, file), check_existing=True)
            n.image.colorspace_settings.name = cs
            return n
        nt.links.new(img("T_%s_BC.png" % SHIP, "sRGB").outputs["Color"], bsdf.inputs["Base Color"])
        nt.links.new(img("T_%s_E.png" % SHIP, "sRGB").outputs["Color"], bsdf.inputs["Emission Color"])
        bsdf.inputs["Emission Strength"].default_value = 2.0
        sep = nt.nodes.new("ShaderNodeSeparateColor")
        nt.links.new(img("T_%s_ORM.png" % SHIP, "Non-Color").outputs["Color"], sep.inputs["Color"])
        nt.links.new(sep.outputs["Green"], bsdf.inputs["Roughness"])
        nt.links.new(sep.outputs["Blue"], bsdf.inputs["Metallic"])
        ob.data.materials.append(mat)
    return ob


def out(result):
    print("__RESULT__" + json.dumps(result, default=str))

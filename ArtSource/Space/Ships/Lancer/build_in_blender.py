"""Lancer M01 source adapter. See review/ for the latest validation status.

Run only after human silhouette review and Lead binary-claim publication:
  blender --background --factory-startup --python build_in_blender.py -- --production-authorized

For explicitly authorized silhouette validation only, use --review-authorized.
Review authorization does not approve final production or Unreal import.

Creates an isolated collection without clearing or saving the current scene.
No FBX, image, .blend or Unreal writes. Inspect/QA before exporting with the
repository blender-unreal-export tools, one reusable module per FBX.
"""
import argparse
import json
from pathlib import Path
import runpy
import sys


def build(production_authorized=False, review_authorized=False):
    if not (production_authorized or review_authorized):
        raise RuntimeError("Obtain review authorization or production approval before running this adapter")
    import bpy
    from mathutils import Matrix, Vector

    folder = Path(__file__).resolve().parent
    repo = folder.parents[3]
    helper_path = repo / ".claude/skills/blender-game-asset/scripts/hardsurface.py"
    hs = runpy.run_path(str(helper_path))
    data = json.loads((folder / "attachments.json").read_text(encoding="utf-8"))
    if bpy.data.collections.get("SRC_Lancer_M01"):
        raise RuntimeError("SRC_Lancer_M01 already exists; refusing to replace user data")
    collection = bpy.data.collections.new("SRC_Lancer_M01")
    bpy.context.scene.collection.children.link(collection)
    # Authoring -Y nose and +X right, metres, with no object scale conversion.
    def co(value):
        x, y, z = value
        return (y / 100.0, -x / 100.0, z / 100.0)

    mats = []
    for spec in data["materials"]:
        # A unique namespace is intentional: preserve all existing user materials.
        name = "M01_" + spec["name"]
        if bpy.data.materials.get(name):
            raise RuntimeError("Material name already exists: " + name)
        mat = bpy.data.materials.new(name)
        mat.use_nodes = True
        bsdf = next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
        rgb = (*hs["hex_color"](spec["hex"]), 1.0)
        bsdf.inputs["Base Color"].default_value = rgb
        bsdf.inputs["Metallic"].default_value = spec["metallic"]
        bsdf.inputs["Roughness"].default_value = spec["roughness"]
        if spec.get("emissive_strength"):
            bsdf.inputs["Emission Color"].default_value = rgb
            bsdf.inputs["Emission Strength"].default_value = spec["emissive_strength"]
        mat.diffuse_color = rgb
        mats.append(mat)

    mesh_data = {}
    for asset in data["assets"]:
        source = json.loads((folder / (asset["name"] + ".mesh.json")).read_text(encoding="utf-8"))
        verts, faces, slots = [], [], []
        for part in source["parts"]:
            base = len(verts)
            verts.extend(co(v) for v in part["vertices"])
            faces.extend(tuple(base + i for i in t) for t in part["triangles"])
            slots.extend([part["material"]] * len(part["triangles"]))
        mesh = bpy.data.meshes.new("M01_" + asset["name"])
        mesh.from_pydata(verts, [], faces)
        mesh.update()
        for mat in mats:
            mesh.materials.append(mat)
        for polygon, slot in zip(mesh.polygons, slots):
            polygon.material_index = slot
        mesh_data[asset["name"]] = mesh

    objects = {}
    for instance in data["instances"]:
        name = "SM_Lancer_M01_" + instance["name"]
        if bpy.data.objects.get(name):
            raise RuntimeError("Object name already exists: " + name)
        obj = bpy.data.objects.new(name, mesh_data[instance["asset"]])
        collection.objects.link(obj)
        obj.location = co(instance["position_cm"])
        obj["source_asset"] = instance["asset"]
        obj["review_status"] = "source_prototype_not_final_art"
        objects[instance["name"]] = obj

    # Unwrap once per shared mesh using the repo helper. UVs are only provisional;
    # evaluated topology, material/UV QA and production bevels remain explicit gates.
    visited = set()
    for obj in objects.values():
        if obj.data.name not in visited:
            hs["smart_uv"](obj)
            visited.add(obj.data.name)
        hs["smooth_by_angle"](obj, angle_deg=35)

    for sock in data["sockets"]:
        name = "SOCKET_" + sock["parent"] + "_" + sock["name"]
        if bpy.data.objects.get(name):
            raise RuntimeError("Socket name already exists: " + name)
        obj = bpy.data.objects.new(name, None)
        collection.objects.link(obj)
        obj.parent = objects[sock["parent"]]
        obj.location = co(sock["position_cm"])
        # Exporter socket visual convention: local -Y forward, +Z up.
        forward = Vector(co([v * 100 for v in sock["forward"]])).normalized()
        up = Vector(co([v * 100 for v in sock["up"]])).normalized()
        # Local -Y is forward; +X must be up cross forward. The opposite
        # cross product creates a reflection and reverses Euler socket direction.
        right = up.cross(forward).normalized()
        obj.rotation_euler = Matrix((right, -forward, up)).transposed().to_euler()
        obj.empty_display_type = "ARROWS"
        obj.empty_display_size = 0.18
        obj["unreal_socket_name"] = sock["name"]
        obj["purpose"] = sock["purpose"]
    bpy.context.view_layer.update()
    return {"collection": collection.name, "mesh_instances": len(objects),
            "sockets": len(data["sockets"]), "saved_files": [],
            "qa_required": True, "version": bpy.app.version_string}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--production-authorized", action="store_true")
    parser.add_argument("--review-authorized", action="store_true")
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else [])
    print(json.dumps(build(args.production_authorized, args.review_authorized)))

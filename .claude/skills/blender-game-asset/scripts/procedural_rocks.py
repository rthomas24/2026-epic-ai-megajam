# SPDX-License-Identifier: MIT
"""
procedural_rocks.py - seedable generators for asteroids, rocks and crystal clusters.

Everything is built with the data API (bmesh + mathutils.noise), so it works the same in the
live Blender session (MCP `execute_blender_code`) and headless (`execute_blender_code_for_cli`
or `blender --background`), with no operator context problems. Objects are named for Unreal
(SM_ prefix), get a UV map, sit with their origin at the centre (asteroids) or the base
(crystals), and are linked into the collection you pass (created if missing).

    import runpy
    rocks = runpy.run_path(r"<repo>/.claude/skills/blender-game-asset/scripts/procedural_rocks.py")
    a = rocks["make_asteroid"]("SM_Asteroid_A", radius=6.0, seed=3, craters=9, target_tris=4000)
    c = rocks["make_crystal_cluster"]("SM_CrystalCluster_A", seed=5, count=9, height=2.5)
    result = {"asteroid": a, "crystals": c}

Each function returns a JSON-serialisable dict describing what it built.
Sizes are in metres (Blender units); 1 m = 100 Unreal units.
"""

import math
import random

import bmesh
import bpy
from mathutils import Matrix, Vector, noise


# --------------------------------------------------------------------------- helpers

def ensure_collection(name, parent=None):
    coll = bpy.data.collections.get(name)
    if coll is None:
        coll = bpy.data.collections.new(name)
        (parent or bpy.context.scene.collection).children.link(coll)
    return coll


def new_mesh_object(name, bm, collection):
    """Write a bmesh into a NEW mesh/object (never reuses an existing name silently)."""
    if bpy.data.objects.get(name) is not None:
        raise ValueError("Object {!r} already exists; pick a new name or delete it first".format(name))
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    collection.objects.link(obj)
    return obj


def apply_modifiers(obj):
    """Bake the evaluated modifier stack into the mesh without operators (context-free)."""
    depsgraph = bpy.context.evaluated_depsgraph_get()
    ev = obj.evaluated_get(depsgraph)
    new_mesh = bpy.data.meshes.new_from_object(ev, preserve_all_data_layers=True, depsgraph=depsgraph)
    old = obj.data
    obj.modifiers.clear()
    obj.data = new_mesh
    if old.users == 0:
        bpy.data.meshes.remove(old)
    new_mesh.name = obj.name
    return obj


def decimate_to(obj, target_tris):
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    if target_tris and tris > target_tris:
        mod = obj.modifiers.new("Decimate", "DECIMATE")
        mod.decimate_type = "COLLAPSE"
        mod.ratio = max(0.01, target_tris / float(tris))
        mod.use_collapse_triangulate = True
        apply_modifiers(obj)
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def box_uv(obj, scale=1.0):
    """Simple triplanar-style box projection UVs (data API, no edit mode needed)."""
    mesh = obj.data
    uv = mesh.uv_layers.get("UVMap") or mesh.uv_layers.new(name="UVMap")
    for poly in mesh.polygons:
        n = poly.normal
        axis = max(range(3), key=lambda i: abs(n[i]))
        for li in poly.loop_indices:
            co = mesh.vertices[mesh.loops[li].vertex_index].co
            if axis == 0:
                u, v = co.y, co.z
            elif axis == 1:
                u, v = co.x, co.z
            else:
                u, v = co.x, co.y
            uv.data[li].uv = (u * scale, v * scale)
    return uv.name


def smart_uv(obj, angle_deg=66.0, island_margin=0.02):
    """Smart UV Project via operator (needs object mode switch). Falls back to box_uv."""
    try:
        view_layer = bpy.context.view_layer
        prev_active = view_layer.objects.active
        for o in view_layer.objects:
            o.select_set(False)
        obj.select_set(True)
        view_layer.objects.active = obj
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.uv.smart_project(angle_limit=math.radians(angle_deg), island_margin=island_margin)
        bpy.ops.object.mode_set(mode="OBJECT")
        view_layer.objects.active = prev_active
        return "smart_project"
    except Exception as ex:  # no suitable context: keep going with a projection that always works
        try:
            bpy.ops.object.mode_set(mode="OBJECT")
        except Exception:
            pass
        box_uv(obj, scale=0.25)
        return "box_uv ({})".format(ex.__class__.__name__)


def make_material(name, base_color, roughness=0.6, metallic=0.0, emission=None, emission_strength=0.0):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    try:
        mat.use_nodes = True
    except Exception:
        pass
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (*base_color, 1.0)
    bsdf.inputs["Roughness"].default_value = roughness
    bsdf.inputs["Metallic"].default_value = metallic
    if emission is not None:
        bsdf.inputs["Emission Color"].default_value = (*emission, 1.0)
        bsdf.inputs["Emission Strength"].default_value = emission_strength
    mat.diffuse_color = (*base_color, 1.0)  # viewport/Workbench colour
    return mat


def make_rock_material(name="M_Rock", dark=(0.035, 0.03, 0.045), light=(0.16, 0.13, 0.19),
                       accent=None, noise_scale=5.0, bump_strength=0.6, roughness=0.82):
    """Two-tone procedural rock (bounding-box 'Generated' coords -> noise -> colour ramp, plus bump),
    so the pattern scale is the same on a 1 m pebble and a 50 m asteroid.
    Procedural nodes do NOT transfer to Unreal: bake them to textures before export."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    try:
        mat.use_nodes = True
    except Exception:
        pass
    nt = mat.node_tree
    bsdf = nt.nodes.get("Principled BSDF")
    coord = nt.nodes.new("ShaderNodeTexCoord")
    tex = nt.nodes.new("ShaderNodeTexNoise")
    tex.inputs["Scale"].default_value = noise_scale
    tex.inputs["Detail"].default_value = 8.0
    tex.inputs["Roughness"].default_value = 0.6
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].position = 0.35
    ramp.color_ramp.elements[0].color = (*dark, 1.0)
    ramp.color_ramp.elements[1].position = 0.7
    ramp.color_ramp.elements[1].color = (*light, 1.0)
    if accent is not None:
        el = ramp.color_ramp.elements.new(0.95)
        el.color = (*accent, 1.0)
    # Fine, high-frequency noise drives the bump so the baked normal map carries real surface detail
    # (broad colour noise alone produces an almost flat normal map).
    detail = nt.nodes.new("ShaderNodeTexNoise")
    detail.inputs["Scale"].default_value = noise_scale * 8.0
    detail.inputs["Detail"].default_value = 12.0
    detail.inputs["Roughness"].default_value = 0.65
    bump = nt.nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = bump_strength
    bump.inputs["Distance"].default_value = 0.25
    nt.links.new(coord.outputs["Generated"], tex.inputs["Vector"])
    nt.links.new(coord.outputs["Generated"], detail.inputs["Vector"])
    nt.links.new(tex.outputs["Fac"], ramp.inputs["Fac"])
    nt.links.new(ramp.outputs["Color"], bsdf.inputs["Base Color"])
    nt.links.new(detail.outputs["Fac"], bump.inputs["Height"])
    nt.links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    bsdf.inputs["Roughness"].default_value = roughness
    mat.diffuse_color = (*light, 1.0)
    return mat


# --------------------------------------------------------------------------- asteroid

def make_asteroid(name="SM_Asteroid", radius=5.0, seed=1, subdivisions=5, elongation=0.3,
                  roughness=0.16, ridge=0.25, craters=7, crater_size=(0.22, 0.5), crater_depth=0.16,
                  target_tris=None, collection="Asteroids", location=(0.0, 0.0, 0.0),
                  material=None, sharp_angle_deg=None):
    """Lumpy, cratered asteroid; radius in metres.
    Density comes from `subdivisions`: 4 = 1,280 tris (background/far), 5 = 5,120 (default),
    6 = 20,480, 7 = 81,920 (Nanite hero rocks).
    Prefer choosing subdivisions over decimating: collapse-decimating noisy surfaces leaves long,
    thin triangles that shade as shards. `sharp_angle_deg` adds hard edges (leave None for rocks)."""
    rng = random.Random(seed)
    bm = bmesh.new()
    bmesh.ops.create_icosphere(bm, subdivisions=subdivisions, radius=1.0)
    stretch = Vector([1.0 + rng.uniform(-elongation, elongation) for _ in range(3)])
    offset = Vector((rng.uniform(-100, 100), rng.uniform(-100, 100), rng.uniform(-100, 100)))
    crater_list = []
    for _ in range(craters):
        d = Vector((rng.gauss(0, 1), rng.gauss(0, 1), rng.gauss(0, 1))).normalized()
        crater_list.append((d, rng.uniform(*crater_size)))
    for v in bm.verts:
        n = v.co.normalized()
        p = n * 1.2 + offset
        # Large-scale lumps + a little ridged detail.
        h = noise.fractal(p, 1.0, 2.0, 3, noise_basis="PERLIN_ORIGINAL") * roughness
        h += (noise.ridged_multi_fractal(p * 1.8, 0.9, 2.0, 2, 1.0, 2.0, noise_basis="PERLIN_ORIGINAL") - 1.0) \
            * roughness * 0.3 * ridge
        # Craters: bowl with a raised rim. Continuous at the rim (x = 1) so no torn edges.
        for c_dir, c_r in crater_list:
            x = n.angle(c_dir) / c_r
            if x < 2.0:
                rim_term = 0.35 * math.exp(-((x - 1.0) / 0.3) ** 2)
                h += crater_depth * ((x * x - 1.0) + rim_term if x < 1.0 else rim_term)
        v.co = Vector((n.x * stretch.x, n.y * stretch.y, n.z * stretch.z)) * (1.0 + h) * radius
    bm.normal_update()
    coll = ensure_collection(collection)
    obj = new_mesh_object(name, bm, coll)
    obj.location = location
    tris = decimate_to(obj, target_tris)
    obj.data.shade_smooth()
    if sharp_angle_deg:
        obj.data.set_sharp_from_angle(angle=math.radians(sharp_angle_deg))
    uv_method = smart_uv(obj)
    mat = material or make_rock_material("M_Asteroid")
    obj.data.materials.append(mat)
    dims = [round(v * 100.0, 1) for v in obj.dimensions]
    return {"object": obj.name, "tris": tris, "size_cm": dims, "uv": uv_method, "seed": seed,
            "material": mat.name, "collection": coll.name}


# --------------------------------------------------------------------------- crystals

def _crystal(bm, base, direction, radius, height, sides, tip_ratio, rng):
    """Append one faceted crystal (tapered prism + pyramid tip) to bm."""
    rot = direction.to_track_quat("Z", "Y").to_matrix().to_4x4()
    twist = Matrix.Rotation(rng.uniform(0, math.tau), 4, "Z")
    xf = Matrix.Translation(base) @ rot @ twist
    ret = bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=sides,
                                radius1=radius, radius2=radius * rng.uniform(0.75, 0.95), depth=height,
                                matrix=xf @ Matrix.Translation((0, 0, height * 0.5)))
    verts = ret["verts"]
    top_z = max((xf.inverted() @ v.co).z for v in verts)
    top_faces = {f for v in verts for f in v.link_faces
                 if all(abs((xf.inverted() @ fv.co).z - top_z) < 1e-5 for fv in f.verts)}
    for f in top_faces:
        poke = bmesh.ops.poke(bm, faces=[f])
        for pv in poke["verts"]:
            pv.co = xf @ Vector((0, 0, top_z + radius * tip_ratio))


def make_crystal_cluster(name="SM_CrystalCluster", seed=1, count=9, height=2.0, radius=0.28,
                         spread=0.9, max_tilt_deg=38.0, sides=6, tip_ratio=1.6, collection="Crystals",
                         location=(0.0, 0.0, 0.0), material=None,
                         glow=(0.0, 0.75, 1.0), base_color=(0.0, 0.32, 0.55), glow_strength=0.6):
    """Faceted crystal cluster growing from a shared base. Origin at the base (place on ground).
    Keep glow_strength low (0.3-1.5): strong emission flattens the facets into a white silhouette
    in both Blender and Unreal. Let the facets' specular highlights carry the shape."""
    rng = random.Random(seed)
    bm = bmesh.new()
    for i in range(count):
        if i == 0:
            base = Vector((0, 0, -height * 0.05))
            direction = Vector((rng.uniform(-0.1, 0.1), rng.uniform(-0.1, 0.1), 1.0)).normalized()
            h, r = height, radius * 1.15
        else:
            ang = rng.uniform(0, math.tau)
            dist = rng.uniform(0.25, 1.0) * spread
            base = Vector((math.cos(ang) * dist, math.sin(ang) * dist, -height * 0.08))
            tilt = math.radians(rng.uniform(12, max_tilt_deg))
            outward = Vector((math.cos(ang), math.sin(ang), 0.0))
            direction = (Vector((0, 0, 1)) * math.cos(tilt) + outward * math.sin(tilt)).normalized()
            scale = rng.uniform(0.35, 0.8)
            h, r = height * scale, radius * rng.uniform(0.55, 1.0)
        _crystal(bm, base, direction, r, h, sides, tip_ratio * rng.uniform(0.8, 1.2), rng)
    bm.normal_update()
    coll = ensure_collection(collection)
    obj = new_mesh_object(name, bm, coll)
    obj.location = location
    obj.data.shade_flat()
    uv_method = smart_uv(obj, angle_deg=40.0)
    mat = material or make_material("M_Crystal", base_color, roughness=0.15, metallic=0.0,
                                    emission=glow, emission_strength=glow_strength)
    obj.data.materials.append(mat)
    tris = sum(len(p.vertices) - 2 for p in obj.data.polygons)
    dims = [round(v * 100.0, 1) for v in obj.dimensions]
    return {"object": obj.name, "tris": tris, "size_cm": dims, "uv": uv_method, "seed": seed,
            "material": mat.name, "collection": coll.name}


"""Rebuild the hero crystal with amber/basalt colors painted in bmesh, then re-render."""
from __future__ import annotations
import json, math, os, random, sys
from pathlib import Path
import bpy, bmesh
from mathutils import Matrix, Vector

sys.path.insert(0, str(Path(__file__).resolve().parent))
from cc_paths import *
from cc_utils import *
from cc_crystal import add_crystal, CLUSTER_HGT
from cc_bake import (ensure_bake_image, bake_self, save_image, make_bevel_normal_material,
                     make_emit_from_attribute, assign_pbr_material, load_image)
from cc_scene import (make_convex_hull, make_pilot, make_clay_material, setup_hdri,
                      add_studio_lights, render_view, export_fbx_unreal, export_fbx_direct, export_glb)

ASSEMBLY = OUT / "SM_CrystalCache_assembly.blend"
FLOOR_Z = 0.3005  # measured

def paint_face(bm, faces, col_layer, emi_layer, rm_layer, kind, meta=None):
    CYAN_DEEP, CYAN_GLOW, CYAN_BC = Vector((0.02,0.22,0.32)), Vector((0.15,0.90,1.00)), Vector((0.10,0.48,0.60))
    AMBER_DEEP, AMBER_GLOW, AMBER_BC = Vector((0.30,0.08,0.01)), Vector((1.00,0.50,0.07)), Vector((0.70,0.30,0.04))
    ROCK_BC, ROCK_ASH = Vector((0.11,0.10,0.105)), Vector((0.24,0.22,0.20))
    rng = random.Random(hash(id(faces[0])) & 0xffffffff if faces else 1)
    for f in faces:
        if not f.is_valid:
            continue
        facet = 0.7 + 0.3 * rng.random()
        tone = 0.88 + 0.2 * rng.random()
        for loop in f.loops:
            v = loop.vert
            if kind == "cyan":
                base, d, length = meta
                h = max(0.0, min(1.0, (v.co - base).dot(d) / max(length, 1e-4)))
                heat = (1.0 - max(0.0, min(1.0, (h - 0.02) / 0.70))) ** 2
                bc = CYAN_DEEP.lerp(CYAN_BC, 0.35 + 0.4*facet) * tone
                em = CYAN_GLOW * (0.55 * facet * (0.18 + 0.82*heat))
                rough, metal = 0.10, 0.02
            elif kind == "amber":
                base, d, length = meta
                h = max(0.0, min(1.0, (v.co - base).dot(d) / max(length, 1e-4)))
                heat = (1.0 - max(0.0, min(1.0, h / 0.65))) ** 1.2
                bc = AMBER_DEEP.lerp(AMBER_BC, 0.5 + 0.3*facet) * tone
                em = AMBER_GLOW * (0.95 * facet * (0.28 + 0.72*heat))
                rough, metal = 0.12, 0.02
            else:
                mix = 0.15 + 0.4 * (0.5 + 0.5*math.sin(v.co.x*14 + v.co.y*11))
                bc = ROCK_BC.lerp(ROCK_ASH, mix)
                em = Vector((0,0,0))
                # bleed
                if v.co.z < 0.28:
                    # warm/cool leak near centre
                    r = math.hypot(v.co.x, v.co.y)
                    if r < 0.22:
                        w = (1-r/0.22)**2 * 0.35
                        em = em + AMBER_GLOW * w * 0.6 + Vector((0.12,0.85,1.0))*w*0.25
                        bc = bc.lerp(AMBER_BC, w*0.25)
                rough, metal = 0.70 + 0.1*mix, 0.10
            loop[col_layer] = (bc.x, bc.y, bc.z, 1.0)
            loop[emi_layer] = (em.x, em.y, em.z, 1.0)
            loop[rm_layer] = (rough, metal, 0.0, 1.0)

def add_hex_rock(bm, center, radius, height, rot=0.0, sides=6, taper=0.85):
    d = Vector((0,0,1))
    faces, meta = add_crystal(bm, Vector(center) - Vector((0,0,0.06)), d, height, radius,
                              sides=sides, tip=0.08, taper=taper, embed=0.15,
                              rot0=rot, flatten=rng_flat(rot), step=None)
    return faces

def rng_flat(x):
    return 0.85 + 0.2 * math.sin(x*3)

def build_new_crystal():
    bm = bmesh.new()
    col_layer = bm.loops.layers.color.new("Col")
    emi_layer = bm.loops.layers.color.new("Emi")
    rm_layer = bm.loops.layers.color.new("RM")
    shards = [
        # cyan, no mid-step (clean taper, 0% wobble)
        ("cyan",  (0.00,  0.02, 0.10),  5.0,  25.0, 1.26, 0.108, 6, 8.0,  1.00, 0.10),
        ("cyan",  (0.03,  0.20, 0.08), 20.0,  95.0, 0.78, 0.088, 6, 14.0, 0.92, 0.10),
        ("cyan",  (0.04, -0.20, 0.08), 18.0, 260.0, 0.92, 0.092, 6, 4.0,  0.95, 0.10),
        ("cyan",  (0.16,  0.12, 0.06), 26.0,  40.0, 0.52, 0.068, 6, 20.0, 1.00, 0.12),
        ("cyan",  (0.14, -0.14, 0.06), 28.0, 310.0, 0.44, 0.058, 6, 10.0, 1.00, 0.12),
        ("cyan",  (-0.14, 0.08, 0.06), 16.0, 155.0, 0.46, 0.055, 6, 0.0,  1.00, 0.12),
        ("cyan",  (-0.06, 0.22, 0.05), 30.0, 110.0, 0.32, 0.042, 6, 22.0, 1.05, 0.14),
        ("cyan",  (0.02, -0.26, 0.05), 32.0, 250.0, 0.28, 0.038, 6, 6.0,  1.00, 0.14),
        ("amber", (0.07,  0.00, 0.08), 10.0,  10.0, 0.62, 0.074, 6, 16.0, 0.90, 0.12),
        ("amber", (0.05,  0.11, 0.07), 18.0,  70.0, 0.38, 0.048, 6, 0.0,  1.00, 0.14),
        ("amber", (0.10, -0.09, 0.06), 22.0, 300.0, 0.34, 0.044, 6, 12.0, 1.00, 0.14),
        ("amber", (-0.02,-0.04, 0.06),  8.0, 200.0, 0.26, 0.036, 6, 28.0, 1.00, 0.16),
    ]
    for kind, base, lean, az, length, radius, sides, rot0, flatten, embed in shards:
        la, aa = math.radians(lean), math.radians(az)
        direction = Vector((math.sin(la)*math.cos(aa), math.sin(la)*math.sin(aa), math.cos(la)))
        faces, meta = add_crystal(bm, Vector(base), direction, length, radius,
                                  sides=sides, tip=0.20, taper=0.76, embed=embed,
                                  rot0=math.radians(rot0), flatten=1.0, step=None)
        paint_face(bm, faces, col_layer, emi_layer, rm_layer, kind, meta)

    # basalt nest: several small hex stubs, not one big box
    rng = random.Random(53)
    rocks = [
        ((0.00,  0.00, 0.02), 0.22, 0.22, 0.0, 6, 0.80),
        ((0.16,  0.10, 0.00), 0.13, 0.16, 0.4, 6, 0.75),
        ((-0.14, 0.12, 0.00), 0.12, 0.15, 1.1, 6, 0.78),
        ((0.12, -0.16, 0.00), 0.12, 0.14, 2.2, 5, 0.82),
        ((-0.12,-0.12, 0.00), 0.11, 0.13, 3.5, 6, 0.80),
        ((0.00,  0.20, 0.00), 0.10, 0.12, 0.7, 6, 0.85),
        ((0.18, -0.02, 0.00), 0.10, 0.12, 4.0, 5, 0.77),
        ((-0.18, 0.00, 0.00), 0.10, 0.11, 5.2, 6, 0.80),
    ]
    for center, rad, h, rot, sides, taper in rocks:
        faces, meta = add_crystal(bm, Vector((center[0], center[1], center[2]-0.04)),
                                  Vector((0,0,1)), h, rad, sides=sides, tip=0.0,
                                  taper=1.0, embed=0.2, rot0=rot, flatten=1.0, step=None)
        paint_face(bm, faces, col_layer, emi_layer, rm_layer, "rock", meta)

    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new("SM_CrystalCacheCrystal")
    bm.to_mesh(me)
    bm.free()
    obj = bpy.data.objects.new("SM_CrystalCacheCrystal", me)
    bpy.context.scene.collection.objects.link(obj)
    obj.data.shade_flat()
    mins, maxs, size = world_bbox(obj)
    obj.location.x -= (mins.x + maxs.x) * 0.5
    obj.location.y -= (mins.y + maxs.y) * 0.5
    obj.location.z -= mins.z
    apply_world_transform(obj)
    mins, maxs, size = world_bbox(obj)
    if abs(size.z - CLUSTER_HGT) > 0.01:
        s = CLUSTER_HGT / size.z
        obj.scale = (s,s,s)
        apply_world_transform(obj)
        mins, maxs, size = world_bbox(obj)
        obj.location.z -= mins.z
        apply_world_transform(obj)
    return obj


def rotate_hdri(degrees=125.0):
    world = bpy.context.scene.world
    if not world or not world.node_tree:
        return
    nt = world.node_tree
    env = next((n for n in nt.nodes if n.type == "TEX_ENVIRONMENT"), None)
    if env is None:
        return
    maps = nt.nodes.new("ShaderNodeMapping")
    coord = nt.nodes.new("ShaderNodeTexCoord")
    maps.inputs["Rotation"].default_value = (0, 0, math.radians(degrees))
    # unlink previous
    for l in list(env.inputs["Vector"].links):
        nt.links.remove(l)
    nt.links.new(coord.outputs["Generated"], maps.inputs["Vector"])
    nt.links.new(maps.outputs["Vector"], env.inputs["Vector"])
    # slightly dim the env so the sun is less of a blown disk; area lights carry shape
    bg = next((n for n in nt.nodes if n.type == "BACKGROUND" and n.inputs[0].is_linked), None)
    if bg:
        bg.inputs["Strength"].default_value = 0.7


def main():
    log("=== repair crystal + re-render ===")
    bpy.ops.wm.open_mainfile(filepath=str(ASSEMBLY))
    # Remove old crystal and helpers
    for name in list(bpy.data.objects.keys()):
        if name in ("SM_CrystalCacheCrystal", "UCX_SM_CrystalCacheCrystal_00", "W_crystal",
                    "GIZMO_SOCKET", "W_cradle", "FIG_Pilot_180"):
            o = bpy.data.objects[name]
            bpy.data.objects.remove(o, do_unlink=True)

    crystal = build_new_crystal()
    rec = mesh_report_dict(crystal, "repaired crystal")
    smart_uv(crystal, angle=46.0, island_margin=0.004)

    # Bake from the color attributes already on the mesh (from bmesh layers)
    # Ensure color attributes exist after to_mesh
    log("color attrs: " + str([a.name for a in crystal.data.color_attributes]))
    for layer, fname, noncolor in (("Col", "T_CrystalCacheCrystal_BC_raw.png", False),
                                   ("Emi", "T_CrystalCacheCrystal_E_raw.png", False),
                                   ("RM",  "T_CrystalCacheCrystal_RM_raw.png", False)):
        make_emit_from_attribute(crystal, layer)
        img, _ = ensure_bake_image(crystal, "r"+layer, TEX_SIZE, False)
        bake_self(crystal, "EMIT", img)
        save_image(img, TEX / fname, False)
    img, _ = ensure_bake_image(crystal, "rAO", TEX_SIZE, False)
    bake_self(crystal, "AO", img, samples=32)
    save_image(img, TEX / "T_CrystalCacheCrystal_AO_raw.png", False)
    make_bevel_normal_material(crystal, radius=0.0018, noise_scale=400.0, bump_strength=0.08, bump_dist=0.0006)
    img, _ = ensure_bake_image(crystal, "rN", TEX_SIZE, False)
    bake_self(crystal, "NORMAL", img, samples=16)
    save_image(img, TEX / "T_CrystalCacheCrystal_N_ogl.png", False)

    rc = os.system(f"python3 {Path(__file__).resolve().parent / 'process_tex.py'} {TEX}")
    if rc != 0:
        raise SystemExit("tex post failed")
    # process_tex rewrites ALL textures including cradle from raw files still in TEX
    for name in ["T_CrystalCacheCrystal_BC.png", "T_CrystalCacheCrystal_N.png",
                 "T_CrystalCacheCrystal_ORM.png", "T_CrystalCacheCrystal_E.png",
                 "T_CrystalCacheCradle_BC.png", "T_CrystalCacheCradle_N.png",
                 "T_CrystalCacheCradle_ORM.png", "T_CrystalCacheCradle_E.png"]:
        if (TEX/name).exists():
            (OUT/name).write_bytes((TEX/name).read_bytes())

    assign_pbr_material(crystal, "T_CrystalCacheCrystal", OUT, emission_strength=2.2)
    # re-assign cradle too (textures may have been rewritten identically)
    cradle_bake = bpy.data.objects.get("BAKE_CrystalCacheCradle")
    cradle_edit = bpy.data.objects.get("SM_CrystalCacheCradle")
    if cradle_bake:
        assign_pbr_material(cradle_bake, "T_CrystalCacheCradle", OUT, emission_strength=2.4)
    if cradle_edit and cradle_bake and cradle_bake.data.materials:
        cradle_edit.data.materials.clear()
        cradle_edit.data.materials.append(cradle_bake.data.materials[0])

    ucx_c = make_convex_hull("UCX_SM_CrystalCacheCrystal_00", crystal, max_points=40)
    parent_keep_world(ucx_c, crystal)
    ucx_c.hide_render = True

    # Export crystal
    try:
        export_fbx_unreal("SM_CrystalCacheCrystal", OUT)
    except Exception as e:
        log(f"export failed {e}")
        export_fbx_direct([crystal, ucx_c], OUT / "SM_CrystalCacheCrystal.fbx")
    try:
        export_glb(crystal, OUT / "SM_CrystalCacheCrystal.glb")
    except Exception as e:
        log(f"glb: {e}")
        # triangulate and retry
        select_only(crystal)
        bpy.ops.object.mode_set(mode="EDIT")
        bpy.ops.mesh.select_all(action="SELECT")
        bpy.ops.mesh.quads_convert_to_tris(quad_method="BEAUTY", ngon_method="BEAUTY")
        bpy.ops.object.mode_set(mode="OBJECT")
        export_glb(crystal, OUT / "SM_CrystalCacheCrystal.glb")

    # Lighting + pilot
    rotate_hdri(140)
    # hide leftover gizmos / wires / ground? keep ground
    for n in ("W_cradle", "W_crystal", "GIZMO_SOCKET"):
        if n in bpy.data.objects:
            bpy.data.objects[n].hide_render = True

    pilot = make_pilot()
    # stand beside the front-right corner, not covering the emblem
    pilot.location = Vector((1.15, -1.35, 0.0))

    # Use the applied bake mesh for beauty
    if cradle_edit:
        cradle_edit.hide_render = True
        cradle_edit.hide_viewport = True
    if cradle_bake:
        cradle_bake.hide_render = False
        cradle_bake.hide_viewport = False
        assign_pbr_material(cradle_bake, "T_CrystalCacheCradle", OUT, emission_strength=2.4)
    src = bpy.data.objects.get("SRC_CrystalCacheCradle")
    if src:
        src.hide_render = True
        src.hide_viewport = True
    ucx0 = bpy.data.objects.get("UCX_SM_CrystalCacheCradle_00")
    if ucx0:
        ucx0.hide_render = True

    crystal.location = Vector((0.0, 0.0, FLOOR_Z))

    render_view(RENDERS / "assembled_hero.png",
                loc=Vector((4.6, -5.2, 2.6)), target=Vector((0.1, 0.0, 0.7)), lens=45)
    hide_for_render(crystal, pilot, hide=True)
    render_view(RENDERS / "cradle_alone.png",
                loc=Vector((4.4, -5.0, 2.5)), target=Vector((0.0, 0.0, 0.85)), lens=45)
    hide_for_render(cradle_bake, hide=True)
    hide_for_render(crystal, hide=False)
    saved = crystal.location.copy()
    crystal.location = Vector((0,0,0))
    render_view(RENDERS / "crystal_alone.png",
                loc=Vector((2.2, -2.4, 1.5)), target=Vector((0.0, 0.0, 0.7)), lens=50)
    crystal.location = saved
    hide_for_render(cradle_bake, crystal, hide=False)
    hide_for_render(pilot, hide=True)
    render_view(RENDERS / "closeup.png",
                loc=Vector((1.7, -1.1, 1.15)), target=Vector((0.15, 0.0, 0.85)), lens=50)
    hide_for_render(pilot, hide=False)
    render_view(RENDERS / "front_ortho.png",
                loc=Vector((6.0, 0.0, 0.95)), target=Vector((0.0, 0.0, 0.9)), ortho=5.2)
    render_view(RENDERS / "end_ortho.png",
                loc=Vector((0.0, -6.0, 0.95)), target=Vector((0.0, 0.0, 0.9)), ortho=2.6)

    clay = make_clay_material()
    for o in (cradle_bake, crystal, pilot):
        if o:
            o.data.materials.clear()
            o.data.materials.append(clay)
    hide_for_render(pilot, cradle_bake, crystal, hide=False)
    render_view(RENDERS / "clay.png",
                loc=Vector((4.6, -5.2, 2.6)), target=Vector((0.1, 0.0, 0.7)), lens=45)
    def wire_copy(o, name):
        w = o.copy(); w.data = o.data.copy(); w.name = name
        bpy.context.scene.collection.objects.link(w)
        mod = w.modifiers.new("Wire", "WIREFRAME")
        mod.thickness = 0.004; mod.use_replace = True
        wmat = bpy.data.materials.new(name+"_w"); wmat.use_nodes = True
        wmat.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.02,0.02,0.02,1)
        wmat.node_tree.nodes["Principled BSDF"].inputs["Emission Color"].default_value = (0.06,0.06,0.07,1)
        wmat.node_tree.nodes["Principled BSDF"].inputs["Emission Strength"].default_value = 0.35
        w.data.materials.clear(); w.data.materials.append(wmat)
        return w
    w1 = wire_copy(cradle_bake, "W_cradle")
    w2 = wire_copy(crystal, "W_crystal")
    render_view(RENDERS / "clay_wire.png",
                loc=Vector((4.6, -5.2, 2.6)), target=Vector((0.1, 0.0, 0.7)), lens=45)
    w1.hide_render = True; w2.hide_render = True

    # Collision callout: solid-ish box + visible axis at the socket (no slow alpha)
    if ucx0:
        ucx0.hide_render = False
        mat = bpy.data.materials.new("M_UCXvis")
        mat.use_nodes = True
        mat.blend_method = "BLEND"
        try: mat.surface_render_method = "BLENDED"
        except Exception: pass
        bsdf = mat.node_tree.nodes["Principled BSDF"]
        bsdf.inputs["Base Color"].default_value = (0.15, 0.85, 0.25, 1)
        bsdf.inputs["Alpha"].default_value = 0.22
        bsdf.inputs["Emission Color"].default_value = (0.05, 0.4, 0.1, 1)
        bsdf.inputs["Emission Strength"].default_value = 0.15
        ucx0.data.materials.clear(); ucx0.data.materials.append(mat)
    ucx_c.hide_render = False
    matb = bpy.data.materials.new("M_UCXc")
    matb.use_nodes = True
    matb.blend_method = "BLEND"
    try: matb.surface_render_method = "BLENDED"
    except Exception: pass
    bsdf = matb.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (0.15, 0.45, 0.95, 1)
    bsdf.inputs["Alpha"].default_value = 0.30
    ucx_c.data.materials.clear(); ucx_c.data.materials.append(matb)
    # axis gizmo at socket
    bm = bmesh.new()
    for axis, L in enumerate([(0.32,0.014,0.014),(0.014,0.32,0.014),(0.014,0.014,0.32)]):
        bmesh.ops.create_cube(bm, size=1.0,
            matrix=Matrix.Translation((L[0]/2 if axis==0 else 0, L[1]/2 if axis==1 else 0, L[2]/2 if axis==2 else 0))
                   @ Matrix.Diagonal((L[0], L[1], L[2], 1)))
    me = bpy.data.meshes.new("Gizmo"); bm.to_mesh(me); bm.free()
    giz = bpy.data.objects.new("GIZMO_SOCKET", me)
    giz.location = Vector((0,0,FLOOR_Z))
    bpy.context.scene.collection.objects.link(giz)
    gm = bpy.data.materials.new("M_Gizmo"); gm.use_nodes = True
    gm.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (1,0.25,0.05,1)
    gm.node_tree.nodes["Principled BSDF"].inputs["Emission Color"].default_value = (1,0.3,0.05,1)
    gm.node_tree.nodes["Principled BSDF"].inputs["Emission Strength"].default_value = 4.0
    giz.data.materials.append(gm)
    # restore pbr on visible meshes for this panel? keep clay + collision overlay, it's a callout
    render_view(RENDERS / "collision_socket.png",
                loc=Vector((4.8, -5.4, 2.8)), target=Vector((0.1, 0.0, 0.7)), lens=45)

    # Restore PBR, hide helpers, save blends
    ucx0.hide_render = True if ucx0 else None
    ucx_c.hide_render = True
    giz.hide_render = True
    assign_pbr_material(cradle_bake, "T_CrystalCacheCradle", OUT, emission_strength=2.4)
    assign_pbr_material(crystal, "T_CrystalCacheCrystal", OUT, emission_strength=2.2)
    if cradle_edit:
        cradle_edit.data.materials.clear()
        cradle_edit.data.materials.append(cradle_bake.data.materials[0])
        cradle_edit.hide_render = False
        cradle_edit.hide_viewport = False
    if cradle_bake:
        cradle_bake.hide_render = True
        cradle_bake.hide_viewport = True
    crystal.location = Vector((0.0, 0.0, FLOOR_Z))
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "SM_CrystalCache_assembly.blend"))
    log("saved assembly")
    (WORK / "repair_meta.json").write_text(json.dumps({"crystal": rec}, indent=2, default=str))
    log("=== repair complete ===")

if __name__ == "__main__":
    main()

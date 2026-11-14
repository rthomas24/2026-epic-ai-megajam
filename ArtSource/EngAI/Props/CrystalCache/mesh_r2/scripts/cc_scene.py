
from __future__ import annotations
import math, runpy
import bpy, bmesh
from mathutils import Matrix, Vector
from cc_paths import *
from cc_utils import *

def make_box(name, center, size, collection=None):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    for v in bm.verts:
        v.co = Vector((v.co.x * size.x, v.co.y * size.y, v.co.z * size.z)) + Vector(center)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me); bm.free()
    obj = bpy.data.objects.new(name, me)
    (collection or bpy.context.scene.collection).objects.link(obj)
    return obj

def make_convex_hull(name, src_obj, collection=None, max_points=48):
    mw = src_obj.matrix_world
    dg = bpy.context.evaluated_depsgraph_get()
    ev = src_obj.evaluated_get(dg)
    me = ev.to_mesh()
    try:
        world_pts = [mw @ v.co for v in me.vertices]
    finally:
        ev.to_mesh_clear()
    golden = math.pi * (3.0 - math.sqrt(5.0))
    chosen = {}
    n = max_points
    for i in range(n):
        y = 1.0 - (i / float(max(n - 1, 1))) * 2.0
        r = math.sqrt(max(0.0, 1.0 - y * y))
        d = Vector((math.cos(golden * i) * r, y, math.sin(golden * i) * r))
        best = max(world_pts, key=lambda p: p.dot(d))
        chosen[tuple(round(c, 5) for c in best)] = best
    bm = bmesh.new()
    for p in chosen.values():
        bm.verts.new(p)
    bmesh.ops.convex_hull(bm, input=list(bm.verts))
    loose = [v for v in bm.verts if not v.link_faces]
    if loose:
        bmesh.ops.delete(bm, geom=loose, context="VERTS")
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me); bm.free()
    obj = bpy.data.objects.new(name, me)
    (collection or bpy.context.scene.collection).objects.link(obj)
    return obj

def make_socket(name, location, collection=None):
    empty = bpy.data.objects.new(name, None)
    empty.empty_display_type = "ARROWS"
    empty.empty_display_size = 0.20
    empty.location = location
    (collection or bpy.context.scene.collection).objects.link(empty)
    return empty

def make_pilot(name="FIG_Pilot_180"):
    def add_cyl(loc, r, h, segs=12):
        bm = bmesh.new()
        bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=segs,
                              radius1=r, radius2=r, depth=h,
                              matrix=Matrix.Translation((loc[0], loc[1], loc[2] + h * 0.5)))
        return bm
    def add_uvsph(loc, r, segs=12):
        bm = bmesh.new()
        bmesh.ops.create_uvsphere(bm, u_segments=segs, v_segments=segs, radius=r,
                                  matrix=Matrix.Translation(loc))
        return bm
    parts = [
        add_cyl((0.06, 0, 0.00), 0.055, 0.90),
        add_cyl((-0.06, 0, 0.00), 0.055, 0.90),
        add_cyl((0.0, 0, 0.90), 0.13, 0.58),
        add_cyl((0.20, 0, 0.92), 0.040, 0.56),
        add_cyl((-0.20, 0, 0.92), 0.040, 0.56),
        add_uvsph((0.0, 0, 1.66), 0.11),
    ]
    bm = bmesh.new()
    for p in parts:
        verts = {v: bm.verts.new(v.co) for v in p.verts}
        for f in p.faces:
            try:
                bm.faces.new([verts[v] for v in f.verts])
            except ValueError:
                pass
        p.free()
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=0.0005)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me); bm.free()
    obj = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(obj)
    mins, maxs, size = world_bbox(obj)
    s = PILOT_H / size.z
    obj.scale = (s, s, s)
    apply_world_transform(obj)
    mins, maxs, size = world_bbox(obj)
    obj.location.z -= mins.z
    apply_world_transform(obj)
    mat = bpy.data.materials.new("M_Pilot")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (0.55, 0.56, 0.58, 1)
    bsdf.inputs["Roughness"].default_value = 0.45
    obj.data.materials.append(mat)
    obj.data.shade_smooth()
    return obj

def make_clay_material(name="M_Clay", color=(0.45, 0.46, 0.48, 1)):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = color
    bsdf.inputs["Roughness"].default_value = 0.50
    bsdf.inputs["Metallic"].default_value = 0.0
    bsdf.inputs["Emission Strength"].default_value = 0.0
    return mat

def setup_hdri(strength=1.0):
    sc = bpy.context.scene
    world = bpy.data.worlds.new("WorldHDRI")
    sc.world = world
    world.use_nodes = True
    nt = world.node_tree
    for n in list(nt.nodes):
        nt.nodes.remove(n)
    env = nt.nodes.new("ShaderNodeTexEnvironment")
    env.image = bpy.data.images.load(str(HDRI))
    try:
        env.image.colorspace_settings.name = "Linear Rec.709"
    except Exception:
        try:
            env.image.colorspace_settings.name = "Linear"
        except Exception:
            pass
    bg = nt.nodes.new("ShaderNodeBackground")
    bg.inputs["Strength"].default_value = strength
    mix = nt.nodes.new("ShaderNodeMixShader")
    bg_grey = nt.nodes.new("ShaderNodeBackground")
    bg_grey.inputs["Color"].default_value = (0.18, 0.18, 0.19, 1)
    bg_grey.inputs["Strength"].default_value = 0.7
    mix.inputs["Fac"].default_value = 0.28
    out = nt.nodes.new("ShaderNodeOutputWorld")
    nt.links.new(env.outputs["Color"], bg.inputs["Color"])
    nt.links.new(bg.outputs["Background"], mix.inputs[1])
    nt.links.new(bg_grey.outputs["Background"], mix.inputs[2])
    nt.links.new(mix.outputs["Shader"], out.inputs["Surface"])
    return world

def add_studio_lights():
    def area(name, loc, rot, energy, size, color=(1, 1, 1)):
        bpy.ops.object.light_add(type="AREA", location=loc, rotation=rot)
        L = bpy.context.object
        L.name = name
        L.data.energy = energy
        L.data.size = size
        L.data.color = color
        L.data.shape = "RECTANGLE"
        L.data.size_y = size * 0.7
        return L
    area("key", (3.0, -3.4, 3.2), (math.radians(55), 0, math.radians(40)), 80, 2.4, (1.0, 0.98, 0.95))
    area("fill", (-3.2, -1.5, 2.2), (math.radians(65), 0, math.radians(-50)), 30, 3.2, (0.90, 0.95, 1.0))
    area("rim", (0.2, 4.0, 2.8), (math.radians(70), 0, math.radians(180)), 40, 2.2, (1.0, 0.97, 0.93))

def look_at(cam_obj, target):
    direction = Vector(target) - cam_obj.location
    cam_obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()

def render_view(filepath, loc, target, ortho=None, res=(1280, 960), lens=50, film_transparent=False):
    sc = bpy.context.scene
    sc.render.resolution_x, sc.render.resolution_y = res
    sc.render.resolution_percentage = 100
    sc.render.film_transparent = film_transparent
    sc.render.engine = "BLENDER_EEVEE_NEXT"
    sc.eevee.taa_render_samples = 48
    sc.view_settings.view_transform = "Standard"
    sc.view_settings.exposure = 0.10
    if "C_Sheet" not in bpy.data.objects:
        cam = bpy.data.cameras.new("C_Sheet")
        cobj = bpy.data.objects.new("C_Sheet", cam)
        bpy.context.scene.collection.objects.link(cobj)
    else:
        cobj = bpy.data.objects["C_Sheet"]
        cam = cobj.data
    sc.camera = cobj
    cobj.location = loc
    look_at(cobj, target)
    if ortho is not None:
        cam.type = "ORTHO"
        cam.ortho_scale = ortho
    else:
        cam.type = "PERSP"
        cam.lens = lens
    sc.render.filepath = str(filepath)
    bpy.ops.render.render(write_still=True)
    log(f"    render {filepath}")
    return filepath

def export_fbx_unreal(root_name, out_dir):
    m = runpy.run_path(str(EXPORT_PY))
    result = m["main"](root=root_name, out_dir=str(out_dir), front_axis="+X", collision="none")
    log(f"    FBX export {root_name}: {result}")
    return result

def export_fbx_direct(objs, path):
    select_only(*objs)
    bpy.ops.export_scene.fbx(
        filepath=str(path),
        use_selection=True,
        global_scale=1.0, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
        axis_forward="-Z", axis_up="Y", use_space_transform=True,
        use_mesh_modifiers=True, mesh_smooth_type="FACE", use_triangles=True, use_tspace=True,
        bake_space_transform=False, path_mode="AUTO",
        embed_textures=False, object_types={"MESH", "EMPTY"},
    )
    log(f"    FBX direct {path}")

def export_glb(obj, path):
    select_only(obj)
    bpy.ops.export_scene.gltf(
        filepath=str(path),
        export_format="GLB",
        use_selection=True,
        export_apply=True,
        export_yup=True,
        export_materials="EXPORT",
        export_image_format="AUTO",
        export_texcoords=True,
        export_normals=True,
        export_tangents=True,
        export_cameras=False,
        export_lights=False,
        export_extras=False,
    )
    log(f"    GLB export {path}")

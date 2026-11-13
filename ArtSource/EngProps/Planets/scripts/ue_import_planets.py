# Import the planet kit into /Game/TriadDrift/Gameplay/Props/Planets: meshes (SM_Planet_Sphere,
# SM_Planet_Atmosphere, SM_Planet_Ring), all textures, master materials (M_TD_Planet,
# M_TD_PlanetAtmosphere, M_TD_PlanetRing, M_TD_Ground) and the instances. Idempotent: masters are kept
# when they exist (editing a material that has instances crashes the headless editor); set
# PL_REBUILD=1 to delete the instances and masters and build them again.
# Run: powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Planets\scripts\ue_import_planets.py
import json
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ue_mat_builder  # noqa: E402
import ue_planet_surface as surf  # noqa: E402
import ue_ground_material as ground_mat  # noqa: E402
ROOT = os.path.dirname(HERE)
TEX = os.path.join(ROOT, "textures")
MESH = os.path.join(ROOT, "meshes")
DEST = "/Game/TriadDrift/Gameplay/Props/Planets"
REBUILD = os.environ.get("PL_REBUILD", "0") == "1"

eal = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
MP = unreal.MaterialProperty
ST = unreal.MaterialSamplerType
problems = []


def log(m):
    unreal.log("TDPY: " + str(m))


def lin(h):
    h = h.lstrip("#")
    c = [int(h[i:i + 2], 16) / 255.0 for i in (0, 2, 4)]
    return [x / 12.92 if x <= 0.04045 else ((x + 0.055) / 1.055) ** 2.4 for x in c]


# ------------------------------------------------------------------ look values (mirror the Blender review)
PLANET = {
    "Verdance": dict(cloud_color=lin("#F4FBFF"), cloud_opacity=0.92, rim=lin("#40E8FF"), rim_strength=0.5,
                     atmo=lin("#3CF2D0"), atmo_intensity=3.0, rim_power=4.0, cloud_pan=0.0015),
    "Cinder": dict(cloud_color=lin("#8C8086"), cloud_opacity=0.80, rim=lin("#FF6A2A"), rim_strength=0.25,
                   atmo=lin("#FF7A2A"), atmo_intensity=2.6, rim_power=4.5, cloud_pan=0.0010),
}
SUN_DIR = surf.SUN_DIR              # default: direction TOWARDS the sun (ATDSkySphere's baked sun); code sets it per level
# Ground look values (GROUND / MACRO) live in ue_ground_material.py.


# ------------------------------------------------------------------ textures
def import_textures():
    names = []
    for p in ("Verdance", "Cinder"):
        names += ["T_Planet_%s_%s" % (p, s) for s in ("BC", "N", "ORM", "E", "Clouds")]
        names += ["T_Ground_%s_%s" % (p, s) for s in ("BC", "N", "ORM", "E")]
    names.append("T_PlanetRing_Verdance")
    names.append("T_Ground_MacroNoise")          # build_ground_noise.py
    tasks = []
    for n in names:
        t = unreal.AssetImportTask()
        t.set_editor_property("filename", os.path.join(TEX, n + ".png").replace("\\", "/"))
        t.set_editor_property("destination_path", DEST)
        t.set_editor_property("automated", True)
        t.set_editor_property("replace_existing", True)
        t.set_editor_property("save", False)
        tasks.append(t)
    tools.import_asset_tasks(tasks)
    out = {}
    for n in names:
        tex = eal.load_asset("%s/%s" % (DEST, n))
        if tex is None:
            problems.append("texture not imported: " + n)
            continue
        sfx = n.rsplit("_", 1)[1]
        if sfx == "N":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property("srgb", False)
            tex.set_editor_property("flip_green_channel", False)
        elif sfx in ("ORM", "Clouds", "MacroNoise"):
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
            tex.set_editor_property("srgb", False)
        else:
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
            tex.set_editor_property("srgb", True)
        if n.startswith("T_PlanetRing"):
            tex.set_editor_property("address_y", unreal.TextureAddress.TA_WRAP)
            tex.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
        eal.save_loaded_asset(tex)
        out[n] = tex
        log("texture %s %dx%d srgb=%s %s" % (n, tex.blueprint_get_size_x(), tex.blueprint_get_size_y(),
                                            tex.get_editor_property("srgb"), tex.get_editor_property("compression_settings")))
    return out


# ------------------------------------------------------------------ meshes
def import_mesh(name):
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", False)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    smd = ui.get_editor_property("static_mesh_import_data")
    for k, v in (("combine_meshes", True), ("auto_generate_collision", False), ("generate_lightmap_u_vs", False),
                 ("convert_scene", True)):
        try:
            smd.set_editor_property(k, v)
        except Exception as e:
            problems.append("%s: %s" % (k, e))
    smd.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", os.path.join(MESH, name + ".fbx").replace("\\", "/"))
    t.set_editor_property("destination_path", DEST)
    t.set_editor_property("destination_name", name)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", False)
    t.set_editor_property("options", ui)
    tools.import_asset_tasks([t])
    mesh = eal.load_asset("%s/%s" % (DEST, name))
    if mesh is None:
        problems.append("mesh not imported: " + name)
        return None
    sm = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    try:
        sm.set_generate_lightmap_uv(mesh, False)
    except Exception:
        pass
    if name == "SM_Planet_Sphere":
        try:
            sm.remove_collisions(mesh)
            enum = getattr(unreal, "ScriptCollisionShapeType", None) or getattr(unreal, "ScriptingCollisionShapeType")
            sm.add_simple_collisions(mesh, enum.SPHERE)
        except Exception as e:
            problems.append("sphere collision: %s" % e)
    eal.save_loaded_asset(mesh)
    b = mesh.get_bounds()
    tris = sm.get_number_verts(mesh, 0)
    log("mesh %s: verts %d, tris(LOD0) %s, bounds extent (%.1f, %.1f, %.1f) cm, nanite %s" % (
        name, tris, _tri_count(mesh), b.box_extent.x, b.box_extent.y, b.box_extent.z,
        mesh.get_editor_property("nanite_settings").get_editor_property("enabled")))
    return mesh


def _tri_count(mesh):
    try:
        return mesh.get_num_triangles(0)
    except Exception:
        return "n/a"


# ------------------------------------------------------------------ material graph helpers (ue_mat_builder.py)
class MB(ue_mat_builder.MB):
    def __init__(self, mat):
        super().__init__(mat, problems)


def new_master(name):
    path = "%s/%s" % (DEST, name)
    if eal.does_asset_exist(path):
        if not REBUILD:
            log("master exists, kept: " + path)
            return None, eal.load_asset(path)
        eal.delete_asset(path)
    mat = tools.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())
    return mat, None


def finish(mat):
    try:
        mel.recompile_material(mat)
    except Exception:
        log("note: no renderer here, %s compiles its shaders on first load" % mat.get_name())
    mel.layout_material_expressions(mat)
    eal.save_asset(mat.get_path_name().split(".")[0], only_if_is_dirty=False)
    log("built " + mat.get_path_name())
    return mat


def sun_terms(b):
    """returns (ndl node) = dot(VertexNormalWS, normalize(SunDirection))."""
    sun = b.vector("SunDirection", SUN_DIR)
    sunm = b.mask(sun, True, True, True)
    n = b.e(unreal.MaterialExpressionVertexNormalWS)
    return b.dot(n, b.norm(sunm))


def build_planet_master(t):
    # Unlit, lights itself from SunDirection: clear terminator, glow only on the night side (ue_planet_surface.py).
    mat, kept = new_master("M_TD_Planet")
    if kept:
        return kept
    surf.build_surface_graph(MB(mat), t)
    return finish(mat)


def build_atmo_master():
    mat, kept = new_master("M_TD_PlanetAtmosphere")
    if kept:
        return kept
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", False)
    b = MB(mat)
    n = b.e(unreal.MaterialExpressionVertexNormalWS)
    v = b.e(unreal.MaterialExpressionCameraVectorWS)
    x = b.sat(b.dot(n, v))                                   # 1 at the centre of the disc, 0 at the shell silhouette
    inner = b.pow(b.one_minus(x), b.scalar("RimPower", 4.0))
    edge = b.pow(b.sat(b.mul(x, b.scalar("EdgeSoftness", 4.5))), b.const(3.0))
    ndl = sun_terms(b)
    day = b.sat(b.add(b.mul(ndl, b.const(0.8)), b.const(0.45)))
    day = b.lerp(b.const(1.0), day, b.scalar("SunMaskAmount", 0.7))
    glow = b.mul(b.mul(inner, edge), day)
    col = b.mul(b.vector("AtmoColor", (0.2, 0.9, 0.8)), b.scalar("AtmoIntensity", 3.0))
    b.out(b.mul(col, glow), "", MP.MP_EMISSIVE_COLOR)
    return finish(mat)


def build_ring_master(t):
    mat, kept = new_master("M_TD_PlanetRing")
    if kept:
        return kept
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    b = MB(mat)
    rt = b.tex("RingTex", t["T_PlanetRing_Verdance"], ST.SAMPLERTYPE_COLOR)
    col = b.mul(b.mul(rt, b.vector("RingTint", (1, 1, 1)), "RGB"), b.scalar("Brightness", 1.2))
    b.out(col, "", MP.MP_EMISSIVE_COLOR)
    b.out(b.mul(rt, b.scalar("Opacity", 0.85), "A"), "", MP.MP_OPACITY)
    return finish(mat)


def build_ground_master(t):
    # Anti-tiling ground graph lives in ue_ground_material.py (also used by ue_ground_rebuild.py).
    mat, kept = new_master("M_TD_Ground")
    if kept:
        return kept
    ground_mat.build_ground_graph(MB(mat), t)
    return finish(mat)


# ------------------------------------------------------------------ instances
def mi(name, parent, scalars=None, vectors=None, textures=None):
    path = "%s/%s" % (DEST, name)
    if REBUILD and eal.does_asset_exist(path):
        pass
    inst = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(
        name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(inst, parent)
    for k, v in (scalars or {}).items():
        mel.set_material_instance_scalar_parameter_value(inst, k, float(v))
    for k, v in (vectors or {}).items():
        mel.set_material_instance_vector_parameter_value(inst, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
    for k, v in (textures or {}).items():
        mel.set_material_instance_texture_parameter_value(inst, k, v)
    eal.save_asset(path, only_if_is_dirty=False)
    log("instance %s -> %s" % (name, parent.get_name()))
    return inst


def delete_instances():
    for n in ("MI_Planet_Verdance", "MI_Planet_Cinder", "MI_PlanetAtmo_Verdance", "MI_PlanetAtmo_Cinder",
              "MI_PlanetRing_Verdance", "MI_Ground_Verdance", "MI_Ground_Cinder"):
        p = "%s/%s" % (DEST, n)
        if eal.does_asset_exist(p):
            eal.delete_asset(p)


def main():
    if not eal.does_directory_exist(DEST):
        eal.make_directory(DEST)
    if REBUILD:
        delete_instances()
    t = import_textures()
    meshes = {n: import_mesh(n) for n in ("SM_Planet_Sphere", "SM_Planet_Atmosphere", "SM_Planet_Ring")}
    m_planet = build_planet_master(t)
    m_atmo = build_atmo_master()
    m_ring = build_ring_master(t)
    m_ground = build_ground_master(t)
    for p, c in PLANET.items():
        mi("MI_Planet_" + p, m_planet,
           scalars=dict(dict(CloudOpacity=c["cloud_opacity"], SurfaceRimStrength=c["rim_strength"], CloudPanSpeed=c["cloud_pan"]),
                        **surf.INSTANCE_VALUES[p]["scalars"]),
           vectors=dict(dict(CloudColor=c["cloud_color"], SurfaceRimColor=c["rim"], SunDirection=SUN_DIR),
                        **surf.INSTANCE_VALUES[p]["vectors"]),
           textures=dict(BaseColorTex=t["T_Planet_%s_BC" % p], NormalTex=t["T_Planet_%s_N" % p], ORMTex=t["T_Planet_%s_ORM" % p],
                         EmissiveTex=t["T_Planet_%s_E" % p], CloudTex=t["T_Planet_%s_Clouds" % p]))
        mi("MI_PlanetAtmo_" + p, m_atmo,
           scalars=dict(AtmoIntensity=c["atmo_intensity"], RimPower=c["rim_power"], EdgeSoftness=4.5, SunMaskAmount=surf.ATMO_VALUES["SunMaskAmount"]),
           vectors=dict(AtmoColor=c["atmo"], SunDirection=SUN_DIR))
    mi("MI_PlanetRing_Verdance", m_ring, scalars=dict(Brightness=1.2, Opacity=0.85), vectors=dict(RingTint=(1, 1, 1)),
       textures=dict(RingTex=t["T_PlanetRing_Verdance"]))
    for g in ground_mat.GROUND:
        scalars, vectors = ground_mat.instance_values(g)
        mi("MI_Ground_" + g, m_ground, scalars=scalars, vectors=vectors,
           textures=dict(BaseColorTex=t["T_Ground_%s_BC" % g], NormalTex=t["T_Ground_%s_N" % g],
                         ORMTex=t["T_Ground_%s_ORM" % g], EmissiveTex=t["T_Ground_%s_E" % g]))
    # default materials on the meshes
    for mname, mat in (("SM_Planet_Sphere", "MI_Planet_Verdance"), ("SM_Planet_Atmosphere", "MI_PlanetAtmo_Verdance"),
                       ("SM_Planet_Ring", "MI_PlanetRing_Verdance")):
        mesh = meshes.get(mname)
        if mesh is None:
            continue
        mesh.set_material(0, eal.load_asset("%s/%s" % (DEST, mat)))
        eal.save_loaded_asset(mesh)
        log("%s slot 0 = %s" % (mname, mat))
    dirty = [str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()]
    log("still dirty (not saved): %s" % dirty)
    log("assets: %s" % sorted(str(a).split(".")[-1] for a in eal.list_assets(DEST)))
    for p_ in problems:
        log("PROBLEM: " + p_)
    if problems:
        log("FAILED (%d problems)" % len(problems))
    else:
        log("OK")


main()

# Import the planet kit into /Game/TriadDrift/Gameplay/Props/Planets: meshes (SM_Planet_Sphere,
# SM_Planet_Atmosphere, SM_Planet_Ring), all textures, master materials (M_TD_Planet,
# M_TD_PlanetAtmosphere, M_TD_PlanetRing, M_TD_Ground) and the instances. Idempotent: masters are kept
# when they exist (editing a material that has instances crashes the headless editor); set
# PL_REBUILD=1 to delete the instances and masters and build them again.
# Run: powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Planets\scripts\ue_import_planets.py
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
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
    "Verdance": dict(cloud_color=lin("#F4FBFF"), cloud_opacity=0.92, emissive=6.0, rim=lin("#40E8FF"), rim_strength=0.5,
                     atmo=lin("#3CF2D0"), atmo_intensity=3.0, rim_power=4.0, cloud_pan=0.0015),
    "Cinder": dict(cloud_color=lin("#8C8086"), cloud_opacity=0.80, emissive=8.0, rim=lin("#FF6A2A"), rim_strength=0.25,
                   atmo=lin("#FF7A2A"), atmo_intensity=2.6, rim_power=4.5, cloud_pan=0.0010),
}
SUN_DIR = (0.8, -0.45, 0.35)        # default: direction TOWARDS the sun; set per level (negated light forward vector)
GROUND = {
    "Verdance": dict(emissive=4.0, tint_a=(0.80, 0.80, 0.72), tint_b=(1.0, 1.15, 1.05)),
    "Cinder": dict(emissive=6.0, tint_a=(0.85, 0.82, 0.82), tint_b=(1.1, 1.0, 0.95)),
}
MACRO = dict(scale1=7.3, scale2=31.7, angle1=37.0, angle2=-61.0, strength=0.6, mip=6,
             b_scale=1.37, b_offset=(0.31, 0.57), blend_contrast=1.5, glow_macro=0.8)


# ------------------------------------------------------------------ textures
def import_textures():
    names = []
    for p in ("Verdance", "Cinder"):
        names += ["T_Planet_%s_%s" % (p, s) for s in ("BC", "N", "ORM", "E", "Clouds")]
        names += ["T_Ground_%s_%s" % (p, s) for s in ("BC", "N", "ORM", "E")]
    names.append("T_PlanetRing_Verdance")
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
        elif sfx in ("ORM", "Clouds"):
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


# ------------------------------------------------------------------ material graph helpers
class MB:
    def __init__(self, mat):
        self.m = mat
        self.y = 0

    def e(self, cls, x=-800, **props):
        self.y += 120
        n = mel.create_material_expression(self.m, cls, x, self.y)
        for k, v in props.items():
            n.set_editor_property(k, v)
        return n

    def c(self, a, ao, b, bi):
        if not mel.connect_material_expressions(a, ao, b, bi):
            problems.append("%s: could not connect %s.%s -> %s.%s" % (self.m.get_name(), a.get_name(), ao, b.get_name(), bi))

    def out(self, a, ao, prop):
        if not mel.connect_material_property(a, ao, prop):
            problems.append("%s: could not connect %s.%s -> %s" % (self.m.get_name(), a.get_name(), ao, prop))

    def scalar(self, name, v):
        return self.e(unreal.MaterialExpressionScalarParameter, parameter_name=name, default_value=float(v))

    def vector(self, name, rgb):
        return self.e(unreal.MaterialExpressionVectorParameter, parameter_name=name,
                      default_value=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))

    def const(self, v):
        return self.e(unreal.MaterialExpressionConstant, r=float(v))

    def const2(self, a, b):
        return self.e(unreal.MaterialExpressionConstant2Vector, r=float(a), g=float(b))

    def const3(self, a, b, c):
        return self.e(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(a, b, c, 1.0))

    def tex(self, name, texture, sampler, uv=None, uv_out=""):
        n = self.e(unreal.MaterialExpressionTextureSampleParameter2D, parameter_name=name)
        n.set_editor_property("texture", texture)          # default texture type must match the sampler type
        n.set_editor_property("sampler_type", sampler)
        if uv is not None:
            self.c(uv, uv_out, n, "UVs")
        return n

    def op(self, cls, a, b, ao="", bo=""):
        n = self.e(cls)
        self.c(a, ao, n, "A")
        self.c(b, bo, n, "B")
        return n

    def mul(self, a, b, ao="", bo=""): return self.op(unreal.MaterialExpressionMultiply, a, b, ao, bo)
    def add(self, a, b, ao="", bo=""): return self.op(unreal.MaterialExpressionAdd, a, b, ao, bo)
    def sub(self, a, b, ao="", bo=""): return self.op(unreal.MaterialExpressionSubtract, a, b, ao, bo)
    def div(self, a, b, ao="", bo=""): return self.op(unreal.MaterialExpressionDivide, a, b, ao, bo)
    def dot(self, a, b, ao="", bo=""): return self.op(unreal.MaterialExpressionDotProduct, a, b, ao, bo)
    def append(self, a, b, ao="", bo=""): return self.op(unreal.MaterialExpressionAppendVector, a, b, ao, bo)

    def lerp(self, a, b, alpha, ao="", bo="", alo=""):
        n = self.e(unreal.MaterialExpressionLinearInterpolate)
        self.c(a, ao, n, "A")
        self.c(b, bo, n, "B")
        self.c(alpha, alo, n, "Alpha")
        return n

    def unary(self, cls, a, ao=""):
        n = self.e(cls)
        self.c(a, ao, n, "")
        return n

    def sat(self, a, ao=""): return self.unary(unreal.MaterialExpressionSaturate, a, ao)
    def frac(self, a, ao=""): return self.unary(unreal.MaterialExpressionFrac, a, ao)
    def norm(self, a, ao=""): return self.unary(unreal.MaterialExpressionNormalize, a, ao)
    def one_minus(self, a, ao=""): return self.unary(unreal.MaterialExpressionOneMinus, a, ao)

    def pow(self, a, b, ao=""):
        n = self.e(unreal.MaterialExpressionPower)
        self.c(a, ao, n, "Base")
        self.c(b, "", n, "Exponent")
        return n

    def mask(self, a, r=True, g=False, b=False, ao=""):
        n = self.e(unreal.MaterialExpressionComponentMask, r=r, g=g, b=b, a=False)
        self.c(a, ao, n, "")
        return n


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
    mat, kept = new_master("M_TD_Planet")
    if kept:
        return kept
    b = MB(mat)
    uv = b.e(unreal.MaterialExpressionTextureCoordinate)
    bc = b.tex("BaseColorTex", t["T_Planet_Verdance_BC"], ST.SAMPLERTYPE_COLOR, uv)
    nn = b.tex("NormalTex", t["T_Planet_Verdance_N"], ST.SAMPLERTYPE_NORMAL, uv)
    orm = b.tex("ORMTex", t["T_Planet_Verdance_ORM"], ST.SAMPLERTYPE_MASKS, uv)
    em = b.tex("EmissiveTex", t["T_Planet_Verdance_E"], ST.SAMPLERTYPE_COLOR, uv)
    # clouds: U offset = frac(time * CloudPanSpeed)
    tm = b.e(unreal.MaterialExpressionTime)
    pan = b.frac(b.mul(tm, b.scalar("CloudPanSpeed", 0.0015)))
    cuv = b.add(uv, b.append(pan, b.const(0.0)))
    cl = b.tex("CloudTex", t["T_Planet_Verdance_Clouds"], ST.SAMPLERTYPE_MASKS, cuv)
    ca = b.mul(cl, b.scalar("CloudOpacity", 0.9), "R")
    base = b.lerp(bc, b.vector("CloudColor", (1, 1, 1)), ca, "RGB")
    b.out(base, "", MP.MP_BASE_COLOR)
    b.out(b.lerp(orm, b.const(0.9), ca, "G"), "", MP.MP_ROUGHNESS)
    b.out(orm, "R", MP.MP_AMBIENT_OCCLUSION)
    b.out(b.const(0.0), "", MP.MP_METALLIC)
    b.out(b.lerp(nn, b.const3(0, 0, 1), ca, "RGB"), "", MP.MP_NORMAL)
    ndl = sun_terms(b)
    night = b.sat(b.mul(b.sub(b.const(0.15), ndl), b.const(3.0)))
    glowmask = b.lerp(b.const(1.0), night, b.scalar("NightGlowMask", 0.5))
    e = b.mul(b.mul(em, b.scalar("EmissiveStrength", 6.0), "RGB"), glowmask)
    e = b.mul(e, b.one_minus(ca))
    fres = b.e(unreal.MaterialExpressionFresnel, exponent=4.0, base_reflect_fraction=0.0)
    b.c(b.e(unreal.MaterialExpressionVertexNormalWS), "", fres, "Normal")
    day = b.sat(b.add(b.mul(ndl, b.const(1.2)), b.const(0.25)))
    rim = b.mul(b.mul(b.vector("SurfaceRimColor", (0.2, 0.8, 1.0)), b.scalar("SurfaceRimStrength", 0.5)), b.mul(fres, day))
    b.out(b.add(e, rim), "", MP.MP_EMISSIVE_COLOR)
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
    mat, kept = new_master("M_TD_Ground")
    if kept:
        return kept
    b = MB(mat)
    wp = b.e(unreal.MaterialExpressionWorldPosition)
    xy = b.mask(wp, True, True, False)
    tile = b.scalar("TileSize", 400.0)
    uva = b.div(xy, tile)
    uvb = b.add(b.div(xy, b.mul(tile, b.scalar("BScale", MACRO["b_scale"]))), b.const2(*MACRO["b_offset"]))
    import math as _m

    def rot_uv(angle, scale_name, scale):
        a = _m.radians(angle)
        u = b.dot(xy, b.const2(_m.cos(a), -_m.sin(a)))
        v = b.dot(xy, b.const2(_m.sin(a), _m.cos(a)))
        return b.div(b.append(u, v), b.mul(tile, b.scalar(scale_name, scale)))
    maps = (("BaseColorTex", "T_Ground_Verdance_BC", ST.SAMPLERTYPE_COLOR), ("NormalTex", "T_Ground_Verdance_N", ST.SAMPLERTYPE_NORMAL),
            ("ORMTex", "T_Ground_Verdance_ORM", ST.SAMPLERTYPE_MASKS), ("EmissiveTex", "T_Ground_Verdance_E", ST.SAMPLERTYPE_COLOR))
    A = {p: b.tex(p, t[d], s, uva) for p, d, s in maps}
    B = {p: b.tex(p, t[d], s, uvb) for p, d, s in maps}
    lumw = b.const3(0.2126, 0.7152, 0.0722)

    def macro(angle, sname, scale):
        n = b.tex("BaseColorTex", t["T_Ground_Verdance_BC"], ST.SAMPLERTYPE_COLOR, rot_uv(angle, sname, scale))
        try:
            n.set_editor_property("mip_value_mode", unreal.TextureMipValueMode.TMVM_MIP_LEVEL)
            n.set_editor_property("const_mip_value", MACRO["mip"])
        except Exception as e:
            problems.append("macro mip level: %s" % e)
        lum = b.dot(n, lumw, "RGB")
        return b.mul(b.sub(lum, b.scalar("MacroMean", 0.05)), b.scalar("MacroK", 10.0))   # ~ -1..1
    d1 = macro(MACRO["angle1"], "MacroScale1", MACRO["scale1"])
    d2 = macro(MACRO["angle2"], "MacroScale2", MACRO["scale2"])
    blend = b.sat(b.add(b.mul(d2, b.scalar("BlendContrast", MACRO["blend_contrast"])), b.const(0.5)))
    bc = b.lerp(A["BaseColorTex"], B["BaseColorTex"], blend, "RGB", "RGB")
    nn = b.lerp(A["NormalTex"], B["NormalTex"], blend, "RGB", "RGB")
    orm = b.lerp(A["ORMTex"], B["ORMTex"], blend, "RGB", "RGB")
    em = b.lerp(A["EmissiveTex"], B["EmissiveTex"], blend, "RGB", "RGB")
    factor = b.add(b.const(1.0), b.mul(d1, b.mul(b.scalar("MacroStrength", MACRO["strength"]), b.const(0.35))))
    tint = b.lerp(b.vector("MacroTintA", (0.8, 0.8, 0.8)), b.vector("MacroTintB", (1, 1, 1)),
                  b.sat(b.add(b.mul(d1, b.const(0.5)), b.const(0.5))))
    ao = b.mask(orm, True, False, False)
    aob = b.lerp(b.const(1.0), ao, b.scalar("AOToBaseColor", 1.0))
    base = b.mul(b.mul(b.mul(bc, factor), tint), aob)
    b.out(base, "", MP.MP_BASE_COLOR)
    b.out(b.mask(orm, False, True, False), "", MP.MP_ROUGHNESS)
    b.out(ao, "", MP.MP_AMBIENT_OCCLUSION)
    b.out(b.const(0.0), "", MP.MP_METALLIC)
    b.out(b.norm(nn), "", MP.MP_NORMAL)
    gm = b.sat(b.add(b.mul(d1, b.const(1.2)), b.const(0.6)))
    glow = b.lerp(b.const(1.0), gm, b.scalar("GlowMacro", MACRO["glow_macro"]))
    b.out(b.mul(b.mul(em, b.scalar("EmissiveStrength", 4.0)), glow), "", MP.MP_EMISSIVE_COLOR)
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
           scalars=dict(CloudOpacity=c["cloud_opacity"], EmissiveStrength=c["emissive"], SurfaceRimStrength=c["rim_strength"],
                        CloudPanSpeed=c["cloud_pan"], NightGlowMask=0.5),
           vectors=dict(CloudColor=c["cloud_color"], SurfaceRimColor=c["rim"], SunDirection=SUN_DIR),
           textures=dict(BaseColorTex=t["T_Planet_%s_BC" % p], NormalTex=t["T_Planet_%s_N" % p], ORMTex=t["T_Planet_%s_ORM" % p],
                         EmissiveTex=t["T_Planet_%s_E" % p], CloudTex=t["T_Planet_%s_Clouds" % p]))
        mi("MI_PlanetAtmo_" + p, m_atmo,
           scalars=dict(AtmoIntensity=c["atmo_intensity"], RimPower=c["rim_power"], EdgeSoftness=4.5, SunMaskAmount=0.7),
           vectors=dict(AtmoColor=c["atmo"], SunDirection=SUN_DIR))
    mi("MI_PlanetRing_Verdance", m_ring, scalars=dict(Brightness=1.2, Opacity=0.85), vectors=dict(RingTint=(1, 1, 1)),
       textures=dict(RingTex=t["T_PlanetRing_Verdance"]))
    for g, c in GROUND.items():
        with open(os.path.join(TEX, "T_Ground_%s_stats.json" % g)) as f:
            st = json.load(f)
        k = 1.0 / (2.0 * st["macro_std_rel"] * st["macro_mean_lum"])
        mi("MI_Ground_" + g, m_ground,
           scalars=dict(TileSize=400.0, MacroMean=st["macro_mean_lum"], MacroK=k, EmissiveStrength=c["emissive"],
                        MacroStrength=MACRO["strength"], BlendContrast=MACRO["blend_contrast"], GlowMacro=MACRO["glow_macro"],
                        MacroScale1=MACRO["scale1"], MacroScale2=MACRO["scale2"], BScale=MACRO["b_scale"], AOToBaseColor=1.0),
           vectors=dict(MacroTintA=c["tint_a"], MacroTintB=c["tint_b"]),
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

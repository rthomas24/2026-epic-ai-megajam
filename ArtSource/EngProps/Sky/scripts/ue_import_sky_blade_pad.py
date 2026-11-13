# Triad Drift - import the nebula sky, the energy blade and the landing pad into Unreal (headless).
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Sky\scripts\ue_import_sky_blade_pad.py
#
# Writes ONLY:
#   /Game/TriadDrift/Gameplay/Props/Sky/     T_TD_NebulaSky, SM_TD_SkySphere, M_TD_NebulaSky, MI_TD_NebulaSky
#   /Game/TriadDrift/Gameplay/Props/Blade/   SM_TD_BladeHilt (+ T_TD_BladeHilt_BC/N/ORM/E, MI_TD_BladeHilt),
#                                            SM_TD_BladeBeam, M_TD_BladeBeam, MI_TD_BladeBeam
#   /Game/TriadDrift/Gameplay/Props/Planet/  SM_LandingPad (+ T_LandingPad_BC/N/ORM/E, MI_LandingPad)
# Reads (never modifies) /Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured and the Interchange pipeline
# /Game/TriadDrift/Gameplay/Pipelines/PL_TD_StaticMesh. Materials are (re)built by deleting and recreating them,
# because editing a material that already has instances crashes the headless editor.

import glob
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
ENG = os.path.dirname(os.path.dirname(HERE))          # ArtSource/EngProps
MASTER_TEXTURED = "/Game/TriadDrift/Gameplay/Materials/M_TD_PropTextured"
PIPELINE = "/Game/TriadDrift/Gameplay/Pipelines/PL_TD_StaticMesh"
SKY = "/Game/TriadDrift/Gameplay/Props/Sky"
BLADE = "/Game/TriadDrift/Gameplay/Props/Blade"
PLANET = "/Game/TriadDrift/Gameplay/Props/Planet"
# Blender bakes emission at strength 2; the game's exposure needs more for glows to read (see Tools/td-import-props.py)
EMISSIVE_SCALE = 8.0

eas = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
problems = []


def log(msg):
    unreal.log("TDPY: " + msg)


def setp(obj, prop, value):
    try:
        obj.set_editor_property(prop, value)
    except Exception as err:
        problems.append("could not set %s.%s: %s" % (type(obj).__name__, prop, err))


def task(filename, dest):
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", filename)
    t.set_editor_property("destination_path", dest)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    return t


def ensure_dir(path):
    if not eas.does_directory_exist(path):
        eas.make_directory(path)


def import_texture(path, dest, kind):
    tools.import_asset_tasks([task(path, dest)])
    name = os.path.splitext(os.path.basename(path))[0]
    tex = eas.load_asset("%s/%s" % (dest, name))
    if tex is None:
        problems.append("texture not imported: " + path)
        return None
    if kind == "N":
        setp(tex, "compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        setp(tex, "srgb", False)
        setp(tex, "flip_green_channel", False)
    elif kind == "ORM":
        setp(tex, "compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        setp(tex, "srgb", False)
    elif kind == "SKY":
        setp(tex, "compression_settings", unreal.TextureCompressionSettings.TC_HDR_COMPRESSED)
        setp(tex, "srgb", False)
        setp(tex, "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        setp(tex, "lod_group", unreal.TextureGroup.TEXTUREGROUP_SKYBOX)
        setp(tex, "never_stream", True)
        setp(tex, "address_x", unreal.TextureAddress.TA_WRAP)
        setp(tex, "address_y", unreal.TextureAddress.TA_CLAMP)
    else:
        setp(tex, "srgb", True)
    eas.save_loaded_asset(tex, False)
    return tex


def import_mesh(fbx, dest, name):
    params = unreal.ImportAssetParameters()
    params.is_automated = True
    params.replace_existing = True
    if eas.does_asset_exist(PIPELINE):
        params.override_pipelines.append(unreal.SoftObjectPath("%s.%s" % (PIPELINE, PIPELINE.rsplit("/", 1)[-1])))
    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    mgr.import_asset(dest, unreal.InterchangeManager.create_source_data(fbx), params)
    mesh = eas.load_asset("%s/%s" % (dest, name))
    if mesh is None:
        problems.append("mesh not imported: " + fbx)
        return None
    try:
        unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem).set_generate_lightmap_uv(mesh, False)
    except Exception as err:
        problems.append("lightmap toggle %s: %s" % (name, err))
    return mesh


def fresh_material(dest, name):
    path = "%s/%s" % (dest, name)
    for mi_name in ("MI" + name[1:],):
        if eas.does_asset_exist("%s/%s" % (dest, mi_name)):
            eas.delete_asset("%s/%s" % (dest, mi_name))
    if eas.does_asset_exist(path):
        eas.delete_asset(path)
    return tools.create_asset(name, dest, unreal.Material, unreal.MaterialFactoryNew())


def make_mi(dest, name, parent, scalars=None, vectors=None, textures=None):
    path = "%s/%s" % (dest, name)
    mi = eas.load_asset(path) if eas.does_asset_exist(path) else tools.create_asset(
        name, dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    for k, v in (scalars or {}).items():
        mel.set_material_instance_scalar_parameter_value(mi, k, float(v))
    for k, v in (vectors or {}).items():
        mel.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(*v))
    for k, v in (textures or {}).items():
        mel.set_material_instance_texture_parameter_value(mi, k, v)
    eas.save_loaded_asset(mi, False)
    return mi


def try_compile(material):
    try:
        mel.recompile_material(material)
    except Exception:
        pass


# ------------------------------------------------------------------ sky
def do_sky():
    ensure_dir(SKY)
    src = os.path.join(ENG, "Sky")
    tex = import_texture(os.path.join(src, "T_TD_NebulaSky.exr"), SKY, "SKY")
    mesh = import_mesh(os.path.join(src, "SM_TD_SkySphere", "SM_TD_SkySphere.fbx"), SKY, "SM_TD_SkySphere")
    if tex is None or mesh is None:
        return
    m = fresh_material(SKY, "M_TD_NebulaSky")
    setp(m, "shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    setp(m, "blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    setp(m, "two_sided", False)
    setp(m, "is_sky", True)
    s = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -800, 0)
    s.set_editor_property("parameter_name", "SkyTex")
    s.set_editor_property("texture", tex)
    s.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    tint = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -800, 300)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    bright = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -800, 450)
    bright.set_editor_property("parameter_name", "Brightness")
    bright.set_editor_property("default_value", 1.0)
    m1 = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -450, 100)
    m2 = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -250, 200)
    ok = mel.connect_material_expressions(s, "RGB", m1, "A")
    ok &= mel.connect_material_expressions(tint, "", m1, "B")
    ok &= mel.connect_material_expressions(m1, "", m2, "A")
    ok &= mel.connect_material_expressions(bright, "", m2, "B")
    ok &= mel.connect_material_property(m2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if not ok:
        problems.append("M_TD_NebulaSky wiring failed")
    try_compile(m)
    eas.save_loaded_asset(m, False)
    mi = make_mi(SKY, "MI_TD_NebulaSky", m, scalars={"Brightness": 1.0}, vectors={"Tint": (1.0, 1.0, 1.0, 1.0)},
                 textures={"SkyTex": tex})
    mesh.set_material(0, mi)
    eas.save_loaded_asset(mesh, False)
    b = mesh.get_bounds()
    log("sky: texture %dx%d, mesh sphere radius %.1f cm, %d material slot(s), is_sky=%s" % (
        tex.blueprint_get_size_x(), tex.blueprint_get_size_y(), b.sphere_radius, len(mesh.static_materials),
        m.get_editor_property("is_sky")))


# ------------------------------------------------------------------ baked props (hilt, pad)
def do_baked(folder, asset, dest):
    ensure_dir(dest)
    short = asset[3:]
    texs = {}
    for p in sorted(glob.glob(os.path.join(folder, "T_%s_*.png" % short))):
        kind = os.path.splitext(p)[0].rsplit("_", 1)[-1]
        t = import_texture(p, dest, kind)
        if t is not None:
            texs[kind] = t
    mesh = import_mesh(os.path.join(folder, asset + ".fbx"), dest, asset)
    if mesh is None:
        return None
    with open(os.path.join(folder, asset + ".manifest.json"), encoding="utf-8") as fh:
        man = json.load(fh)
    strength = float(man.get("emission_strength") or 2.0)
    master = eas.load_asset(MASTER_TEXTURED)
    if master is None:
        problems.append("missing " + MASTER_TEXTURED)
        return mesh
    tp = {k: texs[s] for k, s in (("BaseColorTex", "BC"), ("ORMTex", "ORM"), ("NormalTex", "N"), ("EmissiveTex", "E"))
          if s in texs}
    mi = make_mi(dest, "MI_" + short, master, scalars={"EmissiveStrength": strength * EMISSIVE_SCALE if "E" in texs else 0.0,
                                                       "SelfIllum": 0.0}, textures=tp)
    for i in range(len(mesh.static_materials)):
        mesh.set_material(i, mi)
    eas.save_loaded_asset(mesh, False)
    sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    b = mesh.get_bounds()
    if asset == "SM_TD_BladeHilt" and mesh.find_socket("BladeEmitter") is None:
        # Interchange drops the FBX SOCKET_ empty: add the emitter socket by hand (manifest: X = +19.3 cm)
        try:
            sk = unreal.StaticMeshSocket(mesh)
            sk.set_editor_property("socket_name", "BladeEmitter")
            sk.set_editor_property("relative_location", unreal.Vector(19.3, 0.0, 0.0))
            mesh.add_socket(sk)
            eas.save_loaded_asset(mesh, False)
        except Exception as err:
            problems.append("could not add BladeEmitter socket: %s" % err)
    sockets = []
    for nm in ("BladeEmitter", "SOCKET_BladeEmitter"):
        sk = mesh.find_socket(nm)
        if sk is not None:
            sockets.append("%s @ %s" % (nm, sk.get_editor_property("relative_location")))
    log("%s: textures %s, slots %d, hulls %d, box extent %s, origin %s, sockets %s" % (
        asset, sorted(texs), len(mesh.static_materials), sub.get_convex_collision_count(mesh),
        b.box_extent, b.origin, sockets))
    return mesh


def do_beam():
    folder = os.path.join(ENG, "Blade", "SM_TD_BladeBeam")
    mesh = import_mesh(os.path.join(folder, "SM_TD_BladeBeam.fbx"), BLADE, "SM_TD_BladeBeam")
    if mesh is None:
        return
    m = fresh_material(BLADE, "M_TD_BladeBeam")
    setp(m, "shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    setp(m, "blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    setp(m, "two_sided", False)
    X = mel.create_material_expression
    E = unreal
    fres = X(m, E.MaterialExpressionFresnel, -1100, 0)
    setp(fres, "exponent", 2.0)
    setp(fres, "base_reflect_fraction", 0.0)
    facing = X(m, E.MaterialExpressionOneMinus, -900, 0)
    sharp = X(m, E.MaterialExpressionScalarParameter, -1100, 200)
    sharp.set_editor_property("parameter_name", "CoreSharpness")
    sharp.set_editor_property("default_value", 2.5)
    core = X(m, E.MaterialExpressionPower, -700, 0)
    color = X(m, E.MaterialExpressionVectorParameter, -900, -300)
    color.set_editor_property("parameter_name", "BladeColor")
    color.set_editor_property("default_value", unreal.LinearColor(0.1, 0.65, 1.0, 1.0))
    white = X(m, E.MaterialExpressionConstant3Vector, -900, -150)
    white.set_editor_property("constant", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    lerp = X(m, E.MaterialExpressionLinearInterpolate, -500, -200)
    edge = X(m, E.MaterialExpressionScalarParameter, -900, 350)
    edge.set_editor_property("parameter_name", "EdgeBrightness")
    edge.set_editor_property("default_value", 0.3)
    one = X(m, E.MaterialExpressionConstant, -900, 450)
    one.set_editor_property("r", 1.0)
    blerp = X(m, E.MaterialExpressionLinearInterpolate, -500, 300)
    uv = X(m, E.MaterialExpressionTextureCoordinate, -1100, 600)
    mask = X(m, E.MaterialExpressionComponentMask, -900, 600)
    setp(mask, "r", True)
    setp(mask, "g", False)
    setp(mask, "b", False)
    setp(mask, "a", False)
    inv_u = X(m, E.MaterialExpressionOneMinus, -700, 600)
    tipk = X(m, E.MaterialExpressionConstant, -700, 700)
    tipk.set_editor_property("r", 7.0)
    tipm = X(m, E.MaterialExpressionMultiply, -550, 620)
    tip = X(m, E.MaterialExpressionSaturate, -400, 620)
    inten = X(m, E.MaterialExpressionScalarParameter, -500, 800)
    inten.set_editor_property("parameter_name", "Intensity")
    inten.set_editor_property("default_value", 12.0)
    a1 = X(m, E.MaterialExpressionMultiply, -250, -100)
    a2 = X(m, E.MaterialExpressionMultiply, -150, 300)
    a3 = X(m, E.MaterialExpressionMultiply, -50, 100)
    C = mel.connect_material_expressions
    ok = C(fres, "", facing, "")
    ok &= C(facing, "", core, "Base")
    ok &= C(sharp, "", core, "Exp")
    ok &= C(color, "", lerp, "A")
    ok &= C(white, "", lerp, "B")
    ok &= C(core, "", lerp, "Alpha")
    ok &= C(edge, "", blerp, "A")
    ok &= C(one, "", blerp, "B")
    ok &= C(facing, "", blerp, "Alpha")
    ok &= C(uv, "", mask, "")
    ok &= C(mask, "", inv_u, "")
    ok &= C(inv_u, "", tipm, "A")
    ok &= C(tipk, "", tipm, "B")
    ok &= C(tipm, "", tip, "")
    ok &= C(lerp, "", a1, "A")
    ok &= C(blerp, "", a1, "B")
    ok &= C(tip, "", a2, "A")
    ok &= C(inten, "", a2, "B")
    ok &= C(a1, "", a3, "A")
    ok &= C(a2, "", a3, "B")
    ok &= mel.connect_material_property(a3, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    if not ok:
        problems.append("M_TD_BladeBeam wiring failed")
    try_compile(m)
    eas.save_loaded_asset(m, False)
    mi = make_mi(BLADE, "MI_TD_BladeBeam", m, scalars={"Intensity": 12.0, "CoreSharpness": 2.5, "EdgeBrightness": 0.3},
                 vectors={"BladeColor": (0.1, 0.65, 1.0, 1.0)})
    mesh.set_material(0, mi)
    try:
        body = mesh.get_editor_property("body_setup")
        if body is not None:
            body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
    except Exception as err:
        problems.append("beam collision flag: %s" % err)
    eas.save_loaded_asset(mesh, False)
    b = mesh.get_bounds()
    log("SM_TD_BladeBeam: box extent %s, origin %s, slots %d" % (b.box_extent, b.origin, len(mesh.static_materials)))


def main():
    which = os.environ.get("TD_IMPORT", "sky,blade,pad").split(",")
    if "sky" in which:
        do_sky()
    if "blade" in which:
        do_baked(os.path.join(ENG, "Blade", "SM_TD_BladeHilt"), "SM_TD_BladeHilt", BLADE)
        do_beam()
    if "pad" in which:
        do_baked(os.path.join(ENG, "Pad", "SM_LandingPad"), "SM_LandingPad", PLANET)
    # materials the FBX import created from the Blender preview materials: unused once the MIs are assigned
    for stray in ("%s/M_NebulaSky_BlenderPreview" % SKY, "%s/M_BladeBeam_BlenderPreview" % BLADE,
                  "%s/M_TD_BladeHilt" % BLADE, "%s/M_LandingPad" % PLANET):
        if eas.does_asset_exist(stray):
            refs = eas.find_package_referencers_for_asset(stray, False)
            if refs:
                problems.append("stray %s still referenced by %s" % (stray, refs))
            elif eas.delete_asset(stray):
                log("deleted unused import material " + stray)
    for folder in (SKY, BLADE, PLANET):
        if eas.does_directory_exist(folder):
            log("%s: %s" % (folder, sorted(a.split(".")[-1] for a in eas.list_assets(folder, recursive=False))))
    for p in problems:
        log("PROBLEM: " + p)
    if not problems:
        log("OK")
    else:
        log("finished with %d problem(s)" % len(problems))


main()

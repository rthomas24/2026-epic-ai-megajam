# Triad Drift - import the scatter kit (ArtSource/EngProps/Scatter) into /Game/TriadDrift/Gameplay/Props/Scatter.
# Run headless:  powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Scatter\scripts\ue_import_scatter.py
#
# Uses the blender-unreal-export skill's importer (Interchange, the project's saved pipeline, UCX collision).
# Adds three small master materials and their instances, all inside Props/Scatter:
#   M_TD_ScatterFoliage  two-sided, lit; colour and glow from the 512 px atlas; Tint, EmissiveStrength, SelfIllum
#   M_TD_ScatterCanopy   the same, masked: dithers OUT nearer than NearFadeStart + NearFadeLength (far-ring clumps),
#                        mottled and glinting from the terrain detail mask projected down in world space
#   M_TD_ScatterRock     boulders: the terrain's own rock set (T_Terrain_Rock_M / _N) on the mesh UVs, Tint, and a
#                        TopTint on upward faces (moss on Verdance, ash on Cinder). No texture of its own.
# Idempotent: re-running replaces meshes, textures and instances. A master that already exists is left alone
# (editing a material that has instances crashes the headless editor): delete its .uasset to rebuild it.
# Nothing outside /Game/TriadDrift/Gameplay/Props/Scatter is written.

import glob
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
KIT = os.path.dirname(HERE)
REPO = os.path.abspath(os.path.join(KIT, "..", "..", ".."))
SKILL_SCRIPT = os.path.join(REPO, ".claude", "skills", "blender-unreal-export", "scripts", "ue_import_assets.py")
DEST = "/Game/TriadDrift/Gameplay/Props/Scatter"
PIPELINE = "/Game/TriadDrift/Gameplay/Pipelines/PL_TD_StaticMesh"
ROCK_M = "/Game/TriadDrift/Gameplay/Props/Planets/T_Terrain_Rock_M"
ROCK_N = "/Game/TriadDrift/Gameplay/Props/Planets/T_Terrain_Rock_N"
DETAIL_M = "/Game/TriadDrift/Gameplay/Props/Planets/T_Terrain_Detail_M"

eas = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
MP = unreal.MaterialProperty
ST = unreal.MaterialSamplerType


def log(message):
    unreal.log("TDPY: " + str(message))


def load_skill():
    scope = {"__name__": "ue_import_assets"}
    with open(SKILL_SCRIPT, encoding="utf-8") as handle:
        exec(compile(handle.read(), SKILL_SCRIPT, "exec"), scope)
    scope["PIPELINE"] = PIPELINE
    if eas.does_asset_exist(PIPELINE):
        # The project's pipeline already has the right settings: use it as it is, without re-saving it.
        scope["ensure_pipeline"] = lambda report: unreal.SoftObjectPath("%s.%s" % (PIPELINE, PIPELINE.rsplit("/", 1)[-1]))
    return scope


class Graph:
    def __init__(self, name):
        self.path = "%s/%s" % (DEST, name)
        self.material = tools.create_asset(name, DEST, unreal.Material, unreal.MaterialFactoryNew())
        self.ok = self.material is not None
        self.y = -400
        if self.ok:
            for flag in ("used_with_instanced_static_meshes", "used_with_nanite"):
                try:
                    self.material.set_editor_property(flag, True)
                except Exception as error:
                    log("note: could not set %s (%s)" % (flag, error))

    def node(self, cls, **props):
        self.y += 140
        n = mel.create_material_expression(self.material, cls, -900, self.y)
        for k, v in props.items():
            n.set_editor_property(k, v)
        return n

    def scalar(self, name, default):
        return self.node(unreal.MaterialExpressionScalarParameter, parameter_name=name, default_value=default)

    def vector(self, name, rgb):
        return self.node(unreal.MaterialExpressionVectorParameter, parameter_name=name,
                         default_value=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))

    def texture(self, name, default_path, sampler):
        n = self.node(unreal.MaterialExpressionTextureSampleParameter2D, parameter_name=name)
        tex = eas.load_asset(default_path)
        if tex is not None:
            n.set_editor_property("texture", tex)
        n.set_editor_property("sampler_type", sampler)
        return n

    def link(self, a, a_out, b, b_in):
        if not mel.connect_material_expressions(a, a_out, b, b_in):
            log("FAILED link %s.%s -> %s.%s in %s" % (a.get_name(), a_out, b.get_name(), b_in, self.path))
            self.ok = False

    def out(self, a, a_out, prop):
        if not mel.connect_material_property(a, a_out, prop):
            log("FAILED output %s -> %s in %s" % (a.get_name(), prop, self.path))
            self.ok = False

    def binary(self, cls, a, b, a_out="", b_out=""):
        n = self.node(cls)
        self.link(a, a_out, n, "A")
        self.link(b, b_out, n, "B")
        return n

    def save(self):
        try:
            mel.recompile_material(self.material)
        except RuntimeError:
            log("note: no renderer here, shaders compile on first load")
        if not eas.save_asset(self.path, only_if_is_dirty=False):
            self.ok = False
        log(("built " if self.ok else "FAILED ") + self.path)
        return self.ok


def build_foliage_master(name, atlas_bc, atlas_e, near_fade):
    path = "%s/%s" % (DEST, name)
    if eas.does_asset_exist(path):
        log("kept existing " + path)
        return True
    g = Graph(name)
    if not g.ok:
        return False
    m = g.material
    m.set_editor_property("two_sided", True)
    base = g.texture("BaseColorTex", atlas_bc, ST.SAMPLERTYPE_COLOR)
    emis = g.texture("EmissiveTex", atlas_e, ST.SAMPLERTYPE_COLOR)
    tint = g.vector("Tint", (1.0, 1.0, 1.0))
    colour = g.binary(unreal.MaterialExpressionMultiply, base, tint, "RGB", "")
    sparkle = None
    if near_fade:
        # Canopy only: the crowns are a few hundred flat triangles, so the leaf-scale breakup comes from the
        # terrain's detail mask projected straight down in world space (R = mottling, B = sparse glints).
        plan = g.node(unreal.MaterialExpressionComponentMask, r=True, g=True, b=False, a=False)
        g.link(g.node(unreal.MaterialExpressionWorldPosition), "", plan, "")
        uv = g.binary(unreal.MaterialExpressionDivide, plan, g.scalar("MottleScale", 2500.0))
        detail = g.texture("MottleTex", DETAIL_M, ST.SAMPLERTYPE_MASKS)
        g.link(uv, "", detail, "UVs")
        shade = g.node(unreal.MaterialExpressionLinearInterpolate)
        g.link(g.scalar("MottleDark", 0.35), "", shade, "A")
        g.link(g.scalar("MottleBright", 1.5), "", shade, "B")
        g.link(detail, "R", shade, "Alpha")
        colour = g.binary(unreal.MaterialExpressionMultiply, colour, shade)
        glint = g.binary(unreal.MaterialExpressionMultiply, detail, g.scalar("SparkleStrength", 2.0), "B", "")
        sparkle = g.binary(unreal.MaterialExpressionMultiply, colour, glint)
    g.out(colour, "", MP.MP_BASE_COLOR)
    glow = g.binary(unreal.MaterialExpressionMultiply, emis, g.scalar("EmissiveStrength", 4.0), "RGB", "")
    self_lit = g.binary(unreal.MaterialExpressionMultiply, colour, g.scalar("SelfIllum", 0.0))
    total = g.binary(unreal.MaterialExpressionAdd, glow, self_lit)
    if sparkle is not None:
        total = g.binary(unreal.MaterialExpressionAdd, total, sparkle)
    g.out(total, "", MP.MP_EMISSIVE_COLOR)
    g.out(g.scalar("Roughness", 0.7), "", MP.MP_ROUGHNESS)
    g.out(g.scalar("Specular", 0.2), "", MP.MP_SPECULAR)
    # Two-sided blades: cancel the engine's back-face normal flip, so both sides are lit by the authored normal.
    flat = g.node(unreal.MaterialExpressionConstant3Vector, constant=unreal.LinearColor(0.0, 0.0, 1.0, 0.0))
    sign = g.node(unreal.MaterialExpressionTwoSidedSign)
    g.out(g.binary(unreal.MaterialExpressionMultiply, flat, sign), "", MP.MP_NORMAL)
    if near_fade:
        m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
        m.set_editor_property("dither_opacity_mask", True)
        m.set_editor_property("two_sided", False)
        world = g.node(unreal.MaterialExpressionWorldPosition)
        camera = g.node(unreal.MaterialExpressionCameraPositionWS)
        distance = g.binary(unreal.MaterialExpressionDistance, world, camera)
        past = g.binary(unreal.MaterialExpressionSubtract, distance, g.scalar("NearFadeStart", 70000.0))
        ratio = g.binary(unreal.MaterialExpressionDivide, past, g.scalar("NearFadeLength", 10000.0))
        sat = g.node(unreal.MaterialExpressionSaturate)
        g.link(ratio, "", sat, "")
        g.out(sat, "", MP.MP_OPACITY_MASK)
    return g.save()


def build_rock_master(name):
    path = "%s/%s" % (DEST, name)
    if eas.does_asset_exist(path):
        log("kept existing " + path)
        return True
    if not (eas.does_asset_exist(ROCK_M) and eas.does_asset_exist(ROCK_N)):
        log("FAILED: the terrain rock textures are missing (%s)" % ROCK_M)
        return False
    g = Graph(name)
    if not g.ok:
        return False
    uv = g.binary(unreal.MaterialExpressionMultiply, g.node(unreal.MaterialExpressionTextureCoordinate), g.scalar("UVScale", 1.0))
    masks = g.texture("RockMaskTex", ROCK_M, ST.SAMPLERTYPE_MASKS)        # R albedo luminance, G roughness, B cavity
    normal = g.texture("RockNormalTex", ROCK_N, ST.SAMPLERTYPE_NORMAL)
    g.link(uv, "", masks, "UVs")
    g.link(uv, "", normal, "UVs")
    # colour = Tint * (dark .. bright by the luminance mask) * cavity, with TopTint on faces that look up
    lum = g.node(unreal.MaterialExpressionLinearInterpolate, const_a=0.45, const_b=1.45)
    g.link(masks, "R", lum, "Alpha")
    cav = g.node(unreal.MaterialExpressionLinearInterpolate, const_a=0.55, const_b=1.0)
    g.link(masks, "B", cav, "Alpha")
    shade = g.binary(unreal.MaterialExpressionMultiply, lum, cav)
    rock = g.binary(unreal.MaterialExpressionMultiply, g.vector("Tint", (0.16, 0.18, 0.19)), shade)
    top = g.binary(unreal.MaterialExpressionMultiply, g.vector("TopTint", (0.03, 0.14, 0.09)), cav)
    up = g.node(unreal.MaterialExpressionComponentMask, r=False, g=False, b=True, a=False)
    g.link(g.node(unreal.MaterialExpressionVertexNormalWS), "", up, "")
    # the mask breaks the moss line up: it creeps further down where the rock is bright
    edge = g.binary(unreal.MaterialExpressionAdd, up, masks, "", "R")
    past = g.binary(unreal.MaterialExpressionSubtract, edge, g.scalar("TopStart", 1.05))
    sharp = g.binary(unreal.MaterialExpressionMultiply, past, g.scalar("TopSharpness", 3.0))
    sat = g.node(unreal.MaterialExpressionSaturate)
    g.link(sharp, "", sat, "")
    amount = g.binary(unreal.MaterialExpressionMultiply, sat, g.scalar("TopAmount", 0.8))
    mix = g.node(unreal.MaterialExpressionLinearInterpolate)
    g.link(rock, "", mix, "A")
    g.link(top, "", mix, "B")
    g.link(amount, "", mix, "Alpha")
    g.out(mix, "", MP.MP_BASE_COLOR)
    rough = g.node(unreal.MaterialExpressionLinearInterpolate, const_a=0.72, const_b=0.96)
    g.link(masks, "G", rough, "Alpha")
    g.out(rough, "", MP.MP_ROUGHNESS)
    g.out(masks, "B", MP.MP_AMBIENT_OCCLUSION)
    g.out(normal, "RGB", MP.MP_NORMAL)
    g.out(g.scalar("Specular", 0.3), "", MP.MP_SPECULAR)
    return g.save()


def instance(name, parent, scalars=None, vectors=None, textures=None):
    path = "%s/%s" % (DEST, name)
    mi = eas.load_asset(path) if eas.does_asset_exist(path) else tools.create_asset(
        name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, eas.load_asset("%s/%s" % (DEST, parent)))
    for k, v in (scalars or {}).items():
        mel.set_material_instance_scalar_parameter_value(mi, k, float(v))
    for k, v in (vectors or {}).items():
        mel.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
    for k, v in (textures or {}).items():
        mel.set_material_instance_texture_parameter_value(mi, k, v)
    eas.save_loaded_asset(mi)
    return mi


# The look, tuned in the running game (Verdance, on foot and from 300 m). Edit here and re-run.
INSTANCES = [
    ("MI_Scatter_Foliage", "M_TD_ScatterFoliage", {"EmissiveStrength": 3.0, "SelfIllum": 0.25, "Roughness": 0.65}, {"Tint": (1.0, 1.0, 1.0)}),
    ("MI_Scatter_Canopy", "M_TD_ScatterCanopy", {"EmissiveStrength": 1.0, "SelfIllum": 0.06, "Roughness": 0.7,
                                                   "NearFadeStart": 70000.0, "NearFadeLength": 10000.0}, {"Tint": (1.0, 1.0, 1.0)}),
    # the same look with the near fade switched off (a clump that must stay visible up close)
    ("MI_Scatter_Canopy_NoFade", "M_TD_ScatterCanopy", {"EmissiveStrength": 1.0, "SelfIllum": 0.06, "Roughness": 0.7,
                                                          "NearFadeStart": -1000000.0, "NearFadeLength": 10000.0}, {"Tint": (1.0, 1.0, 1.0)}),
    ("MI_Scatter_Char", "M_TD_ScatterFoliage", {"EmissiveStrength": 14.0, "SelfIllum": 0.0, "Roughness": 0.9, "Specular": 0.1}, {"Tint": (1.0, 1.0, 1.0)}),
    ("MI_Scatter_Rock_Verdance", "M_TD_ScatterRock", {"TopAmount": 0.9, "TopStart": 0.9, "TopSharpness": 3.0},
     {"Tint": (0.20, 0.23, 0.25), "TopTint": (0.03, 0.20, 0.13)}),
    ("MI_Scatter_Rock_Cinder", "M_TD_ScatterRock", {"TopAmount": 0.6, "TopStart": 1.15, "TopSharpness": 2.5},
     {"Tint": (0.045, 0.04, 0.042), "TopTint": (0.13, 0.11, 0.10)}),
]


def main():
    skill = load_skill()
    if not eas.does_directory_exist(DEST):
        eas.make_directory(DEST)
    report = []
    files = [os.path.join(KIT, "textures", "T_Scatter_Atlas_%s.png" % s) for s in ("BC", "E")]
    textures = skill["import_textures"](files, DEST, report)
    if sorted(textures) != ["BC", "E"]:
        log("FAILED: atlas import: %s" % report)
        return
    for tex in textures.values():
        # Colour ramps sit side by side in the atlas: mips would blend one plant's colour into its neighbour's.
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        tex.set_editor_property("never_stream", True)
        eas.save_loaded_asset(tex)
    bc = "%s/T_Scatter_Atlas_BC" % DEST
    e = "%s/T_Scatter_Atlas_E" % DEST
    ok = build_foliage_master("M_TD_ScatterFoliage", bc, e, False)
    ok = build_foliage_master("M_TD_ScatterCanopy", bc, e, True) and ok
    ok = build_rock_master("M_TD_ScatterRock") and ok
    if not ok:
        log("FAILED: a master material did not build")
        return
    made = {}
    for name, parent, scalars, vectors in INSTANCES:
        tex = {} if "Rock" in name else {"BaseColorTex": textures["BC"], "EmissiveTex": textures["E"]}
        made[name] = instance(name, parent, scalars, vectors, tex)

    sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    good = bad = 0
    for manifest_path in sorted(glob.glob(os.path.join(KIT, "SM_Scatter_*", "*.manifest.json"))):
        with open(manifest_path, encoding="utf-8") as handle:
            manifest = json.load(handle)
        name = manifest["asset"]
        fbx = os.path.join(os.path.dirname(manifest_path), name + ".fbx")
        notes = []
        how = skill["import_mesh"](fbx, DEST, notes)
        mesh = eas.load_asset("%s/%s" % (DEST, name))
        if mesh is None:
            log("FAILED %s: not found after import (%s) %s" % (name, how, notes))
            bad += 1
            continue
        try:
            sme.set_generate_lightmap_uv(mesh, False)
        except Exception as error:
            notes.append("lightmap toggle: %s" % error)
        settings = mesh.get_editor_property("nanite_settings")
        settings.enabled = bool(manifest.get("nanite_recommended"))
        mesh.set_editor_property("nanite_settings", settings)
        mi = made[manifest["scatter"]["material_instance"]]
        for i in range(len(mesh.static_materials)):
            mesh.set_material(i, mi)
        want = 1 if manifest["scatter"]["collision"] == "ucx" else 0
        if want == 0:
            # With no UCX_ in the file the importer adds a fallback hull: ground cover and canopy must have none.
            sme.remove_collisions(mesh)
        eas.save_loaded_asset(mesh)
        bounds = mesh.get_bounds()
        hulls = sme.get_convex_collision_count(mesh) + sme.get_simple_collision_count(mesh) * (1 if want == 0 else 0)
        if hulls != want:
            notes.append("expected %d collision hull(s), found %d" % (want, hulls))
            bad += 1
        else:
            good += 1
        log("%s: %s, %d tris, size %.0f x %.0f x %.0f cm, base z %.0f, nanite %s, hulls %d, %s%s" % (
            name, how, sme.get_number_triangles(mesh, 0) if hasattr(sme, "get_number_triangles") else manifest["tris"],
            bounds.box_extent.x * 2, bounds.box_extent.y * 2, bounds.box_extent.z * 2,
            bounds.origin.z - bounds.box_extent.z, settings.enabled, hulls, mi.get_name(),
            ("  NOTES: " + "; ".join(notes)) if notes else ""))
    # Interchange may leave the FBX's own placeholder materials behind: they are not used.
    for stray in ("M_Scatter_Atlas", "M_Scatter_Rock"):
        path = "%s/%s" % (DEST, stray)
        if eas.does_asset_exist(path):
            eas.delete_asset(path)
            log("removed unused import material " + path)
    log("imported %d mesh(es), %d with problems" % (good, bad))
    log("OK" if good > 0 and bad == 0 else "FAILED: see the lines above")


main()

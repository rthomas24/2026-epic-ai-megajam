# SPDX-License-Identifier: MIT
# s7 (environment builder, Nov 14): the far-tier tree clumps as leafy canopy, not smooth pads.
#   M_TD_TreeClump (lit, masked, two-sided, Nanite + instanced): per instance one of four crown colours (yellow-green,
#   mid green, deep teal, ~8 % magenta bloom crowns with a faint glow), per puff a hue jitter (vertex colour G), the baked
#   light in vertex colour R (bright tops, dark undersides), a world-space leafy normal (the moss set's normal at 2.5 m)
#   and value mottling; dithered out nearer than NearFadeStart where the mid ring's real trees take over.
#   MI_TreeClump_Verdance re-parented to it; SM_Scatter_Verd_TreeClump_01..04 re-imported with Nanite.
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngEnvironment\canopy\ue_import_clumps.py
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(REPO, "ArtSource", "EngProps", "Planets", "scripts"))
import ue_mat_builder  # noqa: E402

DEST = "/Game/TriadDrift/Gameplay/Props/Scatter"
PLANETS = "/Game/TriadDrift/Gameplay/Props/Planets"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
MP = unreal.MaterialProperty
ST = unreal.MaterialSamplerType
problems = []
CLUMPS = ["SM_Scatter_Verd_TreeClump_%02d" % i for i in (1, 2, 3, 4)]


def log(m):
    unreal.log("TDPY: " + str(m))


def build_material():
    path = DEST + "/M_TD_TreeClump"
    mat = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(
        "M_TD_TreeClump", DEST, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("tangent_space_normal", False)
    for flag in ("used_with_instanced_static_meshes", "used_with_nanite"):
        try:
            mat.set_editor_property(flag, True)
        except Exception as e:  # noqa: BLE001
            log("note: %s %s" % (flag, e))
    b = ue_mat_builder.MB(mat, problems)
    vc = b.e(unreal.MaterialExpressionVertexColor)
    light = b.mask(vc, True)
    puff = b.mask(vc, False, True)
    rnd = b.e(unreal.MaterialExpressionPerInstanceRandom)
    # crown colour: three greens picked by the instance random plus the puff's jitter
    v = b.frac(b.add(rnd, b.mul(puff, b.const(0.35))))
    yg = b.vector("CrownYellowGreen", (0.10, 0.22, 0.035))
    gr = b.vector("CrownGreen", (0.045, 0.16, 0.05))
    tl = b.vector("CrownTeal", (0.02, 0.11, 0.09))
    t1 = b.lerp(yg, gr, b.sat(b.mul(v, b.const(3.0))))
    tint = b.lerp(t1, tl, b.sat(b.mul(b.sub(v, b.const(0.5)), b.const(3.0))))
    bloom_w = b.sat(b.mul(b.sub(rnd, b.scalar("BloomShare", 0.92)), b.const(200.0)))   # ~8 % of the clumps
    bloom_w = b.mul(bloom_w, b.sat(b.mul(b.sub(puff, b.const(0.35)), b.const(4.0))))   # on some of their puffs
    bloom = b.vector("CrownBloom", (0.32, 0.06, 0.25))
    tint = b.lerp(tint, bloom, bloom_w)
    under = b.vector("UnderTint", (0.006, 0.025, 0.02))
    wp = b.e(unreal.MaterialExpressionWorldPosition)
    xy = b.mask(wp, True, True)
    noise = b.tex("Mottle", eal.load_asset(PLANETS + "/T_Ground_MacroNoise"), ST.SAMPLERTYPE_MASKS, b.div(xy, b.scalar("MottleScale", 2500.0)))
    val = b.add(b.const(0.7), b.mul(b.mask(noise, True), b.const(0.6)))
    shade = b.mul(light, light)
    base = b.mul(b.lerp(b.mask(under, True, True, True), b.mask(tint, True, True, True), shade), val)
    b.out(base, "", MP.MP_BASE_COLOR)
    b.out(b.scalar("Roughness", 0.8), "", MP.MP_ROUGHNESS)
    b.out(b.const(0.3), "", MP.MP_SPECULAR)
    b.out(b.mul(b.mask(bloom, True, True, True), b.mul(bloom_w, b.scalar("BloomGlow", 0.25))), "", MP.MP_EMISSIVE_COLOR)
    # leafy world-space normal: the moss normal projected from above at LeafScale, added to the vertex normal
    nrm = b.tex("LeafNormal", eal.load_asset(PLANETS + "/T_Terr_Verdance_Moss_N"), ST.SAMPLERTYPE_NORMAL, b.div(xy, b.scalar("LeafScale", 250.0)))
    nv = b.e(unreal.MaterialExpressionVertexNormalWS)
    pert = b.append(b.mul(b.mask(nrm, True, True), b.scalar("LeafNormalStrength", 1.4)), b.const(0.0))
    b.out(b.norm(b.add(nv, pert)), "", MP.MP_NORMAL)
    # near fade (the mid ring's real trees take over)
    cam = b.e(unreal.MaterialExpressionCameraPositionWS)
    dist = b.e(unreal.MaterialExpressionDistance)
    b.c(cam, "", dist, "A")
    b.c(wp, "", dist, "B")
    fade = b.sat(b.div(b.sub(dist, b.scalar("NearFadeStart", 36000.0)), b.scalar("NearFadeLength", 6000.0)))
    b.out(fade, "", MP.MP_OPACITY_MASK)
    mel.recompile_material(mat)
    mel.layout_material_expressions(mat)
    eal.save_asset(path, only_if_is_dirty=False)
    ipath = DEST + "/MI_TreeClump_Verdance"
    mi = eal.load_asset(ipath) if eal.does_asset_exist(ipath) else tools.create_asset(
        "MI_TreeClump_Verdance", DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, mat)
    mel.update_material_instance(mi)
    eal.save_asset(ipath, only_if_is_dirty=False)
    try:  # needs a renderer (td-python.ps1 runs -nullrhi: the graph is saved, the stats are skipped)
        log("M_TD_TreeClump instructions %s" % mel.get_statistics(mat).get_editor_property("num_pixel_shader_instructions"))
    except Exception as e:  # noqa: BLE001
        log("stats unavailable: %s" % e)
    return mi


def main():
    mi = build_material()
    for name in CLUMPS:
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", os.path.join(HERE, "export", name + ".fbx").replace("\\", "/"))
        task.set_editor_property("destination_path", DEST)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", False)
        opts = unreal.FbxImportUI()
        opts.set_editor_property("import_mesh", True)
        opts.set_editor_property("import_materials", False)
        opts.set_editor_property("import_textures", False)
        opts.set_editor_property("import_as_skeletal", False)
        opts.static_mesh_import_data.set_editor_property("combine_meshes", True)
        opts.static_mesh_import_data.set_editor_property("vertex_color_import_option", unreal.VertexColorImportOption.REPLACE)
        opts.static_mesh_import_data.set_editor_property("auto_generate_collision", False)
        task.set_editor_property("options", opts)
        tools.import_asset_tasks([task])
        sm = eal.load_asset("%s/%s" % (DEST, name))
        if sm is None:
            problems.append("import " + name)
            continue
        sm.set_material(0, mi)
        ns = sm.get_editor_property("nanite_settings")
        ns.set_editor_property("enabled", True)
        sm.set_editor_property("nanite_settings", ns)
        b = sm.get_bounds()
        log("%s: %d tris, extent %s" % (name, sm.get_num_triangles(0), b.box_extent))
        eal.save_asset("%s/%s" % (DEST, name), only_if_is_dirty=False)
    for p in problems:
        log("PROBLEM: " + p)
    log("FAILED (%d)" % len(problems) if problems else "OK")


main()

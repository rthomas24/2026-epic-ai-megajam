# SPDX-License-Identifier: MIT
# Import the far-tier canopy stands (build_canopy.py) and build their material (environment builder, Nov 14).
#   /Game/TriadDrift/Gameplay/Props/Scatter/SM_Scatter_Verd_CanopyStand_01..03
#   M_TD_CanopyStand (lit, masked): colour from the baked vertex light (dark concave underside, bright crown top),
#   world-space mottling, dissolved out nearer than NearFadeStart where the real trees take over; MI_CanopyStand_Verdance.
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngEnvironment\canopy\ue_import_canopy.py
import os
import sys

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(REPO, "ArtSource", "EngProps", "Planets", "scripts"))
import ue_mat_builder  # noqa: E402

DEST = "/Game/TriadDrift/Gameplay/Props/Scatter"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
MP = unreal.MaterialProperty
ST = unreal.MaterialSamplerType
problems = []
NAMES = ["SM_Scatter_Verd_CanopyStand_%02d" % i for i in (1, 2, 3)]


def log(m):
    unreal.log("TDPY: " + str(m))


def build_material():
    path = DEST + "/M_TD_CanopyStand"
    mat = eal.load_asset(path) if eal.does_asset_exist(path) else tools.create_asset(
        "M_TD_CanopyStand", DEST, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(mat)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("two_sided", True)	# robust to the FBX winding; the crowns are closed anyway
    for flag in ("used_with_instanced_static_meshes",):
        mat.set_editor_property(flag, True)
    b = ue_mat_builder.MB(mat, problems)
    vc = b.e(unreal.MaterialExpressionVertexColor)
    light = b.mask(vc, True)
    dark = b.mask(b.vector("UnderTint", (0.006, 0.03, 0.03)), True, True, True)
    top = b.mask(b.vector("CrownTint", (0.07, 0.22, 0.09)), True, True, True)
    wp = b.e(unreal.MaterialExpressionWorldPosition)
    noise_tex = eal.load_asset("/Game/TriadDrift/Gameplay/Props/Planets/T_Ground_MacroNoise")
    n = b.tex("Mottle", noise_tex, ST.SAMPLERTYPE_MASKS, b.div(b.mask(wp, True, True), b.scalar("MottleScale", 4000.0)))
    hue = b.sat(b.mul(b.sub(b.mask(n, False, True), b.const(0.3)), b.const(2.0)))
    top2 = b.lerp(top, b.mask(b.vector("CrownTint2", (0.04, 0.18, 0.13)), True, True, True), hue)
    val = b.add(b.const(0.8), b.mul(b.mask(n, True), b.const(0.4)))
    base = b.mul(b.lerp(dark, top2, b.mul(light, light)), val)
    b.out(base, "", MP.MP_BASE_COLOR)
    b.out(b.scalar("Roughness", 0.85), "", MP.MP_ROUGHNESS)
    b.out(b.const(0.25), "", MP.MP_SPECULAR)
    # dither out near the camera (the mid ring's real trees take over)
    cam = b.e(unreal.MaterialExpressionCameraPositionWS)
    dist = b.e(unreal.MaterialExpressionDistance)
    b.c(cam, "", dist, "A")
    b.c(wp, "", dist, "B")
    fade = b.sat(b.div(b.sub(dist, b.scalar("NearFadeStart", 70000.0)), b.scalar("NearFadeLength", 12000.0)))
    # a world-space dissolve (no DitherTemporalAA node in editor Python): fine 3 m noise against the fade, clipped at 0.5
    b.out(fade, "", MP.MP_OPACITY_MASK)	# a clean cut at NearFadeStart + half the length (the dissolve variant drew nothing)
    mel.recompile_material(mat)
    mel.layout_material_expressions(mat)
    eal.save_asset(path, only_if_is_dirty=False)
    ipath = DEST + "/MI_CanopyStand_Verdance"
    mi = eal.load_asset(ipath) if eal.does_asset_exist(ipath) else tools.create_asset(
        "MI_CanopyStand_Verdance", DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, mat)
    mel.update_material_instance(mi)
    eal.save_asset(ipath, only_if_is_dirty=False)
    return mi


def main():
    mi = build_material()
    for name in NAMES:
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
        b = sm.get_bounds()
        log("%s: %d tris, extent %s" % (name, sm.get_num_triangles(0), b.box_extent))
        eal.save_asset("%s/%s" % (DEST, name), only_if_is_dirty=False)
    for p in problems:
        log("PROBLEM: " + p)
    log("FAILED (%d)" % len(problems) if problems else "OK")


main()

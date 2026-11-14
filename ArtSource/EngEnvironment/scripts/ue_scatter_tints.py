# SPDX-License-Identifier: MIT
# Scatter kit tints from the environment builder's ground palette (T4's rule: scatter, globe bake and terrain agree).
# - MI_Scatter_Canopy / _NoFade keep the kit's bioluminescent look (values from ue_import_scatter.py): the accent clumps.
# - MI_Scatter_Canopy_Forest (new, same parent and textures): the green forest stands seen from the air, a faint glow.
# - MI_Scatter_Rock_Verdance: greyer, mossier toward the terrain's cliff and moss tints.
#   powershell -File ArtSource\EngShips\Dominion\scripts\run_ue_rendered.ps1 -Script ArtSource\EngEnvironment\scripts\ue_scatter_tints.py
import unreal

DEST = "/Game/TriadDrift/Gameplay/Props/Scatter"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
KIT = {"EmissiveStrength": 1.0, "SelfIllum": 0.06, "Roughness": 0.7, "NearFadeStart": 70000.0, "NearFadeLength": 10000.0}
VALUES = {
    "MI_Scatter_Canopy": (KIT, {"Tint": (1.0, 1.0, 1.0)}),
    "MI_Scatter_Canopy_NoFade": (dict(KIT, NearFadeStart=-1000000.0), {"Tint": (1.0, 1.0, 1.0)}),
    "MI_Scatter_Canopy_Forest": (dict(KIT, EmissiveStrength=0.15, SelfIllum=0.02, Roughness=0.8), {"Tint": (0.28, 0.55, 0.38)}),
    "MI_Scatter_Rock_Verdance": ({}, {"Tint": (0.17, 0.19, 0.20), "TopTint": (0.035, 0.17, 0.12)}),
}
problems = []
src = eal.load_asset(DEST + "/MI_Scatter_Canopy")
for name, (scalars, vectors) in VALUES.items():
    path = "%s/%s" % (DEST, name)
    mi = eal.load_asset(path) if eal.does_asset_exist(path) else None
    if mi is None and name == "MI_Scatter_Canopy_Forest" and src is not None:
        mi = tools.create_asset(name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, src.get_editor_property("parent"))
        for t in ("BaseColorTex", "EmissiveTex"):
            tex = mel.get_material_instance_texture_parameter_value(src, t)
            if tex is None or not mel.set_material_instance_texture_parameter_value(mi, t, tex):
                problems.append("%s.%s" % (name, t))
    if mi is None:
        problems.append("missing " + name)
        continue
    for k, v in scalars.items():
        if not mel.set_material_instance_scalar_parameter_value(mi, k, float(v)):
            problems.append("%s.%s" % (name, k))
    for k, v in vectors.items():
        if not mel.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0)):
            problems.append("%s.%s" % (name, k))
    mel.update_material_instance(mi)
    eal.save_asset(path, only_if_is_dirty=False)
for p in problems:
    unreal.log("TDPY: PROBLEM " + p)
unreal.log("TDPY: " + ("FAILED" if problems else "OK"))

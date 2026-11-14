# SPDX-License-Identifier: MIT
# Check M_Kit_Decal's usage flags and list every decal instance on it (headless).
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Kit\scripts\ue_check_decal.py
import unreal

eas = unreal.EditorAssetLibrary
m = eas.load_asset("/Game/TriadDrift/Gameplay/Props/Kit/Materials/M_Kit_Decal")
unreal.log("TDPY: M_Kit_Decal used_with_instanced_static_meshes=%s used_with_nanite=%s blend=%s" % (
    m.get_editor_property("used_with_instanced_static_meshes"), m.get_editor_property("used_with_nanite"), m.get_editor_property("blend_mode")))
n = 0
for path in eas.list_assets("/Game/TriadDrift/Gameplay/Props/Kit", recursive=True, include_folder=False):
    a = eas.find_asset_data(path)
    if str(a.asset_class_path.asset_name) != "MaterialInstanceConstant":
        continue
    mi = eas.load_asset(path.split(".")[0])
    par = mi.get_editor_property("parent")
    if par and par.get_name() == "M_Kit_Decal":
        n += 1
        unreal.log("TDPY: decal MI %s" % mi.get_name())
unreal.log("TDPY: %d decal instances on M_Kit_Decal" % n)

# SPDX-License-Identifier: MIT
# Sets "Used with Instanced Static Meshes" (+ Nanite) on M_KitD_Trim in place (the same flags ensure_trim_grime in
# ue_import_kit.py now sets on every rebuild), then lists every material instance under the kit's folders with its root
# master and that master's flags, so nothing T6 / the planet scatter instances draws the default material.
#   powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Kit\scripts\ue_fix_usage.py
import unreal

eas, mel = unreal.EditorAssetLibrary, unreal.MaterialEditingLibrary
TRIM = "/Game/TriadDrift/Gameplay/Props/Kit/Materials/M_KitD_Trim"


def log(m):
    unreal.log("TDPY: " + m)


mat = eas.load_asset(TRIM)
for usage in (unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES, unreal.MaterialUsage.MATUSAGE_NANITE):
    try:
        mel.set_material_usage(mat, usage)
    except Exception as error:
        log("usage %s: %s" % (usage, error))
mat.set_editor_property("used_with_instanced_static_meshes", True)
mat.set_editor_property("used_with_nanite", True)
eas.save_asset(TRIM, only_if_is_dirty=False)
PILLAR = "/Game/TriadDrift/Gameplay/Props/Kit/Materials/M_Kit_LightPillar"
pm = eas.load_asset(PILLAR)
try:
    mel.set_material_usage(pm, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
except Exception as error:
    log("pillar usage: %s" % error)
pm.set_editor_property("used_with_instanced_static_meshes", True)
eas.save_asset(PILLAR, only_if_is_dirty=False)

roots = {}
for folder in ("/Game/TriadDrift/Gameplay/Props/Kit", "/Game/TriadDrift/Gameplay/Props/Space/SetPieces"):
    for path in eas.list_assets(folder, recursive=True):
        a = eas.load_asset(path.split(".")[0])
        if isinstance(a, unreal.MaterialInstanceConstant):
            r = a
            while isinstance(r, unreal.MaterialInstance) and r.get_editor_property("parent"):
                r = r.get_editor_property("parent")
            roots.setdefault(r.get_path_name(), []).append(a.get_name())
for rp, kids in sorted(roots.items()):
    r = unreal.load_object(None, rp)
    ism = r.get_editor_property("used_with_instanced_static_meshes") if isinstance(r, unreal.Material) else "?"
    nan = r.get_editor_property("used_with_nanite") if isinstance(r, unreal.Material) else "?"
    log("MASTER %s ism=%s nanite=%s instances=%d" % (rp, ism, nan, len(kids)))

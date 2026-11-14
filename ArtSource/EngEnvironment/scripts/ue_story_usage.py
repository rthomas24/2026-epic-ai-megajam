# SPDX-License-Identifier: MIT
# The Kit/Story decal meshes are scattered as instances (Verdance wreck scar): their parent material needs the
# instanced-static-mesh usage flag, or the game draws the default material (log: "missing usage flag
# InstancedStaticMeshes"). Sets only that flag (and Nanite's) on the parents of MI_Kit_ScorchLines / MI_Kit_DragMark.
import unreal

eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
bad = []
seen = set()
for mi_path in ("/Game/TriadDrift/Gameplay/Props/Kit/Story/MI_Kit_ScorchLines", "/Game/TriadDrift/Gameplay/Props/Kit/Story/MI_Kit_DragMark"):
    mi = eal.load_asset(mi_path)
    if mi is None:
        bad.append(mi_path)
        continue
    par = mi.get_editor_property("parent")
    while isinstance(par, unreal.MaterialInstance):
        par = par.get_editor_property("parent")
    if par is None or par.get_path_name() in seen:
        continue
    seen.add(par.get_path_name())
    par.set_editor_property("used_with_instanced_static_meshes", True)
    mel.recompile_material(par)
    eal.save_loaded_asset(par, only_if_is_dirty=False)
    unreal.log("TDPY: usage flag set on %s" % par.get_path_name())
unreal.log("TDPY: " + ("FAILED %s" % bad if bad else "OK"))

"""Nicholas authorized this isolated material exception in Lead chat Nov15 after reviewing the labeled mockup.
Create one isolated future-only copy. Never save or modify M_Pilot or player instances.
Run with the project's bounded headless Python runner, no full editor or cook.
"""
import unreal

SOURCE = "/Game/TriadDrift/Gameplay/Characters/Pilot/M_Pilot"
DEST = "/Game/TriadDrift/Gameplay/Characters/Pilot/M_FuturePilot"
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
if eal.does_asset_exist(DEST):
    raise RuntimeError("Preserve existing destination; reconcile ownership before editing")
source = eal.load_asset(SOURCE)
if not source:
    raise RuntimeError("Existing pilot master missing")
material = eal.duplicate_asset(SOURCE, DEST)
if not material:
    raise RuntimeError("Clone failed")
nodes = [x for x in mel.get_material_expressions(material)
         if x.get_class().get_name() == "MaterialExpressionCustom"]
color_nodes = [x for x in nodes if "float3 c = lerp(BC.rgb, BC.rgb * Tint.rgb, panel);" in x.get_editor_property("code")]
if len(color_nodes) != 1:
    raise RuntimeError("Unexpected material graph: do not save a guessed change")
node = color_nodes[0]
code = node.get_editor_property("code")
# The original panel mask omits middle luminances. Tint every non-accent surface,
# while retaining the original saturated visor/trim mask, normals and ORM texture.
code = code.replace("float3 c = lerp(BC.rgb, BC.rgb * Tint.rgb, panel);", "float3 c = BC.rgb * Tint.rgb;")
node.set_editor_property("code", code)
material.modify()
eal.save_asset(DEST, only_if_is_dirty=False)
unreal.log("TD.FutureMaterial: saved isolated M_FuturePilot; original M_Pilot untouched")

unreal.log('TDPY: OK')


"""Run inside UE 5.9 after compiling TriadDriftHUD. Writes ONLY the claimed UI folder.

UnrealEditor-Cmd.exe <project> -run=pythonscript -script=<this file>
    -EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities -unattended -nullrhi
Existing assets are validated and preserved; never overwritten by the generator.
"""
import unreal

DESTINATION = "/Game/TriadDrift/UI/TriadHUD"
assets = unreal.AssetToolsHelpers.get_asset_tools()
registry = unreal.AssetRegistryHelpers.get_asset_registry()
for name, native in (("WBP_SpaceHUD", "TDSpaceHUDWidget"),
                     ("WBP_ShipCard", "TDShipCardWidget")):
    parent = unreal.load_class(None, "/Script/TriadDriftHUD." + native)
    if not parent:
        raise RuntimeError("Compile/load TriadDriftHUD first: " + native)
    path = DESTINATION + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        blueprint = unreal.load_asset(path)
        generated = blueprint.generated_class()
        if not generated or not unreal.MathLibrary.class_is_child_of(generated, parent):
            raise RuntimeError("Existing asset has different ownership/parent; preserved: " + path)
        unreal.log("HUD preserved existing asset: " + path)
        continue
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    blueprint = assets.create_asset(name, DESTINATION, unreal.WidgetBlueprint, factory)
    if not blueprint:
        raise RuntimeError("Widget creation failed: " + path)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint):
        raise RuntimeError("Widget save failed: " + path)
    unreal.log("HUD_CREATED " + path)
unreal.log("HUD_WIDGET_GENERATION_COMPLETE")

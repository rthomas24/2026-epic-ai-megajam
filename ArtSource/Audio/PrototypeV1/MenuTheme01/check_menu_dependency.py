"""Prepared read-only style dependency check; execute only in a separately granted current-game host."""
import json

def check():
    import unreal
    style_path = '/Game/TriadDrift/Presentation/DA_UI_Style'
    wave_path = '/Game/TriadDrift/Audio/MenuTheme01/MUS_Menu_EtherialPulse_01'
    class_path = '/Game/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music'
    style = unreal.load_asset(style_path)
    assert style and style.get_class().get_path_name() == '/Script/unrealGameJam.TDUIStyle', 'Style missing or wrong class'
    sound = style.get_editor_property('menu_music')
    assert sound and sound.get_path_name() == wave_path+'.MUS_Menu_EtherialPulse_01', 'MenuMusic not assigned'
    assert abs(sound.get_editor_property('duration')-107.0) < .001 and not sound.get_editor_property('looping')
    assert sound.get_editor_property('sound_class_object').get_path_name() == class_path+'.SC_MenuTheme_Music'
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
    dependencies = {path: [str(d) for d in registry.get_dependencies(unreal.Name(path), options)] for path in (style_path,wave_path,class_path)}
    assert wave_path in dependencies[style_path], 'Style soft dependency absent'
    assert class_path in dependencies[wave_path], 'Music class dependency absent'
    assert not any('tdaudiov1' in ref.lower() for refs in dependencies.values() for ref in refs)
    print(json.dumps({'dependencies':dependencies,'stage':'Read-only reference validation only; cook and actual menu playback pending'},indent=2))

if globals().get('SPEC',{}).get('execute') is True:
    check()
else:
    print('Prepared only; no asset operations, writes or playback.')

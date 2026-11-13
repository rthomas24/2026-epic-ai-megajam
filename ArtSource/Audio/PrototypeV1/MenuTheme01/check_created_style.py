"""Prepared read-only style dependency check; execute only in a separately granted current-game host."""
import json
import hashlib
from pathlib import Path

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
    defaults = unreal.get_default_object(style.get_class())
    preserved = ('squad_colors','enemy_color','ability_icons','main_menu_widget_class','loading_widget_class','pause_widget_class')
    assert all(style.get_editor_property(n) == defaults.get_editor_property(n) for n in preserved), 'Unrelated style default changed'
    assert all(style.get_editor_property(n) is None for n in ('main_menu_widget_class','loading_widget_class','pause_widget_class'))
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous(['/Game/TriadDrift/Presentation','/Game/TriadDrift/Audio/MenuTheme01'], True)
    options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
    dependencies = {path: [str(d) for d in (registry.get_dependencies(unreal.Name(path), options) or [])] for path in (style_path,wave_path,class_path)}
    assert wave_path in dependencies[style_path], 'Style soft dependency absent'
    assert class_path in dependencies[wave_path], 'Music class dependency absent'
    assert not any('tdaudiov1' in ref.lower() for refs in dependencies.values() for ref in refs)
    result = {'dependencies':dependencies,'preserved_style_defaults':list(preserved),'duration_s':sound.get_editor_property('duration'),'looping':sound.get_editor_property('looping'),'stage':'Read-only reference validation only; cook and actual menu playback pending'}
    return result

def inspect_existing(spec):
    import unreal
    host = Path(unreal.Paths.project_dir()).resolve()
    assert host == Path(spec['project_dir']).resolve(), 'Wrong host'
    expected = {
        'Content/TriadDrift/Presentation/DA_UI_Style.uasset':'0a1fa00da9771737cc36e3f0db16ef746fb284b784d210c8537acdc9a2d07df0',
        'Content/TriadDrift/Audio/MenuTheme01/MUS_Menu_EtherialPulse_01.uasset':'6253253fe891d364d0180bacada62b70cb723a08ecfe3f20909406c40adb1042',
        'Content/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music.uasset':'813d76de263314464d0ad6f698a751bec6c99ae85d262601b1df85d80e0600c7'}
    def hashes():return {name:hashlib.sha256((host/name).read_bytes()).hexdigest() for name in expected}
    before = hashes()
    assert before == expected, 'Existing package hash changed'
    output = host/'Saved/AudioMusic/MenuTheme01/Reload01/validation.json'
    assert not output.exists(), 'Evidence exists; stop'
    result = {'before':before,'passed':False}
    try:
        result.update(check())
        result['passed'] = True
    except Exception as error:
        result['error'] = str(error)
        raise
    finally:
        result['after'] = hashes()
        result['packages_unchanged'] = result['after'] == before
        output.parent.mkdir(parents=True,exist_ok=True)
        output.write_text(json.dumps(result,indent=2)+'\n')
        assert result['packages_unchanged'], 'Read-only reload changed package bytes'
    print(json.dumps(result,indent=2))

if globals().get('SPEC',{}).get('execute') is True:
    inspect_existing(SPEC)
else:
    print('Prepared only; no Unreal calls, saves or playback.')

"""One explicitly granted current-game style creation; no import, playback, config or map edits."""
import hashlib
import json
from pathlib import Path

def create(spec):
    import unreal
    assert spec.get('execute') is True and spec['claim_commit'], 'Explicit style-creation grant required'
    host = Path(unreal.Paths.project_dir()).resolve()
    assert host == Path(spec['project_dir']).resolve(), 'Wrong current-game host'
    target = '/Game/TriadDrift/Presentation/DA_UI_Style'
    package = host/'Content/TriadDrift/Presentation/DA_UI_Style.uasset'
    assert not package.exists() and not unreal.EditorAssetLibrary.does_asset_exist(target), 'Existing style: stop, never overwrite'
    report = host/'Saved/AudioMusic/MenuTheme01/style_create_01.json'
    assert not report.exists(), 'Evidence already exists; no retry'
    cls = unreal.load_class(None, '/Script/unrealGameJam.TDUIStyle')
    assert cls, 'Compiled game class missing; do not use private audio host'
    defaults = unreal.get_default_object(cls)
    assert defaults.get_editor_property('menu_music') is None, 'Expected empty default MenuMusic'
    fields = ('squad_colors','enemy_color','ability_icons','main_menu_widget_class','loading_widget_class','pause_widget_class')
    original = {name: defaults.get_editor_property(name) for name in fields}
    for name in ('main_menu_widget_class','loading_widget_class','pause_widget_class'):
        assert original[name] is None, 'Unexpected widget default; request owner review'
    root = '/Game/TriadDrift/Audio/MenuTheme01/'
    hashes = {'MUS_Menu_EtherialPulse_01': '6253253fe891d364d0180bacada62b70cb723a08ecfe3f20909406c40adb1042',
              'SC_MenuTheme_Music': '813d76de263314464d0ad6f698a751bec6c99ae85d262601b1df85d80e0600c7'}
    files = {name: host/('Content/TriadDrift/Audio/MenuTheme01/'+name+'.uasset') for name in hashes}
    def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
    assert all(digest(files[n]) == h for n,h in hashes.items()), 'Portable package hash mismatch'
    config = (host/'Config/DefaultGame.ini').read_text(encoding='utf-8-sig')
    assert any('PrimaryAssetType="TDUIStyle"' in line and 'CookRule=AlwaysCook' in line and '/Game/TriadDrift/Presentation' in line for line in config.splitlines()), 'Style AlwaysCook scan missing'
    wave = unreal.load_asset(root+'MUS_Menu_EtherialPulse_01')
    music_class = unreal.load_asset(root+'SC_MenuTheme_Music')
    assert isinstance(wave, unreal.SoundWave) and isinstance(music_class, unreal.SoundClass)
    assert wave.get_editor_property('sound_class_object') == music_class
    assert abs(wave.get_editor_property('duration')-107.0)<.001 and not wave.get_editor_property('looping')
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', cls)
    style = unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_UI_Style', '/Game/TriadDrift/Presentation', cls, factory)
    assert style and style.get_class() == cls, 'Style creation failed'
    # This is the ONLY authored property. All inherited/default properties remain factory defaults.
    style.set_editor_property('menu_music', wave)
    assert all(style.get_editor_property(n) == v for n,v in original.items()), 'Unrelated defaults changed'
    assert unreal.EditorAssetLibrary.save_loaded_asset(style), 'Style save failed'
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True)
    dependencies = {p:[str(d) for d in registry.get_dependencies(unreal.Name(p),options)] for p in (target,root+'MUS_Menu_EtherialPulse_01',root+'SC_MenuTheme_Music')}
    assert root+'MUS_Menu_EtherialPulse_01' in dependencies[target]
    assert root+'SC_MenuTheme_Music' in dependencies[root+'MUS_Menu_EtherialPulse_01']
    assert not any('tdaudiov1' in d.lower() for deps in dependencies.values() for d in deps)
    assert all(digest(files[n]) == h for n,h in hashes.items()), 'Portable input changed'
    result = {'style':style.get_path_name(),'menu_music':wave.get_path_name(),'package_sha256':digest(package),
              'preserved_default_fields':list(fields),'dependencies':dependencies,'portable_packages_unchanged':True,
              'stage':'Style created/reference checks pass; reload, cook and actual menu playback pending'}
    report.parent.mkdir(parents=True,exist_ok=True)
    report.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))

if globals().get('SPEC',{}).get('execute') is True:
    create(SPEC)
else:
    print('Prepared only; no Unreal calls or asset writes. Await current-game host and explicit grant.')

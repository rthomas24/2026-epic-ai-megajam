"""Separate granted read-only reload after loop edit. No saves or playback."""
import hashlib,json
from pathlib import Path

if globals().get('SPEC',{}).get('execute') is True:
    import unreal
    host=Path(unreal.Paths.project_dir()).resolve()
    assert host==Path(SPEC['project_dir']).resolve()
    root=host/'Content/TriadDrift/Audio/MenuTheme01'
    package=root/'MUS_Menu_EtherialPulse_01.uasset'
    before=hashlib.sha256(package.read_bytes()).hexdigest()
    edited=json.loads((host/'Saved/AudioMusic/MenuTheme01/Loop01/loop_edit.json').read_text())
    assert before==edited['after_package_sha256']
    wave=unreal.load_asset('/Game/TriadDrift/Audio/MenuTheme01/MUS_Menu_EtherialPulse_01')
    assert wave.get_editor_property('looping') is True and abs(wave.get_editor_property('duration')-107)<.001
    assert wave.get_editor_property('pitch')==1 and wave.get_editor_property('volume')==1
    assert wave.get_editor_property('sound_class_object').get_path_name()=='/Game/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music.SC_MenuTheme_Music'
    style=unreal.load_asset('/Game/TriadDrift/Presentation/DA_UI_Style')
    assert style.get_editor_property('menu_music')==wave
    assert hashlib.sha256(package.read_bytes()).hexdigest()==before
    print(json.dumps({'looping':True,'duration_s':107,'unchanged_package_sha256':before,'style_reference_correct':True,'stage':'Read-only reload; actual loop/travel pending'}))
else:
    print('Prepared only; separate native reload grant required.')

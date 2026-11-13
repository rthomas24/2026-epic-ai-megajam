"""One granted native flag edit for Nick's approved107s track; source audio unchanged."""
import hashlib
import json
from pathlib import Path
import shutil

def enable(spec):
    import unreal
    host = Path(unreal.Paths.project_dir()).resolve()
    assert spec.get('execute') is True and host == Path(spec['project_dir']).resolve()
    root = host/'Content/TriadDrift'
    wave_file = root/'Audio/MenuTheme01/MUS_Menu_EtherialPulse_01.uasset'
    protected = {
        root/'Presentation/DA_UI_Style.uasset':'0a1fa00da9771737cc36e3f0db16ef746fb284b784d210c8537acdc9a2d07df0',
        root/'Audio/MenuTheme01/SC_MenuTheme_Music.uasset':'813d76de263314464d0ad6f698a751bec6c99ae85d262601b1df85d80e0600c7'}
    def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
    old = sha(wave_file)
    assert old == '6253253fe891d364d0180bacada62b70cb723a08ecfe3f20909406c40adb1042'
    assert all(sha(p)==h for p,h in protected.items())
    run = host/'Saved/AudioMusic/MenuTheme01/Loop01'
    backup = run/'original_nonlooping.uasset'
    assert not backup.exists() and not (run/'loop_edit.json').exists(), 'Preserve previous attempt; no retry'
    wave = unreal.load_asset('/Game/TriadDrift/Audio/MenuTheme01/MUS_Menu_EtherialPulse_01')
    assert isinstance(wave, unreal.SoundWave)
    assert not wave.get_editor_property('looping') and abs(wave.get_editor_property('duration')-107.0)<.001
    fields=('duration','volume','pitch','sound_class_object','virtualization_mode')
    before={name:wave.get_editor_property(name) for name in fields}
    run.mkdir(parents=True,exist_ok=True);shutil.copyfile(wave_file,backup);assert sha(backup)==old
    wave.set_editor_property('looping',True)
    assert wave.get_editor_property('looping') is True
    assert all(wave.get_editor_property(n)==v for n,v in before.items())
    assert unreal.EditorAssetLibrary.save_loaded_asset(wave)
    assert all(sha(p)==h for p,h in protected.items())
    result={'wave':wave.get_path_name(),'looping':True,'duration_s':wave.get_editor_property('duration'),
            'before_package_sha256':old,'after_package_sha256':sha(wave_file),'backup':str(backup),
            'other_native_inputs_unchanged':True,'source_audio_processing':'NONE; same107s source/tail fade',
            'stage':'Loop flag edited; actual wrap and travel tests pending',
            'approval':'Nick heard initial menu playback and said it sounds really good; requested restart at beginning after107s'}
    (run/'loop_edit.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))

if globals().get('SPEC',{}).get('execute') is True:
    enable(SPEC)
else:
    print('Prepared only; requires separate native loop-edit grant.')

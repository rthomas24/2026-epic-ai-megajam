"""New Drop38 Wave only. Explicit grant required; no shared-style/config/source edit."""
from pathlib import Path
import hashlib,json,wave
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()

def run(spec):
    manifest=json.loads(Path(spec['manifest']).read_text())
    source=Path(manifest['prepared_file']);assert sha(source)==manifest['sha256']
    with wave.open(str(source)) as w:
        assert (w.getnframes(),w.getframerate(),w.getnchannels(),w.getsampwidth())==(2736000,48000,2,2)
    if spec.get('execute') is not True:
        print('Source verified; no native import');return
    import unreal
    assert spec['claim_commit']
    host=Path(unreal.Paths.project_dir()).resolve();assert host==Path(spec['project_dir']).resolve()
    sound_path=manifest['asset_path']
    target=host/'Content'/(sound_path.removeprefix('/Game/')+'.uasset')
    assert not target.exists() and not unreal.EditorAssetLibrary.does_asset_exist(sound_path)
    original=host/'Content/TriadDrift/Audio/CombatMusic01/MUS_Combat_EtherealPulseRemix_01.uasset'
    assert sha(original)=='1110b70395c6ce3b7423245dfa435ba68be4baf31e4d93f4c579dbb8842117aa'
    class_file=host/'Content/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music.uasset'
    assert sha(class_file)=='813d76de263314464d0ad6f698a751bec6c99ae85d262601b1df85d80e0600c7'
    music_class=unreal.load_asset('/Game/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music')
    assert isinstance(music_class,unreal.SoundClass) and music_class.get_editor_property('properties').get_editor_property('is_music')
    out=host/'Saved/AudioMusic/CombatMusic01'/spec['run_id'];assert not out.exists()
    task=unreal.AssetImportTask()
    for key,value in {'filename':str(source),'destination_path':sound_path.rsplit('/',1)[0],
                      'destination_name':sound_path.rsplit('/',1)[1],'automated':True,'replace_existing':False,'save':False}.items():task.set_editor_property(key,value)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound=unreal.load_asset(sound_path);assert isinstance(sound,unreal.SoundWave)
    for key,value in {'sound_class_object':music_class,'looping':True,'volume':.5,'pitch':1.,'virtualization_mode':unreal.VirtualizationMode.PLAY_WHEN_SILENT}.items():sound.set_editor_property(key,value)
    assert abs(sound.get_editor_property('duration')-57)<.001
    assert sound.get_editor_property('num_channels')==2 and sound.get_editor_property('imported_sample_rate')==48000
    assert unreal.EditorAssetLibrary.save_loaded_asset(sound)
    assert sha(class_file)=='813d76de263314464d0ad6f698a751bec6c99ae85d262601b1df85d80e0600c7'
    assert sha(original)=='1110b70395c6ce3b7423245dfa435ba68be4baf31e4d93f4c579dbb8842117aa'
    out.mkdir(parents=True)
    result={'stage':'New Wave imported only; fresh reload/binding/runtime/listening pending','asset':sound.get_path_name(),
            'sha256':sha(target),'bytes':target.stat().st_size,'duration_s':sound.get_editor_property('duration'),
            'looping':True,'volume':.5,'class':music_class.get_path_name(),'source_preserved':True,'old_wave_preserved':True}
    (out/'import.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))

if globals().get('SPEC',{}):run(SPEC)
else:print('Prepared only; manifest, compatible host and fresh native grant required.')

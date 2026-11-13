"""Granted single non-looping Prelude38 import or fresh reload; never touches style."""
from pathlib import Path
import hashlib,json,wave
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()

def run(spec):
    import unreal
    assert spec.get('execute') is True and spec['claim_commit']
    host=Path(unreal.Paths.project_dir()).resolve();assert host==Path(spec['project_dir']).resolve()
    source=Path(spec['source']);assert sha(source)=='7359b155c01f49be385db974c388063fcc86f1130396deb777b4aa75c9d68d16'
    with wave.open(str(source)) as w:assert (w.getnframes(),w.getframerate(),w.getnchannels(),w.getsampwidth())==(1824000,48000,2,2)
    style=host/'Content/TriadDrift/Presentation/DA_UI_Style.uasset'
    cls_file=host/'Content/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music.uasset'
    loop_file=host/'Content/TriadDrift/Audio/CombatMusic01/MUS_Combat_EtherealPulseRemix_Drop38_01.uasset'
    old_file=host/'Content/TriadDrift/Audio/CombatMusic01/MUS_Combat_EtherealPulseRemix_01.uasset'
    expected={style:'5ce634e54470596acc70ed3ed403be802b47435eb301fe10c1729a895ec775a8',cls_file:'813d76de263314464d0ad6f698a751bec6c99ae85d262601b1df85d80e0600c7',loop_file:'d2166e8e1a311161560ee7dab3c6d775e62c63ac1f3a66a1a13379e074a4cae5',old_file:'1110b70395c6ce3b7423245dfa435ba68be4baf31e4d93f4c579dbb8842117aa'}
    assert all(sha(p)==h for p,h in expected.items())
    root='/Game/TriadDrift/Audio/CombatMusic01';name='MUS_Tutorial_EtherealPulse_Prelude38_01';path=root+'/'+name
    package=host/'Content/TriadDrift/Audio/CombatMusic01'/(name+'.uasset')
    out=host/'Saved/AudioMusic/CombatMusic01'/spec['run_id'];assert not out.exists()
    cls=unreal.load_asset('/Game/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music')
    assert isinstance(cls,unreal.SoundClass) and cls.get_editor_property('properties').get_editor_property('is_music')
    if spec['mode']=='import':
        assert not package.exists() and not unreal.EditorAssetLibrary.does_asset_exist(path)
        task=unreal.AssetImportTask()
        for key,value in {'filename':str(source),'destination_path':root,'destination_name':name,'automated':True,'replace_existing':False,'save':False}.items():task.set_editor_property(key,value)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);sound=unreal.load_asset(path)
        assert isinstance(sound,unreal.SoundWave)
        for key,value in {'sound_class_object':cls,'looping':False,'volume':.5,'pitch':1.,'virtualization_mode':unreal.VirtualizationMode.PLAY_WHEN_SILENT}.items():sound.set_editor_property(key,value)
        assert unreal.EditorAssetLibrary.save_loaded_asset(sound)
    else:
        assert spec['mode']=='reload' and sha(package)==spec['wave_sha256']
        sound=unreal.load_asset(path);assert isinstance(sound,unreal.SoundWave)
    assert sound.get_editor_property('sound_class_object')==cls
    assert sound.get_editor_property('looping') is False and abs(sound.get_editor_property('duration')-38)<.001
    assert sound.get_editor_property('volume')==.5 and sound.get_editor_property('pitch')==1
    assert sound.get_editor_property('num_channels')==2 and sound.get_editor_property('imported_sample_rate')==48000
    assert all(sha(p)==h for p,h in expected.items())
    if spec['mode']=='reload':assert sha(package)==spec['wave_sha256']
    out.mkdir(parents=True)
    result={'stage':'Native prelude only; real tutorial synchronization and final video pending','mode':spec['mode'],'asset':sound.get_path_name(),'sha256':sha(package),'bytes':package.stat().st_size,'duration_s':38,'looping':False,'volume':.5,'pitch':1,'music_class':cls.get_path_name(),'style_loop_original_class_unchanged':True}
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))

if globals().get('SPEC',{}).get('execute') is True:run(SPEC)
else:print('Prepared only. Explicit native grant required.')

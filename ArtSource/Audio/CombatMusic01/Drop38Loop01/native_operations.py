"""Granted isolated Drop38 binding/reload. All unrelated style properties preserved."""
from pathlib import Path
import hashlib,importlib.util,json,shutil
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
OLD='1110b70395c6ce3b7423245dfa435ba68be4baf31e4d93f4c579dbb8842117aa'
CLASS='813d76de263314464d0ad6f698a751bec6c99ae85d262601b1df85d80e0600c7'
BASE='6cfe3e6c3ed711b6e7421c82793180094703272966aa56b72b3e3e2b09d34822'

def run(spec):
    import unreal
    assert spec.get('execute') is True and spec['claim_commit']
    host=Path(unreal.Paths.project_dir()).resolve();assert host==Path(spec['project_dir']).resolve()
    run=host/'Saved/AudioMusic/CombatMusic01'/spec['run_id'];assert not run.exists()
    source=Path(spec['author_source']);assert sha(source)==spec['author_sha256']
    helper_path=Path(spec['helper']);assert sha(helper_path)=='aaa1cd4a5e5608264d3a7d25f59bc59a4bcb83028bf6878ae1a545a606832f10'
    loader=importlib.util.spec_from_file_location('helpers',helper_path);helper=importlib.util.module_from_spec(loader);loader.loader.exec_module(helper)
    target='/Game/TriadDrift/Audio/CombatMusic01/MUS_Combat_EtherealPulseRemix_Drop38_01'
    wave_file=host/'Content/TriadDrift/Audio/CombatMusic01/MUS_Combat_EtherealPulseRemix_Drop38_01.uasset'
    assert sha(wave_file)==spec['wave_sha256']
    original=host/'Content/TriadDrift/Audio/CombatMusic01/MUS_Combat_EtherealPulseRemix_01.uasset'
    class_file=host/'Content/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music.uasset'
    assert sha(original)==OLD and sha(class_file)==CLASS
    style_file=host/'Content/TriadDrift/Presentation/DA_UI_Style.uasset'
    style=unreal.load_asset('/Game/TriadDrift/Presentation/DA_UI_Style');assert style
    sound=unreal.load_asset(target);assert isinstance(sound,unreal.SoundWave)
    cls=unreal.load_asset('/Game/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music')
    assert sound.get_editor_property('sound_class_object')==cls
    assert cls.get_editor_property('properties').get_editor_property('is_music')
    assert sound.get_editor_property('looping') and abs(sound.get_editor_property('duration')-57)<.001
    assert sound.get_editor_property('volume')==.5 and sound.get_editor_property('pitch')==1
    assert sound.get_editor_property('num_channels')==2 and sound.get_editor_property('imported_sample_rate')==48000
    run.mkdir(parents=True)
    if spec['mode']=='bind':
        assert sha(style_file)==BASE
        previous=style.get_editor_property('combat_music');assert previous and previous.get_path_name()=='/Game/TriadDrift/Audio/CombatMusic01/MUS_Combat_EtherealPulseRemix_01.MUS_Combat_EtherealPulseRemix_01'
        shutil.copy2(style_file,run/'style_before.uasset')
        before=helper.text_export(unreal,style,run/'style_before.t3d')
        style.set_editor_property('combat_music',sound)
        assert unreal.EditorAssetLibrary.save_loaded_asset(style)
        after=helper.text_export(unreal,style,run/'style_after.t3d')
        assert helper.other_fields(before)==helper.other_fields(after)
        result={'mode':'bind','style_sha_before':BASE,'style_sha_after':sha(style_file),'all_other_exported_fields_unchanged':True}
    else:
        assert spec['mode']=='reload'
        previous_report=Path(spec['binding_report']);previous=json.loads(previous_report.read_text())
        assert sha(style_file)==previous['style_sha_after']
        current=helper.text_export(unreal,style,run/'style_reloaded.t3d')
        raw=(previous_report.parent/'style_after.t3d').read_bytes()
        expected=raw.decode('utf-16' if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')
        assert current.splitlines()==expected.splitlines()
        assert sha(style_file)==previous['style_sha_after']
        result={'mode':'reload','style_sha256':sha(style_file),'all_exported_values_match_binding':True}
    assert style.get_editor_property('combat_music')==sound
    assert sha(wave_file)==spec['wave_sha256'] and sha(original)==OLD and sha(class_file)==CLASS
    result.update({'wave_sha256':sha(wave_file),'wave':sound.get_path_name(),'duration_s':57,'looping':True,'volume':.5,
                   'full_original_and_class_preserved':True,'stage':'Isolated native check only; real tutorial timing/fade/video pending'})
    (run/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))

if globals().get('SPEC',{}).get('execute') is True:run(SPEC)
else:print('Prepared only; no asset load/write/native launch.')

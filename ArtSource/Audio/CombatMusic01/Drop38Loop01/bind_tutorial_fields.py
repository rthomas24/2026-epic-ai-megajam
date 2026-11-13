"""T1 contract: only TutorialPrelude, TutorialLoop, TutorialFadeOutSeconds. Explicit grant only."""
from pathlib import Path
import hashlib,importlib.util,json,shutil
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
FIELDS=('tutorial_prelude','tutorial_loop','tutorial_fade_out_seconds')
CPP_FIELDS=('TutorialPrelude','TutorialLoop','TutorialFadeOutSeconds')

def other_values(text):
    lines=text.splitlines()
    for field in CPP_FIELDS:
        assert sum(line.lstrip().startswith(field+'=') for line in lines)<=1
    return '\n'.join(line for line in lines if not any(line.lstrip().startswith(f+'=') for f in CPP_FIELDS))

def run(spec):
    import unreal
    assert spec.get('execute') is True and spec['claim_commit']
    host=Path(unreal.Paths.project_dir()).resolve();assert host==Path(spec['project_dir']).resolve()
    style_file=host/'Content/TriadDrift/Presentation/DA_UI_Style.uasset'
    assert sha(style_file)==spec['style_sha256']
    helper_path=Path(spec['helper']);assert sha(helper_path)=='aaa1cd4a5e5608264d3a7d25f59bc59a4bcb83028bf6878ae1a545a606832f10'
    loader=importlib.util.spec_from_file_location('export_helpers',helper_path);helper=importlib.util.module_from_spec(loader);loader.loader.exec_module(helper)
    root='/Game/TriadDrift/Audio/CombatMusic01/'
    expected={'MUS_Tutorial_EtherealPulse_Prelude38_01':'9b843cd44cd1a3f377ab002c8213b023b135a1be7ce702c1993fe9af6fe431f9',
              'MUS_Combat_EtherealPulseRemix_Drop38_01':'d2166e8e1a311161560ee7dab3c6d775e62c63ac1f3a66a1a13379e074a4cae5'}
    files={name:host/'Content/TriadDrift/Audio/CombatMusic01'/(name+'.uasset') for name in expected}
    cls_file=host/'Content/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music.uasset'
    assert sha(cls_file)=='813d76de263314464d0ad6f698a751bec6c99ae85d262601b1df85d80e0600c7'
    assert all(sha(files[n])==h for n,h in expected.items())
    prelude,loop=[unreal.load_asset(root+n) for n in expected]
    for sound,duration,looping in ((prelude,38,False),(loop,57,True)):
        assert isinstance(sound,unreal.SoundWave) and abs(sound.get_editor_property('duration')-duration)<.001
        assert sound.get_editor_property('looping')==looping and sound.get_editor_property('volume')==.5
        assert sound.get_editor_property('pitch')==1 and sound.get_editor_property('num_channels')==2
        assert sound.get_editor_property('imported_sample_rate')==48000
        assert sound.get_editor_property('sound_class_object').get_path_name()=='/Game/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music.SC_MenuTheme_Music'
    style=unreal.load_asset('/Game/TriadDrift/Presentation/DA_UI_Style');assert style
    def values():
        result={}
        for field in FIELDS:
            value=style.get_editor_property(field)
            result[field]=value.get_path_name() if field!='tutorial_fade_out_seconds' and value else (None if field!='tutorial_fade_out_seconds' else float(value))
        return result
    prior=values();assert prior==spec['expected_fields'], 'Unreviewed tutorial field values'
    run=host/'Saved/AudioMusic/CombatMusic01'/spec['run_id'];assert not run.exists();run.mkdir(parents=True)
    before=helper.text_export(unreal,style,run/'style_before.t3d')
    if spec['mode']=='bind':
        shutil.copy2(style_file,run/'style_before.uasset')
        for field,value in zip(FIELDS,(prelude,loop,20.)):style.set_editor_property(field,value)
        assert unreal.EditorAssetLibrary.save_loaded_asset(style)
        after=helper.text_export(unreal,style,run/'style_after.t3d')
        assert other_values(before)==other_values(after), 'Unrelated style value changed'
    else:
        assert spec['mode']=='reload'
        raw=Path(spec['binding_export']).read_bytes()
        bound=raw.decode('utf-16' if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')
        assert before.splitlines()==bound.splitlines(), 'Exported style differs across reload'
        after=before
    assert values()=={'tutorial_prelude':prelude.get_path_name(),'tutorial_loop':loop.get_path_name(),'tutorial_fade_out_seconds':20.}
    assert all(sha(files[n])==h for n,h in expected.items()) and sha(cls_file)=='813d76de263314464d0ad6f698a751bec6c99ae85d262601b1df85d80e0600c7'
    result={'stage':'T1 tutorial fields bound/reloaded; actual route and video still pending','mode':spec['mode'],'style_sha256':sha(style_file),'values':values(),'all_other_style_fields_preserved':True,'wave_and_music_class_unchanged':True}
    (run/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))

if globals().get('SPEC',{}).get('execute') is True:run(SPEC)
else:print('Prepared only; current T1 build, exact baseline and separate native grant required.')

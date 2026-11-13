"""Granted fresh-process read-only check after the CombatMusic-only style binding."""
from pathlib import Path
import hashlib,json,importlib.util

def verify(spec):
 import unreal
 host=Path(unreal.Paths.project_dir()).resolve();assert host==Path(spec['project_dir']).resolve()
 previous=json.loads(Path(spec['binding_report']).read_text())
 style_file=host/'Content/TriadDrift/Presentation/DA_UI_Style.uasset'
 digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
 assert digest(style_file)==previous['style_sha_after']
 wave_file=host/'Content/TriadDrift/Audio/CombatMusic01/MUS_Combat_EtherealPulseRemix_01.uasset'
 class_file=host/'Content/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music.uasset'
 assert digest(wave_file)=='1110b70395c6ce3b7423245dfa435ba68be4baf31e4d93f4c579dbb8842117aa'
 assert digest(class_file)=='813d76de263314464d0ad6f698a751bec6c99ae85d262601b1df85d80e0600c7'
 helper=Path(spec['binder_source']);assert digest(helper)=='aaa1cd4a5e5608264d3a7d25f59bc59a4bcb83028bf6878ae1a545a606832f10'
 loader=importlib.util.spec_from_file_location('binder_helpers',helper);module=importlib.util.module_from_spec(loader);loader.loader.exec_module(module)
 style=unreal.load_asset('/Game/TriadDrift/Presentation/DA_UI_Style');assert style
 sound=unreal.load_asset(previous['combat_music']);assert isinstance(sound,unreal.SoundWave)
 assert style.get_editor_property('combat_music')==sound
 assert sound.get_editor_property('looping') and abs(sound.get_editor_property('duration')-139.2)<.001
 assert sound.get_editor_property('volume')==.5 and sound.get_editor_property('pitch')==1
 assert sound.get_editor_property('num_channels')==2 and sound.get_editor_property('imported_sample_rate')==48000
 assert sound.get_editor_property('sound_class_object').get_path_name()=='/Game/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music.SC_MenuTheme_Music'
 run=host/'Saved/AudioMusic/CombatMusic01'/spec['run_id'];assert not run.exists();run.mkdir(parents=True)
 current=module.text_export(unreal,style,run/'style_reloaded.t3d')
 before=Path(spec['binding_report']).parent/'style_after.t3d'
 raw=before.read_bytes();recorded=raw.decode('utf-16' if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')
 assert current.splitlines()==recorded.splitlines(),'Serialized style values changed across reload'
 assert digest(style_file)==previous['style_sha_after']
 assert digest(wave_file)=='1110b70395c6ce3b7423245dfa435ba68be4baf31e4d93f4c579dbb8842117aa'
 assert digest(class_file)=='813d76de263314464d0ad6f698a751bec6c99ae85d262601b1df85d80e0600c7'
 report={'style_sha256':digest(style_file),'combat_music':sound.get_path_name(),'all_exported_values_match_binding':True,'unchanged_wave_and_class':True,'stage':'Read-only bound-style reload verified; automatic game playback pending'}
 (run/'reload.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))

if globals().get('SPEC',{}).get('execute') is True:verify(SPEC)
else:print('Prepared only; fresh compatible-host reload requires its separate Lead grant.')

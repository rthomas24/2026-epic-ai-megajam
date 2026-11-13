"""Proposal: granted CombatMusic-only binding in a compatible host. Never runs by default."""
from pathlib import Path
import hashlib,json,shutil

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def text_export(unreal,asset,path):
 assert not path.exists()
 task=unreal.AssetExportTask()
 for name,value in {'object':asset,'exporter':unreal.ObjectExporterT3D(),'filename':str(path),'automated':True,'prompt':False,'selected':False}.items():
  task.set_editor_property(name,value)
 assert unreal.Exporter.run_asset_export_task(task),list(task.get_editor_property('errors'))
 assert not task.get_editor_property('errors')
 data=path.read_bytes()
 return data.decode('utf-16' if data.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')

def other_fields(text):
 lines=text.splitlines()
 removed=[line for line in lines if line.lstrip().startswith('CombatMusic=')]
 assert len(removed)<=1,'Unexpected nested CombatMusic fields'
 return '\n'.join(line for line in lines if not line.lstrip().startswith('CombatMusic='))

def bind(spec):
 import unreal
 assert spec.get('execute') is True and spec['claim_commit']
 host=Path(unreal.Paths.project_dir()).resolve();assert host==Path(spec['project_dir']).resolve()
 style_file=host/'Content/TriadDrift/Presentation/DA_UI_Style.uasset'
 assert digest(style_file)==spec['style_sha_before'],'Style baseline changed'
 wave_file=host/'Content/TriadDrift/Audio/CombatMusic01/MUS_Combat_EtherealPulseRemix_01.uasset'
 assert digest(wave_file)=='1110b70395c6ce3b7423245dfa435ba68be4baf31e4d93f4c579dbb8842117aa'
 class_file=host/'Content/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music.uasset'
 assert digest(class_file)=='813d76de263314464d0ad6f698a751bec6c99ae85d262601b1df85d80e0600c7'
 run=host/'Saved/AudioMusic/CombatMusic01'/spec['run_id'];assert not run.exists()
 style=unreal.load_asset('/Game/TriadDrift/Presentation/DA_UI_Style');assert style
 sound=unreal.load_asset('/Game/TriadDrift/Audio/CombatMusic01/MUS_Combat_EtherealPulseRemix_01')
 assert isinstance(sound,unreal.SoundWave)
 assert sound.get_editor_property('looping') and abs(sound.get_editor_property('duration')-139.2)<.001
 assert sound.get_editor_property('volume')==.5 and sound.get_editor_property('pitch')==1
 assert sound.get_editor_property('num_channels')==2 and sound.get_editor_property('imported_sample_rate')==48000
 previous=style.get_editor_property('combat_music')
 previous_path=previous.get_path_name() if previous else None
 assert previous_path==spec['expected_previous_music'],'Unreviewed existing combat track'
 run.mkdir(parents=True)
 shutil.copy2(style_file,run/'style_before.uasset')
 before=text_export(unreal,style,run/'style_before.t3d')
 style.set_editor_property('combat_music',sound)
 assert unreal.EditorAssetLibrary.save_loaded_asset(style)
 after=text_export(unreal,style,run/'style_after.t3d')
 assert other_fields(before)==other_fields(after),'Unexpected serialized style-field change; preserve backup and stop'
 assert style.get_editor_property('combat_music')==sound
 assert digest(wave_file)=='1110b70395c6ce3b7423245dfa435ba68be4baf31e4d93f4c579dbb8842117aa'
 assert digest(class_file)=='813d76de263314464d0ad6f698a751bec6c99ae85d262601b1df85d80e0600c7'
 result={'style':style.get_path_name(),'previous_music':previous_path,'combat_music':sound.get_path_name(),
  'style_sha_before':spec['style_sha_before'],'style_sha_after':digest(style_file),'all_other_exported_style_fields_unchanged':True,
  'wave_and_music_class_unchanged':True,'stage':'CombatMusic field bound; fresh reload and automatic gameplay playback pending'}
 (run/'binding.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))

if globals().get('SPEC',{}).get('execute') is True:bind(SPEC)
else:print('Prepared only; requires current Core host, exact style baseline and separate Lead grant.')

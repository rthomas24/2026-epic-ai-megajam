"""Create/reload only the claimed boarding backing Wave; no gameplay/style writes."""
from pathlib import Path
import hashlib,json,wave
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def run(spec):
 import unreal
 assert spec['execute'] is True and spec['claim_commit']
 host=Path(unreal.Paths.project_dir()).resolve();assert host==Path(spec['project_dir']).resolve()
 source=Path(spec['source']);assert sha(source)=='33a0792214143a37efcdea4716e7e5caf957c5370b600b53ff8ef8483fdad69a'
 with wave.open(str(source)) as w:assert (w.getnframes(),w.getframerate(),w.getnchannels(),w.getsampwidth())==(1923840,48000,2,2)
 cls_file=host/'Content/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music.uasset';cls_hash=sha(cls_file)
 assert cls_hash=='813d76de263314464d0ad6f698a751bec6c99ae85d262601b1df85d80e0600c7'
 root='/Game/TriadDrift/Audio/DominionInterior01';name='MUS_DominionInterior_Calm_01';path=root+'/'+name
 package=host/'Content/TriadDrift/Audio/DominionInterior01'/(name+'.uasset')
 output=host/'Saved/DominionInterior01'/spec['run_id'];assert not output.exists()
 cls=unreal.load_asset('/Game/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music');assert cls
 if spec['mode']=='import':
  assert not package.exists() and not unreal.EditorAssetLibrary.does_asset_exist(path)
  task=unreal.AssetImportTask()
  for k,v in {'filename':str(source),'destination_path':root,'destination_name':name,'automated':True,'replace_existing':False,'save':False}.items():task.set_editor_property(k,v)
  unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);sound=unreal.load_asset(path);assert isinstance(sound,unreal.SoundWave)
  for k,v in {'sound_class_object':cls,'looping':False,'volume':1.,'pitch':1.,'virtualization_mode':unreal.VirtualizationMode.PLAY_WHEN_SILENT}.items():sound.set_editor_property(k,v)
  assert unreal.EditorAssetLibrary.save_loaded_asset(sound)
 else:
  assert spec['mode']=='reload' and sha(package)==spec['wave_sha256'];sound=unreal.load_asset(path)
 assert isinstance(sound,unreal.SoundWave) and sound.get_editor_property('sound_class_object')==cls
 assert not sound.get_editor_property('looping') and abs(sound.get_editor_property('duration')-40.08)<.001
 assert sound.get_editor_property('volume')==1 and sound.get_editor_property('pitch')==1
 assert sound.get_editor_property('num_channels')==2 and sound.get_editor_property('imported_sample_rate')==48000
 assert sha(cls_file)==cls_hash
 if spec['mode']=='reload':assert sha(package)==spec['wave_sha256']
 output.mkdir(parents=True);r={'stage':'Native boarding backing Wave checked; runtime hook pending','mode':spec['mode'],'asset':sound.get_path_name(),'sha256':sha(package),'bytes':package.stat().st_size,'duration_s':40.08,'looping':False,'volume':1,'pitch':1,'music_class':cls.get_path_name()}
 (output/'result.json').write_text(json.dumps(r,indent=2));print(json.dumps(r))
if globals().get('SPEC',{}).get('execute') is True:run(SPEC)
else:print('Prepared only; requires coordinated native slot.')

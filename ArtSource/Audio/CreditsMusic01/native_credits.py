"""Scoped two-Wave credits import or fresh readback; no playlist/gameplay binding."""
from pathlib import Path
import json,hashlib,wave
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def run(spec):
 import unreal
 assert spec['execute'] is True and spec['claim_commit']=='8fbb2ac6'
 host=Path(unreal.Paths.project_dir()).resolve();assert host==Path(spec['project_dir']).resolve()
 source=Path(spec['source_order']);assert sha(source)==spec['source_order_sha256']
 manifest=json.loads(source.read_text());tracks=manifest['tracks'];assert len(tracks)==2 and [t['order'] for t in tracks]==[1,2]
 assert [t['asset_name'] for t in tracks]==['MUS_Credits_CathedralPulse_Full_01','MUS_Credits_EtherealPulseRemix_Full_02']
 clsfile=host/'Content/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music.uasset';assert sha(clsfile)=='813d76de263314464d0ad6f698a751bec6c99ae85d262601b1df85d80e0600c7'
 cls=unreal.load_asset('/Game/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music');assert isinstance(cls,unreal.SoundClass)
 out=host/'Saved/CreditsMusic01'/spec['run_id'];assert not out.exists();rows=[]
 for t in tracks:
  original=Path(t['source']);wav=Path(t['wav']);assert sha(original)==t['source_sha256'] and sha(wav)==t['wav_sha256']
  with wave.open(str(wav)) as w:assert (w.getnframes(),w.getframerate(),w.getnchannels(),w.getsampwidth())==(t['decoded_pcm_frames'],48000,2,2)
  path=t['asset_path'];package=host/'Content'/(path.removeprefix('/Game/')+'.uasset')
  if spec['mode']=='import':
   assert not package.exists() and not unreal.EditorAssetLibrary.does_asset_exist(path)
   task=unreal.AssetImportTask()
   for k,v in {'filename':str(wav),'destination_path':'/Game/TriadDrift/Audio/CreditsMusic01','destination_name':t['asset_name'],'automated':True,'replace_existing':False,'save':False}.items():task.set_editor_property(k,v)
   unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task]);sound=unreal.load_asset(path);assert isinstance(sound,unreal.SoundWave)
   for k,v in {'sound_class_object':cls,'looping':False,'volume':.5,'pitch':1.,'virtualization_mode':unreal.VirtualizationMode.PLAY_WHEN_SILENT}.items():sound.set_editor_property(k,v)
   assert unreal.EditorAssetLibrary.save_loaded_asset(sound)
  else:
   assert spec['mode']=='reload' and sha(package)==spec['asset_hashes'][t['asset_name']];sound=unreal.load_asset(path)
  assert isinstance(sound,unreal.SoundWave) and sound.get_editor_property('sound_class_object')==cls
  assert not sound.get_editor_property('looping') and sound.get_editor_property('volume')==.5 and sound.get_editor_property('pitch')==1
  assert abs(sound.get_editor_property('duration')-t['decoded_pcm_duration_s'])<.002
  assert sound.get_editor_property('num_channels')==2 and sound.get_editor_property('imported_sample_rate')==48000
  rows.append({'order':t['order'],'asset':sound.get_path_name(),'sha256':sha(package),'bytes':package.stat().st_size,'decoded_duration_s':t['decoded_pcm_duration_s'],'looping':False,'volume':.5,'music_class':cls.get_path_name()})
 assert sha(clsfile)=='813d76de263314464d0ad6f698a751bec6c99ae85d262601b1df85d80e0600c7'
 out.mkdir(parents=True);r={'stage':'Two full credits Waves verified; runtime ordered playlist/handoff not implemented','mode':spec['mode'],'ordered_assets':rows,'source_order_preserved':True,'crossfade_or_repeat':'Not specified or implemented','credits_roll_separate_from_victory_distress':True}
 (out/'result.json').write_text(json.dumps(r,indent=2));print(json.dumps(r))
if globals().get('SPEC',{}).get('execute') is True:run(SPEC)
else:print('Prepared only. A coordinated native slot is required.')

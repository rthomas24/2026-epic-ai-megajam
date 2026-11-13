"""Granted fresh-editor reload check. Does not save assets or trigger playback."""
import hashlib,json
from pathlib import Path

def verify(spec):
 import unreal
 host=Path(unreal.Paths.project_dir()).resolve();assert host==Path(spec['project_dir']).resolve()
 manifest=json.loads(Path(spec['manifest']).read_text())
 path=manifest['native_root']+'/MUS_Combat_EtherealPulseRemix_01'
 package=host/'Content'/(path.removeprefix('/Game/')+'.uasset')
 before=hashlib.sha256(package.read_bytes()).hexdigest()
 sound=unreal.load_asset(path);assert isinstance(sound,unreal.SoundWave)
 expected={'looping':True,'pitch':1.0,'volume':0.5,'num_channels':2,'imported_sample_rate':48000}
 actual={name:sound.get_editor_property(name) for name in expected}
 assert actual==expected,actual
 duration=sound.get_editor_property('duration');assert abs(duration-139.2)<.001
 cls=sound.get_editor_property('sound_class_object');assert isinstance(cls,unreal.SoundClass)
 assert cls.get_path_name()==manifest['music_class']+'.SC_MenuTheme_Music'
 assert cls.get_editor_property('properties').get_editor_property('is_music')
 class_file=host/'Content'/(manifest['music_class'].removeprefix('/Game/')+'.uasset')
 assert hashlib.sha256(class_file.read_bytes()).hexdigest()==manifest['music_class_sha256']
 assert hashlib.sha256(package.read_bytes()).hexdigest()==before
 result={'wave':sound.get_path_name(),'duration_s':duration,'properties':actual,'music_class':cls.get_path_name(),
  'package_sha256':before,'shared_music_class_unchanged':True,'stage':'Fresh reload verified; combat consumer activation, actual playback/loop and Shipping still pending'}
 out=host/'Saved/AudioMusic/CombatMusic01/reload_01.json';assert not out.exists()
 out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps(result))

if globals().get('SPEC',{}).get('execute') is True:verify(SPEC)
else:print('Prepared only; separate Lead native reload grant required.')

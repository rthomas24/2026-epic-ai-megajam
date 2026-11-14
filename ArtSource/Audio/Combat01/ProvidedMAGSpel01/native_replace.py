"""Nine-wave-only replacement, or read-only reload, in the granted private host."""
from pathlib import Path
import json,hashlib,wave as wave_io,array
import unreal
host=Path(unreal.Paths.project_dir()).resolve()
assert host==Path(SPEC['host']).resolve()
root=Path(SPEC['source']);manifest=json.loads((root/'replacement_manifest.json').read_text())
run=Path(SPEC['run']);mode=SPEC['mode'];assert mode in ('import','reload')
fields=('volume','pitch','looping','sound_class_object','attenuation_settings','concurrency_set','compression_quality','loading_behavior')
def props(w):
 values={k:w.get_editor_property(k) for k in fields}
 return values
def serial(v):
 if isinstance(v,dict):return {k:serial(x) for k,x in v.items()}
 if hasattr(v,'get_path_name'):return v.get_path_name()
 if isinstance(v,(str,int,float,bool)) or v is None:return v
 try:return [serial(x) for x in v]
 except TypeError:return str(v)
rows=[]
baseline=json.loads((Path(SPEC['import_run'])/'native.json').read_text()) if mode=='reload' else None
for index,row in enumerate(manifest['rows']):
 asset=row['asset_path'];wave=unreal.load_asset(asset);assert isinstance(wave,unreal.SoundWave),asset
 before=props(wave);source=Path(row['output'])
 assert hashlib.sha256(source.read_bytes()).hexdigest()==row['output_sha256']
 if mode=='import':
  task=unreal.AssetImportTask()
  for k,v in dict(filename=str(source),destination_path=asset.rsplit('/',1)[0],destination_name=asset.rsplit('/',1)[1],automated=True,replace_existing=True,replace_existing_settings=False,save=False).items():task.set_editor_property(k,v)
  unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
  assert task.get_editor_property('imported_object_paths')
  wave=unreal.load_asset(asset)
  for k,v in before.items():wave.set_editor_property(k,v)
  assert serial(props(wave))==serial(before)
  assert unreal.EditorAssetLibrary.save_loaded_asset(wave,only_if_is_dirty=False)
 else:
  assert serial(before)==baseline['rows'][index]['properties']
 assert abs(wave.get_editor_property('duration')-row['duration_s'])<.001
 assert wave.get_editor_property('num_channels')==1
 assert wave.get_editor_property('imported_sample_rate')==48000
 assert not wave.get_editor_property('looping')
 if mode=='reload':
  dest=run/(asset.rsplit('/',1)[1]+'.wav')
  task=unreal.AssetExportTask()
  for k,v in dict(object=wave,exporter=unreal.SoundExporterWAV(),filename=str(dest),automated=True,prompt=False).items():task.set_editor_property(k,v)
  assert unreal.Exporter.run_asset_export_task(task)
  with wave_io.open(str(source)) as f:expected=(f.getparams()[:3],f.readframes(f.getnframes()))
  with wave_io.open(str(dest)) as f:actual=(f.getparams()[:3],f.readframes(f.getnframes()))
  assert actual[0]==expected[0] and len(actual[1])==len(expected[1])
  # Sound import/export passes through float; tolerate one 16-bit quantization
  # step, never a gain change, timing shift, wrong take or codec loss.
  quantization=max(abs(a-b) for a,b in zip(array.array('h',actual[1]),array.array('h',expected[1])))
  assert quantization<=1,('Native PCM differs',asset,quantization)
 rows.append(dict(asset=asset,duration=wave.get_editor_property('duration'),properties=serial(props(wave)),source_sha256=row['output_sha256'],native_pcm_max_error_lsb=quantization if mode=='reload' else None))
for role in ('NeedleCannon','Hull','Shield'):
 assert unreal.load_asset('/Game/TriadDrift/Audio/Combat01/MSS_'+role)
da=unreal.load_asset('/Game/TriadDrift/Presentation/DA_NeedleCannons_FX');assert da
for field,role in (('muzzle','NeedleCannon'),('impact_hull','Hull'),('impact_shield','Shield')):
 assert da.get_editor_property(field).get_editor_property('sound').get_path_name()=='/Game/TriadDrift/Audio/Combat01/MSS_'+role+'.MSS_'+role
(run/'native.json').write_text(json.dumps(dict(mode=mode,rows=rows,routing_preserved=True),indent=2))
print('PR12 nine-wave '+mode+' PASS')

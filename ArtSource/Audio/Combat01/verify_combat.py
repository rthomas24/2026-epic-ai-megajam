"""Read-only fresh-editor validation; no package saves or gameplay simulation."""
from pathlib import Path
import json

host=Path(unreal.Paths.project_dir()).resolve()
assert host==Path(SPEC['project_dir']).resolve()
source=Path(SPEC['source_root'])
spec=json.loads((source/'build.spec.json').read_text())
paths=[row['path'] for group in ('mix','import','oneshots') for row in spec[group]]
assets={p:unreal.load_asset(p) for p in paths}
assert len(assets)==16 and all(assets.values()),'Missing native asset'
for row in spec['import']:
 wave=assets[row['path']]
 assert isinstance(wave,unreal.SoundWave) and not wave.get_editor_property('looping')
for row in spec['oneshots']:
 sound=assets[row['path']];assert isinstance(sound,unreal.SoundBase)
 for field,key in (('sound_class_object','soundClassObject'),('attenuation_settings','attenuationSettings')):
  assert sound.get_editor_property(field)==assets[row['sound'][key]],(row['path'],field)
 assert set(sound.get_editor_property('concurrency_set'))=={assets[p] for p in row['sound']['concurrencySet']}
cls=unreal.load_class(None,'/Script/unrealGameJam.TDWeaponFX');assert cls
asset=unreal.load_asset('/Game/TriadDrift/Presentation/DA_NeedleCannons_FX');assert asset
defaults=unreal.get_default_object(cls)
slots={'muzzle':'NeedleCannon','impact_hull':'Hull','impact_shield':'Shield'}
for name,ending in slots.items():
 assert asset.get_editor_property(name).get_editor_property('sound')==assets['/Game/TriadDrift/Audio/Combat01/MSS_'+ending]
for name in ('muzzle','charge_loop','overheat','impact_hull','impact_shield','impact_world','detonation','deflected'):
 for field in ('vfx','shake','shake_scale','shake_radius','vfx_scale'):
  assert asset.get_editor_property(name).get_editor_property(field)==defaults.get_editor_property(name).get_editor_property(field),(name,field)
 if name not in slots:assert asset.get_editor_property(name).get_editor_property('sound')==defaults.get_editor_property(name).get_editor_property('sound')
for field in ('tracer','projectile_mesh','projectile_mesh_offset','color'):
 assert asset.get_editor_property(field)==defaults.get_editor_property(field),field
result={'loaded_assets':17,'sound_class_attenuation_concurrency_refs_verified':True,
 'sound_slots':{name:asset.get_editor_property(name).get_editor_property('sound').get_path_name() for name in slots},
 'all_visual_and_other_defaults_preserved':True,'stage':'Read-only reload validation; runtime playback, listening and Shipping still required'}
output=host/'Saved/AudioMusic/Combat01/reload_01.json';assert not output.exists()
output.write_text(json.dumps(result,indent=2)+'\n')
result

"""Granted current-game binding only: three Sound slots, preserving all visual/other fields."""
import json
from pathlib import Path

def bind(spec):
    import unreal
    host=Path(unreal.Paths.project_dir()).resolve()
    assert spec.get('execute') is True and host==Path(spec['project_dir']).resolve() and spec['claim_commit']
    path='/Game/TriadDrift/Presentation/DA_NeedleCannons_FX'
    assert not unreal.EditorAssetLibrary.does_asset_exist(path), 'Existing shared DA requires separate snapshot/review; never overwrite'
    report=host/'Saved/AudioMusic/Combat01/bind_01.json'
    assert not report.exists(), 'Preserve prior attempt'
    cls=unreal.load_class(None,'/Script/unrealGameJam.TDWeaponFX');assert cls
    names={'muzzle':'NeedleCannon','impact_hull':'Hull','impact_shield':'Shield'}
    sounds={slot:unreal.load_asset('/Game/TriadDrift/Audio/Combat01/MSS_'+name) for slot,name in names.items()}
    assert all(isinstance(s,unreal.SoundBase) for s in sounds.values()), 'Missing native cues'
    defaults=unreal.get_default_object(cls)
    # Capture every current cue's non-Sound members and all other explicit WeaponFX fields.
    cue_fields=('vfx','shake','shake_scale','shake_radius','vfx_scale')
    all_cues=('muzzle','charge_loop','overheat','impact_hull','impact_shield','impact_world','detonation','deflected')
    preserved={n:defaults.get_editor_property(n) for n in ('tracer','projectile_mesh','projectile_mesh_offset','color')}
    original_cues={n:{f:defaults.get_editor_property(n).get_editor_property(f) for f in cue_fields} for n in all_cues}
    factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',cls)
    asset=unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_NeedleCannons_FX','/Game/TriadDrift/Presentation',cls,factory);assert asset
    for slot,sound in sounds.items():
        cue=asset.get_editor_property(slot);assert cue.get_editor_property('sound') is None
        cue.set_editor_property('sound',sound);asset.set_editor_property(slot,cue)
    assert all(asset.get_editor_property(n)==v for n,v in preserved.items())
    assert all(asset.get_editor_property(n).get_editor_property(f)==v for n,fields in original_cues.items() for f,v in fields.items())
    assert all(asset.get_editor_property(n).get_editor_property('sound')==defaults.get_editor_property(n).get_editor_property('sound') for n in all_cues if n not in sounds)
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
    result={'asset':asset.get_path_name(),'sound_slots':{n:s.get_path_name() for n,s in sounds.items()},
            'all_visual_and_other_defaults_preserved':True,'stage':'Sound slots assigned; reload/real firing/hits/cook/listening pending'}
    report.parent.mkdir(parents=True,exist_ok=True);report.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))

if globals().get('SPEC',{}).get('execute') is True:
    bind(SPEC)
else:
    print('Source preparation only; separate native author/binding grant required.')

"""Verify source by default. Native execution needs SPEC.execute and a Lead-granted project_dir.

Normal Python: python import_combat_music.py
Granted editor: ue_run_python.py --port 8025 import_combat_music.py --spec <grant.json>
Never edits a menu, map, data asset, configuration or an existing native asset. Combat-state binding is a separate integration step.
"""
import hashlib
import json
from pathlib import Path
import wave


def verify_source(manifest):
    source = Path(manifest['prepared_file'])
    assert hashlib.sha256(source.read_bytes()).hexdigest() == manifest['sha256'], 'Cut hash mismatch'
    with wave.open(str(source), 'rb') as wav:
        actual = (wav.getnframes(), wav.getframerate(), wav.getnchannels(), wav.getsampwidth())
    assert actual == (6681600, 48000, 2, 2), actual
    return {'source_verified': True, 'frames': actual[0], 'duration_s': 139.2}


def import_native(manifest, grant):
    import unreal
    project = Path(unreal.Paths.project_dir()).resolve()
    assert project == Path(grant['project_dir']).resolve(), 'Wrong granted host'
    root = manifest['native_root']
    wave_path = root + '/MUS_Combat_EtherealPulseRemix_01'
    class_path = manifest['music_class']
    class_file = project / 'Content' / (class_path.removeprefix('/Game/') + '.uasset')
    assert hashlib.sha256(class_file.read_bytes()).hexdigest() == manifest['music_class_sha256']
    music_class = unreal.load_asset(class_path)
    assert isinstance(music_class, unreal.SoundClass) and music_class.get_editor_property('properties').get_editor_property('is_music')
    for path in (wave_path,):
        assert not unreal.EditorAssetLibrary.does_asset_exist(path), 'Existing asset: stop for review: ' + path
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', manifest['prepared_file'])
    task.set_editor_property('destination_path', root)
    task.set_editor_property('destination_name', 'MUS_Combat_EtherealPulseRemix_01')
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', False)
    task.set_editor_property('save', False)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    tools.import_asset_tasks([task])
    sound = unreal.load_asset(wave_path)
    assert isinstance(sound, unreal.SoundWave), 'SoundWave import failed'
    sound.set_editor_property('sound_class_object', music_class)
    sound.set_editor_property('looping', True)
    sound.set_editor_property('volume', 0.5)
    sound.set_editor_property('pitch', 1.0)
    sound.set_editor_property('virtualization_mode', unreal.VirtualizationMode.PLAY_WHEN_SILENT)
    assert abs(sound.get_editor_property('duration') - 139.2) < 0.001, 'Imported duration mismatch'
    for asset in (sound,):
        assert unreal.EditorAssetLibrary.save_loaded_asset(asset), 'Asset save failed'
    result = {'wave': sound.get_path_name(), 'music_class': music_class.get_path_name(),
              'duration_s': sound.get_editor_property('duration'), 'looping': sound.get_editor_property('looping'),
              'stage': 'Imported only; game playback, cook and human review pending'}
    evidence = project / 'Saved/AudioMusic/PrototypeV1/CombatMusic01/native_import.json'
    evidence.parent.mkdir(parents=True, exist_ok=True)
    evidence.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    return result


# ue_run_python supplies SPEC; explicit manifest path supports runners that compile without __file__.
grant = globals().get('SPEC', {})
manifest_path = Path(grant['manifest']) if 'manifest' in grant else Path(__file__).with_name('manifest_02.json')
manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
result = verify_source(manifest)
if grant.get('execute') is True:
    result['native'] = import_native(manifest, grant)
print(json.dumps(result, indent=2))

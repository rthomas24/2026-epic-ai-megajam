"""Granted editor/commandlet only: read-only reload validation; no saves or playback.

SPEC must contain execute=True, exact project_dir, and absolute manifest/report paths.
Writes evidence only below the granted host's Saved tree. Not a launcher or grant.
"""
import hashlib
import json
from pathlib import Path


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def validate(spec):
    import unreal
    assert spec.get('execute') is True, 'Separate validation grant required'
    host = Path(unreal.Paths.project_dir()).resolve()
    assert host == Path(spec['project_dir']).resolve(), 'Wrong host'
    manifest = json.loads(Path(spec['manifest']).read_text(encoding='utf-8'))
    imported = json.loads(Path(spec['import_report']).read_text(encoding='utf-8'))
    output = host/'Saved/AudioMusic/PrototypeV1/MenuTheme01/reload_validation_01.json'
    assert not output.exists(), 'Preserve existing evidence; do not overwrite'
    packages = {Path(a['path']): a['sha256'] for a in imported['assets']}
    before = {str(p): sha(p) for p in packages}
    assert all(before[str(p)] == h for p,h in packages.items()), 'Imported packages changed before reload'
    result = {'stage': 'Read-only native reload; no playback/cook/gameplay approval', 'before_sha256': before}
    try:
        wave = unreal.load_asset(manifest['native_wave'])
        music_class = unreal.load_asset(manifest['native_music_class'])
        assert isinstance(wave, unreal.SoundWave), 'Wrong/missing SoundWave'
        assert isinstance(music_class, unreal.SoundClass), 'Wrong/missing SoundClass'
        settings = {
            'wave': wave.get_path_name(), 'music_class': music_class.get_path_name(),
            'assigned_class': wave.get_editor_property('sound_class_object').get_path_name(),
            'is_music': music_class.get_editor_property('properties').get_editor_property('is_music'),
            'duration_s': wave.get_editor_property('duration'),
            'looping': wave.get_editor_property('looping'),
            'pitch': wave.get_editor_property('pitch'), 'volume': wave.get_editor_property('volume'),
        }
        result['settings'] = settings
        assert settings['assigned_class'] == settings['music_class']
        assert settings['is_music'] is True and settings['looping'] is False
        assert abs(settings['duration_s']-107.0) < 0.001
        assert abs(settings['pitch']-1.0) < 0.000001 and abs(settings['volume']-1.0) < 0.000001
        registry = unreal.AssetRegistryHelpers.get_asset_registry()
        options = unreal.AssetRegistryDependencyOptions(
            include_soft_package_references=True, include_hard_package_references=True,
            include_searchable_names=False, include_soft_management_references=False,
            include_hard_management_references=False)
        pending = [manifest['native_wave'], manifest['native_music_class']]
        dependencies = {}
        while pending:
            package = pending.pop()
            if package in dependencies:
                continue
            assert len(dependencies) < 256, 'Unexpected dependency graph size'
            refs = [str(d) for d in registry.get_dependencies(unreal.Name(package), options)]
            dependencies[package] = refs
            assert not any('tdaudiov1' in d.lower() for d in [package]+refs), 'Private module dependency'
            for ref in refs:
                if ref.startswith('/Game/') and ref not in dependencies:
                    assert unreal.EditorAssetLibrary.does_asset_exist(ref), 'Unresolved game dependency: '+ref
                    pending.append(ref)
        result['package_dependencies'] = dependencies
        wave_refs = dependencies[manifest['native_wave']]
        assert manifest['native_music_class'] in wave_refs, 'Music class dependency not registered'
        assert sha(manifest['prepared_file']) == manifest['sha256'], 'Prepared source hash changed'
        data = wave.get_editor_property('asset_import_data')
        if data and hasattr(data, 'extract_filenames'):
            files = list(data.extract_filenames())
            assert files, 'Import data contains no source file'
            paths = [Path(f).resolve() for f in files]
            expected = Path(manifest['prepared_file']).resolve()
            assert expected in paths, 'Import data points to a different source'
            assert sha(expected) == manifest['sha256']
            result['import_source'] = {'filenames': files, 'referenced_file_sha256': sha(expected),
                                       'note': 'Hashes the referenced local source; not a claim to read the embedded Unreal source hash.'}
        else:
            result['import_source'] = {'status': 'optional filename extraction unavailable; manifest source hash verified'}
        result['passed'] = True
    except Exception as error:
        result['passed'] = False
        result['error'] = str(error)
        raise
    finally:
        after = {str(p): sha(p) for p in packages}
        result['after_sha256'] = after
        result['packages_unchanged'] = before == after
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
        assert before == after, 'Package files changed during read-only reload'
    return result


if globals().get('SPEC', {}).get('execute') is True:
    print(json.dumps(validate(SPEC), indent=2))
else:
    print('Prepared read-only check only. Requires a separate Lead validation grant; no Unreal calls made.')

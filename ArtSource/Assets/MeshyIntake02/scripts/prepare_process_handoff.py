"""Consolidate pinned original process records; never synthesize a native PASS.

--records JSON contains pilot_import, pilot_process, pilot_native_reload and
processes (12 records), each {path, sha256}. Output matches the existing transfer
gate. The earlier pilot reload is stage evidence, not a successful batch exit.
"""
import argparse
import json
from intake_common import *


def clean_process(data, historical=False):
    assert data.get('AbortReason') in (None, ''), 'Aborted native process'
    assert type(data.get('PID')) is int and data['PID'] > 0, 'Missing native PID'
    if historical:
        assert type(data.get('ExitCode')) is int and data['ExitCode'] == 0, 'Historical exit failed/unknown'
    else:
        assert data.get('Passed') is True and data.get('ExitCodeKnown') is True, 'Native process failed/unknown'
        assert type(data.get('ActualExitCode')) is int and data['ActualExitCode'] == 0, 'Native exit failed/unknown'
        assert data.get('Errors') == [], 'Native errors missing/nonempty'


def collect(records, manager=MANAGER):
    refs = {}

    def read(pin):
        path = Path(pin['path']).resolve()
        assert path.is_relative_to(manager.resolve()), 'Evidence outside manager host'
        assert digest(path) == pin['sha256'], 'Evidence pin changed: ' + str(path)
        refs[str(path)] = dict(path=str(path), sha256=pin['sha256'])
        return json.loads(path.read_text(encoding='utf-8-sig'))

    def linked(path):
        path = Path(path).resolve()
        return read(dict(path=str(path), sha256=digest(path)))

    pilot = read(records['pilot_import'])
    assert pilot.get('technical_import_passed') is True and pilot.get('errors') == []
    clean_process(read(records['pilot_process']), historical=True)
    groups = list(pilot['imports'])
    assert len(groups) == 2 and {g['name'] for g in groups} == {'CrimsonTempest', 'SentinelArmor'}
    reload = read(records['pilot_native_reload'])
    assert reload.get('passed') is True and reload.get('errors') == []
    assert len(reload['assets']) == 18, 'Exact saved pilot reload required'
    reload_path = Path(records['pilot_native_reload']['path']).resolve()
    # Preserve the failed parent process verbatim; its stage PASS is not an exit PASS.
    linked(reload_path.parent / 'process_result.json')
    process_pins = records['processes']
    assert len(process_pins) == 12, 'Twelve clean per-source processes required'
    seen = set()
    for pin in process_pins:
        process = read(pin)
        clean_process(process)
        attempt = Path(process['AttemptDir']).resolve()
        assert Path(pin['path']).resolve().parent == attempt, 'Process/attempt mismatch'
        assert attempt not in seen, 'Duplicate native attempt'
        seen.add(attempt)
        intake_path = Path(process['IntakeReport']).resolve()
        pilot_path = Path(process['PilotReloadReport']).resolve()
        assert intake_path.parent == pilot_path.parent == attempt, 'Cross-attempt report reuse'
        intake, preservation = linked(intake_path), linked(pilot_path)
        assert intake.get('technical_import_passed') is True and intake.get('errors') == []
        assert intake.get('saved_pilot_baseline_unchanged') is True
        assert preservation.get('passed') is True and preservation.get('errors') == []
        assert Path(preservation['prior_native_reload']).resolve() == reload_path
        assert len(intake['imports']) == 1, 'One exact source per process required'
        group = intake['imports'][0]
        assert group.get('import_checks_passed') is True
        source = intake['sources']
        assert len(source) == 1 and source[0]['name'] == group['name']
        assert all(source[0].get(k) is True for k in (
            'original_hash_verified_before', 'staged_hash_verified', 'original_hash_verified_after'))
        groups.append(group)
    assert len(groups) == len({g['name'] for g in groups}) == 14, 'Duplicate/missing source groups'
    return groups, list(refs.values())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--records', required=True)
    args = parser.parse_args()
    target = EVIDENCE / 'manager_handoff.json'
    assert not target.exists(), 'Preserve prior handoff'
    records_path = Path(args.records).resolve()
    groups, refs = collect(json.loads(records_path.read_text(encoding='utf-8-sig')))
    source_guard()
    assert {g['name'] for g in groups} == {a['name'] for a in MANIFEST['assets']}
    expected, objects = {}, {}
    for source in MANIFEST['assets']:
        group = next(g for g in groups if g['name'] == source['name'])
        classes = [a['class'] for a in group['assets']]
        skinned = bool(source['skins'])
        counts = {'SkeletalMesh' if skinned else 'StaticMesh': 1,
                  'Texture2D': source['images'], 'Material': source['materials'],
                  'Skeleton': int(skinned), 'AnimSequence': len(source['animations'])}
        assert all(classes.count(c) == n for c, n in counts.items()), 'Source class counts changed'
        assert len(classes) == sum(counts.values()), 'Unexpected asset class'
        for entry in group['assets']:
            package = entry['path'].split('.')[0]
            assert package.startswith(source['destination'] + '/') and allowed_package(package)
            relative = package.removeprefix('/Game/') + '.uasset'
            assert relative not in expected and entry['path'] not in objects, 'Duplicate package/object'
            path = MANAGER / 'Content' / relative
            expected[relative] = dict(bytes=path.stat().st_size, sha256=digest(path))
            objects[entry['path']] = entry
    assert len(expected) == len(objects) == 91, 'Exact 91 packages required'
    verify_files(MANAGER / 'Content', expected)
    source_guard()
    assert all(digest(r['path']) == r['sha256'] for r in refs), 'Evidence changed during handoff'
    write('manager_handoff.json', dict(passed=True, manager_root=str(MANAGER),
          packages=expected, objects=objects, groups=groups, source_reports=refs,
          records_manifest=dict(path=str(records_path), sha256=digest(records_path)),
          pilot_reload_evidence='stage-only; failed parent process preserved, not promoted to PASS',
          project_transfer_pending=True))
    print(json.dumps(dict(passed=True, groups=14, packages=91, handoff_sha256=digest(target))))


if __name__ == '__main__':
    main()

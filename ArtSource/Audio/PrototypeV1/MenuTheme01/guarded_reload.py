"""Run02 read-only reload launcher. Default is no launch; needs fresh Lead grant after HUD release."""
import datetime as dt
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
loader = importlib.util.spec_from_file_location('menu_guard', HERE/'guarded_import.py')
guard = importlib.util.module_from_spec(loader)
loader.loader.exec_module(guard)
HOST = guard.HOST
BASE = HOST/'Saved/AudioMusic/PrototypeV1/MenuTheme01'

def inputs():
    hashes = guard.protected()
    for path in (Path(__file__).resolve(), HERE/'validate_reload.py', HERE/'native_import_01.json'):
        hashes[str(path)] = guard.digest(path)
    return hashes

def main():
    if len(sys.argv) != 3 or sys.argv[1] != '--grant':
        print('Run02 prepared only. No launch without fresh Lead slot-release grant.')
        return
    grant = json.loads(Path(sys.argv[2]).read_text())
    assert grant['authorized'] is True and grant['hud_slot_released'] is True and grant['claim_commit']
    assert grant['run_id'] == 'Run02' and Path(grant['project_dir']).resolve() == HOST.resolve()
    expiry = dt.datetime.fromisoformat(grant['expires_utc'])
    now = dt.datetime.now(dt.timezone.utc)
    assert expiry.tzinfo and now < expiry <= now+dt.timedelta(minutes=15)
    assert guard.digest(HERE/'validate_reload.py') == '31d5fa85d8881989ffd0c8fc68d37f1aefce15525f268ed0598e2aa10ef4d816'
    assert guard.digest(HERE/'guarded_import.py') == '85340b2eae4453e943c708601686bcfc389068e41901837a4779182585182ed5'
    guard.idle_guard()
    run = BASE/'Run02'
    assert not run.exists(), 'Preserve prior attempt; no retry'
    assert not (BASE/'reload_validation_01.json').exists(), 'Validation evidence already exists'
    report = json.loads((HERE/'native_import_01.json').read_text())
    for asset in report['assets']:
        assert guard.digest(Path(asset['path'])) == asset['sha256'], 'Imported package changed'
    before = inputs()
    run.mkdir(parents=True)
    (run/'protected_before.json').write_text(json.dumps(before, indent=2)+'\n')
    spec = {'execute': True, 'project_dir': str(HOST), 'manifest': str(HERE/'manifest.json'),
            'import_report': str(HERE/'native_import_01.json')}
    (run/'spec.json').write_text(json.dumps(spec, indent=2)+'\n')
    script = HERE/'validate_reload.py'
    bootstrap = run/'bootstrap.py'
    bootstrap.write_text('SPEC = '+repr(spec)+'\nexec(compile(open('+repr(str(script))+", encoding='utf-8').read(), 'menu_reload', 'exec'))\n", encoding='utf-8')
    guard.idle_guard()
    command = [str(guard.ENGINE), str(guard.PROJECT), '-AgentMode', '-nullrhi', '-NonRealtimeAudio',
               '-NoLiveCoding', '-run=pythonscript', '-script='+str(bootstrap), '-ModelContextProtocolPort=8025',
               '-ExitTimeout=180', '-NoProgressTimeout=60', '-ResultFile='+str(run/'engine_result.json'),
               '-abslog='+str(run/'engine.log')]
    started = time.monotonic(); low_since = None; minimum_free = guard.free_gib(); reason = 'completed'
    process = subprocess.Popen(command, creationflags=subprocess.CREATE_NO_WINDOW)
    print('Read-only Run02 PID', process.pid, flush=True)
    while process.poll() is None:
        now_mono = time.monotonic(); free = guard.free_gib(); minimum_free = min(minimum_free, free)
        low_since = (low_since or now_mono) if free < 1.5 else None
        if now_mono-started > 240 or (low_since is not None and now_mono-low_since > 10):
            reason = 'timeout_or_low_memory'
            subprocess.run(['taskkill', '/PID', str(process.pid), '/T', '/F'], check=True, capture_output=True)
            break
        time.sleep(2)
    code = process.wait()
    after = inputs()
    (run/'protected_after.json').write_text(json.dumps(after, indent=2)+'\n')
    summary = {'pid': process.pid, 'exit_code': code, 'reason': reason, 'minimum_free_gib': minimum_free,
               'all_inputs_and_packages_unchanged': before == after, 'protected_count': len(before)}
    (run/'guard_result.json').write_text(json.dumps(summary, indent=2)+'\n')
    assert code == 0 and reason == 'completed' and before == after, summary
    engine = json.loads((run/'engine_result.json').read_text())
    assert engine['reason'] == 'ok' and engine['log']['errors'] == 0, engine['log']
    check = json.loads((BASE/'reload_validation_01.json').read_text())
    assert check['passed'] is True and check['packages_unchanged'] is True, check
    print(json.dumps(summary))

if __name__ == '__main__': main()

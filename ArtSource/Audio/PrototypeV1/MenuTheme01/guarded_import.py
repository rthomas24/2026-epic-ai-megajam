"""Prepare a host snapshot, or run one explicitly granted native import. No automatic retry."""
import argparse
import ctypes
import datetime as dt
import hashlib
import json
from pathlib import Path
import shutil
import socket
import subprocess
import time

HERE = Path(__file__).resolve().parent
HOST = HERE.parents[3] / 'Saved/AudioMusic/PrototypeV1/Host'
PROJECT = HOST / 'TDAudioV1.uproject'
ENGINE = Path('C:/Program Files/Epic Games/UE_5.9/Engine/Binaries/Win64/UnrealEditor-Cmd.exe')

def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''): h.update(block)
    return h.hexdigest()

def protected():
    files = [PROJECT, HERE/'manifest.json', HERE/'import_menu_theme.py', HERE/'guarded_import.py', HERE/'README.md']
    for folder in ('Source', 'Config', 'Content'):
        files.extend(p for p in (HOST/folder).rglob('*') if p.is_file())
    files.extend(p for p in (HOST/'Binaries/Win64').glob('*') if p.suffix in ('.dll', '.exe', '.modules', '.target'))
    manifest = json.loads((HERE/'manifest.json').read_text())
    files.append(Path(manifest['prepared_file']))
    return {str(p.resolve()): digest(p) for p in sorted(set(files))}

class Memory(ctypes.Structure):
    _fields_ = [('length', ctypes.c_ulong), ('load', ctypes.c_ulong)] + [(n, ctypes.c_ulonglong) for n in ('total', 'available', 'page_total', 'page_available', 'virtual_total', 'virtual_available', 'extended')]

def free_gib():
    m = Memory(); m.length = ctypes.sizeof(m)
    assert ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(m))
    return m.available / 2**30

def idle_guard():
    command = "Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -match '^(UnrealEditor.*|ShaderCompileWorker|blender|dotnet|cl)$' } | Select-Object -ExpandProperty ProcessName"
    busy = subprocess.check_output(['powershell.exe', '-NoProfile', '-Command', command], text=True).strip()
    assert not busy, 'Heavy process active: ' + busy
    assert free_gib() >= 4.0, 'Below 4 GiB launch threshold'
    with socket.socket() as sock:
        sock.bind(('127.0.0.1', 8025))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--snapshot', type=Path, help='Write read-only input hashes; does not launch Unreal')
    parser.add_argument('--grant', type=Path, help='Explicit Lead grant JSON; absent means no launch')
    args = parser.parse_args()
    assert PROJECT.exists() and ENGINE.exists(), 'Proposed host/engine unavailable'
    if args.snapshot:
        assert not args.snapshot.exists(), 'Never overwrite a frozen snapshot'
        args.snapshot.parent.mkdir(parents=True, exist_ok=True)
        args.snapshot.write_text(json.dumps(protected(), indent=2)+'\n', encoding='utf-8')
        print('Protected input snapshot saved; no native execution')
        return
    if not args.grant:
        print('PROPOSAL ONLY. Native execution requires exact published claim and separate Lead grant.')
        return
    grant = json.loads(args.grant.read_text())
    assert grant['authorized'] is True and grant['claim_commit'], 'Missing Lead authorization/claim'
    assert Path(grant['project_dir']).resolve() == HOST.resolve(), 'Wrong host'
    expires = dt.datetime.fromisoformat(grant['expires_utc'])
    assert expires.tzinfo and dt.datetime.now(dt.timezone.utc) < expires, 'Expired grant'
    before = json.loads(Path(grant['protected_snapshot']).read_text())
    assert protected() == before, 'Protected inputs changed; new review required'
    destination = HOST/'Content/TriadDrift/Audio/MenuTheme01'
    assert not destination.exists(), 'Output exists: preserve and request review, do not overwrite'
    idle_guard()
    run = HOST/'Saved/AudioMusic/PrototypeV1/MenuTheme01'/grant['run_id']
    assert run.resolve().is_relative_to((HOST/'Saved/AudioMusic/PrototypeV1/MenuTheme01').resolve())
    assert not run.exists(), 'Run evidence already exists'
    run.mkdir(parents=True)
    # Back up existing packages before the only authorized binary write. No restore/delete automation.
    shutil.copytree(HOST/'Content', run/'protected_content_backup')
    spec = {'execute': True, 'project_dir': str(HOST), 'manifest': str(HERE/'manifest.json')}
    bootstrap = run/'bootstrap.py'
    bootstrap.write_text('SPEC = '+repr(spec)+'\nexec(compile(open('+repr(str(HERE/'import_menu_theme.py'))+", encoding='utf-8').read(), 'menu_import', 'exec'))\n", encoding='utf-8')
    idle_guard()
    command = [str(ENGINE), str(PROJECT), '-AgentMode', '-nullrhi', '-NonRealtimeAudio', '-NoLiveCoding', '-run=pythonscript', '-script='+str(bootstrap), '-ModelContextProtocolPort=8025', '-ExitTimeout=180', '-NoProgressTimeout=60', '-ResultFile='+str(run/'engine_result.json'), '-abslog='+str(run/'engine.log')]
    start = time.monotonic(); low_since = None; reason = 'completed'
    process = subprocess.Popen(command, creationflags=subprocess.CREATE_NO_WINDOW)
    while process.poll() is None:
        now = time.monotonic()
        low_since = (low_since or now) if free_gib() < 1.5 else None
        if now-start > 240 or (low_since is not None and now-low_since > 10):
            reason = 'timeout_or_low_memory'
            subprocess.run(['taskkill', '/PID', str(process.pid), '/T', '/F'], check=True, capture_output=True)
            break
        time.sleep(2)
    code = process.wait()
    changed = [p for p,h in before.items() if not Path(p).exists() or digest(Path(p)) != h]
    expected_names = ('MUS_Menu_EtherialPulse_01', 'SC_MenuTheme_Music')
    content = (HOST/'Content').resolve()
    new_content = [p.resolve() for p in content.rglob('*') if p.is_file() and str(p.resolve()) not in before]
    allowed = {destination.resolve()/(name+suffix) for name in expected_names for suffix in ('.uasset', '.uexp', '.ubulk')}
    unexpected = [str(p) for p in new_content if p not in allowed]
    summary = {'exit_code': code, 'reason': reason, 'protected_changes': changed, 'pid': process.pid,
               'new_content_files': [str(p) for p in new_content], 'unexpected_content_files': unexpected}
    (run/'guard_result.json').write_text(json.dumps(summary, indent=2)+'\n')
    assert code == 0 and reason == 'completed' and not changed and not unexpected, summary
    result = json.loads((run/'engine_result.json').read_text())
    assert result['reason'] == 'ok' and result['log']['errors'] == 0, result
    for name in expected_names:
        package = destination/(name+'.uasset')
        assert package.is_file() and package.stat().st_size > 0, 'Expected nonempty package missing: '+str(package)
    imported = json.loads((HOST/'Saved/AudioMusic/PrototypeV1/MenuTheme01/native_import.json').read_text())
    root = '/Game/TriadDrift/Audio/MenuTheme01/'
    assert imported['wave'] == root+expected_names[0]+'.'+expected_names[0], imported
    assert imported['music_class'] == root+expected_names[1]+'.'+expected_names[1], imported
    assert abs(imported['duration_s']-107.0) < 0.001 and imported['looping'] is False, imported
    print(json.dumps(summary))

if __name__ == '__main__': main()

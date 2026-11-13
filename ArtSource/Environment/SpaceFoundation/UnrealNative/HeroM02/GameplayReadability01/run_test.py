"""Source-only preflight by default. --execute requires a separately scheduled Lead grant.

Owns one editor process and one serial MCP client; never mounts, saves or edits host inputs.
After the first pair, an operator inspects the PNGs and writes initial_gate.json in this
unique run directory: {"readable": true, "reviewer": "...", "notes": "..."}.
False, absent after 90 seconds, or any error ends the run without extended testing.
"""
import argparse
import datetime as dt
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import statistics
import struct
import subprocess
import sys
import time
import zlib

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
HERO = HERE.parent
REPO = HERO.parents[4]
ENGINE = Path('C:/Program Files/Epic Games/UE_5.9/Engine')
HOST_ROOT = Path('C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/day1-integration')
MAP = '/Game/TriadDrift/Maps/L_Galaxy'
MATCH_FIELDS = ('world', 'world_time_seconds', 'camera_position', 'camera_rotation',
                'fov', 'pawn', 'sun_lux', 'sun_rotation', 'post_process', 'paused')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def git(root, *args):
    return subprocess.check_output(['git', '-C', str(root), *args], text=True).strip()


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


def png_info(path):
    """Read-only strict validation; never repair/reencode a viewport image."""
    data = path.read_bytes()
    assert data[:8] == b'\x89PNG\r\n\x1a\n', 'Invalid PNG signature'
    offset, dimensions, idat = 8, None, False
    while offset + 12 <= len(data):
        length = struct.unpack('>I', data[offset:offset + 4])[0]
        kind = data[offset + 4:offset + 8]
        end = offset + length + 12
        assert end <= len(data), 'Truncated PNG chunk'
        assert zlib.crc32(data[offset + 4:end - 4]) & 0xffffffff == struct.unpack('>I', data[end - 4:end])[0], 'PNG CRC mismatch'
        if dimensions is None:
            assert kind == b'IHDR' and length == 13
            dimensions = list(struct.unpack('>II', data[offset + 8:offset + 16]))
        if kind == b'IDAT':
            idat = True
        if kind == b'IEND':
            assert length == 0 and idat and end == len(data), 'Missing IDAT or PNG tail'
            return {'dimensions': dimensions, 'bytes': len(data), 'sha256': sha(path)}
        offset = end
    raise AssertionError('Missing PNG IEND')


def protected(root):
    paths = {root / 'unrealGameJam.uproject'}
    for relative in ('Config', 'Source', 'Content/TriadDrift/Maps',
                     'Content/__ExternalActors__', 'Content/__ExternalObjects__',
                     'Content/TriadDrift/Environment/SpaceFoundation'):
        directory = root / relative
        if directory.exists():
            paths.update(p for p in directory.rglob('*') if p.is_file())
    return {str(p.relative_to(root)).replace('\\', '/'): sha(p) for p in sorted(paths)}


def frozen():
    manifest = json.loads((HERO / 'Saved/Publication/HeroM02_checkpoint_manifest.json').read_text())
    assert len(manifest['files']) == 22
    for item in manifest['files']:
        assert sha(REPO / item['path']) == item['sha256'], 'Frozen checkpoint changed: ' + item['path']
    mounts = json.loads((HERE / 'mounts.json').read_text())
    assert len(mounts['packages']) == 6
    for item in mounts['packages']:
        assert sha(REPO / item['path']) == item['sha256']
    return mounts


def unwrap_value(result):
    assert not result.get('isError'), result
    if 'returnValue' in result.get('structuredContent', {}):
        return result['structuredContent']['returnValue']
    for block in result.get('content', []):
        if block.get('type') == 'text':
            value = json.loads(block['text'])
            if isinstance(value, dict) and 'returnValue' in value:
                return value['returnValue']
    raise RuntimeError('Tool response missing returnValue')


def strings(value):
    if isinstance(value, str):
        yield value
        try:
            yield from strings(json.loads(value))
        except (ValueError, RecursionError):
            pass
    elif isinstance(value, dict):
        for child in value.values():
            yield from strings(child)
    elif isinstance(value, list):
        for child in value:
            yield from strings(child)


def execute(args, project, mounts):
    for item in mounts['packages']:
        assert sha(project.parent / item['path']) == item['sha256'], 'Lead must mount exact package: ' + item['path']
    # Require original M01 native assets as well; no copy/import fallback.
    baseline = project.parent / 'Content/TriadDrift/Environment/SpaceFoundation/MI_TD_Space_Hero.uasset'
    assert baseline.is_file() and baseline.read_bytes()[:4] == bytes.fromhex('c1832a9e')
    preflight = json.loads(subprocess.check_output(['powershell', '-NoProfile', '-Command',
        "$os=Get-CimInstance Win32_OperatingSystem; "
        "$busy=@(Get-Process UnrealEditor*,blender*,ShaderCompileWorker*,cl -ErrorAction SilentlyContinue); "
        "@{free_gib=$os.FreePhysicalMemory/1MB;busy=@($busy|Select-Object Id,ProcessName)}|ConvertTo-Json -Depth 4"], text=True))
    assert preflight['free_gib'] >= 4 and not preflight['busy'], preflight
    out = HERO / 'Saved/GameplayReadability01' / dt.datetime.now(dt.timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
    out.mkdir(parents=True, exist_ok=False)
    report = {'stage': 'starting', 'lead_grant': args.lead_grant,
              'host_project': str(project), 'host_head': git(project.parent, 'rev-parse', 'HEAD'),
              'host_status_before': git(project.parent, 'status', '--porcelain'), 'preflight': preflight,
              'limits': ['Not final art approval, Main/map integration, Shipping or full-journey validation.',
                         'PIE restart is not a fresh-process reload.',
                         'Sparse live frame samples are not a causal FPS comparison.'],
              'events': [], 'captures': [], 'timings': {}}
    before = protected(project.parent)
    write_json(out / 'protected_before.json', before)
    spec = importlib.util.spec_from_file_location('hero_readonly_mcp_client', project.parent / 'Tools/td_mcp.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    client = module.McpClient('http://127.0.0.1:8024/mcp', timeout=45)
    log = out / 'Editor.log'
    bootstrap = ('import importlib.util,sys;sys.dont_write_bytecode=True;'
                 f"s=importlib.util.spec_from_file_location('hero_gameplay_probe',{str(HERE / 'probe.py')!r});"
                 "m=importlib.util.module_from_spec(s);sys.modules[s.name]=m;s.loader.exec_module(m)")
    argv = [str(ENGINE / 'Binaries/Win64/UnrealEditor.exe'), str(project), MAP,
            '-unattended', '-nosplash', '-nopause', '-nosound', '-NoLiveCoding', '-NoSourceControl',
            '-d3d12', '-sm6', '-ModelContextProtocolStartServer', '-ModelContextProtocolPort=8024',
            '-ExitTimeout=600', '-NoProgressTimeout=180', '-HangTimeout=90', '-UTF8Output',
            '-UserDir=' + str(out / 'RuntimeUser'),
            '-ini:EditorPerProjectUserSettings:/Script/UnrealEd.EditorLoadingSavingSettings:bAutoSaveEnable=False',
            '-ini:Engine:[DevOptions.Shaders]:bAllowCompilingThroughWorkers=True',
            '-ini:Engine:[DevOptions.Shaders]:NumUnusedShaderCompilingThreads=15',
            '-ini:Engine:[DevOptions.Shaders]:NumUnusedShaderCompilingThreadsDuringGame=15',
            '-ini:Engine:[DevOptions.Shaders]:ShaderCompilerCoreCountThreshold=999',
            "-ExecCmds=py exec(bytes.fromhex('" + bootstrap.encode().hex() + "').decode())",
            '-abslog=' + str(log), '-ResultFile=' + str(out / 'Editor.result.json'),
            '-UERunMarkerLog=' + str(out / 'Editor.markers.log')]
    write_json(out / 'launch.json', {'argv': argv, 'mounts': mounts, 'probe_sha256': sha(HERE / 'probe.py')})
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    environment = dict(os.environ, TD_HERO_RUN_DIR=str(out))
    process = subprocess.Popen(argv, startupinfo=startup, env=environment)
    print('EDITOR_PID', process.pid, 'OUTPUT', out, flush=True)
    probe_name = None
    pie = False
    prepared = False

    def save():
        write_json(out / 'report.json', report)

    def call(toolset, name, arguments):
        result = client.tool('call_tool', {'toolset_name': toolset, 'tool_name': name, 'arguments': arguments})
        assert not result.get('isError'), module.unwrap(result)
        return result

    def probe(action):
        result = json.loads(unwrap_value(call(probe_name, 'run', {'action': action})))
        report['events'].append({'probe': action, 'result': result})
        save()
        return result

    def command(text, settle=0.2):
        result = call('ConsoleToolset.ConsoleToolset', 'RunConsoleCommand', {'command': text})
        report['events'].append({'console': text, 'result': module.unwrap(result)})
        save()
        time.sleep(settle)

    def start_pie():
        nonlocal pie
        call('EditorToolset.EditorAppToolset', 'StartPIE', {'options': {
            'bSimulate': False, 'playMode': 'PlayMode_InEditorFloating', 'warmupSeconds': 2.0}})
        pie = True
        spawned = probe('spawn_live')
        time.sleep(0.8)  # unpaused game ticks permit both 0.5s fallback cleanup timers
        command('TD.UI.NativeHUD 1', 0.6)
        command('TD.UI.Status')
        text = log.read_text(encoding='utf-8-sig', errors='replace')
        status = re.findall(r'TD.UI.Status: enabled=\d shown=\d class=\S+ source=\S+', text)
        assert status and re.fullmatch(r'TD.UI.Status: enabled=1 shown=1 class=WBP_SpaceHUD_C source=\S*Squad\S*', status[-1]), status[-1:]
        state = probe('snapshot')
        assert state['world_time_seconds'] - spawned['spawn_world_time_seconds'] > 0.5
        return state

    def stop_pie():
        nonlocal pie
        probe('release_live_references')
        call('EditorToolset.EditorAppToolset', 'StopPIE', {})
        pie = False
        probe('confirm_world_stopped')

    def capture(name):
        pre = probe('snapshot')
        result = call('SceneCaptureToolset.SceneCaptureToolset', 'CaptureGameViewport', {
            'world': {'refPath': pre['world']}, 'maxResolution': 1920})
        metadata = unwrap_value(result)
        assert not metadata.get('warnings'), metadata.get('warnings')
        files = module.save_images(result, str(out / name))
        assert len(files) == 1, 'Expected one viewport PNG'
        png = png_info(Path(files[0]))
        assert png['dimensions'] == [1920, 1080], png
        post = probe('snapshot')
        if pre['paused']:
            assert all(pre[k] == post[k] for k in MATCH_FIELDS), 'View changed during paused capture'
        report['captures'].append({'name': name, 'files': files, 'png': png,
                                   'metadata': metadata, 'before': pre, 'after': post})
        save()
        return post

    def pair(name):
        probe('pause')
        probe('M01')
        time.sleep(0.3)
        first = capture(name + '_M01')
        probe('M02')
        time.sleep(0.3)
        second = capture(name + '_M02')
        assert all(first[k] == second[k] for k in MATCH_FIELDS), 'A/B settings or snapshot mismatch'
        assert first['material'].endswith('MI_TD_Space_Hero.MI_TD_Space_Hero')
        assert second['material'].endswith('MI_HeroM02_Benchmark.MI_HeroM02_Benchmark')

    try:
        deadline = time.monotonic() + 120
        while time.monotonic() < deadline:
            assert process.poll() is None, 'Editor exited before probe registration'
            try:
                if log.exists() and 'HERO_GAMEPLAY_PROBE_REGISTERED' in log.read_text(errors='replace'):
                    client.connect()
                    names = [s for s in strings(client.tool('list_toolsets')) if s.endswith('.HeroReadabilityProbe')]
                    if names:
                        probe_name = sorted(set(names))[0]
                        break
            except (OSError, module.McpError):
                pass
            time.sleep(0.5)
        assert probe_name, 'Probe registration/MCP readiness timeout; inspect before retry'
        # Keep actual schemas as evidence; abort rather than guess unsupported APIs.
        schemas = client.tool('describe_toolsets', {'toolset_names': [probe_name,
            'EditorToolset.EditorAppToolset', 'SceneCaptureToolset.SceneCaptureToolset',
            'PerfToolset.StatsToolset'], 'include_schemas': True})
        assert not schemas.get('isError'), module.unwrap(schemas)
        write_json(out / 'tool_schemas.json', module.unwrap(schemas))
        prepared = True  # cleanup must restore settings even if prepare raises halfway through
        probe('prepare')
        start_pie()
        report['lighting_inventory'] = probe('inventory')
        # Both shaders must settle before recording; this editor command does not save assets.
        command('Editor.AsyncTextureCompilationFinishAll', 1.0)
        command('TD.Input.Key T 0.1', 0.3)
        pair('01_Initial')
        report['stage'] = 'awaiting_initial_readability_review'
        save()
        print('INITIAL_PAIR_READY', out, flush=True)
        gate = out / 'initial_gate.json'
        deadline = time.monotonic() + 90
        while not gate.exists() and time.monotonic() < deadline:
            assert process.poll() is None
            time.sleep(0.5)
        assert gate.exists(), 'No initial readability decision; extended sequence skipped'
        decision = json.loads(gate.read_text(encoding='utf-8-sig'))
        report['initial_review'] = decision
        assert decision.get('readable') is True and decision.get('reviewer') and decision.get('notes'), 'Initial readability failed or review incomplete'
        probe('resume')
        # Sparse samples without captures/input RPCs mixed in. Same camera initially;
        # AI and waves advance, so these are workload-qualified observations only.
        for variant in ('M01', 'M02'):
            probe(variant)
            time.sleep(1)
            samples = []
            for _ in range(8):
                sample = unwrap_value(call('PerfToolset.StatsToolset', 'GetFrameTimings', {}))
                samples.append(sample)
                time.sleep(0.25)
            report['timings'][variant] = samples
            report.setdefault('timing_summary', {})[variant] = {
                key: {'sample_count': len(values), 'median_ms': statistics.median(values),
                      'maximum_ms': max(values), 'p95_nearest_rank_ms': sorted(values)[-1]}
                for key in samples[0]
                if key.lower().endswith('ms') and
                (values := [float(s[key]) for s in samples if isinstance(s.get(key), (int, float)) and s[key] > 0])
            }
            save()
        command('TD.Input.Keys W+LeftShift 0.75', 0.15)
        command('TD.Input.Key LeftMouseButton 0.5', 0.2)
        capture('02_Boost_Fire')
        time.sleep(0.6)
        command('TD.Input.ReleaseAll')
        for key, label in (('Two', 'Lancer'), ('Three', 'Bastion'), ('One', 'Rift')):
            command('TD.Input.Key ' + key + ' 0.1', 0.35)
            command('TD.Input.Key Q 0.1', 0.2)
            command('TD.UI.Status')
            capture('03_' + label)
        command('TD.Input.Mouse 8 -2 0.25', 0.5)
        pair('04_TurnedView')
        probe('remember_world')
        stop_pie()
        start_pie()
        report['restart'] = probe('verify_new_world')
        assert report['restart']['material'].endswith('MI_HeroM02_Benchmark.MI_HeroM02_Benchmark')
        capture('05_NewPIEWorld')
        report['stage'] = 'sequence_recorded_pending_visual_review'
    except Exception as error:
        report['stage'] = 'failed_or_stopped'
        report['failure'] = repr(error)
    finally:
        if process.poll() is None and probe_name:
            try:
                if pie:
                    command('TD.Input.ReleaseAll', 0)
                    # Cleanup should not depend on the gameplay/HUD snapshot still being valid.
                    stop_pie()
                if prepared:
                    probe('cleanup')
                try:
                    probe('quit')
                except (OSError, module.McpError):
                    pass  # exit may close HTTP before its response; actual exit remains authority
            except Exception as error:
                report['cleanup_error'] = repr(error)
        try:
            report['process_exit'] = process.wait(timeout=45)
        except subprocess.TimeoutExpired:
            # Only this process and its descendants, never a wildcard/name-based stop.
            subprocess.run(['taskkill', '/PID', str(process.pid), '/T', '/F'], capture_output=True)
            report['process_exit'] = process.wait(timeout=15)
            report['forced_termination'] = True
        after = protected(project.parent)
        write_json(out / 'protected_after.json', after)
        report['changed_protected_inputs'] = sorted(k for k in before.keys() | after.keys() if before.get(k) != after.get(k))
        report['host_status_after'] = git(project.parent, 'status', '--porcelain')
        try:
            frozen()
            report['frozen22_unchanged'] = True
        except Exception as error:
            report['frozen22_unchanged'] = False
            report['frozen_failure'] = repr(error)
        result_path = out / 'Editor.result.json'
        report['engine_result'] = json.loads(result_path.read_text(encoding='utf-8-sig')) if result_path.exists() else None
        digest = subprocess.run([sys.executable, '-B', str(ENGINE / 'Extras/AgentTools/ue_log_digest.py'),
                                 str(log), '--json'], capture_output=True, text=True)
        (out / 'digest.json').write_text(digest.stdout, encoding='utf-8')
        report['digest_exit'] = digest.returncode
        result = report['engine_result'] or {}
        report['technical_pass'] = bool(
            report['stage'] == 'sequence_recorded_pending_visual_review' and
            report.get('process_exit') == 0 and result.get('exit_code') == 0 and result.get('reason') == 'ok' and
            not result.get('suppressed_dialogs') and digest.returncode == 0 and
            not report.get('cleanup_error') and not report.get('forced_termination') and
            not report['changed_protected_inputs'] and report['frozen22_unchanged'] and
            report['host_status_before'] == report['host_status_after'])
        save()
        print('RESULT', report['stage'], 'TECHNICAL_PASS', report['technical_pass'], 'OUTPUT', out, flush=True)
    return 0 if report['technical_pass'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host-project', type=Path, required=True)
    parser.add_argument('--lead-grant', required=True)
    parser.add_argument('--execute', action='store_true')
    args = parser.parse_args()
    project = args.host_project.resolve(strict=True)
    assert project.parent == HOST_ROOT.resolve() and project.name == 'unrealGameJam.uproject', 'Only the Lead-designated compiled candidate is allowed'
    assert args.lead_grant.strip(), 'Fresh serialized Lead grant must be recorded'
    mounts = frozen()
    if not args.execute:
        print(json.dumps({'mode': 'SOURCE_PREFLIGHT_ONLY_NO_ENGINE', 'host': str(project),
                          'host_head': git(project.parent, 'rev-parse', 'HEAD'), 'mounts': mounts,
                          'missing_or_different_mounts': [item['path'] for item in mounts['packages']
                              if not (project.parent / item['path']).is_file() or sha(project.parent / item['path']) != item['sha256']],
                          'probe_sha256': sha(HERE / 'probe.py'), 'frozen22_unchanged': True}, indent=2))
        return 0
    return execute(args, project, mounts)


if __name__ == '__main__':
    raise SystemExit(main())

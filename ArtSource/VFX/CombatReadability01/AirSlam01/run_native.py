"""Isolated AirSlam stages. Dry by default; execute only in an exclusive slot.

The slot argument records scheduling, not an approval gate. Source/package
integrity and resource checks are mandatory on every actual native launch.
"""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import traceback

sys.dont_write_bytecode = True
import prepare_host as host

HERE, REPO = host.HERE, host.REPO
ENGINE = Path('C:/Program Files/Epic Games/UE_5.9/Engine')
spec = importlib.util.spec_from_file_location('air_readonly_resource_helpers', HERE / '../Saved/NativeDraft02/run_native.py')
resource = importlib.util.module_from_spec(spec)
spec.loader.exec_module(resource)
# Reuse read-only hash/RAM/protected-file helpers without editing their frozen source.
resource.ROOT, resource.HOST, resource.REPO = HERE, host.HOST, REPO
resource.PROJECT, resource.OUTPUT = host.PROJECT, host.OUTPUT
NAMES = {'M_AirSlam_Ring.uasset', 'M_AirSlam_Dust.uasset'}


def validate_outputs(stage, before, after):
    if stage in ('probe', 'verify-materials'):
        assert before == after, 'Read-only stage wrote native output'
    else:
        assert not NAMES.intersection(before), 'Preserve existing packages'
        assert set(after) - set(before) == NAMES, 'Unexpected native output set'
        assert all(after.get(key) == value for key, value in before.items()), 'Existing output changed'
        for name in NAMES:
            assert (host.OUTPUT / name).read_bytes()[:4] == bytes.fromhex('c1832a9e')


def stop_owned(process, out, reason):
    if process.poll() is not None:
        return
    identity = json.loads(resource.ps("Get-CimInstance Win32_Process -Filter 'ProcessId=" + str(process.pid)
        + "' | Select-Object ProcessId,ParentProcessId,CommandLine | ConvertTo-Json -Compress"))
    line = identity['CommandLine'].replace('\\', '/').lower()
    assert identity['ParentProcessId'] == os.getpid()
    assert str(host.PROJECT).replace('\\', '/').lower() in line
    assert '-modelcontextprotocolport=8024' in line
    assert str(out / 'RuntimeUser').replace('\\', '/').lower() in line
    resource.write(out / 'stop_identity.json', {'reason': reason, 'identity': identity})
    if process.poll() is None:
        subprocess.run(['taskkill', '/PID', str(process.pid), '/T', '/F'], capture_output=True, check=True, timeout=15)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--stage', choices=('probe', 'materials', 'verify-materials'), required=True)
    parser.add_argument('--slot', default='')
    parser.add_argument('--pins-sha256', default='')
    parser.add_argument('--execute', action='store_true')
    args = parser.parse_args()
    pins_path = HERE / 'Saved/native_pins.json'
    pins = json.loads(pins_path.read_text())
    assert all(resource.sha(HERE / path) == sha for path, sha in pins['files'].items()), 'Source integrity changed'
    if not args.execute:
        print(json.dumps({'stage': args.stage, 'native_executed': False, 'host': str(host.PROJECT),
                          'pins_sha256': resource.sha(pins_path), 'host_exists': host.PROJECT.exists()}, indent=2))
        return
    assert args.slot and args.pins_sha256 == resource.sha(pins_path), 'Slot identity and exact source integrity required'
    assert resource.sha(host.PROJECT) == pins['files']['host_descriptor.json']
    assert resource.sha(host.HOST / 'Config/DefaultEngine.ini') == pins['files']['host_Engine.ini']
    assert host.MOUNT.resolve() == host.OUTPUT.resolve() and host.MOUNT.is_dir()
    assert not json.loads(host.PROJECT.read_text()).get('Modules')
    busy = json.loads(resource.ps("@{busy=@(Get-CimInstance Win32_Process | Where-Object {$_.Name -match "
        "'(?i)^(UnrealEditor.*|unrealGameJam.*|blender.*|ShaderCompileWorker|UnrealBuildTool|cl|UnrealPak|dotnet)\\.exe$'} "
        "| Select-Object ProcessId,Name)} | ConvertTo-Json -Depth 4"))
    available = resource.free_ram()
    assert available >= 4.25 and not busy['busy'], (available, busy)
    before_owned, before = resource.owned(), resource.protected()
    if args.stage == 'materials':
        assert not NAMES.intersection(before_owned), 'Existing target: preserve, do not overwrite'
    if args.stage == 'verify-materials':
        assert NAMES.issubset(before_owned), 'Missing materials'
    out = HERE / 'Saved/NativeRuns' / (args.stage + '_' + str(time.time_ns()))
    out.mkdir(parents=True, exist_ok=False)
    resource.write(out / 'protected_before.json', before)
    resource.write(out / 'owned_before.json', before_owned)
    bound = 300 if args.stage == 'probe' else 420
    argv = [str(ENGINE / 'Binaries/Win64/UnrealEditor-Cmd.exe'), str(host.PROJECT), '-AgentMode',
        '-run=pythonscript', '-script=' + str(HERE / 'native_entry.py'),
        '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities,NiagaraToolsets,AllToolsets',
        '-AllowCommandletRendering', '-d3d12', '-sm6', '-DDC=AirSlamAuthoring', '-nosound',
        '-NoSourceControl', '-NoLiveCoding', '-ModelContextProtocolPort=8024',
        '-ExitTimeout=' + str(bound), '-NoProgressTimeout=120', '-HangTimeout=60',
        '-ini:Engine:[DevOptions.Shaders]:bAllowCompilingThroughWorkers=True',
        '-ini:Engine:[DevOptions.Shaders]:NumUnusedShaderCompilingThreads=15',
        '-ini:Engine:[DevOptions.Shaders]:NumUnusedShaderCompilingThreadsDuringGame=15',
        '-ini:Engine:[DevOptions.Shaders]:ShaderCompilerCoreCountThreshold=999',
        '-UserDir=' + str(out / 'RuntimeUser'), '-abslog=' + str(out / 'Engine.log'),
        '-ResultFile=' + str(out / 'Engine.result.json'), '-UERunMarkerLog=' + str(out / 'Engine.markers.log')]
    resource.write(out / 'launch.json', {'argv': argv, 'slot': args.slot, 'pins_sha256': resource.sha(pins_path),
                                       'free_gib': available, 'stage': args.stage})
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    environment = dict(os.environ, TD_AIR_SLAM_REPORT=str(out / 'Author'), TD_AIR_SLAM_STAGE=args.stage)
    report = {'stage': args.stage, 'technical_pass': False, 'native_executed': True,
              'niagara_system_complete': False, 'gameplay_bound': False, 'slot': args.slot}
    process = None
    samples, low_since = [available], None
    started = time.monotonic()
    try:
        with (out / 'stdout.log').open('w') as stdout, (out / 'stderr.log').open('w') as stderr:
            process = subprocess.Popen(argv, startupinfo=startup, env=environment, stdout=stdout, stderr=stderr)
            print('AIR_SLAM_PID', process.pid, 'OUTPUT', out, flush=True)
            while process.poll() is None:
                now, free = time.monotonic(), resource.free_ram()
                samples.append(free)
                low_since = (low_since if low_since is not None else now) if free < 1.75 else None
                sample = {'elapsed_s': now - started, 'free_gib': free,
                          'below_1_75_seconds': 0 if low_since is None else now - low_since}
                with (out / 'memory_samples.jsonl').open('a') as stream:
                    stream.write(json.dumps(sample) + '\n')
                if now - started >= bound + 60 or sample['below_1_75_seconds'] >= 10:
                    report['abort'] = 'outer_timeout' if now - started >= bound + 60 else 'sustained_memory_pressure'
                    stop_owned(process, out, report['abort'])
                    break
                time.sleep(2)
            process.wait(timeout=20)
        after, after_owned = resource.protected(), resource.owned()
        resource.write(out / 'owned_after.json', after_owned)
        report.update({'exit_code': process.returncode, 'minimum_free_gib': min(samples),
            'duration_s': time.monotonic() - started,
            'changed_protected': sorted(key for key in before.keys() | after.keys() if before.get(key) != after.get(key))})
        assert not report.get('abort') and process.returncode == 0 and before == after
        validate_outputs(args.stage, before_owned, after_owned)
        engine_result = json.loads((out / 'Engine.result.json').read_text(encoding='utf-8-sig'))
        resource.check_engine(engine_result)
        report['engine_result_summary'] = {key: engine_result[key] for key in ('exit_code', 'reason', 'log')}
        digest = subprocess.run([sys.executable, '-B', str(ENGINE / 'Extras/AgentTools/ue_log_digest.py'),
            str(out / 'Engine.log'), '--json'], capture_output=True, text=True, timeout=30)
        (out / 'digest.json').write_text(digest.stdout, encoding='utf-8')
        report['digest_exit'] = digest.returncode
        assert digest.returncode == 0
        result = json.loads((out / 'Author/result.json').read_text())
        assert result['stage'] == args.stage and not result['niagara_system_authored'] and not result['gameplay_bound']
        expected = set() if args.stage == 'probe' else {
            '/Game/TriadDrift/VFX/CombatReadability01/AirSlam01/' + name[:-7] + '.' + name[:-7] for name in NAMES}
        assert set(result['assets']) == expected
        assert all(resource.sha(HERE / path) == sha for path, sha in pins['files'].items())
        report['structured_result'] = result
        report['technical_pass'] = True
    except Exception:
        report['failure'] = traceback.format_exc()
        raise
    finally:
        if process is not None and process.poll() is None:
            stop_owned(process, out, 'wrapper_finalization')
            process.wait(timeout=20)
        if 'changed_protected' not in report:
            # Preserve boundary evidence even when the native stage fails early.
            try:
                after = resource.protected()
                report['changed_protected'] = sorted(key for key in before.keys() | after.keys()
                                                     if before.get(key) != after.get(key))
                resource.write(out / 'owned_after.json', resource.owned())
            except Exception:
                report['post_failure_snapshot_error'] = traceback.format_exc()
        resource.write(out / 'validation.json', report)


if __name__ == '__main__':
    main()

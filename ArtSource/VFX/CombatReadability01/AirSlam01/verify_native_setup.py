"""Offline authoring boundaries. No native executable, RAM query or asset write."""
import ast
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import re
import sys
from types import SimpleNamespace
from unittest import mock

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
spec = importlib.util.spec_from_file_location('air_native_guard_check', HERE / 'run_native.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
checks = []


def rejects(name, fn):
    try:
        fn()
    except AssertionError:
        checks.append(name)
        return
    raise AssertionError('Unsafe case accepted: ' + name)


for file in HERE.glob('*.py'):
    ast.parse(file.read_text())
checks.append('Python syntax parses without importing Unreal')
assert runner.resource.sha(HERE / '../Saved/NativeDraft02/run_native.py') == 'e10f617d9cf68938c7d6dacb22392f467b773a9c9308e2da5b7117bb9ebab9c3'
checks.append('Frozen resource helper unchanged')
with mock.patch.object(runner.resource, 'free_ram', side_effect=AssertionError('No dry RAM query')), \
        mock.patch.object(runner.subprocess, 'Popen', side_effect=AssertionError('No dry process launch')), \
        mock.patch.object(runner.resource, 'ps', side_effect=AssertionError('No dry shell process')):
    for stage in ('probe', 'materials', 'verify-materials'):
        output = io.StringIO()
        with mock.patch.object(sys, 'argv', ['run_native.py', '--stage', stage]), contextlib.redirect_stdout(output):
            runner.main()
        assert json.loads(output.getvalue())['native_executed'] is False
        checks.append(stage + ': dry never launches or queries RAM')
    with mock.patch.object(sys, 'argv', ['run_native.py', '--stage', 'probe', '--execute', '--slot', 'fixture', '--pins-sha256', 'wrong']):
        rejects('Integrity mismatch stops before resource/process operations', runner.main)
runner.validate_outputs('probe', {}, {})
checks.append('Read-only probe has zero native outputs')
rejects('Probe writes rejected', lambda: runner.validate_outputs('probe', {}, {'unexpected.uasset': 'x'}))
rejects('Verification writes rejected', lambda: runner.validate_outputs('verify-materials', {'x': 'old'}, {'x': 'new'}))
rejects('Materials unexpected output rejected', lambda: runner.validate_outputs('materials', {}, {'extra.uasset': 'x'}))
rejects('Existing native target never overwritten', lambda: runner.validate_outputs('materials', {'M_AirSlam_Ring.uasset': 'old'}, {}))
with mock.patch.object(Path, 'read_bytes', return_value=bytes.fromhex('c1832a9e')):
    runner.validate_outputs('materials', {}, {name: 'hash' for name in runner.NAMES})
    checks.append('Only exact two new package names accepted')
with mock.patch.object(Path, 'read_bytes', return_value=b'bad!'):
    rejects('Invalid package header rejected', lambda: runner.validate_outputs('materials', {}, {name: 'hash' for name in runner.NAMES}))
fake_process = SimpleNamespace(pid=99999, poll=lambda: None)
with mock.patch.object(runner.resource, 'ps', return_value=json.dumps({'ParentProcessId': -1, 'CommandLine': 'foreign'})), \
        mock.patch.object(runner.subprocess, 'run', side_effect=AssertionError('Cannot kill a foreign process')):
    rejects('Wrong parent PID cannot be killed', lambda: runner.stop_owned(fake_process, HERE / 'Saved', 'fixture'))
config = (HERE / 'host_Engine.ini').read_text()
ddc = re.search(r'Path="([^"]+)"', config)[1]
assert len(ddc) < 119
assert runner.host.MOUNT.resolve() == runner.host.OUTPUT.resolve()
assert not json.loads(runner.host.PROJECT.read_text()).get('Modules')
checks.append('Only owned output mounted into an engine-only host; short private DDC path')
report = {'native_executed': False, 'checks': checks, 'count': len(checks),
          'source_pins_sha256': runner.resource.sha(HERE / 'Saved/native_pins.json')}
(HERE / 'Saved/native_setup_check.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print(json.dumps(report, indent=2))

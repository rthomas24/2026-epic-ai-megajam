"""Prepare ONLY this task's engine-only host. No native launch or Git writes."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]
HOST = HERE / 'Saved/H00'
PROJECT = HOST / 'AirSlamAuthoring.uproject'
OUTPUT = REPO / 'Content/TriadDrift/VFX/CombatReadability01/AirSlam01'
MOUNT = HOST / 'Content/TriadDrift/VFX/CombatReadability01/AirSlam01'
PINNED = ('native_entry.py', 'run_native.py', 'prepare_host.py', 'host_descriptor.json',
          'host_Engine.ini', 'air_slam_ring.ush', 'air_slam_dust.ush', 'slam.json',
          '../Saved/NativeDraft02/run_native.py')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare():
    assert OUTPUT.is_relative_to(REPO / 'Content/TriadDrift/VFX/CombatReadability01')
    assert HOST.is_relative_to(HERE / 'Saved')
    for source, target in ((HERE / 'host_descriptor.json', PROJECT),
                           (HERE / 'host_Engine.ini', HOST / 'Config/DefaultEngine.ini')):
        target.parent.mkdir(parents=True, exist_ok=True)
        if target.exists():
            assert target.read_bytes() == source.read_bytes(), 'Preserve changed private host: ' + str(target)
        else:
            target.write_bytes(source.read_bytes())
    OUTPUT.mkdir(parents=True, exist_ok=True)
    MOUNT.parent.mkdir(parents=True, exist_ok=True)
    if not MOUNT.exists():
        # Literal environment values; never interpolate filesystem paths as shell code.
        import os
        env = dict(os.environ, TD_AIR_MOUNT=str(MOUNT), TD_AIR_OUTPUT=str(OUTPUT))
        subprocess.run(['powershell', '-NoProfile', '-Command',
            "New-Item -ItemType Junction -Path $env:TD_AIR_MOUNT -Value $env:TD_AIR_OUTPUT -ErrorAction Stop | Out-Null"],
            env=env, check=True, timeout=15)
    assert MOUNT.resolve() == OUTPUT.resolve(), 'Wrong output mount'
    assert not json.loads(PROJECT.read_text()).get('Modules'), 'Engine-only host required'
    return {'project': str(PROJECT), 'mount': str(MOUNT), 'output': str(OUTPUT), 'native_executed': False}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--prepare', action='store_true')
    args = parser.parse_args()
    pins = {'kind': 'SOURCE INTEGRITY / EXECUTION REQUIRES A RESOURCE SLOT',
            'files': {relative: sha(HERE / relative) for relative in PINNED}}
    target = HERE / 'Saved/native_pins.json'
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(pins, indent=2) + '\n', encoding='utf-8')
    result = prepare() if args.prepare else {'project': str(PROJECT), 'host_created': PROJECT.exists(), 'native_executed': False}
    result['pins_sha256'] = sha(target)
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()

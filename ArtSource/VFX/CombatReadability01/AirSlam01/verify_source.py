"""Offline source/compiler checks only; never launches Unreal or edits assets."""
import hashlib
import json
import math
from pathlib import Path
import subprocess
import time

HERE = Path(__file__).resolve().parent
config = json.loads((HERE / 'slam.json').read_text())
assert config['stage'] == 'source_preview_only'
assert config['ring']['radius_cm'] == 300
assert sum(config[k]['spawn_count'] for k in ('ring', 'dust', 'debris', 'glints')) == 35
assert config['runtime_prerequisites']['do_not_assume_fixed_offset'] is True
radii = [300 * (1 - math.exp(-i / 1000 / .095)) for i in range(421)]
assert radii[0] == 0 and all(a <= b < 300 for a, b in zip(radii, radii[1:]))
assert config['dust']['opacity_max'] <= .21
out = HERE / 'Saved' / ('SourceCheck_' + str(time.time_ns()))
out.mkdir(parents=True, exist_ok=False)
dxc = Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/dxc.exe')
assert dxc.is_file(), 'Standalone compiler required for this source check'
results = {}
for name, arguments in (
    ('air_slam_ring', 'float2 UV, float Age, float3 Tint, float Intensity'),
    ('air_slam_dust', 'float2 UV, float Age, float Variation, float3 Tint')):
    kernel = (HERE / (name + '.ush')).read_text()
    wrapper = 'float4 kernel(' + arguments + ') {\n' + kernel + '\n}\n'
    call = 'kernel(uv,.3,float3(.04,.65,.9),1.0)' if 'ring' in name else 'kernel(uv,.3,.47,float3(.3,.26,.2))'
    wrapper += 'float4 main(float2 uv:TEXCOORD0):SV_Target {return ' + call + ';}\n'
    source = out / (name + '.hlsl')
    source.write_text(wrapper, encoding='utf-8')
    result = subprocess.run([str(dxc), '-T', 'ps_6_0', '-E', 'main', '-WX',
        '-Fo', str(out / (name + '.dxil')), str(source)], capture_output=True, text=True, timeout=20)
    results[name] = {'exit': result.returncode, 'stdout': result.stdout, 'stderr': result.stderr}
    assert result.returncode == 0, results[name]
files = [file for file in HERE.iterdir() if file.is_file()]
report = {'stage': 'OFFLINE_SOURCE_ONLY', 'native_executed': False,
    'compiler': str(dxc), 'shader_checks': results, 'recipe_budget_count': 35,
    'placement_contract': 'Actual floor contact and normal remain owner integration prerequisites',
    'sha256': {file.name: hashlib.sha256(file.read_bytes()).hexdigest() for file in files},
    'limits': ['No Unreal/Niagara material compile or binding yet.',
               'No real GPU timing, shipping or gameplay acceptance.',
               'Motion preview approximates dust shading and theme colors.']}
(out / 'source_check.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
print(json.dumps({'source_check': str(out / 'source_check.json'), 'shaders_pass': True,
                  'native_executed': False}, indent=2))

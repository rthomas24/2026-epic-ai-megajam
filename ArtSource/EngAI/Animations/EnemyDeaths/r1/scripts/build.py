import json, subprocess, sys, os
from concurrent.futures import ThreadPoolExecutor
os.chdir('<work>')
C = json.load(open('configs.json')); CH = C.pop('_chars')
only = sys.argv[1:] or list(C)
B = '/home/box/.local/bin/blender'
def one(name):
    c = dict(CH[C[name]['char']]); c.update(C[name]); c['name'] = name
    c.setdefault('rootmotion', True); c.setdefault('grip', 0.6); c.setdefault('vthresh', 3.0); c.setdefault('usf1', False)
    src = c.pop('src')
    r = subprocess.run([B,'-b','--factory-startup','--python','py/retarget.py','--',f'raw/{src}.fbx',f'final/{name}.fbx',json.dumps(c)], capture_output=True, text=True)
    res = [l for l in r.stdout.splitlines() if l.startswith('RESULT')]
    if not res: return name, {'error': (r.stdout[-1500:]+r.stderr[-1500:])}
    subprocess.run(f'rm -rf prev/{name}; {B} -b final/{name}.blend --python py/render.py -- prev/{name} 8 >prev/{name}.log 2>&1', shell=True)
    return name, json.loads(res[0][7:])
M = json.load(open('final/metrics.json')) if os.path.exists('final/metrics.json') else {}
with ThreadPoolExecutor(4) as ex:
    for name, res in ex.map(one, only):
        print(name, json.dumps(res), flush=True); M[name] = res
json.dump(M, open('final/metrics.json','w'), indent=1)

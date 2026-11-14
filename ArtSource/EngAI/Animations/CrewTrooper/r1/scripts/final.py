import json, subprocess, os
from concurrent.futures import ThreadPoolExecutor
cfg = json.load(open('configs.json')); picks = json.load(open('picks.json'))
os.makedirs('final', exist_ok=True)
def one(p):
    name, raw, key = p
    c = dict(cfg[key]); c['name'] = name
    B = '/home/box/.local/bin/blender'
    r = subprocess.run([B,'-b','--factory-startup','--python','retarget.py','--','raw/'+raw+'.fbx','final/'+name+'.fbx',json.dumps(c)], capture_output=True, text=True)
    res = [l for l in r.stdout.splitlines() if l.startswith('RESULT')]
    subprocess.run(f'rm -rf /tmp/f_{name}; {B} -b final/{name}.blend --python render.py -- /tmp/f_{name} 8 >/dev/null 2>&1', shell=True)
    return name, res[0][7:] if res else r.stdout[-800:]+r.stderr[-800:]
out = {}
with ThreadPoolExecutor(4) as ex:
    for name, res in ex.map(one, picks):
        print(name, res, flush=True)
        try: out[name] = json.loads(res)
        except Exception: out[name] = res
json.dump(out, open('final/metrics.json','w'), indent=1)

import json, subprocess, sys, os, glob
from concurrent.futures import ThreadPoolExecutor
cfg = json.load(open('configs.json'))
todo = sys.argv[1:] or sorted(os.path.basename(p)[:-4] for p in glob.glob('raw/*.fbx'))
def one(raw):
    clip = raw.split('-')[0]
    c = dict(cfg[clip]); c['name'] = 'c_' + raw
    r = subprocess.run(['./run1.sh', raw, 'c_'+raw, json.dumps(c), '8'], capture_output=True, text=True)
    return raw, r.stdout.strip()[-1500:] + r.stderr.strip()[-500:]
with ThreadPoolExecutor(4) as ex:
    for raw, out in ex.map(one, todo):
        print('==', raw, out, flush=True)

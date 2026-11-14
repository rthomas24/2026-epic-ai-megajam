import json,subprocess,sys
C=json.load(open('/tmp/wd/clips.json'))
only=sys.argv[1:] or list(C)
TG='/workspace/wt-warden-fs/ArtSource/EngCharacters/Warden/export/SK_Warden.fbx'
ps=[]
for name in only:
    a=dict(C[name]); src=a.pop('src')
    a.update({"src":f"/tmp/wd/raw/{src}.fbx","tgt":TG,"out":f"/tmp/wd/final/{name}.fbx","blend":f"/tmp/wd/final/{name}.blend","name":name,"yaw_keep":1.0,"fist":0,"contact_speed":300,"contact_dz":10,"pin_first":False,"release_out":15})
    ps.append(subprocess.Popen(['blender','-b','--factory-startup','--python','/tmp/wd/py/retarget.py','--',json.dumps(a)],stdout=open(f'/tmp/wd/final/{name}.log','w'),stderr=subprocess.STDOUT))
for p in ps: p.wait()

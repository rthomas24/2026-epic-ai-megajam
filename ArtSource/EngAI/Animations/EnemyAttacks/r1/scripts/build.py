import json,subprocess,sys
C=json.load(open('/tmp/wd_atk/clips.json'))
only=sys.argv[1:] or list(C)
T={'heavy':'/tmp/wt-enemy-attacks/ArtSource/EngCharacters/Crew/export/NullHeavy/SK_NullHeavy.fbx','warden':'/tmp/wt-enemy-attacks/ArtSource/EngCharacters/Warden/export/SK_Warden.fbx','grunt':'/tmp/wt-enemy-attacks/ArtSource/EngCharacters/Crew/export/NullGrunt/SK_NullGrunt.fbx'}
ps=[]
for name in only:
    a=dict(C[name]); src=a.pop('src'); tg=a.pop('tgt')
    base={"yaw_keep":1.0,"contact_speed":300,"contact_dz":10,"pin_first":False}
    base.update(a)
    base.update({"src":f"/tmp/wd_atk/raw/{src}.fbx","tgt":T[tg],"out":f"/tmp/wd_atk/final/{name}.fbx","blend":f"/tmp/wd_atk/final/{name}.blend","name":name})
    ps.append(subprocess.Popen(['/home/box/.local/bin/blender','-b','--factory-startup','--python','/tmp/wd_atk/py/retarget.py','--',json.dumps(base)],stdout=open(f'/tmp/wd_atk/final/{name}.log','w'),stderr=subprocess.STDOUT))
for p in ps: p.wait()

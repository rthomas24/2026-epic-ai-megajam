"""Bounded native job; preserves every host file except the exact nine Waves."""
from pathlib import Path
import importlib.util,json,subprocess,sys,time,shutil
ROOT=Path(__file__).resolve().parent
# Pin the explicit known compatible host.
HOST=Path('C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-tutorial-production-64490fb')
mode=sys.argv[1];assert mode in ('import','reload')
run=HOST/'Saved/PR12Provided01'/('Import01' if mode=='import' else 'Reload02')
assert not run.exists()
gf=HOST.parent/'audio-prototype-v1/ArtSource/Audio/PrototypeV1/MenuTheme01/guarded_import.py'
l=importlib.util.spec_from_file_location('guard',gf);g=importlib.util.module_from_spec(l);l.loader.exec_module(g)
assert g.digest(gf)=='85340b2eae4453e943c708601686bcfc389068e41901837a4779182585182ed5'
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=HOST,text=True).strip()=='28f03b4f788ed25ddf2b3a5cb406d366b953a05b'
assert g.digest(HOST/'Binaries/Win64/UnrealEditor-unrealGameJam.dll')=='3c034cf024ac91d9e7546fd1935decbe26e9401706be02227ac05b2c6a676c24'
freeze=json.loads((ROOT.parent/'native_package_freeze.json').read_text())['packages']
if mode=='import':
 for r in freeze:assert g.digest(HOST/r['path'])==r['sha256'],r['path']
g.idle_guard()
files=[HOST/'unrealGameJam.uproject']+[p for folder in ('Source','Config','Content','Binaries/Win64','Saved/SaveGames') for p in (HOST/folder).rglob('*') if p.is_file()]
files += [p for p in (HOST/'Plugins').rglob('*') if p.is_file() and not any(x in ('Intermediate','Saved','DerivedDataCache') for x in p.relative_to(HOST/'Plugins').parts)]
before={str(p):g.digest(p) for p in files}
manifest=json.loads((ROOT/'replacement_manifest.json').read_text())
allowed={str(HOST/('Content/'+r['asset_path'].removeprefix('/Game/')+'.uasset')) for r in manifest['rows']}
run.mkdir(parents=True);(run/'protected_before.json').write_text(json.dumps(before,indent=2))
if mode=='import':shutil.copytree(HOST/'Content/TriadDrift/Audio/Combat01',run/'original_Combat01')
spec=dict(host=str(HOST),source=str(ROOT),run=str(run),mode=mode,claim='98c956c9',import_run=str(run.parent/'Import01'))
boot=run/'bootstrap.py';boot.write_text('SPEC='+repr(spec)+'\nexec(compile(open('+repr(str(ROOT/'native_replace.py'))+',encoding="utf-8").read(),"pr12_replacement","exec"))')
g.idle_guard();start=time.monotonic();minimum=g.free_gib();low=None;reason='completed'
args=[str(g.ENGINE),str(HOST/'unrealGameJam.uproject'),'/Engine/Maps/Entry','-AgentMode','-nullrhi','-NonRealtimeAudio','-NoLiveCoding','-run=pythonscript','-script='+str(boot),'-ModelContextProtocolPort=8025','-ExitTimeout=180','-NoProgressTimeout=60','-ResultFile='+str(run/'engine_result.json'),'-abslog='+str(run/'engine.log')]
p=subprocess.Popen(args,creationflags=subprocess.CREATE_NO_WINDOW)
print('START PR12',mode,p.pid,'freeGiB',minimum,flush=True)
try:
 while p.poll() is None:
  now=time.monotonic();free=g.free_gib();minimum=min(minimum,free);low=(low or now) if free<1.5 else None
  if now-start>240 or (low and now-low>=10):reason='timeout_or_low_memory';break
  time.sleep(1)
finally:
 if p.poll() is None:subprocess.run(['taskkill','/PID',str(p.pid),'/T','/F'],capture_output=True,timeout=15)
 code=p.wait(timeout=20)
changed=[f for f,h in before.items() if not Path(f).exists() or g.digest(Path(f))!=h]
new=[str(p) for p in (HOST/'Content').rglob('*') if p.is_file() and str(p) not in before]
r=dict(pid=p.pid,exit_code=code,reason=reason,duration_s=time.monotonic()-start,minimum_free_gib=minimum,protected_count=len(before),changed=changed,new_content=new)
(run/'guard_result.json').write_text(json.dumps(r,indent=2));print(json.dumps(r),flush=True)
assert code==0 and reason=='completed' and set(changed)==(allowed if mode=='import' else set()) and not new,r
engine=json.loads((run/'engine_result.json').read_text());assert engine['reason']=='ok' and engine['log']['errors']==0
assert (run/'native.json').exists()

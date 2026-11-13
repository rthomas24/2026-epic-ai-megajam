"""Prepared bounded native import; run only with Lead's exact resource/claim grant."""
import importlib.util,json,subprocess,time,hashlib
from pathlib import Path
ROOT=Path('C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-prototype-v1')
HOST=ROOT.parent/'day1-integration';SRC=ROOT/'Saved/AudioMusic/PrototypeV1/CombatMusic01'
RUN=HOST/'Saved/AudioMusic/CombatMusic01/Reload01'
guard_file=ROOT/'ArtSource/Audio/PrototypeV1/MenuTheme01/guarded_import.py'
assert hashlib.sha256(guard_file.read_bytes()).hexdigest()=='85340b2eae4453e943c708601686bcfc389068e41901837a4779182585182ed5'
loader=importlib.util.spec_from_file_location('guard',guard_file)
guard=importlib.util.module_from_spec(loader);loader.loader.exec_module(guard)
assert guard.digest(SRC/'import_combat_music_02.py')=='c418079939bef70405abf358350a5cdb23b9a4b4a446ac9c7b6c14f7b3ac1b9a'
assert guard.digest(SRC/'manifest_02.json')=='c7a6555880532af4d7e254b589c5bfb9939907765960f4ec8d1c78df2eee26d7'
manifest=json.loads((SRC/'manifest_02.json').read_text());assert guard.digest(Path(manifest['prepared_file']))==manifest['sha256']
names=('MUS_Combat_EtherealPulseRemix_01',)
targets={HOST/'Content/TriadDrift/Audio/CombatMusic01'/(n+'.uasset') for n in names}
assert all(p.exists() for p in targets) and not RUN.exists()
assert guard.digest(HOST/'Content/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music.uasset')==manifest['music_class_sha256']
assert not (HOST/'Saved/AudioMusic/CombatMusic01/reload_01.json').exists()
verifier=ROOT/'ArtSource/Audio/CombatMusic01/verify_combat_music.py'
assert guard.digest(verifier)=='5f4587694dd06fecd6d96c081407e170a3820c25d23e42a88fe9218af25f08b3'
guard.idle_guard()
files=[HOST/'unrealGameJam.uproject']+[p for folder in ('Source','Config','Content','Binaries/Win64','Saved/SaveGames') for p in (HOST/folder).rglob('*') if p.is_file()]
before={str(p):guard.digest(p) for p in files}
RUN.mkdir(parents=True);(RUN/'protected_before.json').write_text(json.dumps(before,indent=2))
grant={'execute':True,'project_dir':str(HOST),'manifest':str(SRC/'manifest_02.json')}
boot=RUN/'bootstrap.py';boot.write_text('SPEC='+repr(grant)+'\nexec(compile(open('+repr(str(verifier))+", encoding='utf-8').read(), 'import_combat_music', 'exec'))\n")
args=[str(guard.ENGINE),str(HOST/'unrealGameJam.uproject'),'/Engine/Maps/Entry','-AgentMode','-nullrhi','-NonRealtimeAudio','-NoLiveCoding','-ModelContextProtocolPort=8025','-run=pythonscript','-script='+str(boot),'-ExitTimeout=180','-NoProgressTimeout=60','-ResultFile='+str(RUN/'engine_result.json'),'-abslog='+str(RUN/'engine.log')]
guard.idle_guard();minimum=guard.free_gib();start=time.monotonic();low=None;reason='completed'
proc=subprocess.Popen(args,creationflags=subprocess.CREATE_NO_WINDOW)
monitor_error=None;cleanup_error=None;cleanup_return_code=None
try:
 print('START COMBAT MUSIC READONLY RELOAD PID',proc.pid,'free GiB',minimum,flush=True)
 (RUN/'launch.json').write_text(json.dumps({'pid':proc.pid,'args':args,'epoch':time.time()},indent=2))
 while proc.poll() is None:
  now=time.monotonic();free=guard.free_gib();minimum=min(minimum,free);low=(low or now) if free<1.5 else None
  if now-start>240 or (low is not None and now-low>=10):
   reason='timeout_or_low_memory';break
  time.sleep(1)
except BaseException as error:
 reason='monitor_exception';monitor_error=repr(error);print('MONITOR ERROR',monitor_error,flush=True)
finally:
 if proc.poll() is None:
  try:
   killed=subprocess.run(['taskkill','/PID',str(proc.pid),'/T','/F'],capture_output=True,text=True,timeout=15)
   cleanup_return_code=killed.returncode
   if killed.returncode and proc.poll() is None:cleanup_error=killed.stderr.strip() or killed.stdout.strip()
  except BaseException as error:cleanup_error=repr(error)
  if proc.poll() is None:proc.kill()
 code=proc.wait(timeout=20);after={p:guard.digest(Path(p)) if Path(p).exists() else None for p in before}
new=[p for p in (HOST/'Content').rglob('*') if p.is_file() and str(p) not in before]
summary={'pid':proc.pid,'exit_code':code,'reason':reason,'monitor_error':monitor_error,'cleanup_error':cleanup_error,'cleanup_return_code':cleanup_return_code,'minimum_free_gib':minimum,'duration_s':time.monotonic()-start,'protected_count':len(before),'protected_unchanged':before==after,'changed_protected':[p for p in before if before[p]!=after[p]],'new_content':{str(p):guard.digest(p) for p in new},'unexpected_content':[str(p) for p in new if p not in targets]}
(RUN/'guard_result.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary),flush=True)
assert code==0 and reason=='completed' and monitor_error is None and cleanup_error is None and before==after and not new,summary
engine=json.loads((RUN/'engine_result.json').read_text());assert engine['reason']=='ok' and engine['log']['errors']==0,engine
r=json.loads((HOST/'Saved/AudioMusic/CombatMusic01/reload_01.json').read_text())
assert abs(r['duration_s']-139.2)<.001 and r['properties']['looping'] is True,r
print('RELEASE: combat music fresh reload verified; runtime binding pending',flush=True)

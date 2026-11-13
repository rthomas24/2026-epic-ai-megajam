"""Attach-only approved-game probes. No launch, asset save, direct audio play or director override."""
import argparse,json,sys,time
from pathlib import Path

p=argparse.ArgumentParser()
p.add_argument('--project-root',type=Path,required=True)
p.add_argument('--out',type=Path,required=True)
p.add_argument('--port',type=int,required=True)
p.add_argument('--phase',choices=['space','loop','ground','cleanup'],required=True)
p.add_argument('--world',default='L_Galaxy')
p.add_argument('--xyz',nargs=3,type=float)
a=p.parse_args()
if a.phase=='ground' and not a.xyz:p.error('Inspect the authored encounter location first; --xyz is required for the real overlap trial')
host=a.project_root.resolve();out=a.out.resolve()
assert out.is_relative_to(host/'Saved') and not out.exists()
out.mkdir(parents=True)
sys.path.insert(0,str(host/'Tools'))
from td_mcp import McpClient,unwrap,save_images
c=McpClient('http://127.0.0.1:%d/mcp'%a.port,timeout=30);c.connect()
events=[]
def console(command):
 r=c.tool('call_tool',{'toolset_name':'ConsoleToolset.ConsoleToolset','tool_name':'RunConsoleCommand','arguments':{'command':command}})
 assert not r.get('isError'),r
 events.append({'epoch':time.time(),'command':command,'result':unwrap(r)})
 (out/'commands.json').write_text(json.dumps(events,indent=2))
def snapshot(label,capture=True,world=None):
 for command in ('TD.Flow.Status','au.DumpActiveSounds','getall AudioComponent Sound','getall AudioComponent VolumeMultiplier','getall AudioComponent bIsActive'):
  console(command)
 if capture:
  name=world or a.world
  r=c.tool('call_tool',{'toolset_name':'SceneCaptureToolset.SceneCaptureToolset','tool_name':'CaptureGameViewport','arguments':{'world':{'refPath':'/Game/TriadDrift/Maps/'+name+'.'+name},'maxResolution':1280}})
  assert not r.get('isError'),r
  save_images(r,str(out/'captures'/label));(out/(label+'.json')).write_text(json.dumps(unwrap(r),indent=2))
 print(label,flush=True)
def wait(seconds):
 end=time.monotonic()+seconds
 while time.monotonic()<end:
  time.sleep(min(25,max(0,end-time.monotonic())))
  if time.monotonic()<end:snapshot('heartbeat',False)
console('au.DisableAppVolume 1');console('TD.God')
if a.phase=='space':
 console('TD.Squad.Passive 1');snapshot('initial');wait(4);snapshot('wave_started')
 console('TD.KillAll');snapshot('after_real_enemy_clear',False)
 wait(4);snapshot('hold_or_next_wave');wait(7);snapshot('post_clear_or_next_wave')
 console('TD.Take');wait(1.2);snapshot('cut_stop',False);wait(4);snapshot('retake')
elif a.phase=='loop':
 console('TD.Squad.Passive 1');console('TD.Phase DominionAssault')
 wait(3);snapshot('dominion_phase_music')
 wait(142);snapshot('past_full_track_duration')
elif a.phase=='ground':
 snapshot('ground_before_encounter')
 console('getall TDEncounterVolume Box');console('getall BoxComponent RelativeLocation')
 console('TD.Ground.Teleport '+' '.join(str(v) for v in a.xyz));wait(3);snapshot('actual_overlap_music')
 console('TD.KillAll');wait(4);snapshot('ground_hold');wait(7);snapshot('ground_post_clear')
else:
 snapshot('before_victory');console('TD.Phase Victory');wait(1.5);snapshot('victory_stop')
 console('open /Game/TriadDrift/Maps/L_MainMenu');wait(5);snapshot('menu_cleanup',world='L_MainMenu')
(out/'probe_complete.json').write_text(json.dumps({'phase':a.phase,'stage':'Commands/probes executed; inspect actual voice, state, log and capture evidence before accepting results','scope':'No source/map/asset edits or fake audio calls. Dominion/Victory use public gameplay-phase helpers; menu cleanup uses console travel, not a pause-menu fade acceptance.'},indent=2))
print('Probe complete; the separately granted native driver owns shutdown and resource release.',flush=True)

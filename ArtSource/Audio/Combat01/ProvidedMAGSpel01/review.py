"""Measure supplied-derived takes and build a local, non-autoplay listening page."""
from pathlib import Path
import sys,os,json,wave,html
import numpy as np
ROOT=Path(__file__).resolve().parent
SCRIPTS=ROOT.parents[3]/'.claude/skills/ue-game-audio/scripts'
sys.path.insert(0,str(SCRIPTS));os.environ['PATH']='C:/Program Files/Shotcut;'+os.environ['PATH']
import audio_common as ac
manifest=json.loads((ROOT/'replacement_manifest.json').read_text())
results=[]
for row in manifest['rows']:
 p=Path(row['output']);r=ac.measure(str(p))
 with wave.open(str(p)) as w:
  x=np.frombuffer(w.readframes(w.getnframes()),dtype='<i2')
 assert x[0]==0 and x[-1]==0
 r['boundary_pcm']=[int(x[0]),int(x[-1])]
 r['standard_sfx_findings']=ac.check(r,'sfx')
 r['level_policy']='SFX category peak target, with a common rapid-fire headroom adjustment; cue gains unchanged.'
 results.append(r)
for role in ('NeedleCannon','Hull','Shield'):
 levels=[results[i]['loudness_lufs'] for i,row in enumerate(manifest['rows']) if row['role']==role]
 assert max(levels)-min(levels)<=2.,(role,levels)
assert all(not r['standard_sfx_findings'] for r in results)
(ROOT/'prepared_qa.json').write_text(json.dumps(results,indent=2))
def player(path,label):
 return '<div><p>'+html.escape(label)+'</p><audio controls preload="none" src="'+html.escape(path)+'"></audio></div>'
body='<h1>PR12 supplied-recording replacements</h1><p>Lancer cannon fire, hull impacts and shield impacts. Source previews only; game attenuation, variation and music are absent. Nothing autoplays.</p>'
body+='<h2>Original supplied recordings — unchanged</h2>'
for row in json.loads((ROOT/'source_receipts.json').read_text()):
 name=Path(row['source']).name;body+=player('source/'+name,name)
for role in ('NeedleCannon','Hull','Shield'):
 body+='<h2>'+role+'</h2>'
 oldroot=ROOT.parents[4]/'audio-prototype-v1/Saved/AudioMusic/PrototypeV1/Combat01'
 for take in range(1,4):
  old=oldroot/('prepared/NeedleCannonShort/SFX_NeedleCannon_%02d.wav'%take if role=='NeedleCannon' else 'export/'+role+'/SFX_Combat_'+role+'Impact_%02d.wav'%take)
  assert old.exists()
  body+=player(old.as_uri(),'Previous PR12 '+role+' take '+str(take))
 for row in manifest['rows']:
  if row['role']==role:body+=player(str(Path(row['output']).relative_to(ROOT)).replace('\\','/'),'Take %s — Impact %s, %.3f seconds'%(row['take'],row['supplied_impact'],row['duration_s']))
body+='<h2>Rapid succession model</h2><p>Offline scheduling model, not a captured game mix.</p>'+player(str(Path(manifest['offline_stress']['file']).relative_to(ROOT)).replace('\\','/'),'80 shots at 20 Hz plus alternating impacts')
capture=ROOT.parents[4]/'audio-tutorial-production-64490fb/Saved/PR12Provided01/GameplayAudioOnly02/game_pid_audio.wav'
if capture.exists():body+='<h2>Actual in-game recording</h2><p>23.99 seconds of the game process output. Lancer fire and shield cues were observed; no Hull-hit event was observed in this brief trial. Includes other game audio. No video.</p>'+player(capture.as_uri(),'Actual game output — replacement test')
(ROOT/'index.html').write_text('<!doctype html><meta charset="utf-8"><title>PR12 replacements</title><style>body{max-width:850px;margin:40px auto;background:#121923;color:#e4edf5;font:17px system-ui}audio{width:100%}h2{margin-top:36px}div{padding:8px 18px;margin:12px 0;background:#202c3d;border-radius:8px}</style>'+body+'<script>document.querySelectorAll("audio").forEach(a=>a.addEventListener("play",()=>document.querySelectorAll("audio").forEach(b=>{if(b!==a)b.pause()})))</script>')
print(json.dumps({'files':len(results),'zero_boundaries':True,'category_findings':[r['standard_sfx_findings'] for r in results]},indent=2))

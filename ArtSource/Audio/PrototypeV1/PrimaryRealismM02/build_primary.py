"""One recorded-material primary-fire concept; no Unreal operation or V1 edits."""
from pathlib import Path
import hashlib, importlib.util, json, os, subprocess, sys, wave
import numpy as np
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
PRIVATE=ROOT/'Saved/AudioMusic/PrototypeV1/PrimaryRealismM02'
TOOLS=ROOT/'.claude/skills/ue-game-audio/scripts'
def run(args):
 p=subprocess.run([str(x) for x in args],text=True,capture_output=True)
 if p.returncode:raise RuntimeError(p.stdout[-3500:]+p.stderr[-2000:])
 return p.stdout
def main():
 os.environ['PATH']=r'C:\Program Files\Shotcut'+os.pathsep+os.environ['PATH']
 log=[];prepared=PRIVATE/'prepared';prepared.mkdir(exist_ok=True)
 names=['Gunshot_1-1','Gunshot_7-1']+['MACHAppl_VCRButton_HA_TerminalUI_'+n+'_Wav' for n in ['04','08','13']]
 paths={}
 for i,name in enumerate(names):
  output=prepared/name
  log.append(run([sys.executable,TOOLS/'audio_prep.py',PRIVATE/'source'/(name+'.wav'),'--category','sfx','--name',name,'--out',output,'--max-duration','.5' if i<2 else '.15']))
  paths[name]=str(output/(name+'_01.wav'))
 # Align the physical discharge onset, not a quiet pre-roll, across the two bodies.
 guns=[];alignment=[]
 for name in names[:2]:
  source=Path(paths[name])
  with wave.open(str(source),'rb') as w:x=np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').astype(float)/32768
  rms=np.sqrt(np.mean(x[:len(x)//48*48].reshape(-1,48)**2,axis=1))
  first=int(np.flatnonzero(rms>=np.max(rms)*10**(-18/20))[0])
  trim=max(0,first*.001-.002);dest=source.with_name(name+'_aligned.wav')
  log.append(run(['ffmpeg','-hide_banner','-nostdin','-y','-v','error','-i',source,'-af',f'atrim=start={trim},asetpts=PTS-STARTPTS,afade=t=in:d=0.001','-c:a','pcm_s16le',dest]))
  guns.append(str(dest));alignment.append(dict(asset=name,trim_seconds=trim,rule='First 1ms RMS bin within18dB of maximum, retain2ms preroll,1ms fade'))
 (HERE/'alignment.json').write_text(json.dumps(alignment,indent=2))
 mechanics=[paths[x] for x in names[2:]]
 recipe=dict(name='Primary_Realism_M02',category='sfx',takes=3,seed=26711,channels=1,layers=[
  dict(role='recorded hardware attack',files=mechanics,start_ms=0,gain_db=-10,pitch_semitones=-1,duration_ms=45,highpass_hz=1100,lowpass_hz=7200,fade_in_ms=1,fade_out_ms=12,vary=dict(gain_db=.3,pitch_semitones=.15)),
  dict(role='gunshot discharge body',files=guns,start_ms=4,gain_db=0,pitch_semitones=-1.5,duration_ms=200,highpass_hz=65,lowpass_hz=6200,fade_in_ms=1,fade_out_ms=65,vary=dict(gain_db=.2,pitch_semitones=.25)),
  dict(role='short filtered alternate gunshot decay',files=guns,start_ms=24,gain_db=-12,pitch_semitones=-2,trim_start_ms=30,duration_ms=110,highpass_hz=80,lowpass_hz=1300,fade_in_ms=8,fade_out_ms=80,vary=dict(gain_db=.3,pitch_semitones=.2))],
  master=dict(highpass_hz=45,lowpass_hz=8500,max_duration_ms=240,fade_out_ms=20))
 (HERE/'recipe.json').write_text(json.dumps(recipe,indent=2))
 export=PRIVATE/'export'
 log.append(run([sys.executable,TOOLS/'audio_layer.py',HERE/'recipe.json','--out',export]))
 files=sorted(export.glob('Primary_Realism_M02_0?.wav'))
 assert len(files)==3
 log.append(run([sys.executable,TOOLS/'audio_report.py',*files,'--category','sfx','--takes','--json',HERE/'takes_qa.json']))
 spec=importlib.util.spec_from_file_location('audio_common',TOOLS/'audio_common.py');ac=importlib.util.module_from_spec(spec);spec.loader.exec_module(ac)
 reference=ROOT/'Saved/AudioMusic/PrototypeV1/Host/Saved/BouncedWavFiles/ue-game-audio/V1_Primary_Five.wav'
 old=ac.measure(str(reference))
 with wave.open(str(reference),'rb') as w:frames=w.getnframes();rate=w.getframerate()
 assert rate==48000
 def readmono(f):
  with wave.open(str(f),'rb') as w:
   assert w.getnchannels()==1 and w.getsampwidth()==2 and w.getframerate()==48000
   return np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').astype(np.float64)/32768
 native=json.loads((HERE.parent/'native_author_02.json').read_text())
 onsets=[r['onset_s'] for r in native['native_preview_behavior']['primary_bursts']]
 assert len(onsets)==5
 take_order=[0,1,2,0,2]
 buf=np.zeros(frames,dtype=np.float64)
 for at,index in zip(onsets,take_order):
  x=readmono(files[index]);start=round(at*rate);buf[start:start+len(x)]+=x
 raw=PRIVATE/'five_unmatched.wav'
 def save(path,x,stereo=False):
  if stereo:x=np.repeat(x[:,None],2,axis=1)
  data=np.rint(np.clip(x,-1,1)*32767).astype('<i2')
  with wave.open(str(path),'wb') as w:w.setnchannels(2 if stereo else 1);w.setsampwidth(2);w.setframerate(rate);w.writeframes(data.tobytes())
 save(raw,buf,True);before=ac.measure(str(raw))
 gain_db=old['ungated_lufs']-before['ungated_lufs']
 preview=PRIVATE/'Primary_Realism_M02_Five.wav';save(preview,buf*10**(gain_db/20),True)
 after=ac.measure(str(preview))
 assert not after['clipped'] and not after['silent']
 assert abs(after['ungated_lufs']-old['ungated_lufs'])<.15
 match=dict(reference=str(reference),candidate=str(preview),method='Equal full-file K-weighted ungated LUFS, identical duration and shot onsets; fixed linear gain only',onsets_s=onsets,take_order=[i+1 for i in take_order],applied_gain_db=gain_db,reference_metrics=old,candidate_metrics=after,delta_lu=after['ungated_lufs']-old['ungated_lufs'])
 (HERE/'comparison_qa.json').write_text(json.dumps(match,indent=2))
 # Tight burst check at 11 shots/sec: no long tail, no clipping after one documented linear trim.
 rapid=np.zeros(round(rate*2.2))
 for i in range(11):
  x=readmono(files[i%3]);start=round((.05+i*.09)*rate);rapid[start:start+len(x)]+=x
 scale=10**(-6/20);max_abs=np.max(np.abs(rapid*scale))
 if max_abs>10**(-3/20):scale*=10**(-3/20)/max_abs
 rapid_path=PRIVATE/'Primary_Realism_M02_Rapid.wav';save(rapid_path,rapid*scale,True)
 rapid_metrics=ac.measure(str(rapid_path));assert not rapid_metrics['clipped']
 (HERE/'rapid_qa.json').write_text(json.dumps(dict(file=str(rapid_path),shots=11,interval_s=.09,linear_gain_db=20*np.log10(scale),metrics=rapid_metrics),indent=2))
 # Preserve every V1 native package and the original audition exactly.
 baseline=json.loads((PRIVATE/'v1_preservation_hashes.json').read_text())
 assert all(hashlib.sha256(Path(r['path']).read_bytes()).hexdigest()==r['sha256'] for r in baseline)
 (HERE/'preparation.log').write_text('\n\n'.join(log))
 print(json.dumps(dict(takes=[str(f) for f in files],preview=str(preview),match_delta_lu=match['delta_lu'],preview_true_peak_dbtp=after['true_peak_dbtp'],v1_reference_files_unchanged=len(baseline)),indent=2))
if __name__=='__main__':main()

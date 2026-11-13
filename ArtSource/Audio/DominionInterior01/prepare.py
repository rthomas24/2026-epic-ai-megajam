from pathlib import Path
import numpy as np,subprocess,wave,json,hashlib
ROOT=Path(__file__).resolve().parent;SOURCE=Path('C:/Users/nicho/Downloads/Calm Loop.mp3');FF='C:/Program Files/Shotcut/ffmpeg.exe';RATE=48000
expected='ddc2a8bcb9d37b6a76a0f94e39283fd9abb29acc534d275925a5e30fa527b67d';assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==expected
x=np.frombuffer(subprocess.check_output([FF,'-nostdin','-v','error','-i',str(SOURCE),'-map','0:a:0','-ar',str(RATE),'-ac','2','-f','f32le','-']),'<f4').reshape(-1,2).astype(float)
start=12*RATE;blend=2*RATE;n=len(x);assert n==1923840
# Carry the incoming pre-roll right to source12. The wrap then advances across adjacent source samples.
y=x.copy();u=np.linspace(0,np.pi/2,blend);y[-blend:]=y[-blend:]*np.cos(u)[:,None]+x[start-blend:start]*np.sin(u)[:,None]
y[:240]*=np.linspace(0,1,240)[:,None];gain_db=-.7;y*=10**(gain_db/20)
loop=y[start:];preview=np.concatenate([y,loop,loop[:20*RATE]]);fade_start=len(y)+len(loop);preview[fade_start:]*=np.linspace(1,0,20*RATE)[:,None]
assert np.max(abs(y))<1 and np.max(abs(preview))<1

def export(name,data,bits):
 scale=(1<<(bits-1))-1;q=np.rint(data*scale).astype('<i4')
 payload=q.astype('<i2').tobytes() if bits==16 else q.reshape(-1).view('u1').reshape(-1,4)[:,:3].tobytes()
 p=ROOT/'export'/name
 assert not p.exists()
 with wave.open(str(p),'wb') as w:w.setnchannels(2);w.setsampwidth(bits//8);w.setframerate(RATE);w.writeframes(payload)
 return {'path':str(p),'bits':bits,'duration_s':len(data)/RATE,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'sample_peak_dbfs':float(20*np.log10(np.max(abs(data))))}
files=[export('MUS_DominionInterior_Calm_Continuous40_01.wav',y,24),export('MUS_DominionInterior_Calm_Import16_01.wav',y,16),export('DominionInterior_LoopAndFade_Audition01.wav',preview,16),export('DominionInterior_Seam_Audition01.wav',np.concatenate([y[-6*RATE:],loop[:8*RATE]]),16)]
step=y[start]-y[-1];original_step=(x[start]-x[start-1])*10**(gain_db/20)
assert np.max(abs(step-original_step))<1e-10
assert np.array_equal(y[240:n-blend],x[240:n-blend]*10**(gain_db/20))
r={'stage':'Offline prepared and measured; Nicklisten/nativeimport/runtimehookpending','source_path':str(SOURCE),'source_sha256':expected,'source_untouched':hashlib.sha256(SOURCE.read_bytes()).hexdigest()==expected,'source_duration_s':n/RATE,'loop_start_s':12,'loop_duration_s':len(loop)/RATE,'initial_play_s':n/RATE,'loop_crossfade_s':2,'crossfade_method':'Equal-power fade fromfinal2s into source10–12s; atwrapresume12, preserving incoming adjacent sample step. No silence, duplicatehead, pitchchange, timestretch or compression.','global_gain_db':gain_db,'attack_fade_ms':5,'seam_step_dbfs':float(20*np.log10(max(abs(step)))),'seam_equals_original_adjacent_step_at12':True,'preview_loop_returns_s':[n/RATE,(n+len(loop))/RATE],'preview_fade_start_s':fade_start/RATE,'preview_fade_duration_s':20,'preview_duration_s':len(preview)/RATE,'files':files,'runtime_contract':{'music_entry_source_s':0,'single_wave_loop_start_s':12,'single_wave_loop_duration_s':len(loop)/RATE,'gameplay_completion':'Engineering exactSequence07completion/quietconsoletransitioncontractpending viaLead','exit_fade_proposed_s':20,'no_other_music_during_active_cue':True,'runtime_gain':'ApplyMusicclass/slider; auditionfilegainisnotingamelevelapproval'},'limitations':'Crossfade harmony/phrasing and finalincontextmix needNicklistening. NativeUnrealimport/gamehooknotdone.'}
(ROOT/'preparation_verification.json').write_text(json.dumps(r,indent=2));print(json.dumps(r,indent=2))

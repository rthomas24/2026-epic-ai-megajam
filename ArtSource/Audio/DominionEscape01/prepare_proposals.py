from pathlib import Path
import numpy as np,subprocess,wave,json,hashlib
ROOT=Path(__file__).resolve().parent;SOURCE=Path('C:/Users/nicho/Downloads/Ritual of the Final Dance.mp3');EXPECTED='097a7a13ff224084b33d8b2a862db4d1969d86c93de40b9251668c56a702de2e';assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==EXPECTED
rate=48000;x=np.frombuffer(subprocess.check_output(['C:/Program Files/Shotcut/ffmpeg.exe','-nostdin','-v','error','-i',str(SOURCE),'-map','0:a:0','-ar',str(rate),'-ac','2','-f','f32le','-']),'<f4').reshape(-1,2).astype(float)
entry=round(15.2*rate);flight=entry+90*rate
full=x[entry:].copy();full[:240]*=np.linspace(0,1,240)[:,None];full[-4800:]*=np.linspace(1,0,4800)[:,None]
# Audition around a hypothetical early arrival55s into the run. No bar-sync claim.
arrival=entry+55*rate;cross=int(1.5*rate);u=np.linspace(0,np.pi/2,cross)
mix=x[arrival:arrival+cross]*np.cos(u)[:,None]+x[flight:flight+cross]*np.sin(u)[:,None]
early=np.concatenate([x[arrival-6*rate:arrival],mix,x[flight+cross:flight+int(10*rate)]]);early[:240]*=np.linspace(0,1,240)[:,None];early[-4800:]*=np.linspace(1,0,4800)[:,None]
(ROOT/'export').mkdir(exist_ok=True)
def put(name,data):
 assert np.max(abs(data))<1
 p=ROOT/'export'/name;assert not p.exists()
 with wave.open(str(p),'wb') as w:w.setnchannels(2);w.setsampwidth(2);w.setframerate(rate);w.writeframes(np.rint(data*32767).astype('<i2').tobytes())
 return {'path':str(p),'duration_s':len(data)/rate,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
outputs=[put('Escape_90Run_22Flight_Proposal01.wav',full),put('Escape_EarlyFlight_Transition_Proposal01.wav',early)]
r={'stage':'Offline arrangement proposals only, not approved or integrated','source_path':str(SOURCE),'source_sha256':EXPECTED,'full_original_preserved':hashlib.sha256(SOURCE.read_bytes()).hexdigest()==EXPECTED,'run_candidate_source_s':[15.2,105.2],'run_length_s':90,'flight_candidate_source_s':[105.2,127.6],'flight_length_s':22.4,'entry_reason':'Energy attack around15.2s; NOT a verified musical bar/downbeat or subjective quality judgment','original_entry_option':'Keep source0 start if Nick prefers it. That leaves37.6s after90s; its relationship toflight timing would be arranged separately. No destructive trimming.','core_hit':'Gameplay brief requestshit2.5s aftercoredeath; exact playback/preroll scheduling pending engineering hook','early_transition_demo':{'hypothetical_shiparrival_run_elapsed_s':55,'excerpt_transition_at_s':6,'crossfade_s':1.5,'outgoing_source_s':70.2,'incoming_source_s':105.2,'bar_alignment':'Unverified; demo is crossfade-only, not a timing-locked finalcue'},'boundary_treatment':'5ms attack/100ms release on audition endpoints; no EQ/compression/pitch/time stretch','outputs':outputs,'remaining':['Nick hears and chooses source start/phrasing','Ryan confirms actual core-hit/run/shiparrival/flight/victory hooks','Verify bar positions before any bar-quantized transition','Native import and gameplay/audio validation on matching build']};(ROOT/'arrangement_proposal.json').write_text(json.dumps(r,indent=2));print(json.dumps(r,indent=2))

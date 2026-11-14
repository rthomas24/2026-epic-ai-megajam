"""Decode the complete approved Push The Attack original; no arrangement or DSP edits."""
from pathlib import Path
import hashlib,json,subprocess,wave,datetime
ROOT=Path(__file__).resolve().parent;SOURCE=Path('C:/Users/nicho/Downloads/Push The Attack.mp3');FF='C:/Program Files/Shotcut/ffmpeg.exe';EXPECTED='c8e48648443243fd78916f5d9f2642e0b454dda4bc89f6df756131f80292ecde'
assert SOURCE.is_file() and hashlib.sha256(SOURCE.read_bytes()).hexdigest()==EXPECTED
meta=json.loads(subprocess.check_output(['C:/Program Files/Shotcut/ffprobe.exe','-v','error','-select_streams','a:0','-show_entries','format=duration:stream=codec_name,sample_rate,channels','-of','json',str(SOURCE)],text=True))
OUT=ROOT/'export/MUS_DominionAssault_PushTheAttack_Full_01.wav';assert not OUT.exists()
subprocess.run([FF,'-nostdin','-n','-v','error','-i',str(SOURCE),'-map','0:a:0','-ar','48000','-ac','2','-c:a','pcm_s16le',str(OUT)],check=True)
with wave.open(str(OUT)) as w:
 assert w.getframerate()==48000 and w.getnchannels()==2 and w.getsampwidth()==2
 frames=w.getnframes();duration=frames/w.getframerate()
assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==EXPECTED
r={'stage':'Full source decoded for Unreal; native import/runtime binding not performed','assignment':'Approved post-wave5 Dominion space assault; keep separate from tutorial/globalcombat music','assignment_reference':'https://github.com/rthomas24/2026-epic-ai-megajam/issues/46#issuecomment-6864925387','claim':'5fcb3254','source_path':str(SOURCE),'source_sha256':EXPECTED,'source_bytes':SOURCE.stat().st_size,'source_metadata':meta,'source_unchanged':True,'output':str(OUT),'output_sha256':hashlib.sha256(OUT.read_bytes()).hexdigest(),'sample_rate':48000,'channels':2,'bits':16,'decoded_frames':frames,'decoded_duration_s':duration,'processing':'Full audio decode/channel-map only. No trim, fade, normalization, crossfade, looping, pitch or tempo edit. Full decoded sample count retained.','proposed_native_path':'/Game/TriadDrift/Audio/DominionAssault02/MUS_DominionAssault_PushTheAttack_Full_01','runtime_behavior':'Loop the complete track until actual assault sequence-end; exact event owned by T1. Natural original ending and restart retained; no musical rearrangement.','cook_exclusivity_gate':'Bind through cooked Presentation reference or explicit cooker rule; suppress competing general combat music while assault cue owns scene','utc':datetime.datetime.now(datetime.timezone.utc).isoformat()};(ROOT/'source_manifest.json').write_text(json.dumps(r,indent=2));print(json.dumps(r,indent=2))

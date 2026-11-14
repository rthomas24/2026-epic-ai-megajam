"""Decode the complete approved Ambient1 original; no arrangement or DSP edits."""
from pathlib import Path
import hashlib,json,subprocess,wave,datetime
ROOT=Path(__file__).resolve().parent;SOURCE=Path('C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-prototype-v1/ArtSource/Audio/IdleAmbient01/source/Ambient1.mp3');FF='C:/Program Files/Shotcut/ffmpeg.exe';EXPECTED='ee09f075d6c955e782c2b12f65f5678cdb4c25be503173cb45ef413750dce464'
assert SOURCE.is_file() and hashlib.sha256(SOURCE.read_bytes()).hexdigest()==EXPECTED
meta=json.loads(subprocess.check_output(['C:/Program Files/Shotcut/ffprobe.exe','-v','error','-select_streams','a:0','-show_entries','format=duration:stream=codec_name,sample_rate,channels','-of','json',str(SOURCE)],text=True))
OUT=ROOT/'export/MUS_SpaceIdle_Ambient1_Full_01.wav';assert not OUT.exists()
subprocess.run([FF,'-nostdin','-n','-v','error','-i',str(SOURCE),'-map','0:a:0','-ar','48000','-ac','2','-c:a','pcm_s16le',str(OUT)],check=True)
with wave.open(str(OUT)) as w:
 assert w.getframerate()==48000 and w.getnchannels()==2 and w.getsampwidth()==2
 frames=w.getnframes();duration=frames/w.getframerate()
assert hashlib.sha256(SOURCE.read_bytes()).hexdigest()==EXPECTED
r={'stage':'Full source decoded for Unreal; native import/runtime binding not performed','assignment':'All space-idle states; yield to tutorial, combat and assault score. Not concurrent environmental AmbienceSpace.','assignment_reference':'Nicholas direct request: Ambient1 for all idle events, at least in space','claim':'986d5a59','source_path':str(SOURCE),'source_sha256':EXPECTED,'source_bytes':SOURCE.stat().st_size,'source_metadata':meta,'source_unchanged':True,'output':str(OUT),'output_sha256':hashlib.sha256(OUT.read_bytes()).hexdigest(),'sample_rate':48000,'channels':2,'bits':16,'decoded_frames':frames,'decoded_duration_s':duration,'processing':'Full audio decode/channel-map only. No trim, fade, normalization, crossfade, looping, pitch or tempo edit. Full decoded sample count retained.','proposed_native_path':'/Game/TriadDrift/Audio/IdleAmbient01/MUS_SpaceIdle_Ambient1_Full_01','runtime_behavior':'Repeat during space-idle ownership only; exact T1 state/suppression contract pending. No double music.','cook_exclusivity_gate':'Bind through cooked Presentation reference or explicit cooker rule; idle score yields to tutorial, combat and assault; environmental ambience remains a separate bus','utc':datetime.datetime.now(datetime.timezone.utc).isoformat()};(ROOT/'source_manifest.json').write_text(json.dumps(r,indent=2));print(json.dumps(r,indent=2))

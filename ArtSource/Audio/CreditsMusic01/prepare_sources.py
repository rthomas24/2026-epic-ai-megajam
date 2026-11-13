"""Reproduce full credits source decoding only; refuses to overwrite outputs."""
from pathlib import Path
import json,subprocess,hashlib,wave
ROOT=Path(__file__).resolve().parent;FF='C:/Program Files/Shotcut/ffmpeg.exe'
ITEMS=[('Cathedral Pulse (1).mp3','12f35e2098ed309aa0833fd69496c045a417e81b90897ced4e041d7f5e5ebfe2','MUS_Credits_CathedralPulse_Full_01'),('Ethereal Pulse (Remix).m4a','07c7a9d84ed252b3281c5ac31db3f612aff9d3d0f6ec0638a4aa423788241051','MUS_Credits_EtherealPulseRemix_Full_02')]
for name,expected,asset in ITEMS:
 source=Path('C:/Users/nicho/Downloads')/name;assert hashlib.sha256(source.read_bytes()).hexdigest()==expected
 out=ROOT/'export'/(asset+'.wav');assert not out.exists()
 out.parent.mkdir(parents=True,exist_ok=True)
 subprocess.run([FF,'-nostdin','-n','-v','error','-i',str(source),'-map','0:a:0','-ar','48000','-ac','2','-c:a','pcm_s16le',str(out)],check=True)
 with wave.open(str(out)) as w:print(asset,w.getnframes()/w.getframerate(),hashlib.sha256(out.read_bytes()).hexdigest())

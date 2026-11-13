"""Recreate or verify the exact private WAV. Never overwrites an existing file."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import wave

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
parser=argparse.ArgumentParser()
parser.add_argument('--source',type=Path,default=Path('C:/Users/nicho/Downloads/Ethereal Pulse (Remix).m4a'))
parser.add_argument('--out',type=Path,default=ROOT/'Saved/AudioMusic/PrototypeV1/CombatMusic01')
args=parser.parse_args()
receipt=json.loads((HERE/'source_receipt.json').read_text())
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert digest(args.source)==receipt['sha256'],'Original source differs'
args.out.mkdir(parents=True,exist_ok=True)
original=args.out/'original'/args.source.name
if not original.exists():
 original.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(args.source,original)
assert digest(original)==receipt['sha256']
output=args.out/'MUS_Combat_EtherealPulseRemix_01.wav'
expected=json.loads((HERE/'manifest_02.json').read_text())['sha256']
if not output.exists():
 subprocess.run(['C:/Program Files/Shotcut/ffmpeg.exe','-hide_banner','-nostats','-n','-threads','1','-i',str(original),'-map','0:a:0','-af','atrim=duration=139.2,asetpts=PTS-STARTPTS,afade=t=in:d=0.005,afade=t=out:st=139.15:d=0.05,volume=-1.65dB,aresample=48000:dither_method=triangular','-ar','48000','-ac','2','-c:a','pcm_s16le',str(output)],check=True)
assert digest(output)==expected,'Prepared output differs; preserve and review, never overwrite'
with wave.open(str(output),'rb') as w:assert (w.getnframes(),w.getframerate(),w.getnchannels(),w.getsampwidth())==(6681600,48000,2,2)
assert digest(args.source)==receipt['sha256']
print(json.dumps({'prepared_file':str(output),'sha256':expected,'source_unchanged':True}))

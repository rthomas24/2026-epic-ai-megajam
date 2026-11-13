"""Nick's exact 38..95 second loop. Preserve source; never overwrite outputs."""
from pathlib import Path
import hashlib,json,wave
import numpy as np

ROOT=Path(__file__).resolve().parents[4]
SOURCE=ROOT/'Saved/AudioMusic/PrototypeV1/CombatMusic01/MUS_Combat_EtherealPulseRemix_01.wav'
OUT=ROOT/'Saved/AudioMusic/PrototypeV1/CombatMusic01/Drop38Loop01'
assert not OUT.exists(), 'Preserve prior render'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(SOURCE)=='a705d8a00948cbf96bb4bbd33911cffa35e1cad102c7abaa1a839739df2a44a5'
with wave.open(str(SOURCE)) as w:
    assert (w.getframerate(),w.getnchannels(),w.getsampwidth())==(48000,2,2)
    original=np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').reshape(-1,2).copy()
rate=48000;begin=38*rate;end=95*rate;n=5760
loop=original[begin:end].astype(np.float64)
# Blend into the original 120ms PRE-ROLL immediately preceding38s. Thus the last
# sample connects to the exact original38s first sample, without skipping/repeating
# the drop, shortening the57s period, or inserting silence. No audio beyond95s.
u=np.linspace(0,1,n)[:,None]
loop[-n:]=loop[-n:]*np.cos(u*np.pi/2)+original[begin-n:begin]*np.sin(u*np.pi/2)
assert np.max(np.abs(loop))<32767, 'Crossfade needs revised headroom; do not clip'
loop=np.rint(loop).astype('<i2')
assert np.array_equal(loop[0],original[begin]) and np.array_equal(loop[-1],original[begin-1])
OUT.mkdir(parents=True)
def write(name,data):
    path=OUT/name
    with wave.open(str(path),'wb') as w:
        w.setnchannels(2);w.setsampwidth(2);w.setframerate(rate);w.writeframes(data.astype('<i2').tobytes())
    return {'path':str(path),'sha256':sha(path),'frames':len(data),'duration_s':len(data)/rate}
asset=write('MUS_Combat_EtherealPulseRemix_Drop38_01.wav',loop)
preview=np.tile(loop,(2,1)).astype(np.float64)
attack=int(.15*rate);release=int(.1*rate)
preview[:attack]*=np.linspace(0,1,attack)[:,None]
preview[-release:]*=np.linspace(1,0,release)[:,None]
double=write('Drop38_TwoLoops_114s.wav',np.rint(preview))
seam=write('Drop38_JoinPreview_6s.wav',np.concatenate([loop[-3*rate:],loop[:3*rate]]))
receipt={'stage':'Offline loop prepared; native import, runtime handoff and Nick listening pending',
         'original_prepared_sha256':sha(SOURCE),'source_start_s':38,'source_end_s':95,'period_s':57,
         'crossfade_ms':120,'crossfade_type':'Equal-power tail into source37.88..38s pre-roll',
         'initial_fade_ms':150,'initial_fade_scope':'Preview only; runtime director must apply150ms once per start, not per loop',
         'asset':asset,'two_loop_preview':double,'join_preview':seam,
         'boundary_delta_pcm':[int(x) for x in loop[0].astype(np.int32)-loop[-1].astype(np.int32)],
         'boundary_is_original_adjacent_samples':True,'source_unchanged':sha(SOURCE)=='a705d8a00948cbf96bb4bbd33911cffa35e1cad102c7abaa1a839739df2a44a5',
         'sample_peak_dbfs':float(20*np.log10(np.max(np.abs(loop.astype(float)))/32768)),
         'clipped_samples':int(np.count_nonzero(np.abs(loop.astype(np.int32))>=32767))}
(OUT/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps(receipt,indent=2))

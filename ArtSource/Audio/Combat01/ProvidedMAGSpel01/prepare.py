"""Prepare Nick's supplied recordings only. No synthesis, layering or pitch edits."""
from pathlib import Path
import json, hashlib, wave
import numpy as np

ROOT = Path(__file__).resolve().parent
RATE = 48000

def read24(path):
    with wave.open(str(path)) as w:
        assert (w.getframerate(), w.getnchannels(), w.getsampwidth()) == (RATE, 2, 3)
        b = np.frombuffer(w.readframes(w.getnframes()), dtype=np.uint8).reshape(-1, 3)
    v = b[:, 0].astype(np.int32) | (b[:, 1].astype(np.int32) << 8) | (b[:, 2].astype(np.int32) << 16)
    return (((v ^ 0x800000) - 0x800000).reshape(-1, 2) / 8388608).mean(axis=1)

receipts = json.loads((ROOT/'source_receipts.json').read_text())
sources = {}
for row in receipts:
    p = ROOT/'source'/Path(row['source']).name
    assert hashlib.sha256(p.read_bytes()).hexdigest() == row['sha256']
    number = int(p.name.split('Impact')[1].split('_')[0])
    sources[number] = (p, read24(p))

# Three distinct supplied takes per role. The same recording can provide a short
# firing body and a longer impact; all raw originals remain byte-identical.
recipe = {
    'NeedleCannon': [(19, .0201875, .320), (109, .0531667, .320), (117, .1120625, .320)],
    'Hull': [(109, .0531667, 1.15), (29, .1209167, 1.15), (117, .1120625, 1.15)],
    'Shield': [(3, .0239167, 2.10), (29, .1209167, 1.55), (19, .0201875, 1.15)],
}
release = {'NeedleCannon': .100, 'Hull': .120, 'Shield': .180}
prepared = {}
rows = []
for role, takes in recipe.items():
    prepared[role] = []
    for take, (number, start, duration) in enumerate(takes, 1):
        p, source = sources[number]
        a = round(start*RATE); b = min(len(source), a+round(duration*RATE))
        x = source[a:b].copy()
        active=np.flatnonzero(np.abs(x)>=.001)
        assert len(active)
        left=max(0,int(active[0])-48);right=min(len(x),int(active[-1])+241)
        x=x[left:right];a+=left;b=a+len(x)
        dc = float(x.mean()); x -= dc
        attack = round(.002*RATE); tail = min(round(release[role]*RATE), len(x)-attack)
        envelope=np.ones(len(x));envelope[:attack]=np.linspace(0.,1.,attack);envelope[-tail:]=np.linspace(1.,0.,tail)
        # Correct the envelope-weighted mean, preserving exact zero endpoints.
        x=(x-float(np.sum(x*envelope)/np.sum(envelope)))*envelope
        # Remove only the already-inaudible end of the faded tail, then reseal
        # its last five milliseconds to zero. Keep every prior export for comparison.
        audible=np.flatnonzero(np.abs(x)>.0015)
        end=min(len(x),int(audible[-1])+241)
        if end<len(x):
            x=x[:end];x[-240:]*=np.linspace(1.,0.,240)
            weight=np.sin(np.linspace(0.,np.pi,len(x)))**2
            x-=np.sum(x)*weight/np.sum(weight)
        # One linear level adjustment. No dynamics, distortion, filtering or re-timing.
        rms = np.sqrt(np.mean(x*x)); peak = np.max(np.abs(x))
        gain = 10**(-1.5/20)/max(peak,1e-12)
        if role=='Shield' and take==3:gain*=10**(-.3/20)
        x *= gain
        prepared[role].append(x)
        rows.append(dict(role=role, take=take, supplied_impact=number, source=str(p),
                         source_sha256=hashlib.sha256(p.read_bytes()).hexdigest(),
                         source_range_s=[a/RATE,(a+len(x))/RATE], duration_s=len(x)/RATE,
                         dc_removed=dc, linear_gain_db=float(20*np.log10(gain)),
                         attack_s=.002, release_s=tail/RATE,
                         asset_path='/Game/TriadDrift/Audio/Combat01/Waves/SW_'+role+'_%02d'%take))

def stress():
    # Conservative source preview: 20 shots/s and alternating hits, no attenuation,
    # maximum existing +1.5 dB variation. Six firing/four impact voices. This is
    # a scheduling model, not a claim about measured engine stealing or final mix.
    output = np.zeros(6*RATE)
    events = []
    for shot in range(80):
        time = shot*.05
        events.append((time,'NeedleCannon',shot%3, -8.,6))
        role = 'Hull' if shot%2==0 else 'Shield'
        events.append((time,role,shot%3,-14. if role=='Hull' else -16.,4))
    for time,role,take,db,limit in events:
        start = round(time*RATE); x = prepared[role][take]
        # At exactly 20 Hz, bounding each contribution to limit/20 seconds models
        # the voice cap without claiming the engine's actual steal/release policy.
        span = min(len(x), round((limit/20)*RATE))
        signal = x[:span].copy()
        if span < len(x):
            n=min(round(.005*RATE),span);signal[-n:] *= np.linspace(1,0,n)
        output[start:start+span] += signal * 10**((db+1.5)/20)
    return output

mix = stress()
common = min(1., 10**(-4/20)/max(np.max(np.abs(mix)),1e-12))
for role in prepared:
    prepared[role] = [x*common for x in prepared[role]]
mix *= common

out = ROOT/'export_v4'; out.mkdir(exist_ok=True)
def write(path, x):
    assert not path.exists()
    # Deterministic 16-bit conversion; source originals are retained in 24-bit stereo.
    pcm = np.round(np.clip(x,-1,32767/32768)*32768).astype('<i2')
    with wave.open(str(path),'wb') as w:
        w.setnchannels(1);w.setsampwidth(2);w.setframerate(RATE);w.writeframes(pcm.tobytes())
    return hashlib.sha256(path.read_bytes()).hexdigest()

for row in rows:
    x = prepared[row['role']][row['take']-1]
    path = out/('SW_'+row['role']+'_%02d.wav'%row['take'])
    row.update(output=str(path), output_sha256=write(path,x),
               common_stress_headroom_gain_db=float(20*np.log10(common)),
               peak_dbfs=float(20*np.log10(np.max(np.abs(x))+1e-12)))
stress_file = out/'PR12_Provided_20Hz_OfflineStress.wav'
write(stress_file,mix)
result = dict(stage='Prepared-file replacements only; native reimport/game playback pending',
              claim='98c956c9', pr12_head_preserved='69c18e33e519096eea065988e6874bd7f86c9cd5',
              rows=rows, routing_changes=False, synthetic_layers=False,
              original_sources_unchanged=True,
              offline_stress=dict(file=str(stress_file), source_peak_dbfs=float(20*np.log10(np.max(np.abs(mix))+1e-12)),
                                  final_engine_mix_verified=False),
              provenance='Supplied by Nick; embedded rawAmbience. Acquisition/free-pack match pending; no raw GitHub publication.')
(ROOT/'replacement_manifest.json').write_text(json.dumps(result,indent=2))
print(json.dumps({'takes':len(rows),'common_gain_db':20*np.log10(common),'stress_peak_dbfs':20*np.log10(np.max(np.abs(mix)))}))

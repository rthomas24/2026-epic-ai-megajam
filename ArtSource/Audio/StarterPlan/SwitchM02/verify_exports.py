"""Read the delivered WAV bytes independently; verify headers and technical QA.

Uses Python standard library and NumPy. No re-render, import, or playback.
"""
from pathlib import Path
import hashlib
import json
import wave
import numpy as np

ROOT = Path(__file__).resolve().parent


def read(path):
    with wave.open(str(path), 'rb') as wav:
        header = dict(channels=wav.getnchannels(), sample_rate=wav.getframerate(),
                      bytes_per_sample=wav.getsampwidth(), frames=wav.getnframes(),
                      compression=wav.getcomptype())
        raw = np.frombuffer(wav.readframes(wav.getnframes()), np.uint8).reshape(-1,3)
    values = raw[:,0].astype(np.int32) | (raw[:,1].astype(np.int32)<<8) | (raw[:,2].astype(np.int32)<<16)
    values = (values ^ 8388608) - 8388608
    return values.reshape(-1,header['channels'])/8388608, header


def db(value):
    return float(20*np.log10(max(float(value),1e-12)))


def measure(path):
    x, header = read(path)
    sr = header['sample_rate']
    peak = np.max(np.abs(x))
    mono = x.mean(axis=1)
    channel_rms = np.sqrt(np.mean(x*x,axis=0))
    spectrum = np.sum(abs(np.fft.rfft(x,axis=0))**2,axis=1)
    f = np.fft.rfftfreq(len(x),1/sr)
    bands = [(0,120),(120,500),(500,2000),(2000,8000),(8000,24001)]
    band_power = {f'{a}-{min(b,24000)}Hz':round(float(100*spectrum[(f>=a)&(f<b)].sum()/spectrum.sum()),3) for a,b in bands}
    onset = np.flatnonzero(np.max(abs(x),axis=1) > 10**(-50/20))
    clip_count = int(np.count_nonzero(abs(x)>=1-1/8388608))
    offset = np.max(abs(x.mean(axis=0)))
    first_last_zero = bool(np.all(x[[0,-1]]==0))
    correlation = float(np.corrcoef(x.T)[0,1])
    checks = dict(pcm24_stereo_48k=header['compression']=='NONE' and header['sample_rate']==48000 and header['channels']==2 and header['bytes_per_sample']==3,
                  no_clipped_samples=clip_count==0, sample_peak_below_minus_2_9=peak<10**(-2.9/20),
                  offset_below_minus_100_dbfs=offset<1e-5, clean_file_edges=first_last_zero,
                  immediate_event_onset=bool(len(onset) and onset[0]/sr<.02),
                  positive_mono_compatibility=correlation>.5)
    checks = {name:bool(value) for name,value in checks.items()}
    return dict(file=str(path.relative_to(ROOT)).replace('\\','/'), bytes=path.stat().st_size,
                sha256=hashlib.sha256(path.read_bytes()).hexdigest(),header=header,
                duration_s=len(x)/sr, peak_dbfs=db(peak), clipped_samples=clip_count,
                dc_offset_dbfs=db(offset), onset_above_minus_50_dbfs_ms=float(onset[0]*1000/sr),
                channel_rms_dbfs=[db(v) for v in channel_rms],
                correlation=correlation, mono_rms_dbfs=db(np.sqrt(np.mean(mono*mono))),
                spectral_energy_percent=band_power, checks=checks, passed=all(checks.values()))


def main():
    paths=sorted((ROOT/'audition').glob('*.wav'))
    assert len(paths)==3, 'Milestone is exactly three WAVs'
    report=dict(method='Independent decoded PCM byte inspection; no auditory or Unreal verification',
                files=[measure(p) for p in paths])
    report['passed']=all(p['passed'] for p in report['files'])
    (ROOT/'export-qa.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2))
    assert report['passed'], 'Export QA failed'


if __name__=='__main__':
    main()

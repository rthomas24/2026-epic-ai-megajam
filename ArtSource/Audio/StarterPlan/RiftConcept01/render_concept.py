"""Rift concept 01 — sample-led 'Veil', independent of M02/M03 synthesis.
Decode the retained licensed source with installed FFmpeg; no network calls.
"""
from pathlib import Path
import subprocess, numpy as np, wave, json, hashlib, sys
ROOT=Path(__file__).resolve().parent
SR=48000
SOURCE=ROOT/'source/Dasgoat_ShimmerGlass_HQ.mp3'
FFMPEG=Path(r'C:/Program Files/Shotcut/ffmpeg.exe')
OUTPUT=ROOT/'audition/SW_TD_Rift_Veil_Concept01.wav'
def smooth(x):
    x=np.clip(x,0,1); return x*x*x*(10+x*(-15+6*x))
def db(v): return float(20*np.log10(max(float(v),1e-12)))
def rms(x):return float(np.sqrt(np.mean(x*x)))
def biquad(x,kind,hz,q=.707):
    w=2*np.pi*hz/SR;c=np.cos(w);a=np.sin(w)/(2*q)
    b=np.array([(1+c)/2,-(1+c),(1+c)/2] if kind=='high' else [(1-c)/2,1-c,(1-c)/2])/(1+a)
    a1=-2*c/(1+a);a2=(1-a)/(1+a); z1=np.zeros(2);z2=z1.copy();y=np.empty_like(x)
    for i,s in enumerate(x):
        v=b[0]*s+z1;z1=b[1]*s-a1*v+z2;z2=b[2]*s-a2*v;y[i]=v
    return y
def truepeak(x):
    p=np.pad(x,((4096,4096),(0,0))); f=np.fft.rfft(p,axis=0);f[-1]*=.5
    return np.max(abs(np.fft.irfft(f,n=4*len(p),axis=0)*4))
def read_pcm24(path):
    with wave.open(str(path),'rb') as w:
        fmt=dict(rate=w.getframerate(),channels=w.getnchannels(),width=w.getsampwidth(),frames=w.getnframes())
        raw=np.frombuffer(w.readframes(w.getnframes()),np.uint8).reshape(-1,3)
    v=raw[:,0].astype(np.int32)|(raw[:,1].astype(np.int32)<<8)|(raw[:,2].astype(np.int32)<<16)
    return (((v^8388608)-8388608)/8388608).reshape(-1,2),fmt
def main():
    result=subprocess.run([str(FFMPEG),'-v','error','-i',str(SOURCE),'-f','f32le','-acodec','pcm_f32le','-ar',str(SR),'-ac','2','pipe:1'],check=True,capture_output=True)
    source=np.frombuffer(result.stdout,dtype='<f4').reshape(-1,2).astype(np.float64)
    # The published glass texture supplies all timbral content: no invented
    # oscillator, pressure burst, impact, rumble, or procedural noise layer.
    duration=1.42;t=np.arange(round(duration*SR))/SR
    # Start after the source's initial transient. A very small downward drift
    # alters the recorded texture continuously rather than introducing new notes.
    read_rate=.80-.025*smooth(t/.60)
    # Original has approximately half a second of lead-in. Use its already
    # sustained shimmer at 0.72s so playback is responsive without a hard strike.
    positions=.72*SR+np.cumsum(read_rate)
    x=np.column_stack([np.interp(positions,np.arange(len(source)),source[:,ch]) for ch in range(2)])
    x=biquad(x,'high',330);x=biquad(x,'low',4200);x=biquad(x,'low',6000)
    mid=x.mean(axis=1); side=(x[:,0]-x[:,1])*.225
    x=np.column_stack([mid+side,mid-side])
    # Soft immediate engagement into one smoothly decaying translucent gesture.
    e=smooth(t/.010)*np.exp(-t/.39)*smooth((duration-.025-t)/.58)
    x*=e[:,None]
    # Restrained physical-sample reflections only; mid stays stable.
    out=.92*x.copy()
    for ch,delay in [(0,.021),(1,.031)]:
        n=round(delay*SR);out[n:,ch]+=.065*x[:-n,1-ch]
    # Gentle stereo travel, without polarity inversion or strong modulation.
    p=.12*np.sin(np.pi*np.clip(t/.7,0,1)-np.pi/2)
    out[:,0]*=np.sqrt(1-p);out[:,1]*=np.sqrt(1+p)
    out*=smooth(t/.008)[:,None]*smooth((duration-.025-t)/.30)[:,None]
    out[-1200:]=0;out[0]=0
    out*=10**(-9/20)/truepeak(out)
    rng=np.random.default_rng(261114)
    dither=(rng.random(out.shape)-rng.random(out.shape))/8388608
    dither[0]=0;dither[-1200:]=0
    v=np.rint((out+dither)*8388608).astype(np.int32).ravel()
    assert np.max(abs(v))<8388607
    raw=np.stack([v&255,(v>>8)&255,(v>>16)&255],axis=1).astype(np.uint8)
    OUTPUT.parent.mkdir(exist_ok=True)
    with wave.open(str(OUTPUT),'wb') as w:
        w.setnchannels(2);w.setsampwidth(3);w.setframerate(SR);w.writeframes(raw.tobytes())
    decoded,fmt=read_pcm24(OUTPUT)
    onset=np.flatnonzero(np.max(abs(decoded),axis=1)>10**(-50/20))
    # Repeated 400ms test is a numerical diagnostic; export remains exactly one WAV.
    bus=np.zeros((round((duration+2.8)*SR),2))
    for at in np.arange(8)*.4:
        start=round(at*SR);bus[start:start+len(decoded)]+=decoded
    old={}
    for rev in ['M02','M03']:
        for f in (ROOT.parent/f'Switch{rev}'/'audition').glob('*.wav'):
            old[str(f.relative_to(ROOT.parent))]=hashlib.sha256(f.read_bytes()).hexdigest()
    info={'concept':'Rift / Veil 01','direction':'Quiet sample-led spatial glass shimmer; no M02/M03 timbral layers',
      'source_mp3_sha256':hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
      'output':OUTPUT.name,'format':fmt,'duration_s':duration,
      'peak_dbfs':db(np.max(abs(decoded))),'peak_4x_fft_estimate_dbfs':db(truepeak(decoded)),
      'rms_dbfs':db(rms(decoded)),'onset_minus50_ms':float(onset[0]*1000/SR),
      'clipped_samples':int(np.count_nonzero(abs(decoded)>=1-1/8388608)),
      'first_last_zero':bool(np.all(decoded[[0,-1]]==0)),
      'final25ms_zero':bool(np.all(decoded[-1200:]==0)),
      'tail50ms_rms_dbfs':db(rms(decoded[-2400:])),
      'dc_offset_dbfs':db(np.max(abs(decoded.mean(axis=0)))),
      'stereo_correlation':float(np.corrcoef(decoded.T)[0,1]),
      'eight_repeats_400ms_peak_dbfs':db(np.max(abs(bus))),
      'sha256':hashlib.sha256(OUTPUT.read_bytes()).hexdigest(),
      'preserved_reference_hashes':old,
      'auditory_review':'Pending Nicholas; no claim of subjective hearing or quality acceptance',
      'source_quality':'Public high-quality MP3 preview; PCM24/48k is the delivery format and does not restore information lost in the compressed source'}
    info['passed']=all([fmt['rate']==48000,fmt['width']==3,fmt['channels']==2,info['clipped_samples']==0,info['first_last_zero'],info['final25ms_zero'],info['onset_minus50_ms']<25,info['tail50ms_rms_dbfs']<-90,info['peak_4x_fft_estimate_dbfs']<=-8.99,info['eight_repeats_400ms_peak_dbfs']<0])
    (ROOT/'verification.json').write_text(json.dumps(info,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(info,indent=2))
    assert info['passed'],'Concept export check failed'
if __name__=='__main__': main()


"""Triad Drift M03: layer-safe envelopes, rounded mechanics and smooth pressure.
Reads M02 helper source/reference WAVs; never writes to M02.
Python + NumPy only. All generated content stays beside this script.
"""
from pathlib import Path
import importlib.util, sys, json, hashlib, wave, itertools, base64, platform
import numpy as np
sys.dont_write_bytecode = True
ROOT=Path(__file__).resolve().parent
OLD=ROOT.parent/'SwitchM02'
spec=importlib.util.spec_from_file_location('m02_reference',OLD/'render_switches.py')
ref=importlib.util.module_from_spec(spec); spec.loader.exec_module(ref)
SR=48000
ref.RNG=np.random.default_rng(261113)
RNG=ref.RNG
T=ref.seconds
NOISE=ref.spectral_noise
DB=ref.db

def smooth(x):
    x=np.clip(x,0,1)
    return x*x*x*(10+x*(-15+6*x))

def edge(x, attack=.008, release=.16):
    x=x.copy(); n=len(x); t=np.arange(n)/SR
    w=smooth(t/attack)*smooth((t[-1]-t)/release)
    return x*w.reshape((n,)+(1,)*(x.ndim-1))

def swell(t, attack, decay):
    return smooth(t/attack)*np.exp(-t/decay)

def pressure(d, bands, attack, decay):
    t=T(d)
    x=NOISE(d,bands)*swell(t,attack,decay)
    return edge(x,.004,min(.22,d*.3))

def body(d, base, decay, attack=.012, drift=0):
    # Soft, irregular phase and mode strength; no saturation or bare note sequence.
    t=T(d); x=np.zeros(len(t))
    for i,k in enumerate([1,1.51,2.37,3.91,6.23]):
        f=base*k*RNG.uniform(.992,1.008)
        phase=RNG.uniform(0,2*np.pi)
        movement=drift*f*.12*(-np.expm1(-t/.12))
        x+=np.sin(2*np.pi*(f*t+movement)+phase)*np.exp(-t/(decay/(1+i*.24)))/(1+i)**1.2
    x/=max(np.max(abs(x)),1e-9)
    return edge(x*smooth(t/attack),.004,min(.30,d*.35))

def contact(d, mass, strength=1):
    # Rounded clamp contact: impact pressure dominates over ringing modes.
    t=T(d)
    thump=pressure(d,[(mass*1.8,.65,1),(480,.65,.42)],.007,.055)
    texture=pressure(d,[(980,.65,1),(2100,.55,.22)],.004,.022)
    return strength*(.58*thump+.19*body(d,mass,.11,.009)+.06*texture)

def put(bus,x,at=0,gain=1,pan=0):
    ref.place(bus,edge(x,.004,min(.12,len(x)/SR*.25)),at,gain,pan)

def diffuse(bus, amount):
    # Dark smooth diffusion, no isolated high-energy echoes or metallic comb tail.
    n=len(bus); mono=bus.mean(axis=1)
    f=np.fft.rfftfreq(n,1/SR)
    send=np.fft.irfft(np.fft.rfft(mono)*(1-np.exp(-(f/250)**4))*np.exp(-(f/2600)**2),n=n)
    out=bus.copy()
    for ch in range(2):
        d=.6; t=T(d)
        ir=NOISE(d,[(850,1.1,1),(2100,.7,.25)])*np.exp(-t/.085)
        ir=edge(ir,.016,.2); ir/=np.sqrt(np.sum(ir*ir))
        # Two very quiet early reflections plus diffuse response.
        m=1<<(n+len(ir)-2).bit_length()
        tail=np.fft.irfft(np.fft.rfft(send,m)*np.fft.rfft(ir,m),m)[:n]
        delay=round((.024+ch*.004)*SR)
        early=np.zeros(n); early[delay:]=send[:-delay]
        out[:,ch]+=amount*(.12*early+.36*tail)
    return out

def design(ship):
    d={'Rift':1.72,'Lancer':1.46,'Bastion':1.88}[ship]
    out=np.zeros((round(d*SR),2))
    if ship=='Rift':
        put(out,contact(.65,95),0,.27)
        t=T(1.10)
        motion=pressure(1.10,[(320,.8,1),(950,.8,.32)],.018,.19)
        put(out,motion,0,.42,.30*np.sin(2*np.pi*.7*t-.4))
        put(out,body(1.18,74,.20,.013,.035),0,.23)
        # Continuously crossfaded textures, not a reverse sample slammed into place.
        a=NOISE(.92,[(450,.7,1),(1450,.7,.18)])
        b=NOISE(.92,[(850,.8,1),(2100,.7,.12)])
        t=T(.92); q=smooth(t/.32)
        movement=(a*np.cos(q*np.pi/2)+b*np.sin(q*np.pi/2))*swell(t,.025,.18)
        put(out,movement,.014,.16,-.22)
        out=diffuse(out,.12)
    elif ship=='Lancer':
        put(out,contact(.46,155),0,.39)
        put(out,contact(.38,215),.044,.13,.08)
        t=T(.94); q=smooth(t/.24)
        low=NOISE(.94,[(310,.7,1),(850,.6,.22)])
        high=NOISE(.94,[(730,.75,1),(1700,.6,.15)])
        turbine=(low*np.cos(q*np.pi/2)+high*np.sin(q*np.pi/2))*swell(t,.018,.155)
        put(out,turbine,.008,.40,.15*np.sin(t*3))
        put(out,body(.83,106,.13,.011,.025),.003,.15)
        out=diffuse(out,.075)
    else:
        put(out,contact(.84,76),0,.48)
        put(out,contact(.68,128),.048,.20,-.08)
        put(out,body(1.24,57,.205,.014,-.015),0,.34)
        put(out,pressure(1.10,[(108,.65,1),(310,.8,.30)],.015,.20),0,.34)
        put(out,pressure(.76,[(620,.8,1),(1500,.7,.15)],.021,.12),.021,.13,.12)
        out=diffuse(out,.10)
    return out

def rbj(kind,f,q=.707,gain=0):
    w=2*np.pi*f/SR; c=np.cos(w); s=np.sin(w); alpha=s/(2*q)
    if kind=='low':
        b=[(1-c)/2,1-c,(1-c)/2]; a=[1+alpha,-2*c,1-alpha]
    elif kind=='high':
        b=[(1+c)/2,-(1+c),(1+c)/2]; a=[1+alpha,-2*c,1-alpha]
    else:
        A=10**(gain/40)
        b=[1+alpha*A,-2*c,1-alpha*A]; a=[1+alpha/A,-2*c,1-alpha/A]
    return np.array(b)/a[0],np.array(a)/a[0]

def finish(x):
    # Causal filtering avoids circular FFT pre-ringing at file boundaries.
    for kind,f,q,g in [('high',28,.707,0),('bell',2300,.7,-4.5),('low',6200,.707,0),('low',6200,.707,0)]:
        b,a=rbj(kind,f,q,g); x=ref.biquad(x,b,a)
    # Smooth final envelope after every processing stage; no nonlinear distortion.
    x=edge(x,.006,.43)
    x[-960:]=0 # 20ms silent guard after the fade has reached inaudible level
    x[0]=0
    return x

def readwav(path):
    with wave.open(str(path),'rb') as w:
        fmt={'rate':w.getframerate(),'channels':w.getnchannels(),'width':w.getsampwidth(),'frames':w.getnframes()}
        raw=np.frombuffer(w.readframes(w.getnframes()),np.uint8).reshape(-1,3)
    v=raw[:,0].astype(np.int32)|(raw[:,1].astype(np.int32)<<8)|(raw[:,2].astype(np.int32)<<16)
    return (((v^8388608)-8388608)/8388608).reshape(-1,2),fmt

def writewav(path,x):
    # Dither only the active/fading region; silent guard stays exact digital zero.
    noise=(RNG.random(x.shape)-RNG.random(x.shape))/8388608
    noise[:1]=0; noise[-960:]=0
    v=np.rint((x+noise)*8388608).astype(np.int32)
    assert np.max(abs(v))<8388607
    v=v.ravel(); raw=np.stack([v&255,(v>>8)&255,(v>>16)&255],axis=1).astype(np.uint8)
    with wave.open(str(path),'wb') as w:
        w.setnchannels(2); w.setsampwidth(3); w.setframerate(SR); w.writeframes(raw.tobytes())

def info(path):
    x,fmt=readwav(path); n=len(x)
    onset=np.flatnonzero(np.max(abs(x),axis=1)>10**(-50/20))
    fft=np.fft.rfft(x,axis=0); f=np.fft.rfftfreq(n,1/SR); power=np.sum(abs(fft)**2,axis=1)
    return {'file':str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else '../SwitchM02/audition/'+path.name,
            'format':fmt,'duration_s':n/SR,'peak_dbfs':DB(np.max(abs(x))),
            'peak_4x_fft_estimate_dbfs':DB(ref.true_peak(x)), 'short_sfx_lufs':ref.loudness(x),
            'onset_minus50_ms':float(onset[0]/SR*1000),
            'dc_offset_dbfs':DB(np.max(abs(x.mean(axis=0)))),
            'clipped_samples':int(np.count_nonzero(abs(x)>=1-1/8388608)),
            'first_last_samples_zero':bool(np.all(x[[0,-1]]==0)),
            'first20ms_max_sample_step_dbfs':DB(np.max(abs(np.diff(x[:960],axis=0)))),
            'last50ms_rms_dbfs':DB(ref.rms(x[-2400:])),
            'final20ms_all_zero':bool(np.all(x[-960:]==0)),
            'stereo_correlation':float(np.corrcoef(x.T)[0,1]),
            'above2khz_power_percent':float(100*power[f>=2000].sum()/power.sum()),
            'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}

def rapid(mixes,order,limited):
    times=np.arange(len(order))*.4
    n=round((times[-1]+2.2)*SR); bus=np.zeros((n,2))
    for i,(ship,at) in enumerate(zip(order,times)):
        x=mixes[ship].copy()
        if limited and i+2<len(order):
            age=times[i+2]-at; start=round(age*SR); fade=round(.060*SR)
            if start<len(x):
                length=min(fade,len(x)-start)
                x[start:start+length]*=smooth(np.linspace(1,0,length))[:,None]
                x[start+length:]=0
        start=round(at*SR); bus[start:start+len(x)]+=x
    return bus

def main():
    folder=ROOT/'audition'; folder.mkdir(exist_ok=True)
    ships=['Rift','Lancer','Bastion']
    baseline={ship:OLD/'audition'/f'SW_TD_{ship}_Switch_M02.wav' for ship in ships}
    before={ship:hashlib.sha256(p.read_bytes()).hexdigest() for ship,p in baseline.items()}
    mixes={ship:finish(design(ship)) for ship in ships}
    levels={s:ref.loudness(x) for s,x in mixes.items()}
    target=min(-19.956,*[levels[s]+DB(10**(-4/20)/ref.true_peak(x)) for s,x in mixes.items()])
    for s,x in mixes.items():
        x*=10**((target-levels[s])/20)
        writewav(folder/f'SW_TD_{s}_Switch_M03.wav',x)
    current={s:info(folder/f'SW_TD_{s}_Switch_M03.wav') for s in ships}
    previous={s:info(baseline[s]) for s in ships}
    # Re-read PCM for every stress test, rather than trusting the float render.
    decoded={s:readwav(folder/f'SW_TD_{s}_Switch_M03.wav')[0] for s in ships}
    tests=[]
    for order in itertools.permutations(ships):
        repeated=list(order)*4
        for limited in [False,True]:
            y=rapid(decoded,repeated,limited)
            tests.append({'order':'/'.join(order),'events':len(repeated),'interval_s':.4,
                          'mode':'two voices + 60ms replacement fade' if limited else 'unrestricted natural tails',
                          'unity_peak_dbfs':DB(np.max(abs(y))),
                          'preview_minus6_peak_dbfs':DB(np.max(abs(y))*.5011872336),
                          'unity_clipped_samples':int(np.count_nonzero(abs(y)>=1))})
    checks={
      'three_pcm24_stereo_48k':all(v['format']['rate']==48000 and v['format']['width']==3 and v['format']['channels']==2 for v in current.values()),
      'zero_clipping':all(v['clipped_samples']==0 for v in current.values()),
      'four_db_estimated_headroom':all(v['peak_4x_fft_estimate_dbfs']<=-3.99 for v in current.values()),
      'responsive_under20ms':all(v['onset_minus50_ms']<20 for v in current.values()),
      'clean_edges_and_silent_guard':all(v['first_last_samples_zero'] and v['final20ms_all_zero'] for v in current.values()),
      'tails_below_minus90_rms':all(v['last50ms_rms_dbfs']< -90 for v in current.values()),
      'smoother_first20ms_steps':all(current[s]['first20ms_max_sample_step_dbfs']<previous[s]['first20ms_max_sample_step_dbfs'] for s in ships),
      'lower_above2k_power_share':all(current[s]['above2khz_power_percent']<previous[s]['above2khz_power_percent'] for s in ships),
      'matched_short_sfx_loudness_within_point1':max(v['short_sfx_lufs'] for v in current.values())-min(v['short_sfx_lufs'] for v in current.values())<.1,
      'rapid_unity_zero_clipping':all(t['unity_clipped_samples']==0 for t in tests),
      'm02_bytes_unchanged':all(hashlib.sha256(p.read_bytes()).hexdigest()==before[s] for s,p in baseline.items())
    }
    report={'revision':'M03','seed':261113,'python':platform.python_version(),'numpy':np.__version__,
      'target_short_sfx_lufs':target,'processing':'New original layered design; all-layer C2 envelopes, reduced modal/noise levels, no saturation, causal finishing EQ',
      'listening_status':'Objective individual/sequence QA performed; subjective hearing and user acceptance pending',
      'meter_limit':'Short SFX K-weighted gate reference and 4x FFT peak estimate; not certified meters',
      'm02':previous,'m03':current,'rapid_tests':tests,'checks':checks,'passed':all(checks.values())}
    (ROOT/'verification.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'target_lufs':target,'m03':current,'checks':checks},indent=2))
    assert report['passed'],'Inspect failed checks before delivery'

if __name__=='__main__': main()


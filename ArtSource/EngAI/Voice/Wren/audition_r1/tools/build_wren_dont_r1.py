"""Usage: python build_wren_dont_r1.py <out_dir>   (needs numpy, scipy, soundfile, ffmpeg)
Build Wren 'Dont' audition r1: dry cut takes + radio candidates. Deterministic."""
import sys,json,os,numpy as np,soundfile as sf
from scipy.signal import butter,sosfilt
HERE=os.path.dirname(os.path.abspath(__file__));ROOT=os.path.abspath(os.path.join(HERE,*['..']*6))
sys.path.insert(0,HERE);sys.path.insert(0,os.path.join(ROOT,'.claude/skills/ue-game-audio/scripts'))
import cut_detect as P, audio_common as ac
SR=48000;OUT=sys.argv[1];os.makedirs(OUT,exist_ok=True)
TAKES=[ # (take, raw file, cut fraction into the /n/)
 ("01","b2_1_ara_E.wav"),("02","b2_3_iris_E.wav"),("03","b2_6_eve_E.wav"),("04","b1_6_liora.wav"),("05","b1_0_ara.wav")]
PRE=0.045;FADE_MS=4;CUT_FRAC=0.5;TARGET=-19.0;TP_MAX=-1.5
def gain_to(path,target):
    m=ac.measure(path);g=target-m['loudness_lufs']
    g=min(g,TP_MAX-m['true_peak_dbtp']);return g,m
def write(path,y): sf.write(path,np.clip(y,-1,1),SR,subtype='PCM_24')
def radio(y,seed):
    rng=np.random.default_rng(seed)
    hp=butter(4,300,'hp',fs=SR,output='sos');lp=butter(6,3400,'lp',fs=SR,output='sos')
    v=sosfilt(lp,sosfilt(hp,y))
    v=v/np.abs(v).max()
    drive=2.2;v=np.tanh(drive*v)/np.tanh(drive)          # light saturation
    v=sosfilt(lp,v)                                       # re-band-limit the harmonics
    tail=int(0.20*SR);n=len(v)
    out=np.concatenate([v,np.zeros(tail)])
    noise=sosfilt(lp,sosfilt(hp,rng.standard_normal(len(out))));noise/=np.abs(noise).max()
    bed=noise*10**(-38/20)                                # faint carrier hiss under the voice
    t=np.arange(tail)/SR;envt=10**(-12/20)*np.exp(-t/0.06);envt[:int(.002*SR)]*=np.linspace(0,1,int(.002*SR))
    burst=np.zeros(len(out));burst[n:]=noise[n:]*envt
    # crackle: sparse impulses in the tail, band-limited
    cr=np.zeros(len(out));idx=n+rng.integers(0,int(.15*SR),14);cr[idx]=rng.uniform(-1,1,14)*0.5*np.exp(-(idx-n)/SR/0.07)
    cr=sosfilt(lp,sosfilt(hp,cr))
    out=out+bed+burst+cr
    out=sosfilt(butter(2,250,'hp',fs=SR,output='sos'),out);out-=out.mean()
    fo=int(.012*SR);out[-fo:]*=np.linspace(1,0,fo)
    fi=int(.003*SR);out[:fi]*=np.linspace(0,1,fi)
    return out
log=[]
for take,src in TAKES:
    raw=os.path.join(HERE,'..','raw',src)
    x,_=sf.read(raw);x=x-x.mean()
    s=P.first_syllable(x);cut=s['nasal']+CUT_FRAC*(s['seg_end']-s['nasal'])
    a=int(max(0,s['onset']-PRE)*SR);b=int(cut*SR);y=x[a:b].copy()
    pad=int(round((PRE-(s['onset']-a/SR))*SR));y=np.concatenate([np.zeros(max(0,pad)),y])
    fi=int(.003*SR);on=int(PRE*SR)-fi;y[on:on+fi]*=np.linspace(0,1,fi);y[:on]=0
    fo=int(FADE_MS/1000*SR);y[-fo:]*=np.cos(np.linspace(0,np.pi/2,fo))**2
    dry=f"{OUT}/VO_FutureWren_Dont_{take}_dry.wav";write(dry,y)
    g,_=gain_to(dry,TARGET);y*=10**(g/20);write(dry,y)
    r=radio(y,int(take));rp=f"{OUT}/VO_FutureWren_Dont_{take}_radio.wav";write(rp,r)
    g2,_=gain_to(rp,TARGET);r*=10**(g2/20);write(rp,r)
    log.append(dict(take=take,source=src,onset_s=round(s['onset'],4),cut_s=round(cut,4),src_start_s=round(a/SR,4),dry_gain_db=round(g,2),radio_gain_db=round(g2,2)))
json.dump(log,open(os.path.join(OUT,'build_log.json'),'w'),indent=1);print(json.dumps(log,indent=1))

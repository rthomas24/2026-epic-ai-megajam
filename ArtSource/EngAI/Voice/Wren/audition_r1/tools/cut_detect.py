import sys,json,numpy as np,soundfile as sf
from scipy.signal import butter,sosfilt
SR=48000
def env(x,hop):
    return np.array([np.sqrt(np.mean(x[i:i+2*hop]**2)) for i in range(0,len(x)-2*hop,hop)])
def first_syllable(x):
    hop=int(.0025*SR);r=env(x,hop)
    k=np.ones(4)/4;r=np.convolve(r,k,'same');db=20*np.log10(r+1e-9);mx=db.max()
    lim=int(0.5/ (hop/SR))
    pk=int(np.argmax(db[:lim]))
    i=pk
    while i<len(db) and db[i]>db[pk]-30: i+=1
    j=pk
    while j<i and db[j]>db[pk]-9: j+=1
    a=pk
    while a>0 and (db[a]>mx-45 or db[max(0,a-12):a].max()>mx-45): a-=1
    return dict(onset=a*hop/SR,peak=pk*hop/SR,nasal=j*hop/SR,seg_end=i*hop/SR)
def dry(src,out,cut_frac=0.55,pre=0.045,fade_ms=4,cut_s=None):
    x,sr=sf.read(src);assert sr==SR;x=x if x.ndim==1 else x.mean(1)
    x=x-np.mean(x)
    s=first_syllable(x)
    cut=cut_s if cut_s else s['nasal']+cut_frac*(s['seg_end']-s['nasal'])
    a=max(0,int((s['onset']-pre)*SR));b=int(cut*SR)
    y=x[a:b].copy()
    fi=int(.003*SR);y[:fi]*=np.linspace(0,1,fi)
    fo=int(fade_ms/1000*SR);y[-fo:]*=np.cos(np.linspace(0,np.pi/2,fo))**2
    # loudness: one linear gain to vo target -19 LUFS (ungated K-weighted, like audio_report for short files)
    sf.write(out,y,SR,subtype='PCM_24')
    s.update(cut=cut,start=a/SR,dur=len(y)/SR);return s
if __name__=="__main__":
    for p in sys.argv[1:]:
        x,_=sf.read(p);print(p,{k:round(v,3) for k,v in first_syllable(x).items()})

"""Measure attacks/boundaries, verify preservation and write a local listening page."""
from pathlib import Path
import hashlib, html, json, os, wave
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
PRIVATE=ROOT/'Saved/AudioMusic/PrototypeV1/PrimaryRealismM02'
def read(p):
 with wave.open(str(p),'rb') as w:
  rate=w.getframerate();channels=w.getnchannels()
  x=np.frombuffer(w.readframes(w.getnframes()),dtype='<i2').reshape(-1,channels).astype(float)/32768
 return rate,x
def db(v):return round(20*np.log10(max(float(v),1e-12)),2)
def main():
 match=json.loads((HERE/'comparison_qa.json').read_text());rows=[];files=sorted((PRIVATE/'export').glob('*_0?.wav'))
 for p in files:
  rate,x=read(p);mono=x.mean(axis=1);rms=np.sqrt(np.mean(mono[:len(mono)//48*48].reshape(-1,48)**2,axis=1))
  rows.append(dict(file=str(p),sha256=hashlib.sha256(p.read_bytes()).hexdigest(),strong_attack_ms=int(np.flatnonzero(rms>=max(rms)*10**(-18/20))[0]),peak_time_ms=round(1000*np.argmax(np.abs(mono))/rate,2),first_sample_dbfs=db(np.max(np.abs(x[0]))),last_sample_dbfs=db(np.max(np.abs(x[-1]))),last5ms_rms_dbfs=db(np.sqrt(np.mean(x[-240:]**2)))))
 def active_rms(path):
  rate,x=read(path);samples=np.concatenate([x[round(t*rate):round((t+.4)*rate)] for t in match['onsets_s']]);return db(np.sqrt(np.mean(samples*samples)))
 old_rms=active_rms(match['reference']);new_rms=active_rms(match['candidate'])
 baseline=json.loads((PRIVATE/'v1_preservation_hashes.json').read_text())
 originals=json.loads((HERE/'source_inventory.json').read_text())
 assert all(hashlib.sha256(Path(r['path']).read_bytes()).hexdigest()==r['sha256'] for r in baseline)
 assert all(hashlib.sha256(Path(r['original_path']).read_bytes()).hexdigest()==r['source_sha256'] for r in originals)
 assert len(set(r['sha256'] for r in rows))==3
 assert all(r['strong_attack_ms']<=10 and r['last_sample_dbfs']<=-90 and r['first_sample_dbfs']<=-90 for r in rows),rows
 result=dict(takes=rows,fixed_400ms_post_trigger_window_rms_dbfs=dict(V1=old_rms,M02=new_rms,delta=round(new_rms-old_rms,2)),full_file_ungated_match_delta_lu=match['delta_lu'],preserved_v1_files=len(baseline),original_cache_packages_unchanged=len(originals),technical_pass=True,human_listening_approval=False)
 (HERE/'boundary_behavior_qa.json').write_text(json.dumps(result,indent=2))
 def url(p):return html.escape(os.path.relpath(p,HERE).replace('\\','/'),quote=True)
 def player(label,p,extra=''):
  return f'<section><h2>{html.escape(label)}</h2><audio controls preload="none" {extra} src="{url(p)}"></audio><a href="{url(p)}">Open WAV</a></section>'
 page='''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Primary Fire — Realism M02</title><style>body{font:17px system-ui;background:#101923;color:#e5edf4;max-width:920px;margin:40px auto;padding:0 24px}h1{color:#7bd2e8}h2{font-size:20px}section{background:#1b2936;padding:18px;margin:16px 0;border-radius:12px}audio{display:block;width:100%;margin:14px 0}a{color:#9cdcef}p{line-height:1.55}.note{color:#b6c8d8}button{font:inherit;padding:10px 18px;background:#75d1e0;border:0;border-radius:8px;cursor:pointer}</style><h1>Primary Fire — Realism M02</h1><p>One gunshot-led concept with three recorded hardware-action variations. V1 and the 53 native assets are preserved. No Unreal integration or final sound approval.</p><p class="note">A and B use identical measured shot positions and duration. Full-file K-weighted energy matches within 0.01 LU; the same 400 ms windows differ by less than 1 dB RMS. M02 has a higher transient peak at the matched energy. Listen for a convincing discharge, clear action, controlled decay and repetition fatigue.</p><button id="compare">Play A, then B</button>'''
 page+=player('A · Original V1 native preview',match['reference'],'id="old"')
 page+=player('B · Primary Realism M02 · five shots',match['candidate'],'id="new"')
 page+=player('M02 · rapid-fire check (11 shots)',PRIVATE/'Primary_Realism_M02_Rapid.wav')
 page+='<p class="note">Individual category exports below play at 50% player volume. Preview mix and live-game level still need listening review.</p>'
 for i,p in enumerate(files,1):page+=player('Take '+str(i),p,'class="take"')
 page+='''<p><a href="README.md">Source, licensing and metrics report</a></p><script>const all=[...document.querySelectorAll('audio')];all.forEach(a=>{if(a.classList.contains('take'))a.volume=.5;a.addEventListener('play',()=>all.forEach(b=>{if(b!==a)b.pause()}))});document.querySelector('#compare').onclick=()=>{const a=document.querySelector('#old'),b=document.querySelector('#new');a.currentTime=0;b.currentTime=0;a.onended=()=>b.play();a.play()};</script></html>'''
 (HERE/'index.html').write_text(page,encoding='utf-8')
 print(json.dumps(result,indent=2))
if __name__=='__main__':main()

"""Private, read-only VaultCache audio audit. No network, Unreal launch or integration.

Reads installed Unreal compressed-buffer/UEWavComp conventions. Uses the already
installed engine's Oodle DLL; never bundles Epic code or licensed media in Git.
"""
from pathlib import Path
import base64, ctypes, datetime, hashlib, html, io, json, re, struct, wave, zlib
import numpy as np

REPO = Path(__file__).resolve().parents[3]
SOURCE = Path(__file__).resolve().parent
PRIVATE = REPO / 'Saved/AudioMusic/EpicLibraryAudit01'
VAULT = Path(r'C:\ProgramData\Epic\EpicGamesLauncher\VaultCache')
DLL = Path(r'C:\Program Files\Epic Games\UE_5.9\Engine\Binaries\DotNET\UnrealBuildTool\runtimes\win-x64\native\oo2core.dll')
PACKS = {
 '50FreeSo92fbc86158f1V1': ('50 Free Game Sounds Pack','b8cc7270-5e0c-4ef7-a277-6a1d4b69358d'),
 'Terminal898ca54d9692V1': ('Terminal User Interface Sound Effects Pack LITE Edition','183a6ec7-cbc3-451a-8fa7-08da97c07fcf'),
 'Atmoshphe294217f9506V1': ('Atmoshpheric Worlds - FREE Game Music Pack - No AI','c9e03a63-9359-4929-9c8a-a680309dbd3f'),
 'EFMSciFie6407db08a21V1': ('EFM Sci-Fi Ambient Music Loop VOL.1','6f30f2fa-3b65-4deb-91b7-9441c635ab29'),
 'FreeSciFe0be43b23c13V1': ('Free Sci-Fi Music Pack Volume 2','e6b9a412-5bc1-4e76-9307-3d8c483d80da'),
 'AmbientBe5520f080ec3V1': ('Ambient BGM Series – Enchanted Forest Vol.1 (25 Tracks)','6670a905-2f14-4922-89da-a01c243039ef'),
 'Ultimate756fb22461edV1': ('Ultimate Fantasy Music Bundle - LITE Version','ec8de18c-2564-4a30-8703-f109dc063c76'),
 'PathofAd4c9281f5fe58V1': ('Path of Adventure - Free Music Pack for JRPG','d1dad56d-7c2f-4214-bbb2-401c462f584d'),
 'Ultimate491df8f4f623V1': ('Ultimate Game SFX Collection – Action, Horror, Ambient, Medical, Futuristic & He','51e20338-666f-4100-b863-a0577540055f'),
}
TOP = [
 ('50FreeSo92fbc86158f1V1','Whoosh_4-1','Rift / general ship-switch transition','Short transition candidate. Low measured energy above 6 kHz; silent endpoint. Spatial character and realism need listening approval.'),
 ('Terminal898ca54d9692V1','MACHAppl_PowerUp_HA_TerminalUI_02_Wav','Lancer / Bastion mechanical engagement','Recorded appliance engagement category, rather than the pack’s retro bleeps. Evaluate whether it feels substantial enough for a ship.'),
 ('Terminal898ca54d9692V1','CMPTMisc_ComputerAmbienceLoop_HA_TerminalUI_06_Wav','Cockpit machinery bed — provisional','Computer hardware ambience, NOT a verified spacecraft engine. Useful for an idle cockpit layer; boost/acceleration coverage remains missing.'),
 ('50FreeSo92fbc86158f1V1','Sci-Fi_Gun_1-1','Cannon / energy weapon','Existing designed sci-fi shot. Has peak headroom and a quiet tail; abrasive resonances and repetition still require human listening.'),
 ('50FreeSo92fbc86158f1V1','Explosion_Medium_2-1','Enemy destruction / missile impact','Compact explosion candidate with a quiet decay. Assess bass weight and whether repeated impacts mask flight feedback.'),
]
EXTRA_NAMES = {
 '50FreeSo92fbc86158f1V1': ['Whoosh_1-1','Sci-Fi_Gun_1_Reload','Explosion_Large_1-1','Magical_Interface_8-1','Sci-Fi_Interface_8-1'],
 'Terminal898ca54d9692V1': ['CMPTMisc_DiskTrayClose_HA_TerminalUI_02_Wav','CMPTMisc_ComputerAmbienceLoop_HA_TerminalUI_02_Wav','MACHAppl_PowerDown_HA_TerminalUI_01_Wav','UIMvmt_Whoosh_HA_TerminalUI_04_Wav','UIMvmt_Whoosh_HA_TerminalUI_33_Wav','UIMvmt_Whoosh_HA_TerminalUI_38_Wav','UIMvmt_Whoosh_HA_TerminalUI_49_Wav','UIMvmt_Whoosh_HA_TerminalUI_67_Wav'],
 'Atmoshphe294217f9506V1': ['Trailer_Solaric','Nordic_Exploration__The_Ancient_Moon'],
}

def sha(b): return hashlib.sha256(b).hexdigest()

def payload(b):
    # Older assets contain a normal RIFF directly. Newer assets use FCompressedBuffer.
    for m in re.finditer(b'RIFF', b):
        i=m.start()
        if b[i+8:i+12] == b'WAVE':
            n=struct.unpack_from('<I',b,i+4)[0]+8
            if i+n <= len(b): return b[i:i+n], 'embedded RIFF'
    for m in re.finditer(bytes.fromhex('b7756362'), b):
        i=m.start()
        if i+64>len(b): continue
        _,crc,method,_,_,exp,count,rawsz,compsz=struct.unpack('>IIBBBBIQQ',b[i:i+32])
        if method not in (0,3) or rawsz>256*1024**2 or compsz>len(b)-i: continue
        meta=0 if method==0 else 4*count
        if zlib.crc32(b[i+8:i+64+meta]) != crc: continue
        if method==0: raw=b[i+64:i+compsz]
        else:
            assert 0<count<65536 and 10<=exp<=28
            sizes=struct.unpack('>'+str(count)+'I',b[i+64:i+64+meta])
            lib=ctypes.CDLL(str(DLL)); f=lib.OodleLZ_Decompress
            f.restype=ctypes.c_ssize_t
            f.argtypes=[ctypes.c_void_p,ctypes.c_ssize_t,ctypes.c_void_p,ctypes.c_ssize_t,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_void_p,ctypes.c_ssize_t,ctypes.c_void_p,ctypes.c_void_p,ctypes.c_void_p,ctypes.c_ssize_t,ctypes.c_int]
            parts=[];pos=i+64+meta
            for idx,sz in enumerate(sizes):
                rs=min(1<<exp,rawsz-idx*(1<<exp)); block=b[pos:pos+sz]
                if sz==rs: parts.append(block)
                else:
                    src=ctypes.create_string_buffer(block);dst=ctypes.create_string_buffer(rs)
                    assert f(src,sz,dst,rs,1,0,0,None,0,None,None,None,0,3)==rs
                    parts.append(dst.raw)
                pos+=sz
            assert pos==i+compsz
            raw=b''.join(parts)
        assert len(raw)==rawsz
        if raw[:4]==b'RIFF' and raw[8:12]==b'WAVE': return raw,'Oodle buffer; header CRC and block lengths verified'
    raise ValueError('No supported audio payload found')

def decode_wave(raw):
    raw=bytearray(raw);pos=12;fmt=None;data=None
    while pos+8<=len(raw):
        tag=bytes(raw[pos:pos+4]);sz=struct.unpack_from('<I',raw,pos+4)[0]
        if tag==b'fmt ':fmt=pos+8
        if tag==b'data':data=(pos+8,sz)
        pos+=8+sz+(sz%2)
    assert fmt and data
    code,ch,rate,_,align,bits=struct.unpack_from('<HHIIHH',raw,fmt)
    transform='none; RIFF byte-identical'
    if code==65535:
        assert bits==16
        start,sz=data;v=np.frombuffer(raw[start:start+sz],np.uint8);n=sz//2
        dif=v[:n].astype(np.int64)+(v[n:].astype(np.int64)<<8)
        frames=dif.reshape(-1,ch);delta=frames.copy();delta[1:]-=128
        samples=np.cumsum(delta,axis=0).astype('<i2')
        # Verify the inverse transform against every original encoded sample.
        enc=samples.astype(np.int64);enc[1:]=samples[1:].astype(np.int64)-samples[:-1].astype(np.int64)+128
        enc=enc.reshape(-1).astype('<u2')
        back=np.concatenate(((enc&255).astype('u1'),(enc>>8).astype('u1'))).tobytes()
        assert back==bytes(raw[start:start+sz])
        raw[start:start+sz]=samples.tobytes();struct.pack_into('<H',raw,fmt,1)
        transform='lossless UEWavComp reversal; full-sample inverse round-trip verified'
    else: assert code in (1,65534),f'Unsupported WAVE format {code}'
    return bytes(raw),transform

def metrics(raw):
    w=wave.open(io.BytesIO(raw));rate=w.getframerate();ch=w.getnchannels();sw=w.getsampwidth()
    d=w.readframes(w.getnframes())
    if sw==2: iv=np.frombuffer(d,'<i2');x=iv.astype(float)/32768;clip=np.count_nonzero((iv==32767)|(iv==-32768))
    elif sw==3:
        t=np.frombuffer(d,'u1').reshape(-1,3).astype(np.int32);iv=t[:,0]+(t[:,1]<<8)+(t[:,2]<<16);iv=(iv^0x800000)-0x800000;x=iv/8388608;clip=np.count_nonzero((iv==8388607)|(iv==-8388608))
    else: raise ValueError('Unsupported sample width')
    x=x.reshape(-1,ch);db=lambda a:round(20*np.log10(max(float(a),1e-12)),3)
    n=min(len(x),round(rate*.02));mono=x.mean(axis=1);take=mono[:min(len(mono),3*rate)]
    ft=abs(np.fft.rfft(take*np.hanning(len(take))))**2;freq=np.fft.rfftfreq(len(take),1/rate)
    # Boundary values are diagnostics, not a judgment that a loop is perceptually seamless.
    return dict(duration=round(len(x)/rate,6),rate=rate,bits=sw*8,channels=ch,peak_dbfs=db(abs(x).max()),rms_dbfs=db(np.sqrt(np.mean(x*x))),full_scale_samples=int(clip),head20ms_dbfs=db(np.sqrt(np.mean(x[:n]**2))),tail20ms_dbfs=db(np.sqrt(np.mean(x[-n:]**2))),first_sample_max=float(abs(x[0]).max()),last_sample_max=float(abs(x[-1]).max()),loop_boundary_step=float(abs(x[0]-x[-1]).max()),high6k_energy_percent=round(100*ft[freq>6000].sum()/max(ft.sum(),1e-20),3))

def main():
    PRIVATE.mkdir(parents=True,exist_ok=True)
    inventory=[]
    for pid,(title,lid) in PACKS.items():
        files=[p for p in (VAULT/pid/'data').rglob('*') if p.is_file()]
        inventory.append(dict(title=title,id=lid,cache_id=pid,files=len(files),bytes=sum(p.stat().st_size for p in files),status='local native/raw files present' if files else 'not found in scanned cache',listing='https://www.fab.com/listings/'+lid))
    cat=json.loads(base64.b64decode(Path(r'C:\ProgramData\Epic\EpicGamesLauncher\Data\Catalog\catcache.bin').read_bytes()))
    owned=[dict(title=r['title'],id=r.get('customAttributes',{}).get('ListingIdentifier',{}).get('value')) for r in cat if re.search(r'sound|music|\bbgm\b|sfx',r.get('title',''),re.I) and not re.search(r'ZoneManager|Persistent Music',r.get('title',''))]
    (SOURCE/'inventory.json').write_text(json.dumps(dict(verified_utc=datetime.datetime.now(datetime.UTC).isoformat(),local=inventory,library_catalog=owned),indent=2),encoding='utf8')
    selections=TOP.copy()
    for pid,names in EXTRA_NAMES.items():
        for name in names:
            use='Cinematic / exploration music' if pid.startswith('Atmos') else 'Alternative / diagnostic audition'
            note='Composer explicitly allows commercial projects; no AI per publisher. Full track preserved; not a combat-intensity implementation.' if pid.startswith('Atmos') else 'Alternative only. Listen for harshness, physical plausibility and natural decay before promoting.'
            if name=='Sci-Fi_Interface_8-1':note='Not shortlisted: very high measured high-frequency energy; may conflict with the requested soft direction.'
            selections.append((pid,name,use,note))
    results=[]
    for idx,(pid,name,use,note) in enumerate(selections):
        matches=list((VAULT/pid/'data').rglob(name+'.uasset'));assert len(matches)==1,(pid,name,matches)
        p=matches[0];b=p.read_bytes();before=sha(b);raw,method=payload(b);wav,transform=decode_wave(raw)
        path=PRIVATE/(name+'.wav');path.write_bytes(wav)
        assert sha(p.read_bytes())==before,'Source changed during audit'
        m=metrics(wav)
        versions=sorted(set(x.decode('ascii','replace') for x in re.findall(rb'\+\+UE[45]\+[A-Za-z0-9_.+-]+',b[:16384])))
        results.append(dict(rank=idx+1 if idx<5 else None,pack=PACKS[pid][0],listing='https://www.fab.com/listings/'+PACKS[pid][1],asset=name,source_path=str(p),local_wav=str(path),source_sha256=before,wav_sha256=sha(wav),payload_sha256=sha(raw),method=method,transform=transform,source_versions=versions,intended_use=use,assessment=note,metrics=m))
    manifest=dict(verified_utc=datetime.datetime.now(datetime.UTC).isoformat(),source_preservation_verified=True,perceptual_audition='not performed: this runtime has no auditory perception tool; Nick approval required',integration='none; Unreal not launched',candidates=results)
    (SOURCE/'audit.json').write_text(json.dumps(manifest,indent=2),encoding='utf8')
    def esc(s):return html.escape(str(s))
    def card(r):
        m=r['metrics'];title=('Candidate '+str(r['rank']) if r['rank'] else 'Alternative')+' · '+r['intended_use']
        attrs=' data-top="true"' if r['rank'] else ''
        return f'''<article{attrs}><h3>{esc(title)}</h3><p><a href="{esc(r['listing'])}">{esc(r['pack'])}</a></p><code>{esc(r['asset'])}</code><p>{esc(r['assessment'])}</p><audio controls preload="none" src="{Path(r['local_wav']).as_uri()}"></audio><label><input type="checkbox" class="loop"> Loop this source</label><p class="metrics">{m['duration']:.3f} s · {m['rate']} Hz / {m['bits']}-bit / {m['channels']} ch · peak {m['peak_dbfs']:.2f} dBFS · full-scale samples {m['full_scale_samples']}</p><details><summary>Source, license and measurements</summary><p>Internal listening copy from Nick’s owned library. Fab commercial project rights apply under the acquisition license; exact acquisition tier/version is not recorded in the local catalog. No standalone/public media redistribution. Source pack credits must be listed for the jam. <a href="https://www.fab.com/eula">Fab terms</a>.</p><p>{esc(r['source_path'])}</p><p><a href="{Path(r['local_wav']).as_uri()}">Direct local WAV</a></p><p>First sample max {m['first_sample_max']:.7f}; last {m['last_sample_max']:.7f}; last 20 ms RMS {m['tail20ms_dbfs']:.1f} dBFS; loop step {m['loop_boundary_step']:.7f}. Sample peak is not true peak/LUFS.</p><p>{esc(r['transform'])}. Original SHA-256: <code>{r['source_sha256']}</code></p></details><div><button class="approve">Mark for approval</button><button class="reject">Reject</button><span class="decision">Listening pending</span></div></article>'''
    rows=''.join(f'<tr><td><a href="{x["listing"]}">{esc(x["title"])}</a></td><td>{x["files"]}</td><td>{x["bytes"]/1024**2:.1f} MiB</td><td>{esc(x["status"])}</td></tr>' for x in inventory)
    extra_owned=''.join('<li><a href="https://www.fab.com/listings/'+esc(x['id'])+'">'+esc(x['title'])+'</a></li>' for x in owned if x['id'] and x['id'] not in {v[1] for v in PACKS.values()})
    page='''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Triad Drift · Epic Library Listening Review 01</title><style>body{font:16px system-ui;background:#0d1421;color:#e4edf6;max-width:1040px;margin:auto;padding:30px}h1{font-size:30px}h2{margin-top:38px}a{color:#8fddff}code{overflow-wrap:anywhere;font-size:13px}article{background:#172338;border:1px solid #36506c;border-radius:12px;padding:22px;margin:18px 0}audio{display:block;width:100%;margin:16px 0}button{background:#254460;color:white;border:1px solid #7899b2;border-radius:6px;padding:10px;margin:4px;cursor:pointer}details{padding:12px;background:#101c2d;font-size:14px;overflow-wrap:anywhere}.notice{border-left:4px solid #e9bf64;padding:15px;background:#242330}.metrics{color:#b7cbdc;font-size:14px}.decision{padding:10px;color:#f5d787}td,th{text-align:left;border-bottom:1px solid #36506c;padding:10px}table{width:100%;font-size:14px}label{font-size:14px}.bar{position:sticky;top:0;background:#0d1421;padding:10px;z-index:2;border-bottom:1px solid #36506c}</style><h1>Triad Drift · Existing Library Review 01</h1><p>Five provisional candidates · original sources preserved · private local review only</p><div class="notice"><b>Listening approval required.</b> Selection uses verified source identity, waveform measurements and intended category. This agent runtime cannot hear computer playback; no subjective audition or AAA approval is claimed. No sounds are imported, integrated, processed or normalized. Begin at low volume. Looped cockpit ambience is not a verified spacecraft engine.</div><div class="bar"><button id="sequence">Play five in sequence</button><button id="rapid">Rapid switch comparison (1 ↔ 2)</button><button id="stop">Stop all</button><label>Playback level <input id="level" type="range" min="0" max="1" step="0.05" value="0.35"></label><button id="export">Save listening choices</button><span id="status"></span></div><h2>Five candidates</h2>'''
    page+=''.join(card(r) for r in results[:5])+'<h2>Alternatives and music</h2>'+''.join(card(r) for r in results[5:])
    page+='''<h2>Coverage and license findings</h2><p>No convincing dedicated spacecraft engine/boost set was identified in the scanned local audio packs. Computer ambience is offered only as a cockpit bed. Shield/portal/targeting coverage is not approved. Hove’s explicitly retro beeps are not shortlisted.</p><p>Fab Standard terms allow commercial packaged projects and private collaboration; standalone raw redistribution is forbidden and attribution is generally optional. Exact acquisition license/tier is not exposed by the cached entitlement record, so retain this as a release verification gate. Jam rules require listing third-party assets regardless. Do not commit or publish these WAVs.</p><p>Atmoshpheric Worlds explicitly permits commercial and non-commercial projects with optional credit; composer Marika Schanz, publisher says no AI. EFM’s additional license document returned 404: hold that pack pending clarification and free-acquisition confirmation. Free Sci-Fi Music Pack Volume 2 carries a generated-with-AI flag, while its author describes AI mastering only: disclose that discrepancy if selected. No AI generation or model input was used in this audit.</p><p>Audio bytes decode locally. Native asset loading, UE 5.9 cook, gameplay and actual loop quality remain untested; no Unreal editor was launched. Technical compatibility of decoded PCM WAV does not certify native assets.</p><h2>Local cache inventory</h2><table><tr><th>Pack</th><th>Files</th><th>Size</th><th>Accessibility</th></tr>'''+rows+'''</table><h2>Additional owned catalog entries · no matching local audio payload found</h2><p>Acquisition dates are not recorded here, so recently added status cannot be proven. These require download/access verification; none was downloaded during this audit. Ownership is not proof of a free commercial license.</p><ul>'''+extra_owned+'''</ul><p><a href="audit.json">Complete metrics and source hashes</a> · <a href="inventory.json">Catalog inventory</a></p><script>
const audios=[...document.querySelectorAll('audio')],cards=[...document.querySelectorAll('article')],status=document.querySelector('#status');let cancelled=0;
audios.forEach((a,i)=>{a.volume=.35;a.addEventListener('play',()=>audios.forEach(b=>{if(b!==a)b.pause()}));a.addEventListener('error',()=>status.textContent='File unavailable: '+a.getAttribute('src'));cards[i].querySelector('.loop').onchange=e=>a.loop=e.target.checked;['approve','reject'].forEach(c=>cards[i].querySelector('.'+c).onclick=()=>cards[i].querySelector('.decision').textContent=c==='approve'?'Selected for Nick approval':'Rejected');});
function stop(){cancelled++;audios.forEach(a=>{a.pause();a.currentTime=0})}document.querySelector('#stop').onclick=stop;document.querySelector('#level').oninput=e=>audios.forEach(a=>a.volume=Number(e.target.value));
async function sequence(ids,gap){stop();const run=cancelled;for(const id of ids){if(run!==cancelled)break;const a=audios[id];a.loop=false;a.currentTime=0;status.textContent='Playing '+(id+1);try{await a.play();await new Promise(resolve=>{const timer=setInterval(()=>{if(a.ended||a.paused||run!==cancelled){clearInterval(timer);resolve()}},100)})}catch(e){status.textContent='Playback unavailable: '+e.message;break}await new Promise(r=>setTimeout(r,gap))}if(run===cancelled)status.textContent='Sequence finished'}
document.querySelector('#sequence').onclick=()=>sequence([0,1,2,3,4],350);document.querySelector('#rapid').onclick=()=>sequence([0,1,0,1],120);document.querySelector('#export').onclick=()=>{const data=cards.map(c=>({asset:c.querySelector('code').textContent,choice:c.querySelector('.decision').textContent}));const a=document.createElement('a');a.href=URL.createObjectURL(new Blob([JSON.stringify(data,null,2)],{type:'application/json'}));a.download='TriadDrift-listening-choices.json';a.click();URL.revokeObjectURL(a.href)};
</script></html>'''
    (SOURCE/'index.html').write_text(page,encoding='utf8')
    print(json.dumps(dict(candidates=len(results),private_wav_bytes=sum(Path(r['local_wav']).stat().st_size for r in results),sources_unchanged=True,top_five=[dict(asset=r['asset'],**r['metrics']) for r in results[:5]],page=str(SOURCE/'index.html')),indent=2))

if __name__=='__main__':main()

"""Preserve Nick's Cinder originals and prepare complete import masters only."""
from pathlib import Path
import json,hashlib,shutil,subprocess,os,sys,wave
ROOT=Path(__file__).resolve().parent
PROJECT=ROOT.parents[2]
PRIVATE=PROJECT/'Saved/MusicEvents01/CinderIntake01'
FF='C:/Program Files/Shotcut/ffmpeg.exe'
os.environ['PATH']='C:/Program Files/Shotcut;'+os.environ['PATH']
sys.path.insert(0,str(PROJECT/'.claude/skills/ue-game-audio/scripts'));import audio_common as ac
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
checkpoint=json.loads((PROJECT/'Saved/MusicEvents01/source_checkpoint_01.json').read_text())
assert all(digest(PROJECT/p)==h for p,h in checkpoint['changed_files'].items())
receipts=json.loads((PRIVATE/'source_receipts.json').read_text())
names={'Cinder World.mp3':('exploration','MUS_Cinder_Exploration_World_Full_01'),'Cinder Fall.mp3':('combat','MUS_Cinder_Combat_Fall_Full_01')}
results=[]
for receipt in receipts:
 name=receipt['name'];role,asset=names[name]
 original=Path(receipt['private_preserved_copy']);assert digest(original)==receipt['sha256']
 source=ROOT/'source'/name;source.parent.mkdir(exist_ok=True,parents=True);assert not source.exists()
 shutil.copy2(original,source);assert digest(source)==receipt['sha256']
 floating=PRIVATE/(asset+'_decode_f32.wav');assert not floating.exists()
 subprocess.run([FF,'-n','-nostdin','-v','error','-i',str(source),'-map','0:a:0','-ar','48000','-ac','2','-c:a','pcm_f32le',str(floating)],check=True)
 before=ac.measure(str(floating))
 gain=min(0.,-14.1-before['loudness_lufs'],-1.-before['true_peak_dbtp'])
 out=ROOT/'export'/(asset+'.wav');out.parent.mkdir(exist_ok=True);assert not out.exists()
 command=[FF,'-n','-nostdin','-v','error','-i',str(floating)]
 if gain<0:command+=['-af','volume=%.8fdB'%gain]
 command+=['-c:a','pcm_s16le',str(out)];subprocess.run(command,check=True)
 qa=ac.measure(str(out));findings=ac.check(qa,'music');assert not findings,findings
 with wave.open(str(out)) as w:
  assert (w.getframerate(),w.getnchannels(),w.getsampwidth())==(48000,2,2)
  frames=w.getnframes();pcm=w.readframes(frames)
 # Separate full source decode verifies that no musical frames were trimmed/looped.
 independent=[FF,'-nostdin','-v','error','-i',str(source),'-map','0:a:0','-ar','48000','-ac','2']
 if gain<0:independent+=['-af','volume=%.8fdB'%gain]
 independent+=['-c:a','pcm_s16le','-f','s16le','pipe:1']
 reference=subprocess.check_output(independent)
 assert pcm==reference,'Independent complete decode differs'
 assert digest(source)==receipt['sha256'] and digest(original)==receipt['sha256']
 results.append(dict(title=name,role=role,role_approval='Nick explicitly: Cinder World for exploration; Cinder Fall for combat',source=str(source),source_sha256=digest(source),source_bytes=source.stat().st_size,original_located_path=receipt['located_path'],original_supplied_path=receipt['supplied_path'],output=str(out),output_sha256=digest(out),frames=frames,duration_s=frames/48000,sample_rate=48000,channels=2,bits=16,technical_gain_db=gain,processing='Full decode; linear gain only if required by music limits. No trim/loop/crossfade/tempo/pitch/endpoint fade.',independent_full_decode_pcm_identical=True,original_unchanged=True,decoded_before_gain=before,qa=qa,category_findings=findings,proposed_native_path='/Game/TriadDrift/Audio/CinderMusic01/'+asset,native_imported=False,gameplay_connected=False))
assert all(digest(PROJECT/p)==h for p,h in checkpoint['changed_files'].items())
(ROOT/'source_manifest.json').write_text(json.dumps(dict(claim='f379bb6e',stage='Source masters prepared and measured only; native import/event hookup pending',provenance='Nicholas supplied these tracks and selected them for Triad Drift. No library acquisition, purchase or generation performed here.',combat_entry_note='Nick notes both have slow starts; a Cinder Fall beat-drop entry should be auditioned. No offset approved/applied; full master retained.',tracks=results),indent=2))
print(json.dumps([dict(title=r['title'],duration_s=r['duration_s'],gain_db=r['technical_gain_db'],peak=r['qa']['true_peak_dbtp'],lufs=r['qa']['loudness_lufs'],pcm_identity=r['independent_full_decode_pcm_identical']) for r in results],indent=2))

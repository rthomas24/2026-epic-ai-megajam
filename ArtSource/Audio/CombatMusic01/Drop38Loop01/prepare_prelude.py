"""Unpadded0..38s prelude for the real tutorial handoff; no fixed cinematic clock."""
from pathlib import Path
import hashlib,json,wave
ROOT=Path(__file__).resolve().parents[4]
SOURCE=ROOT/'Saved/AudioMusic/PrototypeV1/CombatMusic01/MUS_Combat_EtherealPulseRemix_01.wav'
OUT=ROOT/'Saved/AudioMusic/PrototypeV1/CombatMusic01/Prelude38_01'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(SOURCE)=='a705d8a00948cbf96bb4bbd33911cffa35e1cad102c7abaa1a839739df2a44a5'
assert not OUT.exists(), 'Preserve existing output'
with wave.open(str(SOURCE)) as w:
    assert (w.getframerate(),w.getnchannels(),w.getsampwidth())==(48000,2,2)
    frames=w.readframes(1824000)
assert len(frames)==1824000*4
OUT.mkdir(parents=True);file=OUT/'MUS_Tutorial_EtherealPulse_Prelude38_01.wav'
with wave.open(str(file),'wb') as w:
    w.setnchannels(2);w.setsampwidth(2);w.setframerate(48000);w.writeframes(frames)
receipt={'stage':'Source-only prelude, native import/binding and real control synchronization pending',
         'path':str(file),'sha256':sha(file),'duration_s':38,'frames':1824000,'rate':48000,'channels':2,'bits':16,
         'source_range_s':[0,38],'padding_s':0,'time_stretch':False,'extra_normalization':False,
         'same_source_gain_as_loop':True,'end_handling':'No tail fade: final frame immediately precedes loop38s first frame; intended continuous handoff, not standalone ending',
         'first_loop_sample_original_frame':1824000,'runtime_requirements':['Real control event at beat38','Native class and volume must match57sloop','No fixed49scinematic/11sdelay assumption','Earlyskip must reach beatdrop without duplicated or truncated music'],
         'source_preserved':sha(SOURCE)=='a705d8a00948cbf96bb4bbd33911cffa35e1cad102c7abaa1a839739df2a44a5'}
(OUT/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt,indent=2))

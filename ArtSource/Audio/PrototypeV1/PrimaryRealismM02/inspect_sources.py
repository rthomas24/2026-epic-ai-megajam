"""Inspect approved cached source material; never changes V1 or original packages."""
from pathlib import Path
import importlib.util, hashlib, json, subprocess, sys, wave, io
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
PRIVATE=ROOT/'Saved/AudioMusic/PrototypeV1/PrimaryRealismM02'
VAULT=Path(r'C:\ProgramData\Epic\EpicGamesLauncher\VaultCache')
AUDIT=Path(r'C:\Users\nicho\OneDrive\Desktop\GameJam\Worktrees\audio-music\ArtSource\Audio\EpicLibraryAudit01\build_audit.py')
PACK50='50FreeSo92fbc86158f1V1'
PACKTERM='Terminal898ca54d9692V1'
NAMES=[(PACK50,x) for x in ['Gunshot_1-1','Gunshot_7-1','Draw_Weapon_Metal_1-1']]+[(PACKTERM,'MACHAppl_VCRButton_HA_TerminalUI_'+n+'_Wav') for n in ['04','08','13']]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 PRIVATE.mkdir(parents=True,exist_ok=True)
 baseline=PRIVATE/'v1_preservation_hashes.json'
 if not baseline.exists():
  files=list((ROOT/'Saved/AudioMusic/PrototypeV1/Host/Content/TriadDrift/Audio/PrototypeV1').rglob('*'))
  files+=[ROOT/'Saved/AudioMusic/PrototypeV1/Host/Saved/BouncedWavFiles/ue-game-audio/V1_Primary_Five.wav']
  baseline.write_text(json.dumps([dict(path=str(p),sha256=sha(p)) for p in files if p.is_file()],indent=2))
 spec=importlib.util.spec_from_file_location('audit',AUDIT);a=importlib.util.module_from_spec(spec);spec.loader.exec_module(a)
 folder=PRIVATE/'source';folder.mkdir(exist_ok=True);rows=[]
 for pack,name in NAMES:
  matches=list((VAULT/pack/'data').rglob(name+'.uasset'));assert len(matches)==1,(name,matches)
  p=matches[0];before=sha(p);raw,method=a.payload(p.read_bytes());wav,decode=a.decode_wave(raw)
  target=folder/(name+'.wav');target.write_bytes(wav)
  with wave.open(io.BytesIO(wav),'rb') as w:info=dict(rate=w.getframerate(),channels=w.getnchannels(),bits=w.getsampwidth()*8,duration=w.getnframes()/w.getframerate())
  row=dict(asset=name,original_path=str(p),source_sha256=before,decoded_path=str(target),decoded_sha256=sha(target),pack=a.PACKS[pack][0],listing='https://www.fab.com/listings/'+a.PACKS[pack][1],extraction=method,restoration=decode,**info)
  assert sha(p)==before;rows.append(row)
 (HERE/'source_inventory.json').write_text(json.dumps(rows,indent=2))
 print(json.dumps([{k:r[k] for k in ['asset','duration','rate','channels']} for r in rows],indent=2))
if __name__=='__main__':main()

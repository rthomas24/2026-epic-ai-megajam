"""Copy only nine freshly reloaded Waves into the claimed feature worktree."""
from pathlib import Path
import json,hashlib,shutil,subprocess
ROOT=Path(__file__).resolve().parent
FEATURE=ROOT.parents[3]
HOST=FEATURE.parent/'audio-tutorial-production-64490fb'
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert subprocess.check_output(['git','branch','--show-current'],cwd=FEATURE,text=True).strip()=='nick/audio-pr12-replacements-1113'
assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=FEATURE,text=True).strip()=='69c18e33e519096eea065988e6874bd7f86c9cd5'
run=HOST/'Saved/PR12Provided01'
for name in ('Import01','Reload02','GameplayAudioOnly02'):
 r=json.loads((run/name/'guard_result.json').read_text());assert r['exit_code']==0
 if name!='Import01':assert not r.get('changed',r.get('changed_protected')) and not r['new_content']
freeze=json.loads((ROOT.parent/'native_package_freeze.json').read_text())['packages']
before={r['path']:digest(FEATURE/r['path']) for r in freeze}
assert all(before[r['path']]==r['sha256'] for r in freeze)
manifest=json.loads((ROOT/'replacement_manifest.json').read_text())
relative=['Content/'+r['asset_path'].removeprefix('/Game/')+'.uasset' for r in manifest['rows']]
backup=FEATURE/'Saved/PR12Provided01/FeatureOriginalWaves'
assert not backup.exists();backup.mkdir(parents=True)
for rel in relative:
 p=FEATURE/rel;source=HOST/rel;assert digest(source)!=before[rel]
 shutil.copy2(p,backup/p.name);shutil.copy2(source,p)
after={r['path']:digest(FEATURE/r['path']) for r in freeze}
assert {p for p in before if before[p]!=after[p]}==set(relative)
assert all(digest(FEATURE/p)==digest(HOST/p) for p in relative)
receipt=dict(stage='Native nine-Wave replacements copied from tested private host; not Main-integrated or Shipping-tested',changed=relative,sha256={p:after[p] for p in relative},original_backup=str(backup),unchanged_non_wave_packages=[p for p in before if before[p]==after[p]],import_result=json.loads((run/'Import01/guard_result.json').read_text()),reload_result=json.loads((run/'Reload02/guard_result.json').read_text()),gameplay_result=json.loads((run/'GameplayAudioOnly02/guard_result.json').read_text()),gameplay_audio=json.loads((ROOT/'gameplay_audio_qa.json').read_text()))
(ROOT/'native_handoff.json').write_text(json.dumps(receipt,indent=2))
print('Copied nine tested Waves; other eight PR12 packages unchanged.')

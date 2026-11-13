"""Run only under Lead's exact transfer grant after fresh all-five reload passes."""
from pathlib import Path
import hashlib,json,shutil,subprocess
ROOT=Path(__file__).resolve().parents[1];REPO=ROOT.parents[2]
HOST=REPO/'Saved/WeaponIntake01/Host';CLONE=REPO/'Saved/WeaponIntake01/GameValidation'
EVIDENCE=ROOT/'evidence';REPORT=EVIDENCE/'project_transfer_report.json'
PREFIX=Path('TriadDrift/Art/Weapons/MeshyIntake01')
assert subprocess.check_output(['git','branch','--show-current'],cwd=REPO,text=True).strip()=='nick/assets-weapons-intake','Wrong branch'
assert not REPORT.exists(),'Preserve prior transfer evidence'
reload=json.loads((EVIDENCE/'all_saved_reload_report.json').read_text())
assert reload['passed'] and reload['native_hashes_unchanged'],'Fresh saved-package reload required'
hashes=reload['packages_sha256'];assert len(hashes)==27,'Unexpected package count'
game=REPO/'Content';clone=CLONE/'Content'
for target in [game/PREFIX,clone/PREFIX]:
    assert not target.exists() or not any(target.rglob('*')),'Refuse nonempty target: '+str(target)
for relative,sha in hashes.items():
    rel=Path(relative)
    assert not rel.is_absolute() and '..' not in rel.parts and rel.is_relative_to(PREFIX),'Path escapes owned root'
    assert hashlib.sha256((HOST/'Content'/rel).read_bytes()).hexdigest()==sha,'Authoring package changed'
    for target in [game,clone]:assert not (target/rel).exists(),'Refuse overwrite'
report=dict(passed=False,stage='Verified exact owned game Content and clone transfer; native clone reload still required',files=[],errors=[])
try:
    for relative,sha in hashes.items():
        src=HOST/'Content'/relative
        dst=game/relative;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst)
        copied=clone/relative;copied.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(dst,copied)
        assert all(hashlib.sha256(p.read_bytes()).hexdigest()==sha for p in [src,dst,copied]),'Three-way copy mismatch'
        report['files'].append(dict(relative=relative,sha256=sha,game=str(dst),clone=str(copied),bytes=dst.stat().st_size))
    descriptor=CLONE/'WeaponIntake.uproject';assert not descriptor.exists(),'Refuse existing clone host descriptor'
    shutil.copy2(HOST/'WeaponIntake.uproject',descriptor)
    config=CLONE/'Config/DefaultEngine.ini';config.parent.mkdir(exist_ok=True)
    assert not config.exists(),'Refuse existing clone configuration'
    short_cache=REPO/'Saved/WpnDDC02'
    assert len(str(short_cache).replace('\\','/'))<119,'Unreal filesystem DDC path limit'
    config.write_text((HOST/'Config/DefaultEngine.ini').read_text().replace(str(HOST/'DerivedDataCache').replace('\\','/'),str(short_cache).replace('\\','/')),encoding='utf-8')
    report.update(passed=True,game_content_root=str(game/PREFIX),clone_host=str(CLONE),project_clone_reload_pending=True)
except Exception as error:
    report['errors'].append(repr(error))
REPORT.write_text(json.dumps(report,indent=2),encoding='utf-8')
assert report['passed'],report['errors']
print(json.dumps(dict(passed=True,packages=len(report['files']),game_content_root=report['game_content_root'],native_clone_reload_pending=True)))

from pathlib import Path
import sys,traceback
sys.path.insert(0,str(Path(__file__).resolve().parent))
from weapon_common import *
report=dict(passed=False,stage='Read-only four saved weapon recovery after stopped watchdog; fifth remains unimported',assets=[],errors=[])
def fingerprints():
    return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (HOST/'Content/TriadDrift/Art/Weapons/MeshyIntake01').rglob('*') if p.is_file()}
before=fingerprints()
try:
    guard()
    assert not (EVIDENCE/'four_recovery_report.json').exists(),'Preserve earlier evidence'
    preserved=json.loads((EVIDENCE/'four_preserved_packages.json').read_text(encoding='utf-8-sig'))
    assert before==preserved['packages_sha256'] and len(before)==22,'Preserved package set changed'
    assert not unreal.EditorAssetLibrary.list_assets(MANIFEST['assets'][4]['proposed_destination'],recursive=True),'Fifth destination not empty'
    for row in MANIFEST['assets'][:4]:report['assets'].append(inspect(row))
    assert fingerprints()==before,'Native files changed during read-only reload'
    guard();report['packages_sha256']=before;report['native_hashes_unchanged']=True;report['passed']=True
except Exception:report['errors'].append(traceback.format_exc())
write('four_recovery_report.json',report)
if not report['passed']:raise RuntimeError(report['errors'][-1])

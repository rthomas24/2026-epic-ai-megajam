from pathlib import Path
import sys,traceback
sys.path.insert(0,str(Path(__file__).resolve().parent))
from weapon_common import *
report=dict(passed=False,stage='Read-only preserved Neon package recovery; no reimport or game transfer',assets=[],errors=[])
def fingerprints():
    return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (HOST/'Content/TriadDrift/Art/Weapons/MeshyIntake01').rglob('*') if p.is_file()}
before=fingerprints()
try:
    guard()
    assert not (EVIDENCE/'neon_recovery_report.json').exists(),'Preserve previous recovery evidence'
    preserved=json.loads((EVIDENCE/'first_failure_preserved.json').read_text(encoding='utf-8-sig'))
    assert before=={p['path']:p['sha256'] for p in preserved['packages']},'Preserved native package set/hash changed'
    assert len(before)==6,'Unexpected preserved package set'
    report['assets'].append(inspect(MANIFEST['assets'][0]))
    guard()
    assert fingerprints()==before,'Native packages changed during read-only recovery'
    report['native_hashes_unchanged']=True
    report['passed']=True
except Exception:report['errors'].append(traceback.format_exc())
write('neon_recovery_report.json',report)
if not report['passed']:raise RuntimeError(report['errors'][-1])

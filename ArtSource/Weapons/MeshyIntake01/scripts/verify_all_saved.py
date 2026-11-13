from pathlib import Path
import sys,traceback
sys.path.insert(0,str(Path(__file__).resolve().parent))
from weapon_common import *
report=dict(passed=False,stage='Fresh five-weapon saved-package reload; not rendered appearance or gameplay acceptance',assets=[],errors=[])
def fingerprints():
    return {str(p.relative_to(HOST/'Content')):hashlib.sha256(p.read_bytes()).hexdigest() for p in (HOST/'Content/TriadDrift/Art/Weapons/MeshyIntake01').rglob('*') if p.is_file()}
before=fingerprints()
try:
    guard()
    assert not (EVIDENCE/'all_saved_reload_report.json').exists(),'Preserve prior evidence'
    assert json.loads((EVIDENCE/'neon_recovery02_report.json').read_text())['passed'],'Neon recovery required'
    four=json.loads((EVIDENCE/'four_recovery_report.json').read_text())
    final=json.loads((EVIDENCE/'final_import_report.json').read_text())
    assert four['passed'] and four['native_hashes_unchanged'],'Four saved weapon recovery required'
    assert final['passed'] and final['preserved_22_hashes_unchanged'],'Final weapon import required'
    assert len(final['imports'])==1 and final['imports'][0]['name']==MANIFEST['assets'][4]['name'],'Expected fifth weapon only'
    for row in MANIFEST['assets']:report['assets'].append(inspect(row))
    assert sum(len(a['textures']) for a in report['assets'])==17,'Texture total mismatch'
    assert len(before)==27,'Expected exactly five meshes, five materials, seventeen textures'
    assert fingerprints()==before,'Saved native packages changed in read-only reload'
    guard();report['packages_sha256']=before;report['native_hashes_unchanged']=True;report['passed']=True
except Exception:report['errors'].append(traceback.format_exc())
write('all_saved_reload_report.json',report)
if not report['passed']:raise RuntimeError(report['errors'][-1])

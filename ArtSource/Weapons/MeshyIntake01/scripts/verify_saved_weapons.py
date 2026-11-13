from pathlib import Path
import sys,traceback
sys.path.insert(0,str(Path(__file__).resolve().parent))
from weapon_common import *
report=dict(passed=False,stage='Fresh saved-package structural reload, not rendered material acceptance',assets=[],errors=[])
try:
    guard();assert json.loads((EVIDENCE/'import_report.json').read_text())['passed'],'Import did not pass'
    for row in MANIFEST['assets']:report['assets'].append(inspect(row))
    report['passed']=True
except Exception:report['errors'].append(traceback.format_exc())
write('reload_report.json',report)
if not report['passed']:raise RuntimeError(report['errors'][-1])

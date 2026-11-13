"""Non-overwriting exact package copy. Requires separate Lead transfer grant."""
import json,shutil,subprocess,traceback
from intake_common import *
assert subprocess.check_output(['git','branch','--show-current'],cwd=REPO,text=True).strip()=='nick/assets-meshy-intake02'
assert not (EVIDENCE/'project_transfer.json').exists(),'Preserve earlier transfer'
handoff=json.loads((EVIDENCE/'manager_handoff.json').read_text())
assert handoff['passed'];expected=handoff['packages']
source_guard();verify_files(MANAGER/'Content',expected)
for relative in expected:
    rel=Path(relative)
    assert not rel.is_absolute() and '..' not in rel.parts and any(rel.is_relative_to(r) for r in RELATIVE_ROOTS),'Path outside claim'
for content in [REPO/'Content',HOST/'Content']:
    assert not owned_files(content),'Refuse existing destination files'
report=dict(passed=False,files=[],errors=[],native_clone_reload_pending=True)
try:
    for relative,info in expected.items():
        original=MANAGER/'Content'/relative
        target=REPO/'Content'/relative;target.parent.mkdir(parents=True,exist_ok=True)
        assert not target.exists();shutil.copy2(original,target)
        clone=HOST/'Content'/relative;clone.parent.mkdir(parents=True,exist_ok=True)
        assert not clone.exists();shutil.copy2(target,clone)
        report['files'].append(dict(relative=relative,**info))
    for content in [MANAGER/'Content',REPO/'Content',HOST/'Content']:verify_files(content,expected)
    source_guard();report['passed']=True
except Exception:report['errors'].append(traceback.format_exc())
write('project_transfer.json',report)
assert report['passed'],report['errors']
print(json.dumps(dict(passed=True,packages=len(expected),clone_reload_pending=True)))

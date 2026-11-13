from pathlib import Path
import sys,traceback,json,hashlib
sys.path.insert(0,str(Path(__file__).resolve().parent))
import weapon_common as qa
authoring=qa.HOST
qa.HOST=qa.REPO/'Saved/WeaponIntake01/GameValidation'
report=dict(passed=False,stage='Fresh isolated clone reload of exact game Content packages; no gameplay or rendered look acceptance',assets=[],errors=[])
try:
    qa.guard()
    assert not (qa.EVIDENCE/'project_clone_report.json').exists(),'Preserve prior evidence'
    reload=json.loads((qa.EVIDENCE/'all_saved_reload_report.json').read_text())
    assert reload['passed'] and reload['native_hashes_unchanged'],'Fresh authoring reload required'
    package_hashes=reload['packages_sha256']
    assert len(package_hashes)==27,'Expected exact package set'
    def validate_copies():
        for root in [authoring/'Content',qa.REPO/'Content',qa.HOST/'Content']:
            actual={str(p.relative_to(root)) for p in (root/'TriadDrift/Art/Weapons/MeshyIntake01').rglob('*') if p.is_file()}
            assert actual==set(package_hashes),'Package set mismatch in '+str(root)
            for relative,sha in package_hashes.items():
                assert hashlib.sha256((root/relative).read_bytes()).hexdigest()==sha,'Package hash mismatch in '+str(root/relative)
    validate_copies()
    for row in qa.MANIFEST['assets']:report['assets'].append(qa.inspect(row))
    validate_copies();qa.guard()
    report.update(passed=True,three_way_hashes_unchanged=True,project_content_root=str(qa.REPO/'Content/TriadDrift/Art/Weapons/MeshyIntake01'),validation_host=str(qa.HOST),packages_sha256=package_hashes)
except Exception:report['errors'].append(traceback.format_exc())
qa.write('project_clone_report.json',report)
if not report['passed']:raise RuntimeError(report['errors'][-1])

"""Read-only manager handoff. Run with exact reports only after Lead review."""
import argparse,json,shutil
from intake_common import *
p=argparse.ArgumentParser()
p.add_argument('--intake-report',required=True)
p.add_argument('--pilot-report',required=True)
p.add_argument('--process-report',required=True)
args=p.parse_args()
source_guard()
assert not (EVIDENCE/'manager_handoff.json').exists(),'Preserve prior handoff'
paths=[Path(args.intake_report),Path(args.pilot_report),Path(args.process_report)]
assert all(p.resolve().is_relative_to(MANAGER.resolve()) for p in paths),'Report outside manager host'
intake,pilot,process=[json.loads(p.read_text(encoding='utf-8-sig')) for p in paths]
assert intake.get('technical_import_passed') is True and not intake.get('errors'),'Private imports did not pass'
assert pilot.get('passed') is True and not pilot.get('errors'),'Saved pilot reload did not pass'
assert process.get('Passed') is True and process.get('ExitCodeKnown') is True and process.get('ActualExitCode')==0 and not process.get('AbortReason') and not process.get('Errors'),'Native process not cleanly successful'
baseline=json.loads((MANAGER/'Evidence/import_results.json').read_text())
groups=baseline['imports']+intake['imports']
assert len(groups)==14 and {g['name'] for g in groups}=={a['name'] for a in MANIFEST['assets']},'Exact fourteen groups required'
expected={};objects={}
for source in MANIFEST['assets']:
    group=next(g for g in groups if g['name']==source['name'])
    assert source['name'] in ['CrimsonTempest','SentinelArmor'] or group.get('import_checks_passed') is True
    classes=[e['class'] for e in group['assets']]
    skinned=bool(source['skins'])
    assert classes.count('SkeletalMesh' if skinned else 'StaticMesh')==1,'Expected one source mesh'
    assert classes.count('Texture2D')==source['images'] and classes.count('Material')==source['materials'],'Source material/texture counts changed'
    assert classes.count('Skeleton')==int(skinned) and classes.count('AnimSequence')==len(source['animations']),'Rig/clip counts changed'
    for entry in group['assets']:
        package=entry['path'].split('.')[0]
        assert allowed_package(package) and package.startswith(source['destination']+'/'),'Object outside exact claim'
        assert entry['class'] in ['StaticMesh','SkeletalMesh','Skeleton','AnimSequence','Material','MaterialInstanceConstant','Texture2D'],'Unreviewed asset class'
        relative=package.removeprefix('/Game/')+'.uasset'
        path=MANAGER/'Content'/relative
        assert path.is_file(),'Missing saved package'
        expected[relative]=dict(bytes=path.stat().st_size,sha256=digest(path))
        objects[entry['path']]=entry
assert len(expected)==len(objects),'Unexpected multiple asset objects per package'
assert len(expected)==91,'Expected 14 meshes,14 materials,50 textures,2 skeletons,11 clips'
verify_files(MANAGER/'Content',expected)
source_guard()
for name,path in zip(['manager_intake_pass.json','manager_pilot_reload_pass.json','manager_process_pass.json'],paths):
    target=EVIDENCE/name;assert not target.exists();shutil.copy2(path,target)
write('manager_handoff.json',dict(passed=True,manager_root=str(MANAGER),packages=expected,objects=objects,groups=groups,source_reports=[dict(path=str(p),sha256=digest(p)) for p in paths],project_transfer_pending=True))
print(json.dumps(dict(passed=True,groups=14,packages=len(expected),handoff_sha256=digest(EVIDENCE/'manager_handoff.json'))))

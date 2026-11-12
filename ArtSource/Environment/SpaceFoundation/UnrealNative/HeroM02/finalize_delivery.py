"""Freeze a review manifest after real native evidence exists; never creates evidence."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

HERE=Path(__file__).resolve().parent
REPO=HERE.parents[4]
CONTENT=REPO/'Content/TriadDrift/Environment/SpaceFoundation/HeroM02'


def main():
    subprocess.run([sys.executable,'-B',str(HERE/'baseline_guard.py')],check=True)
    verification=json.loads((HERE/'review/verification.json').read_text())
    assert verification['diagnostics_verified'] and not verification['engine_errors']
    assert verification['fresh_process_reload']==6
    assert verification['identical_static_captures'],'Review temporal stability before delivery'
    native=sorted(CONTENT.rglob('*.uasset'))+sorted(CONTENT.rglob('*.umap'))
    assert len(native)==7,[str(p) for p in native]
    for path in native:assert path.read_bytes()[:4]==bytes.fromhex('c1832a9e'),path
    files=sorted(p for p in HERE.rglob('*') if p.is_file() and 'Saved' not in p.relative_to(HERE).parts
                 and '__pycache__' not in p.parts and p.name!='delivery_manifest.json')+native
    manifest={'branch':'nick/environment-vfx','stages':{'prototype_complete':True,'technically_verified':True,
        'presentation_approved':False,'integrated':False,'gameplay_validated':False},
        'protected_baseline_files':35,'native_file_count':len(native),
        'files':{p.relative_to(REPO).as_posix():{'bytes':p.stat().st_size,
                 'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in files}}
    (HERE/'review/delivery_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('Review manifest frozen:',len(files),'files;',len(native),'native; manual approval pending')


if __name__=='__main__':main()

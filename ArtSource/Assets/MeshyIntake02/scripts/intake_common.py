from pathlib import Path
import hashlib,json
ROOT=Path(__file__).resolve().parents[1]
REPO=ROOT.parents[2]
EVIDENCE=ROOT/'evidence'
HOST=REPO/'Saved/I02V/Host'
MANIFEST=json.loads((ROOT/'intake_manifest.json').read_text(encoding='utf-8-sig'))
MANAGER=Path(MANIFEST['private_manager_root'])
DESTINATIONS=[r['destination'] for r in MANIFEST['assets']]
RELATIVE_ROOTS=[Path(r['project_folder']).relative_to('Content') for r in MANIFEST['assets']]
def digest(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
def write(name,data):
    (EVIDENCE/name).write_text(json.dumps(data,indent=2,default=str),encoding='utf-8')
def source_guard():
    for row in MANIFEST['assets']:
        original=Path(row['original'])
        assert digest(original)==row['sha256'] and digest(row['source_copy'])==row['sha256'],'Source SHA changed: '+row['name']
        assert original.stat().st_mtime_ns==row['original_mtime_ns'],'Original timestamp changed'
def allowed_package(package):return any(package.startswith(p+'/') for p in DESTINATIONS)
def owned_files(content):
    return {p.relative_to(content).as_posix():p for rel in RELATIVE_ROOTS for p in (content/rel).rglob('*') if p.is_file()}
def verify_files(content,expected):
    actual=owned_files(content)
    assert set(actual)==set(expected),'Exact owned file set mismatch: '+str(content)
    for relative,p in actual.items():
        item=expected[relative]
        assert p.stat().st_size==item['bytes'] and digest(p)==item['sha256'],'Native bytes changed: '+str(p)

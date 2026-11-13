"""Thin granted MCP runner for the published audio builder; top-level await is intentional."""
from pathlib import Path
import ast,hashlib,json

grant=SPEC
assert grant.get('execute') is True and grant['claim_commit']
root=Path(grant['source_root']).resolve()
host=Path(unreal.Paths.project_dir()).resolve()
assert host==Path(grant['project_dir']).resolve()
spec=json.loads((root/'build.spec.json').read_text())
records=json.loads((root/'sources.json').read_text())
for row in records['prepared']:
    assert hashlib.sha256(Path(row['path']).read_bytes()).hexdigest()==row['sha256'], 'Prepared audio hash changed'
paths=[item['path'] for group in ('mix','import','oneshots') for item in spec[group]]
assert len(paths)==16 and all(p.startswith('/Game/TriadDrift/Audio/Combat01/') for p in paths)
assert all(not unreal.EditorAssetLibrary.does_asset_exist(p) for p in paths), 'Existing target: preserve and stop'
assert not unreal.EditorAssetLibrary.does_asset_exist('/Game/TriadDrift/Presentation/DA_NeedleCannons_FX')
builder=Path(grant['builder']).resolve()
text=builder.read_text(encoding='utf-8')
# The published batch helper continues after a row error. This bounded job must stop at the first error.
line="            RESULT['errors'].append('%s %s: %s' % (section, entry.get('path'), str(error)[:400]))"
assert text.count(line)==1, 'Builder changed; request review'
text=text.replace(line,line+'\n            raise')
SPEC=spec
await eval(compile(text,str(builder),'exec',flags=ast.PyCF_ALLOW_TOP_LEVEL_AWAIT),globals())
assert not RESULT['errors'] and len(RESULT['ok'])==16, RESULT
SPEC=grant
exec(compile((root/'bind_needle_audio.py').read_text(),str(root/'bind_needle_audio.py'),'exec'),globals())
assert all(unreal.EditorAssetLibrary.does_asset_exist(p) for p in paths)
result={'native_audio_assets':16,'presentation_assets':1,'paths':paths,
        'stage':'Created and sound slots assigned; reload, actual firing/hits, cook and listening pending'}
output=host/'Saved/AudioMusic/Combat01/author_01.json'
assert not output.exists()
output.parent.mkdir(parents=True,exist_ok=True)
output.write_text(json.dumps(result,indent=2)+'\n')
result

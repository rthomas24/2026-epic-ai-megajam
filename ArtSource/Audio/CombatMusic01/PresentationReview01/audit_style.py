"""Read-only PR11 style/default comparison. Requires an explicit compatible-host grant."""
from pathlib import Path
import hashlib
import json
import re


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def property_names(header):
    body = header.split('class UNREALGAMEJAM_API UTDUIStyle :', 1)[1].split('\n};', 1)[0]
    names = re.findall(r'UPROPERTY\([^\n]+\)\s*\n\s*[^;\n]+?\s+(\w+)\s*(?:=[^;]+)?;', body)
    assert len(names) >= 29 and len(names) == len(set(names)), names
    def snake(name):
        name = re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1_\2', name)
        return re.sub(r'([a-z0-9])([A-Z])', r'\1_\2', name).lower()
    return [(name, snake(name)) for name in names]


def audit(spec):
    import unreal
    assert spec.get('execute') is True and spec['claim_commit']
    host = Path(unreal.Paths.project_dir()).resolve()
    assert host == Path(spec['project_dir']).resolve()
    package = host/'Content/TriadDrift/Presentation/DA_UI_Style.uasset'
    assert digest(package) == '0a1fa00da9771737cc36e3f0db16ef746fb284b784d210c8537acdc9a2d07df0'
    source = host/'Source/unrealGameJam/TriadDrift/Core/TDPresentation.h'
    assert digest(source) == spec['header_sha256'], 'Reviewed current header changed'
    fields = property_names(source.read_text(encoding='utf-8-sig'))
    style = unreal.load_asset('/Game/TriadDrift/Presentation/DA_UI_Style')
    cls = unreal.load_class(None, '/Script/unrealGameJam.TDUIStyle')
    assert style and style.get_class() == cls
    defaults = unreal.get_default_object(cls)
    # Unreal StructBase equality is wrapper identity for FTDFXCue; inspect every member.
    header = source.read_text(encoding='utf-8-sig')
    cue_body = header.split('struct FTDFXCue', 1)[1].split('\n};', 1)[0]
    cue_members = re.findall(r'UPROPERTY\([^\n]+\)\s*\n\s*[^;\n]+?\s+(\w+)\s*(?:=[^;]+)?;', cue_body)
    assert cue_members == ['VFX', 'Sound', 'Shake', 'ShakeScale', 'ShakeRadius', 'VFXScale'], 'Cue schema changed; review all members'
    cue_fields = set(re.findall(r'FTDFXCue\s+(\w+)\s*;', header.split('class UNREALGAMEJAM_API UTDUIStyle :', 1)[1].split('\n};', 1)[0]))
    def cue_value(cue):
        result = {}
        for member in ('vfx', 'sound', 'shake'):
            obj = cue.get_editor_property(member)
            result[member] = obj.get_path_name() if obj else None
        for member in ('shake_scale', 'shake_radius'):
            result[member] = float(cue.get_editor_property(member))
        vector = cue.get_editor_property('vfx_scale')
        result['vfx_scale'] = [float(vector.x), float(vector.y), float(vector.z)]
        return result
    rows = []
    for cpp, name in fields:
        actual, default = style.get_editor_property(name), defaults.get_editor_property(name)
        if cpp in cue_fields:
            actual, default = cue_value(actual), cue_value(default)
        equal = actual == default
        rows.append({'cpp': cpp, 'python': name, 'equals_current_default': equal,
                     'candidate': actual if cpp in cue_fields else str(actual),
                     'current_default': default if cpp in cue_fields else str(default)})
    differences = [row['python'] for row in rows if not row['equals_current_default']]
    music = style.get_editor_property('menu_music')
    assert music and music.get_path_name() == '/Game/TriadDrift/Audio/MenuTheme01/MUS_Menu_EtherialPulse_01.MUS_Menu_EtherialPulse_01'
    run = host/'Saved/AudioMusic/CombatMusic01'/spec['run_id']
    assert not run.exists()
    run.mkdir(parents=True)
    report = {'host_build_commit': spec['host_build_commit'], 'style_sha256': digest(package),
              'header_sha256': digest(source), 'fields': rows, 'differences_from_current_CDO': differences,
              'passed': differences == ['menu_music'], 'asset_saves': 0,
              'stage': 'Read-only style comparison; no binding or playback implied'}
    (run/'style_audit.json').write_text(json.dumps(report, indent=2)+'\n')
    assert digest(package) == '0a1fa00da9771737cc36e3f0db16ef746fb284b784d210c8537acdc9a2d07df0'
    assert report['passed'], 'Additional style differences: preserve candidate; request review before binding'
    print(json.dumps(report))


if globals().get('SPEC', {}).get('execute') is True:
    audit(SPEC)
else:
    print('Prepared only. No Unreal load, asset edit, or native launch.')

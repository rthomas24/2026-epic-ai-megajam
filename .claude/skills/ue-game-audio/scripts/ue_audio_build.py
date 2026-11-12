"""Triad Drift - build Unreal audio assets from a JSON spec, INSIDE the editor.

This file is not run by a normal Python. It is sent to the editor's `execute_python` tool (Unreal's
built-in MCP server, UE 5.9) by `ue_run_python.py`, which defines SPEC (a dict) before this text:

    python .claude/skills/ue-game-audio/scripts/ue_run_python.py --port 8021 \
        .claude/skills/ue-game-audio/scripts/ue_audio_build.py --spec my_spec.json

Inside `execute_python`, `unreal`, `asyncio` and `call_tool` already exist, top-level `await` works,
and the value of the last expression is returned. Everything here goes through the engine's own
toolsets (SoundWaveToolset, ObjectToolset, MetaSoundAuthoringToolset), checked on UE 5.9.0
CL 60351404 on 2026-11-12.

SPEC sections (all optional; processed in this order; see references/unreal-implementation.md):

  "mix":      [{"type": "SoundClass" | "SoundAttenuation" | "SoundConcurrency" | "SoundSubmix" | "SoundMix",
                "path": "/Game/TriadDrift/Audio/Mix/SC_TD_SFX", "properties": {...ObjectToolset names...}}]
  "import":   [{"file": "C:/abs/take_01.wav", "path": "/Game/TriadDrift/Audio/SFX/Weapons/SW_X_01",
                "properties": {"bLooping": true, "loadingBehavior": "RetainOnLoad"}}]
  "oneshots": [{"path": "/Game/TriadDrift/Audio/SFX/Weapons/MSS_X", "format": "Mono" | "Stereo",
                "sounds": ["/Game/.../SW_X_01", ...], "pitch_semitones": 0, "pitch_spread": 0.7,
                "volume_db": 0, "volume_spread_db": 1.5, "sound": {...USoundBase properties...}}]
  "loops":    [{"path": ".../MSS_X_Loop", "format": "Mono" | "Stereo", "wave": "/Game/.../SW_X_Loop",
                "param": "Speed01", "pitch_range": [-2, 5], "gain_range_db": [-8, 0],
                "smooth_seconds": 0.15, "sound": {...}}]
  "music":    [{"path": ".../MSS_Music_Combat", "layers": [{"wave": "/Game/...", "fade_in": [0.0, 0.0]},
                {"wave": "...", "fade_in": [0.25, 0.55]}, {"wave": "...", "fade_in": [0.6, 0.9]}],
                "param": "Intensity", "smooth_seconds": 1.5, "sound": {...}}]

An asset that already exists is updated in place where that is safe (wave properties, mix-asset
properties, a one-shot's Sounds / spreads / levels, the "sound" block); its graph is never rebuilt.
Delete the asset yourself if the graph has to change.
"""
import json

AUTH = 'MetaSoundToolset.MetaSoundAuthoringToolset.'
OBJ = 'EditorToolset.ObjectToolset.'
RESULT = {'ok': [], 'errors': []}

FACTORIES = {
    'SoundClass': 'SoundClassFactory',
    'SoundAttenuation': 'SoundAttenuationFactory',
    'SoundConcurrency': 'SoundConcurrencyFactory',
    'SoundSubmix': 'SoundSubmixFactory',
    'SoundMix': 'SoundMixFactory',
}


def split_path(path):
    """'/Game/A/B/Name' -> ('/Game/A/B', 'Name', '/Game/A/B/Name.Name')."""
    path = path.split('.')[0]
    folder, name = path.rsplit('/', 1)
    return folder, name, '%s.%s' % (path, name)


def exists(path):
    return unreal.EditorAssetLibrary.does_asset_exist(split_path(path)[2])


def refs(values):
    """Turns '/Game/...' strings (and lists / dicts holding them) into {'refPath': object path}."""
    if isinstance(values, str) and values.startswith('/'):
        return {'refPath': split_path(values)[2]}
    if isinstance(values, list):
        return [refs(v) for v in values]
    if isinstance(values, dict):
        return {k: refs(v) for k, v in values.items()}
    return values


async def set_properties(path, properties):
    if not properties:
        return
    asset = unreal.load_asset(split_path(path)[2])
    if not asset:
        raise RuntimeError('cannot load %s' % path)
    done = await call_tool(OBJ + 'SetProperties', instance=asset, values=json.dumps(refs(properties)))
    if not done:
        raise RuntimeError('SetProperties refused on %s: %s' % (path, sorted(properties)))
    unreal.EditorAssetLibrary.save_loaded_asset(asset)


async def make_mix_asset(item):
    folder, name, object_path = split_path(item['path'])
    if not exists(item['path']):
        kind = item['type']
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        asset = tools.create_asset(name, folder, getattr(unreal, kind), getattr(unreal, FACTORIES[kind])())
        if not asset:
            raise RuntimeError('could not create %s' % item['path'])
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
    await set_properties(item['path'], item.get('properties'))
    return object_path


async def import_wave(item):
    folder, name, object_path = split_path(item['path'])
    await call_tool('EditorToolset.SoundWaveToolset.ImportFile', folderPath=folder, assetName=name,
                    sourceFile=item['file'], bReplaceExisting=True, bSave=True)
    await set_properties(item['path'], item.get('properties'))
    return object_path


async def add_nodes(builder, classes):
    """classes: [(namespace, name, variant)]. Returns the node refs in the same order."""
    specs = [{'className': {'namespace': a, 'name': b, 'variant': c}} for a, b, c in classes]
    report = await call_tool(AUTH + 'AddNodes', builderName=builder, nodes=specs, page='')
    if not report['bOk'] or report['applied'] != len(specs):
        raise RuntimeError('AddNodes: %s' % report['errors'])
    return [node['ref'] for node in report['nodes']]


async def batch(tool, builder, key, items):
    report = await call_tool(AUTH + tool, **{'builderName': builder, key: items})
    if not report['bOk'] or report['applied'] != len(items):
        raise RuntimeError('%s applied %s of %s: %s' % (tool, report['applied'], len(items), report['errors']))


def edge(a, a_pin, b, b_pin):
    return {'fromNode': a, 'fromPin': a_pin, 'toNode': b, 'toPin': b_pin}


def channels(fmt):
    """(wave player variant, [player output pins], [source output nodes])."""
    if fmt == 'Stereo':
        return 'Stereo', ['Out Left', 'Out Right'], ['UE.OutputFormat.Stereo.Audio:0', 'UE.OutputFormat.Stereo.Audio:1']
    return 'Mono', ['Out Mono'], ['UE.OutputFormat.Mono.Audio:0']


async def finish(builder, item):
    folder, name, object_path = split_path(item['path'])
    await call_tool(AUTH + 'LayoutGraph', builderName=builder, page='')
    await call_tool(AUTH + 'BuildToAsset', builderName=builder, assetName=name, packagePath=folder, author='ue-game-audio')
    await set_properties(item['path'], item.get('sound'))
    asset = unreal.load_asset(object_path)
    if asset:
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
    return object_path


def oneshot_defaults(item):
    return [
        {'node': '', 'pin': 'Sounds', 'value': json.dumps([split_path(p)[2] for p in item['sounds']])},
        {'node': '', 'pin': 'PitchBase', 'value': str(item.get('pitch_semitones', 0))},
        {'node': '', 'pin': 'PitchRandomSpread', 'value': str(item.get('pitch_spread', 0.7))},
        {'node': '', 'pin': 'VolumeBase', 'value': str(item.get('volume_db', 0))},
        {'node': '', 'pin': 'VolumeRandomSpread', 'value': str(item.get('volume_spread_db', 1.5))},
    ]


async def make_oneshot(item):
    """Random take + random pitch (semitones) + random level (dB); reports finished so the voice is freed."""
    folder, name, object_path = split_path(item['path'])
    builder = 'TDA_' + name
    if exists(item['path']):  # retune only: the graph inputs carry every tunable
        await call_tool(AUTH + 'AttachBuilderToAsset', assetPath=object_path, builderName=builder, page='Default')
        try:
            await batch('SetDefaults', builder, 'defaults', oneshot_defaults(item))
            await call_tool(AUTH + 'CommitBuilderToAsset', builderName=builder)
        finally:
            await call_tool(AUTH + 'DetachBuilderFromAsset', builderName=builder)
        await set_properties(item['path'], item.get('sound'))
        return object_path + ' (updated)'

    variant, outs, sinks = channels(item.get('format', 'Mono'))
    await call_tool(AUTH + 'CreateSourceBuilder', builderName=builder, outputFormat=variant, bIsOneShot=True)
    members = [{'name': 'Sounds', 'dataType': 'WaveAsset:Array', 'default': '', 'bIsConstructor': False}]
    members += [{'name': n, 'dataType': 'Float', 'default': '0', 'bIsConstructor': False}
                for n in ('PitchBase', 'PitchRandomSpread', 'VolumeBase', 'VolumeRandomSpread')]
    await batch('AddGraphInputs', builder, 'inputs', members)
    pick, player, rnd_pitch, rnd_vol, neg_pitch, neg_vol, add_pitch, add_vol, to_gain = await add_nodes(builder, [
        ('Array', 'Random Get', 'WaveAsset:Array'), ('UE', 'Wave Player', variant),
        ('UE', 'RandomFloat', 'None'), ('UE', 'RandomFloat', 'None'),
        ('UE', 'Multiply', 'Float'), ('UE', 'Multiply', 'Float'),
        ('UE', 'Add', 'Float'), ('UE', 'Add', 'Float'), ('UE', 'Decibels to Linear Gain', 'Float')])
    gains = await add_nodes(builder, [('UE', 'Multiply', 'Audio by Float')] * len(outs))
    play = 'UE.Source.OnPlay'
    edges = [
        edge(play, 'None', pick, 'Next'), edge(play, 'None', rnd_pitch, 'Next'), edge(play, 'None', rnd_vol, 'Next'),
        edge('Sounds', 'None', pick, 'In Array'),
        edge(pick, 'Value', player, 'Wave Asset'), edge(pick, 'On Next', player, 'Play'),
        edge('PitchRandomSpread', 'None', neg_pitch, 'PrimaryOperand'), edge(neg_pitch, 'Out', rnd_pitch, 'Min'),
        edge('PitchRandomSpread', 'None', rnd_pitch, 'Max'),
        edge('PitchBase', 'None', add_pitch, 'PrimaryOperand'), edge(rnd_pitch, 'Value', add_pitch, 'AdditionalOperands'),
        edge(add_pitch, 'Out', player, 'Pitch Shift'),
        edge('VolumeRandomSpread', 'None', neg_vol, 'PrimaryOperand'), edge(neg_vol, 'Out', rnd_vol, 'Min'),
        edge('VolumeRandomSpread', 'None', rnd_vol, 'Max'),
        edge('VolumeBase', 'None', add_vol, 'PrimaryOperand'), edge(rnd_vol, 'Value', add_vol, 'AdditionalOperands'),
        edge(add_vol, 'Out', to_gain, 'Decibels'),
        edge(player, 'On Finished', 'UE.Source.OneShot.OnFinished', 'None'),
    ]
    for out_pin, gain, sink in zip(outs, gains, sinks):
        edges += [edge(player, out_pin, gain, 'PrimaryOperand'), edge(to_gain, 'Linear Gain', gain, 'AdditionalOperands'),
                  edge(gain, 'Out', sink, 'None')]
    await batch('Connect', builder, 'connections', edges)
    defaults = oneshot_defaults(item) + [
        {'node': neg_pitch, 'pin': 'AdditionalOperands', 'value': '-1'},
        {'node': neg_vol, 'pin': 'AdditionalOperands', 'value': '-1'},
        {'node': pick, 'pin': 'No Repeats', 'value': str(1 if len(item['sounds']) > 1 else 0)},
    ]
    await batch('SetDefaults', builder, 'defaults', defaults)
    return await finish(builder, item)


async def make_loop(item):
    """A seamless loop whose pitch and level follow one 0..1 float parameter set from gameplay."""
    folder, name, object_path = split_path(item['path'])
    if exists(item['path']):
        await set_properties(item['path'], item.get('sound'))
        return object_path + ' (exists: only the sound block was applied)'
    builder = 'TDA_' + name
    param = item.get('param', 'Speed01')
    variant, outs, sinks = channels(item.get('format', 'Mono'))
    await call_tool(AUTH + 'CreateSourceBuilder', builderName=builder, outputFormat=variant, bIsOneShot=False)
    await batch('AddGraphInputs', builder, 'inputs', [
        {'name': 'Wave', 'dataType': 'WaveAsset', 'default': split_path(item['wave'])[2], 'bIsConstructor': False},
        {'name': param, 'dataType': 'Float', 'default': str(item.get('param_default', 0)), 'bIsConstructor': False}])
    player, map_pitch, map_gain, smooth_pitch, smooth_gain, to_gain = await add_nodes(builder, [
        ('UE', 'Wave Player', variant), ('MapRange', 'MapRange', 'Float'), ('MapRange', 'MapRange', 'Float'),
        ('UE', 'InterpTo', 'Audio'), ('UE', 'InterpTo', 'Audio'), ('UE', 'Decibels to Linear Gain', 'Float')])
    gains = await add_nodes(builder, [('UE', 'Multiply', 'Audio by Float')] * len(outs))
    edges = [
        edge('UE.Source.OnPlay', 'None', player, 'Play'), edge('Wave', 'None', player, 'Wave Asset'),
        edge(param, 'None', map_pitch, 'In'), edge(map_pitch, 'Out Value', smooth_pitch, 'Target'),
        edge(smooth_pitch, 'Value', player, 'Pitch Shift'),
        edge(param, 'None', map_gain, 'In'), edge(map_gain, 'Out Value', smooth_gain, 'Target'),
        edge(smooth_gain, 'Value', to_gain, 'Decibels'),
    ]
    for out_pin, gain, sink in zip(outs, gains, sinks):
        edges += [edge(player, out_pin, gain, 'PrimaryOperand'), edge(to_gain, 'Linear Gain', gain, 'AdditionalOperands'),
                  edge(gain, 'Out', sink, 'None')]
    await batch('Connect', builder, 'connections', edges)
    pitch_lo, pitch_hi = item.get('pitch_range', [-2, 5])
    gain_lo, gain_hi = item.get('gain_range_db', [-8, 0])
    smooth = str(item.get('smooth_seconds', 0.15))
    await batch('SetDefaults', builder, 'defaults', [
        {'node': player, 'pin': 'Loop', 'value': 'true'},
        {'node': map_pitch, 'pin': 'Out Range A', 'value': str(pitch_lo)}, {'node': map_pitch, 'pin': 'Out Range B', 'value': str(pitch_hi)},
        {'node': map_gain, 'pin': 'Out Range A', 'value': str(gain_lo)}, {'node': map_gain, 'pin': 'Out Range B', 'value': str(gain_hi)},
        {'node': smooth_pitch, 'pin': 'Interp Time', 'value': smooth}, {'node': smooth_gain, 'pin': 'Interp Time', 'value': smooth}])
    return await finish(builder, item)


async def make_music(item):
    """Vertical layering: every layer loops in sync; one 0..1 float fades the upper layers in and out."""
    folder, name, object_path = split_path(item['path'])
    if exists(item['path']):
        await set_properties(item['path'], item.get('sound'))
        return object_path + ' (exists: only the sound block was applied)'
    layers = item['layers']
    if not 2 <= len(layers) <= 8:
        raise RuntimeError('music needs 2 to 8 layers')
    builder = 'TDA_' + name
    param = item.get('param', 'Intensity')
    await call_tool(AUTH + 'CreateSourceBuilder', builderName=builder, outputFormat='Stereo', bIsOneShot=False)
    members = [{'name': param, 'dataType': 'Float', 'default': str(item.get('param_default', 0)), 'bIsConstructor': False}]
    members += [{'name': 'Layer%d' % i, 'dataType': 'WaveAsset', 'default': split_path(layer['wave'])[2], 'bIsConstructor': False}
                for i, layer in enumerate(layers)]
    await batch('AddGraphInputs', builder, 'inputs', members)
    count = len(layers)
    players = await add_nodes(builder, [('UE', 'Wave Player', 'Stereo')] * count)
    maps = await add_nodes(builder, [('MapRange', 'MapRange', 'Float')] * count)
    smooths = await add_nodes(builder, [('UE', 'InterpTo', 'Audio')] * count)
    (mixer,) = await add_nodes(builder, [('AudioMixer', 'Audio Mixer (Stereo, %d)' % count, 'None')])
    edges, defaults = [], []
    for i, layer in enumerate(layers):
        lo, hi = layer.get('fade_in', [0, 0])
        edges += [
            edge('UE.Source.OnPlay', 'None', players[i], 'Play'), edge('Layer%d' % i, 'None', players[i], 'Wave Asset'),
            edge(players[i], 'Out Left', mixer, 'In %d L' % i), edge(players[i], 'Out Right', mixer, 'In %d R' % i),
            edge(param, 'None', maps[i], 'In'), edge(maps[i], 'Out Value', smooths[i], 'Target'),
            edge(smooths[i], 'Value', mixer, 'Gain %d' % i)]
        defaults += [
            {'node': players[i], 'pin': 'Loop', 'value': 'true'},
            {'node': players[i], 'pin': 'Maintain Audio Sync', 'value': 'true'},
            {'node': smooths[i], 'pin': 'Interp Time', 'value': str(item.get('smooth_seconds', 1.5))}]
        if hi > lo:  # fades in between lo and hi
            defaults += [{'node': maps[i], 'pin': 'In Range A', 'value': str(lo)}, {'node': maps[i], 'pin': 'In Range B', 'value': str(hi)}]
        else:        # always on
            defaults += [{'node': maps[i], 'pin': 'Out Range A', 'value': '1'}, {'node': maps[i], 'pin': 'Out Range B', 'value': '1'}]
    edges += [edge(mixer, 'Out L', 'UE.OutputFormat.Stereo.Audio:0', 'None'), edge(mixer, 'Out R', 'UE.OutputFormat.Stereo.Audio:1', 'None')]
    await batch('Connect', builder, 'connections', edges)
    await batch('SetDefaults', builder, 'defaults', defaults)
    return await finish(builder, item)


for section, maker in (('mix', make_mix_asset), ('import', import_wave), ('oneshots', make_oneshot),
                       ('loops', make_loop), ('music', make_music)):
    for entry in SPEC.get(section, []):
        try:
            RESULT['ok'].append(await maker(entry))
        except Exception as error:  # report and carry on: one bad row must not hide the rest
            RESULT['errors'].append('%s %s: %s' % (section, entry.get('path'), str(error)[:400]))
        await asyncio.sleep(0)

RESULT

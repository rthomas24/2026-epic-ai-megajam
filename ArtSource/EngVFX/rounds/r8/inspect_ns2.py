import json, sys
sys.path.insert(0, unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()) + 'ArtSource/EngVFX/scripts')
T = 'NiagaraToolsets.NiagaraToolset_System.'
SYS = '/Game/TriadDrift/Gameplay/VFX/Systems/'
out = {}
for name, em in (('NS_TD_Dominion_LightRow', 'Light00'), ('NS_TD_Debug_GlowOffset', 'Beacon'), ('NS_TD_Cloak_Shimmer', 'Glints')):
    s = unreal.load_asset(SYS + name)
    d = {}
    for tool, arg in (('GetEmitterData', {'emitterRef': {'system': s, 'emitterName': em}}), ('GetRendererData', {'rendererRef': {'system': s, 'emitterName': em, 'rendererIndex': 0}})):
        d[tool] = json.loads((await call_tool(T + tool, **arg))['propertyValues'])
    topo = await call_tool(T + 'GetEmitterTopology', emitterRef={'system': s, 'emitterName': em})
    mods = {}
    for sk in ('emitterUpdateScript', 'particleSpawnScript', 'particleUpdateScript'):
        for m in topo[sk]['modules']:
            mods[topo[sk]['scriptName'].split('::')[-1] + '.' + m['moduleName']] = m['enabled']
    d['modules'] = mods
    out[name] = d
open(unreal.Paths.project_saved_dir() + 'EngVFX/inspect2.json', 'w').write(json.dumps(out, indent=1, default=str))
'written'

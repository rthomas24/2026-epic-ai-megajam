# Triad Drift ENG-37 round 5: copies the Leaders / Followers emitters out of the engine's AttributeReaderTrails template
# system into standalone emitter assets, so tdvfx can add them to our systems ('leaders' / 'followers' bases).
# Run in the editor once:  python ArtSource/EngVFX/scripts/run_editor_py.py 8017 ArtSource/EngVFX/scripts/make_trail_emitters.py
out = []
unreal.load_asset('/Niagara/DefaultAssets/Templates/Systems/AttributeReaderTrails')
DST = '/Game/TriadDrift/Gameplay/VFX/Emitters'
for n in ('Leaders', 'Followers'):
    path = DST + '/NE_TD_' + n
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        out.append(path + ' exists'); continue
    src = unreal.load_object(None, '/Niagara/DefaultAssets/Templates/Systems/AttributeReaderTrails.AttributeReaderTrails:%s_0' % n)
    dup = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset('NE_TD_' + n, DST, src)   # EditorAssetLibrary refuses a sub-object
    out.append('%s -> %s saved=%s' % (n, dup, unreal.EditorAssetLibrary.save_loaded_asset(dup, False) if dup else None))
T = 'NiagaraToolsets.NiagaraToolset_System.'
folder, name = '/Game/TriadDrift/Gameplay/VFX/Probe', 'NS_TD_Probe_Trails3'
if not unreal.EditorAssetLibrary.does_asset_exist(folder + '/' + name):
    await call_tool(T + 'CreateNiagaraSystem', assetName=name, assetPath=folder, templateSystem=unreal.load_asset('/Niagara/DefaultAssets/DefaultSystem'))
system = unreal.load_asset(folder + '/' + name)
for n in ('Leaders', 'Followers'):
    try:
        t = await call_tool(T + 'AddEmitter', system=system, templateEmitter=unreal.load_asset(DST + '/NE_TD_' + n), emitterName=n)
        out.append('added ' + t['emitterName'])
    except Exception as ex:
        out.append('ADD FAIL %s %s' % (n, str(ex)[:300]))
cs = await call_tool(T + 'GetSystemCompileState', system=system)
out.append('compile %s, probe saved=%s' % (cs['aggregateStatus'], unreal.EditorAssetLibrary.save_loaded_asset(system, False)))
"\n".join(out)

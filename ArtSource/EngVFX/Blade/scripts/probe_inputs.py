# #163: list the visible inputs of the template modules the blade systems use (input names change with static switches).
# run_editor_py.py <port> ArtSource/EngVFX/Blade/scripts/probe_inputs.py   (builds a throwaway system, deletes it after)
import sys, importlib, json
sys.path.insert(0, unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()) + 'ArtSource/EngVFX/scripts')
import tdvfx
importlib.reload(tdvfx)
E, X = tdvfx.E, tdvfx.X
ES, EU, PS, PU = tdvfx.ES, tdvfx.EU, tdvfx.PS, tdvfx.PU
fx = tdvfx.Builder(call_tool)
specs = [
    {'name': 'Rate', 'base': 'rate', 'props': {'bLocalSpace': False}, 'add': [],
     'set': [(PS, 'ShapeLocation', 'Shape Primitive', E('Box')), (PS, 'AddVelocity', 'Velocity Mode', E('Linear')),
             (PS, 'InitializeParticle', 'UsePositionOffset', True)]},
    {'name': 'Rib', 'base': 'ribbon', 'add': [], 'set': []},
]
mods = []
for d in ('/Niagara/Modules/Spawn/Location', '/Niagara/Modules/Spawn/Velocity', '/Niagara/Modules/Spawn/Ribbon', '/Niagara/Modules/Ribbons',
          '/Niagara/Modules/Update/Velocity', '/Niagara/Modules/Spawn/Orientation'):
    try:
        mods += [str(p).split('.')[0] for p in unreal.EditorAssetLibrary.list_assets(d, True, False)]
    except Exception as ex:
        mods.append('list %s failed' % d)
specs[1]['add'] = [(PS, '/Niagara/Modules/Spawn/Location/ShapeLocation')] if any(m.endswith('/Location/ShapeLocation') for m in mods) else []
sysobj = await fx.system('NS_TD_Blade_ProbeTmp', specs, user={'Color': (1, 1, 1, 1)}, folder='/Game/TriadDrift/Gameplay/VFX/Dev')
out = [fx.report(), 'MODULES: ' + ', '.join(m.replace('/Niagara/Modules/', '') for m in mods)]
for nm in ('Rate', 'Rib'):
    out.append(await fx.dump(sysobj, nm))
try:
    r = await call_tool(tdvfx.T + 'GetRendererData', rendererRef={'system': sysobj, 'emitterName': 'Rib', 'rendererIndex': 0})
    out.append(json.dumps(r)[:3000])
except Exception as ex:
    out.append('renderer data? ' + str(ex)[:300])
try:
    r = await call_tool(tdvfx.T + 'GetEmitterData', emitterRef={'system': sysobj, 'emitterName': 'Rib'})
    out.append(json.dumps(r)[:1500])
except Exception as ex:
    out.append('emitter data? ' + str(ex)[:300])
unreal.EditorAssetLibrary.delete_asset('/Game/TriadDrift/Gameplay/VFX/Dev/NS_TD_Blade_ProbeTmp')
'\n'.join(out)

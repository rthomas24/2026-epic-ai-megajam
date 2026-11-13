"""Fresh-process read-only graph and dependency check; no style writes."""
from pathlib import Path
import hashlib,json
host=Path(unreal.Paths.project_dir()).resolve();assert host==Path(SPEC['project_dir']).resolve()
assert SPEC['mode']=='reload'
ns={'SPEC':SPEC};exec(compile(Path(SPEC['wave_author']).read_text(),SPEC['wave_author'],'exec'),ns)
root='/Game/TriadDrift/Audio/DominionInterior01';path=root+'/MS_TD_DominionInteriorMusic'
package=host/'Content/TriadDrift/Audio/DominionInterior01/MS_TD_DominionInteriorMusic.uasset'
assert hashlib.sha256(package.read_bytes()).hexdigest()==SPEC['meta_sha256']
asset=unreal.load_asset(path);assert asset and asset.get_class().get_name()=='MetaSoundSource'
assert asset.get_editor_property('volume')==1 and asset.get_editor_property('pitch')==1
cls=asset.get_editor_property('sound_class_object');assert cls.get_path_name()=='/Game/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music.SC_MenuTheme_Music'
AUTH='MetaSoundToolset.MetaSoundAuthoringToolset.';INSPECT='MetaSoundToolset.MetaSoundInspectionToolset.'
builder='TD_DominionInteriorReload01'
await call_tool(AUTH+'AttachBuilderToAsset',assetPath=path+'.MS_TD_DominionInteriorMusic',builderName=builder,page='Default')
graph=await call_tool(INSPECT+'DescribeGraph',builderName=builder,page='Default')
prior=json.loads(Path(SPEC['authored_graph']).read_text())
diag=host/'Saved/DominionInterior01'/SPEC['meta_run_id'];assert not diag.exists();diag.mkdir(parents=True)
(diag/'graph_reloaded.json').write_text(json.dumps(graph,indent=2))
changes=[k for k in prior if k!='builderName' and prior[k]!=graph.get(k)]
(diag/'serialization_changes.json').write_text(json.dumps({'changed_keys':changes},indent=2))
players=[n for n in graph['nodes'] if n['className']['namespace']=='UE' and n['className']['name']=='Wave Player'];assert len(players)==1
pins={x['name']:x for x in players[0]['inputs']}
assert pins['Loop']['value']=='true' and float(pins['Loop Start']['value'])==12 and abs(float(pins['Loop Duration']['value'])-28.08)<.000001
assert pins['Wave Asset']['value']==root+'/MUS_DominionInterior_Calm_01.MUS_DominionInterior_Calm_01'
assert pins['Start Time']['bIsConnected']
assert [x for x in graph['inputs'] if x['name']=='StartTime']==[{'name':'StartTime','dataType':'Float','default':'0','bIsConstructor':False}]
gains=[n for n in graph['nodes'] if n['className']['name']=='Multiply'];assert len(gains)==2
for n in gains:assert float(next(p['value'] for p in n['inputs'] if p['name']=='AdditionalOperands'))==.5
registry=unreal.AssetRegistryHelpers.get_asset_registry()
options=unreal.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True,include_searchable_names=False,include_soft_management_references=False,include_hard_management_references=False)
deps=[str(p) for p in registry.get_dependencies(path,options)]
assert root+'/MUS_DominionInterior_Calm_01' in deps, ('Missing source Wave dependency',deps)
assert '/Game/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music' in deps, ('Missing Music class dependency',deps)
assert not [p for p in deps if p.startswith('/Game/') and p not in (root+'/MUS_DominionInterior_Calm_01','/Game/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music')],deps
await call_tool(AUTH+'DetachBuilderFromAsset',builderName=builder)
assert hashlib.sha256(package.read_bytes()).hexdigest()==SPEC['meta_sha256']
out=diag
(out/'graph_reloaded.json').write_text(json.dumps(graph,indent=2))
result={'stage':'Fresh single-player graph and dependency reload verified; audible render/gameplay pending','asset':asset.get_path_name(),'meta_sha256':SPEC['meta_sha256'],'wave_sha256':SPEC['wave_sha256'],'one_wave_player':True,'start_time_float0':True,'loop_region_s':[12,40.08],'period_s':28.08,'gain_once_in_graph':.5,'source_volume':1,'music_class':cls.get_path_name(),'dependencies':deps,'serialization_changed_keys':changes,'topology_defaults_verified':True,'no_saves':True}
(out/'result.json').write_text(json.dumps(result,indent=2))
result

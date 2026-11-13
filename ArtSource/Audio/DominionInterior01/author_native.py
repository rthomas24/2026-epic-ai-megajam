"""T1 issue24 single-source contract; run inside Unreal MCP Python. No style binding."""
from pathlib import Path
import hashlib,json
host=Path(unreal.Paths.project_dir()).resolve();assert host==Path(SPEC['project_dir']).resolve()
assert SPEC['execute'] is True
root='/Game/TriadDrift/Audio/DominionInterior01'
name='MS_TD_DominionInteriorMusic';path=root+'/'+name;builder='TD_DominionInterior01'
out=host/'Saved/DominionInterior01'/SPEC['meta_run_id'];assert not out.exists();out.mkdir(parents=True)
AUTH='MetaSoundToolset.MetaSoundAuthoringToolset.'
DISC='MetaSoundToolset.MetaSoundDiscoveryToolset.'
INSPECT='MetaSoundToolset.MetaSoundInspectionToolset.'
classes=[('UE','Wave Player','Stereo'),('UE','ConversionFloatToTime',''),('UE','Multiply','Audio by Float')]
descriptions=[]
for namespace,node,variant in classes:
 descriptions.append(await call_tool(DISC+'DescribeNodeClass',namespace=namespace,name=node,variant=variant))
(out/'node_classes.json').write_text(json.dumps(descriptions,indent=2))
# Imported Wave retains all previous source assets and the presentation style unchanged.
ns={'SPEC':SPEC}
exec(compile(Path(SPEC['wave_author']).read_text(),SPEC['wave_author'],'exec'),ns)
assert not unreal.EditorAssetLibrary.does_asset_exist(path)
await call_tool(AUTH+'CreateSourceBuilder',builderName=builder,outputFormat='Stereo',bIsOneShot=True)
async def batch(tool,key,values):
 result=await call_tool(AUTH+tool,**{'builderName':builder,key:values})
 assert result['bOk'] and result['applied']==len(values),result
 return result
await batch('AddGraphInputs','inputs',[{'name':'StartTime','dataType':'Float','default':'0','bIsConstructor':False}])
classes=[classes[0],classes[1],classes[2],classes[2]]
added=await batch('AddNodes','nodes',[{'className':{'namespace':a,'name':b,'variant':c}} for a,b,c in classes])
player,convert,left,right=[x['ref'] for x in added['nodes']]
def edge(a,ap,b,bp):return {'fromNode':a,'fromPin':ap,'toNode':b,'toPin':bp}
await batch('Connect','connections',[
 edge('UE.Source.OnPlay','None',player,'Play'),
 edge(player,'On Finished','UE.Source.OneShot.OnFinished','None'),
 edge('StartTime','None',convert,'In'),edge(convert,'Out',player,'Start Time'),
 edge(player,'Out Left',left,'PrimaryOperand'),edge(left,'Out','UE.OutputFormat.Stereo.Audio:0','None'),
 edge(player,'Out Right',right,'PrimaryOperand'),edge(right,'Out','UE.OutputFormat.Stereo.Audio:1','None')])
wave_path=root+'/MUS_DominionInterior_Calm_01.MUS_DominionInterior_Calm_01'
await batch('SetDefaults','defaults',[
 {'node':player,'pin':'Wave Asset','value':wave_path},
 {'node':player,'pin':'Loop','value':'true'},
 {'node':player,'pin':'Loop Start','value':'12'},
 {'node':player,'pin':'Loop Duration','value':'28.08'},
 {'node':player,'pin':'Pitch Shift','value':'0'},
 {'node':left,'pin':'AdditionalOperands','value':'0.5'},
 {'node':right,'pin':'AdditionalOperands','value':'0.5'}])
await call_tool(AUTH+'LayoutGraph',builderName=builder,page='')
graph=await call_tool(INSPECT+'DescribeGraph',builderName=builder,page='')
(out/'graph_authored.json').write_text(json.dumps(graph,indent=2))
await call_tool(AUTH+'BuildToAsset',builderName=builder,assetName=name,packagePath=root,author='Nicholas Audio')
asset=unreal.load_asset(path);assert asset
cls=unreal.load_asset('/Game/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music');assert cls
for key,value in {'sound_class_object':cls,'volume':1.,'pitch':1.,'virtualization_mode':unreal.VirtualizationMode.PLAY_WHEN_SILENT}.items():asset.set_editor_property(key,value)
assert unreal.EditorAssetLibrary.save_loaded_asset(asset)
package=host/'Content/TriadDrift/Audio/DominionInterior01'/(name+'.uasset')
refs=list(unreal.EditorAssetLibrary.find_package_referencers_for_asset(root+'/MUS_DominionInterior_Calm_01',False))
result={'stage':'Authored boarding40.08sWavePlayer; fresh reload/render/gameplay still pending','asset':asset.get_path_name(),'sha256':hashlib.sha256(package.read_bytes()).hexdigest(),'bytes':package.stat().st_size,'start_parameter':'StartTime Float default0','loop_start_s':12,'loop_duration_s':28.08,'gain_in_graph':.5,'source_volume':1,'music_class':cls.get_path_name(),'wave_referencers':refs,'style_unchanged':True}
(out/'result.json').write_text(json.dumps(result,indent=2))
result

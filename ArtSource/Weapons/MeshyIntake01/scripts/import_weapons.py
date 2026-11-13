from pathlib import Path
import sys,json,traceback,time
sys.path.insert(0,str(Path(__file__).resolve().parent))
from weapon_common import *
report=dict(engine=unreal.SystemLibrary.get_engine_version(),stage='Technical weapon intake, no visual/gameplay acceptance',passed=False,imports=[],errors=[])
try:
    guard()
    assert not (EVIDENCE/'import_report.json').exists(),'Preserve earlier attempt; no automatic overwrite'
    assert all(not unreal.EditorAssetLibrary.list_assets(r['proposed_destination'],recursive=True) for r in MANIFEST['assets']),'Destination already contains assets'
    for row in MANIFEST['assets']:
        print('WEAPON_IMPORT_START '+row['name'],flush=True);start=time.monotonic()
        pipeline=unreal.InterchangeGenericAssetsPipeline();pipeline.set_editor_property('asset_name',row['proposed_mesh_name']);pipeline.set_editor_property('use_source_name_for_asset',False)
        mesh=pipeline.get_editor_property('mesh_pipeline')
        for key,value in [('build_nanite',False),('create_physics_asset',False),('import_static_meshes',True),('import_skeletal_meshes',False)]:mesh.set_editor_property(key,value)
        pipeline.get_editor_property('common_meshes_properties').set_editor_property('remove_degenerates',False)
        pipeline.get_editor_property('animation_pipeline').set_editor_property('import_animations',False)
        override=unreal.InterchangePipelineStackOverride();override.add_pipeline(pipeline)
        task=unreal.AssetImportTask()
        for key,value in dict(filename=row['staged'],destination_path=row['proposed_destination'],automated=True,replace_existing=False,replace_existing_settings=False,save=True,options=override).items():task.set_editor_property(key,value)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        assert task.get_objects(),'Import returned no objects'
        assert unreal.EditorAssetLibrary.save_directory(row['proposed_destination'],only_if_is_dirty=True,recursive=True),'Save failed'
        result=inspect(row);result['seconds']=time.monotonic()-start;report['imports'].append(result);write('import_report.json',report)
        print('WEAPON_IMPORT_DONE '+json.dumps(dict(name=row['name'],triangles=result['triangles'],seconds=result['seconds'])),flush=True)
        unreal.SystemLibrary.collect_garbage()
    guard();report['passed']=True
except Exception:report['errors'].append(traceback.format_exc())
write('import_report.json',report)
if not report['passed']:raise RuntimeError(report['errors'][-1])

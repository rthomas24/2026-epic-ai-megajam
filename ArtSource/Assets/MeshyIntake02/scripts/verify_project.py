"""Read-only fresh engine-host reload of exact actual-game package copies."""
import unreal,json,traceback,os,re,math
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from intake_common import *
attempt=os.environ.get('TD_I02_ATTEMPT','')
assert re.fullmatch('verify[0-9]{2}',attempt),'Missing exact reviewed attempt ID'
report_name='project_'+attempt+'.json'
assert not (EVIDENCE/report_name).exists(),'Preserve previous verification'
report=dict(passed=False,stage='Exact feature-branch game Content/clone technical reload; no look, gameplay or Shipping acceptance',assets=[],errors=[])
def vector(v):return [v.x,v.y,v.z]
def finite(values):assert all(math.isfinite(v) for v in values),'Non-finite geometry/pose'
def inspect(source,handoff):
    dest=source['destination']
    expected={p:e for p,e in handoff['objects'].items() if p.startswith(dest+'/')}
    paths=unreal.EditorAssetLibrary.list_assets(dest,recursive=True,include_folder=False)
    assert set(paths)==set(expected),'Loaded asset set differs from preserved manager report: '+dest
    objects=[unreal.EditorAssetLibrary.load_asset(p) for p in paths]
    assert all(objects),'Could not load saved package'
    registry=unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous([dest],True);registry.wait_for_completion()
    opts=unreal.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True)
    deps={};rows=[];references={}
    for obj in objects:
        path=obj.get_path_name();baseline=expected[path];cls=obj.get_class().get_name();package=path.split('.')[0]
        assert cls==baseline['class'],'Class changed'
        data=registry.get_asset_by_object_path(path,True);assert data.is_valid(),'On-disk asset not registered'
        raw=registry.get_dependencies(package,opts);assert raw is not None,'Registry dependency node missing'
        deps[package]=[str(d) for d in raw]
        assert all(not d.startswith('/Game/') or allowed_package(d) for d in deps[package]),'Game dependency outside exact claims'
        row=dict(path=path,class_name=cls)
        if isinstance(obj,unreal.StaticMesh):
            tri=obj.get_num_triangles(0);verts=obj.get_num_vertices(0);uv=obj.get_num_tex_coords(0)
            assert tri==baseline['triangles'] and 0<tri<=source['triangles'] and verts>0 and uv>=1,'Static geometry/UV invalid'
            box=obj.get_bounding_box();bounds=dict(min=vector(box.min),max=vector(box.max))
            finite(bounds['min']+bounds['max'])
            if 'bounds_cm' in baseline:
                assert all(abs(a-b)<0.01 for key in ['min','max'] for a,b in zip(bounds[key],baseline['bounds_cm'][key])),'Static bounds changed'
            slots=obj.get_editor_property('static_materials')
            refs=[s.material_interface.get_path_name() for s in slots if s.material_interface]
            assert len(refs)==source['materials'] and refs==baseline['materials'],'Static material slots changed'
            references[package]=refs
            assert not obj.get_editor_property('nanite_settings').enabled,'Unexpected Nanite change'
            body=obj.get_editor_property('body_setup');collision=None
            if body:
                geom=body.get_editor_property('agg_geom');collision=sum(len(geom.get_editor_property(k)) for k in ['sphere_elems','box_elems','sphyl_elems','convex_elems'])
            row.update(triangles=tri,source_triangles=source['triangles'],triangle_delta=tri-source['triangles'],vertices=verts,uv_channels=uv,bounds_cm=bounds,materials=refs,simple_collision_shapes=collision,nanite=False)
        elif isinstance(obj,unreal.SkeletalMesh):
            skeleton=obj.get_editor_property('skeleton');assert skeleton and skeleton.get_path_name()==baseline['skeleton']
            comp=unreal.SkeletalMeshComponent();comp.set_skinned_asset_and_update(obj)
            bones=[str(comp.get_bone_name(i)) for i in range(comp.get_num_bones())]
            assert bones==baseline['bones'] and len(bones)==baseline['bone_count'] and len(bones)>0,'Skeleton bones changed'
            vertices=unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem).get_num_verts(obj,0)
            assert vertices>0 and ('vertices' not in baseline or vertices==baseline['vertices']),'Skeletal vertices changed'
            bounds=obj.get_imported_bounds();center=vector(bounds.origin);extent=vector(bounds.box_extent);finite(center+extent)
            assert all(v>0 for v in extent),'Invalid skeletal bounds'
            if source['name']=='SentinelArmor':assert abs(extent[2]*2-170)<0.1,'Pilot source-scale height changed'
            slots=obj.get_editor_property('materials');refs=[s.material_interface.get_path_name() for s in slots if s.material_interface]
            assert refs==baseline['materials'] and len(refs)==source['materials'],'Skeletal material slots changed'
            references[package]=refs+[skeleton.get_path_name()]
            row.update(skeleton=skeleton.get_path_name(),bone_count=len(bones),bones=bones,vertices=vertices,bounds_cm=dict(origin=center,extent=extent),materials=refs,native_skeletal_uv_triangle_qa='No direct UV/triangle count assertion; source metadata and vertices recorded; deformation/topology approval pending')
            comp=None
        elif isinstance(obj,unreal.AnimSequence):
            skeleton=obj.get_editor_property('skeleton');assert skeleton and skeleton.get_path_name()==baseline['skeleton']
            duration=unreal.AnimationLibrary.get_sequence_length(obj);assert duration>0 and abs(duration-baseline['duration'])<0.0001,'Animation duration changed'
            skin=next(e for e in expected.values() if e['class']=='SkeletalMesh')
            bone=skin['bones'][0];samples=[]
            for t in [0,duration/2,duration]:
                pose=unreal.AnimationLibrary.get_bone_pose_for_time(obj,bone,t,False)
                position=vector(pose.translation);q=pose.rotation;rotation=[q.x,q.y,q.z,q.w];finite(position+rotation)
                samples.append(dict(time=t,translation=position,rotation=rotation))
            references[package]=[skeleton.get_path_name()]
            row.update(skeleton=skeleton.get_path_name(),duration=duration,pose_bone=bone,pose_samples=samples)
        elif isinstance(obj,unreal.Texture2D):
            size=[obj.blueprint_get_size_x(),obj.blueprint_get_size_y()];srgb=obj.get_editor_property('srgb');compression=str(obj.get_editor_property('compression_settings'))
            assert size==baseline['size'] and all(s>0 for s in size),'Texture size/data changed'
            assert srgb==baseline['srgb'] and compression==baseline['compression'],'Texture compression/color space changed'
            if 'NORMALMAP' in compression:assert not srgb,'Normal map sRGB enabled'
            row.update(size=size,srgb=srgb,compression=compression)
        elif isinstance(obj,unreal.Material):
            refs=[]
            for expression in unreal.MaterialEditingLibrary.get_material_expressions(obj):
                try:tex=expression.get_editor_property('texture')
                except Exception:continue
                if tex:refs.append(tex.get_path_name())
            texture_paths={p for p,e in expected.items() if e['class']=='Texture2D'}
            assert texture_paths<=set(refs),'Material lost serialized texture references'
            required=['MP_BASE_COLOR','MP_METALLIC','MP_ROUGHNESS','MP_NORMAL']
            if any(m['emissive_texture'] for m in source['source_material_features']):required.append('MP_EMISSIVE_COLOR')
            inputs={}
            for key in required:
                node=unreal.MaterialEditingLibrary.get_material_property_input_node(obj,getattr(unreal.MaterialProperty,key))
                assert node is not None,'Required PBR input missing: '+key
                inputs[key]=node.get_class().get_name()
            references[package]=refs;row.update(texture_references=refs,inputs=inputs)
        elif cls!='Skeleton':raise RuntimeError('Unreviewed asset class: '+cls)
        rows.append(row)
    for package,refs in references.items():
        assert {p.split('.')[0] for p in refs}<=set(deps[package]),'Registry lacks serialized reference coverage'
    textures=[r for r in rows if r['class_name']=='Texture2D']
    assert sum('NORMALMAP' in r['compression'] for r in textures)==1,'Expected one source normal map'
    expected_srgb=int(any(m['base_color_texture'] for m in source['source_material_features']))+int(any(m['emissive_texture'] for m in source['source_material_features']))
    assert sum(bool(r['srgb']) for r in textures)==expected_srgb,'Color/data sRGB roles disagree with glTF material metadata'
    return dict(name=source['name'],destination=dest,objects=rows,dependencies=deps,technical_pass=True,visual_approved=False,gameplay_bound=False,limits='Source-density intake; rig deformation/skin weights, final topology, fit/collision/LOD/performance, shader-backed materials, gameplay and cooked/Shipping use unverified')
try:
    assert Path(unreal.Paths.project_dir()).resolve()==HOST.resolve(),'Wrong verification host'
    source_guard()
    transfer=json.loads((EVIDENCE/'project_transfer.json').read_text());assert transfer['passed'],'Exact game transfer required'
    handoff=json.loads((EVIDENCE/'manager_handoff.json').read_text());assert handoff['passed']
    expected=handoff['packages']
    for content in [MANAGER/'Content',REPO/'Content',HOST/'Content']:verify_files(content,expected)
    for source in MANIFEST['assets']:
        report['assets'].append(inspect(source,handoff));write(report_name,report)
        unreal.SystemLibrary.collect_garbage()
    for content in [MANAGER/'Content',REPO/'Content',HOST/'Content']:verify_files(content,expected)
    source_guard();report.update(passed=True,three_way_hashes_unchanged=True,packages=expected,project_content=str(REPO/'Content'),errors=[])
except Exception:report['errors'].append(traceback.format_exc())
write(report_name,report)
if not report['passed']:raise RuntimeError(report['errors'][-1])

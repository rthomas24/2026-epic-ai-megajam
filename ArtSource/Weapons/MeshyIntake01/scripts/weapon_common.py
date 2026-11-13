from pathlib import Path
import unreal,json,hashlib
ROOT=Path(__file__).resolve().parents[1];REPO=ROOT.parents[2]
HOST=REPO/'Saved/WeaponIntake01/Host';PREFIX='/Game/TriadDrift/Art/Weapons/MeshyIntake01/'
MANIFEST=json.loads((ROOT/'intake_manifest.json').read_text(encoding='utf-8'))
EVIDENCE=ROOT/'evidence';EVIDENCE.mkdir(exist_ok=True)
def write(name,data):(EVIDENCE/name).write_text(json.dumps(data,indent=2,default=str),encoding='utf-8')
def guard():
    assert Path(unreal.Paths.project_dir()).resolve()==HOST.resolve(),'Wrong authoring host'
    for row in MANIFEST['assets']:
        for key in ['original','staged']:assert hashlib.sha256(Path(row[key]).read_bytes()).hexdigest()==row['sha256'],'Source hash mismatch'
def inspect(row):
    dest=row['proposed_destination'];paths=unreal.EditorAssetLibrary.list_assets(dest,recursive=True,include_folder=False)
    objects=[unreal.EditorAssetLibrary.load_asset(p) for p in paths];assert all(objects),'Missing saved object'
    meshes=[o for o in objects if isinstance(o,unreal.StaticMesh)];textures=[o for o in objects if isinstance(o,unreal.Texture2D)]
    materials=[o for o in objects if isinstance(o,unreal.MaterialInterface)]
    assert len(meshes)==1 and len(textures)==row['image_count'] and len(materials)>=1,'Imported object count mismatch'
    mesh=meshes[0];tri=mesh.get_num_triangles(0);assert tri==row['triangles'],'Triangle count changed'
    assert mesh.get_num_tex_coords(0)>=1,'UV channel missing'
    box=mesh.get_bounding_box();size=box.max-box.min;dims=[size.x,size.y,size.z]
    assert all(abs(a-b)<1 for a,b in zip(sorted(dims),sorted(x*100 for x in row['dimensions_gltf_m']))),'Scale/bounds mismatch'
    slots=mesh.get_editor_property('static_materials');assert len(slots)==1 and slots[0].material_interface,'Mesh material missing'
    texrows=[]
    for t in textures:
        texrows.append(dict(path=t.get_path_name(),size=[t.blueprint_get_size_x(),t.blueprint_get_size_y()],srgb=t.get_editor_property('srgb'),compression=str(t.get_editor_property('compression_settings'))))
    normal=[t for t in texrows if 'NORMALMAP' in t['compression']]
    assert len(normal)==1 and not normal[0]['srgb'],'Normal compression/color space incorrect'
    assert sum(t['srgb'] for t in texrows)==1+int(row['has_emissive_map']),'Color/data texture sRGB mismatch'
    assert all(t['size'][0]>0 and t['size'][1]>0 for t in texrows),'Texture source data missing'
    graph=[];refs=set()
    for m in materials:
        r=dict(path=m.get_path_name(),class_name=m.get_class().get_name())
        if isinstance(m,unreal.Material):
            r['texture_references']=[]
            for expression in unreal.MaterialEditingLibrary.get_material_expressions(m):
                try:t=expression.get_editor_property('texture')
                except Exception:continue
                if t:r['texture_references'].append(t.get_path_name())
            r['inputs']={}
            for key in ['MP_BASE_COLOR','MP_METALLIC','MP_ROUGHNESS','MP_NORMAL']+(['MP_EMISSIVE_COLOR'] if row['has_emissive_map'] else []):
                node=unreal.MaterialEditingLibrary.get_material_property_input_node(m,getattr(unreal.MaterialProperty,key))
                assert node is not None,'Missing material output '+key
                r['inputs'][key]=node.get_class().get_name()
        elif isinstance(m,unreal.MaterialInstanceConstant):
            r['parent']=m.get_editor_property('parent').get_path_name()
            r['texture_references']=[v.parameter_value.get_path_name() for v in m.get_editor_property('texture_parameter_values') if v.parameter_value]
        else:raise RuntimeError('Unverified material type')
        refs.update(r['texture_references']);graph.append(r)
    assert {t.get_path_name() for t in textures}<=refs,'Serialized material texture references incomplete'
    dependencies={};registry=unreal.AssetRegistryHelpers.get_asset_registry();opts=unreal.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True)
    registry.scan_paths_synchronous([dest],True)
    registry.wait_for_completion()
    for obj in objects:
        package=obj.get_path_name().split('.')[0]
        data=registry.get_asset_by_object_path(obj.get_path_name(),True)
        assert data.is_valid(),'Saved asset not ready in on-disk registry: '+package
        raw=registry.get_dependencies(package,opts)
        assert raw is not None,'Dependency node unavailable after synchronous scan: '+package
        deps=[str(d) for d in raw]
        assert all(not d.startswith('/Game/') or d.startswith(PREFIX) for d in deps),'Dependency outside owned content root'
        dependencies[package]=deps
    mesh_package=mesh.get_path_name().split('.')[0]
    assert slots[0].material_interface.get_path_name().split('.')[0] in dependencies[mesh_package],'Registry lacks serialized mesh material reference'
    for material in graph:
        expected={p.split('.')[0] for p in material['texture_references']}
        assert expected<=set(dependencies[material['path'].split('.')[0]]),'Registry lacks serialized material texture references'
    body=mesh.get_editor_property('body_setup');collision=None
    if body:
        geom=body.get_editor_property('agg_geom');collision=sum(len(geom.get_editor_property(k)) for k in ['sphere_elems','box_elems','sphyl_elems','convex_elems'])
    return dict(name=row['name'],destination=dest,mesh=mesh.get_path_name(),triangles=tri,vertices=mesh.get_num_vertices(0),uv_channels=mesh.get_num_tex_coords(0),dimensions_cm=dims,
        materials=graph,textures=texrows,dependencies=dependencies,simple_collision_shapes=collision,nanite_enabled=mesh.get_editor_property('nanite_settings').enabled,
        asset_paths=[o.get_path_name() for o in objects],technical_pass=True,visual_approved=False,gameplay_bound=False,
        limits='Source-scale intake only; forward/pivot/grip/muzzle calibration and polygon budgets remain pending. No optimization or final collision/gameplay approval.')

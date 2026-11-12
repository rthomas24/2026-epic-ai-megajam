"""One read-only UE5.9 template export. Run only through the Lead-cleared guard.

API basis: UE5.9 AssetExportTask.h, FbxExportOption.h, EditorExporters.cpp2424,
FbxMainExport.cpp282, SkinnedMeshComponent.h1176 and SkeletalMesh.h874/975.
No asset creation, package save, import, rig/weight edit or animation export.
"""
import datetime
import hashlib
import json
import os
from pathlib import Path
import traceback
import unreal


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as handle:
        for block in iter(lambda: handle.read(1048576), b''):
            digest.update(block)
    return digest.hexdigest()


def exclusive_json(path, value):
    with path.open('x', encoding='utf-8') as handle:
        json.dump(value, handle, indent=2)
        handle.write('\n')


def main():
    context_path = Path(os.environ['TD_LANCER_EXPORT_CONTEXT']).resolve(strict=True)
    context = json.loads(context_path.read_text(encoding='utf-8-sig'))
    character_root = next(p for p in Path(__file__).resolve().parents
                          if (p / 'unrealGameJam.uproject').is_file())
    owned = Path(__file__).resolve().parent.parent
    expected = character_root / 'ArtSource/Characters/Lancer/M01'
    if owned != expected or context['owned_root'] != str(owned):
        raise RuntimeError('Unexpected script/ownership context')
    host = Path(context['host_root']).resolve(strict=True)
    loaded = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    if loaded != host or host.name != 'hud-lancer-polish':
        raise RuntimeError('Only the explicitly selected frozen HUD project is allowed')
    output = Path(context['fbx']).resolve()
    output.relative_to(owned / 'source/base')
    manifest = output.with_suffix('.export.json')
    if output.exists() or manifest.exists():
        raise RuntimeError('Never overwrite an export or its evidence')
    output.parent.mkdir(parents=True, exist_ok=True)
    for item in context['packages']:
        if sha(host / item['path']) != item['sha256']:
            raise RuntimeError('Input package hash differs from approved checkpoint')
    dirty_before = {p.get_path_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()}
    mesh = unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
    skeleton = unreal.load_asset('/Game/Characters/Mannequins/Meshes/SK_Mannequin')
    if not isinstance(mesh, unreal.SkeletalMesh) or not isinstance(skeleton, unreal.Skeleton):
        raise RuntimeError('Exact existing template mesh/skeleton unavailable')
    # BlueprintGetter accessors are reflected properties, not Python methods.
    if mesh.get_editor_property('skeleton') != skeleton:
        raise RuntimeError('Template mesh is not bound to the agreed skeleton')
    component = unreal.SkeletalMeshComponent()
    component.set_editor_property('skeletal_mesh_asset', mesh)  # Uses the reflected setter on a transient component.
    count = component.get_num_bones()
    bones = [dict(index=i, name=str(component.get_bone_name(i)),
                  parent=str(component.get_parent_bone(component.get_bone_name(i)))) for i in range(count)]
    names = {bone['name'] for bone in bones}
    if not {'root', 'pelvis', 'spine_01', 'head', 'hand_r', 'foot_l', 'foot_r'} <= names:
        raise RuntimeError('Required mannequin reference bones missing')
    bounds = mesh.get_imported_bounds()  # BlueprintCallable, unlike the two getters above/below.
    extent = bounds.box_extent
    origin = bounds.origin
    size = [float(extent.x * 2), float(extent.y * 2), float(extent.z * 2)]
    if not all(value > 0 for value in size):
        raise RuntimeError('Invalid imported template bounds')
    materials = []
    for material in mesh.get_editor_property('materials'):
        interface = material.get_editor_property('material_interface')
        materials.append(dict(slot=str(material.get_editor_property('material_slot_name')),
                              material=interface.get_path_name() if interface else None))
    options = unreal.FbxExportOption()
    values = dict(ascii=False, force_front_x_axis=False, level_of_detail=False,
                  collision=False, export_source_mesh=False, export_morph_targets=False,
                  export_cloth_geometry=False, export_preview_mesh=False,
                  map_skeletal_motion_to_root=False, vertex_color=True,
                  fbx_export_compatibility=unreal.FbxExportCompatibility.FBX_2018,
                  bake_material_inputs=unreal.FbxMaterialBakeMode.DISABLED)
    for key, value in values.items():
        options.set_editor_property(key, value)
    task = unreal.AssetExportTask()
    for key, value in dict(object=mesh, exporter=unreal.SkeletalMeshExporterFBX(),
                           filename=str(output), selected=False, replace_identical=False,
                           prompt=False, automated=True, use_file_archive=False,
                           write_empty_files=False, options=options).items():
        task.set_editor_property(key, value)
    unreal.log('TD_LANCER_EXPORT begin exact Manny LOD0/reference skeleton; no package saves')
    exported = unreal.Exporter.run_asset_export_task(task)
    errors = list(task.get_editor_property('errors'))
    if not exported or errors:
        raise RuntimeError('Exporter failure: ' + repr(errors))
    with output.open('rb') as handle:
        signature = handle.read(23)
    if signature != b'Kaydara FBX Binary  \x00\x1a\x00' or output.stat().st_size < 100000:
        raise RuntimeError('FBX missing, empty or not expected binary format')
    dirty_after = {p.get_path_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()}
    if dirty_after - dirty_before:
        raise RuntimeError('Export/load unexpectedly dirtied packages: ' + repr(sorted(dirty_after - dirty_before)))
    for item in context['packages']:
        if sha(host / item['path']) != item['sha256']:
            raise RuntimeError('Template changed on disk')
    evidence = dict(schema=1,stage='Reference export script succeeded; native exit verification pending',
                    prepared_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                    engine=unreal.SystemLibrary.get_engine_version(),host_root=str(host),
                    mesh=mesh.get_path_name(),skeleton=skeleton.get_path_name(),
                    packages=context['packages'],scripts=context['scripts'],export_options={k:str(v) for k,v in values.items()},
                    fbx=dict(path=str(output),bytes=output.stat().st_size,sha256=sha(output)),
                    bones=bones,bone_count=count,materials=materials,material_count=len(materials),
                    imported_bounds_cm=dict(size=size,origin=[float(origin.x),float(origin.y),float(origin.z)]),
                    scale_limit='Imported bounds only; actual vertex dimensions/pose/axes need Blender inspection.',
                    fbx_units='cm per UE5.9 FbxMainExport.cpp; Z-up RH default front-axis convention retained',
                    new_rig_or_weights_or_animation=False,package_saves=False,
                    native_process_exit_verified=False,dirty_packages_added=[])
    exclusive_json(manifest,evidence)
    unreal.log('TD_LANCER_EXPORT SCRIPT_OK ' + json.dumps(dict(fbx=str(output),bones=count,bytes=output.stat().st_size)))


if __name__ == '__main__':
    try:
        main()
    except Exception:
        unreal.log_error('TD_LANCER_EXPORT FAILED\n' + traceback.format_exc())
        raise

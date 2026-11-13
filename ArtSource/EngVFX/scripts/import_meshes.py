# Triad Drift ENG-37: imports ArtSource/EngVFX/meshes/SM_TD_VFX_*.fbx into /Game/TriadDrift/Gameplay/VFX/Meshes
# (run in the editor: run_editor_py.py 8017 import_meshes.py). Removes any material the importer creates; the
# Niagara mesh renderers override the material anyway. Prints each mesh's bounds so the axes can be checked.
import os, glob
DEST = '/Game/TriadDrift/Gameplay/VFX/Meshes'
src = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), 'ArtSource', 'EngVFX', 'meshes')
EAL = unreal.EditorAssetLibrary
tasks = []
for f in sorted(glob.glob(os.path.join(src, 'SM_TD_VFX_*.fbx'))):
    if globals().get('ONLY_MESH') and os.path.basename(f)[:-4] not in ONLY_MESH:   # round 7: import just the named meshes
        continue
    t = unreal.AssetImportTask()
    t.filename = f; t.destination_path = DEST; t.automated = True; t.replace_existing = True; t.save = True
    tasks.append(t)
if not globals().get('NO_IMPORT'):
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
out = []
glow = unreal.load_asset('/Game/TriadDrift/Gameplay/VFX/Materials/M_TD_VFX_Glow')
for p in EAL.list_assets(DEST, True, False):
    a = unreal.load_asset(p)
    if isinstance(a, unreal.StaticMesh):
        a.set_material(0, glow)
        ns = a.get_editor_property('nanite_settings')
        ns.enabled = False          # additive / translucent materials cannot render through Nanite
        a.set_editor_property('nanite_settings', ns)
        a.modify()
        b = a.get_bounding_box()
        EAL.save_loaded_asset(a)
        out.append('%s min %s max %s tris %d' % (a.get_name(), b.min, b.max, a.get_num_triangles(0)))
    else:
        out.append('extra asset %s (%s) -> deleted %s' % (p, a.get_class().get_name(), EAL.delete_asset(p)))
"\n".join(out)

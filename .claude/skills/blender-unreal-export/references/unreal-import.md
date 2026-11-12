# Unreal-side reference (UE 5.7+ / 5.9 preview)

## Facts that drive the settings
- Unreal is left-handed, Z-up, X-forward, centimetres. Blender with Unit Scale 1.0 plus FBX "Apply
  Unit" and "FBX Units Scale" gives the correct size, with object transforms left at 1.0.
- Since UE 5.5, FBX imports go through **Interchange** by default. The console variable
  `Interchange.FeatureFlags.Import.FBX False` brings back the legacy importer.
- In UE 5.7, Interchange always creates **material instances** from FBX materials. That's fine: we
  replace them with our own MIs anyway.
- glTF has no legacy fallback and a reported skinned-mesh regression in 5.8. **Use FBX** for
  authored assets: the UCX / SOCKET / LOD conventions are documented for FBX.
- Lumen projects don't need lightmap UVs, so turn off "Generate Lightmap UVs".
- **Nanite** covers opaque and masked materials only: no translucency, no morph targets, limited
  World Position Offset. Forum reports put the cap at about 64 materials per mesh. Moving static
  meshes (ships on pawns) can use Nanite.
- **Epic's "Send to Unreal" is abandoned.** The last Epic release was 2.4.3 (Nov 2023); the
  community fork has open blockers on Blender 5.x. Don't depend on it.

## Manual import settings (Content Browser → Import → FBX)

| Setting | Value |
|---|---|
| Import uniform scale | 1.0 |
| Combine meshes | on |
| Collision | on, according to mesh name (UCX_), one convex hull per UCX |
| Generate lightmap UVs | off |
| Normal import | Import Normals (keep authored hard edges and custom normals) |
| Build Nanite | per manifest (`nanite_recommended`): big environment pieces and dense rocks yes; glass and blade no |
| Materials | import, then replace with our MIs |
| Textures | import separately: `T_*_BC` / `_E` sRGB on, Default; `T_*_N` Normalmap, Flip Green **off**; `T_*_ORM` Masks, sRGB off |

## Master materials the import script expects (Nick creates them once, in his Art folder)
- **`/Game/TriadDrift/Art/Materials/M_TD_Flat`:** vector params `BaseColor`, `EmissiveColor`; scalar
  params `Metallic`, `Roughness`, `EmissiveStrength`. Wire BaseColor → Base Color, Metallic,
  Roughness, and EmissiveColor × EmissiveStrength → Emissive.
- **`/Game/TriadDrift/Art/Materials/M_TD_Textured`:** texture params `BaseColorTex` (sRGB),
  `ORMTex` (Masks; R → AO, G → Roughness, B → Metallic), `NormalTex` (Normal sampler),
  `EmissiveTex` (sRGB); scalar `EmissiveStrength`. Emissive = EmissiveTex × EmissiveStrength.
- Values from Blender are **linear** RGB, matching Unreal's LinearColor, so no conversion.
- Emissive above 1 drives bloom. Turn on "Emissive Light Source" on small glowing meshes if Lumen
  should light the surroundings.

## Python import API names (UE 5.7 docs; verify with `dir()`)
- `unreal.AssetImportTask`: `filename`, `destination_path`, `destination_name`, `automated`,
  `replace_existing`, `save`, `options`. Run with
  `unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])`.
- `unreal.ImportAssetParameters`: `is_automated`, `replace_existing`, `override_pipelines`.
  Run with `unreal.InterchangeManager.get_interchange_manager_scripted().import_asset(dest, unreal.InterchangeManager.create_source_data(path), params)`.
- The pipeline asset starts as a copy of `/Interchange/Pipelines/DefaultAssetsPipeline`. Its
  `mesh_pipeline` sub-object has `combine_static_meshes`, `collision`,
  `import_collision_according_to_mesh_name`, `one_convex_hull_per_ucx`, `generate_lightmap_u_vs` and
  `build_nanite`.
- `unreal.StaticMeshEditorSubsystem`: `set_nanite_settings`, `set_generate_lightmap_uv`,
  `add_simple_collisions`, `set_convex_decomposition_collisions`, `set_lod_group`. It has no socket
  methods.
- Legacy fallback: `unreal.FbxImportUI()` with `static_mesh_import_data` fields
  (`combine_meshes`, `auto_generate_collision`, `one_convex_hull_per_ucx`, `generate_lightmap_u_vs`,
  `normal_import_method`, `convert_scene`).
- `unreal.MaterialEditingLibrary`: `set_material_instance_parent`,
  `set_material_instance_vector_parameter_value`, `..._scalar_...`, `..._texture_...`.
- Unreal MCP options: the UE 5.9 built-in MCP (experimental; `ImportFile` for static and skeletal
  meshes and textures; Streamable HTTP at `127.0.0.1:8000/mcp`; enable All Toolsets), or flopperam's
  `python_execution` to run `ue_import_assets.py`.

## LODs
Use Nanite for big static pieces. For non-Nanite meshes, let Unreal auto-generate LODs or set a LOD
group (`set_lod_group(sm, "LargeProp")`). Only author `_LOD1` / `_LOD2` meshes if a hero asset
needs hand-made LODs.

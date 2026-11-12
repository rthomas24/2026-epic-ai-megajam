# Blender 5.x to Unreal Engine 5.7+/5.9 Game-Ready Asset Pipeline, incl. AI-Generated Mesh Cleanup (research notes, as of 12 Nov 2026)

_Tags used below: **[B5.2]** = checked against Blender's `blender-v5.2-release` source on GitHub or the Blender 5.x manual/API reference bundled with the Blender Lab MCP server. **[UE5.9 docs]** = Epic's current documentation set. Pages are labelled "Unreal Engine 5.9 Documentation", but much of their content is older (some sections still say UE4). **[UE5.5]/[UE5.7]/[UE5.8]** = forum or bug reports about that version. **"via search summary"** = taken from a search-engine summary of the page, not a full fetch, so confidence is lower. **RECENT CHANGE** marks things that changed in the last ~12 months. All code below is an untested template assembled from the cited API signatures._

---

## 1. Units, scale and axes; how UE Interchange treats FBX vs glTF (plus skeletal-mesh basics)

### Takeaway
Keep Blender at Metric with Unit Scale 1.0 (1 BU = 1 m) and model at real size. Export FBX at Scale 1.0 with "Apply Unit" on. Blender 5.2's own code shows that both "All Local" (the default) and "FBX Units Scale" end up with a 100x metre-to-centimetre conversion. "FBX Units Scale" puts it in the file's unit header, which leaves object and armature transforms at 1.0. UE is left-handed, Z-up, X-forward, in centimetres. Since UE 5.5, FBX goes through Interchange by default (legacy importer reachable via a cvar), and in 5.7 FBX materials always import as Material Instances. glTF has no legacy fallback and uses OpenGL normals, Y-up and metres. Use FBX for kit and ship assets that need UCX_/SOCKET_/LOD conventions, and check orientation and scale once with a calibration asset before mass export.

### Cited Findings

#### Unreal's frame of reference
- UE: "In the Unreal Editor the coordinate system is left-handed and uses a Z-up axis". +X = forward (red), +Y = right (green), +Z = up (blue). [UE5.9 docs] — [Epic: Coordinate System and Spaces](https://dev.epicgames.com/documentation/en-us/unreal-engine/coordinate-system-and-spaces-in-unreal-engine)
- UE's unit is the centimetre. The FBX option "Convert Scene Unit" "converts FBX units to Unreal's unit of centimeters". [UE5.9 docs] — [Epic: FBX Import Options Reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-import-options-reference-in-unreal-engine)
- Lumen/distance fields: mesh distance-field resolution "is based on the imported scale". A mesh imported very small and scaled up on placement may get too little resolution, so import at final size. [UE5.9 docs] — [Epic: Lumen Technical Details](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-technical-details-in-unreal-engine)

#### Blender 5.2 FBX exporter (`io_scene_fbx` add-on v5.15.0, `"blender": (5, 0, 0)`, still a bundled core add-on under `scripts/addons_core`) [B5.2]
Exact operator properties and defaults of `bpy.ops.export_scene.fbx` — [Blender source: io_scene_fbx/__init__.py @ blender-v5.2-release](https://github.com/blender/blender/blob/blender-v5.2-release/scripts/addons_core/io_scene_fbx/__init__.py)

| Property (UI name) | Python name | Default | Notes from source |
|---|---|---|---|
| Selected Objects | `use_selection` | False | "Export selected and visible objects only" |
| Visible Objects / Active Collection / Source Collection | `use_visible`, `use_active_collection`, `collection` | False / False / "" | |
| Object Types | `object_types` | {'EMPTY','CAMERA','LIGHT','ARMATURE','MESH','OTHER'} | ENUM_FLAG |
| Scale | `global_scale` | 1.0 | "Some importers do not support scaled armatures!" |
| Apply Unit | `apply_unit_scale` | True | "Take into account current Blender units settings (if unset, raw Blender Units values are used as-is)" |
| Apply Scalings | `apply_scale_options` | `FBX_SCALE_NONE` ("All Local"). There is no explicit default, so the first item is used | items: `FBX_SCALE_NONE` "All Local", `FBX_SCALE_UNITS` "FBX Units Scale", `FBX_SCALE_CUSTOM` "FBX Custom Scale", `FBX_SCALE_ALL` "FBX All" |
| Forward / Up | `axis_forward`, `axis_up` | `'-Z'`, `'Y'` | `@orientation_helper(axis_forward='-Z', axis_up='Y')` on ExportFBX |
| Use Space Transform | `use_space_transform` | True | "When disabled only the axis space is written to the file and all object transforms are left as-is" |
| Apply Transform | `bake_space_transform` | False | "WARNING! experimental option, use at own risk, known to be broken with armatures/animations" |
| Apply Modifiers | `use_mesh_modifiers` | True | except Armature modifiers |
| Smoothing | `mesh_smooth_type` | `'OFF'` ("Normals Only") | items OFF / FACE / EDGE / SMOOTH_GROUP. Tooltip: "prefer 'Normals Only' option if your target importer understands custom normals" |
| Vertex Colors | `colors_type` | `'SRGB'` | NONE / SRGB / LINEAR |
| Loose Edges | `use_mesh_edges` | False | |
| Tangent Space | `use_tspace` | False | "(will only work correctly with tris/quads only meshes!)" |
| Triangulate Faces | `use_triangles` | False | |
| Custom Properties | `use_custom_props` | False | |
| Add Leaf Bones | `add_leaf_bones` | **True** | source comment: `# False for commit!` |
| Primary / Secondary Bone Axis | `primary_bone_axis`, `secondary_bone_axis` | 'Y' / 'X' | |
| Only Deform Bones | `use_armature_deform_only` | False | |
| Armature FBXNode Type | `armature_nodetype` | 'NULL' | |
| Baked Animation | `bake_anim` (+ `bake_anim_use_all_bones`, `..._use_nla_strips`, `..._use_all_actions`, `..._force_startend_keying`, `bake_anim_step`, `bake_anim_simplify_factor`) | True (all True; step 1.0; simplify 1.0) | |
| Path Mode / Embed Textures | `path_mode`, `embed_textures` | AUTO / False | embed "only for 'Copy' path mode!" |
| Batch Mode | `batch_mode` | 'OFF' | OFF / SCENE / COLLECTION ("does not include content of children collections") / SCENE_COLLECTION / ACTIVE_SCENE_COLLECTION. `use_batch_own_dir` defaults to True |

#### How the 5.2 exporter does the unit math [B5.2]
- `units_blender_to_fbx_factor(scene)` returns `100.0 if (scene.unit_settings.system == 'NONE') else (100.0 * scene.unit_settings.scale_length)` — [Blender source: fbx_utils.py @ v5.2](https://github.com/blender/blender/blob/blender-v5.2-release/scripts/addons_core/io_scene_fbx/fbx_utils.py)
- In `export_fbx_bin.py`: `unit_scale = units_blender_to_fbx_factor(scene) if apply_unit_scale else 100.0`. Then:
  - `FBX_SCALE_NONE`: `global_matrix = Matrix.Scale(unit_scale * global_scale, 4) @ global_matrix; unit_scale = 1.0`. The 100x lands on root object transforms and the FBX `UnitScaleFactor` is 1 (cm).
  - `FBX_SCALE_UNITS`: only `global_scale` goes into the matrix. `UnitScaleFactor` = `unit_scale` (100 at Unit Scale 1.0, meaning file units are metres).
  - `FBX_SCALE_CUSTOM`: unit scale goes into the matrix and `UnitScaleFactor = global_scale`.
  - `FBX_SCALE_ALL`: `unit_scale = global_scale * unit_scale` is written to `UnitScaleFactor`.
  - The file writes `UnitScaleFactor` and `OriginalUnitScaleFactor`.
  - [Blender source: export_fbx_bin.py @ v5.2](https://github.com/blender/blender/blob/blender-v5.2-release/scripts/addons_core/io_scene_fbx/export_fbx_bin.py)
- Consequence of the math: with Unit Scale 0.01, `units_blender_to_fbx_factor` = 1.0. No scaling is applied anywhere and raw Blender values are written as centimetres. — same source

#### Epic's own "Send to Unreal" defaults (a known-working historical reference config, Epic-authored)
- Blender side (`default.json`): `apply_unit_scale=True`, `global_scale=1.0`, `apply_scale_options=FBX_SCALE_NONE`, `axis_forward=Y`, `axis_up=Z`, `bake_space_transform=False`, `mesh_smooth_type=FACE`, `use_tspace=False`, `use_mesh_modifiers=True`, `add_leaf_bones=False`, `primary_bone_axis=Y`, `secondary_bone_axis=X`, `use_custom_props=False`. Export call: `use_selection=True`, `object_types={'ARMATURE','MESH','EMPTY'}`, `bake_anim_use_nla_strips=True`, `bake_anim_use_all_actions=False`. — [EpicGames/BlenderTools default.json](https://github.com/EpicGames/BlenderTools/blob/main/send2ue/resources/setting_templates/default.json); [send2ue/core/export.py](https://github.com/EpicGames/BlenderTools/blob/main/send2ue/core/export.py)
- UE side (legacy FBX importer, static mesh): `convert_scene=False`, `convert_scene_unit=False`, `force_front_x_axis=False`, `transform_vertex_to_absolute=True`, `bake_pivot_in_vertex=False`, `normal_import_method=FBXNIM_IMPORT_NORMALS_AND_TANGENTS`, `normal_generation_method=MIKK_T_SPACE`, `compute_weighted_normals=True`, `combine_meshes=False`, `auto_generate_collision=True`, `one_convex_hull_per_ucx=True`, `generate_lightmap_u_vs=False`, `build_nanite=False`, `remove_degenerates=True`, `reorder_material_to_fbx_order=True`, `static_mesh_lod_group=none`. The skeletal side is similar, plus `import_morph_targets=True`, `import_meshes_in_bone_hierarchy=True`, `use_t0_as_ref_pose=False`. — [same default.json](https://github.com/EpicGames/BlenderTools/blob/main/send2ue/resources/setting_templates/default.json)

#### Legacy FBX importer options [UE5.9 docs]
- **Convert Scene**: FBX coordinate system to Unreal's. **Force Front XAxis**: "the front axis maps to X instead of -Y". **Convert Scene Unit**: converts FBX units to centimetres. **Transform Vertex to Absolute**: when on, the FBX transform, offset and pivot are relative to the world origin. **Bake Pivot in Vertex**: needs Transform Vertex to Absolute off. **Normal Import Method**: Compute Normals / Import Normals / Import Normals and Tangents. **Normal Generation Method**: MikkTSpace or built-in. **Generate Lightmap UVs**. **Auto Generate Collision** (skipped if the file has custom collision). **One Convex Hull per UCX**. **Import Mesh LODs**. **Static Mesh LODGroup**. **Material Import Method**: Create New Materials / Create New Instanced Materials / Do Not Create Material. **Invert Normal Maps**. — [Epic: FBX Import Options Reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-import-options-reference-in-unreal-engine)
- The FBX static-mesh pipeline page says the importer uses FBX SDK **2020.2** and that other versions may cause incompatibilities. — [Epic: FBX Static Mesh Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine)

#### Interchange (the default importer) [UE5.9 docs]
- Interchange Import Reference options:
  - **Common**: Use Source Name for Asset, Asset Name, Offset Translation, Offset Rotation, Offset Uniform Scale.
  - **Common Meshes**: Force All Mesh as Type (None/Static/Skeletal), Import Lods, Vertex Color Import Option (Replace/Ignore/Override). Build: Recompute Normals, Recompute Tangents, Use MikkTSpace, Compute Weighted Normals, Use High Precision Tangent Basis, Use Full Precision UVs, Use Backwards Compatible F16Trunc UVs, Remove Degenerates.
  - **Static Meshes**: Convert Statics with Morph Targets to Skeletals, Import Static Meshes, Combine Static Meshes, LOD Groups (Deco, Foliage, High Detail, Large Prop, Level Architecture, None, Small Prop, Vista). Collision: Import Collision, Import Collision According to Mesh Name, One Convex Hull Per UCX. Build: Build Nanite, Build Reverse Index Buffer, Generate Lightmap UVs, Two-Sided Distance Field Generation, Min Lightmap Resolution, Source/Destination Lightmap Index, Build Scale, Distance Field Resolution Scale (1.0), Distance Field Replacement Mesh, Max Lumen Mesh Cards.
  - **Materials**: Import as Material / Import as Material Instance. Parent Material (None = engine picks; grouped by shader model such as Lambert/Phong, Unlit, Metallic/Roughness PBR, Specular/Glossiness PBR). Parents live in `/Interchange/Materials`.
  - **Textures**: Detect Normal Map Texture (adjusts SRGB, Compression, LOD Group), Flip Normal Map Green Channel, Import UDIMs. Advanced: Prefer Compressed Source Data, Allow Non Power of Two.
  - "Import Into Level" works with FBX, glTF, GLB and MaterialX, with a reduced option set.
  - The page lists no FBX-only or glTF-only options, and no Convert Scene / Force Front X Axis / Convert Scene Unit equivalents.
  - — [Epic: Interchange Import Reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/interchange-import-reference-in-unreal-engine)
- **RECENT CHANGE [UE5.5]**: Interchange became the default for FBX import in 5.5 (forum reports, not official release-note wording). Epic staff: to use legacy FBX "set the console variable `Interchange.FeatureFlags.Import.FBX` to false", with no need to disable the Interchange plugin. For glTF/GLB "there is no more legacy fallback" (via search summary). — [UE forum: FBX import changed?](https://forums.unrealengine.com/t/fbx-import-changed/2144275); [UE forum: Unreal 5.5 Broken Interchange Pipeline (FBX/GLTF)](https://forums.unrealengine.com/t/unreal-5-5-broken-interchange-pipeline-fbx-gltf/2169673)
- The poly-hammer Send to Unreal fork tells users to put this in `DefaultEngine.ini`: `[ConsoleVariables]` / `Interchange.FeatureFlags.Import.FBX=False`. The add-on depends on the legacy FBX importer from UE 5.5 on. — [poly-hammer/BlenderTools release Send to Unreal 2.6.3](https://github.com/poly-hammer/BlenderTools/releases)
- **RECENT CHANGE [UE5.7]**: FBX through Interchange "only generates Material Instances in a similar way to glTF", based on a Phong master material, even when "Import as Materials" is chosen. An Epic developer said the UI is misleading and the fix missed 5.7. The workaround is the cvar above. A user also reports that the cvar fixed skinned-mesh reimport freezes in 5.7 (via search summary). — [UE forum: 5.7 Preview FBX materials imported as instances](https://forums.unrealengine.com/t/5-7-preview-fbx-materials-getting-imported-as-instances-instead-of-standalone-materials/2664714)
- glTF "import into level"/"as scene" generates plain materials rather than instances (via search summary). — [same thread](https://forums.unrealengine.com/t/5-7-preview-fbx-materials-getting-imported-as-instances-instead-of-standalone-materials/2664714?page=2)
- **RECENT CHANGE [UE5.8]**: one forum report says skinned glTF splits into several skeletal meshes because the shipped glTF pipeline preset does not combine, and calls it a regression from 5.7. — [UE forum: UE 5.8 glTF skinned splits](https://forums.unrealengine.com/t/ue-5-8-gltf-skinned-gltf-splits-into-several-skeletal-meshes-the-shipped-gltf-pipeline-preset-ships-do-not-combine-regression-from-5-7/2740512)
- UE's Interchange glTF parser (GLTFCore) recognises extensions including KHR_materials_emissive_strength, unlit, clearcoat, transmission, sheen, variants, IOR, specular, iridescence, anisotropy, volume, plus KHR_texture_transform, KHR_mesh_quantization and KHR_lights_punctual. Recognition does not necessarily mean full support (via search summary). — [Epic API: GLTF::EExtension](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/GLTFCore/GLTF__EExtension)
- The 5.9 release notes do not mention Interchange/FBX/glTF import changes. — [Epic: UE 5.9 Preview Release Notes (updated 11/9/26)](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-9-release-notes)

#### Blender 5.2 glTF exporter (`io_scene_gltf2` v5.2.41) [B5.2]
- `export_format` default index 0 (binary GLB). `export_yup` "+Y Up" = True ("glTF convention"). `export_apply` (Apply Modifiers) = **False**, with the warning "prevents exporting shape keys". `export_tangents` = False. `export_normals` = True. `export_texcoords` = True. `export_materials` = 'EXPORT' (also PLACEHOLDER / VIEWPORT / NONE). `export_image_format` = 'AUTO' (or JPEG / WEBP / NONE). `export_vertex_color` = 'MATERIAL'. `export_draco_mesh_compression_enable` = False. `export_gpu_instances` = False. Lighting mode = 'SPEC'. — [Blender source: io_scene_gltf2/__init__.py @ v5.2](https://github.com/blender/blender/blob/blender-v5.2-release/scripts/addons_core/io_scene_gltf2/__init__.py)

#### Skeletal-mesh basics
- The FBX exporter defaults `add_leaf_bones=True`. Epic's Send to Unreal sets it False. Both use bone axes Y/X. — [Blender FBX source v5.2](https://github.com/blender/blender/blob/blender-v5.2-release/scripts/addons_core/io_scene_fbx/__init__.py); [send2ue default.json](https://github.com/EpicGames/BlenderTools/blob/main/send2ue/resources/setting_templates/default.json)
- "Apply Transform" (`bake_space_transform`) is "known to be broken with armatures/animations". — [Blender FBX source v5.2](https://github.com/blender/blender/blob/blender-v5.2-release/scripts/addons_core/io_scene_fbx/__init__.py)
- Extra root bone: the armature object is exported as a node that UE treats as the root. UE4's FBX importer (since 4.16) removes an object named exactly "Armature" (forum, older UE4 threads; via search summary). — [UE forum: extra root bone fix only works if root object is named "armature"](https://forums.unrealengine.com/t/blender-specific-skeleton-extra-root-bone-fix-only-works-if-root-object-is-named-armature/393432)
- Historical Blender bug: "FBX: exported mesh/animation does not work properly in UE4 when exported using 1 metric unit scene scale" (title only, via search). — [Blender T51704](https://developer.blender.org/T51704)
- Interchange skeletal options include Import Content Type, Import Morph Targets, Create Physics Asset, Use T0 As Ref Pose, Bone Influence Limit, Use High Precision Skin Weights and Import Meshes in Bone Hierarchy. — [Epic: Interchange Import Reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/interchange-import-reference-in-unreal-engine)
- **RECENT CHANGE [B5.0/5.1]**: Blender 5.0 moved `Action.fcurves` into slotted-action channelbags. Old export scripts and add-ons that read `action.fcurves` fail with `AttributeError`. Blender 5.1 ships Python 3.13. — [poly-hammer issue #179](https://github.com/poly-hammer/BlenderTools/issues/179); [poly-hammer issue #193](https://github.com/poly-hammer/BlenderTools/issues/193)

### Inferences

#### Recommended baseline (static meshes; Blender 5.2.2 to UE 5.7/5.9)
| Stage | Setting | Value | Why |
|---|---|---|---|
| Blender scene | `scene.unit_settings.system` / `scale_length` / `length_unit` | `'METRIC'` / `1.0` / `'METERS'` (or `'CENTIMETERS'` for display only) | Real-size modelling. Blender tools (bevel, physics, remesh voxel size, bake cage) all assume metres. |
| Blender object | transforms | Rotation (0,0,0), Scale (1,1,1) applied. Origin = intended UE pivot. Object placed at world origin when exported. | Avoids baked rotations and 100x node scale surprises. Matches Epic's "pivot at origin" rule (section 2). |
| FBX export | `global_scale` / `apply_unit_scale` / `apply_scale_options` | `1.0` / `True` / `'FBX_SCALE_UNITS'` | Per the 5.2 math, object transforms stay 1.0 and the m-to-cm factor goes in the unit header. `'FBX_SCALE_NONE'` (Blender default, Send to Unreal default) is also fine for static meshes. |
| FBX export | axes | Either A: Blender default `axis_forward='-Z', axis_up='Y'` with UE Convert Scene on (legacy default), or B: Send to Unreal pair `axis_forward='Y', axis_up='Z'` with UE `convert_scene=False` | Don't mix the pairs. |
| FBX export | `use_space_transform` / `bake_space_transform` | `True` / `False` | Apply Transform is experimental. |
| UE import | Interchange default pipeline, or legacy via `Interchange.FeatureFlags.Import.FBX False` | Legacy only if a tool (Send to Unreal) or a known 5.7 Interchange bug forces it | 5.7 FBX materials always become MIs under Interchange. |

#### Axis mapping (derived, verify once)
- Both axis pairs above should give: Blender +X goes to UE +X (forward), Blender +Y to UE -Y, Blender +Z to UE +Z. This follows from Force Front XAxis' doc text (default front is -Y), Blender's front view looking along +Y, and the right-to-left-handed Y flip.
- **So model ships and pawns with the nose pointing Blender +X.** A character facing Blender -Y lands facing UE +Y. That matches the common UE convention of rotating Character meshes -90 degrees yaw.
- Not stated verbatim by Epic or Blender. Check it with the calibration asset below.

#### Calibration asset (run once per engine/Blender version)
1. In Blender, make `SM_Calib_100cm`: a 1 m cube with a wedge on the +X face. Add `UCX_SM_Calib_100cm_00` (a slightly inset cube) and an Empty `SOCKET_Nose` at (0.5, 0, 0) parented to it.
2. Export with the baseline settings and import with Interchange defaults.
3. Pass criteria:
   - Bounds are 100x100x100 UU.
   - Actor scale is 1.0.
   - The wedge points along UE's red X axis.
   - The Collision view shows the UCX hull.
   - The Socket Manager lists `Nose` (or `SOCKET_Nose`) at X=50.
4. Repeat with a glTF export to see how Interchange glTF handles the same file.

#### FBX vs glTF for this project
| Need | FBX (recommended for authored assets) | glTF/GLB (fine for AI-generator props) |
|---|---|---|
| UCX_/UBX_/USP_/UCP_ custom collision, SOCKET_ | Documented by Epic for FBX | Not documented for glTF. Add collision in UE (`add_simple_collisions`) instead |
| Materials | 5.7 Interchange makes MIs of a Phong parent anyway, so plan your own master materials and reassign | Makes MIs of glTF parents. ORM packing native (R=AO, G=Rough, B=Metal) |
| Normal maps | You control the convention (bake DirectX, see section 3) | glTF is OpenGL convention. Check the green channel in UE |
| Units/axes | Unit-scale math above | Metres, Y-up. Blender's exporter converts (+Y Up on) |
| Fallback if Interchange breaks | Legacy cvar available | None |

#### Skeletal (crew enemies; light-touch since they're sourced/rigged elsewhere)
- Prefer importing the vendor FBX (Mixamo/Rodin/Meshy/Tripo rig) **directly into UE** and retargeting there, rather than round-tripping through Blender.
- If you must export from Blender:
  - Armature and mesh at scale 1 with transforms applied.
  - `add_leaf_bones=False`, `use_armature_deform_only=True`, `object_types={'ARMATURE','MESH'}`, `bake_space_transform=False`, `apply_scale_options='FBX_SCALE_UNITS'`.
  - The `FBX_SCALE_UNITS` choice keeps the armature node at scale 1 per the 5.2 math. The alternative is Unit Scale 0.01, modelling in cm numerically.
  - Name the armature object "Armature" and test whether Interchange still strips it. Unverified for Interchange.

### Gaps
- No Epic doc found that says how **Interchange's** FBX translator handles `UnitScaleFactor` or the Blender Apply-Scalings modes, or whether it has Convert Scene / Force Front X equivalents. The reference page doesn't list them. Use the calibration asset.
- No official Epic page on **glTF import** found. The "glTF File Format Support" page covers export only. Normal-map green-channel handling, material parents and unit conversion on glTF import are undocumented here.
- The 5.9 Preview release notes are silent on import pipeline changes. Whether the 5.7 FBX-material-instance behaviour is fixed in 5.8/5.9 was not found.
- Whether Interchange keeps UE4's "Armature" root-node stripping is not confirmed.

---

## 2. Mesh requirements (naming, collision, sockets, LODs, pivots and grid, transforms and triangulation, normals and tangents, UVs and lightmaps, texel density, material slots, poly budgets, Nanite)

### Takeaway
Use Epic's prefixes (SM_, SK_, SKEL_, PHYS_, M_, MI_, T_). Custom collision `UCX_<RenderMeshName>_##` (also UBX_/USP_/UCP_) and sockets `SOCKET_<name>` must match the **final object name** in Blender. Export one render mesh per FBX. Pivot at world origin: a corner for kit pieces on a 100 cm grid, centre or centre-of-mass for ships. Apply rotation and scale. Triangulate in Blender before baking and exporting. Use Smooth-by-Angle plus custom normals, exported as "Face" smoothing to silence UE's smoothing-group warning. Tangents are MikkTSpace on both sides. UV0 is for textures; **no lightmap UVs for a Lumen project**. Nanite handles opaque and masked static meshes (and skeletal in 5.5+). It does not do translucency, morph targets or forward rendering, has limited WPO, and caps at about 64 materials. Glass, energy blades and shields go on separate non-Nanite meshes.

### Cited Findings

#### Naming [UE5.9 docs]
- Pattern: `[AssetTypePrefix]_[AssetName]_[Descriptor]_[OptionalVariantLetterOrNumber]`. Prefixes: Static Mesh `SM_`, Skeletal Mesh `SK_`, Skeleton `SKEL_`, Physics Asset `PHYS_`, Physics Material `PM_`, Material `M_`, Material Instance `MI_`, Post Process Material `PPM_`, Texture `T_`, Blueprint `BP_`, Widget BP `WBP_`, Anim BP `ABP_`, Blueprint Interface `BI_`, Actor Component `AC_`, Data Table `DT_`, Curve Table `CT_`, Enum `E_`, Structure `F_`, Niagara System `FXS_`, Niagara Emitter `FXE_`, Niagara Function `FXF_`, Animation Sequence `AS_`, Montage `AM_`, Blend Space `BS_`, Level Sequence `LS_`, Rig `Rig_`, HDRI `HDR_`. The list "is not exhaustive", it is "only a recommendation", and texture suffixes, Material Functions and Sounds are not covered. — [Epic: Recommended Asset Naming Conventions](https://dev.epicgames.com/documentation/en-us/unreal-engine/recommended-asset-naming-conventions-in-unreal-engine-projects)
- Send to Unreal's affix extension defaults: `M_` for materials, `T_` for textures. LOD regex default `(?i)(_LOD\d).*`, which strips the LOD postfix (`pCube1_LOD0` becomes `pCube1`). — [send2ue default.json](https://github.com/EpicGames/BlenderTools/blob/main/send2ue/resources/setting_templates/default.json); [send2ue static-mesh docs](https://github.com/EpicGames/BlenderTools/blob/main/docs/send2ue/asset-types/static-mesh.md)

#### Custom collision [UE5.9 docs; content from the FBX era]
| Prefix | Shape | Rule |
|---|---|---|
| `UBX_[RenderMeshName]_##` | Box | Must stay a regular rectangular prism. Don't move vertices |
| `UCP_[RenderMeshName]_##` | Capsule | Cylinder capped with hemispheres, ~8 segments. Don't move vertices |
| `USP_[RenderMeshName]_##` | Sphere | ~8 segments. Don't move vertices |
| `UCX_[RenderMeshName]_##` | Convex | Any fully closed convex shape |
- RenderMeshName must exactly match the render mesh name (`UCX_Tree_01` for `Tree_01`; extra hulls `UCX_Tree_01_00`, `_01`, and so on). Hulls work best when they don't intersect, so leave gaps. Boxes and spheres don't work on non-uniformly scaled meshes. Spheres are used only for rigid-body and zero-extent traces. UCX is generally recommended. Collision and render mesh go in the same FBX. **If a file has multiple meshes with custom collision, only the first mesh's collision is imported.** — [Epic: FBX Static Mesh Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine)
- Interchange Python names on `InterchangeGenericMeshPipeline` [UE5.7]: `collision` (bool; imports custom collision or auto-generates if none), `fallback_collision_type`, `import_collision_according_to_mesh_name` (UBX_/UCP_/USP_/UCX_), `one_convex_hull_per_ucx`, `force_collision_primitive_generation`. `import_collision` is deprecated in favour of `collision`. — [Epic Python API 5.7: InterchangeGenericMeshPipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/InterchangeGenericMeshPipeline?application_version=5.7)
- Send to Unreal: collision objects are made **children of the mesh**. Known bug [Blender 5.0.1 / UE 5.6]: with the auto-affix option (SM_ added at export), custom collision stops working, because the collision name no longer matches the affixed mesh name. — [send2ue static-mesh docs](https://github.com/EpicGames/BlenderTools/blob/main/docs/send2ue/asset-types/static-mesh.md); [poly-hammer issue #187](https://github.com/poly-hammer/BlenderTools/issues/187)

#### Sockets
- `SOCKET_[RenderMeshName]_##` placed on Dummy/Helper objects. **Each FBX with sockets may contain only a single render mesh.** The Sockets section of the page still says "Unreal Engine 4". — [Epic: FBX Static Mesh Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine)
- Send to Unreal: "Any child mesh that has the pre fix `SOCKET_`" becomes a socket of its parent, with the prefix stripped (`SOCKET_socket_01` becomes `socket_01`). — [send2ue static-mesh docs](https://github.com/EpicGames/BlenderTools/blob/main/docs/send2ue/asset-types/static-mesh.md)

#### LODs
- LOD meshes must be aligned, occupy the same space and share the base mesh's pivot. Each LOD can use different materials. Requires the "Import LODs" option. — [Epic: FBX Static Mesh Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine)
- Interchange: `lod_group`, `auto_compute_lod_screen_sizes`, `lod_screen_sizes`. "Import Lods" is true and hidden by default. — [Epic Python API 5.7: InterchangeGenericMeshPipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/InterchangeGenericMeshPipeline?application_version=5.7); [Epic: Interchange Import Reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/interchange-import-reference-in-unreal-engine)
- UE Python [UE5.7] `StaticMeshEditorSubsystem`: `import_lod(base_static_mesh, lod_index, source_filename)`, `set_lods(static_mesh, reduction_options)` (uses `StaticMeshReductionOptions`), `set_lod_from_static_mesh(...)`, `set_lod_group(static_mesh, lod_group, rebuild_immediately=True)`, `set_lod_screen_sizes(...)`, `get_lod_count(...)`. — [Epic Python API 5.7: StaticMeshEditorSubsystem](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/StaticMeshEditorSubsystem?application_version=5.7)

#### Pivot, origin and modular grid
- "The pivot is at the origin (0,0,0) on export". Model meshes at the origin, "generally with the origin at one corner of the mesh", to make grid snapping easier. — [Epic: FBX Static Mesh Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine)
- Send to Unreal's "Use object origin" option forces the asset to use the Blender object origin instead of the scene's world origin. — [send2ue settings/export docs](https://github.com/EpicGames/BlenderTools/blob/main/docs/send2ue/settings/export.md)
- UE grid: Edit > Editor Preferences > Viewports > Snap > "User Power of Two Snap Size" switches decimal grid sizes to power-of-2 steps (1, 2, 4, 8, 16...). `Pow2GridSizes` can be customised. UE4+ defaults to decimal steps (1, 5, 10, 50, 100...) (via search summary). — [Epic: Actor Snapping](https://dev.epicgames.com/documentation/en-us/unreal-engine/actor-snapping-in-unreal-engine); [UE forum: Modular Environments and the Grid](https://forums.unrealengine.com/t/modular-environments-and-the-grid/283070)
- Lumen kit rules: "Walls should be no thinner than 10 centimeters (cm) to avoid light leaking". Use closed geometry. Walls, floors and ceilings should be separate meshes, so don't import a whole room as one mesh. Large single meshes get poor distance fields. Lumen places 12 cards per mesh by default (`Max Lumen Mesh Cards`). — [Epic: Lumen Technical Details](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-technical-details-in-unreal-engine)

#### Transforms and triangulation
- Meshes must be triangulated. In order of preference: model triangles, triangulate in the DCC, or let the exporter/importer do it ("automatic triangulation can produce undesirable results"). — [Epic: FBX Static Mesh Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine)
- Blender Triangulate modifier [B5.2]: `keep_custom_normals` (default False; "'Fixed' method usually gives the best result here"), `min_vertices` (4), `ngon_method` (default 'BEAUTY'). — [Blender API: TriangulateModifier](https://docs.blender.org/api/current/bpy.types.TriangulateModifier.html) (read via the bundled reference)

#### Smoothing and normals
- The Blender FBX exporter always writes normals (`write_normals = True  # smooth_type in {'OFF'}`). It writes a `LayerElementSmoothing` layer only for FACE/EDGE/SMOOTH_GROUP. [B5.2] — [export_fbx_bin.py @ v5.2](https://github.com/blender/blender/blob/blender-v5.2-release/scripts/addons_core/io_scene_fbx/export_fbx_bin.py)
- UE warns "No smoothing group information was found in this FBX scene" when the file has no smoothing data. Users report Edge or Face smoothing avoids it. Some say it is a false positive. (Forum, via search summary.) — [UE forum thread](https://forums.unrealengine.com/t/solved-no-smoothing-group-information-was-found-in-this-fbx-scene-really-important/152360)
- Blender 5.x `bpy.ops.object.shade_auto_smooth(use_auto_smooth=True, angle=0.523599)` "Add[s a] modifier to automatically set the sharpness of mesh edges based on the angle". This is the post-4.1 replacement for the old Auto Smooth mesh property. — [Blender API: bpy.ops.object](https://docs.blender.org/api/current/bpy.ops.object.html) (bundled reference)

#### Tangents
- Blender's exact tangents use "the MikkTSpace algorithm", which "match[es] the tangents used in Blender's shading system and exported geometry". [B5.x] — [Blender manual: UV Tangent node](https://docs.blender.org/manual/en/latest/modeling/geometry_nodes/mesh/uv/uv_tangent.html) (bundled manual)
- UE import offers "Use MikkTSpace" (Interchange) and Normal Generation Method MikkTSpace (legacy). — [Epic: Interchange Import Reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/interchange-import-reference-in-unreal-engine); [Epic: FBX Import Options Reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-import-options-reference-in-unreal-engine)
- Nanite does not store per-vertex tangents. Tangent space is derived in the pixel shader. — [Epic: Nanite](https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-virtualized-geometry-in-unreal-engine)

#### UV channels and lightmaps
- Multiple UV sets are supported, and static meshes typically use one set for diffuse. — [Epic: FBX Static Mesh Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine)
- Lumen GI "cannot be used with Static Lighting in lightmaps". — [Epic: Lumen Technical Details](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-technical-details-in-unreal-engine)
- If you don't bake lighting with Lightmass, disable **Generate Lightmap UVs**. With Build Nanite on, it adds import time and an extra, data-heavy UV channel. — [Epic: Nanite](https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-virtualized-geometry-in-unreal-engine)
- Software Lumen requires "Generate Mesh Distance Fields" in Project Settings. — [Epic: Lumen Technical Details](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-technical-details-in-unreal-engine)

#### Texel density
- Maxon's docs: first person "e.g. 10.24" px/cm, third person "e.g. 5.12" px/cm (via search summary). A UE forum answer: 512 px/m for third-person games. — [Maxon C4D: Texel Density](https://help.maxon.net/c4d/en-us/Content/html/Texel_Density.html); [UE forum: Texel density for 2048 texture (3rd person)](https://forums.unrealengine.com/t/textel-density-for-2048-texture-for-3rd-person-game/678150)
- "There is actually no correct set standard". Camera distance matters most (forum/Polycount, via search summary). — [UE forum, same thread](https://forums.unrealengine.com/t/textel-density-for-2048-texture-for-3rd-person-game/678150)

#### Material slots
- Nanite meshes are capped at **64 materials** (sections). Above that, the editor crashes when enabling Nanite or the build fails ("Unsupported number of sections: 74"). This is from community and forum reports. One poster cites the engine constant `MaxSectionArraySize` = 64 in 5.4. — [UE forum: Nanite should validate ≤64 materials](https://forums.unrealengine.com/t/nanite-should-validate-that-mesh-has-64-or-fewer-materials-rather-than-crashing/2430076)

#### Poly-count reference points (vendor guidance only)
- Tripo API guidance: "Game-ready: 50,000–100,000. Web/mobile: 10,000–50,000" faces. Adaptive defaults go up to 1.5M (v3.1). — [Tripo developers: Image to 3D (H series)](https://developers.tripo3d.com/en/docs/generation-image-to-model/standard)
- Rodin Gen-2 presets (third-party API docs): Quad 50k / 18k / 8k / 4k faces (high / medium / low / extra-low). Raw (triangles) 500k / 150k / 20k / 2k. — [Runware: Rodin Gen-2 topology and polygon budget](https://runware.ai/docs/models/hyper3d-rodin-gen-2/guides/topology-and-polygon-budget)
- Meshy remesh default `target_polycount` 30,000 (range 100–300,000). — [Meshy API: Image to 3D](https://docs.meshy.ai/en/api/image-to-3d)

#### Nanite support and limits [UE5.9 docs]
- **Supported**: static and skeletal meshes (skeletal uses animation LODs, one draw call per mesh), Geometry Collections, Instanced Static Mesh, Spline Mesh, Foliage Painter, Landscape Grass. **Opaque and Masked** blend modes. Multiple UVs and vertex colors. Decals projected onto Nanite. Lumen and Virtual Shadow Maps.
- **Not supported or limited**: WPO is "supported, [but] it is limited" (clamp displacement). Morph targets "not supported". Translucent not supported (falls back to a default material with a log warning). Mesh Decals (need translucency). Custom nodes may artifact (no analytic derivatives). Forward rendering, stereo VR, MSAA and Lighting Channels unsupported. Min Screen Radius and distance culling not supported. Ray tracing uses the Fallback Mesh by default (`r.RayTracing.Nanite.Mode 1` is experimental). 16M streamed-in instance cap. DX12 SM6.
- **Recommended for**: many triangles, tiny on-screen triangles, many instances, major occluders, VSM shadow casters.
- **Enable**: "Build Nanite" at import, or Content Browser right-click > Nanite > Enable, or "Enable Nanite Support" in the mesh editor.
- — [Epic: Nanite Virtualized Geometry](https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-virtualized-geometry-in-unreal-engine)
- Interchange: `build_nanite`, `nanite_triangle_threshold` (minimum triangles; 0 = all meshes). Python: `StaticMeshEditorSubsystem.get_nanite_settings(sm)` / `set_nanite_settings(sm, settings, apply_changes=True)`. [UE5.7] — [InterchangeGenericMeshPipeline 5.7](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/InterchangeGenericMeshPipeline?application_version=5.7); [StaticMeshEditorSubsystem 5.7](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/StaticMeshEditorSubsystem?application_version=5.7)
- **RECENT CHANGE [UE5.7]**: Nanite Foliage shipped as **Experimental**, with path-tracing and ray-traced-shadow bugs reported (via search summary). — [UE forum: Plants broken in path tracing 5.7](https://forums.unrealengine.com/t/plants-broken-in-path-tracing-mode-in-ue-5-7/2679041); [Digital Production: UE 5.7 foliage, PCG and in-Editor AI](https://digitalproduction.com/2025/11/12/unreal-engine-5-7-foliage-pcg-and-in-editor-ai/)
- **RECENT CHANGE [UE5.9 Preview]**:
  - New Experimental "Dynamic Wind for Nanite Foliage" plugin.
  - Automatic "Nanite Culling Improvements".
  - "Adaptive Voxel-Based Order Independent Transparency" that works with "both Nanite and non-Nanite translucency". This conflicts with the Nanite page, which still says translucency is unsupported; treat Nanite translucency as unconfirmed for the jam.
  - — [Epic: UE 5.9 Preview Release Notes](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-9-release-notes)
- Nanite skeletal: forum guidance says it works for all skeletal meshes in 5.5+ (enable per mesh). LODing off bones on Nanite skinned meshes was broken in 5.7 and works in 5.8 (via search summary). — [UE forum: Status of skinned meshes and Nanite](https://forums.unrealengine.com/t/status-of-skinned-meshes-and-metahuman-and-nanite-support/2835575)

### Inferences

#### Naming table for Triad Drift (Epic prefixes plus conventional suffixes; the suffixes are not in Epic's doc)
| Thing | Blender object or file name | UE asset |
|---|---|---|
| Hero ship render mesh | `SM_Ship_Hero_Vanguard` (object name = final asset name; no auto-affix) | `SM_Ship_Hero_Vanguard` |
| Its convex collision | `UCX_SM_Ship_Hero_Vanguard_00`, `_01`... (children; non-intersecting; ≤ ~8–16 hulls) | inside the SM |
| Sockets (Empties) | `SOCKET_Muzzle_L`, `SOCKET_Muzzle_R`, `SOCKET_Engine_01`, `SOCKET_Engine_02`, `SOCKET_Cockpit` (children; +X axis = fire/thrust direction) | sockets on the SM |
| LODs (only if not Nanite) | `SM_Prop_Crate_A_LOD0..3` → separate files, or one mesh + UE auto-LOD | `SM_Prop_Crate_A` |
| Kit piece | `SM_Kit_Corridor_Straight_400`, `SM_Kit_Corridor_Corner_200`, `SM_Kit_Wall_Panel_100x400` | same |
| Skeletal | `SK_Crew_Grunt`, skeleton `SKEL_Crew`, physics `PHYS_Crew_Grunt` | same |
| Materials | `M_Ship_Master`, `MI_Ship_Vanguard_Red`, `M_Emissive_Additive`, `MI_Energy_Blade_Cyan` | same |
| Textures | `T_<Asset>_BC` (base colour), `_N` (normal, DirectX), `_ORM` (R=AO, G=Rough, B=Metal), `_E` (emissive), `_M` (mask) | same |

#### Pivots and module sizes
- **Ships, pawns, projectiles**: origin at the centre of mass or bounds centre, nose +X, up +Z.
- **Kit pieces**: origin at a floor corner (or floor-centre of an edge, if rotations must stay on-grid). Dimensions are exact multiples of 100 cm (1 m in Blender): 100/200/400 cm. Walls 20 cm thick (≥10 cm Lumen minimum). Use UE's **decimal** grid at 100 (power-of-two grids, 128/256/512, will not land 100/200/400 modules).
- **Props**: origin at bottom centre, so they drop onto floors.
- **Asteroids**: origin at bounds centre, with random rotation at placement.
- Always place the asset at world origin when exporting (or use the batch script in section 5, which temporarily moves each asset to 0,0,0).

#### Normals, tangents and triangulation recipe (hard-surface)
1. Smooth by Angle 30° (`shade_auto_smooth`). Mark sharp/seams. Optional Weighted Normal modifier (Face Area, Keep Sharp) for hard-surface.
2. Add a Triangulate modifier **last** (quad 'FIXED' or 'BEAUTY', `keep_custom_normals=True`). **Bake normal maps from this triangulated low-poly**, so baker and engine share the same triangulation and MikkTSpace basis.
3. FBX: `mesh_smooth_type='FACE'` (normals are still written; silences the UE warning), `use_triangles=True`, `use_tspace=True`.
4. UE: import normals. For tangents, either import them or let UE recompute MikkTSpace; both should match Blender.

#### Poly budgets (planning numbers; inference, not a published standard)
| Asset class | Render path | LOD0 budget (tris) | Notes |
|---|---|---|---|
| 3 hero ships | Nanite SM for hull. Separate non-Nanite meshes for canopy glass, engine flames, shields | Hull 30k–150k. Glass and FX meshes 0.5–3k | Sockets for weapons and engines. 2–4 material slots |
| Enemy ships | Nanite SM | 8k–40k | Many instances suit Nanite |
| Giant mothership | Nanite, **split into 4–16 sections** | 200k–2M total | Better Lumen distance fields and culling. Same pivot conventions per section |
| Asteroids, rocks | Nanite + ISM/HISM/PCG | 20k–200k per variant, 4–8 variants | Ideal Nanite case (dense, many instances, occluders) |
| Crystals | Opaque emissive goes Nanite. Translucent/refractive goes non-Nanite + LODs | 1k–20k | Emissive Light Source for small emitters (section 3) |
| Alien flora | Masked Nanite (supported), or classic SM + LODs. Avoid experimental Nanite Foliage and heavy WPO wind | 2k–20k | Clamp WPO |
| Corridor kit pieces | Nanite optional | 0.5k–20k | ≥10 cm walls, separate floors and walls |
| Props | Nanite or classic | 1k–15k | |
| Energy blade | Non-Nanite, translucent/additive emissive | 200–2k | Translucency unsupported in Nanite |
| Crew enemies (skeletal) | Classic skeletal with 2–3 LODs (Nanite skeletal possible but newer) | 15k–40k | |

#### Texel density for this game (inference)
- A 2048 map across a ~12 m fighter is about 1.7 px/cm, which suits a fast chase camera. Use trim sheets or tiling detail for close-ups.
- Rough targets:
  - Hero ships 2–3 px/cm (2K sets, or 4K for the three heroes).
  - Enemies 1–2 px/cm (1K–2K).
  - Mothership: tiling or trim materials (the per-asset atlas idea fails at this size).
  - Corridor kit 5.12 px/cm (third-person baseline) via 2K trim sheets.
  - Props 2.5–5 px/cm.

#### Pre-export validator (Blender 5.2; untested)
```python
# validate_for_ue.py — run in Blender 5.2 via MCP execute_blender_code; returns a dict report
import bpy, bmesh, re
PREFIX = re.compile(r"^(SM|SK)_[A-Za-z0-9]+(_[A-Za-z0-9]+)*$")
def validate(scene=bpy.context.scene, max_mats=8):
    rep = {"units": (scene.unit_settings.system, scene.unit_settings.scale_length), "errors": [], "warnings": []}
    if scene.unit_settings.system != 'METRIC' or abs(scene.unit_settings.scale_length - 1.0) > 1e-6:
        rep["warnings"].append("Scene not Metric/1.0 — FBX unit math changes (see notes §1)")
    for ob in scene.objects:
        if ob.type != 'MESH' or ob.name.startswith(("UCX_", "UBX_", "USP_", "UCP_")):
            continue
        if not PREFIX.match(ob.name):
            rep["warnings"].append(f"{ob.name}: missing SM_/SK_ prefix")
        if any(abs(v) > 1e-4 for v in ob.rotation_euler) or any(abs(s - 1) > 1e-4 for s in ob.scale):
            rep["errors"].append(f"{ob.name}: unapplied rotation/scale")
        if len(ob.data.materials) > max_mats:
            rep["warnings"].append(f"{ob.name}: {len(ob.data.materials)} material slots")
        if len(ob.data.uv_layers) == 0:
            rep["errors"].append(f"{ob.name}: no UV map (UV0 needed for textures)")
        for c in ob.children:
            m = re.match(r"^(UCX|UBX|USP|UCP)_(.+?)(_\d+)?$", c.name)
            if m and m.group(2) != ob.name:
                rep["errors"].append(f"{c.name}: collision name does not match render mesh {ob.name}")
            if c.type == 'EMPTY' and not c.name.startswith("SOCKET_"):
                rep["warnings"].append(f"{c.name}: child Empty without SOCKET_ prefix")
        bm = bmesh.new(); bm.from_mesh(ob.data)
        nm = sum(1 for e in bm.edges if not e.is_manifold)
        tris = sum(len(f.verts) - 2 for f in bm.faces)
        bm.free()
        rep.setdefault("meshes", {})[ob.name] = {"tris": tris, "non_manifold_edges": nm, "uv_layers": len(ob.data.uv_layers)}
    return rep
```

### Gaps
- Epic's naming page doesn't define texture suffixes. `_BC/_N/_ORM/_E` above is convention, not Epic doctrine.
- Whether Blender's FBX exporter can emit FBX **LOD Group** nodes that UE's "Import LODs" recognises was not verified. Use separate files with `import_lod`, UE auto-LOD (`set_lods`), or Nanite.
- Interchange FBX support for `SOCKET_` and for multiple custom-collision meshes per file is not documented on the Interchange page. Only the legacy FBX page covers it, so test with the calibration asset.
- No authoritative poly budget for an arcade space shooter was found. The table above is inference.
- The official current Nanite material limit (64) comes from forum posts, not an Epic doc.

---

## 3. Textures (normal-map convention, ORM packing, sRGB and compression, power-of-two, emissive, baking procedural Blender materials)

### Takeaway
Blender bakes OpenGL (Y+) normal maps by default and UE expects DirectX (Y−). Either bake with green set to −Y (`normal_g='NEG_Y'`) or tick "Flip Green Channel" on the UE texture, never both. Pack ORM as R=AO, G=Roughness, B=Metallic, the same channel order glTF uses. Import ORM and normal maps with sRGB off (Masks or Normalmap compression). Base colour and emissive stay sRGB with Default/BC7 compression. Keep power-of-two sizes so textures stream and mip. For glowing sci-fi parts, drive Emissive above 1.0 in a material instance (bloom), and tick "Emissive Light Source" on small emitters for Lumen. Bake procedural Cycles materials with `bpy.ops.object.bake` (Diffuse colour-only, Roughness, Normal, Emit). Metallic has no bake pass, so route it through Emit.

### Cited Findings
- "Blender uses the OpenGL convention by default for rendering and baking, where the Y axis in the green channel points up. The DirectX convention… assumes the Y axis points down." Tangent-space normal images must be Non-Color. [B5.x] — [Blender manual: Normal Map node](https://docs.blender.org/manual/en/latest/render/shader_nodes/displacement/normal_map.html) (bundled manual)
- Bake operator [B5.x]: `bpy.ops.object.bake(type='COMBINED', pass_filter=set(), filepath="", width=512, height=512, margin=16, margin_type='EXTEND', use_selected_to_active=False, max_ray_distance=0.0, cage_extrusion=0.0, cage_object="", normal_space='TANGENT', normal_r='POS_X', normal_g='POS_Y', normal_b='POS_Z', target='IMAGE_TEXTURES', save_mode='INTERNAL', use_clear=False, use_cage=False, use_split_materials=False, use_automatic_name=False, uv_layer="")`. — [Blender API: bpy.ops.object.bake](https://docs.blender.org/api/current/bpy.ops.object.html#bpy.ops.object.bake) (bundled reference)
- Cycles bake types: Combined, Ambient Occlusion, Shadow, Position, Normal, UV, Roughness, Emit, Environment, Diffuse, Glossy, Transmission. There is no Metallic type. Baking lives under the Cycles render engine. — [Blender manual: Render Baking](https://docs.blender.org/manual/en/latest/render/cycles/baking.html) (bundled manual)
- glTF: "glTF stores occlusion in the red (R) channel, allowing it to optionally share the same image with the roughness and metallic channels". The glTF Material Output node carries baked AO. The Normal Map node must stay Tangent Space. Bake normals with the defaults (Tangent, R:+X, G:+Y, B:+Z) for glTF. Emission comes from an Image Texture on Principled Emission. — [Blender manual: glTF 2.0](https://docs.blender.org/manual/en/latest/addons/import_export/scene_gltf2.html) (bundled manual)
- Games tend to use DirectX-style normal maps. glTF specifies OpenGL-style, so Unreal generally needs the green channel flipped for glTF assets. "If you bake normal maps in Blender and import them directly into Unreal Engine 5, the green channel will be wrong" (forum, via search summary). — [UE forum: MaterialX should default to flipped green](https://forums.unrealengine.com/t/materialx-should-default-to-flipped-green-channel-for-normal-maps/2396604); [UE forum: Normal map DirectX... or is it?](https://forums.unrealengine.com/t/normal-map-directx-or-is-it/2664394)
- Interchange Textures: "Detect Normal Map Texture" (adjusts SRGB, Compression Settings and LOD Group automatically), "Flip Normal Map Green Channel", "Allow Non Power of Two". — [Epic: Interchange Import Reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/interchange-import-reference-in-unreal-engine)
- UE Texture editor [UE5.9 docs]:
  - sRGB "should be unchecked if using alpha channels individually as masks".
  - Flip Green Channel inverts green.
  - "BC7 is higher quality than BC1 (AutoDXT)"; "BC7 is a good choice for images that do not encode well in BC1"; "HDR Compressed/BC6H… should be used instead of uncompressed HDR".
  - Non-power-of-two textures can mip, but "power of 2 textures is still required in order for them to texture stream". "The top mip map level must be a multiple of 4 in order to use texture compression". NPoT textures import with NoMipMaps.
  - Max Texture Size 0 = platform max.
  - Virtual textures can be up to 32768².
  - — [Epic: Texture Asset Editor](https://dev.epicgames.com/documentation/en-us/unreal-engine/texture-asset-editor-in-unreal-engine)
- Compression enum includes TC_Default, TC_Normalmap, TC_Masks, TC_Grayscale, TC_Displacementmap, TC_HDR, TC_HDR_Compressed, TC_BC7, TC_Alpha, TC_HalfFloat, TC_LQ. Normalmap = DXT5, BC5 on DX11. "Masks (no sRGB)". (Enum page plus UE4.27/Paper2D docs, via search summary.) — [Epic API: TextureCompressionSettings](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/TextureCompressionSettings); [UE 4.27: Texture Compression Settings](https://docs.unrealengine.com/4.27/RenderingAndGraphics/Textures/TextureCompressionSettings)
- Emissive: values above 1.0 enter HDR and produce bloom. With Lumen, emissive light "propagate[s]… with no additional performance cost", but very small and bright emitters cause noise. Bloom can be lowered in a Post Process Volume while keeping emissive strength (via search summary). — [Epic: Using the Emissive Material Input](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-the-emissive-material-input-in-unreal-engine)
- Lumen culls small objects from the Lumen Scene, so small emissive meshes may light inconsistently. Enable the instance's **Emissive Light Source**. Raise Lumen Scene Detail if small meshes go black in reflections. — [Epic: Lumen Technical Details](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-technical-details-in-unreal-engine)
- Generator PBR outputs:
  - Tripo `pbr` (default true): base_color, metallic, roughness, normal.
  - Meshy `enable_pbr` (default false): metallic, normal, roughness. Emission map only with Meshy-6 + PBR at 2k/4k.
  - Rodin `PBR` (default): base colour, metallic, roughness, normal. `Shaded` bakes lighting.
  - — [Tripo API](https://developers.tripo3d.com/en/docs/generation-image-to-model/standard); [Meshy API](https://docs.meshy.ai/en/api/image-to-3d); [Runware Rodin Gen-2](https://runware.ai/docs/models/hyper3d-rodin-gen-2/guides/topology-and-polygon-budget)

### Inferences

#### Texture settings table (UE 5.7+, Lumen, Windows/DX12)
| Map | Blender bake/save | UE sRGB | UE Compression | Other |
|---|---|---|---|---|
| `T_X_BC` base colour (+opacity in A if masked) | Diffuse, `pass_filter={'COLOR'}`, sRGB PNG | On | Default (BC1/BC3) or BC7 | No lighting in albedo |
| `T_X_N` normal | Normal, Tangent, `normal_g='NEG_Y'` (DirectX), Non-Color PNG 8/16-bit | Off | Normalmap (BC5) | If baked OpenGL, set Flip Green Channel instead. LOD group WorldNormalMap |
| `T_X_ORM` | Pack R=AO, G=Roughness, B=Metallic, Non-Color | Off | Masks (or BC7 for higher quality) | Material reads .r/.g/.b |
| `T_X_E` emissive | Emit pass, sRGB | On | Default/BC7 | Multiply by a scalar `EmissiveStrength` param (e.g. 5–50) in MI |
| Sizes | 512 / 1024 / 2048 / 4096 only | | | Hero 2K–4K, enemies 1K–2K, props 512–1K, kit trims 2K |

#### Bright sci-fi emissive recipe (inference)
- Master `M_Ship_Master` with `EmissiveColor = T_E.rgb * EmissiveTint * EmissiveStrength`. Use `EmissiveStrength` around 5–20 for panel lights and 20–100 for engine cores.
- Tune bloom in the PPV rather than lowering the emissive values.
- Untick auto-exposure surprises: set PPV Exposure min/max equal for a consistent bright arcade look.
- For engines and blades, use a separate unlit/additive translucent mesh (non-Nanite) plus Niagara.

#### Baking a procedural Blender material into UE textures (Blender 5.2; untested)
```python
# bake_to_ue.py — Cycles bake from a procedural/high-poly source onto the UV'd low-poly 'low'.
import bpy, os
def target_image(low, name, size, non_color):
    img = bpy.data.images.new(name, size, size, alpha=False)
    if non_color: img.colorspace_settings.name = 'Non-Color'
    for slot in low.material_slots:                      # every slot needs the target active
        nt = slot.material.node_tree
        n = nt.nodes.get("BAKE_TARGET") or nt.nodes.new('ShaderNodeTexImage')
        n.name = "BAKE_TARGET"; n.image = img; nt.nodes.active = n
    return img

def bake_set(high, low, out_dir, asset, size=2048, cage=0.02, same_object=False):
    scn = bpy.context.scene; scn.render.engine = 'CYCLES'; scn.cycles.samples = 16
    bpy.ops.object.select_all(action='DESELECT')
    low.select_set(True); bpy.context.view_layer.objects.active = low
    if not same_object: high.select_set(True)
    kw = dict(use_selected_to_active=not same_object, cage_extrusion=cage, margin=16, margin_type='EXTEND')
    jobs = [("BC", dict(type='DIFFUSE', pass_filter={'COLOR'}), False),
            ("N",  dict(type='NORMAL', normal_space='TANGENT', normal_r='POS_X', normal_g='NEG_Y', normal_b='POS_Z'), True),
            ("R",  dict(type='ROUGHNESS'), True),
            ("E",  dict(type='EMIT'), False)]
    out = {}
    for tag, args, nc in jobs:
        img = target_image(low, f"T_{asset}_{tag}", size, nc)
        bpy.ops.object.bake(**args, **kw)
        img.filepath_raw = os.path.join(out_dir, f"{img.name}.png"); img.file_format = 'PNG'; img.save()
        out[tag] = img
    return out
# Metallic: Blender has no METALLIC bake type. Temporarily link each source Principled BSDF 'Metallic'
# input (or its value) into 'Emission Color' with 'Emission Strength' = 1, bake type='EMIT' into a
# Non-Color image, then restore the materials. Same trick works for AO→emit if not using the AO pass.
```
```python
# pack_orm.py — numpy channel packer (Blender 5.2)
import bpy, numpy as np
def pack_orm(ao, rough, metal, name, path):
    w, h = rough.size
    def r(img):
        a = np.empty(w * h * 4, dtype=np.float32); img.pixels.foreach_get(a); return a.reshape(-1, 4)[:, 0]
    px = np.ones((w * h, 4), dtype=np.float32)
    px[:, 0] = r(ao) if ao else 1.0; px[:, 1] = r(rough); px[:, 2] = r(metal) if metal else 0.0
    img = bpy.data.images.new(name, w, h, alpha=False); img.colorspace_settings.name = 'Non-Color'
    img.pixels.foreach_set(px.ravel()); img.filepath_raw = path; img.file_format = 'PNG'; img.save()
    return img
```

#### UE texture fix-up after import (UE 5.7+ Python; property names to verify with `dir(tex)`)
```python
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
def fix_texture(path, kind, normal_is_opengl=False):
    t = eas.load_asset(path)
    if kind == "N":
        t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        t.set_editor_property("srgb", False)
        t.set_editor_property("flip_green_channel", bool(normal_is_opengl))
    elif kind in ("ORM", "M"):
        t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        t.set_editor_property("srgb", False)
    else:  # BC, E
        t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
        t.set_editor_property("srgb", True)
    eas.save_loaded_asset(t)
```

### Gaps
- No current (UE5) Epic page with a full compression-setting/format table was fetched. The BC5 and Masks mapping comes from older or adjacent docs via search summary.
- Whether Interchange's **glTF** materials auto-correct OpenGL normal maps is undocumented. Test a GLB with an obvious bevel.
- No Epic source states UE's normal convention verbatim. The DirectX claim rests on forum consensus plus the existence of the Flip Green options.

---

## 4. AI-generated mesh cleanup (defects, scripted Blender recipe, generator export options, license terms)

### Takeaway
Expect dense triangle soup (hundreds of thousands to millions of faces), unitless or normalised scale, arbitrary forward axis, floaters and internal or loose parts, non-manifold patches, box-style AI UVs, and sometimes lighting baked into albedo. Fix as much as possible **at generation time**: Rodin `Quad` + `PBR` + `polyCount`, Tripo `face_limit` / `smart_low_poly` / `pbr` / `delight` / `auto_size` / `export_orientation`, Meshy `topology` / `target_polycount` / `enable_pbr` / `remove_lighting` / `auto_size`. Then run a scripted Blender pass: import, join and unparent, apply transforms, rescale and re-orient, merge by distance, remove floaters, recalc normals, reduce (Decimate keeps UVs and textures; Voxel then QuadriFlow gives clean topology but needs re-UV and a rebake), Smart UV, bake high to low, triangulate, export. Licensing:
- **Meshy**: Free is CC BY 4.0 with attribution; paid is private.
- **Rodin**: rights depend on the plan.
- **Tripo**: the free tier's terms are self-contradictory (CC BY 4.0 vs "Tripo retains rights"); use a paid plan or avoid shipping free outputs.
- **Hunyuan3D 2.1** (open weights): its license **does not apply in the EU, UK or South Korea**, and outputs may not be used outside the Territory.

### Cited Findings

#### Typical defects (evidence per source)
- AI models are "far too dense and mesh heavy" for real-time. AI UVs are as dense as the mesh and unwrapped "box"/"lightmap unpack" style. Remapping UVs after decimation breaks texture alignment, so remesh or retopologise and re-texture for game assets (KatsBits, 20 Dec 2025). — [KatsBits: Optimising AI Generated 3D Models](https://www.katsbits.com/codex/?p=9993)
- AI meshes often arrive at 1–2M faces. Decimate keeps the messy topology. QuadriFlow "needs a closed, manifold surface". Suggested repair sequence: Merge by Distance, Degenerate Dissolve, Delete Loose, 3D-Print-Toolbox non-manifold checks (vendor blogs selling cleanup tools, via search summary). — [StraySpark: Blender to game engine pipeline 2026](https://www.strayspark.studio/blog/blender-to-game-engine-pipeline-2026); [StraySpark: MeshPrep](https://www.strayspark.studio/blog/introducing-meshprep-blender-ai-scan-mesh-cleanup); [Neural4D cleanup guide](https://blog.neural4d.com/user-guide/ai-3d-model-blender-cleanup-complete-mesh-fix-guide-2026/)
- Hunyuan3D-2.1's own post-processing shows the expected defects:
  - `FaceReducer` (default `max_facenum=40000`; pymeshlab quadric edge collapse with `preserveboundary/preservenormal/preservetopology=True`).
  - `FloaterRemover` (deletes disconnected components below `nbfaceratio=0.005` of faces).
  - `DegenerateFaceRemover`.
  - `mesh_normalize`, which rescales vertices so the bounding sphere diameter is 1.2 units: unitless scale.
  - — [Hunyuan3D-2.1 postprocessors.py](https://github.com/Tencent-Hunyuan/Hunyuan3D-2.1/blob/main/hy3dshape/hy3dshape/postprocessors.py)
- Lighting baked into albedo:
  - Rodin `Shaded` material "bakes lighting into the texture". `PBR` is the relightable option.
  - Tripo `delight` (default true) "removes baked-in lighting" but is read only by texture model `v3.5-20260815`.
  - Meshy `remove_lighting` (default true) "only works with the Meshy 6 texture model".
  - — [Runware Rodin Gen-2](https://runware.ai/docs/models/hyper3d-rodin-gen-2/guides/topology-and-polygon-budget); [Tripo API](https://developers.tripo3d.com/en/docs/generation-image-to-model/standard); [Meshy API](https://docs.meshy.ai/en/api/image-to-3d)
- Scale and orientation:
  - Tripo `auto_size` (default false) scales to real-world metres. `export_orientation` defaults to `+x` (also -x, -y, +y), and "a wrong orientation can still report `status: success`".
  - Meshy `auto_size` (default false) with `origin_at` `bottom` (default) or `center`.
  - Rodin `boundingBox` caps dimensions, given as `[Y, Z, X]`.
  - — [Tripo API](https://developers.tripo3d.com/en/docs/generation-image-to-model/standard); [Meshy API](https://docs.meshy.ai/en/api/image-to-3d); [Runware Rodin Gen-2](https://runware.ai/docs/models/hyper3d-rodin-gen-2/guides/topology-and-polygon-budget)

#### Generator export and topology options (Nov 2026)
- **Hyper3D Rodin (Gen-2; Gen-2.5 also exists on resellers)**:
  - `meshMode` Quad (default; quad-dominant, for rigging) or Raw (dense triangles, for static/retopo).
  - Presets: Quad 50k/18k/8k/4k; Raw 500k/150k/20k/2k.
  - Custom `polyCount`: Quad 1k–200k, Raw 500–1M. Mutually exclusive with `quality`.
  - `HighPack` (Quad only): about 16x faces and 4K textures (default 2K), roughly 3x cost.
  - `material` PBR / Shaded / All. `taPose` for humanoids.
  - Source is a third-party API host's docs; the official developer.hyper3d.ai reference was not accessed. Replicate lists `geometry_file_format` default `glb` (via search summary).
  - — [Runware Rodin Gen-2 guide](https://runware.ai/docs/models/hyper3d-rodin-gen-2/guides/topology-and-polygon-budget); [Replicate hyper3d/rodin API](https://replicate.com/hyper3d/rodin/versions/9492be065b4a8b671ea929e63f8411ebcfa245e9af641400035e7ece20e1ba28/api)
- **Tripo (H series API)**:
  - Models `v3.1-20260211` (recommended), `v3.0-20250812`, `v2.5-20250123`. Texture `v3.5-20260815` is the newest.
  - `face_limit`: adaptive if omitted. Triangle 1.5M standard / 2M ultra on v3.1. Quad 150k.
  - `quad` (default false): forces FBX, 10k faces if unset. `smart_low_poly`: triangle 500–20k, quad 500–10k; may fail on complex models.
  - `pbr` (default true). `texture_quality` fast / standard / detailed / extreme (8K). `geometry_quality` standard / detailed.
  - `generate_parts` (needs texture and pbr off). `export_uv` (default true).
  - Convert endpoint: formats GLTF / FBX / USDZ / OBJ / STL / 3MF; `fbx_preset` `blender` (default) / `3dsmax` / `mixamo` / `bake_scale`; `pivot_to_center_bottom`; `scale_factor`; `texture_size` 4096; `texture_format` JPEG (default) or PNG and others; `pack_uv`; `bake` (default true); `flatten_bottom`; `force_symmetry` (quad only); `export_orientation`.
  - — [Tripo: Image to 3D](https://developers.tripo3d.com/en/docs/generation-image-to-model/standard); [Tripo: Model Format Conversion](https://developers.tripo3d.com/en/docs/models-convert)
- **Meshy (API)**:
  - `ai_model` default `latest` = Meshy 7.1 (`meshy-7` deprecated, retiring 31 Dec 2026). `model_type` `standard` or `smart-topology` (`meshy-t2` / `meshy-t2.5`, triangles only).
  - `topology` triangle (default) or quad. `target_polycount` 100–300,000 (default 30,000); t2 100–15k (default 4k); t2.5 100–30k (default 15k).
  - `should_remesh` (default false except meshy-6-lite). `enable_pbr` (default false). `texture_resolution` 2k (default) / 4k / 8k.
  - `target_formats` glb / obj / fbx / stl / usdz / 3mf. `pose_mode` a-pose / t-pose.
  - `save_pre_remeshed_model` returns the dense original GLB, which is useful as a bake source.
  - — [Meshy API: Image to 3D](https://docs.meshy.ai/en/api/image-to-3d)
- **Hunyuan3D 2.1 (open source, self-hosted)**:
  - PBR texture synthesis.
  - VRAM: 10 GB for shape, 21 GB for texture, 29 GB for both.
  - Python pipeline `Hunyuan3DDiTFlowMatchingPipeline` + `Hunyuan3DPaintPipeline`.
  - Newer Tencent repos (Omni, Part, WorldClaw) were active in 2025–26.
  - — [Hunyuan3D-2.1 README](https://github.com/Tencent-Hunyuan/Hunyuan3D-2.1)

#### License terms (not legal advice)
- **Hyper3D Rodin (Deemos Corp.)**:
  - Users may "export such Content for private or commercial use depending on your subscription plan".
  - For Rodin, Deemos "will not limit your use of such Output" (§5(b)) and makes no warranty on copyrightability.
  - Deemos may use Prompts and Output to "provide, maintain and improve our services, and conduct and support research".
  - Prohibited: using the services "to train, improve, enhance, or otherwise develop any AI models", and "sell, resell or commercially use the Content, except as expressly permitted".
  - Gifted free credits expire at the end of the subscription period. A "Free 7-Day Trial" is advertised.
  - No last-updated date was visible.
  - — [Hyper3D Terms](https://hyper3d.ai/legal/terms)
- **Meshy**: Free is "CC BY 4.0", "Yes, with attribution", "You own the output". Pro and above are "Private" with "full commercial use". Models generated after a downgrade fall under free-plan terms (via search summary). — [Meshy pricing docs](https://docs.meshy.ai/en/webapp/pricing); [Meshy help: commercial use](https://intercom.help/meshy/en/articles/16102098-can-i-use-meshy-assets-commercially)
- **Tripo** (conflicting sources; Tripo's site returned 403 to fetches):
  - A Tripo help page says "under Section 5.2.1 of the Terms, Tripo retains all rights to Inputs and Outputs generated by Free Users" and that commercial use needs a paid plan.
  - A Tripo blog says the free tier is CC BY 4.0, allowing commercial use with attribution.
  - Paid plans include commercial rights.
  - (via search summary) — [Tripo help: How to use Tripo models commercially](https://www.tripo3d.ai/help/privacy-policy/how-to-use-tripo-models-commercially); contradicted by [Tripo blog: commercial use of AI 3D models](https://www.tripo3d.ai/blog/commercial-use-ai-3d-models); [Tripo help: Can I use models commercially?](https://www.tripo3d.ai/help/privacy-policy/can-i-use-models-commercially)
- **Tencent Hunyuan 3D 2.1 Community License** (released 13 Jun 2025):
  - "THIS LICENSE AGREEMENT DOES NOT APPLY IN THE EUROPEAN UNION, UNITED KINGDOM AND SOUTH KOREA". The Territory is worldwide minus those.
  - "You must not use… the Tencent Hunyuan 3D 2.1 Works, Output or results… outside the Territory."
  - Licensees with more than 1M MAU must request a licence.
  - "Tencent claims no rights in Outputs You generate."
  - An Acceptable Use Policy applies.
  - — [Hunyuan3D-2.1 LICENSE](https://github.com/Tencent-Hunyuan/Hunyuan3D-2.1/blob/main/LICENSE)

#### Blender operators used by the recipe [B5.x bundled API reference]
- `bpy.ops.mesh.remove_doubles(threshold=0.0001, use_centroid=True, use_unselected=False, use_sharp_edge_from_normals=False)` (Merge by Distance). `bpy.ops.mesh.fill_holes(sides=4)` ("zero fills all holes"). — [Blender API: bpy.ops.mesh](https://docs.blender.org/api/current/bpy.ops.mesh.html)
- `bpy.ops.object.quadriflow_remesh(use_mesh_symmetry=True, use_preserve_sharp=False, use_preserve_boundary=False, preserve_attributes=False, smooth_normals=False, mode='FACES', target_ratio=1.0, target_edge_length=0.1, target_faces=4000, mesh_area=-1.0, seed=0)`. "All data layers will be lost", and target_faces counts quads. — [Blender API: bpy.ops.object](https://docs.blender.org/api/current/bpy.ops.object.html)
- `bpy.ops.uv.smart_project(angle_limit=1.15192, margin_method='SCALED', rotate_method='AXIS_ALIGNED_Y', island_margin=0.0, area_weight=0.0, correct_aspect=True, scale_to_bounds=False)`. — [Blender API: bpy.ops.uv](https://docs.blender.org/api/current/bpy.ops.uv.html)
- `bpy.ops.object.shade_auto_smooth(use_auto_smooth=True, angle=0.523599)` and `bpy.ops.object.bake(...)` as in section 3. — [Blender API: bpy.ops.object](https://docs.blender.org/api/current/bpy.ops.object.html)

### Inferences

#### Generation-time settings that save cleanup (inference)
| Asset | Generator settings |
|---|---|
| Hero and enemy ships (hard-surface) | Rodin **Quad**, **PBR**, `polyCount` 30–60k (or HighPack for one hero bake source). Generate side and top concept images with bold silhouettes. Treat the output as a **blockout or bake source**; hand-fix sockets, panels and emissive masks |
| Asteroids, rocks | Rodin Raw high or Tripo detailed. Decimate in Blender to 50–150k and enable Nanite. No retopo needed |
| Crystals | Any generator. Voxel remesh, low-poly 1–5k, bake. Emissive or translucency is authored in UE, not generated |
| Alien flora | Tripo/Meshy with `pbr` on. Use masked cards for thin leaves; AI solid leaves are heavy |
| Props | Meshy `topology=quad`, `target_polycount` 5–15k, `enable_pbr=true`, `auto_size=true`, `target_formats=["fbx","glb"]`; or Tripo `face_limit` 10–20k, `auto_size=true`, `export_orientation='+x'` |
| Crew humanoids | Rodin `taPose`, Meshy `pose_mode` (A/T), or Tripo `fbx_preset='mixamo'`. Rig/retarget outside Blender if possible |

#### Decimate vs QuadriFlow vs Voxel remesh (inference)
| Method | Keeps UVs and textures? | Topology | Use when |
|---|---|---|---|
| Decimate (Collapse) | Mostly yes; a quick path with no rebake | Messy triangles | Rocks, asteroids, background props, Nanite assets |
| Voxel Remesh (Remesh modifier, VOXEL) | No | Uniform, watertight, closes holes and merges intersecting parts | Before QuadriFlow, or for crystals and organic blobs. Rebake required |
| QuadriFlow | No (data layers lost) | Clean quads | Deformable or hero pieces where edge flow matters. Needs manifold input (run voxel first). Rebake required |

#### Scripted cleanup recipe (Blender 5.2 template; untested; verify names against the 5.2 API)
```python
"""
ai_mesh_cleanup.py — Blender 5.2.x. Run via Blender Lab MCP (execute_blender_code / execute_blender_code_for_cli) or:
  blender -b --factory-startup --python ai_mesh_cleanup.py -- <in.glb|fbx|obj> <out_dir> <SM_Name> <size_m> <target_tris> <decimate|voxel_quadriflow|voxel> [rot_z_deg] [pivot]
"""
import bpy, bmesh, math, os, sys
from mathutils import Vector, Matrix

a = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
SRC, OUT, NAME = a[0], a[1], a[2]
SIZE_M, TARGET_TRIS = float(a[3]), int(a[4])
MODE = a[5] if len(a) > 5 else "decimate"
ROT_Z = float(a[6]) if len(a) > 6 else 0.0          # degrees, so the asset's front ends up on +X
PIVOT = a[7] if len(a) > 7 else "bottom_center"     # or "center"
MERGE_DIST = 0.0005                                 # m, applied after rescale
FLOATER_RATIO = 0.002                               # Hunyuan's own filter uses 0.005

def sel(o):
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active = o

def apply_mod(o, m):
    with bpy.context.temp_override(object=o, active_object=o, selected_objects=[o]):
        bpy.ops.object.modifier_apply(modifier=m.name)

def tris(o): return sum(len(p.vertices) - 2 for p in o.data.polygons)

# 1) import, unparent, join
scn = bpy.context.scene
scn.unit_settings.system = 'METRIC'; scn.unit_settings.scale_length = 1.0
before = set(bpy.data.objects)
ext = os.path.splitext(SRC)[1].lower()
if ext in (".glb", ".gltf"): bpy.ops.import_scene.gltf(filepath=SRC)
elif ext == ".fbx":          bpy.ops.import_scene.fbx(filepath=SRC)
elif ext == ".obj":          bpy.ops.wm.obj_import(filepath=SRC)
new = [o for o in bpy.data.objects if o not in before]
meshes = [o for o in new if o.type == 'MESH']
for o in meshes:
    sel(o); bpy.ops.object.parent_clear(type='CLEAR_KEEP_TRANSFORM')
for o in new:
    if o.type == 'EMPTY': bpy.data.objects.remove(o, do_unlink=True)
bpy.ops.object.select_all(action='DESELECT')
for o in meshes: o.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
if len(meshes) > 1: bpy.ops.object.join()
obj = bpy.context.view_layer.objects.active; obj.name = NAME; obj.data.name = NAME
report = {"source_tris": tris(obj), "parts_joined": len(meshes)}

# 2) orient, scale to real size, pivot
sel(obj)
obj.rotation_euler.z += math.radians(ROT_Z)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
longest = max(obj.dimensions) or 1.0
s = SIZE_M / longest; obj.scale = (s, s, s)
bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
bb = [Vector(c) for c in obj.bound_box]
mn = Vector((min(v.x for v in bb), min(v.y for v in bb), min(v.z for v in bb)))
mx = Vector((max(v.x for v in bb), max(v.y for v in bb), max(v.z for v in bb)))
c = (mn + mx) / 2
anchor = Vector((c.x, c.y, mn.z)) if PIVOT == "bottom_center" else c
obj.data.transform(Matrix.Translation(-anchor)); obj.location = (0, 0, 0)

# 3) repair: merge by distance, degenerate dissolve, loose, floaters, normals
bm = bmesh.new(); bm.from_mesh(obj.data)
bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=MERGE_DIST)
bmesh.ops.dissolve_degenerate(bm, dist=MERGE_DIST * 0.5, edges=bm.edges)
bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
bm.faces.ensure_lookup_table(); bm.faces.index_update()
seen, islands = set(), []
for f in bm.faces:
    if f.index in seen: continue
    stack, isl = [f], []; seen.add(f.index)
    while stack:
        cur = stack.pop(); isl.append(cur)
        for e in cur.edges:
            for nf in e.link_faces:
                if nf.index not in seen: seen.add(nf.index); stack.append(nf)
    islands.append(isl)
small = [f for isl in islands if len(isl) < FLOATER_RATIO * max(len(bm.faces), 1) for f in isl]
bmesh.ops.delete(bm, geom=small, context='FACES')
bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
report.update(islands=len(islands), floater_faces_removed=len(small),
              non_manifold_edges=sum(1 for e in bm.edges if not e.is_manifold))
bm.to_mesh(obj.data); bm.free(); obj.data.update()

# keep a high-poly copy as bake source
high = obj.copy(); high.data = obj.data.copy(); high.name = NAME + "_HIGH"
scn.collection.objects.link(high); high.hide_render = False

# 4) reduce
if MODE == "decimate":
    m = obj.modifiers.new("Decimate", 'DECIMATE'); m.decimate_type = 'COLLAPSE'
    m.ratio = min(1.0, TARGET_TRIS / max(tris(obj), 1)); m.use_collapse_triangulate = True
    apply_mod(obj, m)
else:
    m = obj.modifiers.new("Remesh", 'REMESH'); m.mode = 'VOXEL'
    m.voxel_size = SIZE_M / 200.0; m.adaptivity = 0.0          # ~6 cm on a 12 m ship
    apply_mod(obj, m)
    if MODE == "voxel_quadriflow":
        sel(obj)
        bpy.ops.object.quadriflow_remesh(mode='FACES', target_faces=max(TARGET_TRIS // 2, 100),
                                         use_mesh_symmetry=False, use_preserve_sharp=True,
                                         use_preserve_boundary=False, smooth_normals=False, seed=0)
    else:
        m = obj.modifiers.new("Decimate", 'DECIMATE'); m.decimate_type = 'COLLAPSE'
        m.ratio = min(1.0, TARGET_TRIS / max(tris(obj), 1)); apply_mod(obj, m)

# 5) UVs (always for remesh paths; optional for decimate)
if MODE != "decimate":
    sel(obj); bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(66.0), margin_method='SCALED', island_margin=0.003)
    bpy.ops.object.mode_set(mode='OBJECT')

# 6) shading + triangulate (bake AFTER this so tangents match the exported triangulation)
sel(obj)
bpy.ops.object.shade_auto_smooth(angle=math.radians(30.0))
t = obj.modifiers.new("Triangulate", 'TRIANGULATE'); t.quad_method = 'FIXED'; t.keep_custom_normals = True
apply_mod(obj, t)

# 7) rebake (remesh paths): fresh single material on low, then bake_set(high, obj, OUT, NAME) from section 3
# 8) optional LODs (skip for Nanite): copies with Decimate ratio 0.5/0.25/0.125 → NAME_LOD1..3, same pivot
# 9) delete NAME_HIGH, run validate_for_ue (section 2), export with the section 5 batch exporter
report["final_tris"] = tris(obj)
print(report)
```

#### Provenance log for jam disclosure and licensing (inference)
- For every generated asset, keep a row in `asset_provenance.csv` with:
  - generator and model version (e.g. Rodin Gen-2 Quad; Tripo v3.1; Meshy 7.1)
  - plan tier at generation time
  - date
  - input (prompt or image source)
  - licence (CC BY 4.0 attribution text / private)
  - post-processing done
  - final UE asset path
- This covers MegaJam's "list third-party assets and AI tools" rule and Rodin partner-prize eligibility. It also flags Meshy-free assets that need attribution.
- Avoid Hunyuan3D 2.1 outputs in anything distributed to EU/UK/KR players, given the license's Territory clause.

### Gaps
- The Tripo free-tier terms conflict (help page vs blog), and the Tripo ToS page itself could not be fetched (403).
- Rodin's plan-by-plan commercial rights are not spelled out on the terms page. The official Rodin API reference (developer.hyper3d.ai) was not reached, so parameter names come from third-party hosts (Runware, Replicate).
- Terms for Tencent's **hosted** Hunyuan3D 3.x service (as opposed to the 2.1 open weights) were not found.
- No neutral, non-vendor benchmark of decimate vs QuadriFlow vs voxel on AI meshes was found. The trade-off table is inference.

---

## 5. Automation (batch export, "Send to Unreal" status in 2026, UE Python/Interchange import, Unreal MCP servers)

### Takeaway
For Blender 5.2.2, don't depend on Send to Unreal. Epic abandoned it (last Epic release 2.4.3, Nov 2023). The community fork (poly-hammer 2.6.7, Nov 2025) is tested only on Blender 3.6/4.2 with UE 5.3/5.4, has open Blender 5.x blockers, and needs the legacy FBX importer. Instead, run a small bpy batch exporter (one FBX per asset, asset moved to the origin, settings from section 1). Then import with UE Python via Interchange (`InterchangeManager.import_asset` with a saved, pre-configured pipeline asset), or via UE 5.9's built-in Unreal MCP `ImportFile` tool (experimental). Follow with `StaticMeshEditorSubsystem` fix-ups (Nanite, collision, LOD group) and texture fix-ups. The flopperam MCP's `python_execution` can run those UE Python scripts. chongdashu's unreal-mcp has no import tool and was last updated in April 2025.

### Cited Findings

#### Send to Unreal status
- The Epic repo now resolves to `EpicGames/BlenderTools` (MIT). The last Epic release is "Send to Unreal 2.4.3" (2023-11-09). The only commit since mid-2024 is a 2026-09-03 docs-URL update. `bl_info` requires `"blender": (3, 3, 0)`. — [EpicGames/BlenderTools](https://github.com/EpicGames/BlenderTools); [releases](https://github.com/EpicGames/BlenderTools/releases); [send2ue __init__.py](https://github.com/EpicGames/BlenderTools/blob/main/send2ue/__init__.py)
- Open issues: "THIS PROJECT IS ABANDONED BY EPIC. NOW MAINTAINED ON THE POLYHAMMER FORK" (#757, Oct 2024) and ">>> Please Read >>> Epic Has Abandoned this Project" (#737, Apr 2024). An older issue says Send to Unreal was broken in Blender 4.0 beta (#676). — [Issue #757](https://github.com/EpicGames/BlenderTools/issues/757); [Issue #737](https://github.com/EpicGames/BlenderTools/issues/737); [Issue #676](https://github.com/EpicGames/BlenderTools/issues/676)
- Fork `poly-hammer/BlenderTools`, latest "Send to Unreal 2.6.7" (2025-11-23):
  - Fixes preference setters/getters for Blender 5.0.
  - "Tests Passing On: Blender 3.6, 4.2… Unreal 5.3, 5.4".
  - 2.6.4 fixed Blender 4.4 operator compatibility. 2.6.3 added a check telling UE 5.5 users to switch to the legacy FBX importer.
  - — [poly-hammer/BlenderTools releases](https://github.com/poly-hammer/BlenderTools/releases)
- **RECENT** open blockers on the fork:
  - #193 (May 2026, Blender 5.1.1 / Python 3.13): `AttributeError: 'Action' object has no attribute 'fcurves'`; `SyntaxError: expected 'else' after 'if' expression` on any remote Unreal call.
  - #179: animation export broken since 5.0. A tested patch was posted in Oct 2026 but not released.
  - #187: custom collision fails on Blender 5.0.1 with affixes.
  - Unofficial patched zips are circulating in issue comments; they are unreviewed and should not be installed.
  - — [Issue #193](https://github.com/poly-hammer/BlenderTools/issues/193); [Issue #179](https://github.com/poly-hammer/BlenderTools/issues/179); [Issue #187](https://github.com/poly-hammer/BlenderTools/issues/187)

#### UE Python import (Interchange and legacy) [UE5.7 API]
- `unreal.AssetImportTask` properties: `async_`, `automated`, `destination_name` ("if you are using interchange the name must be set in a pipeline"), `destination_path`, `factory`, `filename`, `imported_object_paths`, `options`, `replace_existing`, `replace_existing_settings`, `save`. `result` is deprecated. — [Epic Python 5.7: AssetImportTask](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/AssetImportTask?application_version=5.7)
- `unreal.ImportAssetParameters`: `destination_name`, `follow_redirectors`, `force_show_dialog`, `import_level`, `is_automated`, callbacks (`on_asset_done`, `on_assets_import_done`, `on_scene_import_done`, `on_scene_object_done`), **`override_pipelines: Array[SoftObjectPath]`**, `reimport_asset`, `reimport_source_index`, `replace_existing`. — [Epic Python 5.7: ImportAssetParameters](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/ImportAssetParameters?application_version=5.7)
- `unreal.InterchangeGenericAssetsPipeline` properties: `mesh_pipeline`, `material_pipeline`, `animation_pipeline`, `groom_pipeline` (all read-only sub-objects); `common_meshes_properties`, `common_skeletal_meshes_and_animations_properties`; `import_offset_translation` / `_rotation` / `_uniform_scale`; `asset_name`, `use_source_name_for_asset`, `asset_type_sub_folders`, `scene_name_sub_folder`, `reimport_strategy`. — [Epic Python 5.7: InterchangeGenericAssetsPipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/InterchangeGenericAssetsPipeline?application_version=5.7)
- Epic staff example (UE 5.1 era):
  - `unreal.InterchangeManager.create_source_data(path)`, then `ImportAssetParameters()` with `is_automated=True`.
  - Append `/Interchange/Pipelines/DefaultAssetsPipeline` (or for scenes `DefaultSceneAssetsPipeline` + `DefaultSceneLevelPipeline`) to `override_pipelines`.
  - Call `InterchangeManager.get_interchange_manager_scripted().import_asset(dest, source, params)` or `.import_scene(...)`.
  - Edit a pipeline asset with `pipeline.get_editor_property("mesh_pipeline").set_editor_property("combine_static_meshes", True)` and `save_loaded_asset`.
  - Limitations noted: custom Blueprint pipelines couldn't be added to `override_pipelines` (UE-171404). In 5.1 `AssetImportTask` used Interchange for OBJ, textures and glTF but not FBX, and `FbxImportUI` was needed for legacy FBX options.
  - — [UE forum: Python example for Interchange import](https://forums.unrealengine.com/t/python-or-blueprint-example-for-importing-obj-file-in-ue5-1-interchange-import-pipeline/714987)
- `StaticMeshEditorSubsystem` (5.7) post-import methods: `set_nanite_settings`, `get_nanite_settings`, `add_simple_collisions(static_mesh, shape_type)`, `set_convex_decomposition_collisions(static_mesh, hull_count, max_hull_verts, hull_precision)`, `remove_collisions`, `set_generate_lightmap_uv(static_mesh, generate_lightmap_u_vs)`, `set_lods`, `import_lod`, `set_lod_group`. There are no socket methods on this class. — [Epic Python 5.7: StaticMeshEditorSubsystem](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/StaticMeshEditorSubsystem?application_version=5.7)

#### Unreal MCP options
- **UE 5.9 built-in Unreal MCP (Experimental)**:
  - Runs inside the editor process. Needs the Unreal MCP plugin (on by default) plus **All Toolsets** (or individual toolset plugins); Toolset Registry is auto-enabled.
  - **Import tools: `ImportFile`** for Sound Waves, Textures, **Static Meshes, Skeletal Meshes**, Data Tables, Curve Tables and String Tables. Takes `bReplaceExisting` and `bSave`, both default **false**.
  - Discovery meta-tools: `list_toolsets`, `describe_toolsets`, `describe_tools`, `call_tool`.
  - Transport: Streamable HTTP/SSE only (no stdio or WebSocket) at `http://127.0.0.1:8000/mcp` by default (`-ModelContextProtocolPort=`). Generate client config with `ModelContextProtocol.GenerateClientConfig`.
  - No authentication; loopback plus Origin allow-list. Tool calls run one at a time on the game thread.
  - Custom Python toolsets: subclass `unreal.ToolsetDefinition` with `@toolset_registry.tool_call`, then `ModelContextProtocol.RefreshTools`. "APIs and data formats may change at any time."
  - — [Epic: Unreal MCP in Unreal Editor](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-mcp-in-unreal-editor)
- **UE 5.9 MegaJam Preview** (announced 9 Nov 2026):
  - "about a week of development" and a shorter QA pass. "make a copy of your projects… rather than converting".
  - Known issue UE-412135: new projects give agents **no level-editing tools until Toolset plugins are enabled**.
  - The web target needs ASTC for older phones.
  - The release notes add Python libraries for editing Blueprints, StateTrees and more, extra Python search paths, and **removal of the Python Foundation Packages plugin**.
  - — [UE forum: UE 5.9 MegaJam Preview](https://forums.unrealengine.com/t/unreal-engine-5-9-megajam-preview/2852840); [Epic: UE 5.9 Preview Release Notes](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-9-release-notes)
- **flopperam/unreal-engine-mcp**: the repo says "This MCP is now owned by Aura!". The hosted "Flop MCP" has 50+ tools, including `python_execution` (arbitrary in-editor Python), `unreal_api`, `asset_factory` (enums, structs, DataTables, DataAssets, Enhanced Input only) and `search_assets`. The local `Python/` server is a "basic toolset" for scene and actor control and foundational Blueprint ops. Badges say UE 5.5+, and the agent "Supports Unreal Engine 5.5, 5.6, and 5.7". Last push 2026-06-26. — [flopperam/unreal-engine-mcp README](https://github.com/flopperam/unreal-engine-mcp)
- **chongdashu/unreal-mcp**: actor management, Blueprint and node-graph authoring, viewport control. UE 5.5+, Python 3.12+. No asset-import tool listed. Last push 2025-04-22. — [chongdashu/unreal-mcp README](https://github.com/chongdashu/unreal-mcp)

### Inferences

#### Recommended jam pipeline (Blender 5.2.2 via Blender Lab MCP to UE 5.7/5.9)
1. **Blender (agent via MCP `execute_blender_code`, or headless `execute_blender_code_for_cli`, as observed in this session's tool list):**
   - Run the AI cleanup or author the asset.
   - Run `validate_for_ue()` (section 2).
   - Run `batch_export_fbx()` (below) into `Export/<Category>/SM_*.fbx`, plus `T_*.png` and a `manifest.json`.
2. **UE:** import with `ue_import.py` (below) through flopperam `python_execution`, the UE Python console, or 5.9 Unreal MCP `ImportFile` (then run the fix-up script).
3. **Verify:** calibration asset once (section 1). Then per asset check bounds, scale 1, forward +X, collision view, sockets, Nanite flag and texture settings.

#### Blender batch FBX exporter (Blender 5.2; untested)
```python
# batch_export_fbx.py — one FBX per root SM_/SK_ object (+ its UCX_/SOCKET_ children), asset moved to origin.
import bpy, os, json
FBX_STATIC = dict(
    use_selection=True, object_types={'MESH', 'EMPTY'},
    global_scale=1.0, apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
    axis_forward='-Z', axis_up='Y', use_space_transform=True, bake_space_transform=False,
    use_mesh_modifiers=True, mesh_smooth_type='FACE', use_triangles=True, use_tspace=True,
    use_mesh_edges=False, use_subsurf=False, colors_type='SRGB', use_custom_props=False,
    add_leaf_bones=False, bake_anim=False, path_mode='AUTO', embed_textures=False, batch_mode='OFF')

def batch_export_fbx(out_dir, prefixes=("SM_",)):
    os.makedirs(out_dir, exist_ok=True)
    manifest = []
    roots = [o for o in bpy.context.scene.objects
             if o.type == 'MESH' and o.parent is None and o.name.startswith(prefixes)]
    for root in roots:
        bpy.ops.object.select_all(action='DESELECT')
        saved = root.matrix_world.copy()
        root.matrix_world.translation = (0.0, 0.0, 0.0)          # pivot = object origin at world 0
        root.select_set(True)
        for c in root.children_recursive:
            if c.name.startswith(("UCX_", "UBX_", "USP_", "UCP_", "SOCKET_")):
                c.select_set(True)
        bpy.context.view_layer.objects.active = root
        path = os.path.join(out_dir, f"{root.name}.fbx")
        bpy.ops.export_scene.fbx(filepath=path, **FBX_STATIC)
        root.matrix_world = saved
        manifest.append({"asset": root.name, "file": path,
                         "tris": sum(len(p.vertices) - 2 for p in root.data.polygons)})
    with open(os.path.join(out_dir, "manifest.json"), "w") as f:
        json.dump(manifest, f, indent=2)
    return manifest
```
- Alternative: put each asset in its own collection and use `batch_mode='COLLECTION'`. Note this excludes children collections, and assets must already sit at the origin.

#### UE import via Interchange with a saved pipeline (UE 5.7+; untested; probe names with `dir()`)
```python
# ue_import.py — run in UE Python (Output Log, flopperam python_execution, or a 5.9 MCP Python toolset)
import unreal, json
eas = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
PIPE = "/Game/TriadDrift/Pipelines/PL_StaticMesh_FBX"

def ensure_pipeline():
    if not eas.does_asset_exist(PIPE):
        eas.duplicate_asset("/Interchange/Pipelines/DefaultAssetsPipeline", PIPE)
    p = eas.load_asset(PIPE)
    mp = p.get_editor_property("mesh_pipeline")
    for k, v in {"import_static_meshes": True, "import_skeletal_meshes": False,
                 "combine_static_meshes": False, "collision": True,
                 "import_collision_according_to_mesh_name": True, "one_convex_hull_per_ucx": True,
                 "generate_lightmap_u_vs": False, "build_nanite": False}.items():
        try: mp.set_editor_property(k, v)
        except Exception as e: unreal.log_warning(f"pipeline prop {k}: {e}")
    # materials: 5.7 makes MIs anyway; consider disabling material import and assigning your own MIs
    eas.save_loaded_asset(p)
    return unreal.SoftObjectPath(PIPE + "." + PIPE.rsplit("/", 1)[-1])

def import_file(src, dest_dir):
    params = unreal.ImportAssetParameters()
    params.is_automated = True
    params.replace_existing = True
    params.override_pipelines.append(ensure_pipeline())
    mgr = unreal.InterchangeManager.get_interchange_manager_scripted()
    return mgr.import_asset(dest_dir, unreal.InterchangeManager.create_source_data(src), params)

def post_fix(sm_path, nanite=False, lod_group=None, simple_box=False):
    sm = eas.load_asset(sm_path)
    if nanite:
        ns = sme.get_nanite_settings(sm); ns.enabled = True
        sme.set_nanite_settings(sm, ns, apply_changes=True)
    if lod_group: sme.set_lod_group(sm, lod_group, True)          # e.g. "LargeProp"
    if simple_box: sme.add_simple_collisions(sm, unreal.ScriptCollisionShapeType.BOX)
    eas.save_loaded_asset(sm)
```
- **Legacy fallback** (if Interchange misbehaves): run `Interchange.FeatureFlags.Import.FBX False` (console or `DefaultEngine.ini`). Then use `AssetImportTask` with `options = unreal.FbxImportUI()`, and set `static_mesh_import_data` fields with the same names Send to Unreal uses: `combine_meshes`, `auto_generate_collision`, `one_convex_hull_per_ucx`, `generate_lightmap_u_vs`, `build_nanite`, `normal_import_method=unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS`, `normal_generation_method=unreal.FBXNormalGenerationMethod.MIKK_T_SPACE`, `convert_scene`, `force_front_x_axis`, `transform_vertex_to_absolute`. Run it via `unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])`.
- **5.9 Unreal MCP path:**
  - Enable All Toolsets (needed because of UE-412135).
  - Call `list_toolsets` / `describe_tools` to get the exact `ImportFile` argument schema.
  - Pass `bReplaceExisting=true` and `bSave=true`, since both default false.
  - Then run the `post_fix` and texture fix-ups through Python. A custom Python toolset (`unreal.ToolsetDefinition`) can wrap `import_file` + `post_fix` as one MCP tool.

#### Recent changes to watch (consolidated)
- **Blender 5.0**: `Action.fcurves` moved to channelbags.
- **Blender 5.1**: Python 3.13.
- **Blender 4.1+**: Auto Smooth is the Smooth-by-Angle modifier.
- **UE 5.5**: FBX defaults to Interchange.
- **UE 5.7**: FBX via Interchange gives MIs only. Nanite Foliage experimental.
- **UE 5.8**: glTF skinned-split regression report.
- **UE 5.9 Preview**: experimental built-in MCP with `ImportFile`; Python Foundation Packages plugin removed; copy projects instead of converting them.

### Gaps
- The exact argument names of UE 5.9's `ImportFile` (beyond `bReplaceExisting`/`bSave`), and whether it accepts Interchange pipeline overrides, are not documented on the page read.
- Python names for `common_meshes_properties` fields (Force All Mesh as Type, Recompute Normals, Use MikkTSpace) and for `material_pipeline` / texture sub-pipeline options (Import as Material Instance, Flip Normal Map Green Channel) were not verified for 5.7. Probe with `dir()`.
- Whether `AssetImportTask.options` accepts an Interchange pipeline object in 5.5+, and whether `FbxImportUI` settings are ignored when Interchange FBX is on, is not confirmed.
- flopperam's support for UE 5.9 is not stated; its docs list 5.5–5.7. The local unrealMCP tool set seen in this environment has no mesh-import tool, so import must go through `python_execution`.
- The default pipeline asset paths (`/Interchange/Pipelines/DefaultAssetsPipeline` and friends) come from a UE 5.1 post. Confirm they still exist in 5.7/5.9.

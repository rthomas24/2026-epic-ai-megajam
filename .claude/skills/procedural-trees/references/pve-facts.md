# Facts about PVE and Unreal MCP in this engine build

Established on 2026-11-12 by reading the engine's source and by calling the running editor. Every
"measured" line was observed, not assumed. Re-check them when the engine build changes.

## Engine and plugin

| Fact | Value | How it was established |
|---|---|---|
| Engine | Unreal Engine 5.9.0, branch `++UE5+Release-5.9`, changelist 60351404 (MegaJam Preview) | `Engine/Build/Build.version` |
| Plugin | **`ProceduralVegetationEditor`** ("Procedural Vegetation Editor"), experimental, version 1.0 | `Engine/Plugins/Experimental/ProceduralVegetationEditor/ProceduralVegetationEditor.uplugin` |
| Enabled by default | **No** (`"EnabledByDefault": false`). This repo enables it in `unrealGameJam.uproject` for Editor targets only | the `.uplugin`; the `.uproject` |
| Modules | `ProceduralVegetation` (Runtime) and `ProceduralVegetationEditor` (Editor) | the `.uplugin` |
| Plugins it pulls in | Dataflow, GeometryScripting, PCG, DynamicWind | the `.uplugin` |
| Shipped content | 4 sample plants, 10 grower presets (`PVE_Preset_*`: Leaf_Tree 01-04, Pine, Spruce 01-02, Shrub, Sapling, Tropical), 1 trunk-profile asset, sample twigs and materials | `ListAssets`, `ListDataAssets` |
| Engine's own tests for the scripting API | `Content/Python/pve_tests/*.py` in the plugin: the best worked examples of the calls below | read |

A PVE asset (class `ProceduralVegetation`) wraps a PCG graph (`ProceduralVegetationGraph`), so PCG's
graph calls work on it. Epic's documentation page describes the editor only; it says nothing about
scripting. The scripting entry point is in the source.

## What is exposed

`UProceduralVegetationEditorSubsystem` (`ProceduralVegetationEditorSubsystem.h`). Every function below
is `PythonCallable` **and** `AICallable`, so the same list is the MCP toolset
`ProceduralVegetationEditor.ProceduralVegetationEditorSubsystem` (18 tools):

| Function (Python name) | Use |
|---|---|
| `create_graph(name, path, source=None)` | New PVE asset; returns its graph. With `source`, a copy of a sample |
| `get_graph(asset)` | The graph inside a PVE asset |
| `list_node_types()`, `describe_node(type)`, `get_node_schema(type)` | 49 node types; exact names such as `ProceduralVegetationGrower`, `ProceduralVegetationMesher`, `ProceduralVegetationFoliage` (palette), `ProceduralVegetationFoliageDistributor`, `ProceduralVegetationScale`, `Export` |
| `add_node(graph, type, name, ...)` | Adds a PVE node (PCG's own add-node refuses PVE types) |
| `suggest_pin_connections(a, b)` | Valid pin pairs between two nodes |
| `list_assets(folder, name)`, `list_data_assets(class, folder, name)` | Plants; grower presets, growth data, plant profiles |
| `execute_graph(graph)` -> async result, `get_execution_state`, `cancel_execution` | Runs the graph headless |
| `get_export_settings`, `get_export_support` | Export node settings; whether Nanite / Nanite foliage / voxels are available |
| `export(graph, node, save, overwrite)`, `export_with_settings(...)`, `get_export_state`, `cancel_export` | Writes Static Mesh or Skeletal Mesh assets, no dialogs |

Not exposed as functions, but reachable from Python because they are editor properties:
node settings (`node.get_settings()`), e.g. `PVGrowerSettings.grower_params` (`PVGrowerParams`),
`PVMeshBuilderSettings.mesher_settings`, `PVFoliagePaletteSettings.foliage_infos`,
`PVFoliageDistributorSettings.parametric_settings`, `PVScaleSettings.scale`,
`PVExportSettings.export_settings`; and the preset asset `ProceduralVegetationGrowerPreset.growth_params`.
Edges are made with the PCG graph's own `graph.add_edge(from_node, "Out", to_node, "In")`.

Blueprint: the subsystem class is `BlueprintType` but its functions are not `BlueprintCallable`.
Use Python or MCP.

## MCP in this project

| Fact | Value |
|---|---|
| Server | Engine plugin `ModelContextProtocol`; this project enables 21 toolset plugins for Editor targets (`unrealGameJam.uproject`) |
| Start | `Tools/td-play-mcp.ps1 -Editor -Port <port>` starts a hidden editor (`-RenderOffscreen`) with `-ModelContextProtocolStartServer -ModelContextProtocolPort=<port>`; `Tools/td_mcp.py` is the shell client |
| Run Python | `editor_toolset.toolsets.python_scripting.PythonScriptingTools.execute_python` (top-level `await`, result = last expression). `scripts/ue_exec.py` wraps it |
| Create and edit PVE assets | the PVE toolset above; `PCGEditor.PCGEditorSubsystem` (about 45 tools: `ConnectNodePins`, `UpdateNode`, `GetGraphStructure`, ...) |
| Spawn actors, levels | `EditorToolset.LevelToolset` (`CreateLevel`, `AddToSceneFromAsset`, ...), `EditorToolset.ActorToolset` (`AddComponent`, `SetActorTransform`, ...) |
| Static mesh facts | `EditorToolset.StaticMeshToolset` (`GetTriangleCount`, `GetVertexCount`, `IsNaniteMesh`, `GetLodCount`, `GetBounds`, `SetNaniteEnabled`, `GenerateLods`) |
| Captures | `SceneCaptureToolset.SceneCaptureToolset.Capture` (offscreen render of a world from a camera; modes include `FinalImage`, `Lit`, `BaseColor`, `VisualizeNanite`). `scripts/capture.py` wraps it |
| Performance | `PerfToolset.StatsToolset` (`GetFrameTimings`, `GetStatCounters`), `PerfToolset.GpuProfilerToolset`, `PerfToolset.CsvProfilerToolset` |

## What already existed for vegetation

- **Engine:** PVE (above); the Foliage system (`InstancedFoliageActor`, `FoliageType`); PCG; Nanite
  Foliage / Nanite Assemblies (off in this project: `get_export_support()` returns
  nanite true, nanite foliage false, voxels false); the DynamicWind plugin for skeletal trees.
- **This codebase before this skill:** nothing. No foliage, vegetation or tree code under
  `Source/unrealGameJam`, no vegetation content.

## Measured behaviour that the scripts depend on

1. **A struct assigned from Python does not invalidate the graph.**
   `settings.set_editor_property("grower_params", value)` stores the value, but
   `get_execution_state().is_out_of_date` stays false and the next `execute_graph` returns the cached
   result. Changing a plain property does invalidate it. `pve_lib.touch()` flips the node's unused
   PCG `seed` and restores it. Without this, every "tuning" run silently returns the first tree.
2. **`FPVFloatRamp` has no Python properties.** Ramps, and any nested struct, are set with
   `struct.import_text("(Field=(Sub=1.0))")`; only the fields named change. `export_text()` reads
   them back.
3. **The grower is not scale-free.** The same parameters grown at a 60 m target lose nearly all
   branches (331 triangles); at 12 m they do not. Trees are therefore grown at a canonical 12 m and
   brought to size by the `ProceduralVegetationScale` node.
4. **Thin trunks collapse.** A trunk grown at the species' real (thin) girth bends to the ground under
   the grower's gravity. Girth is grown at a canonical value and corrected in the Mesh Builder
   (`branch_radius.GenerationScales`).
5. **Axillary ramps do not give a clear trunk.** A ramp of zeros still branches (with or without
   `bAxillaryRetry`); `AxillaryParentGrowth=0` removes all trunk branches; the Carve node trims from
   the top for every basis. A clear bole comes from **two Growers in series**: the first raises a bare
   trunk for `crown_base x cycles` cycles, the second resumes from its output and branches.
6. **`BranchTargetLength` does control crown width** (0.5 -> 4.7 m, 2.65 -> 8.4 m, 8 -> 11.3 m on one
   test tree), so the crown is corrected by scaling it, a few passes.
7. **Foliage:** the sample twigs start at the origin and point along +Z; our leaf meshes follow that.
   The exported tree keeps the foliage mesh's UV0 and its material, in a second material slot after
   the bark. Parametric `BranchDensity` on a bare stem gives about 0.62 fronds per unit.
8. **Export:** a from-scratch graph executes in under a second and exports a Nanite static mesh in
   one to five seconds. Nanite Foliage is off in this project (`get_export_support`), so the scripts export with `create_nanite_foliage` false and
   the foliage is merged into the mesh. Rosette forms log "Convex hull generation produced zero convex
   particles" from the Mesh Builder; collision is off, so it is harmless.
9. **The reference article** (x.com/UnrealEngine/status/2119840871168528687, "Botanical Converter")
   could not be fetched (HTTP 402) and no copy was found by search. The pipeline here follows the
   brief's description of it and the two documentation pages, not the article's text.

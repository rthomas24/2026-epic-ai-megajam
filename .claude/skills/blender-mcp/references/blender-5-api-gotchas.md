# Blender 4.0 → 5.2 API changes that break agent-written code

About 90% of failures of LLM-written Blender 5.x scripts were calls to APIs that no longer exist
(3DCodeBench, 2026). Feeding the error message back for two retries raised the runnable rate from
0.69 to 0.97. So: read this table once, look up anything uncertain, and when a call fails, read the
traceback and fix the specific name. Don't rewrite blindly. Everything below was verified
headless on **Blender 5.2.2** unless marked otherwise.

| Old (breaks) | Use instead (5.2) | Since |
|---|---|---|
| `bpy.ops.x.y(ctx_dict, ...)` | `with bpy.context.temp_override(**ctx): bpy.ops.x.y(...)` (a dict argument raises `ValueError`) | 4.0 |
| Principled `Specular`, `Emission`, `Subsurface`, `Transmission`, `Coat`, `Sheen` | `Specular IOR Level`, `Emission Color` (+ `Emission Strength`), `Subsurface Weight`, `Transmission Weight`, `Coat Weight`, `Sheen Weight` (`KeyError: 'Specular'` is the classic symptom) | 4.0 |
| `tree.inputs.new()` / `tree.outputs.new()` on node groups | `tree.interface.new_socket(name, in_out='INPUT'/'OUTPUT', socket_type='NodeSocketFloat')`; iterate `tree.interface.items_tree` | 4.0 |
| `edge.crease`, `bevel_weight`, `bm.edges.layers.crease` | attributes `crease_edge`, `crease_vert`, `bevel_weight_edge`, `bevel_weight_vert`, `sharp_edge`, `sharp_face`, seams `uv_seam`; `mesh.edge_creases_ensure()` | 4.0 |
| `Mesh.calc_normals()` | not needed (normals are always current) | 4.0 |
| `export_scene.obj`, `import_scene.obj`, `import_mesh.ply`, `export_mesh.stl` | `wm.obj_export` / `wm.obj_import` / `wm.ply_import` / `wm.stl_export` / `wm.stl_import` | 4.0 / ≤5.2 |
| `mesh.use_auto_smooth`, `auto_smooth_angle` | `mesh.shade_smooth()` + `mesh.set_sharp_from_angle(angle=rad)`; or `bpy.ops.object.shade_smooth_by_angle(angle=)`; or `bpy.ops.object.shade_auto_smooth(angle=)` (adds the GN "Smooth by Angle" modifier) | 4.1 |
| `calc_normals_split()`, `create/free_normals_split()`, writing `loop.normal` | read `mesh.corner_normals`; write with `mesh.normals_split_custom_set()` | 4.1 |
| `ShaderNodeTexMusgrave` | `ShaderNodeTexNoise` with `noise_type` (`FBM`, `MULTIFRACTAL`, `RIDGED_MULTIFRACTAL`, ...) | ≤5.2 |
| `'BLENDER_EEVEE_NEXT'` | `'BLENDER_EEVEE'` (NEXT only existed 4.2–4.5 and raises in 5.x). Engines: `BLENDER_EEVEE`, `BLENDER_WORKBENCH`, `CYCLES` | 5.0 |
| `scene.eevee.use_bloom`, `use_ssr`, `use_gtao`, `Material.blend_method`, `shadow_method` | bloom → compositor Glare node (`inputs['Type'].default_value = 'Bloom'`); `Material.surface_render_method` (`'DITHERED'`/`'BLENDED'`); `light.use_shadow` | 4.2–5.0 |
| `scene.node_tree`, `scene.use_nodes`, `CompositorNodeComposite` | `ng = bpy.data.node_groups.new(name, 'CompositorNodeTree'); scene.compositing_node_group = ng` with a Group Output | 5.0 |
| `material.use_nodes = True`, `world.use_nodes = True` | not needed: new materials and worlds already have node trees (the setter warns "removed in 6.0") | 5.0 |
| `scene['cycles']`, `ob['some_rna_prop']` | attribute access `scene.cycles.samples`; `ob['x']` only for true custom properties | 5.0 |
| `import bgl`, `image.bindcode` | `gpu` module, `gpu.texture.from_image(img)` | 5.0 |
| Boolean `solver='FAST'` | `'FLOAT'`; also `'EXACT'` (robust, coplanar-safe) and `'MANIFOLD'` (fastest, closed meshes only) | 5.0 |
| `action.fcurves`, `action.groups` | `bpy_extras.anim_utils.action_get_channelbag_for_slot(action, slot).fcurves`; also set `animation_data.action_slot` | 4.4 / 5.0 |
| `image_settings.file_format = 'FFMPEG'` directly | set `image_settings.media_type = 'VIDEO'` (or `'MULTI_LAYER_IMAGE'`) **first** | 5.0 |
| `ShaderNodeSeparateRGB` / `CombineRGB` / `HSV` | `ShaderNodeSeparateColor` / `ShaderNodeCombineColor` (with `mode`) | 5.0 |
| GN modifier `mod["Socket_2"] = v`, `mod["Socket_2_use_attribute"]` | `getattr(mod.properties.inputs, "Socket_2").value = v`; `.type = 'ATTRIBUTE'`; `.attribute_name` (the old form raises `TypeError`) | 5.2 |
| GN Random Value / Compare socket names | `Min`, `Max`, `ID`, `Seed` (`Probability` for BOOLEAN); Compare `A`, `B` | 5.2 |
| Python 3.11 assumptions | Blender 5.1+ ships **Python 3.13** | 5.1 |
| Bool Tool, LoopTools, Extra Objects operators | not bundled in 5.2; they need the Extensions platform. Don't call them | 4.2+ |

## Headless (`--background`) reality on 5.2.2
- **Fail:** `view3d.view_all`, `view3d.snap_cursor_to_center`, `render.opengl`, `screen.screenshot`.
- **Work:** `object.mode_set`, `mesh.select_all`, `uv.smart_project`, `uv.pack_islands`,
  `uv.lightmap_pack`, `object.bake`, `render.render(write_still=True)` (EEVEE and Workbench too),
  `object.convert`, `object.shade_auto_smooth`, `object.join` / `modifier_apply` under
  `temp_override`, `export_scene.fbx` / `gltf`.
- In the live MCP session, code runs in a timer with **no `context.area`**. Operators that need a
  3D View must be wrapped in `temp_override(window=..., area=..., region=...)` or replaced with
  data-API calls.

## Patterns that save retries
- Create objects with the data API (`bpy.data.meshes.new` + `bmesh`, then
  `bpy.data.objects.new` + `collection.objects.link`). Keep the returned objects; don't look them up
  by an assumed name.
- Apply a modifier stack without operators:
  `me = bpy.data.meshes.new_from_object(obj.evaluated_get(dg), preserve_all_data_layers=True, depsgraph=dg)`.
- Count evaluated triangles: `ev = obj.evaluated_get(dg); me = ev.to_mesh(); me.calc_loop_triangles(); n = len(me.loop_triangles); ev.to_mesh_clear()`.
- World-space bounds: transform evaluated verts (or `bound_box` corners) by `matrix_world`. After
  parenting or moving things, call `bpy.context.view_layer.update()` before reading `matrix_world`.
- `bmesh`: call `ensure_lookup_table()` before indexing, then write back with
  `bm.to_mesh(me); bm.free()`. In edit mode, use `bmesh.from_edit_mesh` + `bmesh.update_edit_mesh`.
- Primitive sizes: `primitive_cube_add(size=2)` is 2 m edge to edge (default). Build sized boxes
  with `bmesh.ops.create_cube(bm, size=1, matrix=Matrix.Diagonal((x, y, z, 1)))`.
- Cylinders along an axis: rotate the *mesh data*, not the object's Euler, and keep object rotation
  at zero.
- A visible high-poly darkens low-poly AO bakes; hide everything except the bake target and its
  source.

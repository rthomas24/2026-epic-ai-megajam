# Local validation on Blender 5.2.2 LTS (Windows 11), Nov 12 2026

Hands-on results from running candidate skill scripts headless with
`"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe" --background --factory-startup --python <script>`.
These are first-hand measurements, not web sources. Scripts were held in the session scratchpad and are being
packaged into the repo's skills.

## Environment facts (checked on disk / headless)
- Blender 5.2.2 LTS installed at `C:\Program Files\Blender Foundation\Blender 5.2\blender.exe`; not on PATH; `BLENDER_PATH` unset.
- Official Blender Lab MCP installed as Claude Desktop extension "Blender" v1.0.1 (server code from the 1.0.0 era; bundles Blender 5.1 API + manual RST docs). Tools appear as `mcp__Blender__*`.
- Blender 5.2 user prefs: `lab.blender.org` extensions repo is configured and enabled, but **no MCP add-on is installed** (no `mcp` module found via `addon_utils`), and Blender was not running (nothing listening on port 9876). Live `execute_blender_code` therefore could not be exercised; the doc-search tools (`search_api_docs`, `search_manual_docs`) work without Blender because they read the bundled RST.
- Render tools write to `bpy.app.tempdir/blender_mcp/<basename>` regardless of the directory passed (read the returned `filepath`).

## Scripts and measured results
| Script (purpose) | Result on 5.2.2 |
|---|---|
| `review_renders.py` - multi-view contact sheet in a temporary scene (clay = Workbench, material = EEVEE w/ bundled studio HDRI + key/rim suns, wireframe = Workbench + Wireframe-modifier copies) | All modes render headless. 6 views at 512 px: clay ~0.5-0.9 s, material (EEVEE, 16 samples) ~0.7-1.5 s, wireframe ~0.3 s. User scene verified unchanged (objects, scenes, cameras, lights, worlds). Orthographic views share one scale (blueprint style, `ortho_cm_per_px` returned); perspective views render at 1.5x and auto-crop to the asset. |
| `asset_report.py` - evaluated-mesh health report | Correctly flagged planted defects: unapplied + negative scale, flipped winding (via `BMEdge.is_contiguous`), inside-out closed mesh (signed volume < 0), duplicate verts (`bmesh.ops.find_doubles`), loose verts, missing UVs, empty material slot, `.001`/space names, tri budget. Clean crate: zero issues. |
| `assembly_check.py` - floating-part detection (per-island BVH overlap + nearest-distance union-find) | Ship with a nose ring and no struts: ring flagged floating, gap 0.39 m. With 4 struts: single connected group. ~1 s. |
| `procedural_rocks.py` - asteroid (icosphere + fractal/ridged noise + crater profile), crystal cluster (tapered hex prisms + poked tips) | Asteroid 5,120 tris at subdivisions=5 (icosphere tris: 4=1,280, 5=5,120, 6=20,480, 7=81,920). Crystals ~200 tris. Smart UV via operator works headless. |
| `bake_texture_set.py` - Unreal texture set (T_*_BC sRGB, T_*_N DirectX, T_*_ORM packed, T_*_E) | 512 px set in ~1.2 s on CPU. Metallic purple hull baked to correct base colour (~0.80/0.25/0.88 sRGB) and ORM (rough ~0.30, metal ~1.0). Preview material (green flipped back for Blender) renders equivalent to the procedural original. |
| `cleanup_ai_mesh.py` - import GLB/FBX/OBJ, weld, scale/origin, decimate/remesh, Smart UV, selected-to-active re-bake | Simulated AI GLB (20,480-tri triangle soup, 0.013 scale, baked texture): welded 51,198 verts, rescaled 64.7x to 180 cm, decimated to 3,000 tris, baked BC/N/ORM from the source in ~3 s; asset report clean. |
| `hardsurface.py` - loft hull, swept wing, lathe nacelle, bevel+harden+weighted-normal stack, mirror (with mirror_object), boolean (CUT_ cutters), inset glow panels, region-limited greebles, context-free apply/join/origin | 10-part fighter built in 0.04 s; joined to one mesh with custom normals preserved, 4 materials, 0 non-manifold edges. |
| `export_for_unreal.py` - one FBX per asset (+ manifest) | Round trip verified by re-import: exact names `SM_Ship_Example`, `UCX_SM_Ship_Example_00..05` (auto convex hulls from <=64 support points), `SOCKET_Muzzle_L`; object scale 1.0; dimensions preserved (7.10 m); authoring front (-Y) rotated to +X (nose vertex at +X 3.4 m; socket rotated accordingly); materials preserved; user scene unchanged. FBX settings: apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS', axis -Z forward / Y up, mesh_smooth_type='FACE', use_triangles=True, use_tspace=True, add_leaf_bones=False, bake_anim=False. Unreal-side import not tested (editor not running). |

## Failure modes found by the render -> critique loop (and fixes)
1. **Collapse-decimating a noisy surface** leaves long thin triangles that shade as shards. Fix: choose density at generation (subdivision level) instead of decimating.
2. **`set_sharp_from_angle` on organic rocks** (50 deg) produced faceted noise. Fix: pure smooth shading for rocks.
3. **Discontinuous displacement functions** (crater bowl ends at 0 but rim starts at +0.35 depth) tear the surface. Fix: make profiles continuous at the joins.
4. **Strong emission (3+ with AgX)** turns saturated glow colours pale/white and flattens facets into a silhouette. Fix: keep emission ~0.5-3 and judge glow colour from the emissive map; AgX desaturates bright colours in review renders.
5. **Broad colour noise only** gives an almost flat baked normal map. Fix: drive bump from a separate high-frequency noise.
6. **Angle-limited Bevel at a sharp cone/body junction** collapses into coincident vertices. Fix: add an intermediate (ogive) section; weld at export.
7. **Inset on tiny faces** collapses into duplicates. Fix: area threshold (>= 16 x thickness^2).
8. **Boolean cutters without material** leave an empty slot on the result. Fix: give the cutter the target's material before the boolean (or material_mode TRANSFER) and remove unused slots on join.
9. **Edit-mode operators under `temp_override(active_object=...)` fail poll** (`mesh.select_all`): they read the real view-layer active object. Fix: set `view_layer.objects.active` + selection for real and restore.
10. **Exported copies silently renamed `.001`** because originals still own the names -> Unreal can't pair `UCX_` collision with the mesh. Fix: temporarily rename originals during export and restore.
11. **Cycles DIFFUSE colour bake multiplies by (1 - metallic)** -> metallic hulls bake black. Fix: bake Principled inputs through a temporary Emission shader (EMIT pass).
12. **Floating parts** (a ring around the nose with nothing holding it) look fine in some views. Fix: geometric contact check, struts/overlap of a few cm.
13. **Unrelated scene objects darken AO bakes.** Fix: hide everything except target/sources during the bake.

## Quality caveat (user feedback, Nov 12)
The user reviewed the code-built test fighter and judged it far below AAA ("looks terrible"). Code-only primitive modeling by the agent produced a clean but simplistic, toy-like result even with good shading and detail passes. Hero assets need a stronger design process (concept/reference first, shape language, detail hierarchy) and likely AI image-to-3D or kitbash sources plus cleanup, rather than primitives alone.

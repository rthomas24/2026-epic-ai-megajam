---
name: blender-ai-mesh-cleanup
description: Turns AI-generated or downloaded 3D models (Hyper3D Rodin, Tripo, Meshy, Hunyuan3D, Fab, Sketchfab, Poly Haven; GLB/GLTF/FBX/OBJ/STL) into game-ready meshes for Unreal in Blender 5.2. It welds triangle soup, removes loose and degenerate geometry, fixes normals, scales to real-world size, sets the origin, remeshes or decimates to a triangle budget, re-unwraps UVs, re-bakes the original look (colour, normal detail, roughness/metal, emission) onto the clean mesh, and records the source and licence. Use whenever someone brings in a model from an AI generator or asset site, says an imported model is too heavy, broken, wrongly scaled or has bad UVs, or follows the route-A (concept → image-to-3D) path for hero assets.
compatibility: Blender 5.2.x (Cycles for the re-bake). Read blender-mcp first; uses scripts/cleanup_ai_mesh.py, which calls the blender-texturing bake script.
---

# AI and downloaded meshes → game-ready

AI generators give you shape and texture fast, but the raw files are not game assets. Typical
problems, and what fixes them:

| Defect | Seen in | Fix |
|---|---|---|
| Triangle soup (every triangle has its own vertices), 100k–2M tris | Most raw exports; Rodin "Raw", Hunyuan | Weld (merge by distance) → decimate, or voxel remesh, to budget |
| Unit-less or wrong scale (0.01×, 1 m bounding sphere) | Hunyuan (normalised), many GLBs | Scale to the brief's real size |
| Wrong orientation (Y-up / -Z forward confusion) | Varies per tool; Tripo reports success even when wrong | Check the review renders; pass `up_fix_deg` and rotate to front = -Y |
| Floating fragments, holes, non-manifold, self-intersections | Hunyuan, Tripo, Meshy | Delete loose geometry; voxel remesh (`voxel_size`) closes holes; `assembly_check` finds floaters |
| Lighting or shadows baked into the albedo | Rodin "Shaded", textures without de-lighting | Generate with PBR / delight on; otherwise repaint flat areas or replace with a flat material instance |
| Dense, chaotic UVs (box / lightmap style) | Most | Re-unwrap after reducing, then re-bake from the original |

## Run it

```python
import runpy
m = runpy.run_path(r"<repo>\.claude\skills\blender-ai-mesh-cleanup\scripts\cleanup_ai_mesh.py")
result = m["main"](path=r"<repo>\ArtSource\Space\Ships\Rift\source\rodin_rift.glb", name="SM_Ship_Rift",
                   target_size_cm=700, target_tris=40000,
                   out_dir=r"<repo>\ArtSource\Space\Ships\Rift\export", bake_size=2048)
```
Headless (a good default for big meshes: no UI freeze, no 300 s socket limit):
```powershell
& $blender --background --factory-startup --python "<repo>\.claude\skills\blender-ai-mesh-cleanup\scripts\cleanup_ai_mesh.py" -- `
   --path "<...>\source\rock.glb" --name SM_Rock_Cinder_A --height-cm 180 --tris 4000 `
   --out-dir "<...>\export" --save-blend "<...>\Rock_Cinder_A_v01.blend"
```
**Key parameters:**
- `target_height_cm` or `target_size_cm` (longest side) sets real size.
- `target_tris` is the budget.
- `voxel_size` (metres) remeshes before decimating. Use it for holey or self-intersecting meshes;
  about 0.5–1% of the asset size.
- `up_fix_deg` rotates about X if the model lies on its side.
- `merge_distance` defaults to 0.5 mm.
- `sharp_angle_deg` is 60 for organic shapes; use 30–40 for hard surface.
- `keep_source=True` keeps a hidden `SRC_<Asset>` (re-bake source; never exported).

**Verified on 5.2.2** with a simulated generator file: a 20,480-tri triangle soup at 0.013 scale. The
script welded 51,198 vertices, rescaled 64.7× to 180 cm, decimated to 3,000 tris and re-baked BC, N
and ORM from the source in about 3 s. The health report came back clean.

## Then finish it like any other asset
1. `review_renders.py` in `clay` mode (shape and orientation: front must face -Y) and `material`
   mode (did the bake carry the look?).
2. `asset_report.py` + `assembly_check.py`. Fix the scale, origin or tris if needed and rerun.
3. **Polish (route A).** Code beats AI at crisp details:
   - Mirror symmetric hulls exactly: cut at X = 0 and add a Mirror modifier.
   - Replace mushy nozzles, intakes and canopies with `hardsurface.py` parts.
   - Add emissive inserts, `SOCKET_*` empties and collision.
   - Re-bake if the surface changed.
4. Gate review per `blender-game-asset` (rubric), then `blender-unreal-export`.

## Provenance (required by the jam)
- Keep the original in `<Asset>/source/` with `SOURCE.md`: tool and model version, plan (free or
  paid), date, input image or prompt, and licence.
- Add a `CREDITS.md` line. Rodin, Tripo and Meshy go in "Tools you used" (partner prizes).
- **Licence checks:**
  - Meshy free = CC BY 4.0 (attribution).
  - Tripo free-tier rights are disputed: use a paid plan.
  - Hunyuan3D 2.1 is not licensed in the EU, UK or South Korea.
  - Rodin's commercial rights depend on the plan, and Rodin outputs can't be used to train AI.
  - Downloaded models need an explicit CC0, CC-BY or Fab licence, and no franchise IP.
- Generator settings that reduce cleanup work (PBR on, de-lighting, quad mode, polycount, real-size
  options) are in the blender-game-asset skill's `references/asset-routing.md`.

---
name: blender-texturing
description: Materials, UV unwrapping and texture baking in Blender 5.2 for Unreal Engine 5. Decides between flat material instances and a baked texture set, unwraps and packs UVs, and bakes procedural or layered Blender materials (or a high-poly or AI source) into Unreal-ready maps - T_*_BC base colour, T_*_N DirectX normal, packed T_*_ORM (AO/roughness/metallic), T_*_E emissive - with a preview material to verify the result. Use whenever an asset needs materials or textures that must survive export to Unreal, when baking normals/AO from high to low poly, or when baked maps look wrong (black metal, inverted or flat normals, dark AO, seams).
compatibility: Blender 5.2.x, Cycles (CPU by default; GPU optional). Read the blender-mcp skill first. Script: scripts/bake_texture_set.py.
---

# Materials, UVs and baking for Unreal

Unreal can't read Blender's procedural nodes. Anything driven by node graphs (noise, ramps, bump,
masks) has to be **baked to images** or rebuilt in Unreal. Plain Principled values (one colour per
material slot) don't need baking: the export manifest carries them, and the Unreal import script
rebuilds them as material instances.

## Decide: flat material instances or a baked texture set

| Use **flat MIs** (no bake) when... | **Bake a texture set** when... |
|---|---|
| Stylized hard surface with a few solid materials (paint / trim / glow / glass) per slot | Any procedural or noise-driven material (rocks, worn paint, grime, patterns) |
| Crystals, kit pieces using a trim sheet, simple props, the energy blade | AI or downloaded meshes (their look lives in their textures, so re-bake onto the cleaned mesh) |
| You need it in-engine *today* | Hero assets that need edge wear, AO in crevices, decals or livery baked in |

A hybrid is common: flat MIs per slot, plus a baked **normal + AO** set for surface detail.

## UVs
- One UV map named `UVMap`, unwrapped once for the whole asset after joining (`hs["join"]`). A
  joined, single-texture-set asset is one material and one draw call in Unreal.
- Unwrap with `hs["smart_uv"](obj, angle_deg=66, island_margin=0.02)`. It sets the real active
  object, which edit-mode operators need. Headless also works. Optionally run
  `bpy.ops.uv.pack_islands(margin=0.01)` in edit mode afterwards.
- No lightmap UV channel needed: the project uses Lumen. Turn off "Generate Lightmap UVs" in Unreal.
- Texel density: see the blender-game-asset `art-direction.md` (hero ships 256–512 px/m).

## Bake (scripts/bake_texture_set.py)

```python
import runpy
bk = runpy.run_path(r"<repo>\.claude\skills\blender-texturing\scripts\bake_texture_set.py")
result = bk["main"](target="SM_Asteroid_A", out_dir=r"<repo>\ArtSource\Environment\Asteroids\Asteroid_A\export",
                    size=1024, build_preview_material=True)
```
- **Outputs:** `T_<Asset>_BC.png` (sRGB), `T_<Asset>_N.png` (DirectX: green flipped, so **don't**
  tick Flip Green in Unreal), `T_<Asset>_ORM.png` (R = AO, G = roughness, B = metallic; linear), and
  `T_<Asset>_E.png` (emissive colour; skipped when there's no emission). The result includes
  `emission_strength`; carry it to the Unreal material.
- **Why "via emission":** Cycles' diffuse-colour pass multiplies base colour by (1 − metallic), so
  metal bakes black. The script routes each Principled input through a temporary Emission shader
  and bakes EMIT. Normal and AO use Cycles' native passes. **Verified on 5.2.2:** a metallic purple
  hull bakes to the correct purple.
- **High-to-low bake** (AI meshes, sculpts): `source_objects=["SRC_<Asset>"], cage_extrusion=0.01–0.03 × size, max_ray_distance≈3× that`.
  The `blender-ai-mesh-cleanup` script does this for you.
- While baking, every object except the target and its sources is hidden, so stray objects don't
  darken the AO. The user's selection, render engine and samples are restored afterwards.
- **Speed:** a 512² set takes about 1 s on CPU and 1K about 3 s. 2K is fine headless. In the live
  MCP session, keep bakes small or run them headless (the 300 s socket timeout).
- **Verify:** `build_preview_material=True` swaps the object onto an image-based material equivalent
  to what Unreal will show. Its normal map flips green back for Blender. Then run
  `review_renders.py` in `material` mode and compare against the procedural version. If they
  differ, fix the bake before exporting.

## Making materials read as AAA (before baking)
Full guidance is in `references/materials.md`. The core: a 60/30/10 colour split, a roughness range
(not one value), metallic only on bare metal, emission ≤ 5% of the surface at strength 1.5–3, and
subtle wear in cavities and on edges (curvature/AO masks), plus livery or decal cues.

## Troubleshooting

| Symptom | Fix |
|---|---|
| Metal parts bake black or dark | You used DIFFUSE colour; use this script (EMIT routing) |
| Normal map almost flat (uniform blue) | The material's bump comes only from broad noise; drive the bump from a fine, high-frequency noise |
| Lighting looks inverted in Unreal | The normal map was flipped twice; this script already outputs DirectX, so leave Flip Green off |
| AO dark everywhere | Other objects occluded it, or a visible high-poly overlapped; the script hides them, so check for objects with `hide_render` off inside the asset |
| Seams or black edges | Margin too small or islands too close; `margin=16` px, island margin ≥ 0.01; avoid 256 px maps for big assets |
| Glow too white in Unreal | Emissive colour is normalised and strength is separate; lower the MI's EmissiveStrength rather than the texture |
| "No UV map" | Unwrap first (`smart_uv`) |

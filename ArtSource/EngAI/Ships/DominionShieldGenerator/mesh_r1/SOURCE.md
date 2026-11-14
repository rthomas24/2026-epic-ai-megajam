# SM_DominionShieldGenerator — mesh_r1 source

Route C procedural hard-surface. Blender 4.5.3 LTS, no AI 3D generator.
Issue [#41](https://github.com/rthomas24/2026-epic-ai-megajam/issues/41).
Branch `grok/assets-dominion-shield-gen-r1`.

Built against `TDShieldGenerator.h` / `TDShieldGenerator.cpp`, `UTDDominionVisuals` (`GeneratorMesh` + `GeneratorCoreMesh`) in `TDPresentation.h`, and the mothership r2 generator-plinth contract (`ArtSource/EngAI/Ships/DominionMothership/mesh_r2/manifest.json`, also identical in mesh_r1): plinth top **Z = −50 cm**, dome keep-clear **r = 1800 cm, Z 0..4000 cm** above each `SOCKET_Generator_*`. Palette and trident follow hull r2 / `sheet-DominionMothership-v3_PICK.png`.

## C++ contract (read this before placing the assets)

`ATDShieldGenerator` has two mesh components:

1. **`Mesh` ("Pylon")** is the root. The placeholder is a cylinder at `RelativeScale3D(12, 12, 20)` (12 m wide, 20 m tall, centred on the origin). When `Visuals->GeneratorMesh` is set, `Init()` does `Mesh->SetStaticMesh(...)` and `Mesh->SetRelativeScale3D(OneVector)`. **Author the pylon at real size, scale 1, origin = actor / socket origin.**

2. **`Core` ("Core")** is attached to `Mesh` at **`RelativeLocation (0, 0, 60)` cm**. This offset is set in the constructor and **`Init()` never resets it** — it only calls `SetRelativeScale3D(OneVector)` and `SetVisibility(true)` when `GeneratorCoreMesh` is assigned.

   In placeholder mode the parent's Z-scale is 20, so the core sat at `60 * 20 = 1200 cm`. Once the art pylon is assigned and Mesh scale becomes 1, that same `(0, 0, 60)` is only **+60 cm** above the Mesh origin.

   **Therefore the core FBX is authored with its visual centre at local Z = +1040 cm.** After C++ places the component at +60 cm, the glowing chamber sits in the pylon cradle at **Z = +1100 cm**, which is also `SOCKET_Core`.

3. **`ShowDestroyed()`** hides `Core` and scales `Mesh` by `(1, 1, 0.4)` about the Mesh origin ("Leave a stump: the pylon stays as cover, the core is gone"). The lower 40% of the pylon — base ring + armored collar, **Z = −50 .. +730 cm** — is a solid, plate-heavy stump that still reads as wreckage after the squash. Because the scale is about Z = 0, the base lifts **30 cm** (Z −50 → −20 cm). At this scale that is ~2% of the 14.8 m footprint; the stump still appears planted on the plinth. Documented, not compensated by sinking the live mesh into the plinth.

## Dimensions (Unreal cm, uniform import scale 1.0)

| | Pylon `SM_DominionShieldGenerator` | Core `SM_DominionShieldGeneratorCore` |
|---|---|---|
| Pivot | (0,0,0) = `SOCKET_Generator` / actor root | (0,0,0); **geometry offset** so the visual centre is at Z = +1040 cm |
| Bounds | X,Y ±744; Z −50 .. +1918 | approx X ±328, Y ±298, Z +717 .. +1363 (local) |
| Size | 1488 × 1488 × 1968 cm | ~655 × 596 × 646 cm |
| Evaluated tris | **16,512** | **6,040** |
| Raw (unapplied) | 3,938 verts, 3,600 faces | 1,180 verts, 1,268 faces |
| Bevel-weighted edges | 1,608 | 760 |
| Non-manifold / degenerate | **0 / 0** | **0 / 0** |
| Custom normals | yes (`mesh_smooth_type=OFF`) | yes |
| Object scale | (1, 1, 1) | (1, 1, 1) |

Max pylon radius 744 cm is well inside the mothership's r = 1800 cm keep-clear cylinder (Z 0..4000 cm). The generator is a ~15 m × 20 m hero, not a 18 m sphere-filling dome.

### `mesh_report.py` (render mesh only, Blender reimport in metres)

Reimport bbox is in metres (D4 note: Blender's FBX importer displays metres; Unreal with `UnitScaleFactor=100` and import scale 1.0 receives centimetres).

```
SM_DominionShieldGenerator: verts=8642 tris=16512 nonman=0 boundary=0 wire=0 degen=0
  dihedral flat 36.2%  wobble 36.2%  hard 27.6%
  bbox (-7.44, -7.44, -0.5) .. (7.44, 7.44, 19.18)  scale=(1,1,1)
  custom_normals=True  uv=['UVMap']

SM_DominionShieldGeneratorCore: verts=3132 tris=6040 nonman=0 boundary=0 wire=0 degen=0
  dihedral flat 37.7%  wobble 40.0%  hard 22.3%
  bbox (-3.276, -2.98, 7.172) .. (3.276, 2.98, 13.628)  scale=(1,1,1)
  custom_normals=True  uv=['UVMap']
```

The "wobble" bucket (1–25° dihedral) is dominated by the 48-segment lathe and ring tubes: adjacent facets meet at 7.5°, which is *supposed* to shade-smooth under `smooth_by_angle(30°)`. It is not messy triangulation. Hard edges (≥35–40°) carry the live Bevel.

### Units / export

- Authored in metres, Unreal axes (`+X` forward, `+Y` starboard, `+Z` up).
- FBX: `apply_unit_scale=True`, `apply_scale_options=FBX_SCALE_UNITS`, `axis_forward='-Z'`, `axis_up='Y'`, triangulated, tangents, `mesh_smooth_type=OFF` (preserves custom / weighted normals). Same convention as Dominion hull r2 / Bastion / Lancer.
- **`UnitScaleFactor = 100`, `OriginalUnitScaleFactor = 100`.** Unreal import uniform scale **1.0**.
- Mapping: Blender `(x, y, z)` m → Unreal `(100x, −100y, 100z)` cm. The pylon is radially symmetric, so the Y sign flip does not affect seating.
- glTF (`.glb`) is a metre, +Y-up preview; it embeds an OpenGL-green copy of N.

## Shading (live, unapplied in the `.blend`)

- `shade_smooth` + `set_sharp_from_angle(30°)`
- Bevel, weight-limited, 2 segments, width 7 cm (pylon) / 4 cm (core), `harden_normals`, `clamp_overlap`, `MITER_ARC`. Weights on edges meeting the length / area / dihedral gates in `source/build_shield_generator.py`.
- Weighted Normal, `FACE_AREA`, `keep_sharp`.
- Official blends: `SM_DominionShieldGenerator.blend` (pylon + UCX + `SOCKET_Core`) and `SM_DominionShieldGeneratorCore.blend` (core only). One `.blend` per exported FBX. Modifiers stay live.

## Textures (`tex_stats.py` on the shipped 2K)

2K PBR atlases. DirectX `N` (green = 255 − OpenGL G). ORM is AO / Rough / Metallic. 4K masters in `source/textures_4k/`. OpenGL normals (`*_N_OpenGL.png`) are for the Blender preview material and the glTF.

| Map | Notes (2K) |
|---|---|
| `T_DominionShieldGenerator_BC.png` | Gunmetal / crimson / banner / metal. Lum p50 = **sRGB 77** (target 60–90). p1/5/25/50/75/95/99 = 26/28/66/77/95/136/255. `<40` = 11.6% (recess + metal tiles, not the armour field). |
| `T_DominionShieldGenerator_N.png` | DirectX. Tilt `<2°` 89.7%, `<6°` 93.0%, `6–24°` 7.0% (panel seams, vents, trident relief). meanRGB (128, 128, 255). |
| `T_DominionShieldGenerator_ORM.png` | Packed AO / Rough / Metallic. |
| `T_DominionShieldGenerator_E.png` | Amber strips, white, cyan emitter tips. Mostly black (p50 = 0); emissive tiles are small on purpose. |
| `T_DominionShieldGeneratorCore_BC.png` | Cyan energy + rib metal. Lum p50 = 114 (the core is *meant* to be brighter than armour). |
| `T_DominionShieldGeneratorCore_N/ORM/E.png` | Matching core atlas. E is hot on the energy tiles. |

## Collision and sockets

`UCX_SM_DominionShieldGenerator_00..42` (43 convex hulls):

- 00–02: `base_ring`, `collar`, `stump_cap` (the ShowDestroyed stump)
- then per-pylon: `buttress_foot_*`, `pylon_*_low/mid/up`, `prong_*_a/b` (six of each)
- 39–42: `ring_lower`, `ring_equator`, `ring_upper`, `ring_crown`

Hulls stay outside the open lantern so the visual cradle is not filled with a solid collider. Core has no UCX (`Core` is `NoCollision` in C++).

`SOCKET_Core` is an arrows empty, parented to the pylon, at **(0, 0, 11.0) m = (0, 0, 1100) cm**. After the C++ +60 cm offset the core mesh occupies that point. Use it for hit sparks / GeneratorExplosion attach.

## How to rebuild

```bash
# 2K (+ 4K masters)
python3 source/make_textures.py  /tmp/dsg_r1/tex2k  source/textures_4k

# meshes, blends, FBX, GLB
blender -b --factory-startup --python source/build_shield_generator.py -- /tmp/dsg_r1/tex2k  <out_dir>

# studio HDRI contact renders (opens the workshop blend that holds both meshes)
blender -b <out_dir>/_workshop.blend --python source/render_verify.py -- <renders_dir> 28
python3 source/make_contact_sheet.py <renders_dir> contact_sheet.png <out_dir>/build_report.json
```

Imports: static mesh, uniform scale 1.0, `+X` forward, `+Z` up, combine `UCX_` as custom convex collision, sockets from empties. Assign `T_*_BC` (sRGB), `T_*_N` (normal map, do **not** flip green — it is already DirectX), `T_*_ORM` (Masks, sRGB off), `T_*_E` (sRGB) on `M_DominionShieldGenerator` / `M_DominionShieldGeneratorCore`.

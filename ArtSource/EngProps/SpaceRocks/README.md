# SpaceRocks kit (engineering-made head-start props)

Four asteroids and three debris shards for the asteroid fields of the space level, built headless in
Blender 5.2.2 from seeded code on Nov 12, 2026. Requested by the engineering lead (MESSAGEBOARD.md
R-015); the art director reviews them afterwards and may restyle or replace them. Nothing here is
imported into Unreal yet, and nothing was written under `Content/`.

**Stage reached (AAA.md): technically verified** in Blender and by FBX round trip. Not presentation
approved (that is the art director's call), not integrated, not seen in the engine, not played.

![contact sheet](review/SpaceRocks_contact_sheet.png)

## Sizing contract

Gameplay scatters asteroids as instances of one mesh and scales each instance, today using the
engine's unit sphere (100 cm diameter, pivot at the centre). **Every mesh in this kit fits a 100 cm
diameter bounding sphere with its pivot at the centre of that sphere**, so it can replace the sphere
with the same scale values. The sphere is the minimum enclosing sphere of the game mesh; the mesh
touches it and never crosses it. Measured after export (FBX re-imported into an empty scene):
100.0 cm on all seven, pivot at the origin, object scale 1.

An elongated mesh only touches the sphere at its ends, so the gameplay sphere is larger than what
you see across its short axes (`SM_Asteroid_02` is about 100 x 48 x 40 cm inside its 100 cm sphere).

## What is in the kit

| Asset | Shape | Triangles | Textures | Bounding sphere after export | Collision |
|---|---|---|---|---|---|
| `SM_Asteroid_01` | chunky and faceted: angular block, a second block fused on, stepped ledges (blue basalt) | 3,000 | 1024: BC, N, ORM | 100.0 cm | 1 convex UCX, 32 verts |
| `SM_Asteroid_02` | elongated tapering shard, stepped notch, cyan crystals (rust) | 2,908 | 1024: BC, N, ORM, E | 100.0 cm | 1 convex UCX, 30 verts |
| `SM_Asteroid_03` | rounder, cratered: one giant crater with a central peak, three shear planes (ochre) | 3,200 | 1024: BC, N, ORM | 100.0 cm | 1 convex UCX, 31 verts |
| `SM_Asteroid_04` | fractured: deep wide zig-zag cleft with a side crack, violet crystals in the cleft (violet slate) | 3,078 | 1024: BC, N, ORM, E | 100.0 cm | 1 convex UCX, 30 verts |
| `SM_SpaceDebris_01` | flat slab shard | 420 | 512: BC, N, ORM | 100.0 cm | none |
| `SM_SpaceDebris_02` | long splinter | 380 | 512: BC, N, ORM | 100.0 cm | none |
| `SM_SpaceDebris_03` | blocky wedge chunk | 440 | 512: BC, N, ORM | 100.0 cm | none |

Each `<AssetName>/` folder holds:
- `<AssetName>.blend`: the game mesh with its baked preview material `M_<Asset>_Rock`, the `UCX_`
  hull as a child (asteroids), and the hidden bake sources `SRC_<Asset>_High` (50k-185k triangles,
  procedural material) and `SRC_<Asset>_Crystals` in the collection `SpaceRocks_BakeSource`.
- `<AssetName>.fbx`: one render mesh, triangulated, one material slot, UCX hull named
  `UCX_<AssetName>_00`, hard edges and face-weighted normals stored as custom normals.
- `T_<Asset>_BC.png` (sRGB), `T_<Asset>_N.png` (**DirectX, do not flip green**),
  `T_<Asset>_ORM.png` (R = AO, G = roughness, B = metallic, linear), and `T_<Asset>_E.png`
  (emissive colour, sRGB) on `02` and `04` only.
- `<AssetName>.manifest.json`: the export skill's manifest, plus `fbx_relative`,
  `sizing_contract`, `emission_strength` and `texture_size`.

`review/` holds, per asset, a clay sheet, a material sheet (baked maps), a wireframe sheet and
`<AssetName>_mesh_health.json`; plus `SpaceRocks_contact_sheet.png` (all seven at the same scale,
128 cm frame, one sun on black: front 3/4, back 3/4, silhouette) and `export_verification.json`.

## Notes for the import

- **Emissive is a separate map** so it can be turned off. Multiply it by `EmissiveStrength`;
  the bake used 2.0. With emissive off the crystals are still saturated cyan / violet in the
  base colour. The emissive map also carries a faint halo on the rock around each cluster.
- Emissive colours: cyan `#2EE6FF` (`02`), violet `#A24DFF` (`04`). Bright crystal surface is
  5.0% of the mesh surface on `02` and 5.9% on `04` (the art rule is 5% or less).
- The manifest's `fbx` field is the **absolute path in the worktree that built it**. Use
  `fbx_relative`, or re-run the build, before feeding the folder to `ue_import_assets.py`.
- Nanite is not recommended by the manifest (under 20k triangles); it is the importer's choice.
- At an instance scale of 300 m one texel of a 1024 map is roughly half a metre across. The maps
  carry the broad colour and the medium forms; close-up surface grain needs a tiling detail
  normal in the Unreal material (not part of this kit).
- The UCX hull is built from 32 support directions, so it is slightly inside the true hull: the
  render mesh pokes out of it by up to 6.8 cm (of 100). Gameplay treats asteroids as spheres.

## How it was made (and how to regenerate it)

Three scripts in this folder; they call the repo's skill helpers and change none of them
(`procedural_rocks.py`, `hardsurface.py`, `bake_texture_set.py`, `export_for_unreal.py`,
`review_renders.py`, `asset_report.py`, `assembly_check.py`). Run from the repo root, PowerShell:

```powershell
$blender = "C:\Program Files\Blender Foundation\Blender 5.2\blender.exe"
foreach ($a in "SM_Asteroid_01","SM_Asteroid_02","SM_Asteroid_03","SM_Asteroid_04",
               "SM_SpaceDebris_01","SM_SpaceDebris_02","SM_SpaceDebris_03") {
  & $blender --background --factory-startup --python ArtSource\EngProps\SpaceRocks\build_spacerocks.py -- --asset $a
}
& $blender --background --factory-startup --python ArtSource\EngProps\SpaceRocks\render_contact_sheet.py
& $blender --background --factory-startup --python ArtSource\EngProps\SpaceRocks\verify_exports.py
```

About 15 to 100 seconds per asset on CPU. `--stage shape --wip-dir <dir>` builds the mesh and a clay
sheet only (no bake, nothing saved), for iterating on a silhouette. Shapes, palettes and crystal
placement are the `SPECS` table at the top of `build_spacerocks.py`; every asset has its own seed.

Per asset, `build_spacerocks.py`:
1. builds one or more faceted "lobes" (stretched, lumpy icosphere with exact plane cuts; craters
   on `03`), cuts wedges out of them (wide = step notch, narrow and deep = the cleft), fuses them
   with a voxel remesh and adds flake scars (a continuous Manhattan-Voronoi displacement);
2. decimates a copy to the game mesh, unions the crystals into it (exact booleans, stays
   watertight), and normalises everything to the 100 cm sphere;
3. marks hard edges at 32 degrees, unwraps (smart project), splits the UV islands at every hard
   edge, detaches any face that overlaps another, re-packs, applies face-weighted normals;
4. bakes BC / N / ORM / E from the high-poly source (cage sized from the measured distance
   between the two meshes), fills texels the bake rays missed, and tints baked cavities cooler;
5. adds the UCX hull, saves the `.blend`, runs the checks and review renders, exports the FBX.

## What was verified

Measured in Blender (`review/<Asset>_mesh_health.json`), all seven pass:
- triangles inside budget (asteroids 1,500-3,500; debris 200-600), all faces triangles;
- one material slot; closed and manifold; no flipped or inside-out faces; no loose geometry;
- no floating parts (`assembly_check.py`); `asset_report.py`: 0 errors, 0 warnings;
- bounding sphere 100.0 cm about the pivot;
- UVs inside 0..1 and no overlap (every UV triangle rasterised at 2048 px).

Measured on the exported files (`verify_exports.py`, FBX re-imported into an empty scene;
`review/export_verification.json`), all seven pass: mesh name, triangle count, triangles only,
closed manifold, outward normals, one connected piece, bounding sphere 100.0 cm, pivot at the
origin, scale 1, one UV layer inside 0..1, one material slot, custom normals present, UCX hull
name / convexity / closedness (asteroids) or absence (debris), texture pixel sizes from the PNG
headers.

Looked at: every clay and material sheet and the contact sheet, by the agent that built the kit.

Fresh-eyes review (a separate agent that saw only the brief and the sheets), one round, on the
first complete version. It scored most assets 2-3 out of 5 per category and `03` 1-2, and named
three fixes. All three were then made: `03` got three shear planes, a giant crater that bites the
outline and fewer, more varied craters; base colours were darkened and split into four hue
families with the edge highlight thinned and broken up; `01`'s fused block was pushed proud,
`04`'s cleft was opened to a wide V, and the crystals were enlarged (then trimmed back to stay near
the 5% emissive rule). **The fixed version has not been re-scored by a fresh reviewer.**

## What was not verified, and what I am not happy with

- **Not imported into Unreal.** Scale, pivot, collision pairing, tangent basis and the look under
  the game's sun, bloom and tonemapper are unchecked in the engine. The skill's calibration-arrow
  import has not been run for this project by this task.
- **Not seen at game scale.** Nothing here was looked at several hundred metres across from a
  chase camera; the half-metre texel at that size is the main risk.
- The style is soft: the voxel remesh rounds fracture edges by about 6 mm on the 100 cm mesh and
  the base colour is broad painted tone. It reads as stylised rock, not as crisp fractured stone.
- `SM_Asteroid_03` is still essentially a ball with one large bite; its giant crater floor bakes
  dark (cavity tint) and reads as a hole from some angles.
- `SM_SpaceDebris_01` and `_02` are plain (a slab and a splinter); at 400 triangles they carry
  little detail and were called "bar of soap" and "fingertip" by the reviewer before the fixes.
- `04`'s crystals sit inside the cleft and are hidden from about half the viewing angles.
- Thin triangles (longest edge squared over twice the area above 25): 25 / 43 / 13 / 102 on
  `01`-`04`, mostly at the crystal union seams on `02` and `04`. They are valid and closed, but
  not tidy topology.
- The hard-edge UV split makes many small islands (61 to 234 per asteroid); UV coverage is
  47-60%.
- `texels_missed` in the health JSON counts every near-black texel, including the empty space
  between islands, so it overstates real bake misses.
- No `CREDITS.md` or `DEVLOG.md` entry was written (outside this task's folder). For the lead:
  tools were Blender 5.2.2 and Claude Code; no third-party or downloaded assets; original shapes.

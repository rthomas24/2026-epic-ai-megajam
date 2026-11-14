# Dominion Docking Bay — plans_r1 (issue #54, board R-043)

Route C procedural. Blender 4.5.3 LTS + matplotlib + numpy. **No AI 3D generator, no fal spend.**
Branch `grok/assets-dominion-docking-bay-r1`. Reference: issue #54 board
(`ArtSource/Reference/DominionDockingBay/Docking Bay Dominion Modeling Reference.png`, commit `6bfaea46`).

## Scope split (per R-043)

| Who | What |
|---|---|
| **Builders (Unreal)** | The walkable interior, assembled from `ArtSource/EngProps/DominionInterior` (400 cm grid, 600 cm storeys). The plans and the blockout here are the measured layout to follow; nothing in `blockout/` is meant to ship as a mesh. |
| **Grok (this folder)** | Measured orthographic plans on the kit grid with piece names and 180 cm figures; a blockout `.blend` assembled from the real kit FBXs; the **non-walkable** bay-mouth pieces (two doors + tractor emitter) as game meshes; a trim sheet; renders + contact sheet. |

## Files

| Path | What |
|---|---|
| `dockbay_layout.json` | Single source of truth: 799 kit placements (mesh, x, y, z, yaw, group) + mouth / doors / glide / pads / force field / figure numbers. Made by `source/dockbay_layout.py`. |
| `plans/DB_Plan_Top.png` | Top plan, floor z 0, ceilings removed, every kit piece by pivot, 400 cm grid, hull X on the top axis, legend of piece names, 20 m scale bar, figure. |
| `plans/DB_Section_Side.png` | Section A-A (y = 0): storeys, mouth, glide axis, C++ vs recommended glide end, 180 cm figures, hull Z on the right axis. |
| `plans/DB_Section_Front.png` | Section B-B (x = -6800, looking in): mouth 5000 x 3500 inside the 5200 x 3600 kit pocket, door slabs open/closed, hangar opening, threshold frame, 180 cm figures. |
| `plans/DB_Hull_Mouth_Cone.png` | Hull r2 side + top slices through the bow at SOCKET_Bay, and the ATDDockingBay approach cone as the C++ builds it. |
| `blockout/DominionDockingBay_Blockout.blend` | 818 objects: linked duplicates of the actual kit meshes at scale 1 (sources in a hidden `KitSource` collection), threshold marking, force-field plane, 180 cm figures, ship-footprint proxies, lights. Re-run `source/build_blockout.py` after editing the layout. |
| `mesh/DominionBay_Exterior.blend` | Editable source for the three exterior pieces: live objects with **Bevel (weight-limited) + Triangulate (n-gons) + WeightedNormal** modifiers, hidden unmodified `SRC_*` copies, `UCX_*` collision. |
| `mesh/SM_DominionBay_Door_L.fbx`, `_Door_R.fbx`, `SM_DominionBay_TractorEmitter.fbx` | Game meshes (+ UCX). |
| `mesh/exterior_report.json` | Tris, bounds, bevel edges/width, UCX names. |
| `textures/T_DominionBay_Trim_{BC,N,ORM,E}.png` | 2K tiling trim sheet shared by the three pieces. |
| `review/*.png` | Renders. `contact_sheet.png` = everything on one page. |
| `source/*.py` | All scripts (below). |

## Coordinates

Interior course-local frame of T5's `TDDominionInteriorCourse.inl`: +X deeper into the ship, floor top z = 0, 400 cm grid.
Hull mapping (Dominion local, hull r2): **hull_x = 22800 − x, hull_y = −y, hull_z = z − 3750**. So the mouth plane
x = −7200 is hull X 30000 (SOCKET_Bay), the pocket floor is the mouth sill (hull Z −3750), and the pocket back wall
x = −1200 is the r2 aft bulkhead at hull X 24000.

## Key dimensions (cm)

| | Value | Matches |
|---|---|---|
| Mouth | **5000 W x 3500 H**, centre (−7200, 0, 1750) = hull **SOCKET_Bay (30000, 0, −2000)**, facing hull +X | hull r2 bow opening; `2 x DoorHalfWidth` (2500) wide, `2 x 0.7 x DoorHalfWidth` high |
| Glide pocket (walkable kit) | x −7200..−1200 (**6000 deep**, 15 cells) x **5200 wide** (13 cells, 5140 clear) x **3600 clear** (6 storeys) | hull pocket 6000 x 5000 x 3500 (kit rounds up one cell / one storey) |
| Hangar (T5, unchanged footprint) | 2400 x 2800 x 1200 clear; X0 end wall removed so it opens onto the pocket (2800 x 1200 opening) | T5 as built |
| Pads | A (−200, 0) T5 ship pad; B (−2600, −1600), C (−2600, 1600) dressing; 1200 x 1200 | kit `SM_DI_Hangar_Pad` |
| Docked ship footprint | 524 x 488 x 230 on Pad A, nose +X | largest squad ship (Lancer / Bastion) |
| Pilot start / HangarDoor | (−600, 0) / (1200, 0), 300 x 400 clear | T5 |
| Force field | BlockingVolume (pawn only) at x −7180, |y| 2600, z 0..3600 | — |
| **Bay threshold** | 400 cm hazard-chevron floor band x −7200..−6800 across the full 5000; emissive strip at x −6800; emissive jamb (|y| 2460..2500) + header (z 3460..3500) strips; hazard sill band on both door slabs | Lead N-061 ("unmarked" threshold) |
| Doors | each **300 x 2500 x 3500**, outer face at local X −20, X −320..−20; closed centres Y ∓1250, open ∓3750 (span |Y| 2500..5000, inside the hull) | ATDDockingBay `DoorHalfWidth·(0.5+Open01)`, door depth 300 |
| Tractor emitter | **900 dia x 186 deep**, pivot = mount-face centre, +X = beam; at hull **(31200, 0, 1400)** on the brow | hull r1/r2 brow |
| 180 cm figures | plans (side/front/top), blockout (3), exterior renders, studio renders | — |

## Agreement with ATDDockingBay / ATDDominion (C++) — mismatches called out

1. **Actor placement (biggest one).** `ATDDominion::BeginDockingFinale` spawns the bay at local
   `(Bounds.Max.X, 0, lerp(Min.Z, Center.Z, 0.6))`. With hull r2 that is **(33400, 0, −2933)**: the tip of the
   brow/mandibles, **34 m in front of the real mouth and 9.3 m low**, instead of `SOCKET_Bay (30000, 0, −2000)`.
   The glide then ends at hull X 27400 instead of inside the pocket, and the doors/frame float in front of the bow.
   `GetEscapeStart` uses the same formula (+40 m). **Fix (builders):** use the hull's `SOCKET_Bay` for the bay actor
   and the escape start.
2. **GlideDepth 6000 = the full pocket.** Measured from SOCKET_Bay, the glide end lands exactly on the aft bulkhead
   (hull X 24000) — the ship's nose (524 cm long) is in the wall. **Recommend GlideDepth 5000** (nose 7.4 m clear,
   glide end at interior x −2200, level with the Pad B/C row) or move the glide end onto Pad A (needs an interior hand-off anyway).
3. **DoorHalfWidth 2500: agrees.** Mouth width 5000, door height 2 x 0.7 x 2500 = 3500, door depth 300, open travel
   to |Y| 5000: all match hull r2's 5000 x 3500 opening and its door pockets.
4. **Placeholder frame.** `Frame` is scaled to 150 x 6500 x 4550 (2.6 x DoorHalfWidth, 2.6 x H) — with the art hull it
   should be hidden (the hull r2 mouth frame is modelled); otherwise it covers the bow.
5. **ConeDepth 300000 (3 km), ConeHalfAngle 35°, MouthDistance 30000 / MouthRadius 6000.** No conflict with the
   geometry; the lock box from the C++ actor reaches X 63400 (shown in `DB_Hull_Mouth_Cone.png`). Guide lights are
   placed every ConeDepth/13 = 230 m at `±(DoorHalfWidth + 0.08 X)`.
6. **Kit vs hull pocket.** The kit pocket is 5200 x 3600 (grid) vs the hull's 5000 x 3500: 100 cm per side and 100 cm
   of headroom extra, all hidden behind the hull mouth frame.
7. **Door mesh pivot.** `DoorLeft/Right` sit at the actor origin with `RelativeLocation(0, ∓DoorHalfWidth·(0.5+Open01), 0)`;
   the door FBX pivot is the slab centre with the outer face 20 cm behind X 0 (avoids z-fighting the hull bow face at
   X 30000). Set `RelativeScale3D` to 1 when the art mesh is assigned (the placeholder scale is 3 x 25 x 35).

## Exterior pieces — mesh metrics (`/tmp/audit-scripts/mesh_report.py`, FBX reimport, metres)

```
===== SM_DominionBay_Door_L.fbx objects: [('SM_DominionBay_Door_L', 1.0), ('UCX_SM_DominionBay_Door_L_00', 1.0)]
  obj SM_DominionBay_Door_L verts 1232 polys 2376 uv ['UVMap'] mats ['M_DominionBay_Trim'] custom_normals True
  obj UCX_SM_DominionBay_Door_L_00 verts 8 polys 12 uv [] mats [] custom_normals True
  tris 2388 ngons 0 welded islands 19 non-manifold edges 2 degenerate 0
  bbox min (-3.2, -12.5, -17.5) max (-0.2, 12.5, 17.5) size (3.0, 25.0, 35.0)
  edge dihedral: flat(<1deg) 18.5%  wobble(1-25deg) 58.3%  hard(>25deg) 23.3%
===== SM_DominionBay_Door_R.fbx objects: [('SM_DominionBay_Door_R', 1.0), ('UCX_SM_DominionBay_Door_R_00', 1.0)]
  obj SM_DominionBay_Door_R verts 1232 polys 2376 uv ['UVMap'] mats ['M_DominionBay_Trim'] custom_normals True
  obj UCX_SM_DominionBay_Door_R_00 verts 8 polys 12 uv [] mats [] custom_normals True
  tris 2388 ngons 0 welded islands 19 non-manifold edges 2 degenerate 0
  bbox min (-3.2, -12.5, -17.5) max (-0.2, 12.5, 17.5) size (3.0, 25.0, 35.0)
  edge dihedral: flat(<1deg) 18.5%  wobble(1-25deg) 58.3%  hard(>25deg) 23.3%
===== SM_DominionBay_TractorEmitter.fbx objects: [('SM_DominionBay_TractorEmitter', 1.0), ('UCX_SM_DominionBay_TractorEmitter_00', 1.0), ('UCX_SM_DominionBay_TractorEmitter_01', 1.0)]
  obj SM_DominionBay_TractorEmitter verts 2336 polys 4636 uv ['UVMap'] mats ['M_DominionBay_Emitter'] custom_normals True
  obj UCX_SM_DominionBay_TractorEmitter_00 verts 32 polys 60 uv [] mats [] custom_normals True
  obj UCX_SM_DominionBay_TractorEmitter_01 verts 24 polys 44 uv [] mats [] custom_normals True
  tris 4740 ngons 0 welded islands 13 non-manifold edges 0 degenerate 0
  bbox min (0.0, -4.75, -4.75) max (1.86, 4.75, 4.75) size (1.86, 9.5, 9.5)
  edge dihedral: flat(<1deg) 37.0%  wobble(1-25deg) 49.7%  hard(>25deg) 13.4%
```

Notes: the emitter bbox above includes its UCX hull (r 475); the render mesh is 900 dia (`exterior_report.json`).
The doors' 2 non-manifold edges are on the overlapping chevron prism (separate shells, closed for rendering).
Live polish: edges ≥ 40° and ≥ 3 cm get bevel weight 1; Bevel width 5 cm (doors, 264 edges) / 4 cm (emitter, 752
edges), 2 segments, harden normals, clamp overlap, arc miter; WeightedNormal FACE_AREA + keep sharp; sharp from 30°.

### Units / export

- Built in Unreal cm, mirrored to Blender metres (Blender Y = −Unreal Y), scale 1 on every object.
- FBX: `apply_unit_scale`, `FBX_SCALE_UNITS`, `axis_forward −Z`, `axis_up Y`, triangulated temp copy with modifiers
  applied, `mesh_smooth_type OFF` (custom normals kept), tangents. **UnitScaleFactor = 100** (checked in the binary
  header). Import into Unreal at uniform scale 1.0.
- Materials: `M_DominionBay_Trim` (doors, red-orange `GlowColor`) and `M_DominionBay_Emitter` (cyan `GlowColor`), both
  on the same trim textures. N is **DirectX** (Unreal green).

## Trim sheet (`/tmp/audit-scripts/tex_stats.py`, 2K)

```
T_DominionBay_Trim_BC.png RGB 2048x2048 lum p1/5/25/50/75/95/99=[ 38.  40.  75.  79.  88. 102. 220.] | <20: 0.0% <40: 5.1% >235: 0.0% | mean [85. 77. 82.]
T_DominionBay_Trim_E.png L 2048x2048 lum p1/5/25/50/75/95/99=[  0.   0.   0.   0.   0.   0. 255.] | <20: 98.0% <40: 98.0% >235: 2.0% | mean [5. 5. 5.]
T_DominionBay_Trim_N.png RGB 2048x2048 NORMAL tilt<2deg 80.0% | <6deg 80.1% | 6-24deg 5.9% | >24deg 14.0% | meanRGB [128. 128. 253.]
T_DominionBay_Trim_ORM.png RGB 2048x2048 lum p1/5/25/50/75/95/99=[ 81. 117. 124. 134. 147. 159. 163.] | <20: 0.0% <40: 0.0% >235: 0.0% | mean [248. 100. 154.]
```

BC median **sRGB 79** (dark metal target ≥ 55; the armour field was lifted from #16171B to #4C5058). Bands (px from
top): armour 0–640, **hazard chevrons 640–896 (threshold marking)**, grille 896–1152, gunmetal 1152–1408, crimson
1408–1536, emissive strip 1536–1664, small panels 1664–2048. The N `>24°` share is the ribbed grille band.

## Renders

Cycles CPU, AgX Medium High Contrast, **Blender's bundled neutral `studio.exr` HDRI** (lighting; the camera sees a
flat grey/black backdrop). `ext_*` = hull r2 (`ArtSource/EngAI/Ships/DominionMothership/mesh_r2/SM_DominionMothership.blend`,
merged PR #72) + the pieces at SOCKET_Bay, closed and open. `pc_*` = the pieces alone in the studio with a 180 cm
figure. `bo_*` = the blockout (ships on the pads are render-only stand-ins from the squad/Null GLBs, not saved).

## Rebuild

```
P=ArtSource/EngAI/Props/DominionDockingBay/plans_r1; B=/path/to/blender
python3 $P/source/dockbay_layout.py $P/dockbay_layout.json
python3 $P/source/make_trim_textures.py $P/textures
(cd $P && $B -b --factory-startup --python source/build_bay_exterior.py -- mesh textures)
$B -b --factory-startup --python $P/source/build_blockout.py -- <repo> $P
python3 $P/source/make_plans.py $P <hull_slices_r2.json> <hull_measure_r1.json> <hull_measure_r2.json>   # hull slices from SM_DominionMothership r1/r2
(cd $P && $B -b --factory-startup --python source/render_views.py -- exterior <repo> $PWD review <hull_r2.blend> 48)
(cd $P && $B -b --factory-startup --python source/render_views.py -- pieces <repo> $PWD review "" 64)
(cd $P && $B -b --factory-startup --python source/render_views.py -- interior <repo> $PWD review "" 40)
python3 $P/source/make_contact_sheet.py $P <reference.png>
python3 $P/source/make_manifest.py $P
```

# Outpost: the survey outpost (Verdance research outpost, Cinder survey station)

> **Superseded for everything the pilot walks on (Ryan, Nov 13 evening).** The walkable sites are now built from the
> engineering modular kit: see [`../Kit/README.md`](../Kit/README.md). `BP_Outpost_Site_*`, `BP_Relay`,
> `BP_Relay_Awake` and `BP_Wreck` keep their names and now hold kit pieces; the Rodin relay and wreck below remain
> as sunk exterior dressing, and the Rodin cabin and console are no longer used. This file records the Rodin rounds
> (history in `docs/eng/ART-HISTORY.md`; last committed state `fdccac29`).
>
> **Relay r2 (Rodin, Nov 13):** stair block re-sloped (the 46 cm top riser became a 20-degree carved slope, the
> flight stretched 1.25 x so its nosings fall at 19.3 degrees), landing + ramp collision; walked in `-game` from the
> ground up the stairs onto the court (feet within 5 cm of the collision everywhere); captures in
> `Relay/review/game/`. Emissive on `RelayPower` squared x `GlowStrength` 40 (dormant 0.15 -> 0.9, awake 40): the
> linear x 10 version looked the same dormant and awake in the game. Site manifest `export/Relay_Site.manifest.json`.
> **Wreck r2 (Rodin):** a fallen-plate ramp on both sides; walking up one ramp found the r1 deck hull (built from 8
> support directions) had lost two top corners and sank the pilot to the waist on the +Y half; the hull is exact in
> r2 (`Wreck/scripts/wreck_build.py`). Site manifest `export/Wreck_Site.manifest.json`. Both sites are replaced by
> the kit versions.

Grok Bot's Rodin outpost and console (`ArtSource/EngAI/Props/VerdanceOutpost/mesh_r1`, read only, licence and prompts
in its `SOURCE.md`) made walkable and imported. Nothing here is a redesign: the mesh, UVs and textures are as
delivered except for the changes listed below. Nicholas's reference board:
`ArtSource/Reference/VerdanceResearchOutpost/` (branch `origin/nick/refs-verdance-research-outpost`).

**Stage reached (AAA.md): technically verified, and walked in the running game by an agent** (injected input on
`L_Planet_Verdance`, transient spawn, nothing saved). Not presentation approved, not integrated (no site spawns it).

## What was changed on the delivered mesh (`scripts/op_build.py`)

| Change | Why | Numbers |
|---|---|---|
| Ramp islands stretched 2.25 x along X about the hinge, normals corrected | The delivered ramp was 38.0 degrees | 19.15 degrees; run 385 cm, rise 141 cm; toe at X = 626 |
| Bay floor pad flattened, dark skirting added at the three wall feet | The pad sloped from 166 (back) to 153 cm (front) | Flat at Z = 153; 13 cm step from the deck (Z = 140) |
| A pile under each of the seven feet and the ramp toe, 150 cm down | Nothing floats when the ground falls away | Hidden on flat ground |
| Lights: a beacon on the antenna tip, two bars under the awning edge | No emissive in the delivery; nothing to find it by at night | Emissive through `T_Outpost_Cabin_E` |
| 11 collision hulls (cabin), 2 (console) | None in the delivery | See below |

Not changed: 177 loose shells, 18 zero-area faces, and about 50 two-triangle detail plates that hover 3 to 5 cm off
the frames (not visible at 3 m). The exporter welds the 6,360 seam-split vertices.

## Unreal assets (`/Game/TriadDrift/Gameplay/Props/Outpost/`)

| Asset | What |
|---|---|
| `SM_Outpost_Cabin` | 8,219 tris, 1026 x 770 x 824 cm (bounds -400..626, -385..385, -150..674), pivot on the ground under the deck centre, +X = open bay and ramp, 11 hulls |
| `SM_Outpost_Console` | 6,628 tris, 87 x 260 x 150 cm, pivot bottom-centre, screens face +X, 2 hulls |
| `MI_Outpost_Cabin_Verdance` / `_Cinder`, `MI_Outpost_Console_Verdance` / `_Cinder` | The two looks, by material only |
| `Materials/M_Outpost` | The one master (parameters below) |
| `Textures/T_Outpost_Cabin_BC`, `_N` (2K, as delivered), `_ORM`, `_M`, `_E` (1K) | |
| `Textures/T_Outpost_Console_BC` (2K: the screens are only about 130 px wide in it), `_N`, `_ORM`, `_E`, `_M` (1K) | |
| `BP_Outpost_Site_Verdance`, `BP_Outpost_Site_Cinder` | Cabin + console as one actor (from `export/Outpost_Site.manifest.json`) |
| `Pipelines/PL_Outpost_StaticMesh` | Import pipeline |

`M_Outpost` parameters: `BaseColorTex`, `NormalTex`, `ORMTex`, `EmissiveTex`, `MaskTex` (R = accent paint: the yellow
hazard marks and the two ramp handrails; G = light panels); `Tint`, `Desaturate`, `PanelTint`; `AccentColor`,
`AccentAmount`; `SootAmount`, `SootColor`, `SootBase`, `SootHeight` (a soot gradient by height above the pivot);
`RoughnessScale`, `MetallicScale`; **`ScreenColor`, `ScreenPower`** (emissive = EmissiveTex x ScreenColor x ScreenPower).

- Console screens: `ScreenPower` 1.6 by default. To brighten: `CreateDynamicMaterialInstance(0)` on the console's
  component, `SetScalarParameterValue("ScreenPower", 6..10)`.
- Cabin lights (beacon, awning bars): `ScreenPower` 12, `ScreenColor` cyan on Verdance, amber on Cinder.
- Cinder: `AccentAmount` 1 with amber `AccentColor` (rails and hazard marks), `Tint` / `PanelTint` / `Desaturate` and
  `SootAmount` 0.75 for the sooty look.

## Placing it (T5): `export/Outpost_Site.manifest.json`

Site pivot = cabin pivot, on the ground. Console at site (-19, 81.5, 153), yaw 0. Interact point: site (76, 81.5, 153),
pilot facing yaw 180 (console-local (95, 0, 0)). The FBX socket `SOCKET_Interact` did not survive the Interchange
import: use the numbers. Do not sink the site and do not scale it; keep 3 m of flat ground in front of the ramp toe.

## Walkability as built (Unreal cm, site-relative)

| | |
|---|---|
| Deck | top Z = 140 (collision); X -400..228, Y -385..385 |
| Ramp | X 219..626, Y -4..144; 148 wide, 127 clear between the rails; 19.15 degrees; rails guarded to 110 cm by collision |
| Bay opening | 315 wide (Y -76..239), 246 clear under the ceiling, 254 under the lintel |
| Bay floor | flat at Z = 153, X -66..112 (178 deep); 13 cm step up from the deck |
| In front of the console | 87 cm of bay floor, 203 cm to the deck edge |
| Awning | underside Z 421..462 (281 cm and more above the deck) |

Collision (convex): deck slab; bay floor; cabin rear block (the hollow shell behind the bay is solid: there is no
door into it); two side blocks with the plan chamfers; bay roof; equipment box (down to the deck); awning; ramp wedge;
two rail guards. Console: body, tower.

## Files

```
Outpost/
  README.md
  Outpost_r2.blend                    both meshes with UCX_ children and SOCKET_Interact (Blender Y = -Unreal Y)
  scripts/op_common.py                source path, textured preview material, camera renders
  scripts/op_inspect.py, op_probe.py, op_probe2.py   read-only measurements of the delivery (review/*.txt, *.json)
  scripts/op_build.py                 the build: ramp, bay floor, piles, lights, collision, export
  scripts/op_textures.py              masks, 1K copies, the cabin emissive (system Python + Pillow)
  scripts/op_view.py, op_sheet.py     review renders and contact sheets
  scripts/ue_import_outpost.py        Unreal import + M_Outpost + the four instances (headless)
  scripts/ue_make_outpost_site.py     the two BP_Outpost_Site actors from the manifest
  scripts/op_game.py                  driver for the in-game walk test (console commands, position, captures)
  textures/                           derived maps
  export/SM_Outpost_*.fbx, *.manifest.json, Outpost_Site.manifest.json
  review/                             src_sheet.jpg (as delivered), r1_sheet.jpg, r2_sheet.jpg, build_r2.json,
                                      ue_import_report.json, walk_test.md, game/*.png (in-game captures)
```

Rebuild (from the repo root; set `OUTPOST_SRC` only if Grok Bot's folder is somewhere else):
`blender --background --factory-startup --python ArtSource/EngProps/Outpost/scripts/op_build.py -- --tag r2 --export`,
`python ArtSource/EngProps/Outpost/scripts/op_textures.py "" r2` (first argument: the source folder, or empty),
`powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Outpost\scripts\ue_import_outpost.py`,
`powershell -File Tools\td-python.ps1 -Script ArtSource\EngProps\Outpost\scripts\ue_make_outpost_site.py`.

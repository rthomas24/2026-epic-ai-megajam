# Squad player ships (engineering fallback)

Rift, Lancer and Bastion, code-built in Blender 5.2 (route C) by engineering because the art team's
versions could not land in time. They are **fallbacks**: gameplay looks up `DA_<Ship>_Visuals` in
`/Game/TriadDrift/Presentation/` first, and only when that is missing finds ours in
`/Game/TriadDrift/Gameplay/Characters/Squad/` (`UTDStatics::LoadPresentationAsset`). The art team's
assets replace these automatically the moment they exist.

## Rebuild

```
cd ArtSource/EngShips/Squad/scripts
blender --background --factory-startup --python sq_build.py -- --asset SM_Squad_Rift --tag r7     # also _Lancer, _Bastion
blender --background --factory-startup --python sq_silhouette.py -- --tag r7
powershell -File Tools\td-python.ps1 -Script ArtSource\EngShips\Squad\scripts\ue_import_squad.py   # from the repo root
```

- `sq_ships.py`: the three designs, plus the shared squad parts (canopy, nozzle, livery spine).
- `sq_lib.py`: the bake, check, review and export pipeline, copied from `NullFleet/scripts/nf_lib.py`.
  It renders squad review views (14 m and 30 m chase, 200 m) and runs a part-level weld.
- `sq_build.py`: the per-ship driver.
- `ue_import_squad.py`: the Unreal import, idempotent.
- Each `SM_Squad_<Ship>/` folder holds the `.blend`, the `.fbx`, the manifest and `T_Squad_<Ship>_BC|N|ORM|E.png` (2048).
- `review/` holds the clay and material sheets for every round (b1–b3 blockout, r1–r7) and the silhouette sheets.

## Conventions

- **Axes and units:** Unreal +X forward, +Z up, centimetres.
- **Pivot:** the bounding-box centre, so the hull sits centred on the 220 cm gameplay collision sphere.
  - The centre of mass sits 24–45 cm behind it (engines are heavy) and about 5 cm above it.
  - With a centre-of-mass pivot, the nose would reach 290 cm from the sphere centre (+32%). The brief
    allows the art to be up to +20%.
- **Material:** one slot, `MI_Squad_<Ship>`, from the master `M_TD_Squad`.
  - The master takes BaseColor, ORM (AO, roughness, metallic) and a DirectX normal map.
  - Emissive is `EmissiveTex × EmissiveStrength` (6; lowered from 8 after the T1 live run showed strong bloom against black space).
- **Collision:** UCX convex hulls are in the mesh, but gameplay uses only the sphere.

## Shared design language

- **Hull:** flowing superellipse sections with groove panel lines.
- **Canopy:** one canopy (130 × 50 × 30 cm at scale 1; Bastion uses scale 1.15). It is dark glossy glass with graphite sills and arches.
- **Livery:** a squad-colour dorsal spine runs behind the canopy, and every ship has a graphite belly.
- **Nozzle:** the same design on every ship: a ribbed graphite bell, a white collar with a squad-colour band, and a squad-colour glow core and lip around a small hub.
- **Colours:** pearl white `#E4E8EE` dominant, graphite `#30353D`, squad paint (Rift `#C40E9C`, Lancer `#0596D2`, Bastion `#E8860A`) and squad glow (`#FF2BD6`, `#3FE8FF`, `#FFB21F`).

## Per ship (Unreal cm, sockets relative to the pivot; +Y is the ship's right)

### SM_Squad_Rift
Trickster: a crescent wing swept forward and up, and a magenta portal ring between twin nose prongs.

| | |
|---|---|
| Size | 500 L × 494 W × 119 H |
| Triangles | 16,768 |
| Emissive | 3.8% of the surface |
| Muzzle_0 / Muzzle_1 | (252, −33, −39) / (252, 33, −39) |
| Engine | (−252, 0, −31) |
| Engine_0 / Engine_1 | (−252, −44, −31) / (−252, 44, −31) |
| Portal (ring centre) | (202, 0, −9) |
| Cockpit | (75, 0, 23) |
| WingTip_L / WingTip_R | (186, −243, 16) / (186, 243, 16) |

### SM_Squad_Lancer
Interceptor: a needle hull with an armoured ram prow (four ridges, keel blade and metal spike), four cheek cannons, short drooping wings with wingtip rocket rails, and a tall fin. One bright main engine and two dim small ones.

| | |
|---|---|
| Size | 524 L × 318 W × 164 H |
| Triangles | 16,388 |
| Emissive | 3.0% of the surface |
| Muzzle_0 / Muzzle_1 | (194, −40, −50) / (194, −40, −62) |
| Muzzle_2 / Muzzle_3 | (194, 40, −50) / (194, 40, −62) |
| Engine and Engine_0 | (−262, 0, −44) |
| Engine_1 / Engine_2 | (−232, −58, −52) / (−232, 58, −52) |
| Ram | (262, 0, −45) |
| Cockpit | (25, 0, 6) |
| WingTip_L / WingTip_R | (−214, −153, −71) / (−214, 153, −71) |

### SM_Squad_Bastion
Heavy: a broad hull between two missile pods, which have amber-tipped tubes and hatches. A raised dorsal deck carries the shield-dome emitter ring. Four engines in a row and twin chin cannons.

| | |
|---|---|
| Size | 524 L × 488 W × 145 H |
| Triangles | 27,838 |
| Emissive | 3.5% of the surface |
| Muzzle_0 / Muzzle_1 | (264, −36, −59) / (264, 36, −59) |
| Engine | (−260, 0, −30) |
| Engine_0 / Engine_1 | (−260, −46, −28) / (−260, 46, −28) |
| Engine_2 / Engine_3 | (−228, −162, −34) / (−228, 162, −34) |
| Dome | (−73, 0, 60) |
| Cockpit | (142, 0, 36) |
| MissilePod_L / MissilePod_R | (172, −162, −32) / (172, 162, −32) |

**What gameplay reads:**
- `Muzzle_0..N` through `UTDWeaponComponent`, which needs a contiguous run from 0.
- `Engine` through `ATDShip::GetEngineAttach`.
- The other sockets are informational ability and FX points.
- Every socket's +X points forward.

## Colour blocking (r7, after the T1 live run)

The hulls read white at gameplay distance, so squad colour now covers bold panels on wings, nacelles, fins, decks and pods.
Measured share of the whole surface (`sq_build.py` prints it as `SHARES`):

| | White | Squad colour | Graphite | Other (metal, glass, glow) |
|---|---|---|---|---|
| Rift | 40% | 27% | 22% | 11% |
| Lancer | 26% | 31% | 31% | 13% |
| Bastion | 40% | 29% | 20% | 12% |

The Lancer has the strongest blocking (cyan wings, fin, nacelles, dorsal and flank panels) and full-brightness small engines.

## Checks (r7)

- `asset_report`: 0 errors and 0 warnings on all three.
- `assembly_check`: no floating parts.
- UVs: inside 0–1, with at most 1 overlapping pixel at 1024.
- Triangles: inside the 15k–40k budget.
- Emissive: under 6% of the surface.

## Not verified

- **Rendering in Unreal.** Headless Unreal cannot compile materials, so the master compiles on first load in an editor.
- **The in-game look:** emissive strength, bloom and the hue under Unreal's tonemapper.
- **Nicholas's presentation approval.**

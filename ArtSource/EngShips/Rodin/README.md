# Rodin ships: Grok Bot's Hyper3D Rodin meshes as the game's ships

Intake of the Rodin Gen-2.5 meshes in `ArtSource/EngAI/Ships/**` (Grok Bot's, read-only: never edited here) as
the look of the three squad ships and the Null Armada fighter and gunship. They replace the code-built
engineering ships (`ArtSource/EngShips/Squad`, `ArtSource/EngShips/NullFleet`), which stay in the repo untouched.

Like the ships they replace these are **engineering fallbacks**: the visuals data assets live in
`/Game/TriadDrift/Gameplay/Characters/{Squad,NullFleet}/`, and a `DA_*_Visuals` in `/Game/TriadDrift/Presentation/`
still wins field by field.

## Rebuild

```
cd ArtSource/EngShips/Rodin/scripts
blender --background --factory-startup --python rs_prep.py -- Rift          # Lancer, Bastion, NullFighter, NullGunship, NullInterceptor
powershell -File Tools\td-python.ps1 -Script ArtSource\EngShips\Rodin\scripts\ue_import_rodin_ships.py   # from the repo root
```

- `rs_common.py`: where each ship's source, textures and heat mask are.
- `rs_analyze.py`: prints emissive clusters and forward / rear tips of a source mesh (used to find the sockets).
- `rs_prep.py`: imports the derived FBX, removes loose vertices and degenerate faces only (UVs and Rodin's
  normals are kept: the heat masks are baked in that UV layout), adds `SOCKET_*` empties, renders
  `review/<Ship>_r1_sockets_material_sheet.png` (rear, nose, top, side, two 3/4 views, sockets as dots: green
  muzzles, blue engines, yellow others), saves `../<Ship>/SM_<Ship>.blend`, exports `SM_<Ship>.fbx` and the manifest
  with the repo exporter. `--no-review` skips the sheet, `--no-export` stops before the FBX.
- `ue_import_rodin_ships.py`: Unreal import, idempotent. `TD_RODIN_MI_ONLY=1` only rewrites the material instances
  and visuals (tuning); `TD_RODIN_REVERT=Rift,NullFighter,...` points a ship's visuals back at the old mesh
  (`OldFleet` also drops the material override from the interceptor, bomber and carrier visuals).

Textures are imported straight from `ArtSource/EngAI/Ships/**` (no copies here).

## In Unreal (`/Game/TriadDrift/Gameplay/Ships/`)

| Asset | What |
|---|---|
| `Materials/M_TD_Ship` | one lit master for every ship, Nanite. Parameters below |
| `<Ship>/SM_<Ship>` | Nanite static mesh, one slot, one auto convex hull (gameplay uses its own sphere) |
| `<Ship>/MI_<Ship>` | instance of `M_TD_Ship` |
| `<Ship>/T_<Ship>_BC, _N, _ORM, _E` | 2K (squad) or capped to 1K with `max_texture_size` (Null Armada). N is DirectX |
| `<Ship>/T_<Ship>_HeatMask` | squad only, linear greyscale, capped to 1K |

`M_TD_Ship` parameters: `BaseColorTex` x `BaseTint`; `ORMTex` (AO, roughness x `RoughnessScale`, metallic);
`NormalTex`; `EmissiveTex` x `EmissiveTint` x `EmissiveStrength`; **`HeatGlow` 0..1 (default 0)** x `HeatMaskTex` x
`HeatColor` x `HeatStrength` (20): the re-entry / afterburner glow, to be driven from code on a dynamic instance;
`SelfLight`: base colour added to emissive, so the squad colours read on the side facing away from the sun (the
chase camera mostly sees that side).

| Ship | EmissiveStrength | BaseTint | EmissiveTint | SelfLight |
|---|---|---|---|---|
| Rift, Lancer | 6 | 1 | 1 | 0.12 |
| Bastion | 6 | 1.5 (the Rodin hull is mid grey, it read as a black hulk from behind) | 1 | 0.18 |
| NullFighter | 10 | 0.35 (the albedo came out light silver; brief: near-black armour) | (1, 0.55, 0.35) | 0 |
| NullGunship | 8 | 0.6 | (1, 0.55, 0.35) | 0 |

The enemy emissive tint pushes the seams to red-orange; it also warms the white eyes to a pale peach. At 24 x
untinted the whole fleet glowed white, which broke the red / orange enemy colour code.

**The visuals name the material instance in `MaterialOverrides`.** Gameplay tints the placeholder hull with a
dynamic glow material in slot 0 before the art is applied, and `SetStaticMesh` keeps component overrides, so an
enemy whose visuals carry only a mesh draws as a flat orange glow. The import also sets this on the old
interceptor, bomber and carrier visuals (their own `MI_Null_*`), so the fleet looks consistent.

## Ships (Unreal cm, pivot = bounding-box centre, +X forward, +Y right, +Z up)

| Ship | Size L x W x H | Source tris | Sockets |
|---|---|---|---|
| Rift | 500 x 417 x 226 | 42,371 | `Muzzle_0/1` (252, -/+37, -60); `Engine` (-252, 0, -34); `Engine_0/1` (-252, -/+64, -34); `Portal` (-170, 0, 55) ring centre; `Cockpit`; `WingTip_L/R` |
| Lancer | 524 x 270 x 150 | 49,718 | `Muzzle_0` (152, -45, -59), `Muzzle_1` (108, 45, -55); `Engine` = `Engine_0` (-264, 0, -35); `Engine_1/2` (-209, -/+55, -38); `Ram` (262, 0, -58); `Cockpit`; `WingTip_L/R` |
| Bastion | 524 x 472 x 139 | 49,871 | `Muzzle_0/1` (264, -/+44, -37); `Engine` (-264, 0, 0); `Engine_0/1` (-264, -/+77, 0); `Dome` (-26, 0, 68); `Cockpit`; `MissilePod_L/R` (124, -/+165, -20) |
| NullFighter | 550 x 503 x 171 | 8,912 | `Muzzle_0/1` (268, -/+57, -38); `Engine_0/1` (-277, -/+63, -14), `Engine_2` (-275, 0, -14) |
| NullGunship | 1497 x 1432 x 1116 | 19,800 | `Muzzle_0..3` (748, -280 / -210 / 210 / 280, ~300) turret barrel tips; `Engine_0..3` (-740, -270 / -90 / 75 / 255, 421) |
| NullInterceptor | 660 x 535 x 248 | 8,948 | exported with sockets (`Muzzle_0..3`, `Engine_0`), **not imported**: see below |

All are the same length as the ships they replace (gameplay collision is unchanged: sphere r 220 for the squad
and the fighter, r 700 for the gunship). Nanite is on (as on the ships they replace), so there is no LOD chain;
the Nanite fallback meshes are 5-17k triangles.

## Known limits (honest list)

- **NullInterceptor is not in the game.** Rodin's pack for it has no textures (a grey clay mesh), so it cannot
  carry the faction look. The interceptor keeps the engineering mesh `SM_Null_Interceptor`. The FBX with sockets
  is ready in `../NullInterceptor/` for when a textured drop arrives.
- **Lancer's guns are asymmetric** in the Rodin mesh: a long barrel on the left, a short pod on the right. The
  muzzle sockets sit on the real tips, so the two shots leave 44 cm apart in X.
- **Rift has three nozzles** (the brief has two): `Engine_0/1` are the outer pair, `Engine` the larger middle one.
- **Bastion has two big nozzles** where the brief has four engine pods; `Engine_2/3` do not exist.
- **Gunship**: the four "engine bells" Rodin made are upward-pointing cones on the rear deck; the `Engine_N`
  sockets sit at the rear face of those pods. The hull is much taller than the engineering gunship (1116 vs 677 cm).
- Rodin meshes are not symmetric and have small floating islands (`assembly_check`: Rift 35 groups, Lancer 6);
  nothing was re-modelled.
- The review sheets were rendered before two socket corrections (Lancer `Muzzle_1`, fighter `Engine_N` height).
- The intro (`TDIntroDirector.cpp`) loads `SM_Null_Fighter` / `SM_Null_Interceptor` by path and still shows the
  old fighter.

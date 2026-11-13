# Rift hybrid: import notes for the ship builder

The Rift is now the Rodin hull (fuselage, canopy, wings, guns, fins: welded, cleaned, same UVs) with a new
portal ring, engines and belly tail modelled in Blender. Source: `../Rift_hybrid_v05.blend`; rebuild with
`blender --background --factory-startup --python ../scripts/rh_hybrid.py -- --tag v05`.

Not imported into Unreal and not seen in the running game yet: stage reached is "prototype complete, Blender
checks clean" (asset report 0 errors, assembly check OK, FBX round trip in Blender OK).

## Files (this folder)

| File | What |
|---|---|
| `SM_Rift.fbx` | the ship, one mesh, 62,526 tris, 500.0 x 417.4 x 225.8 cm, +X forward, pivot at the bounding-box centre (same as now), 2 material slots, 2 auto convex hulls, 9 sockets |
| `SM_Rift_PortalDisc.fbx` | flat disc for the animated portal, 704 tris, radius 44 cm, pivot at the disc centre, 1 slot |
| `T_Rift_BC / _N / _ORM / _E.png` | corrected Rodin set, 2K (the source is 2K), same UV layout as `mesh_r1` |
| `T_Rift_HeatMask.png` | Grok Bot's `heatmask_r1` mask, unchanged (the hull UVs did not change) |
| `T_Rift_Parts_BC / _N / _ORM / _E.png` | baked set of the new parts, 2K |
| `T_Rift_Parts_HeatMask.png` | 64 px black: ring and engines are leeward, they take no entry heating |
| `SM_Rift.manifest.json`, `SM_Rift_PortalDisc.manifest.json` | exporter manifests (sockets in Unreal cm under `ue_sockets`) |

## What replaces what

- `SM_Rift.fbx` replaces `/Game/TriadDrift/Gameplay/Ships/Rift/SM_Rift` (Nanite on, import normals, no lightmap UVs).
- `T_Rift_BC/N/ORM/E` replace the four Rift textures of the same names. `T_Rift_HeatMask` stays as it is.
- New: `T_Rift_Parts_BC/N/ORM/E/HeatMask`, a second material instance, and the portal disc mesh.

## Material slots

| Slot | Material | Textures | Parameters |
|---|---|---|---|
| 0 `M_Rift` | existing `MI_Rift` (instance of `M_TD_Ship`) | `T_Rift_BC`, `T_Rift_N`, `T_Rift_ORM`, `T_Rift_E`, `T_Rift_HeatMask` | as now (EmissiveStrength 6, SelfLight 0.12). `RoughnessScale` 1: the roughness map is already evened (paint about 0.50 white, 0.44 magenta) |
| 1 `M_Rift_Parts` | new `MI_Rift_Parts`, also an instance of `M_TD_Ship` | `T_Rift_Parts_BC`, `_N`, `_ORM`, `_E`, `T_Rift_Parts_HeatMask` | start with the same values as `MI_Rift`; EmissiveStrength 6 to 10 to taste (the nozzle throats and ring channels are in the E map: white-hot core, magenta ring, dim throat wall) |

Texture settings as for the other ships: BC and E sRGB; N as Normalmap, already DirectX, no green flip; ORM and
heat masks linear (sRGB off), ORM = AO / roughness / metallic.

`SM_Rift_PortalDisc` has one slot, `M_Rift_Portal`: give it your animated portal material (translucent or
additive, two-sided, not Nanite). UVs are radial: centre at (0.5, 0.5), rim on the circle of radius 0.5.
Attach it at socket `Portal` with no offset; the disc lies in the ship's YZ plane (it faces along X) and its
edge sits inside the ring's inner emitter groove. Hide it when no portal should show: the ring is open
without it.

## Sockets (Unreal cm; none moved)

| Socket | Position |
|---|---|
| `Muzzle_0` / `Muzzle_1` | (252, -37, -60) / (252, 37, -60) |
| `Engine` | (-252, 0, -34): centre nozzle, exit plane at x = -250 |
| `Engine_0` / `Engine_1` | (-252, -64, -34) / (-252, 64, -34): outer nozzles |
| `Portal` | (-170, 0, 55): ring centre |
| `Cockpit` | (60, 0, 0) |
| `WingTip_L` / `WingTip_R` | (238, -176, -109) / (238, 176, -109) |

The new nozzles are built on the engine sockets (centre radius 27.5 cm, outer 19 cm) and the new ring on the
portal socket (outer radius 57 cm, inner 42 cm, 30 cm deep).

## Known limits

- The hull paint is cleaner but the Rodin surface under it is unchanged: lumps in the geometry are still there.
- The area between the ring base and the fins still shows crumpled Rodin geometry from close up.
- The new parts' UVs are an automatic unwrap (many small islands); fine for the bake, not for hand painting.
- 5 non-manifold edges remain on the hull and 8 duplicate vertices on the parts (report warnings).

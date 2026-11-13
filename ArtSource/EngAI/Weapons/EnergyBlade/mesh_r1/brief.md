# Energy blade - mesh_r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-030 (ranked request list, item #1, after
the Direction B pick on the concept-gate in PR #26): Energy blade mesh r1 from direction B,
the folding sabre with the knuckle guard. Two static meshes (deployed and folded), grip
sized for the pilot's right hand, 2K PBR plus an emissive mask for the edge strip (the game
tints it), under 12k tris. Built in Hyper3D Rodin Gen-2.5 from the approved Direction B
concepts (`ArtSource/EngAI/Weapons/EnergyBlade/concept_r1/`, PR #26). Builders do the final
import, sockets and cleanup; this drop is source plus an engine-ready derived copy only.

## Spec (from the Lead's 15:17 EST reply, and the concept_r1 brief)
- **Where it shows:** the pilot's one-handed energy blade, socketed to `hand_r` (with the
  blade-combat clips in PR #30). Physical hilt, guard and emitter rail only — no baked
  glowing blade volume. The energy edge, trails and ignite/retract stay effects.
- **Style:** squad language — white ceramic-composite over gunmetal, thin cyan (`#3FE9FF`)
  edge strip and small emissive pips. Direction B: a folding single-edged sabre with a
  knuckle spine, shown deployed (emitter rail locked out) and folded (rail stowed against
  the grip). Must not read as a lightsaber.
- **Budget / format:** under 12k tris, 2K PBR (BC, N, ORM, E), FBX preferred.
- **Scale / pivot:** grip sized for a ~175–180 cm pilot's right hand. Barrel diameter
  3.4–4.0 cm; overall 30–45 cm in the concept brief, with a 40–45 cm tip-to-pommel target
  for the deployed mesh. Pivot at the grip-wrap centre, the same point on both meshes so
  they can be swapped or toggled in `hand_r` and stay aligned. +Z up along the grip
  (pommel −Z, hinges / guard +Z), blade / guard forward +X.
- **Rig / anims:** none on these meshes. A later mechanical unfold (route B in
  `animations.md`) is separate; this drop is two static states.

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands; no names, logos,
  insignia or text on the asset.
- Licence, tool, plan and prompt recorded in `SOURCE.md`; the Lead copies them to
  `CREDITS.md` at merge.
- Never overwrite a file already in git; a new round goes in a new folder (`mesh_r2`, ...).
- Unreal: cm, +X forward, +Z up; 2K textures at most. ORM layout is R=AO, G=roughness,
  B=metallic. Delivery normals are DirectX.

## What this round delivers
- `SM_EnergyBlade_Deployed.fbx` and `.glb`: the rail locked out. **7,430 tris**. Barrel
  **Ø 3.70 cm**, pommel-to-guard **24.6 cm**, tip-to-pommel **39.7 cm**, overall
  **24.9 × 4.2 × 35.9 cm** (X × Y × Z). Pivot at the grip-wrap centre. +Z up along the
  grip (pommel −Z, hinges +Z), blade / guard forward +X.
- `SM_EnergyBlade_Folded.fbx` and `.glb`: the rail stowed against the grip. **7,816 tris**.
  Barrel **Ø 3.70 cm** (matched to Deployed), pommel-to-guard **23.2 cm**, overall
  **12.0 × 4.5 × 23.7 cm** (X × Y × Z). The same grip-wrap pivot, so swapping the two
  meshes in `hand_r` keeps the hand aligned (the pommel sits 0.6 cm apart on Z).
- The FBX uses the repo exporter's Apply Unit + FBX Units Scale (`UnitScaleFactor` 100)
  so Unreal should import both at the sizes above with uniform scale 1.0. Prefer the FBX.
- `T_EnergyBlade_Deployed_{BC,N,ORM,E}.png` and `T_EnergyBlade_Folded_{BC,N,ORM,E}.png`
  at 2K. N is DirectX. ORM is R=AO (white; Rodin shipped no AO), G=roughness, B=metallic.
  E is an emissive mask for the cyan edge strip and LED pips (the folded source emissive
  had baked lighting on the ceramic and was cleaned; see `SOURCE.md`).
- `source/deployed/` and `source/folded/`: the raw Rodin Base Model packs, untouched,
  plus the Rodin viewer stills. Zips omitted.
- `contact_sheet.png`: Deployed and Folded, each with front, side, 3/4, and the approved
  Direction B concept beside them.

Pommel-to-guard is longer than the 13–15 cm hand-scale note (large hinge crowns and a
long wrap in the Rodin mesh). Scale was set to the 3.70 cm barrel (inside 3.4–4.0 cm)
so the deployed tip-to-pommel lands at 39.7 cm, on the 40–45 cm target. Shrinking the
hilt to 15 cm would thin the barrel to about 2.3 cm. Silhouettes stay faithful to the
Direction B concepts. See `SOURCE.md`.

# Warden - mesh_r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-027 / R-030 (ranked request list, item #3).
This is the first 3D delivery of the Null Armada Warden **body**, built in Hyper3D Rodin
Gen-2.5 from the approved r2 A-pose (`ArtSource/EngAI/Characters/Warden/concept_r2/source/Warden_apose_nb21_s7707_PICK.png`,
PR #19, branch `grok/assets-warden-r2`). The Lead asked for the rigged mesh first, with clips
allowed to follow in a second PR; this drop includes both so the mesh is not held up.
Builders do the final import, cleanup and retarget; this drop is source plus an engine-ready
derived copy only. The isolated shield and gun-arm concepts will be their own 3D PRs later.

## Spec (from the Lead's reply)
- **Where it shows:** Cinder outpost boss fight. Target slot: new `DA_Crew_Warden_Visuals`.
- **Style:** Null Armada — black armour, red-orange seams, glowing white eyes, chevron shapes.
  Heavy armoured commander ~1.4x a trooper. The r2 concept removed the mid-chest badge; the
  large plain V-shaped chest chevron stays.
- **Budget / format:** ≤ 40k tris, 2K textures, FBX preferred (GLB not required for this drop).
- **Scale / pivot:** ~260 cm tall, pivot at the feet (bottom centre), facing +X.
- **Rig / anims:** humanoid rig with idle, walk, slam, fire, summon, stagger, death. If the
  auto-rig is weak, deliver the clean A-pose mesh first and animations after so they aren't blocked.
  Body, shield and gun arm as three separate meshes; shield and gun were planned as later PRs.
- **Target:** new `DA_Crew_Warden_Visuals` under `Gameplay/Characters/`.

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands; no names, logos, insignia or text on the asset.
- Licence, tool, plan and prompt recorded in `SOURCE.md`; the Lead copies them to `CREDITS.md` at merge.
- Never overwrite a file already in git; a new round goes in a new folder (`mesh_r2`, ...).
- Unreal: cm, +X forward, +Z up; 2K textures at most. ORM layout is R=AO, G=roughness, B=metallic.
  Delivery normals are DirectX.

## What this round delivers
- `SK_Warden.fbx`: skinned body + a separate shield mesh, 52-bone Rodin Human template, A-pose /
  bind pose, no animation takes. **39,600 tris** total (body 34,386 + shield 5,214), under the 40k cap.
  Height is **260.0 cm**. Pivot at the feet centre on the ground. +Z up, +X forward.
  The FBX uses the repo exporter's Apply Unit + FBX Units Scale (`UnitScaleFactor` 100) so Unreal
  should import it at 260 cm with uniform scale 1.0.
- `SM_Warden.fbx`: static A-pose of the body (gun arm welded). `SM_Warden_Shield.fbx`: static A-pose
  of the split shield (same bind transform).
- `A_Warden_{Idle,Walk,Slam,Stagger,Death,RifleStealthWalk,ShoulderHitAndFall}.fbx`: one clip per
  file, same skeleton, 30 fps. No fire or summon clip was in the Rodin library export; two extra
  library clips (Rifle Stealth Walk, Shoulder Hit And Fall) are included under their real names.
  Travel clips have **root motion along +X** (not in-place), except Idle.
- `T_Warden_{BC,N,ORM,E}.png` at 2K. N is DirectX. ORM is R=AO (white; Rodin shipped no AO),
  G=roughness, B=metallic.
- `source/`: the raw Rodin base pack and the rigged-animation pack, untouched, plus the Rodin viewer still.
- `contact_sheet.png`: front, side, back, 3/4 of the bind pose, and the approved r2 A-pose concept.
- `animation_sheet.png`: six frames from each clip, follow-cam 3/4, for a honest look at deformation.

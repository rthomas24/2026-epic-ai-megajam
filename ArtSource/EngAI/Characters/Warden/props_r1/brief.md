# Warden - props_r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-030 (ranked request list, item #3): body, shield
and gun arm as three separate meshes. The body (with a split-off skinned shield and a welded gun
arm) is in PR #25. This drop is the isolated, socketable shield and gun-arm meshes, built in
Hyper3D Rodin Gen-2.5 from the approved r2 isolated concepts
(`Warden_shield_nb21_s8808_PICK.png` and `Warden_gun_nb21_s9909_PICK.png`, PR #19).
Builders do the final import, sockets and cleanup; this drop is source plus an engine-ready
derived copy only.

## Spec (from the Lead's reply)
- **Where it shows:** Cinder outpost boss fight, socketed to the Warden body in `DA_Crew_Warden_Visuals`.
- **Style:** Null Armada — black armour, red-orange seams, chevron shapes. Match the r2 A-pose and the isolated shield / gun paintings.
- **Budget / format:** no separate cap was given for the props (the body+props budget is ≤ 40k tris for the character). These two meshes are 7,106 and 8,188 tris. 2K textures, FBX preferred.
- **Scale / pivot:** scale each to match the corresponding part on the 260 cm Warden in PR #25. Shield pivot at the grip/handle centre on the back face (socket to the left hand). Gun arm pivot at the elbow/attach end.
- **Rig / anims:** none. These are static socketable meshes.

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands; no names, logos, insignia or text on the asset.
- Licence, tool, plan and prompt recorded in `SOURCE.md`; the Lead copies them to `CREDITS.md` at merge.
- Never overwrite a file already in git; a new round goes in a new folder (`props_r2`, ...).
- Unreal: cm, +X forward, +Z up; 2K textures at most. ORM layout is R=AO, G=roughness, B=metallic.
  Delivery normals are DirectX.

## What this round delivers
- `SM_Warden_ShieldProp.fbx` and `.glb`: isolated tower shield. **7,106 tris**. **180.981 cm** tall (matched to the AABB height of `SM_Warden_Shield` on the 260 cm Warden in PR #25), 20.4 × 74.0 × 181.0 cm (X × Y × Z). Pivot at the back-face grip centre. +Z up, chevron face along +X. Suggested socket: `hand_l_shield`.
- `SM_Warden_GunArm.fbx` and `.glb`: isolated gun arm (cuff at the elbow, six barrels). **8,188 tris**. **122.593 cm** cuff-to-muzzle (matched to the welded gun arm on `SK_Warden` in PR #25, elbow to muzzle), 122.6 × 42.7 × 50.3 cm (X × Y × Z). Pivot at the cuff opening. +Z up, muzzle along +X. Suggested socket: `lowerarm_r` (or `hand_r_gun`).
- The FBX uses the repo exporter's Apply Unit + FBX Units Scale (`UnitScaleFactor` 100) so Unreal should import both at the sizes above with uniform scale 1.0.
- `T_Warden_ShieldProp_{BC,N,ORM,E}.png` and `T_Warden_GunArm_{BC,N,ORM,E}.png` at 2K. N is DirectX. ORM is R=AO (white; Rodin shipped no AO), G=roughness, B=metallic.
- `source/shield/` and `source/gun/`: the raw Rodin Base Model packs, untouched, plus the Rodin viewer stills.
- `contact_sheet.png`: front, side, back/rear (showing the pivot), 3/4, and the approved r2 concept pick, for both props.

See `SOURCE.md` for the scale measurements, the slightly off-centre grip, and the invented rust-red back plate on the shield.

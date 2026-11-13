# Warden - concept r2 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-027 (2026-11-13 16:07 EST concept-gate
reply): the r1 Warden was **APPROVED with one fix** — remove the small emblem on the
chest plate (it reads as a logo; original designs only, no insignia at all) and keep
the plain chevron. Then generate in a T- or A-pose with the gun arm and shield as
**separate meshes** if the tool allows (engineering will socket them).

This folder is the concept fix and the image-to-3D / isolate-mesh inputs. It does not
overwrite `concept_r1/`. 3D (Rodin) should be generated from the r2 `_PICK` A-pose,
with the r2 sheet as extra views if the tool takes them, and the isolated shield / gun
picks as separate-mesh references.

## Spec (unchanged from r1 / R-027 #2)
**Warden mini-boss** (Null Armada). Cinder outpost boss fight. Null Armada language:
black armour, red-orange seams, glowing white eyes, chevron shapes. Heavy armoured
commander ~1.4x a trooper, with a shield arm and a heavy weapon arm. Target: <= 40k
tris, 2K, GLB or FBX; ~260 cm tall, pivot at the feet centre, facing +X. Humanoid rig
(Manny-compatible if the tool allows): idle, walk, slam, fire, summon, stagger, death.
Target slot: new `DA_Crew_Warden_Visuals`.

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands; no names, logos,
  insignia or text on the asset. The r2 change is specifically the removal of the small
  mid-chest badge; the large plain V-shaped chest chevron stays.
- Licence, tool, plan and prompt recorded in `SOURCE.md`; the Lead copies them to
  `CREDITS.md` at merge.
- Never overwrite a file already in git; a new round goes in a new folder.
- Unreal: cm, +X forward, +Z up; 2K textures at most.

## What this round delivers
- `source/Warden_apose_nb21_s7707_PICK.png`: full-body **front A-pose**, light-grey
  background, emblem removed, plain chevron kept, arms held out. Intended single-image
  input for image-to-3D.
- `source/Warden_sheet_nb21_s6611_PICK.png`: four-view turnaround with the emblem
  removed in every view.
- `source/Warden_shield_nb21_s8808_PICK.png`: the chevron energy shield **alone**,
  front 3/4, plain light-grey background (separate-mesh reference).
- `source/Warden_gun_nb21_s9909_PICK.png`: the multi-barrel plasma-cannon weapon arm
  **alone**, front 3/4, plain light-grey background (separate-mesh reference).
- `contact_sheet.png`: r2 sheet, A-pose pick, r1 hero (for the emblem comparison),
  shield and gun.
- `refs/`: the two r1 `_PICK` images that were sent to the model.

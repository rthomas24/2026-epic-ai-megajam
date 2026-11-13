# Warden - concept turnaround r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-027 (ranked request list, item #2); this round is the
**concept gate** for a hero asset: the Lead approves (or notes) the turnaround before any 3D is generated.

## Spec (from the Lead's reply, verbatim in substance)
**Warden mini-boss** (Null Armada; no mesh exists today, red placeholder blocks). Cinder outpost boss fight.
Null Armada language: black armour, red-orange seams, glowing white eyes, chevron shapes; match the trooper sheets in
`ArtSource/EngCharacters/NullTrooper/`. A heavy armoured commander ~1.4x a trooper, with a **shield arm** and a **heavy
weapon arm**. Target: <= 40k tris, 2K, GLB or FBX; ~260 cm tall, pivot at the feet centre, facing +X. Humanoid rig
(Manny-compatible if the tool allows; otherwise any clean humanoid, engineering retargets): idle, walk, slam (ground pound),
fire, summon (raise arm), stagger, death. Target slot: new `DA_Crew_Warden_Visuals`.

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands; no names, logos, insignia or text on the asset.
- Licence, tool, plan and prompt recorded in `SOURCE.md`; the Lead copies them to `CREDITS.md` at merge.
- Never overwrite a file already in git; a new round goes in a new folder (`concept_r2`, ...).
- Unreal: cm, +X forward, +Z up; 2K textures at most.

## What this round delivers
- `source/*_PICK.png`: the chosen turnaround sheet and the chosen single 3/4 hero view (plain background; the intended
  input for Hyper3D Rodin image-to-3D after approval).
- `source/` also keeps every alternate, untouched (renamed only; original fal file names in `SOURCE.md`).
- `contact_sheet.png`: picks, alternates and the reference side by side.
- `refs/`: the reference image(s) sent to the model.

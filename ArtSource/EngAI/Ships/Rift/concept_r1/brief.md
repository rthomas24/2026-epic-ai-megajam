# Rift - concept turnaround r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-027 (ranked request list, item #1); this round is the
**concept gate** for a hero asset: the Lead approves (or notes) the turnaround before any 3D is generated.

## Spec (from the Lead's reply, verbatim in substance)
**Rift** (player ship #1 of 3). Trickster: swept crescent wings plus a portal-emitter ring. White hull with **magenta**
(linear 1, 0.17, 0.84 ~ #FF2BD6) on ~30% of the hull; glowing engines. Bold readable silhouette.
Target (for the 3D step, Rodin): 40-60k tris (Nanite), 2K PBR (BC, N, ORM, E), GLB; ~500 x 494 x 119 cm (L x W x H);
pivot at the bounding-box centre; nose along +X, +Z up. No rig; engineering adds sockets (`Muzzle_N`, `Engine`, `Engine_N`,
ability points). Target slot: `Gameplay/Characters/Squad/DA_Rift_Visuals.Mesh`.

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

# Lancer - concept_r2 (brief)

Round 2 of the R-027 #1 concept gate. Ryan asked for more: "for the ships, go big or go home, try to make it really cool
and detailed". This round takes the round-1 pick (`../concept_r1/`, PR #14) and upgrades it to an AAA hero-level
finish: layered armour plates, panel lines, greebles, vents and intakes, weapon hardpoints, emissive strips and a detailed
canopy, with no decals. Silhouette and colour spec stay as in r1. **If approved, r2 supersedes r1 for Lancer.**

## Spec
**Lancer** (player ship #2 of 3). Needle interceptor: long nose, small swept wings. White hull with **cyan**
(linear 0.25, 0.91, 1 ~ #40E8FF) on ~30% of the hull; glowing engines. Bold readable silhouette.
Target (for the 3D step, Rodin): 40-60k tris (Nanite), 2K PBR (BC, N, ORM, E), GLB; ~524 x 318 x 164 cm (L x W x H);
pivot at the bounding-box centre; nose along +X, +Z up. No rig; engineering adds sockets. Target slot:
`Gameplay/Characters/Squad/DA_Lancer_Visuals.Mesh`.

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands; no names, logos, insignia or text on the asset.
- Licence, tool, plan and prompt recorded in `SOURCE.md`; the Lead copies them to `CREDITS.md` at merge.
- Never overwrite a file already in git; a new round goes in a new folder.
- Unreal: cm, +X forward, +Z up; 2K textures at most.

## What this round delivers
- `source/*_PICK.png`: the chosen single 3/4 hero view (plain grey background; the intended input for Hyper3D Rodin
  image-to-3D after approval) and the chosen four-view turnaround sheet (front, side, top, 3/4 rear).
- `source/` also keeps every alternate, untouched (renamed only; original fal file names in `SOURCE.md`).
- `contact_sheet.png`: picks, alternates and references side by side.
- `refs/`: the reference images sent to the model.

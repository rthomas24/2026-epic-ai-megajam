# Carrier - concept_r1 (brief)

Round 1 of the Null Armada fleet (R-027 #5). Ryan asked for the enemy ships too, kept on theme with the Warden and
Drone ("also work on enemy ships and keep them on theme"). Concept gate: the Lead approves or sends notes on the
turnaround before any 3D is generated.

## Spec
**Null Armada Carrier** (R-027 #5, enemy fleet; today a placeholder from `ArtSource/EngShips/NullFleet/review/`).
Silhouette idea to keep: **horned carrier**. Faction language as #2 (Warden / Drone): black armour, red-orange glowing seams
(~#FF4A1A), white glowing eye sensors, chevron shapes.
Target (for the 3D step, Rodin hard surface): **30-60k tris (Nanite)**, 2K PBR (BC, N, ORM, E); the carrier may use 2 x 2K sets, GLB; length **~4600 cm**; pivot at the bounding-box centre;
nose along **+X**, +Z up. **No rig.** Engineering adds sockets. Target slot: `Gameplay/Characters/NullFleet/DA_Enemy_Carrier_Visuals`.

Full R-027 #5 budgets for the fleet: lengths fighter ~550 cm, interceptor ~660, gunship ~1500, bomber ~1180, carrier ~4600;
fighter / interceptor 6-10k tris, gunship / bomber 12-20k, carrier 30-60k (Nanite), turret 2-4k; 2K textures (carrier may
use 2 x 2K); pivot at the bounding-box centre; +X forward; no rig.

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

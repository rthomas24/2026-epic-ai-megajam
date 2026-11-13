# Ancient Relay - mesh_r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-030 (Nicholas's six reference
boards, item #2 / GitHub issue #37): the recurring ancient relay / artefact —
a split outer ring, cyan core, radial supports and a stair-fronted plinth.
Hero prop, about 10 m tall, under 30k tris, with an emissive mask for the
core. The 16:15 hold on this board was lifted at 16:23 EST (`ff99b128`):
Nicholas confirmed the Batch 1 Ancient Relay Modeling Reference is the
intended artefact. Built in Hyper3D Rodin Gen-2.5 from the cleaned studio
hero (`refs/37_relay_clean.png`), itself derived from Nicholas's modelling
reference (`refs/Ancient_Relay_Modeling_Reference.png`). Builders do the
final import, collision and placement; this drop is source plus an
engine-ready derived copy only.

## Spec (from the Lead's R-030 board, and issue #37)
- **Where it shows:** a recurring hero prop / story-critical artefact on the
  planets (the player walks around it; the 16:26 walkable-building route
  had this board still listed as on hold).
- **Style:** monumental carved stone-and-alloy relay: split outer ring, inner
  concentric ring, glowing cyan spherical core held by four radial
  conduits, flanking pylons, circular stepped plinth with a front
  staircase, weathered moss and carved glyphs.
- **Budget / format:** under 30k tris, 2K PBR (BC, N, ORM, E), FBX preferred.
- **Scale / pivot:** about 10 m tall. Pivot at the ground-base centre,
  +Z up, +X forward (stair-fronted plinth and ring face toward +X,
  symmetric on Y).

## Rules that bind this asset
- Original design only: no likeness to existing games, films or brands; no
  names, logos, insignia or text on the asset.
- Licence, tool, plan and prompt recorded in `SOURCE.md`; the Lead copies them
  to `CREDITS.md` at merge.
- Never overwrite a file already in git; a new round goes in a new folder
  (`mesh_r2`, ...).
- Unreal: cm, +X forward, +Z up; 2K textures at most. ORM layout is R=AO,
  G=roughness, B=metallic. Delivery normals are DirectX.

## What this round delivers
- `SM_AncientRelay.fbx` and `.glb`: the hero monument. **26,487 tris**
  (decimated COLLAPSE from the 50,000-tri Rodin pack; UVs intact,
  watertight). Overall **1124.0 × 1221.0 × 1000.0 cm** (X × Y × Z).
  Height **1000.0 cm (10.0 m)**, as briefed. Pivot at the ground-base
  centre: min Z = **0.0 cm**, centred in X and Y (X −562.0 to +562.0 cm,
  Y −610.5 to +610.5 cm). +Z up, +X forward (stairs and ring face).
  Stair tip at X = **+562.0 cm**; stair width about **215.2 cm**;
  courtyard / deck height about **242.5 cm**.
- The FBX uses the repo exporter's Apply Unit + FBX Units Scale
  (`UnitScaleFactor` 100) so Unreal should import it at the sizes above
  with uniform scale 1.0. Prefer the FBX. The GLB is clean glTF in
  **metres**, with the same +Z up / +X forward pose stored in the file
  (`export_yup=false`); see `SOURCE.md` if an importer assumes glTF Y-up.
- `T_AncientRelay_{BC,N,ORM,E}.png` at 2K. N is DirectX (green channel
  flipped from Rodin's OpenGL +Y map). ORM is R=AO (white; Rodin shipped
  no AO), G=roughness, B=metallic (byte-identical to
  `source/texture_pbr.png`). E is a lightly thresholded mask
  (`max(R,G,B) <= 18 → 0`) so the cyan core, beams and runes stay crisp
  and residual near-black noise on the stone is gone.
- `source/`: the raw Rodin Base Model pack, untouched, plus the Rodin
  viewer still. Zips omitted.
- `refs/Ancient_Relay_Modeling_Reference.png` and `refs/37_relay_clean.png`:
  Nicholas's board and the fal studio-isolation input used for Rodin.
- `contact_sheet.png`: Front +X, Side +Y, Back −X, Top +Z, 3/4 Hero, a
  scale comparison with a 180 cm pilot box at the foot of the +X stairs,
  and the BC / N / ORM / E swatches.

**Scale note:** Nicholas's modelling board callouts 18.5 m tall and 24.0 m
wide. The R-030 brief is "about 10 m tall"; height was set to **1000.0 cm**.
At that scale the ring-face width is 1221.0 cm and the stair-to-back depth
is 1124.0 cm. The silhouette stays faithful to the cleaned hero. See
`SOURCE.md`.

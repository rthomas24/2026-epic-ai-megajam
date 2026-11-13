# Verdance Crashed Ship Wreck - mesh_r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-030 (16:26 & 16:30 EST, Nicholas's
six reference boards, item / GitHub issue #39): the overgrown Verdance wreck — a
ruptured mid-fuselage the player can walk around and through, with a broken wing,
exposed ribs, cockpit and rear turbines. Hero environmental story asset. Under 30k
tris, 2K PBR. Built in Hyper3D Rodin Gen-2.5 from the cleaned studio hero
(`refs/39_wreck_clean.png`), itself derived from Nicholas's modelling reference
(`refs/Crashed_Shp_Wreck_Modeling_Reference.png`). Builders do the final import,
collision, a flat floor slab in the ruptured bay, and placement; this drop is
source plus an engine-ready derived copy only.

## Spec (from the Lead's R-030 board, and issue #39)
- **Where it shows:** an environmental story asset on Verdance. The player walks
  around and through a ruptured section.
- **Style:** wrecked futuristic transport: cockpit, ruptured mid-fuselage with
  exposed ribs, broken wing, rear turbines and tail fin, weathered paint, moss.
- **Budget / format:** under 30k tris (target ~26,000–28,000), 2K PBR (BC, N, ORM).
  **No emissive** — Rodin did not produce one for #39, and none was invented.
- **Scale / pivot:** realistic crashed-transport overall length around 26–32 m,
  set to **32.0 m** so the clear through-hole is at least 200 cm wide and 250 cm
  high. Pivot at the ground-base centre, +Z up, +X forward (nose).
- **Walkability:** a gap in the fuselage at least 200 cm wide and 250 cm high,
  with a flat floor piece Engineering can add. Pilot reference: 180 cm tall,
  40 cm radius (80 cm diameter).

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
- `SM_VerdanceWreck.fbx` and `.glb`: the hero wreck. **27,000 tris**
  (decimated COLLAPSE from the 50,000-tri Rodin pack after removing 26 stray
  micro-fragments of <15 tris; UVs intact, watertight). Overall
  **3200.0 × 1332.5 × 1171.7 cm** (X × Y × Z). Length **3200.0 cm (32.0 m)**.
  Pivot at the ground-base centre: min Z = **0.0 cm**, centred in X and Y
  (X −1600.0 to +1600.0 cm, Y −666.3 to +666.3 cm). +Z up, +X forward (nose /
  cockpit). Tail, turbines and fin toward −X. The hero ruptured opening faces +Y.
- Clear through-hole walkthrough, measured in Blender on the delivery mesh:
  **360.0 × 255.6 cm** (width along the fuselage × height). Deck / floor lip
  **276.9 cm**, ceiling **532.6 cm**. A 180 cm tall × 80 cm wide pilot box
  stands on the deck in the opening with ~75.6 cm of headroom. The gap meets
  the R-030 200 × 250 cm minimum; length was set to 32.0 m (top of the 26–32 m
  window) so the height clears 250 cm without trimming interior triangles.
- **Floor note for Engineering:** the existing deck is 276.9 cm above the
  ground, with hanging stairs below that are not a walkable ramp. Drop a flat
  floor slab at ~276.9 cm inside the ruptured midsection (opening X −56.0 to
  +304.0 cm, through the beam) and add a ramp or climb from terrain up to that
  deck if the pilot is to enter from ground level.
- The FBX uses the repo exporter's Apply Unit + FBX Units Scale
  (`UnitScaleFactor` 100) so Unreal should import it at the sizes above
  with uniform scale 1.0. Prefer the FBX. The GLB is clean glTF in
  **metres**, with the same +Z up / +X forward pose stored in the file
  (`export_yup=false`); see `SOURCE.md` if an importer assumes glTF Y-up.
- `T_VerdanceWreck_{BC,N,ORM}.png` at 2K. **No E map.** N is DirectX (green
  channel flipped from Rodin's OpenGL +Y map). ORM is R=AO (white; Rodin
  shipped no AO), G=roughness, B=metallic (byte-identical to
  `source/texture_pbr.png`). BC is byte-identical to `source/texture_diffuse.png`.
- `source/`: the raw Rodin Base Model pack, untouched, plus the Rodin
  viewer still. Zips omitted. No `shaded.*` / `base.glb` (mirrors the
  Outpost / Ancient Relay source file list).
- `refs/Crashed_Shp_Wreck_Modeling_Reference.png` and `refs/39_wreck_clean.png`:
  Nicholas's board and the fal studio-isolation input used for Rodin.
- `contact_sheet.png`: Front +X, Rear −X, Left +Y, Right −Y, Top +Z, 3/4 Hero,
  a close-up of the ruptured walkthrough with the bright 180 cm × 80 cm pilot
  figure standing on the deck in the gap, and the BC / N / ORM swatches.

**Scale note:** Nicholas's modelling board callouts an intact ship at 56.0 m
long and 12.8 m tall. The R-030 wreck brief is a walkable crashed transport
around 26–32 m; length was set to **3200.0 cm** so the midsection through-hole
is 360.0 × 255.6 cm. The silhouette stays faithful to the cleaned hero. See
`SOURCE.md`.

# Verdance research outpost + research console - mesh_r1 (brief)

Requested by Ryan's Lead in `MESSAGEBOARD.md` R-030 (Nicholas's six reference
boards, item #1 / GitHub issue #42): a raised modular survey-outpost cabin
with an open console bay, a short ramp and a roof antenna, plus the research
console as a **separate mesh** so the player can walk up to it. About 8 m long,
under 25k tris combined. Built in Hyper3D Rodin Gen-2.5 from the cleaned empty-
bay and standalone-console concepts (`refs/42_outpost_empty.png`,
`refs/42_console_clean.png`), themselves derived from Nicholas's modelling
reference (`ArtSource/Reference/VerdanceResearchOutpost/Research Outpost
Modeling Reference.png`). Builders do the final import, collision and
placement; this drop is source plus an engine-ready derived copy only.

## Spec (from the Lead's R-030 board, and issue #42)
- **Where it shows:** the survey station on both Verdance and Cinder. Keep the
  cabin **open-sided** so the player walks up the ramp and into the bay; the
  console is a second mesh, not welded into the interior.
- **Style:** squad language — white / light-grey hard-surface panels over
  gunmetal frames, short front ramp with handrails, hinged overhead awning,
  side equipment box with cables, roof telemetry antenna. Console: angled
  cyan waveform display, tactile control deck, left diagnostic tower, right
  cyan glass specimen chamber.
- **Budget / format:** under 25k tris combined, 2K PBR (BC, N, ORM; E on the
  console), FBX preferred.
- **Scale / pivot:** outpost about 8 m long. Console sized for a ~175–180 cm
  pilot to stand at (about 140–155 cm overall height, 100–110 cm desk, 220–260
  cm width) and to fit inside the bay. Pivot at the ground-base centre,
  +Z up, +X forward (outpost ramp and open bay face +X; console screens and
  controls face +X).

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
- `SM_VerdanceOutpost.fbx` and `.glb`: the empty raised cabin. **8,058 tris**.
  Overall **800.0 × 770.4 × 662.0 cm** (X × Y × Z). Longest horizontal (ramp
  tip to back of the deck) is **800.0 cm**, as briefed. Pivot at the
  ground-base centre. min Z = **0.0 cm** (deck legs flush on the ground),
  centred in X and Y (X −400.0 to +400.0 cm, Y −385.2 to +385.2 cm). +Z up,
  +X forward (open bay and ramp). Raised deck **138.0 cm**. Open-front
  doorway (first lintel at X = +230 cm): **282.9 cm** high × **338.4 cm**
  wide (wall-to-wall interior). Bay depth, lintel plane to back wall:
  **294.9 cm**. Mid-bay interior ceiling **312.7 cm**. No interior console
  (the bay is hollow / walk-in).
- `SM_ResearchConsole.fbx` and `.glb`: the standalone workstation. **6,628
  tris**. Overall **87.0 × 260.0 × 150.0 cm** (X depth × Y width × Z height).
  Pivot at the ground-base centre, min Z = **0.0 cm**, centred in X and Y
  (X −43.5 to +43.5 cm, Y −130.0 to +130.0 cm). +Z up, +X forward (screens
  and control deck). Overall height **150.0 cm** (inside 140–155 cm); width
  **260.0 cm** (top of the 220–260 cm range). Desk / control-surface height
  **85.8 cm** (see the note below). Combined **14,686 tris**, well under 25k.
- The FBX uses the repo exporter's Apply Unit + FBX Units Scale
  (`UnitScaleFactor` 100) so Unreal should import both at the sizes above
  with uniform scale 1.0. Prefer the FBX.
- `T_VerdanceOutpost_{BC,N,ORM}.png` and `T_ResearchConsole_{BC,N,ORM,E}.png`
  at 2K. N is DirectX. ORM is R=AO (white; Rodin shipped no AO), G=roughness,
  B=metallic. The outpost has no emissive map (the diffuse has no light
  islands that need one). The console E is a cleaned mask for the cyan
  waveform screen, status buttons and specimen-chamber glow (see `SOURCE.md`).
- `source/outpost/` and `source/console/`: the raw Rodin Base Model packs,
  untouched, plus the Rodin viewer stills. Zips omitted.
- `refs/42_outpost_empty.png` and `refs/42_console_clean.png`: the cleaned
  concepts used as Rodin inputs.
- `contact_sheet.png`: outpost 4-view + concept, console 4-view + concept,
  and a composite placement check with the console in the bay and a 180 cm
  pilot box + survey pole in the doorway.

**Desk-height note:** the control surface comes out at 85.8 cm, below the
100–110 cm standing-desk brief, because of the mesh proportions. Height was
set to 150.0 cm so the width lands at 260.0 cm (the binding bay-fit
targets). Scaling the desk onto 105 cm would push overall height to 191 cm
and width to 331 cm, past those targets and a tighter fit in the 338 cm bay.
The silhouette stays faithful to the cleaned console concept; a 175–180 cm
pilot still has ~100 cm of headroom under the 282.9 cm lintel. See
`SOURCE.md`.

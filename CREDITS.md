# Triad Drift credits

## Tools used
- OpenAI Codex: Nicholas Tosti's Environment, VFX, UI/UX, Audio & Polish agents and Lead; source authoring, procedural art and review, 2026-11-12.
- Python standard library, browser Canvas/Web Audio and original SVG: source authoring/preview tools.
- Unreal Engine 5.9 Python API: target API for unexecuted asset builders; no native assets generated in this source-only batch.

## Original source assets
| Asset | Source / author | License / provenance | Added by |
|---|---|---|---|
| Space Foundation | Original procedural sky code, composition and presets | Project-authored; no third-party media bundled | Nicholas / Environment Agent |
| VFX Starter Kit | Original four HLSL effects, builder and preview | Project-authored; no downloaded textures or fonts | Nicholas / VFX Agent |
| Triad HUD | Original vector icons, design tokens, HUD concept and mock display state | Project-authored; system fonts only | Nicholas / UI/UX Agent |
| Audio Starter Plan | Original synthesized switch/UI audition, authoring cue manifest | Project-authored source; no audio recordings acquired or exported | Nicholas / Audio & Polish Agent |

Per-kit credits.json files retain provenance. The audio README lists candidate providers and acquisition gates only; no third-party audio has been licensed, downloaded or imported for this batch. Existing Epic template content remains covered by the team's Unreal license and is not represented here as original art. Any future acquired asset requires its own source, exact license evidence and submission credit.

## Coordinated separate source branch
Lancer M01 source prototype and modular weapon sources: original geometry authored for Nicholas with OpenAI Codex; deterministic Node.js and offline SVG/Canvas previews. No downloaded assets. Separate branch nick/ships-lancer; code hull is a design proxy pending silhouette approval, Blender/UV/export and Unreal validation. See its provenance.json for exact source and helper dependency notes.

## Audio switch candidates M02 — branch review only

Rift, Lancer and Bastion ship-switch WAVs: original layered procedural sound design
for Nicholas Tosti with OpenAI Codex Audio & Music Agent, November 12, 2026.
Python 3.13.9 and NumPy 2.3.4 rendered stochastic pressure, inharmonic shell modes,
mechanical friction and original finite reflections. Existing standalone runtimes
only; no external recordings, library samples, impulse responses, pretrained audio
outputs, downloads or paid credits. Runtime libraries are not redistributed.
Project licensing applies; no third-party audio license is asserted.
Sources/provenance/hashes: `nick/audio-music` at `5979cff`,
`ArtSource/Audio/StarterPlan/SwitchM02/`; draft PR #5. Manual audition and Unreal
integration are pending; candidate media are not integrated into main. The older
Audio Starter Plan tones above were rejected stylistically and remain preserved.

## Native SpaceFoundation milestone

Original procedural SpaceFoundation native package by Nicholas Tosti with OpenAI
Codex Environment & VFX Agent; Unreal Engine5.9 CL60351404 Python API and Python
standard library, November12. Two original textures, master/instances, sky Actors
and isolated review maps; Engine BasicShapes/Sphere referenced under the team's
Unreal license, not copied. No third-party downloads. Source/provenance:
`ArtSource/Environment/SpaceFoundation/UnrealNative/`. Main integration9c629e8;
known zenith star stretching documented, gameplay placement/PIE/package/GPU timing
not verified. Native stage supersedes the earlier source-batch-only tool note.

## Lancer M02 approved source foundation

Original Lancer hull, modular weapons, materials and review work by Nicholas Tosti
with OpenAI Codex Ships & Weapons Agent, November12,2026. Node.js source generation
and Blender5.1.1 refinement/QA/EEVEE renders; repository hard-surface helper
`.claude/skills/blender-game-asset/scripts/hardsurface.py` (MIT, existing license
retained), no external models/textures downloaded. Source/provenance and all
historical review evidence: ArtSource/Space/Ships/Lancer/. Approved refined model
FOUNDATION, integrated from PR3 as100f21c; finalAAA polish and Unreal export/import,
material/collision/LOD/runtime wiring/gameplay validation remain pending. Earlier
separate-branch/M01-only notes above describe historical checkpoints.

## November12 source checkpoints — retained, not final shipped assets

- Rift M01: original modular ship/source and review by Nicholas Tosti with Codex Ships & Weapons Agent, Blender5.1.1 and the existing MIT repository hard-surface kit; no external model/texture acquisition. Branch nick/ships-rift a7c0463, draftPR7. Preserved technical reference after Nick's redesign instruction; no native or final-design acceptance.
- HeroM02: original deterministic nebula/star textures and isolated benchmark source by Codex Environment & VFX Agent, existing Python/NumPy/Unreal5.9 toolchain. Branch nick/environment-vfx fc4db83. NASA images were visual references only, not sampled or redistributed. Native build/capture/presentation remain pending.
- Audio PrototypeV1: existing licensed-cache selections from PlaceHolder Inc's [50 Free Game Sounds Pack](https://www.fab.com/listings/b8cc7270-5e0c-4ef7-a277-6a1d4b69358d), Hove Audio's [Terminal UI LITE](https://www.fab.com/listings/183a6ec7-cbc3-451a-8fa7-08da97c07fcf), and Marika Schanz's [Atmoshpheric Worlds](https://www.fab.com/listings/c9e03a63-9359-4929-9c8a-a680309dbd3f). Exact names/hashes, provenance and outstanding acquisition-license checks: ArtSource/Audio/PrototypeV1/{sources.json,CREDITS_PROTOTYPE.md} on nick/audio-prototype-v1 at0717d9f. All raw/prepared licensed WAV and native audio assets remain local ignored files; these source commits publish only implementation/manifests/QA and review graphs. Nick authorizes prototype audition use, not final in-game quality or media redistribution. No purchases, downloads or AI-audio services. Existing Python/NumPy, Shotcut ffmpeg/ffprobe8.0.1, UE5.9 CL60351404 and Ryan's ue-game-audio skill4096f47 were used; native authoring has not passed.
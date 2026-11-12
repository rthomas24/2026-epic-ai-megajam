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

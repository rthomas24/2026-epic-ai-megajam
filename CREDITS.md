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

## Engineering side (Ryan T with Claude Code) - tools and project-authored assets

- Tools: Claude Code (Anthropic) as lead and five teammate sessions plus review and build sub-agents: all gameplay C++, tests, tools and docs under `Source/`, `Tools/`, `docs/eng/`; Unreal Engine 5.9 built-in MCP server and engine toolsets for agent playtests; Unreal headless Python (`Tools/td-python.ps1`); Blender 5.2 headless with this repo's Blender skills; ffmpeg / ffprobe for the audio skill's measuring scripts. 2026-11-12.
- `Content/TriadDrift/Gameplay/Materials/M_TD_PlaceholderGlow`: original unlit placeholder material built by script (`Tools/td-make-materials.py`); project-authored, no third-party source. Added by Ryan / Lead (Claude Code).
- `.claude/skills/ue-game-audio/` and `reports/Game audio practices for Triad Drift.md`: original skill text and scripts; licence terms of third-party audio sources are summarised there from the sources' own pages, no audio is bundled. Added by Ryan / Lead (Claude Code).
- Engineering-made environment props (see `MESSAGEBOARD.md` R-015). All are original, code-built designs with no third-party model, texture or scan; added by Ryan / Lead (Claude Code sub-agents), 2026-11-12:
  - Space rocks: `SM_Asteroid_01` to `_04`, `SM_SpaceDebris_01` to `_03`. Tool: Blender 5.2 headless (Python), baked textures. Sources `ArtSource/EngProps/SpaceRocks/`.
  - Verdance props: `SM_Verd_Mushroom_01` to `_03`, `SM_Verd_Fern_01`, `_02`, `SM_Verd_GlowPlant_01`, `_02`, `SM_Verd_Rock_01` to `_03`. Tool: Blender 5.2 headless. Sources `ArtSource/EngProps/Verdance/`.
  - Cinder props: `SM_Cind_Crystal_01` to `_04`, `SM_Cind_Basalt_01` to `_03`, `SM_Cind_BasaltColumn_01`, `_02`. Tool: Blender 5.2 headless. Sources `ArtSource/EngProps/Cinder/`.
  - Verdance trees: 18 meshes `SM_TDVeg_<Species>_S<stage>_<seed>` (kapok, Brazil nut, trumpet tree, walking palm, tree fern, cacao as structural analogues). Tool: Unreal Engine 5.9 Procedural Vegetation Editor driven through the engine's MCP server. Botanical figures (heights, crown shapes) were read from Useful Tropical Plants and Wikipedia, with each value's source recorded in `.claude/skills/procedural-trees/biomes/verdance.json`; no media was taken from them.
  - `Content/TriadDrift/Gameplay/Materials/M_TD_PropTextured`: original material built by script (`Tools/td-import-props.py`).
- Null Armada trooper character (`Content/TriadDrift/Gameplay/Characters/NullTrooper/`, sources `ArtSource/EngCharacters/NullTrooper/`): armour and undersuit modelled in Blender 5.2 (headless Python, Claude Code sub-agent) on Epic's UE5 mannequin body and skeleton from the project template, after a concept image supplied by Ryan (`refs/enemy1.png`; **the tool that generated the concept image is to be stated by Ryan**). In progress, Nov 12.
- Null Armada enemy ships (`Content/TriadDrift/Gameplay/Characters/NullFleet/`: fighter, interceptor, gunship, bomber, carrier with launch bay and turret; sources `ArtSource/EngShips/NullFleet/`): original designs, code-built hard surface in Blender 5.2 (headless Python, Claude Code sub-agent) with baked textures; no third-party model, texture or scan. Engineering fallback, used while no `DA_Enemy_*_Visuals` exists in Presentation. Added by Ryan / Lead (Claude Code), 2026-11-13.
- Fallback player ships Rift, Lancer, Bastion (`Content/TriadDrift/Gameplay/Characters/Squad/`, sources `ArtSource/EngShips/Squad/`): original designs after the squad silhouettes in AGENTS.md, code-built in Blender 5.2 (headless Python, Claude Code sub-agent) with baked textures; no third-party source. Used only while no `DA_<Ship>_Visuals` of Nicholas's exists. Added by Ryan / Lead (Claude Code), 2026-11-13.
- On-foot player body trial (`Content/TriadDrift/Gameplay/Characters/Pilot/`): Sentinel Armor, a custom Triad Drift project asset supplied by Nicholas Tosti from his original character design and image (`ArtSource/Characters/Samples/SentinelArmor/`, board N-021). Nicholas confirms it as his original custom-owned model and explicitly authorizes immediate gameplay integration. Meshy AI was used in its creation. Specific subscription/license terms have not been independently verified; no exclusive legal-ownership conclusion is asserted by this credit.

## Primary-fire realism M02 — offline review only

Audio & Music agent used existing PlaceHolder Inc 50 Free Game Sounds Pack `Gunshot_1-1` and `Gunshot_7-1`, with Hove Terminal UI LITE recorded hardware actions `MACHAppl_VCRButton_HA_TerminalUI_04/08/13`. These replace the old Sci-Fi_Gun body for one audition; no new purchase/download/AI audio service. Gunshot effects are publisher-described human designs, with actual field-fire provenance unverified; the mechanical layer is recorded hardware, not a claim of a real firearm mechanism. Exact source hashes, selections, recipe, rights limits and credits: `ArtSource/Audio/PrototypeV1/PrimaryRealismM02/{README.md,source_inventory.json,recipe.json}` on nick/audio-prototype-v1 at4b94319. Processed/licensed WAVs stay ignored/local; no asset redistribution or final listening approval asserted.
## Menu-music preparation checkpoint (November12; not game-integrated)

- **Etherial Pulse**: Nicholas Tosti's selected Suno song, https://suno.com/song/b8ea12f4-b5a1-4bf2-811e-8d94f2acf26c. Nick confirmed authorship and Pro/Premier active at generation. Official MP3 preserved; existing ffmpeg/ffprobe produced an exact107s48kHz stereo16bit cut with ending fade. Audio agent's source/rights/processing record is local ignored Saved/AudioMusic/PrototypeV1/MenuTheme01/checkpoint.json; raw media is not published by this documentation update. No purchase/new generation. Native import, menu playback and in-game acceptance are unfinished. Rejected primary/switch prototypes remain historical references, not approved game audio.
- **Suno** is an AI music tool used by Nicholas for this supplied track; disclose it with other jam tools if the track ships.


### Custom Triad Drift Sentinel Armor - November12,2026

Nick supplied Meshy_AI_Sentinel_Armor_Turnaround_20261113_animated_ue5_mannequin_All_animation.glb. Original bytes are preserved under ArtSource/Characters/Samples/SentinelArmor/. Following the initial inspection transfer, Nick confirmed his original design, custom Triad Drift ownership attribution and Meshy AI creation tool, then expressly authorized immediate gameplay integration with no further licensing questions or development interruption. Ryan owns the playable-body import/integration in Gameplay/Characters/Pilot. Lead verified GLB container/size/SHA and Git LFS transfer only; actual import, animation and gameplay verification are reported separately by engineering.

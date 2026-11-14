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
- Dominion mothership exterior (`Content/TriadDrift/Gameplay/Props/Dominion/` meshes and triplanar panel materials, `Gameplay/Characters/Dominion/DA_Dominion_Visuals`; sources `ArtSource/EngShips/Dominion/`): original design, code-built in Blender 5.2 (headless Python, Claude Code sub-agent) with procedurally generated tileable textures; no third-party source. Engineering fallback until a Presentation `DA_Dominion_Visuals` exists. Added by Ryan / Lead (Claude Code), 2026-11-13.
- Planet set-piece kit, 26 pieces (outpost walls and doorway, signal jammer, crates, Verdance wreck parts, relay platforms and plateau, ruin pillar, choke rocks, stepping stones, console, door, extraction core, route posts, trailhead) in `Content/TriadDrift/Gameplay/Props/Planet/`, sources `ArtSource/EngProps/PlanetKit/`: original designs, code-built in Blender 5.2 (headless Python, Claude Code sub-agent) with baked and procedural textures; no third-party source. Added by Ryan / Lead (Claude Code), 2026-11-13.
- Cinder magma surface (`Content/TriadDrift/Gameplay/Props/Planet/SM_Cind_MagmaSurface`, `Materials/M_TD_Magma`, sources `ArtSource/EngProps/PlanetKit/SM_Cind_MagmaSurface/`): original, built by script in Blender 5.2 and Unreal (procedural tileable textures, world-space animated material); no third-party source. Added by Ryan / Lead (Claude Code), 2026-11-13.
- Engineering fallback sound set (ENG-35; `Content/TriadDrift/Gameplay/Audio/`, sources and recipes `ArtSource/EngAudio/`): 113 sounds in 200 takes, including `Music_Combat` and `Music_Intro`. Original work: synthesised from recipes (numpy oscillators, noise, filters, convolution reverb) and levelled and measured with ffmpeg; no samples and no third-party audio. Tools: Python + numpy (Blender 5.2's bundled Python), ffmpeg, Claude Code. Added by Ryan / Lead (Claude Code sub-agent), 2026-11-13. Not yet approved by ear.
- Engineering particle effects (ENG-37; `Content/TriadDrift/Gameplay/VFX/`, sources `ArtSource/EngVFX/`): `T_TD_VFX_Noise`, `M_TD_VFX_*` materials and `NS_TD_*` Niagara systems (tracers, muzzle flashes, engine exhaust, impacts, explosions). Original: generated procedurally (numpy script, shader maths) and assembled from Unreal Engine template emitters through the UE 5.9 Niagara toolset by Claude Code; no third-party assets. Added by Ryan / Lead (Claude Code sub-agent), 2026-11-13. Work in progress (round 2, reviewer 3/5).
- On-foot player body trial (`Content/TriadDrift/Gameplay/Characters/Pilot/`): Sentinel Armor, a custom Triad Drift project asset supplied by Nicholas Tosti from his original character design and image (`ArtSource/Characters/Samples/SentinelArmor/`, board N-021). Nicholas confirms it as his original custom-owned model and explicitly authorizes immediate gameplay integration. Meshy AI was used in its creation. Specific subscription/license terms have not been independently verified; no exclusive legal-ownership conclusion is asserted by this credit.

## Primary-fire realism M02 — offline review only

Audio & Music agent used existing PlaceHolder Inc 50 Free Game Sounds Pack `Gunshot_1-1` and `Gunshot_7-1`, with Hove Terminal UI LITE recorded hardware actions `MACHAppl_VCRButton_HA_TerminalUI_04/08/13`. These replace the old Sci-Fi_Gun body for one audition; no new purchase/download/AI audio service. Gunshot effects are publisher-described human designs, with actual field-fire provenance unverified; the mechanical layer is recorded hardware, not a claim of a real firearm mechanism. Exact source hashes, selections, recipe, rights limits and credits: `ArtSource/Audio/PrototypeV1/PrimaryRealismM02/{README.md,source_inventory.json,recipe.json}` on nick/audio-prototype-v1 at4b94319. Processed/licensed WAVs stay ignored/local; no asset redistribution or final listening approval asserted.
## Menu-music preparation checkpoint (November12; not game-integrated)

- **Etherial Pulse**: Nicholas Tosti's selected Suno song, https://suno.com/song/b8ea12f4-b5a1-4bf2-811e-8d94f2acf26c. Nick confirmed authorship and Pro/Premier active at generation. Official MP3 preserved; existing ffmpeg/ffprobe produced an exact107s48kHz stereo16bit cut with ending fade. Audio agent's source/rights/processing record is local ignored Saved/AudioMusic/PrototypeV1/MenuTheme01/checkpoint.json; raw media is not published by this documentation update. No purchase/new generation. Native import, menu playback and in-game acceptance are unfinished. Rejected primary/switch prototypes remain historical references, not approved game audio.
- **Suno** is an AI music tool used by Nicholas for this supplied track; disclose it with other jam tools if the track ships.


### Custom Triad Drift Sentinel Armor - November12,2026

Nick supplied Meshy_AI_Sentinel_Armor_Turnaround_20261113_animated_ue5_mannequin_All_animation.glb. Original bytes are preserved under ArtSource/Characters/Samples/SentinelArmor/. Following the initial inspection transfer, Nick confirmed his original design, custom Triad Drift ownership attribution and Meshy AI creation tool, then expressly authorized immediate gameplay integration with no further licensing questions or development interruption. Ryan owns the playable-body import/integration in Gameplay/Characters/Pilot. Lead verified GLB container/size/SHA and Git LFS transfer only; actual import, animation and gameplay verification are reported separately by engineering.

### Planet terrain materials (ENG-34) - November 13, 2026

`M_TD_Terrain`, `MI_Terrain_Verdance`, `MI_Terrain_Cinder` and textures `T_Terrain_Rock_M/N`, `T_Terrain_Detail_M/N` in `Content/TriadDrift/Gameplay/Props/Planets/`. Made during the jam by Ryan's side with Claude Code: textures generated procedurally with numpy (`ArtSource/EngProps/Planets/scripts/build_terrain_textures.py`), material graph built by Unreal editor Python (`ue_terrain_material.py`, `ue_terrain_build.py`). No third-party or downloaded assets; reuses the project's own `T_Ground_*` textures. Licence: original work.

### AI-generated concept art and terrain textures (Grok Bot, R-027) - November 13, 2026

Made during the jam by Grok Bot (Ryan's asset-generation agent) on Ryan's own paid accounts; every folder under `ArtSource/EngAI/**` has a `SOURCE.md` with the tool, endpoint, date, verbatim prompts, seeds and the licence text checked that day.

| Asset | Tool | Licence / plan | Added by |
|---|---|---|---|
| Concept turnarounds: Rift, Lancer, Bastion (r1, r2), Warden, Drone, Null Armada fleet (fighter, interceptor, gunship, bomber, carrier, turret) in `ArtSource/EngAI/{Ships,Characters}/**` (PRs #14, #15). Concepts only, used as image-to-3D inputs; not shipped in the game | fal.ai: OpenAI GPT Image 2 / 2.6 Edit, Google Nano Banana 2.1 Edit | fal model pages mark both "Commercial use"; pay-as-you-go credits incl. the Epic AI MegaJam fal coupon. Original designs, no logos or existing IP | Ryan / Grok Bot |
| Nine 2K seamless PBR terrain sets (Verdance moss, soil, cliff rock, peak lichen-frost; Cinder basalt, ash, obsidian, lava crust, cliff rock) in `ArtSource/EngAI/Terrain/**` (PR #16) | PBRgen (start texture: Nano Banana 2) | PBRgen Pro plan through the Epic AI MegaJam promo; commercial use included with a paid plan | Ryan / Grok Bot |

Tools used (add to the submission's "Tools you used"): fal.ai, PBRgen, Hyper3D Rodin (planned for the 3D generation of the approved concepts).

### Null Armada Drone (first Rodin model in the game) - November 13, 2026

| Asset | Tool | Licence / plan | Added by |
|---|---|---|---|
| `SM_NullDrone`, `T_NullDrone_BC/N/ORM/E`, `MI_NullDrone`, `DA_Crew_Drone_Visuals` in `Content/TriadDrift/Gameplay/Characters/NullDrone/` | Hyper3D Rodin Gen-2.5 image-to-3D, from our own concept image (`Drone_hero_gptimg2_PICK.png`, fal.ai) | Hyper3D terms, paid Business plan, commercial use (see `ArtSource/EngAI/Characters/Drone/mesh_r1/SOURCE.md`) | Generated by Ryan / Grok Bot; imported by Claude Code (Ryan's side) |

Tools used: **Hyper3D Rodin** (list it in the submission for the Rodin prize pool).

## Combat music candidate

Nicholas Tosti supplied his original AI-generated composition **Ethereal Pulse (Remix)** for the combat soundtrack. The Audio agent prepared the stereo loop for Unreal Engine 5.9. Its source receipt and preparation notes are on the audio branch at f5d9cf9d in ArtSource/Audio/CombatMusic01. Final inclusion awaits in-game listening.

### Retargeted template animations (animation lane) - November 13, 2026

116 clips under `Content/TriadDrift/Gameplay/Characters/{Crew,Pilot}/Anims/` (`A_Crew_*`, `A_Pilot_*`): Epic Games template mannequin animations shipped with the Unreal Engine First Person / Shooter template (`/Game/Characters/Mannequins/Anims`), retargeted onto `SKEL_NullCrew` and `SKEL_Pilot` with Unreal's IK Retargeter and re-timed by Claude Code (Ryan's side). Epic content under the Unreal Engine EULA; no third-party or AI-generated motion. Audit, scripts and review sheets: `ArtSource/EngAnim/`. The pilot rig is Nicholas's Sentinel Armor sample.

### Dominion interior kit (boarding finale) - November 13, 2026

`SM_DI_*` (22 meshes), `T_DI_Atlas_*`, `MI_DI_*`, `M_DI_Shield` in `Content/TriadDrift/Gameplay/Props/DominionInterior/`: original, code-built in Blender 5.2 (bmesh + a numpy procedural atlas) by Claude Code for Ryan T; no third-party assets. Sources, scripts and review sheets: `ArtSource/EngProps/DominionInterior/`.

### Squad ships and Null Armada fighter / gunship (Rodin models) - November 13, 2026

| Asset | Tool | Licence / plan | Added by |
|---|---|---|---|
| `SM_Rift`, `T_Rift_BC/N/ORM/E`, `MI_Rift` in `Content/TriadDrift/Gameplay/Ships/Rift/` | Hyper3D Rodin Gen-2.5 image-to-3D, from our own approved concept image (`ArtSource/EngAI/Ships/Rift/concept_r2`) | Hyper3D terms, paid Business plan, commercial use (see `ArtSource/EngAI/Ships/Rift/mesh_r1/SOURCE.md`) | Generated by Ryan / Grok Bot; sockets, material and import by Claude Code (Ryan's side) |
| `SM_Lancer`, `T_Lancer_BC/N/ORM/E`, `MI_Lancer` in `Content/TriadDrift/Gameplay/Ships/Lancer/` | Hyper3D Rodin Gen-2.5 image-to-3D, from our own approved concept image (`ArtSource/EngAI/Ships/Lancer/concept_r2`) | same (see `ArtSource/EngAI/Ships/Lancer/mesh_r1/SOURCE.md`) | same |
| `SM_Bastion`, `T_Bastion_BC/N/ORM/E`, `MI_Bastion` in `Content/TriadDrift/Gameplay/Ships/Bastion/` | Hyper3D Rodin Gen-2.5 image-to-3D, from our own approved concept image (`ArtSource/EngAI/Ships/Bastion/concept_r2`) | same (see `ArtSource/EngAI/Ships/Bastion/mesh_r1/SOURCE.md`) | same |
| `SM_NullFighter`, `T_NullFighter_BC/N/ORM/E`, `MI_NullFighter` in `Content/TriadDrift/Gameplay/Ships/NullFighter/` | Hyper3D Rodin Gen-2.5 image-to-3D, from our own approved concept image (`ArtSource/EngAI/Ships/NullFleet/Fighter/concept_r1`) | same (see `ArtSource/EngAI/Ships/NullFleet/Fighter/mesh_r1/SOURCE.md`) | same |
| `SM_NullGunship`, `T_NullGunship_BC/N/ORM/E`, `MI_NullGunship` in `Content/TriadDrift/Gameplay/Ships/NullGunship/` | Hyper3D Rodin Gen-2.5 image-to-3D, from our own approved concept image; decimated to 19,800 triangles in Blender by Grok Bot | same (see `ArtSource/EngAI/Ships/NullFleet/Gunship/mesh_r1/SOURCE.md`) | same |
| `SM_NullInterceptor`, `T_NullInterceptor_BC/N/ORM/E`, `MI_NullInterceptor` in `Content/TriadDrift/Gameplay/Ships/NullInterceptor/` (mesh_r2, the textured drop) | Hyper3D Rodin Gen-2.5 image-to-3D, from our own approved concept image (`ArtSource/EngAI/Ships/NullFleet/Interceptor/concept_r1`) | same (see `ArtSource/EngAI/Ships/NullFleet/Interceptor/mesh_r2/SOURCE.md`) | same |
| `SM_NullBomber`, `T_NullBomber_BC/N/ORM/E`, `MI_NullBomber` in `Content/TriadDrift/Gameplay/Ships/NullBomber/` | Hyper3D Rodin Gen-2.5 image-to-3D, from our own approved concept image (`ArtSource/EngAI/Ships/NullFleet/Bomber/concept_r1`) | same (see `ArtSource/EngAI/Ships/NullFleet/Bomber/mesh_r1/SOURCE.md`) | same |
| `SM_NullCarrier`, `T_NullCarrier_BC/N/ORM/E`, `MI_NullCarrier` in `Content/TriadDrift/Gameplay/Ships/NullCarrier/` (imported, not used by the game yet) | Hyper3D Rodin Gen-2.5 image-to-3D, from our own approved concept image (`ArtSource/EngAI/Ships/NullFleet/Carrier/concept_r1`) | same (see `ArtSource/EngAI/Ships/NullFleet/Carrier/mesh_r1/SOURCE.md`) | same |
| `SM_NullTurret`, `T_NullTurret_BC/N/ORM/E`, `MI_NullTurret` in `Content/TriadDrift/Gameplay/Ships/NullTurret/` (carrier and Dominion turrets) | Hyper3D Rodin Gen-2.5 image-to-3D, from our own approved concept image (`ArtSource/EngAI/Ships/NullFleet/Turret/concept_r1`) | same (see `ArtSource/EngAI/Ships/NullFleet/Turret/mesh_r1/SOURCE.md`) | same |
| `T_Rift_HeatMask`, `T_Lancer_HeatMask`, `T_Bastion_HeatMask` in the same folders | Computed in Blender from the Rodin meshes by Grok Bot (no generator; see `ArtSource/EngAI/Ships/<Ship>/heatmask_r1/SOURCE.md`) | project-authored | Grok Bot; imported by Claude Code (Ryan's side) |
| `M_TD_Ship` in `Content/TriadDrift/Gameplay/Ships/Materials/` | built by script (`ArtSource/EngShips/Rodin/scripts/ue_import_rodin_ships.py`) | project-authored | Claude Code (Ryan's side) |

Tools used: **Hyper3D Rodin** (list it in the submission for the Rodin prize pool), Blender 5.2.

### Pilot blade animations and the Warden (Grok Bot sources, built by the animation lane) - November 13, 2026

| Asset | Source / tool | Licence | Added by |
|---|---|---|---|
| `A_Pilot_Blade_*` (15 clips) and the re-timed `A_Pilot_Light1/2/3`, `A_Pilot_Heavy`, `A_Pilot_Dash`, `A_Pilot_HitReact_Front/_Back` in `Content/TriadDrift/Gameplay/Characters/Pilot/Anims/` | Motion generated with **HY-Motion 1.0** (Hunyuan Motion, `fal-ai/hunyuan-motion` and `/fast`) on fal.ai by Grok Bot from our own text prompts (`ArtSource/EngAI/Animations/PilotBlade/r1/SOURCE.md`); retargeted from SMPL-H to `SKEL_Pilot` with Unreal's IK Retargeter and cleaned by script (`ArtSource/EngAnim/PilotBlade`) | fal's model pages mark both endpoints "Commercial use"; fal terms of service; Ryan's paid fal.ai account | Grok Bot (generation), Claude Code (retarget and clean-up), Ryan's side |
| `SK_Warden`, `T_Warden_*`, `T_Warden_ShieldProp_*`, `T_Warden_GunArm_*`, `MI_Warden*`, `PA_Warden`, `DA_Crew_Warden_Visuals` in `Content/TriadDrift/Gameplay/Characters/Warden/` | **Hyper3D Rodin Gen-2.5** image-to-3D (body with the Rodin Human auto-rig, shield, gun arm) from our own approved concept images (`ArtSource/EngAI/Characters/Warden/{mesh_r1,props_r1}/SOURCE.md`); re-rigged onto the crew bone set, with the gun arm and shield swapped in, in Blender 5.2 (`ArtSource/EngCharacters/Warden`) | Hyper3D terms, paid Business plan, commercial use | Grok Bot (generation), Claude Code (rig and import), Ryan's side |
| `SKEL_Warden` in the same folder | a copy of the template mannequin skeleton asset (`/Game/Characters/Mannequins/Meshes/SK_Mannequin`) | Epic content, Unreal Engine EULA | Claude Code (Ryan's side) |

Tools used: **HY-Motion 1.0 (through fal.ai)**, **Hyper3D Rodin** (list both in the submission; Rodin for its prize pool), Blender 5.2.

Nicholas Tosti supplied Calm Loop (Dominion boarding), Cathedral Pulse (1) and Ethereal Pulse (Remix) (credits, in that order), and Verdance (planet theme). The Audio specialist prepared the stereo sources and imported them in Unreal Engine 5.9. Calm Loop uses a MetaSound. Source and tool records are in the audio branch at e7e35cd1, 337606bc and e3826232 respectively.

Nicholas Tosti supplied Infinity Zone for the Dominion space assault. Audio prepared the complete 139.1735-second stereo source and imported the SoundWave with the Unreal Engine editor Python API. The original file receipt is in release issue46-infinity-zone-source. Preparation and native verification are in ArtSource/Audio/DominionAssault01 at 6db3aa98.

Nicholas Tosti supplied and selected **Push The Attack** for the Dominion space assault, superseding Infinity Zone for that scene. Audio used FFmpeg to decode the full MP3 to 48 kHz stereo PCM and the Unreal Engine editor Python API to import and reload the looping SoundWave. The input fingerprint, preparation and verification records are in [DominionAssault02](ArtSource/Audio/DominionAssault02/source_manifest.json).

The Verdance buried-structure scan and the Cinder comparison overlay were created during the jam by Nicholas Tosti’s UI team using OpenAI image generation. The Cinder scan used the Verdance original as its only image reference. No third-party stock images or fonts were used. [Originals, prompts and preparation](https://github.com/rthomas24/2026-epic-ai-megajam/tree/b4086108/ArtSource/UI/TriadHUD/Unreal/SurveyTerminals01).

Nicholas Tosti supplied Ambient1 for quiet space flight. Audio used FFmpeg to decode the full stereo track and the Unreal Engine 5.9 editor Python API to import and reload its SoundWave. [Source and tool record](ArtSource/Audio/IdleAmbient01/source_manifest.json).

Credits playlist implementation and verification: Nicholas Tosti supplied the music; the presentation team used OpenAI Codex, Unreal Engine 5.9, Windows WASAPI process capture, FFmpeg and Python for playback, lossless excerpting and source comparison. [Listen to the game recording](docs/visual/Submission/MusicBuild01/credits-playback.md).

Nicholas Tosti supplied the Suno track Ethereal Pulse (Remix) for combat. Audio prepared the full 139.2-second loop with FFmpeg and Python and imported the stereo SoundWave in Unreal Engine 5.9. [Track provenance and preparation](https://github.com/rthomas24/2026-epic-ai-megajam/blob/nick/audio-prototype-v1/ArtSource/Audio/CombatMusic01/CREDITS.md).

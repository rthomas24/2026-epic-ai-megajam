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
- Null Armada enemy ships (`Content/TriadDrift/Gameplay/Characters/NullFleet/`: fighter, interceptor, gunship, bomber, carrier with launch bay and turret; sources `ArtSource/EngShips/NullFleet/`): original designs, code-built hard surface in Blender 5.2 (headless Python, Claude Code sub-agent) with baked textures; no third-party model, texture or scan. Engineering fallback, used while no `DA_Enemy_*_Visuals` exists in Presentation. Added by Ryan / Lead (Claude Code), 2026-11-13. **Not shipped** (package ddc50a2f, Nov 14): the six code-built ship meshes `SM_Null_Fighter / _Interceptor / _Gunship / _Bomber / _Carrier / _Turret` and their materials and textures are excluded from the cook; the game ships the Rodin ships credited below. Still shipped from this set: the carrier launch bay (`SM_Null_CarrierBay`, its textures), the master `M_TD_NullFleet` and two leftover bomber textures (`T_Null_Bomber_E`, `_ORM`).
- Fallback player ships Rift, Lancer, Bastion (`Content/TriadDrift/Gameplay/Characters/Squad/`, sources `ArtSource/EngShips/Squad/`): original designs after the squad silhouettes in AGENTS.md, code-built in Blender 5.2 (headless Python, Claude Code sub-agent) with baked textures; no third-party source. Used only while no `DA_<Ship>_Visuals` of Nicholas's exists. Added by Ryan / Lead (Claude Code), 2026-11-13. **Not shipped** (package ddc50a2f, Nov 14): the three code-built meshes, `M_TD_Squad`, their instances and textures are excluded from the cook; only the `DA_<Ship>_Visuals` assets in that folder ship, pointing at the Rodin ships credited below.
- Dominion mothership exterior (`Content/TriadDrift/Gameplay/Props/Dominion/` meshes and triplanar panel materials, `Gameplay/Characters/Dominion/DA_Dominion_Visuals`; sources `ArtSource/EngShips/Dominion/`): original design, code-built in Blender 5.2 (headless Python, Claude Code sub-agent) with procedurally generated tileable textures; no third-party source. Engineering fallback until a Presentation `DA_Dominion_Visuals` exists. Added by Ryan / Lead (Claude Code), 2026-11-13. **Partly shipped** (package ddc50a2f, Nov 14): `SM_Dominion_Reactor`, `SM_Dominion_ShieldEmitter`, the panel and glow materials and `T_Dominion_Panel*` ship; `SM_Dominion_Hull`, `_Bridge`, `_TrenchWall`, `_Generator`, `_GeneratorCore` and three of the instances are **not shipped** (replaced by Grok Bot's hull r2 and shield generator, credited below).
- Planet set-piece kit, 26 pieces (outpost walls and doorway, signal jammer, crates, Verdance wreck parts, relay platforms and plateau, ruin pillar, choke rocks, stepping stones, console, door, extraction core, route posts, trailhead) in `Content/TriadDrift/Gameplay/Props/Planet/`, sources `ArtSource/EngProps/PlanetKit/`: original designs, code-built in Blender 5.2 (headless Python, Claude Code sub-agent) with baked and procedural textures; no third-party source. Added by Ryan / Lead (Claude Code), 2026-11-13. **Partly shipped** (package ddc50a2f, Nov 14): the Verdance pieces `SM_Verd_Wreck_Hull / _Engine / _Fin / _Wing`, `SM_Verd_RelayAltar`, `SM_Verd_RelayPlatform_S / _M / _L`, `SM_Verd_Plateau`, `SM_Verd_RuinPillar`, `SM_Verd_NodeStub` and their textures are **not shipped** (unplaced since the kit-built sites); the Cinder pieces, console, door, extraction core, route post, trailhead and `SM_Verd_Log` ship.
- Cinder magma surface (`Content/TriadDrift/Gameplay/Props/Planet/SM_Cind_MagmaSurface`, `Materials/M_TD_Magma`, sources `ArtSource/EngProps/PlanetKit/SM_Cind_MagmaSurface/`): original, built by script in Blender 5.2 and Unreal (procedural tileable textures, world-space animated material); no third-party source. Added by Ryan / Lead (Claude Code), 2026-11-13.
- Engineering fallback sound set (ENG-35; `Content/TriadDrift/Gameplay/Audio/`, sources and recipes `ArtSource/EngAudio/`): 113 sounds in 200 takes, including `Music_Combat` and `Music_Intro`. Original work: synthesised from recipes (numpy oscillators, noise, filters, convolution reverb) and levelled and measured with ffmpeg; no samples and no third-party audio. Tools: Python + numpy (Blender 5.2's bundled Python), ffmpeg, Claude Code. Added by Ryan / Lead (Claude Code sub-agent), 2026-11-13. Not yet approved by ear.
- Engineering particle effects (ENG-37; `Content/TriadDrift/Gameplay/VFX/`, sources `ArtSource/EngVFX/`): `T_TD_VFX_Noise`, `M_TD_VFX_*` materials and `NS_TD_*` Niagara systems (tracers, muzzle flashes, engine exhaust, impacts, explosions). Original: generated procedurally (numpy script, shader maths) and assembled from Unreal Engine template emitters through the UE 5.9 Niagara toolset by Claude Code; no third-party assets. Added by Ryan / Lead (Claude Code sub-agent), 2026-11-13. Work in progress (round 2, reviewer 3/5). **Not shipped** (package ddc50a2f, Nov 14): the `VFX/Probe/` test systems, `NS_TD_Entry_*` (five), `NS_TD_Blade_Swing`, `NS_TD_Rift_RingIdle`, `M_TD_VFX_FlipDebris`, `M_TD_VFX_FlipShield`; the rest ships.
- On-foot player body trial (`Content/TriadDrift/Gameplay/Characters/Pilot/`): Sentinel Armor, a custom Triad Drift project asset supplied by Nicholas Tosti from his original character design and image (`ArtSource/Characters/Samples/SentinelArmor/`, board N-021). Nicholas confirms it as his original custom-owned model and explicitly authorizes immediate gameplay integration. Meshy AI was used in its creation. Specific subscription/license terms have not been independently verified; no exclusive legal-ownership conclusion is asserted by this credit.

Subscription clarification recorded November 14: Nicholas reports that he has **Meshy Pro**. This is his reported subscription, not independent verification of the plan active at each generation/download or of the applicable terms. His existing project-asset and integration instructions remain in force; the narrower evidence gaps are retained for submission reporting without repeating the subscription question.

Pre-jam disclosure recorded from Nicholas's explicit R-037 answer: **Coilgun Assault Rifle, Crimson Vanguard, Nebula Shadowfighter, Spaceship (white hull, cyan ring engines), Circular Bulkhead Wall, Gothic Stone Arch, Crimson Tempest, and UnlabelledRifle01** were made before November 12, 2026. Declare the ones included in the final submitted build. This is the supplied-model creation-date statement, not proof all eight are imported or shipped, and does not assign those dates to Sentinel Armor or other supplied files. The Ancient Relay reference later superseded the Gothic Stone Arch artifact proposal.

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
| `SM_NullCarrier`, `T_NullCarrier_BC/N/ORM/E`, `MI_NullCarrier` in `Content/TriadDrift/Gameplay/Ships/NullCarrier/` (imported, not used by the game yet). Shipped (package ddc50a2f, Nov 14). | Hyper3D Rodin Gen-2.5 image-to-3D, from our own approved concept image (`ArtSource/EngAI/Ships/NullFleet/Carrier/concept_r1`) | same (see `ArtSource/EngAI/Ships/NullFleet/Carrier/mesh_r1/SOURCE.md`) | same |
| `SM_NullTurret`, `T_NullTurret_BC/N/ORM/E`, `MI_NullTurret` in `Content/TriadDrift/Gameplay/Ships/NullTurret/` (carrier and Dominion turrets) | Hyper3D Rodin Gen-2.5 image-to-3D, from our own approved concept image (`ArtSource/EngAI/Ships/NullFleet/Turret/concept_r1`) | same (see `ArtSource/EngAI/Ships/NullFleet/Turret/mesh_r1/SOURCE.md`) | same |
| `T_Rift_HeatMask`, `T_Lancer_HeatMask`, `T_Bastion_HeatMask` in the same folders | Computed in Blender from the Rodin meshes by Grok Bot (no generator; see `ArtSource/EngAI/Ships/<Ship>/heatmask_r1/SOURCE.md`) | project-authored | Grok Bot; imported by Claude Code (Ryan's side) |
| `M_TD_Ship` in `Content/TriadDrift/Gameplay/Ships/Materials/` | built by script (`ArtSource/EngShips/Rodin/scripts/ue_import_rodin_ships.py`) | project-authored | Claude Code (Ryan's side) |
| `M_TD_RiftPortal`, `M_TD_ShipGlow`, `T_TD_ShipDetail_RM` in `Content/TriadDrift/Gameplay/Ships/Materials/`. `M_TD_RiftPortal` is **not shipped** (package ddc50a2f, Nov 14); the other two ship. | built by script (same file; HLSL custom nodes, an 8 px neutral map) | project-authored | Claude Code (Ryan's side) |
| `SM_Rift` (hybrid), `SM_Rift_PortalDisc`, `T_Rift_Parts_BC/N/ORM/E/HeatMask`, `MI_Rift_Parts`, `MI_Rift_Portal` in `Content/TriadDrift/Gameplay/Ships/Rift/`. `MI_Rift_Portal` is **not shipped** (package ddc50a2f, Nov 14); the rest ships. | Blender 5.2 (ring, engines, tail and portal disc modelled and baked by the ship polish builder, `ArtSource/EngShips/RiftHero`) on the Hyper3D Rodin hull above | project-authored over the Rodin output (same plan) | Claude Code (Ryan's side) |
| `T_{Rift,Lancer,Bastion}_Detail_N/_RM` in the ships' folders | Grok Bot, SquadDetail_r1: procedural (Blender + NumPy / OpenCV, no generator; `ArtSource/EngAI/Ships/SquadDetail_r1/README.md`) | project-authored | Grok Bot; imported by Claude Code (Ryan's side) |
| `T_Lancer_BC/E`, `T_Bastion_BC/E` (textures_r2) | Grok Bot: OpenCV clean-up of the Rodin maps above, no generator (`ArtSource/EngAI/Ships/{Lancer,Bastion}/textures_r2/SOURCE.md`) | same as the Rodin output | Grok Bot; imported by Claude Code (Ryan's side) |

Tools used: **Hyper3D Rodin** (list it in the submission for the Rodin prize pool), Blender 5.2.

### Pilot blade animations and the Warden (Grok Bot sources, built by the animation lane) - November 13, 2026

| Asset | Source / tool | Licence | Added by |
|---|---|---|---|
| `A_Pilot_Blade_*` (15 clips) and the re-timed `A_Pilot_Light1/2/3`, `A_Pilot_Heavy`, `A_Pilot_Dash`, `A_Pilot_HitReact_Front/_Back` in `Content/TriadDrift/Gameplay/Characters/Pilot/Anims/` | Motion generated with **HY-Motion 1.0** (Hunyuan Motion, `fal-ai/hunyuan-motion` and `/fast`) on fal.ai by Grok Bot from our own text prompts (`ArtSource/EngAI/Animations/PilotBlade/r1/SOURCE.md`); retargeted from SMPL-H to `SKEL_Pilot` with Unreal's IK Retargeter and cleaned by script (`ArtSource/EngAnim/PilotBlade`) | fal's model pages mark both endpoints "Commercial use"; fal terms of service; Ryan's paid fal.ai account | Grok Bot (generation), Claude Code (retarget and clean-up), Ryan's side |
| `SK_Warden`, `T_Warden_*`, `T_Warden_ShieldProp_*`, `T_Warden_GunArm_*`, `MI_Warden*`, `PA_Warden`, `DA_Crew_Warden_Visuals` in `Content/TriadDrift/Gameplay/Characters/Warden/` | **Hyper3D Rodin Gen-2.5** image-to-3D (body with the Rodin Human auto-rig, shield, gun arm) from our own approved concept images (`ArtSource/EngAI/Characters/Warden/{mesh_r1,props_r1}/SOURCE.md`); re-rigged onto the crew bone set, with the gun arm and shield swapped in, in Blender 5.2 (`ArtSource/EngCharacters/Warden`) | Hyper3D terms, paid Business plan, commercial use | Grok Bot (generation), Claude Code (rig and import), Ryan's side |
| `SKEL_Warden` in the same folder | a copy of the template mannequin skeleton asset (`/Game/Characters/Mannequins/Meshes/SK_Mannequin`) | Epic content, Unreal Engine EULA | Claude Code (Ryan's side) |

Tools used: **HY-Motion 1.0 (through fal.ai)**, **Hyper3D Rodin** (list both in the submission; Rodin for its prize pool), Blender 5.2.

### Survey outpost, Ancient Relay and Verdance wreck (Rodin props, made walkable by engineering) - November 13, 2026

| Asset | Tool | Licence / plan | Added by |
|---|---|---|---|
| `SM_Outpost_Cabin`, `SM_Outpost_Console`, `T_Outpost_*`, `M_Outpost`, `MI_Outpost_*`, `BP_Outpost_Site_*` in `Content/TriadDrift/Gameplay/Props/Outpost/`. **Not shipped** (package ddc50a2f, Nov 14): the Rodin cabin and console, `M_Outpost`, their instances and textures are excluded from the cook; only the two `BP_Outpost_Site_*` classes ship, and they are built from the engineering site kit. | Hyper3D Rodin Gen-2.5 image-to-3D, from a cleaned concept (fal) of Nicholas's modelling reference (`ArtSource/EngAI/Props/VerdanceOutpost/mesh_r1/SOURCE.md`); ramp, bay floor, piles, lights and collision added in Blender (`ArtSource/EngProps/Outpost/`) | Hyper3D terms, paid Business plan, commercial use (see that `SOURCE.md`) | Generated by Grok Bot for Ryan; walkability, materials and import by Claude Code (Ryan's side) |
| `SM_Relay`, `T_Relay_*`, `M_Relay`, `MI_Relay`, `MI_Relay_Awake`, `BP_Relay`, `BP_Relay_Awake` in `Content/TriadDrift/Gameplay/Props/Relay/` | Hyper3D Rodin Gen-2.5 image-to-3D, from a cleaned concept (fal) of Nicholas's Ancient Relay modelling reference (`ArtSource/EngAI/Props/AncientRelay/mesh_r1/SOURCE.md`); stair block re-sloped to 20 degrees and collision added in Blender (`ArtSource/EngProps/Outpost/Relay/`) | Hyper3D terms, paid Business plan, commercial use (see that `SOURCE.md`) | Generated by Grok Bot for Ryan; walkability, materials and import by Claude Code (Ryan's side) |
| `SM_Wreck`, `T_Wreck_*`, `M_Wreck`, `MI_Wreck`, `BP_Wreck` in `Content/TriadDrift/Gameplay/Props/Wreck/` | Hyper3D Rodin Gen-2.5 image-to-3D, from a cleaned concept of Nicholas's crashed-ship modelling reference (`ArtSource/EngAI/Props/VerdanceWreck/mesh_r1/SOURCE.md`); ramp, failing lights (emissive authored by us) and collision added in Blender (`ArtSource/EngProps/Outpost/Wreck/`) | Hyper3D terms, paid Business plan, commercial use (see that `SOURCE.md`) | Generated by Grok Bot for Ryan; walkability, materials and import by Claude Code (Ryan's side) |

Tools used: **Hyper3D Rodin** (list it in the submission for the Rodin prize pool), fal.ai, Blender 5.2.

Nicholas Tosti supplied Calm Loop (Dominion boarding), Cathedral Pulse (1) and Ethereal Pulse (Remix) (credits, in that order), and Verdance (planet theme). The Audio specialist prepared the stereo sources and imported them in Unreal Engine 5.9. Calm Loop uses a MetaSound. Source and tool records are in the audio branch at e7e35cd1, 337606bc and e3826232 respectively.

Nicholas Tosti supplied Infinity Zone for the Dominion space assault. Audio prepared the complete 139.1735-second stereo source and imported the SoundWave with the Unreal Engine editor Python API. The original file receipt is in release issue46-infinity-zone-source. Preparation and native verification are in ArtSource/Audio/DominionAssault01 at 6db3aa98.

Nicholas Tosti supplied and selected **Push The Attack** for the Dominion space assault, superseding Infinity Zone for that scene. Audio used FFmpeg to decode the full MP3 to 48 kHz stereo PCM and the Unreal Engine editor Python API to import and reload the looping SoundWave. The input fingerprint, preparation and verification records are in [DominionAssault02](ArtSource/Audio/DominionAssault02/source_manifest.json).

The Verdance buried-structure scan and the Cinder comparison overlay were created during the jam by Nicholas Tosti’s UI team using OpenAI image generation. The Cinder scan used the Verdance original as its only image reference. No third-party stock images or fonts were used. [Originals, prompts and preparation](https://github.com/rthomas24/2026-epic-ai-megajam/tree/b4086108/ArtSource/UI/TriadHUD/Unreal/SurveyTerminals01).

Nicholas Tosti supplied Ambient1 for quiet space flight. Audio used FFmpeg to decode the full stereo track and the Unreal Engine 5.9 editor Python API to import and reload its SoundWave. [Source and tool record](ArtSource/Audio/IdleAmbient01/source_manifest.json).

Credits playlist implementation and verification: Nicholas Tosti supplied the music; the presentation team used OpenAI Codex, Unreal Engine 5.9, Windows WASAPI process capture, FFmpeg and Python for playback, lossless excerpting and source comparison. [Listen to the game recording](docs/visual/Submission/MusicBuild01/credits-playback.md).

Nicholas Tosti supplied the Suno track Ethereal Pulse (Remix) for combat. Audio prepared the full 139.2-second loop with FFmpeg and Python and imported the stereo SoundWave in Unreal Engine 5.9. [Track provenance and preparation](https://github.com/rthomas24/2026-epic-ai-megajam/blob/nick/audio-prototype-v1/ArtSource/Audio/CombatMusic01/CREDITS.md).

### Grok Bot deliveries imported by the import builder (Dominion hull r2, shield generator, Cinder anchor, Null hauler, drill rig) - November 13, 2026

| Asset | Tool | Licence / plan | Added by |
|---|---|---|---|
| `SM_DominionMothership`, `T_DominionMothership_BC/N/ORM/E/Detail_N/Detail_ORM`, `MI_DominionMothership` in `Content/TriadDrift/Gameplay/Ships/DominionMothership/` (PR #72) | Blender 4.5.3, procedural hard-surface build and NumPy textures by Grok Bot from the approved fal concept v3 (`ArtSource/EngAI/Ships/DominionMothership/mesh_r2/SOURCE.md`); no AI 3D generator | project-authored (concept licence in `concept_r1/SOURCE.md`) | Grok Bot; import, material and wiring by Claude Code (Ryan's side) |
| `SM_DominionShieldGenerator`, `SM_DominionShieldGeneratorCore`, their `T_*_BC/N/ORM/E`, `MI_*` in `Content/TriadDrift/Gameplay/Ships/DominionShieldGenerator/` (PR #74) | Blender 4.5.3, procedural hard-surface by Grok Bot (`ArtSource/EngAI/Ships/DominionShieldGenerator/mesh_r1/SOURCE.md`); no AI 3D generator | project-authored | same |
| `SM_CinderAnchorTower`, `SM_CinderAnchorEmitter`, their `T_*_BC/N/ORM/E`, `MI_*` in `Content/TriadDrift/Gameplay/Props/CinderAnchor/` (PR #73) | **Hyper3D Rodin Gen-2.5** image-to-3D from a cleaned fal concept of Nicholas's modelling reference, Blender 4.5.3 polish (`ArtSource/EngAI/Props/CinderFieldAnchor/mesh_r1/SOURCE.md`) | Hyper3D terms, paid Business plan, commercial use | same |
| `SM_NullHauler`, `T_NullHauler_BC/N/ORM/E`, `MI_NullHauler` in `Content/TriadDrift/Gameplay/Ships/NullHauler/`, `DA_Enemy_Hauler_Visuals` in `Gameplay/Characters/NullFleet/` (PR #68) | **Hyper3D Rodin Gen-2.5** image-to-3D from Nicholas's four-view board; emissive map authored in Blender (`ArtSource/EngAI/Ships/NullHauler/mesh_r1/SOURCE.md`) | Hyper3D terms, paid Business plan, commercial use | same |
| `SM_CinderDrillRig`, `T_CinderDrillRig_BC/N/ORM/E`, `MI_CinderDrillRig` in `Content/TriadDrift/Gameplay/Props/CinderDigSite/`, placed in `BP_DigSite_Cabin` (PR #69) | **Hyper3D Rodin Gen-2.5** image-to-3D from a cleaned fal concept of Nicholas's dig-site board, Blender 4.5.3 clean-up (`ArtSource/EngAI/Props/CinderDigSite/mesh_r1/SOURCE.md`) | Hyper3D terms, paid Business plan, commercial use | same |

Tools used: **Hyper3D Rodin** (list it in the submission for the Rodin prize pool), fal.ai, Blender 4.5.3, Claude Code.

Credits playlist implementation and verification: Nicholas Tosti supplied the music; the presentation team used OpenAI Codex, Unreal Engine 5.9, Windows WASAPI process capture, FFmpeg and Python for playback, lossless excerpting and source comparison. [Listen to the game recording](docs/visual/Submission/MusicBuild01/credits-playback.md).

Nicholas Tosti supplied the Suno track Ethereal Pulse (Remix) for combat. Audio prepared the full 139.2-second loop with FFmpeg and Python and imported the stereo SoundWave in Unreal Engine 5.9. [Track provenance and preparation](https://github.com/rthomas24/2026-epic-ai-megajam/blob/nick/audio-prototype-v1/ArtSource/Audio/CombatMusic01/CREDITS.md).

Nicholas Tosti supplied the Verdance theme. For the loop revision, the Lead used Python to smooth its stereo endpoints, Unreal Engine 5.9 for native loop playback, and NumPy and FFmpeg to check and extract the original game recording. The earlier full-length source and asset remain available in project history.


Candidate audio, not merged to the game: Nicholas supplied five Magic Spell Impact recordings whose embedded metadata identifies rawAmbience. Audio used Python, FFmpeg, Unreal Engine 5.9 and Windows process audio capture to prepare and audition nine Lancer replacements. Source receipts, processing and the unresolved exact acquisition/redistribution record are preserved at https://github.com/rthomas24/2026-epic-ai-megajam/tree/2b074bb5/ArtSource/Audio/Combat01/ProvidedMAGSpel01 . Standalone supplied/derived WAVs remain private and excluded from that branch. This entry records provenance; it does not assert verified licensing or final selection.


Industrial reward candidate (59ec2f79, draft PR 85): Nicholas supplied the industrial UI direction. The UI specialist used OpenAI image generation to create the original text-free terminal bezel, and Unreal Engine 5.9 for the native texture and Slate presentation. The unchanged generated PNG, generation prompt and source/tool provenance are preserved in ArtSource/UI/TriadHUD/Unreal/IndustrialTerminal01 on that candidate branch. No third-party stock imagery or fonts were incorporated into the bezel. Appearance and Shipping approval remain pending.

Original Cathedral Pulse core-chamber selection: supplied by Nicholas Tosti, preserved with its source receipt and fingerprint in ArtSource/Audio/DominionCore01. Lead used Python and FFmpeg to decode the complete 69.560-second track to a stereo 48 kHz PCM16 import master, without trimming, looping or changing gain, then Unreal Engine 5.9 to import and independently reload the SoundWave. This is separate from Cathedral Pulse (1), used for credits. Native data verified; gameplay activation and listening approval remain pending.

Cinder World and Cinder Fall: supplied by Nicholas Tosti and explicitly assigned by him to Cinder exploration and combat, respectively. Audio used Python and FFmpeg to prepare full-duration stereo PCM16 masters, preserving both original MP3s and applying no gain, trim, loop or crossfade. Source receipts and measurements are in ArtSource/Audio/CinderMusic01. A possible later combat entry is for audition; no gameplay start offset is approved or applied by this source preparation. Native import, playback and mix acceptance remain separate.
- **Null Trooper rifle clips (13) and Warden fire / summon clips (4)** (`ArtSource/EngAI/Animations/CrewTrooper/r1`, `ArtSource/EngAI/Animations/Warden/r1`; in game as `A_Crew_Trooper_*` and `A_Crew_Warden_*_01/02`): motion generated with HY-Motion 1.0 via fal.ai (AI-generated; commercial use per the fal terms recorded in each SOURCE.md), by Grok Bot for Ryan's side; retargeted in Blender; imported by the animation builder (Claude Code). PRs #86 and #87, Nov 13.
- **Null Armada crew death clips and attack clips** (`ArtSource/EngAI/Animations/EnemyDeaths/r1`, `ArtSource/EngAI/Animations/EnemyAttacks/r1`; in game as `A_Crew_<Type>_Death_*`, `A_Crew_Heavy_ShieldBash`, `A_Crew_Grunt_Melee_*` / `_Charge`, `A_Crew_Trooper_Fire_Burst` and the `A_Crew_Warden_Slam` blend): motion generated with HY-Motion 1.0 via fal.ai (AI-generated; commercial use per the fal terms recorded in each SOURCE.md), by Grok Bot for Ryan's side; retargeted, cleaned and blended by the animation builder (Claude Code), with hand-authored versions kept as `*_Authored`. Nov 14.

Nicholas Tosti supplied the Verdance theme. For the loop revision, the Lead used Python to smooth its stereo endpoints, Unreal Engine 5.9 for native loop playback, and NumPy and FFmpeg to check and extract the original game recording. The earlier full-length source and asset remain available in project history.

### Grok Bot deliveries imported by the import builder, second batch (outpost set dressing, crystal cluster r2) - November 13, 2026

| Asset | Tool | Licence / plan | Added by |
|---|---|---|---|
| `SM_Outpost_{CargoCrate,SupplyCrate,FieldTerminal,DataKiosk,CommAntenna,CommsDish,Floodlight,BollardLight,Generator,PowerJunction}`, `T_OutpostProps_BC/N/ORM/E`, `MI_OutpostProps` in `Content/TriadDrift/Gameplay/Props/OutpostDressing/` (PR #79) | Blender 4.5.3, procedural hard-surface by Grok Bot after Nicholas's landing-pad board (`ArtSource/EngAI/Props/OutpostSetDressing/mesh_r1/SOURCE.md`); no AI 3D generator | project-authored | Grok Bot; import by Claude Code (Ryan's side) |
| `SM_CrystalCacheCrystal` r2, `T_CrystalCacheCrystal_*` in `Content/TriadDrift/Gameplay/Props/CrystalCache/` (PR #80) | Blender 4.5.3, procedural bmesh by Grok Bot (`ArtSource/EngAI/Props/CrystalCache/mesh_r2/SOURCE.md`); no AI 3D generator | project-authored | same |

### Grok Bot deliveries not credited before (asset ledger pass, ART-02) - November 13, 2026

Made during the jam by Grok Bot (Ryan's asset-generation agent) on Ryan's own paid accounts. The facts below are copied from each folder's `SOURCE.md`; states and use are in `docs/eng/ASSETS.md` 2.8.

| Asset | Tool | Licence / plan | Added by |
|---|---|---|---|
| Effect flipbooks `T_TD_VFX_Grok_*` in `Content/TriadDrift/Gameplay/VFX/Textures/` (explosion, smoke, fire, engine exhaust, shockwave, debris; sources `ArtSource/EngAI/VFX/**`). Shipped (package ddc50a2f, Nov 14): explosion, smoke, engine exhaust, shockwave ring (AI-generated) and electric arc, plasma bolt (procedural). **Not shipped**: fire, debris chunks (AI-generated) and shield hex ripple (procedural). | **AI-generated video**: fal.ai, Alibaba Wan 3.0 Prime text-to-video (`alibaba/wan-3.0-prime/text-to-video`), frames packed locally with ffmpeg, Pillow and NumPy. Electric arc, glow, plasma bolt, shield hex ripple and spark are procedural (Python, NumPy, Pillow; no generator) | fal terms: content generated through the fal API is cleared for commercial projects (each `r1/SOURCE.md`); Ryan's paid fal.ai account | Grok Bot; imported and wired by Claude Code (Ryan's side, effects builder) |
| Descent textures (PR #33): `T_Descent_CloudDeck` in `Content/TriadDrift/Gameplay/Props/Sky/` (in the game); cloud wisp, cloud punch, plasma sheath, heat streak, shock cone in `ArtSource/EngAI/Descent/**`. Shipped (package ddc50a2f, Nov 14) besides the cloud deck: `T_TD_VFX_Grok_CloudWisp_8x8`, `_CloudPunch_8x8`, `_PlasmaSheath_8x8` (AI-generated video flipbooks) and `_HeatStreak`, `_ShockCone` (procedural) in `Content/TriadDrift/Gameplay/VFX/Textures/`. | **AI-generated**: fal.ai, Nano Banana 2 (`fal-ai/nano-banana-2`) for the cloud deck, made tileable locally; Alibaba Wan 3.0 Prime text-to-video for the flipbooks; heat streak and shock cone procedural (no generator) | fal terms, cleared for commercial projects; Ryan's paid fal.ai account | Grok Bot; cloud deck material by Claude Code (Ryan's side, environment builder) |
| `SM_CrystalCacheCradle`, `T_CrystalCacheCradle_BC/N/ORM/E`, `MI_CrystalCacheCradle` in `Content/TriadDrift/Gameplay/Props/CrystalCache/` (PR #75) | **Hyper3D Rodin** image-to-3D from Nicholas's crystal-cache board, then Blender 4.5.3 polish (`ArtSource/EngAI/Props/CrystalCache/mesh_r1/SOURCE.md`). The r1 hero crystal in the same PR was procedural Blender, no generator | Hyper3D terms, paid Business plan, commercial use | Grok Bot; import by Claude Code (Ryan's side) |
| Energy blade concept (PR #26) and mesh `SM_EnergyBlade_Deployed` / `_Folded` (PR #47) in `ArtSource/EngAI/Weapons/EnergyBlade/**`. Source only: not in the game | Concept: **AI-generated image**, fal.ai, Google Nano Banana 2.1. Mesh: **Hyper3D Rodin Gen-2.5** image-to-3D | fal terms; Hyper3D terms, paid Business plan, commercial use | Grok Bot |
| Dominion mothership concept v3 (PR #65) in `ArtSource/EngAI/Ships/DominionMothership/concept_r1/`. Concept only, the design input of the hull | **AI-generated image**: fal.ai, Google Nano Banana 2.1 (text-to-image and edit) | fal terms, cleared for commercial projects | Grok Bot; picked by Nicholas |
| Dominion hull mesh_r1 (PR #70) in `ArtSource/EngAI/Ships/DominionMothership/mesh_r1/`. Source only: superseded by mesh_r2 | Blender 4.5.3 procedural hard-surface, NumPy textures; no AI 3D generator | project-authored | Grok Bot |
| Dominion docking bay plans, blockout, `SM_DominionBay_Door_L/R`, `SM_DominionBay_TractorEmitter`, `T_DominionBay_Trim_*` (PR #77) in `ArtSource/EngAI/Props/DominionDockingBay/plans_r1/`. Source only | Blender 4.5.3, matplotlib, NumPy; no AI generator | project-authored | Grok Bot |
| Dominion interior plans, sections, trim sheet and decal atlas (PR #82) in `ArtSource/EngAI/Props/DominionInterior/plans_r1/`. Source only | Blender 4.5.3, matplotlib, NumPy; no AI generator | project-authored | Grok Bot |
| Rift mesh_r2 (PR #67) in `ArtSource/EngAI/Ships/Rift/mesh_r2/`. Source only | Blender 4.5.3 (Python / bmesh), OpenCV and NumPy for textures; no generator | project-authored over the Rodin output | Grok Bot |
| Anchor tower plinth collision in `SM_CinderAnchorTower` (`ArtSource/EngImport/CinderAnchor/`) | Blender 5.2 headless: one convex hull refitted on Grok Bot's Rodin tower | project-authored | Claude Code (Ryan's side, import builder) |

Held, not in the game: a voice audition for Wren (PR #78, not merged) made with **xAI text-to-speech through fal.ai** (AI-generated voice). If a take ships, it is credited here as AI-generated with the terms in its `SOURCE.md`.

### Tools used on Ryan's side, in one list (for the submission's "Tools you used")

AI tools, each marked where its output is AI-generated:
- **Claude Code** (Anthropic): engineering lead, teammates and builders; all gameplay code, tools, imports and docs.
- **Grok Bot** (xAI Grok, Ryan's asset-generation agent): runs the generators below and the Blender builds in `ArtSource/EngAI/**`.
- **Hyper3D Rodin** Gen-2.5 (image-to-3D, AI-generated meshes and textures): squad ships, Null fleet, hauler, drone, Warden, anchor tower, drill rig, crystal cradle, relay, wreck, outpost, energy blade mesh. List it for the Rodin prize pool.
- **fal.ai** hosted models (AI-generated): OpenAI GPT Image 2 / 2.6 Edit and Google Nano Banana 2 / 2.1 (concept images, cloud deck texture); Alibaba Wan 3.0 Prime text-to-video (effect and descent flipbooks); HY-Motion 1.0 / Hunyuan Motion (pilot blade animation); xAI text-to-speech (voice audition, held, not shipped).
- **PBRgen** (AI-generated PBR terrain sets; start textures from Nano Banana 2).

Other tools: Unreal Engine 5.9 (MegaJam Preview) with its built-in MCP server, Niagara, Interchange and Procedural Vegetation Editor; Blender 5.2 and 4.5.3 (headless Python); Python with NumPy, Pillow, OpenCV, matplotlib; ffmpeg.

Tools on Nicholas's side (OpenAI Codex, Suno, Meshy AI, OpenAI image generation and others) are in his entries above and are not restated here.

### Engineering originals not credited before (asset ledger pass, ART-02) - November 13, 2026

All original work made during the jam by Claude Code (Anthropic) sub-agents for Ryan T: code-built in Blender 5.2 (headless Python) or built in Unreal Engine 5.9 by script. No AI mesh, image or texture generator and no third-party model, texture or scan, except where the row says so.

| Asset | Tool | Licence / provenance | Added by |
|---|---|---|---|
| Walkable site kit: `SM_Kit_*` (16 meshes), `MI_Kit_*`, `T_Kit_*` in `Content/TriadDrift/Gameplay/Props/Kit/`, and the site classes built from it (`BP_Relay`, `BP_Relay_Awake`, `BP_Wreck`, `BP_Outpost_Site_Verdance` / `_Cinder`, `BP_DigSite_Cabin`); sources `ArtSource/EngProps/Kit/`. The walk-test classes `BP_KitD_InteriorPreview` and `BP_DockingBay_GalaxyTest` are **not shipped** (package ddc50a2f, Nov 14). | Blender 5.2 code-built hard surface with one trim sheet; classes assembled by Unreal Python. The relay, wreck and drill rig inside those classes are Grok Bot's Rodin meshes, credited above | project-authored | Claude Code (Ryan's side, structures builder) |
| Scatter kit: `SM_Scatter_*` (14 meshes), `M_TD_Scatter*`, `MI_Scatter_*`, `T_Scatter_Atlas_*` in `Content/TriadDrift/Gameplay/Props/Scatter/`; sources `ArtSource/EngProps/Scatter/`. `SM_Scatter_Verd_Canopy_01`, `_03` and `MI_Scatter_Canopy_NoFade` are **not shipped** (package ddc50a2f, Nov 14). | Blender 5.2 code-built | project-authored | Claude Code (Ryan's side) |
| Space dressing kit: 20 meshes (ice, metal and crystal asteroids, cargo pods, satellites, trusses, buoy, gate ring) in `Content/TriadDrift/Gameplay/Props/SpaceDressing/`; sources `ArtSource/EngProps/SpaceDressing/` | Blender 5.2 code-built, baked textures | project-authored | Claude Code (Ryan's side) |
| Nebula sky: `T_TD_NebulaSky` with its mesh and material in `Content/TriadDrift/Gameplay/Props/Sky/`; sources `ArtSource/EngProps/Sky/` | Blender world shader bake, procedural and seeded | project-authored | Claude Code (Ryan's side) |
| Energy blade in the game: `SM_TD_BladeHilt`, `SM_TD_BladeBeam`, `M_TD_BladeBeam`, `MI_TD_Blade*`, `T_TD_BladeHilt_*` in `Content/TriadDrift/Gameplay/Props/Blade/`; sources `ArtSource/EngProps/Blade/` | Blender 5.2 code-built | project-authored | Claude Code (Ryan's side) |
| Landing pad: `SM_LandingPad` with its material and textures in `Content/TriadDrift/Gameplay/Props/Planet/`; sources `ArtSource/EngProps/Pad/` | Blender 5.2 code-built | project-authored | Claude Code (Ryan's side) |
| Null Armada grunt and heavy: `Content/TriadDrift/Gameplay/Characters/{NullGrunt,NullHeavy}/`; sources with the trooper in `ArtSource/EngCharacters/` | Blender 5.2, armour modelled on Epic's UE5 mannequin body and skeleton, the same pipeline and design as the trooper (see the trooper's entry for its concept image) | armour project-authored; body and skeleton: Epic template content, Unreal Engine EULA | Claude Code (Ryan's side) |
| Planets from space: the Verdance and Cinder globes, atmosphere, ring, `M_TD_Planet`, `M_TD_PlanetAtmosphere`, `M_TD_Ground` and their textures in `Content/TriadDrift/Gameplay/Props/Planets/`; sources `ArtSource/EngProps/Planets/` | Blender (Cycles bake) and NumPy; material graphs by Unreal Python | project-authored | Claude Code (Ryan's side) |
| `M_TD_TerrainR2` in `Content/TriadDrift/Gameplay/Props/Planets/` | Material graph by Unreal Python (`ArtSource/EngEnvironment/scripts`). Its `T_Terr_*` textures are Grok Bot's PBRgen sets (AI-generated, credited above) | graph project-authored; textures: PBRgen Pro plan | Claude Code (Ryan's side, environment builder) |
| `M_TD_CloudDeck`, `MI_CloudDeck_*` in `Content/TriadDrift/Gameplay/Props/Sky/` | Material by Unreal Python. Its texture `T_Descent_CloudDeck` is Grok Bot's (AI-generated, fal, credited above) | material project-authored | Claude Code (Ryan's side, environment builder) |
| `M_DI_AtlasLit` in `Content/TriadDrift/Gameplay/Props/DominionInterior/Materials/` | Material by Unreal Python on the interior kit's own atlas | project-authored | Claude Code (Ryan's side, environment builder) |
| `M_TD_PortalMembrane`, `M_TD_FlashGlow`, `M_TD_TrailGlow`, `M_TD_SmokePuff` in `Content/TriadDrift/Gameplay/Materials/` | Built by script in Unreal | project-authored | Claude Code (Ryan's side) |

### Engineering fallback sounds, the gaps (ENG-35) - November 13, 2026

Fourteen more fallback sounds (27 takes) in `Content/TriadDrift/Gameplay/Audio/`, sources and recipes in `ArtSource/EngAudio/` (`recipes/p5_gaps.json`). Original work: synthesised from oscillators, noise and filters by the project's own script (`ArtSource/EngAudio/scripts/td_synth.py`, NumPy), levelled and measured with ffmpeg. No samples, no recordings, no library audio, no AI sound or voice generator. Fallbacks only: a sound of Nicholas's at the Presentation name replaces each one. Not approved by ear.

| Sound | Asset | Tool | Licence | Added by |
|---|---|---|---|---|
| Pickup collected: hull, shield, power, overcharge | `Pickups/SW_Pickup_Collect_{Hull,Shield,Power,Overcharge}_01` | original synthesis (NumPy), ffmpeg | project-authored | Claude Code (Ryan's side, import builder) |
| Reward drop | `Pickups/MSS_Pickup_Drop` (3 takes) | same | project-authored | same |
| Planet descent roar (loop) | `Travel/SW_Travel_DescentRoar_01` | same | project-authored | same |
| Three-pulse signal | `Story/SW_Story_ThreePulse_01` | same | project-authored | same |
| Dominion escape alarm (loop) | `Dominion/SW_Dominion_EscapeAlarm_01` | same | project-authored | same |
| Space-field breaks: small, medium, large, special | `Field/MSS_Field_Break_{Small,Medium,Large,Special}` (4, 3, 2, 2 takes) | same; MetaSound one-shots built with Unreal's MetaSound toolset | project-authored | same |
| Ship impact thuds: light and heavy | `Impacts/MSS_Ship_Impact_Thud`, `Impacts/MSS_Ship_Impact_Heavy` (3 takes each) | same | project-authored | same |
Nicholas Tosti supplied Cinder World (exploration) and Cinder Fall (combat). Full original tracks and measured stereo masters are preserved in ArtSource/Audio/CinderMusic01. OpenAI Codex coordinated preparation with Python/FFmpeg and Unreal Engine 5.9 native SoundWave import and independent reload. Native data verification is recorded in ArtSource/Audio/CinderMusic01/native-checks; gameplay timing, final mix and listening acceptance remain pending.

Gameplay demonstration capture (Shipping 6fe3e90f): OpenAI Codex, Unreal Engine 5.9, Windows GDI foreground-client capture and WASAPI process loopback, Python and FFmpeg. The video contains the game's existing assets and audio; no new music or visual asset was generated. Recording and limitations: docs/visual/Submission/LatestGameplayDemo01/README.md.

Submission trailer rough cut 01: OpenAI Codex used FFmpeg to select three excerpts from that same recorded game, with the corresponding game audio and brief edit-edge fades. No replacement music, generated scene or additional artwork was used. The 50-second draft and reproducible edit list are recorded in docs/visual/Submission/Trailer01; final trailer and human listening/appearance approval remain pending.


### Engineering originals added in batch M (asset ledger pass) - November 14, 2026

Original work made during the jam by Claude Code (Anthropic) sub-agents for Ryan T, in Blender 5.2 (headless Python) or in Unreal Engine 5.9 by script. No AI mesh, image or texture generator and no third-party asset, except the Hyper3D Rodin hulls they are built onto (credited above).

| Asset | Tool | Licence / provenance | Added by |
|---|---|---|---|
| Lancer hybrid: `SM_Lancer` (designed cannons and cockpit), `SM_Lancer_Canopy`, `MI_Lancer_Parts`, `MI_Lancer_Canopy`, `T_Lancer_Parts_BC/N/ORM/E/HeatMask` in `Content/TriadDrift/Gameplay/Ships/Lancer/` | Blender 5.2: parts modelled and baked on the Hyper3D Rodin Lancer hull (`ArtSource/EngShips`) | project-authored over the Rodin output (Hyper3D terms, paid Business plan) | Claude Code (Ryan's side, ship builder) |
| Bastion parts: `MI_Bastion_Parts`, `T_Bastion_Parts_BC/N/ORM/E/HeatMask` in `Content/TriadDrift/Gameplay/Ships/Bastion/` | Blender 5.2, parts modelled and baked on the Rodin Bastion hull | same | same |
| Ship canopies: `SM_Rift_Canopy`, `MI_Rift_Canopy` in `Ships/Rift/`, `SM_Lancer_Canopy` above, and the glass master `M_TD_ShipGlass` in `Ships/Materials/` | Blender 5.2 (canopy meshes); material built by script in Unreal | project-authored | same |
| Planet sea: `M_TD_PlanetSea`, `MI_PlanetSea_Verdance`, `MI_PlanetSea_Cinder` in `Content/TriadDrift/Gameplay/Props/Planets/` | Material built in Unreal by script (procedural: no textures from outside) | project-authored | Claude Code (Ryan's side, environment builder) |


### Engineering originals: space set pieces, story kit, Null ship parts (batches N and O) - November 14, 2026

Made during the jam by Claude Code (Anthropic) sub-agents for Ryan T. Read from the builders' own notes: `ArtSource/EngProps/Kit/brief_space.md` and the headers of `scripts/kit_pieces_space.py`, `kit_pieces_region.py`, `kit_pieces_story.py`, `kit_story_decals.py`, `ue_import_space.py`, `ue_import_story.py` (on `eng/struct` 12a8ec67); `ArtSource/EngShips/RiftHero/scripts/rh_ships.py` and `ArtSource/EngShips/Rodin/scripts/ue_import_rodin_ships.py` (on `lead/int` 2699c13c). "Code-built" means the mesh is written as Blender Python (route C) with no AI mesh, image or texture generator.

| Asset | Tool | Licence / provenance | Added by |
|---|---|---|---|
| Space set pieces: `SM_SP_Patrol_Bow / _Mid / _Aft / _Wing`, `SM_SP_Research_A / _B`, `SM_SP_NullRig`, `SM_SP_RigRock`, `SM_SP_HollowRock`, their `MI_SP_*` instances and the classes `BP_SP_PatrolWreck`, `BP_SP_ResearchA`, `BP_SP_ResearchB`, `BP_SP_NullRig`, `BP_SP_HollowRock` in `Content/TriadDrift/Gameplay/Props/Space/SetPieces/`; sources `ArtSource/EngProps/Kit/export_space/`, `sites/Space_*.manifest.json` | Blender 5.2 headless, code-built in the site kit's hard-surface style (`kit_pieces_space.py`, `build_kit.py --set space`); three LODs and the classes made by Unreal Python (`ue_import_space.py`). Materials reuse the kit trim sheets (`T_Kit_Trim_*`, `T_KitS_Trim_*`: procedural NumPy), the engineering asteroid and debris sets (`MI_Asteroid_03`, `MI_SpaceDebris_01 / _03`: Blender bakes) and the Dominion kit trim `MI_KitD_Trim` (see the last row) | project-authored; no AI generator in the meshes | Claude Code (Ryan's side, structures builder) |
| Region set pieces: `SM_KS_PicketRing`, `SM_KS_Hulk_Fore / _Mid / _Aft`, `SM_KS_ScarRig`, `SM_KS_MoonShard_A / _B` and their `MI_KS_*` instances in `Content/TriadDrift/Gameplay/Props/Kit/Space/`; sources `ArtSource/EngProps/Kit/export_region/`, `sites/KS_*.manifest.json` | Blender 5.2 headless, code-built (`kit_pieces_region.py`); imported by `ue_import_space.py` with `TD_SPACE_SET=region`. `MI_KS_Rock` puts the engineering `T_Asteroid_03_*` textures on the kit trim master; the rig uses `MI_KitD_Trim` | project-authored; no AI generator in the meshes | same |
| Story kit: `SM_Kit_ShelterFrame`, `_LampPostDead`, `_SampleCases`, `_AntennaToppled`, `_SurveyStakeFlag`, `_DragMark`, `_OreCart`, `_RailStub`, `_CableReel`, `_HeatShieldCracked`, `_WarningBeacon`, `_GantryCollapsed`, `_ScorchLines`, `_MarkerPole`, `_LightPool`, `_RelayMast`, `_NullDerrick`, with `MI_Kit_Canopy`, `MI_Kit_Flag`, `MI_Kit_DragMark`, `MI_Kit_ScorchLines`, `MI_Kit_LightPool` and `T_Kit_DragMark_*`, `T_Kit_ScorchLines_*`, `T_Kit_LightPool_*` in `Content/TriadDrift/Gameplay/Props/Kit/Story/`; sources `ArtSource/EngProps/Kit/export_story/` | Blender 5.2 headless, code-built (`kit_pieces_story.py`); decal textures painted procedurally with Pillow (`kit_story_decals.py`); imported by `ue_import_story.py`. The derrick uses `MI_KitD_Trim` | project-authored; no AI generator | same |
| Story sets and landmarks: `BP_StorySet_VerdanceCamp`, `BP_StorySet_CinderMine`, `BP_Landmark_RelayMast`, `BP_Landmark_NullDerrick` in `Content/TriadDrift/Gameplay/Props/Kit/Sites/`; `sites/StorySet_*.manifest.json`, `sites/Landmark_*.manifest.json` | Actor classes assembled by Unreal Python from the Story kit pieces above only (per the site manifests) | project-authored | same |
| Null fighter and gunship part sets: the second material slot of `SM_NullFighter` (three lathed engine nozzles) and of `SM_NullGunship` (four lathed turret barrels), `MI_NullFighter_Parts`, `MI_NullGunship_Parts`, `T_NullFighter_Parts_BC/N/ORM/E`, `T_NullGunship_Parts_BC/N/ORM/E` in `Content/TriadDrift/Gameplay/Ships/{NullFighter,NullGunship}/`; sources `ArtSource/EngShips/{NullFighterHero,NullGunshipHero}/` | Blender 5.2: parts modelled and baked by `rh_hybrid.py --ship NullFighter / NullGunship` on the **Hyper3D Rodin Gen-2.5** hulls (Grok Bot's `ArtSource/EngAI/Ships/NullFleet/{Fighter,Gunship}/mesh_r1`, credited above; the hull, its normals and its maps are kept as they were) | parts project-authored; the hull underneath is AI-generated (Hyper3D terms, paid Business plan, commercial use) | Claude Code (Ryan's side, second ship builder) |
| Shared material under the rigs and the derrick: `MI_KitD_Trim` (`T_KitD_Trim_*`) | The kit trim recomposed (`kit_atlas_from_grok.py`) from Grok Bot's `T_DI_InteriorTrim` bands (PR #82: Blender 4.5.3, matplotlib, NumPy, no AI generator; credited above) | project-authored | Claude Code (structures builder) over Grok Bot's sheet |


### Package check against these credits (ART-02) - November 14, 2026

The Shipping package of `ddc50a2f` (2,480 cooked packages in its container listing, `Builds/playable-ddc50a2f/listing.csv`) was compared with this file by the import builder (Claude Code, Ryan's side). Rows on Ryan's side for assets that are no longer cooked now say "not shipped" or "partly shipped" with "package ddc50a2f"; rows that already say "Source only" are unchanged. Nicholas's entries were not edited. One shipped AI-assisted item had no row of its own and is added here.

| Asset | Tool | Licence / provenance | Added by |
|---|---|---|---|
| Pilot locomotion clips `A_Pilot_Run`, `A_Pilot_Walk`, `A_Pilot_CasualWalk`, `A_Pilot_Idle02`, `A_Pilot_Idle3`, `A_Pilot_Idle6` and the edited `A_Pilot_Idle3_v2`, `A_Pilot_Walk_v2` in `Content/TriadDrift/Gameplay/Characters/Pilot/` | The six animations inside Nicholas's Sentinel Armor file (`Meshy_AI_Sentinel_Armor_..._All_animation.glb`: Running, Walking, Casual_Walk, Idle_02, Idle_3, Idle_6), made with **Meshy AI**; imported as they are (`ArtSource/EngAnim/Audit/inventory.md`, section 3). The `_v2` clips are wrist edits by the animation builder | As the Sentinel Armor entry above: supplied and authorised by Nicholas Tosti; Meshy plan terms not independently verified | Nicholas Tosti (source); import and edits by Claude Code (Ryan's side) |

Epic content in the package (Unreal Engine EULA, not original art): the First Person / Shooter template mannequin (`/Game/Characters/Mannequins/`: `SKM_Manny_Simple`, its textures, rigs and 102 animation clips, the sources of the retargeted `A_Crew_*` / `A_Pilot_*` clips), template input actions and animation blueprints (`/Game/Input`, `/Game/FirstPerson/Anims`, `/Game/Variant_Shooter/Anims`), and engine and plugin content (engine materials and fonts, Niagara, Interchange, Control Rig, Water, Procedural Vegetation Editor defaults). The template folders `LevelPrototyping`, `Variant_Horror`, `Weapons` and the template maps are not shipped.


### Shared "Don't-" voice master (N-097) - November 14, 2026

| Asset | Tool | Licence / provenance | Added by |
|---|---|---|---|
| `SW_VO_FutureWren_Dont_04` in `Content/TriadDrift/Gameplay/Audio/VO/` (one master for `O_Unknown_Dont` and `F_FutureWren_Dont` via `SharedSounds[Dont]`), imported from `ArtSource/EngAI/Voice/Wren/audition_r1/VO_FutureWren_Dont_04_radio.wav` (SHA256 `e5491da7…abc19a`), take 04 chosen by Nicholas | AI-generated voice: xAI Text to Speech (Grok voice) on fal.ai, endpoint `xai/tts/v1`, voice `liora`; radio treatment built by `tools/build_wren_dont_r1.py` (details in that folder's `SOURCE.md`) | fal.ai model page marked "Commercial use"; fal and xAI terms as summarised in `SOURCE.md` | Grok Bot (generation), Claude Code / Teammate 3 (import) |

Tools used (add to the submission's "Tools you used"): xAI Text to Speech via fal.ai.


### Engineering assets added in batches U to AG, not credited before (ART-02 pass by Teammate 6) - November 14, 2026

Found by listing every file added to `Content/` and `ArtSource/` on `main` since Nov 14 00:00 (`git log --diff-filter=A`) and checking each name against this file. Made during the jam by Claude Code (Anthropic) sub-agents for Ryan T unless a row says otherwise; read from each commit's message and the builders' notes.

| Asset | Tool | Licence / provenance | Added by |
|---|---|---|---|
| Cloud pass sounds: `SW_Travel_CloudPunch_01/02/03`, `MSS_Travel_CloudPunch`, `SW_Travel_GlideWind_01` in `Content/TriadDrift/Gameplay/Audio/Travel/`; recipe `ArtSource/EngAudio/recipes/p6_cloud.json`, renders `ArtSource/EngAudio/wav/Travel/` | Original synthesis (`td_synth.py`, no samples), levelled with `audio_report.py`; imported by script into Unreal 5.9 | project-authored (engineering fallback; Nicholas's sound at the slot replaces it) | Claude Code (effects builder) |
| Crew death and attack variants: `A_Crew_<Grunt/Heavy/Trooper>_DeathRM_*` (root-motion deaths from the HY-Motion clips credited above), in-place `A_Crew_*_Death_*`, `A_Crew_Grunt_Charge_Grok`, `A_Crew_Heavy_ShieldBash_Grok / _Authored / _Pre` in `Content/TriadDrift/Gameplay/Characters/Crew/Anims/`; review strips and scripts in `ArtSource/EngAnim/CrewGrok/` | Grok Bot's HY-Motion 1.0 clips (fal.ai; credited above) retargeted, retimed and cleaned in Blender 5.2; `_Authored` versions keyed by hand in Blender | HY-Motion output under fal's model terms (Ryan's paid fal.ai account); authored clips project-authored | Claude Code (animation lane) over Grok Bot's clips |
| Pilot interact: `A_Pilot_Interact`, `A_Pilot_Interact_Pre` in `Content/TriadDrift/Gameplay/Characters/Pilot/Anims/`; fixes and review strips in `ArtSource/EngAnim/PilotFixes/` | Retargeted and re-timed from the Unreal template mannequin set in Unreal 5.9 (IK rig and retargeter, by script) | Epic template content under the Unreal Engine EULA, re-timed | Claude Code (animation lane) |
| `SK_Warden_ShieldSpine` in `Content/TriadDrift/Gameplay/Characters/Warden/` | The earlier Warden skeletal mesh kept under a new name when the shield moved to the forearm | the Rodin Warden credited above | Claude Code (animation lane) |
| Walkable and story kit, second round: `SM_Kit_MagmaBridge`, `SM_Kit_AnchorCharge`, `SM_Kit_AnchorSection_A/B/C`, `SM_Kit_AnchorDebris_A/B/C`, `SM_Kit_InsigniaHull`, `SM_Kit_InsigniaPanel`, `SM_Kit_NullConduit`, `SM_Kit_NullConduitBroken`, `MI_Kit_InsigniaScorched` (`T_Kit_InsigniaScorched_BC/N/ORM/E`), `MI_Kit_CanopyStripe`, `MI_Kit_DerrickBeacon`, `MI_Kit_RelayBand`, `MI_Kit_RelayNav`, `MI_Kit_StoryWorn`, `T_Kit_DragMark_*`, `T_Kit_LightPool_*`, `T_Kit_ScorchLines_*`, `M_Kit_Flare`, `MI_KitD_Glow_Corridor`, `MI_KitD_Trim_Corridor`; site classes `BP_MagmaBridge`, `BP_AnchorCheck`, `BP_ConduitCheck`, `BP_SurveyStation_Verdance`, `BP_SurveyStation_Cinder`; import pipelines `PL_Space_StaticMesh`, `PL_Story_StaticMesh` | Blender 5.2 headless, code-built on the 50 cm kit grid (`ArtSource/EngProps/Kit/scripts/`, `export_story/`, `export_bridge/`, `sites/`, `textures/`); classes and pipelines assembled by Unreal Python | project-authored; no AI generator, no third-party asset | Claude Code (structures builder) |
| Space region dressing classes and materials: `BP_KS_PicketRing`, `BP_KS_Hulk`, `BP_KS_ScarRig`, `BP_KS_MoonShards`, `BP_KS_DomDebris` and the `MI_KS_*` light and surface instances (amber blink, cold run, crack, ember, flare, fracture rim, hazard, hulk, Null beacon, recess, rim light, seam, strata, white strobe) in `Content/TriadDrift/Gameplay/Props/Kit/Space/`; the `MI_SP_*` set-piece instances (breach, runs, strobe, nav, Null eye, hulls, stripe, rig beacon, rock cut, scorch, window, work light) in `Props/Space/SetPieces/` | Unreal Python instancing the kit masters; meshes credited above (batches N and O) | project-authored | Claude Code (structures builder) |
| Ship landing gear: `SM_Rift_Gear`, `SM_Lancer_Gear`, `SM_Bastion_Gear` and `MI_*_GearAccent` in `Content/TriadDrift/Gameplay/Ships/<Ship>/`; sources `ArtSource/EngProps/Kit/export_gear/` | Blender 5.2 headless, code-built | project-authored | Claude Code (structures builder) |
| Null bomber and interceptor part sets (second material slot: twin-boom tail; wing-tip lances and canted tail fins): `MI_NullBomber_Parts`, `T_NullBomber_Parts_BC/N/ORM/E`, `MI_NullInterceptor_Parts`, `T_NullInterceptor_Parts_BC/N/ORM/E`; `T_NullFighter_Parts_ORM`, `T_NullGunship_Parts_ORM`; sources `ArtSource/EngShips/Null*Hero/`, fleet running lights `ArtSource/EngShips/FleetLights/` (`rs_fleet_lights.py`) | Blender 5.2, parts modelled and baked onto the Hyper3D Rodin hulls (hulls untouched) | project-authored over the Rodin output (Hyper3D terms, paid Business plan) | Claude Code (ships builder) |
| Verdance far-tier tree clumps `SM_Scatter_Verd_TreeClump_01..04`, `M_TD_TreeClump`, `MI_TreeClump_Verdance`; `M_TD_ScatterColumn`, `MI_Scatter_Column_Cinder` in `Content/TriadDrift/Gameplay/Props/Scatter/` | Clumps built from the project's own Procedural Vegetation Editor trees (Unreal 5.9 PVE, Kapok and Brazil nut presets; crowns merged, trunks dropped); materials built by script in Unreal | project-authored | Claude Code (environment builder) |
| Effects, round 10 onward: `NS_TD_Plume_Enemy_Fighter / _Interceptor / _Gunship / _Bomber / _Carrier`, `NS_TD_Dominion_ApproachField / _ShieldDrop / _TractorBeam / _LightRow`, `NS_TD_Magma_Heat`, `NS_TD_Region_Beacon`, materials `M_TD_VFX_CloudVeil`, `M_TD_VFX_FieldHex / 2`, `M_TD_VFX_IceBurst`, `M_TD_VFX_IceShatter`, `M_TD_VFX_CloakOverlay`, `M_TD_VFX_RangeGlow` (+ `MI_*_Beacon / _Plume`), `M_TD_VFX_Beacon`, `M_TD_RegionHaze / 2`, `M_TD_PP_DescentWhite` in `Content/TriadDrift/Gameplay/VFX/`; specs and scripts in `ArtSource/EngVFX/`. The `NS_TD_Debug_*` systems and `MI_TD_RegionHaze_Review` are review aids, not referenced by the game | Niagara systems and materials built by Unreal Python in Unreal 5.9 (procedural; no textures from outside) | project-authored | Claude Code (effects builder) |
| Effects data: `DA_CrewBlaster_FX`, `DA_ReactorCore_FX`, `DA_StoryFX` in `Content/TriadDrift/Gameplay/Characters/FX/` | Data assets filled by script | project-authored data | Claude Code (Ryan's side) |
| Source only, not in the game: Wren voice audition r1 for `C8_Wren_Order` (`ArtSource/EngAI/Voice/Wren/present_order_r1/`, five dry takes; record in its `SOURCE.md`) | AI-generated voice: xAI Text to Speech on fal.ai (as the "Don't-" master above) | AI output under the fal.ai / xAI terms; awaiting Nicholas's pick | Grok Bot |

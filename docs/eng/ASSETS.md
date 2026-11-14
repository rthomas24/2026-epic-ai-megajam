# Asset ledger (Ryan's side: asset management, ART-02)

Since Nov 13 (afternoon) asset management is Ryan's side's: this file is the one list of **every asset in the game or
on its way in**, where it comes from, its licence and its state. Ryan's Lead keeps it; an asset builder fills in the
detail. Licences and tools are also in `CREDITS.md` (the submission needs them); replaced versions are in
`docs/eng/ART-HISTORY.md`; Grok Bot's queue is `MESSAGEBOARD.md` R-030; animations have their own ledger,
`animations.md`.

States: `requested` → `concept` → `source in` (raw model / texture in `ArtSource/`) → `in game` (imported, wired,
captured) → `reviewed` → `approved by eye` (Ryan or Nicholas).

## 1. Sources we draw on
| Source | What | Where | Licence / note |
|---|---|---|---|
| **Grok Bot** (fal, Hyper3D Rodin, PBRgen) | Hero ships, fleet, Warden, drone, energy blade, texture sets, effect flipbooks, concepts, animations | `ArtSource/EngAI/**`, branches `grok/assets-*` | Paid plans on Ryan's accounts; per-asset `SOURCE.md` |
| **Engineering builders** (Blender, code, Unreal Python) | Fallback ships and fleet, Dominion exterior and interior kit, planet kit, terrain and materials, sky, Niagara effects, sounds, retargeted animations | `ArtSource/Eng*/**`, `Content/TriadDrift/Gameplay/**` | Original work |
| **Nicholas's supplied models** (Meshy and others; N-041, N-042) | 14 imported (2 pilots incl. Sentinel Armor in the game, 12 others: ships, characters, a stone arch), 5 weapons, 36 further sources | Branches `nick/assets-meshy-intake02` @ `4587658`, `nick/assets-weapons-intake` @ `55d0010`, `nick/assets-meshy-intake03` @ `aa53ff9c` | Nicholas's own; **a library Ryan's side selects from**; Azure Shadow excluded by Nick's side. Review: `docs/eng/ASSET-INTAKE-REVIEW.md` |
| **Epic** | Template mannequin animations; Game Animation Sample Project (approved by Ryan); engine content | Project `Content/Characters/**`; Fab | Unreal Engine EULA; confirm and credit before use. No Game Animation Sample Project file is in the repository yet |

## 2. In the game today

**How this was taken (Nov 13, asset builder).** From the working tree of `lead/int` at `3174021`, which is ahead of
`origin/main` (`7c7e63a`) by: `Characters/Crew/Anims` (72 files), `Characters/Pilot/Anims` (44), `Characters/NullDrone`
(7), `Props/DominionInterior` (34), most of `VFX/` (43 new, 23 changed) and re-saved `DA_*` assets. Counts are files on
disk by name prefix. "Referenced by" comes from two scans: `/Game/...` path strings inside every `.uasset` / `.umap`
under `Content/` (who references whom) and the same strings in `Source/`, `Config/` and the HUD plugin. Sizes are
editor `.uasset` sizes, not cooked sizes. **"in game" below means a data asset or code references it; it does not
mean it was seen in a running game by this builder** (nothing was launched for this ledger).

How gameplay finds art (from `Source/`): a `DA_<Name>_Visuals` / `DA_<Name>_FX` in `/Game/TriadDrift/Presentation/`
wins; otherwise the same name anywhere under `/Game/TriadDrift/Gameplay/Characters/` (`TDStatics.cpp:498-534`,
`TDCrewCharacter.cpp:364-387`). `Content/TriadDrift/Presentation/` does not exist on `lead/int`, so every look in the
game today is the engineering copy.

### 2.1 Ships, fleet and the Dominion
| Set | What it is, where it shows | Source | Licence as recorded | Files (source → Unreal) | Counts | State and what references it | Next |
|---|---|---|---|---|---|---|---|
| Squad ships r7: Rift, Lancer, Bastion | The three player ships, all of space play | Engineering, Blender 5.2 headless, baked textures | Original (`CREDITS.md`, "Fallback player ships"; `ArtSource/EngShips/Squad/README.md`) | `ArtSource/EngShips/Squad/` → `Gameplay/Characters/Squad/` | 3 SM (16,388 / 16,768 / 27,838 tris per the README), 3 MI, 1 M, 12 textures (2K), 3 DA | **in game**: `DA_Rift/Lancer/Bastion_Visuals` (found by name), which also reference `NS_TD_Engine_*`, `NS_TD_Explosion_Medium`, hit effects | Replaced by the Rodin ships below |
| Squad ships, Rodin `mesh_r1` | Same three ships from the approved r2 concepts | Grok Bot, Hyper3D Rodin Gen-2.5 from fal.ai concepts | Hyper3D terms, Business plan, commercial use (`mesh_r1/SOURCE.md` on the branch); concepts: fal "Commercial use" (`concept_r2/SOURCE.md`) | Branch `origin/grok/assets-ships-r1` (`fc40996`), `ArtSource/EngAI/Ships/{Rift,Lancer,Bastion}/mesh_r1/`; concepts on `lead/int` in `concept_r1`, `concept_r2` | Raw 50,000 tris each; Rift derived 42,762 tris; 2K sets | **source in** (branch not on `main` or `lead/int`); concepts **approved** | Merge the branch; clean-up, sockets, import, new `DA_*_Visuals` (T1); credit line |
| Null Armada fleet (engineering) | Fighter, interceptor, gunship, bomber, carrier + bay, turret: every wave | Engineering, Blender 5.2 headless | Original (`CREDITS.md`, "Null Armada enemy ships") | `ArtSource/EngShips/NullFleet/` → `Gameplay/Characters/NullFleet/` | 7 SM, 7 MI, 1 M, 28 textures (1K-4K), 5 DA | **in game**: `DA_Enemy_{Fighter,Interceptor,Gunship,Bomber,Carrier}_Visuals`; fighter and interceptor meshes also by path in `TDIntroDirector.cpp:149`; turret mesh via `DA_DominionTurret_Visuals` | Replaced by the Rodin fleet |
| Null Armada fleet, Rodin | Same roles from the approved concepts | Grok Bot, Hyper3D Rodin Gen-2.5 | Hyper3D Business plan (`mesh_r1/SOURCE.md` on the branches) | `origin/grok/assets-fleet-r1` (`01e64e8`, fighter 8,926 tris per R-030), `origin/grok/assets-fleet-r2` (`a4bc2c8`, interceptor, gunship 19,800 tris); concepts for all six on `lead/int`, `ArtSource/EngAI/Ships/NullFleet/*/concept_r1` | 3 of 6 meshes delivered | **source in** (branches); bomber, carrier, turret **concept** (approved) | Merge, import, swap the `DA_Enemy_*` meshes (T4); a space drone for the opening is **requested** (storyline §11) |
| Dominion exterior | Mothership hull, bridge, trench wall, 4 generators + cores, shield emitter, reactor: the assault | Engineering, Blender 5.2 headless, procedural tileable textures | Original (`CREDITS.md`, "Dominion mothership exterior") | `ArtSource/EngShips/Dominion/` → `Gameplay/Props/Dominion/`, `Gameplay/Characters/Dominion/` | 7 SM, 8 MI, 2 M, 2 textures, 2 DA | **in game**: `DA_Dominion_Visuals`, `DA_DominionTurret_Visuals` | Hangars that open and a light row that pulses (storyline §11, §12); Rodin generator and reactor **requested** (features.md queue item 7) |

### 2.2 Characters and animation
| Set | What it is, where it shows | Source | Licence as recorded | Files (source → Unreal) | Counts | State and what references it | Next |
|---|---|---|---|---|---|---|---|
| Pilot: Sentinel Armor | The on-foot player body (planets, interior) | Nicholas, **Meshy AI** (auto-rigged sample); imported by engineering | Nicholas confirms it as his original custom-owned model (board, N-021 follow-up; `CREDITS.md`). **Meshy plan and licence terms: NOT RECORDED** ("have not been independently verified", `CREDITS.md`; "not cleared to ship" until confirmed, `ArtSource/EngCharacters/Pilot/SOURCE.md`) | `ArtSource/Characters/Samples/SentinelArmor/*.glb` (25.6 MB) → `Gameplay/Characters/Pilot/` | 1 SK (33,938 tris, 74 bones), SKEL, PA, M, MI, 3 textures (2K), 1 DA, 6 clips from the sample | **in game**: `DA_Pilot_Visuals` (`TDFirstPersonCharacter.cpp:190`) | Get the plan and licence line from Nicholas before submission; originality note in Gaps |
| Pilot animations | Locomotion, blade combo, block, dash, hit reacts, deaths | Epic template clips retargeted with the IK Retargeter and re-timed (engineering) | Unreal Engine EULA (`ArtSource/EngAnim/Retarget/SOURCE.md`, `CREDITS.md`) | `ArtSource/EngAnim/`, `ArtSource/EngCharacters/Pilot/anims/` → `Gameplay/Characters/Pilot/Anims/` | 61 `A_Pilot_*`, 2 IK rigs, 1 retargeter | **in game**: loaded by name, `Characters/Pilot/Anims/<Name>` (`TDFirstPersonCharacter.cpp:607`, names at 281, 692-715, 1124). Which of the 61 are asked for was not traced one by one | Grok Bot's blade set below replaces the stand-ins |
| Pilot blade combat clips (HY-Motion) | Light 1-3, heavy, block, air slam and more | Grok Bot, fal.ai `hunyuan-motion` | fal "Commercial use" (`SOURCE.md` on the branch) | `origin/grok/assets-anim-pilotblade-r1` (`04d1510`), `ArtSource/EngAI/Animations/PilotBlade/r1/` (68 files) | FBX clips on a 52-bone SMPL-H skeleton | **source in** (branch) | Retarget to `SKEL_Pilot`, cut, capture (`animations.md`) |
| Crew rig and animations | Shared skeleton and clips for trooper, grunt, heavy | Engineering rig; Epic template clips retargeted | Unreal Engine EULA for the clips; rig original | `ArtSource/EngCharacters/Crew/`, `ArtSource/EngAnim/` → `Gameplay/Characters/Crew/` | `SKEL_NullCrew`, `PA_NullCrew`, `IK_NullCrew`, `RTG_Manny_to_NullCrew`; `Anims/`: 72 `A_Crew_*`, 14 `MM_*` / `MF_*` copies, 1 IK, 1 RTG | **in game**: 14 `MM_*` / `MF_*` by exact path (`TDCrewCharacter.cpp:53-59, 159-166`), `A_Crew_*` by name (`:41`) | Per-type sets; prune clips nothing asks for |
| Null Trooper | Ranged crew enemy | Engineering, Blender on Epic's mannequin body and skeleton, from Ryan's concept image | Armour original; body from Epic template (`source/SOURCE.md`, EULA). **Tool that made the concept `refs/enemy1.png`: NOT RECORDED** (`CREDITS.md`: "to be stated by Ryan") | `ArtSource/EngCharacters/NullTrooper/` → `Gameplay/Characters/NullTrooper/` | 1 SK (10.3 MB), M, MI, 4 textures (2K), 1 DA | **in game**: `DA_Crew_Trooper_Visuals` | A rifle in the hands (see the intake review, pick 1); 9 `.blend` versions and `work/` are untracked in git |
| Null Grunt, Null Heavy | Melee and shielded crew enemies | Engineering (same pipeline as the trooper) | **NOT RECORDED** in `CREDITS.md` (no entry for either; `ART-HISTORY.md` lists them with the trooper) | Sources under `ArtSource/EngCharacters/NullTrooper/` and `Crew/export/` (3 FBX) → `Gameplay/Characters/{NullGrunt,NullHeavy}/` | each 1 SK, MI, 4 textures, 1 DA | **in game**: `DA_Crew_Grunt_Visuals`, `DA_Crew_Heavy_Visuals` | Add the credit line |
| Null Drone | Flying on-foot enemy | Grok Bot, Hyper3D Rodin Gen-2.5 from our fal.ai concept | Hyper3D terms, Business plan, commercial use (`ArtSource/EngAI/Characters/Drone/mesh_r1/SOURCE.md`; `CREDITS.md`) | `ArtSource/EngAI/Characters/Drone/mesh_r1/` → `Gameplay/Characters/NullDrone/` | 1 SM (6,188 tris), MI, 4 textures (2K), 1 DA | **in game**: `DA_Crew_Drone_Visuals` (on `lead/int`, not yet on `main`) | Hitbox and kill check (T5) |
| Warden | Cinder / interior mini-boss | Grok Bot, Rodin Gen-2.5 + Rodin Human auto-rig, from the r2 concept | Hyper3D Business plan (`mesh_r1/SOURCE.md` on the branch); concept fal "Commercial use" | `origin/grok/assets-warden-mesh-r1` (`d0310e8`), `origin/grok/assets-warden-props-r1` (`811836c`: shield 7,106 tris, gun arm 8,188 tris per R-030); concepts on `lead/int` | 50,000-tri body, 52 bones, 7 stock clips | **source in** (branches); concept **approved** | Merge, decimate, import, boss wiring (T5) |

### 2.3 Planets, space and the interior
| Set | What it is, where it shows | Source | Licence as recorded | Files (source → Unreal) | Counts | State and what references it | Next |
|---|---|---|---|---|---|---|---|
| Planet set-piece kit | Outpost walls, jammer tower, crates, Verdance wreck parts, relay platforms and altar, consoles, door, extraction core, route posts | Engineering, Blender 5.2 headless | Original (`CREDITS.md`, "Planet set-piece kit") | `ArtSource/EngProps/PlanetKit/` → `Gameplay/Props/Planet/` | 30 SM, 30 MI, 2 M, 119 textures (1K and 2K), 169.7 MB of textures | **in game**: by name, `Props/Planet/<Name>` (`TDGroundActors.cpp:64`); every mesh is always cooked | Hero versions from Grok Bot **requested** (crash wreck, relay altar, outpost gate, jammer tower) |
| Landing pad | Where the ship sets down on both planets | Engineering, Blender | Original per `ArtSource/EngProps/Pad/README.md`; **no `CREDITS.md` entry** | `ArtSource/EngProps/Pad/` → `Gameplay/Props/Planet/SM_LandingPad` | 1 SM (8,024 tris), MI, 4 textures (2K) | **in game**: by name through the planet kit lookup | Credit line |
| Cinder magma surface | Lava field | Engineering, Blender + Unreal script | Original (`CREDITS.md`) | `ArtSource/EngProps/PlanetKit/SM_Cind_MagmaSurface/` → `Props/Planet/` | 1 SM, MI, `M_TD_Magma`; `T_Cind_MagmaSurface_N` is 9.9 MB | **in game** by name | - |
| Verdance props | Mushrooms, ferns, glow plants, rocks | Engineering, Blender | Original (`CREDITS.md`, R-015) | `ArtSource/EngProps/Verdance/` → `Props/Verdance/` | 10 SM, 10 MI, 12 textures | **in game**: 10 exact paths in `TDPlanetCourse.cpp:62-71` | - |
| Cinder props | Crystals, basalt, columns | Engineering, Blender | Original (`CREDITS.md`, R-015) | `ArtSource/EngProps/Cinder/` → `Props/Cinder/` | 9 SM, 9 MI, 11 textures | **in game**: `TDPlanetCourse.cpp:79-87`, `TDPlanetProps.cpp:581` | The stabilising crystals as a story prop |
| Verdance trees | Six species, three growth stages | Engineering, Unreal 5.9 Procedural Vegetation Editor | Original; botanical figures only read from sources (`CREDITS.md`) | `Gameplay/Vegetation/Verdance/<Species>/` (source graphs and meshes) and a second copy of the meshes in `Gameplay/Props/Trees/` | Vegetation: 24 SM, 18 PVE, 6 PVP, 12 MI, 2 M, 4 leaf SM, 1 demo map. Props/Trees: 18 SM, 19.4 MB | **in game**: 7 of the 18 `Props/Trees` meshes by path (`TDPlanetCourse.cpp:72-78`, `863-870`); their materials are in `Vegetation/**` | Remove the 11 unused copies from the cooked folder (Gaps) |
| Planets from space, ground and terrain materials | Verdance and Cinder spheres, atmosphere, ring; `M_TD_Ground`, `M_TD_Terrain` | Engineering, Blender (Cycles bake) + numpy; material graphs by Unreal Python | Original (`ArtSource/EngProps/Planets/README.md`; `CREDITS.md` covers the terrain materials) | `ArtSource/EngProps/Planets/` → `Gameplay/Props/Planets/` | 3 SM, 9 MI, 5 M, 24 textures (up to 4096 x 2048), 123.5 MB | **in game**: `TDGalaxyLayout.cpp`, `TDPlaceholderSky.cpp:261`, `TDPlanetCourse.cpp:94` | Wire the PBR sets below into `M_TD_Terrain` |
| Terrain PBR sets | Nine 2K seamless sets (4 Verdance, 5 Cinder) | Grok Bot, PBRgen (start texture Nano Banana 2) | PBRgen Pro plan, commercial use (`CREDITS.md`; each folder's `SOURCE.md`) | `ArtSource/EngAI/Terrain/*/` | 36 final textures (2K) | **source in**: no Unreal asset yet | Import and wire; `Cinder_Basalt` r2 **requested** |
| Descent textures | Cloud deck, wisp and punch flipbooks, plasma sheath, heat streak, shock cone | Grok Bot, fal.ai (Wan 3.0 Prime clips) + procedural | fal: content cleared for commercial projects (folder `SOURCE.md` on the branch) | `origin/grok/assets-descent-r1` (`74efc7a`), `ArtSource/EngAI/Descent/` | 6 textures | **source in** (branch) | ENG-38 descent effects |
| Nebula sky | The space sky, 8192 x 4096 | Engineering, Blender world shader bake | Original, procedural and seeded (`ArtSource/EngProps/Sky/README.md`); `CREDITS.md` has no entry for it | `ArtSource/EngProps/Sky/` → `Gameplay/Props/Sky/` | 1 SM, M, MI, 1 texture of **97.0 MB** | **in game**: `TDSkySphere.cpp` | Lighting and grade pass; distortion field; size (Gaps) |
| Space rocks | 4 asteroids, 3 debris pieces | Engineering, Blender | Original (`CREDITS.md`, R-015) | `ArtSource/EngProps/SpaceRocks/` → `Props/Space/` | 7 SM, 7 MI, 23 textures | **in game**: 7 exact code paths | - |
| Space dressing kit | Ice, metal and crystal asteroids, cargo pods, satellites, trusses, buoy, gate ring | Engineering, Blender | Original per `ArtSource/EngProps/SpaceDressing/README.md`; **no `CREDITS.md` entry** | `ArtSource/EngProps/SpaceDressing/` → `Props/SpaceDressing/` | 20 SM (Nanite), 20 MI, 1 M, 75 textures, 140.4 MB of textures | **in game**: 20 exact code paths | Credit line |
| Energy blade | Hilt and beam in the pilot's hand | Engineering, Blender | Original per `ArtSource/EngProps/Blade/README.md`; **no `CREDITS.md` entry** | `ArtSource/EngProps/Blade/` → `Props/Blade/` | 2 SM (hilt 7,392 tris), 2 MI, 1 M, 4 textures | **in game**: `TDFirstPersonCharacter.cpp:317-318` | Grok Bot concept r1 is on `origin/grok/assets-blade-concept-r1` (`2e7870a`, **concept**); Rodin model after approval |
| Dominion interior kit | 22 corridor, room, core, door and debris pieces for the boarding finale | Engineering, Blender (bmesh + numpy atlas) | Original (`CREDITS.md`, "Dominion interior kit") | `ArtSource/EngProps/DominionInterior/` → `Props/DominionInterior/` | 22 SM, 7 MI, 1 M, 3 atlas textures (11.5 MB) | **source in, imported, not in game**: no code, data asset or map references any `SM_DI_*`; `L_Dominion_Interior` does not exist on `lead/int` and is not in the cook list | First room (T5); add the map to `MapsToCook` when it exists |

### 2.4 Effects, sound, materials, UI
| Set | What it is, where it shows | Source | Licence as recorded | Files (source → Unreal) | Counts | State and what references it | Next |
|---|---|---|---|---|---|---|---|
| Niagara effects tier 1 | Tracers, muzzle flashes, engines, impacts, explosions | Engineering, Unreal Niagara toolset from spec scripts | Original (`CREDITS.md`, ENG-37; `ArtSource/EngVFX/README.md`) | `ArtSource/EngVFX/` → `Gameplay/VFX/{Systems,Materials,Meshes,Textures}` | 25 `NS_TD_*`, 25 M, 6 SM, `T_TD_VFX_Noise` | **in game**: 19 systems referenced by the `DA_*_FX` and `DA_*_Visuals` assets. **Not referenced by anything:** `NS_TD_EngineBoost_{Rift,Lancer,Bastion,Enemy}`, `NS_TD_Explosion_FinalBlast`, `NS_TD_Explosion_HullSection` | Wire or remove the six; round 5; tier 2 |
| Niagara probes | Test systems `NS_TD_Probe_P1..P6`, `Q1..Q4` | Engineering | Original | `Gameplay/VFX/Probe/` | 10 NS, 6.7 MB | **not in game**: nothing references them, yet the folder is always cooked | Move out of the cooked folder or delete after review |
| Effect flipbooks | Explosion, smoke, fire, engine exhaust, electric arc, shield hex ripple, shockwave ring, debris, plasma bolt | Grok Bot, fal.ai: Alibaba Wan 3.0 Prime text-to-video, packed locally; some procedural | fal: cleared for commercial projects (`ArtSource/EngAI/VFX/*/r1/SOURCE.md`). **No `CREDITS.md` entry** for the flipbooks or for Wan | `ArtSource/EngAI/VFX/` → `Gameplay/VFX/Textures/T_TD_VFX_Grok_*` | 9 textures, 38 MB | **in game**: 7 referenced by `M_TD_VFX_Flip*`, `M_TD_VFX_Flame`, `M_TD_VFX_Bolt`. `T_TD_VFX_Grok_ElectricArc_8x8` and `T_TD_VFX_Grok_Fire_8x8` (10.9 MB) are referenced by nothing | Credit line; use or drop the two |
| Weapon, ability, travel, blade, ground and UI data assets | The cue tables gameplay reads | Engineering | Original | `Gameplay/Characters/FX/` | 20 DA | **in game**: found by name. `DA_UI_Style` references `SW_Music_Combat_01`, `SW_Music_Intro_01` and Nicholas's `MUS_Menu_EtherialPulse_01` | Nicholas's versions at the Presentation name replace them |
| Fallback sound set | Every sound effect, 3 ambiences, combat and intro music | Engineering, synthesised from recipes (numpy), levelled with ffmpeg | Original, no samples (`CREDITS.md`, ENG-35) | `ArtSource/EngAudio/` (200 WAV, recipes) → `Gameplay/Audio/` | 200 SW, 50 MSS, 10 SCON, 5 ATT, 5 SC | **in game**: every asset is referenced by another asset; default slots by path (`TDGroundAudio.cpp:12`). Not approved by ear | Ear pass by Nicholas; his music and replacements |
| Menu theme "Etherial Pulse" | Main-menu music | Nicholas, **Suno** (AI music); cut with ffmpeg | Nicholas confirmed authorship and an active Pro / Premier plan at generation (`CREDITS.md`). That entry still says "not game-integrated", which is out of date | `ArtSource/Audio/PrototypeV1/MenuTheme01/` (manifest; media not in git) → `Content/TriadDrift/Audio/MenuTheme01/` | `MUS_Menu_EtherialPulse_01` (13.1 MB), `SC_MenuTheme_Music` | **in game**: referenced by `DA_UI_Style`; sound class by path in `TDSettingsSubsystem.cpp` | Update the credit; list Suno under tools |
| Shared materials | Placeholder glow, flash, trail, smoke puff, portal membrane, textured prop master | Engineering, by script | Original (`CREDITS.md` names `M_TD_PlaceholderGlow` and `M_TD_PropTextured` only) | `Gameplay/Materials/` | 6 M | **in game**: 5 by code path, `M_TD_PropTextured` as the parent of the prop instances | - |
| Triad HUD | Nicholas's space HUD widget and ship card | Nicholas, Codex UI agent; original vector icons | Project-authored (`CREDITS.md`, "Triad HUD") | `ArtSource/UI/TriadHUD/` → `Content/TriadDrift/UI/TriadHUD/`, `Plugins/TriadDriftHUD/` (C++, 7 source files) | 2 WBP, 1 plugin | **in game** as a selectable HUD: `WBP_SpaceHUD` by path in `TDUIHostSettings.h`; folder always cooked | Menus and Director's Cut cards (Nicholas) |
| Engineering flight HUD and fallback menus | The default HUD (ENG-36) | Engineering, C++ / Slate | Original | `Source/` only | no content assets | **in game** | Adopt Nicholas's fonts and colours |
| SpaceFoundation sky | Nicholas's procedural sky: 3 skybox Blueprints, material, 2 textures | Nicholas, Codex Environment agent | Project-authored (`CREDITS.md`, "Native SpaceFoundation milestone") | `ArtSource/Environment/SpaceFoundation/` → `Content/TriadDrift/Environment/SpaceFoundation/` | 3 BP, 3 MI, 1 M, 2 T, 3 review maps | **imported, not in game**: referenced only by its own three review maps, which are not in the cook list | Environment is Ryan's now: keep as a reference or retire |
| Lancer M02 source (Nicholas) | Hand-built Lancer hull and weapons | Nicholas, Codex + Blender 5.1.1 | Original; helper script MIT (`CREDITS.md`) | `ArtSource/Space/Ships/Lancer/` (324 files, 158.9 MB) | 8 `.blend` | **source in**, never exported to Unreal | Superseded by the Rodin Lancer; keep (art history) |
| VFX starter kit, audio starter plan (Nicholas) | Four HLSL effects; cue manifest | Nicholas, Codex | Project-authored (`CREDITS.md`) | `ArtSource/VFX/StarterKit/`, `ArtSource/Audio/StarterPlan/` | text sources only | **source in**, no Unreal assets | Reference only |

### 2.5 Epic template content still in the project
| Set | Files | Size | State |
|---|---|---|---|
| Mannequins (meshes, rigs, textures, animations) | `Content/Characters/` (128 files) | 131.4 MB | `/Game/Characters/Mannequins/Anims` is always cooked; engineering assets reference `SKM_Manny_Simple` and `SK_Mannequin`; 3 clips by path in code. Unreal Engine EULA |
| Shooter variant (Blueprints, animations, UI) | `Content/Variant_Shooter/` (37) | 3.9 MB | `/Game/Variant_Shooter/Anims` is always cooked |
| Template weapons | `Content/Weapons/` (27) | 14.6 MB | Not referenced by any Triad Drift asset or code path |
| First Person, Level Prototyping, Horror variant, Input | 60 files | 4.1 MB | Template reference; their maps are not in the cook list |

### 2.6 Requested, not started (from features.md "Asset generation today" and storyline.md)
Bomber, carrier and turret meshes; a space drone for the opening; Dominion shield generator and reactor; hangars that
open; planet hero set pieces (crash wreck, relay altar, outpost gate, jammer tower); the signal-analysis device; the
stabilising crystals as a pick-up; one ancient artefact prop repeated on Verdance, Cinder and in the core chamber;
damaged-armour variants showing Triad's insignia shape; a rifle for the trooper; key art, itch banner, menu still and
trailer end card.

### 2.7 Gaps and risks
**No recorded source or licence**
1. **The pilot in the game is a Meshy model whose plan and licence terms are not recorded.** Nicholas affirms it is
   his original design and model; `ArtSource/EngCharacters/Pilot/SOURCE.md` still says it is not cleared to ship until
   the Meshy plan is confirmed, and notes that its white-and-grey armour silhouette is close to well-known sci-fi
   armour, for Nicholas to judge against "original designs only".
2. **In the game with no `CREDITS.md` entry** (each has a README saying it is original, so this is paperwork, but the
   submission reads `CREDITS.md`): Grok Bot's effect flipbooks and the Wan 3.0 Prime model behind them, the space
   dressing kit, the nebula sky, the energy blade, the landing pad, the grunt and heavy, the planets-from-space kit,
   `M_TD_PortalMembrane`, `M_TD_FlashGlow`, `M_TD_TrailGlow`, `M_TD_SmokePuff`.
3. The tool that generated the trooper concept `refs/enemy1.png` is still "to be stated by Ryan".
4. `CREDITS.md` says the menu theme is not integrated; it is referenced by `DA_UI_Style` and will ship. Suno is not in
   "Tools used" at the top of the file.
5. Not yet credited because not yet merged: Rodin ships, fleet, Warden, the HY-Motion clips, the descent textures.

**Referenced only by a path string**
6. Everything gameplay loads by string lives in an always-cooked folder (`Gameplay/{Materials,Props,Characters,Audio,VFX}`,
   `UI/TriadHUD`), so it cooks. Exceptions to watch: `/Game/TriadDrift/Maps/L_Dominion_Interior` (in
   `TDFlowSettings.cpp:13`; the map does not exist and is not in `MapsToCook`, so the boarding finale cannot ship
   until both are fixed); `/Game/TriadDrift/Audio/Mix/SC_TD_Music` and `SC_TD_SFX` (`TDSettingsSubsystem.cpp:29-30`;
   neither asset exists, so those volume sliders act only on the engineering classes); `Gameplay/Vegetation/**` is
   **not** always cooked and ships only because the `Props/Trees` meshes reference its materials.
7. `Content/TriadDrift/Presentation/` does not exist, although eight primary-asset rules and the code look there
   first. Nothing breaks; it means no asset of Nicholas's overrides an engineering look today.

**Package size (limit 1 GB zipped)**
8. The last recorded Shipping package was 942 MB, 797 MB zipped (`docs/eng/STATUS.md`, `fea764d`); an earlier one
   measured 1,026 MB with debug symbols (`DEVLOG.md`). `fea764d` is from Nov 13, 04:01; the Rodin drone and the
   interior kit were committed at 13:52 the same day, and three 50,000-tri ships, a fleet and the Warden are coming. **There is
   little headroom; measure after every art merge.**
9. The 20 largest files under `Content/` (editor size):

   | MB | File |
   |---|---|
   | 97.0 | `TriadDrift/Gameplay/Props/Sky/T_TD_NebulaSky` |
   | 21.0 | `Characters/Mannequins/Textures/Manny/T_Manny_02_BN` |
   | 19.4 | `TriadDrift/Gameplay/Props/Planets/T_Planet_Cinder_N` |
   | 17.1 | `Characters/Mannequins/Rigs/CR_Mannequin_Body` |
   | 16.3 | `Characters/Mannequins/Meshes/SKM_Quinn_Simple` |
   | 15.8 | `Characters/Mannequins/Meshes/SKM_Manny_Simple` |
   | 13.1 | `TriadDrift/Audio/MenuTheme01/MUS_Menu_EtherialPulse_01` |
   | 10.7 | `TriadDrift/Gameplay/Characters/NullHeavy/SK_NullHeavy` |
   | 10.3 | `TriadDrift/Gameplay/Characters/NullTrooper/SK_NullTrooper` |
   | 10.2 | `TriadDrift/Gameplay/Characters/Pilot/SK_Pilot` |
   | 10.1 | `TriadDrift/Gameplay/Props/Planets/T_Planet_Verdance_N` |
   | 10.0 | `TriadDrift/Gameplay/Characters/NullFleet/Textures/T_Null_Carrier_N` |
   | 9.9 | `TriadDrift/Gameplay/Props/Planet/Textures/T_Cind_MagmaSurface_N` |
   | 9.4 | `TriadDrift/Gameplay/Characters/NullFleet/Textures/T_Null_Carrier_ORM` |
   | 9.0 | `TriadDrift/Gameplay/Props/Planets/T_Ground_Cinder_N` |
   | 8.9 | `TriadDrift/Gameplay/Props/Planets/T_Planet_Cinder_BC` |
   | 8.7 | `TriadDrift/Gameplay/Props/Planets/T_Ground_Verdance_N` |
   | 7.8 | `TriadDrift/Gameplay/Characters/NullGrunt/SK_NullGrunt` |
   | 7.6 | `TriadDrift/Gameplay/Props/Planets/T_Planet_Verdance_BC` |
   | 7.3 | `TriadDrift/Gameplay/VFX/Textures/T_TD_VFX_Grok_Explosion_8x8` |

   By folder: `Props/Planet/Textures` 169.7 MB (115 textures for 30 small pieces), `Props/SpaceDressing/Textures`
   140.4 MB, `Props/Planets` 123.5 MB, `Props/Sky` 97.6 MB, `Characters/NullFleet/Textures` 59.0 MB. `Content/` is
   1,121 MB in all, `Content/TriadDrift/` 1,015 MB. The cheapest savings, as an inference from these sizes: cap the
   sky at 4096 x 2048 or a lower-bit format, and cap the planet-kit and dressing normal and ORM maps at 1K.

**Unused assets that still cook** (folders listed under `DirectoriesToAlwaysCook` in `Config/DefaultGame.ini`)
10. `Gameplay/VFX/Probe/` (10 systems, 6.7 MB); six unwired `NS_TD_*` systems (4.9 MB); two unused Grok flipbooks
    (10.9 MB); seven `M_TD_VFX_*` materials and `SM_TD_VFX_Shard_B` with no referencer; 11 of the 18 `Props/Trees`
    meshes (5.2 MB); the 22 interior meshes until a room uses them (2.0 MB + 11.5 MB of atlas); any `A_Crew_*` /
    `A_Pilot_*` clip that no code asks for (not traced; up to about 40 MB across the two `Anims/` folders).
11. `/Game/Characters/Mannequins/Anims` and `/Game/Variant_Shooter/Anims` are always cooked although the crew now has
    its own copies of the clips it plays; three template clips are still named in code. Inference: these two rules
    pull the mannequin meshes and their 21 MB normal map into the package. Check with a cook report before removing.

**Duplicates**
12. The 18 tree meshes exist twice, in `Gameplay/Vegetation/Verdance/<Species>/` and `Gameplay/Props/Trees/` (same
    names; 19.4 MB each side). Only the `Props/Trees` copies are loaded.
13. Six of the pilot's clips exist at the root of `Characters/Pilot/` (the sample's own) beside the 61 in `Anims/`.
14. Two generations of ground material (`MI_Ground_*` and `M_TD_Terrain`) are both referenced by code; both ship.
15. Nicholas's intake branches hold a second copy of Sentinel Armor under `Content/Pilot/SentinelArmor/`. It must not
    be merged beside `Gameplay/Characters/Pilot/`.

**Housekeeping**
16. Untracked in the `lead/int` working tree: nine `NullTrooper_v0N.blend` files, `NullTrooper/work/` and
    `Pilot/anims/pilot_parents.json`. The art-history rule says sources are committed before anything replaces them.
17. `Plugins/TriadDriftHUD/` has 65.6 MB of `Binaries/` and 12.6 MB of `Intermediate/` on disk; they are not tracked
    in git, which is correct.

### 2.8 Update: everything imported since the ledger was taken (Nov 13 14:19 to 22:50, import builder)

**How this was taken.** From `eng/import` at `a78930a4` (which holds `origin/main` up to PR #82 and `lead/int-k`):
every file added or changed under `Content/` since the ledger's commit `f5f9cea5`, by folder and name prefix, read
against each delivery's `SOURCE.md` / README and against `CREDITS.md`. Nothing was launched for this update; "seen in
a capture" is quoted from the builder's own report or sheet. Nicholas's folders (`Content/TriadDrift/{Audio,UI,
Presentation}`, `ArtSource/{UI,Audio}`, `Story/`) changed too in that time; they are his to record and are not
listed. Rows above that this replaces: the Rodin squad and fleet rows of 2.1 (now in the game), the Warden and blade
clips of 2.2, the terrain sets, descent textures and interior kit of 2.3, and 2.6 (most of it has arrived).

| Set | What it is, where it is used | Source and tool | Licence as recorded | Files (source → Unreal) | State | In `CREDITS.md` |
|---|---|---|---|---|---|---|
| Squad ships, Rodin: Rift (hybrid), Lancer, Bastion | The three player ships | Grok Bot, Hyper3D Rodin Gen-2.5 from fal concepts; Rift's ring, engines, tail and portal disc rebuilt in Blender 5.2 (ship builder); textures_r2 clean-up (OpenCV); heat masks and per-ship detail sets (Blender / NumPy) | Hyper3D terms, paid Business plan, commercial use; the rest project-authored | `ArtSource/EngAI/Ships/{Rift,Lancer,Bastion}/**`, `ArtSource/EngShips/{Rodin,RiftHero,Rift,Lancer,Bastion}` → `Gameplay/Ships/{Rift,Lancer,Bastion}` (4 SM, 7 MI, 26 T) | **in game** through `DA_{Rift,Lancer,Bastion}_Visuals` (Mesh, MaterialOverrides, Parts) | yes |
| Null fleet, Rodin: fighter, interceptor (mesh_r2), gunship, bomber, carrier, turret | Every wave; the turret also on the carrier and the Dominion | Grok Bot, Hyper3D Rodin Gen-2.5 | same plan | `ArtSource/EngAI/Ships/NullFleet/**` → `Gameplay/Ships/Null{Fighter,Interceptor,Gunship,Bomber,Carrier,Turret}` (6 SM, 6 MI, 24 T) | **in game** through `DA_Enemy_*_Visuals` and `DA_DominionTurret_Visuals` | yes |
| Ship masters | `M_TD_Ship`, `M_TD_RiftPortal`, `M_TD_ShipGlow`, 3 shared textures, pipeline `PL_Ship_StaticMesh` | Engineering, Unreal Python (`ue_import_rodin_ships.py`) | project-authored | `Gameplay/Ships/{Materials,Pipelines}` | **in game** (parents of every ship `MI_`) | yes |
| Null hauler (PR #68) | The Verdance convoys | Grok Bot, Hyper3D Rodin Gen-2.5 from Nicholas's four-view board; emissive authored in Blender | same plan | `ArtSource/EngAI/Ships/NullHauler/mesh_r1` → `Gameplay/Ships/NullHauler` (1 SM, 5 UCX, 1 MI, 4 T), `Characters/NullFleet/DA_Enemy_Hauler_Visuals` | **in game** (`TDVerdanceApproach.cpp`, `bf1f9749`); seen in a scripted capture | yes |
| Dominion hull r2 (PR #72; r1 was PR #70) | The mothership exterior | Grok Bot, Blender 4.5.3 procedural, NumPy textures; from the fal concept v3 (PR #65) | project-authored; concept: fal, cleared for commercial projects | `ArtSource/EngAI/Ships/DominionMothership/mesh_r2` → `Gameplay/Ships/DominionMothership` (1 SM, 121 UCX, 12 sockets, 1 MI, 6 T) | **imported, held back** (`65ffa0c3`): with it 4 `TriadDrift.World.Dominion` tests fail; `DA_Dominion_Visuals` points at the engineering r9 hull until T4 adapts. mesh_r1 was never imported | yes (r2); r1 and the concept added in this update |
| Dominion shield generator + core (PR #74) | The four generators of the assault | Grok Bot, Blender 4.5.3 procedural | project-authored | `ArtSource/EngAI/Ships/DominionShieldGenerator/mesh_r1` → `Gameplay/Ships/DominionShieldGenerator` (2 SM, 43 UCX, 2 MI, 8 T) | **in game** through `DA_Dominion_Visuals` (GeneratorMesh, GeneratorCoreMesh); seen in a scripted capture, alive and destroyed | yes |
| Warden (body, shield, gun arm) | Cinder and interior mini-boss | Grok Bot, Hyper3D Rodin Gen-2.5 + Rodin Human auto-rig; re-rigged in Blender 5.2 | Hyper3D Business plan | `ArtSource/EngAI/Characters/Warden/**`, `ArtSource/EngCharacters/Warden` → `Gameplay/Characters/Warden` (1 SK, SKEL, PA, 3 MI, 12 T, 1 DA) | **in game** through `DA_Crew_Warden_Visuals` | yes |
| Pilot blade clips | Light 1-3, heavy, dash, hit reacts and the rest of the set | Grok Bot, HY-Motion 1.0 on fal.ai; retargeted in Unreal (animation builder) | fal "Commercial use" | `ArtSource/EngAI/Animations/PilotBlade/r1` → `Gameplay/Characters/Pilot/Anims` (23 new `A_Pilot_*`); retarget sources in `Content/TriadDrift/Dev/EngAnim/PilotBlade` (18 files) | **in game** by name; state per clip in `animations.md`. `Dev/EngAnim` is not in a cooked folder | yes |
| Barks table | Gameplay dialogue lines | Engineering, from `dialogs.md` (`Tools/td-barks-import.py`) | project-authored text | `Gameplay/Characters/Barks` (1 DT) | **in game** | no (text only) |
| Walkable site kit | 16 modular pieces on a 50 cm grid, one trim sheet; the relay deck, wreck walkway, outposts and dig-site cabin are built from it | Engineering (structures builder), Blender 5.2 code-built | original (`ArtSource/EngProps/Kit/README.md`) | `ArtSource/EngProps/Kit` → `Gameplay/Props/Kit` (16 SM, 4 MI, 3 T, `Sites/BP_DigSite_Cabin`) and the site classes in `Props/{Relay,Wreck,Outpost}` | **in game**: `BP_Relay`, `BP_Relay_Awake`, `BP_Wreck`, `BP_Outpost_Site_*`; `BP_DigSite_Cabin` is placed by nothing yet | **no** |
| Survey outpost cabin and console, Ancient Relay, Verdance wreck (PRs #50, #64, #66) | Relay and wreck: dressing sunk under the kit floors. Outpost cabin and console: no longer used | Grok Bot, Hyper3D Rodin Gen-2.5 from cleaned fal concepts of Nicholas's boards; ramps, lights, collision in Blender | Hyper3D Business plan | `ArtSource/EngAI/Props/{VerdanceOutpost,AncientRelay,VerdanceWreck}`, `ArtSource/EngProps/Outpost` → `Gameplay/Props/{Outpost,Relay,Wreck}` (4 SM, 7 MI, 3 M, 18 T, 5 BP) | relay and wreck **in game** inside the site classes; outpost cabin / console **imported, unused**, in an always-cooked folder | yes |
| Cinder field anchor: tower + emitter (PR #73) | The anchor objective's landmark | Grok Bot, Hyper3D Rodin Gen-2.5 + Blender 4.5.3; plinth collision refitted by the import builder (`ArtSource/EngImport/CinderAnchor`) | Hyper3D Business plan | `ArtSource/EngAI/Props/CinderFieldAnchor/mesh_r1` → `Gameplay/Props/CinderAnchor` (2 SM, 5 UCX, 4 sockets, 2 MI, 8 T) | **imported**; in the game only with the course change `7a958ecf`, which waits for T5. The pilot walked a lap round the plinth (`a78930a4`) | yes |
| Cinder drill rig (PR #69) | Dressing beside the dig-site cabin | Grok Bot, Hyper3D Rodin Gen-2.5 + Blender clean-up | Hyper3D Business plan | `ArtSource/EngAI/Props/CinderDigSite/mesh_r1` → `Gameplay/Props/CinderDigSite` (1 SM, 1 hull, 1 MI, 4 T); a component of `BP_DigSite_Cabin` | **imported, not in game** (nothing spawns the cabin class). `SM_CinderSiteCabin` from the same PR is **not imported** (walkable: rebuilt from the kit) | yes (rig) |
| Crystal cache: cradle (PR #75) and hero cluster r2 (PR #80) | The EMP Pulse objective's last step | Cradle: Grok Bot, Hyper3D Rodin + Blender 4.5.3. Cluster: Blender procedural, no generator | Hyper3D Business plan (cradle); project-authored (cluster) | `ArtSource/EngAI/Props/CrystalCache/{mesh_r1,mesh_r2}` → `Gameplay/Props/CrystalCache` (2 SM, 16 UCX, 1 socket, 2 MI, 8 T) | **imported**; in the game only with `7a958ecf` (T5). Seen in a scripted capture on that commit | cluster yes; cradle added in this update |
| Outpost and landing-pad set dressing (PR #79) | Ten small props for the outposts and pads | Grok Bot, Blender 4.5.3 procedural, one shared 1K trim | project-authored | `ArtSource/EngAI/Props/OutpostSetDressing/mesh_r1` → `Gameplay/Props/OutpostDressing` (10 SM, 17 UCX, 1 MI, 4 T) | **imported, not placed** (the environment builder places them) | yes |
| Dominion docking bay plans, doors and tractor emitter (PR #77) | Measured plans and a blockout for the bay; three non-walkable meshes and a trim sheet | Grok Bot, Blender 4.5.3 + matplotlib, no generator | project-authored | `ArtSource/EngAI/Props/DominionDockingBay/plans_r1` | **source in**: `SM_DominionBay_Door_L/R`, `SM_DominionBay_TractorEmitter` and `T_DominionBay_Trim_*` are not imported | added in this update |
| Dominion interior plans (PR #82) | Measured plans, sections, a trim sheet and decal atlas for the walkable interior | Grok Bot, Blender 4.5.3 + matplotlib, no generator | project-authored | `ArtSource/EngAI/Props/DominionInterior/plans_r1` | **source in** (reference for the structures builder; nothing imported) | added in this update |
| Energy blade, Grok Bot (concept PR #26, mesh PR #47) | Folding sabre, deployed and folded | Concept: fal (Google Nano Banana 2.1). Mesh: Hyper3D Rodin Gen-2.5 | fal commercial; Hyper3D Business plan | `ArtSource/EngAI/Weapons/EnergyBlade/{concept_r1,mesh_r1}` | **source in**: not imported; the blade in the game is still engineering's `Props/Blade` | added in this update |
| Rift mesh_r2 (PR #67) | Grok Bot's rebuilt Rift parts and cleaned textures | Grok Bot, Blender 4.5.3 + OpenCV / NumPy, no generator | project-authored over the Rodin output | `ArtSource/EngAI/Ships/Rift/mesh_r2` | **source in**; whether the hybrid Rift in the game uses any of it was not traced | added in this update |
| Dominion interior look | `M_DI_AtlasLit` and retuned `MI_DI_*` | Engineering (environment builder), Unreal Python | project-authored | `Gameplay/Props/DominionInterior/Materials` | **in game** on `L_Dominion_Interior` (the map now exists) | **no** (the kit itself is credited) |
| Terrain R2 / R3 | `M_TD_TerrainR2` + 18 `T_Terr_*` (BC and N of the nine PBRgen sets) | Grok Bot's PBRgen sets; material graph by engineering (Unreal Python) | PBRgen Pro plan, commercial use | `ArtSource/EngAI/Terrain/**`, `ArtSource/EngEnvironment/scripts` → `Gameplay/Props/Planets` | **in game**: `MI_Terrain_{Verdance,Cinder}` | sets yes; the material **no** |
| Planet cloud deck | `M_TD_CloudDeck`, 2 MI, `T_Descent_CloudDeck` | Texture: Grok Bot's descent set (fal, Nano Banana 2, made tileable locally); material and `ATDCloudDeck` by engineering | fal: cleared for commercial projects | `ArtSource/EngAI/Descent/CloudDeck/r1` → `Gameplay/Props/Sky` | **in game** (`td.CloudDeck`) | texture added in this update; material **no** |
| Descent textures, the other five (PR #33) | Cloud wisp and punch flipbooks, plasma sheath, heat streak, shock cone | Grok Bot: fal (Alibaba Wan 3.0 Prime text-to-video) packed locally; heat streak and shock cone procedural | fal: cleared for commercial projects | `ArtSource/EngAI/Descent/**` | **source in**, unless one of the five new `VFX/Textures` is theirs (not traced by name) | added in this update |
| Scatter kit | 14 meshes: Verdance ground cover and canopy clumps, boulders, Cinder charred trees | Engineering, Blender 5.2 code-built | original (`ArtSource/EngProps/Scatter/README.md`) | `ArtSource/EngProps/Scatter` → `Gameplay/Props/Scatter` (14 SM, 6 MI, 3 M, 2 T) | **imported**; its README: seen on Verdance, not yet used by the scatter | **no** |
| Niagara effects, rounds 5 and 6 | 54 new `NS_TD_*` (tier 2, story, distortion field, portal, ability loops), 2 emitters, 16 M, 1 MI, 1 SM, 5 T | Engineering (effects builder), Unreal Niagara toolset; flipbooks from Grok Bot's fal / Wan set | project-authored; flipbooks: fal commercial | `ArtSource/EngVFX`, `ArtSource/EngAI/VFX` → `Gameplay/VFX` | **in game** where a `DA_*_FX` / `DA_*_Visuals` cue names one; which of the 54 are wired is recorded in `ArtSource/EngVFX`, not re-traced here | systems yes (ENG-37); flipbooks added in this update |

**Still requested, not delivered** (was 2.6): a space drone for the opening; the reactor; hangars that open; the
signal-analysis device; the ancient artefact prop; damaged-armour variants; a rifle for the trooper; key art, itch
banner, menu still, trailer end card. **Held, not merged:** PR #78, a voice audition for Wren made with xAI
text-to-speech through fal.ai (AI-generated voice); it ships only if Nicholas accepts a take.

**Findings (Nov 13 22:50): shipped or cooked with no `CREDITS.md` record.** Each has a README or `SOURCE.md` saying
what it is, so the provenance exists; the submission reads `CREDITS.md`.
1. Engineering originals with no credit line: the walkable site kit (`Props/Kit`, 16 meshes and the site classes), the
   scatter kit (`Props/Scatter`), `M_TD_TerrainR2`, `M_TD_CloudDeck`, `M_DI_AtlasLit`; and, still open from 2.7 item 2,
   the space dressing kit, the nebula sky, the engineering energy blade, the landing pad, the grunt and heavy, the
   planets-from-space kit and four shared materials.
2. Grok Bot deliveries that had no credit line until this update (appended to `CREDITS.md` in the commit after this
   one): the effect flipbooks and descent textures (fal: Alibaba Wan 3.0 Prime, Nano Banana 2), the crystal cache
   cradle (Rodin), the energy blade concept and mesh, the Dominion concept v3 and hull r1, the docking bay and interior
   plans, Rift mesh_r2.
3. Still unrecorded from 2.7: the Meshy plan and licence terms for the pilot in the game (item 1) and the tool
   behind the trooper concept `refs/enemy1.png` (item 3). Both need a human answer.
4. Cooked but unused (always-cooked `Gameplay/Props`): the Rodin outpost cabin and console (`Props/Outpost`), the
   drill rig and `BP_DigSite_Cabin` until something places the cabin, the ten set-dressing props until placed. The
   Dominion r2 hull (`Gameplay/Ships`, not an always-cooked folder) is not referenced while it is held back.
5. `Content/TriadDrift/Dev/EngAnim/PilotBlade` (18 retarget sources) is outside every cooked folder: correct, but
   nothing else should reference it.

## 3. Nicholas's supplied models: review for use
Done on Nov 13: **`docs/eng/ASSET-INTAKE-REVIEW.md`** has one row for each of the 14 imported assets, the 5 weapons
and the 36 sources, with measured triangle counts, the fit to the story, the proposed use, the cost and a decision,
then a ranked shortlist of eight with a builder brief each, and what the library does not cover.

In one paragraph: nothing in the library is game-ready as it stands (33 of the 44 measured models are between
195,000 and 3,080,000 triangles, the textures looked at have fragmented generator UVs, none is in the faction
palette, and no licence or plan is recorded for any of them). Eight are worth the clean-up; the first five are the Coilgun Assault Rifle as
the trooper's rifle (4,124 triangles, the only weapon already inside the budget), Crimson Vanguard and Nebula
Shadowfighter as ships in the lanes of machines streaming to the planets, Spaceship as the wreck of the missing
patrol or the Verdance research vessel, and the Circular Bulkhead Wall as the Dominion's hangar or core door.
Thirteen of the fourteen ZIP sources carry filename dates from March to October 2026, before the jam: they are held
until Nicholas states when they were made, and would have to be declared.

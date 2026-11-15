# Triad Drift

A missile is on your tail. Switch to Rift and send it through a portal. Cut to Lancer, line up the enemy that fired it, and ram straight through. When the next wave closes in, take Bastion and throw a shield over your wingmen. The two ships you leave behind keep fighting.

Fly between two worlds to give your squad new abilities. On Verdance, recover a signal-analysis device and protect the survey transfer in the glowing jungle. On Cinder, follow the survey records to an excavation, recover the EMP prototype and disable the field anchor. Finish Cinder and clear the fifth wave to open the approach to the Dominion. Break its four shield generators, dock, and take the pilot of the ship you landed into the interior. Fight through the corridors to discover what the enemy has been searching for.

Every pilot switch is a camera cut. A clean ram or a missile turned back on its owner can earn a money shot. Lose the take and the director calls for another.

## Play on Windows

Extract the full Windows build to a folder and launch **unrealGameJam.exe** inside. Keep the accompanying folders beside it. Choose **New Game** to start the first scene. Once you have control, press **1** for Rift, **2** for Lancer or **3** for Bastion. On a gamepad, press left or right on the D-pad to cycle through the squad. Use **Settings** to adjust controls and sound.

### Flight controls

| Action | Keyboard and mouse | Gamepad |
|---|---|---|
| Increase / decrease throttle | W / S | Right / left trigger |
| Aim and turn | Mouse | Right stick |
| Roll | A / D | Left stick left / right |
| Boost | Left Shift | Click left stick |
| Drift | C | B |
| Fire primary / secondary | Left / right mouse | Right / left bumper |
| Signature power / unlocked ability | Q / E | X / Y |
| Select Rift / Lancer / Bastion | 1 / 2 / 3 | D-pad right / left to cycle |
| Cycle ships | Tab / Shift+Tab | D-pad right / left |
| Hyperdrive | H | D-pad down |
| Toggle wingman shadow mode | G | View / Select |
| Land near a pad | Hold F when prompted | Hold A when prompted |
| Pause | Escape | Menu / Start |

### On-foot controls

| Action | Keyboard and mouse | Gamepad |
|---|---|---|
| Move / look | WASD / mouse | Left / right stick |
| Strike / block | Left / right mouse | Right / left trigger |
| Interact | E | X |
| Close terminal scan; dialogue continues | X / Backspace | B while the scan is open |
| Jump | Space | A |
| Sprint | Hold Left Shift | Click left stick to toggle |
| Dodge | Q | B |
| Crouch | Left Ctrl | Click right stick |
| Pause | Escape | Menu / Start |

Open Settings from the pause menu to adjust controls and sound.

## Credits and development

**Ryan Thomas — Gameplay & Engineering Lead:** game design, flight and three-ship switching, combat and enemy AI, waves and the game loop, planets and levels, environment, VFX and animation direction, packaging.

**Nicholas Tosti — Player Experience Lead:** UI and menus, audio and music, voice, and story.


Built in Unreal Engine 5.9 for the 2026 Epic AI MegaJam theme **Director's Cut**. The project credits identify the team, tools and assets used. The development log follows the playable build through the jam.

[Project credits](https://github.com/rthomas24/2026-epic-ai-megajam/blob/main/CREDITS.md) · [How we built Triad Drift](https://github.com/rthomas24/2026-epic-ai-megajam/blob/main/docs/visual/Submission/glass-box.md) · [Development log](https://github.com/rthomas24/2026-epic-ai-megajam/blob/main/DEVLOG.md)

## Submission disclosure draft — review before publishing

This section is for the submission form. Reconcile it against the final packaged asset inventory; repository experiments and audition candidates are not automatically shipped assets. Team name, final download and trailer links, and team members' Epic-account emails still need the submitting humans. Do not publish private account emails in the repository.

**Tools used:** Unreal Engine 5.9; Claude Code for engineering and builds; OpenAI Codex for presentation, music integration, documentation and review; ChatGPT/OpenAI image generation for Nicholas's original reference boards and survey screens; Blender; Python/NumPy; FFmpeg; Meshy AI for Nicholas's supplied models; Suno for the supplied tracks identified as Suno in the credits; Hyper3D Rodin Gen-2.5, fal.ai, PBRgen and HY-Motion 1.0 for the generated models, textures and animation records listed in CREDITS.md. Rodin has actual imported assets, not merely a planned experiment. Engineering's generated flipbooks/Wan tool attribution remains in the existing owner reconciliation queue.

**External and supplied assets:**

| Category | Declaration and final-package check |
|---|---|
| Epic templates and animations | Existing Unreal template content and retargeted template animations are identified in CREDITS.md; do not describe these as wholly original team-created media. |
| Nicholas's custom models | Original-design models created with Meshy, including Sentinel Armor; credit Nicholas and Meshy. His reported Pro subscription and integration authorization are recorded; exact generation-date terms are not independently certified. |
| Generated models and animations | Rodin/fal/HY-Motion outputs and their per-asset SOURCE.md records are listed in CREDITS.md. Preserve those attributions for the assets included in the final build. |
| Music | Credit Nicholas's supplied tracks and the creation tool where documented. Do not infer that every supplied track has the same creation or subscription history; Core/Cinder receipts remain an existing documentation gap. |
| Licensed audio candidates | PlaceHolder Inc's 50 Free Game Sounds Pack, Hove Audio's Terminal UI LITE, Marika Schanz's Atmospheric Worlds, and rawAmbience/MAGSpel supplied recordings have provenance or audition records. Final inclusion and exact acquisition/redistribution evidence must be reconciled; PR #12 is not a blanket approval to ship every candidate. |

**Pre-jam declaration:** Nicholas confirmed that these eight supplied models were made before November 12, 2026: Coilgun Assault Rifle; Crimson Vanguard; Nebula Shadowfighter; Spaceship (white hull, cyan ring engines); Circular Bulkhead Wall; Gothic Stone Arch; Crimson Tempest; UnlabelledRifle01. Declare each that appears in the final submitted build. This does not assert that all eight were imported, that Gothic Stone Arch remains the chosen ancient-artifact design, or that the other supplied files share those dates. The Ancient Relay board superseded the earlier artifact proposal. Source: Nicholas's confirmation recorded under R-037 in MESSAGEBOARD.md.

Outstanding disclosures are tracked under R-045 and the asset ledger's section 2.7. This is a review draft, not legal clearance or the final submitted declaration.


## November 15 soundtrack addition

Ritual of the Final Dance was supplied by Nicholas Tosti for the Dominion escape. The complete 127.6-second source was decoded with FFmpeg/Python to 48 kHz stereo 16-bit PCM; no new acquisition or generation took place in this completion pass. The source SHA256 is `097a7a13ff224084b33d8b2a862db4d1969d86c93de40b9251668c56a702de2e`; the master SHA256 is `535fa9783d4f2d545958613652ae5b56c4cf3c7c1f14ca1dee70ab5704d58eec`. Record any creation-tool or subscription attribution only from its actual source evidence; do not infer it from another track's Suno receipt.

The companion CREDITS.md completion addendum records Ritual and the supplied-OST style replacements. Historical package-specific inclusion notes remain preserved. Final inclusion must be checked against the selected Shipping container inventory.

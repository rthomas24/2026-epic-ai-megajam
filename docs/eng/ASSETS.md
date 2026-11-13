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
| **Nicholas's supplied models** (Meshy and others; N-041, N-042) | 14 imported (2 pilots incl. Sentinel Armor in the game, 12 others: ships, characters, a stone arch), 5 weapons, 36 further sources | Branches `nick/assets-meshy-intake02` @ `4587658`, `nick/assets-weapons-intake` @ `55d0010`, `nick/assets-meshy-intake03` @ `aa53ff9c` | Nicholas's own; **a library Ryan's side selects from**; Azure Shadow excluded by Nick's side |
| **Epic** | Template mannequin animations; Game Animation Sample Project (approved by Ryan); engine content | Project `Content/Characters/**`; Fab | Unreal Engine EULA; confirm and credit before use |

## 2. In the game today (summary; detail to be filled by the asset builder)
| Area | Asset | Source | State | Next |
|---|---|---|---|---|
| Player ships | Rift, Lancer, Bastion r7 | Engineering (Blender) | in game | Replaced by Grok Bot's Rodin ships (concepts approved) |
| Enemy fleet | Fighter, interceptor, gunship, bomber, carrier, turret | Engineering | in game | Rodin fleet (concepts approved) |
| Dominion | Exterior r9, generators, reactor | Engineering | in game | Hangars that open (story); Rodin generator / reactor |
| Dominion interior | 22-piece kit `SM_DI_*` | Engineering | source in, imported | First room built by T5; lighting |
| Pilot | Sentinel Armor | Nicholas | in game | Blade animations |
| Crew | Trooper, grunt, heavy on `SKEL_NullCrew` | Engineering | in game | Per-type animation sets |
| Drone | `SM_NullDrone` | Grok Bot (Rodin) | in game | Hitbox / kill check done by T5 |
| Warden | - | Grok Bot | concept approved | Rigged model |
| Energy blade | Placeholder | Engineering | in game | Grok Bot concept sheet → Rodin |
| Planets | Terrain (code), `M_TD_Terrain`, planet kit, trees, 9 PBR sets | Engineering + Grok Bot | in game (PBR sets: source in) | PBR sets into the material; scatter |
| Sky and space | 8K nebula sky, dressing | Engineering | in game | Lighting and grade pass; distortion field |
| Effects | 21+ Niagara systems, Grok flipbooks | Engineering + Grok Bot | in game (tier 1) | Round 5; tier 2 |
| Sounds | 200 stand-in sounds | Engineering | in game | Ear pass; Nick's music and replacements |
| UI | Engineering flight HUD and fallback menus; Nick's HUD plugin | Engineering / Nick | in game | Nick's menus and Director's Cut cards |

## 3. Nicholas's supplied models: review for use (to do)
For each of the 14 imported and 5 weapons, and the 36 sources: what it is, triangle count, whether it fits the story
and the look (the Null Armada is the squad's own future: black armour, red seams), where it could be used (a ship in
the fleet streaming to the planets, wreckage on Verdance, an outpost prop on Cinder, a crew variant, the trooper
rifle), and the cost to make it game-ready. Decision per asset: `use` / `use after clean-up` / `not now`.

# Triad Drift: Feature Spec & Work Split (Ryan × Nick)

> **Working title:** Triad Drift · **Jam:** 2026 Epic AI MegaJam · **Engine:** UE 5.9 (MegaJam Preview), project `unrealGameJam`
> **Theme:** **DIRECTOR'S CUT** (how the game expresses it: C-03)
> **Story:** **The Paradox** (Nicholas's narrative, adopted by Ryan Nov 13): [`storyline.md`](storyline.md). Every feature below serves it.
> **Hard deadline:** Nov 15, 2026, **8:59 PM EST / 5:59 PM PST**. Late entries are disqualified.
> **Feature freeze:** Nov 15, 12:00 PM EST. After that, only fixes, polish, packaging and submission.

This file splits the game into features and assigns each one to **Ryan (Gameplay & Engineering)** or
**Nick (Art, Visuals & Player Experience)**, following the team role briefs (Nov 12). Read §1–§3 first:
the split, folder ownership, and the contracts that let the two of you work at the same time without
editing the same files.

## Scope
- **Core (must ship):** everything not tagged **STRETCH**. That includes:
  - flight, instant three-ship switching, AI wingmen and combat
  - enemies and waves, hyperdrive and portals
  - **all signature powers and unlockable abilities**
  - **both landable planets** with their on-foot objectives
  - the Dominion space-assault finale
  - full presentation (art, VFX, UI, audio) and packaging
- **The boarding finale is WANTED (Ryan, Nov 13; it was stretch):** docking, the Dominion interior on foot, the
  reactor-core fight, the escape run and the escape flight (ENG-29, 30, 31), in a scoped version that can be solid
  by the freeze. It carries acts 4 and 5 of the story. **Fallback, always kept working:** the space-only ending,
  where the squad destroys the exposed reactor from outside (ENG-18, `td.Finale.Boarding 0`).
- **Added Nov 13 (Ryan), core: the storyline.** Nicholas's narrative *The Paradox* is the game's story
  ([`storyline.md`](storyline.md)): objective names, dialogue, set pieces, clues and the ending all follow it.
- **Added Nov 12 (Ryan), core:** a Settings page and an on-foot first / third-person camera choice (ENG-32).
- **Added Nov 12 (Ryan), core:** a high-quality cinematic intro of space, the squad, worlds and the Armada (ENG-33).
- **Added Nov 13 (Ryan), core:** both planets become large explorable worlds with procedural terrain and mountains (ENG-34), and engineering supplies a fallback sound set so the game is never silent (ENG-35).
- **Added Nov 13 (Ryan), core:** the HUD is redesigned: modern, sleek, small, and clear to a beginner (ENG-36).
- **Added Nov 13 (Ryan), core:** AAA projectiles, engine flames and particle effects across the whole game (ENG-37).
- **Added Nov 13 (Ryan), core:** flying from space down into a planet is seamless and AAA, in the spirit of No Man's Sky: orbit the globe, enter the atmosphere anywhere, a visible burn-in, the world coming into focus (ENG-38).
- **Protect above all:** the three-ship switching mechanic and satisfying combat.
- **C++ first;** use Blueprints when they are clearly faster or necessary.
- Keep a playable build at all times, coordinate shared-file edits before starting, verify licences
  for third-party resources, and freeze major features before final submission.

All numbers are **starting tuning values** (Unreal units: cm, cm/s, seconds). Keep them in data
assets or tables, not hard-coded.

**Status key** (edit inline): `⬜ todo` · `🟨 in progress` · `🟢 built and verified` (on `main`, automated
tests, independent review closed, played by an agent in the running game; **waiting for a human to play it**) ·
`✅ done` (all of that, and a human has played it and is happy) · `🐞 has bugs`

**Where the game stands and who does what today: [Nov 13 realignment](#nov-13-realignment-status-and-plan).**
Story: [`storyline.md`](storyline.md). Gameplay dialogue: [`dialogs.md`](dialogs.md). Animation plan and ledger: [`animations.md`](animations.md). Item by item with tests, reviews and open findings: `docs/eng/STATUS.md` (engineering) and
`docs/visual/STATUS.md` (presentation). Live ownership: `CLAIMS.md`.

### Contents
[Nov 13 realignment](#nov-13-realignment-status-and-plan) · 0. [The game](#0-the-game) · 1. [The split](#1-the-split) · 2. [Folder & file ownership](#2-folder--file-ownership) ·
3. [Shared contracts](#3-shared-contracts) · 4. [Shared setup](#4-shared-setup-c-xx) ·
5. [🟦 Ryan: Gameplay & Engineering](#5--ryan-gameplay--engineering) ·
6. [🟩 Nick: Art, Visuals & Player Experience](#6--nick-art-visuals--player-experience) ·
7. [Landing: full sequence](#7-landing-full-sequence-core) · 8. [Boarding: full sequence](#8-boarding-full-sequence-stretch) ·
9. [Integration points](#9-integration-points) · 10. [Dependency order](#10-dependency-order-no-dates) ·
11. [Bugs & requests](#11-bugs--requests-append-only)

---

## Nov 13 realignment: status and plan

Written Nov 13, late morning, by Ryan's Lead at Ryan's request, as a fresh start for the day. It marks what is
built, says what is still weak, and gives each of the **three lanes** its work. The lists for Nick's agents are
**Ryan's proposal**: Nicholas keeps art, visual and audio direction and confirms or changes them on
`MESSAGEBOARD.md` (R-028). Nothing here cuts scope.

### The clock
| When (EST) | What |
|---|---|
| **Today, Nov 13** | Finish the content that is still missing (terrain worlds, new HUD, effects, real 3D ships and enemies, sounds wired). Every lane lands on `main` by tonight. First human playthrough. |
| **Nov 14** | Polish day: the AAA list below, human playtests morning and evening, balance, performance, a packaged build played start to end, the gameplay video and the itch page drafted. |
| **Nov 15, 12:00 PM** | Feature freeze. Only fixes after this. |
| **Nov 15, about 3:00 PM** | Submit a build (the deadline is 8:59 PM; we do not wait for it). Improve and re-upload until 7 PM at the latest. |

### Where the game stands
The whole journey runs on `main`: main menu → intro → waves 1-2 → Verdance → waves 3-4 → Cinder → wave 5 →
Dominion assault → reactor → victory, with an automated end-to-end test, Settings, save and Continue, a
packaged Shipping build of about 800 MB zipped, and about 630 automated tests. Agents have played every part
through the engine's MCP server. **No human has played it yet; that is the largest open risk in the project.**

| Area | Items | State | What is still missing |
|---|---|---|---|
| Flight, input, three ships, switching, wingmen | ENG-01, 02, 03, 04, 07, 08 | 🟢 | A human's judgement of flight feel and switching. Low flight over the new terrain; boost in atmosphere (today) |
| Weapons, targeting, powers, abilities | ENG-05, 06, 09, 10, 11, 12 | 🟢 | Hit feedback and effects are the weak part (ENG-37). Each ability used live once more after the effects land |
| Enemies, waves, balance, difficulty | ENG-13, 14, 27 | 🟢 | Enemy ships read small and dark at range; a human run on each difficulty |
| Hyperdrive, gates, galaxy, Dominion finale | ENG-15, 16, 17, 18 | 🟢 | The whole assault played to victory by a person; the breakup needs real effects |
| Game flow, saves, landing, on foot, blade, crew, objectives | ENG-19, 20, 21, 22, 23, 24 | 🟢 | Deflect timing judged by a person; blade and crew animation quality; Warden and drone have no mesh |
| The two planets | ENG-25, 26, 34 | 🟨 | Objectives are finishable; the worlds are being rebuilt as large terrain with mountains (shape on `main`, materials and scatter today); courses re-fitted and replayed on it |
| Director's Cut (cut, money shots, scenes and takes), intro, third person, Settings | C-03, ENG-32, 33 | 🟢 | Seen with the new HUD and with sound; a person's eye |
| Sound | ENG-35 (+ ART-10) | 🟨 | 200 fallback sounds built and wired, on `main` today. Nothing has been approved by ear. Music: menu theme only, one combat track not wired by Nick's side |
| HUD | ENG-36 (+ ART-09) | 🟨 | New flight HUD: first pass matches the design, second pass today, then it becomes the default. Nick's HUD stays selectable |
| Effects | ENG-37 (+ ART-07) | 🟨 | 21 Niagara systems, round 2 scored 3 of 5; tracers and explosions are being wired today, engines / shields / muzzle flashes need another round |
| Ship, enemy, Dominion, crew, prop art | ART-03, 04, 06, 08 as engineering fallbacks | 🟨 | Everything has a model in the game (score about 3 of 5). Real hero ships, the fleet, Warden and drone are being generated from approved concepts (Grok Bot) |
| Sky, lighting, look (**Ryan's since Nov 13**) | ART-05, 13 | 🟨 | Engineering's 8K sky is in; no lighting or grading pass per level yet: an environment builder starts on it now |
| Menus, reward screen, credits, loading | ART-09 | 🟨 | Engineering fallbacks work; styling is plain. Nick's menu is on a branch |
| Barks, voice | ART-11 | ⬜ | Text barks only |
| Submission (video, page, dev log, credits) | ART-14, ENG-28 | 🟨 | 58 s trailer pipeline exists (silent cut, old HUD); page not started; `CREDITS.md` and `DEVLOG.md` are current |
| Planetary descent (space to surface, anywhere on the globe) | ENG-38 (+ ENG-20, ENG-34) | 🟨 | Added Nov 13. Today: a level change behind a short entry. Research in progress; stage 1 next |
| Boarding finale | ENG-29-31, ART-15, 16 | 🟨 WANTED (Ryan, Nov 13) | Scoped plans in from T4 (docking, escape flight, about 14 h) and T5 (interior, core, escape run); the space-only ending stays as the fallback |
| Story (The Paradox) | `storyline.md`; ENG-18, 25, 26, 29-31, 33; ART-11, 12 | 🟨 | Adopted Nov 13. Hooks and renames on Ryan's side; words, voice and the final say on story with Nicholas |

Judges' categories, our own honest read today: gameplay 3.5, fun 3 (unproven without a human), theme 4,
ambition 4, level design and progression 3, visuals 3, audio 2. **Today and tomorrow go to visuals, audio and
feel, in that order of weakness.**

### The three lanes

| Lane | Who | Owns today | Writes in |
|---|---|---|---|
| **Engineering** | Ryan's Lead + teammates T1-T5 + builder sub-agents (Claude Code) | All gameplay code; integration to `main`; the takeovers Ryan ordered while Nick's side was stopped: planet worlds (ENG-34), fallback sounds (ENG-35), flight HUD (ENG-36), effects (ENG-37); cleaning up, importing and wiring every generated asset | `Source/`, `Config/`, `Content/TriadDrift/Gameplay/**`, `ArtSource/Eng*/**` |
| **Asset generation** | Grok Bot (Ryan's agent; fal, Hyper3D Rodin, PBRgen) | Concepts and raw 3D, textures, animations and key art from the ranked list in R-027. Sources only; never Unreal assets, code or maps | `ArtSource/EngAI/**`, `grok/assets-*` branches, merged by Ryan's Lead |
| **Presentation** | Nick's Lead + agents (Codex) | Direction and sign-off on everything seen and heard; music and the final mix; menus and the Director's Cut presentation; voice; the submission page and video | `Content/TriadDrift/{Art, Environment, VFX, UI, Audio, Cinematics, Presentation}/`, `Plugins/TriadDriftHUD/`, `ArtSource/` (not `Eng*`) |

How the lanes meet, unchanged: gameplay reads `DA_*_Visuals` / `DA_*_FX` / `DA_UI_Style`. An asset of Nick's
at the Presentation name always wins over an engineering fallback with no code change, so nothing engineering
or Grok Bot makes blocks or overwrites Nick's work, and Nicholas can take any task back on the board.

### Who owns what now (Nov 13, decided by Ryan)
Ryan's rule: **anything Claude or Grok Bot is making is under Ryan's name; anything Ryan's side is not making is
under Nick's name.** **Update, Nov 13 afternoon (Ryan): the categories Environment, VFX and Asset Management are taken
over completely by Ryan's side** (Claude and Grok Bot): Nick's agents do no work in them. The items were moved in this file to match: `ART-03, 04, 06, 07, 08` now sit in Ryan's
section (§5, "Art and assets made on Ryan's side"), with their IDs unchanged so older references still work.
Nicholas keeps art direction and final sign-off on everything, and anything he makes at the Presentation name
still replaces ours with no code change.

| What | Item | Owner | Made by | Put in the game by |
|---|---|---|---|---|
| Player ships (Rift, Lancer, Bastion) | ART-03 | 🟦 **Ryan** | Grok Bot (Rodin, approved concepts) | Claude builders (clean-up, import, sockets), T1 |
| Enemy ships and turrets | ART-04 | 🟦 **Ryan** | Grok Bot | Claude builders, T4 |
| Dominion exterior, generators, reactor | ART-04 | 🟦 **Ryan** | Claude builders (exterior, in game); Grok Bot (generator, reactor) | T4 |
| Crew, Warden, drone; blade animations | ART-04, ART-08 | 🟦 **Ryan** | Grok Bot (Warden, drone, animations); Claude builders (trooper, grunt, heavy) | T5 |
| Planets: terrain, mountains, materials, rocks, trees, set pieces | ART-06, ENG-34 | 🟦 **Ryan** | Claude (T4 in code, builders); Grok Bot (texture sets, set pieces) | T4, T5 |
| Effects: projectiles, engines, explosions, powers | ART-07, ENG-37 | 🟦 **Ryan** | Claude effects builder; Grok Bot (flipbook textures) | T1, T2, T4 |
| Flight HUD | ENG-36 | 🟦 **Ryan** | Claude (T2) | T2 |
| Sound effects | ENG-35 | 🟦 **Ryan** | Claude audio builder | T1-T5 |
| Space dressing, 8K sky stand-in, intro, capture pipeline for the video | ENG-17, ENG-33 | 🟦 **Ryan** | Claude | T3, T4 |
| **Animation**, all of it (plan and ledger: `animations.md`): characters, mechanical parts, in-engine motion | ART-08 | 🟦 **Ryan** | Grok Bot (raw clips, rigged models); Lead's animation builder (retarget, clean, cut, blueprints) | T5 (which move plays when, hit and deflect timing) |
| **Asset management**: the inventory of every asset, sourcing, licences, intake of supplied models | ART-02 | 🟦 **Ryan** | Lead (ledger `docs/eng/ASSETS.md`, `CREDITS.md`), Grok Bot (generation) | Lead's asset builders |
| Art direction notes and style guide | ART-01 | 🟩 **Nick** | Nicholas | - |
| **Environment**: space sky, nebulae, galaxy dressing and look | ART-05 | 🟦 **Ryan** | Lead's environment builder; Grok Bot (panoramas, textures) | T4 (galaxy), Lead's builders |
| **Environment**: lighting, fog, sky, atmosphere, grade and post-process on every level (galaxy, both planets, the Dominion interior, the menu), and the descent's atmosphere (ENG-38) | ART-13 | 🟦 **Ryan** | Lead's environment builder; Grok Bot (cloud and sky textures) | T4, T5 (their levels) |
| Menus, reward, victory, credits, loading screens; fonts, icons, colours | ART-09 | 🟩 **Nick** | Nick's UI agent | Nick |
| Music and the final mix; approval of every sound by ear | ART-10 | 🟩 **Nick** | Nick's Audio agent | Nick |
| Voice and barks | ART-11 | 🟩 **Nick** | Nick's Audio agent | Nick |
| Director's Cut presentation design (slate, CUT, name card, letterbox) | ART-12 | 🟩 **Nick** | Nick's UI agent | Nick |
| Itch page, video edit, screenshots, dev log | ART-14 | 🟩 **Nick** | Nick's Lead | Nick + Ryan's Lead |

**What remains Nick's:** UI and menus (ART-09), audio, music and the mix (ART-10), voice and the words of the story
(ART-11), the Director's Cut presentation design (ART-12), the submission page and video (ART-14), and art direction
notes (ART-01). If Nick's side does not pick up one of those rows by Nov 14 noon, Ryan's Lead takes it as a fallback
and says so on the board (AGENTS.md §11).

### Storyline (adopted Nov 13): see [storyline.md](storyline.md)
Ryan adopted Nicholas's narrative, **The Paradox**: the silent mothership and its machines are the squad's own future,
come back for the Origin Core. `storyline.md` holds the story and, act by act, the gameplay moment that carries each
beat. **Nicholas owns the story, its words and voices; Ryan's side owns how each beat is played** and exposes a hook
with a plain placeholder line for every beat. Nothing reveals the twist before the Dominion. What changes in the game:

| Change | Act | Owner | Size |
|---|---|---|---|
| The Dominion is visible from wave 1, unreachable behind a distortion field (its four generators); "reveal" becomes "the passage is open" | 1, 3, 4 | T4 (+ effects builder for the field) | small |
| Intro text and shots follow the premise: the silent ship, machines searching worlds, Triad deployed | opening | T3 (hooks), Nicholas (words, voice) | small |
| Verdance: objective 1 recovers the **signal-analysis device** from the wrecked research site; objective 2 wakes the relay and plays the **voices** | 2 | T5 (names, hook), builders / Grok Bot (dressing), Nicholas (audio) | small |
| Cinder: the outpost is the **mining complex**, the jammer is the excavation's **field anchor**; clearing them yields the stabilising crystals and opens the passage; the Warden guards the dig | 3 | T5, T4, builders / Grok Bot | small |
| Transmissions: a small event system (id, speaker, placeholder line) fired at story beats; echo-voice lines from Cinder on | 2-5 | T3 (system, with T5 / T4 triggers), Nicholas (lines, voice) | medium |
| **Boarding finale**: docking, interior with consoles that recognise the pilots and a recording per room, core fight, escape run and flight | 4, 5 | T4 (docking, escape flight), T5 (interior, core, escape run), builders (interior kit), Grok Bot (Warden) | large |
| Ending stinger: the distress signal with the squad's own identifiers on the victory screen | 5 | T3 / T5 (hook), Nicholas (text, audio) | small |
| **Opening cinematic redone to the *First Contact* brief** (2:20) and waves 1-2 shaped as its tutorial battle (evasion, weapons, switching, a larger craft, the withdrawal boundary, the Verdance distress call); the finale's command-deck payoff reusing the same "Don't-" audio | opening, 1, 4 | T3 (+ T4, T1, effects builder), Nicholas (lines, voice, score) | large; **queued until the models and effects are in, done once** |
| Clues: enemy pre-empting a manoeuvre now and then; Triad's insignia shape under damaged enemy armour; one ancient-technology artefact on both planets and in the core | 1-4 | T4 (AI), Grok Bot / builders (assets) | small each |
| Pilot characters per the story (Wren bold, Kato analytical, Moss steady) in every bark and name card | all | T3 (hooks), Nicholas (lines) | small |

### Environment, VFX and Asset Management: taken over by Ryan's side (Nov 13, afternoon)
Ryan: these categories are taken over completely by Claude and Grok Bot; none of that work is Nick's any more.
- **Environment** (ART-05, ART-06, ART-13, with ENG-34 and ENG-38): space sky and galaxy dressing; both planets'
  terrain, materials, scatter and set pieces; lighting, fog, atmosphere, grade and post-process on every level; the
  Dominion interior's look. Owner: the Lead with an **environment builder**, T4 and T5 for their levels, Grok Bot for
  panoramas, cloud and surface textures.
- **VFX** (ART-07, ART-16, ENG-37): every Niagara effect, camera-shake class and post-process material. Owner: the
  Lead's **effects builder**, with T1 / T2 / T4 / T5 hooks and Grok Bot's flipbooks.
- **Asset Management** (ART-02): one ledger of every asset, its source, licence and state (`docs/eng/ASSETS.md`),
  `CREDITS.md`, `docs/eng/ART-HISTORY.md`; choosing what to import from Nick's supplied-model intake (N-041, N-042);
  briefs and approvals for Grok Bot. Owner: the Lead with an **asset builder**.
- **Nick's side now:** UI and menus, audio, music and the mix, voice and the words of the story, the Director's Cut
  presentation design, the submission, and art direction notes. His agents stop work in the three categories at a safe
  checkpoint and publish what they have as reference; nothing of theirs is deleted.

### Planetary descent (added Nov 13, Ryan): ENG-38
Ryan wants the flight from space into a world to be a centrepiece: orbit the globe, enter the atmosphere anywhere, a
visible burn-in on the ship, cloud deck, and the world coming into focus, with no loading screen, "almost like No
Man's Sky". Spec in ENG-38; research and the recommended architecture in `docs/eng/planet-descent-research.md`.
Everyone has a part: T1 flight and entry, T4 the globe and the focus-in, T5 the hidden load, T2 the HUD, T3 the
camera, the effects and audio builders, Grok Bot for cloud, plasma and planet-surface assets, Nick for the
atmosphere's look. It is built in three stages; stage 1 (the entry sequence at full quality) comes first.

### Gameplay dialogue (added Nov 13): see [dialogs.md](dialogs.md)
Every short line the pilots and Command say during play (targets, hits, powers, switching, travel, landing, on foot,
the Dominion, banter) has a row in **`dialogs.md`**: id, when it fires, who speaks, priority, and working lines for
Wren, Kato and Moss in character. About 75 ids, 22 of which the game fires today. T3 owns the bark system (lines loaded
from a table generated from that file, rotation, a Flavour priority, the banter queue); each row's owner adds its
trigger. The wording is a working draft from Ryan's side: **Nicholas owns the final lines and voices** (ART-11) and
his `DT_Barks` rows replace ours by id.

### Animation (Ryan's lane, added Nov 13): see [animations.md](animations.md)
Every animation in the game has a row in **`animations.md`**: who makes it, by which route, where it lives and its
state. Ryan's Lead leads it. Three routes: generated or library motion through Grok Bot and third-party tools, authored
in Blender, or built in-engine (animation blueprints, blend spaces, Control Rig, code). Order of work: audit what
exists, the pilot's blade set, pilot body mechanics (strafing, foot IK on the new slopes), crew sets, the Warden,
interactions, mechanical parts. Nick's agents ask or send notes with an `Animation:` message on the board.

### Engineering today (Claude side)

| Who | Now | Next |
|---|---|---|
| **Lead** | Integrate and push in batches (sounds, T1-T5, terrain, effects); approve Grok Bot's concepts and merge its PRs; keep `CLAIMS.md`, `docs/eng/STATUS.md`, `DEVLOG.md` current | Lead playtest of `main` and of a packaged build after each batch; a fresh independent review of ENG-34 to 37; hand Ryan a build to play tonight |
| **T1** flight, input, travel | Boost in atmosphere (cap 60% of boost speed) so the mountains are reachable; camera and collisions against hillsides and ridges | Engine exhaust driven by throttle and boost, one per nozzle (for ENG-37); camera feel pass: speed FOV, boost kick, hit shake, all scaled by the Settings sliders; gamepad pass on every screen |
| **T2** combat, powers, HUD | ENG-36 second pass: target bracket, lead marker, edge arrows, missile warning, hit arcs, boost and hyperdrive bars; then the new HUD becomes default | Hit feel: hit and kill confirms (sound + reticle), brief hit-stop on ram and big kills; every power with its real effect; on-foot HUD in the same style |
| **T3** squad, director | Director sounds live (slate, CUT, slow motion); each money shot captured once with the new effects | Onboarding: the first two minutes teach fly, shoot, switch, power with one prompt at a time (a beginner must never be lost); re-cut the gameplay video with sound and the new HUD; victory and credits screen |
| **T4** enemies, waves, world | ENG-34: wire the terrain materials, recapture both planets, biome scatter (rocks, crystals, trees), streaming at 90 m/s | Enemy readability at range (scale, glow, silhouette light); Dominion breakup with real explosions; review minors; frame-rate pass on both planets and wave 5 |
| **T5** flow, ground | Course fit on the new terrain, both planets replayed in third person; footsteps, blade, crew and objective sounds; menu theme loops | Warden and drone meshes and blade animations when Grok Bot delivers; crew fights played end to end; pause, death and retake on foot polished |
| **Effects builder** | Wire tracers, explosions, hull impact; round 3 on engines, shield impact, muzzle flashes | Tier 2: portal burst, ram streak, missile trails, EMP ring, dome, cloak, gravity well, hyperdrive streaks; tier 3: on-foot blade trail, deflect spark, crew bolts, lava and jungle ambient particles |
| **Material / asset builders** | Swap Grok Bot's 2K PBR sets into `M_TD_Terrain` after T4's recapture | Clean up, import and socket each generated ship, enemy and character as it arrives (`blender-ai-mesh-cleanup`, `blender-unreal-export`), one review sheet per asset, old version kept (`docs/eng/ART-HISTORY.md`) |

### Asset generation today (Grok Bot), in this order
1. **Rift, Lancer, Bastion** in 3D from the approved r2 concepts (Rodin). The single most visible upgrade.
2. **The pilot's energy blade** (hero melee weapon, the physical hilt and emitter; added by Ryan Nov 13, R-027 #12), **with the pilot's blade combat animations** (moved up Nov 13; R-030).
3. **Warden** (emblem removed; rigged) for the Cinder boss fight. The drone is delivered and in the game.
4. **Null Armada fleet** from the approved concepts: fighter, interceptor, gunship, bomber, carrier, turret.
5. **Effect textures** (fire, smoke and explosion flipbooks, shockwave ring, sparks, flowing noise): quick, and the effects builder is waiting for them.
6. `Cinder_Basalt` r2 (natural columnar basalt), then **blade combat animations** for the pilot.
7. Dominion shield generator and reactor; planet hero set pieces (crash wreck, relay altar, outpost gate, jammer tower).
8. Key art, itch banner, menu still, trailer end card (Nicholas decides final use).

Each delivery: raw files untouched, `SOURCE.md` with tool, prompt and licence, a contact sheet, a PR. Hero
assets pass the concept gate first; Ryan's Lead answers within the hour.

### Presentation today (Nick's agents): proposed by Ryan
Engineering and Grok Bot now cover models, terrain, effects and the flight HUD as fallbacks, so Nick's agents
are most valuable where **an ear, an eye and taste** decide, and where nobody else is working:

| Priority | Work | Item | Why it is yours |
|---|---|---|---|
| 1 | **Music and the mix.** Combat music that swells with the waves (`MusicIntensity01` is live in `UTDAudioStateSubsystem`), planet themes for Verdance and Cinder, Dominion assault, victory sting; merge the menu loop (PR #11) and the Lancer cues (PR #12); then the final mix by ear across music, effects and voice | ART-10 | Audio is our lowest score, and only Nicholas can approve sound by ear. Engineering's 200 sounds are stand-ins: keep, replace or re-level any of them through `DA_UI_Style` and the `DA_*_FX` Sound fields (yours win) |
| - | ~~Lighting, sky and look per level~~ | ART-05, ART-13 | **Moved to Ryan's side on Nov 13 (afternoon).** Nick's agents do not work on environment, VFX or asset management; Nicholas may still send art notes |
| 3 | **Menus and the Director's Cut presentation.** Main menu, pause, Settings styling, ability reward screen, victory and credits, loading screens; slate card, "CUT!", pilot name card and letterbox as a designed set; fonts, icons and colour tokens that the new flight HUD then adopts | ART-09, ART-12 | Your menu branch (`1a8d942`) and HUD design language already exist. The flight HUD itself is ENG-36 by Ryan's instruction; review it and send notes |
| 4 | **Voice and barks.** Pilot barks for Wren, Kato and Moss, a mission-control line per scene, intro narration | ART-11 | Hooks exist (`OnBarkRequested`, intro narration slot in `UTDUIStyle`); nothing uses them |
| 5 | **Submission.** The itch page, screenshots, the 30-60 s gameplay video (engineering's capture pipeline `Tools/td-trailer.ps1` can supply footage), the dev log for Glass Box, "Tools used" | ART-14 | Has a hard deadline and nobody has started it |
| 6 | **Art review, by eye.** Look at every engineering fallback and every Grok Bot delivery in the running game and say approve / restyle / replace on the board | ART-01 | Nicholas keeps sign-off; an hour of notes steers three lanes |

**Not Nick's any more (Ryan, Nov 13): everything in Environment, VFX and Asset Management**: ships, enemies,
Dominion, crew, props, terrain, sky, lighting and post-process, Niagara effects, asset intake and inventory. Also
parked: the M04 flight HUD. Say on the board if you want any of
them back and engineering stops that fallback the same hour.

### The AAA polish list (both days, in priority order)
Each line names the owner; a line is done when it is seen (or heard) in the running game, not when it compiles.

**Feel (fun, gameplay)**
1. A human plays the whole game and we fix what they hit first (Ryan tonight, Nicholas when awake).
2. Hit feedback on every weapon: tracer, muzzle, impact, hit sound, reticle tick, kill confirm (T2, effects builder).
3. Camera: speed FOV, boost kick, shake on hits and explosions, no clipping into terrain (T1).
4. The first two minutes teach the game one prompt at a time; nothing on screen a beginner cannot read (T3, T2).
5. Difficulty curve over five waves on Story / Standard / Ace; no dead time between waves (T4).
6. On foot: blade swings with weight (animation, trail, hit-stop), deflect window readable (T5, Grok Bot, effects).

**Look (visuals)**
7. Real hero ships and fleet in the game with clean sockets and engine exhausts (Grok Bot → builders → T1, T4).
8. Explosions, engines and shields at the level of the ships (effects builder).
9. Planets: mountains with rock, peaks and lava; scatter; a sky and light that sell the scale (T4, environment builder).
10. Dominion finale: generators, reactor and a breakup worth a trailer shot (T4, effects builder, Grok Bot).
11. One consistent UI style from menu to HUD to victory (T2, Nick).
12. Lighting and grade per level (Lead's environment builder; Ryan's since Nov 13).

**Sound (audio)**
13. Every action has a sound and none is harsh or repetitive: an ear pass on the fallbacks (Ryan / Nicholas).
14. Music for menu, combat (rising with the waves), each planet, the finale and victory (Nick; engineering fallback beds exist).
15. Barks and a few voiced lines (Nick).

**Finish (level design, progression, theme, submission)**
16. Each scene opens with its slate and reads as a "take"; money shots fire on every big moment (T3).
17. Both planets replayed start to end on the new terrain; objectives signposted (T5).
18. 60 fps at 1080p on the mid-spec target through wave 5 and both planets; no hitch over 50 ms (T4, Lead).
19. Packaged build played start to end every day; zip under 1 GB (Lead).
20. Video, page, credits, tools list, dev log (Nick, T3, Lead).

---

## 0. The game

A fast, bright, arcade space-combat game with a story: **The Paradox** ([`storyline.md`](storyline.md)). You lead
**Triad**, a three-pilot reconnaissance unit, in three ships (**Rift**, **Lancer** and **Bastion**), switching between
them at any moment while the two you aren't flying fight on their own. A silent mothership, the **Dominion**, has
appeared; its machines are searching the system's planets. You follow their trail across two worlds, completing
objectives on foot that earn each ship a new ability and a way through the Dominion's distortion field, then break the
field, **board the ship with an energy blade**, learn what it is, destroy its reactor and escape.

### Fiction (the story is Nicholas's; full text in `storyline.md`)
- **The squad, Triad:** **Wren** flies **Rift** (bold, instinctive; earns Cloak). **Kato** flies **Lancer**
  (analytical, cautious; earns EMP Pulse). **Moss** flies **Bastion** (steady, good-humoured; earns Gravity Well). The
  player is the wing lead and can take any cockpit.
- **The enemy:** the **Null Armada**, the authorities' name for the angular red-and-black craft and automated ground
  units that emerge from the **Dominion**. They do not answer hails and they search rather than occupy.
  **The truth (never stated before the Dominion):** they are the squad's own future, come back for the **Origin Core**.
- **The worlds:**
  - **Verdance:** a bioluminescent jungle with giant flora, waterfalls and a ringed moon; an abandoned research site
    holds the **signal-analysis device** and a relay full of voices that should not be there.
  - **Cinder:** black volcanic rock, glowing magma and huge cyan / amber crystal spires; a **mining complex** where the
    machines dig for the crystals that stabilise a passage through the field.

### Player journey (acts of the story)

```
Main Menu ─► Intro: the silent Dominion appears, machines descend on worlds, Triad is deployed
  └─► ACT 1 · FIRST CONTACT · Galaxy, waves 1–2: learn flight, shooting, switching, each signature power.
        The Dominion hangs in the distance behind its distortion field: it cannot be approached.
        └─► ACT 2 · VERDANCE, THE VOICES: hyperdrive → atmospheric entry → fly down → land
              ├─► Obj 1 "Recover the Signal-Analysis Device" (the wrecked research site) → unlock GRAVITY WELL (Bastion)
              └─► Obj 2 "Wake the Relay": human voices under the enemy's signal        → unlock CLOAK (Rift) → take off
  └─► Galaxy · waves 3–4 (gunships, bombers; portal gates matter; the enemy starts to anticipate you)
        └─► ACT 3 · CINDER, THE PATTERN: land at the mining complex
              ├─► Obj 3 "Stop the Excavation" (clear the complex)  → unlock EMP PULSE (Lancer) + the stabilising crystals
              └─► Obj 4 "Destroy the Field Anchor" → THE PASSAGE THROUGH THE FIELD IS OPEN → take off
  └─► Galaxy · wave 5 "Dominion's Guard" (carriers) → approach the Dominion
        └─► ACT 4 · THE DOMINION, THE TRUTH: destroy the 4 field generators + turrets
              ├─► dock → interior on foot (energy blade): doors that know you, a recording in every room, the Warden
              └─► ACT 5 · THE CHOICE: core fight → 90 s escape run → escape flight → breakup → Victory
                    └─► the distress signal, carrying your own identifiers, from decades ahead
              (fallback ending, kept working: destroy the exposed reactor from space → breakup → Victory → the signal)
```
Target length: 20–30 minutes.

---

## 1. The split

| | 🟦 **Ryan: Gameplay & Engineering Lead** | 🟩 **Nick: Art, Visuals & Player Experience Lead** |
|---|---|---|
| **AI assistant** | Claude Code | OpenAI Codex |
| **Mission** | Own the playable game systems, engineering integration and technical reliability, so the game stays fun, responsive and shippable | Own the visual identity, presentation quality and player-facing feel, compatible with the gameplay foundation and the 4-day window |
| **Owns** | Flight and controls; three-ship switching and wingman AI; weapons, damage, shields, enemy AI, waves, targeting, balance, signature and unlockable power *mechanics*; objectives, encounter pacing, hyperdrive and level transitions, mission flow, win/lose and restart; first-person gameplay on the planets (character, blade combat, crew AI, objectives); C++ architecture and interfaces; profiling, testing, packaging, critical bugs | Visual direction; ships, enemies, Dominion and prop *visuals*; space and planet environments, lighting, atmosphere, materials; all VFX (Niagara); HUD, UI/UX, menus, onboarding, ship-switch feedback, targeting clarity; all audio and music; character and ship animation polish; cinematics and the Director's Cut presentation; performance-minded art; trailer and store page |
| **Feature IDs** | `ENG-xx`, and since Nov 13 `ART-02, 03, 04, 05, 06, 07, 08, 13, 15, 16` (all of Environment, VFX and Asset Management) | `ART-01, 09, 10, 11, 12, 14` |

**A third lane since Nov 13: Grok Bot (Ryan's asset-generation agent).** It makes concepts, raw 3D models,
textures, animations and key art with fal, Hyper3D Rodin and PBRgen, as source files only, in
`ArtSource/EngAI/**` on `grok/assets-*` branches. Ryan's Lead sets its ranked list (`MESSAGEBOARD.md` R-027),
approves hero concepts and merges; engineering builders clean up, import and wire the results as fallbacks.
Nicholas keeps direction and sign-off over everything it makes.

**Engineering fallbacks (Nov 12-13, Ryan's instructions, AGENTS.md §11):** while Nick's side is stopped,
engineering may build presentation work in its own folders (models, terrain, sounds, effects, the flight
HUD). These are fallbacks: an asset of Nick's at the Presentation name replaces one with no code change.

**How you avoid stepping on each other:**
- **Ryan makes it work. Nick makes it look, sound and read great.** They meet only through the
  contracts in §3, especially the **presentation hooks** (§3.5): gameplay code reads Nick's data
  assets and fires events that Nick's UI, VFX and audio respond to.
- **Nick** doesn't independently change gameplay architecture, flight, AI, damage, progression or
  ship-switching logic. He coordinates interface needs with Ryan before modifying shared C++ classes,
  levels, assets or config.
- **Ryan** doesn't overwrite Nick's UI styling, audio or story words without coordination. (Environment, VFX and
  asset management moved to Ryan's side on Nov 13.) He provides stable gameplay hooks and data for the
  presentation work.

---

## 2. Folder & file ownership

```
Source/unrealGameJam/              🟦 RYAN: all C++ (Nick requests changes via §11 or a claim + chat)
├── TriadDrift/Core/               🔒 contracts (§3): frozen after C-02; changes agreed by both
├── TriadDrift/Space/              ships, squad, weapons, powers, abilities, enemies, waves, travel, Dominion
├── TriadDrift/Ground/             first-person character, energy blade, crew AI, objectives, hazards
├── TriadDrift/Flow/               GameInstance, game modes, saves, level travel, debug menu
└── Variant_Shooter/ Variant_Horror/ … template code: reference; Ryan owns
Config/*.ini                       🟦 RYAN: claim before editing (input, collision, maps & modes, rendering)
Content/TriadDrift/
├── Gameplay/                      🟦 RYAN: Blueprint children of C++ classes, AI (StateTree/BT), gameplay data
│                                         (DA_*_Stats, DT_Waves), input (IMC_*, IA_*), game modes
├── Maps/                          🔀 SHARED via World Partition One File Per Actor:
│                                         Ryan creates maps + places gameplay actors (spawns, triggers, objectives);
│                                         Ryan's side also places art, lighting and VFX since Nov 13; Nick places UI and audio. World Settings: claim first
├── Presentation/                  🟩 NICK: the presentation data-asset INSTANCES (DA_*_Visuals, DA_*_FX)
│                                         that Ryan's code reads (§3.5)
├── Art/                           🟩 NICK: Ships/ Enemies/ Dominion/ Props/ Weapons/ Characters/ (meshes,
│                                         materials, textures, skeletons, AnimBPs), Materials/ (masters
│                                         M_TD_Flat / M_TD_Textured), Pipelines/ (import pipeline asset)
├── Environment/                   🟦 RYAN since Nov 13 (was Nick): skybox, nebulae, planet visuals, asteroid/crystal sets, terrain
│                                         materials, foliage, lighting & post-process presets
├── VFX/                           🟦 RYAN since Nov 13 (was Nick): all Niagara systems, camera-shake classes, post-process materials
├── UI/                            🟩 NICK: all widgets (WBP_*), fonts, icons, DT_Tips
├── Audio/                         🟩 NICK: SFX, MetaSounds, music, sound classes/mixes, VO, DT_Barks
├── Cinematics/                    🟩 NICK: Level Sequences, director-cam rigs, slate cards
└── Developers/<name>/             personal scratch; never referenced by shipped content
ArtSource/                         🟩 NICK: Blender sources, AI originals, exports (blender-* skills)
    <Category>/<Asset>/{brief.md, refs/, source/, <Asset>_vNN.blend, review/, export/}
Template Content (Characters, FirstPerson, LevelPrototyping, Variant_*, Weapons): reference. Claim before editing.
```

**Environment, VFX and Asset Management are Ryan's since Nov 13 (afternoon).** `Content/TriadDrift/{Environment, VFX}/`,
`ArtSource/{Environment, VFX, Assets, Space, Characters, Weapons}/` and the supplied-model intake branches are now under
Ryan's ownership: his side may use, import or replace anything there. Files Nick's agents made stay in history and are
listed in `docs/eng/ART-HISTORY.md` before any is replaced. Nick's folders are now: `Content/TriadDrift/{UI, Audio,
Cinematics}/`, `Plugins/TriadDriftHUD/`, `ArtSource/{UI, Audio}/`, `Story/`, and his entries under `Presentation/`.

**Engineering's own art folders (fallbacks, Nov 12-13):**
- `Content/TriadDrift/Gameplay/{Characters, Props, Materials, Vegetation, Audio, VFX}/**`: models, materials,
  sounds, Niagara effects and the fallback `DA_*_Visuals` / `DA_*_FX` / `DA_UI_Style` (in `Gameplay/Characters/**`).
  All always cooked.
- `ArtSource/Eng{Ships, Props, Characters, Audio, VFX}/**`: their sources, scripts and review sheets.
- `ArtSource/EngAI/**`: Grok Bot's generated sources (concepts, raw models, texture sets), never edited in place.
- Replaced versions are kept and indexed in `docs/eng/ART-HISTORY.md`.

**Rules**
- Never edit the other person's folders. Ask through your humans, or add a line to §11.
- `Source/.../Core/` is frozen after C-02. **Add new things; never rename or remove existing enums, fields or
  interface functions.** Who approves what (Nov 12, AGENTS.md §1 "Delegation"): an *addition* to Core is an
  engineering decision, made by Ryan's Lead under Ryan's delegation and announced on `MESSAGEBOARD.md`; a
  *rename or removal* of anything presentation reads needs both sides' agreement on the board first. Art,
  visual and audio direction and final quality sign-off are always Nicholas's.
- Maps: claim the **World Settings** and any shared persistent actors before editing them. With One
  File Per Actor, two people can edit different actors in the same level safely.
- Collision channels (C-01): `Ship`, `Projectile`, `Character`, `Interactable`, `Portal`.
- Placeholders: Ryan builds with template or primitive meshes. Nick's data assets swap in the final
  art with no gameplay edits.

---

## 3. Shared contracts

**C++ first.** Contracts are C++ in `Source/unrealGameJam/TriadDrift/Core/` (all `BlueprintType`).
Gameplay classes are C++ (`ATD*` / `UTD*`) with thin Blueprint children (`BP_*`) for tuning and
asset references. Ryan writes them (C-02). Nick reviews §3.5 before it is frozen.

### 3.1 Enums & structs
| Type | Values / fields (append new enum values at the **end** only) |
|---|---|
| `ETDShipId` | `Rift`, `Lancer`, `Bastion` |
| `ETDTeam` | `Player`, `Enemy`, `Neutral` |
| `ETDAbilityId` | `None`, `GravityWell`, `Cloak`, `EMPPulse` |
| `ETDDestination` | `MainMenu`, `Galaxy`, `Verdance`, `Cinder`, `DominionInterior` (stretch), `Victory` |
| `ETDDamageKind` | `Energy`, `Kinetic`, `Explosive`, `Melee`, `Ram`, `Environmental` |
| `ETDGamePhase` | `Intro`, `Waves1to2`, `Verdance`, `Waves3to4`, `Cinder`, `Wave5`, `DominionAssault`, `DominionInterior`, `Escape`, `Victory` |
| `FTDProgress` | `Phase`, `WavesCleared`, `CompletedObjectives (Set<Name>)`, `UnlockedAbilities (Map<ETDAbilityId,ETDShipId>)`, `bDominionRevealed`, `bDominionDefensesDown`, `ReturnPoint`, `LastShip`, `RunTimeSeconds`, `Stats (FTDRunStats)`, `Takes (Map<Name,int>)` (Director's Cut) |
| `FTDRunStats` | `Kills`, `ShipSwitches`, `PortalsUsed`, `ShotsDeflected`, `Deaths`, `MissilesReturned`, `HighlightMoments` |
| `FTDShipHUD` | `Ship`, `Health01`, `Shield01`, `Boost01`, `Heat01`, `SecondaryAmmo`, `PowerCooldown01`, `Power2Cooldown01`, `AbilityCooldown01`, `Ability`, `bPlayerControlled`, `bAlive`, `RespawnSeconds` |

### 3.2 Interfaces (all implemented by Ryan)
| Interface | Implemented by | Used by | Functions |
|---|---|---|---|
| `ITDGameFlow` | `UTDGameInstance` | both | `TravelTo(Dest, ReturnPoint)` · `GetProgress()` · `SetPhase()` · `ReportWaveCleared()` · `CompleteObjective(Id, Reward, ForShip)` · `SetDominionState(Revealed, DefensesDown)` · `ReportPlayerDefeated()` · `AddStat()` · `RequestBark(BarkId, Speaker)` · `ShowTip(TipId)` · `SaveCheckpoint()` |
| `ITDTeam` | everything damageable | weapons, AI | `GetTeam()` |
| `ITDHUDData` | `ATDSquadManager` (space), `ATDFirstPersonCharacter` (ground) | Nick's HUD widgets | `GetSquadHUD()` · `GetCurrentTarget()` · `GetLockedTargets()` · `GetWaveInfo()` · `GetHyperdriveCharge01()` · `GetObjectiveMarkers()` · `GetSpeed01()` · `IsOutOfBounds()` · `GetAltitude()` · ground: `GetHealth01()`, `GetBlockStamina01()`, `GetDashCharges()`, `GetInteractPrompt()` |
| `ITDDeflectable` | `ATDProjectile` | blade, portals, dome | `Deflect(NewInstigator, NewDirection, NewTeam, SpeedMultiplier)` |
| `ITDInteractable` | interactables | first-person character | `GetPrompt()` · `CanInteract()` · `Interact()` · `GetHoldTime()` |

### 3.3 Damage
Use Unreal's built-in `ApplyDamage` / `TakeDamage`, with a `UDamageType` subclass per `ETDDamageKind`.
Always check `GetTeam()` first: friendly fire is **off**.

### 3.4 Gameplay events (Ryan fires; Nick's UI/VFX/audio listen)
Multicast delegates on the owning actors, re-broadcast by `UTDGameInstance` for widgets:
`OnWaveStarted(Index, Intensity01)` · `OnWaveCleared(Index)` · `OnShipSwitched(New, Old)` ·
`OnShipDowned(Ship)` / `OnShipRespawned(Ship)` · `OnSquadWiped()` · `OnPowerUsed(Ship, PowerName)` ·
`OnAbilityUnlocked(Ability, Ship)` · `OnPhaseChanged(Phase)` · `OnObjectiveUpdated(Id, Step, Text)` ·
`OnHitConfirmed(Target, bKill)` · `OnHyperdriveState(State)` · `OnPortalState(Portal, State)` ·
`OnTipRequested(TipId)` · `OnBarkRequested(BarkId, Speaker)` ·
`OnHighlightMoment(Kind, Actor, Importance)` (Director's Cut, C-03) · `OnTakeStarted(SceneId, TakeNumber)`.

### 3.5 Presentation hooks: how Nick's art plugs in without shared-file edits
- Ryan's C++ defines **presentation data-asset classes** (`UPrimaryDataAsset`, soft references only):
  - `UTDShipVisuals`: mesh/skeletal mesh, materials, squad colour, HUD icon, engine-trail / boost /
    drift VFX, switch-in / switch-out VFX + SFX, power wind-up / active / end VFX + SFX, damage-state
    VFX, explosion tier, engine sound, camera shakes.
  - `UTDWeaponFX`: muzzle, tracer, impact on hull / shield / rock VFX, fire + impact sounds, shake,
    hit-marker style.
  - `UTDEnemyVisuals`, `UTDAbilityFX`, `UTDTravelFX` (hyperdrive spool / tunnel / drop-out, portal
    open / idle / transit / close, atmospheric entry, landing dust), `UTDBladeFX`, `UTDUIStyle`.
- **Gameplay actors hold one data-asset reference each and spawn from it at the event.** An empty
  slot does nothing (no crash), so Ryan's code runs before any art exists.
- **Nick owns the instances** in `Content/TriadDrift/Presentation/` and everything they point to.
  Ryan never edits them, and Nick never edits gameplay Blueprints or C++. Need a new slot? Nick
  requests it (§11); Ryan adds it to the class.
- **Animation state:** gameplay exposes read-only state on the pawns (`Speed01`, `bBoosting`,
  `bDrifting`, `PowerWindup01`, `bGearDown`, `bCanopyOpen`, `Damage01`, `BladeState`) for Nick's
  AnimBPs and timelines.
- **Cameras:** Ryan owns control-feel cameras (chase cam, FP cam). Nick owns cinematic cameras and
  sequences, and the camera-shake *assets*.

### 3.6 Stubs (nobody blocks)
- **Ryan:** placeholder data-asset instances with empty slots (in `Gameplay/Debug/`), plus a
  `BP_TD_StubGameFlow` until the real flow lands.
- **Nick:** `WBP_` widgets built against a `BP_TD_FakeHUDData` (UI folder; implements `ITDHUDData`
  with sliders). VFX and sounds are previewed in Nick's own test map
  (`Developers/Nick/L_Test_Presentation`).

---

## 4. Shared setup (`C-xx`)

### C-01 · Project bootstrap · 🟦 Ryan · `✅`
- [x] Create the UE project (`unrealGameJam`, First Person C++ template, UE 5.9.0) and record the engine version in AGENTS.md.
- [x] `.gitignore` and Git LFS (`.uasset .umap .blend` + images, audio, meshes). Not `lockable`; we coordinate through `CLAIMS.md`.
- [ ] Create the `Content/TriadDrift/...` and `Source/unrealGameJam/TriadDrift/...` trees from §2.
- [ ] Collision channels (`Ship`, `Projectile`, `Character`, `Interactable`, `Portal`) and Maps & Modes.
- [ ] Maps (World Partition + One File Per Actor): `L_MainMenu`, `L_Galaxy`, `L_Planet_Verdance`, `L_Planet_Cinder`, `L_Transition`, plus stretch `L_Dominion_Interior`. Also `L_Test_Space` (Ryan) and `L_Test_Presentation` (Nick).
- [ ] Create `CLAIMS.md`, `CREDITS.md` and `DEVLOG.md`, then push.

### C-02 · Contracts · 🟦 Ryan writes, 🟩 Nick reviews §3.5 · `✅`
- [x] Write everything in §3 as C++ (`Core/`) and compile. Create the presentation data-asset classes. (No placeholder instances: every slot is optional and null-safe, and Nick's side creates the instances.)
- [x] Nick checks that the presentation slots cover what he needs for ships, weapons, enemies, travel, blade and UI. (Done over `MESSAGEBOARD.md` R-003 to R-005 and N-001; further slots are added on request.)
- [x] Push. **`Core/` is now frozen.**

### C-03 · Theme: "Director's Cut" · both · `🟢` 🎬
**Story:** scenes and takes are the paradox in the player's hands: a future trying to redo one event, a player retaking a scene. From Cinder onward a take number may glitch or echo (`storyline.md` §7).
Theme fit is a judging category, so it must be **visible in gameplay**, not just the title.

**Decision (Nov 12): we build #1, #2 and #3.** All three are core. They're cheap,
presentation-heavy, and build on the switching mechanic. #4 and #5 are not planned.

1. **Switching is a cut** *(BUILD)*. A ship switch plays as a director cutting between cameras:
   a hard camera cut with a 1-frame flash, a quick letterbox snap and the pilot's name card
   ("WREN · RIFT"). It stays instant, with no lost control. 🟦 switch event + timing · 🟩 cut
   transition, card, sound.
   **(Nov 12)** While Nick's space HUD is up it draws the name card ("CUT TO / LANCER"); the fallback
   card hides and the fallback overlay sits under his HUD (N-007).
2. **Director cam "money shots"** *(BUILD)*. Big moments fire `OnHighlightMoment`: ram kills,
   portal missile returns, multi-kills, generator and reactor kills. Each triggers a 0.5–1 s
   slow-motion beat from a cinematic angle, rate-limited. 🟦 events + time dilation · 🟩 camera rigs,
   framing, grade.
   **(Nov 12, as built)** Also produced: carrier kills, Switch Strike kills and the on-foot perfect
   deflect (0.15 s, deliberately a parry flash rather than a beat). Limits: time dilation 0.4 at the
   lowest importance down to 0.2 at the highest; 3 s global cooldown and 6 s per kind; importance 0.8
   or higher bypasses the cooldowns; multi-kills are capped at importance 0.7.
3. **Scenes and takes, not lives** *(BUILD)*.
   - Each wave or objective is a "scene" opened by a slate card ("SCENE 3 · CINDER · TAKE 1").
   - Dying calls "CUT!" and the scene restarts as TAKE 2.
   - The victory screen shows the takes needed per scene.

   🟦 checkpoint/restart per scene · 🟩 slate cards, "CUT!" presentation.
4. ~~**Your Director's Cut**~~ *(not planned)*. The victory screen plays an auto-edited highlight reel built from
   the recorded highlight moments. 🟦 event log with positions and times · 🟩 camera-cut playback.
5. ~~**Director's commentary**~~ *(not planned)*. A "Director" voice (the squad commander) opens scenes and
   reacts to highlights; subtitles are styled as script pages. 🟩 VO + subtitles, driven by
   `OnBarkRequested`.

**Owners:**
- **#1:** ENG-07 + ART-09/12.
- **#2:** `OnHighlightMoment` + time dilation (ENG-07/09/10/18/22) + ART-12.
- **#3:** ENG-14/19 checkpoints and takes + ART-09/12 slate cards and "CUT!".

---

## 5. 🟦 Ryan: Gameplay & Engineering

**Start here (role brief, Day 1 focus):** a playable prototype with flight, shooting, instant ship
switching and baseline wingman AI. Expose the integration points (§3.4–3.5) so Nick's visual pass
can start.

**Deliverables:**
- a continuously playable build with flight, shooting and reliable three-ship switching
- two autonomous wingmen that keep engaging after a switch
- a complete, balanced combat loop with enemy waves and objectives
- a tested, packaged submission build

### Space core

#### ENG-01 · Space flight model · `🟢`
Arcade 6DOF-lite flight: fast, readable, with a signature **drift**. Gamepad and keyboard + mouse are
both first-class.

| Control | Behaviour | Start values |
|---|---|---|
| Throttle | Analog forward speed, min → cruise | min 1500, cruise 6000 cm/s (per-ship multipliers in ENG-04) |
| Aim (mouse / right stick) | The ship turns toward an aim reticle (mouse = virtual joystick with a deadzone) | 110°/s pitch and yaw |
| Roll | Manual; auto-banks into turns | 180°/s |
| Strafe (optional) | Lateral and vertical thrusters | 4000 cm/s |
| **Boost** | Hold: big speed increase that drains the meter | +80% speed, 3 s meter, regen after 1.2 s |
| **Drift** | Hold: velocity keeps its direction while the nose rotates freely. Release: snap to the nose plus a burst | 1.5× turn rate; +20% burst |
| Brake | Hard deceleration to min speed | |

- `ATDShip` pawn with custom velocity integration; values in `UTDShipData` (`DA_ShipStats_*`).
- Chase camera (rotation lag 8, location lag 12, FOV 90 → 110 with speed, 30% roll follow,
  look-ahead), with an optional cockpit camera.
- Collisions: glancing hits scrape and bounce (damage scales with impact angle); head-on hits deal
  heavy damage.
- Soft spherical play-space boundary: warn, then gently steer back (`IsOutOfBounds`).
- Feel hooks for Nick: boost, drift and speed state exposed; camera-shake slots in `UTDShipVisuals`.

**Done when:** flying through an asteroid field is fun on its own, and drifting around a rock while
shooting backwards feels great.

#### ENG-02 · Space input map · `🟢`
`IMC_Space` (plus `IMC_Atmosphere`, which adds Land and removes Hyperdrive):

| Action | KB/M | Gamepad |
|---|---|---|
| Throttle | W / S | RT / LT |
| Aim | Mouse | Right stick |
| Roll | A / D | Left stick X |
| Fire primary / secondary | LMB / RMB | RB / LB |
| Boost / Drift | Shift / C | L3 / B (hold) |
| Signature power 1 / 2 (hold, Bastion) | Q / hold Q | X / hold X |
| Unlocked ability | E | Y |
| Switch next / prev / direct | Tab / Shift+Tab / 1-2-3 | D-pad right / left / hold up = radial |
| Target cycle | T / MMB | R3 |
| Hyperdrive (hold) | H | D-pad down (hold) |
| Squad command / Land / Interact | F | A |
| Pause / Galaxy map | Esc / M | Start / Select |
| Dodge **(Nov 12)** | Left Alt, or double-tap A / D | Left stick double flick |
| Brake **(Nov 12)** | Space | LT fully pulled |
| Strafe **(Nov 12)** | Arrow keys | Left stick Y |
| Cockpit camera **(Nov 12)** | V | R3 (hold) |

**(Nov 12)** The galaxy map key is hidden until its screen exists (ART-09).

#### ENG-03 · Health, shields, death and respawn · `🟢`
- **Shield:** absorbs damage first; regenerates after 3 s without damage at 25%/s. **(Nov 12)** Set per
  ship in `FTDShipSpec` (`ShieldRegenDelay`, `ShieldRegenFractionPerSecond`); those are the defaults.
- **Hull:** doesn't regenerate in combat, but repairs at 8% per second in the breather between waves.
- **Death:** a 0.6 s tumble, then an explosion (`UTDShipVisuals` tier), then **auto-switch** to the
  healthiest living ship. A downed ship warps back in after 12 s. There are 2 s of invulnerability
  after a respawn or an automatic switch; a manual switch gives 0.35 s, at most once every 6 s per
  ship, so switching cannot be spammed for permanent invulnerability.
- **Squad wipe:** if all three ships are down at once, fire `OnSquadWiped`, then
  `ReportPlayerDefeated()` (Director's Cut: "CUT!", then the next take).

- **(Nov 12)** A manual switch made during your own ship's death tumble gets the same 2 s protection as
  the automatic switch. Between waves only the squad manager repairs (8% of max hull per second). After a
  retake or an in-place respawn every ship, including the one the player was flying, is flown by its AI
  again as soon as the player switches away.

#### ENG-04 · The three ships (gameplay stats) · `🟢`
| | **Rift** (Trickster) | **Lancer** (Interceptor) | **Bastion** (Heavy) |
|---|---|---|---|
| Hull / Shield | 100 / 100 | 70 / 80 | 180 / 160 |
| Cruise / Boost | 6000 / 11000 | 7500 / 15000 | 4800 / 8500 |
| Turn rate | 120°/s | 100°/s | 75°/s |
| Boost meter | 3 s | 4 s, fast regen | 2.5 s |
| Primary / Secondary | Phase Pulse Lasers / Splitter Bolt | Needle Cannons / Spike Rockets | Breaker Blaster / Flak Burst |
| Signature | Portal Blaster | Afterburner Ram | Missile Swarm + Shield Dome |
| Unlockable | Cloak | EMP Pulse | Gravity Well |

**(Nov 12, as built)** Stats are `FTDShipSpec` in `UTDShipLibrary` (C++), overridable per ship on
`ATDSquadManager` (`ShipSpecOverrides`); no `BP_Ship_*` children or `DA_ShipStats_*` assets are needed.
Each ship's look comes from its `DA_<Ship>_Visuals` (`DA_Rift_Visuals`, `DA_Lancer_Visuals`,
`DA_Bastion_Visuals`; Nick, ART-03; exact names in `docs/visual/Presentation-Asset-Paths.md`).

#### ENG-05 · Ship weapons · `🟢`
| Weapon | Ship | Type | Damage | Rate | Speed | Range | Resource | Notes |
|---|---|---|---|---|---|---|---|---|
| Phase Pulse Lasers | Rift | Twin alternating bolts | 8 each | 8/s | 40 000 | 40 000 | Heat: 4 s to overheat, 1.5 s lockout | 3° aim assist |
| Splitter Bolt | Rift | Charge shot (0.8 s) | 40 + 5×12 shards | — | 25 000 | 35 000 | 6 ammo, +1 per 4 s | Splits on impact; great vs swarms |
| Needle Cannons | Lancer | Rapid kinetic | 4 | 20/s | 55 000 | 30 000 | Heat: 5 s | 1.2° spread |
| Spike Rockets | Lancer | Dumb-fire salvo of 4 | 25 each, 300 cm splash | 1 per 1.5 s | 30 000 | 40 000 | 8 salvos, +1 per 5 s | |
| Breaker Blaster | Bastion | Heavy plasma | 30, 250 cm splash | 2.5/s | 28 000 | 35 000 | Heat: 6 s | Knocks back fighters |
| Flak Burst | Bastion | Proximity flak | 15 splash over 800 cm, 500 cm proximity fuse | 1 per 1.2 s | 22 000 | 20 000 | 10 shells, +1 per 3 s | **Shoots down missiles** |

- `UTDWeaponComponent` + `UTDWeaponLibrary` (the `FTDWeaponSpec` table, C++; looks in `UTDWeaponFX`); all projectiles derive from `ATDProjectile` (C++,
  implements `ITDDeflectable`).
- Lead indicator data for the HUD; `OnHitConfirmed` for hit and kill markers; 40 ms hit-stop on
  kills.
- Visuals and audio come from `UTDWeaponFX` (Nick).

#### ENG-06 · Targeting & lock-on · `🟢`
- Soft target = the enemy closest to screen centre within 25°, cycled by distance.
- Bastion missile locks: targets inside the 25° cone are painted automatically, up to 6, at 0.25 s
  each. A lock is dropped when its target dies, becomes invalid (cloaked, wrong team), goes beyond the
  swarm's range, or stays outside the cone for more than 1.5 s; when all six are used, a new
  candidate replaces the stalest lock.
- Off-screen arrows, incoming-missile and objective direction data → `ITDHUDData`. Objective
  markers are the live actors carrying the actor tag `TD.Objective.Marker`. The game flow owns the tag:
  the destination planet's atmosphere, live shield generators, the reactor, on-foot waypoints and the
  board-ship point; the landing pad while in atmosphere (flight). Nothing is tagged during a wave.
- Incoming-missile warning state (Nick supplies the sound and UI).
- More HUD data on the same source, all optional zero-input getters (added Nov 12): `GetObjectiveText`
  (current objective line), `GetBoundaryWarningText` (edge of the play area), `GetInteractPrompt` (landing
  prompt, and for 1.5 s why a switch was refused), `GetSquadMarkers` (where each squad ship is and whether
  the wingman is engaging: off-screen wing arrows), `GetDamageIndicators` (direction of recent hits on the
  player: "who is shooting me").
- Dodge (player ship only, added Nov 12): a 0.25 s sideways burst at 100 m/s with a 1.5 s cooldown and no
  invulnerability. Left Alt (toward the held roll or steering side), double-tap A or D, or a double flick of
  the left stick. Refused during the ram, landing, atmosphere entry, hyperdrive and director slow motion.
  Measured: weaving with a timed dodge takes about 40-50% of straight-line damage. Strafe is 40 m/s.
- Pause: Escape / gamepad Start, in space and on foot (`OnPauseChanged` on the player controller;
  `UTDUIStyle.PauseWidgetClass` for the menu). Refused during level travel, a pending retake and victory.

#### ENG-07 · Squad manager & instant switching (PROTECT) · `🟢`
- `ATDSquadManager` spawns, tracks and respawns all three ships, and owns who the player controls.
  It implements `ITDHUDData`.
- **Switching:** next, previous or direct; possesses the target instantly on the same frame; the ship
  you left hands control to its AI and keeps its velocity; 0.4 s cooldown; downed ships can't be
  selected.
- **Switch Strike:** switching within 1.5 s of using a power gives +25% damage for 4 s.
- A power started on one ship finishes after you switch away (portal stays open, dome stays up).
- Fires `OnShipSwitched`, which drives Nick's "cut" presentation (C-03 #1).

#### ENG-08 · Wingman AI (PROTECT) · `🟢`
- States: Formation → Engage → Evade → Regroup → Downed.
  - Formation: a loose V at 1500–2500 cm offsets; boosts to catch up beyond 8000 cm. **(Nov 12)** The V is
    swept forward (both wingmen ahead and outboard of the player, at different depths) so the chase
    camera shows it.
  - Engage: attacks whoever threatens the player first, then the nearest, then the lowest health;
    primary and secondary weapons only.
  - Evade: kicks in on shield break or an incoming missile.
- **Never uses signature powers or abilities.** Those fire only when the player is in that cockpit.
- Follows the player through portals and gates and jumps on hyperdrive. `IA_SquadCommand` = "focus my
  target" for 8 s.
- Avoids rocks and teammates; moves toward an active Bastion dome when hurt; 2–4° aim error.

- **(Nov 12)** Target choice: one wingman may defend against an off-screen threat at a time; the other
  prefers targets inside the player's view (within 80 m bands of its best target). Wingmen keep clear of
  the aim line and of an active director shot. In a furball a wingman shares the frame with an enemy well
  under half the time, so the wing is made readable through the HUD (`GetSquadMarkers`: named arrows in
  squad colours) and short position calls (`Bark_SquadEngage...`), not by formation flying.

#### ENG-09 · Rift: Portal Blaster · `🟢` ⚠️ highest risk
- **Placement:** Fire 1 opens portal A where it hits or at max range (12 000 cm); Fire 2 opens B; a
  third shot recycles the oldest. Open for 12 s, then an 8 s cooldown.
- **Ships:** pass through keeping their speed, take the exit's facing, and get 0.3 s of
  invulnerability.
- **Enemy projectiles** that enter are `Deflect()`ed to the Player team and **re-home on whoever
  fired them** (counts as `MissilesReturned`, and fires `OnHighlightMoment`).
- **Enemy fighters** that pass through lose tracking for 2 s.
- **Fallback if it's still unstable late:** "Blink" (teleport to the impact point) plus a 3 s
  missile-reflect field.
- Visuals: `UTDTravelFX` portal slots (Nick).

#### ENG-10 · Lancer: Afterburner Ram · `🟢`
- 0.3 s wind-up, then a 1.2 s dash at 3× boost speed. Lancer is invulnerable and steers at 40%.
- Deals **300 to whatever it touches**: passes through fighters, **breaks gunship shields** and
  carrier bay doors.
- Hitting something big stops the dash with a 150-damage shockwave over 1500 cm.
- 7 s cooldown. Exposes `PowerWindup01` for the wing-fold animation (Nick).

#### ENG-11 · Bastion: Missile Swarm & Shield Dome · `🟢`
- **Missile Swarm (tap):** 12 micro-missiles split across painted locks (or the soft target),
  18 damage each with a 200 cm splash; 9 s cooldown.
- **Shield Dome (hold 0.5 s):** a 3000 cm radius for 6 s with a 16 s cooldown.
  - Squad ships inside take no damage and regenerate shields twice as fast.
  - Enemy shots are destroyed on the dome, and missiles are deflected back.
  - The dome follows Bastion slowly.

#### ENG-12 · Unlockable abilities (CORE: powers aren't optional) · `🟢`
One ability slot per ship (`IA_Ability`), filled from `GetProgress().UnlockedAbilities` on spawn
and on `OnAbilityUnlocked`.

| Ability | Ship | Unlocked by | Behaviour | Cooldown |
|---|---|---|---|---|
| **Gravity Well** | Bastion | Verdance Obj 1 | A slow orb that becomes a 5000 cm vortex for 5 s, pulling enemies and their projectiles in. Enemies inside take 10 dps and can't fire; it ends with a 120-damage implosion | 18 s |
| **Cloak** | Rift | Verdance Obj 2 | 6 s untargetable (how it looks is Nick's, `DA_Ability_Cloak_FX`): AI loses lock and missiles lose tracking. The first shot out does **3× damage**; firing or using another power breaks it | 15 s |
| **EMP Pulse** | Lancer | Cinder Obj 3 | A 6000 cm pulse: stuns enemies for 3 s, drops gunship shields for 5 s, fizzles missiles. Stunned enemies take +25% damage | 14 s |

`UTDAbilityComponent` (the slot) installs one `UTDPowerComponent` child per ability (`UTDGravityWellPower`, `UTDCloakPower`, `UTDEMPPower`). Visuals: `UTDAbilityFX` (Nick).

**Powers and ship switching (decided Nov 12):** a power or ability that is running when the player
switches away (cloak, shield dome, an open portal pair, a gravity well) keeps running on that ship
and ends on its own timer. The new ship's presses never reach the old ship's powers, and an
AI-flown ship never starts one. Two exceptions end at the switch (also on an auto-switch after a
death): the ram (wind-up cancelled, dash ended, no invulnerability left behind) and a portal blaster
with only one portal placed (the lone portal closes).

#### ENG-13 · Enemy ships & AI · `🟢`
| Enemy | Role | HP / Shield | Speed | Weapons | Behaviour | Counter |
|---|---|---|---|---|---|---|
| Shard Fighter | Swarm | 30 / 0 | 7000 | Pulse guns (5, 4/s) | Packs of 4–8, strafing runs | Needles, Splitter, Flak |
| Hook Interceptor | Hunter | 50 / 20 | 9000 | Pulse guns + homing missile (25) every 8 s | Chases the player's ship | Portal missile return, drift |
| Wall Gunship | Tank | 400 / frontal shield 300 (recovers 15 s after it is broken) | 3000 | Heavy bolt volley (20) every 4.5 s + missile salvo (3×20) every 12 s | Faces its target; shield blocks frontal fire | Flank, portal behind it, Ram, EMP |
| Maul Bomber | Area denial | 150 / 50 | 4000 | Plasma mines (60, 800 cm) | Lays a trail of 4 mines every 5 s | Flak, Gravity Well |
| Hive Carrier | Spawner / mini-boss | 1500 hull + 3 bays × 300 | 1500 | 4 point-defense turrets (6; bursts of 1.0 s, then 2.0 s quiet) | Launches 3 fighters per bay every 12 s, at most 3 launches per bay and 8 of its fighters alive at once | Kill bays (Ram breaks doors), Missile Swarm |

- `ATDEnemyShip` + AI: approach, attack run, break off, re-approach; fighters fly in formation.
  Attack runs on the player's ship cross in front of its nose, so the fight stays on screen; a group
  that falls behind catches up, so a wave can always be finished.
- **Balance (Nov 12):** the numbers above were tuned with the headless balance run after wave 5 proved
  unwinnable at the first values; the reasoning and the per-wave tables are in `docs/eng/balance.md`.
- Every big attack has a **telegraph** state for Nick's VFX and audio.
- Visuals: `UTDEnemyVisuals` (Nick).

- **(Nov 12)** Enemy aim and AI choices use a deterministic seed per ship (spawn order in the world), so the
  same fight plays the same way. Fighters open fire at a range that grows with closing speed and never ram on
  a head-on pass. Only the interceptor's missile has a point-blank rule (no launch inside 60 m or with too
  little time to dodge); gunship bolts fire down to 15 m and its salvo from 40 m. Enemy ships do not use the
  planet gates. Placeholder ships glow red-orange with white eyes and are drawn larger than their collision.

#### ENG-14 · Wave director · `🟢`
- `ATDWaveDirector` + `DT_Waves`. Waves spawn behind or flanking the player, with a 6 s breather
  between them.
- Reads `GetProgress()` on load and resumes the right phase.
- Fires `OnWaveStarted` / `OnWaveCleared` and `OnTakeStarted` (C-03 #3).

| Wave | Composition | Intensity | Then |
|---|---|---|---|
| 1 | 2 × 4 fighters | 0.2 | |
| 2 | 3 × 5 fighters + 2 interceptors | 0.35 | Verdance marker appears |
| 3 | 2 gunships + 3 × 5 fighters | 0.55 | |
| 4 | 2 gunships, 2 bombers, 4 interceptors, fighters | 0.7 | Cinder marker appears |
| 5 | "Dominion's Guard": 2 carriers + escorts + bombers | 0.9 | Dominion reveal (ENG-18) |

Tutorial beats fire `ShowTip`: switching (wave 1), each signature power (first time in each ship),
drift, hyperdrive.

#### ENG-15 · Hyperdrive · `🟢`
- Hold to spool for 1.2 s; can't spool within 2 s of taking damage, and a hit cancels it.
- Jump at 80 000 cm/s with 10% steering. Auto-drops out near markers and the Dominion. Wingmen jump
  too and arrive in formation.
- Exposes state for `OnHyperdriveState` (spool, tunnel and drop-out VFX and audio are Nick's).

- **Difficulty:** three presets, Story / Standard / Ace (`UTDDifficultySettings`), scale enemy hull,
  damage, aim error and wave size. Target on Standard: the five waves take 8-10 minutes in total.
- **Hyperdrive lock:** the hyperdrive is locked while a wave is active and during the Dominion
  assault, and free in the breather between waves and in the planet phases.

#### ENG-16 · Portal gates (galaxy shortcuts) · `🟢`
Three permanent `ATDPortalGate` pairs: combat zone ↔ Verdance orbit, combat zone ↔ Cinder orbit, and
a flank pair used in waves 3–5. They reuse ENG-09's transit logic, and wingmen follow. Shots pass
through a gate unchanged: only Rift's own portals return enemy missiles.

#### ENG-17 · Galaxy level: gameplay layout · `🟢`
- **Zones:** the combat zone (asteroids, station ruins as cover), Verdance and Cinder orbits, and
  the Dominion approach.
- **Gameplay actors:** `ATDPlanetAtmosphere` triggers, spawn zones, portal gates and return points
  (`Verdance_Orbit`, `Cinder_Orbit`, `Dominion_Hangar_Exit` for the stretch).
- **Shared map:** dressing, lighting and VFX on the same map are Ryan's side's too since Nov 13 (One File Per Actor).

#### ENG-18 · Dominion assault & space finale (CORE) · `🟢`
**Story (acts 1, 4):** the Dominion is visible from wave 1 behind its **distortion field**; the four shield generators are that field. With boarding on, defences down opens the docking bay (ENG-29) instead of exposing the reactor to space; `td.Finale.Boarding 0` keeps the space reactor ending. Transmissions break through as each generator falls.
- **Arrival:** when `bDominionRevealed` is set after wave 5, the Dominion warps in (Nick's reveal
  sequence plays).
- **Assault:**
  - 4 `ATDShieldGenerator`s (800 HP each, guarded by turrets), 10 `ATDDominionTurret`s on the hull
    and 4 in the reactor trench (telegraphed charge shots); fighters keep launching.
  - Each generator kill drops part of the shield and fires `OnHighlightMoment`.
  - All four down sets `SetDominionState(DefensesDown)` and **exposes the reactor**.
- **Core ending:** fly the exposed reactor trench and destroy the reactor (3 phases of 1200 HP with
  escalating turret fire and launches). When it dies the squad is protected and every remaining
  enemy is destroyed; victory is a phase change on the galaxy map after Nick's breakup sequence.
- **Stretch ending:** the docking bay opens instead and leads to ENG-29.
- **(Nov 12) Retakes and saves:** the assault is one scene ("Dominion"). After a squad wipe the squad restarts on
  the approach (at the trench once the defences are down); destroyed generators stay destroyed, reactor phases
  already passed stay passed, and the current phase restarts. The hyperdrive is locked only inside the
  Dominion's gravity well, so the first approach can jump and drops out at the well. Victory cancels a pending
  retake, and a won save does not continue into the fight.

### Game loop & progression

#### ENG-19 · Game instance, progression, saves & travel · `🟢`
- `UTDGameInstance` implements `ITDGameFlow` and holds `FTDProgress`.
- **Travel:** `TravelTo` fades to `L_Transition` (Nick's loading screen widget), then the target
  level.
- **Checkpoints:** taken on level entry, wave clear and objective complete. The **scene/take restart**
  (C-03 #3) reuses them. The save slot supports Continue.
- **Defeat:** "Retry from checkpoint" or "Quit". Run stats feed the victory screen.
- **Debug menu** (F10, dev builds only): jump to any phase, unlock abilities, god mode, kill all,
  skip wave.

#### ENG-20 · Atmosphere flight, landing & take-off (gameplay) · `🟢`
See §7.
- **Atmosphere flight mode:** gravity, speed capped at 60% of space cruise, no hyperdrive,
  altitude data.
- **Zone and pad actors:** `ATDAtmosphereFlightZone` and `ATDLandingPad` (beacon; "Hold F to land").
  - Within 120 m of the pad at or below 60% of the atmosphere speed cap, F lands directly
    (aligning takes 1.5 to 4 s by distance, up to 6 s when the path arcs over a prop).
  - Within 400 m, holding F is an approach assist: it turns the nose onto the pad (150°/s), flies in
    at up to 60 m/s, brakes at 30 m/s² but never below 15 m/s, and hands over to the landing 25 m
    from the hover point. Releasing F, boost, hyperdrive, a switch, death or a retake ends it.
    From 400 m the landing starts about 7 s after F and touchdown is at about 12 s.
  - A ship that cannot assist and is too fast within 400 m gets "SLOW DOWN TO LAND".
  - Near the ground the ship levels out by itself, so a hands-off arrival never crashes.
  - The player pad carries the objective marker while in atmosphere and not yet landed.
- **Landing and take-off:** auto-land (gear and canopy state exposed), wingmen land beside you, the
  exit hands off to first person, and boarding reverses it.

#### ENG-21 · First-person character (planets: CORE) · `🟢`
Extends the template's first-person character.

| Move | Input | Values |
|---|---|---|
| Run / sprint | WASD / Shift | 650 / 950 cm/s |
| Jump / double jump | Space | Z 650 / 500 |
| **Dash** | Ctrl or Q | 1200 cm in 0.18 s, 0.15 s i-frames, 2 charges, 1.2 s regen |
| Slide | Ctrl while sprinting | 0.7 s |
| Mantle | auto at ledges ≤ 180 cm | 0.35 s |
| Attack / Block / Heavy | LMB / RMB / hold LMB | ENG-22 |
| Interact / Objective ping | E / Tab | `ITDInteractable` |

- 100 health, regenerating after 4 s at 15/s; death → checkpoint respawn.
- Implements the ground side of `ITDHUDData`.

#### ENG-22 · Energy blade combat (CORE, used on planets) · `🟢`
| Move | Damage | Behaviour |
|---|---|---|
| Light combo ×3 | 25 / 25 / 45 | 180 cm, 100° arc sweeps; the third hit knocks back; 0.45 s combo window |
| Heavy lunge | 70 | 600 cm dash-stab; breaks Heavy guards; 2 s cooldown |
| Air slam | 50, 300 cm AoE | Downward plunge |
| Dash strike | 35 | Attack during a dash |
| Block | — | 120° front arc; move at 50% speed; 3 s stamina, then guard break |
| **Perfect deflect** (≤ 0.2 s) | Shot damage × 2 | Auto-aims back at the shooter, with a 0.15 s slow-motion beat (`OnHighlightMoment`) |
| Normal deflect | Shot damage × 1 | Reflects toward the crosshair |
| Melee parry | — | Staggers the attacker for 1.2 s; the next hit crits |

Hit-stop: 50 ms on hit, 120 ms on kill. Visuals and audio: `UTDBladeFX` (Nick).

#### ENG-23 · Crew enemies: AI & combat (CORE, planets) · `🟢`
| Enemy | HP | Attacks | Behaviour |
|---|---|---|---|
| Grunt | 60 | Telegraphed baton (15) | Groups of 3–5; flanks |
| Trooper | 50 | Slow, bright, **deflectable** bolts (10, 3-round bursts) | Keeps distance; uses cover |
| Heavy | 200 | Shield bash (25); deflectable shotgun (5 × 6) | Frontal shield blocks slashes |
| Sentry Drone | 30 | Fast small bolts (5) | Hovers out of melee range |
| Warden (mini-boss) | 600 | Charge slam (40 AoE), bolt fan, summons grunts | Phase 2 at 50% |

- Built on the template's `Variant_Shooter` NPC and StateTree AI: Idle/Patrol → Alert → Engage →
  Stagger → Dead.
- Attack tokens: at most 2 grunts attack at once.
- Ragdoll on death. Models and animations are Nick's (ART-08).

#### ENG-24 · Objective framework · `🟢`
`ATDObjective` (Id, steps, waypoint, reward ability and ship) and interactables (tap or hold;
interrupted by damage). Fires `OnObjectiveUpdated` and calls `CompleteObjective`, which brings up
Nick's reward screen. Encounter volumes spawn crew; clearing a room opens its doors.

#### ENG-25 · Verdance: gameplay layout & objectives (CORE) · `🟨`
**Story (act 2, The Voices):** objective 1 is **"Recover the Signal-Analysis Device"** from the wrecked research site (same gameplay as the wreck core); objective 2 **"Wake the Relay"** ends with the recordings: static, then human voices under the enemy's signal. Rewards unchanged. Names, markers and prompts follow `storyline.md` §5 and §8.
- **Level:** a compact level (about 1 km²) with the landing pad and a descent start point. Nick
  dresses the look.
- **Obj 1, Wreck Core** (→ Gravity Well):
  1. Follow the beacon past a trooper ambush.
  2. Clear the wreck site.
  3. Hold-interact for 4 s while 2 waves drop in.
  4. Carry the core back (no sprint or block while carrying).
- **Obj 2, Ancient Relay** (→ Cloak): light platforming (double jump, dash, mantle) with drones,
  three resonance nodes, then the relay pulses.

- **(Nov 12)** Built in code around the pad (`ATDPlanetCourse`): both objectives can be done in either order,
  each starts at its trailhead as its own scene, with route posts, objective lines and markers. Boarding is
  allowed at any time; the way back to space opens when both are done. A required objective with no actor in
  the world is completed with an error in the log, so a content mistake never traps the player.

#### ENG-26 · Cinder: gameplay layout, hazards & objectives (CORE) · `🟨`
**Story (act 3, The Pattern):** the outpost is the **mining complex** (objective 3 "Stop the Excavation", which also yields the stabilising crystals); the jammer is the dig's **field anchor** (objective 4 "Destroy the Field Anchor"): destroying it opens the passage through the distortion field instead of "revealing" the Dominion. Transmissions begin to echo the pilots' own voices. One door at the dig opens to the pilot's ID.
- **Hazards:** magma (heavy damage + respawn at the last safe spot), timed geysers, crystals that
  shatter when slashed.
- **Obj 3, Outpost** (→ EMP): breach the gate (2 conduits), 3 escalating areas, the Warden, then
  claim the EMP prototype.
- **Obj 4, Jammer** (→ reveals the Dominion): collapsing crystal bridges, plant 3 charges under
  attack, then run as the tower collapses.

- **(Nov 12)** The blade breaks blade-breakable props (crystals, gate conduits), not only enemies. The magma
  hurts at its surface only. A checkpoint precedes the Warden. **Director's mercy:** a fight the player keeps losing eases one
  notch at a time (Cinder Warden: from take 3, 75% health and one summon; from take 5, 50% and none); a player who
  wins first time never sees it. Any on-foot fight can opt in (`ATDEncounterVolume::MercyTiers`). Engineering-made props dress both planets
  (`Content/TriadDrift/Gameplay/Props`, trees in `Gameplay/Vegetation`) until Nick's art replaces them.

#### ENG-27 · Balance & tuning · `🟢`
Play the full run at least twice. Tune enemy counts, HP and cooldowns toward 20–30 minutes. Add a
"Pilot / Ace" difficulty setting that scales enemy damage and accuracy.

- **(Nov 12) Pressure targets on Standard, to be confirmed by a human:** a squad ship goes down in about 1 run
  in 5 in waves 1-2, about 1 in 2 in waves 3-4, most runs in wave 5 and the assault, with a squad wipe in about
  1 run in 5 there. Measured in the headless balance run (chance of a ship down and hull lost per minute for
  the active ship) before any tuning.

#### ENG-28 · Integration, performance, packaging & submission build · `🟨`
- **Integration and testing:** C++ architecture reviews, profiling (a steady 60 fps target on mid
  hardware), a smoke test after every merge.
- **Packaging:** Windows Shipping profile with all maps listed, startup map `L_MainMenu`, and the
  debug menu stripped.
- **Builds:** package whenever a major feature lands, into `Builds/` (git-ignored), and smoke-test
  start → finish.
- **Final zip:** `TeamName_GameName`, < 1 GB (otherwise use butler). Upload the build and fill in the
  submission form with Nick (ART-14).

#### ENG-32 · On-foot third-person camera & the Settings page (CORE, added Nov 12 by Ryan) · `🟢`
Ryan, Nov 12 (late): "a settings page, and you should be able to switch between first and third person ...
when you're using a character on the ground". Space flight keeps its own cameras (chase, cockpit); this item is
**on foot only** (planets, and the stretch interior).

**Camera modes on foot**
| Mode | What the player sees | Values |
|---|---|---|
| First person (default) | Today's view: blade on the camera rig, the body below the neck and its shadow | as ENG-21 |
| Third person | Over-the-shoulder chase view of the whole pilot | spring arm 320 cm, shoulder offset right 55 cm / up 65 cm, camera lag 12, rotation lag 18, collision probe 12 cm, FOV 85 |

- **Switching:** keyboard **V**, gamepad **View / Select** (on foot the galaxy-map binding does not apply), or the
  Settings page. Instant, no fade; never while a director shot or the death cam owns the view (it applies when
  they end). The choice is saved with the other options and restored at start-up and after every level travel.
- **Body in third person:** the whole pilot is shown (head included), the blade is held in the pilot's **right
  hand** (socket on the hand, set by eye), and the camera-rig blade is hidden. In first person nothing changes.
- **Facing:** the pilot turns toward the way it moves while just running; while attacking, blocking, aiming a
  deflect or standing still with the camera turned more than 60°, it turns to face the camera direction
  (strafe), so the blade always swings where the crosshair points. Melee targeting, the perfect-deflect return
  and interaction all use the camera's aim, exactly as in first person.
- **Animation readability:** idle / walk / run from `DA_Pilot_Visuals`; attacks, heavy lunge, block, dash, slide,
  air slam, mantle, hit react and death must visibly read in third person: retargeted mannequin animations onto
  the pilot's skeleton where that works headless, otherwise procedural upper-body poses in the pilot's animation
  instance. No T-pose, no sliding idle while moving.
- **Camera collision:** never inside walls, props, trees or crew; pulls in smoothly and eases back out; hides the
  pilot if the camera is closer than 60 cm; tight spaces (corridors, the interior) still readable.
- **Crosshair and HUD:** the crosshair stays centred and melee / interact prompts are unchanged; Nick's HUD and the
  fallback HUD work in both modes.
- **Combat fairness:** crew attacks, tells and deflect windows are identical in both modes; third person must not
  reveal enemies through walls.

**Settings page (in-game, reachable from the main menu and the pause menu)**
- Engineering ships a code-built page (keyboard, mouse and gamepad navigable) over `UTDSettingsSubsystem`:
  **On-foot camera (First / Third person)**, mouse sensitivity, invert mouse Y, gamepad sensitivity, gamepad dead
  zone, invert gamepad Y, control hints on / off, master / music / effects volume, window mode, resolution, and
  "Reset to defaults". Every change applies at once and is saved.
- Nick's side may supply a styled page through `UTDUIStyle::SettingsWidgetClass`; it calls the same Blueprint
  functions and replaces the code-built page with no engineering change.

**Done when:** on both planets a player can switch with V / View and from the Settings page, fights a full room in
third person with readable attacks and blocks, the camera never clips through the level, the choice survives a
restart, and tests cover the toggle, persistence, camera collision, facing rule and the space-only exclusion.

#### ENG-33 · Cinematic intro (CORE, added Nov 12 by Ryan) · `🟢`
**Redo queued (Ryan, Nov 13): the opening is rebuilt to Nicholas's *First Contact* brief** (about 2:20: missing patrol, Triad arrives, the impossible vessel, the first launch with the drone that goes blue and holds fire, the cut-off "Don't-", then straight into the tutorial battle; `storyline.md` §11, `docs/story/First-Contact-Cinematic-Brief.md`). **Done once, after the art it shows is on `main`** (Rodin player ships, the fleet with a space drone, the Dominion with opening hangars, round-4 effects, the new HUD). Until then T3 prepares the shot plan, the dialogue hooks and the drone beat only. Today's 49 s intro stays in the game meanwhile.
**Story (opening):** the intro shows the premise: the silent Dominion, machines descending on worlds, Triad deployed to identify it and report back. No hint of the truth. Text and narration are Nicholas's through the existing slots.
Ryan, Nov 12 (late): the intro should be "really high definition cinematic views of space and the spacecraft going
by and worlds". **Reference for the cinematic language only** (pacing, scale, framing, grade), not its content, world
or style, which belong to another IP: the *Warhammer 40,000: Darktide* intro cutscene
(https://www.youtube.com/watch?v=CZgaAUA9brI, 2:54). What it does well, from frames reviewed by the Lead:
- very slow push-ins on a vast nebula; huge objects (a planet in silhouette, a colossal ship) that cross the frame
  slowly so scale reads; cuts to black between beats; narration as short subtitled lines; a consistent colour grade
  (warm nebula against cold metal); shallow depth of field on close subjects; long lenses for scale shots.
Triad Drift's version is **bright and saturated**, not grimdark (AGENTS.md section 2), and uses our own assets.

**Shot list (about 45 to 55 s, every shot real-time in `L_Galaxy` with the game's own assets)**
| # | Shot | Camera | Duration |
|---|---|---|---|
| 1 | Black. First narration line fades in. The nebula fades up behind it | locked, 35 mm, a very slow push | 6 s |
| 2 | Verdance in eclipse: ringed planet silhouetted, sun breaking over its limb (rim light, lens flare) | long lens 85 to 135 mm, slow lateral dolly | 6 s |
| 3 | The squad: Rift, Lancer and Bastion pass close in formation, engines streaking; each pilot's name card as it passes (C-03 #1 style) | 24 mm low angle, whip-pan follow, shallow depth of field | 8 s |
| 4 | Threading the asteroid belt (ice, metal and crystal rocks, debris) with the squad small in frame for scale | 50 mm drifting through the belt, foreground rocks parallax | 6 s |
| 5 | Cinder, red and volcanic, far off; a portal gate ring flares as the squad passes through | 35 mm, track with the squad | 6 s |
| 6 | The threat: red warp-in flashes; Null Armada fighters pour out; the Dominion's silhouette slides across the stars, vast | 135 mm from far away, slow push; then 24 mm as fighters scream past | 10 s |
| 7 | Hard cut to black. Title card **TRIAD DRIFT**, then the slate "SCENE 1 · WAVE 1 · TAKE 1"; the camera eases into the chase camera behind the player's ship and control starts | blend 1.0 s into the chase camera | 6 s |

- **Look:** 2.39:1 letterbox (the Director's Cut bars), Cine Camera with real filmback and focal lengths, depth of
  field focused on the subject, gentle handheld noise on close shots only, motion blur on, exposure locked per shot,
  bloom and lens flare on engines and suns, a light film grain, colour grade per the art direction (saturated
  nebula hues; squad colours magenta / cyan / amber on the ships, red-orange on the Armada).
- **Narration:** 4 to 6 short lines as subtitles (text cards), written in the Director's Cut voice (a director
  framing the story: "Scene one. A wing of three ..."), localisable text; an optional voice-over cue per line plays
  when presentation assigns one. Music: an intro cue that swells into wave 1 (presentation slot; silence is allowed
  until Audio assigns it).
- **Rules:** plays once on **New Game** (not on Continue, not on a retake); skippable at any time after 1 s with any
  key, click or pad button (hold for 0.5 s to avoid accidental skips; a "HOLD TO SKIP" hint); never takes input
  away for longer than the intro; the HUD is hidden during it; the wave director, enemies and squad AI do not start
  until it ends; the game state is identical whether it was watched or skipped.
- **Presentation override:** if Nick's side authors a Level Sequence (`UTDUIStyle::IntroSequence`), it plays
  instead of the code-driven shots, with the same skip and hand-off rules.
- **Performance:** 60 fps on the target machine at 1080p; shots may hide gameplay-only actors that are not in frame.

**Done when:** New Game plays the intro from the menu into wave 1 with a seamless hand-off; skip works by keyboard,
mouse and pad; Continue and retakes never play it; captures of every shot are reviewed against `AAA.md` by an
independent reviewer; tests cover play-once, skip, hand-off and that waves start only after it.

#### ENG-34 · Planet worlds: large procedural terrain (CORE, added Nov 13 by Ryan) · `🟨`
Ryan, Nov 13: "building out these worlds ... procedural generation, like mountains ... make it look big, not just
one little section on the map ... a massive, massive map that you should be able to explore."
- **What:** each planet (Verdance, Cinder) is a large generated world around the existing objective course, not a flat
  slab: mountain ranges, valleys, ridges, mesas and basins from layered noise (ridged for mountains), seeded per planet
  so the world is the same every run. Target extent: at least 16 x 16 km walkable and flyable, with distant ranges
  beyond it that close the horizon.
- **The course stays playable:** the objective sites, landing pad and the routes between them sit in terrain that is
  flattened and blended around them (a basin / plateau), at the same positions and heights gameplay uses today; no
  existing objective, encounter or test moves. Outside it the player can walk and fly anywhere.
- **Built at runtime, in code:** chunked terrain meshes with level of detail by distance, collision near the player
  and the ships, no map edits (the maps are shared). Nick's tagged environment replaces it when present.
- **Looks:** a terrain material that blends by slope and height (Verdance: moss and soil lowlands, rock faces, glowing
  ridgelines; Cinder: basalt plains, lava rivers in the valleys, ash and crystal on the heights), the planets' fog and
  sky, and biome scatter from the existing kits (trees, ferns, mushrooms, rocks, crystals, basalt columns) by height,
  slope and noise masks, instanced, never on the course's paths.
- **Flight:** atmosphere flight (ENG-20) flies over and between the mountains; arrival and take-off stay as they are;
  the landing assist still finds the pad.
- **Performance:** inside the ENG-28 budget on the target machine; generation is incremental (no hitch above 4 ms a
  frame) and the first view is ready before the arrival camera shows it.
- **Acceptance:** from the pad the player sees mountains in every direction; a 2-minute walk or flight in any direction
  finds new terrain, not an edge; every existing planet test and both objective chains still pass; captures from the
  air and on foot on both planets.
- Owner: T4 (world generation, terrain meshes, scatter), T5 (course fitting and on-foot checks), T1 (flight over
  terrain), Lead (terrain material and art).

#### ENG-35 · Engineering fallback sound set (CORE, added Nov 13 by Ryan's Lead under Ryan's instruction) · `🟨`
Audio is a judged category and Nick's audio lane is stopped with only the menu theme and one cannon sound made. Until
Nick's sounds exist, engineering supplies an original, cohesive fallback set, announced on the board (R-024):
- every weapon, power and ability; hits, shield breaks and explosions; engines, boost, drift and hyperdrive; the switch
  cut, the slate and "CUT!"; UI moves and confirms; on foot: footsteps, blade swings, hits, deflects, crew shots;
  ambience for space and each planet; a combat music bed if none exists.
- Original sounds made with synthesis and processing (the `ue-game-audio` skill), levelled to its mix standard, each
  with attenuation, class and concurrency; logged in `CREDITS.md`.
- Wired only through the existing cue slots, in engineering folders, as fallbacks: a sound of Nick's in the same slot
  always wins, and nothing of his is overwritten. An agent reaches "technically verified"; a person approves by ear.
- Owner: Lead (builder sub-agents) for the sounds, each lane's owner for any missing cue slot.

#### ENG-36 · HUD redesign: modern, sleek, minimal (CORE, added Nov 13 by Ryan) · `🟨`
Ryan, Nov 13: "The HUD currently is kind of bulky and it needs to be more modern and sleek, and not taking up as much
room. We need to just redo the entire HUD ... a similar space type of HUD feel that doesn't take up so much space and
it's easy to utilize, and even the beginner gamer can understand it."
- **Research first:** how current games lay out a flight / action HUD (what sits near the reticle, what sits in the
  corners, what hides until it matters), written up with a wireframe before any code. Original design; other games are
  references for principles only.
- **Budget:** in normal flight the HUD covers no more than about 8% of the screen (the current one covers about 25%
  along the bottom), with the centre and the lower middle clear for the ship.
- **Principles:** the few things a pilot needs every second sit close to the reticle or the ship (hull and shield of
  the flown ship, the signature power's readiness, the target and its health); everything else is small, at the edges,
  and appears only when it changes or matters (wingmen status, ammo and heat, objective, wave, hyperdrive, hints);
  icons and bars before numbers; one accent colour per squad ship; plain words; a new player can tell, without
  reading a manual, which ship they fly, how hurt they are, what to shoot and what the big button does.
- **States:** space combat, travel, atmosphere flight, on foot (first and third person), the Dominion assault, the
  director's moments (cut card, slate, "CUT!"), pause; keyboard and pad prompts that follow the device in use;
  1080p and 1440p, 16:9 and ultrawide safe areas; colour-blind-safe shapes as well as colours.
- **Built in engineering's own code and content** (not in `Plugins/TriadDriftHUD`, which stays Nick's and untouched),
  on the existing HUD data contracts (`ITDHUDData`, the UI host). It is the default HUD; a setting keeps Nick's
  widget selectable, and his HUD replaces this one whenever he says so.
- **Acceptance:** before / after captures in every state above; the coverage measured; a fresh reviewer playing the
  "first five minutes" checklist can name each element's meaning; tests for every data binding.
- Owner: T2 (build), Lead (research, design, review).

#### ENG-37 · Projectiles, engines and particle effects at AAA quality (CORE, added Nov 13 by Ryan) · `🟨`
Ryan, Nov 13: "really focus on projectiles across the entire game, from the blast of the engine to shooting.
Everything kind of looks pixely and AI generated right now ... AAA quality projectiles ... you can see the flames
coming out and there's movement ... a bunch of particles, and everything that makes a game look really solid."
- **What:** real particle effects (Niagara) replace the flat glow shapes everywhere something fires, burns, hits or
  explodes. Each effect has a core, motion and secondary particles, in the squad / enemy colour code:
  - **Projectiles:** each weapon's shot has a hot core, a soft glow, a tapered trail that lingers, and sparks or
    plasma wisps shed along the path: Rift bolts (magenta, phasing), Lancer needles (cyan, tight and fast), Bastion
    shells and missiles (amber, heavy, with smoke trails and a flickering motor flame), enemy fire (red-orange),
    turret and Dominion fire, crew blaster bolts on foot.
  - **Muzzle flashes** on every weapon: a brief shaped flash, sparks, a light pulse.
  - **Engines:** layered exhaust on every ship: a bright inner flame cone with turbulence, heat shimmer, a longer soft
    plume, boost afterburner rings and sparks, idle flicker; enemies in red-orange; wingman trails as thin ribbons.
  - **Impacts:** shield hits (a ripple on the shield surface and blue sparks), hull hits (sparks, debris flecks, a
    scorch flash), ricochets, on-foot hits and deflects.
  - **Explosions and deaths:** fireball with rolling motion, shockwave ring, sparks and hull chunks on the ship's
    velocity, smoke, embers; larger and multi-stage for big ships and the Dominion breakup.
  - **Powers and travel:** ram wake, portal rims and missile return, swarm launches, dome, cloak shimmer, EMP wave,
    gravity well, hyperdrive streaks and exit flash, gate transit, warp-ins.
- **Quality bar:** nothing reads as a flat disc, a billboard square or a low-resolution sprite at any gameplay
  distance; effects move (turbulence, flicker, stretch by velocity), fade smoothly, and stay readable in a 40-ship
  fight without hiding targets; HDR emissive tuned to the game's exposure and bloom so cores are hot but not blown out.
- **Performance:** effects are pooled, GPU-simulated where the count is high, bounded per system, and scale down by
  distance; a full wave-5 fight stays inside the ENG-28 frame budget.
- **Wiring:** through the existing cue slots (`FTDFXCue::VFX`) and engineering fallback data assets; the code
  placeholders stay as the fallback when an effect is missing; an effect of Nick's in the same slot always wins.
- **Acceptance:** before / after captures of every effect at gameplay distance and in slow motion; a fresh reviewer
  scores effects at 4 of 5 or better; frame-time measured in a wave-5 fight.
- Owner: Lead (effects builder sub-agents), T2 (combat cues and placeholders), T1 (engines and trails), T4 (enemy,
  Dominion and world cues), T5 (on-foot cues).

#### ENG-38 · Planetary descent: fly from space into a world, anywhere on the globe (CORE, added Nov 13 by Ryan) · `🟨`
**Ryan, Nov 13:** flying down into a world must be "super realistic, super high quality, AAA quality, make it almost
look like No Man's Sky": a transition "where you hit the atmosphere and then your ship gets some type of animation
showing you're piercing the atmosphere", "the world comes into focus as you get closer and closer", "extremely
detailed", and "you should be able to enter a world from anywhere, flying around that globe of a planet".
Research and the recommended design: `docs/eng/planet-descent-research.md` (No Man's Sky, Elite Dangerous, Unreal
techniques, our current code path; in progress Nov 13). This item replaces the old arrival of ENG-20; the landing rule
and the on-foot hand-over of ENG-20 stay.

- **What the player does, with no loading screen at any point:**
  1. **Approach and orbit.** The planet is a real globe in the galaxy; the player can fly around it, low over the
     cloud tops, and choose where to go in. The globe shows the same world as the ground: the same continents,
     mountain ranges, seas or lava, and the landing zone as a faint beacon.
  2. **Atmosphere interface.** First wisps and a horizon glow; the HUD shows altitude, speed and an entry cue; the
     hyperdrive and boost cut out; sound thins to a rising roar.
  3. **Burning in.** The ship visibly pierces the atmosphere: a plasma sheath that grows from the nose (white core to
     the ship's colour to orange), heat glow on the leading edges, streaks past the canopy, a shock cone, heavy
     shake that eases as the air thickens, control authority reduced but never removed. Wingmen burn in alongside.
  4. **Through the clouds.** A cloud deck swallows the ship; this is where the planet's level is brought in, hidden.
  5. **The world comes into focus.** Break through the cloud base to the whole landscape: horizon curvature
     flattening, aerial haze thinning, mountain ranges resolving first, then rivers of lava or water, forests and
     rock fields, then set pieces and the landing zone, continuously, with no pop the eye can catch.
  6. **Low flight and landing.** Fly anywhere over the terrain (ENG-34); land at the pad (ENG-20 rule) or, as a later
     stage, on any flat ground.
  7. **Take-off is the same in reverse**: climb, clouds, thin air, stars, the globe behind you.
- **Enter anywhere.** Where the player crosses the atmosphere on the globe decides where they arrive over the
  terrain and on what heading: the same latitude and longitude, the same sun angle. The landing zone is always
  findable (objective waypoint and beacon) from wherever they come in.
- **Quality bar:** indistinguishable from continuous at normal play (no black frame, no freeze over 100 ms, no
  visible swap of globe for ground); the ship effect reads from the chase camera and in a still; at least 60 fps at
  1080p through the whole descent on the target machine; works for all three ships and with a gamepad; a retake or
  Continue during the descent restarts it cleanly.
- **Stages** (the research report sets the final cut): **stage 1**: the entry sequence at full quality (effects,
  camera, sound, HUD, the hidden load, the focus-in) from any approach direction, arriving on a matching heading;
  **stage 2**: orbiting the globe and entering at a chosen point that maps to a place on the terrain, the globe
  showing the real terrain, wingmen in the sequence; **stage 3 (stretch)**: landing anywhere, and day / night matching
  the entry longitude.
- **Owners:** T1 (flight: orbit and entry flight model, control authority, camera with T3, the hand-over), T4
  (terrain: the globe-from-orbit view baked from the terrain field, focus-in LOD and fog schedule, arrival placement),
  T5 (flow: bringing the planet level in without a loading screen, arrival state, retake and Continue), T2 (HUD:
  altimeter, hull heat, entry cue), T3 (director: entry camera beats, a money shot on cloud break), Lead effects
  builder (plasma sheath, heat glow, streaks, shock cone, cloud layer, wisps), Lead audio builder (entry roar,
  burn-through, wind, boom), **Grok Bot** (cloud and plasma textures and flipbooks, planet surface maps, ship
  heat-glow masks, any deployable part such as airbrakes or heat shields with its animation), **Nick** (the
  audio of the entry, if he takes it; the atmosphere's look from orbit to ground moved to Ryan's side with ART-13 on
  Nov 13: the Lead's environment builder owns it).
- **Tests:** arrival position and heading as a function of the entry point (unit); no hitch over 100 ms across the
  hidden load (measured in a rendered run); descent completes for each ship at 30 / 60 / 120 fps; retake and Continue
  mid-descent; take-off reverses it. **Only a person can judge** whether it feels like piercing an atmosphere.

### Art and assets made on Ryan's side (moved from Nick's section, Nov 13)

Ryan's rule (Nov 13): what Claude or Grok Bot makes is under Ryan's name. These five items keep their `ART-xx`
IDs and their original spec text; the first line of each says who makes it now. **Since the afternoon of Nov 13 this
covers all of Environment, VFX and Asset Management** (ART-02, 03, 04, 05, 06, 07, 08, 13): Ryan took those three
categories over completely. Nicholas keeps direction and
sign-off, and an asset of his at the Presentation name replaces ours.

#### ART-03 · Ship visuals: Rift, Lancer, Bastion · `🟨`
**🟦 Ryan since Nov 13.** Made by Grok Bot (Rodin, R-027 #1) from the approved concepts; Ryan's builders clean up, import and wire into `Gameplay/Characters/Squad`; engineering's r7 ships are in the game until then. Paths in the text below that name Nick's folders now mean engineering's (`Gameplay/**`, `ArtSource/Eng*/**`).
| | Rift | Lancer | Bastion |
|---|---|---|---|
| Silhouette | Crescent wing + ring-shaped portal emitter on the nose | Long needle fuselage, forward-swept wings | Wide, chunky hull, twin missile pods, dome emitter |
| Colours | Violet `#6B2BD9` / magenta glow `#FF2BD6` | Ice white `#E8F4FF` / cyan `#12C8FF` | Amber gold `#F2A01E` / gunmetal |
| Part animations | Portal ring spin-up, gear, canopy | **Wing fold for the ram**, gear, canopy | **Missile pods open**, dome emitter rises, gear, canopy |

- Models via route A (concept → Rodin → cleanup) or the kits, passing the AAA rubric (the
  `blender-game-asset` skill). Sockets: muzzles, engines, cockpit camera.
- Delivered as `DA_<Ship>_Visuals` (`DA_Rift_Visuals`, `DA_Lancer_Visuals`, `DA_Bastion_Visuals`; exact
  names in `docs/visual/Presentation-Asset-Paths.md`, a differently named asset is silently ignored) (§3.5), with animations driven by the exposed state. **Readable as
  black silhouettes at 64 px.**

#### ART-04 · Enemy & Dominion visuals · `🟨`
**🟦 Ryan since Nov 13.** Enemy ships, turret, Warden, drone, generator and reactor by Grok Bot (R-027 #2, #5, #6); Dominion exterior and crew by engineering, already in the game; T4 and T5 wire them.
- **Null Armada ships:** Shard Fighter, Hook Interceptor, Wall Gunship (shield emitter), Maul Bomber,
  Hive Carrier (animated bay doors).
- **Dominion exterior:** a modular Nanite hull with rotating rings, shield generators, turrets, the
  reactor trench (core ending) and the hangar mouth (stretch).
- **Look:** angular, red/black/white-eye shape language. Telegraph glows tied to the gameplay states.

#### ART-06 · Planet environments: Verdance & Cinder (CORE) · `🟨`
**🟦 Ryan since Nov 13.** Terrain, mountains, materials, rocks, trees and set pieces: engineering (ENG-34) with Grok Bot's texture sets and set pieces (R-027 #3, #8). **Lighting, fog, sky and grade on the planets stay with Nick (ART-13).**
- **Verdance:** bioluminescent jungle, giant mushrooms and ferns, waterfalls, mist, ringed moon,
  pollen, critters, wreck-site and relay-spire art.
- **Cinder:** basalt, magma rivers, cyan and amber crystal spires (crystal kits), falling ash, an
  outpost kit, the jammer tower and its destruction.
- **Across both:** terrain materials, foliage instancing, sky and atmosphere, lighting, and the
  atmospheric-descent vista (canyons and clouds the ship flies through).

#### ART-07 · Combat & travel VFX (Niagara) · `🟨`
**🟦 Ryan since Nov 13.** Built as Niagara effects in `Gameplay/VFX` (ENG-37) with Grok Bot's flipbook textures (R-027 #7). Nick's side may hand over its own effect for any cue.
- **Explosions** in 4 tiers; engine trails per squad colour; boost, drift and speed lines; warp-ins
  (squad white-blue, Armada red tear).
- **Every weapon's muzzle, tracer and impact** (hull, shield, rock); shield ripples; overheat vents;
  missiles and flak.
- **Powers:** portals (ring, swirl, transit flash), ram cone and sonic boom, missile swarm trails,
  shield dome (hex, ripples, shatter).
- **Abilities:** Gravity Well (lens distortion, accretion), Cloak (refraction shimmer), EMP (ring and
  arcs).
- **Travel:** hyperdrive (spool, tunnel, drop-out), atmospheric entry plasma, landing dust,
  tractor beam (stretch).
- **Readability rule:** nothing important hidden by bloom. Budgets: particle caps and significance
  (ART-13).

#### ART-08 · Ground visuals & animation · `🟨`
**🟦 Ryan since Nov 13.** Crew models and rigs by engineering; blade combat animations by Grok Bot (R-027 #4), hooked up by T5. The pilot body stays Nicholas's Sentinel Armor.
- **First-person arms:** idle, run, sprint, jump, slide, mantle and dash; blade ignite and retract;
  combo ×3, lunge, air slam, block, deflect flourish, guard break; interact and carry; climb into
  and out of the cockpit.
- **Energy blade:** emissive core and trail, scorch decals, the perfect-deflect flash.
- **(Nov 12)** The Null Armada trooper model (Trooper, then Grunt and Heavy variants) is built by engineering
  from Ryan's concept (`ArtSource/EngCharacters/NullTrooper/`, `MESSAGEBOARD.md` R-019); the rest of this item
  stays with Nick.
- **Crew models and animations:** idle, locomotion, telegraphed attacks, hit-reacts, stagger,
  deaths, ragdoll-ready; Fab or Mixamo retargets, credited. A visor glow shows alert state.

#### ART-02 · Visual asset inventory & sourcing plan · `🟨`
**🟦 Ryan since Nov 13 (asset management moved to Ryan's side).** The inventory of every asset, where it comes from, its licence and its state is kept by Ryan's Lead: `docs/eng/ASSETS.md` (ledger), `CREDITS.md`, `docs/eng/ART-HISTORY.md`. Sourcing is Grok Bot (generation) and engineering builders (code-built, imports). Nick's supplied-model intake (N-041, N-042) is a library Ryan's side selects from.
- **The list:** every asset the game needs (ships, enemies, Dominion pieces, props, environment sets,
  characters, VFX, UI and audio), each with its route: AI image-to-3D (Rodin first: credits expire
  Nov 20, and using it enters the prize pool), Fab or other licensed library, code-built Blender, or
  the template.
- **Status and credits:** track each asset's status here or in `ArtSource/inventory.md`, and log
  licences in `CREDITS.md`.

#### ART-05 · Space environment & galaxy dressing · `🟨`
**🟦 Ryan since Nov 13 (environment moved to Ryan's side).** The space sky, nebulae, galaxy dressing, lighting and post-process of `L_Galaxy` are built by engineering builders with Grok Bot's panoramas and textures, under the Lead. Today: the 8K sky and code-placed dressing are in the game. Nick's Hero sky work is a reference we may adopt, not a dependency.
- **Space assets:**
  - `BP_GalaxySkybox`: layered nebula, stars, sun with lens flare.
  - `BP_PlanetVisual`: rings, atmosphere rim, Verdance and Cinder variants.
  - Nebula volumes, a speed-dust field, asteroid-field kits (`procedural_rocks.py`), station ruins.
- **Galaxy map:** dress `L_Galaxy` on your own actors (One File Per Actor): lighting, post-process
  and scale cues.

#### ART-13 · Lighting, post-process & performance-minded art · `🟨`
**🟦 Ryan since Nov 13 (environment moved to Ryan's side).** Lighting, fog, sky, atmosphere, colour grade and post-process on every level (galaxy, Verdance, Cinder, the Dominion interior, the menu) are built by engineering under the Lead, including the atmosphere's look for the planetary descent (ENG-38). Performance budgets stay as written.
- Lumen and Nanite settings per level, scalability presets wired to Settings, Niagara budgets,
  foliage culling.
- Hold the art budgets in `art-direction.md` and keep VFX and environment complexity within a steady
  60 fps on mid hardware. Profile with Ryan before the freeze.

### Boarding finale (WANTED since Nov 13, Ryan; story acts 4 and 5; was stretch)

#### ENG-29 · Docking & boarding transition · `🟨` WANTED (Nov 13)
**Story (act 4):** the squad breaks through the field and boards. Plan (T4, Nov 13): docking bay opens when the defences are down, an approach cone with a tractor assist that never fully locks before the mouth, one travel into the interior, wingmen hold outside.
`ATDDockingBay` approach cone with a tractor-beam assist that reduces control, then
`TravelTo(DominionInterior)`; the hangar landing hands off to first person (§8).

#### ENG-30 · Dominion interior encounters & core fight · `🟨` WANTED (Nov 13)
**Story (act 4, The Truth):** familiar military architecture, doors and consoles keyed to the pilots' identities, and an interactable console with a recording of their older selves in each room (hook with a placeholder line; Nicholas's words and voices). The core fight is the choice of act 5.
- **Interior:** 6 sections (hangar, a deflect-lesson corridor, barracks, a vertical shaft with
  sweeper hazards, a security checkpoint, then the Warden) leading to the core chamber.
- **Core fight, phase 1:** 4 conduits at 150 HP each; the core fires deflectable orbs.
- **Core fight, phase 2:** a sweeping laser; the heart is vulnerable whenever the rings align, every
  6 s.
- **Core fight, phase 3:** at 25% health, everything speeds up.

#### ENG-31 · Escape run & escape flight · `🟨` WANTED (Nov 13)
**Story (act 5, The Choice):** the escape plays with time visibly failing (echoes, repeats); after the breakup the victory screen receives the distress signal with the squad's own identifiers.
- **Escape run:** a 90 s run back to the hangar on a changed route.
- **Escape flight:** a 60 s flight out through the falling debris, then the breakup sequence and
  victory. Failing either restarts it as the next take.

---

## 6. 🟩 Nick: Art, Visuals & Player Experience

**Nov 13: read [the realignment](#presentation-today-nicks-agents-proposed-by-ryan) first.** It lists what
engineering and Grok Bot now cover as fallbacks and proposes today's priorities for Nick's agents: music and
the mix, lighting and look, menus and the Director's Cut presentation, voice, the submission, and art review.
**Ownership changed Nov 13 (Ryan):** ART-03, 04, 06, 07 and 08 moved to Ryan's section (§5); and on the afternoon of Nov 13 Ryan took over **Environment, VFX and Asset Management** completely, so ART-02, 05 and 13 moved too. What remains below is Nick's: **ART-01, 09, 10, 11, 12, 14** (UI and menus, audio and music, voice and story words, Director's Cut presentation design, submission, art direction notes).

| Item | State Nov 13 | Owner now / stand-in from Ryan's side | For Nick's agents today |
|---|---|---|---|
| ART-01 direction | 🟨 palettes and colour code in use | - | Review and sign-off notes (priority 6) |
| ART-02 inventory, asset management | 🟨 | **🟦 Ryan** (moved to §5, Nov 13 afternoon): Lead's ledger, Grok Bot | None; stop intake work |
| ART-03 ships | 🟨 Lancer source parked | **🟦 Ryan** (moved to §5): Grok Bot + builders | Review only |
| ART-04 enemies, Dominion | 🟨 | **🟦 Ryan** (moved to §5): Grok Bot + builders | Review only |
| ART-05 space environment | 🟨 | **🟦 Ryan** (moved to §5, Nov 13 afternoon) | None; stop Hero sky work at a safe checkpoint and publish it as a reference |
| ART-06 planets | 🟨 | **🟦 Ryan** (moved to §5): terrain, materials, props, trees | Review only (lighting moved to Ryan with ART-13) |
| ART-07 VFX | 🟨 source only | **🟦 Ryan** (moved to §5): ENG-37 | Review only; offer per-cue studies |
| ART-08 ground visuals | 🟨 Sentinel pilot in game | **🟦 Ryan** (moved to §5): crew, animations | Review only |
| ART-09 HUD and menus | 🟨 M03 HUD on `main`, menu on a branch | ENG-36 flight HUD, fallback menus | **Priority 3:** menus, reward, victory, loading, fonts and tokens |
| ART-10 audio and music | 🟨 menu theme on `main`, PR #11, PR #12, one combat track | ENG-35, 200 stand-in sounds | **Priority 1:** music, mix, ear approval |
| ART-11 barks, voice | ⬜ | Text barks | **Priority 4** |
| ART-12 cinematics, Director's Cut look | 🟨 | Fallback slate, CUT, name card, letterbox; ENG-33 intro | **Priority 3:** design them as a set |
| ART-13 lighting, post-process | 🟨 | **🟦 Ryan** (moved to §5, Nov 13 afternoon) | None |
| ART-14 submission | ⬜ | Trailer capture pipeline, `CREDITS.md`, `DEVLOG.md` | **Priority 5** |
| ART-15, 16 boarding | 🟨 WANTED (Ryan, Nov 13) | Engineering builders make the interior kit as a fallback; effects builder for the collapse | Lighting and alarm states of the interior; the recordings' voices (ART-11) |

**Start here (role brief, Day 1 focus):** establish the art direction and the visual asset inventory,
make the ships readable, and do a minimal combat presentation pass alongside Ryan's playable
foundation.

**Deliverables:**
- a coherent, attractive visual style across all three ships and the combat spaces
- distinct visual and audio feedback for Rift, Lancer and Bastion
- a readable, functional combat HUD and ship-switch presentation
- polished effects and environment dressing that don't compromise the build

**Tools:** the `blender-*` skills (`.claude/skills/`; Codex: see the index in AGENTS.md §10) for
making, cleaning up and exporting 3D assets. The palettes and budgets are in
`blender-game-asset/references/art-direction.md`.

#### ART-01 · Art direction & style guide · `⬜`
- **Content:** a one-page style guide with palettes, the faction shape language, readability rules
  (player projectiles in squad colours, enemies in red and orange), lighting mood per level, and
  the **Director's Cut visual language** (cuts, letterbox, slate cards, grade; C-03).
- **Reference board:** inspiration only; no copying of existing IP.
- **Gate:** both humans agree on the style before mass production.

#### ART-09 · HUD & UI/UX · `⬜`
- **`WBP_SpaceHUD`** (via `ITDHUDData`):
  - **Squad panel:** three cards showing hull, shield, cooldown rings, ability padlocks and respawn
    timers; the **switch animation** ties into the "cut" (C-03 #1).
  - **Combat:** crosshair, target bracket with health, lead indicator, lock brackets, hit and kill
    markers; arrows for enemies, missiles and objectives.
  - **Status:** boost, heat and ammo, hyperdrive charge; banners for "WAVE X", "ABILITY READY",
    the boundary warning and Switch Strike.
  - **Atmosphere variant:** altitude and the landing prompt.
- **`WBP_GroundHUD`:** health, block stamina, perfect-deflect flash, dash charges, objective tracker,
  interact ring, damage direction, escape timer (stretch).
- **Menus and screens:**
  - Main menu (animated galaxy background), Pause, Settings (audio, sensitivity, invert, difficulty,
    subtitles, shake, graphics), Controls.
  - **(Nov 12, Ryan)** The Settings page also carries the on-foot **first / third person** choice (ENG-32).
    Engineering ships a code-built page; a styled one goes in `UTDUIStyle::SettingsWidgetClass`.
  - Galaxy map; `WBP_AbilityUnlocked` (the big reward moment); Defeat, Victory (stats, rank, takes)
    and Credits; Loading.
- **Onboarding:** `WBP_Tip` toasts from `DT_Tips` (on `OnTipRequested`), with glyphs that switch
  between keyboard/mouse and gamepad.
- **Director's Cut UI:** pilot name cards on switch (C-03 #1); scene slate cards and the "CUT!" /
  next-take restart (C-03 #3); takes per scene on the victory screen.

#### ART-10 · Audio & music · `⬜`
- **Space SFX:** engine loops per ship that rise in pitch with throttle, drift screech, every
  weapon's fire and impact (3+ variations each), overheat, lock tones, missile alarm, portals, ram,
  dome, abilities, explosion tiers, shield vs hull hits, low-hull alarm, the switch "cut" whoosh,
  the hyperdrive set, atmosphere wind, gear, canopy, the Dominion horn, turrets, generator kills,
  the breakup roar.
- **Ground SFX:** footsteps by surface, dash, slide, all blade sounds, crew barks, attacks and
  deaths, drones, doors, hazards, magma, crystals, alarms.
- **Music director:** stems that swell with `OnWaveStarted(Intensity01)` and calm on clear; planet
  exploration and combat layers; the Dominion and finale track; stingers (ability unlocked, reveal,
  generator down, reactor destroyed, "CUT!").
- **Mix:** sound classes and submixes (Master, Music, SFX, VO, UI) wired to Settings; music ducks
  under VO.
- **Sources:** licensed only (e.g. the WeLoveIndies jam pack: non-commercial), logged in CREDITS.md.

#### ART-11 · Narrative presentation: barks, VO, intro · `⬜`
**Story (Nov 13):** this item is now the voice of *The Paradox*: pilot barks in character (Wren bold, Kato analytical, Moss steady), the transmissions (static, then human voices, then the pilots' own voices), the recordings inside the Dominion, intro narration and the ending distress signal. Engineering fires the events with placeholder lines; your text and audio replace them (`storyline.md` §5, §10).
- `DT_Barks` (about 80–120 lines: Wren, Kato, Moss and the Armada) played on
  `OnBarkRequested` through a priority queue, with radio-filtered VO (AI-voiced, credited) and
  subtitles.
- **Intro sequence (30–45 s, skippable):** the squad over Verdance's ring, then the Armada warps in.
  **(Nov 12, Ryan)** Engineering builds the intro as real-time shots in code (ENG-33); presentation supplies
  narration voice, music, the grade and optionally its own Level Sequence (`UTDUIStyle::IntroSequence`).

#### ART-12 · Cinematics & the Director's Cut presentation · `⬜`
**Story (Nov 13):** slate, "CUT!" and name cards can lean into the paradox (a take number that glitches from Cinder onward; `storyline.md` §7).
- **Sequences:** the Dominion reveal, the reactor breakup and victory hero shot, landing and
  take-off shots, the planet descent breakthrough.
- **Theme hooks (C-03 #1–#3):**
  - the switch "cut" transition and name cards
  - director-cam rigs, framing and grade for the slow-motion highlight moments
  - scene slate cards and the "CUT!" restart
- **Trailer capture:** Movie Render Queue shots for ART-14.

#### ART-14 · Submission presentation · `⬜`
- **Gameplay video (30–60 s):** gameplay in the first 5 seconds. Dogfight → switch "cut" → portal
  missile return → ram → planet landing → blade deflect → Dominion reactor.
- **itch.io page:** cover art, 5+ screenshots and a GIF, the description, controls, credits and
  "Tools you used". Compile the **dev log** (Glass Box prize) from `DEVLOG.md` with Ryan.

### STRETCH: boarding finale visuals

#### ART-15 · Dominion interior kit & core visuals · `⬜` STRETCH
- **Interior kit:** modular pieces on a 400 cm grid (corridor straight, corner, T, doorway, shaft,
  catwalk, door) in gloss black with red strips; giant eye lenses that track the player.
- **Core chamber:** the white-light reactor with rotating rings, plus the collapse and alarm states
  for the escape run.

#### ART-16 · Boarding & escape cinematics/VFX · `⬜` STRETCH
Tractor-beam docking shot, hangar landing, core-destroyed sequence, escape debris and explosion
VFX, and the squad's launch from the hangar.

---

## 7. Landing: full sequence (CORE)

| # | Step | 🟦 Ryan (gameplay) | 🟩 Nick (presentation) |
|---|---|---|---|
| 1 | Marker appears (after waves 2 and 4) | Reveal marker; `ShowTip` / `RequestBark` | Marker visuals, bark VO, tip |
| 2 | Approach | Hyperdrive drops out in orbit; `ATDPlanetAtmosphere` trigger | Planet visual, atmosphere glow |
| 3 | Atmospheric entry | Trigger → `TravelTo(Verdance)` after 2.5 s | Heat plasma, shake, roar, white-out that hides the load |
| 4 | Breakthrough | Spawn in atmosphere flight mode at `PlayerStart_Descent`; wingmen in formation | Cloud-burst VFX, leftover plasma |
| 5 | Descent flight | Atmosphere flight, altitude data | Canyon and waterfall vista, wind audio |
| 6 | Land | Pad prompt → auto-land (gear and canopy state) | Gear and canopy animations, landing dust, thrusters |
| 7 | Exit ship | Hand-off to first person | Exit sequence, arms montage |
| 8 | Objectives | ENG-24–26 | ART-06/08/09, reward screen |
| 9 | Board ship | Allowed at any time (the player may fly around the planet); the board point is marked as the objective, and the way back to space opens, only once the planet's objectives are done | Enter sequence |
| 10 | Take-off and exit | Lift-off, ceiling → `TravelTo(Galaxy, "<Planet>_Orbit")` | Take-off shot, plasma burst |

## 8. Boarding: full sequence (WANTED since Nov 13; story acts 4 and 5)

| # | Step | 🟦 Ryan | 🟩 Nick |
|---|---|---|---|
| 1 | Defenses down (ENG-18) | Docking bay opens instead of the space-only reactor ending | Bay-door animation, beacon light |
| 2 | Docking approach | Approach cone, tractor-beam control reduction | Tractor-beam VFX, cinematic camera |
| 3 | Hangar landing → step out | `TravelTo(DominionInterior)` → first-person hand-off | Hangar shot, doors seal, red alarm, bark |
| 4 | Interior → core | ENG-30 | ART-15 |
| 5 | Escape run → launch → escape flight | ENG-31 | ART-16 |
| 6 | Breakup → Victory | `TravelTo(Victory)` | Breakup sequence (shared with the core ending) |

---

## 9. Integration points

| # | Integration | 🟦 Ryan delivers | 🟩 Nick delivers |
|---|---|---|---|
| I-1 | Presentation hooks | Data-asset classes, spawn-at-event code, exposed animation state | Data-asset instances + VFX, SFX, meshes and animations |
| I-2 | HUD and menus | `ITDHUDData`, `ITDGameFlow`, events | All widgets |
| I-3 | Maps | Maps, gameplay actors, collision, spawns, and since Nov 13 dressing, lighting and post-process | Audio volumes |
| I-4 | Waves → music and barks | `OnWaveStarted` / `OnWaveCleared`, `OnBarkRequested` | Music director, bark queue, VO |
| I-5 | Abilities | Unlock logic + `OnAbilityUnlocked` | Reward screen, ability VFX and SFX |
| I-6 | Travel and landing | Travel, atmosphere flight, auto-land | Loading screen, entry, landing and take-off presentation |
| I-7 | Director's Cut (C-03 #1–#3) | Switch-cut timing, highlight events + slow motion, scene/take restarts | Cut transitions and name cards, director cams, slate cards and "CUT!" |
| I-8 | Packaging | Build pipeline, performance profiling | Art budgets, scalability settings |
| I-9 | Submission | Packaged build, upload, form | Video, page, screenshots, dev log |

**Integration ritual** (whenever either of you lands something big): pull → play from the main menu
→ package → log bugs in §11 → each bug goes to the owner of the file it lives in.

---

## 10. Dependency order (no dates)

Both of you run in parallel. This shows **what unblocks what**, not a schedule.

```
C-01 Project ─► C-02 Contracts (+ presentation hooks) ─┬─► 🟦 ENG-01 Flight ─► ENG-04 Ships ─► ENG-05 Weapons
                                                       │      ─► ENG-07 Switching ─► ENG-08 Wingmen      (Ryan's start-here)
                                                       │   ENG-13 Enemies ─► ENG-14 Waves ─► ENG-18 Dominion finale
                                                       │   ENG-09/10/11 Signature powers ─► ENG-12 Abilities
                                                       │   ENG-19 Game flow ─► ENG-20 Landing ─► ENG-21/22/23/24 First person
                                                       │      ─► ENG-25 Verdance / ENG-26 Cinder
                                                       │   ENG-28 Packaging: early and continuous
                                                       │
                                                       └─► 🟩 ART-01 Style ─► ART-02 Inventory ─► ART-03 Ships (readability first)
                                                              ─► ART-07 Combat VFX + ART-10 combat SFX    (Nick's start-here)
                                                           ART-09 HUD (against the fake HUD data) · ART-05 Space environment
                                                           ART-06 Planets · ART-08 Ground visuals · ART-11 Barks
                                                           ART-12 Director's Cut presentation · ART-13 Performance art
C-03 Theme decision ─► ENG event hooks + ART-12        Boarding (wanted Nov 13): ENG-29–31 + ART-15–16
```

**Order from here (Nov 13):** the tree above is built except the planets' new worlds. What unblocks what now:
```
Grok Bot concepts (approved) ─► raw 3D ─► builders clean up + import ─► T1 / T4 / T5 sockets, exhausts, rigs ─► in game
T4 terrain shape ─► terrain materials + PBR sets ─► scatter ─► T5 course fit ─► planets replayed ─► environment builder lighting
Effects builder (tracers, explosions ─► engines, shields ─► powers) ─► T2 hit feel ─► T3 money shots + video
T2 HUD second pass ─► default HUD ─► Nick's fonts / tokens ─► on-foot HUD
Fallback sounds on main ─► ear pass (Ryan / Nicholas) ─► Nick's music + mix
Everything ─► human playthrough ─► fixes ─► packaged build ─► video + page ─► submit
```

**Biggest risks (start early):**
- Ryan: ENG-01 flight feel, ENG-07 switching (protect it), ENG-09 portals.
- Shared: ENG-20 + ART-07 landing hand-off, ENG-22 deflect.
- Nick: ART-03 hero-ship quality.
- Both: packaging (ENG-28). Package early and often.
- **Nov 13:** nobody has played the game by hand; audio is unapproved; generated 3D must be cleaned up and in game by Nov 14 noon or the fallbacks ship.

**Feature freeze: Nov 15, 12:00 PM EST.** Every major feature should be in by then; after that, only
fixes, polish, packaging and submission.

---

## 11. Bugs & requests (append-only)

Add new lines at the bottom only, to avoid merge conflicts. Format:
`[for Ryan|Nick] description — reporter, timestamp`

### Requests between Ryan and Nick
- _e.g. `[for Ryan] Add a "drift spark" VFX slot to UTDShipVisuals — Nick, Nov 13 14:20`_

### Bugs
- _e.g. `[for Nick] WBP_SpaceHUD: shield bar doesn't update after respawn — Ryan, Nov 13 18:05`_

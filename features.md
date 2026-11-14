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
[Complete-game checklist](#complete-game-checklist-nov-13-evening) · [Who is doing what next](#who-is-doing-what-next-from-claimsmd-nov-13-evening) · [Nov 13 realignment](#nov-13-realignment-status-and-plan) · 0. [The game](#0-the-game) · 1. [The split](#1-the-split) · 2. [Folder & file ownership](#2-folder--file-ownership) ·
3. [Shared contracts](#3-shared-contracts) · 4. [Shared setup](#4-shared-setup-c-xx) ·
5. [🟦 Ryan: Gameplay & Engineering](#5--ryan-gameplay--engineering) ·
6. [🟩 Nick: Art, Visuals & Player Experience](#6--nick-art-visuals--player-experience) ·
7. [Landing: full sequence](#7-landing-full-sequence-core) · 8. [Boarding: full sequence](#8-boarding-full-sequence-stretch) ·
9. [Integration points](#9-integration-points) · 10. [Dependency order](#10-dependency-order-no-dates) ·
11. [Bugs & requests](#11-bugs--requests-append-only)

---

## Complete-game checklist (Nov 13 evening)

Written by Ryan's Lead's docs agent at Ryan's request: "when every item in features.md is ✅, the game is complete end
to end; only overall polish is left". One row per beat of [`storyline.md`](storyline.md) in play order (reduced forms of
§18, §22, §25 and the reconciliation of `fbc7240`, which win over older text), with the items that deliver it. State is
the weakest state among the row's items, from `docs/eng/STATUS.md`, `CLAIMS.md` and the board; **no row is ✅ yet,
because no item is**. Pure polish is in the [AAA polish list](#the-aaa-polish-list-both-days-in-priority-order) and its
"final polish (after freeze)" group, not here.

| Act | Beat (storyline.md) | Delivered by | State | What still stands between today and done |
|---|---|---|---|---|
| Front end | Boot to main menu: New Game, Continue, Settings, Quit; menu music | ENG-19, ART-09, ART-10 | 🟨 | Styled menu implementation on main ee5dc0ff; MainMenuWidgetClass activation and a human pass with pad and mouse remain |
| Front end | New Game: difficulty, first play forces the tutorial, a later New Game offers "Skip the tutorial" | ENG-41, ENG-45, ART-09 | 🟨 | On main 6fe3e90f and played by an agent (T7): the offer shows on a completed profile and Skip lands in the Aftermath state; review open; the skip start overlaps space junk (#90 no. 10, T4); New Game difficulty (ENG-45) |
| Opening | Intro, 49 s (Nicholas kept 49 s, reconciliation 17:15), skippable, hands off to the tutorial | ENG-33, ART-11, ART-12, ART-19 | 🐞 | ENG-33 pilot names clipped (issue #49 open): the fix 3c76874c / 9fe5d373 is on main 6fe3e90f, capture check in progress (T3); five legacy `Intro_N` captions found by Nick's side (N-088, with T3); re-shoot once with the final ships and effects; intro score and narration voice |
| 1 First Contact | Tutorial battle phases 01-04 (evade, return fire, fight as Triad, heavy contact) over waves 1-2, in a destructible, solid field | ENG-40, ENG-13, ENG-14, ENG-36, ENG-42, ENG-48 | 🟨 | Waves 1-2 and the `TB_*` lines on main 6fe3e90f, played by an agent; the phase prompts (EVASIVE MANEUVERS, TARGET AND FIRE, SQUAD SYSTEMS, HEAVY CONTACT) are not built: T3 next; three checkpoints, the gunship optional |
| 1 First Contact | Withdrawal: Command orders it, pursuers stop at a boundary; crossing it ends the tutorial | ENG-40 | 🟨 | On main 6fe3e90f (00e23e85), played by an agent (T7): boundary, marker, pursuers break off, one completion event, music fade on the crossing; the objective still reads CLEAR THE AREA (T5); 'turn back' warning in the corridor (#90 no. 9, T1 and T4) |
| 1 First Contact | *Aftermath* (`S2_*`), tactical lanes, navigation pulse, Dominion light row in rhythm | ENG-42, ENG-36, ART-18 | 🟨 | Fired on the withdrawal event, on main 6fe3e90f (13623956); lost when the player boosts into Verdance (#90 no. 6): T3's replay 2b6be288 is in batch M, T1's atmosphere-entry hold in progress; lanes, pulse and light row still new |
| 1 to 2 | Jump to Verdance in one hyperdrive jump; convoys that ignore the squad; Dominion visible behind its field; asteroids and junk shootable and solid | ENG-15, ENG-39, ENG-18, ENG-48 | 🟨 | One-jump fix on main 6fe3e90f (030561f9), played by an agent (5.1 km to a 1.7 km stand-off); convoy line on proximity, not a hold; space composition in seven regions (T6, `docs/eng/SPACE-DESIGN.md`) |
| 2 Verdance | Descent with no loading screen, arrival from the entry point; landing; step out | ENG-38, ENG-20, ART-13 | 🟨 | ENG-20's parked-ship fix is on main 6fe3e90f (f9f89594, tested), not yet seen again live; entry held during *Aftermath* (T1); stage 1 then stage 2 of ENG-38; atmosphere look per planet |
| 2 Verdance | Short route: instruments on one buried line, transponder "verified / unknown" flicker, a patrol | ENG-25, ENG-39, ENG-36, ART-17, ART-18 | 🟨 | Dressed play zone; flicker on the HUD; walkable survey station from the kit (R-043) |
| 2 Verdance | Recover the signal-analysis device → **Gravity Well**; reward screen | ENG-25, ENG-12, ART-09 | 🟨 | Played by an agent on main 6fe3e90f; the award was silent (#90 no. 2): the native announcement and the mount for Nick's reward screen are in batch M (944e1ef5, N-081); Nick's reward screen |
| 2 Verdance | Cinematic 03 *Beneath the Surface*, 40-60 s voice-over: monitor graphic, voices ("Maintain the sequence"), drone wakes blue, three pulses | ENG-42, ART-18, ART-11, ART-19 | 🟨 | Fired, on main 6fe3e90f (cef28834); its dialogue plays over the first relay wave (#90 no. 7, T5); monitor graphic (Nick's #44 PNG or T2), drone prop with blue optic, the three-pulse sound |
| 2 Verdance | Protect the data transfer, 60-90 s, three waves, Heavy disruption → **Cloak**; extraction with the device | ENG-25 | 🟨 | On main 6fe3e90f and played by an agent (transfer, Heavy stalls, secured, extraction); nothing teaches the Heavy's guard (#90 no. 3, T5); Cloak awarded silently until batch M |
| 2 Verdance | Take-off; Cinematic 04 as transmissions (`C4_*`); Verdance theme fades | ENG-42, ENG-38, ART-19 | 🟨 | Lines fired on take-off, on main 6fe3e90f (d89a0043); cut to three calls in batch M (N-087); theme wiring (N-045) |
| Between | Waves 3-4 as the interception on the Cinder route (lines only); clue: an enemy pre-empts a manoeuvre; destructible field with special-asteroid boost drops | ENG-14, ENG-42, ENG-43, ENG-48 | 🟨 | Interception lines; the anticipation clue and its bark (storyline §6) |
| 3 Cinder | Descent, short route past one patrol, survey station on the dig's rim | ENG-26, ENG-38, ART-17, ART-18 | 🟨 | Walkable station and dig cabin from the kit; fixed order terminal → excavation → anchor |
| 3 Cinder | Cinematic 05 *The Pattern*, 40-50 s: the one Kato echo, "Origin Core", "temporal device" | ENG-42, ART-11 | 🟨 | Fired, on main 6fe3e90f (cef28834); approved lines (§25.2) as one queued scene with Cinder overlay |
| 3 Cinder | Secure the excavation: gate, areas, Warden (mercy tiers), crystal cache → **EMP Pulse** + crystals | ENG-26, ENG-23, ART-04, ART-08, ART-18 | 🐞 | ENG-23: crew die standing and slide (#90 no. 1, fix in batch M); the conduit marker's bearing crosses lava (#90 no. 11, T5); renames; cache prop (#53); Warden animation set |
| 3 Cinder | Disable the field anchor: bridges, three charges, run from the collapsing tower; HUD "approach prepared, passage pending" | ENG-26, ENG-36, ART-18 | 🟨 | Anchor tower (#52) and its collapse; pending marker |
| 3 Cinder | Take-off; Cinematic 06 as transmissions (`C6_*`), objective "Break the Dominion guard" | ENG-42, ENG-18 | 🟨 | Lines fired on take-off, on main 6fe3e90f (d89a0043); cut to three calls in batch M (N-087); objective line on wave 5 |
| 4 Assault | Wave 5; the approach field opens only when Cinder is done **and** wave 5 is cleared; debris and junk destructible and solid | ENG-14, ENG-18, ENG-48 | 🟨 | Check the rule really needs both; field-opening effect |
| 4 Assault | Four generators 0/4, "shield" wording, two Unknown clue fragments, one friendly-status flicker; docking marker | ENG-18, ENG-36, ENG-42 | 🟨 | 0/4 to 4/4 with director shots and lines played by an agent on main 6fe3e90f; generator pips on the HUD and hull r2 sockets in batch M (cda0a770, b59423d5): live captures of the pips and the flicker (T2); wording |
| 4 Docking | Bay opens, tractor assist, wingmen hold outside, pressure off; Cinematic 07 as six lines | ENG-29, ENG-42, ART-16, ART-18 | 🟨 | Docking not verified by the playtest (35 degree cone, #90 no. 19, T4); `C7_*` lines; bay mouth (#54); docking shot |
| 4 Interior | Hangar (access granted), deflect corridor, arena, Warden, consoles climbing from Unknown to the plain statement | ENG-30, ART-15, ART-17 | 🟨 | Played by an agent on main 6fe3e90f; the pilot spawns against the parked Rift's tail (#90 no. 12, T5); crew hard to read past 15 m (#90 no. 14, batch M); console order and speakers; arena consoles quiet in combat |
| 4 Interior | **The reveal:** mandatory final record at the reactor threshold → Cinematic 08 (five `F_*` lines, full warning, the case, the decision), door sealed, skippable | ENG-30, ENG-42, ART-11, ART-19 | 🟨 | The gate (T5), the queue and skip (T3), the readout line (6c4979f2, 64946350, 6828207b) are on main 6fe3e90f; agent-played on the batch L branch, but T7's run on the merged batch skipped the gate by a debug move: no verdict on the reveal yet |
| 5 The Choice | Reactor fight: "Break the reactor conduits" 0/4, "Destroy the reactor heart", one source of pressure | ENG-30 | 🟨 | CONDUITS 0/4 and the conduit lines played by an agent on main 6fe3e90f; the conduits carry no markers (#90 no. 4, T5 and T2); defenders trio (N-052, T5); `G8_*` lines; pressure in the room |
| 5 The Choice | Escape run 90 s (2.5 s after the heart), debris, "time failing" in sound; reboard | ENG-31, ART-16, ART-19 | 🟨 | `G9_*` lines; alarm and hull sounds; escape music on the run's start |
| 5 The Choice | Escape flight 60 s through debris to the radius; three "clear" reports; breakup; debris solid, impact damage by speed | ENG-31, ENG-44, ART-07, ENG-48 | 🟨 | The line says ninety seconds, the flight runs 60 s and is cleared in about 6 (#90 no. 8, T4); breakup effects at trailer quality; regroup lines |
| Ending | Victory panel (takes per scene), stinger at 18 s "DATED DECADES AHEAD", signal lines, credits roll, back to menu | ENG-44, ART-09, ART-11, ART-19 | 🟨 | Caption and forced stinger on main 6fe3e90f (6c4979f2, 6bfc19fa), seen by an agent with the credits roll and the return to the menu; credits music waits on the N-082 bridge (T1 aee545e4, batch M) and Nick's hookup |
| Always | Death → "CUT!" → next take in every scene, never replaying a cutscene | C-03, ENG-19, ENG-46 | 🟨 | ENG-46 is 🟨: retake checked in every scene of the new beats |
| Always | Save and Continue at every act boundary, including interior and escape; no Continue after victory | ENG-19, ENG-46 | 🟨 | ENG-46 is 🟨: Continue from the interior and the escape played once |
| Always | Fallback ending (`td.Finale.Boarding 0`) still finishable | ENG-47, ENG-18 | 🐞 | ENG-18: the space reactor could not be killed in a played run (#90 no. 5); T4's fix 818d592e is in batch M; ENG-47 is 🟨: the decision record is the Lead's (due Nov 14 20:00) |
| Always | Barks per `dialogs.md`, subtitles, voices | ENG-43, ART-11 | 🟨 | Bark table from the file; ~53 new triggers; voice pickup |
| Always | Sound for every action; music per act (menu, tutorial, combat rising, Verdance, Cinder, assault, interior, escape, victory, credits) | ENG-35, ART-10, ART-19 | 🟨 | Wiring the delivered tracks; Cinder and assault cues; ear approval |
| Always | HUD in every state; Settings and accessibility | ENG-36, ENG-32, ENG-45, ART-09 | 🟨 | On-foot HUD in the new style; subtitles and shake options |
| Always | Ships, fleet, Dominion, crew, props, effects, environment, animation at gameplay distance | ART-03, 04, 05, 06, 07, 08, 13, 17, 18, ENG-34, 37, 39 | 🟨 | Per item below |
| Ship it | Packaged Shipping build under 1 GB, 60 fps at 1080p, played start to end | ENG-28, ENG-46 | 🟨 | Shipping package of main 6fe3e90f published (private prerelease `playable-main-6fe3e90f`, 0.94 GB; menu, intro and flight reached by an agent, no human); performance pass (#89, T7); size trims (`docs/eng/BUILD-SIZE.md`); daily packaged playthrough |
| Ship it | Independent review of every lane, human playthroughs, submission page, video, dev log, credits | ART-20, ART-21, ENG-46, ART-14 | 🟨 | Batch L range 825d8869..6fe3e90f sent to Nick's side for review (R-045, accepted on #88); T7 ran Nick's seven scripts on batch L (log on eng/t7), script 08 for batch M next; Glass Box draft on main (a980fa7f); itch page; trailer re-cut |


**Hourly alignment, Nov 14 01:00 EST (Ryan's Lead):**
- **On main:** batch L at 6fe3e90f (Nov 13, about 23:55; full suite 870/870). Played front end to victory by an agent
  (T7, through Unreal's MCP server; some stages reached by labelled debug moves; no blocker; the log is
  `docs/eng/PLAYTEST-2026-11-13-T7.md` on eng/t7, in batch M). A Shipping package of it is published as the private
  prerelease `playable-main-6fe3e90f` (0.94 GB; menu, intro and flight reached by an agent). **No human has played it,
  so nothing is ✅.**
- **Not on main:** batch M (lead/int-m 0c698327) is in its gate; one test failure (T5's crew-death test) is being fixed.
  Work that is only in batch M or on an eng/* branch counts as 🟨.
- **Markers changed this hour:**
  - ENG-15 🐞 → 🟢: the one-jump fix 030561f9 is on main and was played;
  - ENG-11 🐞 → 🟨: the dome fix 82d6040d is on main with its test, not yet seen again in a running game (T7, batch M
    playtest);
  - ENG-20 🐞 → 🟨: the parked-ship fix f9f89594 is on main with its test, not yet seen again; entry hold (T1) and the
    board prompt (T5) open;
  - ENG-18 🟢 → 🐞: the space-reactor fallback was unfinishable in a played run (#90 no. 5; T4's 818d592e in batch M);
  - ENG-23 🟢 → 🐞: crew deaths (#90 no. 1, fix in batch M) and the Heavy's guard (#90 no. 3, T5);
  - ENG-47 ⬜ → 🟨: the fallback fix is built; the decision record is the Lead's;
  - checklist rows: "Jump to Verdance" and "Descent" 🐞 → 🟨; "Secure the excavation" 🟨 → 🐞; "Fallback ending" ⬜ → 🐞.
- **🐞 now:** ENG-18 (fallback reactor), ENG-23 (crew deaths, Heavy guard), ENG-33 (issue #49 open: the fix is on main,
  T3's capture check in progress). A 🐞 on an item marks only the checklist rows its bug touches: ENG-18's is the
  fallback row, not the assault rows.
- **Open findings are tracked in issue #90** (20 rows, each with an owner); none is a BLOCKER on the boarding route.
- **Machine:** it crashed at 00:30 (GPU scheduler bugcheck: too many rendering Unreal instances) and everything was
  restarted. New rule: at most three rendering Unreal processes machine-wide.
- **New lanes (Ryan, before sleeping):** performance (issue #89, T7); the bug tracker (issue #90); space composition in
  seven themed regions (T6, `docs/eng/SPACE-DESIGN.md`, T4 reviews); planets as realistic as possible, with three story
  zones each (environment builder). Ryan is asleep until about 10:00 EST; his Lead decides overnight.
- **Open work with no owner in the 01:00 lists below** (the Lead assigns these next): ENG-45's colour-blind check and
  New Game difficulty; ENG-43's pre-empt clue and the remaining bark triggers; ENG-38 stage 2; ART-03's squad ships
  after the batch M versions (Rift v12, Lancer v05, Bastion v01), now that ship builder 2 is on the enemy fleet.
- **Nick's side:** every item of theirs is 🟨 from their own reports (`docs/visual/STATUS.md`, #88); no marker of theirs
  changed this hour.

### Contradictions that need a human (Ryan or Nicholas)
1. **Resolved (Ryan, Nov 13 evening): the transfer defence gives Cloak; the relay climb is dropped** (T5, ENG-25; the structures builder rebuilds the relay as a defence platform). Was: **Verdance objective 2.** The build has a separate relay climb with resonance nodes (→ Cloak); the story (§25.1, §25.2
   18-2) makes the relay the station's relay and the transfer defence its objective (→ Cloak); §18 had put the transfer on
   objective 1. **Ryan:** keep the climb, or make the transfer the Cloak objective and drop the climb.
2. **Opening length.** ENG-33 says "redo to the 2:20 *First Contact* brief"; the reconciliation (17:15, #29, #24) says
   "the opening stays 49 seconds". **Ryan:** confirm the redo is a re-shoot of the 49 s with First Contact's content.
3. **Field and shield.** §0, §5 act 4, ENG-18 and the `A3_PassageOpen` line still treat the generators as the field and
   the anchor as opening it; §25.2 splits them (approach field opens on Cinder + wave 5; four generators hold a separate
   shield). Nicholas's documents decided it; corrected below in §0 and ENG-18.
4. **COMMAND or Control** as the authority's name; **who boards** (lines written for Wren); the **three-pulse** definition;
   the ending **caption** wording. **Nicholas.**
5. ~~**AGENTS.md**~~ **Resolved (Ryan's Lead, Nov 13):** AGENTS.md §1, §2 and §11 aligned in the same push: the boarding
   finale is the wanted ending, and Cinder's anchor opens the approach ("passage pending").
6. **ART-15 / ART-16** sat in Nick's section marked STRETCH; §1 lists them as Ryan's since Nov 13. Corrected below.
7. **Music hookups** were T7's (CLAIMS, N-045); Ryan's Lead moved them to Nick's side on Nov 13 evening (ART-19), T7
   as fallback. **Nicholas** to accept on the board.
8. **Escape "changed route"** and the interior's shaft, security and archive rooms are assumed by Nicholas's documents and
   not built; reduced forms proposed in `docs/story/story-0708-actions.md` (b1, b17). **Nicholas** to accept.
9. **Resolved (Ryan, Nov 13 evening): voices over a sealed threshold (no camera takeover); the reactor fight adds only a telegraphed sweeping laser, with no perfect-deflect gate; future-crew defenders approved (T5; the drones stay as the fallback; #29).** Was: **Cinematic 08 form** (voices over a sealed threshold, or a control-away scene) and **ENG-30's extras** (ring timing,
   sweeping laser, acceleration; the documents forbid a perfect-deflect gate). **Ryan.**
10. ~~**Power-ups in space**~~ **Resolved (Ryan, Nov 13):** Ryan approved pickups ("yes ... just go now"; T2's
    `UTDPickupSubsystem` is built) and asked for rewards from destroying asteroids (ENG-48). The galaxy map (ART-09,
    ENG-02) is not required (decided by Ryan's Lead under Ryan's delegation); no web build.

## Who is doing what next (from CLAIMS.md, Nov 13 evening)

**Ryan (Nov 13 evening): "You are the lead of everything"; Nick's agents may be given more work, not art-heavy** (his
machine is lighter). Everything heavy (Niagara, Blender, cooks, terrain, big builds) stays on Ryan's side.

Updated Nov 14 01:00 EST (Ryan's Lead; Ryan's side rows only).

| Owner | Now | Next (in order) |
|---|---|---|
| **Lead** (Ryan's) | Batch M gate (lead/int-m 0c698327; one test fix pending from T5), then main and its Shipping package; hourly features.md alignment | ENG-47 decision record (the space-reactor fallback is finishable with T4's 818d592e, in batch M; due Nov 14 20:00); owners for the open work listed in the hourly alignment |
| **T1** flight, input, travel | Bridge answers and extra events for Nick's audio (N-082, N-089: fight start and end, escape) | Hold the atmosphere entry during *Aftermath*; `-Audio` check of the descent roar; boundary-warning suppression in the withdrawal corridor and the climb-out shot |
| **T2** combat, powers, HUD | Reactor conduit count (CONDUITS n / 4) | Escape clock and direction marker; drone thruster playback; live captures of the generator pips and the transponder flicker |
| **T3** squad, director, story | ENG-40 tutorial phase prompts | The Story Director's wording corrections; #49 capture check; a played Continue test; the TransmissionStatic cue |
| **T4** enemies, waves, world | Escape flight (its time matches the line) | Docking cone; skip-start overlap with space junk; stuck-enemy re-route; space design review (T6) |
| **T5** flow, ground, interior | Crew-death test fix (blocks batch M) | Heavy guard hint, clang and chip-through; Cinematic 03 holds the first relay wave; conduit markers; Cinder lava route; defenders trio (N-052); dressing Dress lists; artefact and survey line placement |
| **T6** space field | Space composition, seven regions (`docs/eng/SPACE-DESIGN.md`), the Shepherd Belt first | Rock field performance budget; staged large and special breaks |
| **T7** profile, playtest, performance | Performance pass (issue #89) and the N-085 audio underrun check | Nick's batch M playtest supplement (script 08), with a look at the ENG-11 dome and ENG-20 parked ships |
| **Ship builder 2** | Enemy fleet close-range pass (ART-04) | - |
| **Effects builder** | Ice breaks seen on real rocks | First-ten-minutes effects review; reactor effects |
| **Environment builder** | Pass 4: real tree canopy, Cinder slope | Realism, then three story zones per planet |
| **Structures builder** | Space set pieces: patrol hull, expedition hulls, survey rig, hollowed rock | - |
| **Animation builder** | Side-camera kill captures | Warden ragdoll cases; trooper rifle clips in a fight |
| **Import builder** | The never-cook list, proven by a cook | Asset ledger (ART-02, `docs/eng/ASSETS.md`); ENG-35 fallback gaps (b8e561cd, batch M) |
| **Packaging agent** | Shipping package after each batch (6fe3e90f published) | Matched reward build for N-081 after batch M |
| **Grok Bot** | R-042 queue; PRs #91 (death clips) and #92 (enemy attacks) open for the Lead | #52 anchor tower, #53 crystal cache, #40 set dressing, #54 bay plans; R-043 measured plans and trim sheets; space set piece concepts |
| **Nick: Lead** | ART-10 cue pickup (DominionInterior01, CreditsMusic01, VerdanceTheme01); #71 menu handoff | ART-14 page, dev log, credits audit; ART-20 review dispatch |
| **Nick: Audio** | ART-10 mix and listening; tutorial music | ART-19 wiring; Cinder, assault, escape and victory cues; three-pulse and "Don't-" asset |
| **Nick: UI** | Victory, reward and credits read; Settings focus; loading | ART-09 screen list below; ENG-36 fonts and tokens |
| **Nick: Story Director** | Voice pickup sheet (C08 first) | ART-11 words for every new id; answers to contradictions 4 and 8 |

**Nick's side: queue** (assigned by Ryan's Lead, Ryan, Nov 13; ordered by what unblocks the game soonest; Nicholas
decides his own art, audio and story direction within each):
1. **Story answers and words** (ART-11): COMMAND or Control, who boards, the three-pulse definition, the caption; then
   approved text for the ~28 new ids of 07-09 and the `C4_`, `C6_`, `G6_` sets. T3 seeds drafts; his rows replace them.
2. **Music and cue playback wiring** (ART-19): credits, Dominion interior loop, Verdance theme, tutorial, then descent,
   Cinder, assault, escape and victory cues, on the events Ryan's side fires. T7 is the fallback.
3. **Independent code reviews** (ART-20) of every lane that lands, so items can reach 🟢.
4. **UI screens** (ART-09): pause, reward, victory with takes, credits roll, loading, skip-tutorial prompt, subtitle and
   accessibility options, objective panel.
5. **Submission** (ART-14): itch page text, dev log write-up, `CREDITS.md` and tools audit, trailer shot list and edit.
   Also the asteroid and junk break and impact sounds (ENG-48).
6. **Playtest checklists and per-act test scripts** (ART-21).
7. **Voice** (ART-11): the approved pickup sheet, then barks.

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
**(Nov 13 evening)** Ryan has since played it (a guided tour, a free session, the ship and walkability findings of R-041
and R-043); Nicholas has not. No full human run start to end exists yet: ENG-46 holds the gates.

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
| Menus, reward screen, credits, loading | ART-09 | 🟨 | Engineering fallbacks work; styling is plain. Nick's styled menu is on main (ee5dc0ff); its activation is pending |
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

### Verdance in the story (Nicholas's four documents, storyline.md §14-18)
**Decided by Ryan, Nov 13 (14:43 EST): Verdance is built in the reduced form, about 8-10 minutes in all.**
Details per question: `docs/story/verdance-scope-questions.md`.

| Part | Reduced form (what gets built) | Owner |
|---|---|---|
| Arrival and route (3-4 min) | The hyperdrive jump, the descent (ENG-38), a landing and a short route to the station. The cheap clue beats are kept: convoys that ignore the squad carry scanning gear, instruments measure one buried line, a transponder marker flickers | T4 (space and clues), T1 (descent), T5 (route) |
| Cinematic 03, *Beneath the Surface* (40-60 s) | Voice over the room with a monitor graphic; the dormant drone wakes and glows blue. Built in the same pass as the opening and *Aftermath* | T3, with T2 (monitor graphic) |
| Facility defence and extraction (3-4 min) | Objective 1's hold-interact becomes a 60-90 s archive transfer with a progress meter and three fixed waves; the transfer pauses while an enemy stands at the terminal; the third wave brings a Heavy as the disruption; the existing carry-back is the extraction with the data package. No new enemy AI, no enemy that damages the terminal | T5, with T2 (meter) |
| Cinematic 04, *Verdance Departure* (30-45 s) | Three transmissions over the take-off, or a short voice-over shot if the cutscene pass has time | T3 |
| The outpost | One open-sided survey station re-dressed from the wreck site, holding the console and the dormant drone. Not a walk-in facility | Environment / prop builder, T5 fits it |
| Cinder shielding | Words only; no mechanic | T3 (lines) |
| Co-op | Not in the game; the documents are read as solo | - |

**Unchanged:** both Verdance objectives and both rewards (Gravity Well, Cloak), waves 3-4 between the planets, the
20-30 minute target for the whole game, and the rule that nothing reveals the twist before the Dominion.
**With Nicholas (story words, issue #29):** objective 1 takes the new fiction (intact station, survey archive, buried
structure); what the relay (objective 2) means now; what opens the distortion field on Cinder now that the survey
station replaces the mining complex; a replacement for Kato's "Time to go." (the story test rejects " time" before
act 4).

### Cinder in the story (Nicholas's three documents, storyline.md §19-22)
**Decided by Ryan, Nov 13 (15:06 EST): Cinder is built in the reduced form, about 6-8 minutes from atmosphere to
take-off.** Details per question: `docs/story/cinder-scope-questions.md`.

| Part | Reduced form (what gets built) | Owner |
|---|---|---|
| Approach in space (no added length) | Waves 3-4 stay between the planets and are told, by lines only, as the interception on the Cinder route. Transports that ignore the squad fly past (the convoy lane shared with Verdance). No scan or route-marking action; the dig's glow shows from orbit | T4, T3 (lines) |
| Descent and route to the station (1.5-2 min) | The descent (ENG-38), the landing, then a short marked route past one existing patrol to a survey station on the rim of the dig. One echo of a Kato line | T1, T5, T3 |
| The survey station | One open-sided station re-using the Verdance station kit, holding the terminal; not a walk-in facility. The terminal is usable once the patrol is dead or left behind | Prop builder, T5 |
| Cinematic 05, *The Pattern* (40-50 s) | Voice over the terminal with the Verdance monitor graphic plus a Cinder layer; the echo; the two markers. Built in the single cutscene pass, ranked below the opening and Cinematic 03 and above Cinematic 04. Fallback: five transmissions | T3, with T2 |
| Secure the excavation, recover the crystals (2-2.5 min) | Today's outpost objective renamed: gate, the areas, the Warden, and the claim becomes the crystal cache, awarding EMP Pulse as now. No new enemy AI | T5 |
| Disable the field anchor and extract (1.5-2 min) | Today's jammer objective renamed: bridges, three charges, and the run from the collapsing tower is the return to extraction. A corridor marker appears on the HUD at shutdown | T5, with T2 |
| Order | Fixed: terminal, then excavation, then anchor (today the two objectives can be done in either order) | T5 |
| Departure | Two or three transmissions over the take-off (Cinematic 06 is not written) | T3 |
| Wave 5 and the field | Wave 5 stays after Cinder. **One rule: the distortion field opens when Cinder is finished and wave 5 is cleared**; no new corridor geometry. The four generators are still the Dominion's shield | T4 |
| Heat or shielding, co-op | No mechanic; solo | - |

**Unchanged:** both Cinder objectives, the Warden and its mercy tiers, the EMP reward, waves 3-5, the 20-30 minute
target for the whole game, and the rule that nothing reveals the twist before the Dominion.
**Correction to section 18:** the Cinder survey station sits at the mining complex; it does not replace it.
**With Nicholas (story words, issue #29):** one sentence each for what the crystals, the anchor and the four
generators do to the field; where "the signal data" comes from now (the relay?); which echo lines are used (his
single Kato echo, our three placeholder `A3_Echo*` lines, or both); where the EMP comes from if not the crystals;
who wants the crystals (the machines, or Triad); whether "Origin Core" is spoken on Cinder.

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

### Worlds that read as real places (Ryan, Nov 13 evening): ENG-39
**Ryan's direction (16:19 EST).** The planets and space must stop looking like one dressed spot in an empty map.
- **Planets: one richly dressed play zone each, not a dressed globe.** Wherever the player enters the atmosphere,
  from any side of the planet, the arrival is routed to the same zone around the landing pad and objectives, with a
  different approach corridor and first view per side. Inside the zone it must be fully true: several ground
  materials, lots of trees and rocks, buildings and points of interest in every direction the player can look or fly.
  The rest is disguised so the player never notices: haze, the cloud deck, the curvature bend, ring terrain, cheap far
  ground, and the flight-zone turn-back.
- **Space: filled everywhere, generated continuously as you fly.** Asteroids, debris, wrecks, moving bodies, other
  ships and power-ups, in streaming cells around the player, deterministic from a seed; no single dense area with
  emptiness around it. Combat lanes stay readable. **Power-ups are new gameplay**: the smallest good version is
  proposed to Ryan before it is built.
- **Atmosphere entry and exit (ENG-38):** both directions get the piercing-the-atmosphere sequence: motion blur,
  streaks, debris and embers past the camera, a heat sheath on the hull, the cloud punch.
- **Plan and findings:** `docs/eng/WORLD-DENSITY-PLAN.md` (a fresh reviewer's audit; Ryan's Lead leads the review).
  **Owners:** T4 (terrain scatter, points of interest, the space field, the arrival mapping), the environment builder
  (materials, fog, sky, the edge disguise), the effects builder and T1 (entry and exit), Grok Bot (set pieces from
  Nicholas's reference boards), T2 (pickup HUD), T5 (keep-out areas on foot).

### Ship quality (Ryan, Nov 13 evening): see [docs/eng/SHIP-QUALITY.md](docs/eng/SHIP-QUALITY.md)
Ryan played the build and found the squad ships "not smooth" and below AAA, and Rift's ring empty. The raw
image-to-3D meshes are being **rebuilt cleanly in Blender over the Rodin design** (exact symmetry, bevelled hard
edges, modelled panel lines, rebuilt nozzles, guns, canopy and ring, re-baked textures), **Rift first**, shown to
Ryan before Lancer and Bastion. Meanwhile the ship builder adds a living portal in Rift's ring and an engine-side
close-range pass on all three; Grok Bot regenerates sharper sources. Same size, sockets and collision: gameplay does
not change. Old meshes are kept.

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

**Final polish (after freeze) and optional extras** (added Nov 13 evening; none of these is needed for the game to be
complete, so none is a build item; do them only when every row of the complete-game checklist is 🟢):
21. Banter exchanges (`dialogs.md` §9) and second and third line variants per bark id (T3, Nick).
22. "Time failing" visuals on the escape (afterimages); sound alone is the build item (ENG-31; Nick, effects builder).
23. A picture on the Cinematic 08 console (the opening's three ships) or a letterbox while the `F_*` lines play (T3).
24. Hangar "familiar architecture" dressing: route stencil, equipment rack, Triad-like unit markings (environment builder).
25. ENG-38 stretch: land anywhere, volumetric clouds, airbrake animation, bigger globes (T1, T4, effects builder).
26. A changed escape route, a service shaft, and separate security and archive rooms in the interior (T5, structures builder).
27. A replayable ending and a recording replay log (T3).
28. The galaxy map screen (ART-09, ENG-02; its key is hidden today).
29. Persistent battle damage on the ships after Verdance (Gameplay 03 asks "if possible").
30. Second skins and damage variants beyond the insignia clue (ART-18).
31. **Three genuinely different endings** (Nicholas's idea, N-052; **Ryan, Nov 13 20:55: "Yes, if time allows"**). This is
    the first optional extra to take once the checklist rows are 🟢. The current ending stays the submission baseline
    until all three are built, tested and played.
    - **Design:** Nicholas decides what differs (the choice, the outcome, the final lines, the caption). Ryan's Lead
      proposes cheap levers: the boarding pilot, the reactor-fight result and the escape outcome.
    - **Owners:**
      - T3: the ending sequences, captions and lines by id;
      - T5: the choice hook and the state that decides the ending;
      - Nick's Story Director: the words.
    - **Done when:** each ending is reachable in a normal play, has its own sequence and caption, saves and Continues
      correctly, and the journey test covers all three.

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

**(Nov 13 evening, from storyline.md §25.2, §32 and the `fbc7240` reconciliation; these win over the diagram above):**
- Act 1 is **one tutorial mission** over waves 1-2 that ends when the squad crosses the withdrawal boundary (the gunship
  may live), then *Aftermath*; nothing is fought between *Aftermath* and Verdance (ENG-40, ENG-42).
- Verdance order: arrival → recover the device (**Gravity Well**) → Cinematic 03 → protect the transfer / relay copy
  (**Cloak**) → extraction → Cinematic 04 as transmissions. Cinder order is fixed: survey terminal (Cinematic 05) →
  excavation, Warden and crystal cache (**EMP Pulse**) → field anchor → extraction.
- The anchor only **prepares** the approach. The **approach distortion field opens when Cinder is complete and wave 5 is
  cleared**; the four generators hold the Dominion's **separate shield**, and all four down open the docking bay.
- Inside, the reveal is the **mandatory final record** after the Warden, then Cinematic 08 at the reactor threshold;
  the thing destroyed is the **reactor** (the Origin Core is what the machines seek and is never shown aboard).

---

## 1. The split

| | 🟦 **Ryan: Gameplay & Engineering Lead** | 🟩 **Nick: Art, Visuals & Player Experience Lead** |
|---|---|---|
| **AI assistant** | Claude Code | OpenAI Codex |
| **Mission** | Own the playable game systems, engineering integration and technical reliability, so the game stays fun, responsive and shippable | Own the visual identity, presentation quality and player-facing feel, compatible with the gameplay foundation and the 4-day window |
| **Owns** | Flight and controls; three-ship switching and wingman AI; weapons, damage, shields, enemy AI, waves, targeting, balance, signature and unlockable power *mechanics*; objectives, encounter pacing, hyperdrive and level transitions, mission flow, win/lose and restart; first-person gameplay on the planets (character, blade combat, crew AI, objectives); C++ architecture and interfaces; profiling, testing, packaging, critical bugs | Visual direction; ships, enemies, Dominion and prop *visuals*; space and planet environments, lighting, atmosphere, materials; all VFX (Niagara); HUD, UI/UX, menus, onboarding, ship-switch feedback, targeting clarity; all audio and music; character and ship animation polish; cinematics and the Director's Cut presentation; performance-minded art; trailer and store page |
| **Feature IDs** | `ENG-xx`, and since Nov 13 `ART-02, 03, 04, 05, 06, 07, 08, 13, 15, 16` (all of Environment, VFX and Asset Management) | `ART-01, 09, 10, 11, 12, 14` |
| **Added Nov 13 evening** | `ENG-39` to `ENG-48`, `ART-17` (walkable kit), `ART-18` (story set pieces) | `ART-19` (music wiring), `ART-20` (independent reviews), `ART-21` (playtest scripts), assigned by Ryan's Lead (Ryan, Nov 13) |

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

#### ENG-11 · Bastion: Missile Swarm & Shield Dome · `🟨`
- **Missile Swarm (tap):** 12 micro-missiles split across painted locks (or the soft target),
  18 damage each with a 200 cm splash; 9 s cooldown.
- **Shield Dome (hold 0.5 s):** a 3000 cm radius for 6 s with a 16 s cooldown.
  - Squad ships inside take no damage and regenerate shields twice as fast.
  - Enemy shots are destroyed on the dome, and missiles are deflected back.
  - The dome follows Bastion slowly.
- **(Nov 14 01:00) State:** the dome-from-inside fix (T2, 82d6040d: the placeholder rings hide while the camera is
  inside) is on main 6fe3e90f with its test. Not yet seen again in a running game: T7 looks at it in the batch M
  playtest; 🟢 once it is.

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
- **Still missing (Nov 13 evening, T7's playtest):** a jump toward a planet dropped out 11.4 km short of Verdance. Done
  when one held jump from the combat zone reaches each planet's orbit and the Dominion's well with no second jump, for
  all three ships (T1, queued in `CLAIMS.md`). The rift look of the jump is in ENG-37 (T7, effects builder).
- **(Nov 14 01:00) State:** the one-jump fix (T1, 030561f9) is on main 6fe3e90f and was played by an agent (T7,
  session 2: one jump from the Aftermath point, 5.1 km to a 1.7 km stand-off off Verdance). The attached streak loop
  (9ba18c84) is in batch M. Waiting for a human.

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

#### ENG-18 · Dominion assault & space finale (CORE) · `🐞`
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
- **(Nov 13 evening) Correction and story pass, Gameplay 06 and Cinematic 06 (`storyline.md` §23-25, §25.2).** The
  "distortion field" and the "shield" are two things: the **approach field** opens when Cinder is complete **and** wave 5
  is cleared (today the code opens it on wave 5's clear alone: check both are required); the four generators hold the
  Dominion's **separate shield**. With boarding on (the default now, ENG-47), the shield's fall opens the bay; it never
  exposes the reactor, starts a countdown or wins. Still missing, each checkable in a played run:
  - wave 5 objective line "Break the Dominion guard"; then "Destroy the shield generators 0 / 4" counting live on the
    HUD, destroyed markers removed; then "Dock with the Dominion" with a docking marker (T4, T2; Nicholas may reword);
  - "shield", never "field", in `A4_GeneratorDown1-4`, the all-down line and `Bark_Dom_FieldDown`; `A3_PassageOpen`
    reworded to "approach prepared" (T3, T4; Nicholas's words);
  - the `G6_*` radio lines and the two Unknown clue fragments `G6_Clue1` / `G6_Clue2` that clear up as generators fall;
    one friendly-status flicker on one hostile marker after the second generator (T3, T2); all pass the story test;
  - wave 5 starts outside unavoidable fire with the Dominion in view; attack pressure off from docking commitment to
    the travel (T4);
  - the generator money shot returns control before a turret volley and never covers the next marker (T3);
  - answer on the board whether EMP and the ram affect generators and turrets (T4; neither is a gate);
  - the assault timed on Standard (target 5-7 min including wave 5) and reported to Ryan (T4).
  Art for the beat: Dominion hull r2, the shield generator mesh, the field-opening and shield-drop effects (ART-04,
  ART-18, ART-07).
- **(Nov 14 01:00) 🐞:** the space-reactor fallback ending could not be finished with real fire (issue #90 no. 5: about
  2 min 15 s of fire to kill the reactor, the ship dies in 5 s). T4's fix 818d592e (reactor 3 x 400, turret bursts
  eased) is in batch M, not on main. The assault with boarding on was played to the bay by an agent on main 6fe3e90f
  (generators by a labelled director command); docking itself is not verified (#90 no. 19, ENG-29).

#### ENG-48 · Destructible, solid space field (CORE, added Nov 13 evening by Ryan) · `🟨`
Ryan: "all things in space, like asteroids and junk, should be destructible ... shoot it and it will explode, and maybe
you get some type of points, or more on your boost if you destroy a certain asteroid ... rewards people for destroying
asteroids. And if you hit an asteroid, the harder you hit it, the more it will do damage to your ship ... you shouldn't be
able to go through them." Applies to everything ENG-39's space field and ENG-17's layout place.
1. **Everything breaks.** Every asteroid, debris piece and junk object is shootable, with health by size tier: **small**
   1-2 shots, then gone; **medium** several shots, then splits into 2-3 smalls; **large** many shots, then splits into
   mediums. Drifting wrecks and buoys break the same way. Each break has a break effect and a sound.
2. **Rewards.** Every kill adds a small score and a scene stat (`AddStat`, shown on the victory panel). About 1 in 15
   asteroids is visibly special (glowing crystal veins) and drops a boost refill and / or a pickup through T2's pickup
   subsystem (ENG-39). Lancer's afterburner ram smashes small and medium asteroids.
3. **Solid.** No ship ever passes through an asteroid or wreck at any speed: swept collision, no tunnelling at boost speed
   or at 30 fps.
4. **Impact damage scales with impact speed.** Below about 15 m/s a scrape does nothing; above it, damage grows with the
   square of the normal speed; a full-boost head-on into a large asteroid takes most of the hull. The ship bounces off
   with a camera shake and a hit sound. Wingmen avoid asteroids (ENG-08).
5. **Performance.** Frame time holds with the field's instance counts (wave 5 with the field on, inside the ENG-28
   budget); breaks are pooled and cheap.
- **Tests:** each tier breaks in its shot count and splits as stated; the special drop fires; no pass-through at boost
  speed at 30 / 60 / 120 fps; damage at 10, 20 and full-boost speeds matches the curve; a played wave with a ram through a
  cluster.
- **Owners:** T6 (space field; lead), T1 (collision response and impact damage in the flight model), T2 (drops, score and
  stat, hit feedback), effects builder (break effects, cheap and pooled), **Nick's side (break and impact sounds,
  assigned by Ryan's Lead, Ryan, Nov 13**; ENG-35 fallbacks until then).
- Depends on ENG-39 (the field and the pickup subsystem), ENG-01 (collisions), ENG-37.

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

#### ENG-20 · Atmosphere flight, landing & take-off (gameplay) · `🟨`
See §7.
- **Atmosphere flight mode:** gravity, speed capped at 60% of space cruise, no hyperdrive,
  altitude data. Boost works in atmosphere too, capped at 60% of boost speed while boosting (Lead under Ryan's
  delegation, Nov 13, for the ENG-34 worlds); cruise and the landing rule below are unchanged.
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
- **(Nov 14 01:00) State:** parked ships are solid to the pilot (T1, f9f89594, with a test) on main 6fe3e90f; landing,
  boarding and take-off were played by an agent there. Open: walking into a parked ship not yet seen live (T7); the
  atmosphere entry is held while *Aftermath* plays (T1, #90 no. 6); the board prompt says BOARD SHIP but needs a held E
  (T5, #90 no. 13).

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

#### ENG-23 · Crew enemies: AI & combat (CORE, planets) · `🐞`
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
- **(Nov 14 01:00) 🐞** (issue #90): crew die in a standing crouch and slide off the capsule (no. 1; T5's ground deaths
  4b71cc02 are in batch M, whose gate is held by that lane's crew-death test); the Heavy's frontal guard is never
  taught and quick slashes do nothing (no. 3; T5: hint, clang cue, chip-through, in progress); crew are hard to read
  past 15 m in the dark interior (no. 14; 0b36d0e9 in batch M).

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
- **(Nov 13 evening) Still missing for the reduced form** (`storyline.md` §18, §25.1, reconciliation `fbc7240` e4-e10;
  about 8-10 min from atmosphere to take-off). Done when a played run shows, in this order:
  1. arrival through ENG-38 into the dressed play zone (ENG-39); Kato's convoy line fires on **proximity** to the lead
     hauler (no hold, no prompt, no marker); no interception force over the landing zone (T4);
  2. a short route with the instruments over one buried line and the transponder "verified / unknown" flicker on the
     HUD (T5, T2), to the **open-sided survey station** built from the walkable kit (ART-17), with the console and the
     dormant drone (ART-18);
  3. objective "Recover the signal-analysis device": the pickup awards **Gravity Well** and checkpoints (T5);
  4. connecting the device at the console plays Cinematic 03 (ENG-42), never replayed on a retake;
  5. objective "Protect the data transfer": 60-90 s, a progress meter on the HUD (T2), three fixed waves, the transfer
     pauses while an enemy stands at the terminal, the third wave brings a Heavy, a checkpoint at the midpoint, tuned for
     one player (T5); completing the relay copy awards **Cloak** (§25.1);
  6. collect the device and archive, return to the pad, take off; Cinematic 04 as transmissions (ENG-42).
  **Decision needed (Ryan):** the built objective 2 is a separate relay climb (resonance nodes, → Cloak). Either it is
  replaced by step 5, or it stays as the route to the relay console and the transfer defence follows it. Until Ryan
  decides, both objectives keep working and keep their rewards.

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
- **(Nov 13 evening) Still missing for the reduced form** (`storyline.md` §22, §25.1-25.2; about 6-8 min from
  atmosphere to take-off). Done when a played run shows:
  1. the dig's glow visible from orbit; ENG-38 descent; a short marked route past one patrol (no echo on the route,
     §25.1) to the survey station on the dig's rim (ART-17), usable once the patrol is dead or left behind (T5);
  2. objective "Access the survey records": the terminal plays Cinematic 05 with the one approved echo and the words
     "Origin Core" (ENG-42), once;
  3. the **fixed order** terminal → "Secure the excavation" → "Recover the stabilising crystals" → "Disable the field
     anchor" → "Return to extraction" (today the two objectives can be done in either order) (T5);
  4. the excavation is today's outpost renamed; the Warden guards the dig with its mercy tiers; one cache interaction
     gives the crystals **and** the EMP prototype, awards **EMP Pulse** and saves both, with Kato's approved line (T5);
  5. the anchor is today's jammer renamed; its shutdown shows a **pending** approach marker on the HUD ("approach
     prepared; passage still blocked"), never "passage open" (T5, T2);
  6. one door at the dig opens to the pilot's ID (storyline §6) (T5);
  7. take-off; Cinematic 06 as transmissions (ENG-42).
  Art: dig site #69 (walkable through ART-17), anchor tower #52 and its collapse, crystal cache #53 (ART-18), lava and
  ash effects (ART-07).

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
- **(Nov 13 evening) Still missing** (`docs/eng/BUILD-SIZE.md`, R-040). Done when, on the freeze commit:
  - the Shipping zip is **under 1,000,000,000 bytes** with the interior, the Rodin fleet and every effect in it (today's
    estimate without trims is about 987-1,020 MB): trims 1-6 applied (denoiser plugin off, MegaLights and path tracer
    shaders off, compression 9, no ARM64 installer); trims 7-9 (texture RDO, 4K sky, ray tracing) only after a human look;
  - `L_Dominion_Interior` is in the cook list and every asset reached only by a path string is cooked (AGENTS.md §11
    "What ships"); the debug menu and dev cheats are stripped;
  - 60 fps at 1080p on the mid-spec target (and no hitch over 50 ms; none over 100 ms across the hidden descent load)
    measured in the packaged build in: wave 5, the assault, both planets' play zones, the descent, the interior arena,
    the escape flight; numbers logged in `DEVLOG.md`;
  - the packaged build is played start to end once a day (ENG-46) and the final zip is named, uploaded and its SHA-256
    recorded (with ART-14).
  Owner: Lead (packaging builder), T4 (frame rate in space and on the planets), T5 (interior).

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

#### ENG-33 · Cinematic intro (CORE, added Nov 12 by Ryan) · `🐞`
**Redo queued (Ryan, Nov 13): the opening is rebuilt to Nicholas's *First Contact* brief** (about 2:20: missing patrol, Triad arrives, the impossible vessel, the first launch with the drone that goes blue and holds fire, the cut-off "Don't-", then straight into the tutorial battle; `storyline.md` §11, `docs/story/First-Contact-Cinematic-Brief.md`). **Done once, after the art it shows is on `main`** (Rodin player ships, the fleet with a space drone, the Dominion with opening hangars, round-4 effects, the new HUD). Until then T3 prepares the shot plan, the dialogue hooks and the drone beat only. Today's 49 s intro stays in the game meanwhile.
**Story (opening):** the intro shows the premise: the silent Dominion, machines descending on worlds, Triad deployed to identify it and report back. No hint of the truth. Text and narration are Nicholas's through the existing slots.
**(Nov 13, 17:15, Nicholas's approved opening decision, #29 / #24):** "The opening stays 49 seconds." So the redo is a
re-shoot of the 49 s with *First Contact*'s content (the silent ship, Triad's arrival, the drone that goes blue and holds
fire, the shared "Don't-" asset), not a 2:20 film; Ryan to confirm (complete-game contradiction 2). Still missing: the
re-shoot once ART-03, ART-04 and ENG-37 are on `main`; the `O_*` lines fired; the intro score bound (ART-19).
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
- **Flight:** atmosphere flight (ENG-20) flies over and between the mountains; arrival and take-off are replaced by the planetary descent (ENG-38);
  the landing assist still finds the pad.
  - (T1, Nov 13) The flight zone grows to the terrain's whole collision core plus 500 m (the edge warning starts at the
    8 km edge) with its ceiling 400 m above the highest ground in it, so the whole world is flyable without a boundary
    warning; past the collision core ships stay above the generated ground. Take-off: once the exit is open the climb-out
    triggers at the old height above the ground under the ship (~1.2 km), not at the raised ceiling. Boost (60% of boost speed) crosses
    it; the ground floor and terrain-ahead pull-up keep the ship off slopes and cliffs at boost speed too.
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
- **(Nov 13 evening) Still missing for the complete game** (fallbacks only; Nick's sound in the slot wins): the descent
  roar, burn-through and boom (ENG-38); the field opening and the shield dropping; the docking clamp; interior doors, the
  access-granted chime and a fallback three-pulse tone (its real definition is Nicholas's); conduit breaks and the heart's
  death hit; escape alarms and hull groans on the run's events ("time failing" in sound, ENG-31); the stinger's radio
  noise bed. Done when each has a cue in its slot, is levelled with `audio_report.py`, and is heard in a played run
  (`-Audio` dump) by the owning teammate.

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
- **(Nov 13 evening) Still missing for the complete game:** the flight HUD is the default (batch B). Still to build and
  capture: the **on-foot HUD** in the same style (health, block stamina, dash charges, interact ring, damage direction,
  objective); the story states: tutorial phase prompts and the withdrawal waypoint (ENG-40), *Aftermath*'s drone-lane
  display and the navigation pulse (ENG-42, reused on Verdance departure), the transponder "verified / unknown" flicker,
  the transfer progress meter, the Cinder "approach prepared, passage pending" marker, the generator count 0 / 4 and
  docking marker, the reactor conduit count, the escape clock and first direction marker, the friendly-status flicker; the
  subtitle line for transmissions and barks with the speaker's colour and a size option (ENG-45). Nick's fonts and colour
  tokens adopted once ART-09 publishes them.
  **Done when** each state is captured live at 1080p and 21:9 and named correctly by a fresh reviewer (ART-20).

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
- **(Nov 13 evening) Still missing for the complete game** (beyond tiers 1-3 above): the **hyperdrive rift** (a tear at the
  end of the spool, a tunnel of stretched streaks and rings in the squad colour, an exit rift and flash; T7 builds new
  systems, the effects builder plugs them into `DA_Travel_FX`, queued in `CLAIMS.md`); the Dominion's **approach field**
  shimmer and its opening (ENG-18); the shield sections dropping per generator; the tractor beam (ENG-29); the
  anchor tower collapse (ENG-26); conduit breaks and the reactor heart's death; interior collapse sparks and falling debris
  (ENG-31); the Dominion breakup at trailer quality, multi-stage (ENG-31, ENG-44); the dormant drone's blue optic and three
  light pulses (ART-18). Same quality bar and frame budget as above.

#### ENG-38 · Planetary descent: fly from space into a world, anywhere on the globe (CORE, added Nov 13 by Ryan) · `🟨`
**Ryan, Nov 13:** flying down into a world must be "super realistic, super high quality, AAA quality, make it almost
look like No Man's Sky": a transition "where you hit the atmosphere and then your ship gets some type of animation
showing you're piercing the atmosphere", "the world comes into focus as you get closer and closer", "extremely
detailed", and "you should be able to enter a world from anywhere, flying around that globe of a planet".
Research and the recommended design: `docs/eng/planet-descent-research.md` (No Man's Sky, Elite Dangerous, Unreal
techniques, our current code path; in progress Nov 13). This item replaces the old arrival of ENG-20; the landing rule
and the on-foot hand-over of ENG-20 stay.

- **Architecture decided (Lead, Nov 13, from the research report): "A+".** Keep the separate planet maps and hide the
  level load inside the brightest second of the plasma phase: a white-hot cover on the viewport and the roar loop
  both survive the level change, replacing today's 0.6 s fade to black and the `L_Transition` loading screen. The
  player arrives **where and on the heading they hit the atmosphere** (one mapping from a point on the globe to a
  point over the flat terrain, with the landing zone as its pole; the same mapping bakes the globe's surface from the
  terrain field and gives the take-off return point). On the planet side an Elite-style **forced glide** (about
  3,000 m down to 900 m, 15-20 s) carries the ship through a cloud deck while fog, terrain detail and scatter
  resolve by altitude. Streaming the terrain into the galaxy map, and a true sphere, were rejected for the jam (30-45
  and 100+ engineer-hours). Today's code: a 2.5 s entry, a fade to black, a loading map, then a hard cut to a fixed
  spot 600 m behind the pad; the entry point and heading are thrown away.
- **Risks to settle in the first hour of stage 1:** the real load time (the white hold reads as a freeze past about
  6 s; log it first), and whether the viewport cover and the sound really persist across the level change (fallback:
  a white camera fade on both sides).
- **Cut order if time runs short:** airbrake / heat-shield animation, volumetric clouds (use a card cloud deck),
  landing anywhere, the polished take-off, the baked globe texture. Never cut: the white cover replacing the loading
  screen, and arrival from the entry point.
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
- **Stages:** **stage 1 (tonight, about 11 engineer-hours):** the white cover that survives the load, no black fade
  and no loading map for planet entry and exit, arrival point and heading from the entry, the terrain's first view
  built around the arrival, the roar persisting, a timing log, a switch back to the old arrival. **Stage 2
  (tomorrow midday, about 22 engineer + 12 builder hours):** the glide, the cloud deck, plasma / streak / heat
  effects, sound layers, altimeter and hull-heat HUD, the globe texture baked from the terrain with the landing
  beacon, map preload while orbiting, a mirrored take-off, the terrain bent down with distance so the horizon falls
  away like a sphere's. **Stretch:** land anywhere, volumetric clouds, airbrake animation, bigger globes.
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

### Complete-game items (added Nov 13 evening, at Ryan's request)
Ryan: "make sure it has all the information in order to pretty much complete the entire storyline and the entire game".
Each item below closes a gap found by walking `storyline.md` beat by beat against this file, `dialogs.md`,
`animations.md`, `CLAIMS.md`, the board and `docs/story/`. Ids continue the sequence (ENG-39 was named in the
realignment without an item; it gets one here).

#### ENG-39 · Worlds that read as real places: planet play zones, the space field, pickups (CORE, Ryan Nov 13) · `🟨`
Spec: the realignment section "Worlds that read as real places" and `docs/eng/WORLD-DENSITY-PLAN.md`.
- **Planets (T4, environment builder, T5):** every atmosphere entry, from any side of the globe, routes into the one
  dressed play zone around the pad and objectives, with its own approach corridor and first view per side. Inside the zone:
  at least three ground materials, trees, rocks, buildings and points of interest in every direction; outside it, haze,
  the cloud deck, the curvature bend, ring terrain and the flight-zone turn-back hide the edge; keep-out areas on foot.
- **Space (T6, T4 reviews):** belts, clusters, debris fields, wrecks, buoy chains and convoy lanes across the flyable
  space, then a streaming ring around the ship, deterministic from a seed (`td.SpaceField`); keep-outs around the combat
  zone, gates, planets and the Dominion; combat lanes stay readable (T6, in progress).
- **Pickups:** new gameplay; the smallest version is proposed to Ryan on the board **before** it is built (v1 built (a094a440, 23a9f302, 09636062), on main 6fe3e90f).
- **Space composition (Ryan, Nov 14; T6, T4 reviews):** the flyable space is composed as seven themed regions (the Watchline, the Shepherd Belt, the Ice Drift, the Quiet Graveyard, the Survey Scar, the Shattered Moons, the Outer Dark) per `docs/eng/SPACE-DESIGN.md` (on eng/t6, not on main yet); the Shepherd Belt first, inside a rock-field performance budget, with staged large and special breaks; its set pieces (patrol hull, expedition hulls, survey rig, hollowed rock) come from the structures builder.
- **Performance (Ryan, Nov 14; T7, issue #89):** no hitches and a steady 60 fps in the Shipping build in every phase, measured per phase with each teammate measuring their own area; targets and findings in #89 and ENG-28. Since the 00:30 crash, at most three rendering Unreal processes run machine-wide.
- **Done when:** from five entry points per planet the arrival reads as a place in captures; a 60 s flight in any
  direction in space passes dressing; wave 5 stays inside the ENG-28 frame budget with the field on.
- Depends on ENG-34, ENG-38, ART-05, ART-06, ART-13.

#### ENG-40 · Act 1: the tutorial battle and the withdrawal (CORE) · `🟨`
`storyline.md` §13 and the reconciliation correction of 17:15 (Nicholas, #29, #24): **one uninterrupted first mission over
waves 1-2**, entered from the intro with no fade.
| Phase | Done when (played) | Owner |
|---|---|---|
| 01 Break formation | A readable first volley arrives before any enemy is targetable; prompt "EVASIVE MANEUVERS"; `TB_*` call | T4, T2, T3 |
| 02 Return fire | A small marked group; prompt "TARGET AND FIRE"; the existing onboarding hints, one at a time | T4, T3 |
| 03 Fight as Triad | Switching, then each ship's **signature** power (portal, ram, swarm / dome; abilities stay earned on the planets), each with a timed hint or substitute, never a hard lock | T3, T2 |
| 04 Heavy contact | Wave 2's gunship arrives; prompt "HEAVY CONTACT"; killing it is **optional**, no gate, no reward | T4 |
| 05 Withdraw | Command orders the retreat after phase 03's tools are taught; a marked corridor and boundary; pursuers stop at it; crossing it fires one completion event that ends the tutorial (with the gunship alive or dead) and starts *Aftermath* (ENG-42); the tutorial music fades on it (#24) | T4, T1, T2, T3 |
- Checkpoints at the engagement, the heavy contact and the withdrawal; a death never replays the intro. Most of the
  Dominion's craft stream past toward Verdance in the background; the drone that hesitated in the intro is never a target.
- Nothing is fought between *Aftermath* and Verdance; waves 3-5 are unchanged.
- **Tests:** the completion event fires on the boundary with the gunship alive and dead; no gate on the gunship; a retake
  in each phase; the five `TB_*` lines fire once.

#### ENG-41 · First-play tutorial flag and "Skip the tutorial" (Nicholas, #29) · `🟨`
- A profile flag (not the run save; a New Game never clears it) is set by ENG-40's completion event.
- With the flag set, New Game offers "Play the tutorial" or "Skip"; Skip starts in the state the tutorial leaves: waves
  1-2 done, their lines marked played, hints off, *Aftermath* next. With the flag clear there is no offer.
- The intro's own hold-to-skip is unchanged. Nick's UI styles the prompt (ART-09).
- **Done when:** first run forces the tutorial; second New Game shows the offer; Skip gives an identical game state to a
  played tutorial (tested); Continue is unaffected. Owner: T7; on main 6fe3e90f (eb02ac51, 80351f1a), played by an agent (T7, session 2: the offer shows on a completed profile and Skip lands in the Aftermath state); review open (ART-20); the skip start overlaps space junk (#90 no. 10, T4); with T3, T5, T1 for their lines.

#### ENG-42 · Story sequences as played: transmissions, voice-over scenes, skips (CORE) · `🟨`
The transmission system exists (ids, speakers, queue, once per run, saved, the story test; `docs/story/transmissions.md`).
Many sequences have lines in the table that **nothing fires**. Each row is done when a played run shows the lines in
order, subtitled, once, never replayed on a retake, with a skip that applies the same state, and the story test passes.
| Sequence (storyline.md) | Form (reduced) | Fired by | Still missing |
|---|---|---|---|
| Intro `O_*` | Over the 49 s intro | T3 | Lines fired; shared `Dont` asset |
| Tutorial `TB_*` | Radio over phases 01-05 | T4 events, T3 | Fired; on main 6fe3e90f (13623956, cef28834, d89a0043, 6c4979f2) |
| *Aftermath* `S2_*` (§12) | 75-100 s of radio over regroup and the jump; drone-lane display; navigation pulse; Dominion light row in the same rhythm | ENG-40 event; T3, T2, T4 / effects | Fired; on main 6fe3e90f (13623956, cef28834, d89a0043, 6c4979f2); display and pulse new |
| Cinematic 03 `C3_*` (§15, §25.1) | 40-60 s voice-over at the survey console: monitor graphic, the voices fragment, drone wakes blue, three pulses, "comms responded" HUD beat | T5 console, T3, T2 | Fired; on main 6fe3e90f (13623956, cef28834, d89a0043, 6c4979f2); monitor graphic from Nick's #44 PNG or T2 |
| Cinematic 04 `C4_*` (§17, §25.1) | Transmissions over the Verdance take-off | T5 extraction, T3 | Fired; on main 6fe3e90f (13623956, cef28834, d89a0043, 6c4979f2) |
| Waves 3-4 interception | Two or three lines naming the interception on the Cinder route | T4, T3 | New ids |
| Cinematic 05 `C5_*` (§20, §25.1-25.2) | 40-50 s voice-over at the Cinder terminal: the one echo, the Origin Core lead | T5 terminal, T3, T2 (Cinder overlay) | Fired; on main 6fe3e90f (13623956, cef28834, d89a0043, 6c4979f2); approved lines; replaces the three `A3_Echo*` placeholders |
| Cinematic 06 `C6_*` (§23) | Six lines at take-off, two as wave 5 forms | T5, T4, T3 | Fired; on main 6fe3e90f (13623956, cef28834, d89a0043, 6c4979f2); Nicholas picks the lines |
| Gameplay 06 `G6_*` (§24) | Radio and two Unknown clue fragments during the generators | T4, T3 | See ENG-18 |
| Cinematic 07 `C7_*` (§26) | Six lines over docking and arrival (no camera scene) | T4, T5, T3 | Four new ids; `G6_Wren_CoverTheBay` duplicate resolved by Nicholas |
| Gameplay 07 `G7_*` (§27) | Four console recordings in route order, Unknown speaker first, then three squad reactions | T5, T3 | Speaker of `A4_Recording_Hangar` to Unknown; see ENG-30 |
| Cinematic 08 `F_*`, `C8_*` (§28) | About 70 s of voices over the sealed reactor threshold: five callback lines, the full warning (own id, **not** the `Dont` asset), the case, the decision; readout "REACTOR · NO VERIFIED SAFE SHUTDOWN" (placeholder) | T5 gate, T3 queue and skip | Fired; on main 6fe3e90f (13623956, cef28834, d89a0043, 6c4979f2) |
| Gameplay 08 `G8_*`, 09 `G9_*` (§29-30) | Conduit, heart, "Reactor lost", hangar and pull-away calls | T5, T4, T3 | Fired; on main 6fe3e90f (13623956, cef28834, d89a0043, 6c4979f2) |
| Cinematic 09 `C9_*` (§31) | Three "clear" reports at the escape radius, Moss at 10 s, three signal lines at 18 s under the held caption | T4, T3 | See ENG-44 |
- **Story test changes** (`docs/story/story-0708-actions.md` (e)): `G7_`, `C8_`, `G8_`, `G9_`, `C9_` join the after-reveal
  set, `C7_` stays out; `A4_Recording_Hangar` checked; the full warning and the stinger never use the `Dont` asset; no
  entry contains "forty-one".
- About 28 new ids for 07-09 plus the `C3_`-`C6_` sets; T3 seeds them with the documents' draft lines; Nicholas's
  `DA_Story_Transmissions` rows replace text, speaker and sound by id (ART-11).
- Owner: T3 (system, ids, skip, test), with the firing lanes named per row.

#### ENG-43 · Gameplay barks from `dialogs.md` (CORE) · `🟨`
- A `Tools/` script turns `dialogs.md`'s tables into `DT_BarksFallback` under `Gameplay/`; Nicholas's `DT_Barks` wins row
  by row (T3).
- Per-id rotation (never the same line twice in a row, not within 60 s), the four priorities with `Flavour` only in calm
  stretches, nothing but `Critical` during a money shot, the slate or "CUT!", transmissions always win (T3).
- Every **(new)** row gets its trigger from its owner with a test that it fires once per held condition (about 53 ids; 22
  fire today; more on main 6fe3e90f (6e49f95c, 1ed3c09e)): T1 travel and landing, T2 combat and powers, T4 enemies and the Dominion, T5 on foot, T3 squad and switch.
- Includes storyline §6's clue bark: an enemy now and then pre-empts a switch or a dodge from wave 3 on, and a pilot
  notices (T4 AI, T3 line).
- **Done when:** a played 10-minute session logs no line spammed or cut, every speaker in character, subtitles in colour.
  Banter (§9) is final polish.

#### ENG-44 · Ending: regroup, victory, stinger, credits, back to the menu (CORE) · `🟨`
Exists: the victory backdrop with flybys, the panel with takes per scene, `StingerSeconds = 18`, the 0.35 s fade, the
credits, a won save with no Continue. Still missing (`storyline.md` §31, `story-0708-actions.md` b18-b23):
- the caption reads "DISTRESS SIGNAL · TRIAD: WREN, KATO, MOSS · DATED DECADES AHEAD" (Nicholas may reword); no text says
  the future is safe, the Origin Core destroyed, or the older crew dead or escaped (T3);
- the three "clear" reports fire as the ship crosses the escape radius (T4); the signal lines at 18 s with the caption
  held on screen as its own element (T3);
- leaving the panel before 18 s plays the stinger first (T3);
- the credits roll lists the team, every third-party asset and licence and the tools used, from `CREDITS.md` (text by
  Nick's side, ART-14; roll by T3 or Nick's UI); credits music bound (ART-19);
- after the credits the game returns to the main menu; Continue is hidden; a New Game works (T5);
- the same ending plays after the space-only fallback (ENG-47).
- **Done when:** played twice (watching and skipping early) with the stinger seen both times.

#### ENG-45 · Settings and accessibility completion (CORE) · `🟨`
Exists (ENG-32): camera, sensitivity, invert, dead zone, hints, three volumes, window mode, resolution, reset; the
difficulty row (in review). Still missing, each saved and applied at once:
- **Subtitles** on / off (default on) and size (normal / large) for barks and transmissions (T3 reads it);
- **Camera shake and speed-FOV** intensity sliders (T1's camera pass already reads "the Settings sliders");
- **Graphics quality** preset wired to scalability (Low / Medium / High / Epic), so the mid-spec target holds 60 fps (ART-13);
- difficulty chosen on **New Game** as well as in Settings (T5, T1);
- every screen navigable by pad, glyphs following the last device used (T1's gamepad pass);
- colour-blind check: the squad colours, enemy red and the HUD warnings stay distinct under a protanopia / deuteranopia
  simulation, with shapes as well as colours (ENG-36).
Owner: T1 (settings backend), T3 (subtitles), T5 (New Game difficulty); Nick's styled page through
`UTDUIStyle::SettingsWidgetClass` (ART-09). **Done when** each row is changed in a played run and survives a restart.

#### ENG-46 · The whole game, played: end-to-end tests and playtest gates (CORE) · `🟨`
- **Automated:** one end-to-end test per route, real input where it matters, in a Development build: (a) New Game →
  tutorial → Verdance → Cinder → assault → docking → interior → reactor → escape → victory → credits → menu; (b) the same
  with Skip the tutorial; (c) the space-only fallback; (d) Continue from each act boundary, the interior and the escape;
  (e) a death and retake in every scene. Extends `TriadDrift.Flow.E2E.*` (T7 with T5).
- **Gates (people):**
  | Gate | When (EST) | Who | Passes when |
  |---|---|---|---|
  | G1 | Nov 14 morning | Ryan, packaged build | Plays start to end; every problem logged in `DEVLOG.md` and routed |
  | G2 | Nov 14 evening | Ryan and Nicholas | Same on the post-fix build; Nicholas listens to every cue (ART-10) |
  | G3 | Nov 15 before 12:00 | Ryan's Lead + T7 | Freeze candidate played start to end packaged, all three routes |
  | G4 | Nov 15 before upload | Ryan | The exact zip to be uploaded, played from download |
- **Timings:** each act timed in the T7 playtest log (`docs/eng/PLAYTEST-*.md`); the whole run on Standard lands in 20-30
  minutes or the over-long beat is reported to Ryan.
- Owner: T7 (playtest lane, in progress), Lead (gates and packaging), Nick's side (scripts, ART-21).

#### ENG-47 · Finale mode decision and the fallback ending (CORE) · `🟨`
- Boarding is the documented ending (`Story/README.md`: "The confirmed ending boards the Dominion; the existing space-only
  route is an emergency fallback"). `td.Finale.Boarding` defaults to 1 on the freeze build once ENG-29-31 pass G2.
- **Decision point: Nov 14, 20:00 EST (Ryan).** If the boarding route has an open BLOCKER then, the freeze build ships with
  `td.Finale.Boarding 0`; the reveal is then carried only by the assault transmissions and placeholders (no fallback
  writing is requested from Nicholas).
- The fallback stays finishable until the freeze: covered by ENG-46 route (c).
- Owner: Lead (decision record, cvar default), T4 and T5 (both routes green).
- **(Nov 14 01:00) In progress:** the fallback was not finishable in a played run (ENG-18, #90 no. 5); T4's fix 818d592e
  is in batch M. The Lead writes the decision record once that is on main and played.

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
- **(Nov 13 evening, `docs/eng/SHIP-QUALITY.md`, R-041, R-042 PR #67)** In game: Rodin r1 hulls; Rift is engineering's
  hybrid (Rodin hull, rebuilt ring, engines and back plate, animated portal membrane) and stays the in-game Rift;
  SquadDetail r1 detail layers and Lancer / Bastion textures r2 approved. Still missing: the close-range pass on all three
  (R-043: Blender rebuilds of nose, cockpit frame and wing edges, a re-packed 4K bake shipped at 2K, painted panels instead
  of chrome) for the hangar and the pads; landing gear and canopy motion (ART-08). Owner: ship builder. Done when Ryan
  approves each ship at 3-10 m on a pad and in the chase camera.

#### ART-04 · Enemy & Dominion visuals · `🟨`
**🟦 Ryan since Nov 13.** Enemy ships, turret, Warden, drone, generator and reactor by Grok Bot (R-027 #2, #5, #6); Dominion exterior and crew by engineering, already in the game; T4 and T5 wire them.
- **Null Armada ships:** Shard Fighter, Hook Interceptor, Wall Gunship (shield emitter), Maul Bomber,
  Hive Carrier (animated bay doors).
- **Dominion exterior:** a modular Nanite hull with rotating rings, shield generators, turrets, the
  reactor trench (core ending) and the hangar mouth (stretch).
- **Look:** angular, red/black/white-eye shape language. Telegraph glows tied to the gameplay states.
- **(Nov 13 evening, R-042)** On `main` or merged: the fleet (fighter, interceptor r2, gunship, bomber, carrier, turret),
  the hauler (#68), the Warden and the drone, Dominion hull r1 (#70, approved for gameplay). Still missing: the carrier
  wired and checked; the ship builder's import of #68 and #70 with T4 switching the Dominion to the hull's 12 sockets;
  Dominion hull r2 (stepped superstructure, crimson bands, window rows; same sockets); the shield generator mesh; the
  bay mouth #54; enemy readability at range (scale, glow, silhouette light, T4). Done when each is seen at gameplay
  distance in a played wave or the assault.
- **(Nov 13 22:45)** Hull r2, generator and bay source merged (#72, #74, #77); imported, on main 6fe3e90f (c3375433); r2 in use with T4's socket work (b59423d5, batch M).

#### ART-06 · Planet environments: Verdance & Cinder (CORE) · `🟨`
**🟦 Ryan since Nov 13.** Terrain, mountains, materials, rocks, trees and set pieces: engineering (ENG-34) with Grok Bot's texture sets and set pieces (R-027 #3, #8). **Lighting, fog, sky and grade on the planets stay with Nick (ART-13).** **(Correction, Nov 13 afternoon, R-036: ART-13 moved to Ryan's side too, so the planets' lighting, fog, sky and grade are the Lead's environment builder's.)** Still missing for the complete game: the dressed play zones (ENG-39), the walkable survey stations and dig cabin (ART-17), the story set pieces (ART-18), and the lighting pass per planet (ART-13).
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
- **(Nov 13 evening) Must-haves for the complete game, from `animations.md` §5** (build items; the rest of the ledger is
  polish). Each is done when it plays in a running game from both cameras with no T-pose or slide:
  | Move | Today | Owner |
  |---|---|---|
  | Deflect flick and perfect-deflect flourish | retargeted, no code slot | T5 slot, animation builder |
  | Death clip then ragdoll (pilot and crew) | ragdoll only | T5 slot, builder (drop the lying pose to the floor) |
  | Interact (console, pickup, charge), board and exit the ship | cut to camera | builder (route A or B), T5 |
  | Air slam fall and land | stand-in | Grok Bot r2, T5 slots |
  | Combat idle with the blade lit | retargeted, no slot | T5 slot |
  | Hit react rate-limited on bursts | restarts every bolt | T5 |
  | Warden set: slam, fire, summon, stagger, phase roar; **capsule fix** (`//` in `TDCrewTypes.cpp`) | crew clips borrowed; hits land on a 40 / 90 capsule | Grok Bot clips, builder, T5 |
  | Trooper rifle set, grunt melee, heavy shield stagger | template set | builder (Epic Animation Sample for locomotion), T5 |
  | Foot IK on the terrain slopes | none | builder (route C), T5 |
  | Landing gear and settle; generator petals; bay doors; reactor rings | placeholder / code | builder (route B), T1, T4, T5 |
  The Epic Game Animation Sample's licence is confirmed and logged in `CREDITS.md` before first use (asset builder).

#### ART-02 · Visual asset inventory & sourcing plan · `🟨`
**🟦 Ryan since Nov 13 (asset management moved to Ryan's side).** The inventory of every asset, where it comes from, its licence and its state is kept by Ryan's Lead: `docs/eng/ASSETS.md` (ledger), `CREDITS.md`, `docs/eng/ART-HISTORY.md`. Sourcing is Grok Bot (generation) and engineering builders (code-built, imports). Nick's supplied-model intake (N-041, N-042) is a library Ryan's side selects from.
- **The list:** every asset the game needs (ships, enemies, Dominion pieces, props, environment sets,
  characters, VFX, UI and audio), each with its route: AI image-to-3D (Rodin first: credits expire
  Nov 20, and using it enters the prize pool), Fab or other licensed library, code-built Blender, or
  the template.
- **Status and credits:** track each asset's status here or in `ArtSource/inventory.md`, and log
  licences in `CREDITS.md`.
- **(Nov 13 evening) Still missing:** the ledger `docs/eng/ASSETS.md` complete for every asset in the cook (source, tool,
  licence, state, who added it); the eight picks from Nick's intake (R-037, `docs/eng/ASSET-INTAKE-REVIEW.md`) imported or
  dropped; every Grok Bot delivery's `SOURCE.md` carried into `CREDITS.md`; Epic's Animation Sample licence logged. Done
  when the asset builder's diff of the cook against `CREDITS.md` finds no unlisted third-party or AI asset (feeds ART-14).

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
- **(Nov 13 evening) Still missing:** the lighting, fog, sky and grade pass: first passes on main 6fe3e90f (671223ca, 03cdafc4, d89a627a) (environment builder,
  map World Settings claimed first). Done when each of `L_MainMenu`, `L_Galaxy`, both planets (orbit, descent, play zone)
  and `L_Dominion_Interior` (normal, alarm and collapse states) has a before / after capture at gameplay distance, the
  squad / enemy colour code still reads, and the frame rate holds (ENG-28). The scalability preset is wired with ENG-45.

#### ART-17 · Walkable hard-surface structure kit (R-043, Ryan Nov 13 evening) · `🟨`
**🟦 Ryan.** "Nothing the player walks on, into or through is an AI-generated mesh." Owner: the Lead's **structures
builder** (Blender, `blender-game-asset` corridor kit), Grok Bot for measured plans, trim sheets and decals; T5 fits each
site.
- A modular kit on a 50 cm grid with bevelled edges, one trim-sheet material (2K, normal and roughness; Verdance-teal and
  Cinder-ember emissive strips), collision equal to the visual floor; doorways at least 140 x 240 cm, ceilings at least
  280 cm, ramps at most 20°, 150 cm clear in front of a console.
- Sites, each keeping its `BP_` name so gameplay is unchanged: the Verdance survey station (re-dressed research site) and
  console; the relay climb (if ENG-25 keeps it); the Verdance wreck interior; the Cinder survey station and dig cabin (#69
  re-made or given a ramp proxy); the Dominion hangar, corridors, arena, Warden room, reactor chamber and the docking bay
  interior (ART-15).
- AI meshes become concept reference, exterior shells out of reach or distant dressing; replaced versions are listed in
  `docs/eng/ART-HISTORY.md`. The on-foot pawn collides with parked ships (T1, done per R-043).
- **Done when:** each site is walk-tested with the real pawn in both cameras (no sinking, no snagging, no walking through
  walls), captured, and Ryan has seen it.

#### ART-18 · Story set pieces and clue props (CORE) · `🟨`
**🟦 Ryan.** Grok Bot (distant and non-walkable meshes, R-042 queue), ship / structures / effects builders (import,
sockets, effects), T4 and T5 (placement). Each is done when it is seen in a played run at the beat it serves.
| Piece | Beat | State (R-042) |
|---|---|---|
| Null transport / hauler, convoys on the lanes | Verdance and Cinder approaches | mesh #68 merged; hauler imported (on main 6fe3e90f); convoy wiring next |
| Verdance survey station, console, dormant drone with a blue optic and three light pulses | Cinematic 03 | outpost and console #50 merged; drone prop and its light beat new |
| Ancient relay | Verdance | #64 merged (walkability per ART-17) |
| Survey instruments on one buried line | Verdance route | new (small props) |
| Cinder dig site and drill rig | Cinder | #69 merged; drill rig imported (on main 6fe3e90f) |
| Field anchor tower #52 with a separate emitter and a charge base | Cinder objective 4 | merged (source) |
| Crystal cache and hero crystal #53 | Cinder objective 3 | merged (source) |
| One ancient-technology artefact, the same on both planets and in the reactor chamber | storyline §6 clue | new |
| Triad's insignia shape under damaged enemy armour (ships and troopers) | storyline §6 clue | new (decal or damage variant) |
| Shield generator (base ring stays as wreckage) | Assault | merged (source) |
| Dominion hull r2, docking bay mouth #54 | Assault, docking | hull r1 #70 merged; r2 and #54 merged (source) |
| *Aftermath*'s Dominion light row pulsing in the navigation rhythm | Act 1 | new (material parameter, T4 / effects builder) |
| Outpost and pad set dressing #40 | Both planets | merged (source) |

### Boarding finale (WANTED since Nov 13, Ryan; story acts 4 and 5; was stretch)

#### ENG-29 · Docking & boarding transition · `🟨` WANTED (Nov 13)
**Story (act 4):** the squad breaks through the field and boards. Plan (T4, Nov 13): docking bay opens when the defences are down, an approach cone with a tractor assist that never fully locks before the mouth, one travel into the interior, wingmen hold outside.
`ATDDockingBay` approach cone with a tractor-beam assist that reduces control, then
`TravelTo(DominionInterior)`; the hangar landing hands off to first person (§8).
- **(Nov 13 evening)** Built by T4 (docking and the escape flight are done on `lead/int`, `CLAIMS.md`). Still missing
  (Cinematic 07, §26): the `C7_*` pair fired on docking commitment; attack pressure off from commitment to the travel;
  confirm the ship docked in is the ship flown out and that the wingmen rejoin (T4, T5); the tractor beam effect and the
  docking shot (ART-16); the bay mouth matching hull r1 (ART-18). Done when docking is played from each of the three ships.

#### ENG-30 · Dominion interior encounters & core fight · `🟨` WANTED (Nov 13)
**Story (act 4, The Truth):** familiar military architecture, doors and consoles keyed to the pilots' identities, and an interactable console with a recording of their older selves in each room (hook with a placeholder line; Nicholas's words and voices). The core fight is the choice of act 5.
- **Interior:** 6 sections (hangar, a deflect-lesson corridor, barracks, a vertical shaft with
  sweeper hazards, a security checkpoint, then the Warden) leading to the core chamber.
- **Core fight, phase 1:** 4 conduits at 150 HP each; the core fires deflectable orbs.
- **Core fight, phase 2:** a sweeping laser; the heart is vulnerable whenever the rings align, every
  6 s.
- **Core fight, phase 3:** at 25% health, everything speeds up.
- **(Nov 13 evening) As built and still missing** (`storyline.md` §27-29, `docs/story/story-0708-actions.md` b1-b14).
  Built from code (`Ground/Interior/TDDominionInteriorCourse.inl`, T5) and played end to end: hangar → deflect corridor →
  flight-deck arena (three waves) → Warden's gate → reactor chamber (four conduits, then the heart; 150 health). The
  phase 2 and 3 beats above (sweeping laser, ring timing, acceleration) are **not built**, and Nicholas's documents forbid
  a mandatory perfect-deflect gate: Ryan decides whether any is built (complete-game contradiction 9). Still missing:
  1. rooms fold the documents' route: the arena plays the barracks, the Warden-room door is the security checkpoint
     ("ACCESS GRANTED · WREN · KATO · MOSS" and the three-pulse sound), the arena-exit console is the command archive (T5);
  2. the four consoles take the four recordings in route order, the first with an **Unknown** speaker; arena consoles
     answer only once the room is clear (T5, T3);
  3. **the reveal gate:** the core console moves to the threshold before the reactor door; the door stays shut until it
     is used; using it saves the evidence, holds combat and plays Cinematic 08 (ENG-42); then the door opens, the
     checkpoint saves and "Break the reactor conduits" starts (T5 about 2 h, T3 about 1.5 h);
  4. player-facing words: "THE REACTOR" (not "THE CORE"), "BREAK THE REACTOR CONDUITS 0 / 4", "DESTROY THE REACTOR HEART";
     the first objective "Reach the command archive" (T5);
  5. one source of pressure in the reactor room (a few drones or two troopers per conduit), telegraphed (T5);
  6. the `G8_*` lines on the conduit, heart and completion events (T5, T3);
  7. the interior built walkable from the kit (ART-17), lit with alarm states (ART-13, ART-15);
  8. a retake in the fight restarts it from 0 / 4 with the shield up, never repeats the Warden (told to Nicholas).
  Done when the interior is played start to finish from Continue and from a fresh dock, the reveal heard in full once.

#### ENG-31 · Escape run & escape flight · `🟨` WANTED (Nov 13)
**Story (act 5, The Choice):** the escape plays with time visibly failing (echoes, repeats); after the breakup the victory screen receives the distress signal with the squad's own identifiers.
- **Escape run:** a 90 s run back to the hangar on a changed route.
- **Escape flight:** a 60 s flight out through the falling debris, then the breakup sequence and
  victory. Failing either restarts it as the next take.
- **(Nov 13 evening) As built and still missing** (`storyline.md` §29-31). Built: the run (`Obj_DI_Escape`, 90 s,
  starting 2.5 s after the heart dies, debris down the route, all doors open, a retake at the reactor exit with the full
  clock); the flight as its own scene (checkpoint at the hangar mouth, debris and quickening explosions, done beyond the
  exit radius, then victory). There is **no changed route** (same rooms, doors open): accepted in reduced form unless
  Nicholas objects. Still missing: the first direction marker visible at the run's start (T5, T2); `G9_*` lines at the
  run's start, the hangar trigger and regained flight control (T5, T4, T3); "time failing" as repeated alarms and hull
  sounds on the run's events (Nick's audio, ENG-35 fallback; visuals are polish); the escape music keyed to the run's start
  (ART-19); the 60 s flight limit and its retake values confirmed in a played run and told to Nicholas (T4); the breakup at
  trailer quality (ENG-37). Done when both halves are played, failed once each and retaken.

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
| ART-09 HUD and menus | 🟨 native credits on main 08f40144; industrial reward frame imported/reloaded and candidate widget compiled/lifecycle-tested; rendered review pending | ENG-36 flight HUD; T2 mounts rewards; T3/T5 own front-end hosts | **Priority 3:** revised victory and Return verification, reward, menus, loading, fonts and tokens |
| ART-10 audio and music | 🟨 Verdance loop repair on main 2275bff3, recorded across two wraps; waves 3–5 director audition 4281f4a6; PR 11 listening and PR 12 replacement audition pending | ENG-35 effects; engineering score exclusivity hooks | **Priority 1:** matched-build music integration, remaining loop checks, mix and ear approval |
| ART-11 barks, voice | 🟨 second draft circulated; approved Wren line reported on eng/t3, not main; delegated playback decisions in Story/README.md | Ryan owns runtime/voice pickup; C4 ID correction and boundary retirement pending (#29) | **Priority 4:** verify corrected main dialogue and performance; no full recording lock |
| ART-12 cinematics, Director's Cut look | 🟨 designed renderer source frozen; renderer compiled, corrected tests queued; no default activation | T3's reflected HUD name-card ownership flag needed (N-038); ENG-33 intro | **Priority 3:** complete build, prevent duplicate pilot cards, capture real gameplay and obtain appearance approval |
| ART-13 lighting, post-process | 🟨 | **🟦 Ryan** (moved to §5, Nov 13 afternoon) | None |
| ART-14 submission | 🟨 itch copy and evidence-linked Glass Box draft on main; final public page/video pending | Trailer capture pipeline, `CREDITS.md`, `DEVLOG.md` | **Priority 5** |
| ART-15, 16 boarding | 🟨 WANTED (Ryan, Nov 13) | Engineering builders make the interior kit as a fallback; effects builder for the collapse | Lighting and alarm states of the interior; the recordings' voices (ART-11) |
| ART-19 music and cue playback wiring | 🟨 credits, boarding and Verdance player on main 844064f1; map recordings published; natural event hookup and Shipping checks pending | T1/T3 event contracts; T7 packaging and fallback | **Assigned to Nick's side by Ryan's Lead (Ryan, Nov 13)**; ambience player assigned under N-075 |
| ART-20 independent reviews of Ryan's lanes | 🟨 | Ryan's Lead's reviewer sub-agents | **Assigned to Nick's side by Ryan's Lead (Ryan, Nov 13)** |
| ART-21 playtest checklists and per-act scripts | 🟨 | T7 | **Assigned to Nick's side by Ryan's Lead (Ryan, Nov 13)** |

**(Nov 13 evening) Ryan: "feel free to give Nick's agents more work. Probably not art related because his computer is
not as strong as mine ... You are the lead of everything."** So Ryan's Lead assigns Nick's side light work by default:
audio and music playback wiring, UI and menus, story words and barks, independent code reviews, the submission, and docs.
Nothing heavy (Niagara, Blender, cooks, terrain, big builds) goes to Nick's side. Nicholas still decides his own art,
audio and story direction within these items, and may hand any of them back on the board.

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

#### ART-01 · Art direction & style guide · `🟨`
- **Content:** a one-page style guide with palettes, the faction shape language, readability rules
  (player projectiles in squad colours, enemies in red and orange), lighting mood per level, and
  the **Director's Cut visual language** (cuts, letterbox, slate cards, grade; C-03).
- **Reference board:** inspiration only; no copying of existing IP.
- **Gate:** both humans agree on the style before mass production.
- **(Nov 13 22:45) State:** the guide `docs/visual/Art-Direction.md` is on main (2d5d5928); both humans have not agreed it yet (`🟨`).

#### ART-09 · HUD & UI/UX · `🟨`
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
- **(Nov 13 evening) The screens the complete game needs, assigned to Nick's side by Ryan's Lead (Ryan, Nov 13).** The
  flight HUD itself is ENG-36. Each screen has an engineering fallback today and is done when Nick's version is on `main`
  through its `UTDUIStyle` class, works by keyboard, mouse and pad, and is captured in a packaged build:
  | Screen | Fallback today | Nick's side delivers | Hook |
  |---|---|---|---|
  | Main menu | code-built (New Game, Continue, Settings, Quit) | MenuM01 (#71 handoff), focus fix | `MainMenuWidgetClass` |
  | "Skip the tutorial" prompt | none (ENG-41) | a two-button prompt in the menu style | ENG-41 exposes the call |
  | Pause (Resume, Settings, Restart scene, Quit to menu) | code-built | styled page | `PauseWidgetClass` |
  | Settings and accessibility (ENG-32, ENG-45 rows) | code-built | styled page, N-038 focus fixes | `SettingsWidgetClass` |
  | Loading (still used for the interior travel and any fallback travel) | code-built | Loading01 | `LoadingWidgetClass` |
  | Ability unlocked (Gravity Well, Cloak, EMP) | fallback card | `WBP_AbilityUnlocked` | `OnAbilityUnlocked` |
  | Subtitle line (barks and transmissions; speaker colour; size option) | dev HUD line | styled line | `OnBarkRequested`, transmission slot |
  | Objective panel polish (objective line, step counts, timers) | ENG-36 | fonts and tokens for T2 | `GetObjectiveText` |
  | Director cards (slate, CUT!, name card, letterbox; a take number that glitches from Cinder on) | fallback set | the designed set (ART-12) | `DirectorWidgetClass` (N-038) |
  | Victory (takes per scene, run stats, the stinger caption held) | code-built | styled panel | ENG-44 |
  | Credits roll | code-built | styled roll; text from `CREDITS.md` | ENG-44 |
  The galaxy map is not required for the complete game (final polish, item 28).

#### ART-10 · Audio & music · `🟨`
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
- **(Nov 13 evening) Music the complete game needs, per beat** (from N-044's ranked list and the story; Nicholas decides
  every track; wiring is ART-19; engineering's beds in ENG-35 fill any gap): menu (on `main`); intro score; tutorial
  prelude and loop with the 20 s exit on the withdrawal (#24, in progress); combat rising with `MusicIntensity01` for waves
  3-5 (CombatMusic01); Verdance theme (delivered); a Cinder on-foot cue; the descent arrival swell; the Dominion assault
  (Infinity Zone proposed); the interior loop (delivered); a reactor-fight loop; the 90 s escape cue on the hit 2.5 s after
  the heart dies; victory warmth interrupted at the 18 s stinger; credits (delivered). Quiet by design: Cinematics 03 and
  05 and the console recordings. Done when each is heard in its beat in a played build and Nicholas approves the mix by ear.
- **Confirmed music placements, November 14:** Cinder World is exploration and Cinder Fall is combat (full SoundWaves on main `5271d79c`). The original Cathedral Pulse (69.56 s, distinct from the credits track) accompanies the core chamber/reveal. Nicholas explicitly approved reusing the full Infinity Zone for the reactor conduit/heart boss fight in Audio's chat; Ritual remains the escape selection. T1/T5 supply the real fight-start/end and escape signals through N-089; Audio handles exclusive handoffs. These are placement decisions, not approval of a new loop, start offset, timer or final gameplay mix. No change to the separately tracked space-assault selection is authorized by this additional boss placement. Quiet dialogue/recording rules above still apply.

#### ART-11 · Narrative presentation: barks, VO, intro · `🟨`
- **November 14 fidelity checkpoint:** [StoryFidelity01](docs/visual/StoryFidelity01/README.md) publishes the verified second-draft matrix and voice pickup register, retaining specific later approvals. The adc6f23b current-main delta resolves stale recording/order/caption findings separately from the historical baseline and marks queued T3/T5 work. Documentation/source evidence only; actual performance, trigger order, subtitles, cook and gameplay acceptance remain open.
**Story (Nov 13):** this item is now the voice of *The Paradox*: pilot barks in character (Wren bold, Kato analytical, Moss steady), the transmissions (static, then human voices, then the pilots' own voices), the recordings inside the Dominion, intro narration and the ending distress signal. Engineering fires the events with placeholder lines; your text and audio replace them (`storyline.md` §5, §10).
- `DT_Barks` (about 80–120 lines: Wren, Kato, Moss and the Armada) played on
  `OnBarkRequested` through a priority queue, with radio-filtered VO (AI-voiced, credited) and
  subtitles.
- **Intro sequence (30–45 s, skippable):** the squad over Verdance's ring, then the Armada warps in.
  **(Nov 12, Ryan)** Engineering builds the intro as real-time shots in code (ENG-33); presentation supplies
  narration voice, music, the grade and optionally its own Level Sequence (`UTDUIStyle::IntroSequence`).
- **(Nov 13 evening) Words the complete game needs, assigned to Nick's side by Ryan's Lead (Ryan, Nov 13)** (Nicholas
  owns the wording; T3 seeds drafts so nothing is mute). Done when each has an approved row by id in `DT_Barks` /
  `DA_Story_Transmissions`, or Nicholas accepts the draft:
  1. **Answers first** (they block engineering): COMMAND or Control; who boards (neutral lines, a variant per pilot, or
     always Wren); the three-pulse definition (interval and tone, shared by the intro, Cinematic 03, the checkpoint door and
     Cinematic 08); the stinger caption; the Cinematic 08 readout; `G6_Wren_CoverTheBay` against Cinematic 07's first line;
     `A5_Choice` and `A5_Escape` kept, reworded or replaced; tags for the Future voices after the reveal.
  2. The lines of every sequence in ENG-42 (about 28 new ids for 07-09, plus `C3_`-`C6_`, `G6_`, waves 3-4).
  3. Every `dialogs.md` id (ENG-43), in character.
  4. Victory, credits and on-screen texts (door access texts, objective titles if he rewords them).
- **Voice:** the approved pickup sheet first (the Story Director's C08 exchange is in progress), then the intro and
  mission lines, then barks; the one shared "Don't-" asset (Audio, `ArtSource/Audio/FirstContact01/`); radio filter.

#### ART-12 · Cinematics & the Director's Cut presentation · `🟨`
**Story (Nov 13):** slate, "CUT!" and name cards can lean into the paradox (a take number that glitches from Cinder onward; `storyline.md` §7).
- **Sequences:** the Dominion reveal, the reactor breakup and victory hero shot, landing and
  take-off shots, the planet descent breakthrough.
- **Theme hooks (C-03 #1–#3):**
  - the switch "cut" transition and name cards
  - director-cam rigs, framing and grade for the slow-motion highlight moments
  - scene slate cards and the "CUT!" restart
- **Trailer capture:** Movie Render Queue shots for ART-14.
- **(Nov 13 evening)** The sequences above are now played as voice-over scenes and transmissions (ENG-42); none needs a
  Level Sequence for the game to be complete. Still Nick's: the designed card set (slate, CUT!, name card, letterbox) on
  `DirectorWidgetClass` (N-038), the take number that glitches from Cinder on (storyline §7), and the shot choices for the
  trailer re-capture. Heavy capture and rendering stay on Ryan's side (T3's `Tools/td-trailer.ps1`).

#### ART-14 · Submission presentation · `🟨`
- **November 14 demonstration:** [actual Shipping 6fe3e90f footage](docs/visual/Submission/LatestGameplayDemo01/README.md) contains 200 seconds of opening and early combat with game-process audio. Captured slice only; batch M, full tutorial-fade proof and human acceptance remain pending. This demonstration does not replace the required 30–60-second submission trailer with gameplay in its first five seconds.
- **Gameplay video (30–60 s):** gameplay in the first 5 seconds. Dogfight → switch "cut" → portal
  missile return → ram → planet landing → blade deflect → Dominion reactor.
- **itch.io page:** cover art, 5+ screenshots and a GIF, the description, controls, credits and
  "Tools you used". Compile the **dev log** (Glass Box prize) from `DEVLOG.md` with Ryan.
- **(Nov 13 evening) The submission package, assigned to Nick's side by Ryan's Lead (Ryan, Nov 13)** except where a row
  says otherwise. Target: submitted by Nov 15 about 15:00 EST, re-uploads until 19:00 at the latest. Each row is done when
  it is on the itch page or in the jam form and both humans have read it:
  | Deliverable | Done when | Owner | State |
  |---|---|---|---|
  | Packaged Windows build `TeamName_GameName.zip`, under 1 GB, uploaded (butler if needed) | ENG-28 size gate; G4 played from download | Lead (Ryan's side) | 🟨 |
  | Gameplay video, 30-60 s, gameplay in the first 5 s | Shot list (`docs/visual/Submission/trailer-45s.json`) updated to the final beats; footage re-captured after the final ships and effects (T3); the edit with music by Nick's editor | Nick's side (edit, shot list), T3 (capture) | 🟨 |
  | itch page text: description, controls (KB / M and pad), screenshots (5+), a GIF, cover and banner | `docs/visual/Submission/itch-page.md` complete; images from the final build | Nick's side | 🟨 |
  | Dev log write-up for the Glass Box prize | A readable story of the jam from `DEVLOG.md` and both sides' boards, with AI workflows that worked and failed | Nick's side, Ryan's Lead supplies facts | 🟨 draft `docs/visual/Submission/glass-box.md` (a980fa7f) |
  | `CREDITS.md` audit: every third-party and AI asset with source, tool, licence and who added it; "Tools used" (Claude Code, OpenAI Codex, Hyper3D Rodin, fal, PBRgen, Grok, Meshy, Tripo, Unreal MCP, Blender); pre-jam declarations (skills and scripts set up before Nov 12) | Checked against the asset builder's cook diff (ART-02) | Nick's side | 🟨 |
  | Team name, members' names and Epic-account emails, game name in the form | Entered by the humans themselves | Ryan and Nicholas | ⬜ |
  | Optional web build | Not in any plan so far; Ryan decides (default: none) | Ryan | ⬜ |

#### ART-19 · Music and cue playback wiring (assigned to Nick's side by Ryan's Lead, Ryan, Nov 13) · `🟨`
Moved from T7 (`CLAIMS.md` "Music hookups", N-045) to Nick's side; **T7 is the fallback** if it is not on `main` by Nov 14
noon. Light work: no cooks of the whole game, no Niagara, no Blender.
- Nick's side binds its delivered tracks and cues in `Plugins/TriadDriftHUD` and its `DA_*` presentation assets, listening
  to the gameplay events of §3.4 and the story events (`OnPhaseChanged`, `OnWaveStarted(Intensity01)`, `OnTakeStarted`,
  transmissions, the objective and completion events, `UTDAudioStateSubsystem`). Ryan's side only exposes any missing
  event or `UTDUIStyle` slot, on request on the board, within the hour.
- In this order: credits (CreditsMusic01, from the roll); Dominion interior loop (from the hangar, yielding to combat);
  Verdance theme (from arrival after the descent reveal, ducking at the terminal, fading on take-off); tutorial prelude
  and loop (#24); then the descent arrival, Cinder, assault, reactor, escape and victory cues as Nicholas delivers them
  (ART-10 list).
- **Done when:** each cue is heard at its beat in a played Development build from `main` (Audio's own `-Audio` capture is
  fine), loops and fades as stated, and the Shipping cook has it (Ryan's Lead checks the packaged build).
- Depends on ART-10 (the tracks), ENG-42 and ENG-44 (the events).

#### ART-20 · Independent code reviews of Ryan's lanes (assigned to Nick's side by Ryan's Lead, Ryan, Nov 13) · `🟨`
AGENTS.md §11 "Independent review": every lane is reviewed by an agent that did not write it before its item can be 🟢.
Reading diffs is light work, and Codex is a reviewer that did not write the code.
- Per review: the item's text here is the spec; findings as BLOCKER / MAJOR / MINOR, each with a concrete failure (input,
  expected, actual, file and line); posted on the board as `Review: ENG-xx`; Ryan's Lead routes fixes and asks for the
  re-check.
- Queue, by what unblocks 🟢 soonest: ENG-41 and ENG-40 (tutorial), ENG-42 (sequences and the story test), ENG-30 (reveal
  gate), ENG-44 (ending), ENG-29 and ENG-31, ENG-36 (HUD states), ENG-45, ENG-38 stage 1, ENG-39, ENG-34, ENG-35 code side.
- Read-only: Nick's agents review from `main` or a published branch and edit nothing of Ryan's.
- **Done when:** every item in the complete-game checklist has one closed review on record.

#### ART-21 · Playtest checklists and per-act test scripts (assigned to Nick's side by Ryan's Lead, Ryan, Nov 13) · `🟨`
- One script per act (front end and Act 1, Verdance, waves 3-4 and Cinder, assault and docking, interior and reveal,
  reactor, escape and ending) in `docs/visual/` (Nick's docs folder): the steps a tester performs, what must be seen and
  heard at each beat (lines by id, music, HUD state), the retake and Continue checks, the expected time.
- Plus the presentation checklist of AGENTS.md §11 per act (silhouettes, colour code, clipping, sound in context).
- T7 runs them in the playtest lane (ENG-46); the humans use them at gates G1-G4.
- **Done when:** T7 has run every script once on `main` and logged the result in `docs/eng/PLAYTEST-*.md`.

### STRETCH: boarding finale visuals

#### ART-15 · Dominion interior kit & core visuals · `🟨` WANTED (Nov 13; Ryan's side: environment and structures builders)
- **Interior kit:** modular pieces on a 400 cm grid (corridor straight, corner, T, doorway, shaft,
  catwalk, door) in gloss black with red strips; giant eye lenses that track the player.
- **Core chamber:** the white-light reactor with rotating rings, plus the collapse and alarm states
  for the escape run.
- **(Nov 13 evening) Correction:** WANTED, not stretch (Ryan, Nov 13), and **🟦 Ryan's** since the afternoon of Nov 13 (§1:
  Environment and VFX moved). The kit is built walkable per ART-17 (50 cm grid, trim sheet; the 400 cm module above becomes
  eight grid cells); lighting and alarm and collapse states by the environment builder (ART-13); the reactor rings turn
  (ART-08). State: interior played end to end on the engineering kit; kit r4/r4b interior and bay on main 6fe3e90f (8ce5c3fb, 71fefa4b); r8 01832cd7 accepted and r9 / r10 in batch M; plans on main (#77, #82) (`🟨`).

#### ART-16 · Boarding & escape cinematics/VFX · `🟨` WANTED (Nov 13; Ryan's side: effects builder)
Tractor-beam docking shot, hangar landing, core-destroyed sequence, escape debris and explosion
VFX, and the squad's launch from the hangar.
- **(Nov 13 evening) Correction:** WANTED and **🟦 Ryan's** (effects builder, T3 for camera beats). For the complete game:
  the tractor beam, the reactor heart's death, interior collapse and debris, the hangar-mouth launch, and the breakup
  (ENG-37). The docking and core sequences play as the travel plus lines (ENG-42), so no Level Sequence is required.
  State: debris and explosions exist in the escape flight (T4); the rest not started (`🟨`).

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

**Complete-game order (Nov 13 evening):**
```
Nicholas's answers (ART-11 #1) ─► T3 seeds ids (ENG-42) ─► each lane fires its lines ─► story test ─► ART-20 review
ENG-40 withdrawal ─► ENG-41 skip ─► ENG-42 Aftermath          ENG-30 reveal gate ─► Cinematic 08 queue ─► reactor fight
ENG-25 / 26 reduced forms ─► ART-17 walkable sites ─► ART-18 props ─► ART-13 lighting ─► planets replayed
ENG-18 assault pass ─► ENG-29 docking lines ─► ENG-31 escape lines ─► ENG-44 ending ─► ART-19 music on every beat
ENG-47 decision (Nov 14 20:00) ─► ENG-28 size + perf ─► ENG-46 gates G1-G4 ─► ART-14 submit (Nov 15 ~15:00)
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

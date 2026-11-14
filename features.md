**Nick Lead runtime checkpoint — November 14, 18:50 EST:** New evidence in sessions40–42: active-escape core re-entry suppression passed on `48a6fd97` with disclosed setup fixtures; intro-bed actual skip at10.5s requested its0.30s fade and the bed was absent from later active-sound dumps on `f85203b1`; sampled asteroid splits with forced array growth passed on `508edec6`. The original asteroid dangling-reference finding is resolved specifically against integrated `d6a98bee` plus this independent source/runtime evidence, not the owner's unavailable branch SHA or whole issue139. Cinder #194 remains a **BLOCKER** per Ryan's triage; legacy-save recovery #96 remains **MAJOR**, awaiting their existing owners. HUD marker/ground-card changes `07384269` are integrated in `1e09640a` but not yet independently retested by Nick. All three sessions exited0; no Lead native operation remains. Original specialists stay held; Asset Handler has a separately authorized isolated intake/native slot. No whole-game, package, listening, performance or AAA acceptance. See the new `docs/visual/STATUS.md` checkpoint for evidence and limits.

**Nick Lead runtime checkpoint — November 14, 18:20 EST:** Sessions 34–39 supersede the older Nick evidence status below. The Moss once-per-run cold-Continue regression passed on `2f07ab5c` ([34–35 evidence](https://github.com/rthomas24/2026-epic-ai-megajam/issues/96#issuecomment-6885414676)). Two **MAJOR progression failures remain on `48a6fd97`**: the original legacy save restores beyond a locked Warden exit and cannot reach the required record ([38](https://github.com/rthomas24/2026-epic-ai-megajam/issues/96#issuecomment-6885700975)); Cinder's walking route drops about 20 m below its course and melee conduits ([39](https://github.com/rthomas24/2026-epic-ai-megajam/issues/194#issuecomment-6885763579)). Existing T5 and terrain/environment owners retain fixes. HUD live-switch card/hint readability passed narrowly at 720p; projected-marker overlap remains ([37](https://github.com/rthomas24/2026-epic-ai-megajam/issues/103#issuecomment-6885592745)). No whole-campaign, package, audio-listening, performance or AAA acceptance. All these sessions exited normally; no Lead-owned Unreal process remains at this checkpoint. Lead playtesting and Issue Coordinator work remain authorized; original Story/UI/Audio/Auditor tasks and schedules stay held. Human checkout and preserved local changes were not updated by these private builds.

**Nick Lead checkpoint, November 14, 17:35 EST:** Lead headless playtesting explicitly resumed; original Story/UI/Audio/Auditor tasks and schedules remain held. Issue Coordinator and Asset Handler have separate direct authorizations. Batch 4 PDF and original evidence delivered on [#140](https://github.com/rthomas24/2026-epic-ai-megajam/issues/140#issuecomment-6884669972). Private Build14 b8a55662 compiled; sessions31-32 reproduced Face to Face replay after Quit to Menu, process exit and cold Continue; [original saves/logs/inputs delivered](https://github.com/rthomas24/2026-epic-ai-megajam/issues/96#issuecomment-6885161858), T3/T5 accepted the defect and fix is pending. This is separate from the legacy final-record bypass. Human main remains at8b4eabae with its local project change preserved; private test updates do not update Nicholas's launch project. No whole-game, audio, performance, package or AAA acceptance is implied. Historical checkpoints below remain preserved.

**Nick Lead checkpoint, November 14, 13:00 EST:** Batch 3 delivered; natural input-driven offscreen campaign reached Dominion assault after Wave 5 on take 1 (runtime b5d1549c). Assisted steering/aim and inspection pauses are disclosed; no full campaign, audio, performance or subjective quality acceptance. Existing Story/UI/Audio/Auditor tasks and schedules remain paused. Nicholas separately authorized the Issue Coordinator for bounded issue/source reconciliation; no specialist production resume. See N-099 and the [Batch 3 report](https://github.com/rthomas24/2026-epic-ai-megajam/issues/140#issuecomment-6881646670). This supersedes older availability/checkpoint descriptions without deleting them.

**Current Nick-side directive, November 14:** Nicholas paused all existing Codex specialists after the morning reconciliation, then authorized Lead-only systematic playtesting, feedback delivery and scoped polish under existing ownership. Story, UI, Audio and Auditor tasks/schedules remain paused; preserved source and queues do not restart automatically. This supersedes the availability notices below. See N-098 and issue #90.

# Triad Drift: Feature Spec & Work Split (Ryan × Nick)

**Nick-side resumed November 14, 08:20 EST:** Nicholas authorized resumption (N-091). Current bounded specialist reconciliation tasks are in Nick-side CLAIMS.md. This supersedes the stopped notice below; historical queues do not restart automatically. Engineering fallbacks and owners continue; no local native job or subjective approval granted.

**Nick-side availability, November 14 at01:16EST:** Nicholas explicitly stopped his Codex agents at safe checkpoints. Lead, Story, UI/HUD, Audio and Auditor are stopped; Lead's goal and the recurring issue check are paused. Existing features and acceptance gates remain unchanged. No Nick-side queued row authorizes restart before Nicholas resumes the team. Ryan's development continues under its existing authority. See CLAIMS.md and MESSAGEBOARD N-090.

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
- **Added Nov 14 (Ryan, #201), core:** shadow mode for the wingmen (on by default), wingmen that use their signature powers and abilities (replacing "wingmen use basic attacks only"), aim assist on all three ships, and enemies hard but fair (ENG-08, ENG-05, ENG-27).
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

Since Nov 14 09:15 EST (Ryan): ✅ also means played by an agent in the running game and judged good; reports still say agent-played.

Since Nov 14 10:15 EST (Ryan: "prioritize by easiest to knock off ... i want to be feature complete soon"): an item is ✅ when every player-facing part named in its heading or one-line description has been played by an agent and judged good, on main or on a batch that has since reached main. Polish, tuning, placeholder words awaiting Nicholas, dev-only quirks and a not-yet-accepted optional effect do not hold the marker: each moves to the [AAA polish list](#the-aaa-polish-list-both-days-in-priority-order) (or the item's "still missing" cell, marked polish) with its owner. An item stays 🟨 only if a player-facing part is unbuilt, unplayed, or has an open BLOCKER or MAJOR. The same rule applies to the complete-game checklist rows. Nick-side items change only from Nick's side's own reports; Nicholas's sign-off on his art, audio and story direction is unchanged.

Since Nov 14 10:25 EST (Ryan: "stop worrying about running tests ... full throttle on features"): the integration gate is compile-only; tests are no longer part of the done bar, and a missing test run never holds a marker.

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
the weakest state among the row's items, from `docs/eng/STATUS.md`, `CLAIMS.md` and the board; **20 rows are ✅ at 18:30** (the three Cinder rows are 🐞 for the route blocker; 22 at 17:30, 22 at 16:30, 24 at 15:30, before Ryan's #194 put the two planet arrival rows back to 🟨; 19 at 14:30, 17 at 13:30, 16 at 12:30, 14 at 11:30, 9 at 10:30) under Ryan's rule of 10:15 (a row is ✅ when its named beats were played good, whatever its items' markers)
(09:30: ENG-02 and ENG-07 are ✅ under Ryan's bar of 09:15, but every row still holds a weaker item). Pure polish is in the [AAA polish list](#the-aaa-polish-list-both-days-in-priority-order) and its
"final polish (after freeze)" group, not here.

| Act | Beat (storyline.md) | Delivered by | State | What still stands between today and done |
|---|---|---|---|---|
| Front end | Boot to main menu: New Game, Continue, Settings, Quit; menu music | ENG-19, ART-09, ART-10 | ✅ | Styled menu implementation on main ee5dc0ff; MainMenuWidgetClass activation and a human pass with pad and mouse remain **(05:00)** Played again by an agent (T7, 04:14-04:29, real keys, batch O tree): New Game, Skip the tutorial, Continue disabled with no save, Settings, Quit all work  **(10:30)** Menu played (T7: New Game, Skip, Continue, Settings, Quit) and Continue GOOD (T3). **To close: menu music confirmed playing at boot** (T3, from the log; PR #11's loop is Nicholas's). Nicholas's menu findings 1, 3, 4, 5 are polish (environment builder, T3, effects builder); 2 and 6 are his side's by ownership, awaiting his answer (#90)  **(11:30)** Menu fly-by findings B1-1, B1-3 and B1-4 closed on agent sight (#141, #143, #144); a menu music log line is on main (d82e682c, AK). **To close: menu music confirmed playing at boot** (T3, from the log)  **(13:30)** **🟨 → ✅:** menu music confirmed playing at boot by log (`MUS_Menu_EtherialPulse_01`, T1, #121). Polish: Nicholas's menu treatment (#142) and the menu-loop join (#134) are his  **(15:30)** The main menu styled natively on main 8d62d14a (ea7f88ff, #142; Nick's Lead approved): button states, W / S, Esc back, a confirm before NEW GAME replaces a saved run. Polish: the mouse and the package checked (T1), the loop join (#134, Nick's side)  **(16:30)** The styled menu played GOOD in Shipping 8d62d14a with a real mouse and keys (T7: hover, click, W / S, confirm with BACK focused, a held Enter does not confirm); the hover fix is on main 28c2fb05 (batch BV). Polish: #142's arrows / pad, Skip Tutorial guard, Esc on difficulty and three-resolution frames (T7, next package); the black menu for 8 s after launch (packaging agent) |
| Front end | New Game: difficulty, first play forces the tutorial, a later New Game offers "Skip the tutorial" | ENG-41, ENG-45, ART-09 | ✅ | On main 6fe3e90f and played by an agent (T7): the offer shows on a completed profile and Skip lands in the Aftermath state; review open; T4's clear skip start (0a4e7501, #90 no. 10) is on main f39a8372 (batch N), awaiting agent play; New Game difficulty (ENG-45, T5, queued) **(05:00)** T7 used New Game and Skip the tutorial again on the batch O tree; the review (ART-20) is still open, so ENG-41 stays 🟨 **(09:30)** ENG-41 is 🟢 under Ryan's bar of 09:15 (agent-played; the separate review no longer gates ✅); not replayed on main since batch U (T7); the New Game difficulty choice is still queued (T5, T3)  **(10:30)** **To close: the New Game difficulty choice is unbuilt** (T5, T3); the forced first-play tutorial and Skip are GOOD (ENG-41 ✅)  **(11:30)** **🟨 → ✅:** the New Game difficulty page seen good by T3 in the running game (d82e682c, on main AK: Story / Standard / Ace, Ace set and the run started; #127); first play and Skip GOOD (ENG-41). Polish: Skip the tutorial through the same page (T3) |
| Opening | Intro, 49 s (Nicholas kept 49 s, reconciliation 17:15), skippable, hands off to the tutorial | ENG-33, ART-11, ART-12, ART-19 | ✅ | #49 fixed on main: names no longer clipped (3c76874c / 9fe5d373, main 6fe3e90f) and the trait line no longer repeats the role word (e234e950, main 61404a79); captured live by T3 at 1280x720, 1920x1080 and 2560x1080 (agent-driven); issue #49 stays open for Nicholas to close (the card's look is his). Still to do: the five legacy `Intro_N` captions still show on main (N-088); T3's rebuild of the opening's words (the 15 approved `O_*` first-contact lines as subtitles over the intro shots, behind `td.Intro.Lines`, 0 = the old captions; 77de9085, Intro / Story / E2E 33 of 33; approved by Ryan's Lead) is on eng/t3, not on main, and removes them; the voice for those lines is Nicholas's; the longer silences after the shortened lines (Nicholas); the re-shoot once with the final ships and effects; intro score and narration voice **(05:00)** T3's rebuild is now in batch U's gate (lead/int-u dea28a8b, not on main): the 15 approved `O_*` lines behind `td.Intro.Lines`, the departure-radio guard, a black hold on the intro's first frames, and a HOLD TO SKIP hint that shows by itself; T3 played the opening's 11 lines at two resolutions on its branch; T7 played the hold-to-skip on the batch O tree **(06:00)** T3's rebuild from the 15 approved `O_*` lines (`td.Intro.Lines`), the black hold on the first frames and HOLD TO SKIP shown by itself are on main 50c4af7a (batch U); the black hold and HOLD TO SKIP were seen live by T3 (agent-driven, 50c4af7a); story lines obeying the subtitles setting are on lead/int-v (15f04846), not on main; the 49 s re-shoot is still open **(06:30)** Every story line path (transmissions, console recordings, the opening's lines) obeys the subtitles setting on main 94b56d11 (batch V), tests only; the 49 s re-shoot is still open **(09:30)** Intro GOOD (T3, agent-played on main): ENG-33 is 🟢. The 49 s re-shoot with the final ships is queued (T3); the voices are Nicholas's  **(10:30)** **🟨 → ✅:** intro GOOD, black hold and HOLD TO SKIP GOOD, hand-off to the tutorial GOOD (T3, T7). Nicholas's findings 7-11 and the 49 s re-shoot are polish with owners (AAA list)  **(14:30)** #148 opening shot polish closed GOOD (exposure -3.0 EV at the eclipse, 9ab7fc5d)  **(17:30)** #146 the Hero's Wake opening bed plays in the game on T3's branch (Nick's Lead's hand-off; the skip fade unproven; not on main)  **(18:30)** #146 the opening bed is on main f85203b1 (61961604); the skip fade not yet shown (T3) |
| 1 First Contact | Tutorial battle phases 01-04 (evade, return fire, fight as Triad, heavy contact) over waves 1-2, in a destructible, solid field | ENG-40, ENG-13, ENG-14, ENG-36, ENG-42, ENG-48 | ✅ | Waves 1-2 and the `TB_*` lines on main 6fe3e90f, played by an agent; the phase prompts v1 (EVASIVE MANEUVERS, TARGET AND FIRE, SQUAD SYSTEMS; db27e563) are on main f39a8372 and the phase clock that counts only while the player flies (e36304e8) on main 61404a79, both awaiting agent play; T4's halves (lesson gate, softer Evade volley, gunship hold, withdrawal wait) are in the T1 + T4 combined commit on their branches (for batch T), not on main; T3's wingman rock avoidance is in batch S's gate; three checkpoints, the gunship optional **(05:00)** Played by an agent (T7, batch O tree): the three lessons (evade 18 s, target and fire, squad switch 22 s) with their lines in order, 8 manual switches each honoured with its pilot card, about 4 minutes of combat with no wingman lost to a rock. T3's rock avoidance is on main 4eb57848 (batch S). T4's halves (lesson gate, softer Evade volley) and T1's hints by phase are in batch U's gate, so the tutorial changes again: replay on main pending **(06:00)** The lesson gate, the softer Evade volley and hints by phase are on main 50c4af7a (batch U), unplayed on main; warp-in points that never sit inside an obstacle are on eng/t4 **(06:30)** T7 played the real tutorial on main 50c4af7a (agent, real keys) on its way to the *Aftermath* replay. Core check by T1 (agent-driven, wave 3): 13 mid-action switches, all instant, no stuck power, worst frame near a switch 15 ms (median 6.5 ms, p99 16.3 ms), no BLOCKER or MAJOR. Warp-in points never inside an obstacle are on main 94b56d11 (batch V); a rapid-switch test (20 presses in 2 s) on main 6cc4f254 (batch W) **(09:30)** The lessons GOOD (T3, real keys, main); waves 1-2 GOOD (T4). ENG-40 is 🟢. Rocks on the route NOT GOOD (T6: 436 layout rocks pushed out of the journey corridors, building); ENG-48's breaks, drops and impact NOT PLAYED on main  **(10:30)** **🟨 → ✅:** the three lessons GOOD (T3, real keys), waves 1-2 GOOD (T4), the field solid and breakable (T6: breaks, ram, drops GOOD; rocks off the corridors, seen live; ENG-48 ✅) |
| 1 First Contact | Withdrawal: Command orders it, pursuers stop at a boundary; crossing it ends the tutorial | ENG-40 | ✅ | On main 6fe3e90f (00e23e85), played by an agent (T7): boundary, marker, pursuers break off, one completion event, music fade on the crossing; the objective still reads CLEAR THE AREA (T5); 'turn back' warning in the corridor (#90 no. 9): T1's boundary rule and release hook (9cfa5ec9) are on main f39a8372, T4's release call is not on main yet **(05:00)** Played again by T7 (batch O tree) by flying at the marker: works. MINOR: the objective still reads CLEAR THE AREA during the withdraw order (T5's withdrawal objective text is on a branch). T4's boundary release is in batch U's gate. Decided by Ryan's Lead: the corridor keeps its Verdance bearing and gains a jump-point waypoint (T4, branch), no speed cap **(06:00)** The boundary release is on main 50c4af7a (batch U); the objective text for the withdraw order (285d4869) is still on eng/t5 and the JUMP POINT waypoint (48124d88) on eng/t4 **(06:30)** The withdraw objective text (285d4869) and the JUMP POINT waypoint (48124d88) are on main 94b56d11 (batch V), tests only, not played on that tree **(09:30)** Galaxy travel withdrawal → JUMP POINT → Verdance GOOD (T4); the withdrawal waiting for Done not watched (T3)  **(10:30)** **🟨 → ✅:** withdrawal → JUMP POINT → Verdance GOOD (T4); boundary and pursuers played (T7). Polish: the withdrawal waiting for Done watched (T3) |
| 1 First Contact | *Aftermath* (`S2_*`), tactical lanes, navigation pulse, Dominion light row in rhythm | ENG-42, ENG-36, ART-18 | 🟨 | Fired on the withdrawal event, on main 6fe3e90f (13623956); lost when the player boosts into Verdance (#90 no. 6): T3's replay 2b6be288 (main eb1bf334) and T1's atmosphere-entry hold (main f39a8372) are both on main, awaiting agent play; lanes, pulse and light row still new **(05:00)** **🐞 MAJOR, played by T7 on the batch O tree (#90):** a player who keeps boosting after the withdrawal flies into Verdance's atmosphere mid-sequence; the entry was held once, then the game travelled 1.4 s later in the middle of Wren's line; the hyperdrive jump beat and 17 queued story lines never played and the planet arrival was silent. The two fixes named above did not stop it. Owners and fixes, all on branches, none on main: T1 (the entry holds for the whole sequence: a crawl at the atmosphere's edge, AWAITING ORDERS), T4 (jump-point waypoint), T3 (must-hear lines survive a map travel). Decided by Ryan's Lead: the entry commits after *Aftermath* if the player is still at Verdance's edge. Back to 🟨 when the fixes are on main; T2's *Aftermath* HUD lanes are on a branch **(06:00)** **Stays 🐞:** T1's hold when boosting into Verdance (23ffe072) and T3's must-hear lines across a map travel (fd06afe5) are on main 50c4af7a (batch U) with passing tests, **not replayed by an agent**; T4's JUMP POINT waypoint is on eng/t4; the navigation-pulse getter for the light row is on lead/int-v (039ff207); the light row effect is not yet accepted. 🟨 when T7 has replayed the beat while boosting and it holds **(06:30)** **🐞 → 🟨: fixed and played.** T7 replayed the beat on main 50c4af7a through the real tutorial with real keys while boosting into Verdance: the entry is held while the sequence plays, all 16 lines air, the entry commits when it finishes (#90). MINOR found there (the entry committed 2 s into the 4.8 s `A1_TrailToVerdance` line): T1's fix (the hold waits for the line on air, capped 6 s; 43703035) is on main 6cc4f254 (batch W), test only. Not 🟢: the JUMP POINT waypoint (main 94b56d11) is unplayed, the tactical lanes are open (T2), the light row effect is accepted but only in batch X (not on main), its `AftermathLightRow` slot is on main 6cc4f254 **(09:30)** The *Aftermath* transmissions GOOD (T3); the tactical lanes still open (T2); the light row accepted, its look not judged in play  **(10:30)** *Aftermath* transmissions and hold GOOD (T3, T7). **To close: the tactical lanes on the HUD** (T2, open); the light row's look in play (effects builder)  **(12:30)** The *Aftermath* lanes GOOD (T2 at 1280x720, #103; small fix with T2: the panel closes on a phase change). **To close: the light row's look seen in play** (effects builder)  **(13:30)** No change. **To close: the light row's look seen in play** (effects builder)  **(15:30)** T1's band guard (06abf7d9, on main a71e3f73): the withdrawal corridor no longer stops short of Verdance, so the *Aftermath* starts (it had never started in that case). **To close: the light row's look seen in play** (effects builder) |
| 1 to 2 | Jump to Verdance in one hyperdrive jump; convoys that ignore the squad; Dominion visible behind its field; asteroids and junk shootable and solid | ENG-15, ENG-39, ENG-18, ENG-48 | ✅ | One-jump fix on main 6fe3e90f (030561f9), played by an agent (5.1 km to a 1.7 km stand-off); convoy line on proximity, not a hold; the asteroid boost top-up (f447189e) is on main eb1bf334, awaiting agent play; the space composition in seven regions with 36 set pieces (T6, `docs/eng/SPACE-DESIGN.md`) is on main ddc50a2f (batch R, 934 tests), awaiting agent play; the four default break systems are registered with the warm-up (96e9fdc6) and the asteroid-break rework is on main 6afa3816, the first-break cost not re-measured (#89) **(05:00)** In T7's play the jump beat was skipped by the *Aftermath* bug (row above), so the one-jump path was not replayed. In batch U's gate: T6's first-break fix (first breaks at most 1.22 ms, was 12-19 ms; 4e01e6d1), `td.Hyperdrive.IgnoreLock`, the region set pieces closed out at r4. On branches: T6's regions moved closer to the routes with more pieces; region beacon and haze. The first pickup spawn still costs 25 ms (T2's pre-spawn, branch) **(06:00)** T6's regions closer to their routes and denser, the Shattered Moons on the Dominion route and the checkerboard fix on the shards and hulls (83a231c9) are on main 50c4af7a (batch U), none seen in the running game on main; the Dominion exterior in the Null colour code is on lead/int-v (b24c8bc4) **(06:30)** On main 94b56d11 (batch V): the Dominion exterior in the Null colour code (b24c8bc4), the JUMP POINT waypoint, the hyperdrive spool and blocked barks; none played. The moon shard material fix was seen in the running game by the structures builder (region 6 at 1 km); shard face patches on main 6cc4f254. The approach field effect is accepted and wired to `DA_Dominion_Visuals.DistortionFieldIdle` in batch X (not on main) **(09:30)** Travel GOOD (T4); **hyperdrive NOT PLAYED this morning** (T1); the space field: journey filled GOOD, R1, R2, R4, R5, R6 GOOD, R3 Ice Drift and R7 Outer Dark NOT GOOD, R6 frame time to re-measure (T6)  **(10:30)** Travel GOOD (T4), the field GOOD (T6). **To close: one hyperdrive jump played** (T1, one more short turn)  **(11:30)** **🟨 → ✅:** one held hyperdrive jump played by T1 (10:27: gravity-well refusal, spool, jump, recharge; #104); travel and the field GOOD; ENG-15 and ENG-39 are ✅ |
| 2 Verdance | Descent with no loading screen, arrival from the entry point; landing; step out | ENG-38, ENG-20, ART-13 | 🟨 | ENG-20's parked-ship fix is on main 6fe3e90f (f9f89594, tested), not yet seen again live; the entry hold during *Aftermath* and the climb-out roar (df563569) are on main f39a8372, awaiting agent play; **ENG-38 ships as a smallest complete version** (Lead's decision, Nov 14): on main and awaiting agent play: T1's 9 s glide-lite and mirrored take-off (616b7a8e, 1e7b1fcb; main 4e894365), T2's descent HUD (altimeter tape, hull temperature, ATMOSPHERE / ORBIT; 46e371c1, main 6afa3816), `M_TD_PP_DescentWhite` and the in-cloud layer v1; on branches for batch T: the glide fitted to the cloud-deck rule (the deck sits at the glide corridor + 300 m, clamped 1,100-1,600 m above the pad, so the glide stays 9-10 s), T4's terrain half and async scatter loading (the 330 ms first entry is still on main, #89), the descent haze, cloud veil v2, CloudPunch and GlideWind; atmosphere look per planet **(05:00)** T7 flew the Verdance atmosphere entry naturally on the batch O tree (white cover, 1.69 s map load); that tree is before the glide of batch P, and the landing was done by a console command, so the glide, the landing by hand and the take-off are still unplayed. In batch U's gate (not on main): the T1 + T4 combined commit (the glide placed over a corridor-fitted cloud deck on every entry path, a 300 m floor for uncovered arrivals, async scatter kind loading at 4 ms per frame, descent haze, glide wind slot, exposure ramp). On branches: cloud veil v2, CloudPunch and GlideWind sounds, T2's descent HUD contrast and ORBIT fix, landing gear meshes for the three ships **(06:00)** On main 50c4af7a (batch U), unplayed: the glide over a corridor-fitted cloud deck on every entry path, the 300 m arrival floor, async scatter loading, descent haze, the descent HUD on its own dark plate, ORBIT surviving the travel, landing gear meshes (not wired). T7's Verdance play began with a staged arrival: no parked ship was visible on the pad (MINOR, to be checked on a landing by hand) **(06:30)** T1 verified the natural Verdance route and the glide live (agent-driven). The structures builder saw the three ships parked on landing gear in the running game. On main 94b56d11: the atmosphere-entry and landing barks. MINOR (T5, T7's play on 50c4af7a): the flown ship is missing from the staged Verdance pad; T1's fix for the flown ship at a level change (8e26e739) is in batch X. Accepted from sheets, in batch X only: cloud v2 for both planets, CloudPunch, the glide wind sound. A landing by hand played to touchdown is still not reported **(09:30)** Descent GOOD (T1), entry and exit GOOD (T1), planet arrival and haze GOOD (T4), descent HUD GOOD (T2): ENG-38 is 🟢. **The hand-flown landing NOT PLAYED** (ENG-20, T1); the glide wind not heard on main; the planet arrival bark is on main 80a2ee4d (batch Z), tests only  **(10:30)** **🟨 → ✅:** natural entry and glide GOOD (T1), the hand-flown landing GOOD (T1, 84 s), planet arrival and haze GOOD (T4), on foot (T7). Polish: the glide wind heard  **(16:30)** **✅ → 🟨 (Ryan, #194):** the landed planet feels empty and flat on foot. **To close:** #194 on Verdance: terrain relief, the mountain ring, rocks, huts and trees through the whole zone, ambient effects, seen full in T7's after frames; descent and take-off flown on the new ground (T1); frame rate held (T6). Both huts kits are on main 28c2fb05 (batch BV: Verdance d55fdc15, Cinder fbda8372), not yet placed  **(17:30)** Fill pass 2 on main (the pad GOOD, some route views open), camps partly placed, relief and mountain ring compiling (f6c1853f), ambient effects built but not spawned. **To close:** #194 as above  **(18:30)** Relief, camps, fill and 49 ambient effects on main; T7 walked it PLAYABLE on 96a66f7c with one blocker (the pylon at 4280, 3505) and a hut-wall camera minor; landing, leaving the ship and take-off GOOD (T7, T6, T1). **To close:** the pylon, T7's after frames judged full |
| 2 Verdance | Short route: instruments on one buried line, transponder "verified / unknown" flicker, a patrol | ENG-25, ENG-39, ENG-36, ART-17, ART-18 | 🟨 | Dressed play zone; the transponder flicker is on main eb1bf334 (cda0a770), awaiting a live capture (T2); the Verdance field camp and the relay-mast landmark r1 are on main 61404a79 (captured by the builder, awaiting agent play), the relay mast r2 on main 4e894365; T5's Dress pass (49 story props on Verdance, the kit `BP_Wreck` at the scar's end; 2767300b) is on main ddc50a2f, awaiting agent play; the outpost floor that showed terrain is lifted 3 cm (f19802f0, main ddc50a2f), walk test pending; walkable survey station from the kit (R-043) **(05:00)** T7 saw the first view on foot with the dressing visible (batch O tree, before the Dress pass of batch R); the objectives on foot are not replayed yet. Walkable survey stations (ART-17) are on the structures branch; the Verdance canopy at about 40% coverage is in batch U's gate; the wreck scar is on the environment branch **(06:00)** T7 walked the Verdance route in its agent play (batch O tree). On main 50c4af7a (batch U) and not seen in the running game: the walkable survey station, the camp canopy r2, the canopy at about 40% coverage **(06:30)** The structures builder walked both survey stations and saw the camp canopy in the running game; a fuller camp canopy is on main 6cc4f254 (batch W, 38d032e2) **(09:30)** Not replayed on main this morning (T5)  **(10:30)** Walked by T7 earlier (batch O). **To close: the transponder flicker seen live** (T2); the route replayed on main (T7, playing Verdance now)  **(11:30)** The route replayed on main by T7 (ENG-25 ✅, #116). **To close: the transponder flicker seen live** (T2, #103)  **(12:30)** T2: the transponder marker, name and range track on and off screen and the flicker line plays (#116); B2-04, the name clipping at the right edge, fixed and seen good at 720p (#157 closed). **To close: the VERIFIED flicker itself seen** (T2, #103)  **(13:30)** No change. **To close: the VERIFIED flicker seen** (T2, #103, in T2's HUD turns) |
| 2 Verdance | Recover the signal-analysis device → **Gravity Well**; reward screen | ENG-25, ENG-12, ART-09 | ✅ | Played by an agent on main 6fe3e90f; the award was silent (#90 no. 2): the native announcement and the mount for Nick's reward screen are on main eb1bf334 (944e1ef5, N-081), awaiting agent play (T2's natural-award capture); Nick's reward screen waits on Nicholas's approval of its look **(06:00)** Played by T7 (batch O tree): the device recovered and Gravity Well announced at the device; the wreck-core fight took 5-7 takes, so the Lead trimmed it (ambush Grunt + Grunt, site Grunt + Trooper, the Heavy moves to the relay transfer, mercy from take 2; T5, batch V); dismissing the announcement by keyboard and pad not verified **(06:30)** The wreck-core trim (ambush Grunt + Grunt, site Grunt + Trooper, mercy from take 2) is on main 6cc4f254 (batch W, b3e2f5d8), tests only, not played **(09:30)** Reward screen GOOD when staged (T2); the natural award NOT PLAYED; the wreck-core trim not played (T5)  **(10:30)** **🟨 → ✅:** the wreck-core fight GOOD on main AB (T7 10:32, take 1 with blocking), the reward screen from a natural award GOOD (T2 play turn), Gravity Well used (T2) |
| 2 Verdance | Cinematic 03 *Beneath the Surface*, 40-60 s voice-over: monitor graphic, voices ("Maintain the sequence"), drone wakes blue, three pulses | ENG-42, ART-18, ART-11, ART-19 | 🟨 | Fired, on main 6fe3e90f (cef28834); its dialogue played over the first relay wave (#90 no. 7): the wave now waits for `C3_Moss_Company`, the scene's end or 60 s (9f964b30, main 6afa3816), awaiting agent play; monitor graphic (Nick's #44 PNG or T2), drone prop with blue optic, the three-pulse sound **(09:30)** The C03 relay transmission GOOD (T3)  **(10:30)** The C03 relay transmission GOOD (T3). **To close: the drone waking blue and the three pulses seen and heard** (T5 prop, T3 cue; the three-pulse sound is Nick's Audio)  **(11:30)** The drone waking blue and lifting off the deck seen good by T5 (e4f599ff, on main; #119). **To close: the approved scan seen on the monitor** (#96, from #62); the three-pulse sound is Nick's ThreePulse slot  **(13:30)** No change. **To close: the approved scan seen on the monitor** (#96); the three-pulse sound is Nick's |
| 2 Verdance | Protect the data transfer, 60-90 s, three waves, Heavy disruption → **Cloak**; extraction with the device | ENG-25 | ✅ | On main 6fe3e90f and played by an agent (transfer, Heavy stalls, secured, extraction); the Heavy's guard hint, clang and chip-through (#90 no. 3, 8e758175) and the Cloak announcement (944e1ef5) are on main eb1bf334, awaiting agent play **(06:00)** Played by T7 (batch O tree): the relay, the transfer resuming from its checkpoint, extraction, boarding by holding E, Cloak announced at the pad; the relay took 2-3 takes; MINOR: two crew stood behind the terminal wall for over two minutes; MINOR: Cloak is announced about four minutes after the transfer (T5's "Cloak at the transfer" is on eng/t5) **(06:30)** Cloak at the transfer and the encounter stragglers fix are on main 94b56d11 (batch V, 285d4869), tests only. MAJOR still open (T5, from T7's interior play): the Heavy's guard never breaks to quick swings and no lunge hint shows **(09:30)** Not replayed on main (T5); the Heavy's guard and lunge hint judged GOOD in the interior (T7)  **(10:30)** **To close: the relay transfer to the Cloak unlock played on main** (T7, playing now)  **(11:30)** **🟨 → ✅:** the transfer 0 to 90% with its fights and no death, Cloak presented as the transfer completes, back to the ship and take-off, played by T7 on main 9155a6dc (#116) |
| 2 Verdance | Take-off; Cinematic 04 as transmissions (`C4_*`); Verdance theme fades | ENG-42, ENG-38, ART-19 | 🟨 | Lines fired on take-off, on main 6fe3e90f (d89a0043); cut to three calls on main eb1bf334 (N-087), awaiting agent play; the mirrored take-off of ENG-38 is on main 4e894365, awaiting agent play; theme wiring (N-045; Nick's hookup not started; T1's native fallback on main 61404a79 does not cover the Verdance theme) **(06:00)** Take-off, the climb to orbit and the return to the galaxy played by T7 (batch O tree); must-hear lines now survive the map travel on main 50c4af7a (batch U), unplayed **(09:30)** The hand-flown take-off NOT PLAYED (T1)  **(10:30)** Take-off hand-flown GOOD (T1). **To close: the C4 lines and the Verdance theme fade confirmed on take-off** (T1, log; Verdance theme on main AD 83cbddba)  **(12:30)** **Not confirmed** (T4's correction on #96): the C4 lines came from a staged departure, not a flown take-off. **To close: the take-off flown into the galaxy with the C4 lines and the theme fade** (T4)  **(13:30)** The Verdance theme fade on take-off confirmed by log (T1, #121). **To close: the take-off flown into the galaxy with the C4 lines** (T4, #96) |
| Between | Waves 3-4 as the interception on the Cinder route (lines only); clue: an enemy pre-empts a manoeuvre; destructible field with special-asteroid boost drops | ENG-14, ENG-42, ENG-43, ENG-48 | ✅ | Interception lines; the anticipation clue and its bark (storyline §6) **(05:00)** `Bark_Anticipated` and T3's trigger are on main 4eb57848 (batch S; the words are Nicholas's), not heard in a played run; the waves 3-4 interception ids are still queued (T3) **(06:30)** The G4 interception lines (wave 3 start and +10 s, wave 4 start and +6 s, wave 4 clear) are on main 6cc4f254 (batch W, 21040d6b), tests only. T1's switching check ran in wave 3 on a live game **(09:30)** **Waves 3, 4 and each enemy type NOT PLAYED** (T4)  **(10:30)** Waves 3-4 GOOD (T4: interceptor missiles, all types), drops GOOD (T6). **To close: the pre-empt clue's AI side is unbuilt** (T4; `Bark_Anticipated` on main)  **(11:30)** The pre-empt clue's AI side is on main in batch AL (0e740f77: once per wave, a visible tell, a log line), not seen in play. **To close: the clue seen once in waves 3-4** (T4) and the interception lines heard (T4, #96)  **(12:30)** The G4 interception lines aired at their moments (T4, log, #96); the pre-empt clue fired after forced switches (log: wave 4, tell 1, capped at one per wave; #122). **To close: the clue's flash seen in a frame and the Anticipated bark confirmed** (T4)  **(13:30)** No change. **To close: the clue's flash in a frame and the Anticipated bark in the log** (T4, #122)  **(14:30)** No change. **To close: the pre-empt tell in a frame and the Anticipated bark in the log** (T4, #122)  **(15:30)** **🟨 → ✅:** the pre-empt tell played GOOD and #122 closed (ENG-43 ✅); the Anticipated bark in its place |
| 3 Cinder | Descent, short route past one patrol, survey station on the dig's rim | ENG-26, ENG-38, ART-17, ART-18 | 🐞 | Walkable station and dig cabin from the kit; fixed order terminal → excavation → anchor; the Cinder mining camp and the Null derrick landmark are on main 61404a79 (captured by the builder, awaiting agent play); Cinder World / Fall play through T1's native music fallback (main 61404a79, not heard by anyone) **(05:00)** T5's Dress pass on Cinder (50 story props, landmarks, the dig cabin; 2767300b) is on main ddc50a2f; the fixed order and the ID door's one greeting (b4d8ed05) are on main 4eb57848 (batch S); none of it played by an agent yet. Cinder from the air is on the environment branch **(06:00)** T7 played Cinder on foot as far as the gate conduits (batch O tree, staged arrival). The walkable Cinder survey station is on main 50c4af7a (batch U), not seen in the running game; Cinder basalt and crystal fields on eng/env **(06:30)** The Cinder survey station rack fix (no climb over the back rail) is on main 6cc4f254 (38d032e2); the station walk was seen in the running game by the structures builder. Cinder crystal clusters, terrain material and the scar are on eng/env, waiting on the s13 sheet **(09:30)** Not played by a player agent on main this morning (T5)  **(10:30)** **🟨 → ✅:** Cinder landing hand-flown GOOD (T1), the station chain GOOD and the magma route GOOD with 0 deaths (T7, batch Z)  **(16:30)** **✅ → 🟨 (Ryan, #194):** the same finding on Cinder. **To close:** #194 on Cinder: relief, the mountain ring, rocks, huts (the Cinder set is in progress, structures builder), ambient vents and embers, T7's after frames, descent and take-off on the new ground (T1); Nicholas's volcanic ground set is imported (#193, unassigned)  **(17:30)** Hut, perimeter and site sets for Cinder on main; fill and relief not started. **To close:** #194 on Cinder  **(18:30)** **🟨 → 🐞 (BLOCKER on main 48a6fd97):** the pilot walks about 19 m under the course (Nick's session 39). **To close:** T5's slab collision where the relief is lower, T4's relief heights, the route walked again; then #194's Cinder fill |
| 3 Cinder | Cinematic 05 *The Pattern*, 40-50 s: the one Kato echo, "Origin Core", "temporal device" | ENG-42, ART-11 | ✅ | Fired, on main 6fe3e90f (cef28834); approved lines (§25.2) as one queued scene with Cinder overlay **(09:30)** The C05 terminal transmission GOOD (T3)  **(10:30)** **🟨 → ✅:** the C05 terminal transmission GOOD (T3) |
| 3 Cinder | Secure the excavation: gate, areas, Warden (mercy tiers), crystal cache → **EMP Pulse** + crystals | ENG-26, ENG-23, ART-04, ART-08, ART-18 | 🐞 | ENG-23 is 🟨 again (04:00): the Warden's slam contact (1.30 s, on the charge's end; 39810902) and its shield re-rigged to the left forearm (e2e74225) are on main 6afa3816, neither seen in a played run; the Warden death is being rebuilt on a new base (animation builder, branch). An Ace-only bug in batch S (an extra crew member spawning through a wall on Cinder) is fixed on T5's branch and lands in batch T. On main and awaiting agent play: crew deaths ending on the ground (ab2ab66b), per-type deaths and attacks and corpses that lie 10 s (b9bbdb25, 494cbcd7); the conduit marker's bearing crosses lava (#90 no. 11, T5); renames; cache prop (#53); Warden animation set and ragdoll **(05:00)** The Ace-only wall-spawn fix (8e1a4e6d, with a spawn test for every preset) is on main 29a15f21 (batch T). In batch U's gate: the Warden clips moved to `SKEL_Warden` (five had silently fallen back to template clips), the Warden death rebuilt, the Heavy bash raised. On branches: Grunt damage-time jitter, the Warden stagger trigger, live captures of hit reactions, walk and stagger. Not played by an agent **(06:00)** **🟨 → 🐞 (MAJOR, T7's agent play on the batch O tree, #90): the conduit marker leads straight through a lethal magma channel; 21 deaths in 4 minutes.** Owners: T5 (marker routed by waypoints, respawn past the channel), environment builder (magma that reads as lethal), effects builder (`NS_TD_Magma_Heat`, built on eng/vfx), structures builder (a bridge); batches V and W. The play stopped at the conduits: the areas, the Warden and the cache are unplayed. The Trooper hit react and the Warden stagger trigger are on main 50c4af7a (batch U) **(06:30)** **🐞 → 🟨:** T5's magma route is on main 6cc4f254 (batch W, b3e2f5d8): markers over the new bridge, an edge warning and tip, a far-bank respawn after two magma deaths, a route test over 11 legs; the bridge (`SM_Kit_MagmaBridge`) was walk-tested by its builder. **Tests only: nobody has played Cinder on this tree**, so the fix is unproven in play; the areas, the Warden and the cache are still unplayed. The magma surface and glow (eng/env) and the heat effect (built, not seen) are not on main. Lead decision: the geyser row moved about 4 m from the bridge exit **(09:30)** On main 8b4eabae / 80a2ee4d: the magma look and heat effect (seen on foot at 5 m and 20 m), the conduit markers, the geyser row clear of the bridge (structures builder walked it: bridge exit GOOD, no throw). **Cinder past the conduits NOT PLAYED** by a player agent (T5): the areas, the Warden and the cache  **(10:30)** **To close: the gate conduits (markers and `SM_Kit_NullConduit` art in batch AE), the areas, the Warden and the crystal cache played on main** (T7); EMP Pulse itself GOOD (T2)  **(11:30)** Both gate conduits seen with the new art and markers (structures builder, 11:10; ART-06 ✅); a CUT at the gate now returns to the far bank (3445cc64, AK). **To close: T7's hand play from the gate: the guards, both conduits broken, the areas, the Warden, the cache** (#117)  **(12:30)** **🟨 → ✅:** played good by T7 (agent, real keys with blocking) on main adc75bc5 from the far-bank checkpoint, nothing skipped: both gate conduits broken with the blade, yard, barracks, dig, the Warden on take 1, the crystals recovered, EMP Pulse presented; takes per encounter 2 / 2 / 2 / 3 / 1, 5 deaths in all (was 24), mercy logged on every retake (#117 closed 12:24)  **(18:30)** **✅ → 🐞 (the same BLOCKER):** the melee gate route cannot be reached from under the course. **To close:** the fix above and the gate, yard, Warden and crystals walked again on main (T7) |
| 3 Cinder | Disable the field anchor: bridges, three charges, run from the collapsing tower; HUD "approach prepared, passage pending" | ENG-26, ENG-36, ART-18 | 🐞 | Anchor tower (#52) and its collapse; pending marker **(05:00)** The pending state (`IsFieldAnchorDisabled`, saved; b4d8ed05) is on main 4eb57848 and T2's on-foot line "APPROACH PREPARED · PASSAGE STILL BLOCKED" with its marker (e1cf2fbb; the wording is a placeholder for Nicholas) on main 29a15f21; tested, not played **(06:00)** Not reached by T7's play (stopped at the gate conduits); still tested only **(09:30)** Anchor charge and collapse pieces in use (batch Z): **the charge mesh stretched, NOT GOOD** (structures builder); the collapse NOT PLAYED  **(10:30)** **To close: the three charges planted (prompt and markers on main in AE 9155a6dc, unplayed; MAJOR on AB) and the collapse run played to the exit** (T5, T7)  **(11:30)** **🟨 → ✅:** three charges planted and the collapse run completed, judged good by T7 on main 9155a6dc (#117, #119); the charge is a small pack at scale 1 (seen). Polish: the collapse pieces seen from the shore (structures builder)  **(14:30)** Stays ✅ on the collapse run; the dropped tower sections still never show (ART-18, T5, #119; polish of this row)  **(15:30)** #119 closed as played GOOD (ART-18 ✅): the anchor pieces now drop at their own points and rest on the ground (d13c8f4f, b51963d7); the collapse polish line is done  **(18:30)** **✅ → 🐞 (the same BLOCKER):** the bridges and charges sit above the walking level. **To close:** the fix above and the anchor run walked again on main (T7) |
| 3 Cinder | Take-off; Cinematic 06 as transmissions (`C6_*`), objective "Break the Dominion guard" | ENG-42, ENG-18 | 🟨 | Lines fired on take-off, on main 6fe3e90f (d89a0043); cut to three calls on main eb1bf334 (N-087), awaiting agent play; objective line on wave 5 **(09:30)** Wave 5's objective line BREAK THE DOMINION GUARD is on main 80a2ee4d (9b878002), tests only  **(10:30)** Cinder take-off GOOD (T1), BREAK THE DOMINION GUARD GOOD (T4). **To close: the C6 lines confirmed on take-off** (T3, log)  **(12:30)** **Not confirmed** (T4's correction on #96): the two C6 lines came from a staged wave 5 and the three Cinder-departure lines never aired. **To close: the Cinder take-off flown into the galaxy with its lines** (T4)  **(13:30)** No change. **To close: the Cinder take-off flown into the galaxy with its lines** (T4, #96) |
| 4 Assault | Wave 5; the approach field opens only when Cinder is done **and** wave 5 is cleared; debris and junk destructible and solid | ENG-14, ENG-18, ENG-48 | ✅ | Check the rule really needs both; field-opening effect **(05:00)** Shield drop and approach-field shimmer effects are on the effects branch; the rule check is still queued (T4, T3) **(06:00)** The shield drop effect is accepted from its review sheet (not yet in the game); the approach-field effect is not yet accepted; the rule check is still queued (T4, T3) **(06:30)** The shield drop (on `DA_Dominion_Visuals.ShieldCollapse`) and the approach field (on `DistortionFieldIdle`) are accepted from sheets and wired in batch X (lead/int-x, not gated, not on main); the rule check is still queued (T4, T3) **(09:30)** The rule checked and fixed: the approach field opens with Cinder and wave 5 in either order (T4, 126b238d, main 80a2ee4d), tests only; wave 5 NOT PLAYED (T4)  **(10:30)** **🟨 → ✅:** wave 5 GOOD (T4: BREAK THE DOMINION GUARD, carrier launches, APPROACH PENDING), the approach opens to the assault (T4, T7), rocks break (T6)  **(17:30)** Wave 5's carrier cap (hold at 10, resume under 6) played on main 6f0536ca (T4) |
| 4 Assault | Four generators 0/4, "shield" wording, two Unknown clue fragments, one friendly-status flicker; docking marker | ENG-18, ENG-36, ENG-42 | ✅ | 0/4 to 4/4 with director shots and lines played by an agent on main 6fe3e90f; generator pips on the HUD and the Dominion on the r2 hull's sockets are on main eb1bf334 (cda0a770, b59423d5), awaiting agent play: live captures of the pips and the flicker (T2); wording **(06:00)** Unplayed since batch L. The Dominion exterior in the Null colour code is on lead/int-v (b24c8bc4) **(06:30)** **Played by T7 on main 50c4af7a (agent, real keys): works**: assault score, pips 0/4 to 4/4, kill shots and lines, the docking route opens. The generators were killed with a labelled debug command, so the fight to kill them by hand is not judged. On main since: the Null colour code on the exterior (94b56d11) and four assault lines (`G6_*`; 6cc4f254, tests only) **(09:30)** The Dominion assault GOOD (T4; generators still killed by a debug command); the pips NOT PLAYED this morning (T2)  **(10:30)** Assault, pips and lines played (T7, T4). **To close: the four generators killed by hand, not by debug command** (T4)  **(12:30)** One generator taken to half hull in 7 s of tries; retuned to about 15 s of focused fire at Normal (main AQ 92c94f81), not yet played. **To close: the four generators killed by real fire** (T4, #112)  **(13:30)** No change. **To close: the four generators killed by real fire** (T4, playing now, #112); the radar header after the waves shows WAVE 1 / 5 (MINOR, Nick's side; T6's fix in batch BI)  **(14:30)** **NOT GOOD: the assault is not survivable without god mode** (22 deaths in 8 min, no generator down; ENG-18 🐞). **To close:** T4's rebalance (generator hull 400 → 250, turret damage and telegraph, fighter cap) and T2's 1.5 s grace after an auto-switch, then the four generators by real fire (T4, #152)  **(15:30)** **🟨 → ✅:** #112 closed as played GOOD: T7 without god mode took two generators in 88 s with 0 deaths on a71e3f73 after T4's rebalance and T2's 1.5 s auto-switch grace with shimmer and a HUD line (ENG-18 ✅). Polish: all four timed without god mode and the turret lock telegraph seen in a frame (T4, T7; #152)  **(16:30)** All four generators in 3 min 28 s with one death, no god mode (T7, a9c635de): the timing polish line is done; the turret telegraph enlarged, not yet seen in a frame (T4) |
| 4 Docking | Bay opens, tractor assist, wingmen hold outside, pressure off; Cinematic 07 as six lines | ENG-29, ENG-42, ART-16, ART-18 | ✅ | Docking not verified by the playtest (35 degree cone, #90 no. 19): T4's near-field capture (80 degrees inside 400 m) and approach waypoint (0a4e7501) are on main f39a8372 with a test, awaiting a live play; `C7_*` lines; bay mouth (#54); docking shot **(05:00)** T4 played docking on its own branch (agent-driven); not yet on main's tree. On branches: the tractor beam effect, T6's docking-cone keep-out for the field **(06:00)** Unplayed by T7. T6's docking-cone keep-out (no set piece in the cone) is on main 50c4af7a (batch U) with a test; the tractor beam effect is not yet accepted **(06:30)** The docking route opened in T7's play; **docking itself is unplayed on main** (T1 is playing it now). On main 6cc4f254 (batch W, tests only): `PlaySequence(Docking)` so `C7_Moss_WayOut` airs, the `TractorBeam` cue slot. In batch X: docking carries the flown ship (8e26e739, new test). The tractor beam effect is not accepted **(09:30)** Docking carry GOOD (T1), the docking bay GOOD (T4 with T1), the pace played at 12-13.5 s (was 38-44 s): ENG-29 is 🟢. The hold for the Docking lines NOT GOOD yet (setup-caused, T1 retrying); the tractor and docking HUD plates on main 80a2ee4d, NOT PLAYED; the tractor beam effect not accepted  **(10:30)** Docking GOOD with all three ships, 12-13.5 s (T1, T4). **To close: the tractor beam seen in docking** (T4; accepted, slot filled on main AB) and the C7 lines heard to the end (T1)  **(12:30)** The tractor beam is seen end-on from the chase camera and reads only as a faint ring at the bay mouth: NOT GOOD (T4 frames, #105); needs a wider cone and motes streaming toward the bay (effects builder; Grok Bot's cone reference #174, 35 degree half-angle); the C7 docking and arrival lines aired (T4, #96); the bay mouth seen good at 94 m (#126). **To close: the tractor beam seen in docking**  **(13:30)** The bay mouth, bay look and guide lights GOOD (ART-15 ✅, #126). **To close: the tractor beam seen in docking** (effects builder, T4; #105, #110)  **(14:30)** The tractor beam v2 did not show in the real dock and is back to v1 in the data (8f59cfe1). **To close: the tractor beam seen in docking** (effects builder once T4's hook is wired; #105, #110); the docking lines (T3, #110)  **(15:30)** **🟨 → ✅:** #110 closed as played GOOD (ENG-29 ✅): the docking-line queue fix (b5472653) and the beam fading out at the bay mouth (45a7652d) are on main. Polish: the tractor beam v2 and the mine effect wired and seen (effects builder; T4's hooks are on main; ART-16, #105, #111)  **(17:30)** T4's beam visibility and `SpawnMine` fixes on main (51f45b35). Polish: the beam v2 and mine effect back in the data and seen (effects builder) |
| 4 Interior | Hangar (access granted), deflect corridor, arena, Warden, consoles climbing from Unknown to the plain statement | ENG-30, ART-15, ART-17 | ✅ | Played by an agent on main 6fe3e90f; the pilot spawns against the parked Rift's tail (#90 no. 12, T5); crew readable at 30 m (#90 no. 14, 0b36d0e9) and "Hold E" on the boarding prompt (8e758175) are on main eb1bf334, awaiting agent play; console order and speakers; arena consoles quiet in combat **(06:00)** Unplayed since batch L. `UTDInteriorLightSubsystem` (alarm and collapse lighting) is on main 50c4af7a (batch U), tested headless, not seen in a played run; collapse roles and emergency strips on eng/env and eng/t5 **(06:30)** **🟨 → 🐞.** T7 played the interior on main 50c4af7a from the hangar to the Warden. **NEW MAJOR (T5 + T3, #90): dying in the Warden fight restarts the scene with the objective already at the reactor conduits and no Warden, so one death skips the mini-boss.** Not reproduced headless; a Warden retake test and diagnostics (9e62d3da) are in batch X; T7 is reproducing it live. MAJOR still open (T5): the Heavy's guard never breaks to quick swings, no lunge hint; the arena took 17-18 takes. MINOR (T5): the hangar spawn is directly behind the parked Rift. Lead decision: crew bodies stay 30 s **(09:30)** **🐞 → 🟨: both MAJORs closed in play.** T7, real keys, batch Y + T5's arena fix: hangar to the Warden GOOD, the Warden retake GOOD (four CUTs, the Warden back each time, objective unchanged), the Heavy's guard and lunge hint GOOD, mercy GOOD (arena cleared by take 5), the Warden fight GOOD (beaten on take 5). MINOR: the staged interior start still spawns the pilot behind the ship (the hangar start fix does not take effect on that path; NOT GOOD, T5). Consoles climbing to the plain statement: not judged  **(10:30)** Hangar, corridor, arena, Warden GOOD (T7). **To close: the consoles climbing to the plain statement heard** (T3); the hangar start after a natural docking (T7, fix in AE)  **(11:30)** The hangar start after a natural docking GOOD (T7, main 473543fb; ART-17 ✅, #120); the interior GOOD (ENG-30 ✅, #129). **To close: the three console recordings heard** (T5, #96)  **(12:30)** **🟨 → ✅:** the console recordings in the hangar, corridor and barracks each play on E, seen good by T5 in the running game (#96); the hangar, corridor, arena, Warden and reactor room were already good (ENG-30). Polish (AAA list): the console reads as a plain black box from 1.4 m (structures builder); a tiny centred on-foot text line (T5) |
| 4 Interior | **The reveal:** mandatory final record at the reactor threshold → Cinematic 08 (five `F_*` lines, full warning, the case, the decision), door sealed, skippable | ENG-30, ENG-42, ART-11, ART-19 | 🟨 | The gate (T5), the queue and skip (T3), the readout line (6c4979f2, 64946350, 6828207b) are on main 6fe3e90f; agent-played on the batch L branch, but T7's run on the merged batch skipped the gate by a debug move: no verdict on the reveal yet **(06:00)** Unplayed since batch L; story lines obeying the subtitles setting on lead/int-v **(06:30)** Not reached by T7's play (stopped at the Warden); still unplayed on a merged build. Story subtitles obey the setting on main 94b56d11 **(09:30)** **NOT PLAYED by anyone** (T7 next, after the reactor threshold)  **(10:30)** **To close: the final record and Cinematic 08 played, with the Dont voice (Wren take 04, 6ee7100d)** (T3, T7)  **(11:30)** Not seen yet (#96): the final record, Cinematic 08 through its readout and the reactor door, its skip, the Dont voice (T5, next lock turn)  **(12:30)** Cinematic 08 plays through in order to its end with the Dont voice and the full warning: GOOD (T5, #96). NOT GOOD yet: Face to Face played before Cinematic 08 (the reactor objective started without the final record; not the intended order, T3 replays it in order), and the skip landed on the next sequence. **To close: the order and the skip** (T3, T5)  **(13:30)** The consoles and Cinematic 08 seen good (#96); the #139 carry MAJOR fixed on main AY (played check, T3). **To close: Face to Face after Cinematic 08 in order, and the skip** (T3)  **(14:30)** **Bug (#96): the reactor can be reached without the final record.** Decision: the reactor door is gated on it (T3 state, T5 door). **To close:** that gate played, Face to Face after Cinematic 08 in order, the skip (T3, T5)  **(15:30)** The final-record gate is on main (3b1fcde5: PLAY THE FINAL RECORD is the Warden objective's second step; the exit door, the Core objective and Face to Face wait for it), played GOOD by T5. **To close:** Continue from the Warden checkpoint restoring that step (T3, T5; #96). Face to Face's fifteen `G8_Self` lines wait on Nicholas's words and are kept off air meanwhile (in batch BQ, not on main)  **(16:30)** Continue restores the final-record step on main (ccaaf4d4, c8eef9a5). On T3's branch: the fifteen Face to Face lines and once per run; the natural route played GOOD with them. **To close:** that batch on main (T3, #96)  **(17:30)** Face to Face's lines and once per run on main 0c986a7d; #96 reopened by Nick's side (replay after a quit and a cold Continue); T3's fix 77ad713e compiling. **To close:** that fix on main and Nick's cold-launch recheck; T5's legacy bypass fix  **(18:30)** The cold-Continue replay passed (Nick's sessions 34-35, 2f07ab5c). Open: T5's old-save gate strands a legacy save beyond the locked Warden exit (Nick's session 38). **To close:** T5's recovery and Nick's same-save proof (#96) |
| 5 The Choice | Reactor fight: "Break the reactor conduits" 0/4, "Destroy the reactor heart", one source of pressure | ENG-30 | ✅ | CONDUITS 0/4 and the conduit lines played by an agent on main 6fe3e90f; the conduits carry no world markers (#90 no. 4, T5); T2's conduit count on the HUD (7fb67c3d) is on main f39a8372, awaiting agent play; the core-chamber and reactor-fight music fallbacks are wired on main 61404a79 and T5's interior events (`ATDPlanetCourse::OnInteriorMoment`, 9f964b30) and the `DA_ReactorCore_FX` slots are on main 6afa3816, but T1's forwarding of them to the music bridge (f88a0bfb) is on eng/t1, so both are still silent on main; defenders trio (N-052, T5); `G8_*` lines; pressure in the room **(05:00)** T1's forwarding of the interior moments to the music bridge is in batch U's gate (not on main), so both are still silent on main **(06:00)** Unplayed since batch L. T1's forwarding of the interior moments to the music bridge is on main 50c4af7a (batch U), so the core-chamber and reactor-fight fallback music can play; heard by no one **(06:30)** Still unplayed. MINOR open (T5): the reactor conduits carry no markers. `DA_ReactorCore_FX` is accepted, in batch X only **(09:30)** **NOT PLAYED** (T7 next). On main: conduit markers (batch Y), Face to Face at the reactor and when its defenders fall (f54ed98c, batch Z); T5 decides at 14:00 whether the future-pilot defenders are solid by the freeze (drones are the fallback)  **(10:30)** Conduits 4/4 with markers and the heart GOOD, Face to Face plays (T7, batch Z). **To close: the one source of pressure, the future-pilot defenders, unbuilt** (T5, decision at 14:00; drones the fallback)  **(11:30)** **🟨 → ✅:** the two future-pilot defenders built and beaten on take 1, Face to Face, conduits 4/4 with the blade, the heart destroyed (T7, real keys; #129). Polish: the defenders too easy (T5 raises them to 200 health with a guard), their look captured (T5) |
| 5 The Choice | Escape run 90 s (2.5 s after the heart), debris, "time failing" in sound; reboard | ENG-31, ART-16, ART-19 | 🟨 | T2's escape clock and exit marker (7fb67c3d) are on main f39a8372, awaiting agent play; `G9_*` lines; alarm and hull sounds; escape music: the fallback slot is empty because the Ritual track is not imported on main (Nicholas / Nick's side) **(05:00)** The Dominion escape debris set (`SM_KS_DomDebris_*`, `BP_KS_DomDebris`; captured at 100 m by its builder) is on main 29a15f21. Interior alarm and collapse lighting (`UTDInteriorLightSubsystem`, on `OnInteriorMoment`) is in batch U's gate, tested headless, not seen live **(06:00)** Unplayed since batch L **(06:30)** Still unplayed. Emergency strip tags are on main 94b56d11 (batch V) **(09:30)** **The escape run and reboard NOT PLAYED.** The line "Thirty seconds. Keep going." (Moss) is on main 80a2ee4d, not heard; the Ritual escape track is Nick's side's audio task (N-095)  **(10:30)** The escape run GOOD and the reboard GOOD (T7). **To close: the "time failing" sound, the Ritual escape track** (Nick's Audio, N-095); afterimages stay polish  **(13:30)** No change. **To close: the Ritual escape track** (Nick's Audio); the time-failing sound |
| 5 The Choice | Escape flight 60 s through debris to the radius; three "clear" reports; breakup; debris solid, impact damage by speed | ENG-31, ENG-44, ART-07, ENG-48 | 🟨 | The line says ninety seconds and the flight was cleared in about 6 s (#90 no. 8): T4's escape flight (clear at 3.5 km or more, about 40 of 60 s; 0a4e7501) is on main f39a8372 with tests, awaiting a live play; breakup effects at trailer quality; regroup lines **(05:00)** T4 played the escape flight on its own branch (agent-driven): victory at 41 s of 60; replay on main pending (T7's next session) **(06:00)** Unplayed on main (T4's own branch play at 05:00 is the only run) **(06:30)** Still unplayed on main. On main 6cc4f254 (batch W, tests only): the ClearSpace roll call at the escape clear and the escape debris drawn with the structures builder's `SM_KS_DomDebris_*` **(09:30)** Not replayed since batch W (T4 played it earlier: GOOD, 41 s)  **(10:30)** The escape flight GOOD (T7; easy, tuning to polish). **To close: the breakup and the three "clear" reports seen at the escape clear** (T4, effects builder)  **(12:30)** The G9 line and the three C9 "clear" reports aired at the escape radius, then victory (T4, log, #96). **To close: the breakup seen at the escape clear** (T4, effects builder; the hull chunks and flash of AQ unseen)  **(13:30)** The breakup after the reactor kill seen GOOD (#112: hull sections drifting apart in a fireball). **To close: the breakup seen at the escape clear** (T4; same breakup, not yet seen on this route)  **(14:30)** Breakup effects v2 on main (8f59cfe1). **To close: the breakup seen at the escape clear** (T4) |
| Ending | Victory panel (takes per scene), stinger at 18 s "DATED DECADES AHEAD", signal lines, credits roll, back to menu | ENG-44, ART-09, ART-11, ART-19 | ✅ | Caption and forced stinger on main 6fe3e90f (6c4979f2, 6bfc19fa), seen by an agent with the credits roll and the return to the menu; credits music: T1's native fallback (eb6e3275: Cathedral Pulse (1) then Ethereal Pulse Remix after the stinger) is on main 61404a79, technically verified, not heard by anyone; Nick's own hookup has not started (his side stopped at 01:16) and replaces it on its first claim; the victory eyebrow MISSION COMPLETE // OPERATION PARADOX and random six-string large headline are explicitly approved by Nicholas (Nov 14; Story/README.md) **(06:00)** Unplayed since batch L **(06:30)** Still unplayed since batch L (victory, stinger, credits) **(09:30)** **Victory and credits NOT PLAYED** on the boarding ending (T3, T7); the space ending reached VICTORY in T4's play on lead/int-aa. N-094 archived: the eyebrow and six random headlines stand  **(10:30)** Victory, stinger and credits music GOOD (T7). **To close: the credits roll and RETURN TO MENU checked on screen** (T3)  **(13:30)** No change. **To close: the credits roll and RETURN TO MENU checked on screen** (T3, #95)  **(14:30)** **🟨 → ✅:** the credits roll, RETURN TO MENU and no CONTINUE after victory played GOOD (#95 closed; ENG-44 ✅); the credits name Ryan Thomas (c5f2fa0d, #181). Ear approvals of the credits music are Nicholas's (ART-10) |
| Always | Death → "CUT!" → next take in every scene, never replaying a cutscene | C-03, ENG-19, ENG-46 | ✅ | ENG-46 is 🟨: retake checked in every scene of the new beats; T3's retake fixes (2b6be288) and the trench retake (818d592e) are on main eb1bf334, awaiting agent play **(06:00)** CUT and retakes on foot, with the director's mercy easing each retake and the transfer resuming from its checkpoint, played by T7 on Verdance (batch O tree); 21 retakes at the Cinder magma channel (the MAJOR above) **(06:30)** **MAJOR (see the interior row): a death in the Dominion Warden fight skips the Warden.** On main 6cc4f254, tests only: the far-bank respawn after two magma deaths on Cinder **(09:30)** Scenes and takes GOOD (T3); the Warden retake GOOD (T7, four CUTs); **the Warden-skip MAJOR is fixed and played**  **(10:30)** **🟨 → ✅:** CUT and retakes GOOD in every scene played (T3, T7: four Warden retakes, arena with mercy). MINOR (polish, T5 + T3): a death in a gap between scenes returns the pilot to the hangar start |
| Always | Save and Continue at every act boundary, including interior and escape; no Continue after victory | ENG-19, ENG-46 | ✅ | ENG-46 is 🟨: Continue from the interior and the escape played once; T3's save / quit / Continue test (007d9f14) is on main f39a8372; in T3's scripted walk a Continue from the menu after a relaunch started a fresh run (under investigation, not yet confirmed as a game bug; T3) **(06:00)** **Continue after a relaunch works** (T3, agent-driven on main 50c4af7a): the menu shows CONTINUE, the run resumes at the saved wave in about 2 s, the intro and heard lines do not replay; the finding is closed as not a bug. Unverified: a real click on the CONTINUE button with mouse or pad (for Ryan) **(09:30)** Save and Continue GOOD (T3); the real click on CONTINUE untested  **(10:30)** Save and Continue GOOD (T3). **To close: no CONTINUE on the menu after victory** (T3, with the credits check); the real click is held for Ryan (polish)  **(13:30)** No change. **To close: no CONTINUE on the menu after victory** (T3, #95)  **(14:30)** **🟨 → ✅:** no CONTINUE on the menu after victory played GOOD (#95 closed). Polish: Ryan's one real click on CONTINUE |
| Always | Fallback ending (`td.Finale.Boarding 0`) still finishable | ENG-47, ENG-18 | ✅ | ENG-18: the space reactor could not be killed in a played run (#90 no. 5); T4's fix 818d592e (retune, trench retake, r2 hull) is on main eb1bf334 with its tests, awaiting agent play (T7); ENG-47 is 🟨: the decision record is the Lead's (due Nov 14 20:00) **(06:30)** Still unplayed: T7's assault play went on to docking, not to the space reactor **(09:30)** **Played to VICTORY** by T4 with the reactor brake assist (lead/int-aa f95043c1, batch AA, not gated; Lead's decision: ease to 20 m/s within 200 m, player keeps control). **NOT GOOD yet:** about ten enemy markers stay after victory; the breakup was not captured  **(10:30)** **To close: the space-reactor ending replayed through the breakup after the after-victory fix (782669d5, on main)** (T4)  **(12:30)** ENG-47 decided (Lead, 11:45, DEVLOG.md, #113): boarding is the ending for the freeze build, `td.Finale.Boarding` stays 1, the space ending stays as the fallback. T4's play (#112): the reactor killed by held fire, slow-motion kill shot with letterbox, GET CLEAR OF THE DOMINION, VICTORY with no enemy markers left; the outside camera GOOD (361b56bc); the breakup NOT GOOD (small sparks on a hull that stays intact); the flash, 11 hull chunks, held letterbox and generator retune are on main AQ 92c94f81, unseen. **To close: the generators by real fire and the breakup on screen** (T4)  **(13:30)** The breakup seen GOOD (#112). **To close: the four generators by real fire and the cut back to VICTORY** (T4, playing now)  **(14:30)** ENG-47 ✅ (#113 closed: the fallback runs to VICTORY). **To close:** the same run survivable without god mode (ENG-18 🐞, T4 and T2)  **(15:30)** **🟨 → ✅:** ENG-47 ✅ and now ENG-18 ✅ (#112: the assault survivable without god mode). Polish: the fallback run timed through all four generators without god mode (T4, #152)  **(16:30)** All four generators without god mode played GOOD (T7); the polish line is done |
| Always | Barks per `dialogs.md`, subtitles, voices | ENG-43, ART-11 | ✅ | Counted tonight (ENG-43): 44 bark ids fire in the game and about 55 ids in `dialogs.md` fire nowhere; the lists per lane are with T1, T4 and T5; `Bark_Anticipated` is in batch S's gate; voice pickup (Nicholas) **(05:00)** `Bark_Anticipated` is on main 4eb57848; ENG-43 world barks (T4) and on-foot barks (T5) are on branches; T1's and T2's sweeps not reported **(06:00)** On lead/int-v, not on main: T1's six travel barks (2fd52a50: boost empty, hyperdrive spool and blocked, atmosphere entry, landing ready and down) and the subtitles setting on the bark lane (fd16f394) and on every story line (15f04846). On branches: T4's world barks (d5e29eab) and T5's on-foot barks (285d4869). T2's combat barks have no list yet **(06:30)** **On main 94b56d11 (batch V), tests only:** T1's six travel barks, T4's world and enemy barks, T5's on-foot barks, the subtitles setting on the bark lane and on every story line. On main 6cc4f254: the retired `Bark_Dom_*` ids replaced by the `G6_*` lines and the roll call. T2's combat barks still have no list; the count against `dialogs.md` (T3) can now run **(09:30)** On main 80a2ee4d (batch Z), tests only: the planet arrival barks, the escape line. T1: travel barks partly seen. T2's combat barks: no list yet  **(10:30)** **To close: T2's combat bark list and T3's count against `dialogs.md`**; subtitles off (T3); voices are Nicholas's  **(13:30)** T3's count from code: 117 of 120 ids triggered, `Bark_Swarm_Saved` added (BB) (#122). **To close: barks heard in a played run; subtitles off in combat and on foot** (T4, T3; #94); voices are Nicholas's  **(14:30)** Subtitles off in combat and on foot played GOOD (#94 closed, ENG-32 ✅). **To close: barks heard in a played run** (T4, T3; #122); voices are Nicholas's  **(15:30)** **🟨 → ✅:** barks played GOOD in a played run with the pre-empt tell (#122 closed, ENG-43 ✅); the docking-line queue fix on main (b5472653). Polish: placeholder lines and voices are Nicholas's (ART-11: the Face to Face words, #171 takes; `C8_Wren_Order` take 03 airs once BQ lands)  **(16:30)** `C8_Wren_Order` take 03 is on main and plays its selected master (by log, #135); an ear check is Nick's side's |
| Always | Sound for every action; music per act (menu, tutorial, combat rising, Verdance, Cinder, assault, interior, escape, victory, credits) | ENG-35, ART-10, ART-19 | 🟨 | Nick's hookup not started; T1's native fallback for Nicholas's assigned placements is on main 61404a79 (R-047): credits, Cinder, assault and quiet flight live; core chamber and reactor fight wired, T5's interior events on main 6afa3816 but T1's forwarding (f88a0bfb) still on eng/t1; escape empty (Ritual not imported); nothing heard by a person; Nick's music folders are in the always-cook list; ear approval (Nicholas) **(05:00)** T1's forwarding (core chamber, reactor fight) and the glide wind slot are in batch U's gate; CloudPunch and GlideWind sounds on the effects branch; still nothing heard by a person **(06:00)** The interior moments reach the music bridge on main 50c4af7a (batch U) (core chamber and reactor fight fallbacks live, heard by no one); CloudPunch and GlideWind are still on eng/vfx; the escape slot is still empty **(06:30)** CloudPunch (3 takes, on `DA_Travel_FX.CloudPunch.Sound`) and the glide wind sound are accepted and in batch X, not on main. Audio is not verified in the package of 50c4af7a. Nick's side is still stopped **(09:30)** Nick's side's Audio task: the Ritual escape track (N-095). Ear approval of the fallback music waits for Nicholas; no audio verified in a package  **(10:30)** **To close: Nick's Ritual escape track; Verdance and waves 3-5 music (AD) confirmed in play** (Nick's Audio; T1); ear approval is Nicholas's  **(13:30)** Menu, Verdance and waves 3-5 music and the entry and climb-out roars confirmed in play by log (T1; ENG-35 ✅, #121). **To close: Nick's Ritual escape track**; ear approval is Nicholas's  **(18:30)** #151 enemy engine loops on main 2f07ab5c (Kenney CC0, three takes per type), log-proven by T4; the audition by ear is Nick's side's |
| Always | HUD in every state; Settings and accessibility | ENG-36, ENG-32, ENG-45, ART-09 | 🟨 | On-foot HUD in the new style: T2's restyle (aa204b7a) is on eng/t2 with passing tests, not seen live; the ENG-38 descent HUD is on main 6afa3816, awaiting agent play; the settings page fit (6d2d9c6e, main eb1bf334) and enemy marker shapes per type (ede33e91, main 61404a79; a fallback under R-047) await agent play; subtitle size and the graphics preset (20bdfe00) and the enemy-bolt shape for colour-blind players (0c5de65c, from T7's deuteranopia check) are on main; the subtitles on / off setting is in the T1 + T4 combined commit (branch) and the ground half of the difficulty (crew health, hits and fight size; 231b9876) in batch S's gate; shake and speed-FOV sliders and the New Game difficulty choice still open **(05:00)** T2's on-foot HUD restyle (aa204b7a) and the Cinder pending marker are on main 29a15f21, tested, not seen live; the enemy marker shapes were seen working by T7 (batch O tree); the ground half of the difficulty is on main 4eb57848 with the Ace fix on 29a15f21; the subtitles setting is in batch U's gate; on T2's branch: descent HUD contrast and the ORBIT fix, a marker placement fix, the *Aftermath* lanes **(06:00)** On main 50c4af7a (batch U): the descent HUD on its own dark plate (never below 80%), ORBIT surviving the travel, enemy markers above the hull at 1.4x, the first subtitles setting. On lead/int-v: the navigation-pulse getter, the wider descent plate, subtitles off hiding bark and story text. None seen in a played run on main **(06:30)** On main 94b56d11 (batch V), tests only: `GetNavigationPulse01`, the wider descent plate, the subtitles setting hiding bark and story text. Seen in T7's play on main 50c4af7a: the generator pips 0/4 to 4/4 and the assault score **(09:30)** T2: flight, enemy markers, on-foot and descent HUD GOOD; **NOT PLAYED:** the glide-in leg, the natural award, special drops, Rift and Bastion primaries, the docking plates, the Cinder waypoint, the *Aftermath* lanes, the tactical display, settings with subtitles off  **(10:30)** **To close: the docking plates, generator pips, escape clock and *Aftermath* lanes played (T2); subtitles off (T3); the New Game difficulty (T5)**  **(11:30)** New Game difficulty GOOD (T3). **New: the flight HUD redesign** (#160, R-049): Concept A chosen and approved by Ryan from the mockup, data model on main AL, T2 builds the squad panel, flags, boost arc and toast, T6 the radar; nothing drawn yet. **To close:** that HUD built and played (T2, T6); the docking plates, pips, escape clock, *Aftermath* lanes and transponder flicker (T2); subtitles off (T3); #154 / #155 proven (T3, T2)  **(12:30)** T2 at 720p (#103): generator pips, escape clock and *Aftermath* lanes GOOD; #157 GOOD; the tractor plate not seen. **The new flight HUD (#160) is drawn on main** (batches AP to AU: squad panel, status flags, radar, boost arc, pickup toast; the first-paint radar crash found by T7 fixed in adc75bc5): first in-game read by T7 at 720p good with a fix list; T2's fixes (720p scale, ship name on the flown card, bars only on wingman cards) on main AU ebe073ad; frames at 1080p and native 720p not yet judged. Subtitles: the ground path GOOD (T3); the flight lane fix (AP) passed Nick's side's retest for C4 at 720p Small / Medium / Large and Large at 1080p and 2560x1080 (#154 closed by Nick's side); MINOR open on #160: C4 Large's third line overlaps the radar (T2); subtitles off GOOD in the opening (T3, #94). **To close:** the new HUD's frames judged, the tractor plate, the transponder flicker, subtitles off in combat and on foot, the Large / radar overlap  **(13:30)** The new flight HUD played GOOD and closed (#160); ENG-45 ✅ (#127). **To close:** the tractor plate, the VERIFIED flicker, the three-line Large subtitle, boost arc and toast in a frame, the post-wave radar header (T2, T6; #103), subtitles off in combat and on foot (T3, #94)  **(14:30)** Subtitles off played GOOD (#94); control hints re-offered on a New Game (f3b8d3f5, #159). **To close:** #103's leftovers (T2, T6) and the #159 hint cards (T1, T2)  **(15:30)** On main: the radar header with the generators and the reactor state (223fbf9e), the grace shimmer and HUD line on an auto-switch; the SWITCH SHIPS lesson needs the pilot's own switch (960cbee7, Nick's playtest #140); the radar missile glyph and more tutorial fixes in BQ, not on main. **To close:** #103 (the follow marker, the director-shot HUD hide, the reactor header, the pilot card against the reticle: T2) and the #159 hint cards (T1, T2)  **(16:30)** The radar missile glyph is on main; DOCK follow markers GOOD; the director-shot HUD hide and the reactor header seen GOOD on T2's branch (next batch). **To close:** #103's low-hull card tint, pause-menu hover and the lock glyph on a finished planet (T2), T2's branch on main, #159 (T1, T2)  **(17:30)** T2's HUD fixes on main 0c986a7d; Ryan's review: the squad cards overlap and sit off-centre, the Q ring. **To close:** T2's measured layout pass, the pause hover, #159  **(18:30)** The squad-row layout on main 7fd85523: GOOD at 1080p (T7) and 720p (Nick's session 37). **To close:** the edge marker in the UNDER FIRE tab (T2), the pause hover, #159 |
| Always | Ships, fleet, Dominion, crew, props, effects, environment, animation at gameplay distance | ART-03, 04, 05, 06, 07, 08, 13, 17, 18, ENG-34, 37, 39 | 🟨 | Per item below **(06:00)** On main 50c4af7a (batch U) and not seen in the running game: Lancer cockpit v08, the Null bomber's twin-boom tail, landing gear meshes, survey stations, the camp canopy, the shard and hull checkerboard fix, the Warden's rebuilt death. Effects accepted from sheets: shield drop, Cinder cloud; not yet accepted: Verdance cloud v2, plumes, approach field, light row, tractor beam, beacon, haze **(06:30)** Seen in the running game by the structures builder: the moon shard material (region 6 at 1 km), the three ships parked on landing gear, both survey station walks, the camp canopy. Effects accepted from sheets and in batch X (not on main): Verdance and Cinder cloud v2, shield drop, approach field, light row, CloudPunch, `DA_ReactorCore_FX`, glide wind. Not accepted: enemy plumes, region beacon, region haze, tractor beam; the magma heat effect is built but not seen. Environment (eng/env, waiting on its s13 sheet): Cinder crystal clusters, terrain material, scar, interior light states, magma surface and glow **(09:30)** Seen: the magma look and heat effect, interior light states, the doorway chamfer. Lead's decision: Lancer contrast via a fresnel darkening in `M_TD_Ship` (default 0, Lancer only; T1, who took over the ship builder's Lancer work at about 07:00). Not accepted: region beacon, plumes, tractor, haze, magma trim; the anchor charge stretched  **(10:30)** Per item: ART-03 Lancer contrast (T1), ART-06 conduit art (T7), ART-07 hyperdrive effect (T1), ART-08 animation verdict (animation builder), ART-15 bay mouth, ART-16 tractor and breakup (T4, effects builder); ART-04 is ✅  **(11:30)** **ART-03, ART-05, ART-06 and ART-17 are ✅** (#108, #115, #118, #120). Left: ART-07 (#105), ART-08 (#124), ART-13 (#125), ART-15 bay mouth (#126), ART-16 (#111), ART-18 (#119), ENG-37 (#128)  **(12:30)** ART-15: the bay mouth seen good (T4 at 94 m; the bay look GOOD, environment builder; #126); guide lights left. New open sub-points under ART-08 (Ryan's focus): the energy blade remake (#163) and the pilot animation (#164). ART-13: #145 NOT GOOD, #148 unverified, #158 provisionally good  **(13:30)** ART-15 ✅ (#126). The new blade and the pilot animation round 1 on main (#163, #164: deflects and the idle stub still open). Left: ART-07 (#105), ART-08 (#124, #163, #164), ART-13 (#125), ART-16 (#111), ART-18 (#119), ENG-37 (#128)  **(14:30)** ART-13 ✅ (#125). On main: breakup v2, the collapse lighting fix, Nicholas's three recoloured Fab props imported (7dc8d2bf; not yet placed: environment builder). On main d9f5ba70 (BM): animation round 2 (275f5436) and the blade's walking-streak fix (010ca3f6). Left: ART-07 (#105), ART-08 (#124, #163, #164), ART-16 (#111), ART-18 (#119: the dropped sections never show), ENG-37 (#128)  **(15:30)** Closed as played GOOD: #163 the blade, #164 the pilot animation, #119 the anchor collapse (ART-18 ✅). On main: the crew clips in fights (Trooper strafe, Grunt combo, Warden roar), the sprint and landing clips, the Fab props placed (Cinder outpost, Dominion hangar and room corners, Verdance site), the Dominion corridor dressing and hangar work lights, the Rift and Lancer canopy glazing (#149). Left: ART-07 (#105: tractor v2, mines, the Lancer flame core), ART-08 (#124: heavy-hit reactions), ART-16 (#111), ENG-37 (#128: the blade light washing armour); the Verdance teal disc at the pole and the take-off frame rate (environment builder)  **(16:30)** Done this hour: the Verdance take-off frame rate (38 -> 1 slow frames) and the textured orbit disc, the blade light (#128), the Lancer plume's cyan, all three canopies. Left: #194 the planets filled on foot (T4; environment, structures and effects builders), ART-07 (#105: tractor v2, mines), ART-08 (#124: T5's hooks), ART-16 (#111)  **(17:30)** T5's blade and crew hooks on main (bb7a8ba5), the camera-blade guard, the Grunt wind-up, the 16 ambient systems as assets. Left: #194, ART-07 (#105), ART-08 (#124: hooks seen in a fight), ART-16 (#111) |
| Ship it | Packaged Shipping build under 1 GB, 60 fps at 1080p, played start to end | ENG-28, ENG-46 | 🟨 | Shipping package of main eb1bf334 published (private prerelease `playable-main-eb1bf334`): the zip is 981,016,363 bytes, 19 MB under the limit; it is still the newest finished package; a trial package of main ddc50a2f has been cooking since 03:32, and the never-cook rules (about 120 MB) and Oodle RDO stay unproven until it finishes (import builder); performance pass (#89, T7): audio buffers 1 against 3 measured, inconclusive and not committed; the warm-up off / on and natural first-ten-minutes runs are in progress, so no frame-rate claim is verified; T2's projectile pool fix and combat warm-up list are on main 6afa3816; dropping the DirectX 11 shader set is held for Ryan; daily packaged playthrough **(05:00)** **🐞 The Shipping game target does not compile on main 4eb57848 or 29a15f21** (a squad test sat outside its `WITH_DEV_AUTOMATION_TESTS` guard; found by the packaging agent), so neither can be packaged; the fix (dea28a8b) is in batch U's gate. Back to 🟨 when it is on main. The newest published Shipping zip is main ddc50a2f (batch R): 872,722,389 bytes, 127 MB under the limit, a clean cook with the never-cook rules (134 unreferenced packages; on main 29a15f21), launch check to first combat only; prerelease `playable-main-ddc50a2f`. Oodle RDO is a trial, unproven. Performance (#89): the warm-up removes combat frames over 100 ms (assault worst frame 133 → 51 ms) at +650 MB RAM (per-map lists in progress); tutorial combat had 9 frames over 33 ms in 119 s on a loaded machine (cause unconfirmed); the audio buffer test is inconclusive. Held for Ryan: the DirectX 11 shader set, a precompiled pipeline-state cache **(06:00)** **🐞 → 🟨:** main 50c4af7a compiles for Shipping again (dea28a8b, batch U), so main is packageable; the Shipping cook of 50c4af7a is running and not published; the newest published package is still `playable-main-ddc50a2f` (872,722,389 bytes); no package has been played end to end and no 60 fps claim is verified **(06:30)** **The package of batch U exists:** prerelease `playable-main-50c4af7a`, `Builds\for-ryan\TriadDrift_50c4af7a_Shipping.zip`, 876,082,811 bytes (123.9 MB under 1 GB), cook 2,595 packages with 0 errors and 0 warnings, six maps, IoStore 8/8. Shipping launch check: menu, New Game, intro, hold-to-skip, wave 1 for 80 s, no crash. **Not verified in the package:** audio, flight input (window-message injection does not reach gameplay controls), anything past wave 1; it needs a human. No package was made from batch V (Lead: W supersedes it); the Shipping target of main 6cc4f254 compiles and its cook is running. No 60 fps claim is verified **(09:30)** Newest package `playable-main-8b4eabae` (batch Y): 877,326,337 bytes, cook 2,632 packages clean, launch check to wave 1; a cook of 80a2ee4d is running. Nothing past wave 1, no audio and no flight input verified in a package; no 60 fps claim  **(10:30)** Newest package `playable-main-80a2ee4d` (batch Z); hourly cooks of the newest main on the hour. **To close: a package played start to end with audio and input** (Ryan or an agent with real input), a frame-rate pass (T7)  **(11:30)** #153: clean routes GOOD (T6: 0 or 1 frame over 33 ms per route); the death / CUT hitch not measured (T6); next hourly cook 12:00. **To close: a package played start to end with audio and input** (#152)  **(12:30)** Shipping package of main adc75bc5 built (0 cook warnings), its launch check pending (packaging agent; next cook 13:00); no package played past wave 1 (#152); the death hitch not measured (T6, #153)  **(13:30)** #153 frame budget measured GOOD; Shipping 4bbdd282 (0.879 GB) launch-checked and adc75bc5 played 6.5 min by T7; packages every 30 minutes. **To close: a package played start to end with audio and input** (T7, #152)  **(14:30)** Shipping TriadDrift_f40a39c3 (0.88 GB) played from the menu through wave 2 by T7; on foot not reached in Shipping; T6's galaxy load fix on main (space field build 2,800 → 165 ms, 2cb0c27e); next cook 14:30. **To close: a package played start to end with audio and input** (T7, #152), after ENG-18's rebalance  **(15:30)** Shipping TriadDrift_9f20c5be launch-checked; T7 continued from a Verdance save in it (blade, carry, on-foot card GOOD in Shipping; no fight reached). T6: galaxy reload 1.4 s, climb-out longest frame 1.7 s (was 4.9 s); the re-entry hitching fix compiling. Next cook 15:30 from 8d62d14a. **To close: a package played start to end with audio and input** (T7, #152)  **(16:30)** TriadDrift_8d62d14a_Shipping.zip launch-checked (0.90 GB) and played into the menu, wave 5 and Cloak by T7 with real input; the re-entry hitching fix on main (reload 0.27 s, longest climb-out frame 0.35 s). Next cook from 99d489e7 or later with the size check. **To close:** a package played start to end (T7, #152)  **(17:30)** #89 at 17:20: wave 5, field build, re-entry and retakes GOOD; Verdance take-off NOT GOOD; boot to menu 30.9 s black on 685d8bea (splash and no menu warm-up on main, not yet measured). **To close:** #152, the #89 rows  **(18:30)** Boot: menu at 13.4 s (was 30.9 s) with the splash at 1.3 s; about 10 s of black window left (T6's loading card). Packaging held at 96a66f7c for the Cinder blocker. **To close:** #152 |
| Ship it | Independent review of every lane, human playthroughs, submission page, video, dev log, credits | ART-20, ART-21, ENG-46, ART-14 | 🟨 | Batch L range 825d8869..6fe3e90f sent to Nick's side for review (R-045, accepted on #88); T7 ran Nick's seven scripts on batch L; **no agent has played batches M to R end to end** (T7 plays main with script 08 after its performance window; not started at 03:35); eight evidence frames for Nick's asset issues are published as the prerelease `evidence-main-eb1bf334` and linked on #37, #40, #41, #42, #51, #54 and #63 with a request to close; Nick's side stopped at 01:16 (N-090), its reviews resume on restart; Glass Box draft on main (a980fa7f); itch page; trailer re-cut **(05:00)** **The first end-to-end agent play since batch L has started:** T7, 04:14-04:29, real keys through the engine's MCP server in a `-game` session, on the batch O tree (behind main by batches P-T): menu, intro skip, tutorial, switching, the Bastion dome, wingmen, the withdrawal and the Verdance entry played and working; the *Aftermath* beat broken (MAJOR). Not played: landing by hand, both planets' objectives, the assault, docking, interior, reactor, escape, victory (T7's next session is running). Evidence frames are now on all eleven asset issues (#37-#42, #51-#54, #63) with a close request; nothing closed (Nick's side is stopped) **(06:00)** T7's agent play now covers the opening through Verdance on foot and back to orbit, and Cinder on foot to the gate conduits (batch O tree); the assault, docking, the interior, the reactor, the escape and victory are unplayed by anyone since batch L; batch U's range for ART-20 is 29a15f21..50c4af7a; Nick's side is still stopped **(06:30)** T7's agent play on main 50c4af7a now also covers the *Aftermath* replay, the Dominion assault and the interior from the hangar to the Warden. **Still unplayed by anyone:** docking on main (T1 is playing it), the reactor conduits and heart, the escape run and flight, victory, credits, the space-reactor fallback. Batch W (main 6cc4f254) is tests only. ART-20 ranges added: 50c4af7a..94b56d11 (V) and 94b56d11..6cc4f254 (W); Nick's side is still stopped **(09:30)** Nicholas is running a human playtest in the editor on main 8b4eabae (his note on #90; findings pending, no approval inferred). Agent play since 06:30: T7 interior to the Warden (GOOD), T1, T2, T3, T4, T6 verdicts. Still unplayed: the reactor, reveal, escape run, reboard, credits, Cinder past the conduits, waves 3-5. The independent review of batches U to X is Nick's side's task (N-095)  **(10:30)** **To close:** Nick's side's review of batches U to AE (ART-20), Nicholas's playthrough batches (Batch 1 received, triaged on #90), the submission page and video (ART-14)  **(12:30)** From Nick's side (#139): a bounded file-range matrix of batches U to AE, then one source review of batch U's story carry across travel: **MAJOR (source only, not reproduced): a fast New Game after quitting with a required recording unheard can queue the old run's recording into the new run**; routed to T3 by Ryan's Lead. Nicholas approved the written art-direction guide (#132)  **(14:30)** Nick's side reconciled historical audio uses and the scoped package inventory in `CREDITS.md` (1e2fdba7). Nicholas told his Lead to decide anything pending his approval unless critical (#133, #136). **To close:** the reviews of the newest ranges, the submission page and video (ART-14, ART-20) |


**Hourly alignment, Nov 14 18:30 EST (Ryan's Lead; replaces the 17:30 block):**
- **Rules unchanged** (under the status key): 09:15 agent-played and judged good = ✅, and an item whose issue the Lead closed as
  played GOOD is ✅; 10:15 every named player-facing part played good = ✅, polish moves to the AAA list; 10:25 compile-only gate.
  Branch-only work is 🟨. Nick-side items move only on Nick's side's own reports (its 18:20 checkpoint changes no Nick marker);
  Nicholas has told his Project Lead to make the call on anything pending his approval unless it is critical (#133, #136).
- **BLOCKER ON MAIN (from 48a6fd97; Nick's session 39, #194):** on Cinder the pilot walks about 19 m under the course (the relief
  sits low there and the slab's collision is now off). **The Lead's mistake:** c9edb62a went to main before its route walk. T5 puts
  the slab collision back wherever the relief is lower than the course; T4 fixes the relief heights. ENG-26, ENG-39 and the three
  Cinder checklist rows are 🐞 until the route is walked again. **Packaging is held at 96a66f7c**, the last good build (cooking).
- **Also open from Nick's session 38 (#96):** T5's old-save gate strands a legacy saved pawn beyond the locked Warden exit (MAJOR,
  legacy saves only; ENG-42 kept 🟨, the Lead may make it 🐞). The cold-Continue replay half passed (sessions 34-35).
- **On main f85203b1** (since 17:30: 2f07ab5c relief on both planets, the #96 run-save fix, the #151 engine loops, Fab texture caps;
  7fd85523 T2's squad-row layout; e0b9385d Verdance camps, whole-area fill, arches, landmarks, 49 ambient effects; 96a66f7c the relief
  casts no shadow; 48a6fd97 T5's slab collision handover, the #96 old-save gate, #139's chamber re-entry fix, the Grunt telegraph
  call, the retract shrink (mostly unplayed); f85203b1 #146 the opening bed, seven #207 assets, tractor beam v3 visible in a staged
  spawn, the Grunt wind-up v3). **Compiling, NOT on main:** T1's take-off turn through the notch (cd2ec8f2); T6's asteroid-split fix
  (0ed9cd18, #139's MAJOR, stress-tested in a game). **On a branch, sent back:** #201 shadow mode and wingman powers (too strong:
  wave 3 cleared by the wingmen alone in about 50 s).
- **Played this hour:** T7 walked Verdance on 96a66f7c with the real pawn: PLAYABLE, one blocker (a pylon on the beacon path at
  4280, 3505), a camera-inside-a-hut-wall minor (both environment builder), landing and leaving the ship GOOD, the squad row at
  1080p in a wave GOOD; T6: Verdance on foot and the take-off climb GOOD on 2f07ab5c; T1: descent, landing and take-off on both
  planets, the climb-out fixed; boot in the a946ca93 package: splash 1.3 s, menu 13.4 s (was 30.9 s), about 10 s black window left.
  Nick's side: cold Continue PASS (#96), the squad row clean at 720p with an edge marker intruding into UNDER FIRE (T2).
- **Markers changed (18:30):**
  - **🟨 → 🐞:** ENG-39 (the Cinder route blocker).
  - **✅ → 🐞:** ENG-26 (the same blocker breaks Cinder's gate route).
  - **Checklist → 🐞 (3):** the Cinder arrival row (🟨), "Secure the excavation" (✅) and "Disable the field anchor" (✅).
- **Counts:** item headings 51 ✅, 0 🟢, 19 🟨, 2 🐞, 0 ⬜ (Ryan's side: 50 ✅, 11 🟨, 2 🐞; Nick's side: 1 ✅, 8 🟨);
  the 37 checklist rows: 20 ✅, 14 🟨, 3 🐞.
- **Issues:** closed since 17:30: #206 (answered: at most eight assets in one batch), #207 (seven taken, Damaged Concrete skipped).
  26 open. #96 stays open until an old-style save is played through T5's gate. #151 on main, log-proven, audition with Nicholas's
  side. #146 on main, the skip fade unshown.
- **Decisions made by Ryan's Lead under delegation since 17:30 (Ryan may overrule any):** c9edb62a taken to main before its route walk
  (the mistake that exposed the Cinder blocker); #151's assets in our `Gameplay/Audio` folder; #206 capped at eight assets, #207
  seven taken and Damaged Concrete skipped; carried *Aftermath* lines wait for the galaxy; encounter boxes allow colliding pieces in
  their outer 30%; colliding scenery kept 4 m off straight pad-to-objective lines; the Grunt wind-up ring reduced to a thin outline;
  Cinder's pad crystals thinned by a third; the packaging hold; AGENTS.md's squad section updated for shadow mode.
- **Ryan:** "Yes, keep three" on old packaged builds (36 GB freed; the loop prunes to three). Still open with him, not blocking:
  whether he agreed to #192's tracking shots. Still his whenever he wants them: a play of the newest package, one real click on
  CONTINUE, `MI_TD_DominionGlowRed`, the DirectX 11 shader set, pilot foot IK, a precompiled pipeline-state cache, T3's stash
  "t3-failed-merge-lead-int-u-0921".
- **Waits on Nicholas (now mostly on his Lead under the delegation):** by-ear approvals (`C8_Wren_Order` take 03, the #146 bed,
  the #151 loops); appearance reviews of the menu frames (#142) and the squad row (#103); #149 after T7's real-speed check; the
  Wren takes (#171); #134, #136; the Ritual escape track; where he wants skipped assets if he disagrees.
- **THE REMAINING LIST, most urgent first** (owners on `CLAIMS.md`):
  1. **The Cinder route BLOCKER** (T5 slab collision, T4 relief heights, T7 walk; Nick's session 39 recheck): ENG-26, ENG-39, three rows.
  2. ENG-42 and the reveal row: T5's legacy-save recovery and Nick's same-save proof (#96).
  3. ENG-36 and the HUD row: the UNDER FIRE marker, the pause hover, #159 (T2, T1).
  4. The take-off rows: both take-offs flown with their C4 and C6 lines (T4, #96).
  5. ART-08: T5's hooks seen in a played fight (#124).
  6. ENG-37, ART-07 and ART-16: the beam v3 in a real docking, mines in a real wave, the hull-scale explosions (effects builder, T4).
  7. The escape flight row: the breakup seen at the escape clear (T4).
  8. ENG-05, ENG-08, ENG-27: #201 rebalanced and played (wingman builder, T2, T4, T7).
  9. ENG-39 after the blocker: the Verdance pylon, Cinder's fill, the after frames (#194).
  10. ENG-28, ENG-46 and the Ship-it rows: the loading card, a package played start to end (T6, T7, #152).
  11. Nick-side items ART-09, 10, 11, 12, 14, 19, 20, 21: next steps in R-044; the Ritual track closes the escape-run and sound rows.

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

Updated Nov 14 18:30 EST (Ryan's Lead; Ryan's side rows only; "now" is what each lane had in hand at about 18:25; first: the Cinder route BLOCKER on main; top focus since 16:15: #194, the planets filled on foot; since 17:05: #201, the wingmen; since 13:25 (Ryan) render turns of about 5 minutes (cap 10) in three render slots behind a measured launch guard (AGENTS.md §11, `Builds/render-slot/README.txt`), and a package every 30 minutes). Nick's side: only its Lead is active (playtests and feedback batches, N-098 archived); its Story, UI, Audio and Auditor tasks stay paused, so its rows below are
as its own board and ledger left them, not current work.

| Owner | Now | Next (in order) |
|---|---|---|
| **Lead** (Ryan's) | The batch with T1's notch turn (cd2ec8f2) and T6's asteroid-split fix (0ed9cd18); the packaging hold at 96a66f7c; boards | The Cinder blocker fix to main only after its route walk; #201 from eng/wing once rebalanced |
| **T1** flight feel, hints, Settings, music bridge, descent | The take-off turn through the notch (cd2ec8f2, compiling) | #159 the hint cards with T2; the #138 check cases (bffd905a) to main |
| **T2** combat, powers, HUD, switching | #103 the edge marker and its distance intruding into the UNDER FIRE tab | #201 / #192 aim assist on all three ships with a HUD indicator; the pause-menu hover; #159; #133 |
| **T3** story, intro, ending, camera, tutorial | #146 the opening bed's skip fade shown | `Bark_Wipe`; the victory recap names and camera fade seen |
| **T4** waves, assault, planet terrain, atmosphere | **The Cinder BLOCKER:** relief heights under the course (#194) | #201 enemy balance once it lands; the beam v3 in a real docking and mines in a real wave; the two take-offs with their lines (#96); the breakup at the escape clear |
| **T5** on-foot enemies, interior, blade input | **The Cinder BLOCKER:** the slab collision back wherever the relief is lower than the course; #96 the legacy-save recovery (Nick's session 38) | The blade and crew hooks seen in a fight (#124); drone tilt / look / tumble; the stray attack after CONTINUE; the orange disc near Grunts |
| **T6** space field, radar, performance | The loading card over the last 10 s of black at boot (#89); the asteroid-split fix (0ed9cd18, compiling) | The on-foot frame rate on the filled planets (#194); the Cinder take-off |
| **T7** end-to-end play | The Cinder route walked once T5's and T4's fixes are built; #149 the real-speed intro check | #201 the verdict with shadow on and off; #142 the menu checks; #152 a package start to end |
| **Ship builder** | Stopped (usage limit) | (restart only on the Lead's dispatch) |
| **Wingman builder** (`.wt/wing`, eng/wing) | #201 rebalance: sent back as too strong (wave 3 cleared by the wingmen alone in about 50 s); proofs | Tuning with T4; T7's verdict |
| **Effects builder** | Tractor beam v3 seen in a real docking; Cinder's ambient effects placed (#194) | The hull-scale explosions on the breakup (#111) |
| **Blade effects builder** (`.wt/blade`, eng/blade) | Idle: #128's blade light fix is on main 685d8bea | Polish (AAA list), on the Lead's dispatch |
| **Environment builder** | The Verdance pylon on the beacon path (4280, 3505) and the camera inside a hut wall; Cinder's pad crystals thinned by a third | Cinder's fill once the route is fixed (#194) |
| **Structures builder** | #194 kit pieces as the environment builder asks (huts, perimeter and site sets on main) | The story console look (#96) |
| **Animation builder** | The sprint frame with the blade behind the helmet (reproducing) | Clips for T5's hooks as asked (#124) |
| **Intake builder** (`.wt/imp`, eng/import) | #151 enemy engine loops (`Gameplay/Audio/Engines`, EngineTrail.Sound on two enemy data assets) | Nicholas's next batch issue (at most eight assets, #206) |
| **Packaging agent** | Held at 96a66f7c, the last good build (cooking) | The next package after the Cinder fix is walked; keep the newest three builds |
| **Grok Bot** | No asks open (R-042); PRs #162, #165 to #170 and #172 to #174 merged as source and reference | The next ask is posted on R-042 when there is one |
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

**Cells refreshed Nov 14 18:30 EST** (main f85203b1: 2f07ab5c, 7fd85523, e0b9385d, 96a66f7c, 48a6fd97, f85203b1 since 17:30; the Cinder route BLOCKER from 48a6fd97 puts ENG-26 and ENG-39 at 🐞; at 17:30 main a7e35ba2: BV 28c2fb05, 0c986a7d, b8a55662, 6f0536ca, bb7a8ba5, 8ea0f5f1, 67021ade, 6e6c088b, a7e35ba2 since 16:30; T4's relief f6c1853f and T3's 77ad713e compiling; #201 puts the wingmen, aim assist and balance back in progress; at 16:30 main 685d8bea: batches BQ a9c635de, BR 99d489e7, BS c8eef9a5, 761877e2 and BU 685d8bea since 15:30, batch BV 28c2fb05 landed at 16:35; Ryan's #194 puts the two planets back in progress; at 15:30 main 8d62d14a: batches BM d9f5ba70, BN 9f20c5be, BO a71e3f73, BP 8d62d14a since 14:30, batch BQ compiling, not on main; at 14:30 main dd139312: batches BJ 043c4517, BK 87815c4e, BL dd139312 since 13:30, batch BM compiling; at 13:30 main 2a7ff74f; batches AV a14bf410, AW 7c7af25d, AX b5d1549c, AY d1ae9b18, AZ 618921ef, BA 5e6ef16f, BB 6c7521b8, BC 8dce9161, BD 26bda51a, BE 98c96926, BF de4e53bf, BG f9574191 and BH b8066135 landed since 12:30; batch BI compiling, not on main; Ryan's rule since 10:15: every named player-facing part played good = ✅, polish moves to the AAA list; compile-only gate since 10:25). The group states are the Nov 13 ones; an item can differ on its own heading. ✅ on the headings now: C-01, 02, 03, ENG-01, 02, 03, 04, 05, 06, 07, 08, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 29, 30, 31, 32, 33, 34, 35, 38, 39, 40, 41, 43, 44, 45, 47, 48, ART-01, 02, 03, 04, 05, 06, 13, 15, 17, 18; 🐞: none.

| Area | Items | State | What is still missing |
|---|---|---|---|
| Flight, input, three ships, switching, wingmen | ENG-01, 02, 03, 04, 07, 08 | 🟢 | A human's judgement of flight feel and switching. Switching played by an agent at 04:14 (8 switches honoured, batch O tree). Wingmen grazing rocks (#90): T3's avoidance fix is on main 4eb57848 and T1's formation fix on main 50c4af7a; no wingman was lost in T7's 4 minutes of combat (on the tree before the fix); replay on main pending **(06:30)** T1's core switching check on a live game (wave 3): 13 mid-action switches, all instant, no stuck power, worst frame near a switch 15 ms; a rapid-switch test (20 presses in 2 s) on main 6cc4f254 **(09:30)** **ENG-02 and ENG-07 are ✅** (agent-played and judged GOOD, T1). Flight and boost GOOD, drift NOT PLAYED (ENG-01 stays 🟢); wingmen not judged this morning **(10:30)** **ENG-01, 03, 04 and 08 are ✅** (drift GOOD; damage and retakes; all three ships flown and docked; wingmen through combat): the whole group is ✅  **(17:30)** Ryan's #201: shadow mode and wingmen using their powers replace "basic attacks only"; ENG-08 is 🟨 (wingman builder, T7) |
| Weapons, targeting, powers, abilities | ENG-05, 06, 09, 10, 11, 12 | 🟢 | Both ability awards (Gravity Well, Cloak) were announced in T7's Verdance play and the Cloak prompt showed in flight (batch O tree); the Cloak award comes late (MINOR, T5). The dome from inside was seen working by T7. The pickup pre-spawn is on main 50c4af7a. Each ability used live once more after the effects **(06:30)** Cloak at the transfer is on main 94b56d11 (tests only) **(09:30)** **NOT PLAYED this morning:** the signature powers, Cloak, EMP Pulse and Gravity Well in a fight, Rift and Bastion primaries (T2); Lancer weapons and pickups GOOD **(10:30)** **ENG-10 is ✅** (ram). Portal blaster, dome, EMP Pulse and Gravity Well GOOD (T2, live); **NOT PLAYED:** Rift and Bastion primaries, the missile swarm locks, a portal fly-through and missile return, Cloak in a fight (T2); the hint card MAJOR fixed on main AC **(11:30)** **ENG-05, ENG-06 and ENG-11 are ✅** (the primaries and the swarm locks GOOD, T2). Left: ENG-09 a portal fly-through and a returned missile (T2, #100); ENG-12 Cloak in a fight (T2) and the Cloak effect's last pass (effects builder, #102) **(12:30)** ENG-09: both portals open and read well (T2); a missile returned through a portal still to play (#100). ENG-12: Cloak in a fight (T7) and the Cloak effect's last pass (effects builder, #102) **(13:30)** **ENG-12 🟢 → 🟨:** Cloak works but the Rift stays opaque and no CLOAKED flag shows on main (T7, #102); the effects builder's shell is unmerged. ENG-09: a returned missile still to play (T2, #100)  **(14:30)** ENG-12: the Cloak hull swap is written, not compiled (T2, #102); ENG-09: the portal return (T2, #100)  **(15:30)** **ENG-12 ✅** (#102: the Cloak hull swap played GOOD); ENG-09 stays 🟢: the portal return (T2 tracing the crossing in its test, #100)  **(17:30)** ENG-09 ✅ (#100); ENG-05 🟨 for #201's aim assist on all three ships (T2) |
| Enemies, waves, balance, difficulty | ENG-13, 14, 27 | 🟢 | A human run on each difficulty. Enemy readability materials, marker shapes (now above the hull at 1.4x), the Null interceptor's silhouette and the bomber's twin-boom tail (main 50c4af7a) await agent play on main; warp-in points never inside an obstacle on eng/t4; the first on-foot fight was too hard for a scripted player and is being trimmed (T5) **(06:30)** Warp-in points never inside an obstacle and the world and enemy barks are on main 94b56d11; the wreck-core trim on main 6cc4f254; none played **(09:30)** Waves 1-2 GOOD (T4); **waves 3-5 and each enemy type NOT PLAYED**; wave 5's objective line on main 80a2ee4d **(10:30)** **ENG-14 and ENG-27 are ✅** (waves 1-5 GOOD, T4; every fight finishable). ENG-13 stays 🟢: bomber mines NOT SEEN (T4) **(11:30)** ENG-13: bomber mines in T4's lock turn (#109) **(12:30)** **ENG-13 is ✅** (bomber mines laid and detonated, the Mines Ahead bark once; T4, #109): the whole group is ✅. The mine look moved to ART-07 (#105)  **(17:30)** Wave 5's carrier cap played (6f0536ca); ENG-27 🟨 for #201: enemies hard but fair after the wingman and aim-assist changes (T4, T7) |
| Hyperdrive, gates, galaxy, Dominion finale | ENG-15, 16, 17, 18 | ✅ | ENG-18 is 🟨: the space-reactor fallback retune is on main, awaiting agent play; **the assault has not been played by anyone since batch L**. The Dominion exterior in the Null colour code is on lead/int-v; shield drop accepted from its sheet, the approach field not yet; the breakup needs real effects **(06:30)** **The assault was played by T7 on main 50c4af7a and works** (score, pips, kill shots and lines, docking opens; generators killed by a labelled debug command). The exterior recolour is on main 94b56d11; the space-reactor fallback is still unplayed; shield drop and approach field are accepted, in batch X only **(09:30)** Galaxy travel GOOD and the assault GOOD (T4); **ENG-16 is 🟨 (portal gates never played)**, hyperdrive NOT PLAYED this morning (T1); the space-reactor fallback played to VICTORY with the brake assist (batch AA) but NOT GOOD yet (enemy markers after victory, breakup not captured) **(10:30)** Hyperdrive lock GOOD, **the jump NOT PLAYED** (T1); **portal gates NOT PLAYED** (T1); the space-reactor ending's after-victory fix (782669d5) on main, not replayed through the breakup (T4) **(11:30)** **ENG-15, ENG-16 and ENG-17 are ✅** (the held jump, T1 10:27; the ring centred and one gate transit, T1 10:46). ENG-18: the breakup replay and the generators by hand in T4's lock turn (#112, #113) **(12:30)** ENG-18: the reactor by held fire, slow-motion kill, VICTORY with no markers, the outside camera GOOD (T4, #112); the breakup NOT GOOD (small sparks); flash, hull chunks and the generator retune on main AQ, unseen; the generators by real fire still to play (T4) **(13:30)** ENG-18: the breakup seen GOOD (#112); the four generators by real fire and the cut back to VICTORY (T4, playing now)  **(14:30)** **ENG-18 🟨 → 🐞:** the assault is not survivable without god mode (22 deaths in 8 min, no generator down); T4 rebalances, T2 adds a 1.5 s grace after an auto-switch. ENG-47 ✅ (#113)  **(15:30)** **ENG-18 🐞 → ✅** (#112: without god mode, two generators in 88 s, 0 deaths; T4's rebalance and T2's grace): ENG-15, 16, 17 and 18 are all ✅. Polish: all four generators timed in a full Shipping run (#152) |
| Game flow, saves, landing, on foot, blade, crew, objectives | ENG-19, 20, 21, 22, 23, 24 | 🟢 | ENG-23 is 🟨: the Warden's slam, shield, clips on `SKEL_Warden`, rebuilt death and stagger trigger are on main 50c4af7a, and the Warden has not been reached in a played run. ENG-20 is 🟨 (no landing by hand played). Blade fights against Grunts, Troopers, Drones and a Heavy, CUT and retakes with mercy, and boarding by holding E were played by T7 on Verdance (batch O tree). Continue after a relaunch works (T3 on main 50c4af7a); a real click on CONTINUE is for Ryan. Deflect timing judged by a person **(06:30)** MAJOR open on crew combat (T5): the Heavy's guard never breaks to quick swings and no lunge hint shows (the arena took 17-18 takes in T7's interior play). The flown ship is missing from the staged Verdance pad (MINOR). T1 verified the natural Verdance route and glide live **(09:30)** **ENG-23 is 🟢:** the Heavy's guard and lunge hint, mercy and the Warden fight GOOD (T7, real keys); Save and Continue GOOD (T3; the click untested); entry and exit GOOD, the hand-flown landing and take-off NOT PLAYED (ENG-20 🟨); the staged interior start spawns the pilot behind the ship (MINOR) **(10:30)** **ENG-19, 20, 21, 22, 23 and 24 are ✅:** the whole group is ✅ (the hand-flown landing and take-off GOOD on both planets, T1; the on-foot marker MAJOR fixed on AB; polish: the real CONTINUE click, the perfect-deflect feel, the staged start) **(13:30)** Still ✅. The new blade (BH) and the pilot animation round 1 (AV) are on main; their open points are on ART-08 (#163, #164) |
| The two planets | ENG-25, 26, 34 | 🐞 | **ENG-26 is 🟨 again (06:30):** the Cinder magma route (markers over the new bridge, an edge warning and tip, a far-bank respawn after two magma deaths, a route test over 11 legs) is on main 6cc4f254 (batch W), tests only, not played; the bridge was walk-tested by its builder; the rest of Cinder is unplayed. The wreck-core trim on Verdance is on main in the same commit, unplayed. Verdance was played on foot from the research site to orbit by T7 (batch O tree): working, with the wreck-core fight too hard (being trimmed) and three MINORs. On main 50c4af7a and not seen in a played run: the survey stations, the camp canopy r2, the Verdance canopy at about 40%, T5's Dress pass, the lifted outpost floors **(09:30)** Not replayed by a player agent on main this morning; on main: the magma look and heat effect (seen on foot), the geyser row clear of the bridge (walked, GOOD), anchor pieces (the charge stretched, NOT GOOD). **Cinder past the conduits NOT PLAYED** (T5) **(10:30)** **ENG-34 is ✅** (both worlds flown, landed and walked). ENG-25: the wreck-core fight GOOD on main AB, the transfer to Cloak not yet (T7 playing). ENG-26: the station chain and magma route GOOD; the anchor charges MAJOR (AE), the conduits, cache and collapse unplayed (T5, T7) **(11:30)** **ENG-25 is ✅** (Verdance played through by T7, the transfer to Cloak). ENG-26: the anchor GOOD (T7); the gate to the cache by hand (T7, #117) **(12:30)** **ENG-26 is ✅** (T7 by hand from the gate to the field anchor on main adc75bc5: takes 2 / 2 / 2 / 3 / 1, 5 deaths, #117): the whole group is ✅. The anchor collapse freeze on a fast escape is on ART-18 (#119; T5's fix on main AU, unseen)  **(16:30)** Ryan (#194): the planets feel empty and flat on foot; relief, a mountain ring, rocks, huts, trees and ambient effects through both play zones (T4, environment, structures and effects builders; T6 frame rate; T7 frames and verdict; T1 descent and take-off). ENG-39 and the two arrival rows are 🟨 until #194 closes  **(18:30)** **🐞:** the Cinder route BLOCKER on main 48a6fd97 (the pilot about 19 m under the course; T5, T4). Verdance walked PLAYABLE on 96a66f7c with one pylon blocker (environment builder) |
| Director's Cut (cut, money shots, scenes and takes), intro, third person, Settings | C-03, ENG-32, 33 | ✅ | ENG-33 is 🟨: T3's rebuild from the approved `O_*` lines is on main 50c4af7a; the black hold and HOLD TO SKIP were seen live by T3; the 49 s re-shoot with the final ships is still to do. CUT and takes played on foot by T7. A person's eye **(06:30)** Switching evidence for C-03 #1: T1's 13 mid-action switches, all instant (median frame 6.5 ms, p99 16.3 ms). A death in the Dominion Warden fight skips the Warden (MAJOR on ENG-30, scenes and takes) **(09:30)** The cut and name card GOOD, scenes and takes GOOD, the intro GOOD (**ENG-33 is 🟢**); money shots NOT PLAYED; subtitles off NOT PLAYED **(10:30)** **ENG-33 is ✅** (findings 7-11 to polish). C-03: money shots NOT PLAYED (T3); ENG-32: subtitles off NOT PLAYED (T3) **(11:30)** C-03: a money shot in slow motion (T3, #97); ENG-32: subtitles off (T3, #94); subtitle wrap and size fixes on main AM (3c5e68f6), not proven by a capture (#154, #155) **(12:30)** C-03: the ram-kill director shot GOOD in frame; slow motion to be shown in a log line (T3, #97). ENG-32: subtitles off GOOD in the opening; combat and on foot still to check (T3, #94). Subtitles in flight: T2's lane fix passed Nick's side's retest (#154 closed by Nick's side); #155 open; the Large / radar overlap is MINOR on #160 **(13:30)** **ENG-45 is ✅** (#127: shake and speed-FOV measured, difficulty played). C-03: the slow-motion log line (T3, #97); ENG-32: subtitles off in combat and on foot (T3, #94)  **(14:30)** **C-03 ✅** (#97) and **ENG-32 ✅** (#94): the whole group is ✅ (ENG-33, ENG-45 already ✅). Nick's Director cards' look stays ART-12 (#136) |
| Sound | ENG-35 (+ ART-10) | 🟨 | Fallback sounds on main; a native music fallback plays Nicholas's assigned placements (escape empty, the Ritual track is not imported); the interior moments reach the music bridge on main 50c4af7a, so the core-chamber and reactor-fight music can play (heard by no one); CloudPunch and GlideWind on eng/vfx. Nothing has been approved by ear **(06:30)** CloudPunch and the glide wind sound are accepted, in batch X only; audio is not verified in the Shipping package **(09:30)** Nick's side's Audio task: the Ritual escape track (N-095); ear approval of the fallback music waits for Nicholas **(10:30)** Verdance and waves 3-5 fallback music on main AD (83cbddba), waves 3-5 seen in the log; the escape still silent (Ritual, Nick's Audio) **(13:30)** **ENG-35 is ✅** (#121: menu, Verdance and waves 3-5 music, entry and climb-out roars, by log in play, T1). Left on Nick's side: the Ritual escape track and ear approvals |
| HUD | ENG-36 (+ ART-09) | 🟨 | The new flight HUD is the default; on main 50c4af7a: the descent HUD on its own dark plate, ORBIT surviving the travel, enemy markers above the hull at 1.4x; the on-foot HUD restyle and the Cinder pending marker on main since 29a15f21; story states (pips, conduit count, escape clock) await agent play; the navigation-pulse getter on lead/int-v; *Aftermath* lanes still open (T2) **(06:30)** The navigation-pulse getter, the wider descent plate and the subtitles setting on the bark lane are on main 94b56d11; the pips and assault score were seen in T7's play **(09:30)** Flight, marker, on-foot and descent HUD GOOD (T2); the docking plates and on-foot waypoint on main 80a2ee4d, NOT PLAYED; *Aftermath* lanes still open **(10:30)** The docking plates, generator pips, escape clocks and *Aftermath* lanes NOT PLAYED (T2) **(11:30)** **The flight HUD redesign (#160, R-049):** Concept A chosen, data model on main AL, T2 squad panel, flags, boost arc and toast, T6 radar; nothing drawn yet. The transponder flicker moved here (#103); #157 and #159 fixes on main (AL, AK), not seen **(12:30)** Pips, escape clock and *Aftermath* lanes GOOD (T2, #103); #157 GOOD. **The new flight HUD (#160) is drawn on main** (AP to AU: squad panel, flags, radar, boost arc, toast; first-paint crash fixed in adc75bc5); T7's first read good with a fix list; T2's fixes on main AU; frames at 1080p and native 720p not yet judged (T2); radar contrast (T6). Left: the tractor plate, the VERIFIED flicker **(13:30)** The new flight HUD played GOOD and closed (#160); the radar fix (BC), Minimal HUD setting (BG) and on-foot pilot card (BH) on main. Left on #103: post-wave radar header (fix in BI), boost arc, toast, three-line Large subtitle, tractor plate, VERIFIED flicker, the stray dev line, the CLOAKED flag  **(14:30)** No change on #103; the control hints re-offered on a New Game (f3b8d3f5, #159)  **(15:30)** Radar header with generators and reactor (223fbf9e), the grace HUD line; left on #103: the follow marker, the director-shot HUD hide, the reactor header, the pilot card against the reticle (T2); #159 hint cards (T1, T2) |
| Effects | ENG-37 (+ ART-07) | 🟨 | Tiers 1-3, round 9, the asteroid-break rework, `M_TD_PP_DescentWhite`, the in-cloud layer v1 and descent haze on main 50c4af7a. Round 10 on eng/vfx: **accepted from sheets: shield drop, Cinder cloud; not yet accepted: Verdance cloud v2, per-type plumes, approach field, light row, tractor beam, region beacon and haze**; `NS_TD_Magma_Heat` built for the Cinder MAJOR; a trailer-quality breakup still open **(06:30)** Accepted from sheets and in batch X (not on main): Verdance and Cinder cloud v2, shield drop, approach field, light row, CloudPunch, `DA_ReactorCore_FX`, glide wind. Not accepted: enemy plumes, region beacon, region haze, tractor beam. The magma heat effect is built, not seen **(09:30)** Seen: the magma heat effect (on foot at 5 m and 20 m). Not accepted: region beacon, plumes, tractor beam, haze, magma trim **(10:30)** Plume beacons accepted and wired; region haze and beacon DROPPED; the tractor beam accepted and in its slot, not seen in play (T4); the Cloak shimmer not accepted (polish) **(11:30)** The Cloak effect not accepted, one last pass queued (effects builder, #102), then #111 and #105; the tractor beam in docking (T4) **(12:30)** From play (#105): the bomber mines read as flat pale discs, the tractor beam only as a faint ring seen end-on; both with the effects builder (Grok Bot's concepts #173, #174), with the breakup explosions (#111) and the Cloak (#102). The blade beam remake is ART-08 (#163) **(13:30)** The effects builder's order: the Cloak shell (#102), the tractor beam and mines (#105), the breakup explosions at hull scale (#111)  **(14:30)** Breakup v2 on main (8f59cfe1); the tractor beam v2 and mine tracer taken back out of the data until seen (effects builder, waiting on T4's hooks)  **(15:30)** The Cloak shell done (#102). Left: the tractor beam v2 and the mine effect unwired (effects builder; T4's hooks on main), the Lancer flame core, the blade light on enemy armour (#128) |
| Ship, enemy, Dominion, crew, prop art | ART-03, 04, 06, 08 as engineering fallbacks | 🟨 | On main 50c4af7a: Rift v14, Lancer cockpit v08, Bastion v02, the Null interceptor's lances and fins, the bomber's twin-boom tail, landing gear meshes for the three ships (not wired), survey stations, the escape debris set, the Warden's clips and rebuilt death; none of batch U's art has been seen in the running game. On lead/int-v: the Dominion exterior in the Null colour code. The hauler repaint is open; the Bastion cockpit design waits for Ryan **(06:30)** The Dominion exterior in the Null colour code is on main 94b56d11; the magma bridge, station rack fix, camp canopy and shard patches on main 6cc4f254. Seen in the running game by the structures builder: the shard material, the three ships on landing gear, both station walks, the canopy. `MI_TD_DominionGlowRed` is held for Ryan **(09:30)** Lancer contrast via a fresnel darkening in `M_TD_Ship` (T1; the ship builder stopped at 07:00); the anchor charge mesh stretched (NOT GOOD); doorway chamfer on main **(10:30)** **ART-04 is ✅** (fleet, plumes, Dominion and crew seen in play). ART-03: Lancer contrast against cyan (T1); ART-08: no animation verdict yet **(11:30)** **ART-03 is ✅** (closed as shipped by the Lead's time-boxed decision; the Lancer boost plume rebuild to polish) and **ART-06 is ✅** (conduit art seen). ART-08: no animation verdict yet (#124) **(12:30)** ART-08 gains two open sub-points (Ryan's focus): the energy blade remake (#163: first frames NOT GOOD yet, round 2 on eng/blade) and the pilot animation (#164: round 1 good for the light combo, block and air-slam landing, in batch AV a14bf410; round 2 queued) **(13:30)** ART-08: the new blade on main BH (deflects and ignite still to prove, #163); the pilot animation round 1 played twice by T7 (idle stub and a Grunt inside the pilot NOT GOOD; round 2 on eng/anim; #164). Three Fab props from Nicholas in intake (#175 to #177, intake builder)  **(14:30)** Nicholas's three Fab props imported (7dc8d2bf), placement from code next (environment builder). ART-08: animation round 2 and the blade's walking-streak fix on main d9f5ba70 (BM), to be played (#163, #164)  **(15:30)** ART-08: #163 the blade and #164 the pilot animation closed GOOD; the crew clips live in fights; left: heavy-hit reactions (#124). Fab props placed, Rift and Lancer canopies glazed (#149; the Bastion's in rework) |
| Sky, lighting, look (**Ryan's since Nov 13**) | ART-05, 13 | ✅ | First lighting passes on main; on main 50c4af7a: the Verdance canopy at about 40% coverage and `UTDInteriorLightSubsystem` (alarm and collapse lighting; tested headless, driven live in the builder's own capture, not seen in a played run). On eng/env: the wreck scar as burnt ground, Cinder basalt and crystal fields, interior collapse roles; magma that reads as lethal is new work (the Cinder MAJOR); a per-level before / after still open **(06:30)** On eng/env, waiting on the s13 sheet: Cinder crystal clusters, terrain material, the scar, interior light states, the magma surface and glow **(09:30)** Interior light states seen in play; the wreck scar from the approach and a walking-distance pass queued (environment builder) **(10:30)** Ice Drift and Outer Dark changed in AD, not looked at (T6); Nicholas's findings 1, 5, 7, 8 with the environment builder **(11:30)** **ART-05 is ✅** (Ice Drift and Outer Dark accepted, #115). ART-13: the environment builder on #145, #148, #158 (60974009 on main AL, not seen) and the bay exposure (#125) **(12:30)** ART-13: #145 NOT GOOD (a teal rectangle inside the right limb remains), #148 unverified (frames black after the intro timing moved), #158 provisionally good; the bay look GOOD (#126) **(13:30)** ART-13: #145, #148, #158, #125 with the environment builder; ART-15's bay look GOOD and closed (#126)  **(14:30)** **ART-13 ✅** (#125, with #145 and #148 closed; collapse lighting 1cea37ed): the whole group is ✅ |
| Menus, reward screen, credits, loading | ART-09 | 🟨 | Engineering fallbacks work. Nick's styled menu is on main (ee5dc0ff), its activation pending; the reward mount is on main for his candidate (PR #85, draft) **(10:30)** From Nick's side: no new report. Nicholas's findings 2 (menu treatment) and 6 are his by ownership, awaiting his answer on #90; PR #85 closed by the Lead at 10:30 with its branch kept (03ab7ec7) **(13:30)** From Nick's side: the reward candidate's captures are ours to take (T2, #133); the Director cards presented to Nicholas (#136), no decision |
| Barks, voice | ART-11 | 🟨 | From Nick's side's own ledger: second draft published (2240d3cd); StoryFidelity01 corrections applied by T3 on main; Wren audition PR #78 waits for Nicholas to pick a master take; no voice recorded for the rest **(10:30)** From Nick's side: Wren take 04 chosen (N-097), imported as the shared Dont master (6ee7100d); not yet heard in play (T3) **(12:30)** From Nick's side (#171): the take selection for Grok Bot's 36 Wren lines (PRs #167 to #170) was asked of Nicholas; he answered "Send to project lead for guidance"; his Lead prepared a private listening package; no take is approved or imported yet **(13:30)** From Nick's side: no change; #171 waits on Nicholas's ear |
| Submission (video, page, dev log, credits) | ART-14, ENG-28 | 🟨 | Nick's side: itch page, tools and Glass Box drafts (e0b543c0, 2d549ba4), a 50 s rough cut of batch L footage (2690aad5). **ENG-28 is 🟨:** the newest published Shipping zip is main 50c4af7a (batch U), prerelease `playable-main-50c4af7a`: 876,082,811 bytes, 123.9 MB under 1 GB, cook 0 errors 0 warnings, launch check to wave 1 for 80 s; audio, flight input and anything past wave 1 are not verified in it. The cook of main 6cc4f254 is running; `CREDITS.md` checked against that cook (ART-02); final trailer, public page and both humans' sign-off open **(09:30)** Newest package `playable-main-8b4eabae` (877,326,337 bytes, cook 2,632 clean, launch check to wave 1); the cook of 80a2ee4d is running; submission drafts are Nick's side's task (N-095) **(10:30)** Newest package `playable-main-80a2ee4d` (batch Z); hourly cooks of the newest main on the hour; no package played past wave 1 **(11:30)** ENG-28: clean routes GOOD (#153), the death hitch not measured (T6); next cook 12:00; nothing from Nick's side this hour on ART-14 **(12:30)** ENG-28: Shipping package of main adc75bc5 built (0 cook warnings), launch check pending; next cook 13:00; no package played past wave 1 (#152). Nicholas approved the written art-direction guide (ART-01, #132; waits on Ryan's agreement) **(13:30)** ENG-28: #153 measured GOOD; Shipping 4bbdd282 launch-checked, adc75bc5 played 6.5 min (T7); packages every 30 minutes; the start-to-end package play is open (#152). **ART-01 is ✅** from Nick's side's closure of #132  **(14:30)** ENG-28: Shipping f40a39c3 (0.88 GB) played through wave 2 by T7; the galaxy load frame fixed (2cb0c27e); next cook 14:30. ART-14: Nick's side's credits reconciliation (1e2fdba7)  **(15:30)** ENG-28: Shipping 9f20c5be launch-checked, a Verdance save played in it (T7); next cook 15:30 from 8d62d14a; #152 open |
| Planetary descent (space to surface, anywhere on the globe) | ENG-38 (+ ENG-20, ENG-34) | 🟨 | **Ships as a smallest complete version** (Lead, Nov 14). On main 50c4af7a, unplayed: the glide placed over a corridor-fitted cloud deck on every entry path (300 m floor for uncovered arrivals), the mirrored take-off, async scatter loading at 4 ms per frame, haze, the wind slot, the exposure ramp, the descent HUD plate. T7 flew the Verdance entry on the batch O tree (before the glide) and took off and climbed to orbit there; the landing by hand is unplayed. Left for Ryan: see ENG-38 **(06:30)** T1 verified the natural Verdance route and the glide live (agent-driven); the entry hold waits for the story line on air (main 6cc4f254, test only); cloud v2, CloudPunch and glide wind accepted, in batch X only **(09:30)** **ENG-38 is 🟢:** descent GOOD (T1), descent HUD GOOD (T2); the glide-in leg NOT PLAYED; the hand-flown landing is ENG-20's **(10:30)** **ENG-38 is ✅** (natural entry, glide, descent and the hand-flown landing GOOD); glide wind and the HUD's glide-in leg to polish **(11:30)** **ENG-38 ✅ → 🐞:** Ryan's own play found the ship stuck against Cinder in wave 5, no burn and no turn-back (#161); T4's avoidance half on main AL; the player's bounce and burn with T1, no commit yet **(12:30)** **Stays 🐞** until #161 closes: T1's fix played GOOD by T7 and T1 (the ship turned away every pass, never parks); the notice, wording by cause and burn (83ea90cf, AT) not yet seen in a frame (T1) **(13:30)** **ENG-38 🐞 → ✅** (#161 closed: turned away at about 360 m, skips off, burn visible, PLANET CLOSED on its own plate; T1 on main 98c96926) |
| Boarding finale | ENG-29-31, ART-15, 16 | 🟨 WANTED (Ryan, Nov 13) | Built and played end to end by an agent on main 6fe3e90f (the reveal gate not verified on a merged build); **not played by anyone on batches M to U**; T4's docking capture and escape flight on main, played only on T4's branch; the docking-cone keep-out, alarm and collapse lighting and the music forwarding on main 50c4af7a; the tractor beam not yet accepted; the space-only ending stays as the fallback **(06:30)** **ENG-30 is 🐞:** T7 played the interior on main 50c4af7a from the hangar to the Warden; a death in the Warden fight skips the Warden (MAJOR, T5 + T3) and the Heavy's guard never breaks (MAJOR, T5). The docking route opens; docking on main is being played by T1; the reactor, the escape and victory are unplayed **(09:30)** **ENG-30 🐞 → 🟨:** both MAJORs fixed and played (T7: Warden retake and fight, Heavy hint, mercy GOOD); **ENG-29 is 🟢** (docking GOOD, pace 12-13.5 s; the lines hold NOT GOOD yet). **NOT PLAYED:** the reactor conduits and heart, the reveal, the escape run, reboard **(10:30)** **ENG-31 is ✅** (escape run and flight GOOD, T7). ENG-30: conduits, heart and Face to Face GOOD; the reveal and console recordings NOT PLAYED (T3), the defenders unbuilt (T5, 14:00). ENG-29 🟢: the tractor beam not seen (T4) **(11:30)** **ENG-30 is ✅** (#129) and **ART-17 is ✅** (#120). ENG-29: the tractor beam and the C7 lines (T4, T1; #110); ART-15: the bay mouth frame on main AL, T4's hook in AN, not seen (#126); ART-16 (#111) **(12:30)** ENG-29: the C7 lines aired; the tractor beam NOT GOOD seen end-on (#105, #110). ART-15: the bay mouth and bay look GOOD (#126); the guide lights left (T4). ART-16: the breakup explosions (#111) **(13:30)** **ART-15 is ✅** (#126). ENG-29: the tractor beam (#110). ART-16: the breakup GOOD, the hull-scale explosions open (#111)  **(14:30)** ENG-29: the tractor beam v2 unseen in the real dock and back to v1 (8f59cfe1), the docking lines (T3, #110). ART-16: breakup v2 on main, hull-scale explosions open (#111)  **(15:30)** **ENG-29 🟢 → ✅** (#110). ART-16: the tractor beam v2 and the hull-scale explosions (effects builder, #105, #111) |
| Story (The Paradox) | `storyline.md`; ENG-18, 25, 26, 29-31, 33; ART-11, 12 | 🟨 | Sequences fired on main; **ENG-42 is 🟨 again (06:30):** the *Aftermath* fix was played by T7 on main 50c4af7a through the real tutorial while boosting (the entry held, 16 lines aired, commit after); the line-tail hold, the interception, docking and assault lines are on main 6cc4f254 (tests only); T4's waypoint and story lines obeying the subtitles setting are on main 94b56d11. The Story Director's corrections applied on main 61404a79; words, voice and the final say with Nicholas; Nicholas explicitly approved the victory eyebrow MISSION COMPLETE // OPERATION PARADOX and random six-string large headline (Nov 14; Story/README.md) **(09:30)** **ENG-42 is 🟢:** transmissions GOOD (T3); console recordings, the final record and Cinematic 08 NOT PLAYED; Wren take 04 chosen by Nicholas (N-097), T3 imports it **(10:30)** ENG-42 🟢: the console recordings and the Dont voice not heard (T3); Wren take 04 imported (6ee7100d) **(11:30)** ENG-42 🟢: the seen / not-seen list is on #96; T5 plays the console recordings, the final record, Cinematic 08 and its skip; T4 the C4, C6, C7, interception and C9 lines **(12:30)** ENG-42 🟢: the three console recordings and Cinematic 08 with the Dont voice GOOD (T5); the C4 and C6 take-off lines NOT confirmed (T4 redoes them flown); Face to Face before Cinematic 08 and the skip NOT GOOD yet (T3, T5); a MAJOR from Nick's side's source review (an old run's recording into a fast New Game; T3, #139) **(13:30)** ENG-42 🟢: the #139 carry MAJOR fixed on main AY (played check, T3); the take-offs flown (T4); Face to Face order and the skip (T3)  **(14:30)** **ENG-42 🟢 → 🐞** (#96): the reactor can be reached without the final record; the reactor door is gated on it (T3 state, T5 door)  **(15:30)** **ENG-42 🐞 → 🟨:** the final-record gate on main (3b1fcde5), played GOOD by T5; left: Continue from the Warden checkpoint restoring it (#96) and the two take-offs flown (T4). **ART-18 ✅** (#119); **ENG-43 ✅** (#122) |

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
**Ryan's new top focus (Nov 14 16:15, issue #194): "the planets feel empty and flat on foot".** Both planets get terrain
relief (divots, craters, holes, mounds), mountains all around the play zone, and rocks, huts and trees filling the whole place,
with ambient effects (spores, mist, vents, embers). Owners: T4 terrain relief and the mountain ring (`World/TDTerrainField`,
`TDPlanetTerrain`); the environment builder density, trees, rocks, placement of the huts and Fab assets; the structures builder
the huts kits (both on main 28c2fb05); the effects builder the ambient effects; T6 the frame-rate check; T7 the
baseline and after frames and the verdict; T1 descent and take-off on the new terrain. ENG-39 and the two planet arrival rows of
the checklist are 🟨 until #194 closes.

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

**Moved here at 10:30 under Ryan's 10:15 rule** (leftovers that no longer hold an item's marker; each is still worth doing,
easiest first within its owner):
- **Nicholas's Manual Feedback Batch 1 (#90, human playtest of the menu and intro):** 1 white object above the menu planet,
  5 the blurry teal patch where the rings cross the planet, 7 the white spot in the opening planet shot, 8 general polish of
  that shot (environment builder; T3 for the menu and intro scenes); 3 the menu ship leaving before the frame edge, 10
  blurry ships in the intro flybys, the cue list for 11 and the pilot mesh paths (T3; effects builder on 10); 4 thruster
  and exhaust detail on the menu flyby (effects builder); 9 game-ready pilot meshes for Wren, Kato and Moss (the Lead;
  needs a character builder and an approved look from Nicholas); 2 the menu's visual treatment, 6 ambient music before the
  intro track and the sounds of 11 (Nick's side by ownership, awaiting his answer).
- The 49 s intro re-shoot with the final ships and effects (T3; ENG-33).
- One real mouse or pad click on CONTINUE (held for Ryan; ENG-19).
- The perfect-deflect window judged by feel (T5; ENG-22, line 6 above).
- A human run on each difficulty (line 5 above); the escape flight is easy, cleared with 25 s left (T4; ENG-27, ENG-31).
- The wave 5 frame budget with the field on (T6; ENG-48, line 18) and the full-boost impact curve watched (T1; ENG-48).
- Wingman rock avoidance watched on main (T3; ENG-08).
- The glide wind heard and the descent HUD's glide-in leg looked at (effects builder, T2; ENG-38).
- Verdance ground detail at walking distance (environment builder, parked: no visible change; ENG-34).
- The withdrawal waiting for Done watched (T3; ENG-40).
- Dev-only quirks: `TD.Stage DominionInterior` spawns the pilot behind the ship (T5, fix in AE); a staged wave 3 shows
  the tutorial prompt (T4).
- MINOR: a death in a gap between scenes sends the pilot back to the hangar start, 104 m from the reactor (T5, T3).
- `MI_TD_DominionGlowRed` 20 → about 6 (held for Ryan) and Nicholas's review of the Dominion recolour (ART-04).
- The Cloak shimmer on the Rift (effects builder; not yet accepted, an optional look once Cloak itself is played).
- Placeholder words waiting for Nicholas: Face to Face, APPROACH PREPARED · PASSAGE STILL BLOCKED, the travel, world and
  on-foot barks, AWAITING ORDERS (T3 seeds; Nick's Story Director).

**Moved here at 11:30** (same rule):
- Done on agent sight this hour from Batch 1: findings 1, 3, 4, 7 and 10 (#141, #143, #144, #147, #150); still open: 5 and 8
  (environment builder; fixes on main AL, not seen), 9 (pilot meshes, waits on a look), 2, 6 and the sounds of 11 (Nick's side).
- The Lancer's boost plume rebuilt for contrast against the cyan nebula (ART-03; effects builder or T1).
- The future-pilot defenders harder for the reveal (200 health with a guard) and their look captured (T5; ENG-30).
- The interior scripted walker wedging at a doorway from the Warden room (dev only; T5's markers 128a5907 in AN; T7 replays).
- The two Overcome lines and the other placeholder words (Nicholas, ART-11).
- Skip the tutorial through the new difficulty page (T3; ENG-45).

**Moved here at 12:30** (same rule):
- The Dominion console reads as a plain black box filling the lower view from 1.4 m (structures builder) and a tiny centred on-foot
  text line in very small type (T5) (#96).
- The defenders' look captured and their 200 health with a guard (T5; ENG-30), carried from 11:30.
- T4's four bay-mouth frame bars at about half emissive (#126).
- The radar's wave header named twice and its plate contrast over bright nebula (T2, T6; #160).

**Moved here at 13:30** (same rule):
- The #156 frame of Settings over the pause screen and the Subtitle size row cut by the Back bar on the Settings page (T1; ENG-45).
- A beauty close-up of a guide-light housing (structures builder; ART-15, #126).
- The effects builder's hull-scale explosions on top of the breakup that is already good (#111; ENG-18).
- The ENG-38 left-for-Ryan list (land anywhere, airbrake, bigger globes) stays with the final polish group (item 25).
- The blade's swing sheet a little smaller and less flat at its widest (blade builder; taste, #163).

**Moved here at 14:30** (same rule):
- Done on agent sight this hour: Batch 1 findings 5 and 8 (#145 the teal patch, #148 the opening shot) and B2-03 (#156 the pause
  screen through Settings); still open from Batch 1: 2, 6 and the sounds of 11 (Nick's side), 9 (pilot meshes, waits on a look).
- Nicholas's three recoloured Fab props placed in the levels from code (environment builder; imported 7dc8d2bf).
- The main menu reads plain (#133): its native styling is now T1's (#142, Nick's Lead accepted our offer; DA_UI_Style read-only).
- The intro cockpit canopies in tinted glazing (#149, Nick's Lead's choice; structures builder).
- The anchor collapse's dropped tower sections seen from the shore (T5, structures builder; #119; the collapse run itself is ✅).
- One real click on CONTINUE (held for Ryan; ENG-19).

**Moved here at 15:30** (same rule):
- Done on agent sight this hour: #163 the blade remake, #164 the pilot animation, #119 the anchor collapse (the 14:30 line on its
  dropped sections is done), the native main menu styling (#142, on main), the Rift and Lancer canopy glazing (#149).
- All four generators timed without god mode in a full Shipping run, and the turret lock telegraph seen in a frame (T4, T7; ENG-18, #152).
- The tractor beam v2 and the mine effect wired and seen (effects builder; T4's hooks on main; ENG-29, ART-07).
- The blade light washing enemy armour at arm's reach (blade builder, #128).
- Cloak used inside a wave with enemies targeting the ship (T7; ENG-12).
- The styled main menu checked with the mouse and in the package (T1; Nick's Lead's review, #142); the Bastion canopy (structures builder, #149).
- Face to Face's fifteen `G8_Self` lines (Nicholas; kept off air meanwhile) and `Bark_Wipe` (T3).

**Moved here at 16:30** (same rule):
- Done on agent sight this hour: all four generators timed without god mode (T7), Cloak inside wave 5 (T7), the styled menu with the
  mouse in the Shipping package (T7), the blade light washing armour (#128, on main), the Lancer flame core (on main), the Bastion
  canopy (on main; #149 open only for T7's real-speed intro check).
- The black menu for 8 s after launch in the 8d62d14a package (packaging agent; boot-to-menu now timed).
- #142's remaining menu checks: arrows / pad, a deliberate confirm, the Skip Tutorial guard, Esc on difficulty, frames at three
  resolutions (T7, next package; Nick's Lead's review).
- #192 mild tracking on the Rift and Lancer regular attacks, the Bastion unchanged (T2, queued; balance).
- Wave 5 carriers capped at 10, resuming under 6 (T4, on branch).
**Moved here at 17:30** (same rule):
- Done on agent sight this hour: the portal missile return (#100), the wave 5 carrier cap, the camera-blade guard (the sprint frame),
  the finished-planet line without a padlock, #139's hyperdrive MINOR.
- Verdance route views still open after fill pass 2 (environment builder; #194).
- Boot to menu: 30.9 s black on 685d8bea; the splash and no menu warm-up measured in the next package (packaging agent, T6; #89).
- The #146 opening bed's skip fade (T3).
**Moved here at 18:30** (same rule):
- Done on agent sight this hour: the relief shadow-map overflow at the pad, the squad-row layout at 1080p and 720p, the climb-out,
  boot to menu 30.9 s -> 13.4 s.
- The camera inside a hut wall on Verdance (environment builder; MINOR).
- About 10 s of black game window after the splash (T6's loading card; #89).
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

### C-03 · Theme: "Director's Cut" · both · `✅` 🎬
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
- **(Nov 14 06:30)** **Evidence (#1, switching is a cut; stays 🟢):** T1's core check on a live game in wave 3 (agent-driven): 13 mid-action switches, all instant, no stuck power, worst frame near a switch 15 ms, median 6.5 ms, p99 16.3 ms; no BLOCKER or MAJOR. A rapid-switch test (20 presses in 2 s at 30 / 60 / 120 fps) is on main 6cc4f254. **Open against #3 (scenes and takes):** a death in the Dominion Warden fight restarts the scene past the Warden (MAJOR on ENG-30; T5 + T3).
- **(Nov 14 09:30)** **Agent verdicts (Ryan's bar since 09:15; T3, T1):** the cut and pilot name card GOOD; scenes and takes GOOD (the Warden retake played four times on Y, the Warden back each time); **money shots NOT PLAYED** (slow motion not caught in frame). Stays 🟢: not judged good as a whole. Nick's parts unchanged.
- **(Nov 14 10:30)** **Stays 🟢:** the cut and name card GOOD, scenes and takes GOOD; **money shots in slow motion NOT PLAYED** (T3, next). Nick's parts unchanged.
- **(Nov 14 12:30)** **Stays 🟢 (Ryan's parts).** T3: `TD.Director.RamKill` gives a director shot clearly in frame (side camera on the Lancer, letterbox, the KATO · LANCER card, speed streaks); slow motion cannot be proven from stills, so T3 adds a log line for time dilation in and out (#97). **To close: the slow-motion beat shown in the log.** Nick's parts unchanged.
- **(Nov 14 14:30)** **🟢 → ✅ (Ryan's parts).** #97 closed by the Lead as played GOOD: the cut and pilot name card, the money shots with their slow motion (T3's time-dilation log line in and out on a director shot), and scenes and takes with CUT! all judged good in the running game. Nick's parts unchanged: the designed Director cards' look is ART-12 (#136, now on Nicholas's Lead under his delegation).

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

#### ENG-01 · Space flight model · `✅`
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
- **(Nov 14 09:30)** **Agent verdict (T1):** flight and boost GOOD; **drift NOT PLAYED**. Stays 🟢 until drift is played and judged.
- **(Nov 14 10:30)** **🟢 → ✅ (10:15 rule):** flight and boost GOOD (T1, 09:40), **drift GOOD** (T1 turn 1, 09:40-09:47, hand-flown). Nothing named is left.

#### ENG-02 · Space input map · `✅`
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
- **(Nov 14 09:30)** **🟢 → ✅ (agent-played and judged GOOD, T1, Ryan's bar since 09:15):** the space input map played with real keys through the tutorial, waves 1-2, docking and the assault (T1, T7). Not a human pass.

#### ENG-03 · Health, shields, death and respawn · `✅`
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
- **(Nov 14 10:30)** **🟢 → ✅ (10:15 rule):** damage and shields in play (wave 3 interceptor missiles hurt, T4 10:15; gunship shield break heard), death and the CUT retake GOOD in every scene played (T3, T7: four Warden retakes, arena takes with mercy). Polish: none.

#### ENG-04 · The three ships (gameplay stats) · `✅`
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
- **(Nov 14 10:30)** **🟢 → ✅ (10:15 rule):** all three ships flown and switched in combat (T1: 13 mid-action switches), docked with all three (T1, 12-13.5 s). Stat tuning is polish (AAA list, T4).

#### ENG-05 · Ship weapons · `🟨`
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
- **(Nov 14 09:30)** **Agent verdict (T2):** Lancer weapons GOOD; **Rift and Bastion primaries NOT PLAYED** this morning. Stays 🟢.
- **(Nov 14 10:30)** **Stays 🟢:** Lancer weapons GOOD; **Rift and Bastion primaries NOT PLAYED** (T2).
- **(Nov 14 11:30)** **🟢 → ✅:** Rift and Bastion primaries played good by T2 in the running game (agent-played; #98 closed 10:51); Lancer weapons were already good.
- **(Nov 14 17:30)** **✅ → 🟨 (Ryan, #201, 17:05):** good aim assist on all three ships (extends #192's mild tracking on the Rift and Lancer regular attacks; aim assist for all three decided by Ryan's Lead under delegation), with a HUD indicator (T2). **To close:** aim assist played in a wave on each ship and judged good (T2, T7; #201).

#### ENG-06 · Targeting & lock-on · `✅`
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
- **(Nov 14 10:30)** **Stays 🟢:** soft target, markers and the incoming-missile warning GOOD (T2, T4); **the Bastion missile locks (missile swarm) NOT PLAYED** (T2).
- **(Nov 14 11:30)** **🟢 → ✅:** the missile swarm locks played good by T2 with the swarm (agent-played; #99).

#### ENG-07 · Squad manager & instant switching (PROTECT) · `✅`
- `ATDSquadManager` spawns, tracks and respawns all three ships, and owns who the player controls.
  It implements `ITDHUDData`.
- **Switching:** next, previous or direct; possesses the target instantly on the same frame; the ship
  you left hands control to its AI and keeps its velocity; 0.4 s cooldown; downed ships can't be
  selected.
- **Switch Strike:** switching within 1.5 s of using a power gives +25% damage for 4 s.
- A power started on one ship finishes after you switch away (portal stays open, dome stays up).
- Fires `OnShipSwitched`, which drives Nick's "cut" presentation (C-03 #1).
- **(Nov 14 05:00)** **Played:** T7 made 8 manual switches in its agent play (04:14-04:29, real keys, the batch O tree (364aabb3 + T2's pool fix + T7's performance work; behind main by batches P-T)); each was honoured with its pilot card. No source commit in batches P-T touches switching. Stays 🟢.
- **(Nov 14 06:30)** **Played:** T1's core switching check on a live game in wave 3 (agent-driven): 13 mid-action switches, all instant, no stuck power, worst frame near a switch 15 ms, median 6.5 ms, p99 16.3 ms; no BLOCKER or MAJOR. New test on main 6cc4f254 (fe28d58b): 20 presses in 2 s, each leaving a power pressed and boost held, leave one possessed and flown ship, the camera on it and no power stuck. Stays 🟢
- **(Nov 14 09:30)** **🟢 → ✅ (agent-played and judged GOOD, T1, Ryan's bar since 09:15):** switching GOOD; the 13 mid-action switches of 06:15 and the rapid-switch test stand; no source change to switching since. Not a human pass.
- **(Nov 14 16:30)** **Stays ✅.** Switching under fire played GOOD by T1 (15 manual switches); the auto-switch grace fires in a real fight (T7, Shipping 8d62d14a).

#### ENG-08 · Wingman AI (PROTECT) · `🟨`
- States: Formation → Engage → Evade → Regroup → Downed.
  - Formation: a loose V at 1500–2500 cm offsets; boosts to catch up beyond 8000 cm. **(Nov 12)** The V is
    swept forward (both wingmen ahead and outboard of the player, at different depths) so the chase
    camera shows it.
  - Engage: attacks whoever threatens the player first, then the nearest, then the lowest health;
    primary and secondary weapons only.
  - Evade: kicks in on shield break or an incoming missile.
- ~~**Never uses signature powers or abilities.** Those fire only when the player is in that cockpit.~~ **Replaced by Ryan, Nov 14 17:05 (#201):** wingmen **use their signature powers and unlocked abilities** when ready, and use them well. **Shadow mode, on by default:** the two wingmen shadow the player's ship and fight within range (not glued to it); **shadow off:** they range farther as independent fighters and hit harder, but not too far. Owners: the wingman builder (`.wt/wing`, eng/wing: shadow mode, wingman powers), T2 (the HUD indicator), T4 (balance once it lands), T7 (the verdict). Done when played by an agent in a wave and in the assault with shadow on and off.
- Follows the player through portals and gates and jumps on hyperdrive. `IA_SquadCommand` = "focus my
  target" for 8 s.
- Avoids rocks and teammates; moves toward an active Bastion dome when hurt; 2–4° aim error.

- **(Nov 12)** Target choice: one wingman may defend against an off-screen threat at a time; the other
  prefers targets inside the player's view (within 80 m bands of its best target). Wingmen keep clear of
  the aim line and of an active director shot. In a furball a wingman shares the frame with an enemy well
  under half the time, so the wing is made readable through the HUD (`GetSquadMarkers`: named arrows in
  squad colours) and short position calls (`Bark_SquadEngage...`), not by formation flying.
- **(Nov 14 03:00) Open finding, marker unchanged:** wingmen graze rocks inside a pocket of the space field (#90,
  comment of 02:30; seen by an agent). T3's rock-avoidance fix is in progress, not on main. If a played run on main
  shows wingmen dying on rocks, this item goes to 🐞.
- **(Nov 14 04:00):** T3's avoidance fix (55ba5156: a heading probe, a close push-out, slowing near rocks) is in
  batch S's gate (lead/int-s b64aaea7), not on main. Marker unchanged.
- **(Nov 14 05:00)** **State:** T3's avoidance fix is on main 4eb57848 (batch S, 936 tests). Played evidence: T7 flew about 4 minutes of combat through the asteroid field with no wingman lost to a rock, on the batch O tree, which is before the fix; T3 flew a 2-minute pocket run with the fix on its own branch (0 rock hits). Replay on main pending. On T1's branch: a formation fix. Stays 🟢.
- **(Nov 14 06:00)** **State:** T1's formation fix (7e2af961: no ceiling lowering once the squad is over the deck; the live wingman drop) is on main 50c4af7a. Replay on main still pending.
- **(Nov 14 06:30)** **State:** no new play of the wingmen on main; T7's tutorial replay on main 50c4af7a reported no wingman problem.
- **(Nov 14 10:30)** **🟢 → ✅ (10:15 rule):** wingmen engaging and kept alive through about 4 minutes of combat with no wingman lost (T7, real keys, batch O tree, since on main), and flying with the squad through waves 1-5 (T4). Polish: wingman rock avoidance watched on main (T3).
- **(Nov 14 17:30)** **✅ → 🟨 (Ryan, #201, 17:05):** shadow mode (on by default) and wingmen that use their powers and abilities replace "basic attacks only" (spec above). **To close:** played by an agent in a wave and in the assault with shadow on and off (wingman builder, T7; #201).
- **(Nov 14 18:30)** **Stays 🟨.** Shadow mode and wingman powers on eng/wing (1a3025fa, d7d67f08), played offscreen by the builder, **not merged: sent back as too strong** (the wingmen cleared wave 3 alone in about 50 s); rebalance and proofs in work. AGENTS.md section 2 updated for shadow mode (a946ca93).

#### ENG-09 · Rift: Portal Blaster · `✅` ⚠️ highest risk
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
- **(Nov 14 09:30)** **Not played this morning** (T2: signature powers NOT PLAYED tonight; last seen in T1's 06:15 switching check: portal open, ram, swarm, dome). Stays 🟢; T2 plays each power in a fight next.
- **(Nov 14 10:30)** **Stays 🟢:** the portal blaster fired GOOD (T2, live); **flying through a portal and a missile returned through one NOT PLAYED** (T2).
- **(Nov 14 12:30)** **Stays 🟢.** T2: both portals open and read well, the Q pip counts down (#100). **To close: a missile returned through a portal** (T2, a hand-aimed pass).
- **(Nov 14 13:30)** **Stays 🟢.** `TD.Power.ToPortal` dev command on main (81674996, BD) for scripted portal passes. **To close:** a missile returned through a portal (T2, #100).
- **(Nov 14 15:30)** **Stays 🟢.** T2 is tracing why the ship's crossing does not register in its portal test. **To close:** a missile returned through a portal (T2, #100).
- **(Nov 14 16:30)** **Stays 🟢 (branch only).** T2's missile returned through a portal is seen GOOD on its branch, with the Rift's own portal transit GOOD by trace; it reaches main in the next batch. **To close:** that batch on main and #100 closed.
- **(Nov 14 17:30)** **🟢 → ✅.** #100 closed as played GOOD: on main 0c986a7d the portal trace and the missile-at-portal staging (`TD.Power.MissileAtPortal`, d9a5e1a3, 0d63dcaf), each seen GOOD in the running game by T2 and checked by Ryan's Lead.

#### ENG-10 · Lancer: Afterburner Ram · `✅`
- 0.3 s wind-up, then a 1.2 s dash at 3× boost speed. Lancer is invulnerable and steers at 40%.
- Deals **300 to whatever it touches**: passes through fighters, **breaks gunship shields** and
  carrier bay doors.
- Hitting something big stops the dash with a 150-damage shockwave over 1500 cm.
- 7 s cooldown. Exposes `PowerWindup01` for the wing-fold animation (Nick).
- **(Nov 14 09:30)** **Not played this morning**; last seen in T1's 06:15 switching check (ram). Stays 🟢; T2 plays it in a fight next.
- **(Nov 14 10:30)** **🟢 → ✅ (10:15 rule):** the afterburner ram GOOD (T2 play turn, live) and the ram through an asteroid cluster GOOD (T6: three pieces in one pass).

#### ENG-11 · Bastion: Missile Swarm & Shield Dome · `✅`
- **Missile Swarm (tap):** 12 micro-missiles split across painted locks (or the soft target),
  18 damage each with a 200 cm splash; 9 s cooldown.
- **Shield Dome (hold 0.5 s):** a 3000 cm radius for 6 s with a 16 s cooldown.
  - Squad ships inside take no damage and regenerate shields twice as fast.
  - Enemy shots are destroyed on the dome, and missiles are deflected back.
  - The dome follows Bastion slowly.
- **(Nov 14 03:00) State:** the dome-from-inside fix (T2, 82d6040d: the placeholder rings hide while the camera is
  inside) is on main with its test. Not yet seen again in a running game: T7 looks at it in the end-to-end playtest
  of main 61404a79 (queued after its performance window); 🟢 once it is.
- **(Nov 14 05:00)** **🟨 → 🟢:** T7 saw the dome from inside in its agent play (a faint hexagon field, not three yellow lines), on the batch O tree (364aabb3 + T2's pool fix + T7's performance work; behind main by batches P-T). That tree holds the fix 82d6040d and its test; no source commit in batches P-T touches the Bastion's powers, so the play covers what is on main 29a15f21. No review finding is open. Not ✅: no human has judged it.
- **(Nov 14 09:30)** **Not played this morning**; last seen in T1's 06:15 switching check (swarm, dome). Stays 🟢; T2 plays it in a fight next.
- **(Nov 14 10:30)** **Stays 🟢:** the shield dome GOOD (T2, live); **the missile swarm NOT PLAYED** (T2; TD.Power.Hold/Release on main AC for the lock).
- **(Nov 14 11:30)** **🟢 → ✅:** the missile swarm played good by T2 (agent-played; #101); the shield dome was already good.

#### ENG-12 · Unlockable abilities (CORE: powers aren't optional) · `✅`
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
- **(Nov 14 06:00)** **Played:** both awards were announced in T7's Verdance play (Gravity Well at the device, Cloak at the pad) and the Cloak prompt showed in flight, on the batch O tree. MINOR (T5): Cloak is announced about four minutes after the transfer; "Cloak at the transfer" is on eng/t5. Dismissing the announcement by keyboard and pad is not verified.
- **(Nov 14 06:30)** **State:** Cloak is now announced at the transfer (285d4869, main 94b56d11, batch V), tests only; dismissing the announcement by keyboard and pad is still not verified. Stays 🟢
- **(Nov 14 09:30)** **Agent verdict (T2): Cloak, EMP Pulse and Gravity Well NOT PLAYED in a fight**; the reward screen GOOD staged, the natural award NOT PLAYED. Stays 🟢 on the earlier announcements and Cloak prompt; T2 plays all three next.
- **(Nov 14 10:30)** **Stays 🟢:** EMP Pulse GOOD (flash and bark) and the Gravity Well launch GOOD (T2, live); the hint card MAJOR (wrong ship, EMPPULSE) fixed on main AC (d5cb3ff5), not replayed; **Cloak used in a fight NOT PLAYED** (T2); the Cloak hull overlay slot is on main AC, the effect itself is not accepted (effects builder; an optional look, it does not hold the marker once Cloak is played).
- **(Nov 14 11:30)** **Stays 🟢.** EMP Pulse and Gravity Well GOOD (T2). **To close: Cloak used in a fight** (T2, #102). The Cloak effect is not accepted; one last pass is queued (effects builder), then it stays polish.
- **(Nov 14 13:30)** **🟢 → 🟨.** T7's first look on main a14bf410 (wave 3-4, Rift, real E key; #102): the ability triggers (cloak-on bark, cooldown on Wren's card, the ambush bark on the first shot), but the Rift stays fully opaque with no shimmer and the squad card shows no CLOAKED flag, so a player cannot tell Cloak is on: the visible part is unbuilt on main. **To close:** the effects builder's cloak shell (magenta edge, hull about a third visible) merged and seen; T2 finds why the CLOAKED flag did not show; T7 uses Cloak inside a wave with enemies targeting the ship.
- **(Nov 14 15:30)** **🟨 → ✅.** #102 closed as played GOOD: the Cloak hull swap on main 8d62d14a; the shell made far more see-through at Cloak 1 (44d9fa1b). Polish: Cloak used inside a wave with enemies targeting the ship (T7).
- **(Nov 14 16:30)** **Stays ✅.** Cloak used in wave 5 GOOD (T7, Shipping 8d62d14a, real mouse and keys); the 15:30 polish line on Cloak inside a wave is done.

#### ENG-13 · Enemy ships & AI · `✅`
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
- **(Nov 14 09:30)** **Agent verdict (T4):** waves 1-2 GOOD; **waves 3, 4, 5 and each enemy type NOT PLAYED**. Stays 🟢.
- **(Nov 14 10:30)** **Stays 🟢:** fighters, interceptor missiles, gunships and carriers GOOD (T4, waves 3-5); **bomber mines NOT SEEN** (T4).
- **(Nov 14 11:30)** **Stays 🟢.** **To close: bomber mines seen in play** (T4, in its lock turn; #109).
- **(Nov 14 12:30)** **🟢 → ✅:** bomber mines played good by T4 (agent) on c94f1795: three bombers lay a mine trail, one detonates, the Mines Ahead bark fires once; the Lead looked at the frames (#109 closed 11:34). The mine look (flat pale discs) moved to the effects list (ART-07, #105).

#### ENG-14 · Wave director · `✅`
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
- **(Nov 14 06:30)** **State:** wave warp-in points never sit inside an obstacle (12 retries, last-resort step-out; test `Waves.Spawn.NeverInsideAnObstacle`) and the JUMP POINT waypoint are on main 94b56d11 (batch V); the G4 interception lines for waves 3 and 4 on main 6cc4f254 (batch W). Tests only. Stays 🟢
- **(Nov 14 09:30)** **Agent verdict (T4):** waves 1-2 GOOD; waves 3-5 NOT PLAYED. On main 80a2ee4d (batch Z): a wave's own objective line (wave 5 BREAK THE DOMINION GUARD) and the approach field opening with Cinder and wave 5 in either order (126b238d, 9b878002), tests only. Stays 🟢.
- **(Nov 14 10:30)** **🟢 → ✅ (10:15 rule):** waves 1-2 GOOD (T4), waves 3, 4 and 5 GOOD (T4 lock turn 10:15-10:20: interceptor missiles, all four types warp in, carrier launches, the count rises, BREAK THE DOMINION GUARD, APPROACH PENDING). Bomber mines are ENG-13's. MINOR (staging artifact, polish): a staged wave 3 shows the tutorial prompt (T4).
- **(Nov 14 17:30)** **Stays ✅.** Wave 5's carriers hold at 10 fighters alive and resume under 6 (09bba192), played on main 6f0536ca (T4).

#### ENG-15 · Hyperdrive · `✅`
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
- **(Nov 14 03:00) State:** the one-jump fix (T1, 030561f9) is on main 6fe3e90f and was played by an agent (T7,
  session 2: one jump from the Aftermath point, 5.1 km to a 1.7 km stand-off off Verdance). The attached streak loop
  (9ba18c84, main eb1bf334) has not been seen by an agent on main yet. Waiting for a human.
- **(Nov 14 05:00)** **State:** in T7's play the jump beat after the withdrawal never happened (the *Aftermath* bug, ENG-42), so the one-jump path was not replayed. `td.Hyperdrive.IgnoreLock` is in batch U's gate; T4's jump-point waypoint is on a branch.
- **(Nov 14 09:30)** **NOT PLAYED this morning** (T1); the one-jump path was last played at 03:00 on 6fe3e90f. T4's galaxy travel (withdrawal → JUMP POINT → Verdance) GOOD. Stays 🟢; T1 replays one held jump to each planet.
- **(Nov 14 10:30)** **Stays 🟢:** the hyperdrive lock GOOD; **the jump NOT PLAYED** (T1, method problems, one more short turn).
- **(Nov 14 11:30)** **🟢 → ✅:** one held hyperdrive jump played by T1 (10:27, real held key at Cinder orbit): the gravity-well refusal, the spool with its bark and effect, the jump, the recharge (#104); the combat lock was already good.
- **(Nov 14 16:30)** **Stays ✅.** A hyperdrive jump played GOOD again by T1 with the new HUD.
- **(Nov 14 17:30)** **Stays ✅.** #139's hyperdrive MINOR fixed on main 8ea0f5f1 (aa63ca76: `td.Hyperdrive.IgnoreLock` at the spool and during the jump, drop-outs log their cause).

#### ENG-16 · Portal gates (galaxy shortcuts) · `✅`
Three permanent `ATDPortalGate` pairs: combat zone ↔ Verdance orbit, combat zone ↔ Cinder orbit, and
a flank pair used in waves 3–5. They reuse ENG-09's transit logic, and wingmen follow. Shots pass
through a gate unchanged: only Rift's own portals return enemy missiles.
- **(Nov 14 09:30)** **🟢 → 🟨 (Ryan's bar since 09:15: NOT PLAYED is 🟨):** no agent has flown through a portal gate in any played run on record (T1: NOT PLAYED). Owner T1: fly each of the three gate pairs with wingmen following.
- **(Nov 14 10:30)** **Stays 🟨:** **a portal gate transit NOT PLAYED** (T1 missed the ring; one more short turn).
- **(Nov 14 11:30)** **🟨 → ✅:** the ring is centred on the gate (498a9081, on main in batch AG efdf4284; 0 m offset on every gate) and T1 flew through the Flank_West ring and came out at Flank_East (10:46, agent-played; #106).
- **(Nov 14 16:30)** **Stays ✅.** A portal gate flown GOOD by T1.

#### ENG-17 · Galaxy level: gameplay layout · `✅`
- **Zones:** the combat zone (asteroids, station ruins as cover), Verdance and Cinder orbits, and
  the Dominion approach.
- **Gameplay actors:** `ATDPlanetAtmosphere` triggers, spawn zones, portal gates and return points
  (`Verdance_Orbit`, `Cinder_Orbit`, `Dominion_Hangar_Exit` for the stretch).
- **Shared map:** dressing, lighting and VFX on the same map are Ryan's side's too since Nov 13 (One File Per Actor).
- **(Nov 14 09:30)** **Agent verdict (T4):** galaxy travel withdrawal → JUMP POINT → Verdance GOOD; planet arrival and haze GOOD. Stays 🟢.
- **(Nov 14 10:30)** **Stays 🟢:** travel and the space field along the journey GOOD (T4, T6); the portal gates (ENG-16) unplayed.
- **(Nov 14 11:30)** **🟢 → ✅:** one portal gate transit played by T1 (Flank_West to Flank_East, 10:46) on the fix from ENG-16 (#107).

#### ENG-18 · Dominion assault & space finale (CORE) · `✅`
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
- **(Nov 14 03:00) State (🟨 since 02:00, was 🐞):** the space-reactor fallback ending could not be finished with real
  fire (issue #90 no. 5). T4's fix 818d592e (reactor 3 x 400, turret bursts eased, a trench death retakes at the trench
  start) is on main with its tests and the Dominion on the r2 hull (b59423d5); generator pips and the transponder
  flicker (cda0a770) are on main too. None of it has been played by an agent on main yet (T7's end-to-end playtest of
  61404a79; T2's live captures); 🟢 once it is. The assault with boarding on was played to the bay by an agent on main
  6fe3e90f (generators by a labelled director command); docking: see ENG-29. Without an owner until this hour (queued
  on `CLAIMS.md`, T4 with T3): the wording pass ("shield", never "field"; "approach prepared") and the check that the
  approach field needs Cinder **and** wave 5.
- **(Nov 14 04:00) State:** nothing new on main for the assault; the fallback is still unplayed (T7's end-to-end
  play has not started). The shield-drop, approach-field shimmer and tractor-beam effects are specs on the effects
  branch, with slots asked of T4. The wording pass and the approach-rule check are still queued (T4, T3).
- **(Nov 14 05:00)** **State:** nothing new on main for the assault, and T7's first session did not reach it (its next session is running). T4 played docking and the escape flight on its own branch. Shield drop and approach-field shimmer effects are on the effects branch; the wording pass and the rule check are still queued (T4, T3).
- **(Nov 14 06:00)** **State (🟨):** not played by anyone since batch L; T7's play stopped on Cinder. On lead/int-v, not on main: the Dominion exterior in the Null colour code (b24c8bc4; `TD_DOMINION_LOOK=old` restores the old values). The shield drop effect is accepted from its sheet and waits for T4's slot; the approach field is not yet accepted. T6's docking-cone keep-out is on main 50c4af7a.
- **(Nov 14 06:30)** **Played (stays 🟨):** T7 played the assault on main 50c4af7a (agent, real keys): **it works**: the assault score, generator pips 0/4 to 4/4, kill shots and lines, the docking route opens. The generators were killed with a labelled debug command, so the fight by hand is not judged, and the space-reactor fallback is still unplayed. On main since: the exterior in the Null colour code (94b56d11); four assault lines (`G6_Kato_MarkingTargets`, `G6_Kato_ShieldWeakening`, `G6_Moss_TurretsCharging`, `G6_Wren_ClearCarriers`), the ClearSpace roll call, the `TractorBeam` and `AftermathLightRow` slots, escape debris as art (6cc4f254, 21040d6b; tests only). In batch X, not on main: the shield drop on `DA_Dominion_Visuals.ShieldCollapse` and the approach field on `DistortionFieldIdle` (both accepted). `MI_TD_DominionGlowRed` is left as it is, held for Ryan.
- **(Nov 14 09:30)** **Agent verdicts (T4):** the Dominion assault GOOD (generators still killed by a debug command); **the space-reactor fallback NOT GOOD yet:** played to VICTORY by T4 with the reactor brake assist on lead/int-aa (f95043c1; Lead's decision: ease to 20 m/s within 200 m, the player keeps control), but about ten enemy markers stay after victory and the breakup was not captured. Stays 🟨.
- **(Nov 14 10:30)** **Stays 🟨:** the Dominion assault GOOD (generators by debug command, T4); the space-reactor ending played to VICTORY with the brake assist; the after-victory fix (782669d5: the final blast retires turrets, generators and reactor) is on main, **not replayed through the breakup** (T4).
- **(Nov 14 11:30)** **Stays 🟨.** In T4's lock turn (#112): the space-reactor ending replayed through the breakup after 782669d5, the four generators killed by hand.
- **(Nov 14 12:30)** **Stays 🟨.** T4's play (#112): the reactor killed by held fire, the slow-motion kill shot with letterbox, GET CLEAR OF THE DOMINION, VICTORY with Command's line and no enemy markers left; the outside director camera (361b56bc) GOOD. NOT GOOD yet: the breakup (small sparks; the ship still looks intact) and the generators by real fire (one at half hull in 7 s). On main AQ 92c94f81, unseen: a full-screen flash at the final blast, the hull replaced by 11 hull-sized chunks that tumble and burn, the camera held 2.2 s past the blast, the letterbox held over the cut, generators at about 15 s of focused fire. **To close: the generators killed by real fire and the breakup on screen** (T4; hull-scale explosions by the effects builder).
- **(Nov 14 13:30)** **Stays 🟨.** The breakup seen GOOD (#112, T4's outside camera, the Lead looked at the frames): after the reactor kill the cut holds the letterbox, explosions walk the hull, and at the final blast large dark burning hull sections drift apart in a big fireball (log: 11 hull chunks, 7 hull sections of 3 meshes, hull 656 m; the structures builder's `SM_KS_DomHullChunk_A/B/C` on main AZ). **To close (T4, playing now):** the four generators killed by real fire in one run and the cut back to VICTORY. The effects builder's hull-scale explosions are an extra on top.
- **(Nov 14 14:30)** **🟨 → 🐞.** #112 closed earlier on the breakup and the four generators played with god mode, but the assault **without god mode is not survivable**: 22 deaths in 8 minutes and no generator down (Lead's look at the captures, #152). **Still missing:** T4's rebalance (generator hull 400 → 250, turret damage and telegraph, a fighter cap) and T2's 1.5 s grace after an auto-switch on death, then the four generators by real fire without god mode on Standard. Breakup effects v2 on main (8f59cfe1); the hull-scale explosions stay ART-16 (#111).
- **(Nov 14 15:30)** **🐞 → ✅.** #112 closed as played GOOD on a71e3f73: T7 without god mode, two generators in 88 s, 0 deaths, after T4's rebalance (17e2b929, with the turret lock telegraph at the muzzle) and T2's 1.5 s auto-switch grace with shimmer and a HUD line. The withdrawal fix (corridor aimed beside Verdance, reminder and leash, proven by log) and T1's band guard (06abf7d9) are on main. Polish: all four generators timed without god mode in a full Shipping run (T4, T7, #152); the turret telegraph seen in a frame (T4); the hull-scale explosions stay ART-16 (#111).
- **(Nov 14 16:30)** **Stays ✅.** All four shield generators in 3 min 28 s with one death, without god mode, played GOOD by T7 on a9c635de (the 15:30 polish line on timing all four is done); generator hull 300 on main (e6b3f8d5); the turret lock telegraph enlarged (350f7faf, on main), not yet seen in a frame. Wave 5 carriers capped (hold at 10, resume under 6) is on T4's branch, not on main.

#### ENG-48 · Destructible, solid space field (CORE, added Nov 13 evening by Ryan) · `✅`
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
- **(Nov 14 05:00)** **State:** T6's first-break fix (4e01e6d1: `DA_SpaceField_FX` looked up at BeginPlay) is in batch U's gate; measured first breaks at most 1.22 ms (were 12-19 ms; the session's very first was 26.8 ms). The first reward pickup spawn still costs 25 ms (T2's pre-spawn, branch). T7's 4 minutes of combat in the field were on the batch O tree, before the seven regions: replay on main pending.
- **(Nov 14 06:00)** **State (🟨):** on main 50c4af7a (batch U): regions closer to their routes and denser (Graveyard 220 fragments, Scar 8 husks, Ice Drift 900 rocks), the Shattered Moons along the Dominion route, the effects data looked up at BeginPlay, one inert pickup of each type pre-spawned (first real pickup 25 ms → 0.98 ms, asserted in a test), and the material flag that fixes the checkerboard on the shards and hulls placed through instanced meshes (83a231c9). None of it has been seen in the running game on main. On eng/t4: warp-in points never inside an obstacle.
- **(Nov 14 06:30)** **State (🟨):** the moon shard material fix was seen in the running game by the structures builder (region 6 at 1 km); shard face patches are on main 6cc4f254 (the fine-grid mesh change was not requested, Lead). Warp-in points never inside an obstacle are on main 94b56d11. T6's re-cut branch eng/t6-w (dressing default off, journey keep-outs; approved by the Lead) is coming in batch X.
- **(Nov 14 09:30)** **Agent verdicts (T6):** break performance GOOD on its branch; **breaks by tier, drops, solid impact (T1), the wave 5 budget and avoidance NOT PLAYED on main**. Lead's decision: T6 pushes 436 adopted layout rocks out of the journey corridors (rocks on the route NOT GOOD, fix building). Stays 🟨.
- **(Nov 14 10:30)** **🟨 → ✅ (Lead's decision under the 10:15 rule):** small rocks break to fire, medium to bolts, the ram through a cluster, the special drop and pickup and the break cost (avg 1.03 ms, worst 2.17 ms) all played GOOD (T6, live); the rock health tuned on main in batch AD (27e9b92e: small 2, medium 8, large 14-20 bolts); rocks moved off the journey corridors (seen live). Polish: the wave 5 frame budget with the field on (T6, AAA 18) and the full-boost impact curve watched (T1).
- **(Nov 14 18:30)** **Stays ✅ (Lead to confirm).** #139's independent review found a MAJOR in the space field's state reference (asteroid split); T6's fix 0ed9cd18 is stress-tested in a game and compiling, not on main.

### Game loop & progression

#### ENG-19 · Game instance, progression, saves & travel · `✅`
- `UTDGameInstance` implements `ITDGameFlow` and holds `FTDProgress`.
- **Travel:** `TravelTo` fades to `L_Transition` (Nick's loading screen widget), then the target
  level.
- **Checkpoints:** taken on level entry, wave clear and objective complete. The **scene/take restart**
  (C-03 #3) reuses them. The save slot supports Continue.
- **Defeat:** "Retry from checkpoint" or "Quit". Run stats feed the victory screen.
- **Debug menu** (F10, dev builds only): jump to any phase, unlock abilities, god mode, kill all,
  skip wave.
- **(Nov 14 06:00)** **Continue after a relaunch works** (T3, agent-driven on main 50c4af7a): the menu shows CONTINUE, the run resumes at the saved wave in about 2 s, the intro and already-heard lines do not replay. The 03:00 finding is closed as not a game bug. Not checked by an agent: a real click on the CONTINUE button (for Ryan, one press with mouse or pad). Stays 🟢.
- **(Nov 14 06:30)** **State:** T3's Continue check stands; the real click on CONTINUE is still for Ryan. Stays 🟢
- **(Nov 14 09:30)** **Agent verdict (T3):** Save and Continue GOOD (the real button click untested: injected keys do not reach Slate). Stays 🟢 until the click is made.
- **(Nov 14 10:30)** **🟢 → ✅ (10:15 rule):** Save and Continue GOOD (T3, agent-driven: CONTINUE resumes the saved wave in about 2 s, nothing replays), the menu and travel between every map played (T7, T1, T4). Held for Ryan (method limit, polish): one real mouse or pad click on CONTINUE.

#### ENG-20 · Atmosphere flight, landing & take-off (gameplay) · `✅`
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
- **(Nov 14 03:00) State:** parked ships are solid to the pilot (T1, f9f89594, with a test) on main; landing,
  boarding and take-off were played by an agent on 6fe3e90f. On main and awaiting agent play: "Hold E" on the board
  prompt (T5, 8e758175, main eb1bf334); the atmosphere-entry hold while *Aftermath* plays (T1, #90 no. 6) and the
  climb-out roar (df563569), both main f39a8372. Open: walking into a parked ship not yet seen live (T7). The arrival
  and take-off themselves are ENG-38.
- **(Nov 14 05:00)** **State:** the landing by hand is still unplayed (T7 used a console command to land). Landing gear for the three ships is being modelled as separate meshes (structures builder, branch; Lead's decision, it had been listed as not possible tonight). Decided by Ryan's Lead: the atmosphere entry commits after *Aftermath* if the player is still at Verdance's edge.
- **(Nov 14 06:00)** **State (🟨):** T7 took off, climbed to orbit and returned to the galaxy from Verdance in its play (batch O tree), and boarded by holding E at the pad console. The arrival was staged by a command: no parked ship was visible on the pad (MINOR, to be checked on a landing by hand). Landing gear meshes for the three ships are on main 50c4af7a, not wired to the landing (T1, animation builder). A landing by hand is still unplayed.
- **(Nov 14 06:30)** **State (🟨):** T1 verified the natural Verdance route and the glide live (agent-driven); the structures builder saw the three ships parked on landing gear in the running game. The landing barks (`Bark_Land_Ready`, `Bark_Land_Down`) are on main 94b56d11. MINOR (T5, T7's play on main 50c4af7a): the flown ship is missing from the staged Verdance pad; T1's fix that puts the flown ship's pilot on at any level change (8e26e739) is in batch X. A landing by hand played to touchdown is still not reported.
- **(Nov 14 09:30)** **Agent verdict (T1):** entry and exit GOOD; **hand-flown landing and take-off NOT PLAYED**. Stays 🟨.
- **(Nov 14 10:30)** **🟨 → ✅ (10:15 rule):** **the hand-flown landing and take-off GOOD on both planets** (T1 turn 1: Verdance 84 s down, 39 s up; Cinder 59 s down, 41 s up); atmosphere entry and exit GOOD (T1).

#### ENG-21 · First-person character (planets: CORE) · `✅`
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
- **(Nov 14 06:00)** **Played:** the on-foot pilot was used through the whole Verdance chain and on Cinder to the gate conduits by T7 (batch O tree). Stays 🟢.
- **(Nov 14 06:30)** **Played:** the pilot was used through the Dominion interior from the hangar to the Warden (T7, main 50c4af7a). MINOR (T5): the hangar spawn is directly behind the parked Rift. Stays 🟢
- **(Nov 14 09:30)** **Played (T7, real keys, batch Y + T5's arena fix):** the pilot from the hangar through the corridor, arena and deck waves to the Warden, all GOOD. MINOR open: the staged interior start (`TD.Stage DominionInterior`) still spawns the pilot behind the parked ship (hangar start fix in Y not taking effect on that path; structures builder and T5: NOT GOOD). Stays 🟢.
- **(Nov 14 10:30)** **🟢 → ✅ (10:15 rule):** the pilot on foot walked on both planets and through the Dominion interior (T7, real keys). Dev-only quirk (polish): `TD.Stage DominionInterior` spawns the pilot behind the ship (T5; the fix for the staged start is in batch AE).
- **(Nov 14 12:30)** **Stays ✅** (the pilot on foot was played good). **New open sub-point, Ryan's focus (#164): the pilot animation remade** (walk, run and blade combat, properly rigged), tracked under ART-08.

#### ENG-22 · Energy blade combat (CORE, used on planets) · `✅`
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
- **(Nov 14 06:00)** **Played:** real blade input against Grunts, Troopers, Drones and a Heavy on Verdance (T7, batch O tree). Not verified: the Heavy's lunge hint; block and deflect by a scripted player are weak, so deflect timing still needs a person. Stays 🟢.
- **(Nov 14 06:30)** **State:** **MAJOR open from T7's interior play (T5; carried on ENG-30 and ENG-23):** the Heavy's guard never breaks to quick swings and no lunge hint shows. The blade itself worked through the hangar, the deflect corridor and the arena. Stays 🟢 on the blade's own behaviour
- **(Nov 14 09:30)** **Played (T7, real keys):** the Heavy's guard and lunge hint GOOD; the Warden beaten with the real blade and block on take 5 (GOOD). The perfect deflect is not separately judged. Stays 🟢 until the deflect window is judged.
- **(Nov 14 10:30)** **🟢 → ✅ (10:15 rule):** the blade combo and block GOOD in play (T7: the Warden beaten with the real blade and block; the Verdance wreck-core fight cleared on take 1 with blocking on main AB; the deflect corridor GOOD). Polish: the perfect-deflect window judged by feel (T5, AAA 6).
- **(Nov 14 12:30)** **Stays ✅** (the combat played good). **New open sub-point, Ryan's focus (#163): the energy blade remake** (a proper beam with a hot core, coloured glow, bloom and flicker, a Niagara swing trail, sparks on the arc, hit sparks, ignite and retract), tracked under ART-08 because it is the blade's look, not its combat. Note: #163 was closed automatically by PR #172's merge (the concept sheet, 46e285fc); the work is not done.
- **(Nov 14 13:30)** **Stays ✅** (combat played good). The new blade look is on main in batch BH (#163, ART-08); its deflect, perfect deflect and ignite effects are not yet proven in a sheet (blade builder).

#### ENG-23 · Crew enemies: AI & combat (CORE, planets) · `✅`
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
- **(Nov 14 03:00) 🟨 → 🐞** (issue #90, comment of 02:30, after batch O landed): on main 61404a79 the **Warden's slam
  contact is mistimed** (T5) and the **Warden's shield floats off the body** (a rig fix, animation builder). Both fixes
  are in progress and not on main; back to 🟨 when they are, 🟢 only after an agent has played the Cinder fight.
  On main and awaiting agent play (T7's end-to-end playtest of 61404a79; the animation builder's kill captures): crew
  deaths end on the ground (no. 1; 4b71cc02, ab2ab66b); four in-place ground deaths per crew type on its own skeleton
  (b9bbdb25), per-type attacks, corpses that lie 10 s then sink (494cbcd7), Warden deaths at 2.8 s (5e122dfd); the
  Heavy's frontal guard taught by a hint, a clang cue and chip-through (no. 3; 8e758175); crew readable at 30 m in the
  dark interior (no. 14; 0b36d0e9); the drone's thrusters (0d767e62). Still open: the reactor conduits carry no world
  markers and the Cinder conduit marker's bearing crosses lava (no. 4, no. 11; T5); the Warden ragdoll (T5, animation
  builder).
- **(Nov 14 04:00) 🐞 → 🟨:** both fixes are on main 6afa3816 (batch Q, 930 tests): the Warden slam's contact at
  1.30 s on the charge's end and the Grunt swings' contacts at 0.45 / 0.48 s (T5, 39810902, with `td.Crew.LogHits`),
  and the Warden shield rigid on the left forearm, checked across guard, walk, stagger, fire, summon, slam and death
  (animation builder, e2e74225; the old mesh kept as `SK_Warden_ShieldSpine`). No other defect is known on main, so by
  the rule above the item is 🟨, not 🐞. It is not 🟢: neither fix has been seen in a played run (T7's end-to-end
  play has not started), and the Warden death is being rebuilt on a new base (animation builder, on a branch, with the
  Summon pose). If the played run shows the slam or the shield still wrong, back to 🐞. In batch S's gate: the run's
  difficulty scales crew health, hits and fight size (231b9876); that commit has an Ace-only bug (an extra crew member
  spawning through a wall on Cinder), fixed on T5's branch, which must land in batch T.
- **(Nov 14 05:00)** **State:** the Ace-only wall-spawn fix (8e1a4e6d, with a spawn test for every preset) is on main 29a15f21. In batch U's gate: the Warden clips moved to `SKEL_Warden` (five had silently fallen back to template clips), the Warden death rebuilt, the Heavy bash raised. On branches: Grunt damage-time jitter and the Warden stagger trigger (T5); live captures of hit reactions, walk and stagger (animation builder). None of it seen in a played run: stays 🟨.
- **(Nov 14 06:00)** **State (🟨):** on main 50c4af7a (batch U): the Trooper hit react interrupts its burst, the Warden stagger trigger, the Warden clips on `SKEL_Warden`, the rebuilt Warden death, the raised Heavy bash. T7 fought Grunts, Troopers, Drones and a Heavy on Verdance (batch O tree, before these fixes); **the Warden has still not been reached in a played run** (the Cinder play stopped at the conduits). Findings (T5): the first on-foot fight was too hard for a scripted player; two crew stood behind the terminal wall for over two minutes after the transfer (encounter stragglers fix on eng/t5). **Decided by Ryan's Lead:** the wreck-core fight is trimmed (ambush Grunt + Grunt, site Grunt + Trooper, the Heavy moves to the relay transfer, mercy from take 2); T5, batch V.
- **(Nov 14 06:30)** **State (🟨):** **MAJOR still open (T5), played by T7 on main 50c4af7a:** the Heavy's guard never breaks to quick swings and no lunge hint shows; the interior arena took 17-18 takes. **NEW MAJOR on the Warden fight** (a death skips the Warden) is carried on ENG-30. On main: encounter stragglers (94b56d11); the wreck-core trim (6cc4f254, b3e2f5d8: ambush Grunt + Grunt, site Grunt + Trooper, mercy from take 2), tests only. **Decided by Ryan's Lead:** crew bodies stay 30 s. The Cinder Warden is still not reached in a played run.
- **(Nov 14 09:30)** **🟨 → 🟢:** played by T7 with real keys on batch Y + T5's arena fix: the Heavy's guard and lunge hint GOOD (the MAJOR is closed in play), the director's mercy GOOD, the arena cleared by take 5, the Warden fight GOOD. Not ✅: the Cinder crew and Cinder Warden are unplayed.
- **(Nov 14 10:30)** **🟢 → ✅ (10:15 rule):** crew fights GOOD on both planets and in the interior (T7: interior corridor, arena with mercy, the Heavy's guard and lunge hint, the Warden fight; the Verdance wreck-core fight on take 1; the Cinder station chain). The future-pilot defenders are ENG-30's.

#### ENG-24 · Objective framework · `✅`
`ATDObjective` (Id, steps, waypoint, reward ability and ship) and interactables (tap or hold;
interrupted by damage). Fires `OnObjectiveUpdated` and calls `CompleteObjective`, which brings up
Nick's reward screen. Encounter volumes spawn crew; clearing a room opens its doors.
- **(Nov 14 10:30)** **🟢 → ✅ (10:15 rule):** objectives, interactables, rewards and room doors played through Verdance, Cinder and the Dominion (T7, T2: the reward screen from a natural award). The MAJOR "no on-foot marker on the HEAD TO legs" is fixed on main AB (9b5d9eae; the marker seen on T7's tree).

#### ENG-25 · Verdance: gameplay layout & objectives (CORE) · `✅`
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
- **(Nov 14 04:00) State:** on main and awaiting agent play: the relay's first wave waits for `C3_Moss_Company`, the
  scene's end or 60 s (9f964b30, main 6afa3816; #90 no. 7); T5's Dress pass (49 story props, the kit `BP_Wreck` at the
  scar's end, landmarks; 2767300b, main ddc50a2f); relay mast r2 (main 4e894365); the research outpost's floor, which
  showed terrain, lifted 3 cm (f19802f0, main ddc50a2f; walk test pending, T5).
- **(Nov 14 05:00)** **State:** T7 reached Verdance on foot (first view, dressing visible) on the batch O tree, before the Dress pass; the objectives were not replayed. The canopy at about 40% coverage is in batch U's gate; the wreck scar is on the environment branch.
- **(Nov 14 06:00)** **Played (T7, agent-driven with real keys, batch O tree, started by a stage command):** the whole chain worked: research site, clear it, recover the signal-analysis device, the relay, connect the device, protect the data transfer (resumed from its checkpoint after a retake), back to the ship, board, take off. **Stays 🟨:** the wreck-core fight took 5-7 takes and the relay 2-3 (trim decided by the Lead, T5, batch V); three MINORs open (crew behind the terminal wall, the late Cloak announcement, no parked ship on a staged arrival); the play was on a tree behind main by batches P to U; the survey station and camp canopy on main 50c4af7a are not seen in the running game; the independent review is open.
- **(Nov 14 06:30)** **State (🟨):** on main, tests only: Cloak at the transfer, encounter stragglers, the withdrawal objective text (94b56d11); the wreck-core trim and Verdance story lines (6cc4f254, b3e2f5d8); a fuller camp canopy (38d032e2). The structures builder walked the Verdance survey station and saw the camp canopy in the running game. MINOR open (T5): the flown ship missing from the staged pad. Verdance has not been replayed on foot on main.
- **(Nov 14 09:30)** **Not replayed on main this morning:** the wreck-core trim, the stragglers fix and Cloak at the transfer are tests only. T1 and T4: planet arrival and the Verdance route GOOD. Stays 🟨 (T5 plays Verdance on main next).
- **(Nov 14 10:30)** **Stays 🟨:** the wreck-core fight GOOD on main AB (T7 10:32: cleared on take 1 with blocking); **the relay transfer to the Cloak unlock NOT PLAYED on main** (T7 playing now).
- **(Nov 14 11:30)** **🟨 → ✅:** Verdance played through by T7 on main 9155a6dc (real keys, with blocking): the wreck-core fight on take 1, the device connected, the data transfer 0 to 90% with its fights and no death, Cloak presented in the same frame the transfer completes (it used to arrive four minutes late), back to the ship, take-off, "Obj_Relay complete (5 steps)" (#116). The transponder flicker seen live is a HUD beat and moved to ENG-36 (#103, T2).

#### ENG-26 · Cinder: gameplay layout, hazards & objectives (CORE) · `🐞`
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
- **(Nov 14 03:00) State:** both objectives in the reduced form were played by an agent on main 6fe3e90f; the mining
  camp props and the Null derrick landmark are on main 61404a79 (builder captures only). Not started and queued on
  `CLAIMS.md` this hour (T5, with T2 for the marker): the fixed order (step 3), the pending approach marker (step 5)
  and the ID door (step 6). The Warden fight has open bugs (ENG-23).
- **(Nov 14 04:00) State:** in batch S's gate, not on main (T5, b4d8ed05): the fixed order (already tested), the ID
  door that greets the pilot once, and `IsFieldAnchorDisabled` (saved) for the pending approach marker; T2's on-foot
  line "APPROACH PREPARED · PASSAGE STILL BLOCKED" (e1cf2fbb) is on eng/t2. On main ddc50a2f and awaiting agent play:
  the Dress pass (50 story props, landmarks, the dig cabin) and the outpost lifted 3 cm; the Cinder magma retune (main
  4e894365).
- **(Nov 14 05:00)** **State:** on main 4eb57848 (batch S): the fixed order, the ID door's one greeting and the saved pending state (b4d8ed05); on main 29a15f21 (batch T): T2's pending approach marker and line (e1cf2fbb; wording a placeholder for Nicholas). Tested, not played by an agent. Cinder from the air is on the environment branch.
- **(Nov 14 06:00)** **🟨 → 🐞 (MAJOR, T7's agent play on the batch O tree, #90): the gate conduit marker leads straight through a lethal magma channel; 21 deaths in 4 minutes.** The earlier note (#90 no. 11, "the conduit marker's bearing crosses lava") is now a played failure. Owners: T5 (route the marker by waypoints; respawn past the channel), the environment builder (magma that reads as lethal), the effects builder (`NS_TD_Magma_Heat`, built on eng/vfx), the structures builder (a bridge); batches V and W. The play stopped at the conduits: the three areas, the Warden, the cache and the whole field-anchor objective are unplayed. Back to 🟨 when T5's route and respawn are on main; 🟢 needs both objectives replayed to the end. The walkable Cinder survey station is on main 50c4af7a, not seen in the running game.
- **(Nov 14 06:30)** **🐞 → 🟨:** T5's fix for the magma MAJOR is on main 6cc4f254 (batch W, b3e2f5d8), which was the condition the 06:00 note set: the conduit markers run over the new bridge (`SM_Kit_MagmaBridge`, walk-tested by the structures builder), an edge warning and a tip, a far-bank respawn after two magma deaths, a route test over 11 legs. **Tests only: nobody has played Cinder on this tree, so the fix is unproven in play.** Lead decision: the geyser row moved about 4 m from the bridge exit. Not on main: the magma surface and glow (eng/env, waiting on the s13 sheet); the magma heat effect is built but not seen. Still unplayed: the three areas, the Warden, the cache, the whole field-anchor objective. 🟢 needs both objectives replayed to the end.
- **(Nov 14 09:30)** **On main since 06:30:** the magma look and heat effect (seen on foot at 5 m and 20 m), the geyser row moved clear of the bridge (walked by the structures builder: bridge exit GOOD, no throw), the anchor charge and collapse pieces in use (**the charge mesh stretched: NOT GOOD**; collapse NOT PLAYED). **Cinder past the conduits is NOT PLAYED by any player agent.** Stays 🟨 (T5 plays Cinder; structures builder fixes the charge).
- **(Nov 14 10:30)** **Stays 🟨:** the station chain GOOD and the magma route GOOD (T7, 0 magma deaths); **the anchor charges MAJOR** (could not be planted on AB; markers and prompt on main in batch AE 9155a6dc, unplayed), **the collapse not played to the exit**, the gate conduits (markers and art in AE) and the cache unplayed (T5, T7).
- **(Nov 14 11:30)** **Stays 🟨.** The anchor played good by T7 on main 9155a6dc (three charges planted, the collapse run completed; the anchor-charges MAJOR is closed). Both gate conduits seen with the new art and markers by the structures builder (11:10); the charge is a small pack at scale 1 on the tower face. A CUT at the gate conduits now comes back on the far bank, not at the pad (3445cc64, AK), after T5's scripted player died 13 times re-walking the route. **To close: T7's hand play from the gate: the guards, both conduits broken (the stump seen), the areas, the Warden and the crystal cache** (#117).
- **(Nov 14 12:30)** **🟨 → ✅:** played good by T7 (agent, real keys with blocking) on main adc75bc5, from the far-bank checkpoint with nothing skipped inside: gate conduits 2 / 2 with the blade, yard, barracks, dig, the Warden on take 1, the crystals, EMP Pulse presented, on to the field anchor in about 6 minutes; takes per encounter: gate guards 2, yard 2, barracks 2, dig 3, Warden 1 (5 deaths, down from 24 at 11:50); the mercy logs on every retake (#117 closed 12:24). The fixes behind it: crew never spawn within 10 m of the pilot with a 2 s grace, mercy on every course fight, a lip on the river banks (24317963), the gate guards stay down after the gate and the barracks Heavy goes first under mercy (2f523bf3). The anchor was already good. Leftover on ART-18 (#119): the anchor collapse froze on a fast escape (found by Nick's side); T5's fix 5158b170 is on main AU, not seen.
- **(Nov 14 18:30)** **✅ → 🐞 (BLOCKER on main 48a6fd97, #194):** on Cinder the pilot walks about 19 m under the course (Nick's session 39: the player near Z -1908 while the course and the conduits stay near Z 0), so the melee gate route cannot be played. Cause: the relief sits low there and c9edb62a turned the flat slab's collision off; it went to main before its route walk (the Lead's mistake). **To close:** T5's slab collision back wherever the relief is lower than the course, T4's relief heights, then the gate, yard, Warden and anchor route walked again on main (T7, Nick's session 39 recheck). Packaging held at 96a66f7c.

#### ENG-27 · Balance & tuning · `🟨`
Play the full run at least twice. Tune enemy counts, HP and cooldowns toward 20–30 minutes. Add a
"Pilot / Ace" difficulty setting that scales enemy damage and accuracy.

- **(Nov 12) Pressure targets on Standard, to be confirmed by a human:** a squad ship goes down in about 1 run
  in 5 in waves 1-2, about 1 in 2 in waves 3-4, most runs in wave 5 and the assault, with a squad wipe in about
  1 run in 5 there. Measured in the headless balance run (chance of a ship down and hull lost per minute for
  the active ship) before any tuning.
- **(Nov 14 10:30)** **🟢 → ✅ (10:15 rule):** every played fight is finishable at the tuned numbers (waves 1-5, T4; arena by take 5 and the wreck core on take 1, T7; rock health in AD). Tuning leftovers (polish): a human run on each difficulty (AAA 5); the escape flight is easy (cleared with 25 s left, T4).
- **(Nov 14 17:30)** **✅ → 🟨 (Ryan, #201):** enemies hard but not overpowered, winnable for a normal player, retuned once shadow mode, wingman powers and aim assist land (T4, T7). HeavyHit now at 60% of max hull or a returned bolt (fb4592e5, decided by Ryan's Lead).

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
- **(Nov 14 03:00) State:** the Shipping zip of main eb1bf334 is **981,016,363 bytes** (19 MB under the limit; private
  prerelease `playable-main-eb1bf334`); batch O's package (61404a79) is not built yet. On main 61404a79: T7's warm-up
  subsystem, async music loading and path cache, and the headless combat benchmark (`TriadDrift.Perf.CombatWave5`).
  Not done: the measurement runs in the packaged build (#89), so no frame-rate claim is verified; the import builder's
  never-cook rules and Oodle RDO are unproven until a trial cook. **Held for Ryan:** dropping the DirectX 11 shader
  set (about 11 MB; the game would then need a DirectX 12 card).
- **(Nov 14 04:00) State:** the eb1bf334 zip is still the newest finished package. A trial package of main ddc50a2f
  has been cooking since 03:32; the never-cook rules (about 120 MB) and Oodle RDO are unproven until it is finished and
  looked at. `BP_Wreck` stays in the cook (Lead: T5 places it on Verdance). T2's projectile pool fix and combat
  warm-up list are on main 6afa3816. T7 (#89): audio buffers 1 against 3 measured, inconclusive, not committed; the
  warm-up off / on and natural first-ten-minutes runs are in progress. Still held for Ryan: the DirectX 11 shader set.
- **(Nov 14 05:00)** **🟨 → 🐞: the Shipping game target does not compile on main 4eb57848 (batch S) or 29a15f21 (batch T).** A squad test (`EngageInsideARockPocketWithoutHittingIt`) sat after the `#endif` of its `WITH_DEV_AUTOMATION_TESTS` guard; found by the packaging agent; fixed by dea28a8b in batch U's gate. Back to 🟨 when batch U is on main and its Shipping target has compiled. **Decided by Ryan's Lead:** no package from batches S or T; from now on the packaging agent compiles the Shipping target of each batch branch before it is pushed. **Package:** the newest published Shipping zip is main ddc50a2f (batch R): **872,722,389 bytes** (127 MB under the limit), a clean cook with the never-cook rules (134 unreferenced packages; the rules and the `td-package` change are on main 29a15f21), launch check to first combat only; prerelease `playable-main-ddc50a2f`; Oodle RDO is still an unproven trial. **Performance (#89):** with the warm-up on no combat frame is over 100 ms (assault worst frame 133 → 51 ms) at +650 MB RAM, over the 400 MB target (per-map lists in progress, T7); tutorial combat had 9 frames over 33 ms in 119 s on a loaded machine (cause unconfirmed); the audio buffer test is inconclusive; first asteroid breaks 12-19 ms → about 1.2 ms; the first pickup spawn still 25 ms (T2). **Held for Ryan:** the DirectX 11 shader set; a precompiled pipeline-state cache.
- **(Nov 14 06:00)** **🐞 → 🟨: main 50c4af7a (batch U) compiles for Shipping again** (dea28a8b), so main is packageable; this was the condition the 05:00 note set. The Shipping cook of 50c4af7a is running and **not published**; the newest published package is still `playable-main-ddc50a2f` (872,722,389 bytes; launch check to first combat only). Not 🟢: no package of the current main, no package played end to end, no frame-rate claim verified in a Shipping build. Build-size results on main (981.0 → 872.7 MB from the never-cook rules; 12 music tracks in the package). Gate 959 / 959 headless tests.
- **(Nov 14 06:30)** **The package of batch U exists (stays 🟨):** prerelease `playable-main-50c4af7a`, `Builds\for-ryan\TriadDrift_50c4af7a_Shipping.zip`, 876,082,811 bytes (123.9 MB under 1 GB), cook 2,595 packages with 0 errors and 0 warnings, six maps, IoStore 8/8. Shipping launch check: menu, New Game, intro, hold-to-skip, wave 1 for 80 s, no crash. **Not verified in the package:** audio, flight input (window-message injection does not reach gameplay controls; the Lead declined OS-level input injection on Ryan's desktop), anything past wave 1: it needs a human. No package was made from batch V (Lead: W supersedes it). Main 6cc4f254 (batch W, gate 986 / 986) compiles for Shipping and its cook is running; `playable-main-6cc4f254` does not exist yet. Not 🟢: no package played end to end, no frame-rate claim verified in a Shipping build.
- **(Nov 14 09:30)** **Newest package:** `playable-main-8b4eabae` (batch Y): 877,326,337 bytes, cook 2,632 packages with 0 errors and 0 warnings, Shipping launch check to wave 1. A cook of main 80a2ee4d (batch Z) is running. Still not verified in a package: audio, flight input, anything past wave 1; no 60 fps claim. Stays 🟨.
- **(Nov 14 10:30)** **Stays 🟨:** newest package `playable-main-80a2ee4d` (batch Z); the packaging agent now cooks the newest main hourly on the hour; **no package played past wave 1, no audio or flight input verified in one, no 60 fps claim** (Lead, packaging agent; Ryan).
- **(Nov 14 11:30)** **Stays 🟨.** #153: T6's clean-route frame watch (enemies cleared): Verdance approach 1 frame over 33 ms (mean 7.4 ms), tutorial escape 1 (one 123 ms frame, not repeatable), Cinder and Dominion approaches 0, escape corridor 1: routes GOOD; the death / CUT hitch in the zone is not measured (T6's next turn). The packaging agent's next hourly cook is at 12:00; no package played past wave 1 (#152).
- **(Nov 14 12:30)** **Stays 🟨.** The Shipping package of main adc75bc5 is built (0 cook warnings); its launch check is pending (packaging agent; next hourly cook 13:00). No package has been played past wave 1 with audio and input (#152); the death / CUT hitch is not measured (T6, #153).
- **(Nov 14 13:30)** **Stays 🟨.** #153 performance closed as measured GOOD (12:55 EST, T6: clean flight routes at most one frame over 33 ms; losing the flown ship with an auto switch 2,080 frames, worst 10.4 ms; a full wipe to CUT and retake one 277 ms frame that is a development-only stack dump, compiled out of Shipping and now behind `td.Debug.TeleportStack`, 58036bc9). Packages: Shipping TriadDrift_adc75bc5 launch-checked and played 6.5 min by T7 (menu, difficulty, intro skip, tutorial to WITHDRAW, switching, new HUD; no crash); Shipping TriadDrift_4bbdd282 (main through BD, 0.879 GB, 0 cook warnings, prerelease `playable-main-4bbdd282`) launch-checked (no letterbox left after the intro, pause and Settings). The later acts are covered in development builds. Packages every 30 minutes from 13:25 (Ryan). **To close:** the final package played start to end with audio and input (#152, T7); the 4.9 s galaxy load frame at climb-out (T6); the Settings Subtitle size row cut by the Back bar (polish).
- **(Nov 14 15:30)** **Stays 🟨.** Shipping TriadDrift_9f20c5be launch-checked; T7 continued from a prepared Verdance save in it (blade, carry and the on-foot card GOOD in Shipping; no fight reached in Shipping). T6: galaxy reload 1.4 s, climb-out longest frame 1.7 s (was 4.9 s); the re-entry hitching fix (warm-up under the white cover) compiling; the Verdance take-off frame rate with the environment builder (scatter cut). The two-slot compile script is on main (0483a34b). Next cook 15:30 from 8d62d14a. **To close:** a package played start to end with audio and input (T7, #152), 60 fps at 1080p (T6).
- **(Nov 14 16:30)** **Stays 🟨.** TriadDrift_8d62d14a_Shipping.zip launch-checked at 0.90 GB and played by T7 with a real mouse and keys (menu, Cloak, auto-switch GOOD); the menu was black for 8 s after launch in that package. On main 685d8bea: cached planet bakes and the re-entry hitching fix (galaxy reload 0.27 s, cover 0.46 s, longest climb-out frame 0.35 s; T6), Verdance take-off slow frames 38 -> 1, two compile slots with the compiler run directly (a9c635de). Next cook from 99d489e7 or later with the size check (new always-cooked Fab folders, about +21 MB estimated) and boot-to-menu timed. **To close:** a package played start to end with audio and input (T7, #152).
- **(Nov 14 17:30)** **Stays 🟨.** #89 at 17:20 (T6, measured): wave 5 with the field on GOOD (1 of 12,170 frames over 16.6 ms), field build 165-220 ms, galaxy return 0.27-1.0 s, climb-out longest frame 588 ms, the first 10 s after re-entry 2.1% slow frames, death and retake GOOD in Shipping; **NOT GOOD:** Verdance take-off and climb below 800 m (158 of 5,071 frames over 16.6 ms, render-thread bound; environment builder); boot to menu 30.9 s black on 685d8bea (the splash 8ea0f5f1 and no menu warm-up 235674b1 are on main, not yet measured in a package); not measured: Cinder take-off, on-foot frame rate on the filled planets (#194). The galaxy-only keep is on main a7e35ba2. **To close:** a package played start to end (T7, #152) and the open #89 rows.
- **(Nov 14 18:30)** **Stays 🟨.** Boot in the a946ca93 package: the splash at 1.3 s, the menu at 13.4 s (was 30.9 s), about 10 s of black game window left (T6 adding a loading card). Packaging held at 96a66f7c, the last good build, for the Cinder blocker. Fab texture cook caps on main (03f78396). Compiling: T6's asteroid-split fix (0ed9cd18, #139's MAJOR on the space field's state reference, stress-tested in a game). **To close:** a package played start to end (T7, #152).

#### ENG-32 · On-foot third-person camera & the Settings page (CORE, added Nov 12 by Ryan) · `✅`
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
- **(Nov 14 09:30)** **Agent verdict (T3):** subtitles on GOOD; **subtitles off NOT PLAYED**. Stays 🟢.
- **(Nov 14 10:30)** **Stays 🟢:** **subtitles off NOT PLAYED** (T3).
- **(Nov 14 12:30)** **Stays 🟢.** Subtitles off checked in the opening by T3: lines aired (log), no subtitle drawn, the pilot name card still shows: GOOD (#94). **To close: the same check in space combat and on foot** (T3).
- **(Nov 14 13:30)** **Stays 🟢.** Third person on foot is the default (settings v2 migrates old saves; the chase camera blocked by scatter trunks; 4dc9e155, AY); T7's fight confirmed the camera out of the trees. **To close:** subtitles off in combat and on foot (T3, #94).
- **(Nov 14 14:30)** **🟢 → ✅.** #94 closed by the Lead as played GOOD: third person on foot by default, first person selectable, the Settings page, and subtitles off honoured in combat and on foot. Polish only from here (AAA list).

#### ENG-33 · Cinematic intro (CORE, added Nov 12 by Ryan) · `✅`
**(Nov 14 03:00) 🐞 → 🟨:** issue #49 (pilot names cut off) is fixed on main: the names (3c76874c / 9fe5d373, main
6fe3e90f) and the trait line that repeated the role word (e234e950, main 61404a79). T3 captured the cards live at
1280x720, 1920x1080 and 2560x1080 with the names and both lines fully on screen (agent-driven; no human has looked).
The issue stays open for Nicholas to close; the card's look is his. Not 🟢: the re-shoot below, the `O_*` lines, the
score, the five legacy `Intro_N` captions Nick's side found (N-088, T3; not verified fixed this hour) and the longer
silences after the shortened lines are all open.
**(Nov 14 04:00), decided by Ryan's Lead:** the opening's words are rebuilt from the 15 approved `O_*` first-contact
lines, shown as subtitles over the existing intro shots behind `td.Intro.Lines` (0 = the old captions). T3's build is
on eng/t3 (0622bf0f, 77de9085; Intro / Story / E2E 33 of 33), approved by the Lead, not on main and not seen live. Until
it lands, main still shows the five legacy narrator captions. The voice for the lines is Nicholas's.
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
- **(Nov 14 05:00)** **State:** T3's rebuild is in batch U's gate (lead/int-u dea28a8b), not on main: the 15 approved `O_*` lines (`td.Intro.Lines`), the departure-radio guard, a black hold on the intro's first frames, and the HOLD TO SKIP hint shown by itself for 4 s once skipping opens. T3 played the opening's 11 lines at two resolutions on its branch; T7 played the hold-to-skip on the batch O tree. The voice for the lines is Nicholas's.
- **(Nov 14 06:00)** **State (🟨):** on main 50c4af7a (batch U): the opening speaks the 15 approved `O_*` lines (`td.Intro.Lines`, 0 = the old captions), the black hold over the galaxy's first frames, HOLD TO SKIP shown by itself for 4 s, the departure-radio guard. Seen live by T3 (agent-driven, 50c4af7a): the black hold and HOLD TO SKIP. On lead/int-v: the opening's lines obey the subtitles setting (15f04846). Not 🟢: the 49 s re-shoot with the final ships, the voices (Nicholas) and the independent review are open.
- **(Nov 14 06:30)** **State (🟨):** the opening's lines obey the subtitles setting on main 94b56d11 (15f04846), tests only. The 49 s re-shoot, the voices (Nicholas) and the independent review are still open.
- **(Nov 14 09:30)** **🟨 → 🟢:** Intro GOOD (T3, agent-played on main). Not ✅: the 49 s re-shoot with the final ships is still queued (T3) and the voices are Nicholas's.
- **(Nov 14 10:30)** **🟢 → ✅ (10:15 rule):** the intro GOOD, the black hold and HOLD TO SKIP GOOD (T3, T7), the hand-off to the tutorial GOOD. Nicholas's human findings on the intro (#90, Manual Feedback Batch 1) are polish with owners: 7 white spot and 8 the opening shot (environment builder + T3), 9 game-ready pilot meshes (Lead, needs a character builder), 10 blurry ships in the flybys (T3 + effects builder), 11 the capital-ship sequence's sounds (Nick's Audio; T3 posts the cue list); the 49 s re-shoot with the final ships (T3).
- **(Nov 14 11:30)** Nicholas's Batch 1 polish findings closed this hour on agent sight: B1-1 and B1-7 (the landing beacon hidden on the menu and in the intro; c0ea8682, #141, #147), B1-3 (menu fly-by ships clear the frame, a2436be7, #143), B1-4 (fly-by plumes and trails, cf28153e, #144), B1-10 (fly-by motion blur 0.12, c2340d05, #150). Open: B1-5, B1-8 (environment builder), B1-9 (pilot meshes), B1-2, B1-6, B1-11 (Nick's side by ownership).

#### ENG-34 · Planet worlds: large procedural terrain (CORE, added Nov 13 by Ryan) · `✅`
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
- **(Nov 14 06:00)** **State (🟨):** the Verdance canopy at about 40% of the vegetated lowland (1,475 clumps, coverage test 30-50%) is on main 50c4af7a, seen on the builder's sheets only. On eng/env: Cinder basalt variation, crystal fields 2-4.5x, the wreck scar as burnt ground. New from T7's play: Cinder's magma does not read as lethal (environment builder, with the effects builder).
- **(Nov 14 06:30)** **State (🟨):** on eng/env, waiting on its s13 sheet, not on main: Cinder crystal clusters, the terrain material, the scar, the magma surface and glow. A fuller Verdance camp canopy is on main 6cc4f254.
- **(Nov 14 10:30)** **🟨 → ✅ (10:15 rule):** both large worlds flown into and across, landed on by hand and walked (T1: natural Verdance entry and glide, hand-flown landing and take-off on both planets; T7 on foot). Polish: Verdance ground detail (environment builder, parked: no visible change).
- **(Nov 14 16:30)** **Stays ✅; Ryan's #194 (16:15) is tracked under ENG-39:** terrain relief and a mountain ring on both planets (T4, `World/TDTerrainField`, `TDPlanetTerrain`).

#### ENG-35 · Engineering fallback sound set (CORE, added Nov 13 by Ryan's Lead under Ryan's instruction) · `✅`
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
- **(Nov 14 03:00) State:** the 14 gap sounds (b8e561cd) are on main eb1bf334. T1's native **music fallback**
  (`UTDMusicFallbackSubsystem`, eb6e3275, R-047) is on main 61404a79, technically verified and heard by no one: credits
  (Cathedral Pulse (1) then Ethereal Pulse Remix after the stinger), Cinder World / Cinder Fall, the assault and quiet
  flight are live; the core chamber and the reactor fight are wired but wait for T5's interior events (not on main);
  **the escape is empty** because the Ritual track is not imported on main. It stands down on any exclusive claim by
  Nick's plugin and is off with `td.Music.Fallback 0`. On the effects branch, not main: the CloudPunch sound and a wind
  layer for the descent.
- **(Nov 14 04:00) State:** T5's interior moments (`ATDPlanetCourse::OnInteriorMoment`: core chamber entered, reactor
  fight started and ended, escape started; 9f964b30) are on main 6afa3816, but T1's forwarding of them to the music
  bridge (f88a0bfb) is on eng/t1, so the core-chamber and reactor-fight music is still silent on main. On the effects
  branch: the CloudPunch and GlideWind sounds. The escape is still empty.
- **(Nov 14 05:00)** **State:** T1's forwarding of the interior moments to the music bridge and the glide wind slot are in batch U's gate; the CloudPunch and GlideWind sounds are on the effects branch. Core chamber and reactor fight are therefore still silent on main; the escape slot is still empty (Ritual not imported).
- **(Nov 14 06:00)** **State (🟨):** T1's forwarding of T5's interior moments to the music bridge is on main 50c4af7a, so the core-chamber and reactor-fight fallback placements can play; heard by no one. CloudPunch (3 takes) and the GlideWind loop are rendered and levelled on eng/vfx, not in Unreal on main. The escape slot is still empty.
- **(Nov 14 06:30)** **State (🟨):** CloudPunch (3 takes, wired to `DA_Travel_FX.CloudPunch.Sound`) and the glide wind sound are accepted and in batch X (eng/vfx 3da11150), not on main. Audio is **not verified** in the Shipping package of 50c4af7a. The escape slot is still empty.
- **(Nov 14 10:30)** **Stays 🟨:** waves 3-5 and Verdance have their own fallback music on main AD (83cbddba; waves 3-5 seen in the log), **Verdance's not yet confirmed in play (T1)**; the escape is silent until Nick's Ritual track lands (Nick's Audio). Ear approval stays with Nicholas.
- **(Nov 14 13:30)** **🟨 → ✅** (#121 closed 13:28 EST): seen in play by log across T1's turns, the last with the audio engine on (main 98c96926 and later): menu music at boot (`MUS_Menu_EtherialPulse_01`), the Verdance score starting after the reveal and fading on take-off, the waves 3-5 track, the entry roar and the climb-out roar on a hand-flown take-off. Not engineering's: the Ritual escape track and Nicholas's ear approvals (#134, #138). Routed from that log to T6: one 4.9 s frame under the white cover on the galaxy load at climb-out (ENG-28).

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
- **(Nov 14 03:00) State:** on main and awaiting agent play: generator pips and the transponder flicker (cda0a770,
  main eb1bf334); on-foot counted-step pips (reactor conduits 0 / 4), the escape clock and EXIT marker (7fb67c3d, main
  f39a8372); enemy marker shapes per type with a heading tick (ede33e91, main 61404a79; a fallback under R-047, flagged
  for Nicholas). Not on main: the ENG-38 descent HUD (altimeter, hull temperature; T2). Not started: the on-foot HUD
  in the new style, the *Aftermath* lane display and navigation pulse, the Cinder pending marker.
- **(Nov 14 04:00) State:** the ENG-38 descent HUD (altimeter tape, HULL TEMP bar, ATMOSPHERE during the burn, glide
  and entry, ORBIT for 3 s after the climb-out; 46e371c1, test `FlightHUD.DescentAltimeterHeatAndCues`) is on main
  6afa3816, awaiting agent play. On eng/t2, not main: the on-foot HUD in the flight HUD's style (aa204b7a; tests pass,
  not seen live) and the Cinder pending line (e1cf2fbb). Not started: the *Aftermath* lane display and navigation
  pulse (T2, queued this hour).
- **(Nov 14 05:00)** **State:** on main 29a15f21 (batch T): the on-foot HUD restyle (aa204b7a) and the Cinder pending approach marker (e1cf2fbb), tested, not seen live. The enemy marker shapes were seen working in T7's play (batch O tree). On T2's branch: descent HUD contrast and the ORBIT fix, a marker placement fix, the *Aftermath* HUD lanes. Still 🟨: most story states are unplayed.
- **(Nov 14 06:00)** **State (🟨):** on main 50c4af7a (batch U): the descent HUD on its own dark plate with full-white text (never below 80% while the HUD is up), ORBIT surviving the travel, enemy markers just above the ship's outline at 1.4x. On lead/int-v: `GetNavigationPulse01` (the pulse envelope for the Dominion light row) and a wider descent plate. Still open: the *Aftermath* tactical lanes; live captures of the on-foot HUD and the story states on main.
- **(Nov 14 06:30)** **State (🟨):** on main 94b56d11 (batch V), tests only: `GetNavigationPulse01`, the wider descent plate, the bark lane drawing no text when subtitles are off. Seen in T7's play on main 50c4af7a: the assault score and the generator pips 0/4 to 4/4. Still open: the *Aftermath* tactical lanes; live captures of the on-foot HUD, the conduit count and the escape clock.
- **(Nov 14 09:30)** **Agent verdicts (T2):** flight HUD GOOD, enemy marker shapes GOOD, on-foot HUD GOOD, descent HUD GOOD (glide-in leg NOT PLAYED); **NOT PLAYED:** generator pips this morning, the docking plates and on-foot waypoint (on main 80a2ee4d, a62deca8), the Cinder waypoint, the *Aftermath* lanes (still open), the tactical display, settings with subtitles off. Stays 🟨.
- **(Nov 14 10:30)** **Stays 🟨:** flight, marker, on-foot and descent HUD GOOD (T2); **the docking plates, generator pips and escape clocks NOT PLAYED this morning** (T2); the *Aftermath* lanes open (T2).
- **(Nov 14 11:30)** **Stays 🟨.** The transponder flicker on the Verdance route moved here from ENG-25 (#103). **New sub-point (Ryan, Nov 14 morning; #160, board R-049): the flight HUD redesign, a richer spacecraft-shooter HUD.** Ryan asked to see each ship's health and power-ups and a small radar of nearby enemies and what is coming. Concept A is chosen (`docs/eng/hud-concept/HUD-CONCEPT.md`, `mockup.png`; Ryan's Lead under delegation, and Ryan looked at the mockup and said it looks good): a squad panel bottom-left (shield, hull, signature power, ability and status flags per ship), a radar bottom-right (forward up, 1.5 km, height stems, rim pins, the warp-in arc, a wave header), a boost arc and a pickup toast at the reticle; colours and faces from Nick's `FTDHUDStyle` (his reply on #160). The data model is on main (`FTDHUDModel` 4d8e8b39, `GetPendingGroups` 0e740f77, batch AL); T2 builds the squad panel, flags, boost arc and toast, T6 the radar; **nothing is drawn in game yet**. Also on main, not seen: #157 transponder label inside the safe frame (4d8e8b39, AL), #159 power hint names the ship's action (4076a658, AK); #155 the subtitle scale on the flight HUD's story lane (T2). Still to play: the docking plates, generator pips, escape clock, *Aftermath* lanes, transponder flicker (T2).
- **(Nov 14 12:30)** **Stays 🟨.** Played by T2 at 1280x720 (#103): the generator pips, escape clock and *Aftermath* lanes GOOD; #157 (the transponder name at the right edge) GOOD and closed; the tractor plate not seen; two small fixes with T2 (a count shown twice; the *Aftermath* panel closing on a phase change). **The new flight HUD (#160) is drawn on main:** squad panel (AP 99969023, AR c58285f5), status flags and ability states by shape and text (AR), the radar (T6, AP), the boost arc and pickup toast (AS 04dae969); T7 found a crash on the first HUD paint (the radar added an array's own element by reference), fixed in adc75bc5. First in-game read by T7 (wave 3, 720p; the Lead looked at both frames): the active card swaps at once on a switch, wingman cards show who is hurt, the radar shows a pack behind: reads well. T2's fixes from that read are on main AU ebe073ad (the HUD drew at 67% at 720p, now 85% with text at full size; ship name on the flown card only; wingman cards with bars only); the radar plate contrast and the wave named twice are with T6 and T2. Nick's Lead's notes on #160 apply (ship identity on inactive cards, ability states distinct at native 720p, room for the Large subtitle block). **To close: frames at 1080p and native 720p judged** (T2). Subtitles in flight: T2's lane fix (eb911f4b, AP) passed Nick's side's retest on c58285f5 (#154 closed by Nick's side); MINOR on #160: C4 Large's third line overlaps the radar (T2). Also left: the transponder VERIFIED flicker, the tractor plate (T2).
- **(Nov 14 13:30)** **Stays 🟨.** The new flight HUD (#160) played and judged GOOD and closed (12:52 EST: T2 in a real wave 3 at 1920x1080 and native 1280x720, T7 in wave 3 and in the Shipping package; squad panel, flown card, wingman cards, flags, instant swap, radar with the wave header, the minimal-HUD setting). On main since: T6's radar fix (BC: plate 0.88 with a light rim, 238 px at 720p; seen painting in wave 5), the squad panel and radar leaving with the combat HUD (BF), Minimal HUD as a saved setting (BG), the on-foot pilot card in the flight HUD's style (BH, read well in T7's fight), `TD.UI.Cost`. Left on #103: the radar header showing WAVE 1 / 5 after the waves (MINOR from Nick's side; T6 shows the generators and reactor instead; fix painted, in batch BI), the boost arc, pickup toast and three-line Large subtitle seen in a frame, the tractor plate, the VERIFIED flicker, the stray `Unlocked ETDAbilityId::Cloak` dev line, the CLOAKED flag (#102).
- **(Nov 14 15:30)** **Stays 🟨.** On main 8d62d14a: the radar header with the generators and the reactor state and phase (223fbf9e), the grace shimmer and HUD line on an auto-switch, the SWITCH SHIPS lesson completed only by the pilot's own switch (960cbee7, from Nick's playtest #140), the hint order on a new game (68680c9e); the radar missile glyph and descent fix and more tutorial fixes in batch BQ, not on main. **To close:** #103 (the follow marker, the director-shot HUD hide, the reactor header, the pilot card against the reticle: T2) and the #159 hint cards (T1, T2).
- **(Nov 14 16:30)** **Stays 🟨.** On main: the radar missile glyph (batch BQ); docking follow markers named DOCK GOOD (T2); flight feel with the new HUD GOOD (T1). Seen GOOD on T2's branch (next batch): the director shot hides the HUD, the reactor header. **To close (#103):** the low-hull card tint, the pause-menu hover, the lock glyph on a finished planet (T2), T2's branch on main; #159 the hint cards (T1, T2).
- **(Nov 14 17:30)** **Stays 🟨.** On main 0c986a7d: the follow marker named by the objective's verb, no OVERCHARGE 0.0, the under-25% hull tint on every card, the switch pilot card clear of the reticle (d44efa8f, 52c410e0); the planet line without a padlock (e70b3bba). **Ryan's HUD review (17:00): the squad cards overlap and sit off-centre, and the Q ring.** **To close (#103):** T2's measured layout pass (top of its list), the pause-menu hover, three-resolution frames for Nick's side; #159 (T1, T2).
- **(Nov 14 18:30)** **Stays 🟨.** T2's measured squad-row layout from Ryan's review is on main 7fd85523 (994d8965): played GOOD at 1080p in a wave (T7) and clean at 720p on a live switch (Nick's session 37). **To close (#103):** an edge marker and its distance text intrude into the UNDER FIRE tab (T2); the pause-menu hover; #159.

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
- **(Nov 14 03:00) State:** on main: round 9 systems, the break flash sized per class (63761937), enemy bolts readable
  by shape (0c5de65c). On the effects branch, not main: the asteroid break shatter, the ENG-38 exposure ramp, per-type
  enemy engine plumes. The reactor Exposed / Hit effects could not be judged (the side camera lands inside the hull);
  an above-trench camera is needed. Still not started: the approach-field shimmer and opening, shield sections
  dropping per generator, the tractor beam.
- **(Nov 14 04:00) State:** on main 6afa3816: the asteroid break reworked as a shatter (flash, ice shards, a quick
  pale dust puff), `M_TD_PP_DescentWhite` and the in-cloud layer v1 (`NS_TD_Entry_CloudWisps`). On the effects branch:
  cloud veil v2, per-type enemy engine plumes, the *Aftermath* light row, shield drop, approach-field shimmer and the
  tractor beam (specs written, not built; slots asked of T4). Builder captures only.
- **(Nov 14 05:00)** **State:** the descent haze is in batch U's gate. On the effects branch: cloud veil v2, per-type enemy plumes, region beacon and haze, shield drop, approach-field shimmer, tractor beam, the light row. The range flare for the region set pieces moves here as a Niagara effect (Lead's decision; the mesh flares were removed).
- **(Nov 14 06:00)** **State (🟨):** descent haze following the glide is on main 50c4af7a. Round 10 is on eng/vfx. **Accepted by the Lead from review sheets:** the shield drop and the Cinder cloud. **Not yet accepted:** the Verdance cloud v2, the per-type Null plumes, the approach field, the *Aftermath* light row, the tractor beam, the region beacon and haze. `NS_TD_Magma_Heat` (embers, spatter, shimmer) is built for the Cinder magma MAJOR. Nothing of round 10 is in the game on main.
- **(Nov 14 06:30)** **State (🟨):** **accepted by the Lead from review sheets and in batch X (eng/vfx 3da11150, not gated, not on main):** Verdance and Cinder cloud v2, the shield drop on `DA_Dominion_Visuals.ShieldCollapse`, the approach field on `DistortionFieldIdle` (FieldHex2), the light row, CloudPunch, `DA_ReactorCore_FX`, the glide wind sound. **Not accepted:** enemy plumes, region beacon, region haze, tractor beam. `NS_TD_Magma_Heat` is built but not seen. The slots for the light row and the tractor beam are on main 6cc4f254 (T4).
- **(Nov 14 09:30)** **On main since 06:30:** the magma heat effect (seen on foot at 5 m and 20 m), interior light states (seen). **Not accepted:** region beacon, plumes, tractor beam, haze, magma trim (effects builder). Stays 🟨.
- **(Nov 14 10:30)** **Stays 🟨:** plume beacons accepted and wired (4d0a21c8), enemy engine glows seen (T4); region haze and region beacon DROPPED (regions read without them, Lead); **the tractor beam accepted and in its slot, not seen in play** (T4). The Cloak shimmer is not accepted (an optional look: polish, effects builder); a trailer-quality breakup is AAA 10.
- **(Nov 14 13:30)** **Stays 🟨.** The effects builder's queue: the Cloak shell (#102), then the tractor beam, the mines (#105) and the breakup explosions (#111); T4 sees the tractor beam in docking (#110).
- **(Nov 14 15:30)** **Stays 🟨.** On main: the Cloak shell far more see-through (44d9fa1b), the beam fading at the bay mouth (45a7652d); the Lancer plume rebuilt at kk 0.7 (932570cc) is in batch BQ, not on main. **To close:** the tractor beam v2 and the mine effect wired and seen (effects builder; T4's cue fix and `TD.Dev.SpawnMine` are on main), the Lancer flame core (effects builder), the blade light washing enemy armour at arm's reach (blade builder, #128).
- **(Nov 14 16:30)** **Stays 🟨.** On main 685d8bea: the blade light no longer washes enemy armour (#128 fix, batch BU), the Lancer boost plume keeps its cyan (flame core capped, c31d5a1b), the turret lock telegraph twice as large (350f7faf, not yet seen in a frame). The tractor beam v2 and mine trial wiring was taken back out (eb5de6fa): the beam is still invisible in a real docking and `TD.Dev.SpawnMine` draws a placeholder disc. **To close:** both wired and seen (T4's fixes queued, effects builder; #105, #128).
- **(Nov 14 17:30)** **Stays 🟨.** On main 0c986a7d: T4's tractor beam at least 0.6 from the moment the tractor holds the ship and `TD.Dev.SpawnMine` wearing the mine's real FX (51f45b35); the Grunt melee wind-up (bbad3927, 9c672e36). **To close:** the beam v2 and the mine effect back in the data and seen in a real docking and a real wave (effects builder, T4; #105, #128).
- **(Nov 14 18:30)** **Stays 🟨.** Tractor beam v3 (f6cb2c84, aimed from the mouth at the held ship each frame, 42f823ab) wired and visible in a staged spawn on main f85203b1; the Grunt wind-up v3 (a thin ring under the Grunt). **To close:** the beam seen in a real docking, the mine effect seen in a real wave (effects builder, T4).

#### ENG-38 · Planetary descent: fly from space into a world, anywhere on the globe (CORE, added Nov 13 by Ryan) · `✅`
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
- **(Nov 14 02:10, decided by Ryan's Lead under Ryan's standing delegation; Ryan can overrule) ENG-38 ships as a
  smallest complete version**, because stage 2 had no one queued at 02:00 and the freeze is Nov 15 12:00:
  - **In it:** an 8-10 s forced glide from above the cloud deck down to 900 m (not the 15-20 s from 3,000 m above);
    fog by altitude; scatter ready before the reveal; an altimeter tape and a hull-temperature bar; an exposure push
    into the white cover; a wind layer; the take-off return point mirrored from the entry.
  - **Left for Ryan (nothing is cut silently; each is his to schedule or drop):** the full-length glide; map preload
    in the gravity well; the in-space atmosphere phase; scatter fade-in by distance; canopy droplets and heat haze; a
    full reverse glide on take-off; the sonic boom.
- **(Nov 14 03:00) State (🟨):** the stage 1 entry and climb-out work is on main (cdf4145e, 2d6ec326, df563569; not re-checked line by line this hour). In batch P's gate (lead/int-p eb5827d9), not on main: T1's
  glide-lite (616b7a8e: arrival 300 m over the cloud deck, a 9 s forced glide at 120 m/s to 900 m, steering 50%) and
  the mirrored take-off (1e7b1fcb; off with `td.Descent.Glide 0`). On branches or in worktrees: T4's terrain half, T2's
  descent HUD, the effects builder's exposure ramp, CloudPunch sound and wind layer. Measured on main: the first planet
  entry hitches about 330 ms from synchronous scatter mesh loads (T4's async loading in progress; over the 100 ms
  bar). Nothing of the glide has been played by an agent on main; only a person can judge the feel.
- **(Nov 14 04:00) State (🟨):** on main and not yet played by an agent: T1's glide-lite and mirrored take-off (main
  4e894365), T2's descent HUD, `M_TD_PP_DescentWhite` and the in-cloud layer v1 (main 6afa3816). **Decided by Ryan's
  Lead:** the cloud deck is fitted to the glide: it sits at the glide corridor + 300 m, clamped to 1,100-1,600 m above
  the pad, so the glide stays 9-10 s and the climb-out stays short. That fit, T4's terrain half (`GetCloudDeckZ`,
  `PrepareArrival` / `IsArrivalReady`), async scatter kind loading (the 330 ms first entry is still on main), the
  descent haze, glide wind and the exposure ramp hook are in the T1 + T4 combined commit on branches, for batch T;
  cloud veil v2 and the CloudPunch and GlideWind sounds are on the effects branch.
- **(Nov 14 05:00)** **State (🟨):** in batch U's gate, not on main: the glide placed over a corridor-fitted cloud deck on every entry path, a 300 m floor for uncovered arrivals, the boundary release during the glide, async scatter kind loading at 4 ms per frame, descent haze, the glide wind slot, the exposure ramp. Played: T7 flew the Verdance entry naturally on the batch O tree (white cover, 1.69 s map load), which is before the glide of batch P; the glide, the landing by hand and the take-off are unplayed. Found in that play: the entry hold did not last through *Aftermath* (ENG-42 🐞); T1's fix (a crawl at the atmosphere's edge, AWAITING ORDERS) is on a branch. Decided by Ryan's Lead: the entry commits after *Aftermath* if the player is still at Verdance's edge.
- **(Nov 14 06:00)** **State (🟨):** on main 50c4af7a (batch U), unplayed: arrival on the corridor-fitted cloud deck with the glide paced at 85 m/s (9-12 s), every covered planet arrival placed over the deck before the glide, a 300 m floor for uncovered arrivals, the arrival released from the zone until the glide ends, retake and switch end paths, async scatter kind loading, descent haze, the exposure ramp, the descent HUD plate and ORBIT. T7's Verdance take-off and climb to orbit were played on the batch O tree (before this). A natural entry on main and a landing by hand are still unplayed.
- **(Nov 14 06:30)** **State (🟨):** T1 verified the natural Verdance route and the glide live (agent-driven). On main 6cc4f254 (batch W, test only): the entry hold waits for the story line on air or queued next, capped at 6 s (43703035). On main 94b56d11: `Bark_Atmo_Entry`. In batch X: cloud v2 for both planets, CloudPunch, glide wind. A natural entry and landing by hand in one played run on main is still not reported.
- **(Nov 14 09:30)** **🟨 → 🟢:** descent GOOD (T1, agent-played on main), descent HUD GOOD (T2). Not ✅: the glide-in leg of the HUD NOT PLAYED, the glide wind not heard on main, the landing by hand is ENG-20's.
- **(Nov 14 10:30)** **🟢 → ✅ (10:15 rule):** the natural Verdance entry and glide GOOD, descent GOOD, landing and take-off hand-flown GOOD (T1), the descent HUD GOOD (T2). Polish: the glide wind heard (effects builder, T1) and the HUD's glide-in leg looked at (T2).
- **(Nov 14 11:30)** **✅ → 🐞 (open bug from Ryan's own play, #161):** flying the Lancer in wave 5, Ryan flew into Cinder and the ship pressed against the surface and stuck: no atmospheric entry, no burn, no turn-back. Expected: a planet is never a static wall; when entry is not allowed the atmosphere burns and pushes the ship back out with a HUD line. T4's half (planets are avoidance obstacles for wingmen, enemies and spawn points) is on main in batch AL (90a132aa); the player's bounce, turn-away and burn are with T1, no commit yet. Back to ✅ when T1's fix is played good.
- **(Nov 14 12:30)** **Stays 🐞** until #161 is closed. T1's fix (6483b56a, AN) played GOOD by T7 (closed Verdance in wave 3 from 4,209 m: turned away at about 1,066 m from the centre every pass, never parks) and by T1 (wave 5, both planets closed, three ships, cruise, boost and ram: turned away every time; an open planet still commits entry; wingmen pushed out; hyperdrive into a planet refused). Not yet: the notice, its wording by cause and the burn (83ea90cf, AT 74567456: holds 2.5 s per approach, FINISH THE FIGHT FIRST during a wave, heat from the band's edge, a flash as the ship skips off) not seen in a frame. **To close: a frame with the heat bar, hull glow and flash** (T1, #161), then T1 goes to #156.
- **(Nov 14 13:30)** **🐞 → ✅** (#161 closed 13:21 EST): played GOOD by T1 on main 98c96926 (batch BE; wave 5, Lancer at cruise, boost and ram at closed Cinder; the behaviour also played by T7 earlier; the Lead looked at the frames): the ship is turned away about 360 m from the atmosphere and skips off, never stops and never reaches the planet; the burn shows beside the ship with heat capped at 0.6 (47788d47); the HUD stays at full strength with PLANET CLOSED · FINISH THE FIGHT FIRST on its own plate (32e4d404); by 3.3 s the planet is out of view. The left-for-Ryan list above stays polish.

### Complete-game items (added Nov 13 evening, at Ryan's request)
Ryan: "make sure it has all the information in order to pretty much complete the entire storyline and the entire game".
Each item below closes a gap found by walking `storyline.md` beat by beat against this file, `dialogs.md`,
`animations.md`, `CLAIMS.md`, the board and `docs/story/`. Ids continue the sequence (ENG-39 was named in the
realignment without an item; it gets one here).

#### ENG-39 · Worlds that read as real places: planet play zones, the space field, pickups (CORE, Ryan Nov 13) · `🐞`
Spec: the realignment section "Worlds that read as real places" and `docs/eng/WORLD-DENSITY-PLAN.md`.
- **Planets (T4, environment builder, T5):** every atmosphere entry, from any side of the globe, routes into the one
  dressed play zone around the pad and objectives, with its own approach corridor and first view per side. Inside the zone:
  at least three ground materials, trees, rocks, buildings and points of interest in every direction; outside it, haze,
  the cloud deck, the curvature bend, ring terrain and the flight-zone turn-back hide the edge; keep-out areas on foot.
- **Space (T6, T4 reviews):** belts, clusters, debris fields, wrecks, buoy chains and convoy lanes across the flyable
  space, then a streaming ring around the ship, deterministic from a seed (`td.SpaceField`); keep-outs around the combat
  zone, gates, planets and the Dominion; combat lanes stay readable (T6, in progress).
- **Pickups:** new gameplay; the smallest version is proposed to Ryan on the board **before** it is built (v1 built (a094a440, 23a9f302, 09636062), on main 6fe3e90f).
- **Space composition (Ryan, Nov 14; T6, T4 reviews):** the flyable space is composed as seven themed regions (the Watchline, the Shepherd Belt, the Ice Drift, the Quiet Graveyard, the Survey Scar, the Shattered Moons, the Outer Dark) per `docs/eng/SPACE-DESIGN.md` (on eng/t6, not on main yet); the Shepherd Belt first, inside a rock-field performance budget, with staged large and special breaks; its set pieces (patrol hull, expedition hulls, survey rig, hollowed rock) come from the structures builder. **(Nov 14 03:00)** Four regions (Shepherd Belt, Watchline halo, Quiet Graveyard, Outer Dark) are built and green on eng/t6 fae01308 and measured cheaper than the old field (5.4-6.7 ms frames); the Ice Drift, Survey Scar and Shattered Moons are in progress (757941fc); none is on main. Space set pieces r1-r4 are on main; region set pieces r3 are in batch P's gate. Planet landmarks r1 (relay mast, Null derrick) and the story camps are on main 61404a79, seen in builder captures only. **(Nov 14 04:00)** All seven regions with 36 set pieces are on main ddc50a2f (batch R; T6's 19 region tests green on batch P), awaiting agent play; region set pieces r3 and r4 are on main; the route report (90e30ad6) is on eng/t6. The planet Dress pass is on main ddc50a2f (ENG-25, ENG-26).
- **Performance (Ryan, Nov 14; T7, issue #89):** no hitches and a steady 60 fps in the Shipping build in every phase, measured per phase with each teammate measuring their own area; targets and findings in #89 and ENG-28. Since the 00:30 crash, at most two rendering Unreal processes run machine-wide (R-046; three until 01:20). T7's baseline (Nov 14 02:00): averages 92-193 fps, hitches from first-use loads and map changes, 412 audio underruns in 15 min; a warm-up subsystem, async music and audio buffers at 3 are in progress. **(Nov 14 03:00)** The warm-up subsystem, async music loading and the path cache are on main 61404a79; T2's projectile pool and combat warm-up list are on eng/t2 5e1013aa with three tests failing. The wave-3 and Verdance-entry hitches of the baseline were traced to the stage cheats; the natural-play runs have not been made. New: first planet entry about 330 ms (T4), first asteroid break of each kind 12-19 ms (T6). **(Nov 14 04:00)** T2's projectile pool fix (6200aa26) and combat warm-up list, and the field's four break systems registered with the warm-up (96e9fdc6), are on main. T7 measured audio buffers 1 against 3: inconclusive, not committed; the warm-up off / on and natural first-ten-minutes runs are in progress. T4's async scatter loading is still on a branch.
- **Done when:** from five entry points per planet the arrival reads as a place in captures; a 60 s flight in any
  direction in space passes dressing; wave 5 stays inside the ENG-28 frame budget with the field on.
- Depends on ENG-34, ENG-38, ART-05, ART-06, ART-13.
- **(Nov 14 05:00)** **State:** decided by Ryan's Lead: the region set pieces are closed for tonight at r4 (moon shards back to r4, the failed flares removed; in batch U's gate), with the range flare moved to the effects builder. On T6's branch: regions moved closer to the routes with more pieces, the docking-cone keep-out. T4's spawn points that reject obstacles are on a branch.
- **(Nov 14 06:00)** **State (🟨):** on main 50c4af7a and not seen in the running game: the regions closer to their routes, the shard and hull checkerboard fix, the survey stations on both planets, the camp canopy r2, the Verdance canopy. T7 walked Verdance with batch O's dressing. On eng/t6 and eng/vfx: region beacons and haze (not yet accepted).
- **(Nov 14 06:30)** **State (🟨):** seen in the running game by the structures builder: the moon shard material (region 6 at 1 km), both survey station walks, the camp canopy, the three ships on landing gear. On main 6cc4f254: the magma bridge, the Cinder station rack fix, a fuller camp canopy, shard face patches. Not accepted: region beacon and haze. Coming in batch X: T6's dressing default off and journey keep-outs.
- **(Nov 14 09:30)** **Agent verdicts (T6, 09:25):** the space filled along the journey GOOD; regions R1, R2, R4, R5, R6 distinct GOOD; **NOT GOOD:** rocks on the route (fix building: 436 layout rocks pushed out of the journey corridors), R3 Ice Drift, R7 Outer Dark, R6 frame time until re-measured; beacons and haze off. Stays 🟨.
- **(Nov 14 10:30)** **Stays 🟨:** space filled along the journey GOOD, five of seven regions distinct (T6), pickups and drops GOOD; **Ice Drift and Outer Dark changed on main AD (27e9b92e), not looked at** (T6).
- **(Nov 14 11:30)** **🟨 → ✅:** Ice Drift and Outer Dark accepted by Ryan's Lead after T6's look on main (batch AD); the rest of the space field was already good (#114).
- **(Nov 14 16:30)** **✅ → 🟨 (Ryan's verdict, #194, 16:15):** "the planets feel empty and flat on foot". New top focus: terrain relief (divots, craters, holes, mounds), mountains all around, rocks, huts and trees filling the whole of both play zones, ambient effects (spores, mist, vents, embers). Owners: T4 terrain relief and the mountain ring (`World/TDTerrainField`, `TDPlanetTerrain`); the environment builder density, trees, rocks, placement of the huts and Fab assets; the structures builder the huts kit (Verdance d55fdc15 and Cinder fbda8372, on main 28c2fb05, batch BV, not yet placed); the effects builder the ambient effects; T6 the frame-rate check; T7 baseline and after frames and the verdict; T1 descent and take-off on the new terrain. **To close:** T7's after frames on both planets judged full, #194 closed, frame rate held (T6).
- **(Nov 14 17:30)** **Stays 🟨 (#194).** On main: Verdance fill pass 2 (26124c44, 2cc9245a: the pad GOOD; some route views still open), hut camps partly placed, brighter hut bodies (208a8ffc), perimeter and ground-break site sets (b30488d3, 69f5be8d), 16 ambient planet systems built (b8fbd53c; assets only, not yet spawned in the levels), Nicholas's Quixel plants, rock cluster, stump, volcanic rock, scorch and ash decals. Compiling, not on main: T4's Verdance terrain relief and mountain ring (f6c1853f). Not started: Cinder fill and relief. **To close:** both planets filled and relieved, effects spawned, T7's after frames judged full, the on-foot frame rate measured (T6).
- **(Nov 14 18:30)** **🟨 → 🐞 (the Cinder route BLOCKER above, ENG-26).** On main: terrain relief and the mountain ring on both planets (2f07ab5c), casting no shadow (96a66f7c: the shadow-map overflow at the pad gone); Verdance camps, whole-area fill, arches, landmarks and 49 ambient effects (e0b9385d). Played: T7 walked Verdance on 96a66f7c with the real pawn, PLAYABLE, one blocker (a pylon on the beacon path at 4280, 3505) and a camera-inside-a-hut-wall minor (environment builder); landing and leaving the ship GOOD; T6: Verdance on foot and the take-off climb GOOD on 2f07ab5c; T1: descent, landing and take-off on both planets. **To close:** the Cinder route fixed and walked; the Verdance pylon; Cinder's fill; T7's after frames judged full on both planets.

#### ENG-40 · Act 1: the tutorial battle and the withdrawal (CORE) · `✅`
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
- **(Nov 14 03:00) State:** the phase prompts v1 (EVASIVE MANEUVERS, TARGET AND FIRE, SQUAD SYSTEMS; T3, db27e563) and
  T1's boundary rule and release hook (9cfa5ec9) are on main f39a8372; the phase clock that counts only while the
  player flies (e36304e8) and the R011 line that now leaves the instruction to the SQUAD SYSTEMS prompt (0c638c1b) are
  on main 61404a79. None of it has been played by an agent on main. Not on main: T4's halves (the lesson gate, a softer
  Evade volley, gunship hold, withdrawal wait) and the boundary release call; T1's tutorial hint order by phase. Then
  played once by T3.
- **(Nov 14 04:00) State:** T4's halves (lesson gate, softer Evade volley, boundary release) and T1's hints by phase
  are in the T1 + T4 combined commit on branches, for batch T; nothing of ENG-40 changed on main this hour and none of
  it has been played by an agent.
- **(Nov 14 05:00)** **Played (T7, 04:14-04:29, real keys, batch O tree):** the three lessons (evade 18 s, target and fire, squad switch 22 s) with the lines in order, and the withdrawal by flying at the marker: working. Stays 🟨: T4's lesson gate and softer Evade volley and T1's hints by phase are in batch U's gate and change the lessons (replay on main pending); the hand-off into *Aftermath* is broken (ENG-42 🐞); MINOR: the objective reads CLEAR THE AREA during the withdraw order (T5's text is on a branch). **Decided by Ryan's Lead:** the withdrawal corridor keeps its Verdance bearing and gains a jump-point waypoint (T4, branch); no speed cap.
- **(Nov 14 06:00)** **State (🟨):** the lesson gate, the softer Evade volley, the boundary release and hints by phase are on main 50c4af7a (batch U), unplayed on main. Still on branches: the withdrawal objective text (eng/t5, 285d4869) and the JUMP POINT waypoint (eng/t4, 48124d88).
- **(Nov 14 06:30)** **State (🟨):** T7 played the real tutorial on main 50c4af7a (agent, real keys) up to the withdrawal and the *Aftermath* hold: it worked. The withdrawal objective text and the JUMP POINT waypoint are on main 94b56d11 (batch V), not played on that tree.
- **(Nov 14 09:30)** **🟨 → 🟢:** the lessons GOOD (T3, real keys, main); galaxy travel withdrawal → JUMP POINT → Verdance GOOD (T4). Not ✅: the withdrawal waiting for Done was not watched.
- **(Nov 14 10:30)** **🟢 → ✅ (10:15 rule):** the three lessons GOOD (T3, real keys, main), waves 1-2 GOOD (T4), the withdrawal → JUMP POINT → Verdance GOOD (T4), the *Aftermath* hold GOOD (T7). Polish: the withdrawal waiting for Done watched (T3).

#### ENG-41 · First-play tutorial flag and "Skip the tutorial" (Nicholas, #29) · `✅`
- A profile flag (not the run save; a New Game never clears it) is set by ENG-40's completion event.
- With the flag set, New Game offers "Play the tutorial" or "Skip"; Skip starts in the state the tutorial leaves: waves
  1-2 done, their lines marked played, hints off, *Aftermath* next. With the flag clear there is no offer.
- The intro's own hold-to-skip is unchanged. Nick's UI styles the prompt (ART-09).
- **Done when:** first run forces the tutorial; second New Game shows the offer; Skip gives an identical game state to a
  played tutorial (tested); Continue is unaffected. Owner: T7; on main 6fe3e90f (eb02ac51, 80351f1a), played by an agent (T7, session 2: the offer shows on a completed profile and Skip lands in the Aftermath state); review open (ART-20); the skip start overlapped space junk (#90 no. 10): T4's clear skip start (0a4e7501) is on main f39a8372, awaiting agent play; with T3, T5, T1 for their lines.
- **(Nov 14 05:00)** **Played again:** T7 used the main menu (New Game, Skip the tutorial, Continue disabled) on the batch O tree. Stays 🟨: the independent review (ART-20) is still open, because Nick's side is stopped.
- **(Nov 14 09:30)** **🟨 → 🟢 (Ryan's bar since 09:15 drops the human and the separate review from ✅; the agent play stands):** the offer and Skip were played by T7 (sessions at 03:00 and 05:00). Not ✅: not replayed and judged on main since batch U (T7).
- **(Nov 14 10:30)** **🟢 → ✅ (10:15 rule):** the forced first-play tutorial and "Skip the tutorial" on a completed profile played (T7, real keys; New Game and Skip again on the batch O tree). The New Game difficulty choice is ENG-45's.

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
- **(Nov 14 03:00) State:** on main 61404a79: the StoryFidelity01 wording corrections (71 ids), R102 bound as
  `G7_RecKato_SameConstruction`, the Story Director's decisions on R011, R132 (speaker COMMAND) and R020 (0c638c1b), the
  save / quit / Continue test for heard lines (007d9f14), the TransmissionStatic cue, `G9_Wren_PullAway`. No sequence
  has been heard in order by an agent on these batches. Without an owner until this hour (queued on `CLAIMS.md`, T3):
  the waves 3-4 interception ids and the `C7_*` set.
- **(Nov 14 04:00) State:** on main 6afa3816: Cinematic 03 no longer plays over the relay's first wave (the wave
  waits for `C3_Moss_Company`; 9f964b30). The waves 3-4 interception ids and the `C7_*` set are still queued (T3).
- **(Nov 14 05:00)** **🟨 → 🐞 (MAJOR, played by T7 on the batch O tree, #90):** the *Aftermath* beat is lost if the player keeps boosting after the withdrawal. The corridor leads into Verdance's atmosphere; the entry was held once, then the game travelled 1.4 s later in the middle of Wren's line; the hyperdrive jump beat and 17 queued story lines never played, and the arrival was silent. T3's replay (2b6be288) and T1's first entry hold are on main and did not prevent it. Fixes, all on branches: T1 (the entry holds for the whole sequence), T4 (jump-point waypoint), T3 (must-hear lines survive a map travel). Back to 🟨 when all three are on main; 🟢 needs the beat replayed while boosting.
- **(Nov 14 06:00)** **Stays 🐞:** two of the three *Aftermath* fixes are on main 50c4af7a (batch U) with passing tests: T1's hold when boosting into Verdance (23ffe072: a repeat request during the hold is taken, the ship crawls at the edge) and T3's must-hear lines surviving a map travel and resuming after the arrival (fd06afe5). T4's JUMP POINT waypoint (48124d88) is on eng/t4. **No agent has replayed the beat while boosting**; 🟨 when T7 has and it holds. On lead/int-v: every story line path obeys the subtitles setting (15f04846). Cinematic 03 to 08 past Verdance are unplayed since batch L.
- **(Nov 14 06:30)** **🐞 → 🟨: the *Aftermath* fix was played and held.** T7 on main 50c4af7a, real keys, through the real tutorial, boosting into Verdance: the entry is held while the sequence plays, all 16 lines air, the entry commits when it finishes (#90). The MINOR it found (the entry committed 2 s into the 4.8 s `A1_TrailToVerdance` line) is fixed on main 6cc4f254 (43703035), test only. Also on main 6cc4f254 (21040d6b), tests only: the G4 interception lines, `PlaySequence(Docking)` so `C7_Moss_WayOut` airs, four assault lines, the ClearSpace roll call. On main 94b56d11: every story line path obeys the subtitles setting. Not 🟢: none of batch V or W is played; the reveal (Cinematic 08) and Cinematics 03 to 06 are unplayed since batch L; the review is open.
- **(Nov 14 09:30)** **🟨 → 🟢:** story transmissions GOOD (T3: *Aftermath*, C03 relay, C05 terminal). Not ✅: **the console recordings, the final record and Cinematic 08 (the reveal) NOT PLAYED** (T3, T7).
- **(Nov 14 10:30)** **Stays 🟢:** transmissions GOOD (T3); **the console recordings and the Dont voice (Wren take 04, imported 6ee7100d) NOT HEARD** (T3).
- **(Nov 14 11:30)** **Stays 🟢.** As played (T5's list on #96). SEEN: the intro and tutorial lines, *Aftermath*, Cinematic 03 and the drone wake, Cinematic 05, the conduit, heart and escape lines, Face to Face on entry. **NOT SEEN:** the three console recordings (hangar, corridor, barracks), the final record and Cinematic 08 through its readout and the reactor door, the Cinematic 08 skip, the Dont voice in the warning (T5, next lock turn); C4 at the Verdance take-off, C6 at the Cinder take-off and wave 5, C7 docking and arrival, the waves 3-4 interception, C9 at the escape radius (T4, queued turn); both approved scans on their terminals (from #62).
- **(Nov 14 12:30)** **Stays 🟢.** Seen this hour: the hangar, corridor and barracks console recordings each play on E (T5, GOOD); Cinematic 08 plays through to its end with the Dont voice (SW_VO_FutureWren_Dont_04 in the log) and the full warning (T5, GOOD); the waves 3-4 interception (G4), the escape radius (G9, C9 x3), victory, the C7 docking and arrival lines aired (T4, log). **Not confirmed** (T4's correction): the Verdance (C4) and Cinder (C6) take-off lines; both take-offs were cut short; T4 redoes them flown. NOT GOOD: Face to Face played before Cinematic 08 (not the intended order; T3 replays it in order) and the Cinematic 08 skip landed on the next sequence (T5 re-checks). Wave 5 has two lines by design (T3). **Open from Nick's side's review (#139), MAJOR, source only and not reproduced:** quitting to the menu with a required recording unheard and starting a fast New Game can queue the old run's recording into the new run (the carry slot is not cleared at a new run); T3 owns the fix (invalidate the carry at a new-run or menu boundary, keep same-run travel). Placeholder words (Face to Face) are Nicholas's (polish).
- **(Nov 14 13:30)** **Stays 🟢.** Consoles and Cinematic 08 seen good (#96); the story consoles now use `SM_DI_StoryConsole` (BC). The #139 travel-carry MAJOR is fixed on main (886ee784, AY; source re-read by Nick's side: addressed); the played check (a recording cut off by a quit to menu, New Game at once) is T3's. **To close:** the Verdance and Cinder take-offs flown into the galaxy with their C4 and C6 lines (T4), Face to Face after Cinematic 08 in order and the skip (T3).
- **(Nov 14 14:30)** **🟢 → 🐞.** #96: a player can reach the reactor without playing the final record, so Cinematic 08 can be missed. **Decision (Ryan's Lead):** the reactor door is gated on the final record (T3 the story state, T5 the door). **Still missing:** that gate played; the Verdance and Cinder take-offs flown with their C4 and C6 lines (T4); Face to Face after Cinematic 08 in order and the skip (T3); the #139 carry played check (T3); the docking lines (T3, #110).
- **(Nov 14 15:30)** **🐞 → 🟨.** The bug is closed by the gate: the Warden objective's second step is PLAY THE FINAL RECORD (3b1fcde5, with `IsSequenceDoneOrSkipped` b5472653); the exit door, the Core objective and Face to Face wait for it; played GOOD by T5. The docking lines' queue fix is on main (#110 closed). **Still missing:** Continue from the Warden checkpoint restoring the final-record step (T3, T5; #96 stays open); the Verdance and Cinder take-offs flown with their C4 and C6 lines (T4). Polish: Face to Face's fifteen `G8_Self` lines wait on Nicholas's words (kept off air in batch BQ, not on main).
- **(Nov 14 16:30)** **Stays 🟨 (branch only).** On main: Continue restores the final-record step and the reactor door (ccaaf4d4, batch BS c8eef9a5, T5); `C8_Wren_Order` plays its selected master (#135; by log, not by ear); placeholder lines kept off air. On T3's branch (next batch): the fifteen Face to Face lines (ad1694fa) and Face to Face once per run (9c7b3050); T3 played the #96 natural route GOOD with the real Face to Face lines. **To close:** that batch on main and #96 closed (T3, T5).
- **(Nov 14 17:30)** **Stays 🟨.** On main 0c986a7d: the fifteen Face to Face lines in Nicholas's exact words (ad1694fa) and Face to Face once per run (9c7b3050). **#96 was closed and reopened by Nick's side:** Face to Face replays after a quit and a cold Continue (their sessions 31-32 on b8a55662); T3's fix (77ad713e, every once-per-run claim and must-hear line writes the run save at once) is proven by two sessions with a cold launch between and is compiling, not on main. Separate: the legacy final-record bypass from Nick's session 28 (T5). **To close:** 77ad713e on main, Nick's unchanged input save through two cold launches, T5's bypass fix.
- **(Nov 14 18:30)** **Stays 🟨.** On main 2f07ab5c: the once-per-run claims and must-hear lines written to the run save at once (7a1e6598); Nick's sessions 34-35 passed the cold Continue (Moss). On main 48a6fd97: T5's old-save gate (4648d817), which Nick's session 38 shows stranding a legacy saved pawn beyond the locked Warden exit with the console on the other side (MAJOR, legacy saves only). Kept 🟨, not 🐞, because a new run cannot reach that state; the Lead may decide otherwise. **To close:** T5's recovery for that save (record prerequisite and Warden progress kept), Nick's same-save proof, #96 closed.

#### ENG-43 · Gameplay barks from `dialogs.md` (CORE) · `✅`
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
- **(Nov 14 03:00) State:** `Bark_MissileEvaded` (357d7b02) is on main. The pre-empt clue has owners (T4 AI, T3 line)
  but is queued behind their current work. The remaining new triggers have no one working on them: queued on
  `CLAIMS.md` this hour as one sweep per lane (T1 travel and landing, T2 combat, T4 enemies, T5 on foot), T3 checking
  the count against `dialogs.md`.
- **(Nov 14 04:00) State:** counted tonight: **44 bark ids fire in the game; about 55 ids in `dialogs.md` fire
  nowhere.** The missing ids are listed per lane and sent to T1, T4 and T5. `Bark_Anticipated` (the pre-empt clue's
  line; `DT_BarksFallback` re-imported at 265 rows, b3c04349, with T3's trigger 55ba5156) is in batch S's gate, not on
  main; its words are Nicholas's to approve.
- **(Nov 14 05:00)** **State:** `Bark_Anticipated` and its trigger are on main 4eb57848 (words: Nicholas). On branches: T4's world barks and T5's on-foot barks. T1's travel barks are queued; T2 has no list yet. The count of 44 firing ids is not redone.
- **(Nov 14 06:00)** **State (🟨):** on lead/int-v, not on main: T1's six travel barks (2fd52a50: `Bark_BoostEmpty`, `Bark_Hyper_Spool`, `Bark_Hyper_Blocked`, `Bark_Atmo_Entry`, `Bark_Land_Ready`, `Bark_Land_Down`). On branches: T4's world barks (d5e29eab) and T5's on-foot barks (285d4869). T2's combat barks have no list yet. The count against `dialogs.md` (T3) follows once these are on main.
- **(Nov 14 06:30)** **State (🟨):** **on main 94b56d11 (batch V), tests only:** T1's six travel barks, T4's world and enemy barks, T5's on-foot barks. On main 6cc4f254: `Bark_Dom_GeneratorDown` / `Turrets` / `Clear` retired in favour of the `G6_*` lines and the roll call. T2's combat barks have no list yet; T3's count against `dialogs.md` can now run. None heard in a played run on these trees.
- **(Nov 14 09:30)** **Partly seen (T1):** travel barks; the planet arrival barks and the escape line "Thirty seconds. Keep going." are on main 80a2ee4d (batch Z), tests only. T2's combat barks still have no list. Stays 🟨.
- **(Nov 14 10:30)** **Stays 🟨:** travel and world barks partly seen (T1, T4); **T2's combat barks have no list; the count against `dialogs.md` not run** (T2, T3). Placeholder words await Nicholas (polish).
- **(Nov 14 12:30)** **Stays 🟨.** T4: the pre-empt clue fired after forced switches (log, wave 4, once per wave); the flash not yet seen in a frame and the Anticipated bark not confirmed (squad barks now log their id, b21f34a3) (#122). **To close: T2's combat bark list and T3's count against `dialogs.md`**.
- **(Nov 14 13:30)** **Stays 🟨.** T3's count from code (#122): 120 bark ids in the table from `dialogs.md`, 117 with a trigger in game code; `Bark_Swarm_Saved` now fires (df433eb0, BB), `Bark_Wipe` is T3's, `Bark_Cmd_Retreat` is left unused (there is no retreat command); on-foot barks in the styled subtitle strip (c06a18d7, AY; seen by T7). **To close:** barks heard across a played run, the pre-empt flash in a frame and the Anticipated bark in the log (T4).
- **(Nov 14 15:30)** **🟨 → ✅.** #122 closed as played GOOD: the pre-empt tell in play with its Anticipated bark; barks heard across the played runs; the docking-line queue fix (b5472653). Polish: `Bark_Wipe` (T3); words and voices are Nicholas's (ART-11).

#### ENG-44 · Ending: regroup, victory, stinger, credits, back to the menu (CORE) · `✅`
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
- **(Nov 14 03:00) State:** the credits music plays through T1's native fallback on main 61404a79 (not heard by
  anyone); the escape flight that leads into the ending is T4's 0a4e7501 on main f39a8372. The victory eyebrow reads
  "MISSION COMPLETE // OPERATION PARADOX": explicitly retained by Nicholas on November 14, with one of six random large headlines; approved as a post-ending operation label, not a public game subtitle
  (noted on #29), no change made. Not yet played twice (watching and skipping) on these batches.
- **(Nov 14 09:30)** **Victory and credits NOT PLAYED** on the boarding ending (T3); the space ending was played to VICTORY by T4 on lead/int-aa (enemy markers remain after victory). Nicholas's victory eyebrow and six headlines: N-094, archived. Stays 🟨.
- **(Nov 14 10:30)** **Stays 🟨:** victory, stinger and credits music GOOD (T7, batch Z); **the credits roll and RETURN TO MENU not checked on screen** (T3).
- **(Nov 14 13:30)** **Stays 🟨.** **To close:** the credits roll, RETURN TO MENU and no CONTINUE after victory checked on screen (T3, #95).
- **(Nov 14 14:30)** **🟨 → ✅.** #95 closed by the Lead as played GOOD: regroup, victory with takes, the 18 s stinger, the credits roll (now naming Ryan Thomas, Gameplay & Engineering Lead, with a roles line: c5f2fa0d, #181, DA_Credits edited by Ryan's Lead on Ryan's direct instruction), RETURN TO MENU and no CONTINUE after victory. Still missing: nothing on Ryan's side; Nicholas's ear on the credits music stays ART-10.

#### ENG-45 · Settings and accessibility completion (CORE) · `✅`
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
- **(Nov 14 03:00) State:** on main: subtitle size and the graphics preset (T7, 20bdfe00); T7's deuteranopia check,
  which led to enemy bolts readable by shape (0c5de65c) and the enemy marker shapes (ede33e91). Still open: subtitles
  on / off (T3) and the shake and speed-FOV sliders (T1), both queued on `CLAIMS.md` this hour; New Game difficulty
  (T5, queued); a protanopia pass; each row changed in a played run and kept across a restart.
- **(Nov 14 04:00) State:** in batch S's gate: the run's difficulty scales crew health, every crew hit and each
  on-foot fight's size (T5, 231b9876; its Ace-only bug is fixed on T5's branch for batch T). On a branch: the
  subtitles on / off setting (in the T1 + T4 combined commit). Still open: shake and speed-FOV sliders (T1), the
  difficulty choice on New Game, a protanopia pass (T7).
- **(Nov 14 05:00)** **State:** the on-foot half of the difficulty (231b9876) is on main 4eb57848 and its Ace-only wall-spawn fix (8e1a4e6d) on main 29a15f21, with a spawn test for every preset; the subtitles setting is in batch U's gate. Still queued: the shake and speed-FOV sliders (T1), the New Game difficulty choice (T5, T3). Not played.
- **(Nov 14 06:00)** **State (🟨):** the first subtitles on / off setting is on main 50c4af7a. On lead/int-v: the flight HUD's bark lane and story transmissions draw no text when it is off (fd16f394), and every story line path obeys it (15f04846). Still queued: shake and speed-FOV sliders (T1), the New Game difficulty choice (T5, T3), a protanopia pass (T7).
- **(Nov 14 06:30)** **State (🟨):** on main 94b56d11 (batch V), tests only: the bark lane and story transmissions draw no text when subtitles are off (fd16f394), and every story line path obeys the setting (15f04846). Still queued: shake and speed-FOV sliders (T1), the New Game difficulty choice (T5, T3), a protanopia pass (T7).
- **(Nov 14 09:30)** **Agent verdict (T3):** subtitles on GOOD, off NOT PLAYED; the shake and speed-FOV sliders and the New Game difficulty are still queued. Stays 🟨.
- **(Nov 14 10:30)** **Stays 🟨:** **subtitles off NOT PLAYED** (T3); **the New Game difficulty choice unbuilt** (T5, T3); shake and speed-FOV sliders queued (T1).
- **(Nov 14 11:30)** **Stays 🟨.** The New Game difficulty choice is built and seen good by T3 in the running game (d82e682c, on main in batch AK: CHOOSE DIFFICULTY with Story / Standard / Ace and their lines, Standard focused, Back; Ace set and the run started; #127). The Settings page now covers the pause screen (ef34a7a7, AK; #156, not seen). Subtitle wrap inside 80% of the screen and the size setting followed live on foot, inside, in the intro and the menu (3c5e68f6, AM; #154, #155): not proven by a capture yet (T3). Left: subtitles off played (T3, #94), the shake and speed-FOV sliders (T1), Skip the tutorial through the same page (T3).
- **(Nov 14 13:30)** **🟨 → ✅** (#127 closed 13:06 EST): the shake and speed-FOV settings measured in a running game by T1 (`TD.Flight.CamProfile`, 4 s of boost: at 1, shake 1.50 degrees, FOV kick 5.99 degrees, blur 0.040; at 0, all zero; set through the settings system, not by dragging the sliders); the New Game difficulty page played and seen again in the Shipping package; subtitle size retests passed on Nick's side (#155 closed); Pause → Settings opens with every row in Shipping 4bbdd282 (#156). Polish (AAA list): the #156 frame of Settings over the pause screen (T1), the Subtitle size row cut by the Back bar (packaging agent's finding), Skip through the difficulty page (T3). Subtitles off in combat and on foot stays on ENG-32 (#94).

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
- **(Nov 14 03:00) State:** the last end-to-end agent playtest is T7's of batch L (main 6fe3e90f). Batches M, N and O
  have not been played end to end; T7 plays main 61404a79 after its performance window. No gate G1-G4 has been run:
  **G1 (Ryan, packaged build) is due this morning**, on the eb1bf334 package unless batch O's is built first.
- **(Nov 14 04:00) State:** unchanged and now the main blocker: batches M to R (main ddc50a2f) have not been played
  end to end by an agent; T7 is still in its performance window. No gate G1-G4 has been run.
- **(Nov 14 05:00)** **State: the first end-to-end play evidence since batch L.** T7, 04:14-04:29 EST, real keys through the engine's MCP server in a `-game` session, on the batch O tree (364aabb3 + T2's pool fix + T7's performance work), so behind main by batches P-T; not a human playtest. Working: main menu, intro skip, the tutorial lessons, switching, the Bastion dome, wingmen against rocks, marker shapes, the withdrawal, the Verdance entry, the first view on foot. Broken: the *Aftermath* beat (MAJOR, ENG-42). MINOR: CLEAR THE AREA during the withdraw order. Not played: landing by hand, both planets' objectives, the assault, docking, the interior, the reactor, the escape, victory (T7's next session is running). Batches P to T are still unplayed end to end, and the published ddc50a2f package has only a launch check. No gate G1-G4 has been run.
- **(Nov 14 06:00)** **State (🟨):** T7's agent play (batch O tree, behind main by batches P to U) now covers: the opening, the tutorial, the withdrawal, the Verdance entry, Verdance on foot through the return to orbit, and Cinder on foot to the gate conduits. Found: two MAJORs (*Aftermath*, the Cinder magma channel). **Unplayed by anyone since batch L: the Dominion assault, docking, the interior, the reactor, the escape, victory**; also the rest of Cinder and a landing by hand. No human has played since batch M. Gate 959 / 959 on main 50c4af7a.
- **(Nov 14 06:30)** **State (🟨):** gate 986 / 986 on main 6cc4f254 (batch W), 980 / 980 on 94b56d11 (batch V). T7's agent play on main 50c4af7a added: the *Aftermath* replay (fixed), the Dominion assault (works), the interior from the hangar to the Warden. Found: **MAJOR, a death in the Warden fight skips the Warden**; the Heavy guard MAJOR still open; three MINORs. T1 played the Verdance route, the glide and a switching check; T3 Continue and the intro. **Still unplayed by anyone: docking on main (T1 is playing it), the reactor conduits and heart, the escape run and flight, victory, credits, the space-reactor fallback.** Batches V and W are tests only. No human has played since batch M; the Shipping package is checked to wave 1 only.
- **(Nov 14 09:30)** **Gates:** X 991 / 991 (206e33df), Y 999 / 999 (8b4eabae), Z 1005 / 1005 (80a2ee4d). Played with real keys by T7 on Y + T5's arena fix: the interior hangar to the Warden, the Warden retake and fight, all GOOD. **Still NOT PLAYED by anyone:** the reactor conduits and heart on foot, the reveal, the escape run, reboard, credits, Cinder past the conduits, waves 3-5 with every enemy type. Stays 🟨.
- **(Nov 14 10:30)** **Stays 🟨:** every act has now been played by an agent in pieces, but **no single start-to-end run on one main and no packaged play past wave 1** (T7, Lead).
- **(Nov 14 11:30)** **Stays 🟨.** T5: `TD.God` on foot seen holding in play (#131). Left: a start-to-end run on a package with audio and input (#152).
- **(Nov 14 13:30)** **Stays 🟨.** Nick's Lead's natural campaign cleared wave 5 on take 1 and entered the Dominion assault (b5d1549c, N-099, archived); T7's Shipping play reached the tutorial's withdrawal. **To close:** a start-to-end run on the newest main or package (T7, #131, #152).
- **(Nov 14 15:30)** **Stays 🟨.** T7 continued from a prepared Verdance save in Shipping 9f20c5be (blade, carry, on-foot card GOOD; no fight reached in Shipping); the Dominion assault is survivable without god mode (#112). **To close:** a start-to-end run on the newest package with all four generators without god mode (T7, #131, #152).
- **(Nov 14 16:30)** **Stays 🟨.** Played GOOD this hour: all four generators without god mode (T7, a9c635de); the styled menu, Cloak in wave 5 and the auto-switch grace in Shipping 8d62d14a (T7, real mouse and keys); switching under fire, a portal gate, a hyperdrive jump (T1). **To close:** a full Shipping run start to end (T7, #152).
- **(Nov 14 17:30)** **Stays 🟨.** No package played start to end yet (#152). **To close:** a full Shipping run (T7).

#### ENG-47 · Finale mode decision and the fallback ending (CORE) · `✅`
- Boarding is the documented ending (`Story/README.md`: "The confirmed ending boards the Dominion; the existing space-only
  route is an emergency fallback"). `td.Finale.Boarding` defaults to 1 on the freeze build once ENG-29-31 pass G2.
- **Decision point: Nov 14, 20:00 EST (Ryan).** If the boarding route has an open BLOCKER then, the freeze build ships with
  `td.Finale.Boarding 0`; the reveal is then carried only by the assault transmissions and placeholders (no fallback
  writing is requested from Nicholas).
- The fallback stays finishable until the freeze: covered by ENG-46 route (c).
- Owner: Lead (decision record, cvar default), T4 and T5 (both routes green).
- **(Nov 14 03:00) In progress:** the fallback was not finishable in a played run (ENG-18, #90 no. 5); T4's fix 818d592e
  is on main with its tests, still awaiting agent play (T7's end-to-end playtest of 61404a79). The Lead writes the
  decision record once it has been played.
- **(Nov 14 06:00)** **State (🟨):** the fallback ending is still unplayed on main; the decision record is due Nov 14 20:00 (Lead).
- **(Nov 14 06:30)** **State (🟨):** the fallback ending is still unplayed (T7's assault play went on to the docking route); the decision record is due Nov 14 20:00 (Lead).
- **(Nov 14 09:30)** **The space-reactor fallback was played to VICTORY** by T4 with the brake assist (lead/int-aa f95043c1, not gated), **NOT GOOD yet** (enemy markers after victory, breakup not captured). The decision record is still due Nov 14 20:00 (Lead). Stays 🟨.
- **(Nov 14 10:30)** **Stays 🟨:** **the space-reactor fallback not replayed through the breakup after 782669d5** (T4); the decision record is due 20:00 (Lead).
- **(Nov 14 12:30)** **Stays 🟨.** **Decision recorded** (Lead, on Ryan's instruction to choose what fits the game, 11:45; DEVLOG.md 9286fc82, #113): the boarding ending is the game's ending and `td.Finale.Boarding` stays 1 for the freeze build; the space-only reactor ending stays as the emergency fallback. **To close: the fallback seen good once more: the generators by real fire and the breakup on screen** (T4, with ENG-18).
- **(Nov 14 13:30)** **Stays 🟨.** The fallback's breakup is seen GOOD (#112); #113 closes with ENG-18 on the four generators by real fire and the cut back to VICTORY (T4).
- **(Nov 14 14:30)** **🟨 → ✅.** #113 closed by the Lead as played GOOD (13:37 EST): the fallback ending (`td.Finale.Boarding 0`) runs through the generators, the exposed reactor, the breakup (v2, 8f59cfe1) and the cut back to VICTORY. The survivability of the assault without god mode is a bug on ENG-18, not this item.

### Art and assets made on Ryan's side (moved from Nick's section, Nov 13)

Ryan's rule (Nov 13): what Claude or Grok Bot makes is under Ryan's name. These five items keep their `ART-xx`
IDs and their original spec text; the first line of each says who makes it now. **Since the afternoon of Nov 13 this
covers all of Environment, VFX and Asset Management** (ART-02, 03, 04, 05, 06, 07, 08, 13): Ryan took those three
categories over completely. Nicholas keeps direction and
sign-off, and an asset of his at the Presentation name replaces ours.

#### ART-03 · Ship visuals: Rift, Lancer, Bastion · `✅`
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
- **(Nov 14 03:00) State:** on main 61404a79: Rift v13, Lancer canopy glass v06 (on the v05 hybrid), Bastion v02 (the
  glowing squiggle in the windscreen cleared, emissive strength 6 → 4). Rift v14 is in batch P's gate. **The Bastion
  glass cockpit was attempted and stopped; v02 stays. Whether Bastion gets a glass cockpit is a design decision for
  Ryan.** Seen in builder captures only; Ryan has not approved any ship at 3-10 m on these versions.
- **(Nov 14 04:00) State:** on main: Rift v14 (4e894365) and Lancer cockpit v07 (lighter seat and consoles, pale visor
  band, glass opacity 0.8; dedd9735, main 6afa3816). On a branch: Lancer v08. The Bastion cockpit design is still held
  for Ryan. Builder captures only.
- **(Nov 14 05:00)** **State:** on branches: Lancer v08, a rear-view review of the three ships, and landing gear meshes for the three ships (structures builder; Lead's decision to model them as separate meshes). The Bastion cockpit design is still held for Ryan.
- **(Nov 14 06:00)** **State (🟨):** Lancer cockpit v08 is on main 50c4af7a (imported headless, not yet seen in the game); landing gear meshes `SM_Rift_Gear`, `SM_Lancer_Gear`, `SM_Bastion_Gear` are on main, not wired. The Bastion cockpit design waits for Ryan.
- **(Nov 14 06:30)** **State (🟨):** the three ships parked on landing gear were seen in the running game by the structures builder; the gear is still not wired to the landing. The ship rim (5ee29c14) waits on T1's live yes for batch X. The Bastion cockpit design waits for Ryan.
- **(Nov 14 09:30)** **Lead's decision:** Lancer contrast through a fresnel darkening in `M_TD_Ship` (default 0, Lancer only; T1). The ship builder stopped at about 07:00 on a usage limit; T1 took over its Lancer work. Stays 🟨.
- **(Nov 14 10:30)** **Stays 🟨 (Ryan's side):** **Lancer contrast against the cyan nebula not checked** (T1; the fresnel darkening in `M_TD_Ship`).
- **(Nov 14 11:30)** **🟨 → ✅ (closed as shipped, Lead's time-boxed decision, #108):** `M_TD_Ship` has RimDarken (Lancer only), a deep-blue nozzle core and a dimmer boost plume (707937a8, on main in batch AG efdf4284): slightly better against the cyan nebula at speed, no change at boost. Polish: rebuild the Lancer's boost plume (AAA list).
- **(Nov 14 16:30)** **Stays ✅.** All three canopies on main (the Bastion's glazing r2, d0051abe; Nick's Lead accepted the look; #149 open only for T7's real-speed intro check); the Lancer boost plume keeps its cyan (c31d5a1b).

#### ART-04 · Enemy & Dominion visuals · `✅`
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
- **(Nov 13 22:45)** Hull r2, generator and bay source merged (#72, #74, #77); imported, on main 6fe3e90f (c3375433); r2 in use with T4's socket work (b59423d5, on main eb1bf334, awaiting agent play). Enemy readability materials (fighter, interceptor, gunship, bomber, carrier) are in batch N's gate; ship builder 2 reviews them next, with the turret and the hauler.
- **(Nov 14 03:00)** The enemy readability materials (fighter, interceptor, gunship, bomber, carrier) are on main
  f39a8372; per-type engine plumes are on the effects branch. From captures the Null fighter is still only a few
  pixels tall at fight range in the chase camera, so the read is carried by T2's marker shapes (main 61404a79). The
  Warden's shield floats off the body on main (rig fix in progress; ENG-23).
- **(Nov 14 04:00)** On main 6afa3816: the Null interceptor's silhouette (wing-tip lances and twin canted tail fins,
  so it no longer reads as the fighter at 60-150 m; 74749d4c), the fleet's range glow retune (0e5ded3f) and the Warden
  shield on the left forearm. On branches: the hauler repaint, per-type engine plumes. Builder captures only.
- **(Nov 14 05:00)** On branches: the hauler repaint (#51) and per-type enemy engine plumes. On main 29a15f21: the Dominion escape debris set (`SM_KS_DomDebris_*`). Evidence frames are posted on #41 and #51 with a close request.
- **(Nov 14 06:00)** **State (🟨):** on main 50c4af7a: the Null bomber's twin-boom tail and the fighter's emissive 12 → 7 (builder captures only). On lead/int-v: the Dominion exterior in the Null colour code (near-black hull, red seam light, dimmed window clusters; b24c8bc4). The hauler repaint is open.
- **(Nov 14 06:30)** **State (🟨):** the Dominion exterior in the Null colour code is on main 94b56d11 (b24c8bc4), builder captures only; `MI_TD_DominionGlowRed` is left as it is, held for Ryan. Enemy plumes are not accepted. Escape debris drawn with `SM_KS_DomDebris_*` on main 6cc4f254.
- **(Nov 14 10:30)** **🟨 → ✅ (10:15 rule; Ryan's side):** the fleet seen in play (T4: all four wave 4 types warp in, carrier launches, engine glows on every enemy and none without exhaust; plumes wired, 4d0a21c8), the Dominion exterior in the assault GOOD (T4, T7), the Warden and crew GOOD in fights (T7). Polish: `MI_TD_DominionGlowRed` 20 → about 6 (held for Ryan); Nicholas's review of the Dominion recolour (his art direction, unchanged).

#### ART-06 · Planet environments: Verdance & Cinder (CORE) · `✅`
**🟦 Ryan since Nov 13.** Terrain, mountains, materials, rocks, trees and set pieces: engineering (ENG-34) with Grok Bot's texture sets and set pieces (R-027 #3, #8). **Lighting, fog, sky and grade on the planets stay with Nick (ART-13).** **(Correction, Nov 13 afternoon, R-036: ART-13 moved to Ryan's side too, so the planets' lighting, fog, sky and grade are the Lead's environment builder's.)** Still missing for the complete game: the dressed play zones (ENG-39), the walkable survey stations and dig cabin (ART-17), the story set pieces (ART-18), and the lighting pass per planet (ART-13).
- **Verdance:** bioluminescent jungle, giant mushrooms and ferns, waterfalls, mist, ringed moon,
  pollen, critters, wreck-site and relay-spire art.
- **Cinder:** basalt, magma rivers, cyan and amber crystal spires (crystal kits), falling ash, an
  outpost kit, the jammer tower and its destruction.
- **Across both:** terrain materials, foliage instancing, sky and atmosphere, lighting, and the
  atmospheric-descent vista (canyons and clouds the ship flies through).
- **(Nov 14 03:00) State:** on main 61404a79: far-tier tree clumps from the real trees and the Cinder ambient floor
  (pass 4), the Verdance field camp and Cinder mining camp, the relay-mast and Null-derrick landmarks r1. In batch P's
  gate: Verdance cliffs, the Cinder magma retune, relay mast r2 (r1 reads too thin). On the environment branch: the
  Verdance canopy and terrain pass (#90 no. 20). Builder captures only; not seen by an agent in a played run.
- **(Nov 14 04:00) State:** on main: Verdance cliffs, the Cinder magma retune and relay mast r2 (4e894365); the
  planet Dress pass (ddc50a2f). On eng/env: the Verdance canopy and terrain pass s8 (1,475 clumps over about 40 % of
  the vegetated lowland, the scar left open; e2830e63). Builder captures only.
- **(Nov 14 05:00)** **State:** the Verdance canopy at about 40% coverage is in batch U's gate; Cinder from the air and the wreck scar are on the environment branch. T7 saw Verdance's first view on foot on the batch O tree only.
- **(Nov 14 06:00)** **State (🟨):** the Verdance canopy at about 40% coverage is on main 50c4af7a. New from T7's play: Cinder's magma channel does not read as lethal (environment builder; a bridge from the structures builder).
- **(Nov 14 06:30)** **State (🟨):** on main 6cc4f254 (38d032e2): the magma bridge on Cinder (walk-tested by its builder), the Cinder station rack fix, a fuller Verdance camp canopy. On eng/env, waiting on the s13 sheet: Cinder crystal clusters, terrain material, the scar, the magma surface and glow. Accepted, in batch X: cloud v2 for both planets.
- **(Nov 14 10:30)** **Stays 🟨 (Ryan's side):** both planets walked; **the Cinder gate conduit art (`SM_Kit_NullConduit`, in AE) not seen in play** (T7); Verdance ground detail is polish (parked).
- **(Nov 14 11:30)** **🟨 → ✅:** the Cinder gate conduits are the new Null conduit art on main (`SM_Kit_NullConduit`, swapped for `SM_Kit_NullConduitBroken` on break), seen in the running game by the structures builder at 7 m and 4 m and looked at by Ryan's Lead (#118). The broken stump after a blade break is checked in T7's gate play (ENG-26, #117).

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
- **(Nov 14 05:00)** **State:** on the effects branch, not on main: cloud veil v2, per-type enemy plumes, region beacon and haze, shield drop, approach-field shimmer, tractor beam, the light row; the range flare is added to this lane.
- **(Nov 14 06:00)** **State (🟨):** see ENG-37: shield drop and Cinder cloud accepted from sheets; the Verdance cloud v2, plumes, approach field, light row, tractor beam, beacon and haze not yet accepted; none of round 10 is on main.
- **(Nov 14 06:30)** **State (🟨):** see ENG-37: accepted and in batch X (not on main): cloud v2 for both planets, shield drop, approach field, light row, CloudPunch, `DA_ReactorCore_FX`, glide wind; not accepted: enemy plumes, region beacon, region haze, tractor beam; the magma heat effect built, not seen.
- **(Nov 14 10:30)** **Stays 🟨 (Ryan's side):** **the hyperdrive jump effect unplayed** (T1, with ENG-15) and **the tractor beam unseen** (T4); the Cloak shimmer not accepted (polish).
- **(Nov 14 12:30)** **Stays 🟨 (Ryan's side).** From play (T4, the Lead looked at the frames, #105): the bomber mines read as flat pale discs (they need a dark spiked core, a red pulsing light, an arming blink; Grok Bot's mine concept #173, about 2.8 m); the tractor beam is seen end-on as a faint ring (a wider cone and motes streaming toward the bay; concept #174). Both with the effects builder, with the breakup explosions and the Cloak (#111, #102).
- **(Nov 14 13:30)** **Stays 🟨.** With the effects builder after the Cloak shell (#102): the tractor beam cone and motes, the mine look (#105), the breakup explosions at hull scale (#111).
- **(Nov 14 15:30)** **Stays 🟨.** The Cloak shell is done (#102 closed). **To close:** the tractor beam v2 and the mine effect wired and seen (effects builder; T4's hooks on main), the Lancer flame core (the plume rebuild 932570cc in BQ), the hull-scale explosions (#111).
- **(Nov 14 16:30)** **Stays 🟨.** The Lancer flame core is done (c31d5a1b, on main). **To close:** the tractor beam v2 seen in a real docking and the mine effect on `SpawnMine` (trial wiring taken back out at eb5de6fa; T4's fixes queued, effects builder; #105); the #194 ambient planet effects (spores, mist, vents, embers; effects builder).
- **(Nov 14 17:30)** **Stays 🟨.** On main: 16 ambient planet systems (b8fbd53c) and the stronger Verdance set (bbad3927: fireflies, spores with shafts, teal mist, falling leaves), not yet spawned in the levels; T4's beam and `SpawnMine` fixes (51f45b35). **To close:** the beam v2 and mines in the data and seen (#105), the ambient effects placed and seen on both planets (#194).
- **(Nov 14 18:30)** **Stays 🟨.** 49 ambient effects placed on Verdance (e0b9385d); tractor beam v3 visible in a staged spawn (f6cb2c84). **To close:** the beam in a real docking, mines in a real wave, Cinder's ambient effects placed.

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
  **(Nov 14 03:00) State:** on main 61404a79: four in-place ground deaths per crew type, per-type attacks, Warden deaths
  (the "death clip then ragdoll" row for crew). Open bug: the Warden's shield floats off the body (rig fix in progress).
  No one is working on interact / board clips, foot IK or landing gear: queued on `CLAIMS.md` this hour (animation
  builder, after the Warden fixes).
  **(Nov 14 04:00) State:** the Warden shield is on the left forearm on main 6afa3816 (e2e74225). On eng/anim: the
  Warden death rebuilt on a new base, the Summon pose, a pilot interact clip and board clips (not captured in game).
  Foot IK and landing gear still queued.
  - **(Nov 14 05:00)** **State:** in batch U's gate: the Warden clips moved to `SKEL_Warden` (five had silently fallen back to template clips), the Warden death rebuilt, the Heavy bash raised. On eng/anim: the pilot interact rebuild; live captures of hit reactions, walk and stagger. Landing gear: separate meshes, in progress (structures builder). Pilot foot IK (about half a day) is held for Ryan.
- **(Nov 14 06:00)** **State (🟨):** on main 50c4af7a: the Warden clips on `SKEL_Warden`, the rebuilt Warden death, the raised Heavy bash, the Trooper hit reacts, pilot interact r5 (press at chest height); captured live by the builder. Landing gear meshes on main; the settle is queued. Foot IK is held for Ryan.
- **(Nov 14 06:30)** **State (🟨):** the interact clip is on main 94b56d11 (T5's use of it). Lead decision: crew bodies stay 30 s. From T7's interior play: no lunge hint on the Heavy (T5). Foot IK is still held for Ryan.
- **(Nov 14 10:30)** **Stays 🟨 (Ryan's side):** crew and Warden animation in fights GOOD as gameplay; **no visual verdict on the pilot and crew animation set** (animation builder); pilot foot IK held for Ryan.
- **(Nov 14 12:30)** **Stays 🟨 (Ryan's side).** Two new open sub-points, both Ryan's focus: **(1) the energy blade remake (#163; blade effects builder, `.wt/blade`, eng/blade):** the first frames were judged NOT GOOD yet by the Lead; round 2 is in progress on eng/blade (not on main); the direction is Grok Bot's concept r1 (#172: a single-edged wedge of energy on the existing rail, hot near-white core, pilot-colour glow, ignite as a spark crawling along the rail, ribbon trail with sparks shed on the arc). Done when seen in the running game on foot (idle, walk, light 1-2-3, heavy, air slam, block, deflect) and judged good by the Lead. #163 was auto-closed by PR #172's merge and needs reopening. **(2) The pilot animation (#164; animation builder, `.wt/anim`, eng/anim):** round 1 judged good by the Lead for the light combo (three distinct arcs, contact on the hit time, blade clear of the body, 0.08 s cross-fade), the block and the air-slam landing; also fixed: feet sliding in the walk and run, turn in place. It is in batch AV (eng/anim through 8b32f082, on main a14bf410 as this audit ran, not yet seen on main). Not yet captured: heavy thrust, deflect flick, hit reacts, combat idle, strafe legs, slow walk; round 2 queued (those captures, the run wrist, starts and stops, then foot IK); then the crew and Warden clips from Grok Bot's PRs #165 and #166. The old set stays switchable with `TD.Pilot.MeleePass 0`.
- **(Nov 14 13:30)** **Stays 🟨 (Ryan's side).** **The new energy blade is on main** (b8066135, batch BH; #163): the V2 beam in the pilot's squad colour, the always-on aura with tip-speed sparks and swing sheet, ignite / retract / hit / deflect / perfect deflect / air slam wired through `DA_Blade_FX`, the old crescent kept in ART-HISTORY. Seen in game sessions (round 3 sheets, the Lead looked): solid full-length beam at idle, heavy and block; swing sheet and coloured tip sparks on lights 1 and 2; the heavy's spark fan; a hit on a Grunt with a crescent and a spark spray; the air-slam ring; retract flash. A fix for sparks lighting while running or retracting (3e3e7e70) is in batch BI. **Not yet proven:** deflect, perfect deflect and the ignite flash (blade builder, one more short turn). **The pilot animation round 1 is on main** (a14bf410, AV; #164) and played twice by T7: walk, light 1-2-3 landing where the blade is, block stance, heavy wind-up holding, barks in the styled strip, the chase camera out of trees GOOD; NOT GOOD: the out-of-combat idle shows the blade as a stub by the hip (animation builder, first item) and a Grunt walked inside the pilot during the wind-up (T5 adds a minimum separation). Round 2 (idle v3, block v3, jump / fall / dash, foot IK) is on eng/anim, not on main; then the crew and Warden clips (#124).
- **(Nov 14 15:30)** **Stays 🟨 (Ryan's side).** **#163 (the blade remake) and #164 (the pilot animation) closed as played GOOD.** On main 8d62d14a: the blade walking fix and cast light (278c1351), the walk / run / idle carry v4-v5 and the foot-slide fix, the sprint clip (828601aa, seen 05e35edd), the landing clip, the crew clips in real fights (4c013d58, 5b4ccb23: Trooper aimed strafe and backpedal, Grunt two-hit combo, Warden phase-2 roar). **To close:** heavy-hit reactions for the Grunt and Trooper (animation builder, T5's slot; #124). Polish: the blade light washing enemy armour at arm's reach (blade builder, #128).
- **(Nov 14 16:30)** **Stays 🟨 (Ryan's side).** On main: Grunt / Trooper heavy-hit knock-back clips (4fffeea7), blade ignite / retract and the perfect-deflect flourish v2 (8eb936dd), the Grunt fighting idle retargeted from Nicholas's Fab pack (b5ec3925), the sprint from the follow camera (bef5364b). **To close (#124):** T5's hooks for those clips, the drone tilt / look / tumble, a possible stray attack after CONTINUE by mouse click, the flat orange placeholder disc near Grunts in first person (T5); the sprint frame with the blade behind the helmet (animation builder, reproducing).
- **(Nov 14 17:30)** **Stays 🟨 (Ryan's side).** On main bb7a8ba5: T5's hooks for the blade ignite / retract, the perfect-deflect flourish v2, HeavyHit for Grunt and Trooper and Stagger for the Heavy on a returned bolt, the Grunt fighting idle (f8ed544d); the camera-blade guard (40184e39: the first-person blade hidden in third person; the sprint frame fixed). **To close (#124):** those hooks seen in a played fight; drone tilt / look / tumble; the stray attack after CONTINUE by mouse click and the orange placeholder disc near Grunts (T5).
- **(Nov 14 18:30)** **Stays 🟨 (Ryan's side).** On main 48a6fd97: the Grunt telegraph call and the blade retract shrink (T5, compiled, mostly unplayed). **To close (#124):** T5's hooks seen in a played fight; drone tilt / look / tumble; the stray attack after CONTINUE; the orange disc near Grunts.

#### ART-02 · Visual asset inventory & sourcing plan · `✅`
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
- **(Nov 14 05:00)** **State:** the never-cook rules (134 unreferenced superseded packages, editor-only `PL_*` pipelines, two walk-test classes; 146cbb65) are on main 29a15f21 and gave a clean cook of ddc50a2f; the Oodle RDO trial is unproven. The cook diff against `CREDITS.md` can start (import builder).
- **(Nov 14 06:00)** **State (🟨):** the package check of `CREDITS.md` against the ddc50a2f cook is on main 50c4af7a (44892c02: not-shipped notes on Ryan-side rows, the Meshy pilot clips row, the Epic template content note). Next: the same check against the 50c4af7a cook and rows for batch U's new assets (import builder).
- **(Nov 14 06:30)** **State (🟨):** the 50c4af7a cook (2,595 packages, 0 errors, 0 warnings) is the next reference for the `CREDITS.md` package check; rows for the assets of batches U to X are still to add (import builder).
- **(Nov 14 10:30)** **Stays 🟨 (Ryan's side):** **credits rows for the assets of batches U to AE** (import builder).
- **(Nov 14 11:30)** **🟨 → ✅:** `CREDITS.md` rows for the assets, voices, motion and tools of batches U to AG are on main (c63633a6, T6, batch AI); later additions get their row in the commit that adds them (#123).

#### ART-05 · Space environment & galaxy dressing · `✅`
**🟦 Ryan since Nov 13 (environment moved to Ryan's side).** The space sky, nebulae, galaxy dressing, lighting and post-process of `L_Galaxy` are built by engineering builders with Grok Bot's panoramas and textures, under the Lead. Today: the 8K sky and code-placed dressing are in the game. Nick's Hero sky work is a reference we may adopt, not a dependency.
- **Space assets:**
  - `BP_GalaxySkybox`: layered nebula, stars, sun with lens flare.
  - `BP_PlanetVisual`: rings, atmosphere rim, Verdance and Cinder variants.
  - Nebula volumes, a speed-dust field, asteroid-field kits (`procedural_rocks.py`), station ruins.
- **Galaxy map:** dress `L_Galaxy` on your own actors (One File Per Actor): lighting, post-process
  and scale cues.
- **(Nov 14 05:00)** **State:** the region set pieces are closed at r4 for tonight (batch U's gate); region beacon and haze and the range flare are with the effects builder (branch).
- **(Nov 14 06:00)** **State (🟨):** the checkerboard on the moon shards, hulks and hulls placed through instanced meshes is fixed on main 50c4af7a (83a231c9), not yet seen in the game; the set pieces are closed at r4. Region beacon and haze are not yet accepted.
- **(Nov 14 06:30)** **State (🟨):** the checkerboard fix on the moon shards was seen in the running game by the structures builder (region 6 at 1 km); shard face patches on main 6cc4f254. Region beacon and haze are not accepted.
- **(Nov 14 10:30)** **Stays 🟨 (Ryan's side):** **Ice Drift and Outer Dark re-judged** (T6); region haze and beacon dropped.
- **(Nov 14 11:30)** **🟨 → ✅:** Ice Drift and Outer Dark accepted by Ryan's Lead; regions R1, R2, R4, R5, R6 were already good (#115).

#### ART-13 · Lighting, post-process & performance-minded art · `✅`
**🟦 Ryan since Nov 13 (environment moved to Ryan's side).** Lighting, fog, sky, atmosphere, colour grade and post-process on every level (galaxy, Verdance, Cinder, the Dominion interior, the menu) are built by engineering under the Lead, including the atmosphere's look for the planetary descent (ENG-38). Performance budgets stay as written.
- Lumen and Nanite settings per level, scalability presets wired to Settings, Niagara budgets,
  foliage culling.
- Hold the art budgets in `art-direction.md` and keep VFX and environment complexity within a steady
  60 fps on mid hardware. Profile with Ryan before the freeze.
- **(Nov 13 evening) Still missing:** the lighting, fog, sky and grade pass: first passes on main 6fe3e90f (671223ca, 03cdafc4, d89a627a) (environment builder,
  map World Settings claimed first). Done when each of `L_MainMenu`, `L_Galaxy`, both planets (orbit, descent, play zone)
  and `L_Dominion_Interior` (normal, alarm and collapse states) has a before / after capture at gameplay distance, the
  squad / enemy colour code still reads, and the frame rate holds (ENG-28). The scalability preset is wired with ENG-45.
- **(Nov 14 04:00) State:** the alarm pulse and collapse flicker states for `L_Dominion_Interior` are built on eng/env
  (142abfa5), not on main and not captured. The descent white-out post-process is on main 6afa3816.
- **(Nov 14 05:00)** **State:** `UTDInteriorLightSubsystem` (alarm pulse and collapse flicker on `OnInteriorMoment`) and the exposure ramp are in batch U's gate, tested headless, **not seen live**. Per-level before / after captures still queued.
- **(Nov 14 06:00)** **State (🟨):** `UTDInteriorLightSubsystem` (normal, alarm pulse, collapse flicker) is on main 50c4af7a; the builder drove 7 tagged lights live for its sheet; not seen in a played run. Per-level before / after sheets exist on eng/env.
- **(Nov 14 06:30)** **State (🟨):** interior light states are on eng/env, waiting on the s13 sheet; nothing new on main.
- **(Nov 14 10:30)** **Stays 🟨 (Ryan's side):** interior light states seen; **Nicholas's findings 1, 5, 7, 8 (menu and opening shot) and the per-level before / after** (environment builder).
- **(Nov 14 12:30)** **Stays 🟨 (Ryan's side).** The environment builder's lock turn (e3b5edea, on main AT): #145 NOT GOOD (the ring now stops at the planet limb, but a teal rectangle still shows inside the right limb; next it isolates the atmosphere shell and the cloud layer); #148 unverified (both after-frames black because the intro timing moved; recapture at the eclipse shot's new time); #158 provisionally good (closed domed undersides on the canopy clumps; closes on a frame from under a clump at eye height); the bay look GOOD (#126). **To close:** #145, #148, #158 and #125.
- **(Nov 14 13:30)** **Stays 🟨.** With the environment builder: #145 (the teal patch), #148 (the opening shot), #158 (canopy undersides), #125 (per-level before / after).
- **(Nov 14 14:30)** **🟨 → ✅.** #125 closed by the Lead as seen GOOD: per-level after frames on the g3 poses for the galaxy, Verdance, Cinder and the Dominion interior (normal, alarm, collapse; dbbb200f), the collapse lighting readable (1cea37ed: the brightest 40 % of a room's lights brown out to 0.45, 20 % dead, stronger strips; floor, cover and exit door read at 1.6 s), #145 the menu planet's teal patch and #148 the opening shot both closed GOOD. Leftovers are polish.

#### ART-17 · Walkable hard-surface structure kit (R-043, Ryan Nov 13 evening) · `✅`
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
- **(Nov 14 04:00) State:** both outposts sit 3 cm above the ground with every piece lifted with the floor
  (`BP_Outpost_Site_Verdance` / `_Cinder`; f19802f0, main ddc50a2f), after the Verdance floor showed terrain; the walk
  test with the real pawn is pending (T5). The dig cabin is placed by the Dress pass. No site has been seen by Ryan.
- **(Nov 14 05:00)** **State:** walkable survey stations are built on the structures branch, not on main and not walk-tested with the real pawn (T5).
- **(Nov 14 06:00)** **State (🟨):** `BP_SurveyStation_Verdance` and `BP_SurveyStation_Cinder` (floors 3 cm proud, manifests with footprint, console stand and ramp foot) and the camp canopy r2 are on main 50c4af7a. Not placed or walked with the real pawn yet (T5 with the structures builder). New: a bridge over the Cinder magma channel (structures builder).
- **(Nov 14 06:30)** **State (🟨):** both survey stations were walked in the running game by the structures builder; on main 6cc4f254 (38d032e2): the magma bridge (`SM_Kit_MagmaBridge`, walk-tested; T5's route test crosses it over 11 legs), the Cinder station rack off the deck, a fuller camp canopy. A walk by T5 or T7 in a played run is still open.
- **(Nov 14 09:30)** **Structures builder (09:40):** bridge exit GOOD (no geyser throw); doorway chamfer and anchor pieces on main (Y); **the anchor charge mesh stretched: NOT GOOD**; **the hangar start NOT GOOD** (not taking effect on the staged start); the collapse NOT PLAYED. Stays 🟨.
- **(Nov 14 10:30)** **Stays 🟨 (Ryan's side):** survey stations and the magma bridge walked GOOD; **the hangar start after a natural docking** (fix in AE; T7 to play).
- **(Nov 14 11:30)** **🟨 → ✅:** played good by T7 (real input) on main 473543fb: approach from 1205 m, docking 7.7 s, travel to the interior, hangar arrival 3 s, the pilot starts beside the parked ship facing the door, clear of it (#120). Earlier: the walkable kit on both planets and inside, played.

#### ART-18 · Story set pieces and clue props (CORE) · `✅`
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
| Verdance field camp, Cinder mining camp; landmarks (relay mast, Null derrick) | Both planets | on main 61404a79 (d308d8d6, 3d495c16; builder captures at 5 m and 30 m); mast r2 in batch P's gate |
| Space set pieces (patrol wreck, research hulls, survey rig, hollowed rock) and region pieces (picket ring, hulks, scar rig, moon shards) | Space regions (ENG-39) | r1-r4 on main; region pieces r3 in batch P's gate; placed by T6's regions, not on main |

**(Nov 14 03:00)** No one is working on the insignia clue or the *Aftermath* light row: both queued on `CLAIMS.md` this
hour (structures builder for the insignia decal, effects builder with T4 for the light row). Dress placements for #40
and #37 are on T5's branch.
**(Nov 14 04:00)** The insignia clue is built and on main ddc50a2f (`SM_Kit_InsigniaHull`, `SM_Kit_InsigniaPanel`,
`MI_Kit_InsigniaScorched`; f19802f0) but **not yet placed** (T5). The Dress pass placed 49 story props on Verdance and
50 on Cinder (2767300b, main ddc50a2f). The *Aftermath* light row is a spec on the effects branch.
- **(Nov 14 05:00)** The *Aftermath* light row is on the effects branch; the insignia decals are still not placed (T5, queued); T5's emergency strip tags are on a branch.
- **(Nov 14 06:00)** **State (🟨):** the insignia decals are still not placed (T5); the *Aftermath* light row effect is not yet accepted, and its pulse getter is on lead/int-v.
- **(Nov 14 06:30)** **State (🟨):** the *Aftermath* light row is accepted and in batch X; its `AftermathLightRow` slot (T4) is on main 6cc4f254 and the pulse getter on main 94b56d11. Escape debris as art on main 6cc4f254. The insignia decals are still not placed (T5).
- **(Nov 14 09:30)** Anchor charge and collapse pieces on main (batch Y, Z); the charge stretched (NOT GOOD), the collapse NOT PLAYED. Stays 🟨.
- **(Nov 14 10:30)** **Stays 🟨 (Ryan's side):** **the anchor charges and collapse (AE), the gate conduits (AE), the console recordings** (T5, T7, T3).
- **(Nov 14 11:30)** **Stays 🟨.** Anchor charges and the collapse run GOOD (T7); the Cinematic 03 relay drone seen good by T5 (the optic dark to glowing blue, lifting off the deck; e4f599ff, on main e60b3554); the Cinder conduits seen with the new art. **To close: the collapse pieces seen from the shore** (structures builder; its batch AE capture b8e175b4 is on main, the verdict is not posted) **and the consoles heard** (T5, ENG-42 #96). The three-pulse sound is Nick's ThreePulse slot (#119).
- **(Nov 14 12:30)** **Stays 🟨.** Nick's side found (#119, natural Cinder pass on b82ed996): the anchor collapse freezes half standing when the escape succeeds (MINOR); T5's fix 5158b170 (success only cancels the deadline; the tower falls fully over 6 s with its sections and debris; a retake stands it back up) is on main AU ebe073ad, not yet seen. **To close: the collapse seen from the shore after a fast escape** (T5 or the structures builder; retest by Nick's side welcome). The consoles were heard (ENG-42).
- **(Nov 14 13:30)** **Stays 🟨.** The anchor collapse is still NOT GOOD (#119): the structures builder filmed it from the shore twice (before and after T5's 68607c8c in batch BA: sections at native size, left as wreckage, debris 12 s): the tower sinks and leans but no section or debris shows. Likely cause: the sections drop 4 m from the tower's axis, inside its plinth and collision. **To close:** T5 moves the drop point 12-15 m out and checks at 10-15 s; the structures builder re-films. The story console mesh `SM_DI_StoryConsole` is on main (BC, #96); its frame is T5's.
- **(Nov 14 15:30)** **🟨 → ✅.** #119 closed as played GOOD: the Cinder anchor collapse with its pieces placed at their own drop points and resting on the traced ground (d13c8f4f, b51963d7). Polish: the story console look from 1.4 m (structures builder, #96).

### Boarding finale (WANTED since Nov 13, Ryan; story acts 4 and 5; was stretch)

#### ENG-29 · Docking & boarding transition · `✅` WANTED (Nov 13)
**Story (act 4):** the squad breaks through the field and boards. Plan (T4, Nov 13): docking bay opens when the defences are down, an approach cone with a tractor assist that never fully locks before the mouth, one travel into the interior, wingmen hold outside.
`ATDDockingBay` approach cone with a tractor-beam assist that reduces control, then
`TravelTo(DominionInterior)`; the hangar landing hands off to first person (§8).
- **(Nov 13 evening)** Built by T4 (docking and the escape flight are done on `lead/int`, `CLAIMS.md`). Still missing
  (Cinematic 07, §26): the `C7_*` pair fired on docking commitment; attack pressure off from commitment to the travel;
  confirm the ship docked in is the ship flown out and that the wingmen rejoin (T4, T5); the tractor beam effect and the
  docking shot (ART-16); the bay mouth matching hull r1 (ART-18). Done when docking is played from each of the three ships.
- **(Nov 14 03:00) State:** T4's near-field capture (80 degrees inside 400 m), the approach waypoint 800 m out on the
  axis and its test (0a4e7501, b6f967d4) are on main f39a8372. Docking has still not been flown by an agent on main
  (#90 no. 19); `C7_*` lines not started.
- **(Nov 14 05:00)** **State:** T4 played docking on its own branch (agent-driven); T7 has not reached it. On branches: the tractor beam effect; T6's docking-cone keep-out for the field.
- **(Nov 14 06:00)** **State (🟨):** unplayed by T7. T6's docking-cone keep-out (no set piece inside the tractor cone or the near-field capture) is on main 50c4af7a with a test; the tractor beam effect is not yet accepted.
- **(Nov 14 06:30)** **State (🟨):** in T7's play on main 50c4af7a the docking route opened after the assault; **docking itself is unplayed on main; T1 is playing it now**. On main 6cc4f254 (tests only): `PlaySequence(Docking)` so `C7_Moss_WayOut` airs and the `TractorBeam` slot. In batch X: the flown ship's pilot goes on at any level change, so docking carries the flown ship (8e26e739, new test). The tractor beam effect is not accepted.
- **(Nov 14 09:30)** **🟨 → 🟢:** docking carry GOOD (T1), the docking bay GOOD (T4 with T1 live), the docking pace played at 12-13.5 s from 1.5 km (was 38-44 s); the tractor / docking HUD plates on main 80a2ee4d. Not ✅: **the hold for the Docking lines is NOT GOOD yet** (setup-caused, T1 retrying), the tractor beam effect not accepted.
- **(Nov 14 10:30)** **Stays 🟢:** docking GOOD with all three ships at 12-13.5 s (T1, T4); **the tractor beam seen in docking NOT PLAYED** (accepted, slot filled on main AB, 4606b4f9; T4); the hold for the Docking lines not replayed (T1).
- **(Nov 14 11:30)** **Stays 🟢.** **To close: the tractor beam seen in docking** (T4, lock turn) **and the C7 lines heard to the end** (#110).
- **(Nov 14 12:30)** **Stays 🟢.** The C7 docking and arrival lines aired (T4, #96); the tractor beam reads only as a faint ring at the bay mouth (NOT GOOD, #105; effects builder). **To close: the tractor beam seen in docking** (#110).
- **(Nov 14 13:30)** **Stays 🟢.** **To close:** the tractor beam seen in docking once the effects builder's cone and motes land (#110, #105).
- **(Nov 14 15:30)** **🟢 → ✅.** #110 closed as played GOOD: docking with the docking-line queue fix (b5472653) and the beam fading to nothing at the bay mouth (45a7652d). Polish: the tractor beam v2 seen in docking once wired (effects builder; ART-16, #105).

#### ENG-30 · Dominion interior encounters & core fight · `✅` WANTED (Nov 13)
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
- **(Nov 14 03:00) State:** played end to end by an agent on main 6fe3e90f, but the reveal gate has no verdict on a
  merged build (T7's run skipped it by a debug move). On main since: the conduit count on the HUD, per-type crew deaths
  and attacks. On T5's branch, not main: the interior music events (core chamber and reactor fight, which the music
  fallback is waiting for), slam / contact fixes. Open: conduit world markers, the interior spawn against the Rift's
  tail (#90 no. 4, no. 12), the defenders trio (N-052).
- **(Nov 14 04:00) State:** on main 6afa3816: the interior music moments and the `DA_ReactorCore_FX` slots at the
  reactor and escape events (9f964b30); interior lights tagged `TD.InteriorLight` (main ddc50a2f). On branches: T1's
  forwarding to the music bridge, the alarm and collapse lighting (142abfa5). The reveal gate still has no verdict.
- **(Nov 14 05:00)** **State:** T1's forwarding of the interior moments to the music bridge and the alarm and collapse lighting are in batch U's gate (the lighting not seen live). Not replayed by an agent since batch L.
- **(Nov 14 06:00)** **State (🟨):** not played by anyone on batches M to U. On main 50c4af7a: `UTDInteriorLightSubsystem` on `OnInteriorMoment` and the music forwarding. On branches: emergency strip tags (eng/t5), collapse roles (eng/env). Reactor conduit markers and the interior spawn are still open (T5).
- **(Nov 14 06:30)** **🟨 → 🐞.** T7 played the interior on main 50c4af7a (agent, real keys) from the hangar to the Warden. **NEW MAJOR (T5 + T3, #90): dying in the Warden fight restarts the scene with the objective already at the reactor conduits and no Warden, so one death skips the mini-boss.** Not reproduced headless; a Warden retake test and objective / room log lines (9e62d3da) are in batch X; T7 is reproducing it live. **MAJOR still open (T5):** the Heavy's guard never breaks to quick swings and no lunge hint shows; the arena took 17-18 takes. MINORs (T5): the hangar spawn directly behind the parked Rift; the reactor conduits carry no markers. Emergency strip tags are on main 94b56d11. Unplayed: the reveal, the reactor conduits and the heart. Back to 🟨 when the retake fix is on main and replayed.
- **(Nov 14 09:30)** **🐞 → 🟨: both MAJORs closed in play.** T7 with real keys on batch Y + T5's arena fix: the interior hangar to the Warden GOOD, the Warden retake GOOD (four CUTs, the Warden back each time, the objective unchanged), the Heavy's guard and lunge hint GOOD, mercy GOOD, the Warden fight GOOD (beaten on take 5). In batch Z on main 80a2ee4d: arena spawns away from the retake spot, Face to Face at the reactor. **Not 🟢: the reactor conduits and heart on foot, the reveal and everything after are NOT PLAYED** (T7 next). MINOR: the staged interior start still spawns the pilot behind the ship.
- **(Nov 14 10:30)** **Stays 🟨:** corridor, arena, Heavy, Warden, reactor conduits 4/4 with markers, the heart and the Face to Face beat GOOD (T7, real keys, batch Z); **NOT PLAYED: the console recordings and the reveal (T3), the hangar start after a natural docking (T7); the future-pilot defenders are unbuilt (T5, decision at 14:00, drones the fallback)**. Placeholder Face to Face words await Nicholas (polish).
- **(Nov 14 11:30)** **🟨 → ✅:** played good by T7 (real keys, two sessions; #129 closed 11:29): corridor and arena cleared with 0 CUTs, the Warden beaten on take 1 (16 s), the reactor room with the two future-pilot defenders beaten on take 1, Face to Face, conduits 4/4 with the blade, the heart exposed and destroyed. Leftovers (polish, AAA list): the defenders are too easy for the reveal (T5 raises them to 200 health with a guard, Lead's decision); their look not captured (T5); the scripted walker wedged at one doorway from the Warden room (T5's doorway markers 128a5907, batch AN; T7 replays); the two Overcome lines are placeholders (Nicholas, ART-11). The console recordings and the reveal are ENG-42's (#96).

#### ENG-31 · Escape run & escape flight · `✅` WANTED (Nov 13)
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
- **(Nov 14 03:00) State:** on main f39a8372, awaiting a live play: the escape flight as a real beat (clear at 3.5 km
  or more, about 40 of 60 s, debris every 0.4 s, `G9_Wren_PullAway`; 0a4e7501) and the escape clock with an EXIT marker
  on the run (7fb67c3d). The escape music slot is empty: the Ritual track Nicholas chose is not imported on main.
- **(Nov 14 05:00)** **State:** the Dominion escape debris set is on main 29a15f21 (captured at 100 m by its builder). T4 played the escape flight on its own branch: victory at 41 s of 60. Replay on main pending.
- **(Nov 14 06:00)** **State (🟨):** not played on main; T4's own branch play (victory at 41 s of 60) is the only run of the escape flight.
- **(Nov 14 06:30)** **State (🟨):** still unplayed on main. On main 6cc4f254 (tests only): the ClearSpace roll call at the escape clear and escape debris as art (capped 28 m, block fallback).
- **(Nov 14 09:30)** **The escape run and reboard NOT PLAYED**; the escape flight was GOOD when T4 played it earlier (41 s), not replayed since batch W. The line "Thirty seconds. Keep going." (Moss) is on main 80a2ee4d, not heard. The Ritual escape track is Nick's side's next audio task. Stays 🟨.
- **(Nov 14 10:30)** **🟨 → ✅ (10:15 rule):** **the escape run GOOD** (T7, real keys, batch Z: the ship reached in about 20 s, boards and travels on its own) and **the escape flight GOOD** (T7; cleared with 25 s left by boosting straight). Leftovers: the flight is easy (tuning, T4, polish); the escape clock is ENG-36's (T2); the "time failing" sound is the Ritual track (Nick's Audio, ART-10); afterimages stay AAA 22. MINOR (T5, T3; polish): dying in a gap between scenes sends the pilot to the hangar start.

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
| ART-14 submission | 🟨 November 14 itch/tools/pre-jam disclosure and Glass Box drafts e0b543c0/2d549ba4; actual 200-second demo 11b8ab97; 50-second gameplay-first rough cut 2690aad5; final public page/trailer approval pending | Trailer capture pipeline, `CREDITS.md`, `DEVLOG.md` | **Priority 5** |
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

#### ART-01 · Art direction & style guide · `✅`
- **Content:** a one-page style guide with palettes, the faction shape language, readability rules
  (player projectiles in squad colours, enemies in red and orange), lighting mood per level, and
  the **Director's Cut visual language** (cuts, letterbox, slate cards, grade; C-03).
- **Reference board:** inspiration only; no copying of existing IP.
- **Gate:** both humans agree on the style before mass production.
- **(Nov 13 22:45) State:** the guide `docs/visual/Art-Direction.md` is on main (2d5d5928); both humans have not agreed it yet (`🟨`).
- **(Nov 14 12:30)** **From Nick's side (#132, 17:28 UTC):** Nicholas approved the written guide (`docs/visual/Art-Direction.md` at c58285f5) in his coordinator chat ("Approve the written guide"); this approves the written reference only, not assets, audio or the HUD. **Waits on Ryan's agreement** recorded on #132; stays 🟨 until both humans agree.
- **(Nov 14 13:30)** **🟨 → ✅ from Nick's side's own report** (#132 closed by Nick's side 17:35 UTC): the gate "both humans agree" is closed with Nicholas's direct approval of the written guide and Ryan's side's agreement recorded by Ryan's Lead under Ryan's standing delegation (Nick's side's words: "not represented as Ryan personally typing approval"). The agreement covers the written guide `docs/visual/Art-Direction.md` (c58285f5) only, not any asset, sound or in-game result. Ryan can overrule on #132 or add his own words.

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
- **(Nov 14 13:30)** **From Nick's side (#133):** the industrial reward candidate is preserved at 59ec2f79 on `nick/ui-reward-industrial-1113`; its captures were never supplied. Ryan's side accepted the task: T2 takes the on-foot Gravity Well reward, dismissal, the next unlock and the device-key refresh at 1280x720, 1920x1080 and 2560x1080 on an isolated build; nothing is activated before Nicholas decides.

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
- **(Nov 14 13:30)** **From Nick's side (#134):** Nicholas's answer on the menu-loop join was "Send to Lead for guidance"; it is forwarded to his Lead Coordinator; the loop join stays unapproved.

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
- **(Nov 14 13:30)** **From Nick's side:** no change; the Wren takes (#171) still wait for Nicholas's ear.

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
- **(Nov 14 13:30)** **From Nick's side (#136):** the existing Director-card candidate (paired 2946312b, host cf55d3bc, evidence release `evidence-director-cards-cf55d3bc`) is presented to Nicholas for a decision on the look of the slate / TAKE, CUT! and pilot cards only; no decision yet.

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
- **(Nov 14 13:30)** **From Nick's side (#138):** a bounded re-review of T1's bridge fix fca00389 (on main BE 98c96926) found all three preserved concerns source-addressed and no new defect. T1 adds the three check cases named there (compiled under the compile-only gate). The Ritual import, cue hookups and Nicholas's listening approval stay open.

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
- **(Nov 14 12:30)** **From Nick's side (#139):** a bounded coverage matrix of batches U to AE (11 ranges, 29a15f21 to 9155a6dc; private report), then one source review of batch U's story carry across travel (29a15f21..50c4af7a against c58285f5): one MAJOR (an old run's required recording can enter a fast New Game), routed to T3 by Ryan's Lead. Stays 🟨 (the other ranges are not reviewed).
- **(Nov 14 13:30)** **From Nick's side (#139):** the travel-carry MAJOR is source-addressed at 886ee784 (on main AY d1ae9b18), runtime check pending (T3); a new MINOR, the cached waves 3-5 fallback ignoring `td.Music.Fallback 0`, routed to T1 and fixed in ecb6776c (batch BI, compiling); a bounded Ground AE review found no new defect.

#### ART-21 · Playtest checklists and per-act test scripts (assigned to Nick's side by Ryan's Lead, Ryan, Nov 13) · `🟨`
- One script per act (front end and Act 1, Verdance, waves 3-4 and Cinder, assault and docking, interior and reveal,
  reactor, escape and ending) in `docs/visual/` (Nick's docs folder): the steps a tester performs, what must be seen and
  heard at each beat (lines by id, music, HUD state), the retake and Continue checks, the expected time.
- Plus the presentation checklist of AGENTS.md §11 per act (silhouettes, colour code, clipping, sound in context).
- T7 runs them in the playtest lane (ENG-46); the humans use them at gates G1-G4.
- **Done when:** T7 has run every script once on `main` and logged the result in `docs/eng/PLAYTEST-*.md`.
- **(Nov 14 13:30)** **From Nick's side (#140, N-099 archived):** Batch 3 delivered (PDF and originals); the natural campaign cleared wave 5 on take 1 and entered the Dominion assault on b5d1549c; next Lead play: generators, docking, interior, escape and victory.

### STRETCH: boarding finale visuals

#### ART-15 · Dominion interior kit & core visuals · `✅` WANTED (Nov 13; Ryan's side: environment and structures builders)
- **Interior kit:** modular pieces on a 400 cm grid (corridor straight, corner, T, doorway, shaft,
  catwalk, door) in gloss black with red strips; giant eye lenses that track the player.
- **Core chamber:** the white-light reactor with rotating rings, plus the collapse and alarm states
  for the escape run.
- **(Nov 13 evening) Correction:** WANTED, not stretch (Ryan, Nov 13), and **🟦 Ryan's** since the afternoon of Nov 13 (§1:
  Environment and VFX moved). The kit is built walkable per ART-17 (50 cm grid, trim sheet; the 400 cm module above becomes
  eight grid cells); lighting and alarm and collapse states by the environment builder (ART-13); the reactor rings turn
  (ART-08). State: interior played end to end on the engineering kit; kit r4/r4b interior and bay on main 6fe3e90f (8ce5c3fb, 71fefa4b); r8 01832cd7 accepted and r9 / r10 on main eb1bf334 (batch M), awaiting agent play; plans on main (#77, #82) (`🟨`).
  **(Nov 14 03:00)** Still awaiting agent play on main; alarm and collapse lighting states not started.
  **(Nov 14 04:00)** Alarm and collapse lighting (`UTDInteriorLightSubsystem`, on `OnInteriorMoment`; 142abfa5) is on eng/env, not on main; the tractor beam is a spec on the effects branch.
  - **(Nov 14 05:00)** Alarm and collapse lighting (`UTDInteriorLightSubsystem`) is in batch U's gate, tested headless, not seen live.
- **(Nov 14 10:30)** **Stays 🟨 (Ryan's side):** the interior played GOOD as a space; **the bay mouth (#54) unbuilt** (structures builder).
- **(Nov 14 11:30)** **Stays 🟨.** Corridor, arena, Warden and reactor room played good (ENG-30 ✅). The bay mouth was engine placeholders washing out to white; `SM_Kit_BayMouthFrame` and `SM_Kit_BayGuideLight` are on main in batch AL (bffb1036), with the bay pocket lights toned down (60974009) and the placeholders dimmed (45d5652c); T4's hook of the new meshes (c94f1795) is in batch AN, compiling. **To close: the bay seen in a docking approach with the ship readable** (structures builder captures, environment builder checks exposure; #126).
- **(Nov 14 12:30)** **Stays 🟨.** The bay mouth seen good in game (T4 frame at 94 m, looked at by the Lead: the dark frame with ember strips, a clear opening, the interior readable); the environment builder's before / after: with the bay look on (interior lights x0.3, exposure -0.5, bloom 0.25) the structure and ship read clearly, GOOD (#126). **To close: the guide-light housings seen up close** (T4). Suggested polish: T4's four frame bars at about half emissive.
- **(Nov 14 13:30)** **🟨 → ✅** (#126 closed 13:13 EST): seen GOOD in the running game: the dark bay mouth frame with ember strips and a clear opening (T4 at 94 m and 159 m while gliding), the interior readable and no longer blown out (environment builder's before / after with the bay look on), the guide lights flanking the approach at about 1.4 km with no bloom or white-out; the Lead looked at the frames. A close-up of a guide-light housing is polish.

#### ART-16 · Boarding & escape cinematics/VFX · `🟨` WANTED (Nov 13; Ryan's side: effects builder)
Tractor-beam docking shot, hangar landing, core-destroyed sequence, escape debris and explosion
VFX, and the squad's launch from the hangar.
- **(Nov 13 evening) Correction:** WANTED and **🟦 Ryan's** (effects builder, T3 for camera beats). For the complete game:
  the tractor beam, the reactor heart's death, interior collapse and debris, the hangar-mouth launch, and the breakup
  (ENG-37). The docking and core sequences play as the travel plus lines (ENG-42), so no Level Sequence is required.
  State: debris and explosions exist in the escape flight (T4); the rest not started (`🟨`).
- **(Nov 14 05:00)** The tractor beam is on the effects branch; the escape debris set is on main 29a15f21.
- **(Nov 14 10:30)** **Stays 🟨 (Ryan's side):** **the tractor beam unseen in docking and the breakup not captured** (T4, effects builder).
- **(Nov 14 12:30)** **Stays 🟨 (Ryan's side).** The breakup: the outside camera GOOD, the explosions NOT GOOD (small sparks); the flash and hull chunks on main AQ, unseen; hull-scale explosions by the effects builder. The tractor beam NOT GOOD seen end-on (#105).

---
- **(Nov 14 13:30)** **Stays 🟨.** The breakup itself is seen GOOD (#112); the hull-scale explosions and the tractor beam remain with the effects builder (#111, #105).
- **(Nov 14 15:30)** **Stays 🟨.** The breakup is GOOD (#112); the victory backdrop's wreckage is in batch BQ, not on main. **To close:** the tractor beam v2 seen in docking and the hull-scale explosions (effects builder; #105, #111).
- **(Nov 14 16:30)** **Stays 🟨.** The victory backdrop's wreckage is on main (a7789713). **To close:** the tractor beam v2 seen in docking (T4, effects builder) and the hull-scale explosions (#111).
- **(Nov 14 17:30)** **Stays 🟨.** T4's beam fix is on main (51f45b35). **To close:** the tractor beam v2 seen in a real docking (effects builder, T4) and the hull-scale explosions (#111).
- **(Nov 14 18:30)** **Stays 🟨.** Tractor beam v3 visible in a staged spawn. **To close:** seen in a real docking; the hull-scale explosions (#111).

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

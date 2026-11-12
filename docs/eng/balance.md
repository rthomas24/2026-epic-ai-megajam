# Balance & difficulty (Teammate 4): ENG-27 groundwork

Code: `Source/unrealGameJam/TriadDrift/Waves/Balance/`. Output: `Saved/Balance/balance_<Difficulty>_<Skill>_<time>.csv|.md`.

## Difficulty presets: `UTDDifficultySettings`
Project Settings > Game > Triad Drift Difficulty (`DefaultGame.ini`). `SetCurrent(Story|Standard|Ace)` is
runtime state for the front end (default Standard). It applies when enemies and waves are created:

| | Story | Standard | Ace |
|---|---|---|---|
| Enemy hull / shields / frontal shield / bay & turret hull | ×0.75 | ×1 | ×1.25 |
| Enemy weapon damage (ships, turrets) | ×0.6 | ×1 | ×1.3 |
| Enemy aim error | ×1.6 | ×1 | ×0.6 |
| Wave size (pack sizes; small-ship pack counts; big ships never multiplied) | ×0.75 | ×1 | ×1.25 |

Persisting the player's choice in the save is T5's (flow) side.

## The balance run
`.\Tools\td-test.ps1 -Filter TDBalance` plays waves 1-5 headless at 30 fps, once per difficulty × pilot skill
(9 runs, roughly 30-100 s each). Run one with e.g. `-Filter TDBalance.Standard.Average`. It isn't in the
default `TriadDrift` suite.

- **Squad:** T3's real squad manager, with three ships and two AI wingmen.
- **Player's ship:** flown by `FTDBalancePilot` through the same calls the player controller makes:
  `SetFlightInput`, the weapon triggers, `RequestSwitchNext`, and `UTDPowerComponent::TryActivate`.
- **Retakes:** a squad wipe counts as a new take of that wave, like the CUT retake.
- **Pilot skills** (`FTDPilotSkill`):

| | aim error | reaction | fire cone | switches | powers | goes for carrier bays |
|---|---|---|---|---|---|---|
| Weak | 7° | 0.9 s | 5° | never | never | no |
| Average | 3° | 0.4 s | 8° | every 25 s | every 12 s | yes (from 11 000 cm stand-off) |
| Strong | 1° | 0.15 s | 10° | every 15 s | every 6 s | yes |

- **Per wave it records:**
  - time, takes, kills against enemies (carrier launches included)
  - damage taken, ship downs
  - kills by ship and by damage kind
  - bays, turrets and carriers killed
  - pilot time on bays vs ships
  - percentage of time with an enemy ship on screen (45° of the nose, within 25 000 cm)
  - damage by source (enemy id / Turret / collision)

## Findings and tuning (Nov 12)
First run (features.md numbers): **wave 5 was unwinnable at every difficulty.** Story with an average pilot
killed 71 enemies of 97 in 300 s. Each carrier relaunched forever (3 bays × 3 fighters every 12 s, up to 12
alive), and its 4 point-defence turrets were the top damage source.

The diagnostics showed:
1. Launches never ran out.
2. The bot spent most of the wave on fighters and none on bays.
3. Once it did go for bays, it flew into point-defence range and died.
4. Wall Gunships dealt most of the damage in waves 3-4.

Changes, each followed by a rerun:

| Change | Why |
|---|---|
| `MaxLaunchesPerBay` 3 (new), carrier `MaxLaunchedAlive` 12 → 8 | a carrier wave must end |
| Carrier hull 2000 → **1500** (deviates from features.md) | wave 5 took 267 s with an average pilot |
| Point-defence burst 1.5 s on / 1 s off → 1.0 s on / 2 s off | the top damage source in wave 5 |
| Gunship heavy-bolt volley every 3.5 → 4.5 s, salvo 10 → 12 s | the top damage source in waves 3-4 |
| Enemy attack runs on the player steer 5000 cm in front of its nose (`TD.EnemyCrossInFront`) | the fight stays on screen (readability is judged) |

Current numbers, Standard:

| Pilot | W1 | W2 | W3 | W4 | W5 | Total | Notes |
|---|---|---|---|---|---|---|---|
| Weak | 42 s | 94 s | 128 s | 173 s | **not cleared** (300 s, 2 takes) | 12.3+ min | a weak player belongs on Story, where W5 clears in 250 s |
| Average | 19 s | 57 s | 73 s | 112 s | 164 s | **7.1 min** | target 8-10 min, no wave over ~3 min: met |
| Strong | 27 s | 55 s | 65 s | 68 s | 227 s | 7.4 min | ran before the PD and cross-in-front changes |

Average with and without cross-in-front: enemy on screen 55-75% vs 53-64% per wave. Story Average is
6.6 min and Ace Average 12.9 min, with Ace W4 needing 2 takes (both before the last two changes).

## Known gaps
- **Bot limitations:**
  - The bot's powers are "press when close"; it doesn't aim Rift's portals or use Bastion's dome
    deliberately.
  - It doesn't dodge mines, so bomber damage is pessimistic.
  - Wingmen are T3's real AI.
- **Missing from the run:**
  - There is no galaxy layout in the run (no asteroids), so obstacle effects aren't measured.
  - The Dominion finale isn't in the run yet (add it next to measure the whole space journey).
- **Spec deviation:** the carrier hull of 1500 deviates from features.md ENG-13 (2000). Lead: update
  features.md or revert.

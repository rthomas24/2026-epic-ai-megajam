# Approved M03 — live gameplay integration

UI/UX & HUD Agent. Branch `nick/hud-lancer-polish`; M03 visual checkpoint `aca9334`.
Nick approved this appearance as the current prototype. Lead source dispatch
`eade758`; safely synchronized base `db3472e`. No new layout or other-ship styling.

## Verified native checkpoint

Open **[preview.html](preview.html)** for16 actual-game captures. CRCs, IEND,
decompression, SHA-256 and every HTML link/image target verified. Images visually
inspected: fourteen1920x1080, one1280x720, one2560x1080;7,844,975 bytes total.
Chrome rendering was not verified. Existing Galaxy/ship placeholder art is unchanged.

- Editor Development build: **PASS**,14 actions,MaxParallelActions1,123.47s executor.
- **8/8 TriadDrift.UI tests PASS**,28.3s,exit0; automation forced exit expected.
  Four host tests (FrozenNames,N004AbilitySequence,NoWidgetWithoutSquad,ShowHideRules)
  and four plugin tests (ContractSchema,Intercept,LancerBenchmarkIsolation,SnapshotSafety).
- Actual Galaxy session: **exit0 / reason ok /41.2s /0 log errors**.
  Warnings:23 EditorDataStorageUI factory registrations,1 MCP licensing notice,
  3 Linker HLOD imports. Known Fab dependency descriptor warning during build.
- Real **WBP_SpaceHUD_C → TDSquadManager_0**, no preview mode/mock data.
  Development scene/real-target-spawn/progression-unlock commands were used;
  **TD.God enabled after frame01** to keep the bounded automated sequence alive.

See [verification.json](verification.json), [host-status.txt](host-status.txt),
[capture-manifest.json](capture-manifest.json) and [capture-driver.py](capture-driver.py).
The archived driver's root lookup was adapted for its review-folder location; the
captured run used the same command sequence at Saved/M03Gameplay-drive.py.

## Mock versus live data audit

The old `LancerVisualM03` screenshots are explicitly mock data, created only by the
isolated preview game mode. The actual gameplay host never calls preview mode.

| HUD information | Gameplay source already consumed by the native widget | Current verification |
|---|---|---|
| Three ships / selection / vitals | `ITDHUDData.GetSquadHUD` from `ATDSquadManager` | One/Two/Three switches verified;100% hull/shield all3. **Runtime damage/regen not verified under God**; native N004 death/respawn test passed |
| Boost / heat / ammo | Real movement/weapons squad snapshot | Lancer heat0→18%,ammo8→7 after fire (04); boost reduced with W+Shift (07), recovers later |
| Signature / earned ability | Same snapshot,0 just used /1 ready; None while locked | Q ram→cooldown→ready; real EMP unlock+E→charging. N004 attached-widget sequence passes at30/60/120fps |
| Current target / missile locks | `GetCurrentTarget`, `GetLockedTargets` | Real T acquired TDEnemyShip_8 at814,606 (03). **Locks stayed0; acquisition not verified** |
| Target vitals / lead / hostile markers | Existing reflected component getters | Real target panel rendered with10 markers; Intercept/FrozenNames pass; missile warning acceptance remains |
| Wave / enemies remaining | `GetWaveInfo` | Wave1 count changes during fight; Wave5 shows10 (12). Frame16 is immediate travel initialization, not settled-wave evidence |
| Flight / hyperdrive | `GetSpeed01`, `GetHyperdriveCharge01` | Speed100% after actual input (07); **hyperdrive spool/block-reason not exercised**,charge0 |
| Hit/kill / objective events | `OnHitConfirmed`, `OnObjectiveUpdated` | Subscriptions unchanged; live hit-caption centering not captured or claimed |
| Initial mission / objective markers | Event-only text; empty inherited squad objective markers | **Missing initial text reproduced**. Agreed Ryan getter/marker forwarding needs next safe sync/consumer verification |

The widget keeps its existing 10 Hz continuous-data sampler and UI-only smoothing.
No added per-frame gameplay queries, actor searches, competing host or extra timers.
Static ship names, labels and colors are presentation metadata, not fake telemetry.
Unknown data remains unavailable. Cooldowns display readiness; no seconds are invented.

## Existing engineering host — read-only

`UTDUIHostSubsystem` in `Source/unrealGameJam/TriadDrift/UI/` creates
`/Game/TriadDrift/UI/TriadHUD/WBP_SpaceHUD.WBP_SpaceHUD_C` (native fallback supported),
calls `SetDataSource(SquadManager)`, keeps it across switches, and removes it on
foot/travel/menu/disabled state. It suppresses the development HUD per world.
Its 5 Hz lifecycle checks are independent of the widget's 10 Hz data reads.

`TD.UI.NativeHUD 1` enables it only for the running process; `0` removes it;
`-1` follows the unchanged shared setting. `TD.UI.Status` logs class, provider,
target and decoded squad cards. Never edit shared defaults to run this test.

## Presentation, performance and remaining gates

720p/1080p/ultrawide retain readable labels, cards and safe margins with no new
HUD clipping observed. Existing **engineering fallback switch card overlaps**
flight telemetry/Rift card (08,09,12,16). Reported through Lead; Ryan reports fix
7c9c17d, **not included in this tested base**. Do not hide unrelated Director UI.
Host off/on, on-foot removal and return-to-space recreation verified (13–16).

Five capped60fps post-travel samples: frame16.67ms; game thread2.73–3.50ms;
GPU3.52–3.70ms; render thread median5.09ms with one31.08ms outlier. This is a short
sample, **not a sustained test or before/after regression baseline**. No new
materials, geometry, animations, input, provider, paint or polling changes.
Memory available before game4.82GiB, after exit4.85GiB. All our processes ended;
heavy slot released to Lead. No further heavy launch without a fresh grant.

Lead R014 agreed optional zero-argument reflected FText `GetObjectiveText() const`
on the same authoritative ITDHUDData source. Confirm implementation after a
Lead-controlled sync before consuming it; preserve event fallback/missing-function
guard/empty-text clearing. No private ground-system reflection or shared edits.

This is a verified technical checkpoint, **not full gameplay/Shipping acceptance**.
Ryan/Lead own mission producer, duplicate card fix, default-on, cooking and final
host integration. Remaining damage/regen, locks, hyperdrive and hit feedback checks
above need follow-up acceptance. Nick retains final visual approval.

Character campaign has not begun. Finish HUD publication/handoff, then explicit
Lead role dispatch, isolated branch/claims, skeleton/asset-route/resource clearance
before Lancer production. Enemy infantry waits Nick's Lancer model approval;
heavy, sensor special and coordinated interceptor retain separate approval gates.

## Reproduction / remaining acceptance instructions

Run only after the Lead grants the private8023 heavy slot, one job at a time:

1. Build `unrealGameJamEditor Win64 Development` with `-MaxParallelActions=1`.
2. Run **all** `TriadDrift.UI` tests (expected8: four plugin tests and four host tests).
3. Launch this worktree's actual `/Game/TriadDrift/Maps/L_Galaxy`, windowed,
   private MCP port8023, bounded process/result/log. Do not use the preview game mode.
4. Use the existing `Tools/td_mcp.py` ConsoleToolset: `TD.UI.NativeHUD 1`,
   `TD.UI.Status`, `TD.Input.Key Two 0.1` (Lancer), `TD.Input.Key One` / `Three`,
   `TD.Input.Keys W+LeftShift 2`, real fire/power inputs and `TD.Stage Wave5`.
   Observe before/after snapshots and actual viewport captures, not only command success.
5. Check locked/unlocked/cooling/ready ability states, all three ships' vitals,
   heat/ammo, target/lock/lead, wave/enemy count, speed/boost/hyperdrive, hit feedback
   and authoritative objectives where the agreed producer supports them.
6. Capture1920x1080,1280x720,2560x1080 via SceneCaptureToolset or ordinary native
   screenshots. Check labels, targeting, card footprints, safe margins and centering.
7. Check host removal on foot and recreation on returning to space; switch off/on.
   Record performance, actual process exit/result, errors and known warnings.
8. Validate PNG CRCs, dimensions, IEND and hashes; retain actual-game screenshots here.
   Release the heavy slot, freeze evidence, then request Lead commit/push audit.

Full local logs: Saved/Logs/M03Gameplay-build.txt, M03Gameplay-tests.log,
M03Gameplay-live.log and associated result/digest JSON. verification.json records
raw result/build hashes. Historical LancerVisualM03 evidence remains unchanged.
Ready for Lead's serialized owned-file audit/commit/push; no merge/default activation.

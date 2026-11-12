# Space HUD host (engineering side of Nick's HUD plugin)

Owner: Teammate 2. Code: `Source/unrealGameJam/TriadDrift/UI/`. Tests: `TriadDrift.UI.HUDHost.*`.

Nick's HUD lives in `Plugins/TriadDriftHUD` (read-only for engineering: change requests go through the
Lead to `MESSAGEBOARD.md`). It reads gameplay only through reflection and never installs itself. This
host is the part that puts it on screen.

## What it does

`UTDUIHostSubsystem` (a tickable world subsystem, game worlds only) checks every 0.2 s and shows the
widget when all of these hold:

- the switch is on (below)
- a local player controller exists
- the world has a squad manager (`ATDSquadManager::Get`)
- that controller's pawn is an `ATDShip`

So:

| Situation | Space HUD |
|---|---|
| L_Galaxy, flying any ship | shown |
| Ship switch | kept: same widget, same data source |
| On foot (planets, Dominion interior) | removed; the dev HUD draws (the widget has no on-foot half yet) |
| L_MainMenu, L_Transition (no squad manager) | none |
| Level travel | the world's subsystem goes, the widget with it; re-created on arrival |
| Switch turned off at runtime | removed |

- **Data source:** the squad manager (`ITDHUDData`), handed over with `SetDataSource`. The widget then
  samples it on its own 0.1 s timer and binds the event bus itself.
- **Dev HUD:** while the widget is up the host sets Core's `ATDDebugHUD::SetRealHUDActive(World, true)`,
  so the dev HUD draws nothing; it clears the flag on every removal and in `Deinitialize`.
- **Cut card:** the widget draws its own small "CUT TO / <SHIP>" card on a switch. It is not a full cut
  presentation, so `UTDDirectorSubsystem::bCutPresentationHandledExternally` stays off and T3's
  fallback name card still plays (see Known issues: the two overlap).

## The switch

- Project Settings > Game > **Triad Drift UI Host** (`UTDUIHostSettings`, `DefaultGame.ini`):
  `bUseNativeSpaceHUD` (default **off** until the Lead approves it from play), `SpaceHUDClass`
  (default `WBP_SpaceHUD`), `NativeFallbackClass` (`/Script/TriadDriftHUD.TDSpaceHUDWidget`), `ZOrder` 10.
- Console: `TD.UI.NativeHUD -1|0|1` (-1 = use the setting, 0 = force off, 1 = force on).
- Dev: `TD.UI.Status` logs whether it's shown, the class, the data source, the source's current target
  (and where it projects), and the widget's decoded squad cards.

## No module dependency, by design

The host doesn't include the plugin's header or link its module. It loads the class by path, calls
`SetDataSource` / `RefreshData` through `ProcessEvent`, and reads the widget's reflected `Ships` array.

Why: the plugin is read-only to us and changes under us. A header dependency would turn every plugin
edit into a possible compile break of the game module, and our module (loading phase Default) would
hard-link a PostDefault plugin. With reflection, a missing or renamed symbol degrades to "no native
HUD, dev HUD stays" plus one warning, and the frozen-name test fails on it.

## Tests

- `TriadDrift.UI.HUDHost.FrozenNames`: every name the plugin looks up on us (class paths, the zero-input
  getters on the squad manager with their return types, `FTDShipHUD` / `FTDWaveInfo` field names, the
  health and targeting component getters, `K2_GetMarkers` → `Actor` weak pointer, `GetLeadIndicator`'s
  `OutWorldLocation`, the event bus delegates' signatures against the widget's handlers), and every
  name we look up on the plugin (`SetDataSource`, `RefreshData`, `Ships` and its `FTDUIShip` fields,
  `WBP_SpaceHUD` loading as a child of the native class).
- `TriadDrift.UI.HUDHost.ShowHideRules`: off → none; on → created for the local player with the squad
  manager as source; the same widget across three switches; removed on foot and re-created back in a
  ship; removed when switched off; the dev-HUD flag follows.
- `TriadDrift.UI.HUDHost.NoWidgetWithoutSquad`: a ship and a local player but no squad manager → none.
- `TriadDrift.UI.HUDHost.N004AbilitySequence` (Nick's N-004, read back from the widget's own decoded
  snapshot, at 30/60/120 fps): fresh run all locked → unlock Rift's Cloak → ready → activate →
  charging → ready → Rift dies and respawns → still unlocked and ready. **Fails at the "locked" steps
  until T3's squad-HUD fix lands** (`GetSquadHUD` reports the spec's ability for every ship); every
  other step passes.

## Cooking

`WBP_SpaceHUD` is loaded by path from config, so the cooker doesn't see it. `DefaultGame.ini` needs
(Lead applies):

```
+DirectoriesToAlwaysCook=(Path="/Game/TriadDrift/UI/TriadHUD")
```

Checked: the Nov 12 Shipping package (`Builds/Shipping_1112_1457`, 754 MB) compiles the plugin module in but
cooks no `WBP_SpaceHUD` (the cook log never mentions it). Without the line a packaged game falls back to the plugin's native class (always present, same look with
default style), so nothing breaks, but Nick's Blueprint tuning wouldn't ship.

## Play status (AAA.md stages)

Integrated stage pending the Lead's merge. Played live on L_Galaxy over MCP (port 8012), switch forced on
with `TD.UI.NativeHUD 1`, driven by console commands and injected input (a real Tab switch).
Captures in `Saved/TDShots/mcp_hud/`:

- `01_hud_on`: squad cards, wave and hostile count, speed and boost, heat, ammo, signature power,
  input hints, all live. The dev HUD is gone.
- `02_switch_card`: a real Tab switch to Lancer: the widget's "CUT TO / LANCER" card and the cards
  re-highlight.
- `10_target_marked`: the current target bracketed with "HOSTILE TARGET", other hostiles bracketed,
  edge arrows for off-screen ones.
- `05_wave5`: after `TD.Stage Wave5`: WAVE 05, 12 hostiles, the incoming-missile warning.

Readable at 1280x720. Not played by a human yet.

## Known issues

- **All three abilities show READY on a fresh run** (T3's squad-HUD data; fix pending). It's visible
  live in every capture.
- **Two name cards on a switch:** T3's fallback card ("KATO · LANCER", bottom-left) is drawn over
  the Rift squad card and the hyperdrive meter, while the widget draws its own card top-left. Fix: either
  set `bCutPresentationHandledExternally` while the native HUD is up (the host can do this in one
  line), or move T3's card. This is the Lead's call.
- When a target is close and dead ahead, the widget's distance text and health meters under the
  bracket sit behind the player's own ship (`10_target_marked`). That's a layout note for Nick.
- The objective reads "AWAITING MISSION DATA" during waves: the widget only fills it from
  `OnObjectiveUpdated`, and waves don't broadcast one.

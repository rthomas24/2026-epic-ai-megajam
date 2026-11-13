# Settings integration checkpoint

Source preparation only; native build, focused tests and gameplay captures remain pending.

`settings-only.patch` applies to the published MenuM01 baseline `1a8d942907ab39928a68e791a810bd1c6219434a`, exactly three files listed in manifest.json. It deliberately excludes the unrendered M02 planet actor, material authoring, visual changes and module edits. A current-Main host without MenuM01 needs the previously reviewed four-file MenuM01 source overlay first; Lead owns that handoff. Do not merge the entire HUD branch or apply the patch blindly to current Main.

Behavior: use engineering OpenSettingsPage; yield focus while open; return to Settings when focus is otherwise absent; prevent background actions during the modal; retain older-host compatibility and truthful failure messages.

Verified: patch applicability/reversal against private copies of the exact baseline, per-file overlay hashes, no M02 actor/visual dependency, git diff --check. No C++ compilation or automation execution is claimed.

Next native acceptance: matching-current-Core Editor build; all UI tests including TriadDrift.UI.Menu.EngineeringSettings; real keyboard/pad Settings opening, changing an option, Escape/B return, focus on Settings, reopening, New Game and travel; 720p/1080p/ultrawide capture. Menu/victory/backdrop/Shipping activation remains separate and requires coordinated ownership.

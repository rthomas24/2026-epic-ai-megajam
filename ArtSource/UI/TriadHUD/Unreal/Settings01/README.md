# Settings integration checkpoint — N-038 corrections

Source preparation only. Native build, automation execution and gameplay captures remain pending.

`settings-only.patch` applies to the published MenuM01 baseline `1a8d942907ab39928a68e791a810bd1c6219434a`, changing exactly the three source files listed in manifest.json. A current-Main host without MenuM01 needs the previously reviewed four-file MenuM01 overlay first; Lead owns that handoff. Do not merge the whole HUD branch or apply this patch blindly to current Main. M02 planet, material, module and visual work are excluded.

## Review corrections

1. The return-to-Settings request is now a persistent `bReturnToSettings` latch. It survives frames where focus remains on the closing external page, and survives an unsuccessful focus request. It clears only when focus is confirmed on a control in this menu's ActionsBox, including Settings or another menu control. Restoration still claims only absent/viewport focus; another live page or control keeps its focus.
2. A failed `IsSettingsPageOpen` bridge query retains `bExternalSettingsOpen`, disabled background buttons and the early return from focus restoration. `LogTDMenuSettings` warns once per external Settings session rather than every tick. A valid subsequent query can resolve closure normally.
3. A valid `OpenSettingsPage` call returning false reports **no viewport** in both its log and visible status. An unavailable/incompatible bridge has a separate log and a truthful menu-motion-only fallback message for the preserved older host. The focused native test now checks that a valid open bridge can return false for a world with no viewport; that test has not been executed.

No gameplay API, shared asset, default assignment, presentation styling or camera behavior changes. Existing underlying-menu input blocking and older-host compatibility are retained.

## Source verification performed

- Read T5's review and the current public Settings API contract in N-038.
- Applied the patch to private copies of the exact baseline with standard `git apply --check` and `git apply`.
- Checked and applied its reverse, restoring all three baseline files byte for byte.
- Recorded exact baseline and resulting overlay hashes for each file, and the patch hash in manifest.json.
- Passed `git diff --check`; only README.md, manifest.json and settings-only.patch are modified in this handoff worktree.

Hashes describe UTF-8 LF-normalized text, so normalize Windows CRLF checkout conversion before comparing. Verification used `core.autocrlf=false` on private copies. No C++ compile, Slate focus test, live input test, Shipping test or presentation approval is claimed.

## Required native acceptance

Use a Lead-provisioned matching-current-Core candidate with the reviewed MenuM01 overlay, then:

- Build Editor and run all UI tests, including `TriadDrift.UI.Menu.EngineeringSettings` and its new no-viewport case.
- Keyboard and pad: open Settings, change an option, close with Escape/B, confirm focus returns to Settings, reopen, then New Game and travel.
- Retain closing-page focus for multiple frames, then release to absent/viewport focus: Settings must remain the return target. Repeat when the initial focus request cannot yet succeed.
- Focus another menu control after closure: no later tick should steal it back to Settings.
- Simulate a failed open-page query for multiple ticks while the page is up: no background action or focus restoration, one warning per session. Recover the query and close normally.
- Distinguish a valid no-viewport open failure from an unavailable/incompatible bridge; verify the latter labels the older-host fallback as menu-motion options only.
- Check focus, pause interactions and clipping at 720p, 1080p and ultrawide.

Menu/victory/backdrop activation and packaged Shipping validation remain separate coordinated work. Production defaults are unchanged.

# MenuM01 presentation follow-up (#71)

Source candidate only. Native compile and packaged gameplay verification remain pending.

## Scope and baseline

One changed source file: Plugins/TriadDriftHUD/Source/TriadDriftHUD/Private/TDMainMenuWidget.cpp.
Apply menu-presentation-only.patch AFTER the tested MenuM01 overlay1a8d9429 and Settings01 patch b7dc3544. This patch does not include or replace the three other overlay files. All four prior overlay versions were preserved as .before files locally. See manifest.json for LF-normalized hashes. Claim dfdd0d25; existing private nick/ui-settings-validation-33bc6d1 checkout at33bc6d16.

## Change

- Add an input-transparent focus border to each enabled menu action, following actual user0 button focus regardless of EFocusCause. Directly restored focus is now intended to be visible immediately, without an extra direction press. Does not request focus, change navigation or steal focus from the external Settings page.
- Move the cyan title accent into the same1920x1080 ScaleToFit foreground as title/buttons. It no longer uses the independently stretched backdrop geometry.
- Attach the three-color rule to the520-unit action group with equal slots and two9-unit gutters. Left/right outer edges align with the action group and its position follows the actions, including compatibility menu-motion actions. Orbital backdrop remains unchanged apart from removing these two foreground decorations.

No new materials, textures, animation, gameplay contracts, global activation, settings storage or music edits. Cost is a few Slate overlay/border widgets and focus attributes; runtime performance has not been measured.

## Verification performed

prepare-handoff.py verifies baseline hash, creates the incremental patch, applies it in a temporary isolated tree and reverses it byte-equivalently after LF normalization. The complete source tail beginning at InvokeSettingsPage is identical to the tested baseline: reflection bridge, external Settings yielding, return latch, navigation/actions and release logic are unchanged. Header/module/tests are unchanged byte-for-byte. No native build or editor session launched due the known local memory limit.

## Requested low-memory packaged verification

Use Ryan's isolated Development package route from #45. Apply this incremental patch, build, record exact revision + source hashes + archive checksum. Launch real L_MainMenu with the existing -TDUIMenuReview fixture and private UserDir; confirm host activation log. Production style stays unchanged.

At1280x720,1920x1080,2560x1080:

1. Open Settings, change Master volume100->90, close with Esc. Capture immediately and after3.5seconds WITHOUT directional navigation: Settings must have a clear visible focus border.
2. Enter must reopen;90% remains displayed. Repeat close with simulated padB and reopen withA; label simulated input separately from physical hardware.
3. Confirm focus outline moves to a different action after navigation, never marks disabledContinue, and is absent behind the open external page.
4. Capture menu before/after: cyan title accent stays beside title; all three squad lines sit below the buttons and align to action-group edges at every size. Inspect720p vertical fit and fallback/status text spacing.
5. Retain the already-covered open/change/close/focus/reopen behavior and check no new warnings/errors. Repeat relevant existing14 menu/Settings tests after compilation; do not claim prior tests cover this new patch.

Send actual packaged captures and test account for UI review. No physical controller, Shipping, NewGame or restart persistence claim is implied by simulated Development checks. No local retry requested.

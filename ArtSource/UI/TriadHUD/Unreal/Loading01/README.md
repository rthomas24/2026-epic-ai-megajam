# Native transition screen — technically verified candidate

Native class: `/Script/TriadDriftHUD.TDLoadingWidget`.

Uses the existing `ATDTransitionGameMode.LoadingWidgetClass` / `UTDUIStyle.LoadingWidgetClass` hook. No second host, travel implementation, automatic activation, or shared style write. Production files are the three TDLoadingWidget source/test files in the owned HUD plugin.

The screen uses engine fonts and Slate geometry: readable scene/status labels, a restrained plate, and fixed squad-colour marks. The marks do not represent progress. SceneTitle and StatusText can be supplied by a host; the defaults claim no destination or progress that the host has not supplied. No new polling, materials, timers, animation, or input focus.

## Verification — November 13, 2026

Tested on exact DAE base `dae4943af133bd21407f2d9c07a2ee575f31eb55` in the isolated `ui-loading-validation-dae` worktree. Corrected native module rebuild passed in 10.61 seconds; all 15 TriadDrift.UI tests passed in 28.84 seconds, including the loading hook, Slate construction, input flags, and GC lifetime check. The initial new-test crash was caused by bypassing UUserWidget.Initialize in the test fixture; failed evidence remains preserved. The production widget did not require a visual or behavioral change.

The real `L_Transition` game map mounted the candidate through the existing host's instance override. The fixture is opt-in and development-only, and must not be promoted as activation code. Actual viewport captures at 1280x720, 1920x1080 and 2560x1080 are in [review-20261113/review.html](review-20261113/review.html). No clipping observed. The game exited cleanly (0) after 29.59 seconds, minimum sampled free RAM 2.918 GiB. Source, Config and project input hashes remained unchanged. No comparative frame-time benchmark was taken.

The capture helper saved all three PNGs, recorded their resolutions and requested a clean game exit, then encountered an unsupported client.close() call in its own cleanup. The helper has been corrected without repeating the native run; this did not affect the PNGs or native exit result.

## Pending approval and integration checks

Nick's visual approval; long/localized labels; full New Game/Continue/return travel lifecycle; packaged Shipping cook and behavior; any destination/status binding beyond generic text. Direct-map captures verify the actual loading presentation, not a full travel sequence. The host must have a chance to paint before blocking work: the widget alone cannot solve a synchronous-loading stall.

No defaults, shared presentation style, Config, maps, or Ryan-owned gameplay source changed. Ready for manual presentation review; not production-integrated or gameplay-validated.

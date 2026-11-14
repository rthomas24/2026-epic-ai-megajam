# Presentation71 focus correction — source handoff

Stage: implemented source candidate; native build and packaged runtime verification pending. Claim 3f997e1a. No local native launch.

Apply menu-focus-only.patch AFTER the MenuM01 1a8d9429 overlay, Settings01 b7dc3544 patch, and Presentation71 234117fe patch. This is an incremental one-file patch over the source delivered by234117fe, not a replacement overlay. Check manifest.json before/after LF hashes. Preserve all three other overlay files. Lead owns publication.

Change: replace texture-backed FocusRectangle with a stable-lifetime, texture-free FSlateRoundedBoxBrush: transparent fill, square corners, cyan3-unit outline. BorderImage chooses outline or NoBrush from the unchanged enabled/user0-focus predicate; no alpha-dependent hiding. Only the input-transparent focus SBorder is forced volatile to refresh under invalidation. No timers, input handlers, focus requests, geometry or alignment changes. Existing BuildActions and all source following it are identical to234117fe, including Settings reflection bridge, return focus latch, navigation/travel and cleanup. No header/module/test changes, music, style asset or production-default edits.

Diagnostics: Development launch with -TDUIMenuReview -TDMenuFocusDiagnostics and a private UserDir. TD_MENU_FOCUS logs initial and changed enabled/focused states per action, button and user0 focus pointers, focused widget type, focus cause, outline predicate and expected texture-free brush/width. Diagnostic code excluded from Shipping. Logs describe attribute evaluation, not proof of pixel output. Only a few outline widgets redraw; cost not measured.

Verification completed: baseline SHA matches234117fe manifest; git apply --check/apply/reverse round-trip passed; other three overlay files byte-identical to preserved versions; BuildActions and following source identical. Engine5.9 local headers confirm rounded-brush constructor, ForceVolatile and GetTypeAsString APIs. Not compiled or played locally;14 automation tests must be rerun on the matching build.

Requested matched-package checks at1280x720,1920x1080,2560x1080:
1. Initial NewGame focus clearly visible; disabled Continue never outlined. Navigate Settings/Quit/NewGame and verify one outline follows focus.
2. Settings open: no outlined enabled action behind page. Change Master100->90; Escclose. Capture0.3s and3.5s WITHOUT direction; Settings outline visible. Enterreopens with90retained.
3. Repeat simulated Bclose and Areopen; distinguish Slate simulation from physical device evidence. Capture full-resolution menu frames, not just contact sheets.
4. Preserve title accent and equal squadron-rule alignment, Settings fit, existing behavior. Run14 existing menu/Settings tests and record raw results, source hashes, package hash, warnings and errors.
5. If outline still absent, use TD_MENU_FOCUS logs: outline=0 means investigate focus/enabled predicate; outline=1 with no visible border means renderer/geometry/invalidation requires investigation. Do not declare focus or Shipping verified based on source checks.

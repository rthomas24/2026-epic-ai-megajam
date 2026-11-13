# Menu M01 — verified native candidate

Final build04 PASS OS0 66.624s; all17 UI tests04 PASS OS0 27.543s.
Actual windowed game03 PASS OS0/driver0 46.061s. Six untouched native screenshots in verified/.
Observed Settings -> ACCESSIBILITY, FULL -> REDUCED, Escape -> main menu, Enter New Game ->
L_Transition -> L_Galaxy, existing native gameplay HUD shown1. Engine focus logs report SButton.
Actual PNG dimensions:1280x720,1920x1080,2560x1080; labels and actions remain readable.
Open preview.html for matched1080 before/after and all final screenshots. All referenced PNGs
exist and were opened for visual inspection; byte-identical copies and dimensions/SHA in screenshots.json.

Implementation files: Public/TDMainMenuWidget.h; Private/TDMainMenuWidget.cpp, TDMenuTests.cpp,
TriadDriftHUDModule.cpp in existing Plugins/TriadDriftHUD/Source/TriadDriftHUD/.
Native UMG/Slate animated orbital plate, squad accents, refined title/actions, existing host actions,
truthful Continue availability, failed-action guard and menu-only reduced-motion setting.
This is an animated2D galaxy foundation, not a3D spacecraft cinematic or AAA/final visual approval.
No imported assets, new materials/lights/scene captures or gameplay polling. Menu focus restores
only when absent/on SViewport after attachment/resize; it preserves focused child buttons.

Development-only -TDUIMenuReview, isolated fresh-save main-menu: uses actual existing
ATDMenuGameMode.MenuWidgetClass before StartPlay; no CDO edit, asset/config/map save,
competing host, default activation or Shipping activation. Delegates cleaned on module shutdown.
Existing NewGame/ContinueGame/CanContinue/QuitGame actions use checked reflected zero-input
bool/void calls. Continue correctly disabled without a save. Actual continuing a saved run,
mouse/gamepad actions and new-menu Quit remain separate checks, not claimed from keyboard NewGame.

Known limits: ultrawide decorative backdrop accents scale differently from the safe foreground;
no essential action clipping, but accent alignment needs later polish. No visible missing menu
glyphs in final captures; earlier M04 gameplay glyph anomaly remains unresolved, not assumed fixed.
One nonfatal engine HTTP socket_send_failure at startup; no fatal/assert/ensure found in final run.
Final captured timing sample is AFTER Galaxy transition, not menu performance. No before baseline
or sustained performance/Shipping test: no regression or performance-readiness claim.
Final free-RAM minimums:build2.490GiB/tests0.948GiB/game1.433GiB; brief low readings passed the
agreed sustained<1.5GiB for10s guard, no threshold lowered. All own PIDs terminal/released.
Protected Source/Config/uproject hashes unchanged in every phase.

Preserved failures: build01 Slate bracket/delegate warning; tests02 instantaneous memory abort
(then corrected to agreed sustained monitor); game01 CDO fixture left old fallback so Quit selected;
game02 new menu visible but SViewport focus prevented actions. Baselines and visible-input-failed/
remain explicitly diagnostic. Final build04/tests04/game03 supersede these for candidate acceptance.

Production dependencies: Audio owns DA_UI_Style; assignment needs serialized handoff and Ryan must
preserve existing victory statistics (current host shares main-menu/victory widget slot). Camera,
movement, recoil, landing and blur stay Ryan-owned; no shared changes. Menu motion preference is
widget-lifetime only; full Settings/persistence and visual refinement remain future work.
No main merge, no character/ship/environment/audio edits. HUD8fbe607 checkpoint preserved.

Reproduce only in a fresh Lead-exclusive slot with>=4GiB free/zero heavy processes, private8026:
verify-menu.ps1 -Phase build -LeadCleared -Attempt <unused>; then tests(expected17); game;
capture-menu.py --attempt <matching game attempt>. Unique attempts refuse overwrite; no auto retry.
Logs and raw MCP records remain Saved/HUDMenuM01 (ignored). Source/evidence hashes in source-freeze.json.
Campaign began22:55EST Nov12; hard stop23:55EST, no extension. Technically verified prototype,
not integrated, Shipping-verified, presentation-approved or a complete cinematic/immersive-effects campaign.

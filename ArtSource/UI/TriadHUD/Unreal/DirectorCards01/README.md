# Director cards — paired candidate for build and gameplay review

This handoff contains an **uncompiled candidate patch**, not active game source. Nicholas requested the updated Director cards be built, all five tests run, and actual gameplay shown at 1280x720, 1920x1080 and 2560x1080. Shared defaults remain unchanged until appearance approval.

Apply `director-cards-paired-candidate.patch` only in an isolated review checkout with T3's agreed `FTDDirectorCards.bNameCardClaimedByHUD` reflected bool. It adds exactly three files under the HUD plugin: `TDDirectorCardsWidget.h`, `TDDirectorCardsWidget.cpp` and `TDDirectorCardsWidgetTests.cpp`. No fixture, game-module dependency, project/config or style asset is included. Preserve any existing work at those paths and verify the manifest before applying; never overwrite a conflicting candidate.

The new field is mandatory. Older hosts fail the compatibility check and draw blank rather than guess ownership; they cannot validate the actual-host test. A true claim suppresses both the pilot NameCard and its SwitchCut text fallback, preserving the independent switch cue, quick cuts, bars, flash, slate, CUT and money shot. The renderer consumes the host's state and timing and accepts no input.

Build against the matching T3 runtime and run `TriadDrift.UI.Director.`: expect **five explicit results**, including ActualHostReflectionContract and AuthoritativeNameOwnership. The previous snapshot's renderer compiled, but its test flag enum needed correction; that is not proof this paired revision compiles. No tests have been run on this candidate. `source-manifest.json` includes raw Windows hashes and normalized LF hashes for portable comparison.

For review only, assign `/Script/TriadDriftHUD.TDDirectorCardsWidget` to DirectorWidgetClass in a private isolated style copy, preserving every unrelated field. Return original PNGs, source/runtime fingerprints, test results and logs for all three resolutions: slate, pilot, CUT, quick switch and money shot, claiming/non-claiming HUDs, intro suppression and travel cleanup. Label staged states explicitly. The detailed checklist is `remote-capture-request.json`.

N-038 tracks the T3 host revision and Ryan's matched build/capture. UI's frozen authoring snapshot remains in `nick/ui-director-cards-1113`; the Lead publishes this small portable handoff so verification can proceed without consuming the local native queue. No presentation, gameplay or Shipping acceptance is implied.

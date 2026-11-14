# Victory reference revision — T3 source handoff

Stage: prepared source patch, not applied to a project or compiled. Base fc074b84394db611a2530d1f2ba91c776c969efa; claim4d54513e. The original blobs and edited candidate copies live only under Saved/Credits01/VictoryRevision01/. T3 owns actual integration. Apply victory-reference-random-headlines.patch only after checking ownership/current source and the before hashes in manifest.json. No changes to Ryan worktrees, host Source, DA_UIStyle, credits asset, Settings overlays, music or defaults.

Behavior: six separately localized FText choices, sampled uniformly by one RandHelper call on first ShowMenuUI for a victory lifecycle. The host caches the chosen FText; paints, ticks, focus and rebuilds use it without another draw. ReturnToMenu clears it; a new victory host can roll again. Repeats between victories are allowed. No weighting by results or new results-based subtitles. GetVictoryHeadline exposes the same cached value to any custom victory widget. Eyebrow: MISSION COMPLETE // OPERATION PARADOX.

Reference presentation: existing live backdrop retained, with a single translucent full-screen dark veil, texture-free fine cyan circles/diagonals and registration marks. White large centred headline, cyan eyebrow/stat row, separators and thin rule; remove the opaque rectangular victory card. All presentation geometry uses1920x1080 ScaleToFit, keeping ultrawide aspect ratio. Headline/stat rows each scale within fixed bounds, preserving all values. Existing scene/take rows remain; a90-reference-unit bounded scroll area prevents a long run from pushing credits/footer offscreen. Scene text wraps. Native FTDCredits::MakeRoll and DA_Credits remain the data/clock/scroll owner. A1.4x Slate scale enlarges the existing native roll (720x200 ->1008x280 reference), improving text visibility without changing TDCredits.cpp or data. Same existing focus-framed Return action moves bottom-left. No unsupported B/P/Tab hints or new pause/fast-forward behavior. No backdrop, lighting, assets, gameplay, timer or music edits.

Verification performed: original base bytes verified through git show; git apply --check passes against the isolated original tree inside Saved; no patch applied. Original blobs remain byte-identical. Source comparison proves normal menu card/action path and KeepMenuFocus, ChooseRefocus, StartPlay, music lifecycle, NewGame/Continue/Quit and transition StartPlay unchanged. Header API signatures checked in installed UE5.9. One random draw site and six exact approved strings verified. Existing native asset checksum remains unchanged. These are source checks, not compilation or pixels.

Added focused test: TriadDrift.Flow.VictoryHeadlineIsCachedForThePresentation. Tests six localized/nonempty choices, normal menu leaves headline empty, real victory host selects a pool member,60 ticks/repeated reads keep it stable, ReturnToMenu clears the cache and restores normal mode. Private automation save-slot override; no production style dependency. The test is written but NOT run. Also rerun existing TriadDrift.Flow.MenuNewGameContinueVictoryAndBack, menu focus and music tests; UHT/full build required because header exposes a Blueprint getter.

T3 validation before publication/default approval:
- Compile matching Editor/Development and relevant Shipping target. Run focused test and existing Flow/Director/menu regression coverage. Do not apply to another active owner's file without integration coordination.
- Actual native screenshots at1280x720,1920x1080,2560x1080, all six long/short headline choices, large numeric stats, zero/few/many scenes, long scene IDs. Verify no overlap with footer, no cropped headline or credits, enough contrast on bright backdrop and the18s black stinger. Source-specific MUSIC/track names confirm DA_Credits.
- Verify choice remains fixed during resize/focus/rebuild, and fresh victory can select any approved choice. Repeated choice across separate victories is valid; no statistical randomness claim from a few captures.
- Verify Return with keyboard/pad/mouse, Settings return and normal menu styling/music unchanged. Check scene-list wheel and keyboard/controller access when it overflows; new bounded area needs real input verification.
- Native credits content/full loop and natural campaign victory/Shipping return still need acceptance. No audio/performance claim. Linework is static3x96-segment rings plus lines, cached by Slate, no active timer; runtime cost unmeasured.

The reference is a design target, not proof this source candidate matches it. Native screenshots and Nick's visual approval remain required. Local heavy operations require a separate safe slot; previous ultrawide run exceeded memory despite5.78GiB starting free.

## Nicholas's victory wording decision — November 14, 2026

Nicholas explicitly confirmed the victory eyebrow **MISSION COMPLETE // OPERATION PARADOX**, with the large message selected randomly from six strings. This is an approved post-ending operation label; it does not rename the game or add a public subtitle. It supersedes the pending eyebrow question in issue #29 comment 6873641374 and the removal recommendation recorded during the resumed Story review.

The existing six headline strings are preserved:

- A FUTURE RECLAIMED
- BALANCE RESTORED
- TOMORROW IS OURS
- THE WORLDS ENDURE
- A NEW DAWN
- AGAINST ALL ODDS

Choose one when the victory presentation opens and keep it stable during that presentation; a later victory may select again, including the same string. Current TDFrontEndGameModes.cpp already contains the eyebrow, six-choice pool and cached selection. This records Nicholas's wording/direction approval, not a new code change, new test execution or final appearance/gameplay acceptance. T3 retains runtime ownership; actual current-build presentation still needs verification.

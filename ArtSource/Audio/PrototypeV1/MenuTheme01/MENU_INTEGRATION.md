# Minimal menu integration handoff

## Current readiness handoff — Nov12 bounded cycle (source review only)

Reviewed latest published audio skill and current origin/main1730d9d Core/Flow/config plus N020; no native execution. **The menu hook is now on main**: UTDUIStyle.MenuMusic; ATDMenuGameMode loads it into a nonspatial UI audio component, requests a1s fade for NewGame/Continue, and stops on EndPlay. Default boot is L_MainMenu with TDMenuGameMode. Earlier hook-pending notes below are historical.

- **Exact host prerequisite:** Lead must supply an absolute current-game unrealGameJam.uproject path and matching compiled UnrealEditor-unrealGameJam DLL/modules/build receipt containing /Script/unrealGameJam.TDUIStyle.MenuMusic and the published Flow/menu boot code (current reference1730d9d). The stale day1 host is insufficient; private TDAudioV1 has no game class. No compatible physical host/build receipt is supplied yet. N020 requests it from Ryan. Host must mount the two frozen audio packages at their exact /Game/TriadDrift/Audio/MenuTheme01 paths, with matching recorded SHA; check exact host grant, RAM/process/port guards and backup/no-overwrite before any later native job. This review authorizes no launch/build/mount.
- **Class/default dependencies:** three prepared files remain MENU_INTEGRATION.md, create_menu_style.py and check_menu_dependency.py; both scripts parse, native APIs remain unexecuted. Active exact style claim permits NEW /Game/TriadDrift/Presentation/DA_UI_Style of UTDUIStyle, setting ONLY MenuMusic to MUS_Menu_EtherialPulse_01. Preserve empty SquadColors/AbilityIcons, default EnemyColor and null MainMenuWidgetClass/LoadingWidgetClass/PauseWidgetClass; engineering confirmed safe per-slot fallback. Existing style means STOP for owner review, never overwrite. Missing compiled menu_music/class/factory or nonnull unexpected widget defaults means STOP, no substitution. Protect all pre-existing host assets/source/config and both audio hashes; later reload the created style.
- **Cook/reference checks:** current DefaultGame.ini already AlwaysCook-scans TDUIStyle in Presentation. Verify saved style -> wave -> SC_MenuTheme_Music registry dependencies, no private TDAudioV1 dependency, and all THREE packages in actual cooked Shipping output. A registry pass or existing package built without this style is not selected-song cook/playback proof.
- **Actual-game checks:** normal boot must show menu and produce one audible music instance, correct music-volume/mute behavior, natural ending at107s with no restart, NewGame/Continue fade then no music surviving transition/Galaxy, return-to-menu one fresh instance, no missing references/errors. Inspect the real fade: a1s FadeOut request can be shortened by map teardown/EndPlay; code inspection does not prove a full smooth audible fade. Keep current-game logs/output evidence; no new source mix/loop/edit is proposed.
- **Remaining ear gate / next deliverable:** source/native wave/class checks PASS; style creation, current-game/cook/playback remain UNVERIFIED. After Lead supplies the compiled host and a bounded grant, create and reload the music-only style, then verify cook and actual menu playback under separately scheduled tests. Nick must hear it in the game before presentation approval. No licenses were re-audited, no assets changed, no new audition produced. Request Lead preflight before any commit/publication; stop after this handoff.


Reviewed origin/main41c3ad5, N015 engineering reply19:26, TDPresentation.h, TDFrontEndGameModes.cpp and Presentation-Asset-Paths.md. MenuMusic is promised/engineering-owned, not yet in the inspected Core/Flow code. No DA_UI_Style package is tracked under Presentation on this revision. Do not assign against stale compiled metadata.

Exact assignment after hook publication and explicit style ownership: `/Game/TriadDrift/Presentation/DA_UI_Style.DA_UI_Style` (class `/Script/unrealGameJam.TDUIStyle`), `MenuMusic` (expected Python `menu_music`) = `/Game/TriadDrift/Audio/MenuTheme01/MUS_Menu_EtherialPulse_01.MUS_Menu_EtherialPulse_01`.

If style exists in the eventual host, preserve ALL unrelated properties and back up its package before the single-slot edit. If absent, creation requires a new exact published claim or a UI/engineering owner handoff. Proposed minimal class-default asset leaves SquadColors/AbilityIcons empty; EnemyColor retains class default(1,.12,.24); MainMenuWidgetClass,LoadingWidgetClass,PauseWidgetClass remain null. These are a proposal pending owner's confirmation, not permission to remove existing UI references. This audio task does not create widgets or choose new visual defaults.

Current DefaultGame.ini has TDUIStyle primary-asset scan of `/Game/TriadDrift/Presentation`, CookRule AlwaysCook and recursive application. Once assigned, verify registry dependencies DA_UI_Style -> wave -> music class with `check_menu_dependency.py`, then verify those packages in a real cook. Registry presence alone is not cook or playback evidence. No config edit proposed.

Native evidence: private import/reload PASS,107s/nonlooping/Music class/pitch1/volume1;78protected inputs unchanged; source hash/loudness pass. Branch2fa17b3 includes exact two portable packages. Current-game dependency script is prepared only, no live run. N015 Core/Flow/menu boot tests, matching compiled host, style claim/assignment, cook, audible menu boot/volume/end/travel/reentry remain gates.

Proposed PR title: **audio: add the selected107-second main-menu theme assets**

Proposed body: Adds Nick's selected Etherial Pulse107-second cut as a portable SoundWave and Music SoundClass. Original source is preserved and rights/AI credits documented. Private UE5.9 import and read-only reload pass;107s,nonlooping,source/class references correct,protected hashes unchanged. Main-menu assignment and gameplay/cook acceptance depend on engineering N015 and a separately claimed DA_UI_Style edit. No shared C++, maps, config or rejected audio promotion.

PR allow-list: only `Content/TriadDrift/Audio/MenuTheme01/MUS_Menu_EtherialPulse_01.uasset`, `SC_MenuTheme_Music.uasset`, and `ArtSource/Audio/PrototypeV1/MenuTheme01/**`. Lead must construct/review the actual PR against current main because this isolated branch also preserves earlier prototype history. Do not include other PrototypeV1 directories or rejected PR5 output. Future style assignment belongs in a separately claimed owner change.

Exact proposed owned file list:

- `ArtSource/Audio/PrototypeV1/MenuTheme01/HOST_PROPOSAL.md`
- `ArtSource/Audio/PrototypeV1/MenuTheme01/MENU_INTEGRATION.md`
- `ArtSource/Audio/PrototypeV1/MenuTheme01/README.md`
- `ArtSource/Audio/PrototypeV1/MenuTheme01/check_menu_dependency.py`
- `ArtSource/Audio/PrototypeV1/MenuTheme01/guarded_import.py`
- `ArtSource/Audio/PrototypeV1/MenuTheme01/guarded_reload.py`
- `ArtSource/Audio/PrototypeV1/MenuTheme01/import_menu_theme.py`
- `ArtSource/Audio/PrototypeV1/MenuTheme01/manifest.json`
- `ArtSource/Audio/PrototypeV1/MenuTheme01/native_import_01.json`
- `ArtSource/Audio/PrototypeV1/MenuTheme01/native_reload_01.json`
- `ArtSource/Audio/PrototypeV1/MenuTheme01/validate_reload.py`
- `Content/TriadDrift/Audio/MenuTheme01/MUS_Menu_EtherialPulse_01.uasset`
- `Content/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music.uasset`

## Ownership resolved / next native operation

N017 confirms Audio may create DA_UI_Style with pure class defaults/null widgets and set ONLY MenuMusic. Exact Presentation asset claim published dc924d04fd437893c8d5705d7b11302fa1351322; this supersedes the open ownership question above. Core MenuMusic is on main2d931e4; compiled current-game host and completed Flow/menu boot publication remain prerequisites. No game-class asset is created in TDAudioV1.

`create_menu_style.py` is prepared only. A separately granted execution requires exact current-game project_dir and claim_commit in SPEC. It refuses existing style/evidence, verifies compiled class/property and null widget defaults, source package hashes and TDUIStyle AlwaysCook rule before creation. The only authored field is MenuMusic. It compares all current explicit unrelated UIStyle fields against class defaults, saves only the new style, checks registry style -> wave -> class and unchanged wave/class bytes. Runtime API and newly created style still require a fresh read-only reload. No retries or overwrite.

PR asset scope is now exactly THREE assets: the two audio packages and Content/TriadDrift/Presentation/DA_UI_Style.uasset; source preparation/evidence remains only this owned MenuTheme01 source folder. No other Presentation asset, Source, config, map or widget change. Lead constructs the exact PR scope and excludes unrelated branch history.

Acceptance after separate grants: reload exact style/class/107s assignment; confirm AlwaysCook scan and actual cooked inclusion of all3 packages; boot the current game directly into L_MainMenu and observe one music instance with audible output; verify music volume/mute; let the track end at107s without restart; leave before endpoint to verify engineering fade/stop and no menu music in gameplay/transition; reenter to verify one fresh instance. Preserve native logs/captures or measured output, report engineering test results separately, and ask Nick to hear the game before presentation approval. No current-game playback or cook pass is claimed by this preparation.

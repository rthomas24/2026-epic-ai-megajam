# Current-game job commands — preparation, not a launch grant

Fastest known safe candidate: Lead fast-forwards the clean designated `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/day1-integration` checkout and builds its Editor target against current main. Existing private TDAudioV1 is compiled but has no TDUIStyle game class; older day1/HUD candidate binaries predate current menu metadata. No verified compatible current-game DLL/receipt is known to Audio. Lead must provide exact host and required hydrated Content/plugin mounts; Audio will not sync or build it autonomously.

Use a unique host Saved/AudioMusic/MenuTheme01 run directory for each separately granted job. Lead verifies current game DLL/modules/receipt, protected input hashes, absent style/evidence, two frozen portable-package hashes and claimed style output first. Every launch checks >=4GiB RAM/zero other heavy processes/port8025 clear, enforces180s native/60s no-progress/240s outer limits, aborts only its own PID tree after10s below1.5GiB, and never retries without clearance. Source/script/native backups and before/after hashes remain required. No playback during creation/reload.

## 1. Native creation (headless)

Exact bootstrap SPEC, replacing HOST only with Lead's granted absolute directory:

```python
SPEC = {
    'execute': True,
    'project_dir': 'HOST',
    'claim_commit': 'dc924d04fd437893c8d5705d7b11302fa1351322'
}
exec(compile(open('C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-prototype-v1/ArtSource/Audio/PrototypeV1/MenuTheme01/create_menu_style.py', encoding='utf-8').read(), 'menu_style_create', 'exec'))
```

Command argument array (do not execute as an unquoted shell string):

```text
C:/Program Files/Epic Games/UE_5.9/Engine/Binaries/Win64/UnrealEditor-Cmd.exe
HOST/unrealGameJam.uproject
-AgentMode -nullrhi -NonRealtimeAudio -NoLiveCoding
-run=pythonscript -script=RUN/create_bootstrap.py
-ExitTimeout=180 -NoProgressTimeout=60
-abslog=RUN/engine.log -ResultFile=RUN/engine_result.json
```

Require process0/result0/reasonok/no unexpected errors or suppressed dialogs; one newly created style only, original packages/source/config unchanged, style_create_01.json and registry references correct. Stop on existing asset, missing compiled property, unexpected class/widget defaults or first API error. No overwrite fallback.

## 2. Reload reference check (fresh headless process)

Bootstrap `SPEC={'execute':True}` and execute the committed `check_menu_dependency.py` at the same absolute source root, using the same engine command with a NEW run/bootstrap/evidence path. It performs no saves, imports, map edits or playback. Protect and compare all three native package hashes plus source/config; confirm style -> wave -> class references,107s and looping=false. This script prints results to the engine log; process result and logs must be reviewed. Runtime API success remains pending; no extra getter calls assumed available.

## 3. Actual audible menu (separate windowed job)

Use normal real-time audio: NO `-nosound`, NO `-NonRealtimeAudio`, NO `-nullrhi`. Do not use AgentMode's implicit offscreen rendering for Nick's visible audition. Launch argument array after separate fresh grant:

```text
C:/Program Files/Epic Games/UE_5.9/Engine/Binaries/Win64/UnrealEditor.exe
HOST/unrealGameJam.uproject /Game/TriadDrift/Maps/L_MainMenu
-game -windowed -ResX=1280 -ResY=720 -unattended -nosplash -nopause -NoLiveCoding
-ModelContextProtocolStartServer -ModelContextProtocolPort=8025
-Seconds=140 -ExitTimeout=200 -NoProgressTimeout=60 -HangTimeout=60
-abslog=RUN/game.log -ResultFile=RUN/game_result.json
```

Discover the actual runtime toolsets first. Inspect the live menu actor's reflected Music component and its assigned sound/play state using supported object tools; GetMusic/WasMusicStarted/IsMusicFadingOut are C++ methods, not reflected UFUNCTIONs, so do not assume Python/MCP callable getters. Capture menu and output evidence. Observe one voice shortly after boot, near105s, and beyond107s: silent/stopped after natural endpoint, no restart or duplicates. Confirm class volume/mute through supported runtime controls only if available; no new settings framework or OS-volume changes. Nick hears the normal game output; measurements do not replace his approval.

## 4. Travel/reentry and cooked acceptance

Use a separately bounded short game job or an agreed extension of the next grant, with isolated test save storage: back up/protect real player saves before NewGame/Continue. Real Slate action `TD.UI.Key Enter` activates focused NewGame; inspect actual menu focus first. Check requested1s fade versus teardown timing, no menu music in transition/Galaxy, then a supported return-to-menu action and exactly one fresh instance. Do not fake success via forced reflection of private flags or use a stage skip as full journey proof. Stop the owned process and review result/logs.

Lead/Ryan coordinate style adoption and Shipping cook. Existing TDUIStyle AlwaysCook scan must include style and both referenced audio packages; inspect all three in the actual resulting package and boot that package audibly. Old Shipping/menu evidence without this style does not validate this song. Source/private import/reload are already checked; new style, current-game playback/travel, cook and Nick listening approval remain open.

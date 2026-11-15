# Packaging how-to (handover, Nov 14 evening)

How a Shipping package of any `main` sha is made on Ryan's machine, how it is checked, and what it costs. Written by
Ryan's packaging agent from the half-hourly cooks of Nov 14 (14:00 to 21:10).

## 1. Make a Shipping package of a sha

Use a clean checkout or worktree that holds only that sha (the cooks used `.wt/pkg`, detached at the sha, with
`git lfs checkout` run after each checkout). From its root, in PowerShell:

```powershell
git checkout --detach <sha>; git lfs checkout
.\Tools\td-build.ps1                                   # editor target first: the cook runs the editor (-nocompileeditor)
.\Tools\td-package.ps1 -Configuration Shipping -Name playable_<short-sha>
```

- Output: `<checkout>\Builds\playable_<short-sha>\Windows\` (run `unrealGameJam.exe`). Log:
  `<checkout>\Saved\Logs\td-package_playable_<short-sha>.log`. The script prints `PACKAGE SUCCEEDED ... size=... MB`.
- Never set `UE_SKIP_UBT_SDK_SETUP` around packaging.
- `td-package.ps1` passes `-ubtargs=-WaitMutex`, and `td-build.ps1` waits for one of the two team compile slots
  (`Builds/compile-slot/C1`, `C2`). When other agents are compiling this queued the cook for up to 27 minutes. The
  teammates build with `-NoMutex`; to do the same, run BuildCookRun directly (same arguments as `td-package.ps1`, only
  the mutex flag changed):

```powershell
& "C:\Program Files\Epic Games\UE_5.9\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun `
  "-project=$PWD\unrealGameJam.uproject" -noP4 -platform=Win64 -clientconfig=Shipping `
  -build -cook -stage -pak -archive "-archivedirectory=$PWD\Builds\playable_<short-sha>" -prereqs -utf8output `
  -unattended -nocompileeditor -nodebuginfo "-ubtargs=-NoMutex"
```

  After a direct run, delete `vc_redist.arm64.exe` and `d3d12SDKLayers.dll` from the output as `td-package.ps1` does.
- Time on this machine (8 cores / 16 threads, 64 GB): editor build 1-9 min (9 min after a large C++ batch), Shipping
  game compile 1-4.5 min, cook 0.5-12 min (incremental in the same checkout; about 12 min for the first full cook of
  ~2,700 packages), IoStore/pak 1.5 min, zip 3 min. A typical half-hourly cycle took 15-25 min end to end.

## 2. Zip and size budget

- Zip the contents of `Builds\playable_<short-sha>\Windows\` (not the folder above it), for example with
  `[IO.Compression.ZipFile]::CreateFromDirectory(<Windows folder>, <zip>, 'Optimal', $false)`.
- **The zip must stay under 1 GB (1,000,000,000 bytes).** At main `6a33e972` (cooked as `3f6d29ac`) it is
  **918.6 MB** (81.4 MB margin). It grew about 15 MB per batch while Fab props landed; texture caps saved 26 MB once.
  If it nears 960 MB, list the largest new packages from the container listing (UnrealPak `-ListContainer`) and cap
  their textures.

## 3. What gets cooked

- The maps in the cook list (`Config/DefaultGame.ini`, `+MapsToCook`): L_MainMenu, L_Galaxy, L_Planet_Verdance,
  L_Planet_Cinder, L_Transition, L_Dominion_Interior, and everything they reference.
- Every data asset in `/Game/TriadDrift/Presentation/` and what it references.
- Every folder in `+DirectoriesToAlwaysCook` (same file): TriadHUD UI, Gameplay Materials / Props / Characters / Audio
  / VFX, the mannequin and shooter anims, the listed Audio music folders, `/Engine/BasicShapes`.
- Anything loaded only by a path string from elsewhere is **not** cooked and is missing in the package.

## 4. The Shipping-only compile trap, and the check that catches it

The editor build can pass while the Shipping game target fails: editor-only engine APIs and automation tests placed
outside `#if WITH_DEV_AUTOMATION_TESTS` compile in the editor but not in Shipping. Seen on Nov 14: `UStaticMesh::IsNaniteEnabled`
(editor-only) in a dev command broke `fdd449d2` (fix: `HasValidNaniteData()`, which exists in every configuration);
earlier, two tests after `#endif // WITH_DEV_AUTOMATION_TESTS`. Run this compile-only check on every new main, before
cooking (about 1-4.5 min, no cook):

```powershell
Push-Location "C:\Program Files\Epic Games\UE_5.9\Engine\Source"
cmd /c "call ..\Build\BatchFiles\GetDotnetPath.bat && dotnet ..\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll unrealGameJam Win64 Shipping -Project=<checkout>\unrealGameJam.uproject -NoMutex -NoHotReload"
Pop-Location
```

Exit code 0 = Shipping compiles. Otherwise the `error C....` lines name the file and line.

## 5. Launch check (smoke test of the package)

On Ryan's machine take a render slot first (`Builds/render-slot/README.txt`: mkdir `LOCK`/`LOCK2`/`LOCK3`, then
`bash Builds/render-slot/can_launch.sh` with Git Bash), then:

```powershell
& "<package>\Windows\unrealGameJam\Binaries\Win64\unrealGameJam-Win64-Shipping.exe" -windowed -ResX=1280 -ResY=720 -nosound "-UserDir=<package>\launch\UserDir"
```

- Always pass your own `-UserDir` under `Builds\`, never the default profile (the player's real saves live there).
- Close only the process you started, by its PID.
- Check: splash at about 1 s, loading card (splash picture, "LOADING" throbber bottom right) until the menu at about
  11-13 s, no white frame, no "(Not Responding)". Then NEW GAME, difficulty page, hold Space to skip the intro, wave 1
  with HUD (squad panel, radar), Esc for pause and Settings. With a save present, NEW GAME shows "START A NEW GAME?"
  with BACK focused.
- Last measured (`3f6d29ac` family): menu 11-13 s, peak memory about 1.2 GB at the menu and 1.5 GB in wave 1-2.

## 6. Weak machines: request a package instead of cooking

A cook on this project needs, measured on Nov 14:
- **Memory:** the Shipping compile runs 16 compiler processes; the cook commandlet peaked at 4.5 GB virtual, the pak
  step at 2.4 GB, on top of anything else running. With other agents' editors and games up, plan for 16 GB free.
- **Disk:** about 12 GB per packaging checkout beyond the source (Intermediate 7.3 GB, Binaries 1.4 GB, Saved 1.5 GB,
  package 1.0 GB, zip 0.9 GB), plus the shared derived-data cache.
- **Time:** 15-25 min per cycle here; a first, cold cook takes longer.

So a 16 GB machine (Nick's) should not cook: it would swap or fail. It should request a package from Ryan's side
(message board or an issue): Ryan's side cooks the sha and publishes the zip as a private prerelease
`playable-main-<short-sha>` on the private repo, with the SHA-256 and the launch-check result in the notes.

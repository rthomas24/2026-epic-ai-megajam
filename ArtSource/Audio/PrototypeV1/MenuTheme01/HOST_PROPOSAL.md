# Native import host proposal — not a launch grant

Proposed authoring host: `C:/Users/nicho/OneDrive/Desktop/GameJam/Worktrees/audio-prototype-v1/Saved/AudioMusic/PrototypeV1/Host/TDAudioV1.uproject`. Existing UE5.9 editor DLL and receipt are present. This isolated host has previously authored the V1 assets. Its module is **TDAudioV1**, not the current Triad Drift gameplay module: it can author portable SoundWave/SoundClass packages but cannot prove the pending Ryan MenuMusic contract or current-game playback. No build, Source copy, shared project or plugin change is proposed.

Proposed output ONLY: host `Content/TriadDrift/Audio/MenuTheme01/MUS_Menu_EtherialPulse_01.uasset` and `SC_MenuTheme_Music.uasset`. The published game-root reservation does not implicitly activate this private-host binary write; Lead must explicitly grant this physical destination and serialized operation. Native assets would later be copied by coordinated integration into the matching current-game host, after the N015 engineering hook and a new playback-test grant. No game implementation is complete at import.

`guarded_import.py` defaults to proposal-only. Its `--snapshot` mode reads source/project/config/content/compiled runtime inputs and writes hashes to a new ignored Saved JSON. No Unreal is executed. The frozen snapshot must still match immediately before a separately granted run.

The later `--grant <private JSON>` mode requires `authorized: true`, a published `claim_commit`, exact `project_dir`, timezone-aware future `expires_utc`, frozen `protected_snapshot`, and a unique `run_id`. No grant file is supplied here. Lead must approve the launcher source first. Fixed engine UE5.9, port8025 reservation, >=4GiB available memory and zero UE/ShaderCompileWorker/Blender/dotnet/compiler processes are checked before launch. Commandlet uses NullRHI/NonRealtimeAudio,180s engine timeout,60s no-progress timeout and240s outer limit. Free RAM below1.5GiB for10s stops only the owned process tree. One attempt, no retry. No MCP listener is requested for this commandlet.

Existing native destination means STOP, never replace. After an explicit grant, existing Content packages would be copied into a unique private run backup before import; protected-input hashes are checked after execution. No deletion, restore, overwrite or automatic rollback is provided. All partial outputs and logs remain for Lead review. The engine result, exit code, errors and expected package must pass; native analysis/reload and actual-game playback remain further checks.

This launcher has only source syntax/default-mode/snapshot verification so far. Unreal API calls, commandlet import and resource failure paths have not been executed. Current blockers: explicit physical native claim/host grant, pilot's prior heavy window, N015 menu hook and matching current-game binaries. No music redesign, new acquisition or audition is proposed.

## Installed UE5.9 header review (Nov12, source inspection only)

Under `C:/Program Files/Epic Games/UE_5.9/Engine/Source/`:

- `Runtime/Engine/Classes/Sound/SoundClass.h:107` declares reflected editable `FSoundClassProperties.bIsMusic`; expected Python spelling `SoundClassProperties.is_music`.
- `Runtime/Engine/Classes/Sound/SoundBase.h:117` declares reflected editable `SoundClassObject`; expected Python `sound_class_object`. Line202 declares reflected read-only `Duration`; script reads `duration` and does not set it.
- `Runtime/Engine/Classes/Sound/SoundWave.h:452` declares reflected editable `bLooping`; expected Python `looping`.
- `Editor/AudioEditor/Classes/Factories/SoundClassFactory.h:21` declares UCLASS `USoundClassFactory` with FactoryCreateNew. `SoundFactory.h:25` declares UCLASS `USoundFactory`; it also exposes bAutoCreateCue. Their headers are present in this installation; expected Python SoundClassFactory/SoundFactory bindings still require the granted editor to confirm. No live reflection or factory execution was performed.

Python names above follow Unreal's reflected snake-case/boolean-prefix convention; headers establish member presence and editability, not a successful runtime API test. Fail on any unavailable class/property without substitution or retry.

Post-run checks now require BOTH expected uassets to be nonempty, the ignored `native_import.json` to match exact wave/class object paths,107s duration and looping=false, and no unexpected new Content files. Only the two expected packages and their .uexp/.ubulk sidecars are allowed. Existing protected hashes remain checked. These guards have syntax review only; no engine operation was executed to test them. After this source revision, a new frozen protected snapshot is required before any later approved launch; protected_host_01.json is historical and will intentionally fail the changed launcher hash.

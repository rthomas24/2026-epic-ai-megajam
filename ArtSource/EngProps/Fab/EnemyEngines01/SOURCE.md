# Enemy engine loops (#151, EnemyEngines Candidate01)

Engine loops for the Null fighter and interceptor, heard on their EngineTrail cue in the intro and in waves.

## Source and licence

- Built by Nick's side (OpenAI Codex, Nicholas-approved) from **Kenney "Sci-Fi Sounds" 1.0** (www.kenney.nl), **CC0 1.0**
  (`KENNEY-LICENSE.txt`); recipe and measurements in `README.md`, take sheets `Fighter-takes.png`, `Interceptor-takes.png`.
  Delivered as `Audio151-EnemyEngines-Candidate01.zip` (release `evidence-agent-feedback-batch3-20261114`), SHA-256
  `70a05fa34ca3ccc1c2bda1fb8fe19f8485cb87d7bd600de30bfda9703e0ad64d` (checked); nothing in it was run; the download folder
  `Builds/intake9/EnemyEngines/` was deleted after the import.
- `source/`: the six final takes, renamed (mono, 48 kHz, 16-bit, 3.800 s, -23 LUFS, whole-file loops).

## In Unreal (Ryan's side, Claude Code, Nov 14 2026; claim on main 2c82325d)

`/Game/TriadDrift/Gameplay/Audio/Engines/EnemyEngines01/`:
- `SW_EnemyEngine_{Fighter,Interceptor}_Loop_0{1,2,3}`: looping, volume 0.28, the player engines' presets unedited
  (`ATT_TDE_Space_Near`, `SC_TDE_SFX`, `SCON_TDE_Loop`), virtualization Restart.
- `CUE_EnemyEngine_Fighter`, `CUE_EnemyEngine_Interceptor`: Modulator (pitch 0.96-1.04, volume 0.841-1.189 = -1.5..+1.5 dB)
  -> Random (no immediate repeat) -> three looping wave players; same three presets on the cue. Built headless from Python
  (`../scripts/ue_import_enemy_engines.py`) and read back (three players under the random node).
- Bound: `EngineTrail.Sound` on `Characters/NullFleet/DA_Enemy_Fighter_Visuals` and `DA_Enemy_Interceptor_Visuals` (were empty).
  Full text export before and after in `readback/`: the only difference is the added `Sound=` inside `EngineTrail`.

Not heard yet: listening, mix, intro skip / destruction stop and package checks stay with Nicholas and the playtesters.

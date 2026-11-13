# Lancer firing and impact listening review

Actual game playback passed on November 13, 2026, in the Lead-granted compiled test host:

`C:\Users\nicho\OneDrive\Desktop\GameJam\Worktrees\day1-integration\unrealGameJam.uproject`

Open `/Game/TriadDrift/Maps/L_Galaxy` and play. Press **2** to fly Lancer, then hold **left mouse** for short firing bursts. The new sounds are attached to NeedleCannons muzzle fire, hull hits and shield hits. In a Development session, stationary close targets can be created with the existing console commands `TD.World.SpawnEnemy ShardFighter 3000 0 0` (hull) and `TD.World.SpawnEnemy HookInterceptor 3000 0 0` (shield, then hull). These spawn test actors only; no map saves are needed.

Use this test host for listening: the Combat01 packages have not been published to Main. Observe the shared Unreal resource queue and the established 4 GiB launch floor; keep one heavy process running at a time.

The completed automated test used real `TD.Input.Key Two` and `LeftMouseButton` inputs plus these two targets. Its capture shows Lancer marked YOU and 67% weapon heat. The engine reported active `MSS_NeedleCannon`, `MSS_Hull` and `MSS_Shield` cues. Output was Headphones (Xbox Controller), WASAPI, 48 kHz stereo. OS exit 0; zero logged engine errors; 1,123 protected files unchanged. It ran 39.72 seconds and reached 1.553 GiB minimum available RAM.

Evidence: `native_gameplay_01.json`, `review/Lancer_Gameplay01.png`, and the private host directory `Saved/AudioMusic/Combat01/Gameplay01/`. The initial authoring and fresh reload checks are preserved separately.

These results establish event playback in the game. They do not establish subjective sound approval, in-game PCM loudness/clipping, exact rendered-voice limits, Shipping acceptance or Main integration. Active-sound dumps include retiring/deferred-stop voices; the observed maximum of eight cannon entries must not be reported as a measured six-voice hard cap.

The local prepared-file comparison remains available at:

`C:\Users\nicho\OneDrive\Desktop\GameJam\Worktrees\audio-prototype-v1\Saved\AudioMusic\PrototypeV1\Combat01\audition.html`

# Proposed Core handoff - combat music

Nick directly requests that his new original AI track play when gameplay enters combat, including ship battles and ground/melee fights. Audio has prepared the entire 139.2-second song. This is a request for the smallest existing-contract integration; Core chooses and owns its Source implementation.

Proposed sound reference: `/Game/TriadDrift/Audio/CombatMusic01/MUS_Combat_EtherealPulseRemix_01.MUS_Combat_EtherealPulseRemix_01` (native import queued, not available yet). It will be a looping stereo SoundWave, pitch 1, source volume 0.5, using the existing `/Game/TriadDrift/Audio/MenuTheme01/SC_MenuTheme_Music` class without changing that asset. The R020 Music volume control should therefore cover both tracks.

A small optional `UTDUIStyle.CombatMusic` soft SoundBase reference would provide a cook-reachable ingress using the same presentation-asset path as menu music. An automatic native presentation-side consumer can avoid shared map edits. Please confirm the actual property/consumer contract, or propose an existing alternative.

Existing signals in the compiled host:

- `UTDAudioStateSubsystem.GetState`: Scene, Phase, bWaveActive, bSceneCut, Combat01, SecondsSinceHit/Kill.
- `UTDEventBus`: OnWaveStarted/Cleared, OnPhaseChanged, OnHitConfirmed, OnTakeStarted/SceneCut.
- `ATDEncounterVolume`: BlueprintPure IsActive(), IsCleared(), GetAliveCount(), OnCleared. The encounter activates on real player overlap and resets on retake.

Do not gate combat music using MusicIntensity01 alone: idle Ground already targets 0.3, idle Space 0.15, and breathers 0.25. Use actual wave/encounter activity, with an appropriate fallback for combat outside encounters if supported. Dominion combat must be considered without music running in menus or exploration by default.

Requested behavior: one 2D instance per active music controller; fade in on engagement, continue/loop while combat remains active, fade out when it clears; repeated combat notifications must not restart or stack tracks. Respect retakes, death, victory, menu return and world teardown. Proposed transition time is 1.5-2 seconds. Preserve pitch through director slow motion and the existing menu theme's lifetime. Source volume already supplies the 0.5 designer gain: avoid applying that twice accidentally.

Verification needs actual space wave start/clear and actual ground encounter activation/clear, plus no duplicates, cleanup, Music volume control and a loop/listening check. Direct play calls or broadcast-only probes do not prove automatic gameplay activation. Core changes need its normal build/tests and ownership; Audio will perform native sound-reference and audible tests after the compatible host is available.

Nick affirms this is his own original AI-generated song with game-use permission. No purchase or further license approval is requested. The metadata identifies Suno; credits/AI disclosure should record the user-supplied track and file provenance.

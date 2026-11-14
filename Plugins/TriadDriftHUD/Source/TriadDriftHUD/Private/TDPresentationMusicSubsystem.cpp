#include "TDPresentationMusicSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/UObjectGlobals.h"

UTDPresentationMusicSubsystem::UTDPresentationMusicSubsystem()
{
	VerdanceMusic = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/TriadDrift/Audio/VerdanceTheme01/MUS_Verdance_Main_Full_01.MUS_Verdance_Main_Full_01")));
	BoardingMusic = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/TriadDrift/Audio/DominionInterior01/MS_TD_DominionInteriorMusic.MS_TD_DominionInteriorMusic")));
	CreditsTracks.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/TriadDrift/Audio/CreditsMusic01/MUS_Credits_CathedralPulse_Full_01.MUS_Credits_CathedralPulse_Full_01"))));
	CreditsTracks.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/TriadDrift/Audio/CreditsMusic01/MUS_Credits_EtherealPulseRemix_Full_02.MUS_Credits_EtherealPulseRemix_Full_02"))));
}

void UTDPresentationMusicSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(this, &UTDPresentationMusicSubsystem::HandlePreLoadMap);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UTDPresentationMusicSubsystem::HandleWorldCleanup);
}

void UTDPresentationMusicSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMapWithContext.Remove(PreLoadMapHandle);
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	StopCreditsMusic();
	ReleaseBoardingVoice();
	ReleaseVerdanceVoice();
	Super::Deinitialize();
}

void UTDPresentationMusicSubsystem::StartCreditsMusic()
{
	if (bCreditsRollActive) return;
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld()) return;
	if (BoardingState.bActive || VerdanceState.bActive)
	{
		// Preserve a one-shot roll entry while the required boarding fade owns the voice.
		if (BoardingState.bEnding || VerdanceState.bEnding) { bCreditsStartPending = true; PendingCreditsWorld = World; }
		return;
	}
	bCreditsStartPending = false;
	PendingCreditsWorld.Reset();
	bCreditsRollActive = true;
	if (Playlist.Begin()) PlayCurrentTrack();
}

void UTDPresentationMusicSubsystem::StopCreditsMusic()
{
	// Invalidate first: even a queued completion from Stop cannot advance the playlist.
	bCreditsRollActive = false;
	bCreditsStartPending = false;
	PendingCreditsWorld.Reset();
	Playlist.Stop();
	ReleaseVoice();
}

void UTDPresentationMusicSubsystem::ReleaseVoice()
{
	if (Voice)
	{
		UAudioComponent* OldVoice = Voice;
		Voice = nullptr;
		OldVoice->OnAudioFinishedNative.RemoveAll(this);
		OldVoice->Stop();
		OldVoice->DestroyComponent();
	}
}

void UTDPresentationMusicSubsystem::PlayCurrentTrack()
{
	ReleaseVoice();
	UWorld* World = GetWorld();
	USoundBase* Sound = CreditsTracks.IsValidIndex(Playlist.Index) ? CreditsTracks[Playlist.Index].LoadSynchronous() : nullptr;
	if (!World || !World->IsGameWorld() || !Sound)
	{
		UE_LOG(LogTemp, Warning, TEXT("TD credits music unavailable at track %d; check cooked references"), Playlist.Index);
		Playlist.Stop(); // Keep roll active: duplicate start events must not retry/restart it.
		return;
	}
	Voice = NewObject<UAudioComponent>(this);
	Voice->bAutoActivate = false;
	Voice->bAutoDestroy = false;
	Voice->bAllowSpatialization = false;
	Voice->bIsUISound = true;
	Voice->bIgnoreForFlushing = false;
	Voice->SetSound(Sound); // Preserve authored Music class, pitch and gain.
	Voice->OnAudioFinishedNative.AddUObject(this, &UTDPresentationMusicSubsystem::HandleFinished, Playlist.Generation);
	Voice->RegisterComponentWithWorld(World);
	Voice->Play(0.f);
}

void UTDPresentationMusicSubsystem::HandleFinished(UAudioComponent* FinishedVoice, uint64 CallbackGeneration)
{
	if (!bCreditsRollActive || FinishedVoice != Voice || !Playlist.Finish(CallbackGeneration)) return;
	ReleaseVoice();
	if (Playlist.Index != INDEX_NONE) PlayCurrentTrack();
	// After track two, remain silent until an explicit Stop ends this roll.
}

void UTDPresentationMusicSubsystem::HandlePreLoadMap(const FWorldContext& Context, const FString& MapName)
{
	if (Context.OwningGameInstance != GetGameInstance()) return;
	StopCreditsMusic();
	// Confirmed end owns a persistent fade. Unexpected travel aborts; it is not a guessed sequence end.
	if (!BoardingState.bEnding) ReleaseBoardingVoice();
	if (!VerdanceState.bEnding) ReleaseVerdanceVoice();
}

void UTDPresentationMusicSubsystem::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if ((Voice && Voice->GetWorld() == World) || GetWorld() == World) StopCreditsMusic();
	if (BoardingWorld.Get() == World && !BoardingState.bEnding) ReleaseBoardingVoice();
	if (VerdanceWorld.Get() == World && !VerdanceState.bEnding) ReleaseVerdanceVoice();
}

void UTDPresentationMusicSubsystem::StartBoardingMusic()
{
	if (BoardingState.bActive || VerdanceState.bActive || bCreditsRollActive) return;
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld()) return;
	USoundBase* Sound = BoardingMusic.LoadSynchronous();
	if (!Sound)
	{
		UE_LOG(LogTemp, Warning, TEXT("TD boarding music unavailable; check cooked MetaSound and backing Wave"));
		return;
	}
	// Engine creates an audio-device-owned component when persist=true: old world cleanup
	// cannot truncate the explicit 20-second fade. The GI owns the GC reference.
	BoardingVoice = UGameplayStatics::CreateSound2D(World, Sound, 1.f, 1.f, 0.f, nullptr, true, false);
	if (!BoardingVoice) return;
	BoardingState.Begin();
	BoardingWorld = World;
	BoardingVoice->OnAudioFinishedNative.AddUObject(this, &UTDPresentationMusicSubsystem::HandleBoardingFinished, BoardingState.Generation);
	UAudioComponent* StartingVoice = BoardingVoice;
	const uint64 StartingGeneration = BoardingState.Generation;
	OnBoardingMusicOwnershipChanged.Broadcast(true);
	// Ownership listeners can synchronously cancel playback during travel/shutdown.
	if (BoardingVoice != StartingVoice || BoardingState.Generation != StartingGeneration) return;
	if (BoardingState.bEnding)
	{
		// End arrived before any audio started: nothing to fade, and no completion is guaranteed.
		ReleaseBoardingVoice();
		TryPendingCredits();
		return;
	}
	// One gain layer: base multiplier remains 1, ducking uses only ComponentVolumeFader.
	if (BoardingState.bActive && BoardingVoice && !BoardingState.bEnding)
		BoardingVoice->FadeIn(0.f, bBoardingCombatActive ? 0.5f : 1.f, 0.f);
}

void UTDPresentationMusicSubsystem::EndBoardingMusic()
{
	if (BoardingVoice && BoardingState.End()) BoardingVoice->FadeOut(20.f, 0.f);
}

void UTDPresentationMusicSubsystem::SetBoardingCombatActive(bool bInCombat)
{
	bBoardingCombatActive = bInCombat;
	if (BoardingVoice && BoardingState.bActive && !BoardingState.bEnding)
	{
		// -6 dB during a fight, 200 ms ramp; keep the current source position.
		BoardingVoice->AdjustVolume(0.2f, bInCombat ? 0.5f : 1.f);
	}
}

void UTDPresentationMusicSubsystem::ReleaseBoardingVoice()
{
	const bool bWasHolding = BoardingState.bActive;
	BoardingState.Reset();
	BoardingWorld.Reset();
	bBoardingCombatActive = false;
	if (BoardingVoice)
	{
		UAudioComponent* OldVoice = BoardingVoice;
		BoardingVoice = nullptr;
		OldVoice->OnAudioFinishedNative.RemoveAll(this);
		OldVoice->Stop();
		OldVoice->DestroyComponent();
	}
	if (bWasHolding) OnBoardingMusicOwnershipChanged.Broadcast(false);
}

void UTDPresentationMusicSubsystem::HandleBoardingFinished(UAudioComponent* FinishedVoice, uint64 CallbackGeneration)
{
	if (FinishedVoice != BoardingVoice || CallbackGeneration != BoardingState.Generation) return;
	ReleaseBoardingVoice();
	// Stop/exit/travel clears the pending request. Never revive an old world's roll.
	TryPendingCredits();
}

void UTDPresentationMusicSubsystem::TryPendingCredits()
{
	if (bCreditsStartPending && !BoardingState.bActive && !VerdanceState.bActive &&
		PendingCreditsWorld.IsValid() && PendingCreditsWorld.Get() == GetWorld()) StartCreditsMusic();
}

void UTDPresentationMusicSubsystem::StartVerdanceMusic(float FadeInSeconds)
{
	if (VerdanceState.bActive || BoardingState.bActive || bCreditsRollActive) return;
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || !FMath::IsFinite(FadeInSeconds) || FadeInSeconds < 0.f) return;
	USoundBase* Sound = VerdanceMusic.LoadSynchronous();
	if (!Sound)
	{
		UE_LOG(LogTemp, Warning, TEXT("TD Verdance music unavailable; check cooked full theme and Music class"));
		return;
	}
	VerdanceVoice = UGameplayStatics::CreateSound2D(World, Sound, 1.f, 1.f, 0.f, nullptr, true, false);
	if (!VerdanceVoice) return;
	VerdanceState.Begin();
	VerdanceWorld = World;
	VerdanceVoice->OnAudioFinishedNative.AddUObject(this, &UTDPresentationMusicSubsystem::HandleVerdanceFinished, VerdanceState.Generation);
	UAudioComponent* StartingVoice = VerdanceVoice;
	const uint64 StartingGeneration = VerdanceState.Generation;
	OnVerdanceMusicOwnershipChanged.Broadcast(true);
	if (VerdanceVoice != StartingVoice || VerdanceState.Generation != StartingGeneration) return;
	if (VerdanceState.bEnding)
	{
		ReleaseVerdanceVoice();
		TryPendingCredits();
		return;
	}
	if (VerdanceVoice && VerdanceState.bActive && !VerdanceState.bEnding)
		VerdanceVoice->FadeIn(FadeInSeconds, VerdanceDuck.Gain(), 0.f);
}

void UTDPresentationMusicSubsystem::EndVerdanceMusic(float FadeOutSeconds)
{
	if (!FMath::IsFinite(FadeOutSeconds) || FadeOutSeconds < 0.f) return;
	if (VerdanceVoice && VerdanceState.End())
	{
		VerdanceFadeDuration = FadeOutSeconds;
		VerdanceFadeStartGain = VerdanceDuck.Gain();
		VerdanceFadeEndAt = FPlatformTime::Seconds() + FadeOutSeconds;
		VerdanceVoice->FadeOut(FadeOutSeconds, 0.f);
	}
}

void UTDPresentationMusicSubsystem::SetVerdanceDialogueActive(bool bPlaying)
{
	if (VerdanceDuck.bDialogue == bPlaying) return;
	VerdanceDuck.bDialogue = bPlaying;
	if (VerdanceVoice && VerdanceState.bActive && !VerdanceState.bEnding)
		VerdanceVoice->AdjustVolume(bPlaying ? 0.08f : 0.35f, VerdanceDuck.Gain());
}

void UTDPresentationMusicSubsystem::SetVerdanceCombatActive(bool bInCombat)
{
	if (VerdanceDuck.bCombat == bInCombat) return;
	VerdanceDuck.bCombat = bInCombat;
	if (VerdanceVoice && VerdanceState.bActive && !VerdanceState.bEnding)
		VerdanceVoice->AdjustVolume(bInCombat ? 0.08f : 0.35f, VerdanceDuck.Gain());
}

void UTDPresentationMusicSubsystem::ReleaseVerdanceVoice()
{
	const bool bWasHolding = VerdanceState.bActive;
	VerdanceState.Reset();
	VerdanceWorld.Reset();
	VerdanceDuck = FTDVerdanceDuckState();
	VerdanceFadeEndAt = 0.0;
	if (VerdanceVoice)
	{
		UAudioComponent* OldVoice = VerdanceVoice;
		VerdanceVoice = nullptr;
		OldVoice->OnAudioFinishedNative.RemoveAll(this);
		OldVoice->Stop();
		OldVoice->DestroyComponent();
	}
	if (bWasHolding) OnVerdanceMusicOwnershipChanged.Broadcast(false);
}

void UTDPresentationMusicSubsystem::HandleVerdanceFinished(UAudioComponent* FinishedVoice, uint64 CallbackGeneration)
{
	if (FinishedVoice != VerdanceVoice || CallbackGeneration != VerdanceState.Generation) return;
	const float RemainingFade = FMath::Max(0.f, float(VerdanceFadeEndAt - FPlatformTime::Seconds()));
	if (VerdanceState.bEnding && RemainingFade <= 0.f)
	{
		ReleaseVerdanceVoice();
		TryPendingCredits();
	}
	else
	{
		// Keep coverage using the full original, without editing or replacing the native Wave.
		++VerdanceState.Generation;
		VerdanceVoice->OnAudioFinishedNative.RemoveAll(this);
		VerdanceVoice->OnAudioFinishedNative.AddUObject(this, &UTDPresentationMusicSubsystem::HandleVerdanceFinished, VerdanceState.Generation);
		if (VerdanceState.bEnding)
		{
			// A natural track end must not shorten the requested take-off fade.
			// Restart at the remaining envelope level and keep the original deadline.
			const float Gain = VerdanceFadeStartGain * FMath::Clamp(RemainingFade / FMath::Max(VerdanceFadeDuration, SMALL_NUMBER), 0.f, 1.f);
			VerdanceVoice->FadeIn(0.f, Gain, 0.f);
			VerdanceVoice->FadeOut(RemainingFade, 0.f);
		}
		else VerdanceVoice->FadeIn(0.f, VerdanceDuck.Gain(), 0.f);
	}
}

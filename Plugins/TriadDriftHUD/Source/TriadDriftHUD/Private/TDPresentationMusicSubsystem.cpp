#include "TDPresentationMusicSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/UObjectGlobals.h"

UTDPresentationMusicSubsystem::UTDPresentationMusicSubsystem()
{
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
	Super::Deinitialize();
}

void UTDPresentationMusicSubsystem::StartCreditsMusic()
{
	if (bCreditsRollActive) return;
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld()) return;
	if (BoardingState.bActive)
	{
		// Preserve a one-shot roll entry while the required boarding fade owns the voice.
		if (BoardingState.bEnding) { bCreditsStartPending = true; PendingCreditsWorld = World; }
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
}

void UTDPresentationMusicSubsystem::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if ((Voice && Voice->GetWorld() == World) || GetWorld() == World) StopCreditsMusic();
	if (BoardingWorld.Get() == World && !BoardingState.bEnding) ReleaseBoardingVoice();
}

void UTDPresentationMusicSubsystem::StartBoardingMusic()
{
	if (BoardingState.bActive || bCreditsRollActive) return;
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
	OnBoardingMusicOwnershipChanged.Broadcast(true);
	// Ownership listeners can synchronously cancel playback during travel/shutdown.
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
	if (bCreditsStartPending && PendingCreditsWorld.IsValid() && PendingCreditsWorld.Get() == GetWorld()) StartCreditsMusic();
}

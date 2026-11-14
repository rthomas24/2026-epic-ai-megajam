#include "TDPresentationMusicSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "UObject/UObjectGlobals.h"

UTDPresentationMusicSubsystem::UTDPresentationMusicSubsystem()
{
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
	Super::Deinitialize();
}

void UTDPresentationMusicSubsystem::StartCreditsMusic()
{
	if (bCreditsRollActive) return;
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld()) return;
	bCreditsRollActive = true;
	if (Playlist.Begin()) PlayCurrentTrack();
}

void UTDPresentationMusicSubsystem::StopCreditsMusic()
{
	// Invalidate first: even a queued completion from Stop cannot advance the playlist.
	bCreditsRollActive = false;
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
	if (Context.OwningGameInstance == GetGameInstance()) StopCreditsMusic();
}

void UTDPresentationMusicSubsystem::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if ((Voice && Voice->GetWorld() == World) || GetWorld() == World) StopCreditsMusic();
}

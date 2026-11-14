#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TDPresentationMusicSubsystem.generated.h"

class UAudioComponent;
class USoundBase;
class UWorld;
struct FWorldContext;

/** Small playback state shared by the owner and its callback-race tests. */
struct FTDPresentationPlaylistState
{
	uint64 Generation = 0;
	int32 Index = INDEX_NONE;
	bool Begin()
	{
		if (Index != INDEX_NONE) return false;
		++Generation;
		Index = 0;
		return true;
	}
	bool Finish(uint64 CallbackGeneration)
	{
		if (Index == INDEX_NONE || CallbackGeneration != Generation) return false;
		++Generation;
		Index = Index == 0 ? 1 : INDEX_NONE;
		return true;
	}
	void Stop() { ++Generation; Index = INDEX_NONE; }
};

/** Credits playback only. The credits owner calls Start AFTER yielding menu music,
 * Stop BEFORE returning music ownership to the menu. No inferred phase/timer hooks. */
UCLASS()
class TRIADDRIFTHUD_API UTDPresentationMusicSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UTDPresentationMusicSubsystem();
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Idempotent during this roll, including after both tracks finish. */
	UFUNCTION(BlueprintCallable, Category="Triad Drift|Music")
	void StartCreditsMusic();
	/** Ends the roll; also called on map travel. A subsequent roll may start anew. */
	UFUNCTION(BlueprintCallable, Category="Triad Drift|Music")
	void StopCreditsMusic();

private:
	void PlayCurrentTrack();
	void ReleaseVoice();
	void HandleFinished(UAudioComponent* FinishedVoice, uint64 CallbackGeneration);
	void HandlePreLoadMap(const FWorldContext& Context, const FString& MapName);
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	// Explicit soft references for inspection. Shipping cook must include these packages
	// through an AlwaysCook presentation asset/rule; native defaults alone are no cook proof.
	UPROPERTY() TArray<TSoftObjectPtr<USoundBase>> CreditsTracks;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> Voice;
	FTDPresentationPlaylistState Playlist;
	bool bCreditsRollActive = false;
	FDelegateHandle PreLoadMapHandle;
	FDelegateHandle WorldCleanupHandle;
};

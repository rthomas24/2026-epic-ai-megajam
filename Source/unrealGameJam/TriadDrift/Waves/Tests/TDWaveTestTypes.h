// Triad Drift - Enemies & Waves (Teammate 4). Stand-in enemy for wave director tests.
// UHT only parses headers, so the test helper class lives here. Not used by gameplay.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/GameInstance.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Waves/TDWaveEnemy.h"
#include "TDWaveTestTypes.generated.h"

class UTDHealthComponent;

/** A fake game flow: progress is set by the test; calls are recorded. */
UCLASS(NotBlueprintable, Transient, HideDropdown)
class UNREALGAMEJAM_API UTDWaveTestGameInstance : public UGameInstance, public ITDGameFlow
{
	GENERATED_BODY()

public:
	virtual bool TravelTo(ETDDestination Destination, FName ReturnPoint) override { Travels.Add(Destination); return true; }
	virtual FTDProgress GetProgress() const override { return Progress; }
	virtual void SetPhase(ETDGamePhase Phase) override { Progress.Phase = Phase; }
	virtual void ReportWaveCleared(int32 WaveNumber) override { ReportedWaves.Add(WaveNumber); Progress.WavesCleared = FMath::Max(Progress.WavesCleared, WaveNumber); }
	virtual void CompleteObjective(FName ObjectiveId, ETDAbilityId Reward, ETDShipId ForShip) override {}
	virtual void SetDominionState(bool bRevealed, bool bDefensesDown) override { ++DominionStateCalls; Progress.bDominionRevealed = bRevealed; Progress.bDominionDefensesDown = bDefensesDown; }
	virtual void ReportPlayerDefeated() override {}
	virtual void AddStat(FName StatName, int32 Amount) override {}
	virtual void RequestBark(FName BarkId, FName Speaker) override {}
	virtual void ShowTip(FName TipId) override { Tips.Add(TipId); }
	virtual void SaveCheckpoint() override {}

	FTDProgress Progress;
	TArray<int32> ReportedWaves;
	TArray<ETDDestination> Travels;
	int32 DominionStateCalls = 0;
	TArray<FName> Tips;
	TArray<ETDHighlightKind> Highlights;
};

/** Records event-bus wave events (dynamic delegates need UFUNCTION receivers). */
UCLASS(Transient, HideDropdown)
class UNREALGAMEJAM_API UTDWaveTestListener : public UObject
{
	GENERATED_BODY()

public:
	void Bind(UObject* WorldContext);

	UFUNCTION() void OnWaveStarted(int32 WaveNumber, float Intensity01) { Started.Add(WaveNumber); Intensities.Add(Intensity01); }
	UFUNCTION() void OnWaveCleared(int32 WaveNumber) { Cleared.Add(WaveNumber); }
	UFUNCTION() void OnTakeStarted(FName SceneId, int32 TakeNumber) { Takes.Add(FString::Printf(TEXT("%s:%d"), *SceneId.ToString(), TakeNumber)); }
	UFUNCTION() void OnMarkerRevealed(ETDDestination Destination) { Markers.Add(Destination); }
	UFUNCTION() void OnTipRequested(FName TipId) { Tips.Add(TipId); }
	UFUNCTION() void OnHighlight(ETDHighlightKind Kind, AActor* Subject, float Importance01) { Highlights.Add(Kind); }

	TArray<int32> Started;
	TArray<float> Intensities;
	TArray<int32> Cleared;
	TArray<FString> Takes;
	TArray<ETDDestination> Markers;
	TArray<FName> Tips;
	TArray<ETDHighlightKind> Highlights;
};

/** A minimal wave enemy: a health component on the Enemy team, records what the director asked. */
UCLASS(NotBlueprintable, NotPlaceable, HideDropdown)
class UNREALGAMEJAM_API ATDWaveTestEnemy : public AActor, public ITDWaveEnemy
{
	GENERATED_BODY()

public:
	ATDWaveTestEnemy();

	virtual void InitWaveEnemy(FName InEnemyId) override { EnemyId = InEnemyId; }
	virtual void JoinFormation(AActor* InLeader, const FVector& LocalOffset) override { Leader = InLeader; FormationOffset = LocalOffset; }
	virtual void PlayWarpIn() override { bWarpedIn = true; }

	UPROPERTY(VisibleAnywhere, Category = "Test")
	TObjectPtr<UTDHealthComponent> Health;

	FName EnemyId = NAME_None;
	TWeakObjectPtr<AActor> Leader;
	FVector FormationOffset = FVector::ZeroVector;
	bool bWarpedIn = false;
};

// Triad Drift - gameplay event bus (features.md Â§3.4). Lead-owned; append only.
// Gameplay fires; UI, VFX, audio and other systems listen. Get it with UTDEventBus::Get(this).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDEventBus.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnWaveStarted, int32, WaveNumber, float, Intensity01);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnWaveCleared, int32, WaveNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnShipSwitched, ETDShipId, NewShip, ETDShipId, OldShip);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnShipEvent, ETDShipId, Ship);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTDOnSquadWiped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnPowerUsed, ETDShipId, Ship, FName, PowerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnAbilityUnlocked, ETDAbilityId, Ability, ETDShipId, Ship);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnPhaseChanged, ETDGamePhase, Phase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTDOnObjectiveUpdated, FName, ObjectiveId, int32, Step, FText, Text);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnHitConfirmed, AActor*, Target, bool, bKill);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTDOnActorKilled, AActor*, Victim, AActor*, Killer, ETDDamageKind, Kind);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnHyperdriveState, ETDHyperdriveState, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnPortalState, AActor*, Portal, ETDPortalState, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnTipRequested, FName, TipId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnBarkRequested, FName, BarkId, FName, Speaker);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTDOnSpacecraftTeleported, AActor*, Spacecraft, FVector, From, FVector, To);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTDOnHighlightMoment, ETDHighlightKind, Kind, AActor*, Subject, float, Importance01);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnTakeStarted, FName, SceneId, int32, TakeNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTDOnSceneCut, FName, SceneId, int32, NextTakeNumber, float, DelaySeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnMarkerRevealed, ETDDestination, Destination);

UCLASS()
class UNREALGAMEJAM_API UTDEventBus : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Null only when there is no game instance (e.g. during shutdown). Always null-check. */
	static UTDEventBus* Get(const UObject* WorldContextObject);

	// ------------------------------------------------------------------ waves
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnWaveStarted OnWaveStarted;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnWaveCleared OnWaveCleared;

	// ------------------------------------------------------------------ squad
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnShipSwitched OnShipSwitched;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnShipEvent OnShipDowned;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnShipEvent OnShipRespawned;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnSquadWiped OnSquadWiped;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnPowerUsed OnPowerUsed;

	// ------------------------------------------------------------------ progression
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnAbilityUnlocked OnAbilityUnlocked;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnPhaseChanged OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnObjectiveUpdated OnObjectiveUpdated;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnMarkerRevealed OnMarkerRevealed;

	// ------------------------------------------------------------------ combat
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnHitConfirmed OnHitConfirmed;

	/** Fired by UTDHealthComponent for every death (ships, enemies, crew, destructibles). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnActorKilled OnActorKilled;

	// ------------------------------------------------------------------ travel
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnHyperdriveState OnHyperdriveState;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnPortalState OnPortalState;

	/** Portal / gate / hyperdrive moved a spacecraft discontinuously (wingmen follow on this). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnSpacecraftTeleported OnSpacecraftTeleported;

	// ------------------------------------------------------------------ presentation requests
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnTipRequested OnTipRequested;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnBarkRequested OnBarkRequested;

	// ------------------------------------------------------------------ Director's Cut
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnHighlightMoment OnHighlightMoment;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnTakeStarted OnTakeStarted;

	/** "CUT!": the player was defeated; the scene restarts as NextTakeNumber after DelaySeconds (real time). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Events")
	FTDOnSceneCut OnSceneCut;

	// ------------------------------------------------------------------ broadcast helpers
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastWaveStarted(int32 WaveNumber, float Intensity01) { OnWaveStarted.Broadcast(WaveNumber, Intensity01); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastWaveCleared(int32 WaveNumber) { OnWaveCleared.Broadcast(WaveNumber); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastShipSwitched(ETDShipId NewShip, ETDShipId OldShip) { OnShipSwitched.Broadcast(NewShip, OldShip); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastShipDowned(ETDShipId Ship) { OnShipDowned.Broadcast(Ship); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastShipRespawned(ETDShipId Ship) { OnShipRespawned.Broadcast(Ship); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastSquadWiped() { OnSquadWiped.Broadcast(); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastPowerUsed(ETDShipId Ship, FName PowerName) { OnPowerUsed.Broadcast(Ship, PowerName); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastAbilityUnlocked(ETDAbilityId Ability, ETDShipId Ship) { OnAbilityUnlocked.Broadcast(Ability, Ship); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastPhaseChanged(ETDGamePhase Phase) { OnPhaseChanged.Broadcast(Phase); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastObjectiveUpdated(FName ObjectiveId, int32 Step, FText Text) { OnObjectiveUpdated.Broadcast(ObjectiveId, Step, Text); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastMarkerRevealed(ETDDestination Destination) { OnMarkerRevealed.Broadcast(Destination); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastHitConfirmed(AActor* Target, bool bKill) { OnHitConfirmed.Broadcast(Target, bKill); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastActorKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind) { OnActorKilled.Broadcast(Victim, Killer, Kind); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastHyperdriveState(ETDHyperdriveState State) { OnHyperdriveState.Broadcast(State); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastPortalState(AActor* Portal, ETDPortalState State) { OnPortalState.Broadcast(Portal, State); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastSpacecraftTeleported(AActor* Spacecraft, FVector From, FVector To) { OnSpacecraftTeleported.Broadcast(Spacecraft, From, To); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastTipRequested(FName TipId) { OnTipRequested.Broadcast(TipId); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastBarkRequested(FName BarkId, FName Speaker) { OnBarkRequested.Broadcast(BarkId, Speaker); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastTakeStarted(FName SceneId, int32 TakeNumber) { OnTakeStarted.Broadcast(SceneId, TakeNumber); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Events")
	void BroadcastSceneCut(FName SceneId, int32 NextTakeNumber, float DelaySeconds) { OnSceneCut.Broadcast(SceneId, NextTakeNumber, DelaySeconds); }

private:
	// Money shots go through UTDDirectorSubsystem::ReportHighlight (rate limit, slow-mo, camera hint,
	// blocked during CUT and travel). Only the director may put one on the bus.
	friend class UTDDirectorSubsystem;
	void BroadcastHighlightMoment(ETDHighlightKind Kind, AActor* Subject, float Importance01) { OnHighlightMoment.Broadcast(Kind, Subject, Importance01); }
};

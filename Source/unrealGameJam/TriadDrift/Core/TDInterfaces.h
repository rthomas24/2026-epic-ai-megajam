// Triad Drift - shared interfaces (features.md §3.2). Lead-owned; append only.
// C++ implements them; Blueprint (Nick's widgets) can call them.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDInterfaces.generated.h"

// ---------------------------------------------------------------------------------------------- Team

UINTERFACE(MinimalAPI, BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class UTDTeam : public UInterface
{
	GENERATED_BODY()
};

class UNREALGAMEJAM_API ITDTeam
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Team")
	virtual ETDTeam GetTeam() const PURE_VIRTUAL(ITDTeam::GetTeam, return ETDTeam::Neutral;);
};

// ---------------------------------------------------------------------------------------- Deflectable

UINTERFACE(MinimalAPI, BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class UTDDeflectable : public UInterface
{
	GENERATED_BODY()
};

/** Projectiles: the energy blade, Rift's portals and Bastion's dome send them back. */
class UNREALGAMEJAM_API ITDDeflectable
{
	GENERATED_BODY()

public:
	/** Flip to NewTeam, fly along NewDirection (re-homing on the original shooter if homing). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Combat")
	virtual void Deflect(AActor* NewInstigator, FVector NewDirection, ETDTeam NewTeam, float SpeedMultiplier)
		PURE_VIRTUAL(ITDDeflectable::Deflect, );
};

// --------------------------------------------------------------------------------------- Interactable

UINTERFACE(MinimalAPI, BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class UTDInteractable : public UInterface
{
	GENERATED_BODY()
};

class UNREALGAMEJAM_API ITDInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Interact")
	virtual FText GetPrompt() const { return FText::GetEmpty(); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Interact")
	virtual bool CanInteract(AActor* Interactor) const { return true; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Interact")
	virtual void Interact(AActor* Interactor) PURE_VIRTUAL(ITDInteractable::Interact, );

	/** Seconds the interact button must be held; 0 = tap. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Interact")
	virtual float GetHoldTime() const { return 0.f; }
};

// ------------------------------------------------------------------------------------------- HUD data

UINTERFACE(MinimalAPI, BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class UTDHUDData : public UInterface
{
	GENERATED_BODY()
};

/**
 * Everything the HUD widgets read. The squad manager implements the space half, the first-person
 * character the ground half; each leaves the other half at its defaults.
 */
class UNREALGAMEJAM_API ITDHUDData
{
	GENERATED_BODY()

public:
	// ---- space
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	virtual TArray<FTDShipHUD> GetSquadHUD() const { return TArray<FTDShipHUD>(); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	virtual AActor* GetCurrentTarget() const { return nullptr; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	virtual TArray<AActor*> GetLockedTargets() const { return TArray<AActor*>(); }

	/** Every hostile (and incoming missile) for brackets / edge arrows. ENG-06; forwarded from UTDTargetingComponent. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	virtual TArray<FTDTargetMarker> GetTargetMarkers() const { return TArray<FTDTargetMarker>(); }

	/** Incoming-missile warning state (Nick supplies the sound and UI). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	virtual ETDMissileWarning GetMissileWarning() const { return ETDMissileWarning::None; }

	/** Lead-indicator intercept point for the current target; false when there is none. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	virtual bool GetLeadIndicator(FVector& OutWorldLocation) const { return false; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	virtual FTDWaveInfo GetWaveInfo() const { return FTDWaveInfo(); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	virtual float GetHyperdriveCharge01() const { return 0.f; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	virtual TArray<AActor*> GetObjectiveMarkers() const { return TArray<AActor*>(); }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	virtual float GetSpeed01() const { return 0.f; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	virtual bool IsOutOfBounds() const { return false; }

	/** Why the hyperdrive cannot be used right now ("HYPERDRIVE LOCKED - ENEMIES IN RANGE"); empty when it can. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	virtual FText GetHyperdriveBlockText() const { return FText::GetEmpty(); }

	/** Metres above ground in atmosphere flight; negative when not applicable. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	virtual float GetAltitude() const { return -1.f; }

	// ---- ground
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	virtual float GetHealth01() const { return 1.f; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	virtual float GetBlockStamina01() const { return 1.f; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	virtual int32 GetDashCharges() const { return 0; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	virtual FText GetInteractPrompt() const { return FText::GetEmpty(); }
};

// ------------------------------------------------------------------------------------------ Game flow

UINTERFACE(MinimalAPI, BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class UTDGameFlow : public UInterface
{
	GENERATED_BODY()
};

/** Implemented by UTDGameInstance. Reach it with UTDStatics::GetGameFlow(). */
class UNREALGAMEJAM_API ITDGameFlow
{
	GENERATED_BODY()

public:
	/** Returns false (and stays put) if the destination's map is missing. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	virtual bool TravelTo(ETDDestination Destination, FName ReturnPoint) PURE_VIRTUAL(ITDGameFlow::TravelTo, return false;);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	virtual FTDProgress GetProgress() const PURE_VIRTUAL(ITDGameFlow::GetProgress, return FTDProgress(););

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	virtual void SetPhase(ETDGamePhase Phase) PURE_VIRTUAL(ITDGameFlow::SetPhase, );

	/** WaveNumber is 1-based. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	virtual void ReportWaveCleared(int32 WaveNumber) PURE_VIRTUAL(ITDGameFlow::ReportWaveCleared, );

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	virtual void CompleteObjective(FName ObjectiveId, ETDAbilityId Reward, ETDShipId ForShip)
		PURE_VIRTUAL(ITDGameFlow::CompleteObjective, );

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	virtual void SetDominionState(bool bRevealed, bool bDefensesDown) PURE_VIRTUAL(ITDGameFlow::SetDominionState, );

	/** Squad wipe or on-foot death: "CUT!" and restart the scene as the next take. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	virtual void ReportPlayerDefeated() PURE_VIRTUAL(ITDGameFlow::ReportPlayerDefeated, );

	/** StatName is an FTDRunStats field name, e.g. "Kills". */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	virtual void AddStat(FName StatName, int32 Amount) PURE_VIRTUAL(ITDGameFlow::AddStat, );

	/** Speaker is a name: "Wren", "Kato", "Moss", "Armada". */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	virtual void RequestBark(FName BarkId, FName Speaker) PURE_VIRTUAL(ITDGameFlow::RequestBark, );

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	virtual void ShowTip(FName TipId) PURE_VIRTUAL(ITDGameFlow::ShowTip, );

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flow")
	virtual void SaveCheckpoint() PURE_VIRTUAL(ITDGameFlow::SaveCheckpoint, );
};

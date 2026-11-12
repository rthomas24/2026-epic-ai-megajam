// Triad Drift - landing pad (ENG-20). Owner: Teammate 1.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDLandingPad.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class ATDShip;

/**
 * Where a ship lands on a planet. The player lands on a non-wingman pad ("Hold F to land" within
 * PromptRange and below the speed limit); wingmen land on side pads (SidePads, or the nearest
 * wingman pads within WingmanPadSearchRadius). Ships land facing the pad's forward (+X).
 */
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDLandingPad : public AActor
{
	GENERATED_BODY()

public:
	ATDLandingPad();

	/** Top-centre of the pad (beacon; touch-down point). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	FVector GetBeaconLocation() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	bool IsWingmanPad() const { return bWingmanPad; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	bool IsOccupied() const { return Occupant.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	ATDShip* GetOccupant() const { return Occupant.Get(); }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	bool IsPadEnabled() const { return bPadEnabled; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Landing")
	void SetPadEnabled(bool bEnabled) { bPadEnabled = bEnabled; }

	/** Wingman pads take the squadmates; the player never lands on one. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Landing")
	void SetWingmanPad(bool bInWingmanPad) { bWingmanPad = bInWingmanPad; }

	void SetSidePads(const TArray<ATDLandingPad*>& InSidePads) { SidePads.Reset(); for (ATDLandingPad* P : InSidePads) { SidePads.Add(P); } }

	/** Reserve / release (landing component). Returns false if another ship holds it. */
	bool Reserve(ATDShip* Ship);
	void Release(ATDShip* Ship);

	const TArray<TObjectPtr<ATDLandingPad>>& GetSidePads() const { return SidePads; }
	float GetWingmanPadSearchRadius() const { return WingmanPadSearchRadius; }
	float GetPromptRange() const { return PromptRange; }
	const FText& GetPromptText() const { return PromptText; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Landing")
	TObjectPtr<USceneComponent> Root;

	/** Placeholder deck (engine cylinder). Art replaces it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Landing")
	TObjectPtr<UStaticMeshComponent> PadMesh;

	/** Wingmen land here, not the player. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Landing")
	bool bWingmanPad = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Landing")
	bool bPadEnabled = true;

	/** Explicit wingman pads for this (player) pad; otherwise the nearest wingman pads are used. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Landing")
	TArray<TObjectPtr<ATDLandingPad>> SidePads;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Landing", meta = (ClampMin = "0"))
	float WingmanPadSearchRadius = 8000.f;

	/** "Hold F to land" appears within this distance of the beacon (features.md: 3000 cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Landing", meta = (ClampMin = "0"))
	float PromptRange = 12000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Landing")
	FText PromptText;

	/** Height of the deck top above the actor origin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Landing")
	float DeckHeight = 40.f;

private:
	TWeakObjectPtr<ATDShip> Occupant;
};

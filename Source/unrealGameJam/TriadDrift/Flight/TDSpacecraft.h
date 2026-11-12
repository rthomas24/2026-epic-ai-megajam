// Triad Drift - the flyable body shared by squad ships and enemy ships (ENG-01). Owner: Teammate 1.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Flight/TDFlightTypes.h"
#include "TDSpacecraft.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UTDShipMovementComponent;
class UTDHealthComponent;
class UDamageType;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FTDOnTeleported, ATDSpacecraft*, Spacecraft, FVector, FromLocation, FVector, ToLocation, FRotator, NewFacing);

/**
 * A flyable spacecraft: collision sphere root, placeholder hull mesh (engine basic shape), arcade
 * movement, health. Fly it only through SetFlightInput (player controller, wingman AI and enemy AI
 * all use the same path). Possession changes never touch velocity.
 */
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDSpacecraft : public APawn, public ITDTeam
{
	GENERATED_BODY()

public:
	ATDSpacecraft();

	// ---- Flight -----------------------------------------------------------------------------------

	/** Latched: stays applied until the next call. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void SetFlightInput(const FTDFlightInput& In);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	const FTDFlightInput& GetFlightInput() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	UTDShipMovementComponent* GetShipMovement() const { return ShipMovement; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void ApplyFlightTuning(const FTDFlightTuning& Tuning);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	float GetSpeed01() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	bool IsBoosting() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	bool IsDrifting() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	float GetBoost01() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	bool IsOutOfBounds() const;

	/** Instant move (portals, gates, hyperdrive drop-out). Fires OnTeleported. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void TeleportSpacecraft(const FVector& Location, const FRotator& Facing, bool bKeepSpeed);

	/** Forces a speed (hyperdrive, ram). SteeringScale multiplies turn and roll rates. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void SetSpeedOverride(float SpeedCmPerSec, float SteeringScale);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void ClearSpeedOverride();

	/** Fired after every TeleportSpacecraft (wingmen warp-follow, presentation transit FX). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Flight")
	FTDOnTeleported OnTeleported;

	// ---- Components, team, health -------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "TriadDrift")
	UTDHealthComponent* GetHealth() const { return Health; }

	/** Placeholder hull (engine basic shape). Presentation swaps the mesh later. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift")
	UStaticMeshComponent* GetHullMesh() const { return HullMesh; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift")
	USphereComponent* GetCollisionSphere() const { return CollisionSphere; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift")
	void SetTeam(ETDTeam NewTeam);

	virtual ETDTeam GetTeam() const override { return Team; }

	// ---- AActor / APawn ------------------------------------------------------------------------------
	virtual void Tick(float DeltaSeconds) override;
	virtual UPawnMovementComponent* GetMovementComponent() const override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleFlightImpact(const FTDFlightImpact& Impact);

	/** Applies the visual bank to the hull mesh. */
	void UpdateHullPresentation(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift")
	TObjectPtr<USphereComponent> CollisionSphere;

	/** Pivot for visual bank; the hull mesh hangs off it so the mesh keeps its own relative transform. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift")
	TObjectPtr<USceneComponent> HullPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift")
	TObjectPtr<UStaticMeshComponent> HullMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift")
	TObjectPtr<UTDShipMovementComponent> ShipMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift")
	TObjectPtr<UTDHealthComponent> Health;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift")
	ETDTeam Team = ETDTeam::Neutral;

	/** Damage kind for collision damage (glancing scrapes and head-on hits). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Flight")
	ETDDamageKind CollisionDamageKind = ETDDamageKind::Environmental;

	/** Placeholder wings (engine cube) so the silhouette reads before art exists. Hidden once real art is set. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift")
	TObjectPtr<UStaticMeshComponent> PlaceholderWings;

	/** Pitch added on top of the visual bank so the nose dips into hard turns (deg at full pitch rate). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Flight")
	float VisualPitchLean = 6.f;

private:
	float VisualPitch = 0.f;
};

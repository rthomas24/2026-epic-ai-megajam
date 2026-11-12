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
class UTDWeaponComponent;
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

	/** Metres above the ground straight below (atmosphere flight HUD), or -1 if nothing is below within 20 km. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	float GetAltitudeMeters() const;

	/** Instant move (portals, gates, hyperdrive drop-out). Fires OnTeleported. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void TeleportSpacecraft(const FVector& Location, const FRotator& Facing, bool bKeepSpeed);

	/** Forces a speed (hyperdrive, ram). SteeringScale multiplies turn and roll rates. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void SetSpeedOverride(float SpeedCmPerSec, float SteeringScale);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void ClearSpeedOverride();

	/**
	 * Clean flight state: latched input cleared, boost meter full, no drift / boost / speed override /
	 * catch-up allowance, angular rates zeroed. bKeepVelocity=false restarts at cruise along the nose.
	 * Squad: call on ship down and on respawn.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void ResetFlightState(bool bKeepVelocity = false);

	/** Parked: the flight model stops simulating (zero velocity, input ignored). Downed / hidden ships, landing. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Flight")
	void SetFlightParked(bool bParked);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Flight")
	bool IsFlightParked() const;

	/** Fired after every TeleportSpacecraft (wingmen warp-follow, presentation transit FX). */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Flight")
	FTDOnTeleported OnTeleported;

	// ---- Components, team, health -------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "TriadDrift")
	UTDHealthComponent* GetHealth() const { return Health; }

	/** T2's weapons (squad and enemy ships both fire through it). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift")
	UTDWeaponComponent* GetWeapons() const { return Weapons; }

	/** Placeholder hull (engine basic shape). Presentation swaps the mesh later. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift")
	UStaticMeshComponent* GetHullMesh() const { return HullMesh; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift")
	USphereComponent* GetCollisionSphere() const { return CollisionSphere; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift")
	void SetTeam(ETDTeam NewTeam);

	/**
	 * Show / hide the placeholder-only parts (cube wings, engine glow block). Call with false once a
	 * real presentation mesh is on the hull (ATDShip does this itself from its UTDShipVisuals).
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift")
	void SetPlaceholderPartsVisible(bool bVisible);

	/** Re-applies the placeholder colours (team colour here; squad colour on ATDShip). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift")
	virtual void ApplyPlaceholderLook();

	UFUNCTION(BlueprintPure, Category = "TriadDrift")
	UStaticMeshComponent* GetEngineGlow() const { return EngineGlow; }

	virtual ETDTeam GetTeam() const override { return Team; }

	// ---- AActor / APawn ------------------------------------------------------------------------------
	virtual void Tick(float DeltaSeconds) override;
	virtual UPawnMovementComponent* GetMovementComponent() const override;
	/** APawn::Restart (run on every possess) stops movement; ships keep flying through a switch. */
	virtual void Restart() override;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift")
	TObjectPtr<UTDWeaponComponent> Weapons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift")
	ETDTeam Team = ETDTeam::Neutral;

	/** Damage kind for collision damage (glancing scrapes and head-on hits). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Flight")
	ETDDamageKind CollisionDamageKind = ETDDamageKind::Environmental;

	/** Placeholder wings (engine cube) so the silhouette reads before art exists. Hidden once real art is set. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift")
	TObjectPtr<UStaticMeshComponent> PlaceholderWings;

	/** Placeholder engine block at the tail: glows in the ship colour and stretches with boost. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift")
	TObjectPtr<UStaticMeshComponent> EngineGlow;

	/** Emissive strength of the placeholder engine glow. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift")
	float EngineGlowBoost = 8.f;

	/** Engine glow length at cruise and at full boost (relative X scale). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift")
	FVector2D EngineGlowLength = FVector2D(0.35f, 1.4f);

	/** Colour used for the placeholder look (resolved by ApplyPlaceholderLook). */
	virtual FLinearColor GetPlaceholderColor() const;

	/** Tints the placeholder hull, wings and engine block (no-op once real art replaced them). */
	void TintPlaceholderParts();

	/** Puts the engine cone back on the hull component (after real art is removed). */
	void RestorePlaceholderHull();

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> PlaceholderHullMesh;

	FRotator PlaceholderHullRotation = FRotator(-90.f, 0.f, 0.f);
	FVector PlaceholderHullScale = FVector(1.6f, 1.6f, 4.2f);

	bool bPlaceholderParts = true;

	/** Pitch added on top of the visual bank so the nose dips into hard turns (deg at full pitch rate). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Flight")
	float VisualPitchLean = 6.f;

private:
	float VisualPitch = 0.f;
	float GlowLength01 = 0.f;
};

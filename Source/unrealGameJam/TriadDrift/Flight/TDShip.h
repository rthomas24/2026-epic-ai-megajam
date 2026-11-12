// Triad Drift - a squad ship with the chase camera (ENG-01). Owner: Teammate 1 (Flight & Input).

#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TDShip.generated.h"

class USpringArmComponent;
class UCameraComponent;

/** Chase-camera feel. Defaults are features.md ENG-01. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDChaseCameraTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float ArmLength = 1400.f;

	/** Camera height above the ship's centre line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float Height = 280.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float RotationLag = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float LocationLag = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "5", ClampMax = "170"))
	float MinFOV = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "5", ClampMax = "170"))
	float MaxFOV = 110.f;

	/** How quickly the FOV follows speed (1/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float FOVResponse = 4.f;

	/** Fraction of the ship's roll (and visual bank) the camera follows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0", ClampMax = "1"))
	float RollFollow = 0.3f;

	/** Camera leads into turns by up to this many degrees at full turn rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float LookAheadAngle = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera", meta = (ClampMin = "0"))
	float LookAheadResponse = 5.f;
};

/** A squad ship (Rift, Lancer, Bastion). Squad stats come from T3's UTDShipLibrary through ApplyFlightTuning. */
UCLASS(Blueprintable)
class UNREALGAMEJAM_API ATDShip : public ATDSpacecraft
{
	GENERATED_BODY()

public:
	ATDShip();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ship")
	ETDShipId GetShipId() const { return ShipId; }

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ship")
	void SetShipId(ETDShipId NewId);

	/**
	 * Only toggles the chase camera (active + ticking). Never touches velocity, input latching or
	 * possession: the squad manager possesses / unpossesses around this call.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Ship")
	void SetPlayerControlled(bool bInPlayerControlled);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ship")
	bool IsPlayerControlled() const { return bPlayerControlled; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Camera")
	USpringArmComponent* GetSpringArm() const { return SpringArm; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Camera")
	UCameraComponent* GetChaseCamera() const { return ChaseCamera; }

	/** Snaps camera lag and FOV to their targets (switch cuts, teleports). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Camera")
	void SnapCamera();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	void UpdateChaseCamera(float DeltaSeconds, bool bSnap);

	UFUNCTION()
	void HandleTeleported(ATDSpacecraft* Spacecraft, FVector FromLocation, FVector ToLocation, FRotator NewFacing);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Ship")
	ETDShipId ShipId = ETDShipId::Rift;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TriadDrift|Camera")
	FTDChaseCameraTuning CameraTuning;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TriadDrift|Camera")
	TObjectPtr<UCameraComponent> ChaseCamera;

private:
	bool bPlayerControlled = false;
	float CurrentFOV = 90.f;
	FVector2D LookAhead = FVector2D::ZeroVector;
};

// Triad Drift - Enemies & Waves (Teammate 4). ENG-13 Hive Carrier launch bay.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TDCarrierBay.generated.h"

class UTDHealthComponent;
class UStaticMeshComponent;
class ATDEnemyShip;
class ATDCarrierBay;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDCarrierBaySignature, ATDCarrierBay*, Bay);

/**
 * A destructible launch bay attached to a Hive Carrier. Every LaunchInterval seconds it telegraphs
 * (doors open) and then launches FightersPerLaunch fighters through its owner. Destroying it stops
 * its launches for good. Lancer's ram breaks its doors (BreakDoors), exposing it to full damage.
 */
UCLASS()
class UNREALGAMEJAM_API ATDCarrierBay : public AActor, public ITDTeam
{
	GENERATED_BODY()

public:
	ATDCarrierBay();

	void Init(ATDEnemyShip* InCarrier, float Hull, int32 InFightersPerLaunch, float InLaunchInterval, float InTelegraphSeconds, float InitialDelay);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Carrier")
	bool IsAlive() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Carrier")
	bool IsLaunchTelegraphing() const { return bTelegraphing; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Carrier")
	float GetTimeToNextLaunch() const { return NextLaunchRemaining; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Carrier")
	int32 GetLaunchCount() const { return LaunchCount; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Carrier")
	UTDHealthComponent* GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Carrier")
	ATDEnemyShip* GetCarrier() const { return Carrier.Get(); }

	/** Ram hit: doors break, damage taken is multiplied by DoorsBrokenDamageMultiplier. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Carrier")
	void BreakDoors();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Carrier")
	bool AreDoorsBroken() const { return bDoorsBroken; }

	/** Pause launches (carrier stunned by EMP). */
	void SetPaused(bool bInPaused) { bPaused = bInPaused; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Carrier", meta = (ClampMin = "1"))
	float DoorsBrokenDamageMultiplier = 2.f;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Carrier")
	FTDCarrierBaySignature OnLaunchTelegraph;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Carrier")
	FTDCarrierBaySignature OnLaunched;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Carrier")
	FTDCarrierBaySignature OnBayDestroyed;

	// ITDTeam
	virtual ETDTeam GetTeam() const override;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow);

	float FilterDamage(const FTDDamageInfo& Info);

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Carrier")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "TriadDrift|Carrier")
	TObjectPtr<UTDHealthComponent> Health;

	TWeakObjectPtr<ATDEnemyShip> Carrier;
	int32 FightersPerLaunch = 3;
	float LaunchInterval = 12.f;
	float TelegraphSeconds = 1.5f;
	float NextLaunchRemaining = 12.f;
	int32 LaunchCount = 0;
	bool bTelegraphing = false;
	bool bDoorsBroken = false;
	bool bPaused = false;
	bool bDestroyed = false;
};

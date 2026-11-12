// Triad Drift - Powers (Teammate 2). Lancer: Afterburner Ram (ENG-10).

#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Enemies/TDEnemyTypes.h"
#include "TDRamPower.generated.h"

struct FTDFlightImpact;

/**
 * 0.3 s wind-up (PowerWindup01 for the wing fold), then a 1.2 s dash at 3x boost speed with 40% steering.
 * Lancer is invulnerable throughout and until its speed bleeds back to boost speed. During the dash Lancer
 * ignores ships for movement (it really passes through fighters); everything hostile its path touches
 * takes 300 Ram damage once, gunship frontal shields and carrier bay doors break. A Large/Boss enemy, or any
 * non-ship blocker the flight model hits (rock, Dominion hull), ends the dash with a 150-damage, 1500 cm
 * shockwave (the stopper is excluded). Refused while another speed override (hyperdrive, atmosphere entry)
 * is active. 7 s cooldown.
 */
UCLASS(Blueprintable, ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDRamPower : public UTDPowerComponent
{
	GENERATED_BODY()

public:
	UTDRamPower();

	virtual float GetWindup01() const override;
	virtual bool CanActivate() const override;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ram")
	bool IsDashing() const { return IsPowerActive() && bDashStarted; }

	/** Actors hit by the current (or last) dash. */
	const TArray<TWeakObjectPtr<AActor>>& GetHitThisDash() const { return HitThisDash; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ram")
	bool WasStoppedByImpact() const { return bStoppedByImpact; }

	/** Distance flown during the last dash (tests, tuning). */
	float GetLastDashDistance() const { return DashDistance; }

	// --- Tuning (ENG-10) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Ram", meta = (Units = "s"))
	float WindupSeconds = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Ram", meta = (Units = "s"))
	float DashSeconds = 1.2f;

	/** Dash speed = this x the ship's boost speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Ram")
	float BoostSpeedMultiplier = 3.f;

	/** Used when the owner has no ship movement to read boost speed from. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Ram", meta = (Units = "CentimetersPerSecond"))
	float FallbackBoostSpeed = 15000.f;

	/** Override acceleration during the dash: effectively an instant kick to dash speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Ram", meta = (Units = "CentimetersPerSecondSquared"))
	float DashAcceleration = 400000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Ram")
	float SteeringScale = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Ram")
	float RamDamage = 300.f;

	/** Extra reach beyond Lancer's own bounds radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Ram", meta = (Units = "cm"))
	float HitPadding = 150.f;

	/** Enemies of this size or bigger stop the dash. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Ram")
	ETDEnemySize StopOnSize = ETDEnemySize::Large;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Ram")
	float ShockwaveDamage = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Ram", meta = (Units = "cm"))
	float ShockwaveRadius = 1500.f;

	/** After the dash, stay invulnerable until speed is back to boost speed, but never longer than this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Ram", meta = (Units = "s"))
	float MaxPostDashInvulnerableSeconds = 1.5f;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual bool OnActivate() override;
	virtual void OnTickActive(float DeltaTime) override;
	virtual void OnEnd(bool bInterrupted) override;

private:
	void BeginDash();
	void SweepHits(const FVector& From, const FVector& To);
	void HitActor(AActor* Victim, const FVector& Direction);
	void StopDash(const FVector& Where, AActor* Stopper);
	void SetShipCollisionIgnored(bool bIgnore);
	float GetDashSpeed() const;
	float GetBoostSpeed() const;
	bool IsBig(const AActor* Actor) const;
	bool IsHyperdriveBusy() const;
	void RefreshInvulnerability(float Seconds);

	UFUNCTION()
	void HandleFlightImpact(const FTDFlightImpact& Impact);

	TArray<TWeakObjectPtr<AActor>> HitThisDash;
	/** Reused every tick (no per-frame allocation). */
	TArray<AActor*> HostilesScratch;
	FVector LastLocation = FVector::ZeroVector;
	FVector DashDirection = FVector::ForwardVector;
	float DashDistance = 0.f;
	float PostDashGrace = 0.f;
	bool bDashStarted = false;
	bool bStoppedByImpact = false;
	bool bIgnoringShips = false;
	bool bOwnsOverride = false;
};

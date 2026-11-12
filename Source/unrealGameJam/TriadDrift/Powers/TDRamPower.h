// Triad Drift - Powers (Teammate 2). Lancer: Afterburner Ram (ENG-10).

#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Enemies/TDEnemyTypes.h"
#include "TDRamPower.generated.h"

/**
 * 0.3 s wind-up (PowerWindup01 for the wing fold), then a 1.2 s dash at 3x boost speed with 40% steering.
 * Lancer is invulnerable throughout. Everything hostile it touches takes 300 Ram damage once; fighters are
 * smashed through, gunship frontal shields break, carrier bay doors break. Hitting something big (or
 * world geometry) ends the dash with a 150-damage, 1500 cm shockwave. 7 s cooldown.
 */
UCLASS(ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDRamPower : public UTDPowerComponent
{
	GENERATED_BODY()

public:
	UTDRamPower();

	virtual float GetWindup01() const override;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ram")
	bool IsDashing() const { return IsPowerActive() && ActiveElapsed >= WindupSeconds; }

	/** Actors hit by the current (or last) dash. */
	const TArray<TWeakObjectPtr<AActor>>& GetHitThisDash() const { return HitThisDash; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Ram")
	bool WasStoppedByImpact() const { return bStoppedByImpact; }

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

	/** Grace invulnerability after the dash ends (so you don't eat the debris). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Ram", meta = (Units = "s"))
	float PostDashInvulnerableSeconds = 0.25f;

protected:
	virtual bool OnActivate() override;
	virtual void OnTickActive(float DeltaTime) override;
	virtual void OnEnd(bool bInterrupted) override;

private:
	void BeginDash();
	void SweepHits(const FVector& From, const FVector& To);
	void HitActor(AActor* Victim, const FVector& Direction);
	void Shockwave(const FVector& Where);
	float GetDashSpeed() const;
	bool IsBig(const AActor* Actor) const;

	TArray<TWeakObjectPtr<AActor>> HitThisDash;
	FVector LastLocation = FVector::ZeroVector;
	FVector DashDirection = FVector::ForwardVector;
	bool bDashStarted = false;
	bool bStoppedByImpact = false;
};

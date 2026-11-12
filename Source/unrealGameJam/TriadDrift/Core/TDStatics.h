// Triad Drift - shared helpers: teams, damage, presentation cues, game-feel time dilation. Lead-owned.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Subsystems/WorldSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TDStatics.generated.h"

class ITDGameFlow;
class UNiagaraComponent;
class USceneComponent;
class UTDHealthComponent;

UCLASS()
class UNREALGAMEJAM_API UTDStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ------------------------------------------------------------------ teams
	/** ITDTeam on the actor, else its UTDHealthComponent::DefaultTeam, else Neutral. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Team")
	static ETDTeam GetTeam(const AActor* Actor);

	/** True only for Player vs Enemy (either way). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Team")
	static bool AreTeamsHostile(ETDTeam A, ETDTeam B);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Team")
	static bool AreHostile(const AActor* A, const AActor* B);

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Health")
	static UTDHealthComponent* GetHealth(const AActor* Actor);

	// ------------------------------------------------------------------ damage
	/**
	 * THE way to deal damage. Friendly fire is off: the hit is dropped when the source team
	 * (Causer's team if it has one, else Instigator's) equals the target's team.
	 * Returns the damage actually dealt.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Damage")
	static float ApplyTDDamage(AActor* Target, float Amount, ETDDamageKind Kind, AActor* Instigator, AActor* Causer,
		FVector HitLocation = FVector::ZeroVector, FVector HitDirection = FVector::ZeroVector, bool bIgnoreShield = false);

	/**
	 * Splash damage to every live hostile of SourceTeam within Radius of Origin (measured to the
	 * actor's bounds). bFalloff scales linearly to 25% at the edge. Returns the number of actors hit.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Damage", meta = (WorldContext = "WorldContextObject"))
	static int32 ApplyTDRadialDamage(const UObject* WorldContextObject, FVector Origin, float Radius, float Amount,
		ETDDamageKind Kind, ETDTeam SourceTeam, AActor* Instigator, AActor* Causer, bool bFalloff = false,
		AActor* IgnoreActor = nullptr);

	// ------------------------------------------------------------------ flow
	/** The game flow (UTDGameInstance), or null in worlds that run a plain game instance. */
	static ITDGameFlow* GetGameFlow(const UObject* WorldContextObject);

	// ------------------------------------------------------------------ presentation (all null-safe)
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|FX", meta = (WorldContext = "WorldContextObject"))
	static UNiagaraComponent* PlayCue(const UObject* WorldContextObject, const FTDFXCue& Cue, FVector Location,
		FRotator Rotation = FRotator::ZeroRotator);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|FX")
	static UNiagaraComponent* PlayCueAttached(const FTDFXCue& Cue, USceneComponent* AttachTo, FName Socket = NAME_None);

	/** True in the editor / dev builds when placeholder debug drawing should stand in for missing art. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|FX")
	static bool ShouldDrawPlaceholders();
};

/**
 * Game-feel time control: hit-stop and slow motion share one owner of global time dilation so they
 * never fight. Timers run on real (undilated) time.
 */
UCLASS()
class UNREALGAMEJAM_API UTDFeelSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDFeelSubsystem* Get(const UObject* WorldContextObject);

	/** Brief freeze on impact. Defaults: 40 ms at 5% speed (features.md ENG-05). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Feel")
	void RequestHitStop(float RealSeconds = 0.04f, float Dilation = 0.05f);

	/** Cinematic slow motion (Director's Cut money shots). The slowest active request wins. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Feel")
	void RequestSlowMo(float RealSeconds, float Dilation = 0.3f);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Feel")
	void CancelAll();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Feel")
	float GetCurrentDilation() const { return CurrentDilation; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Feel")
	bool IsSlowMoActive() const;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	void Apply();

	double HitStopEnd = 0.0;
	float HitStopDilation = 1.f;
	double SlowMoEnd = 0.0;
	float SlowMoDilation = 1.f;
	float CurrentDilation = 1.f;
};

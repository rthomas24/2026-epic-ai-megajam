// Triad Drift - crew AI (ENG-23). Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TriadDrift/Ground/Crew/TDCrewTypes.h"
#include "TDCrewAIController.generated.h"

class ATDCrewCharacter;

/**
 * Idle/Patrol -> Alert -> Engage -> Stagger -> Dead. It senses the nearest hostile through the combat
 * registry, with line of sight, then steers with AddMovementInput (no navmesh needed: planet maps are
 * built at runtime). Each type keeps its preferred distance band, and attacks through ATDCrewCharacter:
 * - grunts flank and need an attack token (2 at once)
 * - troopers and drones keep their distance and strafe
 * - heavies close in and choose bash or shotgun
 * - the Warden cycles charge slam, bolt fan and summons
 */
UCLASS()
class UNREALGAMEJAM_API ATDCrewAIController : public AAIController
{
	GENERATED_BODY()

public:
	ATDCrewAIController();

	/** Seconds between target / line-of-sight refreshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew AI", meta = (Units = "s"))
	float SenseInterval = 0.2f;

	/** Patrol wander radius around the spawn point (0 = stand guard). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew AI", meta = (Units = "cm"))
	float PatrolRadius = 300.f;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Crew") ETDCrewState GetState() const { return State; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Crew") AActor* GetTarget() const { return Target.Get(); }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Crew") int32 GetCoverIndex() const { return CoverIndex; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Crew") bool IsInCover() const { return CoverPhase == ECoverPhase::Hidden || CoverPhase == ECoverPhase::Peeking; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Crew") bool IsPeeking() const { return CoverPhase == ECoverPhase::Peeking; }
	/** Times this crew member gave up a cover spot (flanked, approached, cover destroyed). */
	int32 GetCoverRelocations() const { return CoverRelocations; }

	/** Force a target (encounters that start "already alerted"). */
	void SetTarget(AActor* NewTarget);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	enum class ECoverPhase : uint8 { None, Moving, Hidden, Peeking };

	/** Trooper cover behaviour; returns true if it handled movement and attacks this frame. */
	bool TickCover(ATDCrewCharacter* Crew, float DeltaSeconds, float Distance);
	void LeaveCover(bool bRelocation);
	void Sense();
	bool HasLineOfSight(const AActor* To) const;
	void TickEngage(ATDCrewCharacter* Crew, float DeltaSeconds);
	void TickPatrol(ATDCrewCharacter* Crew, float DeltaSeconds);
	void Steer(ATDCrewCharacter* Crew, const FVector& WorldDir, float Scale);
	void ChooseAttack(ATDCrewCharacter* Crew, float Distance);
	void HandleAttackFinished(ATDCrewCharacter* Crew, ETDCrewAttack Attack);

	ETDCrewState State = ETDCrewState::Idle;
	TWeakObjectPtr<AActor> Target;
	float SenseTimer = 0.f;
	float AlertLeft = 0.f;
	float StrafeTimer = 0.f;
	float StrafeSign = 1.f;
	float FlankSign = 1.f;
	float PatrolTimer = 0.f;
	FVector Home = FVector::ZeroVector;
	FVector PatrolPoint = FVector::ZeroVector;
	bool bHasLOS = false;
	int32 BossCycle = 0;
	FDelegateHandle FinishedHandle;
	ECoverPhase CoverPhase = ECoverPhase::None;
	int32 CoverIndex = INDEX_NONE;
	float CoverTimer = 0.f;
	float CoverSearchTimer = 0.f;
	float MoveBestDist = 0.f;
	float MoveStallTime = 0.f;
	float MoveTotalTime = 0.f;
	float PeekTime = 0.f;
	int32 BlockedCoverIndex = INDEX_NONE;
	float BlockedCoverTime = 0.f;
	FVector PeekLocation = FVector::ZeroVector;
	int32 CoverRelocations = 0;
};

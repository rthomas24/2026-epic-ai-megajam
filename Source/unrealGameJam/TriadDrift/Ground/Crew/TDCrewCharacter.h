// Triad Drift - on-foot Armada crew (ENG-23). Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Subsystems/WorldSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Ground/TDGroundTypes.h"
#include "TriadDrift/Ground/Crew/TDCrewTypes.h"
#include "TDCrewCharacter.generated.h"

class UStaticMeshComponent;
class UTDHealthComponent;
class UTDEnemyVisuals;

DECLARE_MULTICAST_DELEGATE_TwoParams(FTDOnCrewAttack, class ATDCrewCharacter* /*Crew*/, ETDCrewAttack /*Attack*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FTDOnCrewEvent, class ATDCrewCharacter* /*Crew*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FTDOnCrewCue, class ATDCrewCharacter* /*Crew*/, FName /*CueName*/);

/**
 * One crew member. The AI controller (ATDCrewAIController) decides when to attack; the character owns
 * how: a telegraphed wind-up (IsTelegraphing, Visuals->Telegraph), then the hit resolves through the real
 * damage and projectile paths. Bolts and shotgun pellets are T2's ATDProjectile, so the blade can deflect
 * them. Heavies carry a frontal shield that blocks blade slashes until a heavy lunge breaks it. The Warden
 * has two phases (phase 2 at 50% health). Parries stagger anyone (ITDStaggerable).
 */
UCLASS()
class UNREALGAMEJAM_API ATDCrewCharacter : public ACharacter, public ITDTeam, public ITDStaggerable
{
	GENERATED_BODY()

public:
	ATDCrewCharacter(const FObjectInitializer& ObjectInitializer);

	/** Type used on BeginPlay (or call InitFromType). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew")
	ETDCrewType CrewType = ETDCrewType::Grunt;

	/** Live numbers (filled from FTDCrewLibrary; edit per instance for tuning). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew")
	FTDCrewSpec Spec;

	/** Nick's look. If empty, /Game/TriadDrift/Presentation/DA_Crew_<Id>_Visuals is loaded when it exists. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew")
	TObjectPtr<UTDEnemyVisuals> Visuals;

	/** Seconds the body stays after death (placeholder ragdoll). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crew", meta = (Units = "s"))
	float CorpseSeconds = 4.f;

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Crew")
	void InitFromType(ETDCrewType Type);

	/** Begin an attack's wind-up against Target. False if busy, staggered, dead or the type can't do it. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Crew")
	bool StartAttack(ETDCrewAttack Attack, AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Crew")
	void CancelAttack();

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Crew") ETDCrewAttack GetCurrentAttack() const { return CurrentAttack; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Crew") bool IsTelegraphing() const { return CurrentAttack != ETDCrewAttack::None && WindupLeft > 0.f; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Crew") bool IsAttacking() const { return CurrentAttack != ETDCrewAttack::None; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Crew") bool IsStaggered() const { return StaggerLeft > 0.f; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Crew") bool IsGuardBroken() const { return GuardBrokenLeft > 0.f; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Crew") bool IsDead() const;
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Crew") int32 GetBossPhase() const { return BossPhase; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Crew") bool IsCharging() const { return ChargeTimeLeft > 0.f; }
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Crew") float GetCooldownLeft(ETDCrewAttack Attack) const;
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Crew") bool IsPlaceholderVisible() const;
	/** The placeholder wind-up tell (muzzle glow + aim line) is up: only while telegraphing and only when the visuals
	 *  supply no Telegraph cue of their own. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Crew") bool IsTellShown() const;
	UStaticMeshComponent* GetTellGlow() const { return TellGlow; }
	UStaticMeshComponent* GetTellLine() const { return TellLine; }

	/** Summoned grunts still alive (Warden). */
	int32 GetAliveSummons() const;

	/** Director's mercy (ATDEncounterVolume::MercyTiers): health scaled now, summons capped for this life (-1 = no cap). */
	void ApplyMercy(float HealthScale, int32 MaxSummons);
	/** No attack may start for this long (an encounter's opening: the crew show themselves before they hurt). */
	void SetOpeningGrace(float Seconds) { OpeningGraceLeft = FMath::Max(0.f, Seconds); }
	float GetOpeningGraceLeft() const { return OpeningGraceLeft; }
	/** Summons this member may still cast (-1 = no cap). */
	int32 GetSummonsLeft() const { return SummonsLeft; }

	/** Is WorldLocation inside the frontal shield arc? */
	bool IsInShieldArc(const FVector& WorldLocation) const;

	UTDHealthComponent* GetHealth() const { return Health; }
	UStaticMeshComponent* GetBody() const { return Body; }

	/** Applies Visuals (or the placeholder). Public so tests can assign a transient asset. */
	void ApplyVisuals();

	/** Where DA_Crew_<Id>_Visuals is found: /Game/TriadDrift/Presentation (Nick's, always wins), else anywhere under
	 *  /Game/TriadDrift/Gameplay/Characters (engineering-made crew looks). Null when neither has it. */
	static class UTDEnemyVisuals* FindCrewVisuals(FName VisualsId);

	/** The shared look lookup (crew and pilot): Presentation/<Name> first, else <Name> of Class anywhere under
	 *  /Game/TriadDrift/Gameplay/Characters. Null when neither exists. */
	static UObject* FindLookAsset(const FString& Name, UClass* Class);

	/** Where bolts and the tell come from: the mesh's "Muzzle" socket when the skeletal look has one, else in front of the capsule. */
	FVector GetMuzzleLocation(const FVector& AimDirection) const;

	/** The anim blueprint in use on the skeletal look came from the template fallback (the visuals had none). */
	bool UsesFallbackAnimation() const { return bFallbackAnim; }

	/** Template mannequin loops for a skeletal look with no AnimBP, played by the C++ loop player (the template anim
	 *  blueprints collapse the pose on our crew: they expect the template's own character classes). 0 idle, 1 walk, 2 run. */
	static const TCHAR* FallbackLoopFor(ETDCrewType Type, int32 Loop);
	/** The crew's own copy of a template sequence (Gameplay/Characters/Crew/Anims/<same name>), or empty when it doesn't
	 *  exist. The template sequences themselves are never played on crew meshes: compressed, they collapse every bone
	 *  to the root (Nov 13 probes), so without the crew set a look stands in its reference pose. */
	static FString CrewAnimPath(const TCHAR* TemplatePath);
	static FString CrewAnimPath(ETDCrewType Type, int32 Loop) { return CrewAnimPath(FallbackLoopFor(Type, Loop)); }

	/** Scale of the skeletal look (a type borrowing another type's look is sized to its own capsule). */
	float GetLookScale() const { return LookScale; }

	FTDOnCrewAttack OnAttackStarted;   // wind-up begins (telegraph)
	FTDOnCrewAttack OnAttackReleased;  // the hit / shots go out
	FTDOnCrewAttack OnAttackFinished;  // recovered; tokens release here
	FTDOnCrewEvent OnBossPhase2;
	FTDOnCrewEvent OnCrewDied;
	/** Every presentation cue request (Telegraph, Stagger, Death, ShieldHit, HullHit, SlamImpact), even if the slot is empty. */
	FTDOnCrewCue OnCueRequested;

	// ITDTeam
	virtual ETDTeam GetTeam() const override { return ETDTeam::Enemy; }
	// ITDStaggerable
	virtual void ApplyStagger(float Seconds, AActor* Source) override;
	virtual void BreakGuard(AActor* Source) override;

	virtual void Tick(float DeltaSeconds) override;
	virtual void FellOutOfWorld(const UDamageType& DmgType) override;

	/** The encounter that owns this member (summons are adopted into it). */
	TWeakObjectPtr<class ATDEncounterVolume> OwningEncounter;

	/** Is there floor ahead in Direction within Reach (no walking or charging off ledges)? */
	bool HasGroundAhead(const FVector& Direction, float Reach) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "Crew")
	TObjectPtr<UStaticMeshComponent> Body;

	/** Placeholder glowing white eye slit (Armada look: black-red armour, white eyes). Hidden with art. */
	UPROPERTY(VisibleAnywhere, Category = "Crew")
	TObjectPtr<UStaticMeshComponent> Eyes;

	UPROPERTY(VisibleAnywhere, Category = "Crew")
	TObjectPtr<UTDHealthComponent> Health;

	/** Placeholder tell: an enemy red-orange glow at the muzzle growing over the wind-up (all attacks). */
	UPROPERTY(VisibleAnywhere, Category = "Crew")
	TObjectPtr<UStaticMeshComponent> TellGlow;

	/** Placeholder tell: a thin aim line from the muzzle to the target during a ranged wind-up. */
	UPROPERTY(VisibleAnywhere, Category = "Crew")
	TObjectPtr<UStaticMeshComponent> TellLine;

private:
	void ReleaseAttack();
	void FinishAttack();
	void DoMelee();
	void FireBolt(const FVector& Direction, float Damage, float Speed, float Radius);
	FVector AimAt(const AActor* At) const;
	void DoShotgun();
	void DoBoltFan();
	void DoSummon();
	void TickCharge(float DeltaSeconds);
	void EndCharge();
	float AttackCooldown(ETDCrewAttack Attack) const;
	float FilterDamage(const FTDDamageInfo& Info);
	void LoadDefaultVisuals();
	void PlayCrewCue(FName Name, const struct FTDFXCue& Cue, bool bAttached, const FVector& Location);
	void UpdateTell();
	bool bTellTinted = false;

	UFUNCTION()
	void HandleDamaged(UTDHealthComponent* InHealth, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage);

	UFUNCTION()
	void HandleDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow);

	ETDCrewAttack CurrentAttack = ETDCrewAttack::None;
	TWeakObjectPtr<AActor> AttackTarget;
	float WindupLeft = 0.f;
	float RecoverLeft = 0.f;
	int32 ShotsLeft = 0;
	float ShotTimer = 0.f;
	float StaggerLeft = 0.f;
	float GuardBrokenLeft = 0.f;
	float ChargeTimeLeft = 0.f;
	FVector ChargeVelocity = FVector::ZeroVector;
	int32 BossPhase = 1;
	int32 SummonsLeft = -1;
	bool bFallbackAnim = false;
	void UpdateFallbackLoop();
	/** A skeletal look with no death animation topples over about its feet (no physics: PA_Mannequin on our crew
	 *  meshes exploded the ragdoll 15 m in 0.4 s, Nov 13 live). */
	void TickTopple(float DeltaSeconds);
	bool bToppling = false;
	float ToppleAlpha = 0.f;
	FVector ToppleAxis = FVector::RightVector;
	FQuat ToppleStart = FQuat::Identity;
public:
	bool IsToppling() const { return bToppling; }
private:
	/** A look whose bones don't map onto the mannequin animations collapses to the floor under them (playtest
	 *  17e0437, SK_NullHeavy): detect it and show the look in its standing reference pose instead. */
	void GuardAgainstCollapse(float DeltaSeconds);
	float RefHeadHeight = 0.f;
	float CollapseCheckTimer = 0.f;
	int32 CollapseStrikes = 0;
	int32 CollapseRetries = 0;
	float FrozenRetryLeft = 0.f;
public:
	/** The skeletal look fell back to its reference pose because the animations collapsed it. */
	bool IsLookPoseFrozen() const { return bLookPoseFrozen; }
	/** Tests and probes can switch the collapse guard off. */
	static bool bCollapseGuard;
private:
	bool bLookPoseFrozen = false;
	float LookScale = 1.f;
	void PlayFallbackAnim(const TCHAR* AssetPath, float Rate = 1.f);
	const TCHAR* FallbackDeathAnim(const FVector& HitDirection) const;
	float OpeningGraceLeft = 0.f;
	bool bSpecApplied = false;
	TMap<ETDCrewAttack, float> Cooldowns;
	TArray<TWeakObjectPtr<ATDCrewCharacter>> Summons;
};

/** Melee attack tokens: at most MaxMeleeAttackers crew swing at the player at once (features.md ENG-23). */
UCLASS()
class UNREALGAMEJAM_API UTDCrewTokenSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDCrewTokenSubsystem* Get(const UObject* WorldContext);

	int32 MaxMeleeAttackers = 2;

	bool TryAcquire(AActor* Attacker);
	void Release(AActor* Attacker);
	bool Holds(const AActor* Attacker) const;
	int32 GetActiveCount() const;

private:
	TArray<TWeakObjectPtr<AActor>> Holders;
};

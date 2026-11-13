// Triad Drift - on-foot shared types (ENG-21, ENG-22). Owner: Teammate 5.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDGroundTypes.generated.h"

/** What the blade is doing (read by Nick's AnimBP and the HUD). Append only. */
UENUM(BlueprintType)
enum class ETDBladeState : uint8
{
	Idle,
	Light1,
	Light2,
	Light3,
	HeavyLunge,
	AirSlam,
	DashStrike,
	Blocking,
	GuardBroken
};

/**
 * Director's mercy (C-03 #3, scenes and takes): one easing step for an on-foot fight the player keeps losing.
 * An encounter counts its own takes (each time it starts; a retake resets and restarts it). From FromTake on,
 * the highest matching tier applies to every member it spawns. Invisible to anyone who wins first time.
 */
USTRUCT(BlueprintType)
struct FTDRetakeMercyTier
{
	GENERATED_BODY()

	/** The fight's take number (1 = first attempt) this tier starts at. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mercy", meta = (ClampMin = "2"))
	int32 FromTake = 3;

	/** Members spawn with this fraction of their health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mercy", meta = (ClampMin = "0.1", ClampMax = "1"))
	float HealthScale = 1.f;

	/** How many times a boss may summon in this take (-1 = no limit, 0 = never). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mercy")
	int32 MaxSummons = -1;
};

/** First-person movement numbers (features.md ENG-21). */
USTRUCT(BlueprintType)
struct FTDGroundMoveTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move", meta = (Units = "cm/s"))
	float RunSpeed = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Move", meta = (Units = "cm/s"))
	float SprintSpeed = 950.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump", meta = (Units = "cm/s"))
	float JumpZ = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump", meta = (Units = "cm/s"))
	float DoubleJumpZ = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash", meta = (Units = "cm"))
	float DashDistance = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash", meta = (Units = "s"))
	float DashDuration = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash", meta = (Units = "s"))
	float DashIFrames = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash", meta = (ClampMin = "1"))
	int32 DashCharges = 2;

	/** Seconds to regain one dash charge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash", meta = (Units = "s"))
	float DashRechargeSeconds = 1.2f;

	/** Speed kept when a dash ends (so it flows into running instead of stopping dead). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash", meta = (Units = "cm/s"))
	float DashExitSpeed = 950.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slide", meta = (Units = "s"))
	float SlideDuration = 0.7f;

	/** Slide starts at this speed and eases down to RunSpeed over SlideDuration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slide", meta = (Units = "cm/s"))
	float SlideStartSpeed = 1250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mantle", meta = (Units = "cm"))
	float MantleMaxHeight = 180.f;

	/** Ledges lower than this are stepped over by the movement component, not mantled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mantle", meta = (Units = "cm"))
	float MantleMinHeight = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mantle", meta = (Units = "s"))
	float MantleDuration = 0.35f;

	/** How far ahead of the capsule to look for a ledge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mantle", meta = (Units = "cm"))
	float MantleReach = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (Units = "s"))
	float HealthRegenDelay = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float HealthRegenPerSecond = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact", meta = (Units = "cm"))
	float InteractRange = 260.f;

	/** How long the objective ping marker stays up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact", meta = (Units = "s"))
	float PingSeconds = 4.f;
};

/** One blade swing (features.md ENG-22 move table). */
USTRUCT(BlueprintType)
struct FTDBladeSwing
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing")
	float Damage = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing", meta = (Units = "cm"))
	float Range = 180.f;

	/** Full arc width in front of the camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing", meta = (Units = "deg"))
	float ArcDegrees = 100.f;

	/** Time the swing locks out the next attack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing", meta = (Units = "s"))
	float Duration = 0.28f;

	/** Velocity added to hit targets (the third combo hit knocks back). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing", meta = (Units = "cm/s"))
	float Knockback = 0.f;
};

/** Anything a melee parry or a heavy can stagger (crew AI, ENG-23). */
UINTERFACE(MinimalAPI, BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class UTDStaggerable : public UInterface
{
	GENERATED_BODY()
};

class UNREALGAMEJAM_API ITDStaggerable
{
	GENERATED_BODY()

public:
	/** Interrupt the current attack and stun for Seconds. */
	virtual void ApplyStagger(float Seconds, AActor* Source) PURE_VIRTUAL(ITDStaggerable::ApplyStagger, );

	/** Heavy enemies with a frontal guard: a heavy lunge breaks it. Default: no guard. */
	virtual void BreakGuard(AActor* Source) {}
};

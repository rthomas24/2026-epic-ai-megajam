// Triad Drift - auto-landing and take-off (ENG-20). Owner: Teammate 1.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TDLandingComponent.generated.h"

class ATDShip;
class ATDLandingPad;
class UTDTravelFX;
class UTDHealthComponent;
struct FTDDamageInfo;

/** Each step of the landing and take-off sequences (Nick drives gear / canopy / thruster animation from these). */
UENUM(BlueprintType)
enum class ETDLandingState : uint8
{
	Flying,
	Aligning,      // fly to the hover point above the pad and turn to the pad's facing
	GearDown,
	Descending,
	Touchdown,
	EnginesOff,
	CanopyOpening,
	Landed,        // canopy open: OnShipLanded fired, first person takes over
	CanopyClosing, // take-off starts here
	EnginesOn,
	Ascending,
	GearUp
};

/**
 * Defaults: features.md ENG-20, as changed by the Lead on Nov 12. The prompt comes within 120 m of the pad
 * (ATDLandingPad::PromptRange). Landing is allowed at or below 60% of the plain top speed (the atmosphere cap on
 * a planet, cruise in space). Within SlowDownPromptRange but too fast, the prompt says why.
 */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDLandingTuning
{
	GENERATED_BODY()

	/** Landing is offered at or below this fraction of the ship's plain top speed (atmosphere cap / space cruise). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing", meta = (ClampMin = "0", ClampMax = "1"))
	float MaxSpeedFraction = 0.6f;

	/** Within this distance of a pad, holding F starts the approach assist; if it cannot, the HUD says slow down (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing", meta = (ClampMin = "0"))
	float SlowDownPromptRange = 40000.f;

	// ---- Approach assist (hold F near a pad: Lead decision, Nov 12) -----------------------------------
	/** Top approach speed (cm/s). Above the atmosphere cap, so 400 m takes seconds, not a crawl. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing|Assist", meta = (ClampMin = "0"))
	float AssistSpeed = 6000.f;

	/** Braking toward the hover point (cm/s^2): speed follows sqrt(End^2 + 2 * Brake * distance-to-go). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing|Assist", meta = (ClampMin = "1"))
	float AssistBrake = 3000.f;

	/** Speed when the assist hands over to the landing sequence (cm/s). Never below this while assisting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing|Assist", meta = (ClampMin = "0"))
	float AssistEndSpeed = 1500.f;

	/** Distance to the hover point at which the landing sequence takes over (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing|Assist", meta = (ClampMin = "0"))
	float AssistHandOffDistance = 2500.f;

	/** How fast the assist swings the nose onto the pad (deg/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing|Assist", meta = (ClampMin = "0"))
	float AssistTurnRate = 150.f;

	/** Seconds the land button must be held (the controller applies it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing", meta = (ClampMin = "0"))
	float HoldToLandSeconds = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing", meta = (ClampMin = "0"))
	float HoverHeight = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing", meta = (ClampMin = "0.01"))
	float AlignSeconds = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing", meta = (ClampMin = "0.01"))
	float GearSeconds = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing", meta = (ClampMin = "0.01"))
	float DescendSeconds = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing", meta = (ClampMin = "0.01"))
	float TouchdownSeconds = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing", meta = (ClampMin = "0.01"))
	float EnginesSeconds = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing", meta = (ClampMin = "0.01"))
	float CanopySeconds = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing", meta = (ClampMin = "0.01"))
	float AscendSeconds = 1.6f;

	/** Climb above the hover height during take-off (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing", meta = (ClampMin = "0"))
	float TakeOffClimb = 1500.f;

	/** During GearUp (end of take-off) the ship eases this far forward and up before flight resumes (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing", meta = (ClampMin = "0"))
	float GearUpForward = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing", meta = (ClampMin = "0"))
	float GearUpClimb = 300.f;

	/** Forward speed handed back to the flight model after take-off, as a fraction of cruise. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing", meta = (ClampMin = "0"))
	float TakeOffSpeedFraction = 0.4f;

	/** Pilot exit point: this far to the ship's right of its centre (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing")
	float ExitSideOffset = 320.f;

	/** Height of the exit point above the pad deck (cm): a character capsule's half height. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing", meta = (ClampMin = "0"))
	float ExitHeightAboveDeck = 96.f;

	/** Squadmates within this distance land on side pads with the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landing", meta = (ClampMin = "0"))
	float WingmanLandRadius = 60000.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTDOnLandingStateChanged, UTDLandingComponent*, Landing, ETDLandingState, NewState);

/**
 * Auto-landing and take-off for a squad ship. While busy or landed the ship is moved by the sequence
 * (its flight model is paused, so controller / AI input is ignored); flying resumes at the end of
 * take-off. Fires UTDTravelSubsystem::OnShipLanded / OnShipTookOff for T5's first-person hand-off.
 */
UCLASS(ClassGroup = (TriadDrift), meta = (BlueprintSpawnableComponent))
class UNREALGAMEJAM_API UTDLandingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTDLandingComponent();

	/** The pad the player could land on right now (in range, slow enough, flying), or null. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	ATDLandingPad* GetAvailablePad() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	bool CanLandNow() const { return GetAvailablePad() != nullptr; }

	/**
	 * What the HUD shows for landing (feeds ITDHUDData::GetInteractPrompt in flight): the pad's "Hold F to land"
	 * when landing or the approach assist is possible, "SLOW DOWN TO LAND - HOLD SPACE / LT TO BRAKE" near a pad
	 * while too fast and the assist cannot start (boosting), else empty.
	 */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	FText GetLandPrompt() const;

	/**
	 * Approach assist (hold F within SlowDownPromptRange of the player pad): flies the ship to the pad's hover point,
	 * nose on the pad, braking late and never below AssistEndSpeed, then starts the landing sequence. The pilot's
	 * steering is ignored while it runs. Once engaged it carries on after F is released; pressing F again, boosting,
	 * a switch, the hyperdrive, death or a retake stop it.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Landing")
	bool StartApproachAssist();

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Landing")
	void StopApproachAssist();

	/** The pad the assist would fly to right now, or null (flying, player-flown, not boosting, hyperdrive idle, in range). */
	ATDLandingPad* GetAssistPad(float RangeScale = 1.f) const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	bool IsApproachAssisting() const { return AssistPad.IsValid(); }
	/** Landing speed limit right now (cm/s). */
	float GetLandingSpeedLimit() const;

	/** Player: land on the available pad; nearby squadmates land on its side pads. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Landing")
	bool RequestLand();

	/** Land on a specific pad (wingmen, scripted arrivals). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Landing")
	bool LandOnPad(ATDLandingPad* Pad);

	/** Reverse the sequence; the player's take-off lifts the landed squadmates too. T5 calls this on re-board. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Landing")
	bool RequestTakeOff();

	/**
	 * Hard reset for a retake: instantly Flying (gear up, canopy closed, engines on), pad released, flight
	 * model un-parked, wingman AI un-passived. No events other than the state change. Safe when already flying.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Landing")
	void AbortToFlying();

	/** Puts the ship straight into the Landed state on a pad (loading a checkpoint on a planet). */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Landing")
	bool SnapLanded(ATDLandingPad* Pad);

	// ---- State for presentation and gameplay ------------------------------------------------------
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	ETDLandingState GetLandingState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	bool IsGearDown() const { return bGearDown; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	bool IsCanopyOpen() const { return bCanopyOpen; }

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	bool AreEnginesOn() const { return bEnginesOn; }

	/** 0 = flying, 1 = landed with the canopy open; runs back down during take-off. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	float GetLanding01() const;

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	bool IsLanded() const { return State == ETDLandingState::Landed; }

	/** In any landing / take-off step or landed (the flight model is paused). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	bool IsBusy() const { return State != ETDLandingState::Flying; }

	/** In a landing step (Aligning .. CanopyOpening): coming down, not yet landed, not taking off. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	bool IsLandingInProgress() const
	{
		return State == ETDLandingState::Aligning || State == ETDLandingState::GearDown || State == ETDLandingState::Descending
			|| State == ETDLandingState::Touchdown || State == ETDLandingState::EnginesOff || State == ETDLandingState::CanopyOpening;
	}

	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	ATDLandingPad* GetPad() const { return Pad.Get(); }

	/**
	 * Where the pilot steps out on foot (T5's first-person spawn) and where re-boarding should be offered:
	 * beside the ship on its right, at capsule height above the deck, facing the ship's forward.
	 * Valid whenever the ship is on or above a pad; otherwise beside the ship at its current height.
	 */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Landing")
	FTransform GetExitTransform() const;

	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Landing")
	FTDOnLandingStateChanged OnLandingStateChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Landing")
	FTDLandingTuning Tuning;

	/** Landing dust / take-off cues (Nick). Empty slots do nothing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Landing")
	TObjectPtr<UTDTravelFX> TravelFX;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleOwnerDeath(UTDHealthComponent* HealthComp, const FTDDamageInfo& KillingBlow);

private:
	ATDShip* GetShip() const;
	void SetState(ETDLandingState NewState);
	float StateDuration(ETDLandingState S) const;
	void PauseFlight(bool bPause);
	void SetParkedBlock(bool bOn);
	UPROPERTY(Transient) TObjectPtr<class UBoxComponent> ParkedBlock;
	FVector TouchPoint() const;
public:
	/**
	 * Ryan's bug (Nov 13): on foot the pilot walked through parked ships (the 2.2 m flight sphere is far smaller than the
	 * hull). While parked a box fitted to the visible hull (shrunk by ParkedBlockShrink) blocks the Pawn channel only;
	 * it goes at take-off. True while it is on.
	 */
	bool IsParkedBlockOn() const;
	static constexpr float ParkedBlockShrink = 0.85f;
	/** The visible meshes' bounds in the ship's own frame (empty when it has none). */
	static FBox VisibleHullBoxLocal(const ATDShip* Ship);
	/** How far the visible hull reaches below the ship's origin (a "Skid" socket, else the meshes' bounds; else the
	 *  collision sphere's radius). Parked ships rest on this, ParkClearance over the pad. */
	static float HullBottomBelowOrigin(const ATDShip* Ship);
	static constexpr float ParkClearance = 4.f;
private:
	FVector HoverPoint() const;
	FQuat PadFacing() const;
	void LandWingmen(ATDLandingPad* PlayerPad);
	void TakeOffWingmen();
	void FinishTakeOff();
	void TickApproachAssist(float DeltaTime);
	/** HUD objective marker: the player pad while the player flies in this atmosphere and hasn't landed yet. */
	void UpdatePadMarker();
	/** Begins the landing sequence on a pad (shared by RequestLand, LandOnPad and the assist). */
	bool BeginLanding(ATDLandingPad* InPad);

	ETDLandingState State = ETDLandingState::Flying;
	bool bLandReadySaid = false; // ENG-43: the pad-in-range bark, once per approach
	float StateTime = 0.f;
	TWeakObjectPtr<ATDLandingPad> Pad;
	FVector SegmentStart = FVector::ZeroVector;
	FQuat SegmentStartRot = FQuat::Identity;
	bool bGearDown = false;
	bool bCanopyOpen = false;
	bool bEnginesOn = true;
	TWeakObjectPtr<ATDLandingPad> AssistPad;
	/** Landed since entering this atmosphere: the pad is no longer the objective (cleared on leaving the atmosphere). */
	bool bLandedThisVisit = false;
	/** Aligning time for this landing: longer when the sequence starts far from the hover point. */
	float AlignSecondsNow = 1.5f;
	/** Peak height of the arc added to the aligning path to clear a prop between the ship and the pad (cm). */
	float AlignArc = 0.f;
	/** Sweeps the straight aligning path; if static geometry blocks it, sets AlignArc to clear its top. */
	void PlanAlignArc();
};

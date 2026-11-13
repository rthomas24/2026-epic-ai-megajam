// Triad Drift - engineering space HUD (ENG-36, Teammate 2). Spec: docs/eng/hud-redesign.md. See docs/eng/ui-host.md.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDFlightHUDWidget.generated.h"

/** ENG-36 colour tokens (spec section 5). Mirrored into UTDUIStyle once that has slots for them, so Nicholas can restyle. */
struct FTDFlightHUDTokens
{
	static FLinearColor Neutral() { return FLinearColor::FromSRGBColor(FColor(0xE8, 0xEE, 0xF7)); }
	static FLinearColor Dim() { return FLinearColor::FromSRGBColor(FColor(0x8A, 0x94, 0xA6)); }
	static FLinearColor Rift() { return FLinearColor::FromSRGBColor(FColor(0xFF, 0x4F, 0xD8)); }
	static FLinearColor Lancer() { return FLinearColor::FromSRGBColor(FColor(0x2F, 0xE6, 0xFF)); }
	static FLinearColor Bastion() { return FLinearColor::FromSRGBColor(FColor(0xFF, 0xB1, 0x3B)); }
	static FLinearColor Shield() { return FLinearColor::FromSRGBColor(FColor(0x9F, 0xD8, 0xFF)); }
	static FLinearColor Warning() { return FLinearColor::FromSRGBColor(FColor(0xFF, 0xE1, 0x4A)); }
	static FLinearColor Danger() { return FLinearColor::FromSRGBColor(FColor(0xFF, 0x4A, 0x3D)); }
	static FLinearColor Plate() { return FLinearColor::FromSRGBColor(FColor(0x0A, 0x0F, 0x1C)); }
	static FLinearColor Ship(ETDShipId Ship)
	{
		return Ship == ETDShipId::Rift ? Rift() : Ship == ETDShipId::Lancer ? Lancer() : Bastion();
	}
	/** Hull colour band: white above 50%, amber to 25%, red below. */
	static FLinearColor Hull(float Hull01)
	{
		return Hull01 > .5f ? Neutral() : Hull01 > .25f ? Bastion() : Danger();
	}
};

/**
 * Engineering's own space HUD (ENG-36): the essentials at the reticle, a compact squad strip, one objective line; the
 * rest contextual. Built in phases (spec section 7). P1, this file: reticle with hull and shield arcs, the signature
 * power pip, the squad strip and the objective line.
 *
 * Painted in one NativePaint in a 1920x1080 design space letterboxed into the viewport (so on ultrawide everything sits
 * in a centred 16:9 frame), like UTDGroundHUDWidget. Every frame it samples its data source (the squad manager, through
 * ITDHUDData) into FView, which is what it draws and what the tests read.
 *
 * Shown by UTDUIHostSubsystem when UTDUIHostSettings picks the engineering HUD (TD.UI.SpaceHUD). Nicholas's widget and
 * the dev HUD stay selectable. It exposes SetDataSource / RefreshData under the same names as his widget, so the host
 * drives both the same way.
 */
UCLASS()
class UNREALGAMEJAM_API UTDFlightHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** One squad chip as drawn. */
	struct FChip
	{
		ETDShipId Ship = ETDShipId::Rift;
		float Hull = 1.f;
		float Shield = 1.f;
		float RespawnSeconds = 0.f;
		bool bActive = false;
		bool bAlive = true;
		/** Alive with hull under TroubleHull: amber pulsing outline and a "!" badge. */
		bool bTrouble = false;
		/** Its signature power is ready (the dot on the chip, so a wingman's ready power is visible before switching). */
		bool bPowerReady = false;
		/** Switch key / button shown on the keycap ("1", "2", "3"; empty on a pad). */
		FString Key;
		/** Drawn position and size in the design space (tests check the layout). */
		FVector2D Pos = FVector2D::ZeroVector;
		FVector2D Size = FVector2D::ZeroVector;
	};

	/** P2: one hostile or missile as drawn: a bracket / tick on screen, an edge chevron off screen. */
	struct FMark
	{
		bool bOnScreen = false;
		/** Design-space position (on screen) or the chevron's spot on the safe frame (off screen). */
		FVector2D Pos = FVector2D::ZeroVector;
		/** Off screen: the chevron's angle in radians (screen space, 0 = right, +PI/2 = down). */
		float Angle = 0.f;
		bool bCurrent = false;
		bool bLocked = false;
		bool bMissile = false;
		bool bFocus = false;
		/** Current target: bracket size (48..160) and its health 0..1 (-1 = unknown). */
		float Size = 0.f;
		float Health = -1.f;
		/** Metres. */
		float Distance = 0.f;
	};

	/** P2: a wingman's arrow (off screen only, hollow chevron in its colour; pulses in trouble). */
	struct FWingArrow
	{
		ETDShipId Ship = ETDShipId::Rift;
		FVector2D Pos = FVector2D::ZeroVector;
		float Angle = 0.f;
		bool bTrouble = false;
	};

	/** P2: one hit-direction arc around the reticle. */
	struct FHitArc
	{
		float Angle = 0.f;
		float Opacity = 0.f;
		bool bShieldOnly = false;
	};

	/** What the one lower-lane message is (priority order). */
	enum class ELane : uint8 { None, Prompt, HyperdriveBlocked, Hint };

	/** Snapshot of what the widget draws. Updated every tick from the data source. */
	struct FView
	{
		// ---- P2
		TArray<FMark> Marks;
		bool bHasLead = false;
		FVector2D Lead = FVector2D::ZeroVector;
		TArray<FWingArrow> Wings;
		ETDMissileWarning Missile = ETDMissileWarning::None;
		TArray<FHitArc> Hits;
		/** Low-hull edge glow strength 0..1 (hull under 25%). */
		float Vignette = 0.f;
		/** Boost bar: shown while boosting or refilling, hidden 1.5 s after full. */
		bool bShowBoost = false;
		float Boost = 1.f;
		bool bDodgeReady = true;
		/** Hyperdrive charge bar while charging. */
		float Hyperdrive = 0.f;
		ELane Lane = ELane::None;
		FString LaneText;
		FString Boundary;
		float SwitchStrike = 0.f;
		/** Metres, -1 when not in atmosphere. */
		float Altitude = -1.f;
		/** The flown ship's ability (E / Y), second power (Bastion's dome, hold Q) and secondary ammo. */
		bool bHasAbility = false;
		float Ability = 1.f;
		FString AbilityGlyph;
		bool bHasPower2 = false;
		float Power2 = 1.f;
		int32 Ammo = 0;
		float Heat = 0.f;
		// ---- P1
		bool bHasData = false;
		/** The flown ship (the reticle cluster's colour). */
		ETDShipId Flown = ETDShipId::Rift;
		float Hull = 1.f;
		float Shield = 1.f;
		/** Arc opacity: 1 while either value is below 100% or changed in the last ArcAwakeSeconds, else ArcRestOpacity. */
		float ArcOpacity = 1.f;
		/** Signature power: 0 just used, 1 ready. */
		float Power = 1.f;
		bool bPowerReady = true;
		/** 0..1 through the ready pulse (0 when not pulsing). */
		float PowerPulse = 0.f;
		/** The power key / button glyph ("Q", or "X" on a pad). */
		FString PowerGlyph;
		TArray<FChip> Chips;
		/** The objective line's text in caps, and its right-hand counter ("5 LEFT", empty between waves). */
		FString Objective;
		FString Counter;
		/** Second line of the expanded objective ("WAVE 3 OF 5"). */
		FString ObjectiveDetail;
		/** 0 collapsed .. 1 expanded (for ObjectiveExpandSeconds after the text changes, eased over 0.3 s). */
		float ObjectiveExpand = 0.f;
		bool bUsingGamepad = false;
	};
	const FView& GetView() const { return View; }

	/** Same name and signature as Nick's UTDSpaceHUDWidget, so the host's reflection path drives both. */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	void SetDataSource(UObject* InSource);

	UFUNCTION(BlueprintCallable, Category = "TriadDrift|HUD")
	void RefreshData();

	UObject* GetDataSource() const { return Source.Get(); }

	/** Re-reads the data source and advances the timers by DeltaSeconds (the tick calls this every frame). */
	void Sample(float DeltaSeconds);

	/** Tests: force the glyph device (-1 = ask the owning controller). */
	int32 ForceGamepad = -1;

	/** The player pressed the hyperdrive key: if it is blocked, the reason shows in the lower lane for 3 s (spec: only then). */
	UFUNCTION()
	void HandleHyperdriveInput(bool bHeld);

	// Surfaces this widget draws, so the dev HUD stops drawing them (UTDUIHostSubsystem reads these by name). Subtitles
	// and the pause line stay with the dev HUD / pause screen.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	bool bCoversDamageIndicators = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	bool bCoversSquadMarkers = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	bool bCoversInteractPrompt = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	bool bCoversHyperdriveBlockText = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	bool bCoversAltitude = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	bool bCoversBoundaryWarning = true;

	static constexpr float BlockedSeconds = 3.f;
	static constexpr float BoostHideSeconds = 1.5f;
	static constexpr float HitSeconds = 1.2f;
	static constexpr float HitRadius = 150.f;
	static constexpr float BracketMin = 48.f;
	static constexpr float BracketMax = 160.f;

	static constexpr float TroubleHull = .35f;
	static constexpr float ArcAwakeSeconds = 3.f;
	static constexpr float ArcRestOpacity = .55f;
	static constexpr float PulseSeconds = .3f;
	static constexpr float ObjectiveExpandSeconds = 3.f;
	static constexpr float ObjectiveEaseSeconds = .3f;

	// Design-space layout (1920x1080, spec section 3.1; the strip at the Lead's 1.3x).
	static constexpr float CenterX = 960.f;
	static constexpr float CenterY = 540.f;
	static constexpr float SafeX = 96.f;
	static constexpr float SafeY = 54.f;
	static constexpr float HullRadius = 44.f;
	static constexpr float ShieldRadius = 53.f;
	static constexpr float ArcHalfDeg = 40.f;
	static constexpr float PipOffsetX = 64.f;
	static constexpr float PipOffsetY = 56.f;
	static constexpr float PipRadius = 17.f;
	static constexpr float StripScale = 1.3f;
	static constexpr float ChipHeight = 44.f * StripScale;
	static constexpr float ActiveChipWidth = 148.f * StripScale;
	static constexpr float WingChipWidth = 100.f * StripScale;
	static constexpr float ChipGap = 6.f * StripScale;
	static constexpr float ObjectiveWidth = 480.f;
	static constexpr float ObjectiveHeight = 28.f;
	static constexpr float ObjectiveExpandedWidth = 720.f;
	static constexpr float ObjectiveExpandedHeight = 72.f;

protected:
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	bool IsUsingGamepad() const;
	void LayoutChips();
	void SampleCombat(const class ITDHUDData& Data, const FTDShipHUD* Flown, float Dt);
	/** Viewport pixels -> design space (the same letterbox the painter uses). */
	FVector2D ToDesign(const FVector2D& Pixels) const;
	/** A point on the safe frame along Dir from the centre (edge chevrons). */
	static FVector2D OnSafeFrame(const FVector2D& Dir);
	void BindController();

	FVector2D ViewportSize = FVector2D(1920, 1080);
	float BlockedLeft = 0.f;
	float BoostFullFor = 1e6f;
	TWeakObjectPtr<class ATDSpacePlayerController> BoundPC;

	FView View;
	TWeakObjectPtr<UObject> Source;
	float LastHull = -1.f;
	float LastShield = -1.f;
	float ArcAwake = 0.f;
	float PulseLeft = 0.f;
	bool bWasPowerReady = true;
	ETDShipId LastFlown = ETDShipId::Rift;
	FString LastObjective;
	/** Seconds since the objective text last changed (huge = never). */
	float ObjectiveAge = 1e6f;
	float Time = 0.f;
};

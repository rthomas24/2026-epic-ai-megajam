// Triad Drift - on-foot HUD (Teammate 2). See docs/eng/ui-host.md.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TDGroundHUDWidget.generated.h"

class UTDHealthComponent;
struct FTDDamageInfo;

/** One hit on the on-foot player, remembered for its direction chevron. */
struct FTDGroundHit
{
	/** Where the hit came from (the shooter, else back along the hit direction). */
	FVector From = FVector::ZeroVector;
	float Age = 0.f;
};

/**
 * The on-foot HUD in the space HUD's language (Nick's Plugins/TriadDriftHUD, copied through FTDUIPainter): a vitals
 * panel bottom-left (health bar, block stamina bar, dash charges as pips), the objective panel top-centre, the reticle,
 * hostile brackets on nearby crew, the interact prompt on a plate, damage direction chevrons and the perfect-deflect
 * flash. Reads the owning player's pawn through ITDHUDData (ATDFirstPersonCharacter). Shown by UTDUIHostSubsystem
 * on foot only; Nick can restyle it by setting UTDUIStyle::GroundHUDWidgetClass.
 */
UCLASS()
class UNREALGAMEJAM_API UTDGroundHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Snapshot of what the widget draws (tests and diagnostics). Updated every tick from the pawn. */
	struct FView
	{
		float Health = 1.f;
		float Block = 1.f;
		int32 Dash = 0;
		int32 DashMax = 2;
		FString Objective;
		FString Prompt;
		int32 Hits = 0;
		int32 Hostiles = 0;
		float Deflect = 0.f;
		bool bPerfect = false;
		/** Verdance's archive transfer (T5's ATDArchiveTransfer::GetArchiveTransfer): meter shown while it runs. */
		bool bTransfer = false;
		float Transfer01 = 0.f;
		bool bTransferPaused = false;
		/** The Warden (T5's ATDCrewCharacter::GetActiveBoss): bar shown while one is alive. */
		bool bBoss = false;
		float Boss01 = 0.f;
		int32 BossPhase = 1;
		FString BossName;
		/**
		 * A counted step on foot (T5's ATDStepCounter whose step is current: the reactor conduits 0/4, Cinder's conduits,
		 * Verdance's nodes): one pip each under the objective panel, "CONDUITS  n / N" (the noun is the counter's last
		 * word); StepFlash01 1..0 after each one.
		 */
		int32 StepCount = 0;
		int32 StepRequired = 0;
		FString StepNoun;
		float StepFlash01 = 0.f;
		/** The escape run (T5's ATDCollapseRun while it runs; its Seconds is the course's EscapeSeconds): clock and bar. */
		bool bEscape = false;
		float EscapeLeft = 0.f;
		float EscapeTotal = 0.f;
		/** During the escape, the way out (the pawn's first objective marker): on screen, or on the frame edge. */
		bool bExitMarker = false;
		bool bExitOnScreen = false;
		FVector2D ExitPos = FVector2D::ZeroVector;
		float ExitMetres = 0.f;
		/** The sensor marker flicker (UTDUIHostSubsystem::ShowSensorFlicker; storyline.md §18 traverse). */
		bool bSensor = false;
		bool bSensorVerified = false;
		FVector2D SensorPos = FVector2D::ZeroVector;
		bool bSensorOnScreen = false;
	};
	const FView& GetView() const { return View; }
	/** The boss bar's damage ghost (tests). */
	float GetBossGhost01() const { return GhostBoss; }

	/** Tests: >= 0 forces a running transfer at this progress (no ATDArchiveTransfer needed); -1 reads the world. */
	float ForceTransfer01 = -1.f;
	bool bForceTransferPaused = false;
	/** Tests: a non-empty value replaces the pawn's objective line. */
	FString ForceObjective;
	/** Tests: >= 0 forces a Warden at this hull (phase 2 below BossPhase2At); -1 reads the world. */
	float ForceBoss01 = -1.f;
	/** Tests: >= 0 forces a counted step (count, required) / a running escape (left, total) / an exit marker spot. */
	int32 ForceStepRequired = -1;
	int32 ForceStepCount = 0;
	FString ForceStepNoun;
	float ForceEscapeLeft = -1.f;
	float ForceEscapeTotal = 90.f;
	TOptional<FVector> ForceExitAt;
	/** Where the bar marks the phase change (T5: Phase2HealthFraction 0.5). */
	static constexpr float BossPhase2At = .5f;
	/** The objective panel grows to fit its line up to this width (design px), then wraps to two lines. */
	static constexpr float ObjectiveMaxWidth = 1000.f;
	/** The flight-style objective pill: at least this wide, one line / two lines tall. */
	static constexpr float ObjectiveMinWidth = 300.f;
	static constexpr float ObjectiveHeight = 40.f;
	static constexpr float ObjectiveHeight2 = 64.f;

	/** Named boxes painted last frame (tests: "transfer" for the meter plate). */
	mutable TArray<TPair<FName, FBox2D>> LastBoxes;

	/** Re-reads the pawn now (the tick does this every frame). */
	void Sample(float DeltaSeconds);

	/** Seconds a damage chevron stays up. */
	static constexpr float HitSeconds = 1.6f;
	/** Seconds the perfect / normal deflect flash lasts. */
	static constexpr float PerfectSeconds = .55f;
	static constexpr float DeflectSeconds = .3f;
	/** Crew within this range get hostile brackets. */
	static constexpr float BracketRangeCm = 4500.f;

	// Surfaces this widget draws (UTDUIHostSubsystem::IsSurfaceCoveredByNativeHUD reads these by name).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	bool bCoversDamageIndicators = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	bool bCoversInteractPrompt = true;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	UFUNCTION()
	void HandleDamaged(UTDHealthComponent* Health, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage);

private:
	void BindPawn(APawn* Pawn);
	void UnbindPawn();
	void HandleDeflect(AActor* Projectile, bool bPerfect);

	FView View;
	float SmoothHealth = 1.f;
	float SmoothBlock = 1.f;
	float DamageFlash = 0.f;
	float SmoothTransfer = 0.f;
	/** Boss bar: the fill eases to the hull; the ghost holds where it was for a moment, then drains (recent damage). */
	float SmoothBoss = 0.f;
	float GhostBoss = 0.f;
	float GhostHold = 0.f;
	float LastBoss = 0.f;
	float Time = 0.f;
	/** Counted step / escape run: re-found every 0.5 s; the step count last seen (a rise flashes) and the flash clock. */
	TWeakObjectPtr<AActor> StepCounter;
	TWeakObjectPtr<AActor> EscapeRun;
	float PropScanLeft = 0.f;
	int32 StepCountSeen = -1;
	float StepFlashLeft = 0.f;
	/** The exit marker's world spot and direction from the view; the sensor marker's spot. */
	FVector ExitWorld = FVector::ZeroVector;
	FVector2D ExitDirection = FVector2D(0.f, -1.f);
	FVector SensorWorld = FVector::ZeroVector;
	TArray<FTDGroundHit> Hits;
	TArray<TWeakObjectPtr<AActor>> Hostiles;
	TWeakObjectPtr<APawn> BoundPawn;
	TWeakObjectPtr<UTDHealthComponent> BoundHealth;
	FDelegateHandle DeflectHandle;
	TWeakObjectPtr<UObject> BoundBlade;
};

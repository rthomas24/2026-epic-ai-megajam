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
	};
	const FView& GetView() const { return View; }

	/** Tests: >= 0 forces a running transfer at this progress (no ATDArchiveTransfer needed); -1 reads the world. */
	float ForceTransfer01 = -1.f;
	bool bForceTransferPaused = false;

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
	float Time = 0.f;
	TArray<FTDGroundHit> Hits;
	TArray<TWeakObjectPtr<AActor>> Hostiles;
	TWeakObjectPtr<APawn> BoundPawn;
	TWeakObjectPtr<UTDHealthComponent> BoundHealth;
	FDelegateHandle DeflectHandle;
	TWeakObjectPtr<UObject> BoundBlade;
};

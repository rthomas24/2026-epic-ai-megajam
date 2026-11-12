#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TDSpaceHUDWidget.generated.h"
class IInputProcessor;

/** Presentation-only snapshot. Indices are Rift=0, Lancer=1, Bastion=2, matching ETDShipId. */
USTRUCT(BlueprintType)
struct TRIADDRIFTHUD_API FTDUIShip
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float Hull = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float Shield = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float Boost = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float Heat = 0;
    /** Contract convention: 0 just used, 1 ready. Never inverted. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float Power = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float Power2 = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float Ability = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float RespawnSeconds = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") int32 Ammo = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") bool bActive = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") bool bAlive = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") bool bUnlocked = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") bool bAvailable = false;
};

USTRUCT(BlueprintType)
struct TRIADDRIFTHUD_API FTDHUDStyle
{
    GENERATED_BODY()
    // Exact sRGB tokens from the frozen HUD Study 01, converted for Slate's linear tint.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style") FLinearColor Rift = FLinearColor::FromSRGBColor(FColor(241,140,250));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style") FLinearColor Lancer = FLinearColor::FromSRGBColor(FColor(121,229,255));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style") FLinearColor Bastion = FLinearColor::FromSRGBColor(FColor(255,206,119));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style") FLinearColor Ink = FLinearColor::FromSRGBColor(FColor(240,248,255));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style") FLinearColor Muted = FLinearColor::FromSRGBColor(FColor(162,182,201));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style") FLinearColor Panel = FLinearColor::FromSRGBColor(FColor(8,19,31,230));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style") FLinearColor Hostile = FLinearColor::FromSRGBColor(FColor(255,148,124));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style", meta=(ClampMin="0.5",ClampMax="2")) float TextScale = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style|Readability", meta=(ClampMin="12",ClampMax="18")) int32 EssentialLabelSize = 14;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style|Readability", meta=(ClampMin="16",ClampMax="22")) int32 StatusNumberSize = 18;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style|Readability", meta=(ClampMin="6",ClampMax="12")) float HealthBarThickness = 10.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style|Readability", meta=(ClampMin="5",ClampMax="10")) float ShieldBarThickness = 7.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style", meta=(ClampMin="1",ClampMax="30")) float MeterResponse = 9.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style", meta=(ClampMin="0.05",ClampMax="2")) float SwitchSeconds = .65f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style") bool bReducedMotion = false;
    /** Isolated visual-review experiment. Only affects the controlled Lancer; opt-in until approved. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style|Review") bool bLancerVisualBenchmark = false;
    FLinearColor ShipColor(int32 Index) const { return Index == 0 ? Rift : Index == 1 ? Lancer : Bastion; }
};

/** Reusable native UMG card. Can be used independently in any Widget Blueprint. */
UCLASS(BlueprintType, Blueprintable)
class TRIADDRIFTHUD_API UTDShipCardWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD", meta=(ClampMin="0",ClampMax="2")) int32 ShipIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") FTDUIShip State;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") FTDHUDStyle Style;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual int32 NativePaint(const FPaintArgs&, const FGeometry&, const FSlateRect&, FSlateWindowElementList&, int32, const FWidgetStyle&, bool) const override;
};

/** Native UMG HUD. No gameplay writes, no project settings, no automatic installation. */
UCLASS(BlueprintType, Blueprintable)
class TRIADDRIFTHUD_API UTDSpaceHUDWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") FTDHUDStyle Style;
    /** Fallback for continuous contract values. Events/pushes can call RefreshData immediately. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD", meta=(ClampMin="0.05",ClampMax="1")) float SampleSeconds = .1f;
    /** Readiness (1 = ready), not unknown remaining seconds. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") float LowHullThreshold = .25f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD", meta=(ClampMin="1")) float WarningSeconds = 2.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD") bool bGamepadGlyphs = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Inputs") FText KeyboardHint = FText::FromString(TEXT("[1 / 2 / 3]  SWITCH SHIP     [LMB]  FIRE     [Q]  SIGNATURE"));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Inputs") FText GamepadHint = FText::FromString(TEXT("[D-PAD L / R]  SWITCH SHIP     [RB]  FIRE     [X]  SIGNATURE"));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Inputs") FText SignatureKeyboardKey = FText::FromString(TEXT("Q"));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD|Inputs") FText SignatureGamepadKey = FText::FromString(TEXT("X"));
    /** Set by the weapon owner for physical intercept. 0 hides lead; never fabricate ballistics. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HUD", meta=(ClampMin="0")) float ProjectileSpeedCm = 0;

    UFUNCTION(BlueprintCallable, Category="HUD") void SetDataSource(UObject* Source);
    UFUNCTION(BlueprintCallable, Category="HUD") void RefreshData();
    UFUNCTION(BlueprintCallable, Category="HUD") void ApplySquadSnapshot(const TArray<FTDUIShip>& Snapshot);
    UFUNCTION(BlueprintCallable, Category="HUD") void ShowWarning(FText Message);
    UFUNCTION(BlueprintCallable, Category="HUD") void ConfirmHit(bool bKill);
    UFUNCTION(BlueprintCallable, Category="HUD") void SetObjectivePrompt(FText Message);
    /** Only the preview controller calls this; gameplay never enables it automatically. */
    UFUNCTION(BlueprintCallable, Category="HUD|Preview") void SetPreviewMode(bool bEnabled);
    UFUNCTION(BlueprintCallable, Category="HUD|Preview") void PreviewSelectShip(int32 Index);
    UFUNCTION(BlueprintCallable, Category="HUD|Preview") void PreviewDamage();
    UFUNCTION(BlueprintCallable, Category="HUD|Preview") void PreviewDowned();
    UFUNCTION(BlueprintCallable, Category="HUD|Preview") void PreviewPower();
    const TArray<FTDUIShip>& GetSnapshot() const { return Ships; }
    bool HasValidSource() const { return DataSource.IsValid(); }
    bool IsLancerBenchmarkActive() const { return Style.bLancerVisualBenchmark && ActiveIndex==1; }
    /** Preview-only CPU paint sampling. No production telemetry or gameplay writes. */
    void ResetBenchmarkPaintMetrics() const;
    FString GetBenchmarkPaintMetrics() const;
    static bool SolveIntercept(const FVector& RelativePosition, const FVector& TargetVelocity, float Speed, FVector& OutAim);
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry&, float) override;
    virtual int32 NativePaint(const FPaintArgs&, const FGeometry&, const FSlateRect&, FSlateWindowElementList&, int32, const FWidgetStyle&, bool) const override;
private:
    UPROPERTY(Transient) TArray<FTDUIShip> Ships;
    UPROPERTY(Transient) TArray<FTDUIShip> Display;
    TWeakObjectPtr<UObject> DataSource;
    TWeakObjectPtr<UObject> EventBus;
    TWeakObjectPtr<AActor> Target;
    TArray<TWeakObjectPtr<AActor>> Locked;
    TArray<TWeakObjectPtr<AActor>> Objectives;
    FTimerHandle SampleTimer;
    TSharedPtr<IInputProcessor> InputObserver;
    float Elapsed = 0, SwitchLeft = 0, HitLeft = 0, WarningLeft = 0;
    float Speed01 = 0, Hyperdrive01 = 0;
    float TargetHull = 0, TargetShield = 0, ReadyLeft = 0;
    float AcquisitionLeft = 0, ActivationLeft = 0, DamageLeft = 0;
    TWeakObjectPtr<AActor> VisualTarget;
    bool bVisualPreviewTarget = false;
    mutable double PaintTotalMicros = 0, PaintMaxMicros = 0;
    mutable int32 PaintSamples = 0;
    FVector LeadWorld = FVector::ZeroVector;
    bool bHasTargetHealth = false, bHasGameplayLead = false;
    int32 IncomingMissiles = 0;
    FText ReadyPrompt;
    TArray<TWeakObjectPtr<AActor>> Hostiles;
    int32 Wave = 0, Enemies = 0, ActiveIndex = INDEX_NONE, ReadyShip = 0;
    bool bPreview = false, bKillConfirmed = false, bOutOfBounds = false;
    FText Warning, ObjectivePrompt;
    void BindEvents(bool bBind);
    UFUNCTION() void HandleHit(AActor* Actor, bool bKill);
    UFUNCTION() void HandleObjective(FName Id, int32 Step, FText Text);
};

// Triad Drift - Director (Teammate 3). C-03 #2 "Director cam money shots".
//
// Watches for big moments, picks the ones worth a slow-motion beat, rate-limits them, fires
// UTDEventBus::OnHighlightMoment with an importance score, requests slow-mo from UTDFeelSubsystem
// and publishes a camera hint (FTDDirectorShot) for Nick's director cams.
//
// Sources:
//  - automatic, from UTDEventBus::OnActorKilled: ram kills, multi-kills (3+ player kills within
//    1.5 s), Switch Strike kills, and objective kills of actors tagged TDDirectorTags::*;
//  - reported by the owning system with ReportHighlight(): portal missile returns (T2), perfect
//    deflects (T5's blade), ability combos, anything else.
// Never fires while a retake ("CUT!") or level travel is pending.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TDDirectorSubsystem.generated.h"

/** Actor tags that mark objective kills worth a money shot (add them on the victim actor). */
namespace TDDirectorTags
{
	inline const FName Generator(TEXT("TD.Highlight.Generator"));
	inline const FName Reactor(TEXT("TD.Highlight.Reactor"));
	inline const FName Carrier(TEXT("TD.Highlight.Carrier"));
}

/** The current money shot: what to look at, roughly from where, and for how long. */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDDirectorShot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	ETDHighlightKind Kind = ETDHighlightKind::RamKill;

	/** Main subject (the victim for objective kills, the player's ship for ram/multi/switch-strike kills). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	TWeakObjectPtr<AActor> Subject;

	/** Secondary subject for two-shots (the killer, or the victim). May be null. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	TWeakObjectPtr<AActor> Secondary;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	float Importance01 = 0.f;

	/** World point the camera should frame (captured at the moment of the highlight). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	FVector FocusLocation = FVector::ZeroVector;

	/** Suggested cinematic camera position and rotation (side-on, slightly above, alternating sides). */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	FVector SuggestedCameraLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	FRotator SuggestedCameraRotation = FRotator::ZeroRotator;

	/** Slow-motion length (real seconds) and time dilation requested for this shot. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	float DurationSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	float Dilation = 1.f;

	/** World real-time seconds the shot started. */
	UPROPERTY(BlueprintReadOnly, Category = "TriadDrift|Director")
	float StartRealTime = 0.f;
};

/** Every Director's Cut number (features.md C-03 #2 defaults). */
USTRUCT(BlueprintType)
struct UNREALGAMEJAM_API FTDDirectorTuning
{
	GENERATED_BODY()

	/** Base importance per kind (0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Director")
	TMap<ETDHighlightKind, float> BaseImportance;

	/** Highlights below this are ignored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Director", meta = (ClampMin = "0", ClampMax = "1"))
	float MinImportance = 0.3f;

	/** Slow-mo length at importance 0 and 1 (C-03: 0.5 - 1 s, real time). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Director", meta = (Units = "s"))
	float MinSlowMoSeconds = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Director", meta = (Units = "s"))
	float MaxSlowMoSeconds = 1.0f;

	/** Per-kind slow-mo length overriding the importance curve (ENG-22: a perfect deflect is a 0.15 s beat). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Director")
	TMap<ETDHighlightKind, float> DurationOverrideSeconds;

	/** Time dilation at importance 0 and 1 (lower = slower). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Director", meta = (ClampMin = "0.05", ClampMax = "1"))
	float DilationAtMinImportance = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Director", meta = (ClampMin = "0.05", ClampMax = "1"))
	float DilationAtMaxImportance = 0.2f;

	/** After a shot ends, no new shot for this long (real seconds)... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Director", meta = (Units = "s"))
	float GlobalCooldownSeconds = 3.f;

	/** ...and the same kind again only after this long... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Director", meta = (Units = "s"))
	float SameKindCooldownSeconds = 6.f;

	/** ...unless it is at least this important (generator, reactor, carrier kills always get their beat). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Director", meta = (ClampMin = "0", ClampMax = "1"))
	float CooldownBypassImportance = 0.8f;

	/** During a shot, a new highlight replaces it if this much more important (story beats at
	 *  CooldownBypassImportance or above replace any shot that is not more important than them). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Director", meta = (ClampMin = "0", ClampMax = "1"))
	float PreemptMargin = 0.25f;

	/** Multi-kill: this many player kills... */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Director", meta = (ClampMin = "2"))
	int32 MultiKillCount = 3;

	/** ...within this window (real seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Director", meta = (Units = "s"))
	float MultiKillWindowSeconds = 1.5f;

	/** Multi-kill importance never exceeds this (and always stays below CooldownBypassImportance). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Director", meta = (ClampMin = "0", ClampMax = "1"))
	float MultiKillMaxImportance = 0.7f;

	/** Extra importance per kill beyond MultiKillCount. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Director")
	float MultiKillImportancePerExtraKill = 0.1f;

	/** Suggested camera distance from the focus at importance 0 and 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Director", meta = (Units = "cm"))
	float CameraDistanceMin = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Director", meta = (Units = "cm"))
	float CameraDistanceMax = 7000.f;

	FTDDirectorTuning();
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTDOnDirectorShot, const FTDDirectorShot&, Shot);

class UTDDirectorCameraRig;

UCLASS()
class UNREALGAMEJAM_API UTDDirectorSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTDDirectorSubsystem* Get(const UObject* WorldContext);

	/**
	 * Report a big moment. Importance01 < 0 uses the kind's base importance. Returns true if it
	 * became a money shot (event + slow-mo + camera hint); false if rate-limited or suppressed.
	 */
	UFUNCTION(BlueprintCallable, Category = "TriadDrift|Director")
	bool ReportHighlight(ETDHighlightKind Kind, AActor* Subject, float Importance01 = -1.f, AActor* Secondary = nullptr);

	/** True while a money shot's slow-mo is running. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Director")
	bool IsShotActive() const;

	/** The current (or last) shot. Check IsShotActive. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Director")
	const FTDDirectorShot& GetCurrentShot() const { return CurrentShot; }

	/** False while a retake ("CUT!") or level travel is pending: no money shots then. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Director")
	bool CanTriggerHighlights() const;

	/** Number of money shots fired this world (also counted in FTDRunStats::HighlightMoments). */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Director")
	int32 GetShotCount() const { return ShotCount; }

	/** Fired for every accepted shot, after UTDEventBus::OnHighlightMoment. */
	UPROPERTY(BlueprintAssignable, Category = "TriadDrift|Director")
	FTDOnDirectorShot OnDirectorShot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TriadDrift|Director")
	FTDDirectorTuning Tuning;

	/** The director camera (C-03 #2): view takeover, blends, cancels. */
	UFUNCTION(BlueprintPure, Category = "TriadDrift|Director")
	UTDDirectorCameraRig* GetCameraRig() const { return CameraRig; }

	// ---- UWorldSubsystem
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	// ---- bus handlers
	UFUNCTION()
	void HandleActorKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind);

	UFUNCTION()
	void HandleSceneCut(FName SceneId, int32 NextTakeNumber, float DelaySeconds);

	UFUNCTION()
	void HandleTakeStarted(FName SceneId, int32 TakeNumber);

private:
	void BindBus();
	void UnbindBus();
	double RealNow() const;
	double GameNow() const;
	float GetBaseImportance(ETDHighlightKind Kind) const;
	void BuildCameraHint(FTDDirectorShot& Shot);

	UPROPERTY(Transient)
	TObjectPtr<UTDDirectorCameraRig> CameraRig;

	double LastTickRealTime = -1.0;

	FTDDirectorShot CurrentShot;
	double ShotEndRealTime = -1000.0;
	double GlobalCooldownEndRealTime = -1000.0;
	TMap<ETDHighlightKind, double> KindCooldownEnd;

	/** Recent player kill times (game seconds), oldest first; bounded. */
	TArray<double> RecentKillTimes;

	int32 ShotCount = 0;
	bool bSceneCutPending = false;
	bool bCameraSideFlip = false;
	bool bBusBound = false;
};

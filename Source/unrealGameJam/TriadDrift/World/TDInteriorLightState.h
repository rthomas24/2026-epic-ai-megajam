// Triad Drift - World (environment builder, ART-13, Nov 14). The Dominion interior's lighting states, driven by T5's
// ATDPlanetCourse::OnInteriorMoment without touching T5's files:
//   Normal   (CoreChamberEntered / ReactorFightStarted, and at start): every light as T5 built it.
//   Alarm    (ReactorFightEnded / EscapeStarted): a pulsing red wash; emergency strips pulse toward the exit.
//   Collapse (the last CollapseSeconds of the escape): the alarm plus random dropouts and brown-outs (flicker).
// It drives lights tagged LightTag (point / spot / rect components, or every light on an actor with that tag) and
// primitives tagged StripTag (scalar material parameter StripParam). With nothing tagged it does nothing.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDInteriorLightState.generated.h"

class ULightComponent;
class UMeshComponent;

UENUM()
enum class ETDInteriorLightMode : uint8 { Normal, Alarm, Collapse };

UCLASS()
class UNREALGAMEJAM_API UTDInteriorLightSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static const FName LightTag;	// TD.InteriorLight
	static const FName StripTag;	// TD.EmergencyStrip
	static const FName StripParam;	// EmergencyPulse (0..1 on the strip material)

	/** The collapse window at the escape's end (T5's ATDPlanetCourse::GetEscapeSecondsLeft(); when the moment's source is
	 *  not a course, a timer from EscapeStarted with EscapeSeconds, the storyline's 90 s run). */
	float EscapeSeconds = 90.f;
	float CollapseSeconds = 30.f;
	/** Alarm look. */
	FLinearColor AlarmColor = FLinearColor(1.f, 0.07f, 0.03f);
	float AlarmTint = 0.75f;		// how far each light's colour goes toward AlarmColor
	float PulseHz = 1.1f;
	float PulseMin = 0.35f;			// intensity multiplier at the pulse's trough (peak 1.2)

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return Mode != ETDInteriorLightMode::Normal || bRestorePending; }

	/** The moment handler (also called by tests). Moments from another world are ignored. */
	void HandleMoment(FName Moment, AActor* Source);
	/** Advance the state by DeltaTime (Tick calls this; tests call it directly). */
	void Step(float DeltaTime);

	ETDInteriorLightMode GetMode() const { return Mode; }
	/** Dev / captures (TD.Env.InteriorLight <Normal|Alarm|Collapse>): jump straight to a state. */
	void ForceMode(ETDInteriorLightMode NewMode);
	int32 GetLightCount() const { return Lights.Num(); }

private:
	struct FLightRef { TWeakObjectPtr<ULightComponent> Light; float Intensity = 0.f; FLinearColor Color = FLinearColor::White; };
	void Gather();
	void Apply(float Pulse, float Flicker);
	void Restore();

	ETDInteriorLightMode Mode = ETDInteriorLightMode::Normal;
	bool bRestorePending = false;
	float Time = 0.f;
	float SinceEscape = -1.f;
	TArray<FLightRef> Lights;
	TArray<TWeakObjectPtr<UMeshComponent>> Strips;
	FDelegateHandle MomentHandle;
	TWeakObjectPtr<class ATDPlanetCourse> Course;	// the source of the moments: its GetEscapeSecondsLeft() drives the collapse
	FRandomStream Rng = FRandomStream(4517);
	float FlickerHold = 0.f, FlickerValue = 1.f;
};

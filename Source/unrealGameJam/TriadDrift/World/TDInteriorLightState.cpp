// Triad Drift - World (environment builder, ART-13). See TDInteriorLightState.h.

#include "TriadDrift/World/TDInteriorLightState.h"

#include "Components/LightComponent.h"
#include "Components/MeshComponent.h"
#include "EngineUtils.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"

const FName UTDInteriorLightSubsystem::LightTag(TEXT("TD.InteriorLight"));
const FName UTDInteriorLightSubsystem::StripTag(TEXT("TD.EmergencyStrip"));
const FName UTDInteriorLightSubsystem::StripParam(TEXT("EmergencyPulse"));

void UTDInteriorLightSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	MomentHandle = ATDPlanetCourse::OnInteriorMoment.AddUObject(this, &UTDInteriorLightSubsystem::HandleMoment);
}

void UTDInteriorLightSubsystem::Deinitialize()
{
	ATDPlanetCourse::OnInteriorMoment.Remove(MomentHandle);
	Restore();
	Super::Deinitialize();
}

TStatId UTDInteriorLightSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDInteriorLightSubsystem, STATGROUP_Tickables);
}

void UTDInteriorLightSubsystem::HandleMoment(FName Moment, AActor* Source)
{
	if (Source && Source->GetWorld() != GetWorld())
	{
		return;
	}
	if (Moment == TEXT("CoreChamberEntered") || Moment == TEXT("ReactorFightStarted"))
	{
		// A retake from before the reactor's death: back to the interior as built.
		if (Mode != ETDInteriorLightMode::Normal) { Restore(); }
		Mode = ETDInteriorLightMode::Normal;
		SinceEscape = -1.f;
		return;
	}
	if (Moment == TEXT("ReactorFightEnded") || Moment == TEXT("EscapeStarted"))
	{
		if (Mode == ETDInteriorLightMode::Normal)
		{
			Gather();
			Time = 0.f;
			Mode = ETDInteriorLightMode::Alarm;
		}
		if (Moment == TEXT("EscapeStarted") && SinceEscape < 0.f)
		{
			SinceEscape = 0.f;
		}
	}
}

void UTDInteriorLightSubsystem::Gather()
{
	Lights.Reset();
	Strips.Reset();
	UWorld* W = GetWorld();
	if (!W)
	{
		return;
	}
	for (TActorIterator<AActor> It(W); It; ++It)
	{
		const bool bActorTagged = It->ActorHasTag(LightTag);
		TInlineComponentArray<UActorComponent*> Comps(*It);
		for (UActorComponent* C : Comps)
		{
			if (ULightComponent* L = Cast<ULightComponent>(C))
			{
				if (bActorTagged || L->ComponentHasTag(LightTag))
				{
					FLightRef R;
					R.Light = L;
					R.Intensity = L->Intensity;
					R.Color = L->GetLightColor();
					Lights.Add(R);
				}
			}
			else if (UMeshComponent* P = Cast<UMeshComponent>(C))
			{
				if (P->ComponentHasTag(StripTag) || It->ActorHasTag(StripTag)) { Strips.Add(P); }
			}
		}
	}
}

void UTDInteriorLightSubsystem::Step(float DeltaTime)
{
	if (Mode == ETDInteriorLightMode::Normal)
	{
		if (bRestorePending) { Restore(); }
		return;
	}
	Time += DeltaTime;
	if (SinceEscape >= 0.f)
	{
		SinceEscape += DeltaTime;
		if (SinceEscape >= EscapeSeconds - CollapseSeconds) { Mode = ETDInteriorLightMode::Collapse; }
	}
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Time * 2.f * PI * PulseHz);
	float Flicker = 1.f;
	if (Mode == ETDInteriorLightMode::Collapse)
	{
		// Brown-outs and dropouts: hold a random level for 40-250 ms, deeper and more often as the end nears.
		FlickerHold -= DeltaTime;
		if (FlickerHold <= 0.f)
		{
			const float Late = FMath::Clamp((SinceEscape - (EscapeSeconds - CollapseSeconds)) / FMath::Max(CollapseSeconds, 1.f), 0.f, 1.f);
			const float R = Rng.FRand();
			FlickerValue = R < 0.12f + 0.2f * Late ? Rng.FRandRange(0.f, 0.15f) : (R < 0.45f ? Rng.FRandRange(0.4f, 0.8f) : 1.f);
			FlickerHold = Rng.FRandRange(0.04f, 0.25f);
		}
		Flicker = FlickerValue;
	}
	Apply(Pulse, Flicker);
}

void UTDInteriorLightSubsystem::Tick(float DeltaTime)
{
	Step(DeltaTime);
}

void UTDInteriorLightSubsystem::Apply(float Pulse, float Flicker)
{
	const float Mul = FMath::Lerp(PulseMin, 1.2f, Pulse) * Flicker;
	for (const FLightRef& R : Lights)
	{
		if (ULightComponent* L = R.Light.Get())
		{
			L->SetIntensity(R.Intensity * Mul);
			L->SetLightColor(FMath::Lerp(R.Color, AlarmColor, AlarmTint));
		}
	}
	for (const TWeakObjectPtr<UMeshComponent>& S : Strips)
	{
		if (UMeshComponent* P = S.Get()) { P->SetScalarParameterValueOnMaterials(StripParam, Pulse * Flicker); }
	}
	bRestorePending = true;
}

void UTDInteriorLightSubsystem::Restore()
{
	for (const FLightRef& R : Lights)
	{
		if (ULightComponent* L = R.Light.Get())
		{
			L->SetIntensity(R.Intensity);
			L->SetLightColor(R.Color);
		}
	}
	for (const TWeakObjectPtr<UMeshComponent>& S : Strips)
	{
		if (UMeshComponent* P = S.Get()) { P->SetScalarParameterValueOnMaterials(StripParam, 0.f); }
	}
	Lights.Reset();
	Strips.Reset();
	bRestorePending = false;
}

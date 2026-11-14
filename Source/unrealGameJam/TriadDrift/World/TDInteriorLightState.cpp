// Triad Drift - World (environment builder, ART-13). See TDInteriorLightState.h.

#include "TriadDrift/World/TDInteriorLightState.h"

#include "Components/LightComponent.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"

const FName UTDInteriorLightSubsystem::LightTag(ATDPlanetCourse::InteriorLightTag);	// T5's tag on the interior's lights
const FName UTDInteriorLightSubsystem::StripTag(TEXT("TD.EmergencyStrip"));
const FName UTDInteriorLightSubsystem::StripParam(TEXT("GlowStrength"));

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
	if (ATDPlanetCourse* C = Cast<ATDPlanetCourse>(Source)) { Course = C; }
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
					const float Roll = Rng.FRand();
					R.Role = Roll < 0.4f ? ERole::Dead : (Roll < 0.7f ? ERole::Flicker : ERole::Dim);
					Lights.Add(R);
				}
			}
			else if (UMeshComponent* P = Cast<UMeshComponent>(C))
			{
				if (P->ComponentHasTag(StripTag) || It->ActorHasTag(StripTag))
				{
					for (int32 m = 0; m < P->GetNumMaterials(); ++m)
					{
						UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(P->GetMaterial(m));
						float Base = 0.f;
						if (Mid && Mid->GetScalarParameterValue(FHashedMaterialParameterInfo(StripParam), Base))
						{
							FStripRef S; S.Mid = Mid; S.Base = Base;
							Strips.Add(S);
						}
					}
				}
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
		const float Left = Course.IsValid() ? Course->GetEscapeSecondsLeft() : -1.f;
		if (Left >= 0.f ? Left <= CollapseSeconds : SinceEscape >= EscapeSeconds - CollapseSeconds) { Mode = ETDInteriorLightMode::Collapse; }
	}
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Time * 2.f * PI * PulseHz);
	float Late = 0.f;
	if (Mode == ETDInteriorLightMode::Collapse)
	{
		const float Left = Course.IsValid() ? Course->GetEscapeSecondsLeft() : -1.f;
		Late = Left >= 0.f ? FMath::Clamp(1.f - Left / FMath::Max(CollapseSeconds, 1.f), 0.f, 1.f)
			: FMath::Clamp((SinceEscape - (EscapeSeconds - CollapseSeconds)) / FMath::Max(CollapseSeconds, 1.f), 0.f, 1.f);
	}
	Apply(Pulse, Late, DeltaTime);
}

void UTDInteriorLightSubsystem::Tick(float DeltaTime)
{
	Step(DeltaTime);
}

void UTDInteriorLightSubsystem::Apply(float Pulse, float Late, float DeltaTime)
{
	const bool bCollapse = Mode == ETDInteriorLightMode::Collapse;
	const float AlarmMul = FMath::Lerp(PulseMin, 1.2f, Pulse);
	for (FLightRef& R : Lights)
	{
		ULightComponent* L = R.Light.Get();
		if (!L) { continue; }
		float Mul = AlarmMul;
		if (bCollapse)
		{
			// Sections dead, some lights stuttering on their own, the rest browned out: darker overall, never in step.
			switch (R.Role)
			{
			case ERole::Dead: Mul = 0.f; break;
			case ERole::Dim: Mul = 0.22f * (1.f - 0.5f * Late); break;
			case ERole::Flicker:
				R.Hold -= DeltaTime;
				if (R.Hold <= 0.f)
				{
					const float Roll = Rng.FRand();
					R.Level = Roll < 0.35f + 0.25f * Late ? 0.f : (Roll < 0.7f ? Rng.FRandRange(0.15f, 0.5f) : Rng.FRandRange(0.8f, 1.3f));
					R.Hold = Rng.FRandRange(0.03f, 0.22f);
				}
				Mul = R.Level;
				break;
			}
		}
		L->SetIntensity(R.Intensity * Mul);
		L->SetLightColor(FMath::Lerp(R.Color, AlarmColor, bCollapse && R.Role == ERole::Flicker ? 0.35f : AlarmTint));
	}
	// The emergency strips: pulsing with the alarm; in the collapse the steady main light, a little brighter.
	const float StripMul = bCollapse ? 1.8f : FMath::Lerp(0.5f, 1.6f, Pulse);
	for (const FStripRef& S : Strips)
	{
		if (UMaterialInstanceDynamic* M = S.Mid.Get()) { M->SetScalarParameterValue(StripParam, S.Base * StripMul); }
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
	for (const FStripRef& S : Strips)
	{
		if (UMaterialInstanceDynamic* M = S.Mid.Get()) { M->SetScalarParameterValue(StripParam, S.Base); }
	}
	Lights.Reset();
	Strips.Reset();
	bRestorePending = false;
}

void UTDInteriorLightSubsystem::ForceMode(ETDInteriorLightMode NewMode)
{
	if (NewMode == ETDInteriorLightMode::Normal)
	{
		HandleMoment(TEXT("CoreChamberEntered"), nullptr);
		return;
	}
	HandleMoment(TEXT("EscapeStarted"), nullptr);
	if (NewMode == ETDInteriorLightMode::Collapse)
	{
		Course.Reset();
		SinceEscape = EscapeSeconds - CollapseSeconds;
	}
}

#if !UE_BUILD_SHIPPING
namespace
{
	FAutoConsoleCommandWithWorldAndArgs GTDInteriorLightCmd(TEXT("TD.Env.InteriorLight"),
		TEXT("TD.Env.InteriorLight <Normal|Alarm|Collapse>: show an interior lighting state (captures)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UTDInteriorLightSubsystem* S = World ? World->GetSubsystem<UTDInteriorLightSubsystem>() : nullptr;
			if (!S || Args.Num() < 1) { return; }
			const FString& A = Args[0];
			S->ForceMode(A.Equals(TEXT("Collapse"), ESearchCase::IgnoreCase) ? ETDInteriorLightMode::Collapse
				: (A.Equals(TEXT("Alarm"), ESearchCase::IgnoreCase) ? ETDInteriorLightMode::Alarm : ETDInteriorLightMode::Normal));
			UE_LOG(LogTemp, Display, TEXT("TD.Env.InteriorLight %s: %d lights"), *A, S->GetLightCount());
		}));
}
#endif

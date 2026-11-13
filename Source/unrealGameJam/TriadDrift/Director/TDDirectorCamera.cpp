// Triad Drift - Director (Teammate 3). C-03 #2: the director camera that actually cuts.

#include "TriadDrift/Director/TDDirectorCamera.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/World/TDWorldSubsystem.h"

namespace TDDirectorCameraPrivate
{
	static TAutoConsoleVariable<int32> CVarDirectorCam(
		TEXT("td.DirectorCam"), -1,
		TEXT("Director's Cut camera: -1 = project setting, 0 = off, 1 = reduced (slow-mo + FOV punch), 2 = full cut."),
		ECVF_Default);

#if !UE_BUILD_SHIPPING
	static TAutoConsoleVariable<float> CVarDebugHold(
		TEXT("td.Director.DebugHoldSeconds"), 0.f,
		TEXT("Dev only: extra real seconds added to director shots and cut cards, for screenshot capture."),
		ECVF_Cheat);
#endif

	static float Ease(float A)
	{
		A = FMath::Clamp(A, 0.f, 1.f);
		return A * A * (3.f - 2.f * A); // smoothstep: fast, no snap at either end
	}
}

using namespace TDDirectorCameraPrivate;

// ---------------------------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------------------------

UTDDirectorSettings::UTDDirectorSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Triad Drift Director");
	// Story beats get a touch slower in and out.
	FTDDirectorBlend Story;
	Story.BlendIn = 0.18f;
	Story.BlendOut = 0.3f;
	BlendByKind.Add(ETDHighlightKind::ReactorKill, Story);
	BlendByKind.Add(ETDHighlightKind::GeneratorKill, Story);
	BlendByKind.Add(ETDHighlightKind::CarrierKill, Story);
}

ETDDirectorCameraMode UTDDirectorSettings::GetEffectiveMode()
{
	const int32 Override = CVarDirectorCam.GetValueOnGameThread();
	if (Override >= 0 && Override <= 2)
	{
		return static_cast<ETDDirectorCameraMode>(Override);
	}
	return Get()->CameraMode;
}

float UTDDirectorSettings::GetDebugHoldSeconds()
{
#if !UE_BUILD_SHIPPING
	return FMath::Max(0.f, CVarDebugHold.GetValueOnGameThread());
#else
	return 0.f;
#endif
}

const FTDDirectorBlend& UTDDirectorSettings::GetBlend(ETDHighlightKind Kind) const
{
	const FTDDirectorBlend* B = BlendByKind.Find(Kind);
	return B ? *B : DefaultBlend;
}

// ---------------------------------------------------------------------------------------------
// FOV punch (Reduced mode)
// ---------------------------------------------------------------------------------------------

void UTDDirectorFOVPunch::Punch(float Degrees, float RealSeconds)
{
	const UWorld* World = GetWorld();
	StartRealTime = World ? World->GetRealTimeSeconds() : 0.0;
	PunchDegrees = Degrees;
	PunchSeconds = FMath::Max(0.01f, RealSeconds);
}

float UTDDirectorFOVPunch::GetCurrentPunch() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}
	const float T = static_cast<float>(World->GetRealTimeSeconds() - StartRealTime) / PunchSeconds;
	if (T < 0.f || T >= 1.f)
	{
		return 0.f;
	}
	// Fast attack, smooth release.
	return PunchDegrees * (T < 0.2f ? T / 0.2f : 1.f - Ease((T - 0.2f) / 0.8f));
}

bool UTDDirectorFOVPunch::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	InOutPOV.FOV += GetCurrentPunch();
	return false;
}

// ---------------------------------------------------------------------------------------------
// Camera actor
// ---------------------------------------------------------------------------------------------

ATDDirectorCameraActor::ATDDirectorCameraActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);
	if (UCameraComponent* Cam = GetCameraComponent())
	{
		Cam->bConstrainAspectRatio = false;
		Cam->SetFieldOfView(65.f);
	}
}

// ---------------------------------------------------------------------------------------------
// Rig
// ---------------------------------------------------------------------------------------------

APlayerController* UTDDirectorCameraRig::GetPlayerController() const
{
	UWorld* World = OwnerWorld.Get();
	if (!World)
	{
		return nullptr;
	}
	if (const ATDSquadManager* Squad = ATDSquadManager::Get(World))
	{
		if (APlayerController* SquadPC = Squad->GetSquadPlayerController())
		{
			return SquadPC;
		}
	}
	return World->GetFirstPlayerController();
}

ATDShip* UTDDirectorCameraRig::GetPlayerShip() const
{
	const APlayerController* Controller = PC.Get();
	return Controller ? Cast<ATDShip>(Controller->GetPawn()) : nullptr;
}

bool UTDDirectorCameraRig::ResolveCameraLocation(const UWorld* World, const FVector& Focus, const FVector& Preferred,
	const TArray<const AActor*>& Ignore, const AActor* PlayerShip, FVector& OutLocation)
{
	if (!World)
	{
		return false;
	}
	const UTDDirectorSettings* S = UTDDirectorSettings::Get();
	const UTDWorldSubsystem* WorldSub = World->GetSubsystem<UTDWorldSubsystem>();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDDirectorCam), false);
	for (const AActor* A : Ignore)
	{
		Params.AddIgnoredActor(A);
	}
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	Objects.AddObjectTypesToQuery(TDCollision::Ship);
	const FCollisionShape Probe = FCollisionShape::MakeSphere(S->CameraProbeRadius);

	const FVector Offset = Preferred - Focus;
	const float Dist = Offset.Size();
	if (Dist < KINDA_SMALL_NUMBER)
	{
		return false;
	}
	// Preferred, the other side, the two quarter turns, then straight above.
	const FVector Candidates[] = {
		Offset,
		Offset.RotateAngleAxis(180.f, FVector::UpVector),
		Offset.RotateAngleAxis(90.f, FVector::UpVector),
		Offset.RotateAngleAxis(-90.f, FVector::UpVector),
		FVector::UpVector * Dist
	};

	bool bHaveFallback = false;
	FVector Fallback = FVector::ZeroVector;
	for (const FVector& C : Candidates)
	{
		FVector P = Focus + C;
		bool bPulledIn = false;
		FHitResult Hit;
		if (World->SweepSingleByObjectType(Hit, Focus, P, FQuat::Identity, Objects, Probe, Params))
		{
			// Pull in to just short of whatever is in the way.
			P = Focus + (P - Focus).GetSafeNormal() * FMath::Max(0.f, Hit.Distance - S->CameraProbeRadius);
			bPulledIn = true;
		}
		if (FVector::Dist(P, Focus) < S->MinDistanceFromFocus)
		{
			continue;
		}
		if (WorldSub && WorldSub->IsInsideObstacle(P, S->CameraProbeRadius))
		{
			continue;
		}
		if (PlayerShip && FVector::Dist(P, PlayerShip->GetActorLocation()) < S->MinDistanceFromPlayerShip)
		{
			continue;
		}
		// The camera itself must not sit inside anything.
		if (World->OverlapAnyTestByObjectType(P, FQuat::Identity, Objects, Probe, Params))
		{
			continue;
		}
		if (!bPulledIn)
		{
			OutLocation = P;
			return true;
		}
		if (!bHaveFallback)
		{
			Fallback = P;
			bHaveFallback = true;
		}
	}
	if (bHaveFallback)
	{
		OutLocation = Fallback;
		return true;
	}
	return false;
}

FVector UTDDirectorCameraRig::TrackedLocationFor(const FVector& Focus) const
{
	// Keep the placement offset relative to the moving focus; if something has come between them
	// this frame, pull in (never through geometry or ships).
	const FVector Desired = Focus + ShotOffset;
	const UWorld* World = OwnerWorld.Get();
	if (!World)
	{
		return Desired;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDDirectorCamTrack), false);
	Params.AddIgnoredActor(Subject.Get());
	Params.AddIgnoredActor(Secondary.Get());
	Params.AddIgnoredActor(PawnAtStart.Get());
	for (const TWeakObjectPtr<AActor>& Extra : PlacementIgnore)
	{
		Params.AddIgnoredActor(Extra.Get());
	}
	// Squad wingmen are moving and steer clear of the shot camera themselves: pulling in for one put
	// the camera right in front of it, filling the frame (live capture, ram kill).
	if (const ATDSquadManager* Squad = ATDSquadManager::Get(World))
	{
		for (const ATDShip* Mate : Squad->GetLivingShips())
		{
			Params.AddIgnoredActor(Mate);
		}
	}
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	Objects.AddObjectTypesToQuery(TDCollision::Ship);
	FHitResult Hit;
	const float R = UTDDirectorSettings::Get()->CameraProbeRadius;
	if (World->SweepSingleByObjectType(Hit, Focus, Desired, FQuat::Identity, Objects, FCollisionShape::MakeSphere(R), Params))
	{
		return Focus + (Desired - Focus).GetSafeNormal() * FMath::Max(0.f, Hit.Distance - R);
	}
	return Desired;
}

FVector UTDDirectorCameraRig::CurrentFocus() const
{
	if (bSubjectGone || bStaticFocus)
	{
		return LastFocus;
	}
	const AActor* A = Subject.Get();
	const AActor* B = Secondary.Get();
	// A dead secondary (the fighter that was just rammed) is left behind: frame the subject alone.
	if (const UTDHealthComponent* BH = UTDStatics::GetHealth(B); BH && BH->IsDead() && A)
	{
		B = nullptr;
	}
	const UTDDirectorSubsystem* Director = OwnerWorld.IsValid() ? UTDDirectorSubsystem::Get(OwnerWorld.Get()) : nullptr;
	const float MaxTwoShot = Director ? Director->Tuning.MaxTwoShotDistance : 8000.f;
	if (A && B && FVector::Dist(A->GetActorLocation(), B->GetActorLocation()) <= MaxTwoShot)
	{
		return FMath::Lerp(A->GetActorLocation(), B->GetActorLocation(), 0.35f);
	}
	if (A)
	{
		return A->GetActorLocation();
	}
	return LastFocus;
}

void UTDDirectorCameraRig::ApplyPose(const FVector& Location, const FRotator& Rotation, float FOV)
{
	if (CameraActor)
	{
		CameraActor->SetActorLocationAndRotation(Location, Rotation);
		if (UCameraComponent* Cam = CameraActor->GetCameraComponent())
		{
			Cam->SetFieldOfView(FOV);
		}
	}
}

void UTDDirectorCameraRig::PunchFOV()
{
	APlayerController* Controller = GetPlayerController();
	if (!Controller || !Controller->PlayerCameraManager)
	{
		return;
	}
	UTDDirectorFOVPunch* Mod = FOVPunch.Get();
	if (!Mod)
	{
		Mod = Cast<UTDDirectorFOVPunch>(Controller->PlayerCameraManager->FindCameraModifierByClass(UTDDirectorFOVPunch::StaticClass()));
		if (!Mod)
		{
			Mod = Cast<UTDDirectorFOVPunch>(Controller->PlayerCameraManager->AddNewCameraModifier(UTDDirectorFOVPunch::StaticClass()));
		}
		FOVPunch = Mod;
	}
	if (Mod)
	{
		const UTDDirectorSettings* S = UTDDirectorSettings::Get();
		Mod->Punch(S->FOVPunchDegrees, S->FOVPunchSeconds);
	}
}

bool UTDDirectorCameraRig::BeginShot(const FTDDirectorShot& Shot, double ShotEndRealTime)
{
	UWorld* World = OwnerWorld.Get();
	APlayerController* Controller = GetPlayerController();
	ATDShip* Ship = Controller ? Cast<ATDShip>(Controller->GetPawn()) : nullptr;
	if (!World || !Controller)
	{
		return false;
	}
	if (!Ship)
	{
		// On foot (a perfect deflect, the blade): the view stays first-person. The director never cuts
		// away from the pilot's eyes, so the body is never framed from outside and the first-person
		// head-hiding is never touched (rule agreed with T5). The beat is the slow-mo plus an FOV punch.
		PunchFOV();
		return false;
	}
	const UTDDirectorSettings* S = UTDDirectorSettings::Get();

	// Where can the camera go?
	TArray<const AActor*> Ignore;
	Ignore.Add(Shot.Subject.Get());
	Ignore.Add(Shot.Secondary.Get());
	Ignore.Add(Ship);
	for (const TWeakObjectPtr<AActor>& Extra : Shot.IgnoreForPlacement)
	{
		Ignore.Add(Extra.Get());
	}
	FVector Location;
	if (!ResolveCameraLocation(World, Shot.FocusLocation, Shot.SuggestedCameraLocation, Ignore, Ship, Location))
	{
		PunchFOV(); // nowhere safe to cut to: keep the view, still mark the beat
		return false;
	}

	if (!CameraActor)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		CameraActor = World->SpawnActor<ATDDirectorCameraActor>(ATDDirectorCameraActor::StaticClass(), FTransform::Identity, Params);
		if (!CameraActor)
		{
			return false;
		}
	}

	const double Now = World->GetRealTimeSeconds();
	if (IsActive() && CameraActor)
	{
		// A bigger shot takes over mid-shot: blend from where the camera is now.
		FromLocation = CameraActor->GetActorLocation();
		FromRotation = CameraActor->GetActorRotation();
		FromFOV = CameraActor->GetCameraComponent() ? CameraActor->GetCameraComponent()->FieldOfView : ShotFOV;
	}
	else
	{
		// Start exactly on the live chase camera: becoming the view target can't pop.
		const FTransform Chase = Ship->GetChaseCameraTransformNow();
		FromLocation = Chase.GetLocation();
		FromRotation = Chase.Rotator();
		FromFOV = Ship->GetChaseCameraFOVNow();
	}

	PC = Controller;
	bSubjectGone = false;
	PawnAtStart = Ship;
	Subject = Shot.Subject;
	Secondary = Shot.Secondary;
	const UTDHealthComponent* SubjectHealth = UTDStatics::GetHealth(Shot.Subject.Get());
	bSubjectAliveAtStart = Shot.Subject.IsValid() && (!SubjectHealth || SubjectHealth->IsAlive());
	PlacementIgnore = Shot.IgnoreForPlacement;
	bStaticFocus = Shot.bStaticFocus;
	LastFocus = Shot.FocusLocation;
	ShotLocation = Location;
	ShotOffset = Location - Shot.FocusLocation;
	TrackedLocation = Location;
	ShotFOV = S->ShotFOV;
	const FTDDirectorBlend& Blend = S->GetBlend(Shot.Kind);
	BlendIn = Blend.BlendIn;
	BlendOut = Blend.BlendOut;
	HoldEnd = FMath::Max(ShotEndRealTime, Now + BlendIn) + UTDDirectorSettings::GetDebugHoldSeconds();
	Phase = ETDDirectorCameraPhase::BlendIn;
	PhaseStart = Now;

	ApplyPose(FromLocation, FromRotation, FromFOV);
	if (Controller->GetViewTarget() != CameraActor)
	{
		Controller->SetViewTarget(CameraActor); // no engine blend: the rig does the blend in real time
	}
	return true;
}

void UTDDirectorCameraRig::FinishReturn()
{
	if (APlayerController* Controller = PC.Get())
	{
		if (Controller->GetViewTarget() == CameraActor)
		{
			Controller->SetViewTarget(Controller->GetPawn());
		}
	}
	Phase = ETDDirectorCameraPhase::Idle;
}

void UTDDirectorCameraRig::Cancel(ETDDirectorCameraCancel Reason)
{
	if (!IsActive())
	{
		return;
	}
	LastCancel = Reason;
	++CancelCount;
	FinishReturn();
}

void UTDDirectorCameraRig::Tick(double RealNow, float RealDelta)
{
	if (!IsActive())
	{
		return;
	}
	APlayerController* Controller = PC.Get();
	ATDShip* Ship = GetPlayerShip();

	// ---- cancel paths
	if (UTDDirectorSettings::GetEffectiveMode() != ETDDirectorCameraMode::Full)
	{
		Cancel(ETDDirectorCameraCancel::Disabled);
		return;
	}
	// A switch (possessing another ship also moves the view target onto it) ends the shot first.
	if (Controller && (!Ship || Ship != PawnAtStart.Get()))
	{
		Cancel(ETDDirectorCameraCancel::ShipSwitch);
		return;
	}
	if (!Controller || !CameraActor || Controller->GetViewTarget() != CameraActor)
	{
		// Someone else (cinematic, travel, menu) took the view: let go without touching it.
		LastCancel = ETDDirectorCameraCancel::ViewTakenOver;
		++CancelCount;
		Phase = ETDDirectorCameraPhase::Idle;
		return;
	}
	if (const UTDHealthComponent* H = UTDStatics::GetHealth(Ship); H && H->IsDead())
	{
		Cancel(ETDDirectorCameraCancel::PlayerDown);
		return;
	}
	if (bSubjectAliveAtStart && !bSubjectGone)
	{
		const AActor* S = Subject.Get();
		const UTDHealthComponent* SH = UTDStatics::GetHealth(S);
		if (!IsValid(S) || (SH && SH->IsDead() && S != Ship))
		{
			// Review minor 7: the subject dying IS the payoff (a missile return kills its shooter). Hold the
			// frame on where it died until the shot ends, then blend out normally: no snap.
			bSubjectGone = true;
			LastCancel = ETDDirectorCameraCancel::SubjectGone;
		}
	}

	// ---- pose
	const FVector Focus = CurrentFocus();
	LastFocus = Focus;
	TrackedLocation = TrackedLocationFor(Focus);
	const FRotator ShotRotation = (Focus - TrackedLocation).Rotation();

	switch (Phase)
	{
	case ETDDirectorCameraPhase::BlendIn:
	{
		const float A = BlendIn > 0.f ? Ease(static_cast<float>((RealNow - PhaseStart) / BlendIn)) : 1.f;
		ApplyPose(FMath::Lerp(FromLocation, TrackedLocation, A),
			FQuat::Slerp(FromRotation.Quaternion(), ShotRotation.Quaternion(), A).Rotator(),
			FMath::Lerp(FromFOV, ShotFOV, A));
		if (A >= 1.f)
		{
			Phase = ETDDirectorCameraPhase::Hold;
			PhaseStart = RealNow;
		}
		break;
	}
	case ETDDirectorCameraPhase::Hold:
		ApplyPose(TrackedLocation, ShotRotation, ShotFOV);
		if (RealNow >= HoldEnd)
		{
			Phase = ETDDirectorCameraPhase::BlendOut;
			PhaseStart = RealNow;
			FromLocation = TrackedLocation;
			FromRotation = ShotRotation;
			FromFOV = ShotFOV;
		}
		break;
	case ETDDirectorCameraPhase::BlendOut:
	{
		// Blend onto the LIVE chase camera every frame, so we land exactly where it is.
		const FTransform Chase = Ship->GetChaseCameraTransformNow();
		const float A = BlendOut > 0.f ? Ease(static_cast<float>((RealNow - PhaseStart) / BlendOut)) : 1.f;
		ApplyPose(FMath::Lerp(FromLocation, Chase.GetLocation(), A),
			FQuat::Slerp(FromRotation.Quaternion(), Chase.GetRotation(), A).Rotator(),
			FMath::Lerp(FromFOV, Ship->GetChaseCameraFOVNow(), A));
		if (A >= 1.f)
		{
			FinishReturn();
		}
		break;
	}
	default:
		break;
	}
}

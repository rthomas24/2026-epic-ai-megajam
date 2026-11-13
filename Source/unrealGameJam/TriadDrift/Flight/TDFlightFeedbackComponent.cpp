// Triad Drift - flight feel that reads with NO art. Owner: Teammate 1 (Flight & Input).

#include "TriadDrift/Flight/TDFlightFeedbackComponent.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/World/TDPortalGate.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

namespace TDFlightFeedback
{
	static TAutoConsoleVariable<int32> CVarSpeedDust(TEXT("TD.Flight.SpeedDust"), 1,
		TEXT("Near-field speed dust behind the player's ship: 1 on (default), 0 off (frame-cost comparisons)."));
	static TAutoConsoleVariable<int32> CVarPlaceholders(TEXT("TD.Flight.Placeholders"), 1,
		TEXT("No-art flight feedback (camera kicks, placeholder glow bursts): 1 on (default), 0 off."));

	static UStaticMesh* LoadShape(const TCHAR* Path)
	{
		return LoadObject<UStaticMesh>(nullptr, Path);
	}

	static float Envelope(float Age, float Seconds)
	{
		// Fast attack, ease-out decay: reads as a kick, not a wobble.
		const float T = Seconds > 0.f ? FMath::Clamp(Age / Seconds, 0.f, 1.f) : 1.f;
		constexpr float Attack = 0.12f;
		return T < Attack ? T / Attack : FMath::Square(1.f - (T - Attack) / (1.f - Attack));
	}

	static float Bell(float X) { return FMath::Sin(PI * FMath::Clamp(X, 0.f, 1.f)); }
}

UTDFlightFeedbackComponent::UTDFlightFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	LastHyperState = ETDHyperdriveState::Idle;
	LastLandingState = ETDLandingState::Flying;
}

ATDShip* UTDFlightFeedbackComponent::GetShip() const
{
	return Cast<ATDShip>(GetOwner());
}

bool UTDFlightFeedbackComponent::WantsVisualPlaceholder(const FTDFXCue* Cue)
{
	return TDFlightFeedback::CVarPlaceholders.GetValueOnGameThread() != 0 && (!Cue || Cue->VFX.IsNull());
}

bool UTDFlightFeedbackComponent::WantsCameraPlaceholder(const FTDFXCue* Cue)
{
	return TDFlightFeedback::CVarPlaceholders.GetValueOnGameThread() != 0 && (!Cue || Cue->Shake.IsNull());
}

void UTDFlightFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();
	if (ATDShip* Ship = GetShip())
	{
		Ship->OnTeleported.AddDynamic(this, &UTDFlightFeedbackComponent::HandleTeleported);
		if (UTDShipMovementComponent* Move = Ship->GetShipMovement())
		{
			Move->OnDriftReleased.AddDynamic(this, &UTDFlightFeedbackComponent::HandleDriftReleased);
		}
	}
}

void UTDFlightFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (UStaticMeshComponent* Mesh : BurstMeshes)
	{
		if (Mesh) { Mesh->DestroyComponent(); }
	}
	BurstMeshes.Reset();
	BurstMIDs.Reset();
	Bursts.Reset();
	if (Dust) { Dust->DestroyComponent(); Dust = nullptr; }
	if (Trail) { Trail->DestroyComponent(); Trail = nullptr; }
	Super::EndPlay(EndPlayReason);
}

void UTDFlightFeedbackComponent::AddCameraKick(float Roll, float Pitch, float FOV, float Shake, float Seconds)
{
	FKick& K = Kicks.AddDefaulted_GetRef();
	K.Roll = Roll;
	K.Pitch = Pitch;
	K.FOV = FOV;
	K.Shake = Shake;
	K.Seconds = FMath::Max(Seconds, 0.05f);
	if (Kicks.Num() > 8)
	{
		Kicks.RemoveAt(0); // never pile up
	}
}

UTDFlightFeedbackComponent::FBurst* UTDFlightFeedbackComponent::AcquireBurst()
{
	for (FBurst& B : Bursts)
	{
		if (B.Age >= B.Seconds)
		{
			return &B;
		}
	}
	if (Bursts.Num() >= 4)
	{
		return &Bursts[0];
	}
	ATDShip* Ship = GetShip();
	UStaticMesh* Sphere = TDFlightFeedback::LoadShape(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!Ship || !Sphere || !Ship->GetRootComponent())
	{
		return nullptr;
	}
	UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Ship, NAME_None, RF_Transient);
	Mesh->SetStaticMesh(Sphere);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCastShadow(false);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->CanCharacterStepUpOn = ECB_No;
	Mesh->SetupAttachment(Ship->GetRootComponent());
	Mesh->RegisterComponent();
	Mesh->SetVisibility(false);
	FBurst& B = Bursts.AddDefaulted_GetRef();
	B.Mesh = Mesh;
	B.MID = UTDStatics::TintPlaceholder(Mesh, FLinearColor::White, 1.f);
	BurstMeshes.Add(Mesh);
	BurstMIDs.Add(B.MID);
	return &B;
}

void UTDFlightFeedbackComponent::SpawnBurst(const FVector& LocalOffset, const FVector& Scale, const FLinearColor& Color, float Seconds)
{
	FBurst* B = AcquireBurst();
	if (!B || !B->Mesh)
	{
		return;
	}
	B->Age = 0.f;
	B->Seconds = FMath::Max(Seconds, 0.05f);
	B->Color = Color;
	B->Scale = Scale;
	B->Mesh->SetRelativeLocation(LocalOffset);
	B->Mesh->SetRelativeScale3D(Scale);
	B->Mesh->SetVisibility(true);
	if (B->MID)
	{
		B->MID->SetVectorParameterValue(TEXT("Color"), Color * Tuning.BurstGlow);
	}
}

int32 UTDFlightFeedbackComponent::GetActiveBurstCount() const
{
	int32 N = 0;
	for (const FBurst& B : Bursts)
	{
		N += (B.Age < B.Seconds) ? 1 : 0;
	}
	return N;
}

void UTDFlightFeedbackComponent::PlayDodge(float Direction)
{
	ATDShip* Ship = GetShip();
	if (!Ship)
	{
		return;
	}
	const UTDShipVisuals* Visuals = Ship->GetShipVisuals();
	const FTDFXCue* Cue = Visuals ? &Visuals->Dodge : nullptr;
	const float Sign = Direction >= 0.f ? 1.f : -1.f;
	bool bAny = false;
	if (WantsVisualPlaceholder(Cue))
	{
		// The thruster that pushes the ship fires on the side away from the move.
		const FLinearColor Color = UTDStatics::ShipColor(Ship->GetShipId());
		SpawnBurst(FVector(-20.f, -Sign * 230.f, 0.f), FVector(0.7f, 2.4f, 0.7f), Color, Tuning.BurstSeconds);
		SpawnBurst(FVector(-20.f, -Sign * 380.f, 0.f), FVector(0.35f, 1.6f, 0.35f), FLinearColor::White, Tuning.BurstSeconds * 0.6f);
		bAny = true;
	}
	if (WantsCameraPlaceholder(Cue))
	{
		AddCameraKick(Sign * Tuning.DodgeRoll, 0.f, Tuning.DodgeFOV, 0.25f, 0.45f);
		bAny = true;
	}
	if (bAny) { Note(TEXT("Dodge")); }
}

void UTDFlightFeedbackComponent::HandleDriftReleased(bool bBurst)
{
	ATDShip* Ship = GetShip();
	if (!bBurst || !Ship || !Ship->IsPlayerFlown())
	{
		return;
	}
	const UTDShipVisuals* Visuals = Ship->GetShipVisuals();
	const FTDFXCue* Cue = Visuals ? &Visuals->DriftRelease : nullptr;
	if (WantsVisualPlaceholder(Cue))
	{
		SpawnBurst(FVector(-280.f, 110.f, 0.f), FVector(2.2f, 0.4f, 0.4f), UTDStatics::ShipColor(Ship->GetShipId()), Tuning.BurstSeconds);
		SpawnBurst(FVector(-280.f, -110.f, 0.f), FVector(2.2f, 0.4f, 0.4f), UTDStatics::ShipColor(Ship->GetShipId()), Tuning.BurstSeconds);
	}
	if (WantsCameraPlaceholder(Cue))
	{
		AddCameraKick(0.f, -1.f, 5.f, 0.35f, 0.5f);
	}
	Note(TEXT("DriftRelease"));
}

void UTDFlightFeedbackComponent::HandleTeleported(ATDSpacecraft* Spacecraft, FVector FromLocation, FVector ToLocation, FRotator NewFacing)
{
	ATDShip* Ship = GetShip();
	if (!Ship || !Ship->IsPlayerFlown() || FVector::Dist(FromLocation, ToLocation) < 1000.f)
	{
		return;
	}
	const UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive();
	const UTDLandingComponent* Landing = Ship->GetLanding();
	const UTDAtmosphereEntryComponent* Entry = Ship->GetAtmosphereEntry();
	if ((Hyper && Hyper->GetState() != ETDHyperdriveState::Idle) || (Landing && Landing->IsBusy()) || (Entry && Entry->IsEntering()))
	{
		return; // hyperdrive drop-out, landing snaps and planet entry have their own moments
	}
	// Only a real transit: the ship left from inside a portal gate or one of Rift's portals. Squad placement,
	// arrival on a new map, retakes and dev teleports are cuts of their own and get no kick.
	bool bFromPortal = false;
	for (TActorIterator<AActor> It(GetWorld()); It && !bFromPortal; ++It)
	{
		if (!It->IsA<ATDPortalGate>() && !It->IsA<ATDPortal>())
		{
			continue;
		}
		FVector Origin, Extent;
		It->GetActorBounds(true, Origin, Extent);
		bFromPortal = FBox(Origin - Extent, Origin + Extent).ExpandBy(2500.f).IsInsideOrOn(FromLocation);
	}
	if (!bFromPortal)
	{
		return;
	}
	const UTDTravelFX* FX = UTDTravelSubsystem::GetDefaultTravelFX(this);
	const FTDFXCue* Cue = FX ? &FX->PortalTransit : nullptr;
	if (WantsVisualPlaceholder(Cue))
	{
		SpawnBurst(FVector(150.f, 0.f, 0.f), FVector(7.f, 7.f, 7.f), FLinearColor(0.85f, 0.6f, 1.f), 0.3f);
	}
	if (WantsCameraPlaceholder(Cue))
	{
		AddCameraKick(FMath::RandBool() ? 5.f : -5.f, 0.f, 12.f, 0.8f, 0.6f);
	}
	Note(TEXT("PortalTransit"));
}

void UTDFlightFeedbackComponent::PollMoments(float DeltaTime)
{
	ATDShip* Ship = GetShip();
	UTDShipMovementComponent* Move = Ship ? Ship->GetShipMovement() : nullptr;
	if (!Ship || !Move)
	{
		return;
	}
	const bool bPlayer = Ship->IsPlayerFlown();
	const UTDShipVisuals* Visuals = Ship->GetShipVisuals();
	const UTDTravelFX* FX = UTDTravelSubsystem::GetDefaultTravelFX(this);
	const FLinearColor Color = UTDStatics::ShipColor(Ship->GetShipId());

	// ---- Boost start: punch and an engine flare.
	const bool bBoosting = Move->IsBoosting();
	if (bPlayer && bBoosting && !bWasBoosting)
	{
		const FTDFXCue* Cue = Visuals ? &Visuals->BoostStart : nullptr;
		if (WantsVisualPlaceholder(Cue))
		{
			SpawnBurst(FVector(-300.f, 110.f, 0.f), FVector(2.6f, 0.45f, 0.45f), Color, 0.4f); // twin flares beside the engine:
			SpawnBurst(FVector(-300.f, -110.f, 0.f), FVector(2.6f, 0.45f, 0.45f), Color, 0.4f); // never over the crosshair
		}
		if (WantsCameraPlaceholder(Cue))
		{
			AddCameraKick(0.f, 0.f, 6.f, 0.35f, 0.55f);
		}
		Note(TEXT("BoostStart"));
	}
	bWasBoosting = bBoosting;

	// ---- Brake (no cue slot): retro flares at the nose and a dip.
	const bool bBraking = Ship->GetFlightInput().bBrake && Move->GetSpeed01() > 0.1f;
	if (bPlayer && bBraking && !bWasBraking)
	{
		if (WantsVisualPlaceholder(nullptr))
		{
			SpawnBurst(FVector(160.f, 210.f, 0.f), FVector(1.8f, 0.6f, 0.6f), Color, 0.35f);
			SpawnBurst(FVector(160.f, -210.f, 0.f), FVector(1.8f, 0.6f, 0.6f), Color, 0.35f);
		}
		if (WantsCameraPlaceholder(nullptr))
		{
			AddCameraKick(0.f, -1.5f, -3.f, 0.2f, 0.45f);
		}
		Note(TEXT("Brake"));
	}
	bWasBraking = bBraking;

	// ---- Sustained: drift rumble, hyperdrive spool / tunnel, atmosphere entry.
	float FOV = 0.f;
	float Shake = 0.f;
	if (bPlayer && Move->IsDrifting() && WantsCameraPlaceholder(Visuals ? &Visuals->DriftLoop : nullptr))
	{
		Shake += 0.12f;
	}
	const UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive();
	const ETDHyperdriveState HyperState = Hyper ? Hyper->GetState() : ETDHyperdriveState::Idle;
	if (bPlayer && Hyper)
	{
		if (HyperState == ETDHyperdriveState::Spooling && WantsCameraPlaceholder(FX ? &FX->HyperdriveSpool : nullptr))
		{
			const float C = Hyper->GetCharge01();
			FOV -= 7.f * C;  // the view tightens as the drive winds up...
			Shake += 0.5f * C;
		}
		if (HyperState == ETDHyperdriveState::Jumping && WantsCameraPlaceholder(FX ? &FX->HyperdriveTunnel : nullptr))
		{
			FOV += 10.f;     // ...and opens wide in the tunnel
			Shake += 0.25f;
		}
		if (HyperState != LastHyperState)
		{
			if (HyperState == ETDHyperdriveState::Jumping)
			{
				const FTDFXCue* Cue = FX ? &FX->HyperdriveTunnel : nullptr;
				if (WantsCameraPlaceholder(Cue)) { AddCameraKick(0.f, 0.f, 12.f, 1.f, 0.7f); }
				if (WantsVisualPlaceholder(Cue)) { SpawnBurst(FVector(-350.f, 140.f, 0.f), FVector(4.f, 0.5f, 0.5f), FLinearColor(0.7f, 0.85f, 1.f), 0.5f); SpawnBurst(FVector(-350.f, -140.f, 0.f), FVector(4.f, 0.5f, 0.5f), FLinearColor(0.7f, 0.85f, 1.f), 0.5f); }
				Note(TEXT("HyperdriveJump"));
			}
			else if (HyperState == ETDHyperdriveState::DropOut)
			{
				const FTDFXCue* Cue = FX ? &FX->HyperdriveDropOut : nullptr;
				if (WantsCameraPlaceholder(Cue)) { AddCameraKick(FMath::RandBool() ? 2.f : -2.f, 0.f, -8.f, 1.2f, 0.8f); }
				if (WantsVisualPlaceholder(Cue)) { SpawnBurst(FVector(250.f, 0.f, 0.f), FVector(6.f, 6.f, 6.f), FLinearColor(0.7f, 0.85f, 1.f), 0.3f); }
				Note(TEXT("HyperdriveDropOut"));
			}
		}
	}
	LastHyperState = HyperState;

	const UTDAtmosphereEntryComponent* Entry = Ship->GetAtmosphereEntry();
	if (bPlayer && Entry && Entry->IsEntering() && WantsCameraPlaceholder(FX ? &FX->AtmosphericEntry : nullptr))
	{
		const float Heat = TDFlightFeedback::Bell(Entry->GetEntry01());
		Shake += 0.9f * Heat;
		FOV += 4.f * Heat;
		if (WantsVisualPlaceholder(FX ? &FX->AtmosphericEntry : nullptr) && GetActiveBurstCount() == 0 && Heat > 0.2f)
		{
			// Re-entry glow on the nose, re-lit as each flash fades.
			SpawnBurst(FVector(260.f, 0.f, 0.f), FVector(2.2f, 2.2f, 1.6f) * (0.6f + 0.6f * Heat), FLinearColor(1.f, 0.45f, 0.12f), 0.25f);
			Note(TEXT("AtmosphereEntry"));
		}
	}

	// ---- Landing touchdown / take-off.
	const UTDLandingComponent* Landing = Ship->GetLanding();
	const ETDLandingState LandState = Landing ? Landing->GetLandingState() : ETDLandingState::Flying;
	if (bPlayer && LandState != LastLandingState)
	{
		if (LandState == ETDLandingState::Touchdown)
		{
			const FTDFXCue* Cue = FX ? &FX->LandingDust : nullptr;
			if (WantsCameraPlaceholder(Cue)) { AddCameraKick(0.f, -2.f, 0.f, 0.6f, 0.5f); }
			if (WantsVisualPlaceholder(Cue)) { SpawnBurst(FVector(0.f, 0.f, -160.f), FVector(5.f, 5.f, 0.4f), FLinearColor(0.9f, 0.85f, 0.75f), 0.5f); }
			Note(TEXT("Touchdown"));
		}
		else if (LandState == ETDLandingState::Ascending)
		{
			const FTDFXCue* Cue = FX ? &FX->TakeOff : nullptr;
			if (WantsCameraPlaceholder(Cue)) { AddCameraKick(0.f, 1.f, 3.f, 0.5f, 0.7f); }
			if (WantsVisualPlaceholder(Cue)) { SpawnBurst(FVector(-60.f, 0.f, -230.f), FVector(3.2f, 3.2f, 0.2f), Color, 0.5f); }
			Note(TEXT("TakeOff"));
		}
	}
	LastLandingState = LandState;

	SustainedFOV = FMath::FInterpTo(SustainedFOV, FOV, DeltaTime, 6.f);
	SustainedShake = FMath::FInterpTo(SustainedShake, Shake, DeltaTime, 8.f);
}

void UTDFlightFeedbackComponent::UpdateCamera(float DeltaTime)
{
	float Roll = 0.f, Pitch = 0.f, FOV = 0.f, Shake = SustainedShake;
	for (int32 i = Kicks.Num() - 1; i >= 0; --i)
	{
		FKick& K = Kicks[i];
		K.Age += DeltaTime;
		if (K.Age >= K.Seconds)
		{
			Kicks.RemoveAtSwap(i);
			continue;
		}
		const float E = TDFlightFeedback::Envelope(K.Age, K.Seconds);
		Roll += K.Roll * E;
		Pitch += K.Pitch * E;
		FOV += K.FOV * E;
		Shake += K.Shake * E;
	}
	ShakeTime += DeltaTime;
	const float S = Shake * Tuning.CameraScale;
	const float T = ShakeTime;
	// Cheap procedural shake: incommensurate sines per axis (no camera-shake asset needed).
	const float JPitch = S * (FMath::Sin(T * 41.f) + 0.5f * FMath::Sin(T * 73.f + 1.3f)) / 1.5f;
	const float JYaw = S * (FMath::Sin(T * 37.f + 0.7f) + 0.5f * FMath::Sin(T * 89.f + 2.1f)) / 1.5f;
	const float JRoll = S * 0.6f * FMath::Sin(T * 29.f + 0.4f);
	CameraOffset = FRotator(Pitch * Tuning.CameraScale + JPitch, JYaw, Roll * Tuning.CameraScale + JRoll);
	FOVOffset = (FOV + SustainedFOV) * Tuning.CameraScale;
}

void UTDFlightFeedbackComponent::UpdateBursts(float DeltaTime)
{
	for (FBurst& B : Bursts)
	{
		if (B.Age >= B.Seconds)
		{
			continue;
		}
		B.Age += DeltaTime;
		if (B.Age >= B.Seconds)
		{
			if (B.Mesh) { B.Mesh->SetVisibility(false); }
			continue;
		}
		const float T = B.Age / B.Seconds;
		const float Fade = FMath::Square(1.f - T);
		if (B.Mesh) { B.Mesh->SetRelativeScale3D(B.Scale * (0.55f + 0.45f * (1.f - T))); }
		if (B.MID) { B.MID->SetVectorParameterValue(TEXT("Color"), B.Color * (Tuning.BurstGlow * Fade)); }
	}
}

void UTDFlightFeedbackComponent::EnsureDust()
{
	if (Dust)
	{
		return;
	}
	ATDShip* Ship = GetShip();
	UStaticMesh* Cube = TDFlightFeedback::LoadShape(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Ship || !Cube || !Ship->GetRootComponent())
	{
		return;
	}
	Dust = NewObject<UInstancedStaticMeshComponent>(Ship, TEXT("TDSpeedDust"), RF_Transient);
	Dust->SetStaticMesh(Cube);
	Dust->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Dust->SetGenerateOverlapEvents(false);
	Dust->SetCastShadow(false);
	Dust->SetCanEverAffectNavigation(false);
	Dust->CanCharacterStepUpOn = ECB_No;
	Dust->SetupAttachment(Ship->GetRootComponent());
	Dust->SetUsingAbsoluteLocation(true);
	Dust->SetUsingAbsoluteRotation(true);
	Dust->SetUsingAbsoluteScale(true);
	Dust->RegisterComponent();
	Dust->SetWorldTransform(FTransform::Identity); // instance transforms are then world transforms
	UTDStatics::TintPlaceholder(Dust, FLinearColor(0.75f, 0.85f, 1.f), Tuning.DustGlow);

	FRandomStream Rand(0x7D5D);
	const FVector Centre = Ship->GetActorLocation() + Ship->GetActorForwardVector() * Tuning.DustAhead;
	const float H = Tuning.DustHalfExtent;
	DustPositions.SetNum(Tuning.DustCount);
	DustTransforms.SetNum(Tuning.DustCount);
	for (int32 i = 0; i < DustPositions.Num(); ++i)
	{
		DustPositions[i] = Centre + FVector(Rand.FRandRange(-H, H), Rand.FRandRange(-H, H), Rand.FRandRange(-H, H));
		DustTransforms[i] = FTransform(FQuat::Identity, DustPositions[i], FVector(0.01f));
	}
	Dust->AddInstances(DustTransforms, false, true);
	Dust->SetVisibility(false);
}

void UTDFlightFeedbackComponent::UpdateDust(float DeltaTime)
{
	ATDShip* Ship = GetShip();
	UTDShipMovementComponent* Move = Ship ? Ship->GetShipMovement() : nullptr;
	const UTDLandingComponent* Landing = Ship ? Ship->GetLanding() : nullptr;
	const bool bWanted = Ship && Move && Tuning.bSpeedDust && Ship->IsPlayerFlown() && !Ship->IsHidden()
		&& TDFlightFeedback::CVarSpeedDust.GetValueOnGameThread() != 0 && !(Landing && Landing->IsLanded());
	const float Speed = Move ? Move->Velocity.Size() : 0.f;
	const float Thick01 = FMath::Clamp((Speed - Tuning.DustMinSpeed) / FMath::Max(1.f, Tuning.DustFullSpeed - Tuning.DustMinSpeed), 0.f, 1.f);
	const bool bShow = bWanted && Thick01 > 0.f;
	if (!bShow)
	{
		if (Dust && bDustVisible) { Dust->SetVisibility(false); }
		bDustVisible = false;
		return;
	}
	EnsureDust();
	if (!Dust)
	{
		return;
	}
	const FVector Centre = Ship->GetActorLocation() + Ship->GetActorForwardVector() * Tuning.DustAhead;
	const float H = Tuning.DustHalfExtent;
	const float Span = 2.f * H;
	const float Length = FMath::Clamp(Speed * Tuning.DustStreakSeconds, 20.f, Tuning.DustStreakMax);
	const float Thick = Tuning.DustThickness * FMath::Sqrt(Thick01);
	const FQuat Facing = FRotationMatrix::MakeFromX(Move->Velocity.GetSafeNormal()).ToQuat();
	const APlayerController* PC = Cast<APlayerController>(Ship->GetController());
	const FVector Eye = (PC && PC->PlayerCameraManager) ? PC->PlayerCameraManager->GetCameraLocation() : Ship->GetActorLocation();
	const float Clear = FMath::Max(1.f, Tuning.DustCameraClear);
	for (int32 i = 0; i < DustPositions.Num(); ++i)
	{
		// The motes stay still in the world; the box follows the ship and wraps them around.
		FVector Offset = DustPositions[i] - Centre;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			double& C = Offset[Axis];
			if (C > H || C < -H)
			{
				C = FMath::Fmod(C + H, (double)Span);
				if (C < 0.0) { C += Span; }
				C -= H;
			}
		}
		DustPositions[i] = Centre + Offset;
		const float Near = FMath::Clamp(FVector::Dist(DustPositions[i], Eye) / Clear - 0.25f, 0.f, 1.f);
		const float T = FMath::Max(0.01f, Thick * Near) / 100.f;
		DustTransforms[i] = FTransform(Facing, DustPositions[i], FVector(FMath::Max(0.01f, Length * Near) / 100.f, T, T));
	}
	Dust->BatchUpdateInstancesTransforms(0, DustTransforms, false, false, true); // incremental: never re-create the render state per frame
	if (!bDustVisible) { Dust->SetVisibility(true); }
	bDustVisible = true;
}

void UTDFlightFeedbackComponent::UpdateTrail(float DeltaTime)
{
	ATDShip* Ship = GetShip();
	UTDShipMovementComponent* Move = Ship ? Ship->GetShipMovement() : nullptr;
	const UTDShipVisuals* Visuals = Ship ? Ship->GetShipVisuals() : nullptr;
	const bool bWanted = Ship && Move && Tuning.bEngineTrail && !Ship->IsHidden() && Ship->AreEnginesRunning()
		&& WantsVisualPlaceholder(Visuals ? &Visuals->EngineTrail : nullptr) && Move->Velocity.Size() > 500.f;
	auto Hide = [this]()
	{
		if (Trail && bTrailVisible) { Trail->SetVisibility(false); }
		bTrailVisible = false;
		TrailHistory.Reset();
		TrailLength = 0.f;
	};
	if (!bWanted)
	{
		Hide();
		return;
	}
	Ship->GetEngineExhaustLocations(TrailNozzles);
	const int32 Engines = TrailNozzles.Num();
	const int32 N = FMath::Max(2, Tuning.TrailSegments);
	if (!Trail || TrailTransforms.Num() != N * Engines)
	{
		if (!Trail)
		{
			UStaticMesh* Cube = TDFlightFeedback::LoadShape(TEXT("/Engine/BasicShapes/Cube.Cube"));
			if (!Cube || !Ship->GetRootComponent())
			{
				return;
			}
			Trail = NewObject<UInstancedStaticMeshComponent>(Ship, TEXT("TDEngineTrail"), RF_Transient);
			Trail->SetStaticMesh(Cube);
			Trail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Trail->SetGenerateOverlapEvents(false);
			Trail->SetCastShadow(false);
			Trail->SetCanEverAffectNavigation(false);
			Trail->CanCharacterStepUpOn = ECB_No;
			Trail->SetupAttachment(Ship->GetRootComponent());
			Trail->SetUsingAbsoluteLocation(true);
			Trail->SetUsingAbsoluteRotation(true);
			Trail->SetUsingAbsoluteScale(true);
			Trail->RegisterComponent();
			Trail->SetWorldTransform(FTransform::Identity);
			TrailMID = UTDStatics::TintPlaceholder(Trail, UTDStatics::ShipColor(Ship->GetShipId()), Tuning.TrailGlow);
			TrailMIDGlow = Tuning.TrailGlow;
		}
		Trail->ClearInstances();
		TrailTransforms.SetNum(N * Engines);
		for (FTransform& T : TrailTransforms) { T = FTransform(FQuat::Identity, Ship->GetActorLocation(), FVector(0.01f)); }
		Trail->AddInstances(TrailTransforms, false, true);
		TrailHistory.Reset();
	}

	// The flown ship's short flame burns dimmer than a wingman's streak (it sits right under the camera).
	const float WantGlow = (Ship->IsPlayerFlown() && !Ship->IsCockpitView()) ? Tuning.PlayerFlameGlow : Tuning.TrailGlow;
	if (TrailMID && !FMath::IsNearlyEqual(WantGlow, TrailMIDGlow))
	{
		TrailMID->SetVectorParameterValue(TEXT("Color"), UTDStatics::ShipColor(Ship->GetShipId()) * WantGlow);
		TrailMIDGlow = WantGlow;
	}

	// Path samples per engine: [0] follows the nozzle every frame; a new sample is pushed every TrailSampleSeconds.
	if (TrailHistory.Num() != Engines)
	{
		TrailHistory.SetNum(Engines);
		for (int32 e = 0; e < Engines; ++e) { TrailHistory[e].Init(TrailNozzles[e], N + 1); }
	}
	TrailClock += DeltaTime;
	const bool bSample = TrailClock >= Tuning.TrailSampleSeconds;
	if (bSample) { TrailClock = 0.f; }
	// A teleport (portal, hyperdrive drop-out, respawn) never draws a streak across space.
	const float MaxStep = FMath::Max(5000.f, (float)Move->Velocity.Size() * Tuning.TrailSampleSeconds * 3.f);
	// Several engines: thinner ribbons, so a five-engine Bastion doesn't drag a wall of light.
	const float Thick = (Ship->IsPlayerFlown() && !Ship->IsCockpitView() ? Tuning.PlayerFlameThickness : Tuning.TrailThickness)
		* (Ship->IsBoosting() ? 1.25f : 1.f) * (Engines > 1 ? 0.75f : 1.f);
	// Near the camera a ribbon would smear across the view (the chase camera sits right above the player's own
	// trail): segments shrink to nothing as they come within DustCameraClear of the eye, as the dust does.
	const APlayerController* ViewPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const bool bHaveEye = ViewPC && ViewPC->PlayerCameraManager;
	const FVector Eye = bHaveEye ? ViewPC->PlayerCameraManager->GetCameraLocation() : FVector::ZeroVector;
	const float Clear = FMath::Max(1.f, Tuning.DustCameraClear);
	// The flown ship in the chase view: the camera sits ~14 m behind, right above its own exhaust, so the ribbon
	// stops PlayerTrailMax behind the nozzle (tapering over that length) instead of streaming under the camera.
	const bool bOwnChase = Ship->IsPlayerFlown() && !Ship->IsCockpitView();
	const float Cap = bOwnChase ? Tuning.PlayerTrailMax : TNumericLimits<float>::Max();
	float FirstLength = 0.f;
	for (int32 e = 0; e < Engines; ++e)
	{
		TArray<FVector>& H = TrailHistory[e];
		H[0] = TrailNozzles[e];
		if (bSample)
		{
			H.Insert(TrailNozzles[e], 0);
			H.SetNum(N + 1);
		}
		float Length = 0.f;
		for (int32 i = 0; i < N; ++i)
		{
			const FVector A = H[i];
			FVector B = H[i + 1];
			FVector D = A - B;
			float Len = D.Size();
			if (Len > MaxStep)
			{
				Hide();
				return;
			}
			if (Length + Len > Cap)
			{
				// Cut this segment at the cap; the rest of the ribbon draws nothing.
				const float Keep = FMath::Max(0.f, Cap - Length);
				B = Len > KINDA_SMALL_NUMBER ? A - D * (Keep / Len) : A;
				D = A - B;
				Len = Keep;
			}
			// Quadratic taper: thin quickly, then a long hairline tail that reads as fading out.
			const float Lin = bOwnChase ? FMath::Clamp(1.f - Length / FMath::Max(Cap, 1.f), 0.f, 1.f) : 1.f - (float)i / (float)N;
			const float Taper = Lin * Lin;
			const float Near = bHaveEye ? FMath::Clamp(FVector::Dist((A + B) * 0.5f, Eye) / Clear - 0.25f, 0.f, 1.f) : 1.f;
			const float T = FMath::Max(0.01f, Thick * Taper * Near / 100.f);
			const FQuat Q = Len > KINDA_SMALL_NUMBER ? FRotationMatrix::MakeFromX(D / Len).ToQuat() : Ship->GetActorQuat();
			TrailTransforms[e * N + i] = FTransform(Q, (A + B) * 0.5f, FVector(FMath::Max(0.01f, Len / 100.f), T, T));
			Length += Len;
		}
		if (e == 0) { FirstLength = Length; }
	}
	TrailLength = FirstLength;
	Trail->BatchUpdateInstancesTransforms(0, TrailTransforms, false, false, true);
	if (!bTrailVisible) { Trail->SetVisibility(true); }
	bTrailVisible = true;
}

void UTDFlightFeedbackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const double Start = FPlatformTime::Seconds();
	PollMoments(DeltaTime);
	UpdateCamera(DeltaTime);
	UpdateBursts(DeltaTime);
	UpdateDust(DeltaTime);
	UpdateTrail(DeltaTime);
	const float Ms = float((FPlatformTime::Seconds() - Start) * 1000.0);
	const float Alpha = FMath::Clamp(DeltaTime / 2.f, 0.f, 1.f);
	AverageTickMs = AverageTickMs <= 0.f ? Ms : FMath::Lerp(AverageTickMs, Ms, Alpha);
}

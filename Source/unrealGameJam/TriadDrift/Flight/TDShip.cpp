// Triad Drift - a squad ship with the chase camera (ENG-01). Owner: Teammate 1 (Flight & Input).

#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Combat/TDTargetingComponent.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/Flight/TDFlightFeedbackComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/SpringArmComponent.h"

namespace TDShipCamera
{
	/** Signed angle from A to B around Axis (deg), both assumed perpendicular to Axis. */
	float SignedAngle(const FVector& A, const FVector& B, const FVector& Axis)
	{
		return FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(FVector::CrossProduct(A, B), Axis), FVector::DotProduct(A, B)));
	}

	FVector Rotate(const FVector& V, const FVector& Axis, float Deg)
	{
		return FQuat(Axis, FMath::DegreesToRadians(Deg)).RotateVector(V);
	}

	/** Perpendicular part of V against unit Axis, normalised (zero if V is parallel). */
	FVector Perp(const FVector& V, const FVector& Axis)
	{
		return (V - Axis * FVector::DotProduct(V, Axis)).GetSafeNormal();
	}

	/** Continuous through inverted flight: past 90 deg of bank the follow eases back to 0 at 180. */
	float FoldBank(float Deg)
	{
		Deg = FRotator::NormalizeAxis(Deg);
		return FMath::Abs(Deg) > 90.f ? FMath::Sign(Deg) * (180.f - FMath::Abs(Deg)) : Deg;
	}
}

ATDShip::ATDShip()
{
	Team = ETDTeam::Player;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ChaseArm"));
	SpringArm->SetupAttachment(GetRootComponent());
	SpringArm->SetUsingAbsoluteRotation(true); // we drive its rotation (partial roll follow, look-ahead)
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bDoCollisionTest = false;       // open space; rocks are handled by the flight model
	SpringArm->bEnableCameraLag = true;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraLagMaxDistance = CameraTuning.LagMaxDistance;

	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	ChaseCamera->bUsePawnControlRotation = false;
	ChaseCamera->SetAutoActivate(false);

	Targeting = CreateDefaultSubobject<UTDTargetingComponent>(TEXT("Targeting"));
	Hyperdrive = CreateDefaultSubobject<UTDHyperdriveComponent>(TEXT("Hyperdrive"));
	Landing = CreateDefaultSubobject<UTDLandingComponent>(TEXT("Landing"));
	AtmosphereEntry = CreateDefaultSubobject<UTDAtmosphereEntryComponent>(TEXT("AtmosphereEntry"));
	FlightFeedback = CreateDefaultSubobject<UTDFlightFeedbackComponent>(TEXT("FlightFeedback"));

	CockpitCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("CockpitCamera"));
	CockpitCamera->SetupAttachment(HullPivot);
	CockpitCamera->SetRelativeLocation(CockpitOffset);
	CockpitCamera->bUsePawnControlRotation = false;
	CockpitCamera->SetAutoActivate(false);

	bFindCameraComponentWhenViewTarget = true;
}

void ATDShip::BeginPlay()
{
	Super::BeginPlay();
	OnTeleported.AddDynamic(this, &ATDShip::HandleTeleported);
	if (ShipMovement)
	{
		ShipMovement->OnBoostStarted.AddDynamic(this, &ATDShip::HandleBoostStarted);
		ShipMovement->OnBoostEnded.AddDynamic(this, &ATDShip::HandleBoostEnded);
		ShipMovement->OnDriftStarted.AddDynamic(this, &ATDShip::HandleDriftStarted);
		ShipMovement->OnDriftReleased.AddDynamic(this, &ATDShip::HandleDriftEnded);
		ShipMovement->OnFlightImpact.AddDynamic(this, &ATDShip::HandleFlightImpactCue);
	}

	SpringArm->TargetArmLength = CameraTuning.ArmLength;
	SpringArm->SocketOffset = FVector(0.f, 0.f, CameraTuning.Height);
	SpringArm->CameraLagSpeed = CameraTuning.LocationLag;
	SpringArm->CameraRotationLagSpeed = CameraTuning.RotationLag;
	SpringArm->CameraLagMaxDistance = CameraTuning.LagMaxDistance;
	CurrentFOV = CameraTuning.MinFOV;
	for (UCameraComponent* Cam : { ChaseCamera.Get(), CockpitCamera.Get() })
	{
		if (Cam)
		{
			Cam->PostProcessSettings.bOverride_MotionBlurAmount = true;
			Cam->PostProcessSettings.MotionBlurAmount = CameraTuning.MotionBlurAmount;
			Cam->PostProcessSettings.bOverride_MotionBlurMax = true;
			Cam->PostProcessSettings.MotionBlurMax = CameraTuning.MotionBlurMax;
			Cam->PostProcessBlendWeight = 1.f;
		}
	}
	const float FOVKick = FlightFeedback ? FlightFeedback->GetFOVOffset() : 0.f;
	ChaseCamera->SetFieldOfView(FMath::Clamp(CurrentFOV + FOVKick, 5.f, 170.f));

	SetPlayerControlled(bPlayerControlled);
	UpdateChaseCamera(0.f, true);
}

void ATDShip::SetShipId(ETDShipId NewId)
{
	ShipId = NewId;
	if (HasActorBegunPlay())
	{
		ApplyPlaceholderLook();
	}
}

void ATDShip::SetShipVisuals(UTDShipVisuals* InVisuals)
{
	Visuals = InVisuals;
	if (HasActorBegunPlay())
	{
		ApplyPlaceholderLook();
	}
}

FLinearColor ATDShip::GetPlaceholderColor() const
{
	return Team == ETDTeam::Player ? UTDStatics::ShipColor(ShipId) : UTDStatics::TeamColor(Team);
}

void ATDShip::ApplyPlaceholderLook()
{
	RebuildVisuals();
	if (!bHasRealArt)
	{
		Super::ApplyPlaceholderLook(); // placeholder tint in the squad colour
	}
}

UStaticMeshComponent* ATDShip::FindVisualPart(FName PartName) const
{
	const int32 Index = VisualPartNames.IndexOfByKey(PartName);
	return (Index != INDEX_NONE && VisualParts.IsValidIndex(Index)) ? VisualParts[Index].Get() : nullptr;
}

void ATDShip::ClearVisualParts()
{
	for (TObjectPtr<UStaticMeshComponent>& Part : VisualParts)
	{
		if (Part)
		{
			Part->DestroyComponent();
		}
	}
	VisualParts.Reset();
	VisualPartNames.Reset();
}

void ATDShip::RebuildVisuals()
{
	// Start from the placeholder every time, so re-assigning or clearing visuals is clean.
	ClearVisualParts();
	RestorePlaceholderHull();
	bHasRealArt = false;

	if (Visuals && HullMesh)
	{
		USceneComponent* Primary = nullptr; // sockets are looked up on the primary mesh
		if (!Visuals->Mesh.IsNull())
		{
			if (UStaticMesh* Mesh = Visuals->Mesh.LoadSynchronous())
			{
				HullMesh->SetStaticMesh(Mesh);
				HullMesh->SetRelativeRotation(FRotator::ZeroRotator); // exported +X forward (blender-unreal-export)
				HullMesh->SetRelativeScale3D(FVector::OneVector);
				for (int32 i = 0; i < Visuals->MaterialOverrides.Num(); ++i)
				{
					if (UMaterialInterface* Mat = Visuals->MaterialOverrides[i].LoadSynchronous())
					{
						HullMesh->SetMaterial(i, Mat);
					}
				}
				Primary = HullMesh;
				bHasRealArt = true;
			}
		}
		if (!Primary && Visuals->Parts.Num() > 0)
		{
			HullMesh->SetVisibility(false); // Parts only: the first part is the fuselage
		}

		for (const FTDShipVisualPart& Spec : Visuals->Parts)
		{
			UStaticMesh* PartMesh = Spec.Mesh.IsNull() ? nullptr : Spec.Mesh.LoadSynchronous();
			if (!PartMesh)
			{
				continue;
			}
			const FName CompName = MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(),
				*FString::Printf(TEXT("VisualPart_%s"), *Spec.PartName.ToString()));
			UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, CompName, RF_Transient);
			Part->SetStaticMesh(PartMesh);
			Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Part->SetCollisionResponseToAllChannels(ECR_Ignore);
			Part->SetGenerateOverlapEvents(false);
			Part->SetCanEverAffectNavigation(false);
			Part->CanCharacterStepUpOn = ECB_No;
			Part->PrimaryComponentTick.bCanEverTick = false;
			Part->SetComponentTickEnabled(false);

			const UStaticMeshComponent* PrimaryMesh = Cast<UStaticMeshComponent>(Primary);
			if (Spec.ParentSocket != NAME_None && PrimaryMesh && PrimaryMesh->DoesSocketExist(Spec.ParentSocket))
			{
				Part->SetupAttachment(Primary, Spec.ParentSocket);
			}
			else
			{
				if (Spec.ParentSocket != NAME_None)
				{
					UE_LOG(LogTriadDrift, Warning, TEXT("%s: visual part %s wants socket %s, which the primary mesh lacks; attaching to the ship root."),
						*GetName(), *Spec.PartName.ToString(), *Spec.ParentSocket.ToString());
				}
				Part->SetupAttachment(HullPivot); // ship root (banks with the hull)
			}
			Part->SetRelativeTransform(Spec.RelativeTransform);
			for (int32 i = 0; i < Spec.MaterialOverrides.Num(); ++i)
			{
				if (UMaterialInterface* Mat = Spec.MaterialOverrides[i].LoadSynchronous())
				{
					Part->SetMaterial(i, Mat);
				}
			}
			Part->RegisterComponent();
			VisualParts.Add(Part);
			VisualPartNames.Add(Spec.PartName);
			if (!Primary)
			{
				Primary = Part; // Parts only: later parts' sockets are on the fuselage part
			}
			bHasRealArt = true;
		}
		if (!bHasRealArt)
		{
			HullMesh->SetVisibility(true); // nothing loaded after all
		}
	}
	SetPlaceholderPartsVisible(!bHasRealArt);
	StartEngineTrail();
}

// ------------------------------------------------------------------------------- presentation cues

USceneComponent* ATDShip::GetEngineAttach(FName& OutSocket) const
{
	OutSocket = NAME_None;
	if (UStaticMeshComponent* EnginePart = FindVisualPart(TEXT("Engine")))
	{
		return EnginePart;
	}
	if (HullMesh && HullMesh->DoesSocketExist(TEXT("Engine")))
	{
		OutSocket = TEXT("Engine");
		return HullMesh;
	}
	return bHasRealArt ? static_cast<USceneComponent*>(HullMesh) : static_cast<USceneComponent*>(EngineGlow);
}

FTDFXCue ATDShip::ForThisShip(const FTDFXCue& Cue) const
{
	FTDFXCue Out = Cue;
	if (!bPlayerControlled)
	{
		Out.Shake.Reset();
	}
	return Out;
}

void ATDShip::StopLoop(FName Slot, TWeakObjectPtr<UNiagaraComponent>& FX, TWeakObjectPtr<UAudioComponent>& Audio)
{
	if (UNiagaraComponent* N = FX.Get()) { N->Deactivate(); }
	if (UAudioComponent* A = Audio.Get()) { A->FadeOut(0.25f, 0.f); }
	FX.Reset();
	Audio.Reset();
	ActiveLoops.Remove(Slot);
}

void ATDShip::StartLoop(FName Slot, const FTDFXCue& Cue, TWeakObjectPtr<UNiagaraComponent>& OutFX, TWeakObjectPtr<UAudioComponent>& OutAudio)
{
	StopLoop(Slot, OutFX, OutAudio);
	if (!Visuals)
	{
		return;
	}
	NoteCue(Slot);
	ActiveLoops.Add(Slot);
	FName Socket;
	USceneComponent* Attach = GetEngineAttach(Socket);
	UAudioComponent* Audio = nullptr;
	OutFX = UTDStatics::PlayCueAttachedLoop(ForThisShip(Cue), Attach, Socket, Audio);
	OutAudio = Audio;
}

bool ATDShip::ShouldEnginesRun() const
{
	if (!Visuals)
	{
		return false;
	}
	if (const UTDHealthComponent* H = GetHealth())
	{
		if (H->IsDead())
		{
			return false;
		}
	}
	if (Landing && Landing->IsBusy())
	{
		return Landing->AreEnginesOn(); // gear / canopy steps keep the engines lit until EnginesOff
	}
	return !IsFlightParked();
}

const FName ATDShip::AudioParamSpeed01(TEXT("Speed01"));
const FName ATDShip::AudioParamBoost01(TEXT("Boost01"));
const FName ATDShip::AudioParamDrift01(TEXT("Drift01"));
TFunction<UAudioComponent*(USoundBase*, USceneComponent*, FName)> ATDShip::TestSpawnEngineLoop;

void ATDShip::ApplyEngineAudioParams(UAudioComponent* Audio) const
{
	if (!Audio)
	{
		return;
	}
	const float Speed01 = GetSpeed01();
	Audio->SetFloatParameter(AudioParamSpeed01, Speed01);
	Audio->SetFloatParameter(AudioParamBoost01, AudioBoost01);
	Audio->SetFloatParameter(AudioParamDrift01, AudioDrift01);
	// A MetaSound reads Speed01 itself; pitching it here as well would pitch it twice. Found by name so the
	// module needs no MetasoundEngine dependency.
	static const UClass* MetaSoundClass = FindObject<UClass>(nullptr, TEXT("/Script/MetasoundEngine.MetaSoundSource"));
	const USoundBase* Sound = Audio->GetSound();
	if (!(MetaSoundClass && Sound && Sound->IsA(MetaSoundClass)))
	{
		Audio->SetPitchMultiplier(0.8f + 0.6f * Speed01);
	}
}

void ATDShip::StopEngine()
{
	StopLoop(TEXT("EngineTrail"), EngineTrailFX, EngineTrailAudio);
	if (UAudioComponent* A = EngineLoopAudio.Get()) { A->FadeOut(0.3f, 0.f); }
	EngineLoopAudio.Reset();
	ActiveLoops.Remove(TEXT("EngineLoop"));
}

void ATDShip::StartEngineTrail()
{
	StopEngine();
	if (!ShouldEnginesRun())
	{
		return;
	}
	StartLoop(TEXT("EngineTrail"), Visuals->EngineTrail, EngineTrailFX, EngineTrailAudio);
	// The engine hum (UTDShipVisuals::EngineLoop): pitched by speed in Tick.
	NoteCue(TEXT("EngineLoop"));
	ActiveLoops.Add(TEXT("EngineLoop"));
	if (!Visuals->EngineLoop.IsNull())
	{
		if (USoundBase* Sound = Visuals->EngineLoop.LoadSynchronous())
		{
			FName Socket;
			USceneComponent* Attach = GetEngineAttach(Socket);
			EngineLoopAudio = TestSpawnEngineLoop ? TestSpawnEngineLoop(Sound, Attach, Socket) : UGameplayStatics::SpawnSoundAttached(Sound, Attach, Socket);
			ApplyEngineAudioParams(EngineLoopAudio.Get());
		}
	}
}

void ATDShip::HandleBoostStarted()
{
	if (!Visuals)
	{
		return;
	}
	NoteCue(TEXT("BoostStart"));
	FName Socket;
	UTDStatics::PlayCueAttached(ForThisShip(Visuals->BoostStart), GetEngineAttach(Socket), Socket);
	StartLoop(TEXT("BoostLoop"), Visuals->BoostLoop, BoostLoopFX, BoostLoopAudio);
}

void ATDShip::HandleBoostEnded()
{
	StopLoop(TEXT("BoostLoop"), BoostLoopFX, BoostLoopAudio);
}

void ATDShip::HandleDriftStarted()
{
	if (Visuals)
	{
		StartLoop(TEXT("DriftLoop"), Visuals->DriftLoop, DriftLoopFX, DriftLoopAudio);
	}
}

void ATDShip::HandleDriftEnded(bool bBurst)
{
	StopLoop(TEXT("DriftLoop"), DriftLoopFX, DriftLoopAudio);
	if (Visuals && bBurst)
	{
		NoteCue(TEXT("DriftRelease"));
		FName Socket;
		UTDStatics::PlayCueAttached(ForThisShip(Visuals->DriftRelease), GetEngineAttach(Socket), Socket);
	}
}

void ATDShip::HandleFlightImpactCue(const FTDFlightImpact& Impact)
{
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Visuals && !Impact.bHeadOn && Now - LastScrapeCueTime >= ScrapeCueInterval)
	{
		LastScrapeCueTime = Now;
		NoteCue(TEXT("CollisionScrape"));
		UTDStatics::PlayCue(this, ForThisShip(Visuals->CollisionScrape), Impact.ImpactPoint, Impact.ImpactNormal.Rotation());
	}
}

void ATDShip::SetPlayerControlled(bool bInPlayerControlled)
{
	const bool bWasControlled = bPlayerControlled;
	if (!bInPlayerControlled && bWasControlled && Hyperdrive)
	{
		// Before dropping player-flown, so the Idle broadcast still goes out on the event bus.
		Hyperdrive->EndForPilotLoss();
	}
	bPlayerControlled = bInPlayerControlled;
	ApplyCameraActivation();
	if (bInPlayerControlled && !bWasControlled)
	{
		SnapCamera(); // a switch is a hard cut (Director's Cut hook #1): no lag swoop from the old ship
	}
}

bool ATDShip::TryDodge(float Direction)
{
	if (!bPlayerControlled || !GetShipMovement() || FMath::IsNearlyZero(Direction))
	{
		return false;
	}
	if (Hyperdrive && Hyperdrive->GetState() != ETDHyperdriveState::Idle)
	{
		return false;
	}
	if ((Landing && Landing->IsBusy()) || (AtmosphereEntry && AtmosphereEntry->IsEntering()))
	{
		return false;
	}
	if (const UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(this))
	{
		if (Feel->IsSlowMoActive())
		{
			return false; // a director money shot is playing
		}
	}
	if (!GetShipMovement()->RequestDodge(Direction))
	{
		return false;
	}
	NoteCue(TEXT("Dodge"));
	UE_LOG(LogTriadDrift, Log, TEXT("TD dodge: %s %s"), *GetName(), Direction > 0.f ? TEXT("right") : TEXT("left")); // agent playtests count these
	if (Visuals)
	{
		FName Socket;
		UTDStatics::PlayCueAttached(ForThisShip(Visuals->Dodge), GetEngineAttach(Socket), Socket);
	}
	if (FlightFeedback)
	{
		FlightFeedback->PlayDodge(Direction); // placeholder burst / camera kick while the Dodge cue has no art
	}
	return true;
}

bool ATDShip::IsSwitchLocked(FText& OutReason) const
{
	if (Hyperdrive && Hyperdrive->GetState() != ETDHyperdriveState::Idle)
	{
		OutReason = NSLOCTEXT("TDShip", "SwitchLockHyperdrive", "Hyperdrive engaged");
		return true;
	}
	if (Landing && Landing->IsBusy())
	{
		OutReason = Landing->IsLanded()
			? NSLOCTEXT("TDShip", "SwitchLockLanded", "Landed")
			: NSLOCTEXT("TDShip", "SwitchLockLanding", "Landing in progress");
		return true;
	}
	if (AtmosphereEntry && AtmosphereEntry->IsEntering())
	{
		OutReason = NSLOCTEXT("TDShip", "SwitchLockEntry", "Entering atmosphere");
		return true;
	}
	OutReason = FText::GetEmpty();
	return false;
}

void ATDShip::SetCockpitView(bool bCockpit)
{
	bCockpitView = bCockpit;
	ApplyCameraActivation();
	if (!bCockpit)
	{
		SnapCamera();
	}
}

void ATDShip::ApplyCameraActivation()
{
	const bool bChase = bPlayerControlled && !bCockpitView;
	const bool bCockpit = bPlayerControlled && bCockpitView;
	if (ChaseCamera)
	{
		ChaseCamera->SetActive(bChase);
	}
	if (SpringArm)
	{
		SpringArm->SetComponentTickEnabled(bChase);
	}
	if (CockpitCamera)
	{
		CockpitCamera->SetRelativeLocation(CockpitOffset);
		CockpitCamera->SetFieldOfView(CockpitFOV);
		CockpitCamera->SetActive(bCockpit);
	}
	// The pilot doesn't see their own placeholder hull from inside it.
	if (HullMesh) { HullMesh->SetOwnerNoSee(bCockpit); }
	if (PlaceholderWings) { PlaceholderWings->SetOwnerNoSee(bCockpit); }
	for (UStaticMeshComponent* Part : VisualParts)
	{
		if (Part) { Part->SetOwnerNoSee(bCockpit); } // modular art (Nick's parts) too
	}
}

void ATDShip::SnapCamera()
{
	if (!SpringArm)
	{
		return;
	}
	UpdateChaseCamera(0.f, true);
	// Lag stays off until the arm has evaluated once without it (see Tick).
	SpringArm->bEnableCameraLag = false;
	SpringArm->bEnableCameraRotationLag = false;
	SnapFramesRemaining = 2;
}

void ATDShip::HandleTeleported(ATDSpacecraft* Spacecraft, FVector FromLocation, FVector ToLocation, FRotator NewFacing)
{
	if (bPlayerControlled)
	{
		SnapCamera();
	}
}

void ATDShip::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bPlayerControlled && !bCockpitView)
	{
		UpdateChaseCamera(DeltaSeconds, false);
	}

	// Engine loops follow the ship's state: off while dead, parked (downed, hidden) or with engines off.
	const bool bRun = ShouldEnginesRun();
	if (bRun && !ActiveLoops.Contains(TEXT("EngineTrail")))
	{
		StartEngineTrail();
	}
	else if (!bRun && ActiveLoops.Contains(TEXT("EngineTrail")))
	{
		StopEngine();
	}
	constexpr float AudioEaseRate = 5.f; // 0 -> 1 in 0.2 s
	AudioBoost01 = FMath::FInterpConstantTo(AudioBoost01, IsBoosting() ? 1.f : 0.f, DeltaSeconds, AudioEaseRate);
	AudioDrift01 = FMath::FInterpConstantTo(AudioDrift01, IsDrifting() ? 1.f : 0.f, DeltaSeconds, AudioEaseRate);
	ApplyEngineAudioParams(EngineLoopAudio.Get());
	ApplyEngineAudioParams(EngineTrailAudio.Get());
}

FQuat ATDShip::ComputeChaseQuat(FVector& InOutRefUp, bool bSnap, float DeltaSeconds, float& OutRoll) const
{
	// Bank is measured against a reference "up" that tracks world up (rate-limited, held while the
	// nose is near vertical), so barrel rolls and loops never make the target jump.
	const FQuat ShipQuat = GetActorQuat();
	const FVector Forward = ShipQuat.GetForwardVector();
	const FVector ShipUp = ShipQuat.GetUpVector();

	FVector Current = TDShipCamera::Perp(InOutRefUp, Forward);
	FVector Target = FMath::Abs(Forward.Z) < CameraTuning.VerticalHoldLimit ? TDShipCamera::Perp(FVector::UpVector, Forward) : Current;
	if (Target.IsNearlyZero()) { Target = ShipUp; }
	if (bSnap || Current.IsNearlyZero()) { Current = Target; }
	const float ToTarget = TDShipCamera::SignedAngle(Current, Target, Forward);
	const float MaxStep = CameraTuning.ReferenceUpRate * DeltaSeconds;
	InOutRefUp = TDShipCamera::Rotate(Current, Forward, bSnap ? ToTarget : FMath::Clamp(ToTarget, -MaxStep, MaxStep));

	const float VisualBank = ShipMovement ? ShipMovement->GetVisualBank() : 0.f;
	const FVector VisualUp = (ShipQuat * FQuat(FRotator(0.f, 0.f, VisualBank))).GetUpVector();
	const float Bank = TDShipCamera::SignedAngle(InOutRefUp, ShipUp, Forward) + TDShipCamera::SignedAngle(ShipUp, VisualUp, Forward);
	OutRoll = CameraTuning.RollFollow * TDShipCamera::FoldBank(Bank);
	const FVector CameraUp = TDShipCamera::Rotate(InOutRefUp, Forward, OutRoll);
	return FRotationMatrix::MakeFromXZ(Forward, CameraUp).ToQuat();
}

FTransform ATDShip::GetChaseCameraTransformNow() const
{
	if (bPlayerControlled && !bCockpitView && ChaseCamera)
	{
		return ChaseCamera->GetComponentTransform();
	}
	FVector RefUp = FVector::ZeroVector; // snap: as the camera would be on a fresh cut to this ship
	float Roll = 0.f;
	const FQuat ArmQuat = ComputeChaseQuat(RefUp, true, 0.f, Roll);
	const FVector ArmEnd = GetActorLocation() - ArmQuat.GetForwardVector() * CameraTuning.ArmLength;
	const FVector CameraLocation = ArmEnd + ArmQuat.RotateVector(FVector(0.f, 0.f, CameraTuning.Height));

	FQuat CameraQuat = ArmQuat;
	if (ShipMovement)
	{
		const FVector Rates = ShipMovement->GetAngularRate();
		const float TurnRef = FMath::Max(ShipMovement->GetFlightTuning().TurnRate, 1.f);
		const FRotator Look(FMath::Clamp(Rates.Y / TurnRef, -1.f, 1.f) * CameraTuning.LookAheadAngle,
			FMath::Clamp(Rates.Z / TurnRef, -1.f, 1.f) * CameraTuning.LookAheadAngle, 0.f);
		CameraQuat = ArmQuat * Look.Quaternion();
	}
	return FTransform(CameraQuat, CameraLocation);
}

float ATDShip::GetChaseCameraFOVNow() const
{
	return FMath::Lerp(CameraTuning.MinFOV, CameraTuning.MaxFOV, GetSpeed01());
}

void ATDShip::UpdateChaseCamera(float DeltaSeconds, bool bSnap)
{
	if (!SpringArm || !ChaseCamera || !ShipMovement)
	{
		return;
	}

	if (!bSnap && SnapFramesRemaining > 0 && --SnapFramesRemaining > 0)
	{
		bSnap = true; // still inside the snap window: keep lag off, hard-set FOV and look-ahead
	}
	if (!bSnap)
	{
		// Restore lag after a snap.
		SpringArm->bEnableCameraLag = CameraTuning.LocationLag > 0.f;
		SpringArm->bEnableCameraRotationLag = CameraTuning.RotationLag > 0.f;
		SpringArm->CameraLagSpeed = CameraTuning.LocationLag;
		SpringArm->CameraRotationLagSpeed = CameraTuning.RotationLag;
	}

	// --- Arm rotation: ship forward, with only RollFollow of its bank -------------------------------
	ChaseTargetQuat = ComputeChaseQuat(CameraRefUp, bSnap, DeltaSeconds, CameraRoll);
	SpringArm->SetWorldRotation(ChaseTargetQuat);

	// --- Look-ahead: the camera leans into the turn --------------------------------------------------
	const FTDFlightTuning& Tuning = ShipMovement->GetFlightTuning();
	const FVector Rates = ShipMovement->GetAngularRate();
	const float TurnRef = FMath::Max(Tuning.TurnRate, 1.f);
	const FVector2D TargetLook(FMath::Clamp(Rates.Z / TurnRef, -1.f, 1.f), FMath::Clamp(Rates.Y / TurnRef, -1.f, 1.f));
	LookAhead = bSnap ? TargetLook : FMath::Vector2DInterpTo(LookAhead, TargetLook, DeltaSeconds, CameraTuning.LookAheadResponse);
	const FRotator Kick = FlightFeedback ? FlightFeedback->GetCameraOffset() : FRotator::ZeroRotator; // no-art camera kicks
	ChaseCamera->SetRelativeRotation(FRotator(LookAhead.Y * CameraTuning.LookAheadAngle, LookAhead.X * CameraTuning.LookAheadAngle, 0.f) + Kick);

	// --- FOV widens with speed -------------------------------------------------------------------------
	const float TargetFOV = FMath::Lerp(CameraTuning.MinFOV, CameraTuning.MaxFOV, ShipMovement->GetSpeed01());
	CurrentFOV = bSnap ? TargetFOV : FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaSeconds, CameraTuning.FOVResponse);
	ChaseCamera->SetFieldOfView(CurrentFOV);
}

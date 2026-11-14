// Triad Drift - the on-foot third-person camera (ENG-32). Owner: Teammate 3.

#include "TriadDrift/Ground/Camera/TDThirdPersonCameraComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/ShapeComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "TriadDrift/Director/TDDirectorCamera.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"

namespace TDThirdPerson
{
	static float Ease(float T)
	{
		T = FMath::Clamp(T, 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}
}

UTDThirdPersonCameraComponent::UTDThirdPersonCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics; // after the character moved; cameras update after all ticks
	PrimaryComponentTick.bTickEvenWhenPaused = true; // a Settings-page change while paused shows at once
}

void UTDThirdPersonCameraComponent::OnRegister()
{
	Super::OnRegister();
}

void UTDThirdPersonCameraComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Owner = GetOwner();
	if (!Camera && Owner)
	{
		Camera = NewObject<UCameraComponent>(Owner, TEXT("TDThirdPersonLens"), RF_Transient);
		Camera->SetupAttachment(this);
		Camera->SetUsingAbsoluteLocation(true);
		Camera->SetUsingAbsoluteRotation(true);
		Camera->bAutoActivate = false;
		Camera->bUsePawnControlRotation = false;
		Camera->SetFieldOfView(FieldOfView);
		Camera->RegisterComponent();
		Camera->Deactivate();
	}
	if (!FirstPersonCamera.IsValid())
	{
		FirstPersonCamera = FindFirstPersonCamera();
	}
}

void UTDThirdPersonCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetBodyHidden(false);
	if (Camera && (bThirdPersonActive || bBlendingOut))
	{
		Camera->Deactivate();
		if (UCameraComponent* FP = FirstPersonCamera.Get())
		{
			FP->Activate();
		}
	}
	Super::EndPlay(EndPlayReason);
}

UCameraComponent* UTDThirdPersonCameraComponent::FindFirstPersonCamera() const
{
	TArray<UCameraComponent*> Cameras;
	if (const AActor* Owner = GetOwner())
	{
		Owner->GetComponents(Cameras);
	}
	for (UCameraComponent* C : Cameras)
	{
		if (C != Camera)
		{
			return C;
		}
	}
	return nullptr;
}

void UTDThirdPersonCameraComponent::SetFirstPersonCamera(UCameraComponent* InCamera)
{
	FirstPersonCamera = InCamera;
}

UCameraComponent* UTDThirdPersonCameraComponent::GetViewCamera() const
{
	if (bThirdPersonActive && Camera)
	{
		return Camera;
	}
	return FirstPersonCamera.IsValid() ? FirstPersonCamera.Get() : FindFirstPersonCamera();
}

void UTDThirdPersonCameraComponent::SetThirdPerson(bool bWanted)
{
	bWantThirdPerson = bWanted;
	// Now if the view is ours (instant, even while paused); otherwise when the director gives it back.
	if (Camera && bWantThirdPerson != bThirdPersonActive && IsViewOurs())
	{
		ApplyMode(bWantThirdPerson);
		if (BlendSeconds <= 0.f && bBlendingOut)
		{
			bBlendingOut = false;
			Camera->Deactivate();
			if (UCameraComponent* FP = FirstPersonCamera.Get())
			{
				FP->Activate();
			}
			SetBodyHidden(false);
		}
	}
}

float UTDThirdPersonCameraComponent::GetDesiredActorYaw(const FVector& MoveVector, float CameraYaw, bool bCombatAction, float CurrentYaw)
{
	const float CamYaw = FRotator::NormalizeAxis(CameraYaw);
	const float Current = FRotator::NormalizeAxis(CurrentYaw);
	if (bCombatAction)
	{
		return CamYaw; // the blade swings where the crosshair points
	}
	const FVector2D Move(MoveVector.X, MoveVector.Y);
	if (Move.SizeSquared() > FMath::Square(0.1f))
	{
		return FRotator::NormalizeAxis(FMath::RadiansToDegrees(FMath::Atan2(Move.Y, Move.X)));
	}
	if (FMath::Abs(FMath::FindDeltaAngleDegrees(Current, CamYaw)) > 60.f)
	{
		return CamYaw;
	}
	return Current;
}

bool UTDThirdPersonCameraComponent::IsViewOurs() const
{
	if (const UTDDirectorSubsystem* Director = UTDDirectorSubsystem::Get(this))
	{
		if (const UTDDirectorCameraRig* Rig = Director->GetCameraRig(); Rig && Rig->IsActive())
		{
			return false; // a director shot or the death cam owns the view
		}
	}
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	return !PC || PC->GetViewTarget() == Pawn;
}

FRotator UTDThirdPersonCameraComponent::GetViewRotation() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->GetController())
	{
		return Pawn->GetController()->GetControlRotation();
	}
	return GetOwner() ? GetOwner()->GetActorRotation() : FRotator::ZeroRotator;
}

float UTDThirdPersonCameraComponent::SweepProbe(const FVector& From, const FVector& To) const
{
	const float Length = FVector::Dist(From, To);
	const UWorld* World = GetWorld();
	if (!World || Length < KINDA_SMALL_NUMBER)
	{
		return Length;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDThirdPersonCamera), false, GetOwner());
	if (const AActor* Owner = GetOwner())
	{
		TArray<AActor*> Attached;
		Owner->GetAttachedActors(Attached, true, true);
		Params.AddIgnoredActors(Attached);
	}
	const FCollisionShape Probe = FCollisionShape::MakeSphere(ProbeRadius);
	FCollisionObjectQueryParams Objects; // crew and other pawns ignore the camera channel by default: by object type
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
	float Safe = Length;
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		// Up to 4 crewmen pressed against the pilot: a probe that starts inside one looks past it (stopping at its
		// pivot would put the lens inside him). Starting inside the level means no room at all: stay at the pivot.
		for (int32 Try = 0; Try < 4; ++Try)
		{
			FHitResult Hit;
			const bool bHit = Pass == 0
				? World->SweepSingleByChannel(Hit, From, To, FQuat::Identity, ECC_Camera, Probe, Params)
				: World->SweepSingleByObjectType(Hit, From, To, FQuat::Identity, Objects, Probe, Params);
			if (!bHit)
			{
				break;
			}
			if (Hit.bStartPenetrating && Cast<APawn>(Hit.GetActor()))
			{
				Params.AddIgnoredActor(Hit.GetActor());
				continue;
			}
			Safe = FMath::Min(Safe, Hit.bStartPenetrating ? 0.f : Hit.Distance);
			break;
		}
	}
	return Safe;
}

bool UTDThirdPersonCameraComponent::HasLineToPilot(const FVector& Pivot, const FVector& Lens) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}
	// Complex: a tree's branches are outside its simple (trunk) collision, and a branch across the lens hides the pilot.
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDThirdPersonCameraLine), true, GetOwner());
	for (int32 Try = 0; Try < 4; ++Try)
	{
		FHitResult Hit;
		if (!World->LineTraceSingleByChannel(Hit, Pivot, Lens, ECC_Camera, Params))
		{
			return true;
		}
		if (!Cast<APawn>(Hit.GetActor()))
		{
			return false;
		}
		Params.AddIgnoredActor(Hit.GetActor()); // a crewman in between may hide the pilot for a moment
	}
	return true;
}

float UTDThirdPersonCameraComponent::ArmWithLineOfSight(const FVector& Pivot, const FVector& Shoulder, const FVector& Back, float SafeArm) const
{
	// The lens path bends at the shoulder, so a corner can sit between the lens and the pilot even when the
	// path itself is clear: shorten the arm until the pilot is in sight (the shoulder always is).
	float Arm = SafeArm;
	for (int32 i = 0; i < 6 && Arm > 1.f; ++i)
	{
		if (HasLineToPilot(Pivot, Shoulder + Back * Arm))
		{
			return Arm;
		}
		Arm *= 0.7f;
	}
	return HasLineToPilot(Pivot, Shoulder + Back * Arm) ? Arm : 0.f;
}

FVector UTDThirdPersonCameraComponent::SafePoint(const FVector& From, const FVector& To) const
{
	const float Length = FVector::Dist(From, To);
	if (Length < KINDA_SMALL_NUMBER)
	{
		return From;
	}
	return From + (To - From) / Length * SweepProbe(From, To);
}

FVector UTDThirdPersonCameraComponent::ShoulderFor(const FVector& Anchor, const FRotator& View) const
{
	const FRotator Yaw(0.f, View.Yaw, 0.f);
	const float E = CombatBlend * CombatBlend * (3.f - 2.f * CombatBlend);
	const float Right = FMath::Lerp(ShoulderRight, CombatShoulderRight, E);
	const float Up = FMath::Lerp(ShoulderUp, CombatShoulderUp, E);
	return SafePoint(Anchor, Anchor + Yaw.RotateVector(FVector(0.f, Right, 0.f)) + FVector::UpVector * Up);
}

FVector UTDThirdPersonCameraComponent::ResolveChaseLocation(const FVector& Pivot, const FRotator& View, float& OutSafeArm) const
{
	const FVector Shoulder = ShoulderFor(Pivot, View);
	const FVector Back = -View.Vector();
	OutSafeArm = ArmWithLineOfSight(Pivot, Shoulder, Back, SweepProbe(Shoulder, Shoulder + Back * ArmLength));
	return Shoulder + Back * OutSafeArm;
}

void UTDThirdPersonCameraComponent::UpdateChase(float DeltaTime, bool bSnap)
{
	const FVector Pivot = GetComponentLocation();
	const FRotator View = GetViewRotation();
	if (bSnap || !bHaveChase || FVector::DistSquared(LaggedPivot, Pivot) > FMath::Square(500.f))
	{
		bSnap = true; // a respawn or teleport: start again on the pilot
		LaggedPivot = Pivot;
		LaggedRotation = View;
	}
	else
	{
		LaggedPivot = LocationLagSpeed > 0.f ? FMath::VInterpTo(LaggedPivot, Pivot, DeltaTime, LocationLagSpeed) : Pivot;
		LaggedRotation = RotationLagSpeed > 0.f ? FMath::RInterpTo(LaggedRotation, View, DeltaTime, RotationLagSpeed) : View;
	}
	// Every probe starts inside the capsule (the true pivot), so the lens is always on a clear path from the pilot.
	const FVector Anchor = SafePoint(Pivot, LaggedPivot);
	const FVector Shoulder = ShoulderFor(Anchor, LaggedRotation);
	const FVector Back = -LaggedRotation.Vector();
	const float SafeArm = ArmWithLineOfSight(Pivot, Shoulder, Back, SweepProbe(Shoulder, Shoulder + Back * ArmLength));
	if (bSnap || !bHaveChase || SafeArm <= CurrentArm)
	{
		CurrentArm = SafeArm; // in at once: never a frame inside geometry
	}
	else
	{
		CurrentArm = FMath::Min(SafeArm, FMath::FInterpTo(CurrentArm, SafeArm, DeltaTime, EaseOutSpeed));
	}
	ChaseLocation = Shoulder + Back * CurrentArm;
	bHaveChase = true;
}

void UTDThirdPersonCameraComponent::ApplyMode(bool bThirdPerson)
{
	UCameraComponent* FP = FirstPersonCamera.IsValid() ? FirstPersonCamera.Get() : FindFirstPersonCamera();
	FirstPersonCamera = FP;
	if (bThirdPerson)
	{
		if (bBlendingOut)
		{
			BlendFrom = Camera->GetComponentTransform(); // turned back mid-blend: from where the lens is
			BlendFromFOV = Camera->FieldOfView;
		}
		else
		{
			BlendFrom = FP ? FP->GetComponentTransform() : GetComponentTransform();
			BlendFromFOV = FP ? FP->FieldOfView : FieldOfView;
			Camera->SetWorldTransform(BlendFrom);
			Camera->SetFieldOfView(BlendFromFOV);
		}
		bBlendingOut = false;
		bThirdPersonActive = true;
		bHaveChase = false; // start the chase on the pilot, no lag catch-up
		if (FP)
		{
			FP->Deactivate();
		}
		Camera->Activate();
	}
	else
	{
		BlendFrom = Camera->GetComponentTransform();
		BlendFromFOV = Camera->FieldOfView;
		bThirdPersonActive = false;
		bBlendingOut = true;
	}
	BlendAlpha = BlendSeconds > 0.f ? 0.f : 1.f;
}

void UTDThirdPersonCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Camera)
	{
		return;
	}
	if (bWantThirdPerson != bThirdPersonActive && IsViewOurs())
	{
		ApplyMode(bWantThirdPerson);
	}
	if (!bThirdPersonActive && !bBlendingOut)
	{
		SetBodyHidden(false);
		return;
	}

	// The blend runs in real time (a director slow-mo must not stretch a mode change).
	float Dilation = 1.f;
	if (const UWorld* World = GetWorld(); World && World->GetWorldSettings())
	{
		Dilation = FMath::Max(World->GetWorldSettings()->GetEffectiveTimeDilation(), 0.01f);
	}
	if (BlendAlpha < 1.f)
	{
		BlendAlpha = FMath::Min(1.f, BlendAlpha + DeltaTime / Dilation / FMath::Max(BlendSeconds, 0.01f));
	}

	if (CombatBlendSeconds > 0.f)
	{
		CombatBlend = FMath::FInterpConstantTo(CombatBlend, bCloseCombat ? 1.f : 0.f, DeltaTime / Dilation, 1.f / CombatBlendSeconds);
	}
	else
	{
		CombatBlend = bCloseCombat ? 1.f : 0.f;
	}
	UpdateChase(DeltaTime, false);
	FVector TargetLocation = ChaseLocation;
	FQuat TargetRotation = LaggedRotation.Quaternion();
	float TargetFOV = FieldOfView;
	if (bBlendingOut)
	{
		if (const UCameraComponent* FP = FirstPersonCamera.Get())
		{
			// The first-person camera follows the control rotation itself only while active; use the view.
			TargetLocation = FP->GetComponentLocation();
			TargetRotation = GetViewRotation().Quaternion();
			TargetFOV = FP->FieldOfView;
		}
	}
	const float A = TDThirdPerson::Ease(BlendAlpha);
	const FVector Location = FMath::Lerp(BlendFrom.GetLocation(), TargetLocation, A);
	const FQuat Rotation = FQuat::Slerp(BlendFrom.GetRotation(), TargetRotation, A);
	Camera->SetWorldLocationAndRotation(Location, Rotation);
	Camera->SetFieldOfView(FMath::Lerp(BlendFromFOV, TargetFOV, A));

	if (bBlendingOut && BlendAlpha >= 1.f)
	{
		bBlendingOut = false;
		Camera->Deactivate();
		if (UCameraComponent* FP = FirstPersonCamera.Get())
		{
			FP->Activate();
		}
		SetBodyHidden(false);
		return;
	}
	if (IsViewOurs())
	{
		UpdateBodyHidden();
	}
	else
	{
		SetBodyHidden(false); // the director / death cam frames the pilot from outside: never an empty spot (review M2)
	}
}

void UTDThirdPersonCameraComponent::UpdateBodyHidden()
{
	const FVector Lens = Camera->GetComponentLocation();
	float Distance = FVector::Dist(Lens, GetComponentLocation());
	if (const ACharacter* Char = Cast<ACharacter>(GetOwner()))
	{
		if (const UCapsuleComponent* Capsule = Char->GetCapsuleComponent())
		{
			const float Radius = Capsule->GetScaledCapsuleRadius();
			const float Half = FMath::Max(0.f, Capsule->GetScaledCapsuleHalfHeight() - Radius);
			const FVector C = Capsule->GetComponentLocation();
			const FVector Up = Capsule->GetUpVector();
			Distance = FMath::PointDistToSegment(Lens, C - Up * Half, C + Up * Half) - Radius;
		}
	}
	if (!bBodyHidden && Distance < HideBodyDistance)
	{
		SetBodyHidden(true);
	}
	else if (bBodyHidden && Distance > HideBodyDistance + 10.f)
	{
		SetBodyHidden(false);
	}
}

void UTDThirdPersonCameraComponent::SetBodyHidden(bool bHide)
{
	if (bHide == bBodyHidden)
	{
		return;
	}
	bBodyHidden = bHide;
	if (!bHide)
	{
		for (const TWeakObjectPtr<UPrimitiveComponent>& P : HiddenByUs)
		{
			if (UPrimitiveComponent* Prim = P.Get())
			{
				Prim->SetHiddenInGame(false);
			}
		}
		HiddenByUs.Reset();
		return;
	}
	TArray<UPrimitiveComponent*> Prims;
	if (const AActor* Owner = GetOwner())
	{
		Owner->GetComponents(Prims);
	}
	for (UPrimitiveComponent* Prim : Prims)
	{
		// Hidden-in-game is separate from the visibility T5's body code drives, so the two never fight.
		if (Prim && !Prim->IsA<UShapeComponent>() && !Prim->bHiddenInGame)
		{
			Prim->SetHiddenInGame(true);
			HiddenByUs.Add(Prim);
		}
	}
}

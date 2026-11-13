// Triad Drift - first-person character (ENG-21). Owner: Teammate 5.

#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Ground/TDEnergyBladeComponent.h"
#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "UObject/ConstructorHelpers.h"
#include "TriadDrift/Ground/TDPilotAnim.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequenceBase.h"
#include "Materials/MaterialInterface.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h"
#include "TriadDrift/Ground/Camera/TDThirdPersonCameraComponent.h"
#include "TriadDrift/Ground/TDGroundAudio.h"
#include "Engine/StaticMeshSocket.h"
#include "Materials/MaterialInstanceDynamic.h"

ATDFirstPersonCharacter::ATDFirstPersonCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 90.f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(95.f);
	BaseEyeHeight = 64.f;

	// Placeholder blade, held like a weapon low-right: a grip (posed per move), a dark hilt and a thin
	// emissive blade 70 cm long running up the grip's +Z. Never crosses the near plane or the crosshair at rest.
	BladeGrip = CreateDefaultSubobject<USceneComponent>(TEXT("BladeGrip"));
	BladeGrip->SetupAttachment(Camera);
	BladeGrip->SetRelativeLocationAndRotation(FVector(40.f, 24.f, -26.f), FRotator(-40.f, 0.f, 15.f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	BladeHilt = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BladeHilt"));
	BladeHilt->SetupAttachment(BladeGrip);
	BladeHilt->SetRelativeScale3D(FVector(0.035f, 0.035f, 0.13f));
	BladePlaceholder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BladePlaceholder"));
	BladePlaceholder->SetupAttachment(BladeGrip);
	BladePlaceholder->SetRelativeLocation(FVector(0.f, 0.f, 42.f));
	BladePlaceholder->SetRelativeScale3D(FVector(0.012f, 0.028f, 0.7f));
	for (UStaticMeshComponent* C : { BladeHilt.Get(), BladePlaceholder.Get() })
	{
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetOnlyOwnerSee(true);
		if (Cube.Succeeded())
		{
			C->SetStaticMesh(Cube.Object);
		}
	}

	Health = CreateDefaultSubobject<UTDHealthComponent>(TEXT("Health"));
	Health->MaxHull = Tuning.MaxHealth;
	Health->MaxShield = 0.f;
	Health->HullRegenDelay = Tuning.HealthRegenDelay;
	Health->HullRegenPerSecond = Tuning.HealthRegenPerSecond;
	Health->ShieldRegenFractionPerSecond = 0.f;
	Health->DefaultTeam = ETDTeam::Player;

	Blade = CreateDefaultSubobject<UTDEnergyBladeComponent>(TEXT("Blade"));

	StepSoilSound = TDGroundAudio::Default(TEXT("Foot/MSS_Foot_Step_Soil"));
	StepRockSound = TDGroundAudio::Default(TEXT("Foot/MSS_Foot_Step_Rock"));
	DashSound = TDGroundAudio::Default(TEXT("Foot/MSS_Foot_Dash"));
	JumpSound = TDGroundAudio::Default(TEXT("Foot/MSS_Foot_Jump"));
	LandSound = TDGroundAudio::Default(TEXT("Foot/MSS_Foot_Land"));
	HurtSound = TDGroundAudio::Default(TEXT("Foot/MSS_Player_Hurt"));

	// ENG-32: T3's chase rig on the capsule, and the blade the body holds whenever the whole pilot is on screen.
	ThirdPersonRig = CreateDefaultSubobject<UTDThirdPersonCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonRig->SetupAttachment(GetCapsuleComponent());
	HandBladeHilt = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandBladeHilt"));
	HandBladeHilt->SetupAttachment(GetMesh());
	HandBladeBeam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandBladeBeam"));
	HandBladeBeam->SetupAttachment(HandBladeHilt);
	for (UStaticMeshComponent* C : { HandBladeHilt.Get(), HandBladeBeam.Get() })
	{
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCanEverAffectNavigation(false);
		C->SetVisibility(false);
	}
	HandBladeBeam->SetCastShadow(false);
	// hand_r of the pilot skeleton (retargeted Manny): the grip in the palm (set by eye, TD.Ground.HandBlade).
	HandBladeOffset = FTransform(FRotator(-90.f, 0.f, 0.f), FVector(-9.f, -3.f, 0.f));

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = Tuning.RunSpeed;
	Move->MaxAcceleration = 6000.f;            // arcade-snappy starts
	Move->BrakingDecelerationWalking = 5000.f; // and stops
	Move->GroundFriction = 8.f;
	Move->AirControl = 0.6f;
	Move->GravityScale = 1.6f;                 // weighty jumps that still reach ~130 cm
	Move->JumpZVelocity = Tuning.JumpZ;
	Move->NavAgentProps.bCanCrouch = true;
	Move->SetCrouchedHalfHeight(50.f);
	Move->MaxWalkSpeedCrouched = Tuning.SlideStartSpeed; // the slide drives speed itself
	Move->BrakingDecelerationFlying = 0.f;
	Move->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = true;
	JumpMaxCount = 1; // our own double jump (TryJump) handles the second
}

void ATDFirstPersonCharacter::BeginPlay()
{
	Super::BeginPlay();
	// Tuning may have been edited on the instance: push it into the components.
	Health->InitMaxValues(Tuning.MaxHealth, 0.f, true);
	Health->HullRegenDelay = Tuning.HealthRegenDelay;
	Health->HullRegenPerSecond = Tuning.HealthRegenPerSecond;
	Health->OnDamaged.AddUniqueDynamic(this, &ATDFirstPersonCharacter::HandleDamaged);
	Health->OnDeath.AddUniqueDynamic(this, &ATDFirstPersonCharacter::HandleDeath);
	GetCharacterMovement()->JumpZVelocity = Tuning.JumpZ;
	DashCharges = Tuning.DashCharges;

	ApplyBladeVisuals();
	ApplyPilotVisuals();
	Blade->OnRetractChanged.AddUObject(this, &ATDFirstPersonCharacter::HandleBladeRetract);

	ThirdPersonRig->SetFirstPersonCamera(Camera);
	// Almost no motion blur on foot (the Rodin drone smeared under the planet volume's 0.2): both views.
	for (UCameraComponent* View : { Camera.Get(), ThirdPersonRig->GetThirdPersonCamera() })
	{
		if (View)
		{
			View->PostProcessSettings.bOverride_MotionBlurAmount = true;
			View->PostProcessSettings.MotionBlurAmount = OnFootMotionBlur;
		}
	}
	if (const UWorld* World = GetWorld())
	{
		bRockyGround = bRockyGround || World->GetMapName().Contains(TEXT("Cinder"));
	}
	if (UTDSettingsSubsystem* Options = UTDSettingsSubsystem::GetTDSettings(this))
	{
		// The saved choice applies at start-up and after every level travel (a new pilot each level).
		Options->OnOnFootCameraModeChanged.AddUniqueDynamic(this, &ATDFirstPersonCharacter::HandleCameraModeChanged);
		SetThirdPerson(Options->GetOnFootCameraMode() == ETDOnFootCamera::ThirdPerson);
	}

	// No extra tick ordering: CharacterMovement already ticks before its owner (bTickBeforeOwner). Forced
	// moves sweep the capsule themselves in our Tick, so they don't depend on the order.
}

bool ATDFirstPersonCharacter::HasBody() const
{
	return GetMesh() && GetMesh()->GetSkeletalMeshAsset() && GetMesh()->IsVisible();
}

int32 ATDFirstPersonCharacter::ChooseBodyLoop(float GroundSpeed)
{
	return GroundSpeed < 60.f ? 0 : GroundSpeed < 520.f ? 1 : 2; // run speed is 650, sprint 950
}

void ATDFirstPersonCharacter::ApplyPilotVisuals()
{
	USkeletalMeshComponent* Body = GetMesh();
	if (!Body)
	{
		return;
	}
	UTDPilotVisuals* V = PilotVisuals ? PilotVisuals.Get()
		: Cast<UTDPilotVisuals>(ATDCrewCharacter::FindLookAsset(TEXT("DA_Pilot_Visuals"), UTDPilotVisuals::StaticClass()));
	USkeletalMesh* SK = V && !V->SkeletalMesh.IsNull() ? V->SkeletalMesh.LoadSynchronous() : nullptr;
	AppliedPilotVisuals = SK ? V : nullptr;
	if (!SK)
	{
		Body->SetVisibility(false); // no look: first person as before, nothing else changes
		return;
	}
	Body->SetSkeletalMesh(SK);
	for (int32 i = 0; i < V->MaterialOverrides.Num(); ++i)
	{
		if (UMaterialInterface* Mat = V->MaterialOverrides[i].LoadSynchronous())
		{
			Body->SetMaterial(i, Mat);
		}
	}
	UClass* AnimClass = V->AnimBPClass.IsNull() ? nullptr : V->AnimBPClass.LoadSynchronous();
	Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body->SetAnimInstanceClass(AnimClass ? AnimClass : UTDLoopBlendAnimInstance::StaticClass());
	Body->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Body->SetCastShadow(true);
	Body->bCastHiddenShadow = true;
	Body->SetOnlyOwnerSee(false);
	Body->SetOwnerNoSee(false);
	Body->SetVisibility(true);
	bBodyViewKnown = false;
	PlaceBody();
	AttachHandBlade();
	UpdateBody(0.f);
}

void ATDFirstPersonCharacter::PlaceBody()
{
	USkeletalMeshComponent* Body = GetMesh();
	const UTDPilotVisuals* V = AppliedPilotVisuals.Get();
	if (!Body || !V)
	{
		return;
	}
	const float Back = bBodyFirstPerson ? FirstPersonBodyBack : 0.f;
	const FVector Loc(-Back, 0.f, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
	const FRotator Rot(0.f, V->MeshYaw, 0.f);
	Body->SetRelativeLocationAndRotation(Loc, Rot);
	Body->SetRelativeScale3D(FVector(FMath::Max(0.01f, V->MeshScale)));
	CacheInitialMeshOffset(Loc, Rot); // crouch and slide keep the feet on the floor from here
}

void ATDFirstPersonCharacter::SetBodyFirstPersonView(bool bFirstPerson)
{
	USkeletalMeshComponent* Body = GetMesh();
	const UTDPilotVisuals* V = AppliedPilotVisuals.Get();
	if (!Body || !V || (bBodyViewKnown && bFirstPerson == bBodyFirstPerson))
	{
		return;
	}
	bBodyViewKnown = true;
	bBodyFirstPerson = bFirstPerson;
	if (!V->HeadBone.IsNone() && Body->GetBoneIndex(V->HeadBone) != INDEX_NONE)
	{
		if (bFirstPerson)
		{
			Body->HideBoneByName(V->HeadBone, PBO_None);
		}
		else
		{
			Body->UnHideBoneByName(V->HeadBone);
		}
	}
	if (!Body->IsSimulatingPhysics())
	{
		PlaceBody();
	}
}

void ATDFirstPersonCharacter::UpdateBody(float DeltaSeconds)
{
	USkeletalMeshComponent* Body = GetMesh();
	const UTDPilotVisuals* V = AppliedPilotVisuals.Get();
	if (!Body || !V)
	{
		return;
	}
	// The owner's own view is first person only while the camera is ours (director shots, death and boarding cut away).
	const APlayerController* PC = Cast<APlayerController>(GetController());
	const bool bFirstPerson = PC && PC->PlayerCameraManager && PC->PlayerCameraManager->GetViewTarget() == this && !IsDead() && !IsThirdPersonView();
	SetBodyFirstPersonView(bFirstPerson);
	if (UTDLoopBlendAnimInstance* Loops = Cast<UTDLoopBlendAnimInstance>(Body->GetAnimInstance()))
	{
		const UCharacterMovementComponent* Move = GetCharacterMovement();
		const bool bAir = Move && Move->IsFalling();
		const int32 Want = bAir && Loops->GetLoop() ? -1 : ChooseBodyLoop((float)GetVelocity().Size2D());
		UAnimSequenceBase* Fall = bAir && !bMantling ? LoadPilotAnim(TEXT("A_Pilot_FallLoop")) : nullptr;
		UAnimSequenceBase* Loop = Fall ? Fall : Want == 0 ? V->Idle.LoadSynchronous() : Want == 1 ? V->Walk.LoadSynchronous() : Want == 2 ? V->Run.LoadSynchronous() : Loops->GetLoop();
		if (!Loop)
		{
			Loop = V->Idle.LoadSynchronous(); // a look with fewer loops still moves
		}
		const float Speed = (float)GetVelocity().Size2D();
		const float Ref = Want == 1 ? BodyWalkSpeed : Want == 2 ? BodyRunSpeed : 0.f;
		const float Rate = Fall ? 1.f : bAir ? 0.4f : (Ref > 0.f ? FMath::Clamp(Speed / Ref, 0.6f, 1.5f) : 1.f); // in the air: the fall loop, else the last loop slowed
		Loops->SetLoop(Loop, Rate);
	}
}

void ATDFirstPersonCharacter::ApplyBladeVisuals()
{
	ApplyHandBladeVisuals(); // the blade in the body's hand follows the same FX
	const UTDBladeFX* FX = Blade ? Blade->BladeFX.Get() : nullptr;
	if (FX && !FX->BladeMesh.IsNull())
	{
		if (UStaticMesh* ArtMesh = FX->BladeMesh.LoadSynchronous())
		{
			// Nick's first-person blade replaces the placeholder (authored at final scale, pivot in the hand).
			BladePlaceholder->AttachToComponent(BladeGrip, FAttachmentTransformRules::KeepRelativeTransform);
			BladePlaceholder->SetRelativeRotation(FRotator::ZeroRotator);
			BladePlaceholder->SetStaticMesh(ArtMesh);
			BladePlaceholder->SetRelativeLocation(FVector::ZeroVector); // authored with its pivot in the hand
			BladePlaceholder->SetRelativeScale3D(FVector::OneVector);
			BladeHilt->SetVisibility(false);
			bBladeMeshFromArt = true;
			return;
		}
	}
	bBladeMeshFromArt = false;
	const FLinearColor Color = FX ? FX->BladeColor : FLinearColor(0.1f, 0.65f, 1.f);
	// Engineering's blade (Gameplay/Props/Blade): the sculpted hilt in the grip and the two-layer beam out of its
	// emitter, 70 cm, the same pair the body holds in third person. The engine cubes only if the art is missing.
	UStaticMesh* HiltArt = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/TriadDrift/Gameplay/Props/Blade/SM_TD_BladeHilt.SM_TD_BladeHilt"), nullptr, LOAD_Quiet | LOAD_NoWarn);
	UStaticMesh* BeamArt = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/TriadDrift/Gameplay/Props/Blade/SM_TD_BladeBeam.SM_TD_BladeBeam"), nullptr, LOAD_Quiet | LOAD_NoWarn);
	if (HiltArt && BeamArt)
	{
		BladeHilt->SetStaticMesh(HiltArt);
		BladeHilt->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator(90.f, 0.f, 0.f)); // the hilt's +X up the grip's +Z
		BladeHilt->SetRelativeScale3D(FVector::OneVector);
		BladePlaceholder->SetStaticMesh(BeamArt);
		const bool bEmitter = HiltArt->FindSocket(TEXT("BladeEmitter")) != nullptr;
		BladePlaceholder->AttachToComponent(BladeHilt, FAttachmentTransformRules::SnapToTargetNotIncludingScale, bEmitter ? FName(TEXT("BladeEmitter")) : NAME_None);
		BladePlaceholder->SetRelativeLocationAndRotation(bEmitter ? FVector::ZeroVector : FVector(19.3f, 0.f, 0.f), FRotator::ZeroRotator);
		BladePlaceholder->SetRelativeScale3D(FVector(0.7f, 1.f, 1.f));
		if (UMaterialInstanceDynamic* MID = BladePlaceholder->CreateDynamicMaterialInstance(0))
		{
			MID->SetVectorParameterValue(TEXT("BladeColor"), Color);
		}
		return;
	}
	UTDStatics::TintPlaceholder(BladePlaceholder, Color, 1.4f);
	UTDStatics::TintPlaceholder(BladeHilt, FLinearColor(0.25f, 0.27f, 0.32f), 0.15f); // metal, never pure black
}

void ATDFirstPersonCharacter::UpdateBladePose(float DeltaSeconds)
{
	// Placeholder first-person blade motion (until Nick's arms/AnimBP): readable arcs for each move.
	if (!BladeGrip || !Blade)
	{
		return;
	}
	const float A = Blade->GetMoveProgress01();
	const float Ease = FMath::InterpEaseInOut(0.f, 1.f, A, 2.f);
	FVector Loc(40.f, 24.f, -26.f);
	FRotator Rot(-40.f, 0.f, 15.f); // rest: up and forward, leaning in a little
	switch (Blade->GetBladeState())
	{
	case ETDBladeState::Light1: // flat cut, right to left
		Loc = FVector(35.f, FMath::Lerp(26.f, -14.f, Ease), -14.f);
		Rot = FRotator(-80.f, FMath::Lerp(55.f, -65.f, Ease), 0.f);
		break;
	case ETDBladeState::Light2: // backhand, left to right
		Loc = FVector(35.f, FMath::Lerp(-14.f, 26.f, Ease), -12.f);
		Rot = FRotator(-80.f, FMath::Lerp(-65.f, 55.f, Ease), 0.f);
		break;
	case ETDBladeState::Light3: // overhead finisher
		Loc = FVector(35.f, 8.f, FMath::Lerp(-4.f, -22.f, Ease));
		Rot = FRotator(FMath::Lerp(15.f, -110.f, Ease), 0.f, 0.f);
		break;
	case ETDBladeState::HeavyLunge:
	case ETDBladeState::DashStrike: // thrust
		Loc = FVector(FMath::Lerp(25.f, 50.f, Ease), 10.f, -16.f);
		Rot = FRotator(-88.f, -4.f, 0.f);
		break;
	case ETDBladeState::AirSlam: // raised, ready to drive down
		Loc = FVector(30.f, 8.f, -6.f);
		Rot = FRotator(20.f, 0.f, 0.f);
		break;
	case ETDBladeState::Blocking: // across the lower view, right to left
		Loc = FVector(42.f, 24.f, -14.f);
		Rot = FRotator(0.f, -10.f, -85.f);
		break;
	case ETDBladeState::GuardBroken: // knocked low and wide
		Loc = FVector(30.f, 32.f, -36.f);
		Rot = FRotator(25.f, 0.f, 45.f);
		break;
	default:
		break;
	}
	// Swings snap fast; everything else eases back so the blade never teleports.
	const float Speed = Blade->IsAttacking() ? 40.f : 14.f;
	const FVector NewLoc = FMath::VInterpTo(BladeGrip->GetRelativeLocation(), Loc, DeltaSeconds, Speed);
	const FRotator NewRot = FMath::RInterpTo(BladeGrip->GetRelativeRotation(), Rot, DeltaSeconds, Speed);
	BladeGrip->SetRelativeLocationAndRotation(NewLoc, NewRot);
}

void ATDFirstPersonCharacter::HandleBladeRetract(bool bRetracted)
{
	const bool bShow = !bRetracted && !IsThirdPersonView();
	BladePlaceholder->SetVisibility(bShow);
	BladeHilt->SetVisibility(bShow && !bBladeMeshFromArt);
}

// ------------------------------------------------------------------------------------------------
// Third person (ENG-32): T3's rig, the facing rule, aim through the crosshair, the blade in the hand, readable moves
// ------------------------------------------------------------------------------------------------

void ATDFirstPersonCharacter::SetThirdPerson(bool bWanted)
{
	if (ThirdPersonRig)
	{
		ThirdPersonRig->SetThirdPerson(bWanted);
	}
}

bool ATDFirstPersonCharacter::IsThirdPersonView() const
{
	return ThirdPersonRig && ThirdPersonRig->IsThirdPersonActive();
}

UCameraComponent* ATDFirstPersonCharacter::GetViewCamera() const
{
	UCameraComponent* Rig = ThirdPersonRig ? ThirdPersonRig->GetViewCamera() : nullptr;
	return Rig ? Rig : Camera.Get();
}

void ATDFirstPersonCharacter::HandleCameraModeChanged(ETDOnFootCamera NewMode)
{
	SetThirdPerson(NewMode == ETDOnFootCamera::ThirdPerson);
	// From the pause page our Tick is not running but the rig switches at once: apply the body side now too
	// (no headless pilot or floating first-person blade until resume).
	ApplyViewState();
}

void ATDFirstPersonCharacter::ApplyViewState()
{
	const bool bThird = IsThirdPersonView();
	bUseControllerRotationYaw = !bThird;
	if (BladeGrip && BladeGrip->IsVisible() == bThird)
	{
		BladeGrip->SetVisibility(!bThird, false);
		HandleBladeRetract(Blade ? Blade->IsRetracted() : false); // the camera-rig blade only in first person
	}
	UpdateBody(0.f);       // head shown / hidden, body placed for the view
	UpdateHandBlade(0.f);  // the hand blade shows with the whole body
}

void ATDFirstPersonCharacter::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const
{
	Super::GetActorEyesViewPoint(OutLocation, OutRotation);
	const UCameraComponent* Chase = IsThirdPersonView() ? ThirdPersonRig->GetThirdPersonCamera() : nullptr;
	const UWorld* World = GetWorld();
	if (!Chase || !World)
	{
		return; // first person: the eye looks where the camera does
	}
	// Third person: the crosshair is the chase lens's centre. Aim from the eye at whatever that centre ray is on
	// (melee targeting, the deflect return and interaction then go where the player points, as in first person).
	const FVector Lens = Chase->GetComponentLocation();
	const FVector Ray = Chase->GetForwardVector();
	const float Near = FMath::Max(0.f, (float)FVector::DotProduct(OutLocation - Lens, Ray)); // nothing behind the pilot
	const float Far = 5000.f;
	FVector Target = Lens + Ray * Far;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDThirdPersonAim), false, this);
	if (World->LineTraceSingleByChannel(Hit, Lens + Ray * Near, Target, ECC_Visibility, Params))
	{
		Target = Hit.ImpactPoint;
	}
	const FVector Aim = Target - OutLocation;
	if (Aim.Size() > 100.f && FVector::DotProduct(Aim.GetSafeNormal(), Ray) > 0.5f)
	{
		OutRotation = Aim.Rotation();
	}
	else
	{
		OutRotation = Ray.Rotation(); // something right against the pilot: straight along the crosshair
	}
}

void ATDFirstPersonCharacter::UpdateFacing(float DeltaSeconds)
{
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	FVector MoveVector = bForcedMove ? ForcedVelocity : (Move ? Move->GetCurrentAcceleration() : FVector::ZeroVector);
	MoveVector.Z = 0.f;
	const bool bCombat = Blade && (Blade->IsAttacking() || Blade->IsBlocking() || Blade->GetBladeState() == ETDBladeState::Blocking);
	const float CameraYaw = (float)GetControlRotation().Yaw;
	const float Current = (float)GetActorRotation().Yaw;
	float Desired = UTDThirdPersonCameraComponent::GetDesiredActorYaw(MoveVector, CameraYaw, bCombat, Current);
	const bool bStill = MoveVector.SizeSquared() < 1.f && !bCombat;
	if (bStill && FMath::Abs(FMath::FindDeltaAngleDegrees(Current, CameraYaw)) > 60.f)
	{
		bIdleTurnToCamera = true;
	}
	if (bIdleTurnToCamera)
	{
		// The rule starts the turn past 60 deg; finish it, so the body ends up facing the camera, not 60 deg off.
		bIdleTurnToCamera = bStill && FMath::Abs(FMath::FindDeltaAngleDegrees(Current, CameraYaw)) > 1.f;
		Desired = bIdleTurnToCamera ? CameraYaw : Desired;
	}
	// A new attack snaps to the camera at once (no frames of swing the wrong way); blocking turns twice as fast.
	const bool bAttacking = Blade && Blade->IsAttacking();
	const bool bSnap = bAttacking && !bFacingWasAttacking;
	bFacingWasAttacking = bAttacking;
	const float Rate = ThirdPersonTurnRate * (bCombat ? 2.f : 1.f);
	SetActorRotation(FRotator(0.f, bSnap ? CameraYaw : FMath::FixedTurn(Current, Desired, Rate * DeltaSeconds), 0.f));
}

void ATDFirstPersonCharacter::AttachHandBlade()
{
	USkeletalMeshComponent* Body = GetMesh();
	if (!Body || !HandBladeHilt)
	{
		return;
	}
	const FName Socket = Body->DoesSocketExist(TEXT("BladeSocket")) ? FName(TEXT("BladeSocket")) : FName(TEXT("hand_r"));
	HandBladeHilt->AttachToComponent(Body, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Socket);
	if (Socket == TEXT("hand_r"))
	{
		HandBladeHilt->SetRelativeTransform(HandBladeOffset);
	}
	// The body is scaled (MeshScale): keep the blade at its authored size.
	const FVector BodyScale = Body->GetComponentScale();
	HandBladeHilt->SetRelativeScale3D(FVector(1.f / FMath::Max(0.01f, (float)BodyScale.X)));
}

void ATDFirstPersonCharacter::ApplyHandBladeVisuals()
{
	const UTDBladeFX* FX = Blade ? Blade->BladeFX.Get() : nullptr;
	if (FX && !FX->BladeMesh.IsNull())
	{
		if (UStaticMesh* ArtMesh = FX->BladeMesh.LoadSynchronous())
		{
			// Nick's blade wins: authored with its pivot in the hand and the blade up its +Z; turned onto the hilt's +X.
			HandBladeHilt->SetStaticMesh(nullptr);
			HandBladeBeam->SetStaticMesh(ArtMesh);
			HandBladeBeam->AttachToComponent(HandBladeHilt, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			HandBladeBeam->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
			return;
		}
	}
	static const TCHAR* HiltPath = TEXT("/Game/TriadDrift/Gameplay/Props/Blade/SM_TD_BladeHilt.SM_TD_BladeHilt");
	static const TCHAR* BeamPath = TEXT("/Game/TriadDrift/Gameplay/Props/Blade/SM_TD_BladeBeam.SM_TD_BladeBeam");
	UStaticMesh* Hilt = LoadObject<UStaticMesh>(nullptr, HiltPath, nullptr, LOAD_Quiet | LOAD_NoWarn);
	UStaticMesh* Beam = LoadObject<UStaticMesh>(nullptr, BeamPath, nullptr, LOAD_Quiet | LOAD_NoWarn);
	if (!Beam)
	{
		Beam = BladePlaceholder ? BladePlaceholder->GetStaticMesh() : nullptr; // no art: the engine cube, as in first person
	}
	HandBladeHilt->SetStaticMesh(Hilt);
	HandBladeBeam->SetStaticMesh(Beam);
	const bool bEmitter = Hilt && Hilt->FindSocket(TEXT("BladeEmitter"));
	HandBladeBeam->AttachToComponent(HandBladeHilt, FAttachmentTransformRules::SnapToTargetNotIncludingScale, bEmitter ? FName(TEXT("BladeEmitter")) : NAME_None);
	if (!bEmitter)
	{
		HandBladeBeam->SetRelativeLocation(FVector(Hilt ? 19.3f : 6.f, 0.f, 0.f));
	}
	const FLinearColor Color = FX ? FX->BladeColor : FLinearColor(0.1f, 0.65f, 1.f);
	if (Beam && Beam->GetName() == TEXT("SM_TD_BladeBeam"))
	{
		if (UMaterialInstanceDynamic* MID = HandBladeBeam->CreateDynamicMaterialInstance(0))
		{
			MID->SetVectorParameterValue(TEXT("BladeColor"), Color);
		}
	}
	else
	{
		UTDStatics::TintPlaceholder(HandBladeBeam, Color, 1.4f);
	}
}

void ATDFirstPersonCharacter::UpdateHandBlade(float DeltaSeconds)
{
	if (!HandBladeBeam || !HandBladeHilt)
	{
		return;
	}
	const bool bRetracted = !Blade || Blade->IsRetracted();
	HandBladeExtend = FMath::Clamp(HandBladeExtend + (bRetracted ? -1.f : 1.f) * DeltaSeconds / 0.15f, 0.f, 1.f);
	// Shown whenever the whole pilot is (third person, director shots, the death view), never in first person.
	const bool bShow = HasBody() && !bBodyFirstPerson;
	HandBladeHilt->SetVisibility(bShow && HandBladeHilt->GetStaticMesh() != nullptr);
	HandBladeBeam->SetVisibility(bShow && HandBladeExtend > 0.01f);
	const UStaticMesh* Beam = HandBladeBeam->GetStaticMesh();
	const bool bFromFX = Blade && Blade->BladeFX && !Blade->BladeFX->BladeMesh.IsNull();
	if (bFromFX)
	{
		HandBladeBeam->SetRelativeScale3D(FVector(1.f, 1.f, FMath::Max(HandBladeExtend, 0.001f))); // retract along its +Z
		return;
	}
	const float Authored = Beam ? FMath::Max(1.f, (float)Beam->GetBoundingBox().GetSize().X) : 100.f;
	const bool bArtBeam = Beam && Beam->GetName() == TEXT("SM_TD_BladeBeam");
	const float Length = HandBladeLength * FMath::Max(HandBladeExtend, 0.001f);
	if (bArtBeam)
	{
		HandBladeBeam->SetRelativeScale3D(FVector(Length / Authored, 1.f, 1.f)); // origin at the emitter: grows out of it
	}
	else
	{
		// The placeholder cube is centred: push it out by half its length.
		HandBladeBeam->SetRelativeScale3D(FVector(Length / Authored, 0.03f, 0.015f));
		HandBladeBeam->SetRelativeLocation(FVector(6.f + Length * 0.5f, 0.f, 0.f));
	}
}

UAnimSequenceBase* ATDFirstPersonCharacter::LoadPilotAnim(FName Anim) const
{
	if (Anim.IsNone())
	{
		return nullptr;
	}
	const FString Name = Anim.ToString();
	const FString Package = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Characters/Pilot/Anims/%s"), *Name);
	const FString Path = Package + TEXT(".") + Name;
	UAnimSequenceBase* Seq = FindObject<UAnimSequenceBase>(nullptr, *Path);
	if (!Seq && FPackageName::DoesPackageExist(Package))
	{
		Seq = LoadObject<UAnimSequenceBase>(nullptr, *Path, nullptr, LOAD_Quiet | LOAD_NoWarn);
	}
	return Seq ? TDMakePlayable(Seq) : nullptr;
}

void ATDFirstPersonCharacter::PlayBodyAction(FName Anim, float Rate, bool bUpperOnly)
{
	USkeletalMeshComponent* Body = GetMesh();
	UTDLoopBlendAnimInstance* Loops = Body && HasBody() ? Cast<UTDLoopBlendAnimInstance>(Body->GetAnimInstance()) : nullptr;
	UAnimSequenceBase* Seq = Loops ? LoadPilotAnim(Anim) : nullptr;
	if (Seq)
	{
		Loops->PlayOneShot(Seq, Rate, bUpperOnly);
		BodyActionName = Anim;
	}
}

void ATDFirstPersonCharacter::PlayFootSound(const TSoftObjectPtr<USoundBase>& Sound, FName Event, bool b2D)
{
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetSimpleCollisionHalfHeight());
	TDGroundAudio::Play(this, Sound, Feet, b2D, SoundEvents, Event);
}

void ATDFirstPersonCharacter::UpdateFootsteps(float DeltaSeconds)
{
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move || !Move->IsMovingOnGround() || bSliding || bDashing || bMantling)
	{
		return;
	}
	StepDistance += (float)GetVelocity().Size2D() * DeltaSeconds;
	const float Stride = bSprinting ? SprintStepStride : StepStride;
	if (StepDistance >= Stride)
	{
		StepDistance = FMath::Fmod(StepDistance, Stride);
		PlayFootSound(bRockyGround ? StepRockSound : StepSoilSound, bRockyGround ? FName(TEXT("StepRock")) : FName(TEXT("StepSoil")));
	}
}

void ATDFirstPersonCharacter::UpdateCloseCombat(float DeltaSeconds)
{
	if (!ThirdPersonRig)
	{
		return;
	}
	CloseCombatScan -= DeltaSeconds;
	if (CloseCombatScan <= 0.f)
	{
		CloseCombatScan = 0.15f; // a few crew at most: a cheap scan, not every frame
		bEnemyClose = false;
		const FVector Here = GetActorLocation();
		for (TActorIterator<ATDCrewCharacter> It(GetWorld()); It && !bEnemyClose; ++It)
		{
			bEnemyClose = !It->IsDead() && FVector::DistSquared(It->GetActorLocation(), Here) < FMath::Square(CloseCombatRange);
		}
	}
	const bool bBladeBusy = Blade && (Blade->IsAttacking() || Blade->IsBlocking());
	const bool bClose = !IsDead() && (bEnemyClose || bBladeBusy);
	if (bClose != bLastCloseCombat)
	{
		bLastCloseCombat = bClose;
		ThirdPersonRig->SetCloseCombat(bClose);
	}
}

void ATDFirstPersonCharacter::UpdateBodyActions()
{
	USkeletalMeshComponent* Body = GetMesh();
	UTDLoopBlendAnimInstance* Loops = Body && HasBody() ? Cast<UTDLoopBlendAnimInstance>(Body->GetAnimInstance()) : nullptr;
	if (!Loops || !Blade || IsDead())
	{
		return;
	}
	// Moving: swings play from the spine up so the legs keep running; standing: the whole body commits.
	const bool bMoving = GetVelocity().Size2D() > 150.f;
	const ETDBladeState State = Blade->GetBladeState();
	if (State != LastBodyBladeState)
	{
		switch (State)
		{
		case ETDBladeState::Light1: PlayBodyAction(TEXT("A_Pilot_Light1"), 1.6f, bMoving); break;
		case ETDBladeState::Light2: PlayBodyAction(TEXT("A_Pilot_Light2"), 1.6f, bMoving); break;
		case ETDBladeState::Light3: PlayBodyAction(TEXT("A_Pilot_Light3"), 1.5f, bMoving); break;
		case ETDBladeState::HeavyLunge:
		case ETDBladeState::DashStrike: PlayBodyAction(TEXT("A_Pilot_Heavy"), 1.4f, false); break;
		case ETDBladeState::AirSlam: PlayBodyAction(TEXT("A_Pilot_Light3"), 1.2f, false); break;
		case ETDBladeState::GuardBroken: PlayBodyAction(TEXT("A_Pilot_HitReact_Front"), 1.f, true); break;
		default: break;
		}
		LastBodyBladeState = State;
	}
	// The guard is held on the upper body for as long as the block is.
	Loops->SetUpperLoop(State == ETDBladeState::Blocking ? LoadPilotAnim(TEXT("A_Pilot_Block")) : nullptr);
	if (bDashing && !bLastBodyDashing && !Blade->IsAttacking())
	{
		PlayBodyAction(TEXT("A_Pilot_Dash"), 1.8f, false);
	}
	if (bSliding && !bLastBodySliding)
	{
		PlayBodyAction(TEXT("A_Pilot_Dash"), 0.8f, false);
	}
	if (bMantling && !bLastBodyMantling)
	{
		PlayBodyAction(TEXT("A_Pilot_JumpStart"), 1.4f, false);
	}
	bLastBodyDashing = bDashing;
	bLastBodySliding = bSliding;
	bLastBodyMantling = bMantling;
}

bool ATDFirstPersonCharacter::IsDead() const
{
	return Health && Health->IsDead();
}

ETDBladeState ATDFirstPersonCharacter::GetBladeState() const
{
	return Blade ? Blade->GetBladeState() : ETDBladeState::Idle;
}

float ATDFirstPersonCharacter::GetDashRecharge01() const
{
	return DashCharges >= Tuning.DashCharges ? 1.f : FMath::Clamp(DashRechargeTime / FMath::Max(Tuning.DashRechargeSeconds, 0.01f), 0.f, 1.f);
}

float ATDFirstPersonCharacter::GetHealth01() const
{
	return Health ? Health->GetHull01() : 0.f;
}

float ATDFirstPersonCharacter::GetBlockStamina01() const
{
	return Blade ? Blade->GetBlockStamina01() : 0.f;
}

float ATDFirstPersonCharacter::GetInteractProgress01() const
{
	return HoldRequired > 0.f ? FMath::Clamp(HoldProgress / HoldRequired, 0.f, 1.f) : 0.f;
}

FText ATDFirstPersonCharacter::GetInteractPrompt() const
{
	if (const ITDInteractable* I = Cast<ITDInteractable>(InteractTarget.Get()))
	{
		if (I->CanInteract(const_cast<ATDFirstPersonCharacter*>(this)))
		{
			return I->GetPrompt();
		}
	}
	return FText::GetEmpty();
}

TArray<AActor*> ATDFirstPersonCharacter::GetObjectiveMarkers() const
{
	TArray<AActor*> Out;
	if (const UTDObjectiveSubsystem* Objectives = UTDObjectiveSubsystem::Get(this))
	{
		Objectives->GetActiveWaypoints(Out);
	}
	return Out;
}

void ATDFirstPersonCharacter::OnPausePressed()
{
	if (ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(GetController()))
	{
		PC->OnPauseInput.Broadcast(); // the same pause the ships use (T1's controller owns what it opens)
	}
}

FText ATDFirstPersonCharacter::GetObjectiveText() const
{
	const UTDGameInstance* GI = UTDGameInstance::Get(this);
	return GI ? GI->GetCurrentObjectiveText() : FText::GetEmpty();
}

// ------------------------------------------------------------------------------------------------
// Moves
// ------------------------------------------------------------------------------------------------

bool ATDFirstPersonCharacter::TryJump()
{
	if (IsDead() || bMantling)
	{
		return false;
	}
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (bSliding)
	{
		SlideTimeLeft = 0.f;
		bSliding = false;
		UnCrouch();
	}
	if (Move->IsMovingOnGround())
	{
		LaunchCharacter(FVector(0.f, 0.f, Tuning.JumpZ), false, true);
		JumpsUsed = 1;
		PlayFootSound(JumpSound, TEXT("Jump"));
		return true;
	}
	if (JumpsUsed < 2)
	{
		// Falling off a ledge without jumping still allows exactly one air jump.
		LaunchCharacter(FVector(0.f, 0.f, Tuning.DoubleJumpZ), false, true);
		JumpsUsed = 2;
		PlayFootSound(JumpSound, TEXT("Jump"));
		return true;
	}
	return false;
}

bool ATDFirstPersonCharacter::TryDash()
{
	if (IsDead() || bMantling || bDashing || DashCharges <= 0 || bCarrying)
	{
		return false;
	}
	const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
	FVector Dir = Yaw.RotateVector(FVector(MoveInput.Y, MoveInput.X, 0.f));
	if (Dir.IsNearlyZero())
	{
		Dir = Yaw.Vector();
	}
	Dir = Dir.GetSafeNormal2D();
	if (bSliding)
	{
		SlideTimeLeft = 0.f;
		bSliding = false;
		UnCrouch();
	}

	--DashCharges;
	if (DashCharges == Tuning.DashCharges - 1)
	{
		DashRechargeTime = 0.f; // first spent charge starts the recharge clock
	}
	const float Duration = FMath::Max(Tuning.DashDuration, 0.01f);
	StartForcedMove(Dir * (Tuning.DashDistance / Duration), Duration, Tuning.DashExitSpeed);
	bDashing = true;
	Health->SetInvulnerableFor(Tuning.DashIFrames);
	PlayFootSound(DashSound, TEXT("Dash"));
	return true;
}

void ATDFirstPersonCharacter::StartForcedMove(const FVector& Velocity, float Duration, float ExitSpeed)
{
	if (Duration <= 0.f)
	{
		return;
	}
	bForcedMove = true;
	ForcedVelocity = Velocity;
	ForcedDirection = Velocity.GetSafeNormal();
	ForcedTimeLeft = Duration;
	ForcedExitSpeed = ExitSpeed;
	bDashing = false; // TryDash sets it after; a lunge is not a dash
	// MOVE_None: the movement component stands aside while TickForcedMove places the capsule.
	GetCharacterMovement()->SetMovementMode(MOVE_None);
	GetCharacterMovement()->Velocity = Velocity;
}

bool ATDFirstPersonCharacter::TrySlide()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (IsDead() || bSliding || bDashing || bMantling || !bSprinting || !Move->IsMovingOnGround())
	{
		return false;
	}
	const FVector Vel2D(Move->Velocity.X, Move->Velocity.Y, 0.f);
	SlideDirection = Vel2D.IsNearlyZero() ? GetActorForwardVector().GetSafeNormal2D() : Vel2D.GetSafeNormal();
	SlideTimeLeft = Tuning.SlideDuration;
	bSliding = true;
	Crouch();
	return true;
}

void ATDFirstPersonCharacter::CrouchAction()
{
	if (bSprinting && GetCharacterMovement()->IsMovingOnGround())
	{
		TrySlide();
	}
	else
	{
		TryDash();
	}
}

bool ATDFirstPersonCharacter::FindMantleTarget(FVector& OutTarget) const
{
	const UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!World || !Capsule)
	{
		return false;
	}
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FVector Loc = GetActorLocation();
	const float FeetZ = Loc.Z - HalfHeight;
	const FVector Fwd = FRotator(0.f, GetControlRotation().Yaw, 0.f).Vector();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDMantle), false, this);

	// 1) A wall right ahead, above step height.
	const FVector WallStart(Loc.X, Loc.Y, FeetZ + Tuning.MantleMinHeight);
	FHitResult Wall;
	if (!World->LineTraceSingleByChannel(Wall, WallStart, WallStart + Fwd * (Radius + Tuning.MantleReach), ECC_Visibility, Params))
	{
		return false;
	}
	if (FMath::Abs(Wall.ImpactNormal.Z) > 0.3f)
	{
		return false; // a slope, not a wall
	}

	// 2) Its top, within reach.
	const FVector Over = Wall.ImpactPoint + Fwd * (Radius + 6.f);
	const FVector DownStart(Over.X, Over.Y, FeetZ + Tuning.MantleMaxHeight + 5.f);
	const FVector DownEnd(Over.X, Over.Y, FeetZ + Tuning.MantleMinHeight - 5.f);
	FHitResult Top;
	if (!World->LineTraceSingleByChannel(Top, DownStart, DownEnd, ECC_Visibility, Params) || Top.bStartPenetrating)
	{
		return false; // nothing within 180 cm (too tall) or no top
	}
	if (Top.ImpactNormal.Z < 0.7f)
	{
		return false;
	}
	const float LedgeHeight = Top.ImpactPoint.Z - FeetZ;
	if (LedgeHeight < Tuning.MantleMinHeight || LedgeHeight > Tuning.MantleMaxHeight)
	{
		return false;
	}

	// 3) Room to stand there.
	const FVector Target(Top.ImpactPoint.X, Top.ImpactPoint.Y, Top.ImpactPoint.Z + HalfHeight + 2.f);
	if (World->OverlapBlockingTestByChannel(Target, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Radius, HalfHeight), Params))
	{
		return false;
	}
	OutTarget = Target;
	return true;
}

bool ATDFirstPersonCharacter::TryMantle()
{
	if (IsDead() || bMantling || bForcedMove || MantleCooldown > 0.f || (Blade && Blade->GetBladeState() == ETDBladeState::AirSlam))
	{
		return false; // a slam commits you downward
	}
	FVector Target;
	if (!FindMantleTarget(Target))
	{
		return false;
	}
	if (bSliding)
	{
		SlideTimeLeft = 0.f;
		bSliding = false;
		UnCrouch();
	}
	bMantling = true;
	MantleStart = GetActorLocation();
	MantleEnd = Target;
	MantleTime = 0.f;
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Flying); // no gravity while we place the body
	return true;
}

void ATDFirstPersonCharacter::SetCarrying(bool bNewCarrying)
{
	bCarrying = bNewCarrying;
	if (bCarrying && Blade)
	{
		Blade->CancelAll();
	}
}

void ATDFirstPersonCharacter::RespawnAt(const FTransform& Where)
{
	LastSafeLocation = Where.GetLocation();
	bForcedMove = bDashing = bSliding = bMantling = false;
	ForcedTimeLeft = SlideTimeLeft = 0.f;
	UnCrouch();
	CancelInteract();
	if (Blade)
	{
		Blade->CancelAll();
	}
	SetActorLocation(Where.GetLocation(), false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* C = GetController())
	{
		C->SetControlRotation(Where.Rotator());
	}
	if (USkeletalMeshComponent* Body = GetMesh(); Body && Body->IsSimulatingPhysics())
	{
		// Back from the ragdoll: the body rejoins the capsule.
		Body->SetSimulatePhysics(false);
		Body->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Body->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		bBodyViewKnown = false;
		PlaceBody();
		AttachHandBlade();
	}
	if (UTDLoopBlendAnimInstance* Loops = GetMesh() ? Cast<UTDLoopBlendAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr)
	{
		Loops->StopOneShot(); // a new take starts clean (no swing or flinch carried over)
		Loops->SetUpperLoop(nullptr);
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Health->Revive(1.f, 1.f);
	bCarrying = false;
	if (Blade)
	{
		Blade->ResetState(); // a new take starts with a fresh blade (stamina, cooldowns, crit)
		Blade->Ignite();
	}
	DashCharges = Tuning.DashCharges;
	DashRechargeTime = 0.f;
	JumpsUsed = 0;
}

// ------------------------------------------------------------------------------------------------
// Interact & ping
// ------------------------------------------------------------------------------------------------

AActor* ATDFirstPersonCharacter::TraceInteractable() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	FVector Eye;
	FRotator Rot;
	GetActorEyesViewPoint(Eye, Rot);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDInteract), false, this);
	FHitResult Hit;
	const FVector End = Eye + Rot.Vector() * Tuning.InteractRange;
	// Interactables block the Interactable trace channel; a fat sweep makes aiming forgiving.
	if (World->SweepSingleByChannel(Hit, Eye, End, FQuat::Identity, TDCollision::Interactable, FCollisionShape::MakeSphere(20.f), Params))
	{
		AActor* A = Hit.GetActor();
		if (A && A->GetClass()->ImplementsInterface(UTDInteractable::StaticClass()))
		{
			return A;
		}
	}
	return nullptr;
}

void ATDFirstPersonCharacter::BeginInteract()
{
	if (IsDead() || bMantling)
	{
		return;
	}
	AActor* Target = TraceInteractable();
	ITDInteractable* I = Cast<ITDInteractable>(Target);
	if (!I || !I->CanInteract(this))
	{
		return;
	}
	const float Hold = I->GetHoldTime();
	if (Hold <= 0.f)
	{
		I->Interact(this);
		OnInteractCompleted.Broadcast(Target);
		return;
	}
	HoldTarget = Target;
	HoldRequired = Hold;
	HoldProgress = 0.f;
}

void ATDFirstPersonCharacter::EndInteract()
{
	CancelInteract();
}

void ATDFirstPersonCharacter::CancelInteract()
{
	HoldTarget = nullptr;
	HoldProgress = 0.f;
	HoldRequired = 0.f;
}

void ATDFirstPersonCharacter::PingObjective()
{
	PingTimeLeft = Tuning.PingSeconds;
	if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
	{
		Flow->ShowTip(TEXT("Tip_ObjectivePing"));
	}
}

void ATDFirstPersonCharacter::HandleDamaged(UTDHealthComponent* InHealth, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage)
{
	UE_LOG(LogTriadDrift, Verbose, TEXT("Pilot hit: %.0f %s from %s, hull %.0f"), HullDamage, *UEnum::GetValueAsString(Info.Kind), *GetNameSafe(Info.Causer), InHealth ? InHealth->GetHull() : 0.f);
	if (HullDamage > 0.f && GetWorld() && GetWorld()->GetTimeSeconds() - LastHurtSoundTime > 0.25)
	{
		LastHurtSoundTime = GetWorld()->GetTimeSeconds(); // a burst of bolts is one grunt, not five
		PlayFootSound(HurtSound, TEXT("Hurt"), true);
	}
	if (HullDamage > 0.f && InHealth && InHealth->IsAlive() && Blade && !Blade->IsAttacking())
	{
		// A flinch the camera can read in third person: front or back by where the hit came from.
		const AActor* From = Info.Causer ? Info.Causer.Get() : Info.Instigator.Get();
		const bool bFromBehind = From && FVector::DotProduct((From->GetActorLocation() - GetActorLocation()).GetSafeNormal2D(), GetActorForwardVector()) < 0.f;
		PlayBodyAction(bFromBehind ? TEXT("A_Pilot_HitReact_Back") : TEXT("A_Pilot_HitReact_Front"), 1.2f, true);
	}
	const ATDInteractableActor* Held = Cast<ATDInteractableActor>(HoldTarget.Get());
	if (ShieldDamage + HullDamage > 0.f && !(Held && Held->bHoldSurvivesDamage))
	{
		CancelInteract(); // holds are interrupted by damage (except a hold made to be done under fire)
	}
}

void ATDFirstPersonCharacter::HandleDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow)
{
	// Every on-foot death says what did it (playtests, balance: a death nobody can explain is a bug).
	UE_LOG(LogTriadDrift, Log, TEXT("Pilot down at %s: %.0f %s from %s (causer %s)"), *GetActorLocation().ToCompactString(), KillingBlow.Amount,
		*UEnum::GetValueAsString(KillingBlow.Kind), *GetNameSafe(KillingBlow.Instigator), *GetNameSafe(KillingBlow.Causer));
	CancelInteract();
	if (Blade)
	{
		Blade->CancelAll();
	}
	bForcedMove = bDashing = bSliding = bMantling = false;
	if (Blade)
	{
		Blade->Retract();
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	if (USkeletalMeshComponent* Body = GetMesh(); Body && HasBody() && Body->GetPhysicsAsset())
	{
		SetBodyFirstPersonView(false); // the death view sees the whole pilot fall
		Body->SetCollisionProfileName(TEXT("Ragdoll"));
		Body->SetSimulatePhysics(true);
	}
	if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
	{
		Flow->ReportPlayerDefeated(); // CUT! The scene restarts as the next take.
	}
}

void ATDFirstPersonCharacter::FellOutOfWorld(const UDamageType& DmgType)
{
	// Never destroy the player pawn: a fall is a death like any other (next take).
	if (Health && Health->IsAlive())
	{
		Health->Kill(nullptr, ETDDamageKind::Environmental);
	}
}

void ATDFirstPersonCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	JumpsUsed = 0;
	StepDistance = 0.f; // the landing is the step
	PlayFootSound(LandSound, TEXT("Land"));
	if (Blade)
	{
		Blade->NotifyOwnerLanded();
	}
}

// ------------------------------------------------------------------------------------------------
// Tick
// ------------------------------------------------------------------------------------------------

void ATDFirstPersonCharacter::UpdateMoveSpeed()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const bool bBladeBusy = Blade && (Blade->IsBlocking() || Blade->IsAttacking());
	if (bSprintToggled && MoveInput.Size() < SprintToggleMinStick)
	{
		bSprintToggled = false; // the pad sprint ends when the stick is let go (or eased right down)
	}
	bSprinting = (bSprintHeld || bSprintToggled) && MoveInput.Y > 0.1f && !bCarrying && !bBladeBusy && !bSliding;
	const float Scale = Blade ? Blade->GetMoveSpeedScale() : 1.f;
	Move->MaxWalkSpeed = (bSprinting ? Tuning.SprintSpeed : Tuning.RunSpeed) * Scale;
}

void ATDFirstPersonCharacter::TickForcedMove(float DeltaSeconds)
{
	if (!bForcedMove)
	{
		return;
	}
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (ForcedTimeLeft <= 0.f)
	{
		// Done: carry some speed out of it and hand back to normal movement.
		bForcedMove = false;
		bDashing = false;
		Move->Velocity = ForcedDirection.GetSafeNormal2D() * ForcedExitSpeed;
		Move->SetMovementMode(MOVE_Falling); // resolves to walking next frame if grounded
		return;
	}
	// Move the capsule ourselves (sweeping, sliding along walls) so the distance is exact at any frame
	// rate and doesn't depend on whether movement ticks before or after us. The last frame is partial.
	const float StepTime = FMath::Min(ForcedTimeLeft, DeltaSeconds);
	const FVector Delta = ForcedVelocity * StepTime;
	FHitResult Hit;
	Move->SafeMoveUpdatedComponent(Delta, GetActorQuat(), true, Hit);
	if (Hit.IsValidBlockingHit())
	{
		// Slide the rest of the step along the wall.
		const FVector Rest = FVector::VectorPlaneProject(Delta * (1.f - Hit.Time), Hit.Normal);
		FHitResult SlideHit;
		Move->SafeMoveUpdatedComponent(Rest, GetActorQuat(), true, SlideHit);
	}
	Move->Velocity = ForcedVelocity; // for HUD / anim readers
	ForcedTimeLeft -= DeltaSeconds;
}

void ATDFirstPersonCharacter::TickDash(float DeltaSeconds)
{
	if (DashCharges >= Tuning.DashCharges)
	{
		DashRechargeTime = 0.f;
		return;
	}
	DashRechargeTime += DeltaSeconds;
	while (DashRechargeTime >= Tuning.DashRechargeSeconds && DashCharges < Tuning.DashCharges)
	{
		DashRechargeTime -= Tuning.DashRechargeSeconds;
		++DashCharges;
	}
}

void ATDFirstPersonCharacter::TickSlide(float DeltaSeconds)
{
	if (!bSliding)
	{
		return;
	}
	UCharacterMovementComponent* Move = GetCharacterMovement();
	SlideTimeLeft -= DeltaSeconds;
	if (SlideTimeLeft <= 0.f || !Move->IsMovingOnGround())
	{
		bSliding = false;
		SlideTimeLeft = 0.f;
		UnCrouch();
		return;
	}
	const float Alpha = 1.f - SlideTimeLeft / FMath::Max(Tuning.SlideDuration, 0.01f);
	const float Speed = FMath::Lerp(Tuning.SlideStartSpeed, Tuning.RunSpeed, Alpha);
	Move->Velocity = FVector(SlideDirection.X * Speed, SlideDirection.Y * Speed, Move->Velocity.Z);
}

void ATDFirstPersonCharacter::TickMantle(float DeltaSeconds)
{
	MantleCooldown = FMath::Max(0.f, MantleCooldown - DeltaSeconds);
	if (!bMantling)
	{
		// Auto-mantle: pushing forward into a ledge we can climb.
		if (MoveInput.Y > 0.3f && !bForcedMove && !IsDead() && MantleCooldown <= 0.f)
		{
			TryMantle();
		}
		return;
	}
	MantleTime += DeltaSeconds;
	const float A = FMath::Clamp(MantleTime / FMath::Max(Tuning.MantleDuration, 0.01f), 0.f, 1.f);
	// Rise first, then pull forward over the lip.
	const float Up = FMath::InterpEaseOut(0.f, 1.f, FMath::Min(1.f, A / 0.6f), 2.f);
	const float Fwd = FMath::InterpEaseInOut(0.f, 1.f, FMath::Max(0.f, (A - 0.3f) / 0.7f), 2.f);
	const FVector P(FMath::Lerp(MantleStart.X, MantleEnd.X, Fwd), FMath::Lerp(MantleStart.Y, MantleEnd.Y, Fwd), FMath::Lerp(MantleStart.Z, MantleEnd.Z, Up));
	SetActorLocation(P, false, nullptr, ETeleportType::None);
	GetCharacterMovement()->Velocity = FVector::ZeroVector;
	if (A >= 1.f)
	{
		bMantling = false;
		MantleCooldown = 0.2f;
		JumpsUsed = 0;
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void ATDFirstPersonCharacter::TickInteract(float DeltaSeconds)
{
	InteractTarget = TraceInteractable();
	if (!HoldTarget.IsValid())
	{
		return;
	}
	ITDInteractable* I = Cast<ITDInteractable>(HoldTarget.Get());
	if (!I || InteractTarget.Get() != HoldTarget.Get() || !I->CanInteract(this))
	{
		CancelInteract(); // looked away or it became unavailable
		return;
	}
	HoldProgress += DeltaSeconds;
	if (HoldProgress >= HoldRequired)
	{
		AActor* Done = HoldTarget.Get();
		CancelInteract();
		I->Interact(this);
		OnInteractCompleted.Broadcast(Done);
	}
}

void ATDFirstPersonCharacter::TickSafeGround(float DeltaSeconds)
{
	SafeSampleTimer -= DeltaSeconds;
	if (SafeSampleTimer > 0.f)
	{
		return;
	}
	SafeSampleTimer = 0.25f;
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	const FFindFloorResult& Floor = Move->CurrentFloor;
	if (!Move->IsMovingOnGround() || !Floor.IsWalkableFloor() || bForcedMove)
	{
		return;
	}
	const AActor* FloorActor = Floor.HitResult.GetActor();
	if (FloorActor && FloorActor->ActorHasTag(TEXT("TD.Unsafe")))
	{
		return;
	}
	LastSafeLocation = GetActorLocation();
}

void ATDFirstPersonCharacter::ReturnToSafeGround()
{
	if (LastSafeLocation.IsZero())
	{
		return;
	}
	CancelInteract();
	bForcedMove = bDashing = bSliding = bMantling = false;
	GetCharacterMovement()->StopMovementImmediately();
	SetActorLocation(LastSafeLocation, false, nullptr, ETeleportType::TeleportPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	SafeSampleTimer = 0.5f; // don't re-sample a spot at the hazard edge straight away
}

void ATDFirstPersonCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (DeltaSeconds <= 0.f)
	{
		return;
	}
	if (!IsDead())
	{
		if (!bMantling && !bForcedMove && !MoveInput.IsNearlyZero() && !bSliding)
		{
			const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
			AddMovementInput(Yaw.RotateVector(FVector::ForwardVector), MoveInput.Y);
			AddMovementInput(Yaw.RotateVector(FVector::RightVector), MoveInput.X);
		}
		UpdateMoveSpeed();
		TickForcedMove(DeltaSeconds);
		UpdateBody(DeltaSeconds);
		TickSlide(DeltaSeconds);
		TickMantle(DeltaSeconds);
		TickInteract(DeltaSeconds);
		TickSafeGround(DeltaSeconds);
	}
	TickDash(DeltaSeconds);
	PingTimeLeft = FMath::Max(0.f, PingTimeLeft - DeltaSeconds);

	// Third person: the body turns by the facing rule instead of with the mouse; first person: as before.
	ApplyViewState();
	if (IsThirdPersonView() && !IsDead() && !bMantling)
	{
		UpdateFacing(DeltaSeconds);
	}
	UpdateBodyActions();
	UpdateHandBlade(DeltaSeconds);
	UpdateCloseCombat(DeltaSeconds);
	if (!IsDead())
	{
		UpdateFootsteps(DeltaSeconds);
	}

	// Mirror state for animation and HUD.
	BladeState = GetBladeState();
	UpdateBladePose(DeltaSeconds);
	bBlocking = Blade && Blade->IsBlocking();
	const float Horizontal = GetVelocity().Size2D();
	Speed01 = FMath::Clamp(Horizontal / FMath::Max(Tuning.SprintSpeed, 1.f), 0.f, 1.f);
}

// ------------------------------------------------------------------------------------------------
// Input (built in C++, no assets)
// ------------------------------------------------------------------------------------------------

namespace TDGroundInput
{
	static UInputAction* MakeAction(UObject* Outer, FName Name, EInputActionValueType Type)
	{
		UInputAction* A = NewObject<UInputAction>(Outer, Name);
		A->ValueType = Type;
		return A;
	}

	template <typename T>
	static T* AddModifier(FEnhancedActionKeyMapping& Mapping, UObject* Outer)
	{
		T* M = NewObject<T>(Outer);
		Mapping.Modifiers.Add(M);
		return M;
	}
}

void ATDFirstPersonCharacter::BuildInput()
{
	if (InputContext)
	{
		return;
	}
	using namespace TDGroundInput;
	using V = EInputActionValueType;
	IA_Move = MakeAction(this, TEXT("IA_Ground_Move"), V::Axis2D);
	IA_Look = MakeAction(this, TEXT("IA_Ground_Look"), V::Axis2D);
	IA_LookStick = MakeAction(this, TEXT("IA_Ground_LookStick"), V::Axis2D);
	IA_Jump = MakeAction(this, TEXT("IA_Ground_Jump"), V::Boolean);
	IA_Sprint = MakeAction(this, TEXT("IA_Ground_Sprint"), V::Boolean);
	IA_SprintToggle = MakeAction(this, TEXT("IA_Ground_SprintToggle"), V::Boolean);
	IA_Dash = MakeAction(this, TEXT("IA_Ground_Dash"), V::Boolean);
	IA_Crouch = MakeAction(this, TEXT("IA_Ground_Crouch"), V::Boolean);
	IA_Attack = MakeAction(this, TEXT("IA_Ground_Attack"), V::Boolean);
	IA_Block = MakeAction(this, TEXT("IA_Ground_Block"), V::Boolean);
	IA_Interact = MakeAction(this, TEXT("IA_Ground_Interact"), V::Boolean);
	IA_Pause = MakeAction(this, TEXT("IA_Ground_Pause"), V::Boolean);
	IA_Pause->bTriggerWhenPaused = true;
	IA_Ping = MakeAction(this, TEXT("IA_Ground_Ping"), V::Boolean);

	UInputMappingContext* C = NewObject<UInputMappingContext>(this, TEXT("IMC_Ground"));
	InputContext = C;

	// WASD -> (X right, Y forward)
	{
		FEnhancedActionKeyMapping& W = C->MapKey(IA_Move, EKeys::W);
		AddModifier<UInputModifierSwizzleAxis>(W, C)->Order = EInputAxisSwizzle::YXZ;
		FEnhancedActionKeyMapping& S = C->MapKey(IA_Move, EKeys::S);
		AddModifier<UInputModifierSwizzleAxis>(S, C)->Order = EInputAxisSwizzle::YXZ;
		AddModifier<UInputModifierNegate>(S, C);
		C->MapKey(IA_Move, EKeys::D);
		FEnhancedActionKeyMapping& A = C->MapKey(IA_Move, EKeys::A);
		AddModifier<UInputModifierNegate>(A, C);
		FEnhancedActionKeyMapping& Stick = C->MapKey(IA_Move, EKeys::Gamepad_Left2D);
		AddModifier<UInputModifierDeadZone>(Stick, C);
	}
	C->MapKey(IA_Look, EKeys::Mouse2D);
	C->MapKey(IA_LookStick, EKeys::Gamepad_Right2D); // dead zone, scale and invert come from the settings (ShapeLook)
	C->MapKey(IA_Jump, EKeys::SpaceBar);
	C->MapKey(IA_Jump, EKeys::Gamepad_FaceButton_Bottom);
	C->MapKey(IA_Sprint, EKeys::LeftShift);
	C->MapKey(IA_SprintToggle, EKeys::Gamepad_LeftThumbstick); // click to sprint (T1's pad pass); Shift is held
	C->MapKey(IA_Dash, EKeys::Q);
	C->MapKey(IA_Dash, EKeys::Gamepad_FaceButton_Right);
	C->MapKey(IA_Crouch, EKeys::LeftControl);
	C->MapKey(IA_Crouch, EKeys::Gamepad_RightThumbstick); // right-stick click (T1's audit: crouch had no pad binding)
	C->MapKey(IA_Attack, EKeys::LeftMouseButton);
	C->MapKey(IA_Attack, EKeys::Gamepad_RightTrigger);
	C->MapKey(IA_Block, EKeys::RightMouseButton);
	C->MapKey(IA_Block, EKeys::Gamepad_LeftTrigger);
	C->MapKey(IA_Interact, EKeys::E);
	C->MapKey(IA_Interact, EKeys::Gamepad_FaceButton_Left);
	C->MapKey(IA_Pause, EKeys::Escape);
	C->MapKey(IA_Pause, EKeys::Gamepad_Special_Right);
	C->MapKey(IA_Ping, EKeys::Tab);
	C->MapKey(IA_Ping, EKeys::Gamepad_DPad_Up);
}

void ATDFirstPersonCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BuildInput();
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("ATDFirstPersonCharacter needs an EnhancedInputComponent."));
		return;
	}
	EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &ATDFirstPersonCharacter::OnMove);
	EIC->BindAction(IA_Move, ETriggerEvent::Completed, this, &ATDFirstPersonCharacter::OnMoveReleased);
	EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ATDFirstPersonCharacter::OnLook);
	EIC->BindAction(IA_LookStick, ETriggerEvent::Triggered, this, &ATDFirstPersonCharacter::OnLookStick);
	EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &ATDFirstPersonCharacter::OnJumpPressed);
	EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &ATDFirstPersonCharacter::OnSprintPressed);
	EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &ATDFirstPersonCharacter::OnSprintReleased);
	EIC->BindAction(IA_SprintToggle, ETriggerEvent::Started, this, &ATDFirstPersonCharacter::OnSprintTogglePressed);
	EIC->BindAction(IA_Dash, ETriggerEvent::Started, this, &ATDFirstPersonCharacter::OnDashPressed);
	EIC->BindAction(IA_Crouch, ETriggerEvent::Started, this, &ATDFirstPersonCharacter::OnCrouchPressed);
	EIC->BindAction(IA_Attack, ETriggerEvent::Started, this, &ATDFirstPersonCharacter::OnAttackPressed);
	EIC->BindAction(IA_Attack, ETriggerEvent::Completed, this, &ATDFirstPersonCharacter::OnAttackReleased);
	EIC->BindAction(IA_Block, ETriggerEvent::Started, this, &ATDFirstPersonCharacter::OnBlockPressed);
	EIC->BindAction(IA_Block, ETriggerEvent::Completed, this, &ATDFirstPersonCharacter::OnBlockReleased);
	EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &ATDFirstPersonCharacter::OnInteractPressed);
	EIC->BindAction(IA_Interact, ETriggerEvent::Completed, this, &ATDFirstPersonCharacter::OnInteractReleased);
	EIC->BindAction(IA_Pause, ETriggerEvent::Started, this, &ATDFirstPersonCharacter::OnPausePressed);
	EIC->BindAction(IA_Ping, ETriggerEvent::Started, this, &ATDFirstPersonCharacter::OnPingPressed);
}

void ATDFirstPersonCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	BuildInput();
	const APlayerController* PC = Cast<APlayerController>(Controller);
	const ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP ? LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr)
	{
		// Rebuild now, not on the next deferred tick: keys work on the very first frame after a hand-over.
		FModifyContextOptions Now;
		Now.bForceImmediately = true;
		Subsystem->RemoveMappingContext(InputContext, Now);
		Subsystem->AddMappingContext(InputContext, 1, Now);
	}
	if (APlayerController* MyPC = Cast<APlayerController>(Controller); MyPC && MyPC->IsLocalController())
	{
		// Coming from a menu (UI-only input sticks to the viewport across level loads): back to gameplay input.
		MyPC->SetInputMode(FInputModeGameOnly());
		MyPC->SetShowMouseCursor(false);
	}
}

void ATDFirstPersonCharacter::UnPossessed()
{
	// Hand the keys back: the ship's controller must not see IMC_Ground (WASD / LMB / Space) any more.
	const APlayerController* PC = Cast<APlayerController>(Controller);
	const ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP ? LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr)
	{
		FModifyContextOptions Now;
		Now.bForceImmediately = true;
		Subsystem->RemoveMappingContext(InputContext, Now);
	}
	MoveInput = FVector2D::ZeroVector;
	bSprintHeld = false;
	bSprintToggled = false;
	if (Blade)
	{
		Blade->SetBlockHeld(false);
		Blade->ReleaseAttack();
	}
	CancelInteract();
	Super::UnPossessed();
}

void ATDFirstPersonCharacter::OnMove(const FInputActionValue& Value)
{
	SetMoveInput(Value.Get<FVector2D>());
}

FVector2D ATDFirstPersonCharacter::ShapeLook(const FVector2D& Raw, bool bGamepad, const FTDPlayerSettings& S)
{
	if (!bGamepad)
	{
		const float Sens = FMath::Max(0.f, S.MouseSensitivity);
		return FVector2D(Raw.X * Sens, -Raw.Y * Sens * (S.bInvertMouseY ? -1.f : 1.f));
	}
	// Radial dead zone, the rest rescaled to 0..1 so a small push past it is a small turn.
	const float Len = (float)Raw.Size();
	const float Dead = FMath::Clamp(S.GamepadDeadzone, 0.f, 0.95f);
	if (Len <= Dead || Len <= KINDA_SMALL_NUMBER)
	{
		return FVector2D::ZeroVector;
	}
	const FVector2D Live = Raw / Len * FMath::Min(1.f, (Len - Dead) / (1.f - Dead));
	const float Sens = StickLookScale * FMath::Max(0.f, S.GamepadSensitivity);
	return FVector2D(Live.X * Sens, -Live.Y * Sens * (S.bInvertGamepadY ? -1.f : 1.f));
}

void ATDFirstPersonCharacter::ApplyLookInput(const FVector2D& Raw, bool bGamepad)
{
	const UTDSettingsSubsystem* Options = UTDSettingsSubsystem::GetTDSettings(this);
	const FVector2D Look = ShapeLook(Raw, bGamepad, Options ? Options->GetSettings() : FTDPlayerSettings());
	AddControllerYawInput(Look.X);
	AddControllerPitchInput(Look.Y);
}

void ATDFirstPersonCharacter::OnLook(const FInputActionValue& Value)
{
	ApplyLookInput(Value.Get<FVector2D>(), false);
}

void ATDFirstPersonCharacter::OnLookStick(const FInputActionValue& Value)
{
	ApplyLookInput(Value.Get<FVector2D>(), true);
}

void ATDFirstPersonCharacter::OnAttackPressed()
{
	if (Blade && !bCarrying && !IsDead())
	{
		Blade->PressAttack();
	}
}

void ATDFirstPersonCharacter::OnAttackReleased()
{
	if (Blade)
	{
		Blade->ReleaseAttack();
	}
}

void ATDFirstPersonCharacter::OnBlockPressed()
{
	if (Blade && !bCarrying && !IsDead())
	{
		Blade->SetBlockHeld(true);
	}
}

void ATDFirstPersonCharacter::OnBlockReleased()
{
	if (Blade)
	{
		Blade->SetBlockHeld(false);
	}
}

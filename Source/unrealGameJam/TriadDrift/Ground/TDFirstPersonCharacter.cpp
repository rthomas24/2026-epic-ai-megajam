// Triad Drift - first-person character (ENG-21). Owner: Teammate 5.

#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/TDEnergyBladeComponent.h"
#include "TriadDrift/Ground/TDObjective.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDInterfaces.h"
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

	// Placeholder blade: a long thin glowing-white cube held low-right in view.
	BladePlaceholder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BladePlaceholder"));
	BladePlaceholder->SetupAttachment(Camera);
	BladePlaceholder->SetRelativeLocationAndRotation(FVector(45.f, 22.f, -24.f), FRotator(55.f, -10.f, 0.f));
	BladePlaceholder->SetRelativeScale3D(FVector(0.03f, 0.03f, 0.9f));
	BladePlaceholder->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BladePlaceholder->SetCastShadow(false);
	BladePlaceholder->SetOnlyOwnerSee(true);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		BladePlaceholder->SetStaticMesh(Cube.Object);
	}

	Health = CreateDefaultSubobject<UTDHealthComponent>(TEXT("Health"));
	Health->MaxHull = Tuning.MaxHealth;
	Health->MaxShield = 0.f;
	Health->HullRegenDelay = Tuning.HealthRegenDelay;
	Health->HullRegenPerSecond = Tuning.HealthRegenPerSecond;
	Health->ShieldRegenFractionPerSecond = 0.f;
	Health->DefaultTeam = ETDTeam::Player;

	Blade = CreateDefaultSubobject<UTDEnergyBladeComponent>(TEXT("Blade"));

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

	// Movement integrates after our Tick, so forced moves and slides set velocity deterministically.
	GetCharacterMovement()->PrimaryComponentTick.AddPrerequisite(this, PrimaryActorTick);
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
		return true;
	}
	if (JumpsUsed < 2)
	{
		// Falling off a ledge without jumping still allows exactly one air jump.
		LaunchCharacter(FVector(0.f, 0.f, Tuning.DoubleJumpZ), false, true);
		JumpsUsed = 2;
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
	if (IsDead() || bMantling || bForcedMove || MantleCooldown > 0.f)
	{
		return false;
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
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Health->Revive(1.f, 1.f);
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
	if (ShieldDamage + HullDamage > 0.f)
	{
		CancelInteract(); // holds are interrupted by damage
	}
}

void ATDFirstPersonCharacter::HandleDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow)
{
	CancelInteract();
	if (Blade)
	{
		Blade->CancelAll();
	}
	bForcedMove = bDashing = bSliding = bMantling = false;
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
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
	bSprinting = bSprintHeld && MoveInput.Y > 0.1f && !bCarrying && !bBladeBusy && !bSliding;
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
		TickSlide(DeltaSeconds);
		TickMantle(DeltaSeconds);
		TickInteract(DeltaSeconds);
	}
	TickDash(DeltaSeconds);
	PingTimeLeft = FMath::Max(0.f, PingTimeLeft - DeltaSeconds);

	// Mirror state for animation and HUD.
	BladeState = GetBladeState();
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
	IA_Jump = MakeAction(this, TEXT("IA_Ground_Jump"), V::Boolean);
	IA_Sprint = MakeAction(this, TEXT("IA_Ground_Sprint"), V::Boolean);
	IA_Dash = MakeAction(this, TEXT("IA_Ground_Dash"), V::Boolean);
	IA_Crouch = MakeAction(this, TEXT("IA_Ground_Crouch"), V::Boolean);
	IA_Attack = MakeAction(this, TEXT("IA_Ground_Attack"), V::Boolean);
	IA_Block = MakeAction(this, TEXT("IA_Ground_Block"), V::Boolean);
	IA_Interact = MakeAction(this, TEXT("IA_Ground_Interact"), V::Boolean);
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
	{
		FEnhancedActionKeyMapping& Stick = C->MapKey(IA_Look, EKeys::Gamepad_Right2D);
		AddModifier<UInputModifierDeadZone>(Stick, C);
		UInputModifierScalar* Scale = AddModifier<UInputModifierScalar>(Stick, C);
		Scale->Scalar = FVector(3.f, 3.f, 1.f);
	}
	C->MapKey(IA_Jump, EKeys::SpaceBar);
	C->MapKey(IA_Jump, EKeys::Gamepad_FaceButton_Bottom);
	C->MapKey(IA_Sprint, EKeys::LeftShift);
	C->MapKey(IA_Sprint, EKeys::Gamepad_LeftThumbstick);
	C->MapKey(IA_Dash, EKeys::Q);
	C->MapKey(IA_Dash, EKeys::Gamepad_FaceButton_Right);
	C->MapKey(IA_Crouch, EKeys::LeftControl);
	C->MapKey(IA_Attack, EKeys::LeftMouseButton);
	C->MapKey(IA_Attack, EKeys::Gamepad_RightTrigger);
	C->MapKey(IA_Block, EKeys::RightMouseButton);
	C->MapKey(IA_Block, EKeys::Gamepad_LeftTrigger);
	C->MapKey(IA_Interact, EKeys::E);
	C->MapKey(IA_Interact, EKeys::Gamepad_FaceButton_Left);
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
	EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &ATDFirstPersonCharacter::OnJumpPressed);
	EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &ATDFirstPersonCharacter::OnSprintPressed);
	EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &ATDFirstPersonCharacter::OnSprintReleased);
	EIC->BindAction(IA_Dash, ETriggerEvent::Started, this, &ATDFirstPersonCharacter::OnDashPressed);
	EIC->BindAction(IA_Crouch, ETriggerEvent::Started, this, &ATDFirstPersonCharacter::OnCrouchPressed);
	EIC->BindAction(IA_Attack, ETriggerEvent::Started, this, &ATDFirstPersonCharacter::OnAttackPressed);
	EIC->BindAction(IA_Attack, ETriggerEvent::Completed, this, &ATDFirstPersonCharacter::OnAttackReleased);
	EIC->BindAction(IA_Block, ETriggerEvent::Started, this, &ATDFirstPersonCharacter::OnBlockPressed);
	EIC->BindAction(IA_Block, ETriggerEvent::Completed, this, &ATDFirstPersonCharacter::OnBlockReleased);
	EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &ATDFirstPersonCharacter::OnInteractPressed);
	EIC->BindAction(IA_Interact, ETriggerEvent::Completed, this, &ATDFirstPersonCharacter::OnInteractReleased);
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
		Subsystem->RemoveMappingContext(InputContext);
		Subsystem->AddMappingContext(InputContext, 0);
	}
}

void ATDFirstPersonCharacter::OnMove(const FInputActionValue& Value)
{
	SetMoveInput(Value.Get<FVector2D>());
}

void ATDFirstPersonCharacter::OnLook(const FInputActionValue& Value)
{
	const FVector2D V = Value.Get<FVector2D>();
	AddControllerYawInput(V.X);
	AddControllerPitchInput(-V.Y);
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

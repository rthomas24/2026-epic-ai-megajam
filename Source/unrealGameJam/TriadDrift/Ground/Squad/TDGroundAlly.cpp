// Triad Drift - the two other pilots on foot beside the player (#209). Owner: ground squad builder.

#include "TriadDrift/Ground/Squad/TDGroundAlly.h"
#include "TriadDrift/Ground/Squad/TDGroundSquadSubsystem.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/TDEnergyBladeComponent.h"
#include "TriadDrift/Ground/TDPilotAnim.h"
#include "TriadDrift/Ground/Crew/TDFutureCrew.h"
#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Ground/Crew/TDCrewAIController.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/CollisionProfile.h"
#include "Misc/PackageName.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"

ATDGroundAlly::ATDGroundAlly()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(34.f, 90.f);
	// Solid to the level, the crew and their shots, but not a Pawn object: the player's chase camera pulls in for
	// pawns (crew pressed against the pilot), and it must never do that for a friend walking behind.
	Capsule->SetCollisionProfileName(TEXT("Pawn"));
	Capsule->SetCollisionObjectType(ECC_WorldDynamic);
	Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(TDCollision::Interactable, ECR_Ignore); // never between the player and a console
	Capsule->SetCanEverAffectNavigation(false);

	Health = CreateDefaultSubobject<UTDHealthComponent>(TEXT("Health"));
	Health->MaxHull = MaxHealth;
	Health->MaxShield = 0.f;
	Health->ShieldRegenFractionPerSecond = 0.f;
	Health->HullRegenDelay = HealthRegenDelay;
	Health->HullRegenPerSecond = HealthRegenPerSecond;
	Health->DefaultTeam = ETDTeam::Player;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = RunSpeed;
	Move->MaxAcceleration = 5000.f;
	Move->BrakingDecelerationWalking = 2600.f;
	Move->GroundFriction = 8.f;
	Move->AirControl = 0.5f;
	Move->GravityScale = 1.6f;
	Move->JumpZVelocity = 650.f;
	Move->MaxStepHeight = 55.f;
	Move->SetWalkableFloorAngle(50.f);
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 540.f, 0.f);
	Move->bCanWalkOffLedges = true;
	bUseControllerRotationYaw = false;

	GetMesh()->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	GetMesh()->SetCanEverAffectNavigation(false);

	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ATDGroundAlly::BeginPlay()
{
	Super::BeginPlay();
	Health->InitMaxValues(MaxHealth, 0.f, true);
	Health->HullRegenDelay = HealthRegenDelay;
	Health->HullRegenPerSecond = HealthRegenPerSecond;
	Health->DamageFilter.BindUObject(this, &ATDGroundAlly::FilterDamage);
	Health->OnDeath.AddUniqueDynamic(this, &ATDGroundAlly::HandleDeath);
	ApplyLook();
}

void ATDGroundAlly::SetPilot(FName InPilot, int32 InSlot)
{
	Pilot = InPilot;
	Slot = InSlot;
	if (HasActorBegunPlay())
	{
		ApplyLook();
	}
}

ETDShipId ATDGroundAlly::GetShipId() const
{
	return TDFutureCrew::ShipOf(Pilot);
}

FLinearColor ATDGroundAlly::GetSquadColor() const
{
	return UTDStatics::ShipColor(GetShipId());
}

float ATDGroundAlly::GetHealth01() const
{
	return IsDown() || !Health ? 0.f : Health->GetHull01();
}

void ATDGroundAlly::ApplyLook()
{
	USkeletalMeshComponent* Body = GetMesh();
	if (!Body)
	{
		return;
	}
	const FLinearColor Color = GetSquadColor();
	if (!bLookApplied)
	{
		// The pilot's own body, loops and grip (T5's shared future-pilot look: the same rig the player wears).
		if (TDFutureCrew::ApplyPilotLook(Body, GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()))
		{
			Body->SetCastShadow(true);
			BladeBeam = TDFutureCrew::AttachBlade(this, Body, Color);
			// The player's own beam (core and glow shell, #163) when it is on disk: the same blade, another colour.
			UStaticMesh* V2 = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/TriadDrift/Gameplay/Props/Blade/SM_TD_BladeBeam_V2.SM_TD_BladeBeam_V2"), nullptr, LOAD_Quiet | LOAD_NoWarn);
			if (BladeBeam && V2 && BladeBeam->GetStaticMesh() && BladeBeam->GetStaticMesh()->GetName() == TEXT("SM_TD_BladeBeam"))
			{
				BladeBeam->SetStaticMesh(V2);
				const float Authored = FMath::Max(1.f, (float)V2->GetBoundingBox().GetSize().X);
				BladeBeam->SetRelativeScale3D(FVector(GetDefault<ATDFirstPersonCharacter>()->HandBladeLength / Authored, 1.f, 1.f));
			}
		}
		// The suit's squad colour. The pilot material has no tint parameter today (textures only), so the colour is
		// worn as lit shoulder plates and a back light; the parameters below take over the moment the material gets them.
		UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		struct FAccent { const TCHAR* Bone; FVector Scale; };
		const FAccent Accents[] = {
			{ TEXT("upperarm_l"), FVector(0.17f, 0.13f, 0.13f) },
			{ TEXT("upperarm_r"), FVector(0.17f, 0.13f, 0.13f) },
			{ TEXT("spine_03"), FVector(0.10f, 0.22f, 0.10f) } };
		for (const FAccent& A : Accents)
		{
			if (!Sphere || Body->GetBoneIndex(A.Bone) == INDEX_NONE)
			{
				continue;
			}
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
			C->SetStaticMesh(Sphere);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCanEverAffectNavigation(false);
			C->SetCastShadow(false);
			C->SetupAttachment(Body, A.Bone);
			const float Inv = 1.f / FMath::Max(0.01f, (float)Body->GetComponentScale().X);
			C->SetRelativeScale3D(A.Scale * Inv);
			C->RegisterComponent();
			AddInstanceComponent(C);
			SuitAccents.Add(C);
		}
		bLookApplied = true;
	}
	// The pilot's own suit when the animation builder's instance is on disk (MI_Pilot_Wren / _Kato / _Moss; the folder
	// is always cooked, so a load by name ships). Then the lit plates are not needed.
	const FString SuitPackage = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Characters/Pilot/MI_Pilot_%s"), *Pilot.ToString());
	SuitMaterial = (FindPackage(nullptr, *SuitPackage) || FPackageName::DoesPackageExist(SuitPackage))
		? LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("%s.MI_Pilot_%s"), *SuitPackage, *Pilot.ToString()), nullptr, LOAD_Quiet | LOAD_NoWarn) : nullptr;
	if (SuitMaterial)
	{
		Body->SetMaterial(0, SuitMaterial);
	}
	for (UStaticMeshComponent* C : SuitAccents)
	{
		UTDStatics::TintPlaceholder(C, Color, 2.2f);
		C->SetVisibility(SuitMaterial == nullptr);
	}
	for (int32 i = 0; i < Body->GetNumMaterials(); ++i)
	{
		if (UMaterialInstanceDynamic* MID = Body->CreateDynamicMaterialInstance(i))
		{
			MID->SetVectorParameterValue(TEXT("SuitTint"), Color);
			MID->SetVectorParameterValue(TEXT("SuitAccent"), Color);
		}
	}
	if (BladeBeam)
	{
		for (int32 i = 0; i < BladeBeam->GetNumMaterials(); ++i)
		{
			if (UMaterialInstanceDynamic* MID = BladeBeam->CreateDynamicMaterialInstance(i))
			{
				MID->SetVectorParameterValue(TEXT("BladeColor"), Color);
			}
		}
	}
}

UAnimSequenceBase* ATDGroundAlly::Clip(std::initializer_list<const TCHAR*> Names)
{
	for (const TCHAR* Name : Names)
	{
		const FName Key(Name);
		if (TObjectPtr<UAnimSequenceBase>* Known = Clips.Find(Key))
		{
			if (*Known)
			{
				return *Known;
			}
			continue; // looked for once, not on disk
		}
		UAnimSequenceBase* Seq = TDFutureCrew::PilotAnim(Name);
		Seq = Seq ? TDMakePlayable(Seq) : nullptr;
		Clips.Add(Key, Seq);
		if (Seq)
		{
			return Seq;
		}
	}
	return nullptr;
}

UTDLoopBlendAnimInstance* ATDGroundAlly::GetLoops() const
{
	return GetMesh() ? Cast<UTDLoopBlendAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr;
}

bool ATDGroundAlly::PlayShot(std::initializer_list<const TCHAR*> Names, float Rate, bool bUpperOnly, bool bHold)
{
	UTDLoopBlendAnimInstance* Loops = GetLoops();
	UAnimSequenceBase* Seq = Loops ? Clip(Names) : nullptr;
	if (!Seq)
	{
		return false;
	}
	Loops->PlayOneShot(Seq, Rate, bUpperOnly, bHold);
	return true;
}

void ATDGroundAlly::UpdateBody(float DeltaSeconds)
{
	UTDLoopBlendAnimInstance* Loops = GetLoops();
	if (!Loops || IsDown())
	{
		return;
	}
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	const float Speed = (float)GetVelocity().Size2D();
	const bool bAir = Move && Move->IsFalling();
	const ATDFirstPersonCharacter* Defaults = GetDefault<ATDFirstPersonCharacter>();
	const int32 Want = ATDFirstPersonCharacter::ChooseBodyLoop(Speed);
	UAnimSequenceBase* Loop = nullptr;
	float Rate = 1.f;
	if (bAir)
	{
		Loop = Clip({ TEXT("A_Pilot_FallLoop_v2"), TEXT("A_Pilot_FallLoop") });
	}
	else if (Want == 0)
	{
		// Fighting: the ready guard the swings start from; otherwise the low side guard the walk blends out of.
		Loop = State == ETDGroundAllyState::Fight
			? Clip({ TEXT("A_Pilot_Blade_Idle_v3"), TEXT("A_Pilot_Blade_Idle_v2"), TEXT("A_Pilot_Blade_Idle") })
			: Clip({ TEXT("A_Pilot_Idle3_v4"), TEXT("A_Pilot_Idle3_v3"), TEXT("A_Pilot_Blade_Idle") });
	}
	else if (Want == 1)
	{
		Loop = Clip({ TEXT("A_Pilot_Walk_v5"), TEXT("A_Pilot_Walk_v4"), TEXT("A_Pilot_Walk_v3"), TEXT("A_Pilot_Walk_v2") });
		Rate = FMath::Clamp(Speed / FMath::Max(1.f, Defaults->BodyWalkSpeed), 0.6f, 2.f);
	}
	else
	{
		UAnimSequenceBase* Sprint = Speed > 820.f ? Clip({ TEXT("A_Pilot_Sprint") }) : nullptr;
		Loop = Sprint ? Sprint : Clip({ TEXT("A_Pilot_Run_v5"), TEXT("A_Pilot_Run_v4"), TEXT("A_Pilot_Run_v3"), TEXT("A_Pilot_Run_v2") });
		Rate = Sprint ? FMath::Clamp(Speed / 900.f, 0.8f, 1.3f) : FMath::Clamp(Speed / FMath::Max(1.f, Defaults->BodyRunSpeed), 0.5f, 1.6f);
	}
	if (Loop)
	{
		Loops->SetLoop(Loop, Rate);
	}
	// The closed hand on the hilt. The loop player only closes it for the player's own pawn class, so it is fed from
	// here each frame (it eases back out by itself if this stops).
	if (BladeBeam)
	{
		if (!Loops->GripPose && !Loops->bGripPoseTried)
		{
			Loops->bGripPoseTried = true;
			Loops->GripPose = Loops->GripPoseAsset.IsNull() ? nullptr : TDLoadPlayableAnim(Loops->GripPoseAsset.ToSoftObjectPath());
		}
		if (Loops->GripPose)
		{
			Loops->GripAlpha = 1.f;
		}
	}
	const int32 Gait = bAir ? -1 : Want;
	if (Gait != LastGait)
	{
		LastGait = Gait;
		Loops->SetGait(Gait);
	}
}

void ATDGroundAlly::UpdateCameraFade()
{
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC || !PC->PlayerCameraManager || !GetMesh())
	{
		return;
	}
	// Never a suit filling the frame: the body (and its blade and plates) is not drawn while the lens is on top of it.
	const FVector Cam = PC->PlayerCameraManager->GetCameraLocation();
	const FVector Me = GetActorLocation();
	const float Flat = (float)FVector::Dist2D(Cam, Me);
	const bool bInside = Flat < CameraHideDistance * (bBodyHidden ? 1.25f : 1.f) && FMath::Abs(Cam.Z - Me.Z) < 160.f;
	if (bInside != bBodyHidden)
	{
		bBodyHidden = bInside;
		GetMesh()->SetVisibility(!bInside, true);
		if (!bInside)
		{
			// Showing everything again must not bring back what was off on purpose.
			for (UStaticMeshComponent* C : SuitAccents)
			{
				if (C) { C->SetVisibility(SuitMaterial == nullptr); }
			}
			if (BladeBeam && IsDown())
			{
				BladeBeam->SetVisibility(false);
			}
		}
	}
}

UTDGroundSquadSubsystem* ATDGroundAlly::GetSquad() const
{
	return UTDGroundSquadSubsystem::Get(this);
}

ATDFirstPersonCharacter* ATDGroundAlly::GetLeader() const
{
	const UTDGroundSquadSubsystem* Squad = GetSquad();
	return Squad ? Squad->GetLeader() : nullptr;
}

void ATDGroundAlly::FaceYaw(float Yaw, float DeltaSeconds, float Rate)
{
	const FRotator Now = GetActorRotation();
	SetActorRotation(FRotator(0.f, FMath::FixedTurn(Now.Yaw, Yaw, Rate * DeltaSeconds), 0.f));
}

// ------------------------------------------------------------------------------------------------ tick

void ATDGroundAlly::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (DeltaSeconds <= 0.f)
	{
		return;
	}
	SwingCooldown = FMath::Max(0.f, SwingCooldown - DeltaSeconds);
	DeflectLeft = FMath::Max(0.f, DeflectLeft - DeltaSeconds);
	IgnoreTargetLeft = FMath::Max(0.f, IgnoreTargetLeft - DeltaSeconds);
	AvoidLeft = FMath::Max(0.f, AvoidLeft - DeltaSeconds);
	JumpLeft = FMath::Max(0.f, JumpLeft - DeltaSeconds);

	ATDFirstPersonCharacter* Leader = GetLeader();
	if (State == ETDGroundAllyState::Down)
	{
		TickDown(DeltaSeconds);
	}
	else if (Leader)
	{
		// Far behind (a lift, a drop, a door that closed): rejoin at once.
		if (FVector::Dist(Leader->GetActorLocation(), GetActorLocation()) > TeleportDistance)
		{
			TeleportToLeader();
		}
		if (State == ETDGroundAllyState::Boarding)
		{
			GetCharacterMovement()->bOrientRotationToMovement = true;
			MoveToward(Leader->GetActorLocation(), 120.f, true, DeltaSeconds);
		}
		else
		{
			SenseLeft -= DeltaSeconds;
			if (SenseLeft <= 0.f)
			{
				SenseLeft = 0.25f;
				PickTarget(Leader);
			}
			if (FightTarget.IsValid())
			{
				State = ETDGroundAllyState::Fight;
				TickFight(DeltaSeconds, Leader);
			}
			else
			{
				State = ETDGroundAllyState::Follow;
				PendingHitLeft = -1.f;
				TickFollow(DeltaSeconds, Leader);
			}
		}
	}
	UpdateBody(DeltaSeconds);
	UpdateCameraFade();
}

void ATDGroundAlly::TickFollow(float DeltaSeconds, ATDFirstPersonCharacter* Leader)
{
	UTDGroundSquadSubsystem* Squad = GetSquad();
	if (!Squad)
	{
		return;
	}
	GetCharacterMovement()->bOrientRotationToMovement = true;
	FVector Goal = Squad->GetFormationSlot(Slot);
	const float ToSlot = (float)FVector::Dist2D(Goal, GetActorLocation());
	// Hysteresis: stand until the slot has moved away, then walk until back on it.
	// While the pilot is on the move the ally goes with the first step, not once the slot has run away from it.
	const bool bLeaderMoving = Leader->GetVelocity().Size2D() > 100.f;
	if (bHolding && ToSlot > (bLeaderMoving ? SlotArrive + 15.f : SlotLeave))
	{
		bHolding = false;
	}
	else if (!bHolding && ToSlot < SlotArrive)
	{
		bHolding = true;
	}
	FVector Aside;
	const bool bAside = FindStepAside(Leader, Aside);
	bool bWants = false;
	if (bAside)
	{
		bWants = MoveToward(Aside, 20.f, false, DeltaSeconds); // out of the player's way first
	}
	else if (!bHolding)
	{
		bWants = MoveToward(Goal, SlotArrive * 0.6f, ToSlot > (bLeaderMoving ? 260.f : SprintDistance), DeltaSeconds);
	}
	else if (GetVelocity().Size2D() < 20.f)
	{
		FaceYaw(Squad->GetFormationYaw(), DeltaSeconds, 180.f); // standing: look where the player looks
	}
	// Stuck behind something, or on another floor from the slot: rejoin behind the player.
	const bool bStuck = bWants && GetVelocity().Size2D() < 40.f;
	const bool bWrongFloor = FMath::Abs(Goal.Z - GetActorLocation().Z) > 260.f && ToSlot < 300.f;
	StuckTime = (bStuck || bWrongFloor) ? StuckTime + DeltaSeconds : FMath::Max(0.f, StuckTime - DeltaSeconds * 2.f);
	if (StuckTime >= StuckSeconds)
	{
		TeleportToLeader();
	}
}

bool ATDGroundAlly::FindStepAside(const ATDFirstPersonCharacter* Leader, FVector& OutGoal) const
{
	if (!Leader)
	{
		return false;
	}
	const FVector Me = GetActorLocation();
	const FVector L = Leader->GetActorLocation();
	auto OffLine = [&](const FVector& To, float Width) -> bool
	{
		const FVector2D A(L.X, L.Y), B(To.X, To.Y), P(Me.X, Me.Y);
		const FVector2D AB = B - A;
		const float Len2 = (float)AB.SizeSquared();
		if (Len2 < 100.f)
		{
			return false;
		}
		const float T = FMath::Clamp((float)FVector2D::DotProduct(P - A, AB) / Len2, 0.f, 1.f);
		const FVector2D Closest = A + AB * T;
		const FVector2D Off = P - Closest;
		if (T <= 0.02f || Off.Size() >= Width)
		{
			return false;
		}
		// Straight off the line, on the side the ally already leans to (its slot side when dead centre).
		FVector2D Dir = Off.GetSafeNormal();
		if (Dir.IsNearlyZero())
		{
			const FVector2D N = AB.GetSafeNormal();
			Dir = FVector2D(-N.Y, N.X) * (Slot == 0 ? -1.f : 1.f);
		}
		OutGoal = FVector(Closest.X + Dir.X * (Width + 60.f), Closest.Y + Dir.Y * (Width + 60.f), Me.Z);
		return true;
	};
	// The path the player is walking (the next 3.5 m).
	const FVector Vel = Leader->GetVelocity();
	if (Vel.Size2D() > 60.f && OffLine(L + Vel.GetSafeNormal2D() * 350.f, PathClearance))
	{
		return true;
	}
	// What the player is looking at to use (a console, a core, the ship's hatch).
	if (const AActor* Use = Leader->GetInteractTarget())
	{
		if (FVector::Dist2D(Use->GetActorLocation(), L) < 600.f && OffLine(Use->GetActorLocation(), PathClearance))
		{
			return true;
		}
	}
	return false;
}

bool ATDGroundAlly::MoveToward(const FVector& Goal, float StopRadius, bool bSprint, float DeltaSeconds)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	const FVector Me = GetActorLocation();
	FVector To = Goal - Me;
	To.Z = 0.f;
	const float Dist = (float)To.Size();
	if (Dist <= StopRadius)
	{
		return false;
	}
	// Ease into the stop so the ally does not overshoot and shuffle.
	// Keeps pace with the pilot (their speed, plus what closes the gap) rather than trailing and catching up.
	const ATDFirstPersonCharacter* Pace = State == ETDGroundAllyState::Follow ? GetLeader() : nullptr;
	const float PaceSpeed = Pace ? (float)Pace->GetVelocity().Size2D() : 0.f;
	Move->MaxWalkSpeed = FMath::Clamp(PaceSpeed + (Dist - StopRadius) * 3.f + 160.f, 160.f, bSprint ? FMath::Max(SprintSpeed, PaceSpeed + 300.f) : FMath::Max(RunSpeed, PaceSpeed + 260.f));
	FVector Dir = To / Dist;
	// Feel the way: a wall ahead turns the ally along it; a low step ahead is jumped.
	if (AvoidLeft <= 0.f)
	{
		AvoidLeft = 0.15f;
		AvoidSign = 0.f;
		const UWorld* World = GetWorld();
		FCollisionQueryParams Params(SCENE_QUERY_STAT(TDGroundAllyFeel), false, this);
		if (const AActor* L = GetLeader())
		{
			Params.AddIgnoredActor(L);
		}
		FCollisionObjectQueryParams Solid;
		Solid.AddObjectTypesToQuery(ECC_WorldStatic);
		Solid.AddObjectTypesToQuery(ECC_WorldDynamic);
		const float Reach = 110.f;
		auto Blocked = [&](const FVector& D, float Height) -> bool
		{
			FHitResult Hit;
			const FVector From = Me + FVector(0.f, 0.f, Height);
			return World && World->LineTraceSingleByObjectType(Hit, From, From + D * Reach, Solid, Params) && Hit.GetActor() != FightTarget.Get();
		};
		if (Blocked(Dir, -30.f) || Blocked(Dir, 30.f))
		{
			const FVector Left = Dir.RotateAngleAxis(-55.f, FVector::UpVector);
			const FVector Right = Dir.RotateAngleAxis(55.f, FVector::UpVector);
			const bool bLeftFree = !Blocked(Left, 0.f);
			const bool bRightFree = !Blocked(Right, 0.f);
			AvoidSign = bLeftFree && (!bRightFree || Slot == 0) ? -1.f : bRightFree ? 1.f : 0.f;
			// Knee-high and clear at head height: hop it.
			if (!Blocked(Dir, 70.f) && JumpLeft <= 0.f && Move->IsMovingOnGround())
			{
				Jump();
				JumpLeft = 0.8f;
			}
		}
	}
	if (AvoidSign != 0.f)
	{
		Dir = Dir.RotateAngleAxis(55.f * AvoidSign, FVector::UpVector);
	}
	AddMovementInput(Dir, 1.f);
	return true;
}

// ------------------------------------------------------------------------------------------------ fight

bool ATDGroundAlly::HasClearLine(const AActor* To) const
{
	const UWorld* World = GetWorld();
	if (!World || !To)
	{
		return false;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDGroundAllySight), false, this);
	Params.AddIgnoredActor(To);
	FHitResult Hit;
	const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, 50.f);
	return !World->LineTraceSingleByChannel(Hit, Eye, To->GetActorLocation() + FVector(0.f, 0.f, 20.f), ECC_Visibility, Params);
}

bool ATDGroundAlly::IsTargetUsable(const AActor* Target, const ATDFirstPersonCharacter* Leader, float Slack) const
{
	const UTDHealthComponent* TH = Target ? UTDStatics::GetHealth(Target) : nullptr;
	if (!TH || TH->IsDead() || Target->IsHidden() || !Leader)
	{
		return false;
	}
	const FVector At = Target->GetActorLocation();
	return FVector::Dist(At, GetActorLocation()) <= EngageRange * Slack
		&& FVector::Dist(At, Leader->GetActorLocation()) <= LeashRange * Slack
		&& FMath::Abs(At.Z - GetActorLocation().Z) <= 450.f;
}

bool ATDGroundAlly::IsPlayersToFinish(const AActor* Target) const
{
	if (const ATDCrewCharacter* Crew = Cast<ATDCrewCharacter>(Target))
	{
		if (Crew->CrewType == ETDCrewType::Heavy || Crew->CrewType == ETDCrewType::Warden || Crew->CrewType == ETDCrewType::FutureCrew)
		{
			return true;
		}
	}
	// The last one standing near the player.
	const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this);
	const ATDFirstPersonCharacter* Leader = GetLeader();
	if (!Registry || !Leader)
	{
		return false;
	}
	TArray<AActor*> Hostiles;
	Registry->GetHostilesOf(ETDTeam::Player, Hostiles, true);
	int32 Standing = 0;
	for (const AActor* H : Hostiles)
	{
		if (H && H->IsA<APawn>() && FVector::Dist(H->GetActorLocation(), Leader->GetActorLocation()) < 4000.f)
		{
			++Standing;
		}
	}
	return Standing <= 1;
}

void ATDGroundAlly::PickTarget(ATDFirstPersonCharacter* Leader)
{
	// Keep the one being fought while it stays in reach (a little slack so it does not flicker at the edge).
	if (AActor* Current = FightTarget.Get())
	{
		// Two on one: the second ally looks again, and moves to an enemy nobody is on if there is one.
		const bool bShared = Slot != 0 && GetSquad() && GetSquad()->IsTargetTaken(Current, this) && PendingHitLeft < 0.f;
		if (IsTargetUsable(Current, Leader, 1.2f) && !bShared)
		{
			return;
		}
		if (!IsTargetUsable(Current, Leader, 1.2f))
		{
			FightTarget = nullptr;
		}
	}
	const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this);
	const UTDGroundSquadSubsystem* Squad = GetSquad();
	if (!Registry || !Squad)
	{
		return;
	}
	TArray<AActor*> Hostiles;
	Registry->GetHostilesOf(ETDTeam::Player, Hostiles);
	AActor* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	// The enemy nearest the player is the player's: taken only when it is the only one.
	const AActor* PlayersEnemy = nullptr;
	float PlayersBest = TNumericLimits<float>::Max();
	for (const AActor* H : Hostiles)
	{
		const float D = H && H->IsA<APawn>() ? (float)FVector::Dist(H->GetActorLocation(), Leader->GetActorLocation()) : TNumericLimits<float>::Max();
		if (D < PlayersBest)
		{
			PlayersBest = D;
			PlayersEnemy = H;
		}
	}
	for (AActor* H : Hostiles)
	{
		if (!H || !H->IsA<APawn>() || !IsTargetUsable(H, Leader, 1.f) || (IgnoreTargetLeft > 0.f && H == IgnoredTarget.Get()))
		{
			continue;
		}
		// Each ally its own enemy: one the other ally is on is only taken when nothing else is in reach.
		float Score = (float)FVector::Dist(H->GetActorLocation(), GetActorLocation());
		if (Squad->IsTargetTaken(H, this))
		{
			Score += 100000.f;
		}
		if (H == PlayersEnemy)
		{
			Score += PlayersEnemyHandicap;
		}
		if (Score < BestScore && HasClearLine(H))
		{
			BestScore = Score;
			Best = H;
		}
	}
	if (Best && Best != FightTarget.Get())
	{
		FightTarget = Best;
		if (UTDGroundSquadSubsystem* Mine = GetSquad())
		{
			Mine->NotifyAllyEvent(this, ETDGroundSquadEvent::Engaged, Best);
		}
		StuckTime = 0.f;
		ComboStep = 0;
		SwingCooldown = FMath::Max(SwingCooldown, 0.25f);
	}
}

void ATDGroundAlly::TickFight(float DeltaSeconds, ATDFirstPersonCharacter* Leader)
{
	AActor* Target = FightTarget.Get();
	if (!Target)
	{
		return;
	}
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = false;
	const FVector Me = GetActorLocation();
	FVector To = Target->GetActorLocation() - Me;
	To.Z = 0.f;
	const float Dist = (float)To.Size();
	const float TargetRadius = Target->GetSimpleCollisionRadius();
	const TArray<FTDBladeSwing>& Combo = GetDefault<UTDEnergyBladeComponent>()->LightCombo;
	const float Range = (Combo.Num() > 0 ? Combo[0].Range : 180.f) * 0.8f + TargetRadius;
	FaceYaw(To.Rotation().Yaw, DeltaSeconds, 720.f);

	const bool bWants = MoveToward(Target->GetActorLocation(), FMath::Max(60.f, Range - 35.f), Dist > SprintDistance, DeltaSeconds);
	// Cannot get to it (a ledge, a wall): leave it for a while and go back to the player.
	StuckTime = (bWants && GetVelocity().Size2D() < 40.f) ? StuckTime + DeltaSeconds : FMath::Max(0.f, StuckTime - DeltaSeconds * 2.f);
	if (StuckTime >= StuckSeconds)
	{
		IgnoredTarget = Target;
		IgnoreTargetLeft = 6.f;
		FightTarget = nullptr;
		StuckTime = 0.f;
		return;
	}
	if (PendingHitLeft >= 0.f)
	{
		PendingHitLeft -= DeltaSeconds;
		if (PendingHitLeft < 0.f)
		{
			LandSwing();
		}
		return;
	}
	const float Facing = FMath::Abs(FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, To.Rotation().Yaw));
	if (Dist <= Range + 15.f && Facing < 35.f && SwingCooldown <= 0.f && !Move->IsFalling())
	{
		StartSwing();
	}
}

void ATDGroundAlly::StartSwing()
{
	const TArray<FTDBladeSwing>& Combo = GetDefault<UTDEnergyBladeComponent>()->LightCombo;
	ComboStep = (ComboStep % 3) + 1;
	const bool bMoving = GetVelocity().Size2D() > 120.f;
	switch (ComboStep)
	{
	case 1: PlayShot({ TEXT("A_Pilot_Light1_v3"), TEXT("A_Pilot_Light1") }, 1.6f, bMoving); break;
	case 2: PlayShot({ TEXT("A_Pilot_Light2_v3"), TEXT("A_Pilot_Light2") }, 1.6f, bMoving); break;
	default: PlayShot({ TEXT("A_Pilot_Light3_v3"), TEXT("A_Pilot_Light3") }, 1.5f, bMoving); break;
	}
	PendingHitLeft = HitDelay;
	const float Lock = Combo.IsValidIndex(ComboStep - 1) ? Combo[ComboStep - 1].Duration : 0.28f;
	SwingCooldown = Lock + (ComboStep == 3 ? ComboRest : FMath::FRandRange(SwingGap.X, SwingGap.Y));
	if (const UTDBladeFX* FX = UTDStatics::LoadPresentation<UTDBladeFX>(TEXT("DA_Blade_FX")))
	{
		UTDStatics::PlayCue(this, FX->SwingLight, GetActorLocation() + GetActorForwardVector() * 60.f, GetActorRotation());
	}
}

void ATDGroundAlly::LandSwing()
{
	AActor* Target = FightTarget.Get();
	const UTDHealthComponent* TH = Target ? UTDStatics::GetHealth(Target) : nullptr;
	if (!TH || TH->IsDead())
	{
		return;
	}
	const TArray<FTDBladeSwing>& Combo = GetDefault<UTDEnergyBladeComponent>()->LightCombo;
	const FTDBladeSwing Swing = Combo.IsValidIndex(ComboStep - 1) ? Combo[ComboStep - 1] : FTDBladeSwing();
	const FVector Me = GetActorLocation();
	FVector To = Target->GetActorLocation() - Me;
	const float Dist = (float)To.Size2D();
	const FVector Dir = To.GetSafeNormal2D();
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp((float)FVector::DotProduct(GetActorForwardVector(), Dir), -1.f, 1.f)));
	if (Dist > Swing.Range + Target->GetSimpleCollisionRadius() || Angle > Swing.ArcDegrees * 0.5f + 10.f)
	{
		return; // it stepped out of the arc: a miss
	}
	const FVector HitAt = Target->GetActorLocation() - Dir * Target->GetSimpleCollisionRadius();
	// No instigator on purpose (the causer carries the team): the kill is nobody's, so the director's money shots,
	// the kill stats, the pilot's own kill barks, hit-stop and hit markers never come from a follower's blade.
	float Damage = Swing.Damage * DamageScale;
	if (bLeaveFinishers && IsPlayersToFinish(Target))
	{
		Damage = FMath::Min(Damage, TH->GetHull() + TH->GetShield() - FinishHull); // held on its last point: the player's kill
	}
	const float Dealt = Damage > 0.f ? UTDStatics::ApplyTDDamage(Target, Damage, ETDDamageKind::Melee, nullptr, this, HitAt, Dir) : 0.f;
	if (Dealt > 0.f)
	{
		++SwingsLanded;
		if (const UTDBladeFX* FX = UTDStatics::LoadPresentation<UTDBladeFX>(TEXT("DA_Blade_FX")))
		{
			UTDStatics::PlayCue(this, FX->Hit, HitAt, Dir.Rotation());
		}
	}
	if (TH->IsDead())
	{
		++Kills;
		FightTarget = nullptr;
		SenseLeft = 0.f;
		if (UTDGroundSquadSubsystem* Squad = GetSquad())
		{
			Squad->NotifyAllyEvent(this, ETDGroundSquadEvent::Kill, Target);
		}
	}
}

// ------------------------------------------------------------------------------------------------ damage, down, up

float ATDGroundAlly::FilterDamage(const FTDDamageInfo& Info)
{
	if (IsDown())
	{
		return 0.f;
	}
	// A bolt arriving in the front arc is sometimes turned aside with the blade.
	const bool bBolt = Info.Kind != ETDDamageKind::Melee && Info.Kind != ETDDamageKind::Environmental && Info.Causer && Info.Causer != Info.Instigator;
	if (bBolt && DeflectLeft <= 0.f && PendingHitLeft < 0.f && FMath::FRand() < DeflectChance)
	{
		const FVector From = Info.Instigator ? Info.Instigator->GetActorLocation() : Info.HitLocation - Info.HitDirection * 100.f;
		const FVector To = (From - GetActorLocation()).GetSafeNormal2D();
		if (FVector::DotProduct(To, GetActorForwardVector()) > 0.3f)
		{
			DeflectLeft = DeflectCooldown;
			++Deflects;
			PlayShot({ TEXT("A_Pilot_Blade_DeflectFlick_v3"), TEXT("A_Pilot_Blade_DeflectFlick_v2"), TEXT("A_Pilot_Blade_DeflectFlick") }, 1.f, true);
			if (const UTDBladeFX* FX = UTDStatics::LoadPresentation<UTDBladeFX>(TEXT("DA_Blade_FX")))
			{
				UTDStatics::PlayCue(this, FX->Deflect, GetActorLocation() + To * 60.f + FVector(0.f, 0.f, 30.f), To.Rotation());
			}
			return 0.f;
		}
	}
	// The blow that would kill puts the ally down instead: it never dies, so nothing that listens for a death on the
	// player's team (CUT, retakes, squad wipes) ever hears one from an ally.
	if (Health && Info.Amount >= Health->GetHull())
	{
		GoDown();
		return 0.f;
	}
	if (PendingHitLeft < 0.f && Info.Amount >= 8.f)
	{
		PlayShot({ TEXT("A_Pilot_HitReact_Front_v3"), TEXT("A_Pilot_HitReact_Front") }, 1.2f, true);
	}
	return Info.Amount;
}

void ATDGroundAlly::HandleDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow)
{
	// Only an outright Kill() gets here (a kill volume, a cheat): the same as going down.
	if (!IsDown())
	{
		GoDown();
	}
}

void ATDGroundAlly::GoDown()
{
	if (IsDown())
	{
		return;
	}
	State = ETDGroundAllyState::Down;
	DownLeft = DownSeconds;
	++TimesDown;
	FightTarget = nullptr;
	PendingHitLeft = -1.f;
	if (Health)
	{
		Health->SetInvulnerable(true);
		Health->SetTargetable(false);
	}
	GetCharacterMovement()->StopMovementImmediately();
	// The pilot's death clip, held on its last frame: down on the floor, no ragdoll.
	PlayShot({ TEXT("A_Pilot_Blade_Death1_v2"), TEXT("A_Pilot_Blade_Death1"), TEXT("A_Pilot_Death_Front") }, 1.f, false, true);
	if (BladeBeam)
	{
		BladeBeam->SetVisibility(false);
	}
	DropCrewTargetsOnMe();
	if (UTDGroundSquadSubsystem* Squad = GetSquad())
	{
		Squad->NotifyAllyEvent(this, ETDGroundSquadEvent::Down);
	}
	UE_LOG(LogTriadDrift, Display, TEXT("GroundSquad: %s is down (%d), up again in %.0f s"), *Pilot.ToString(), TimesDown, DownSeconds);
}

void ATDGroundAlly::DropCrewTargetsOnMe()
{
	// The crew keep a target until it dies or hides; a downed ally does neither, so tell them to look again
	// (their own sense then finds the nearest standing pilot).
	const UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this);
	if (!Registry)
	{
		return;
	}
	TArray<AActor*> Hostiles;
	Registry->GetHostilesOf(ETDTeam::Player, Hostiles, true);
	for (AActor* H : Hostiles)
	{
		const APawn* P = Cast<APawn>(H);
		ATDCrewAIController* AI = P ? Cast<ATDCrewAIController>(P->GetController()) : nullptr;
		if (AI && AI->GetTarget() == this)
		{
			AI->SetTarget(nullptr);
		}
	}
}

void ATDGroundAlly::TickDown(float DeltaSeconds)
{
	DownLeft -= DeltaSeconds;
	if (UTDLoopBlendAnimInstance* Loops = GetLoops())
	{
		if (Loops->GetOneShot() && Loops->ShotTime >= DownPoseTime)
		{
			Loops->ShotRate = 0.f; // down: the fall held where the body is on the floor and in view
		}
	}
	if (DownLeft <= 0.f)
	{
		GetUp();
	}
}

void ATDGroundAlly::GetUp()
{
	State = ETDGroundAllyState::Follow;
	DownLeft = 0.f;
	bHolding = false;
	StuckTime = 0.f;
	if (Health)
	{
		Health->Revive(ReviveHealth01, 0.f);
		Health->SetInvulnerable(false);
		Health->SetTargetable(true);
		Health->SetInvulnerableFor(1.f); // on its feet before the next blow
	}
	if (UTDLoopBlendAnimInstance* Loops = GetLoops())
	{
		// Out of the held pose through a landing clip (a rise), else straight back to the loops.
		if (!PlayShot({ TEXT("A_Pilot_Land_v2"), TEXT("A_Pilot_Land") }, 1.f, false))
		{
			Loops->StopOneShot();
		}
	}
	if (BladeBeam)
	{
		BladeBeam->SetVisibility(!bBodyHidden);
	}
	if (UTDGroundSquadSubsystem* Squad = GetSquad())
	{
		Squad->NotifyAllyEvent(this, ETDGroundSquadEvent::Recovered);
	}
	UE_LOG(LogTriadDrift, Display, TEXT("GroundSquad: %s is back up at %.0f%% health"), *Pilot.ToString(), ReviveHealth01 * 100.f);
}

void ATDGroundAlly::ResetBesideLeader()
{
	State = ETDGroundAllyState::Follow;
	DownLeft = 0.f;
	FightTarget = nullptr;
	IgnoredTarget = nullptr;
	PendingHitLeft = -1.f;
	SwingCooldown = 0.5f;
	ComboStep = 0;
	if (Health)
	{
		Health->Revive(1.f, 0.f);
		Health->SetInvulnerable(false);
		Health->SetTargetable(true);
	}
	if (UTDLoopBlendAnimInstance* Loops = GetLoops())
	{
		Loops->StopOneShot();
	}
	if (BladeBeam)
	{
		BladeBeam->SetVisibility(true);
	}
	TeleportToLeader();
	--Teleports; // a reset is not a rescue
}

void ATDGroundAlly::BeginBoarding()
{
	if (!IsDown())
	{
		State = ETDGroundAllyState::Boarding;
		FightTarget = nullptr;
		PendingHitLeft = -1.f;
	}
}

void ATDGroundAlly::TeleportToLeader()
{
	const UTDGroundSquadSubsystem* Squad = GetSquad();
	const ATDFirstPersonCharacter* Leader = GetLeader();
	if (!Squad || !Leader)
	{
		return;
	}
	const FVector Spot = Squad->FindSpotNearLeader(Slot);
	GetCharacterMovement()->StopMovementImmediately();
	SetActorLocationAndRotation(Spot, FRotator(0.f, Squad->GetFormationYaw(), 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	StuckTime = 0.f;
	bHolding = true;
	++Teleports;
}

void ATDGroundAlly::FellOutOfWorld(const UDamageType& DmgType)
{
	// Never destroyed by a kill plane: back beside the player (the ally's own magma / pit safety).
	if (GetLeader())
	{
		TeleportToLeader();
		return;
	}
	Super::FellOutOfWorld(DmgType);
}

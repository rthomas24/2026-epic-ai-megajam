// Triad Drift - on-foot Armada crew (ENG-23). Owner: Teammate 5.

#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Ground/Crew/TDCrewAIController.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDWeaponTypes.h"
#include "TriadDrift/Powers/TDPowerPresentation.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Animation/AnimInstance.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "HAL/IConsoleManager.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "Animation/AnimSequenceBase.h"
#include "Materials/MaterialInterface.h"
#include "TriadDrift/Ground/TDPilotAnim.h"
#include "TriadDrift/Ground/TDGroundAudio.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "AnimationRuntime.h"

// The crew set (Gameplay/Characters/Crew/Anims, retargeted onto SKEL_NullCrew with ACL, Nov 13): what a crew look with a
// skeletal mesh but no AnimBP plays. Cooked through DirectoriesToAlwaysCook (/Game/TriadDrift/Gameplay/Characters).
FString ATDCrewCharacter::CrewAnimPath(const TCHAR* TemplatePath)
{
	if (!TemplatePath)
	{
		return FString();
	}
	const FString Template(TemplatePath);
	const FString Name = FPackageName::GetShortName(Template.Left(Template.Find(TEXT("."))));
	const FString Package = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/%s"), *Name);
	if (!FindPackage(nullptr, *Package) && !FPackageName::DoesPackageExist(Package))
	{
		return FString();
	}
	return Package + TEXT(".") + Name;
}

const TCHAR* ATDCrewCharacter::FallbackLoopFor(ETDCrewType Type, int32 Loop)
{
	if (Type == ETDCrewType::Grunt)
	{
		return Loop == 0 ? TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/MM_Idle.MM_Idle")
			: Loop == 1 ? TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd")
			: TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/MF_Unarmed_Jog_Fwd.MF_Unarmed_Jog_Fwd");
	}
	return Loop == 0 ? TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/MF_Rifle_Idle_ADS.MF_Rifle_Idle_ADS")
		: Loop == 1 ? TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/MF_Rifle_Walk_Fwd.MF_Rifle_Walk_Fwd")
		: TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/MF_Rifle_Jog_Fwd.MF_Rifle_Jog_Fwd");
}

bool ATDCrewCharacter::bCollapseGuard = true;

void ATDCrewCharacter::GuardAgainstCollapse(float DeltaSeconds)
{
	USkeletalMeshComponent* M = GetMesh();
	if (bLookPoseFrozen && CollapseRetries < 0 && M) // retries disabled: each one showed the collapse again
	{
		// Standing in the reference pose for now: try the animation again (an async compression may have landed).
		FrozenRetryLeft -= DeltaSeconds;
		if (FrozenRetryLeft <= 0.f)
		{
			bLookPoseFrozen = false;
			bFallbackAnim = true;
			CollapseStrikes = 0;
			M->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			M->SetAnimInstanceClass(UTDLoopBlendAnimInstance::StaticClass());
			UpdateFallbackLoop();
		}
		return;
	}
	if (!bCollapseGuard || !bFallbackAnim || bLookPoseFrozen || RefHeadHeight < 50.f || !M || !M->IsVisible())
	{
		return;
	}
	CollapseCheckTimer -= DeltaSeconds;
	// Only a pose that was actually evaluated counts (bones are refreshed when rendered, or always when asked to).
	const bool bFresh = M->WasRecentlyRendered(0.2f) || M->VisibilityBasedAnimTickOption == EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	if (CollapseCheckTimer > 0.f || !bFresh || M->GetBoneIndex(TEXT("head")) == INDEX_NONE)
	{
		return;
	}
	CollapseCheckTimer = 0.25f;
	const float Feet = (float)(GetActorLocation().Z - GetSimpleCollisionHalfHeight());
	const float Head = (float)M->GetBoneLocation(TEXT("head")).Z - Feet;
	CollapseStrikes = Head < RefHeadHeight * 0.35f ? CollapseStrikes + 1 : 0;
	if (CollapseStrikes >= 2)
	{
		bLookPoseFrozen = true;
		++CollapseRetries;
		FrozenRetryLeft = 1.f;
		// No usable animation data for this look: pose it procedurally (C++, from the reference pose).
		if (UTDLoopBlendAnimInstance* Loops = Cast<UTDLoopBlendAnimInstance>(M->GetAnimInstance()))
		{
			Loops->bProcedural = true;
			Loops->Current = nullptr;
			Loops->Previous = nullptr;
			Loops->Shot = nullptr;
		}
		else
		{
			bFallbackAnim = false;
			M->SetAnimInstanceClass(nullptr);
			M->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			M->SetAnimation(nullptr);
		}
		UE_LOG(LogTriadDrift, Warning, TEXT("Crew look %s collapsed under its animation (head %.0f cm, expected %.0f): procedural pose from now on."),
			*GetNameSafe(M->GetSkeletalMeshAsset()), Head, RefHeadHeight);
	}
}

void ATDCrewCharacter::UpdateFallbackLoop()
{
	UTDLoopBlendAnimInstance* Loops = bFallbackAnim && GetMesh() ? Cast<UTDLoopBlendAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr;
	if (!Loops)
	{
		return;
	}
	const float Speed = (float)GetVelocity().Size2D();
	if (Loops->bProcedural)
	{
		const float Dt = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
		Loops->ProcForwardCS = GetMesh()->GetComponentTransform().InverseTransformVectorNoScale(GetActorForwardVector());
		Loops->ProcSpeed01 = FMath::FInterpTo(Loops->ProcSpeed01, FMath::Clamp(Speed / 500.f, 0.f, 1.f), Dt, 8.f);
		Loops->ProcPhase += Dt * (Speed > 40.f ? Speed / 55.f : 2.f); // a stride every ~3.5 m; slow breathing at rest
		const float Wind = IsTelegraphing() ? 1.f : (IsAttacking() ? -0.4f : 0.f);
		Loops->ProcAttack = FMath::FInterpTo(Loops->ProcAttack, Wind, Dt, 10.f);
		Loops->ProcFlinch = FMath::FInterpTo(Loops->ProcFlinch, 0.f, Dt, 6.f);
		return;
	}
	const int32 Want = Speed < 40.f ? 0 : Speed < 300.f ? 1 : 2;
	// The type's own loops first (the animation builder's set), else the template copies.
	{
		const TCHAR* Own = nullptr;
		switch (Spec.Type)
		{
		case ETDCrewType::Trooper: Own = Want == 0 ? TEXT("Idle_Aim") : nullptr; break;
		case ETDCrewType::Grunt: Own = Want == 2 ? TEXT("Charge") : nullptr; break;
		case ETDCrewType::Heavy: Own = Want == 0 ? TEXT("Idle_Guard") : TEXT("Walk_Fwd"); break;
		case ETDCrewType::Warden: Own = Want == 0 ? TEXT("Guard") : TEXT("Walk"); break;
		default: break;
		}
		if (UAnimSequenceBase* OwnLoop = Own ? TypeClip(Own) : nullptr)
		{
			const float OwnRef = Want == 1 ? 200.f : Want == 2 ? 450.f : 0.f;
			Loops->SetLoop(OwnLoop, OwnRef > 0.f ? FMath::Clamp(Speed / OwnRef, 0.6f, 1.5f) : 1.f);
			return;
		}
	}
	const FString Path = CrewAnimPath(Spec.Type, Want);
	UAnimSequenceBase* Loop = Path.IsEmpty() ? nullptr : LoadObject<UAnimSequenceBase>(nullptr, *Path, nullptr, LOAD_Quiet | LOAD_NoWarn);
	if (!Loop && Want != 0)
	{
		const FString IdlePath = CrewAnimPath(Spec.Type, 0);
		Loop = IdlePath.IsEmpty() ? nullptr : LoadObject<UAnimSequenceBase>(nullptr, *IdlePath, nullptr, LOAD_Quiet | LOAD_NoWarn);
	}
	if (!Loop)
	{
		Loops->bProcedural = true; // no crew set on disk: pose it in C++ rather than stand in the T-pose
		return;
	}
	const float Ref = Want == 1 ? 200.f : Want == 2 ? 450.f : 0.f;
	Loops->SetLoop(Loop, Ref > 0.f ? FMath::Clamp(Speed / Ref, 0.6f, 1.5f) : 1.f);
}
static TAutoConsoleVariable<int32> CVarCrewLogHits(TEXT("td.Crew.LogHits"), 0,
	TEXT("1: log every enemy damage instant (type, action, the playing per-type clip and its time) to check contact frames."));

namespace TDCrewAnims
{
	static const TCHAR* Melee = TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/MM_Attack_01.MM_Attack_01");
	static const TCHAR* Slam = TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/MM_ChargedAttack.MM_ChargedAttack");
	static const TCHAR* Fire = TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/MM_Rifle_Fire.MM_Rifle_Fire");
	static const TCHAR* Hit = TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/MM_HitReact_Front_Lgt_01.MM_HitReact_Front_Lgt_01");
	static const TCHAR* DeathFront = TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/MM_Death_Front_01.MM_Death_Front_01");
	static const TCHAR* DeathBack = TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/MM_Death_Back_01.MM_Death_Back_01");
	static const TCHAR* DeathLeft = TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/MM_Death_Left_01.MM_Death_Left_01");
	static const TCHAR* DeathRight = TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/MM_Death_Right_01.MM_Death_Right_01");
}

namespace TDCrewImpl
{
	static float TargetRadius(const AActor* A)
	{
		FVector Origin, Extent;
		A->GetActorBounds(true, Origin, Extent);
		const float R = FVector2D(Extent.X, Extent.Y).Size();
		return R > 1.f ? FMath::Min(R, 150.f) : 40.f;
	}
}

ATDCrewCharacter::ATDCrewCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(40.f, 90.f);
	RifleShotSound = TDGroundAudio::Default(TEXT("Crew/MSS_Crew_RifleShot"));
	GuardClangSound = TDGroundAudio::Default(TEXT("Crew/MSS_Crew_ShieldHit"));
	DroneHumSound = TDGroundAudio::Default(TEXT("Crew/SW_Drone_Hum_01"));

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(GetCapsuleComponent());
	Body->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Body->SetCanEverAffectNavigation(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Body->SetStaticMesh(Cube.Object);
	}
	Eyes = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Eyes"));
	Eyes->SetupAttachment(GetCapsuleComponent());
	Eyes->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Eyes->SetCanEverAffectNavigation(false);
	Eyes->SetCastShadow(false);
	if (Cube.Succeeded())
	{
		Eyes->SetStaticMesh(Cube.Object);
	}
	GetMesh()->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	TellGlow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TellGlow"));
	TellGlow->SetupAttachment(GetCapsuleComponent());
	TellLine = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TellLine"));
	TellLine->SetupAttachment(GetCapsuleComponent());
	for (UStaticMeshComponent* Tell : { TellGlow.Get(), TellLine.Get() })
	{
		Tell->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Tell->SetCanEverAffectNavigation(false);
		Tell->SetCastShadow(false);
		Tell->SetHiddenInGame(true);
		Tell->SetUsingAbsoluteLocation(true);
		Tell->SetUsingAbsoluteRotation(true);
		Tell->SetUsingAbsoluteScale(true);
	}
	if (Sphere.Succeeded())
	{
		TellGlow->SetStaticMesh(Sphere.Object);
	}
	if (Cube.Succeeded())
	{
		TellLine->SetStaticMesh(Cube.Object);
	}

	Health = CreateDefaultSubobject<UTDHealthComponent>(TEXT("Health"));
	Health->MaxShield = 0.f;
	Health->ShieldRegenFractionPerSecond = 0.f;
	Health->DefaultTeam = ETDTeam::Enemy;

	AIControllerClass = ATDCrewAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxAcceleration = 2400.f;
	GetCharacterMovement()->BrakingDecelerationFlying = 2400.f;
	Spec = FTDCrewLibrary::Get(CrewType);
}

void ATDCrewCharacter::InitFromType(ETDCrewType Type)
{
	CrewType = Type;
	Spec = FTDCrewLibrary::Get(Type);
	bSpecApplied = true;

	GetCapsuleComponent()->SetCapsuleSize(Spec.CapsuleRadius, Spec.CapsuleHalfHeight);
	if (Health)
	{
		Health->InitMaxValues(Spec.MaxHealth, 0.f, true);
	}
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = Spec.MoveSpeed;
	Move->MaxFlySpeed = Spec.MoveSpeed;
	if (Spec.bFlying)
	{
		Move->GravityScale = 0.f;
		Move->SetMovementMode(MOVE_Flying);
	}
	// Placeholder body sized from the capsule (engine cube = 100 cm).
	Body->SetRelativeScale3D(Spec.BodyScale);
	// Eye slit on the front face, near the top of the body.
	Eyes->SetRelativeLocation(FVector(Spec.BodyScale.X * 50.f + 1.f, 0.f, Spec.BodyScale.Z * 50.f * 0.6f));
	Eyes->SetRelativeScale3D(FVector(0.02f, FMath::Max(Spec.BodyScale.Y * 0.6f, 0.12f), 0.06f));

	if (!Visuals)
	{
		LoadDefaultVisuals();
	}
	ApplyVisuals();
}

void ATDCrewCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (!bSpecApplied)
	{
		InitFromType(CrewType);
	}
	Health->OnDamaged.AddUniqueDynamic(this, &ATDCrewCharacter::HandleDamaged);
	Health->OnDeath.AddUniqueDynamic(this, &ATDCrewCharacter::HandleDeath);
	Health->DamageFilter.BindUObject(this, &ATDCrewCharacter::FilterDamage);
	if (Spec.Type == ETDCrewType::Drone)
	{
		DroneHum = TDGroundAudio::PlayAttached(DroneHumSound, GetRootComponent(), SoundEvents, TEXT("DroneHum"));
		StartDroneThrusters();
	}
}

void ATDCrewCharacter::StartDroneThrusters()
{
	// The drone's EngineTrail (DA_Crew_Drone_Visuals; the effects builder sets NS_TD_Drone_Thruster): one per thruster
	// socket on the art mesh, or one at the hull's origin. Throttle follows the speed (Tick).
	StopDroneThrusters();
	if (!Visuals || Visuals->EngineTrail.IsEmpty() || !Body)
	{
		return;
	}
	TArray<FName> Sockets;
	for (const TCHAR* Name : { TEXT("Engine_0"), TEXT("Engine_1") })
	{
		if (Body->DoesSocketExist(Name)) { Sockets.Add(Name); }
	}
	if (Sockets.Num() == 0)
	{
		Sockets.Add(NAME_None);
	}
	for (const FName Socket : Sockets)
	{
		UAudioComponent* Audio = nullptr;
		if (UNiagaraComponent* FX = UTDStatics::PlayCueAttachedLoop(Visuals->EngineTrail, Body, Socket, Audio))
		{
			DroneThrusters.Add(FX);
		}
		if (Audio)
		{
			DroneThrusterAudio.Add(Audio);
		}
	}
}

void ATDCrewCharacter::StopDroneThrusters()
{
	for (UNiagaraComponent* FX : DroneThrusters)
	{
		if (FX) { FX->Deactivate(); }
	}
	for (UAudioComponent* Audio : DroneThrusterAudio)
	{
		if (Audio) { Audio->FadeOut(.3f, 0.f); }
	}
	DroneThrusters.Reset();
	DroneThrusterAudio.Reset();
}

void ATDCrewCharacter::FellOutOfWorld(const UDamageType& DmgType)
{
	// Knocked or walked off the map: that's a death (encounters must see it), then clean up.
	if (Health && Health->IsAlive())
	{
		Health->Kill(nullptr, ETDDamageKind::Environmental);
	}
	Destroy();
}

bool ATDCrewCharacter::HasGroundAhead(const FVector& Direction, float Reach) const
{
	if (Spec.bFlying || Direction.IsNearlyZero())
	{
		return true;
	}
	const FVector Ahead = GetActorLocation() + Direction.GetSafeNormal2D() * (Spec.CapsuleRadius + Reach);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDCrewLedge), false, this);
	FHitResult Hit;
	// Floor within a step-down of 1.5 m counts; anything deeper is a ledge.
	return GetWorld()->LineTraceSingleByChannel(Hit, Ahead + FVector(0.f, 0.f, 50.f), Ahead - FVector(0.f, 0.f, Spec.CapsuleHalfHeight + 150.f), ECC_Visibility, Params);
}

void ATDCrewCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (const TWeakObjectPtr<ATDCrewCharacter>& S : Summons)
	{
		if (ATDCrewCharacter* Grunt = S.Get(); Grunt && !Grunt->IsActorBeingDestroyed())
		{
			Grunt->Destroy(); // no orphaned summons after the Warden is gone
		}
	}
	if (UTDCrewTokenSubsystem* Tokens = UTDCrewTokenSubsystem::Get(this))
	{
		Tokens->Release(this);
	}
	if (Health && Health->DamageFilter.IsBoundToObject(this))
	{
		Health->DamageFilter.Unbind();
	}
	Super::EndPlay(EndPlayReason);
}

bool ATDCrewCharacter::GetActiveBoss(const UObject* WorldContextObject, float& OutHull01, int32& OutPhase, FText& OutName)
{
	OutHull01 = 0.f;
	OutPhase = 0;
	OutName = FText::GetEmpty();
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const FVector From = PC && PC->GetPawn() ? PC->GetPawn()->GetActorLocation() : FVector::ZeroVector;
	const ATDCrewCharacter* Best = nullptr;
	double BestDist = TNumericLimits<double>::Max();
	for (TActorIterator<ATDCrewCharacter> It(World); World && It; ++It)
	{
		if (It->CrewType != ETDCrewType::Warden || It->IsDead())
		{
			continue;
		}
		const double D = FVector::DistSquared(It->GetActorLocation(), From);
		if (D < BestDist) { BestDist = D; Best = *It; }
	}
	const UTDHealthComponent* H = Best ? UTDStatics::GetHealth(Best) : nullptr;
	if (!H)
	{
		return false;
	}
	OutHull01 = H->GetHull01();
	OutPhase = Best->BossPhase;
	OutName = NSLOCTEXT("TDCrew", "WardenName", "WARDEN");
	return true;
}

bool ATDCrewCharacter::IsDead() const
{
	return Health && Health->IsDead();
}

// ------------------------------------------------------------------------------------------------
// Visuals
// ------------------------------------------------------------------------------------------------

void ATDCrewCharacter::LoadDefaultVisuals()
{
	// Core's one loader: quiet when Nick hasn't made the asset yet, cooked in packaged builds.
	Visuals = FindCrewVisuals(Spec.VisualsId);
	// Build: a heavy is bulkier, a grunt slighter, whether the look is their own or borrowed (Lead, 17e0437).
	LookScale = Spec.Type == ETDCrewType::Heavy ? 1.08f : Spec.Type == ETDCrewType::Grunt ? 0.96f : 1.f;
	// Until each type has its own look, grunts and heavies wear the trooper's, sized to their build (R-019).
	if (!Visuals && (Spec.Type == ETDCrewType::Grunt || Spec.Type == ETDCrewType::Heavy))
	{
		if (UTDEnemyVisuals* Trooper = FindCrewVisuals(TEXT("Trooper")); Trooper && !Trooper->SkeletalMesh.IsNull())
		{
			Visuals = Trooper;
		}
	}
}

UTDEnemyVisuals* ATDCrewCharacter::FindCrewVisuals(FName VisualsId)
{
	return Cast<UTDEnemyVisuals>(FindLookAsset(FString::Printf(TEXT("DA_Crew_%s_Visuals"), *VisualsId.ToString()), UTDEnemyVisuals::StaticClass()));
}

UObject* ATDCrewCharacter::FindLookAsset(const FString& Name, UClass* Class)
{
	// Nick's presentation asset always wins.
	if (UObject* Art = UTDStatics::LoadPresentationAsset(Name, Class))
	{
		return Art;
	}
	// Engineering-made crew looks (R-019): the same name anywhere under Gameplay/Characters (e.g. NullTrooper/).
	IAssetRegistry* Registry = IAssetRegistry::Get();
	if (!Registry)
	{
		return nullptr;
	}
	// By class, then by folder prefix: also finds assets in folders the registry has not scanned as paths yet.
	FARFilter Filter;
	Filter.ClassPaths.Add(Class->GetClassPathName());
	TArray<FAssetData> Found;
	Registry->GetAssets(Filter, Found);
	for (const FAssetData& A : Found)
	{
		if (A.AssetName.ToString() == Name && A.PackagePath.ToString().StartsWith(TEXT("/Game/TriadDrift/Gameplay/Characters")))
		{
			if (UObject* V = A.GetAsset(); V && V->IsA(Class))
			{
				return V;
			}
		}
	}
	return nullptr;
}

FVector ATDCrewCharacter::GetMuzzleLocation(const FVector& AimDirection) const
{
	// The computed chest-front point: always valid, always toward the aim.
	const FVector Chest = GetActorLocation() + FVector(0.f, 0.f, Spec.CapsuleHalfHeight * 0.4f) + AimDirection * (Spec.CapsuleRadius + 35.f);
	// The art's Muzzle socket (skeletal look, else the weapon hand; or a static-mesh look's Muzzle, the Rodin drone) is
	// only a START POSITION: the shot's direction is always aimed from it to the target (AimAt). A point that isn't
	// in front of the body toward the aim falls back to the chest-front point.
	static const FName MuzzleSocket(TEXT("Muzzle"));
	static const FName Hand(TEXT("hand_r"));
	const USkeletalMeshComponent* Look = GetMesh();
	const bool bSkeletal = Look && Look->IsVisible() && Look->GetSkeletalMeshAsset();
	FVector Point;
	if (bSkeletal && Look->DoesSocketExist(MuzzleSocket))
	{
		Point = Look->GetSocketLocation(MuzzleSocket);
	}
	else if (bSkeletal && Look->GetBoneIndex(Hand) != INDEX_NONE)
	{
		Point = Look->GetBoneLocation(Hand);
	}
	else if (!bSkeletal && Body && Body->IsVisible() && Body->DoesSocketExist(MuzzleSocket))
	{
		Point = Body->GetSocketLocation(MuzzleSocket);
	}
	else
	{
		return Chest;
	}
	const FVector Aim2D = AimDirection.GetSafeNormal2D();
	FVector Out = Point - GetActorLocation();
	const float Ahead = (float)FVector::DotProduct(FVector(Out.X, Out.Y, 0.f), Aim2D);
	const float Side = (float)(FVector(Out.X, Out.Y, 0.f) - Aim2D * Ahead).Size();
	// Between waist and shoulder: a pose not evaluated yet (or a hand hanging low) reports near the feet, and a
	// bolt started there hits the floor (seen with the socketed NullTrooper: Z 85 cm below the centre).
	const bool bHeight = Out.Z > -Spec.CapsuleHalfHeight * 0.3f && Out.Z < Spec.CapsuleHalfHeight * 0.9f;
	if (Aim2D.IsNearlyZero() || Ahead < 0.f || Side > Spec.CapsuleRadius + 40.f || !bHeight)
	{
		return Chest;
	}
	// Never inside our own capsule: at least just past its front along the aim.
	const float MinAhead = Spec.CapsuleRadius + 20.f;
	if (Ahead < MinAhead)
	{
		Point += Aim2D * (MinAhead - Ahead);
	}
	return Point;
}

void ATDCrewCharacter::PlayFallbackAnim(const TCHAR* AssetPath, float Rate)
{
	if (!bFallbackAnim || !GetMesh() || !AssetPath)
	{
		return;
	}
	UTDLoopBlendAnimInstance* Loops = Cast<UTDLoopBlendAnimInstance>(GetMesh()->GetAnimInstance());
	const FString Own = CrewAnimPath(AssetPath);
	UAnimSequenceBase* Seq = Own.IsEmpty() ? nullptr : LoadObject<UAnimSequenceBase>(nullptr, *Own, nullptr, LOAD_Quiet | LOAD_NoWarn);
	if (Loops && Seq)
	{
		Loops->PlayOneShot(Seq, Rate);
	}
}

const TCHAR* ATDCrewCharacter::FallbackDeathAnim(const FVector& HitDirection) const
{
	// HitDirection is the way the killing shot travelled; the side it came from picks the fall.
	if (HitDirection.IsNearlyZero())
	{
		return TDCrewAnims::DeathFront;
	}
	const FVector From = -HitDirection.GetSafeNormal2D();
	const float Fwd = FVector::DotProduct(From, GetActorForwardVector());
	const float Right = FVector::DotProduct(From, GetActorRightVector());
	if (FMath::Abs(Fwd) >= FMath::Abs(Right))
	{
		return Fwd >= 0.f ? TDCrewAnims::DeathFront : TDCrewAnims::DeathBack;
	}
	return Right >= 0.f ? TDCrewAnims::DeathRight : TDCrewAnims::DeathLeft;
}

void ATDCrewCharacter::ApplyVisuals()
{
	if (Visuals && !Visuals->SkeletalMesh.IsNull())
	{
		if (USkeletalMesh* SK = Visuals->SkeletalMesh.LoadSynchronous())
		{
			GetMesh()->SetSkeletalMesh(SK);
			UClass* AnimClass = Visuals->AnimBPClass.IsNull() ? nullptr : Visuals->AnimBPClass.LoadSynchronous();
			bFallbackAnim = false;
			if (!AnimClass)
			{
				// A mesh with no AnimBP would stand frozen in its reference pose: the C++ loop player drives it with the
				// template mannequin loops (playtest 17e0437: the template ABPs collapsed every bone to the floor).
				AnimClass = UTDLoopBlendAnimInstance::StaticClass();
				bFallbackAnim = true;
			}
			if (AnimClass)
			{
				GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
				GetMesh()->SetAnimInstanceClass(AnimClass);
			}
			for (int32 i = 0; i < Visuals->MaterialOverrides.Num(); ++i)
			{
				if (UMaterialInterface* Mat = Visuals->MaterialOverrides[i].LoadSynchronous())
				{
					GetMesh()->SetMaterial(i, Mat);
				}
			}
			GetMesh()->SetVisibility(true);
			GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -Spec.CapsuleHalfHeight));
			GetMesh()->SetRelativeRotation(FRotator(0.f, Visuals->SkeletalMeshYaw, 0.f));
			GetMesh()->SetRelativeScale3D(FVector(LookScale));
			const FReferenceSkeleton& Ref = SK->GetRefSkeleton();
			const int32 HeadIndex = Ref.FindBoneIndex(TEXT("head"));
			RefHeadHeight = HeadIndex != INDEX_NONE ? (float)FAnimationRuntime::GetComponentSpaceTransformRefPose(Ref, HeadIndex).GetLocation().Z * LookScale : 0.f;
			CollapseStrikes = 0;
			bLookPoseFrozen = false;
			UpdateFallbackLoop();
			Body->SetVisibility(false);
			Eyes->SetVisibility(false);
			return;
		}
	}
	GetMesh()->SetVisibility(false); // not a skeletal look (any more): the static mesh or the block below
	if (Visuals && !Visuals->Mesh.IsNull())
	{
		if (UStaticMesh* SM = Visuals->Mesh.LoadSynchronous())
		{
			Body->SetStaticMesh(SM);
			Body->SetRelativeScale3D(FVector::OneVector);
			for (int32 i = 0; i < Visuals->MaterialOverrides.Num(); ++i)
			{
				if (UMaterialInterface* Mat = Visuals->MaterialOverrides[i].LoadSynchronous())
				{
					Body->SetMaterial(i, Mat);
				}
			}
			Body->SetVisibility(true);
			Eyes->SetVisibility(false);
			return;
		}
	}
	// Placeholder: a solid (lit) Armada-coloured block with glowing white eyes, until Nick's crew art exists.
	// The engine's glow material is translucent, so it is only used for the eyes.
	Body->SetVisibility(true);
	Eyes->SetVisibility(true);
	UTDStatics::TintPlaceholder(Body, Spec.TeamColor, 0.f);
	UTDStatics::TintPlaceholder(Eyes, FLinearColor(1.f, 0.95f, 0.9f), 4.f);
}

void ATDCrewCharacter::PlayCrewCue(FName Name, const FTDFXCue& Cue, bool bAttached, const FVector& Location)
{
	OnCueRequested.Broadcast(this, Name);
	if (Cue.IsEmpty())
	{
		return;
	}
	if (bAttached)
	{
		UTDStatics::PlayCueAttached(Cue, GetRootComponent());
	}
	else
	{
		UTDStatics::PlayCue(this, Cue, Location, GetActorRotation());
	}
}

bool ATDCrewCharacter::IsTellShown() const
{
	return TellGlow && !TellGlow->bHiddenInGame;
}

void ATDCrewCharacter::UpdateTell()
{
	if (!TellGlow || !TellLine)
	{
		return;
	}
	const bool bArtTell = Visuals && !Visuals->Telegraph.IsEmpty();
	const bool bShow = !bArtTell && !IsDead() && IsTelegraphing();
	if (!bShow)
	{
		TellGlow->SetHiddenInGame(true);
		TellLine->SetHiddenInGame(true);
		return;
	}
	if (!bTellTinted)
	{
		// Enemy red-orange (Armada colour code), bright enough to read against the planets' glow.
		// Pure Armada red-orange, dim enough never to bloom to white or hide the enemy (Lead, 17e0437 playtest).
		UTDStatics::TintPlaceholder(TellGlow, FLinearColor(1.f, 0.22f, 0.02f), 1.5f);
		UTDStatics::TintPlaceholder(TellLine, FLinearColor(1.f, 0.22f, 0.02f), 1.2f);
		bTellTinted = true;
	}
	const float Windup = FMath::Max(0.05f, CurrentAttack == ETDCrewAttack::Melee ? Spec.MeleeWindup : CurrentAttack == ETDCrewAttack::ChargeSlam ? Spec.ChargeWindup : Spec.BurstWindup);
	const float Alpha = 1.f - FMath::Clamp(WindupLeft / Windup, 0.f, 1.f); // grows as the shot comes
	const AActor* Target = AttackTarget.Get();
	const FVector Dir = AimAt(Target);
	const FVector Muzzle = GetMuzzleLocation(Dir);
	TellGlow->SetWorldLocation(Muzzle);
	TellGlow->SetWorldScale3D(FVector(FMath::Lerp(0.1f, 0.25f, Alpha))); // 10 -> 25 cm
	TellGlow->SetHiddenInGame(false);
	const bool bRanged = CurrentAttack == ETDCrewAttack::Burst || CurrentAttack == ETDCrewAttack::Shotgun || CurrentAttack == ETDCrewAttack::BoltFan;
	if (bRanged && Target)
	{
		const FVector To = Target->GetActorLocation() - Muzzle;
		const float Len = FMath::Min((float)To.Size(), 4000.f);
		TellLine->SetWorldLocation(Muzzle + To.GetSafeNormal() * Len * 0.5f);
		TellLine->SetWorldRotation(To.Rotation());
		TellLine->SetWorldScale3D(FVector(Len / 100.f, 0.02f, 0.02f)); // 2 cm
		TellLine->SetHiddenInGame(false);
	}
	else
	{
		TellLine->SetHiddenInGame(true);
	}
}

bool ATDCrewCharacter::IsPlaceholderVisible() const
{
	return Body && Body->IsVisible() && Body->GetStaticMesh() && Body->GetStaticMesh()->GetName() == TEXT("Cube");
}

// ------------------------------------------------------------------------------------------------
// Attacks
// ------------------------------------------------------------------------------------------------

float ATDCrewCharacter::AttackCooldown(ETDCrewAttack Attack) const
{
	const float PhaseScale = BossPhase >= 2 ? Spec.Phase2CooldownScale : 1.f;
	switch (Attack)
	{
	case ETDCrewAttack::Melee: return Spec.MeleeCooldown;
	case ETDCrewAttack::Burst: return Spec.BurstCooldown;
	case ETDCrewAttack::Shotgun: return Spec.ShotgunCooldown;
	case ETDCrewAttack::ChargeSlam: return Spec.BossAttackInterval * 2.f * PhaseScale;
	case ETDCrewAttack::BoltFan: return Spec.BossAttackInterval * 1.5f * PhaseScale;
	case ETDCrewAttack::Summon: return 12.f * PhaseScale;
	default: return 0.f;
	}
}

float ATDCrewCharacter::GetCooldownLeft(ETDCrewAttack Attack) const
{
	const float* Left = Cooldowns.Find(Attack);
	return Left ? FMath::Max(0.f, *Left) : 0.f;
}

void ATDCrewCharacter::ApplyMercy(float HealthScale, int32 MaxSummons)
{
	if (!bSpecApplied)
	{
		InitFromType(CrewType);
	}
	if (Health && HealthScale < 1.f)
	{
		Health->InitMaxValues(FMath::Max(1.f, Spec.MaxHealth * FMath::Clamp(HealthScale, 0.1f, 1.f)), 0.f, true);
	}
	SummonsLeft = MaxSummons;
}

int32 ATDCrewCharacter::GetAliveSummons() const
{
	int32 Alive = 0;
	for (const TWeakObjectPtr<ATDCrewCharacter>& S : Summons)
	{
		Alive += (S.IsValid() && !S->IsDead()) ? 1 : 0;
	}
	return Alive;
}

bool ATDCrewCharacter::StartAttack(ETDCrewAttack Attack, AActor* Target)
{
	if (Attack == ETDCrewAttack::None || IsDead() || IsStaggered() || IsAttacking() || GetCooldownLeft(Attack) > 0.f || OpeningGraceLeft > 0.f)
	{
		return false;
	}
	bool bCan = false;
	float Windup = Spec.BurstWindup;
	switch (Attack)
	{
	case ETDCrewAttack::Melee: bCan = Spec.MeleeDamage > 0.f && Target; Windup = Spec.MeleeWindup; break;
	case ETDCrewAttack::Burst: bCan = Spec.BurstCount > 0 && Target; break;
	case ETDCrewAttack::Shotgun: bCan = Spec.ShotgunPellets > 0 && Target; break;
	case ETDCrewAttack::ChargeSlam: bCan = Spec.ChargeSlamDamage > 0.f && Target; Windup = Spec.ChargeWindup; break;
	case ETDCrewAttack::BoltFan: bCan = Spec.FanBolts > 0 && Target; break;
	case ETDCrewAttack::Summon: bCan = Spec.SummonCount > 0 && SummonsLeft != 0 && GetAliveSummons() < Spec.SummonMaxAlive; break;
	default: break;
	}
	if (!bCan)
	{
		return false;
	}
	CurrentAttack = Attack;
	AttackTarget = Target;
	WindupLeft = FMath::Max(0.01f, Windup);
	RecoverLeft = 0.f;
	ShotsLeft = 0;
	PlayCrewCue(TEXT("Telegraph"), Visuals ? Visuals->Telegraph : FTDFXCue(), true, GetActorLocation());
	// The type's own move, its contact frame on the wind-up's end (the animation builder's times, Nov 13-14), else today's.
	bool bOwn = false;
	switch (Attack)
	{
	case ETDCrewAttack::Melee:
		bOwn = Spec.Type == ETDCrewType::Heavy ? PlayTypeClip(TEXT("ShieldBash"), 0.70f, WindupLeft) // Grok's: impact 0.70 s, the 0.7 s wind-up
			: Spec.Type == ETDCrewType::Grunt ? ((MeleeSwings++ % 2) ? PlayTypeClip(TEXT("Melee_03"), 0.48f, WindupLeft) : PlayTypeClip(TEXT("Melee_02"), 0.45f, WindupLeft)) : false; // contacts measured at 30 fps
		break;
	case ETDCrewAttack::Burst: bOwn = Spec.Type == ETDCrewType::Trooper && PlayTypeClip(TEXT("Fire_Burst"), 0.40f, WindupLeft); break;
	case ETDCrewAttack::BoltFan: bOwn = PlayTypeClip(TEXT("Fire_01"), 0.50f, WindupLeft); break;
	// The slam lands after the wind-up AND the charge (damage at ChargeWindup + ChargeDuration); the live blend's contact is 1.30 s.
	case ETDCrewAttack::ChargeSlam: bOwn = PlayTypeClip(TEXT("GroundSlam"), 1.30f, WindupLeft + Spec.ChargeDuration) || PlayTypeClip(TEXT("Slam"), 1.30f, WindupLeft + Spec.ChargeDuration); break;
	case ETDCrewAttack::Summon: bOwn = PlayTypeClip(TEXT("Summon_01"), 0.50f, WindupLeft); break;
	default: break;
	}
	if (!bOwn)
	{
		PlayFallbackAnim(Attack == ETDCrewAttack::Melee ? TDCrewAnims::Melee
			: (Attack == ETDCrewAttack::ChargeSlam || Attack == ETDCrewAttack::Summon) ? TDCrewAnims::Slam : TDCrewAnims::Fire);
	}
	OnAttackStarted.Broadcast(this, Attack);
	return true;
}

void ATDCrewCharacter::ReleaseAttack()
{
	const ETDCrewAttack Attack = CurrentAttack;
	Cooldowns.Add(Attack, AttackCooldown(Attack));
	OnAttackReleased.Broadcast(this, Attack);
	switch (Attack)
	{
	case ETDCrewAttack::Melee:
		DoMelee();
		RecoverLeft = 0.3f;
		break;
	case ETDCrewAttack::Burst:
		ShotsLeft = Spec.BurstCount;
		ShotTimer = 0.f;
		break;
	case ETDCrewAttack::Shotgun:
		DoShotgun();
		RecoverLeft = 0.4f;
		break;
	case ETDCrewAttack::ChargeSlam:
	{
		const AActor* T = AttackTarget.Get();
		FVector To = T ? (T->GetActorLocation() - GetActorLocation()) : GetActorForwardVector() * Spec.ChargeDistance;
		To.Z = 0.f;
		const float Dist = FMath::Min(To.Size(), Spec.ChargeDistance);
		ChargeVelocity = To.GetSafeNormal() * (Dist / FMath::Max(Spec.ChargeDuration, 0.05f));
		ChargeTimeLeft = Spec.ChargeDuration;
		break;
	}
	case ETDCrewAttack::BoltFan:
		DoBoltFan();
		RecoverLeft = 0.5f;
		break;
	case ETDCrewAttack::Summon:
		DoSummon();
		RecoverLeft = 0.5f;
		break;
	default:
		RecoverLeft = 0.1f;
		break;
	}
}

void ATDCrewCharacter::FinishAttack()
{
	const ETDCrewAttack Was = CurrentAttack;
	CurrentAttack = ETDCrewAttack::None;
	WindupLeft = RecoverLeft = 0.f;
	ShotsLeft = 0;
	OnAttackFinished.Broadcast(this, Was);
}

void ATDCrewCharacter::CancelAttack()
{
	ChargeTimeLeft = 0.f;
	if (CurrentAttack != ETDCrewAttack::None)
	{
		FinishAttack();
	}
}

FVector ATDCrewCharacter::AimAt(const AActor* At) const
{
	if (!At)
	{
		return GetActorForwardVector();
	}
	// Aim from where the shot actually starts (socket, hand or chest), so it flies at the target, not beside it.
	const FVector Chest = GetActorLocation() + FVector(0.f, 0.f, Spec.CapsuleHalfHeight * 0.4f);
	const FVector Rough = (At->GetActorLocation() - Chest).GetSafeNormal();
	const FVector Start = GetMuzzleLocation(Rough);
	const FVector Dir = (At->GetActorLocation() - Start).GetSafeNormal();
	return Dir.IsNearlyZero() ? Rough : Dir;
}

void ATDCrewCharacter::DoMelee()
{
	AActor* T = AttackTarget.Get();
	const UTDHealthComponent* TH = T ? UTDStatics::GetHealth(T) : nullptr;
	if (!T || !TH || TH->IsDead())
	{
		return;
	}
	FVector To = T->GetActorLocation() - GetActorLocation();
	To.Z = 0.f;
	const float Reach = Spec.MeleeRange + TDCrewImpl::TargetRadius(T);
	const bool bInFront = FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), To.GetSafeNormal()) >= 0.5f; // 120° swing
	if (To.Size() <= Reach && bInFront)
	{
		const FVector Dir = To.GetSafeNormal();
		LogHitInstant(TEXT("Melee"));
		UTDStatics::ApplyTDDamage(T, Spec.MeleeDamage, ETDDamageKind::Melee, this, this, T->GetActorLocation() - Dir * 40.f, Dir);
	}
}

void ATDCrewCharacter::FireBolt(const FVector& Direction, float Damage, float Speed, float Radius)
{
	UWorld* World = GetWorld();
	if (!World || Direction.IsNearlyZero())
	{
		return;
	}
	FTDProjectileSpec P;
	P.Damage = Damage;
	P.DamageKind = ETDDamageKind::Energy;
	P.Speed = Speed;
	P.Range = Spec.RangedMaxRange * 1.5f;
	P.CollisionRadius = Radius;
	P.bInheritShooterVelocity = false;
	P.FX = FTDPowerPresentation::LoadWeaponFX(TEXT("CrewBlaster")); // DA_CrewBlaster_FX (Gameplay/Characters/FX fallback; T2): tracer and impacts, else the placeholder bolt
	const FVector Muzzle = GetMuzzleLocation(Direction);
	ATDProjectile::Launch(World, P, FTransform(Direction.Rotation(), Muzzle), this, ETDTeam::Enemy, nullptr, FVector::ZeroVector, 1.f, false);
	if (World->GetTimeSeconds() - LastShotSoundTime > 0.05) // a shotgun's pellets or a fan of bolts is one shot
	{
		LastShotSoundTime = World->GetTimeSeconds();
		TDGroundAudio::Play(this, RifleShotSound, Muzzle, false, SoundEvents, TEXT("RifleShot"));
	}
}

void ATDCrewCharacter::DoShotgun()
{
	const FVector Base = AimAt(AttackTarget.Get());
	const FRotator BaseRot = Base.Rotation();
	// Deterministic cone: one pellet on axis, the rest on a ring at the spread angle.
	for (int32 i = 0; i < Spec.ShotgunPellets; ++i)
	{
		FVector Dir = Base;
		if (i > 0)
		{
			const float Around = 360.f * (i - 1) / FMath::Max(1, Spec.ShotgunPellets - 1);
			const float Off = Spec.ShotgunSpreadDeg * 0.5f;
			Dir = (BaseRot + FRotator(FMath::Sin(FMath::DegreesToRadians(Around)) * Off, FMath::Cos(FMath::DegreesToRadians(Around)) * Off, 0.f)).Vector();
		}
		FireBolt(Dir, Spec.ShotgunPelletDamage, Spec.BoltSpeed, 16.f);
	}
}

void ATDCrewCharacter::DoBoltFan()
{
	const int32 N = BossPhase >= 2 && Spec.Phase2FanBolts > 0 ? Spec.Phase2FanBolts : Spec.FanBolts;
	const FRotator Base = AimAt(AttackTarget.Get()).Rotation();
	for (int32 i = 0; i < N; ++i)
	{
		const float Alpha = N > 1 ? (float)i / (N - 1) : 0.5f;
		const float Yaw = FMath::Lerp(-Spec.FanArcDeg * 0.5f, Spec.FanArcDeg * 0.5f, Alpha);
		FireBolt((Base + FRotator(0.f, Yaw, 0.f)).Vector(), Spec.BoltDamage, Spec.BoltSpeed, Spec.BoltRadius);
	}
}

void ATDCrewCharacter::DoSummon()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (SummonsLeft == 0)
	{
		return;
	}
	if (SummonsLeft > 0)
	{
		--SummonsLeft;
	}
	const int32 Want = BossPhase >= 2 && Spec.Phase2SummonCount > 0 ? Spec.Phase2SummonCount : Spec.SummonCount;
	const int32 Room = FMath::Max(0, Spec.SummonMaxAlive - GetAliveSummons());
	const int32 Count = FMath::Min(Want, Room);
	for (int32 i = 0; i < Count; ++i)
	{
		const float Angle = 2.f * PI * (i + 0.5f) / FMath::Max(Count, 1);
		const FVector Loc = GetActorLocation() + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * (Spec.CapsuleRadius + 220.f);
		const FTransform Xf(GetActorRotation(), Loc);
		ATDCrewCharacter* Grunt = World->SpawnActorDeferred<ATDCrewCharacter>(ATDCrewCharacter::StaticClass(), Xf, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (Grunt)
		{
			Grunt->CrewType = ETDCrewType::Grunt;
			Grunt->FinishSpawning(Xf);
			Summons.Add(Grunt);
			if (ATDEncounterVolume* OwnerRoom = OwningEncounter.Get())
			{
				OwnerRoom->AdoptMember(Grunt); // counts toward the clear, removed by a reset
			}
		}
	}
	Summons.RemoveAll([](const TWeakObjectPtr<ATDCrewCharacter>& S) { return !S.IsValid(); });
}

void ATDCrewCharacter::TickCharge(float DeltaSeconds)
{
	if (ChargeTimeLeft <= 0.f)
	{
		return;
	}
	const float Step = FMath::Min(ChargeTimeLeft, DeltaSeconds);
	if (!HasGroundAhead(ChargeVelocity.GetSafeNormal2D(), ChargeVelocity.Size2D() * Step + Spec.CapsuleRadius))
	{
		EndCharge(); // slam at the edge rather than charging off it
		return;
	}
	FHitResult Hit;
	GetCharacterMovement()->SafeMoveUpdatedComponent(ChargeVelocity * Step, GetActorQuat(), true, Hit);
	ChargeTimeLeft -= DeltaSeconds;
	if (ChargeTimeLeft <= 0.f || (Hit.IsValidBlockingHit() && Cast<APawn>(Hit.GetActor())))
	{
		EndCharge();
	}
}

void ATDCrewCharacter::EndCharge()
{
	ChargeTimeLeft = 0.f;
	PlayCrewCue(TEXT("SlamImpact"), Visuals ? Visuals->Explosion : FTDFXCue(), false, GetActorLocation());
	LogHitInstant(TEXT("ChargeSlam"));
	UTDStatics::ApplyTDRadialDamage(this, GetActorLocation(), Spec.ChargeSlamRadius, Spec.ChargeSlamDamage, ETDDamageKind::Explosive,
		ETDTeam::Enemy, this, this);
	RecoverLeft = 0.6f;
}

// ------------------------------------------------------------------------------------------------
// Damage, stagger, death
// ------------------------------------------------------------------------------------------------

bool ATDCrewCharacter::IsInShieldArc(const FVector& WorldLocation) const
{
	FVector To = WorldLocation - GetActorLocation();
	To.Z = 0.f;
	if (To.IsNearlyZero())
	{
		return true;
	}
	const float CosHalf = FMath::Cos(FMath::DegreesToRadians(Spec.ShieldArcDeg * 0.5f));
	return FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), To.GetSafeNormal()) >= CosHalf;
}

float ATDCrewCharacter::FilterDamage(const FTDDamageInfo& Info)
{
	if (!Spec.bFrontalShield || IsGuardBroken() || Info.Kind != ETDDamageKind::Melee)
	{
		return Info.Amount;
	}
	const FVector From = Info.Instigator ? Info.Instigator->GetActorLocation() : (Info.HitLocation - Info.HitDirection * 100.f);
	if (!IsInShieldArc(From))
	{
		return Info.Amount;
	}
	PlayGuardClang(Info.HitLocation.IsNearlyZero() ? GetActorLocation() + GetActorForwardVector() * Spec.CapsuleRadius : Info.HitLocation);
	// Quick slashes chip the guard: GuardChipsToBreak in a row stagger it open (the held lunge still breaks it at once).
	GuardChips = GuardChipAge <= GuardChipWindow ? GuardChips + 1 : 1;
	GuardChipAge = 0.f;
	// Teach the lunge: on the first blocked hit, and again every 5 more (the on-foot prompt line, 4 s).
	if (ATDFirstPersonCharacter* Pilot = Cast<ATDFirstPersonCharacter>(Info.Instigator))
	{
		const int32 Blocked = Pilot->CountGuardedHit();
		if (Blocked == 1 || (Blocked - 1) % 5 == 0)
		{
			Pilot->ShowTip(NSLOCTEXT("TriadDrift", "GuardTip", "HOLD ATTACK: LUNGE BREAKS GUARDS"), 4.f);
		}
	}
	if (GuardChips >= GuardChipsToBreak)
	{
		GuardChips = 0;
		BreakGuard(Info.Instigator);
	}
	return 0.f; // the shield takes the slash
}

void ATDCrewCharacter::PlayGuardClang(const FVector& At)
{
	// Nick's look cue first; the ground cue GuardClang (effects builder) or the engineering clang sound; and a hot spark.
	const bool bArt = Visuals && !Visuals->ShieldHit.IsEmpty();
	PlayCrewCue(TEXT("ShieldHit"), Visuals ? Visuals->ShieldHit : FTDFXCue(), false, At);
	if (!bArt)
	{
		TDGroundAudio::Play(this, GuardClangSound, At, false, SoundEvents, TEXT("GuardClang"));
		if (!GuardFlash)
		{
			GuardFlash = NewObject<UStaticMeshComponent>(this, TEXT("GuardFlash"));
			GuardFlash->SetStaticMesh(TellGlow ? TellGlow->GetStaticMesh() : nullptr);
			GuardFlash->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
			GuardFlash->SetCastShadow(false);
			GuardFlash->SetUsingAbsoluteLocation(true);
			GuardFlash->SetUsingAbsoluteScale(true);
			GuardFlash->SetupAttachment(GetRootComponent());
			GuardFlash->RegisterComponent();
			UTDStatics::TintPlaceholder(GuardFlash, FLinearColor(1.f, 0.85f, 0.5f), 12.f);
		}
		GuardFlash->SetWorldLocation(At);
		GuardFlash->SetWorldScale3D(FVector(0.45f));
		GuardFlash->SetHiddenInGame(false);
		GuardFlash->SetVisibility(true);
		GuardFlashLeft = 0.12f;
	}
}

void ATDCrewCharacter::HandleDamaged(UTDHealthComponent* InHealth, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage)
{
	if (HullDamage > 0.f)
	{
		PlayCrewCue(TEXT("HullHit"), Visuals ? Visuals->HullHit : FTDFXCue(), false, Info.HitLocation);
		if (!IsDead() && !IsAttacking())
		{
			if (UTDLoopBlendAnimInstance* Loops = GetMesh() ? Cast<UTDLoopBlendAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr; Loops && Loops->bProcedural)
			{
				Loops->ProcFlinch = 1.f;
			}
			const bool bFromBehind = !Info.HitDirection.IsNearlyZero() && FVector::DotProduct(-Info.HitDirection.GetSafeNormal2D(), GetActorForwardVector()) < 0.f;
			const bool bOwn = Spec.Type == ETDCrewType::Trooper ? PlayTypeClip(bFromBehind ? TEXT("HitReact_Back") : TEXT("HitReact_Front"))
				: Spec.Type == ETDCrewType::Grunt ? PlayTypeClip(TEXT("HitReact_01")) : false;
			if (!bOwn && Spec.Type != ETDCrewType::Grunt) // the template hit react holds a rifle: wrong on an unarmed grunt
			{
				PlayFallbackAnim(TDCrewAnims::Hit); // flinch (attacks keep their own animation)
			}
		}
	}
	if (Spec.Type == ETDCrewType::Warden && BossPhase == 1 && !IsDead() && Health->GetHull01() <= Spec.Phase2HealthFraction)
	{
		BossPhase = 2;
		Cooldowns.Reset(); // phase 2 opens with an attack
		OnBossPhase2.Broadcast(this);
		if (ITDGameFlow* Flow = UTDStatics::GetGameFlow(this))
		{
			Flow->RequestBark(TEXT("Bark_WardenPhase2"), TEXT("Armada"));
		}
	}
}

void ATDCrewCharacter::ApplyStagger(float Seconds, AActor* Source)
{
	if (IsDead())
	{
		return;
	}
	CancelAttack();
	const bool bWasStaggered = IsStaggered();
	StaggerLeft = FMath::Max(StaggerLeft, Seconds);
	if (!bWasStaggered)
	{
		// Entering the stagger state: Nick's Stagger cue (falls back to the older Stunned slot).
		const FTDFXCue* Cue = Visuals ? (Visuals->Stagger.IsEmpty() ? &Visuals->Stunned : &Visuals->Stagger) : nullptr;
		PlayCrewCue(TEXT("Stagger"), Cue ? *Cue : FTDFXCue(), true, GetActorLocation());
		PlayTypeClip(TEXT("Stagger")); // the Heavy's and the Warden's own (a cue only before)
	}
}

void ATDCrewCharacter::BreakGuard(AActor* Source)
{
	if (Spec.bFrontalShield)
	{
		GuardBrokenLeft = Spec.GuardBrokenSeconds;
		ApplyStagger(0.5f, Source);
	}
}

void ATDCrewCharacter::HandleDeath(UTDHealthComponent* InHealth, const FTDDamageInfo& KillingBlow)
{
	CancelAttack();
	if (Spec.Type == ETDCrewType::Drone)
	{
		SoundEvents.Add(TEXT("DroneHumStop"));
		if (DroneHum)
		{
			DroneHum->FadeOut(0.3f, 0.f);
			DroneHum = nullptr;
		}
		StopDroneThrusters();
	}
	if (UTDCrewTokenSubsystem* Tokens = UTDCrewTokenSubsystem::Get(this))
	{
		Tokens->Release(this);
	}
	OnCrewDied.Broadcast(this);
	// The Warden's summons fall with it.
	for (const TWeakObjectPtr<ATDCrewCharacter>& S : Summons)
	{
		if (ATDCrewCharacter* Grunt = S.Get(); Grunt && !Grunt->IsDead() && Grunt->GetHealth())
		{
			Grunt->GetHealth()->Kill(KillingBlow.Instigator, ETDDamageKind::Environmental);
		}
	}
	PlayCrewCue(TEXT("Death"), Visuals ? Visuals->Death : FTDFXCue(), false, GetActorLocation());

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	// Ragdoll: the skeletal mesh if Nick's art has a physics asset, otherwise the placeholder block tumbles.
	const FVector Push = (KillingBlow.HitDirection.IsNearlyZero() ? -GetActorForwardVector() : KillingBlow.HitDirection.GetSafeNormal()) * 450.f + FVector(0.f, 0.f, 250.f);
	RagdollPush = Push * 0.35f;
	ToppleAway = (KillingBlow.HitDirection.IsNearlyZero() ? -GetActorForwardVector() : KillingBlow.HitDirection).GetSafeNormal2D();
	if (GetMesh()->IsVisible() && GetMesh()->GetSkeletalMeshAsset())
	{
		// The fall starts in the death clip (in place, no root travel) and ends in the ragdoll (or a topple without a
		// physics asset), so every body lands on the ground (animation builder's sheet crew_deaths_as_played, Nov 13).
		if (UAnimSequenceBase* DeathAnim = ChooseDeathClip(KillingBlow.HitDirection))
		{
			// The fall plays whether or not the body is on screen (a corpse behind the camera must still land; headless tests).
			GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
			GetMesh()->PlayAnimation(DeathAnim, false);
			RagdollIn = bGroundDeathClip ? -1.f : DeathClipSeconds; // a fall that ends lying down plays out and holds
		}
		else
		{
			StartRagdollOrTopple();
		}
	}
	else if (Body->IsVisible())
	{
		Body->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
		Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Body->SetSimulatePhysics(true);
		Eyes->SetVisibility(false); // the eyes go dark when it dies
		Body->AddImpulse(Push, NAME_None, true);
	}
	DetachFromControllerPendingDestroy();
	// The body lies a moment after its fall ends (the per-type falls run 2.6-3.9 s; a 4 s corpse vanished as it landed).
	const UAnimSequenceBase* Fall = GetMesh() ? Cast<UAnimSequenceBase>(GetMesh()->GetSingleNodeInstance() ? GetMesh()->GetSingleNodeInstance()->GetCurrentAsset() : nullptr) : nullptr;
	CorpseLeft = FMath::Max3(0.1f, CorpseSeconds, Fall ? Fall->GetPlayLength() + 2.5f : 0.f);
	SetLifeSpan(CorpseLeft + 0.1f);
}

// ------------------------------------------------------------------------------------------------

UAnimSequenceBase* ATDCrewCharacter::ChooseDeathClip(const FVector& HitDirection) const
{
	const USkeletalMesh* SK = GetMesh() ? GetMesh()->GetSkeletalMeshAsset() : nullptr;
	if (!SK)
	{
		return nullptr;
	}
	auto Fits = [SK](UAnimSequenceBase* A) { return A && A->GetSkeleton() == SK->GetSkeleton() ? A : nullptr; };
	auto Load = [](const FString& Name) -> UAnimSequenceBase*
	{
		const FString Package = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/%s"), *Name);
		if (!FindPackage(nullptr, *Package) && !FPackageName::DoesPackageExist(Package)) { return nullptr; }
		return LoadObject<UAnimSequenceBase>(nullptr, *(Package + TEXT(".") + Name), nullptr, LOAD_Quiet | LOAD_NoWarn);
	};
	const bool bFromBehind = !HitDirection.IsNearlyZero() && FVector::DotProduct(-HitDirection.GetSafeNormal2D(), GetActorForwardVector()) < 0.f;
	const FString Type = Spec.VisualsId.ToString();
	// 1. Falls that end lying on the ground (Grok's set, made in place by the animation builder: A_Crew_Death_<Side>, and a
	//    per-type A_Crew_<Type>_Death_<Side> when one exists), picked by the side the killing blow came from.
	const FString Template(FallbackDeathAnim(HitDirection));
	const TCHAR* Side = Template.Contains(TEXT("Back")) ? TEXT("Back") : Template.Contains(TEXT("Left")) ? TEXT("Left") : Template.Contains(TEXT("Right")) ? TEXT("Right") : TEXT("Front");
	bGroundDeathClip = false;
	// The type's own ground falls first (Grok's, made in place: A_Crew_<Type>_Death_<Side>, the Warden on its own rig).
	if (UAnimSequenceBase* A = Fits(Load(FString::Printf(TEXT("A_Crew_%s_Death_%s"), *Type, Side)))) { bGroundDeathClip = true; return TDMakePlayable(A); }
	// The shared in-place set. The animation builder's captures (Nov 13): the Heavy's shield stands on edge in its Front
	// fall (a frontal kill falls Left) and the Warden's chest shield stands up in every fall but Back.
	const TCHAR* SharedSide = (Spec.Type == ETDCrewType::Heavy && FCString::Strcmp(Side, TEXT("Front")) == 0) ? TEXT("Left") : Spec.Type == ETDCrewType::Warden ? TEXT("Back") : Side;
	if (UAnimSequenceBase* A = Fits(Load(FString::Printf(TEXT("A_Crew_Death_%s"), SharedSide)))) { bGroundDeathClip = true; return TDMakePlayable(A); }
	// 2. Falls that only start (today's): the ragdoll ends them.
	TArray<FString> Names = {
		FString::Printf(TEXT("A_Crew_%s_Death_%s"), *Type, bFromBehind ? TEXT("02") : TEXT("01")),
		FString::Printf(TEXT("A_Crew_%s_Death_01"), *Type),
		FString::Printf(TEXT("A_Crew_%s_Death"), *Type),
		bFromBehind ? TEXT("A_Crew_Death_Front_03") : TEXT("A_Crew_Death_Front_02"),
		TEXT("A_Crew_Death_Front_02") };
	for (const FString& N : Names)
	{
		if (UAnimSequenceBase* A = Fits(Load(N))) { return TDMakePlayable(A); }
	}
	// The template set (root travel and a held crouch: only when nothing else fits; the ragdoll ends it anyway).
	const FString Path = bFallbackAnim ? CrewAnimPath(FallbackDeathAnim(HitDirection)) : FString();
	return Path.IsEmpty() ? nullptr : Fits(TDMakePlayable(LoadObject<UAnimSequenceBase>(nullptr, *Path, nullptr, LOAD_Quiet | LOAD_NoWarn)));
}

UAnimSequenceBase* ATDCrewCharacter::TypeClip(const TCHAR* Action) const
{
	const USkeletalMesh* SK = GetMesh() ? GetMesh()->GetSkeletalMeshAsset() : nullptr;
	if (!SK || !Action)
	{
		return nullptr;
	}
	const FString Name = FString::Printf(TEXT("A_Crew_%s_%s"), *Spec.VisualsId.ToString(), Action);
	const FString Package = FString::Printf(TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/Anims/%s"), *Name);
	if (!FindPackage(nullptr, *Package) && !FPackageName::DoesPackageExist(Package))
	{
		return nullptr;
	}
	UAnimSequenceBase* A = LoadObject<UAnimSequenceBase>(nullptr, *(Package + TEXT(".") + Name), nullptr, LOAD_Quiet | LOAD_NoWarn);
	return A && A->GetSkeleton() == SK->GetSkeleton() ? TDMakePlayable(A) : nullptr;
}

bool ATDCrewCharacter::PlayTypeClip(const TCHAR* Action, float Contact, float Windup)
{
	UTDLoopBlendAnimInstance* Loops = bFallbackAnim && GetMesh() ? Cast<UTDLoopBlendAnimInstance>(GetMesh()->GetAnimInstance()) : nullptr;
	UAnimSequenceBase* Clip = Loops ? TypeClip(Action) : nullptr;
	if (!Clip)
	{
		return false;
	}
	// The clip's contact frame on the gameplay hit: the damage keeps its timing (and the Heavy guard / deflect windows).
	const float Rate = (Contact > 0.f && Windup > 0.f) ? FMath::Clamp(Contact / Windup, 0.5f, 2.f) : 1.f;
	Loops->PlayOneShot(Clip, Rate);
	LastTypeClip = Clip->GetFName();
	LastTypeClipStart = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	LastTypeClipRate = Rate;
	return true;
}

void ATDCrewCharacter::LogHitInstant(const TCHAR* What) const
{
	if (CVarCrewLogHits.GetValueOnGameThread() == 0)
	{
		return;
	}
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	UE_LOG(LogTriadDrift, Display, TEXT("td.Crew.LogHits: %s %s %s, clip %s at %.3f s (rate %.2f)"), *GetName(), *FTDCrewLibrary::TypeName(Spec.Type).ToString(), What,
		LastTypeClip.IsNone() ? TEXT("(template)") : *LastTypeClip.ToString(), (Now - LastTypeClipStart) * LastTypeClipRate, LastTypeClipRate);
}

bool ATDCrewCharacter::IsRagdolling() const
{
	return GetMesh() && GetMesh()->IsSimulatingPhysics();
}

void ATDCrewCharacter::StartRagdollOrTopple()
{
	RagdollIn = -1.f;
	USkeletalMeshComponent* M = GetMesh();
	if (!M || !M->GetSkeletalMeshAsset())
	{
		return;
	}
	// Only our own crew ragdoll (PA_NullCrew, on its rig): the template PA_Mannequin on our meshes exploded the body 15 m in
	// 0.4 s (Nov 13 live), and PA_Warden launched the Warden 3 m (DeathsEndOnTheGroundWhereTheyFell, Nov 13): those topple.
	UPhysicsAsset* Ours = LoadObject<UPhysicsAsset>(nullptr, Spec.Type == ETDCrewType::Warden ? TEXT("/Game/TriadDrift/Gameplay/Characters/Warden/PA_Warden.PA_Warden")
		: TEXT("/Game/TriadDrift/Gameplay/Characters/Crew/PA_NullCrew.PA_NullCrew"), nullptr, LOAD_Quiet | LOAD_NoWarn); // PA_Warden settles now (Lead, Nov 14)
	if (Ours && Ours->GetPreviewMesh() && Ours->GetPreviewMesh()->GetSkeleton() == M->GetSkeletalMeshAsset()->GetSkeleton())
	{
		if (M->GetPhysicsAsset() != Ours) { M->SetPhysicsAsset(Ours, true); }
		M->SetCollisionProfileName(TEXT("Ragdoll"));
		M->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		M->SetAllBodiesSimulatePhysics(true);
		M->SetSimulatePhysics(true);
		M->WakeAllRigidBodies();
		M->AddImpulse(RagdollPush, NAME_None, true);
		return;
	}
	// No ragdoll for this look: topple over about the feet, away from the killing shot, from the pose it is in.
	ToppleAxis = FVector::CrossProduct(FVector::UpVector, ToppleAway).GetSafeNormal();
	ToppleStart = M->GetComponentQuat();
	ToppleAlpha = 0.f;
	bToppling = !ToppleAxis.IsNearlyZero();
}

void ATDCrewCharacter::TickTopple(float DeltaSeconds)
{
	// The corpse sinks out over its last CorpseSinkSeconds (the mesh and the placeholder block; a ragdoll stops simulating).
	if (CorpseLeft >= 0.f)
	{
		CorpseLeft -= DeltaSeconds;
		if (CorpseLeft < CorpseSinkSeconds)
		{
			const float Sink = 120.f * DeltaSeconds / CorpseSinkSeconds;
			for (USceneComponent* C : { static_cast<USceneComponent*>(GetMesh()), static_cast<USceneComponent*>(Body.Get()) })
			{
				if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(C))
				{
					if (Prim->IsSimulatingPhysics()) { Prim->SetSimulatePhysics(false); }
					Prim->AddWorldOffset(FVector(0.f, 0.f, -Sink));
				}
			}
		}
	}
	if (RagdollIn >= 0.f)
	{
		RagdollIn -= DeltaSeconds;
		if (RagdollIn < 0.f)
		{
			StartRagdollOrTopple();
		}
	}
	if (!bToppling || !GetMesh())
	{
		return;
	}
	ToppleAlpha = FMath::Min(1.f, ToppleAlpha + DeltaSeconds / 0.6f);
	const float Ease = ToppleAlpha * ToppleAlpha; // a body falls: slow, then fast
	const FQuat Fall(ToppleAxis, FMath::DegreesToRadians(85.f * Ease));
	GetMesh()->SetWorldRotation(Fall * ToppleStart); // the mesh's origin is its feet: it pivots there
	if (ToppleAlpha >= 1.f)
	{
		bToppling = false;
	}
}

void ATDCrewCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TickTopple(DeltaSeconds);
	OpeningGraceLeft = FMath::Max(0.f, OpeningGraceLeft - DeltaSeconds);
	if (DroneThrusters.Num() > 0)
	{
		// "Throttle" 0..1 from the drone's speed (a hover idles at a low flame).
		const float MaxSpeed = GetCharacterMovement() ? FMath::Max(GetCharacterMovement()->GetMaxSpeed(), 1.f) : 600.f;
		const float Throttle = FMath::Clamp(.2f + .8f * GetVelocity().Size() / MaxSpeed, 0.f, 1.f);
		for (UNiagaraComponent* FX : DroneThrusters)
		{
			if (FX) { FX->SetVariableFloat(TEXT("Throttle"), Throttle); }
		}
	}
	UpdateTell();
	if (!IsDead())
	{
		UpdateFallbackLoop();
		GuardAgainstCollapse(DeltaSeconds);
	}
	if (DeltaSeconds <= 0.f || IsDead())
	{
		return;
	}
	StaggerLeft = FMath::Max(0.f, StaggerLeft - DeltaSeconds);
	GuardBrokenLeft = FMath::Max(0.f, GuardBrokenLeft - DeltaSeconds);
	GuardChipAge += DeltaSeconds;
	if (GuardFlashLeft > 0.f)
	{
		GuardFlashLeft -= DeltaSeconds;
		if (GuardFlashLeft <= 0.f && GuardFlash) { GuardFlash->SetVisibility(false); }
	}
	for (TPair<ETDCrewAttack, float>& C : Cooldowns)
	{
		C.Value = FMath::Max(0.f, C.Value - DeltaSeconds);
	}
	TickCharge(DeltaSeconds);

	if (CurrentAttack == ETDCrewAttack::None)
	{
		return;
	}
	if (WindupLeft > 0.f)
	{
		// Square up to the target during the wind-up (the body does not follow the AI's control rotation on its own:
		// a grunt circling beside the pilot used to swing at the air forever - journey run 2, wreck core stall).
		if (const AActor* T = AttackTarget.Get(); T && CurrentAttack != ETDCrewAttack::ChargeSlam)
		{
			const float Want = (T->GetActorLocation() - GetActorLocation()).Rotation().Yaw;
			SetActorRotation(FRotator(0.f, FMath::FixedTurn(GetActorRotation().Yaw, Want, 720.f * DeltaSeconds), 0.f));
		}
		WindupLeft -= DeltaSeconds;
		if (WindupLeft <= 0.f)
		{
			WindupLeft = 0.f;
			ReleaseAttack();
		}
		return;
	}
	if (ShotsLeft > 0)
	{
		ShotTimer -= DeltaSeconds;
		if (ShotTimer <= 0.f)
		{
			FireBolt(AimAt(AttackTarget.Get()), Spec.BoltDamage, Spec.BoltSpeed, Spec.BoltRadius);
			--ShotsLeft;
			ShotTimer += Spec.BurstSpacing;
			if (ShotsLeft == 0)
			{
				RecoverLeft = 0.25f;
			}
		}
		return;
	}
	if (ChargeTimeLeft > 0.f)
	{
		return;
	}
	RecoverLeft -= DeltaSeconds;
	if (RecoverLeft <= 0.f)
	{
		FinishAttack();
	}
}

// ================================================================================================

UTDCrewTokenSubsystem* UTDCrewTokenSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UTDCrewTokenSubsystem>() : nullptr;
}

bool UTDCrewTokenSubsystem::TryAcquire(AActor* Attacker)
{
	if (!Attacker)
	{
		return false;
	}
	Holders.RemoveAll([](const TWeakObjectPtr<AActor>& H) { return !H.IsValid(); });
	if (Holds(Attacker))
	{
		return true;
	}
	if (Holders.Num() >= MaxMeleeAttackers)
	{
		return false;
	}
	Holders.Add(Attacker);
	return true;
}

void UTDCrewTokenSubsystem::Release(AActor* Attacker)
{
	Holders.RemoveAll([Attacker](const TWeakObjectPtr<AActor>& H) { return !H.IsValid() || H.Get() == Attacker; });
}

bool UTDCrewTokenSubsystem::Holds(const AActor* Attacker) const
{
	for (const TWeakObjectPtr<AActor>& H : Holders)
	{
		if (H.Get() == Attacker)
		{
			return true;
		}
	}
	return false;
}

int32 UTDCrewTokenSubsystem::GetActiveCount() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<AActor>& H : Holders)
	{
		N += H.IsValid() ? 1 : 0;
	}
	return N;
}

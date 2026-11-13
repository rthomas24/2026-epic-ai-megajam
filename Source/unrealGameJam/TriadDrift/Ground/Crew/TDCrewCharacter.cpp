// Triad Drift - on-foot Armada crew (ENG-23). Owner: Teammate 5.

#include "TriadDrift/Ground/Crew/TDCrewCharacter.h"
#include "TriadDrift/Ground/Crew/TDCrewAIController.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Combat/TDWeaponTypes.h"
#include "Components/CapsuleComponent.h"
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
#include "Animation/AnimSequenceBase.h"
#include "Materials/MaterialInterface.h"
#include "TriadDrift/Ground/TDPilotAnim.h"
#include "AnimationRuntime.h"

// Template mannequin set (Content/Characters/Mannequins): the stand-in for a crew look that has a skeletal mesh but no
// AnimBP. Cooked through DirectoriesToAlwaysCook (/Game/Characters/Mannequins/Anims).
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
		return Loop == 0 ? TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle")
			: Loop == 1 ? TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd")
			: TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_Fwd.MF_Unarmed_Jog_Fwd");
	}
	return Loop == 0 ? TEXT("/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS.MF_Rifle_Idle_ADS")
		: Loop == 1 ? TEXT("/Game/Characters/Mannequins/Anims/Rifle/Walk/MF_Rifle_Walk_Fwd.MF_Rifle_Walk_Fwd")
		: TEXT("/Game/Characters/Mannequins/Anims/Rifle/Jog/MF_Rifle_Jog_Fwd.MF_Rifle_Jog_Fwd");
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
		bFallbackAnim = false;
		++CollapseRetries;
		FrozenRetryLeft = 1.f;
		M->SetAnimInstanceClass(nullptr);
		M->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		M->SetAnimation(nullptr);
		UE_LOG(LogTriadDrift, Warning, TEXT("Crew look %s collapsed under its animation (head %.0f cm, expected %.0f): reference pose from now on."),
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
	const int32 Want = Speed < 40.f ? 0 : Speed < 300.f ? 1 : 2;
	const FString Path = CrewAnimPath(Spec.Type, Want);
	UAnimSequenceBase* Loop = Path.IsEmpty() ? nullptr : LoadObject<UAnimSequenceBase>(nullptr, *Path, nullptr, LOAD_Quiet | LOAD_NoWarn);
	if (!Loop && Want != 0)
	{
		const FString IdlePath = CrewAnimPath(Spec.Type, 0);
		Loop = IdlePath.IsEmpty() ? nullptr : LoadObject<UAnimSequenceBase>(nullptr, *IdlePath, nullptr, LOAD_Quiet | LOAD_NoWarn);
	}
	if (!Loop)
	{
		return; // no crew set yet: the loop player shows the reference pose (standing)
	}
	const float Ref = Want == 1 ? 200.f : Want == 2 ? 450.f : 0.f;
	Loops->SetLoop(Loop, Ref > 0.f ? FMath::Clamp(Speed / Ref, 0.6f, 1.5f) : 1.f);
}
namespace TDCrewAnims
{
	static const TCHAR* Melee = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01");
	static const TCHAR* Slam = TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack.MM_ChargedAttack");
	static const TCHAR* Fire = TEXT("/Game/Characters/Mannequins/Anims/Rifle/MM_Rifle_Fire.MM_Rifle_Fire");
	static const TCHAR* Hit = TEXT("/Game/Characters/Mannequins/Anims/Rifle/HitReact/MM_HitReact_Front_Lgt_01.MM_HitReact_Front_Lgt_01");
	static const TCHAR* DeathFront = TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01.MM_Death_Front_01");
	static const TCHAR* DeathBack = TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01.MM_Death_Back_01");
	static const TCHAR* DeathLeft = TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Left_01.MM_Death_Left_01");
	static const TCHAR* DeathRight = TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Right_01.MM_Death_Right_01");
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
	const USkeletalMeshComponent* Look = GetMesh();
	if (!Look || !Look->IsVisible() || !Look->GetSkeletalMeshAsset())
	{
		return Chest;
	}
	// The art's Muzzle socket (else the weapon hand) is only a START POSITION: the shot's direction is always aimed
	// from it to the target (AimAt). A hand posed beside or behind the body would send bolts past the player, so a
	// point that isn't in front of the body toward the aim falls back to the chest-front point.
	static const FName MuzzleSocket(TEXT("Muzzle"));
	static const FName Hand(TEXT("hand_r"));
	FVector Point;
	if (Look->DoesSocketExist(MuzzleSocket))
	{
		Point = Look->GetSocketLocation(MuzzleSocket);
	}
	else if (Look->GetBoneIndex(Hand) != INDEX_NONE)
	{
		Point = Look->GetBoneLocation(Hand);
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
	PlayFallbackAnim(Attack == ETDCrewAttack::Melee ? TDCrewAnims::Melee
		: (Attack == ETDCrewAttack::ChargeSlam || Attack == ETDCrewAttack::Summon) ? TDCrewAnims::Slam : TDCrewAnims::Fire);
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
	const FVector Muzzle = GetMuzzleLocation(Direction);
	ATDProjectile::Launch(World, P, FTransform(Direction.Rotation(), Muzzle), this, ETDTeam::Enemy, nullptr, FVector::ZeroVector, 1.f, false);
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
	PlayCrewCue(TEXT("ShieldHit"), Visuals ? Visuals->ShieldHit : FTDFXCue(), false, Info.HitLocation);
	return 0.f; // the shield takes the slash
}

void ATDCrewCharacter::HandleDamaged(UTDHealthComponent* InHealth, const FTDDamageInfo& Info, float ShieldDamage, float HullDamage)
{
	if (HullDamage > 0.f)
	{
		PlayCrewCue(TEXT("HullHit"), Visuals ? Visuals->HullHit : FTDFXCue(), false, Info.HitLocation);
		if (!IsDead() && !IsAttacking())
		{
			PlayFallbackAnim(TDCrewAnims::Hit); // flinch (attacks keep their own animation)
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
	const FString DeathPath = bFallbackAnim ? CrewAnimPath(FallbackDeathAnim(KillingBlow.HitDirection)) : FString();
	UAnimSequenceBase* DeathAnim = DeathPath.IsEmpty() ? nullptr : TDMakePlayable(LoadObject<UAnimSequenceBase>(nullptr, *DeathPath, nullptr, LOAD_Quiet | LOAD_NoWarn));
	if (GetMesh()->IsVisible() && DeathAnim)
	{
		GetMesh()->PlayAnimation(DeathAnim, false); // falls the way the killing shot came from, then holds
	}
	else if (GetMesh()->IsVisible() && GetMesh()->GetPhysicsAsset())
	{
		GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->AddImpulse(Push, NAME_None, true);
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
	SetLifeSpan(FMath::Max(0.1f, CorpseSeconds));
}

// ------------------------------------------------------------------------------------------------

void ATDCrewCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	OpeningGraceLeft = FMath::Max(0.f, OpeningGraceLeft - DeltaSeconds);
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

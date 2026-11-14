// Triad Drift - Enemies & Waves (Teammate 4).
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "TriadDrift/World/TDWorldBarks.h"

#include "TriadDrift/Combat/TDCombatPlaceholders.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ATDEnemyTurret::ATDEnemyTurret()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh->SetRelativeScale3D(FVector(1.5f, 1.5f, 1.f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		Mesh->SetStaticMesh(Cylinder.Object);
	}

	Health = CreateDefaultSubobject<UTDHealthComponent>(TEXT("Health"));
	Health->DefaultTeam = ETDTeam::Enemy;
	Health->MaxShield = 0.f;

	Weapons = CreateDefaultSubobject<UTDWeaponComponent>(TEXT("Weapons"));

	Guns.Kind = ETDEnemyAttack::Guns;
	Guns.WeaponId = TEXT("Enemy_PointDefense");
	Guns.Damage = 6.f;
	Guns.Range = 8000.f;
	Guns.ConeHalfAngleDeg = 6.f;
}

void ATDEnemyTurret::Init(const FTDEnemyAttackSpec& InGuns, float Hull)
{
	if (InGuns.IsValid())
	{
		Guns = InGuns;
	}
	StartHull = FMath::Max(1.f, Hull);
	if (Health)
	{
		Health->InitMaxValues(StartHull, 0.f, true);
	}
	if (Weapons && !Guns.WeaponId.IsNone())
	{
		Weapons->EquipWeapon(ETDWeaponSlot::Primary, Guns.WeaponId);
	}
	bInitialized = true;
	if (HasActorBegunPlay()) { ApplyLook(); }	// owners set Visuals, then Init, after the spawn's BeginPlay
}

void ATDEnemyTurret::SetDamageMultiplier(float Multiplier)
{
	if (Weapons) { Weapons->SetOutgoingDamageMultiplier(TEXT("Difficulty"), Multiplier); }
}

void ATDEnemyTurret::BeginPlay()
{
	Super::BeginPlay();
	if (!bInitialized)
	{
		Init(Guns, StartHull);	// placed standalone in a level
	}
	if (Health)
	{
		Health->OnDeath.AddUniqueDynamic(this, &ATDEnemyTurret::HandleDeath);
	}
	ApplyLook();
}

void ATDEnemyTurret::ApplyLook()
{
	// Art from Visuals (set by the owner before Init), else the dark-red placeholder cylinder. Idempotent.
	if (Visuals && !Visuals->TurretMesh.IsNull())
	{
		if (UStaticMesh* Art = Visuals->TurretMesh.LoadSynchronous())
		{
			if (Mesh->GetStaticMesh() != Art)
			{
				Mesh->SetStaticMesh(Art);
				Mesh->SetRelativeScale3D(FVector::OneVector);
				Mesh->EmptyOverrideMaterials();
			}
			return;
		}
	}
	UTDStatics::TintPlaceholder(Mesh, FLinearColor(0.55f, 0.06f, 0.04f), 0.f);
}

void ATDEnemyTurret::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Health)
	{
		Health->OnDeath.RemoveDynamic(this, &ATDEnemyTurret::HandleDeath);
	}
	Super::EndPlay(EndPlayReason);
}

ETDTeam ATDEnemyTurret::GetTeam() const
{
	return ETDTeam::Enemy;
}

bool ATDEnemyTurret::IsAlive() const
{
	return !bDestroyed && Health && Health->IsAlive();
}

void ATDEnemyTurret::ApplyStun(float Seconds)
{
	StunRemaining = FMath::Max(StunRemaining, Seconds);
	bTelegraphing = false;
	TelegraphRemaining = 0.f;
	BurstRemaining = 0.f;
	if (Weapons) { Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, false); }
}

void ATDEnemyTurret::HandleDeath(UTDHealthComponent* /*InHealth*/, const FTDDamageInfo& /*KillingBlow*/)
{
	if (bDestroyed)
	{
		return;
	}
	bDestroyed = true;
	if (Weapons) { Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, false); }
	if (Mesh)
	{
		Mesh->SetVisibility(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (Visuals) { UTDStatics::PlayCue(this, Visuals->TurretDestroyed, GetActorLocation(), GetActorRotation()); }
	if (!Visuals || !FTDCombatPlaceholders::HasVisualArt(Visuals->TurretDestroyed))
	{
		if (UTDProjectileSubsystem* Booms = UTDProjectileSubsystem::Get(this))
		{
			Booms->PlayPlaceholderExplosion(GetActorLocation(), 600.f, FLinearColor(1.f, 0.35f, 0.08f), 6);
		}
	}
	OnTurretDestroyed.Broadcast(this);
}

void ATDEnemyTurret::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsAlive() || !Weapons)
	{
		return;
	}

	if (StunRemaining > 0.f)
	{
		StunRemaining -= DeltaSeconds;
		return;
	}

	CooldownRemaining = FMath::Max(0.f, CooldownRemaining - DeltaSeconds);

	RetargetTimer -= DeltaSeconds;
	if (RetargetTimer <= 0.f)	// keep the timer even when nothing was found: no per-frame scans
	{
		RetargetTimer = 0.5f;
		UTDCombatRegistry* Registry = UTDCombatRegistry::Get(this);
		Target = Registry ? Registry->FindNearestHostile(ETDTeam::Enemy, GetActorLocation(), Guns.Range, this) : nullptr;
	}

	AActor* T = Target.Get();
	if (!T)
	{
		if (BurstRemaining > 0.f)
		{
			BurstRemaining = 0.f;
			Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, false);
		}
		bTelegraphing = false;
		return;
	}

	const FVector ToTarget = T->GetActorLocation() - GetActorLocation();
	const FRotator Desired = ToTarget.Rotation();
	SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), Desired, DeltaSeconds, TurnRateDeg));

	const FVector Aim = GetActorForwardVector();
	const bool bLinedUp = FVector::DotProduct(Aim, ToTarget.GetSafeNormal()) >= FMath::Cos(FMath::DegreesToRadians(Guns.ConeHalfAngleDeg));
	const bool bInRange = ToTarget.SizeSquared() <= FMath::Square(Guns.Range);
	Weapons->SetAimDirection(ToTarget.GetSafeNormal());

	if (BurstRemaining > 0.f)
	{
		BurstRemaining -= DeltaSeconds;
		if (BurstRemaining <= 0.f || !bInRange)
		{
			BurstRemaining = 0.f;
			Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, false);
			CooldownRemaining = BurstCooldownSeconds;
		}
		return;
	}

	if (bTelegraphing)
	{
		TelegraphRemaining -= DeltaSeconds;
		if (TelegraphRemaining <= 0.f)
		{
			bTelegraphing = false;
			BurstRemaining = BurstSeconds;
			++BurstCount;
			Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, true);
			if (GetAttachParentActor() && FTDCombatBarks::IsPlayerFlown(Target.Get()))
			{
				// dialogs.md Bark_Dom_Turrets: the Dominion's turret fire first comes at the flown ship.
				FTDWorldBarks::Say(this, TDWorldBarkIds::DomTurrets, ETDBarkPriority::High, FTDWorldBarks::EWho::Wingman, 600.f);
			}
		}
		return;
	}

	if (CooldownRemaining <= 0.f && bLinedUp && bInRange)
	{
		if (Guns.TelegraphSeconds > 0.f)
		{
			bTelegraphing = true;
			TelegraphRemaining = Guns.TelegraphSeconds;
			if (Visuals)
			{
				const bool bTurretVFX = !Visuals->TurretTelegraph.VFX.IsNull();	// sound-only must not hide the generic flash
				UTDStatics::PlayCueAttached(bTurretVFX ? Visuals->TurretTelegraph : Visuals->Telegraph, Mesh);
				if (!bTurretVFX && !Visuals->TurretTelegraph.IsEmpty()) { UTDStatics::PlayCueAttached(Visuals->TurretTelegraph, Mesh); }
			}
			OnTelegraphStarted.Broadcast(this);
		}
		else
		{
			BurstRemaining = BurstSeconds;
			++BurstCount;
			Weapons->SetTriggerHeld(ETDWeaponSlot::Primary, true);
		}
	}
}

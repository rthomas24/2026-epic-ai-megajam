// Triad Drift - Enemies & Waves (Teammate 4).
#include "TriadDrift/Enemies/TDCarrierBay.h"
#include "TriadDrift/World/TDWorldBarks.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Story/TDStorySubsystem.h"

#include "TriadDrift/Combat/TDCombatPlaceholders.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ATDCarrierBay::ATDCarrierBay()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh->SetRelativeScale3D(FVector(3.f, 6.f, 4.f));	// 3 m deep (outward), 6 m wide, 4 m tall
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		Mesh->SetStaticMesh(Cube.Object);
	}

	Health = CreateDefaultSubobject<UTDHealthComponent>(TEXT("Health"));
	Health->DefaultTeam = ETDTeam::Enemy;
	Health->MaxShield = 0.f;
}

void ATDCarrierBay::Init(ATDEnemyShip* InCarrier, float Hull, int32 InFightersPerLaunch, float InLaunchInterval, float InTelegraphSeconds, float InitialDelay)
{
	Carrier = InCarrier;
	FightersPerLaunch = FMath::Max(0, InFightersPerLaunch);
	LaunchInterval = FMath::Max(0.1f, InLaunchInterval);
	TelegraphSeconds = FMath::Clamp(InTelegraphSeconds, 0.f, LaunchInterval);
	NextLaunchRemaining = FMath::Max(0.f, InitialDelay);
	LaunchCount = 0;
	bTelegraphing = false;
	if (Health)
	{
		Health->InitMaxValues(FMath::Max(1.f, Hull), 0.f, true);
	}
	if (Visuals && !Visuals->BayMesh.IsNull())
	{
		if (UStaticMesh* Art = Visuals->BayMesh.LoadSynchronous())
		{
			Mesh->SetStaticMesh(Art);
			Mesh->SetRelativeScale3D(FVector::OneVector);
			Mesh->EmptyOverrideMaterials();
		}
	}
	else
	{
		UTDStatics::TintPlaceholder(Mesh, FLinearColor(1.f, 0.35f, 0.05f), 2.f);	// glowing orange bay doors: the weak point
	}
}

void ATDCarrierBay::BeginPlay()
{
	Super::BeginPlay();
	if (Health)
	{
		Health->DamageFilter.BindUObject(this, &ATDCarrierBay::FilterDamage);
		Health->OnDeath.AddUniqueDynamic(this, &ATDCarrierBay::HandleDeath);
	}
}

void ATDCarrierBay::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Health)
	{
		Health->DamageFilter.Unbind();
		Health->OnDeath.RemoveDynamic(this, &ATDCarrierBay::HandleDeath);
	}
	Super::EndPlay(EndPlayReason);
}

float ATDCarrierBay::GetHalfExtent() const
{
	return Mesh ? Mesh->GetRelativeScale3D().X * 50.f : 150.f;
}

ETDTeam ATDCarrierBay::GetTeam() const
{
	return ETDTeam::Enemy;
}

bool ATDCarrierBay::IsSpent() const
{
	const ATDEnemyShip* C = Carrier.Get();
	return (MaxLaunches > 0 && LaunchCount >= MaxLaunches) || (C && C->IsLaunchSpent());
}

bool ATDCarrierBay::IsAlive() const
{
	return !bDestroyed && Health && Health->IsAlive();
}

void ATDCarrierBay::BreakDoors()
{
	if (bDoorsBroken || !IsAlive())
	{
		return;
	}
	bDoorsBroken = true;
}

float ATDCarrierBay::FilterDamage(const FTDDamageInfo& Info)
{
	float Amount = Info.Amount;
	if (Info.Kind == ETDDamageKind::Ram)
	{
		BreakDoors();	// the ram itself lands on broken doors
	}
	if (bDoorsBroken)
	{
		Amount *= DoorsBrokenDamageMultiplier;
	}
	return Amount;
}

void ATDCarrierBay::HandleDeath(UTDHealthComponent* /*InHealth*/, const FTDDamageInfo& /*KillingBlow*/)
{
	if (bDestroyed)
	{
		return;
	}
	bDestroyed = true;
	bTelegraphing = false;
	if (Mesh)
	{
		Mesh->SetVisibility(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (Visuals) { UTDStatics::PlayCue(this, Visuals->BayDestroyed, GetActorLocation(), GetActorRotation()); }
	if (!Visuals || !FTDCombatPlaceholders::HasVisualArt(Visuals->BayDestroyed))
	{
		if (UTDProjectileSubsystem* Booms = UTDProjectileSubsystem::Get(this))
		{
			Booms->PlayPlaceholderExplosion(GetActorLocation(), 900.f, FLinearColor(1.f, 0.35f, 0.05f), 8);
		}
	}
	OnBayDestroyed.Broadcast(this);
}

void ATDCarrierBay::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsAlive() || bPaused || FightersPerLaunch <= 0 || IsSpent())
	{
		if (bPaused) { bTelegraphing = false; }
		return;
	}
	// The screen is full: hold (no telegraph, no countdown) until it thins out (hysteresis, checked 4 times a second).
	ScreenCheckTimer -= DeltaSeconds;
	if (ScreenCheckTimer <= 0.f && ScreenFullAt > 0)
	{
		ScreenCheckTimer = 0.25f;
		const int32 Alive = ATDEnemyShip::CountCarrierFightersAlive(this);
		const bool bWas = bScreenHeld;
		bScreenHeld = bScreenHeld ? Alive >= ResumeBelow : Alive >= ScreenFullAt;
		if (bScreenHeld != bWas) { UE_LOG(LogTriadDrift, Log, TEXT("TD carrier: launches %s (%d carrier fighters alive)"), bScreenHeld ? TEXT("held") : TEXT("resume"), Alive); }
	}
	if (bScreenHeld)
	{
		bTelegraphing = false;
		return;
	}

	ATDEnemyShip* CarrierShip = Carrier.Get();
	if (!CarrierShip || CarrierShip->IsDying())
	{
		return;
	}

	NextLaunchRemaining -= DeltaSeconds;

	if (!bTelegraphing && TelegraphSeconds > 0.f && NextLaunchRemaining <= TelegraphSeconds)
	{
		bTelegraphing = true;
		if (Visuals) { UTDStatics::PlayCueAttached(Visuals->BayLaunchTelegraph, Mesh); }
		OnLaunchTelegraph.Broadcast(this);
	}

	if (NextLaunchRemaining <= 0.f)
	{
		bTelegraphing = false;
		CarrierShip->LaunchFightersFromBay(this, FightersPerLaunch);
		const UTDGameInstance* GI = UTDGameInstance::Get(this);
		static TWeakObjectPtr<const UWorld> FirstLaunchWorld;
		const bool bFirstThisRun = FirstLaunchWorld.Get() != GetWorld();
		if (GI && GI->GetProgressRef().Phase == ETDGamePhase::DominionAssault && !UTDStorySubsystem::HasLineAired(this, TEXT("G6_Wren_ClearCarriers")))
		{
			UTDStorySubsystem::Fire(this, TEXT("G6_Wren_ClearCarriers"));	// storyline 24: the first carrier launch in the assault
		}
		else if (bFirstThisRun)
		{
			// Lead (Nov 14): the first launch says what to do about it. dialogs.md Bark_CarrierKillIt.
			FTDWorldBarks::Say(this, TEXT("Bark_CarrierKillIt"), ETDBarkPriority::High, FTDWorldBarks::EWho::Wingman, 0.f);
		}
		else
		{
			FTDWorldBarks::Say(this, TDWorldBarkIds::CarrierLaunch, ETDBarkPriority::Normal, FTDWorldBarks::EWho::Wingman, 20.f);	// dialogs.md Bark_CarrierLaunch
		}
		FirstLaunchWorld = GetWorld();
		++LaunchCount;
		if (Visuals) { UTDStatics::PlayCue(this, Visuals->BayLaunch, GetActorLocation(), GetActorRotation()); }
		OnLaunched.Broadcast(this);
		// Keep the cadence exact regardless of frame time.
		NextLaunchRemaining += LaunchInterval;
		if (NextLaunchRemaining <= 0.f) { NextLaunchRemaining = LaunchInterval; }
	}
}

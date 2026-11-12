// Triad Drift - Enemies & Waves (Teammate 4).
#include "TriadDrift/Enemies/TDCarrierBay.h"

#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ATDCarrierBay::ATDCarrierBay()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh->SetRelativeScale3D(FVector(4.f, 2.5f, 2.f));
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
	if (UMaterialInstanceDynamic* MID = Mesh ? Mesh->CreateDynamicMaterialInstance(0) : nullptr)
	{
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.9f, 0.35f, 0.05f));
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

ETDTeam ATDCarrierBay::GetTeam() const
{
	return ETDTeam::Enemy;
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
	OnBayDestroyed.Broadcast(this);
}

void ATDCarrierBay::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsAlive() || bPaused || FightersPerLaunch <= 0)
	{
		if (bPaused) { bTelegraphing = false; }
		return;
	}

	ATDEnemyShip* Owner = Carrier.Get();
	if (!Owner || Owner->IsDying())
	{
		return;
	}

	NextLaunchRemaining -= DeltaSeconds;

	if (!bTelegraphing && TelegraphSeconds > 0.f && NextLaunchRemaining <= TelegraphSeconds)
	{
		bTelegraphing = true;
		OnLaunchTelegraph.Broadcast(this);
	}

	if (NextLaunchRemaining <= 0.f)
	{
		bTelegraphing = false;
		Owner->LaunchFightersFromBay(this, FightersPerLaunch);
		++LaunchCount;
		OnLaunched.Broadcast(this);
		// Keep the cadence exact regardless of frame time.
		NextLaunchRemaining += LaunchInterval;
		if (NextLaunchRemaining <= 0.f) { NextLaunchRemaining = LaunchInterval; }
	}
}

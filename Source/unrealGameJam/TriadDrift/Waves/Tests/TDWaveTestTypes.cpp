// Triad Drift - Enemies & Waves (Teammate 4).
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDEventBus.h"

void UTDWaveTestListener::Bind(UObject* WorldContext)
{
	if (UTDEventBus* Bus = UTDEventBus::Get(WorldContext))
	{
		Bus->OnWaveStarted.AddUniqueDynamic(this, &UTDWaveTestListener::OnWaveStarted);
		Bus->OnWaveCleared.AddUniqueDynamic(this, &UTDWaveTestListener::OnWaveCleared);
		Bus->OnTakeStarted.AddUniqueDynamic(this, &UTDWaveTestListener::OnTakeStarted);
		Bus->OnMarkerRevealed.AddUniqueDynamic(this, &UTDWaveTestListener::OnMarkerRevealed);
		Bus->OnTipRequested.AddUniqueDynamic(this, &UTDWaveTestListener::OnTipRequested);
	}
}

ATDWaveTestEnemy::ATDWaveTestEnemy()
{
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	Health = CreateDefaultSubobject<UTDHealthComponent>(TEXT("Health"));
	Health->DefaultTeam = ETDTeam::Enemy;
	Health->MaxHull = 30.f;
	Health->MaxShield = 0.f;
}

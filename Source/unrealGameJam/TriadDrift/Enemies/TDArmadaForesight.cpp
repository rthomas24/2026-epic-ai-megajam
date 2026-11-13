// Triad Drift - Enemies (Teammate 4). Story clue: the Armada now and then pre-empts a switch or a dodge.
#include "TriadDrift/Enemies/TDArmadaForesight.h"

#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyAIController.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Core/TDEventBus.h"

#include "Engine/World.h"
#include "EngineUtils.h"

const FName UTDArmadaForesight::KindSwitch(TEXT("Switch"));
const FName UTDArmadaForesight::KindDodge(TEXT("Dodge"));

UTDArmadaForesight* UTDArmadaForesight::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UTDArmadaForesight>() : nullptr;
}

void UTDArmadaForesight::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (UTDEventBus* Bus = UTDEventBus::Get(&InWorld))
	{
		Bus->OnWaveStarted.AddUniqueDynamic(this, &UTDArmadaForesight::HandleWaveStarted);
		Bus->OnShipSwitched.AddUniqueDynamic(this, &UTDArmadaForesight::HandleShipSwitched);
	}
}

void UTDArmadaForesight::Deinitialize()
{
	if (UTDEventBus* Bus = UTDEventBus::Get(GetWorld()))
	{
		Bus->OnWaveStarted.RemoveDynamic(this, &UTDArmadaForesight::HandleWaveStarted);
		Bus->OnShipSwitched.RemoveDynamic(this, &UTDArmadaForesight::HandleShipSwitched);
	}
	for (const TWeakObjectPtr<UTDShipMovementComponent>& M : Bound)
	{
		if (M.IsValid()) { M->OnDodge.RemoveDynamic(this, &UTDArmadaForesight::HandleDodge); }
	}
	Bound.Reset();
	Super::Deinitialize();
}

void UTDArmadaForesight::Tick(float DeltaSeconds)
{
	// Listen to every squad ship's dodges (ships respawn and change; the handler only acts for the flown one).
	BindTimer -= DeltaSeconds;
	if (BindTimer > 0.f)
	{
		return;
	}
	BindTimer = 1.f;
	Bound.RemoveAll([](const TWeakObjectPtr<UTDShipMovementComponent>& M) { return !M.IsValid(); });
	if (const ATDSquadManager* Squad = ATDSquadManager::Get(GetWorld()))
	{
		for (const ETDShipId Id : { ETDShipId::Rift, ETDShipId::Lancer, ETDShipId::Bastion })
		{
			const ATDShip* Ship = Squad->GetShip(Id);
			UTDShipMovementComponent* Move = Ship ? Ship->GetShipMovement() : nullptr;
			if (Move && !Bound.Contains(Move))
			{
				Move->OnDodge.AddUniqueDynamic(this, &UTDArmadaForesight::HandleDodge);
				Bound.Add(Move);
			}
		}
	}
}

void UTDArmadaForesight::HandleWaveStarted(int32 WaveNumber, float /*Intensity01*/)
{
	Wave = WaveNumber;
}

bool UTDArmadaForesight::Roll()
{
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	if (Wave < MinWave || Now - LastTellTime < CooldownSeconds)
	{
		return false;
	}
	return Rng.FRand() < Chance;
}

ATDShip* UTDArmadaForesight::GetFlownShip() const
{
	const ATDSquadManager* Squad = ATDSquadManager::Get(GetWorld());
	return Squad ? Squad->GetActiveShip() : nullptr;
}

ATDEnemyShip* UTDArmadaForesight::PickEnemy(const FVector& Near, const FVector* Facing) const
{
	ATDEnemyShip* Best = nullptr;
	float BestD = Range;
	for (TActorIterator<ATDEnemyShip> It(GetWorld()); It; ++It)
	{
		ATDEnemyShip* E = *It;
		if (!E || !E->IsAliveEnemy() || E->IsDying() || E->IsStunned() || E->IsHidden())
		{
			continue;
		}
		const FVector To = Near - E->GetActorLocation();
		const float D = To.Size();
		// For a dodge the enemy must already be looking roughly that way (the shot has to be believable).
		if (Facing && FVector::DotProduct(E->GetActorForwardVector(), To.GetSafeNormal()) < 0.5f)
		{
			continue;
		}
		if (D < BestD)
		{
			BestD = D;
			Best = E;
		}
	}
	return Best;
}

void UTDArmadaForesight::Tell(ATDEnemyShip* Enemy, const FVector& AimPoint, AActor* Target, FName Kind)
{
	const UWorld* World = GetWorld();
	LastTellTime = World ? World->GetTimeSeconds() : 0.0;
	++Tells;
	// No reaction time: the nose is already there and the guns are already on.
	Enemy->SetActorRotation((AimPoint - Enemy->GetActorLocation()).Rotation());
	if (ATDEnemyAIController* AI = Cast<ATDEnemyAIController>(Enemy->GetController()))
	{
		AI->SetTarget(Target);
	}
	Enemy->SetGunsFiring(true, Target);
	OnAnticipated.Broadcast(Enemy, Kind);
	UE_LOG(LogTriadDrift, Verbose, TEXT("Armada foresight: %s pre-empted a %s"), *Enemy->GetName(), *Kind.ToString());
}

void UTDArmadaForesight::HandleShipSwitched(ETDShipId NewShip, ETDShipId /*OldShip*/)
{
	const ATDSquadManager* Squad = ATDSquadManager::Get(GetWorld());
	ATDShip* Ship = Squad ? Squad->GetShip(NewShip) : nullptr;
	if (!Ship || !Roll())
	{
		return;
	}
	if (ATDEnemyShip* E = PickEnemy(Ship->GetActorLocation(), nullptr))
	{
		Tell(E, Ship->GetActorLocation(), Ship, KindSwitch);
	}
}

void UTDArmadaForesight::HandleDodge(float Direction)
{
	ATDShip* Ship = GetFlownShip();
	if (!Ship || !Ship->GetShipMovement() || !Ship->GetShipMovement()->IsDodging() || !Roll())
	{
		return;
	}
	// Where the dodge will put the ship: to the side it chose, plus where it is going anyway.
	const FVector Where = Ship->GetActorLocation() + Ship->GetActorRightVector() * FMath::Sign(Direction) * 2500.f + Ship->GetVelocity() * 0.5f;
	const FVector Here = Ship->GetActorLocation();
	if (ATDEnemyShip* E = PickEnemy(Here, &Here))
	{
		Tell(E, Where, Ship, KindDodge);
	}
}

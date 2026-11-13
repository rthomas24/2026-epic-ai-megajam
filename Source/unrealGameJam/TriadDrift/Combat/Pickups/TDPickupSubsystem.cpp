// Triad Drift - space pickups (Teammate 2). See header.

#include "TriadDrift/Combat/Pickups/TDPickupSubsystem.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Combat/TDCombatBarks.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Squad/TDSquadManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogTDPickups, Log, All);

namespace TDPickups
{
	static TAutoConsoleVariable<int32> CVarEnabled(TEXT("td.Pickups"), 1,
		TEXT("Space pickups (hull repair, shield cell, power cell, overcharge): 1 on, 0 off (today's game exactly)."), ECVF_Default);
	static const FName OverchargeKey(TEXT("Overcharge"));
	static TAutoConsoleVariable<int32> CVarLog(TEXT("td.Pickups.Log"), 0, TEXT("1: log each kill's drop decision."), ECVF_Default);
}

bool UTDPickupSubsystem::IsEnabled()
{
	return TDPickups::CVarEnabled.GetValueOnGameThread() != 0;
}

UTDPickupSubsystem* UTDPickupSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UTDPickupSubsystem>() : nullptr;
}

bool UTDPickupSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void UTDPickupSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	Rng.Initialize(FPlatformTime::Cycles());
	if (UTDEventBus* B = UTDEventBus::Get(&InWorld))
	{
		Bus = B;
		B->OnActorKilled.AddUniqueDynamic(this, &UTDPickupSubsystem::HandleActorKilled);
		B->OnWaveStarted.AddUniqueDynamic(this, &UTDPickupSubsystem::HandleWaveStarted);
		B->OnSceneCut.AddUniqueDynamic(this, &UTDPickupSubsystem::HandleSceneCut);
		B->OnTakeStarted.AddUniqueDynamic(this, &UTDPickupSubsystem::HandleTakeStarted);
	}
}

void UTDPickupSubsystem::Deinitialize()
{
	if (UTDEventBus* B = Bus.Get())
	{
		B->OnActorKilled.RemoveDynamic(this, &UTDPickupSubsystem::HandleActorKilled);
		B->OnWaveStarted.RemoveDynamic(this, &UTDPickupSubsystem::HandleWaveStarted);
		B->OnSceneCut.RemoveDynamic(this, &UTDPickupSubsystem::HandleSceneCut);
		B->OnTakeStarted.RemoveDynamic(this, &UTDPickupSubsystem::HandleTakeStarted);
	}
	ClearAll();
	Super::Deinitialize();
}

TStatId UTDPickupSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDPickupSubsystem, STATGROUP_Tickables);
}

FName UTDPickupSubsystem::BarkFor(ETDPickupType Type)
{
	switch (Type)
	{
	case ETDPickupType::Hull: return TEXT("Bark_Pickup_Hull");
	case ETDPickupType::Shield: return TEXT("Bark_Pickup_Shield");
	case ETDPickupType::Power: return TEXT("Bark_Pickup_Power");
	default: return TEXT("Bark_Pickup_Overcharge");
	}
}

ATDShip* UTDPickupSubsystem::GetFlownShip() const
{
	const ATDSquadManager* Squad = ATDSquadManager::Get(this);
	ATDShip* Ship = Squad ? Squad->GetActiveShip() : nullptr;
	return Ship && FTDCombatBarks::IsPlayerFlown(Ship) ? Ship : nullptr;
}

ATDPickup* UTDPickupSubsystem::SpawnPickup(const UObject* WorldContext, ETDPickupType Type, FVector Location)
{
	UTDPickupSubsystem* Self = IsEnabled() ? Get(WorldContext) : nullptr;
	return Self ? Self->SpawnInternal(Type, Location) : nullptr;
}

ATDPickup* UTDPickupSubsystem::SpawnInternal(ETDPickupType Type, const FVector& Location)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATDPickup* Pickup = World->SpawnActor<ATDPickup>(ATDPickup::StaticClass(), Location, FRotator::ZeroRotator, Params);
	if (Pickup)
	{
		Pickup->Init(Type, Tuning.LifetimeSeconds, Tuning.BlinkSeconds);
		Pickups.Add(Pickup);
		UE_LOG(LogTDPickups, Display, TEXT("TD pickup: %s at (%.0f, %.0f, %.0f)"), ATDPickup::WordOf(Type), Location.X, Location.Y, Location.Z);
	}
	return Pickup;
}

bool UTDPickupSubsystem::PickDropType(ETDPickupType& Out)
{
	// What the player needs most first (a hurt ship wants hull), else an even roll; never a type past its wave cap.
	const ATDShip* Flown = GetFlownShip();
	const UTDHealthComponent* Health = Flown ? UTDStatics::GetHealth(Flown) : nullptr;
	const float Hull = Health ? Health->GetHull01() : 1.f;
	const float Shield = Health ? Health->GetShield01() : 1.f;
	float Weights[4] = { .5f + 2.f * (1.f - Hull), .5f + 1.f * (1.f - Shield), .6f, .6f };
	float Total = 0.f;
	for (int32 I = 0; I < 4; ++I)
	{
		if (DropsThisWave[I] >= Tuning.CapPerTypePerWave)
		{
			Weights[I] = 0.f;
		}
		Total += Weights[I];
	}
	if (Total <= 0.f)
	{
		return false;
	}
	float Roll = Rng.FRand() * Total;
	for (int32 I = 0; I < 4; ++I)
	{
		Roll -= Weights[I];
		if (Weights[I] > 0.f && Roll <= 0.f)
		{
			Out = static_cast<ETDPickupType>(I);
			return true;
		}
	}
	return false;
}

int32 UTDPickupSubsystem::HandleKill(AActor* Victim, AActor* Killer)
{
	const ATDEnemyShip* Enemy = Cast<ATDEnemyShip>(Victim);
	const bool bLog = TDPickups::CVarLog.GetValueOnGameThread() != 0;
	// A squad kill only: no killer = the Dominion's breakup or the environment (T4), never loot.
	if (!IsEnabled() || !Enemy || !Killer || UTDStatics::GetTeam(Killer) != ETDTeam::Player)
	{
		UE_CLOG(bLog, LogTDPickups, Display, TEXT("TD pickup kill %s by %s: no (enabled %d, enemy %d, killer team %d)"), *GetNameSafe(Victim),
			*GetNameSafe(Killer), IsEnabled() ? 1 : 0, Enemy ? 1 : 0, Killer ? static_cast<int32>(UTDStatics::GetTeam(Killer)) : -1);
		return 0;
	}
	// Nothing during a money shot's slow motion (the director owns that frame).
	if (const UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(this); Feel && Feel->IsSlowMoActive())
	{
		UE_CLOG(bLog, LogTDPickups, Display, TEXT("TD pickup kill %s: no (slow motion)"), *GetNameSafe(Victim));
		return 0;
	}
	UE_CLOG(bLog, LogTDPickups, Display, TEXT("TD pickup kill %s (%s): safety armed %d used %d, alive %d"), *GetNameSafe(Victim),
		*Enemy->GetEnemyId().ToString(), bSafetyArmed ? 1 : 0, bSafetyUsedThisScene ? 1 : 0, Pickups.Num());
	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	const FVector At = Victim->GetActorLocation();
	auto Alive = [this]() { return Pickups.Num(); };
	int32 Made = 0;

	// The safety repair: armed when the flown ship fell under SafetyHull01, once per scene, not right after another.
	if (bSafetyArmed && !bSafetyUsedThisScene && Now - LastRepairDropTime >= Tuning.SafetyGapSeconds && Alive() < Tuning.MaxAlive)
	{
		bSafetyUsedThisScene = true;
		bSafetyArmed = false;
		LastRepairDropTime = Now;
		SpawnInternal(ETDPickupType::Hull, At);
		++Made;
	}

	const FName Id = Enemy->GetEnemyId();
	const bool bCarrier = Id == TDEnemyIds::HiveCarrier;
	const int32 Tries = bCarrier ? Tuning.CarrierDrops : 1;
	const float Chance = bCarrier ? 1.f : (Id == TDEnemyIds::ShardFighter ? Tuning.FighterChance : Tuning.MidChance);
	for (int32 T = 0; T < Tries; ++T)
	{
		ETDPickupType Type;
		if (Alive() >= Tuning.MaxAlive || Rng.FRand() >= Chance || !PickDropType(Type))
		{
			continue;
		}
		++DropsThisWave[static_cast<int32>(Type)];
		if (Type == ETDPickupType::Hull)
		{
			LastRepairDropTime = Now;
		}
		// Carrier pairs side by side, not stacked.
		SpawnInternal(Type, At + FVector(0.f, (T - (Tries - 1) * .5f) * 600.f, 0.f));
		++Made;
	}
	return Made;
}

void UTDPickupSubsystem::HandleActorKilled(AActor* Victim, AActor* Killer, ETDDamageKind Kind)
{
	HandleKill(Victim, Killer);
}

void UTDPickupSubsystem::HandleWaveStarted(int32 WaveNumber, float Intensity01)
{
	for (int32& N : DropsThisWave)
	{
		N = 0;
	}
}

void UTDPickupSubsystem::HandleSceneCut(FName SceneId, int32 NextTakeNumber, float DelaySeconds)
{
	ClearAll(); // "CUT!": nothing carries into the next take
}

void UTDPickupSubsystem::HandleTakeStarted(FName SceneId, int32 TakeNumber)
{
	ClearAll();
	bSafetyUsedThisScene = false;
	bSafetyArmed = false;
	LastRepairDropTime = -1e9; // a new take is a fresh scene
	for (int32& N : DropsThisWave)
	{
		N = 0;
	}
}

void UTDPickupSubsystem::ClearAll()
{
	for (ATDPickup* P : Pickups)
	{
		if (IsValid(P))
		{
			P->Destroy();
		}
	}
	Pickups.Reset();
	EndOvercharge();
}

void UTDPickupSubsystem::EndOvercharge()
{
	if (ATDShip* Ship = OverchargedShip.Get())
	{
		if (UTDWeaponComponent* W = Ship->FindComponentByClass<UTDWeaponComponent>())
		{
			W->ClearOutgoingDamageMultiplier(TDPickups::OverchargeKey);
		}
	}
	OverchargedShip.Reset();
	OverchargeLeft = 0.f;
}

bool UTDPickupSubsystem::ApplyEffect(ETDPickupType Type, ATDShip* Ship)
{
	UTDHealthComponent* Health = Ship ? UTDStatics::GetHealth(Ship) : nullptr;
	if (!Health || !Health->IsAlive())
	{
		return false;
	}
	switch (Type)
	{
	case ETDPickupType::Hull:
		Health->RepairHull(Health->MaxHull * Tuning.HullRepair01);
		break;
	case ETDPickupType::Shield:
		Health->RestoreShield(Health->MaxShield * Tuning.Shield01);
		if (const ATDSquadManager* Squad = ATDSquadManager::Get(this))
		{
			for (int32 I = 0; I < 3; ++I)
			{
				ATDShip* Wing = Squad->GetShip(static_cast<ETDShipId>(I));
				UTDHealthComponent* WH = Wing && Wing != Ship ? UTDStatics::GetHealth(Wing) : nullptr;
				if (WH && WH->IsAlive())
				{
					WH->RestoreShield(WH->MaxShield * Tuning.WingmanShield01);
				}
			}
		}
		break;
	case ETDPickupType::Power:
		// The signature power only (Q and Bastion's hold-Q), not the unlockable ability (the Lead, Nov 13).
		for (const ETDPowerInput Slot : { ETDPowerInput::Signature1, ETDPowerInput::Signature2 })
		{
			if (UTDPowerComponent* Power = UTDPowerComponent::FindPower(Ship, Slot))
			{
				Power->ResetCooldown();
			}
		}
		break;
	default:
		// Never stacks: the same key on the same ship; another ship's overcharge moves over.
		if (OverchargedShip.IsValid() && OverchargedShip.Get() != Ship)
		{
			EndOvercharge();
		}
		if (UTDWeaponComponent* W = Ship->FindComponentByClass<UTDWeaponComponent>())
		{
			W->SetOutgoingDamageMultiplier(TDPickups::OverchargeKey, Tuning.OverchargeMultiplier);
			OverchargedShip = Ship;
			OverchargeLeft = Tuning.OverchargeSeconds;
		}
		break;
	}
	return true;
}

bool UTDPickupSubsystem::Collect(ATDPickup* Pickup, ATDShip* Ship)
{
	if (!IsValid(Pickup) || !ApplyEffect(Pickup->GetType(), Ship))
	{
		return false;
	}
	const ETDPickupType Type = Pickup->GetType();
	++Collected[static_cast<int32>(Type)];
	Pickups.Remove(Pickup);
	Pickup->Destroy();
	UE_LOG(LogTDPickups, Display, TEXT("TD pickup: collected %s by %s"), ATDPickup::WordOf(Type), *GetNameSafe(Ship));
	FTDCombatBarks::SayAsPilot(Ship, BarkFor(Type), ETDBarkPriority::Flavour);
	OnPickupCollected.Broadcast(Type, Ship);
	return true;
}

void UTDPickupSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const bool bEnabled = IsEnabled();
	if (!bEnabled)
	{
		if (bWasEnabled)
		{
			ClearAll(); // switched off mid-run: back to today's game at once
		}
		bWasEnabled = false;
		return;
	}
	bWasEnabled = true;

	if (OverchargeLeft > 0.f)
	{
		OverchargeLeft -= DeltaTime;
		if (OverchargeLeft <= 0.f || !OverchargedShip.IsValid())
		{
			EndOvercharge();
		}
	}

	ATDShip* Flown = GetFlownShip();
	if (const UTDHealthComponent* H = Flown ? UTDStatics::GetHealth(Flown) : nullptr; H && H->IsAlive() && H->GetHull01() < Tuning.SafetyHull01)
	{
		bSafetyArmed = true;
	}
	const FVector Me = Flown ? Flown->GetActorLocation() : FVector::ZeroVector;
	for (int32 I = Pickups.Num() - 1; I >= 0; --I)
	{
		ATDPickup* P = Pickups[I];
		if (!IsValid(P))
		{
			Pickups.RemoveAt(I);
			continue;
		}
		P->Advance(DeltaTime);
		if (P->IsExpired())
		{
			Pickups.RemoveAt(I);
			P->Destroy();
			continue;
		}
		if (!Flown)
		{
			continue;
		}
		// The flown ship only. A generous pull inside MagnetRadius, then contact.
		const FVector To = Me - P->GetActorLocation();
		const float Dist = To.Size();
		if (Dist <= Tuning.CollectRadius)
		{
			Collect(P, Flown);
		}
		else if (Dist <= Tuning.MagnetRadius)
		{
			const float Speed = Tuning.MagnetSpeed * (1.f + 1.5f * (1.f - Dist / Tuning.MagnetRadius));
			P->SetActorLocation(P->GetActorLocation() + To / Dist * FMath::Min(Speed * DeltaTime, Dist));
		}
	}
}

namespace TDPickups
{
	static FAutoConsoleCommandWithWorldAndArgs CmdSpawn(TEXT("TD.Pickups.Spawn"),
		TEXT("TD.Pickups.Spawn <Hull|Shield|Power|Overcharge> [metres ahead=40]: drop a pickup ahead of the flown ship (dev, captures)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
			const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
			if (!Pawn || Args.Num() == 0)
			{
				return;
			}
			const FString& S = Args[0];
			const ETDPickupType Type = S.StartsWith(TEXT("S")) ? ETDPickupType::Shield : S.StartsWith(TEXT("P")) ? ETDPickupType::Power
				: S.StartsWith(TEXT("O")) ? ETDPickupType::Overcharge : ETDPickupType::Hull;
			const float Metres = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 40.f;
			UTDPickupSubsystem::SpawnPickup(World, Type, Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * Metres * 100.f);
		}));
}

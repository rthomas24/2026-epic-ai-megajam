// Triad Drift - flight feel readouts for agent playtests (dev builds only). Owner: Teammate 1.
//
//   TD.Flight.Threats   log the player ship's hull / shield, damage taken since the last call, the nearest enemy and how
//                       many enemies are within 1 km / 2 km (pilot's-eye view of a fight: am I being chased, am I hit)

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Flight/TDShip.h"

namespace TDFlightDebug
{
	void Threats(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		ATDShip* Ship = PC ? Cast<ATDShip>(PC->GetPawn()) : nullptr;
		UTDCombatRegistry* Registry = World ? UTDCombatRegistry::Get(World) : nullptr;
		if (!Ship || !Registry || !Ship->GetHealth())
		{
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Flight.Threats: no player ship"));
			return;
		}
		static TWeakObjectPtr<ATDShip> LastShip;
		static float LastTotal = -1.f;
		const UTDHealthComponent* H = Ship->GetHealth();
		const float Total = H->GetHull() + H->GetShield();
		const float Taken = (LastShip.Get() == Ship && LastTotal >= 0.f) ? FMath::Max(0.f, LastTotal - Total) : 0.f;
		LastShip = Ship;
		LastTotal = Total;

		TArray<AActor*> Enemies;
		Registry->GetActorsOfTeam(ETDTeam::Enemy, Enemies, false);
		float Nearest = -1.f;
		int32 Within1 = 0, Within2 = 0, Alive = 0;
		for (const AActor* E : Enemies)
		{
			if (!IsValid(E))
			{
				continue;
			}
			++Alive;
			const float D = FVector::Dist(E->GetActorLocation(), Ship->GetActorLocation());
			Nearest = Nearest < 0.f ? D : FMath::Min(Nearest, D);
			Within1 += D <= 100000.f ? 1 : 0;
			Within2 += D <= 200000.f ? 1 : 0;
		}
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Flight.Threats: hull %.0f shield %.0f taken %.0f | enemies %d nearest %.0f m within1km %d within2km %d | speed %.0f boosting %d drifting %d"),
			H->GetHull(), H->GetShield(), Taken, Alive, Nearest / 100.f, Within1, Within2, (float)Ship->GetVelocity().Size(),
			Ship->IsBoosting() ? 1 : 0, Ship->IsDrifting() ? 1 : 0);
	}
}

static FAutoConsoleCommandWithWorldAndArgs GTDFlightThreatsCmd(TEXT("TD.Flight.Threats"),
	TEXT("Log the player ship's hull/shield, damage since the last call, and nearby enemies (agent feel playtests)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDFlightDebug::Threats));

#endif // !UE_BUILD_SHIPPING

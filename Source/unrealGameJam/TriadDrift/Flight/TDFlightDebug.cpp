// Triad Drift - flight feel readouts for agent playtests (dev builds only). Owner: Teammate 1.
//
//   TD.Flight.Threats   log the player ship's hull / shield, damage taken since the last call, the nearest enemy and how
//                       many enemies are within 1 km / 2 km (pilot's-eye view of a fight: am I being chased, am I hit)
//   TD.Flight.Feedback  log the no-art feedback: tick cost, frame time, dust motes, bursts, camera kick, placeholder counts

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDFlightFeedbackComponent.h"
#include "Misc/App.h"

namespace TDFlightDebug
{
	void Feedback(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		ATDShip* Ship = PC ? Cast<ATDShip>(PC->GetPawn()) : nullptr;
		UTDFlightFeedbackComponent* FB = Ship ? Ship->GetFlightFeedback() : nullptr;
		if (!FB)
		{
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Flight.Feedback: no player ship"));
			return;
		}
		FString Counts;
		for (const TCHAR* M : { TEXT("Dodge"), TEXT("BoostStart"), TEXT("DriftRelease"), TEXT("Brake"), TEXT("HyperdriveJump"),
			TEXT("HyperdriveDropOut"), TEXT("PortalTransit"), TEXT("AtmosphereEntry"), TEXT("Touchdown"), TEXT("TakeOff") })
		{
			Counts += FString::Printf(TEXT(" %s=%d"), M, FB->GetPlaceholderCount(M));
		}
		const FRotator Off = FB->GetCameraOffset();
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Flight.Feedback: tick %.4f ms | frame %.2f ms | dust %d | bursts %d | cam P%.2f Y%.2f R%.2f FOV%+.2f |%s"),
			FB->GetAverageTickMs(), FApp::GetDeltaTime() * 1000.0, FB->GetVisibleDustCount(), FB->GetActiveBurstCount(),
			Off.Pitch, Off.Yaw, Off.Roll, FB->GetFOVOffset(), *Counts);
	}

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

static FAutoConsoleCommandWithWorldAndArgs GTDFlightFeedbackCmd(TEXT("TD.Flight.Feedback"),
	TEXT("Log the no-art flight feedback: its tick cost, dust motes shown, active bursts, camera kick and placeholder counts."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDFlightDebug::Feedback));

#endif // !UE_BUILD_SHIPPING

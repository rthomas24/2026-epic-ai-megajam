// Triad Drift - travel debug console commands (dev builds only). Owner: Teammate 1.
// They drive each ENG-20 stage for scripted end-to-end runs (Tools/td-shot.ps1 -ExecAt "t:cmd"):
//   TD.Travel.ToPlanet Verdance|Cinder   put the player ship just outside that planet's atmosphere, nose in
//   TD.Travel.Land                       on a planet map: hop next to the player pad, slow, and auto-land
//   TD.Travel.TakeOff                    re-possess the landed player ship and take off
//   TD.Travel.Exit                       open the ceiling and climb out (TravelTo Galaxy, "<Planet>_Orbit")
//   TD.Travel.Status                     log the player ship's travel state

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "HAL/IConsoleManager.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Travel/TDLandingPad.h"
#include "TriadDrift/Travel/TDPlanetTravelRig.h"

#if __has_include("TriadDrift/World/TDPlanetAtmosphere.h")
#include "TriadDrift/World/TDPlanetAtmosphere.h"
#define TD_TRAVELDEBUG_HAS_PLANETS 1
#else
#define TD_TRAVELDEBUG_HAS_PLANETS 0
#endif

namespace TDTravelDebug
{
	APlayerController* GetPC(UWorld* World)
	{
		return World ? World->GetFirstPlayerController() : nullptr;
	}

	ATDShip* GetPlayerShip(UWorld* World)
	{
		APlayerController* PC = GetPC(World);
		return PC ? Cast<ATDShip>(PC->GetPawn()) : nullptr;
	}

	template <typename T>
	T* FindFirst(UWorld* World)
	{
		if (!World) { return nullptr; }
		for (TActorIterator<T> It(World); It; ++It) { return *It; }
		return nullptr;
	}

	void ToPlanet(const TArray<FString>& Args, UWorld* World)
	{
#if TD_TRAVELDEBUG_HAS_PLANETS
		ATDShip* Ship = GetPlayerShip(World);
		if (!Ship)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Travel.ToPlanet: no player ship"));
			return;
		}
		const FString Want = Args.Num() > 0 ? Args[0] : TEXT("Verdance");
		for (TActorIterator<ATDPlanetAtmosphere> It(World); It; ++It)
		{
			const FString Name = StaticEnum<ETDDestination>()->GetNameStringByValue((int64)It->GetDestination());
			if (!Name.Equals(Want, ESearchCase::IgnoreCase))
			{
				continue;
			}
			const FVector Center = It->GetPlanetCenter();
			FVector Out = (Ship->GetActorLocation() - Center).GetSafeNormal();
			if (Out.IsNearlyZero()) { Out = FVector::BackwardVector; }
			const FVector Start = Center + Out * (It->GetAtmosphereRadius() + 1500.f);
			if (UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive()) { Hyper->ResetHyperdrive(); }
			It->SetOpen(true); // the marker would be up by now in the real journey
			Ship->TeleportSpacecraft(Start, (-Out).Rotation(), false);
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Travel.ToPlanet: %s at %s, entering"), *Name, *Start.ToString());
			return;
		}
		UE_LOG(LogTriadDrift, Warning, TEXT("TD.Travel.ToPlanet: no planet atmosphere named %s"), *Want);
#else
		UE_LOG(LogTriadDrift, Warning, TEXT("TD.Travel.ToPlanet: ATDPlanetAtmosphere (T4) not in this build"));
#endif
	}

	void Land(const TArray<FString>& Args, UWorld* World)
	{
		ATDShip* Ship = GetPlayerShip(World);
		ATDPlanetTravelRig* Rig = FindFirst<ATDPlanetTravelRig>(World);
		ATDLandingPad* Pad = Rig ? Rig->GetPlayerPad() : nullptr;
		if (!Ship || !Pad || !Ship->GetLanding())
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Travel.Land: need a player ship and an ATDPlanetTravelRig (ship=%d pad=%d)"), Ship != nullptr, Pad != nullptr);
			return;
		}
		const FTransform PadXf = Pad->GetActorTransform();
		Ship->TeleportSpacecraft(PadXf.TransformPosition(FVector(-1500.f, 0.f, 1200.f)), PadXf.Rotator(), false);
		Ship->GetShipMovement()->SetForwardSpeedImmediate(1500.f);
		const bool bOk = Ship->GetLanding()->RequestLand();
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Travel.Land: RequestLand -> %s"), bOk ? TEXT("landing") : TEXT("refused"));
	}

	void TakeOff(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = GetPC(World);
		if (!World || !PC)
		{
			return;
		}
		for (TActorIterator<ATDShip> It(World); It; ++It)
		{
			UTDLandingComponent* Landing = It->GetLanding();
			if (Landing && Landing->IsLanded() && Landing->GetPad() && !Landing->GetPad()->IsWingmanPad())
			{
				if (PC->GetPawn() != *It)
				{
					PC->Possess(*It); // re-board (T5's interactable does this in the real flow)
				}
				It->SetPlayerControlled(true);
				const bool bOk = Landing->RequestTakeOff();
				UE_LOG(LogTriadDrift, Log, TEXT("TD.Travel.TakeOff: %s"), bOk ? TEXT("taking off") : TEXT("refused"));
				return;
			}
		}
		UE_LOG(LogTriadDrift, Warning, TEXT("TD.Travel.TakeOff: no landed player ship"));
	}

	void Exit(const TArray<FString>& Args, UWorld* World)
	{
		ATDShip* Ship = GetPlayerShip(World);
		ATDAtmosphereFlightZone* Zone = FindFirst<ATDAtmosphereFlightZone>(World);
		if (!Ship || !Zone)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Travel.Exit: need a player ship and a flight zone"));
			return;
		}
		Zone->SetExitAllowed(true);
		const FBox Box = Zone->GetZoneBox();
		Ship->TeleportSpacecraft(FVector(Ship->GetActorLocation().X, Ship->GetActorLocation().Y, Box.Max.Z + 1000.f), FRotator(80.f, Ship->GetActorRotation().Yaw, 0.f), false);
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Travel.Exit: exit opened, climbing out"));
	}

	void Status(const TArray<FString>& Args, UWorld* World)
	{
		ATDShip* Ship = GetPlayerShip(World);
		if (!Ship)
		{
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Travel.Status: map %s, no player ship (pawn: %s)"), World ? *World->GetMapName() : TEXT("?"),
				(GetPC(World) && GetPC(World)->GetPawn()) ? *GetPC(World)->GetPawn()->GetName() : TEXT("none"));
			return;
		}
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Travel.Status: map %s ship %s at %s speed %.0f atmosphere %d hyperdrive %d landing %d altitude %.0f m"),
			*World->GetMapName(), *Ship->GetName(), *Ship->GetActorLocation().ToString(), (float)Ship->GetVelocity().Size(),
			Ship->GetShipMovement()->IsInAtmosphere() ? 1 : 0,
			Ship->GetHyperdrive() ? (int32)Ship->GetHyperdrive()->GetState() : -1,
			Ship->GetLanding() ? (int32)Ship->GetLanding()->GetLandingState() : -1,
			Ship->GetAltitudeMeters());
	}
}

static FAutoConsoleCommandWithWorldAndArgs GTDTravelToPlanetCmd(TEXT("TD.Travel.ToPlanet"),
	TEXT("Put the player ship just outside a planet's atmosphere, nose in: TD.Travel.ToPlanet Verdance|Cinder"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDTravelDebug::ToPlanet));
static FAutoConsoleCommandWithWorldAndArgs GTDTravelLandCmd(TEXT("TD.Travel.Land"),
	TEXT("On a planet map: move next to the player pad, slow down and auto-land"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDTravelDebug::Land));
static FAutoConsoleCommandWithWorldAndArgs GTDTravelTakeOffCmd(TEXT("TD.Travel.TakeOff"),
	TEXT("Re-board the landed player ship and take off"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDTravelDebug::TakeOff));
static FAutoConsoleCommandWithWorldAndArgs GTDTravelExitCmd(TEXT("TD.Travel.Exit"),
	TEXT("Open the planet ceiling and climb out to the galaxy"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDTravelDebug::Exit));
static FAutoConsoleCommandWithWorldAndArgs GTDTravelStatusCmd(TEXT("TD.Travel.Status"),
	TEXT("Log the player ship's travel state"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDTravelDebug::Status));

#endif // !UE_BUILD_SHIPPING

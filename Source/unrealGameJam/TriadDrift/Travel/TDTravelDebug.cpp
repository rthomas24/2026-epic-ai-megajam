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
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Ground/Planets/TDPlanetDirector.h"
#include "TriadDrift/Core/TDInterfaces.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/World/TDDockingBay.h"
#include "TriadDrift/Travel/TDHyperdriveComponent.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Travel/TDLandingPad.h"
#include "TriadDrift/Travel/TDPlanetTravelRig.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"

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

	void CompleteObjectives(const TArray<FString>& Args, UWorld* World)
	{
		// Dev shortcut for travel playtests: mark this planet's required objectives done through the game flow (the
		// on-foot course itself is T5's to play), so the board point and the ceiling exit open the normal way.
		ATDAtmosphereFlightZone* Zone = FindFirst<ATDAtmosphereFlightZone>(World);
		ITDGameFlow* Flow = UTDStatics::GetGameFlow(World);
		if (!Zone || !Flow)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Travel.CompleteObjectives: need a planet flight zone and a game flow"));
			return;
		}
		for (const FName& Id : Zone->RequiredObjectives)
		{
			Flow->CompleteObjective(Id, ETDAbilityId::None, ETDShipId::Rift);
		}
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Travel.CompleteObjectives: %d objective(s) marked done; exit allowed %d"),
			Zone->RequiredObjectives.Num(), Zone->IsExitAllowed() ? 1 : 0);
	}

	void ToBoardPoint(const TArray<FString>& Args, UWorld* World)
	{
		// Dev shortcut: stand the on-foot pilot 1.5 m in front of the board point, facing it (then press E for real).
		const ATDPlanetDirector* Director = World ? ATDPlanetDirector::Get(World) : nullptr;
		AActor* Board = Director ? Director->GetBoardPoint() : nullptr;
		APlayerController* PC = GetPC(World);
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Board || !Pawn)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Travel.ToBoardPoint: need a planet director with a board point and an on-foot pawn"));
			return;
		}
		const FVector Target = Board->GetActorLocation();
		FVector From = Pawn->GetActorLocation();
		FVector Dir = FVector(Target.X - From.X, Target.Y - From.Y, 0.f).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		const FVector Stand(Target.X - Dir.X * 150.f, Target.Y - Dir.Y * 150.f, From.Z);
		Pawn->SetActorLocation(Stand, false, nullptr, ETeleportType::TeleportPhysics);
		const FRotator Look = (Target - (Stand + FVector(0.f, 0.f, 60.f))).Rotation();
		PC->SetControlRotation(Look);
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Travel.ToBoardPoint: pilot at %s facing the board point at %s"), *Stand.ToCompactString(), *Target.ToCompactString());
	}

	void ToBoundary(const TArray<FString>& Args, UWorld* World)
	{
		ATDShip* Ship = GetPlayerShip(World);
		const UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(World);
		FVector Center;
		float Radius = 0.f;
		if (!Ship || !Travel || !Travel->GetPlayBounds(Center, Radius))
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Travel.ToBoundary: need a player ship and play bounds"));
			return;
		}
		FVector Out = (Ship->GetActorLocation() - Center).GetSafeNormal();
		if (Out.IsNearlyZero()) { Out = FVector::ForwardVector; }
		// Just inside the edge, nose straight out: a hands-off ship must come back by itself.
		Ship->TeleportSpacecraft(Center + Out * (Radius - 2000.f), Out.Rotation(), false);
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Travel.ToBoundary: radius %.0f, flying out hands-off"), Radius);
	}

	void Status(const TArray<FString>& Args, UWorld* World)
	{
		ATDShip* Ship = GetPlayerShip(World);
		if (!Ship)
		{
			APawn* Pawn = GetPC(World) ? GetPC(World)->GetPawn() : nullptr;
			const ITDHUDData* Hud = Cast<ITDHUDData>(Pawn);
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Travel.Status: map %s, no player ship (pawn: %s at %s, prompt '%s')"), World ? *World->GetMapName() : TEXT("?"),
				Pawn ? *Pawn->GetName() : TEXT("none"), Pawn ? *Pawn->GetActorLocation().ToCompactString() : TEXT("-"),
				Hud ? *Hud->GetInteractPrompt().ToString() : TEXT(""));
			return;
		}
		const FTDFlightInput& In = Ship->GetFlightInput();
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Travel.Status: input throttle %.2f aim %.2f,%.2f roll %.2f boost %d drift %d brake %d | boost meter %.2f boosting %d drifting %d out-of-bounds %d near-boundary %d | hull roll %.1f visual bank %.1f"),
			In.Throttle, In.Aim.X, In.Aim.Y, In.Roll, In.bBoost, In.bDrift, In.bBrake, Ship->GetBoost01(), Ship->IsBoosting(), Ship->IsDrifting(),
			Ship->IsOutOfBounds(), Ship->GetShipMovement()->IsNearBoundary(),
			Ship->GetActorRotation().Roll, Ship->GetShipMovement()->GetVisualBank());
		if (const UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive())
		{
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Travel.Status: hyperdrive state %d charge %.2f block '%s' player-flown %d"),
				(int32)Hyper->GetState(), Hyper->GetCharge01(), *Hyper->GetBlockReasonText().ToString(), Ship->IsPlayerFlown());
		}
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Travel.Status: land prompt '%s' | rotation %s"),
			Ship->GetLanding() ? *Ship->GetLanding()->GetLandPrompt().ToString() : TEXT(""), *Ship->GetActorRotation().ToCompactString());
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Travel.Status: map %s ship %s at %s speed %.0f atmosphere %d hyperdrive %d landing %d altitude %.0f m"),
			*World->GetMapName(), *Ship->GetName(), *Ship->GetActorLocation().ToString(), (float)Ship->GetVelocity().Size(),
			Ship->GetShipMovement()->IsInAtmosphere() ? 1 : 0,
			Ship->GetHyperdrive() ? (int32)Ship->GetHyperdrive()->GetState() : -1,
			Ship->GetLanding() ? (int32)Ship->GetLanding()->GetLandingState() : -1,
			Ship->GetAltitudeMeters());
	}
}

static FAutoConsoleCommandWithWorldAndArgs GTDTravelJumpFromCmd(TEXT("TD.Travel.JumpFrom"),
	TEXT("Captures: put the player ship <Km> back from a planet's centre, facing it, and spool the hyperdrive: TD.Travel.JumpFrom Verdance|Cinder [Km=8]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
#if TD_TRAVELDEBUG_HAS_PLANETS
		ATDShip* Ship = TDTravelDebug::GetPlayerShip(World);
		if (!Ship) { UE_LOG(LogTriadDrift, Warning, TEXT("TD.Travel.JumpFrom: no player ship")); return; }
		const FString Want = Args.Num() > 0 ? Args[0] : TEXT("Verdance");
		const float Km = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 8.f;
		for (TActorIterator<ATDPlanetAtmosphere> It(World); It; ++It)
		{
			if (!StaticEnum<ETDDestination>()->GetNameStringByValue((int64)It->GetDestination()).Equals(Want, ESearchCase::IgnoreCase)) { continue; }
			const FVector Centre = It->GetPlanetCenter();
			FVector Out = (Ship->GetActorLocation() - Centre).GetSafeNormal2D();
			if (Out.IsNearlyZero()) { Out = FVector::BackwardVector; }
			const FVector Start = Centre + Out * Km * 100000.f;
			if (UTDHyperdriveComponent* Hyper = Ship->GetHyperdrive())
			{
				Hyper->ResetHyperdrive();
				Ship->TeleportSpacecraft(Start, (-Out).Rotation(), false);
				const bool bSpool = Hyper->StartSpool();
				UE_LOG(LogTriadDrift, Log, TEXT("TD.Travel.JumpFrom: %s from %.1f km, spool %s"), *Want, Km, bSpool ? TEXT("started") : TEXT("refused"));
			}
			return;
		}
		UE_LOG(LogTriadDrift, Warning, TEXT("TD.Travel.JumpFrom: no planet named %s"), *Want);
#endif
	}));
static FAutoConsoleCommandWithWorldAndArgs GTDTravelToDockCmd(TEXT("TD.Travel.ToDock"),
	TEXT("Captures: put the flown ship <m> out on the Dominion bay's approach axis, <side m> off it, facing the mouth (then fly in by hand): TD.Travel.ToDock [m=1500] [side m=0] [aim 0 = the mouth | 1 = straight along the axis]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		ATDShip* Ship = TDTravelDebug::GetPlayerShip(World);
		ATDDockingBay* Bay = nullptr;
		for (TActorIterator<ATDDockingBay> It(World); It; ++It) { Bay = *It; break; }
		if (!Ship || !Bay) { UE_LOG(LogTriadDrift, Warning, TEXT("TD.Travel.ToDock: %s"), !Ship ? TEXT("no player ship") : TEXT("no docking bay")); return; }
		const float Out = (Args.Num() > 0 ? FCString::Atof(*Args[0]) : 1500.f) * 100.f;
		const float Side = (Args.Num() > 1 ? FCString::Atof(*Args[1]) : 0.f) * 100.f;
		const bool bAlongAxis = Args.Num() > 2 && FCString::Atoi(*Args[2]) == 1;
		const FVector Axis = Bay->GetOutward();
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Axis).GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
		const FVector At = Bay->GetMouth() + Axis * Out + Right * Side;
		const FRotator Facing = bAlongAxis ? (-Axis).Rotation() : (Bay->GetMouth() - At).Rotation();
		Ship->TeleportSpacecraft(At, Facing, false);
		UE_LOG(LogTriadDrift, Log, TEXT("TD.Travel.ToDock: %.0f m out, %.0f m off the axis, bay %s"), Out / 100.f, Side / 100.f,
			*UEnum::GetValueAsString(Bay->GetDockingState()));
	}));
static FAutoConsoleCommandWithWorldAndArgs GTDTravelOpenPlanetCmd(TEXT("TD.Travel.OpenPlanet"),
	TEXT("Captures: open a planet's atmosphere trigger without moving the ship (a natural approach): TD.Travel.OpenPlanet Verdance|Cinder"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
#if TD_TRAVELDEBUG_HAS_PLANETS
		const FString Want = Args.Num() > 0 ? Args[0] : TEXT("Verdance");
		for (TActorIterator<ATDPlanetAtmosphere> It(World); It; ++It)
		{
			if (!StaticEnum<ETDDestination>()->GetNameStringByValue((int64)It->GetDestination()).Equals(Want, ESearchCase::IgnoreCase)) { continue; }
			It->SetOpen(true);
			UE_LOG(LogTriadDrift, Log, TEXT("TD.Travel.OpenPlanet: %s open"), *Want);
			return;
		}
		UE_LOG(LogTriadDrift, Warning, TEXT("TD.Travel.OpenPlanet: no planet named %s"), *Want);
#endif
	}));
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
static FAutoConsoleCommandWithWorldAndArgs GTDTravelToBoundaryCmd(TEXT("TD.Travel.ToBoundary"),
	TEXT("Put the player ship at the play-bounds edge, nose out (boundary turn-back check)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDTravelDebug::ToBoundary));
static FAutoConsoleCommandWithWorldAndArgs GTDTravelCompleteObjectivesCmd(TEXT("TD.Travel.CompleteObjectives"),
	TEXT("On a planet map: mark the zone's required objectives done (opens re-board and the ceiling exit normally)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDTravelDebug::CompleteObjectives));
static FAutoConsoleCommandWithWorldAndArgs GTDTravelToBoardPointCmd(TEXT("TD.Travel.ToBoardPoint"),
	TEXT("On foot on a planet: stand in front of the board point, facing it (then press E)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDTravelDebug::ToBoardPoint));
static FAutoConsoleCommandWithWorldAndArgs GTDTravelStatusCmd(TEXT("TD.Travel.Status"),
	TEXT("Log the player ship's travel state"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDTravelDebug::Status));

#endif // !UE_BUILD_SHIPPING

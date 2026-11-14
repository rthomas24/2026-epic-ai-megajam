// Triad Drift - World (environment builder). Issue #145 (B1-5): a debug command to hide a planet's look parts one at a time,
// so a capture can tell which part draws the teal patch at the menu planet's limb.
//   TD.Env.PlanetHide <mask>   1 = surface, 2 = atmosphere shell, 4 = ring; 0 shows everything again.
#include "TriadDrift/World/TDPlanetAtmosphere.h"

#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

static FAutoConsoleCommandWithWorldAndArgs GTDPlanetHideCmd(
	TEXT("TD.Env.PlanetHide"),
	TEXT("Dev (#145): hide planet look parts in every planet. Mask: 1 surface, 2 atmosphere shell, 4 ring; 0 = show all."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World) { return; }
		const int32 Mask = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
		int32 Planets = 0;
		for (TActorIterator<ATDPlanetAtmosphere> It(World); It; ++It)
		{
			TInlineComponentArray<UStaticMeshComponent*> Parts(*It);
			for (UStaticMeshComponent* C : Parts)
			{
				if (!C) { continue; }
				const FName N = C->GetFName();
				const int32 Bit = N == TEXT("SurfaceLook") ? 1 : (N == TEXT("AtmosphereLook") ? 2 : (N == TEXT("RingLook") ? 4 : 0));
				if (Bit) { C->SetHiddenInGame((Mask & Bit) != 0); }
			}
			++Planets;
		}
		UE_LOG(LogTemp, Display, TEXT("TD.Env.PlanetHide %d on %d planets"), Mask, Planets);
	}));

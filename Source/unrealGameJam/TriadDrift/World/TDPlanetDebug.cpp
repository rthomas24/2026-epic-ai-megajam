// Triad Drift - World (environment builder). Issue #145 (B1-5): a debug command to hide a planet's look parts one at a time,
// so a capture can tell which part draws the teal patch at the menu planet's limb.
//   TD.Env.PlanetHide <mask>   1 = surface, 2 = atmosphere shell, 4 = ring; 0 shows everything again.
#include "TriadDrift/World/TDPlanetAtmosphere.h"

#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
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

// Issue #158 (B2-05): stand the player under the nearest canopy clump (Nth nearest with an argument), looking up at its
// underside from eye height.   TD.Env.UnderClump [N] [Pitch]
static FAutoConsoleCommandWithWorldAndArgs GTDUnderClumpCmd(
	TEXT("TD.Env.UnderClump"),
	TEXT("Dev (#158): teleport the player under the Nth nearest tree clump (default 0) and look up (default pitch 40)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Pawn) { UE_LOG(LogTemp, Warning, TEXT("TD.Env.UnderClump: no pawn")); return; }
		const int32 Nth = Args.Num() > 0 ? FMath::Max(0, FCString::Atoi(*Args[0])) : 0;
		const float Pitch = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 40.f;
		const FVector From = Pawn->GetActorLocation();
		TArray<TPair<double, FTransform>> Found;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TInlineComponentArray<UInstancedStaticMeshComponent*> Comps(*It);
			for (UInstancedStaticMeshComponent* C : Comps)
			{
				if (!C || !C->GetStaticMesh() || !C->GetStaticMesh()->GetName().Contains(TEXT("TreeClump"))) { continue; }
				for (int32 i = 0; i < C->GetInstanceCount(); ++i)
				{
					FTransform T;
					if (C->GetInstanceTransform(i, T, true)) { Found.Emplace(FVector::DistSquared2D(T.GetLocation(), From), T); }
				}
			}
		}
		if (Found.Num() == 0) { UE_LOG(LogTemp, Warning, TEXT("TD.Env.UnderClump: no clumps")); return; }
		Found.Sort([](const TPair<double, FTransform>& A, const TPair<double, FTransform>& B) { return A.Key < B.Key; });
		const FTransform& T = Found[FMath::Min(Nth, Found.Num() - 1)].Value;
		const FVector At = T.GetLocation();
		FHitResult Hit;
		FCollisionQueryParams Q(TEXT("TDUnderClump"), false, Pawn);
		FVector Ground = At;
		if (World->LineTraceSingleByChannel(Hit, At + FVector(0, 0, 20000.f), At - FVector(0, 0, 20000.f), ECC_Visibility, Q)) { Ground = Hit.ImpactPoint; }
		Pawn->SetActorLocation(Ground + FVector(0, 0, Pawn->GetSimpleCollisionHalfHeight() + 5.f), false, nullptr, ETeleportType::TeleportPhysics);
		PC->SetControlRotation(FRotator(Pitch, PC->GetControlRotation().Yaw, 0.f));
		UE_LOG(LogTemp, Display, TEXT("TD.Env.UnderClump #%d of %d at %s ground %.0f scale %.2f dist %.0f m"), Nth, Found.Num(), *At.ToString(), Ground.Z,
			T.GetScale3D().X, FMath::Sqrt(Found[FMath::Min(Nth, Found.Num() - 1)].Key) / 100.0);
	}));

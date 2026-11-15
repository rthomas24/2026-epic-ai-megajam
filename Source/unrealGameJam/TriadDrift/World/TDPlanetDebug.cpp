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

// Fab dressing captures: stand the player D m from the Nth static mesh whose mesh name contains <Sub> (on the side the player
// came from), facing it.   TD.Env.GotoMesh <Sub> [N] [D m = 5] [side yaw offset deg = 0]
static FAutoConsoleCommandWithWorldAndArgs GTDGotoMeshCmd(
	TEXT("TD.Env.GotoMesh"),
	TEXT("Dev: stand the player D m (default 5) from the Nth mesh whose name contains <Sub>, facing it. [N] [D] [yaw offset]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Pawn || Args.Num() < 1) { return; }
		const int32 Nth = Args.Num() > 1 ? FCString::Atoi(*Args[1]) : 0;
		const float D = (Args.Num() > 2 ? FCString::Atof(*Args[2]) : 5.f) * 100.f;
		const float Off = Args.Num() > 3 ? FCString::Atof(*Args[3]) : 0.f;
		TArray<UStaticMeshComponent*> Found;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TInlineComponentArray<UStaticMeshComponent*> Comps(*It);
			for (UStaticMeshComponent* C : Comps)
			{
				if (C && !C->IsA<UInstancedStaticMeshComponent>() && C->GetStaticMesh() && C->GetStaticMesh()->GetName().Contains(Args[0])) { Found.Add(C); }
			}
		}
		if (!Found.IsValidIndex(Nth)) { UE_LOG(LogTemp, Warning, TEXT("TD.Env.GotoMesh %s: %d found"), *Args[0], Found.Num()); return; }
		const FVector At = Found[Nth]->Bounds.Origin;
		FVector Dir = (Pawn->GetActorLocation() - At).GetSafeNormal2D();
		if (Dir.IsNearlyZero()) { Dir = FVector::ForwardVector; }
		Dir = Dir.RotateAngleAxis(Off, FVector::UpVector);
		// The first of 8 directions (from the one the player came from) with a clear line to the prop and a floor under it:
		// in a room, never outside its wall.
		FHitResult Hit;
		FCollisionQueryParams Q(TEXT("TDGotoMesh"), false, Pawn);
		Q.AddIgnoredComponent(Found[Nth]);
		FVector P = At + Dir * D;
		for (int32 k = 0; k < 8; ++k)
		{
			const FVector Try = At + Dir.RotateAngleAxis(45.f * k, FVector::UpVector) * D;
			FHitResult Floor;
			const bool bFloor = World->LineTraceSingleByChannel(Floor, Try + FVector(0, 0, 150.f), Try - FVector(0, 0, 3000.f), ECC_Visibility, Q);
			const bool bBlocked = World->LineTraceTestByChannel(At + FVector(0, 0, 60.f), Try + FVector(0, 0, 60.f), ECC_Visibility, Q);
			if (bFloor && !bBlocked) { P = Try; break; }
		}
		if (World->LineTraceSingleByChannel(Hit, P + FVector(0, 0, 150.f), P - FVector(0, 0, 3000.f), ECC_Visibility, Q)) { P.Z = Hit.ImpactPoint.Z; }
		Pawn->SetActorLocation(P + FVector(0, 0, Pawn->GetSimpleCollisionHalfHeight() + 5.f), false, nullptr, ETeleportType::TeleportPhysics);
		const FRotator Face = (At - P).Rotation();
		PC->SetControlRotation(FRotator(-14.f, Face.Yaw + 22.f, 0.f));	// a little to the side: the pilot does not hide the prop
		UE_LOG(LogTemp, Display, TEXT("TD.Env.GotoMesh %s #%d of %d at %s (owner %s)"), *Args[0], Nth, Found.Num(), *At.ToCompactString(), *GetNameSafe(Found[Nth]->GetOwner()));
	}));

// Route captures: stand the player on the ground at world X Y (cm), facing Yaw.   TD.Env.GroundAt X Y Yaw [Pitch]
static FAutoConsoleCommandWithWorldAndArgs GTDGroundAtCmd(
	TEXT("TD.Env.GroundAt"),
	TEXT("Dev: stand the player on the ground at world X Y (cm), facing Yaw [Pitch = -8]."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Pawn || Args.Num() < 3) { return; }
		FVector P(FCString::Atof(*Args[0]), FCString::Atof(*Args[1]), 0.f);
		FHitResult Hit;
		FCollisionQueryParams Q(TEXT("TDGroundAt"), false, Pawn);
		if (World->LineTraceSingleByChannel(Hit, P + FVector(0, 0, 200000.f), P - FVector(0, 0, 200000.f), ECC_Visibility, Q)) { P.Z = Hit.ImpactPoint.Z; }
		Pawn->SetActorLocation(P + FVector(0, 0, Pawn->GetSimpleCollisionHalfHeight() + 5.f), false, nullptr, ETeleportType::TeleportPhysics);
		PC->SetControlRotation(FRotator(Args.Num() > 3 ? FCString::Atof(*Args[3]) : -8.f, FCString::Atof(*Args[2]), 0.f));
		UE_LOG(LogTemp, Display, TEXT("TD.Env.GroundAt %s (hit %s)"), *P.ToCompactString(), *GetNameSafe(Hit.GetActor()));
	}));

// #194: count instanced dressing in the world (components, instances, shadow casters).   TD.Env.Instances
static FAutoConsoleCommandWithWorldAndArgs GTDEnvInstancesCmd(
	TEXT("TD.Env.Instances"),
	TEXT("Dev: log instanced mesh components, instances and how many cast shadows."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World) { return; }
		int32 Comps = 0, Inst = 0, ShadowInst = 0, Static = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TInlineComponentArray<UStaticMeshComponent*> Cs(*It);
			for (UStaticMeshComponent* C : Cs)
			{
				if (UInstancedStaticMeshComponent* I = Cast<UInstancedStaticMeshComponent>(C)) { ++Comps; Inst += I->GetInstanceCount(); if (I->CastShadow) { ShadowInst += I->GetInstanceCount(); } }
				else if (C) { ++Static; }
			}
		}
		UE_LOG(LogTemp, Display, TEXT("TD.Env.Instances: %d instanced components, %d instances (%d casting shadow), %d other static mesh components"), Comps, Inst, ShadowInst, Static);
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

// Triad Drift - World (Teammate 4). Dev console commands for playtests and captures (not shipped as cheats).
//   TD.World.Goto <Dominion|Reactor|Verdance|Cinder|Gate|Belt|Start>   teleport the player's ship to a viewing spot
//   TD.World.SpawnEnemy <ShardFighter|HookInterceptor|WallGunship|MaulBomber|HiveCarrier> [dist] [pilot 0/1] [side]
//                                                                   an enemy in front of the player, facing it
//   TD.World.Dominion <Reveal|DropShields>                          drive the finale for captures
//   TD.Spawn <Id> [count] [distance]                                live, piloted enemies ahead of the player

#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/World/TDPlanetTerrain.h"
#include "TriadDrift/World/TDDistortionField.h"
#include "TriadDrift/World/TDVerdanceApproach.h"
#include "TimerManager.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/World/TDShieldGenerator.h"
#include "EngineUtils.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "TriadDrift/World/TDPlanetAtmosphere.h"
#include "TriadDrift/World/TDPortalGate.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Core/TDTypes.h"

#include "TriadDrift/Combat/TDWeaponLibrary.h"
#include "TriadDrift/Combat/TDProjectile.h"
#include "TriadDrift/Waves/TDWaveDirector.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

#if !UE_BUILD_SHIPPING

namespace TDWorldDebug
{
	void PlaceFacing(APawn* Pawn, const FVector& Loc, const FRotator& Facing)
	{
		UE_LOG(LogTriadDrift, Display, TEXT("TD.World.Goto: %s -> %s"), Pawn ? *Pawn->GetName() : TEXT("none"), *Loc.ToString());
		if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(Pawn)) { Craft->TeleportSpacecraft(Loc, Facing, false); }
		else if (Pawn) { Pawn->SetActorLocationAndRotation(Loc, Facing); }
	}

	void Place(APawn* Pawn, const FVector& Loc, const FVector& LookAt)
	{
		PlaceFacing(Pawn, Loc, (LookAt - Loc).Rotation());
	}

	void Goto(const TArray<FString>& Args, UWorld* World)
	{
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
		ATDGalaxyLayout* Layout = ATDGalaxyLayout::Get(World);
		if (!Pawn || !Layout || Args.Num() == 0)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.World.Goto <Dominion|Reactor|Verdance|Cinder|Gate|Belt|Start> (needs a player pawn and a galaxy layout)"));
			return;
		}
		const FString& Where = Args[0];
		if (Where.Equals(TEXT("Dominion"), ESearchCase::IgnoreCase) || Where.Equals(TEXT("Reactor"), ESearchCase::IgnoreCase))
		{
			ATDDominion* D = Layout->GetDominion();
			if (!D) { return; }
			const bool bReactor = Where.Equals(TEXT("Reactor"), ESearchCase::IgnoreCase) && D->GetReactor();
			const FVector Focus = bReactor ? D->GetReactor()->GetActorLocation() : D->GetActorLocation();
			const FVector Eye = bReactor ? Focus + D->GetActorForwardVector() * 9000.f + FVector(0, 0, 4000.f)
				: Focus + D->GetActorForwardVector() * 70000.f + D->GetActorRightVector() * 25000.f + FVector(0, 0, 15000.f);
			Place(Pawn, Eye, Focus);
		}
		else if (Where.Equals(TEXT("Verdance"), ESearchCase::IgnoreCase) || Where.Equals(TEXT("Cinder"), ESearchCase::IgnoreCase))
		{
			for (ATDPlanetAtmosphere* P : Layout->GetPlanets())
			{
				const bool bMatch = (P->GetDestination() == ETDDestination::Verdance) == Where.Equals(TEXT("Verdance"), ESearchCase::IgnoreCase);
				if (!bMatch) { continue; }
				FTransform Orbit;
				if (Layout->FindReturnPoint(P->GetReturnPoint(), Orbit)) { PlaceFacing(Pawn, Orbit.GetLocation(), Orbit.Rotator()); }	// the orbit point's own facing (home), never into the planet
			}
		}
		else if (Where.Equals(TEXT("Gate"), ESearchCase::IgnoreCase))
		{
			if (ATDPortalGate* G = Layout->FindGate(TEXT("Gate_Verdance_Zone")))
			{
				Place(Pawn, G->GetActorLocation() - G->GetActorForwardVector() * 9000.f + FVector(0, 0, 1500.f), G->GetActorLocation());
			}
		}
		else if (Where.Equals(TEXT("Belt"), ESearchCase::IgnoreCase))
		{
			Place(Pawn, FVector(-48000.f, -20000.f, 9000.f), FVector::ZeroVector);
		}
		else
		{
			Place(Pawn, Layout->GetPlayerStart().GetLocation(), Layout->GetPlayerStart().GetLocation() + Layout->GetPlayerStart().GetRotation().GetForwardVector());
		}
	}

	void SpawnEnemy(const TArray<FString>& Args, UWorld* World)
	{
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
		const FName Id = Args.Num() > 0 ? FName(*Args[0]) : TDEnemyIds::ShardFighter;
		if (!Pawn || !World || !UTDEnemyLibrary::HasEnemy(Id))
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.World.SpawnEnemy <ShardFighter|HookInterceptor|WallGunship|MaulBomber|HiveCarrier> [distance cm] [pilot 0/1] [side cm]"));
			return;
		}
		const float Dist = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 3000.f;
		const bool bPilot = Args.Num() > 2 ? FCString::Atoi(*Args[2]) != 0 : false;
		const float Side = Args.Num() > 3 ? FCString::Atof(*Args[3]) : 0.f;
	const FVector Loc = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * FMath::Max(500.f, Dist) + Pawn->GetActorRightVector() * Side;
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ATDEnemyShip* E = World->SpawnActor<ATDEnemyShip>(ATDEnemyShip::StaticClass(), Loc, (Pawn->GetActorLocation() - Loc).Rotation() + FRotator(0.f, 35.f, 0.f), P);
		if (!E) { return; }
		if (!bPilot)
		{
			if (AController* C = E->GetController()) { C->UnPossess(); C->Destroy(); }
			E->SetSpeedOverrideFor(TEXT("Debug"), 0.f, 0.f);	// hold still for a close-up
		}
		E->InitFromSpec(Id);
		UE_LOG(LogTriadDrift, Display, TEXT("TD.World.SpawnEnemy: %s at %s (player at %s)"), *Id.ToString(), *Loc.ToString(), *Pawn->GetActorLocation().ToString());
	}

	/** TD.Spawn <Id> [count] [distance]: live, piloted enemies ahead of the player, spread in a fan (for playing
	 *  the never-seen-running power cases: flak, portal returns, dome, EMP, cloak, ram). */
	void SpawnLive(const TArray<FString>& Args, UWorld* World)
	{
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
		const FName Id = Args.Num() > 0 ? FName(*Args[0]) : TDEnemyIds::ShardFighter;
		if (!Pawn || !World || !UTDEnemyLibrary::HasEnemy(Id))
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Spawn <ShardFighter|HookInterceptor|WallGunship|MaulBomber|HiveCarrier> [count=1] [distance cm=12000]"));
			return;
		}
		const int32 Count = Args.Num() > 1 ? FMath::Clamp(FCString::Atoi(*Args[1]), 1, 40) : 1;
		const float Dist = Args.Num() > 2 ? FMath::Max(1000.f, FCString::Atof(*Args[2])) : 12000.f;
		for (int32 i = 0; i < Count; ++i)
		{
			const float Spread = Count > 1 ? FMath::Lerp(-40.f, 40.f, float(i) / (Count - 1)) : 0.f;
			const FVector Dir = Pawn->GetActorForwardVector().RotateAngleAxis(Spread, FVector::UpVector);
			const FVector Loc = Pawn->GetActorLocation() + Dir * Dist + FVector(0.f, 0.f, (i % 3 - 1) * 800.f);
			FActorSpawnParameters P;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (ATDEnemyShip* E = World->SpawnActor<ATDEnemyShip>(ATDEnemyShip::StaticClass(), Loc, (Pawn->GetActorLocation() - Loc).Rotation(), P))
			{
				E->InitFromSpec(Id);
			}
		}
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Spawn: %d x %s at %.0f cm ahead of %s"), Count, *Id.ToString(), Dist, *Pawn->GetName());
	}

	void Dominion(const TArray<FString>& Args, UWorld* World)
	{
		ATDDominion* D = ATDDominion::Get(World);
		if (!D || Args.Num() == 0) { return; }
		if (Args[0].Equals(TEXT("Reveal"), ESearchCase::IgnoreCase)) { D->WarpIn(); }
		else if (Args[0].Equals(TEXT("DropShields"), ESearchCase::IgnoreCase)) { D->DebugDropShields(); }
	}

	AActor* DominionTarget(UWorld* World, const TArray<FString>& Args)
	{
		// The tutorial's escape marker (ATDWaveDirector::BeginWithdrawal), for flying the withdrawal through the real input.
		if (Args.Num() > 0 && Args[0].Equals(TEXT("Escape"), ESearchCase::IgnoreCase))
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (It->ActorHasTag(TEXT("TD.Tutorial.Escape"))) { return *It; }
			}
			return nullptr;
		}
		ATDDominion* D = nullptr;
		for (TActorIterator<ATDDominion> It(World); It && !D; ++It) { D = *It; }
		if (!D || Args.Num() == 0) { return nullptr; }
		if (Args[0].Equals(TEXT("Reactor"), ESearchCase::IgnoreCase)) { return D->GetReactor(); }
		const TArray<ATDShieldGenerator*> Gens = D->GetGenerators();
		const int32 I = Args.IsValidIndex(1) ? FCString::Atoi(*Args[1]) : 0;
		return Gens.IsValidIndex(I) ? Gens[I] : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs CmdAimAt(TEXT("TD.World.AimAt"), TEXT("Steer the player's nose at a target through the real input (TD.Input.TurnToward): Generator <i> | Reactor [seconds] | Escape [seconds] (the tutorial withdrawal marker)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
			AActor* Target = DominionTarget(World, Args);
			if (!Pawn || !Target) { UE_LOG(LogTriadDrift, Warning, TEXT("TD.World.AimAt Generator <i> | Reactor [seconds]: no pawn or target")); return; }
			const FRotator R = (Target->GetActorLocation() - Pawn->GetActorLocation()).Rotation();
			const float Secs = Args.Num() > 2 ? FCString::Atof(*Args[2]) : (Args.Num() == 2 && (Args[0].Equals(TEXT("Reactor"), ESearchCase::IgnoreCase) || Args[0].Equals(TEXT("Escape"), ESearchCase::IgnoreCase)) ? FCString::Atof(*Args[1]) : 2.f);
			UE_LOG(LogTriadDrift, Display, TEXT("TD.World.AimAt %s: yaw %.1f pitch %.1f, %.0f m"), *Target->GetName(), R.Yaw, R.Pitch, FVector::Dist(Target->GetActorLocation(), Pawn->GetActorLocation()) / 100.f);
			GEngine->Exec(World, *FString::Printf(TEXT("TD.Input.TurnToward %.2f %.2f %.2f"), R.Yaw, R.Pitch, FMath::Max(0.5f, Secs)));
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdDominionStatus(TEXT("TD.World.DominionStatus"), TEXT("Log the Dominion state and every generator / reactor hull."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			for (TActorIterator<ATDDominion> It(World); It; ++It)
			{
				UE_LOG(LogTriadDrift, Display, TEXT("TD.World.DominionStatus: state %d, generators left %d"), (int32)It->GetState(), It->GetGeneratorsRemaining());
				for (ATDShieldGenerator* G : It->GetGenerators())
				{
					const UTDHealthComponent* H = G ? G->GetHealth() : nullptr;
					UE_LOG(LogTriadDrift, Display, TEXT("  %s hull %.0f / %.0f"), G ? *G->GetName() : TEXT("-"), H ? H->GetHull() : -1.f, H ? H->MaxHull : -1.f);
				}
				if (ATDDominionReactor* R = It->GetReactor())
				{
					const UTDHealthComponent* H = R->GetHealth();
					UE_LOG(LogTriadDrift, Display, TEXT("  reactor hull %.0f / %.0f"), H ? H->GetHull() : -1.f, H ? H->MaxHull : -1.f);
				}
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdDominionParts(TEXT("TD.World.DominionParts"), TEXT("Log every visible primitive of the Dominion and its parts: mesh, material, size, distance to the player."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
			for (TActorIterator<ATDDominion> It(World); It; ++It)
			{
				TArray<AActor*> Actors;
				It->GetAttachedActors(Actors, true, true);
				Actors.Insert(*It, 0);
				for (AActor* A : Actors)
				{
					TInlineComponentArray<UPrimitiveComponent*> Prims(A);
					for (UPrimitiveComponent* P : Prims)
					{
						if (!P || !P->IsVisible() || A->IsHidden()) { continue; }
						const UStaticMeshComponent* SM = Cast<UStaticMeshComponent>(P);
						const UMaterialInterface* M = P->GetNumMaterials() > 0 ? P->GetMaterial(0) : nullptr;
						for (int32 Slot = 1; Slot < P->GetNumMaterials(); ++Slot)
						{
							const UMaterialInterface* SM2 = P->GetMaterial(Slot);
							UE_LOG(LogTriadDrift, Display, TEXT("TD.World.DominionParts:   %s.%s slot %d mat %s"), *A->GetName(), *P->GetName(), Slot, SM2 ? *SM2->GetName() : TEXT("-"));
						}
						UE_LOG(LogTriadDrift, Display, TEXT("TD.World.DominionParts: %s.%s mesh %s mat %s size %s m, %.0f m from player%s"),
							*A->GetName(), *P->GetName(), SM && SM->GetStaticMesh() ? *SM->GetStaticMesh()->GetName() : TEXT("-"),
							M ? *M->GetName() : TEXT("-"), *(P->Bounds.BoxExtent * 0.02f).ToCompactString(),
							Pawn ? FVector::Dist(P->Bounds.Origin, Pawn->GetActorLocation()) / 100.f : -1.f,
							Cast<UInstancedStaticMeshComponent>(P) ? *FString::Printf(TEXT(" (%d instances)"), Cast<UInstancedStaticMeshComponent>(P)->GetInstanceCount()) : TEXT(""));
					}
				}
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdKillNearest(TEXT("TD.World.KillNearest"), TEXT("Kill the nearest live enemy ship through the real damage path (its death and explosion play): [max distance m = 1000]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
			if (!Pawn) { return; }
			const float MaxCm = (Args.Num() > 0 ? FCString::Atof(*Args[0]) : 1000.f) * 100.f;
			ATDEnemyShip* Best = nullptr;
			float BestD = MaxCm;
			for (TActorIterator<ATDEnemyShip> It(World); It; ++It)
			{
				const float D = FVector::Dist(It->GetActorLocation(), Pawn->GetActorLocation());
				if (It->IsAliveEnemy() && D < BestD) { Best = *It; BestD = D; }
			}
			if (!Best) { UE_LOG(LogTriadDrift, Warning, TEXT("TD.World.KillNearest: no live enemy in range")); return; }
			UE_LOG(LogTriadDrift, Display, TEXT("TD.World.KillNearest: %s at %.0f m"), *Best->GetName(), BestD / 100.f);
			UTDStatics::ApplyTDDamage(Best, 1000000.f, ETDDamageKind::Explosive, Pawn, Pawn);
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdHold(TEXT("TD.World.Hold"), TEXT("Hold the player's ship at a speed for the camera (trailer shots): <seconds|0 to release> [speed m/s = 0]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ATDSpacecraft* Craft = Cast<ATDSpacecraft>(UGameplayStatics::GetPlayerPawn(World, 0));
			if (!Craft) { return; }
			static const FName HoldOwner(TEXT("TD.World.Hold"));
			const float Secs = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 10.f;
			if (Secs <= 0.f) { Craft->ClearSpeedOverrideFor(HoldOwner); UE_LOG(LogTriadDrift, Display, TEXT("TD.World.Hold: released")); return; }
			const float Speed = Args.Num() > 1 ? FCString::Atof(*Args[1]) * 100.f : 0.f;
			Craft->SetSpeedOverrideFor(HoldOwner, Speed, 1.f, 50000.f);
			TWeakObjectPtr<ATDSpacecraft> Weak(Craft);
			FTimerHandle H;
			World->GetTimerManager().SetTimer(H, [Weak]() { if (Weak.IsValid()) { Weak->ClearSpeedOverrideFor(HoldOwner); } }, Secs, false);
			UE_LOG(LogTriadDrift, Display, TEXT("TD.World.Hold: %.0f m/s for %.1f s"), Speed / 100.f, Secs);
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdPlanetGoto(TEXT("TD.World.PlanetGoto"), TEXT("On a planet: put the player's ship over the terrain: <x km> <y km> <m above ground> [yaw] [pitch] (course-local)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ATDPlanetTerrain* T = nullptr;
			for (TActorIterator<ATDPlanetTerrain> It(World); It && !T; ++It) { T = *It; }
			APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
			if (!T || !Pawn || Args.Num() < 3) { UE_LOG(LogTriadDrift, Warning, TEXT("TD.World.PlanetGoto <x km> <y km> <m above ground> [yaw] [pitch]: needs a planet terrain and a pawn")); return; }
			const FVector Local(FCString::Atof(*Args[0]) * 100000.f, FCString::Atof(*Args[1]) * 100000.f, 0.f);
			FVector P = T->GetActorTransform().TransformPosition(Local);
			P.Z = T->GetHeightAt(P) + FCString::Atof(*Args[2]) * 100.f;
			const FRotator R(Args.Num() > 4 ? FCString::Atof(*Args[4]) : 0.f, Args.Num() > 3 ? FCString::Atof(*Args[3]) : 0.f, 0.f);
			PlaceFacing(Pawn, P, R);
		}));

	FAutoConsoleCommandWithWorldAndArgs CmdPlanetView(TEXT("TD.World.PlanetView"), TEXT("Look at a planet's globe: <Verdance|Cinder> <x> <y> <z> (direction in the planet's frame) [radii out = 3]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
			if (!Pawn || Args.Num() < 4) { return; }
			const bool bCinder = Args[0].Equals(TEXT("Cinder"), ESearchCase::IgnoreCase);
			for (TActorIterator<ATDPlanetAtmosphere> It(World); It; ++It)
			{
				if ((It->GetDestination() == ETDDestination::Cinder) != bCinder) { continue; }
				const FVector Dir = It->GetGlobeQuat().RotateVector(FVector(FCString::Atof(*Args[1]), FCString::Atof(*Args[2]), FCString::Atof(*Args[3]))).GetSafeNormal();
				const float Radii = Args.Num() > 4 ? FCString::Atof(*Args[4]) : 3.f;
				const FVector P = It->GetPlanetCenter() + Dir * It->GetPlanetRadius() * Radii;
				PlaceFacing(Pawn, P, (It->GetPlanetCenter() - P).Rotation());
				if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(Pawn)) { Craft->SetFlightParked(true); }
				UE_LOG(LogTriadDrift, Display, TEXT("TD.World.PlanetView: %s from (%s), baked %d"), *Args[0], *Dir.ToCompactString(), It->IsGlobeBaked() ? 1 : 0);
				break;
			}
		}));
	FAutoConsoleCommandWithWorldAndArgs CmdPhase(TEXT("TD.World.Phase"), TEXT("Set the run's phase in place (no travel): <Waves1to2|Verdance|Waves3to4|Cinder|Wave5|DominionAssault|Escape|...>."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const UEnum* E = StaticEnum<ETDGamePhase>();
			ITDGameFlow* Flow = UTDStatics::GetGameFlow(World);
			const int64 V = (E && Args.Num() > 0) ? E->GetValueByNameString(Args[0]) : INDEX_NONE;
			if (!Flow || V == INDEX_NONE) { UE_LOG(LogTriadDrift, Warning, TEXT("TD.World.Phase <Phase>")); return; }
			Flow->SetPhase(static_cast<ETDGamePhase>(V));
		}));
	FAutoConsoleCommandWithWorldAndArgs CmdApproachView(TEXT("TD.World.ApproachView"), TEXT("The way to Verdance: put the ship near <0|1 (convoy)|beacon> [m off = 300] [side 0..1], facing it."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
			const ATDGalaxyLayout* L = ATDGalaxyLayout::Get(World);
			if (!Pawn || !L || Args.Num() < 1) { return; }
			const float Off = (Args.Num() > 1 ? FCString::Atof(*Args[1]) : 300.f) * 100.f;
			const float Side = Args.Num() > 2 ? FCString::Atof(*Args[2]) : 0.6f;
			int32 Convoy = 0;
			for (AActor* A : L->GetVerdanceApproach())
			{
				FVector Target;
				if (ATDArmadaConvoy* C = Cast<ATDArmadaConvoy>(A)) { if (Args[0] != FString::FromInt(Convoy++)) { continue; } Target = C->GetHaulerLocation(0); }
				else if (Cast<ATDTransponderBeacon>(A) && Args[0].Equals(TEXT("beacon"), ESearchCase::IgnoreCase)) { Target = A->GetActorLocation(); }
				else { continue; }
				const FVector P = Target + FVector(-Off, Off * Side, Off * 0.25f);
				PlaceFacing(Pawn, P, (Target - P).Rotation());
				return;
			}
		}));
	FAutoConsoleCommandWithWorldAndArgs CmdApproachEscort(TEXT("TD.World.ApproachEscort"), TEXT("Fly alongside the scannable hauler for <seconds> (the scan is a hold within 250 m)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
			const ATDGalaxyLayout* L = ATDGalaxyLayout::Get(World);
			ATDArmadaConvoy* C = nullptr;
			for (AActor* A : L ? L->GetVerdanceApproach() : TArray<TObjectPtr<AActor>>()) { if (!C) { C = Cast<ATDArmadaConvoy>(A); } }
			if (!Pawn || !C) { return; }
			const float Secs = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 4.f;
			TWeakObjectPtr<APawn> WP(Pawn);
			TWeakObjectPtr<ATDArmadaConvoy> WC(C);
			const double End = World->GetTimeSeconds() + Secs;
			FTimerHandle H;
			FTimerDelegate D;
			D.BindLambda([WP, WC, End, World]()
			{
				if (!WP.IsValid() || !WC.IsValid() || World->GetTimeSeconds() > End) { return; }
				const FVector T = WC->GetHaulerLocation(0);
				WP->SetActorLocation(T + FVector(-6000.f, 12000.f, 3000.f));
				WP->SetActorRotation((T - WP->GetActorLocation()).Rotation());
			});
			World->GetTimerManager().SetTimer(H, D, 0.02f, true);
			FTimerHandle Stop;
			World->GetTimerManager().SetTimer(Stop, FTimerDelegate::CreateLambda([World, H]() mutable { World->GetTimerManager().ClearTimer(H); }), Secs + 0.1f, false);
			UE_LOG(LogTriadDrift, Display, TEXT("TD.World.ApproachEscort: %.1f s beside the lead hauler"), Secs);
		}));
	FAutoConsoleCommandWithWorldAndArgs CmdFieldProbe(TEXT("TD.World.FieldProbe"), TEXT("Story field: put the player's ship <m> outside the distortion field, facing its centre (fly in with TD.World.Hold)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ATDDistortionField* F = nullptr;
			for (TActorIterator<ATDDistortionField> It(World); It && !F; ++It) { F = *It; }
			APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
			if (!F || !Pawn) { UE_LOG(LogTriadDrift, Warning, TEXT("TD.World.FieldProbe: no field (the passage is open) or no pawn")); return; }
			const float Out = (Args.Num() > 0 ? FCString::Atof(*Args[0]) : 600.f) * 100.f;
			const FVector Dir = (Pawn->GetActorLocation() - F->GetActorLocation()).GetSafeNormal();
			const FVector P = F->GetActorLocation() + Dir * (F->GetFieldRadius() + Out);
			PlaceFacing(Pawn, P, (F->GetActorLocation() - P).Rotation());
		}));
	FAutoConsoleCommandWithWorldAndArgs CmdFieldStatus(TEXT("TD.World.FieldStatus"), TEXT("Story field: log the field radius, every squad ship's distance to the shell, repels and the warning."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			for (TActorIterator<ATDDistortionField> It(World); It; ++It)
			{
				UE_LOG(LogTriadDrift, Display, TEXT("TD.World.FieldStatus: radius %.0f m, repels %d, warning %d"), It->GetFieldRadius() / 100.f, It->GetRepelCount(), It->IsWarningShown() ? 1 : 0);
				for (TActorIterator<ATDSpacecraft> S(World); S; ++S)
				{
					if (UTDStatics::GetTeam(*S) != ETDTeam::Player) { continue; }
					UE_LOG(LogTriadDrift, Display, TEXT("  %s: %.0f m from the shell, %.0f m/s"), *S->GetName(), (FVector::Dist(S->GetActorLocation(), It->GetActorLocation()) - It->GetFieldRadius()) / 100.f, S->GetVelocity().Size() / 100.f);
				}
			}
		}));
	FAutoConsoleCommandWithWorldAndArgs CmdDressing(TEXT("TD.World.Dressing"), TEXT("Space dressing (landmarks, lane rocks, ruin art) on or off: <0|1>."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			for (TActorIterator<ATDGalaxyLayout> It(World); It; ++It)
			{
				It->SetSpaceDressing(Args.Num() == 0 || FCString::Atoi(*Args[0]) != 0);
			}
		}));
	FAutoConsoleCommandWithWorldAndArgs CmdGoto(TEXT("TD.World.Goto"), TEXT("Teleport the player's ship to Dominion|Reactor|Verdance|Cinder|Gate|Belt|Start."), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Goto));
	FAutoConsoleCommandWithWorldAndArgs CmdSpawn(TEXT("TD.World.SpawnEnemy"), TEXT("Spawn an enemy in front of the player: <Id> [distance] [pilot 0/1]."), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnEnemy));
	// Play the withdrawal without waiting for the lessons and the heavy contact: TD.Waves.OrderWithdrawal (wave 2).
	FAutoConsoleCommandWithWorldAndArgs CmdOrderWithdrawal(TEXT("TD.Waves.OrderWithdrawal"), TEXT("Tutorial wave 2: Command orders the withdrawal now (then fly it with TD.World.AimAt Escape)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			ATDWaveDirector* D = ATDWaveDirector::Get(World);
			const bool bOk = D && D->DevOrderWithdrawal();
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Waves.OrderWithdrawal: %s"), bOk ? TEXT("ordered") : TEXT("not in the tutorial's battle (use TD.Stage Wave2 first)"));
		}));
	// Effects builder (through the Lead, Nov 14): one bomber mine ahead of the flown ship, its fuse unarmed for 10 s, so
	// the mine body can be captured.   TD.Dev.SpawnMine [metres ahead = 60]
	FAutoConsoleCommandWithWorldAndArgs CmdSpawnMine(TEXT("TD.Dev.SpawnMine"), TEXT("Drop one bomber plasma mine ahead of the flown ship, unarmed for 10 s (captures): [metres ahead = 60]."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			APawn* Pawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
			if (!Pawn) { UE_LOG(LogTriadDrift, Warning, TEXT("TD.Dev.SpawnMine: no player pawn")); return; }
			const float Metres = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 60.f;
			FTDProjectileSpec Spec = UTDWeaponLibrary::Get(TDWeaponIds::EnemyPlasmaMines).Projectile;
			Spec.FuseArmDelay = FMath::Max(Spec.FuseArmDelay, 10.f);
			Spec.bInheritShooterVelocity = false;
			const FVector At = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * Metres * 100.f;
			ATDProjectile* Mine = ATDProjectile::Spawn(World, Spec, FTransform(Pawn->GetActorRotation(), At), nullptr, ETDTeam::Enemy);
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Dev.SpawnMine: %s %.0f m ahead, fuse unarmed for %.0f s"), *GetNameSafe(Mine), Metres, Spec.FuseArmDelay);
		}));
	FAutoConsoleCommandWithWorldAndArgs CmdSpawnLive(TEXT("TD.Spawn"), TEXT("Spawn live, piloted enemies ahead of the player: <Id> [count=1] [distance cm=12000]. Ids: ShardFighter, HookInterceptor, WallGunship, MaulBomber, HiveCarrier."), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnLive));
	FAutoConsoleCommandWithWorldAndArgs CmdDominion(TEXT("TD.World.Dominion"), TEXT("Reveal | DropShields"), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Dominion));
}

#endif // !UE_BUILD_SHIPPING

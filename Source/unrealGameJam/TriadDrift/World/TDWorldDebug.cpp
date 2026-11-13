// Triad Drift - World (Teammate 4). Dev console commands for playtests and captures (not shipped as cheats).
//   TD.World.Goto <Dominion|Reactor|Verdance|Cinder|Gate|Belt|Start>   teleport the player's ship to a viewing spot
//   TD.World.SpawnEnemy <ShardFighter|HookInterceptor|WallGunship|MaulBomber|HiveCarrier> [dist] [pilot 0/1] [side]
//                                                                   an enemy in front of the player, facing it
//   TD.World.Dominion <Reveal|DropShields>                          drive the finale for captures
//   TD.Spawn <Id> [count] [distance]                                live, piloted enemies ahead of the player

#include "TriadDrift/World/TDGalaxyLayout.h"
#include "EngineUtils.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "TriadDrift/World/TDPlanetAtmosphere.h"
#include "TriadDrift/World/TDPortalGate.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Core/TDTypes.h"

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
	FAutoConsoleCommandWithWorldAndArgs CmdSpawnLive(TEXT("TD.Spawn"), TEXT("Spawn live, piloted enemies ahead of the player: <Id> [count=1] [distance cm=12000]. Ids: ShardFighter, HookInterceptor, WallGunship, MaulBomber, HiveCarrier."), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnLive));
	FAutoConsoleCommandWithWorldAndArgs CmdDominion(TEXT("TD.World.Dominion"), TEXT("Reveal | DropShields"), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Dominion));
}

#endif // !UE_BUILD_SHIPPING

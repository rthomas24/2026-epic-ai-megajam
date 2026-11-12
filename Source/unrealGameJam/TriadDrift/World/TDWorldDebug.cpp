// Triad Drift - World (Teammate 4). Dev console commands for playtests and captures (not shipped as cheats).
//   TD.World.Goto <Dominion|Reactor|Verdance|Cinder|Gate|Belt|Start>   teleport the player's ship to a viewing spot
//   TD.World.SpawnEnemy <ShardFighter|HookInterceptor|WallGunship|MaulBomber|HiveCarrier> [dist] [pilot 0/1] [side]
//                                                                   an enemy in front of the player, facing it
//   TD.World.Dominion <Reveal|DropShields>                          drive the finale for captures

#include "TriadDrift/World/TDGalaxyLayout.h"
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
	void Place(APawn* Pawn, const FVector& Loc, const FVector& LookAt)
	{
		UE_LOG(LogTriadDrift, Display, TEXT("TD.World.Goto: %s -> %s"), Pawn ? *Pawn->GetName() : TEXT("none"), *Loc.ToString());
		const FRotator Facing = (LookAt - Loc).Rotation();
		if (ATDSpacecraft* Craft = Cast<ATDSpacecraft>(Pawn)) { Craft->TeleportSpacecraft(Loc, Facing, false); }
		else if (Pawn) { Pawn->SetActorLocationAndRotation(Loc, Facing); }
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
				if (Layout->FindReturnPoint(P->GetReturnPoint(), Orbit)) { Place(Pawn, Orbit.GetLocation(), P->GetPlanetCenter()); }
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

	void Dominion(const TArray<FString>& Args, UWorld* World)
	{
		ATDDominion* D = ATDDominion::Get(World);
		if (!D || Args.Num() == 0) { return; }
		if (Args[0].Equals(TEXT("Reveal"), ESearchCase::IgnoreCase)) { D->WarpIn(); }
		else if (Args[0].Equals(TEXT("DropShields"), ESearchCase::IgnoreCase)) { D->DebugDropShields(); }
	}

	FAutoConsoleCommandWithWorldAndArgs CmdGoto(TEXT("TD.World.Goto"), TEXT("Teleport the player's ship to Dominion|Reactor|Verdance|Cinder|Gate|Belt|Start."), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Goto));
	FAutoConsoleCommandWithWorldAndArgs CmdSpawn(TEXT("TD.World.SpawnEnemy"), TEXT("Spawn an enemy in front of the player: <Id> [distance] [pilot 0/1]."), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnEnemy));
	FAutoConsoleCommandWithWorldAndArgs CmdDominion(TEXT("TD.World.Dominion"), TEXT("Reveal | DropShields"), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Dominion));
}

#endif // !UE_BUILD_SHIPPING

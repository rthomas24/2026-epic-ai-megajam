// Triad Drift - Powers (Teammate 2). Dev console commands for play-testing powers without input injection.
// Not compiled into Shipping.
//
//   TD.Power.Use <Signature1|Signature2|Ability>   press that power on the player's ship
//   TD.Power.Ship <Rift|Lancer|Bastion>            switch the player to that ship
//   TD.Power.Turn <yaw> [pitch]                    turn the player's ship (place the second portal elsewhere)
//   TD.Power.Targets [count] [distance]            spawn still Shard Fighters ahead of the player
//   TD.Power.Ready                                 every power on every ship ready now (cooldowns cleared)
//   TD.Power.TimeScale <x>                         global time dilation (capture short effects in slow motion)

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"

namespace
{
	APawn* TDPowerCheatPlayerPawn(UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		return PC ? PC->GetPawn() : nullptr;
	}

	void TDPowerCheatUse(const TArray<FString>& Args, UWorld* World)
	{
		APawn* Pawn = TDPowerCheatPlayerPawn(World);
		const FString Which = Args.Num() > 0 ? Args[0] : TEXT("Signature1");
		ETDPowerInput Slot = ETDPowerInput::Signature1;
		if (Which.Equals(TEXT("Signature2"), ESearchCase::IgnoreCase) || Which == TEXT("2"))
		{
			Slot = ETDPowerInput::Signature2;
		}
		else if (Which.Equals(TEXT("Ability"), ESearchCase::IgnoreCase) || Which.Equals(TEXT("E"), ESearchCase::IgnoreCase))
		{
			Slot = ETDPowerInput::Ability;
		}
		const bool bRouted = UTDPowerComponent::RoutePowerInput(Pawn, Slot, true);
		UTDPowerComponent* Power = UTDPowerComponent::FindPower(Pawn, Slot);
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Power.Use %s on %s: %s (active=%d)"), *Which, *GetNameSafe(Pawn),
			bRouted ? *GetNameSafe(Power) : TEXT("no power in that slot"), Power && Power->IsPowerActive());
	}

	void TDPowerCheatShip(const TArray<FString>& Args, UWorld* World)
	{
		ATDSquadManager* Squad = ATDSquadManager::Get(World);
		if (!Squad || Args.Num() == 0)
		{
			UE_LOG(LogTriadDrift, Warning, TEXT("TD.Power.Ship <Rift|Lancer|Bastion>: no squad manager or no ship given"));
			return;
		}
		const ETDShipId Ship = Args[0].Equals(TEXT("Lancer"), ESearchCase::IgnoreCase) ? ETDShipId::Lancer
			: (Args[0].Equals(TEXT("Bastion"), ESearchCase::IgnoreCase) ? ETDShipId::Bastion : ETDShipId::Rift);
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Power.Ship %s: %s"), *Args[0], Squad->RequestSwitchTo(Ship) ? TEXT("switched") : TEXT("refused"));
	}

	void TDPowerCheatTurn(const TArray<FString>& Args, UWorld* World)
	{
		APawn* Pawn = TDPowerCheatPlayerPawn(World);
		if (!Pawn || Args.Num() == 0)
		{
			return;
		}
		const FRotator R(Args.Num() > 1 ? FCString::Atof(*Args[1]) : 0.f, FCString::Atof(*Args[0]), 0.f);
		Pawn->SetActorRotation(R);
		if (AController* C = Pawn->GetController())
		{
			C->SetControlRotation(R);
		}
	}

	void TDPowerCheatTargets(const TArray<FString>& Args, UWorld* World)
	{
		APawn* Pawn = TDPowerCheatPlayerPawn(World);
		if (!Pawn || !World)
		{
			return;
		}
		const int32 Count = Args.Num() > 0 ? FMath::Clamp(FCString::Atoi(*Args[0]), 1, 12) : 5;
		const float Distance = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 9000.f;
		const FVector Fwd = Pawn->GetActorForwardVector();
		const FVector Right = Pawn->GetActorRightVector();
		for (int32 i = 0; i < Count; ++i)
		{
			const FVector Loc = Pawn->GetActorLocation() + Fwd * Distance + Right * ((i - (Count - 1) * 0.5f) * 1200.f);
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (ATDEnemyShip* Enemy = World->SpawnActor<ATDEnemyShip>(ATDEnemyShip::StaticClass(), Loc, (-Fwd).Rotation(), Params))
			{
				if (AController* C = Enemy->GetController())
				{
					C->UnPossess();
					C->Destroy();
				}
				Enemy->InitFromSpec(TDEnemyIds::ShardFighter);
			}
		}
	}

	void TDPowerCheatReady(const TArray<FString>& Args, UWorld* World)
	{
		for (TActorIterator<ATDShip> It(World); It; ++It)
		{
			TInlineComponentArray<UTDPowerComponent*> Powers(*It);
			for (UTDPowerComponent* P : Powers)
			{
				P->ResetCooldown();
			}
		}
	}

	void TDPowerCheatTimeScale(const TArray<FString>& Args, UWorld* World)
	{
		if (World && World->GetWorldSettings() && Args.Num() > 0)
		{
			World->GetWorldSettings()->SetTimeDilation(FMath::Clamp(FCString::Atof(*Args[0]), 0.0001f, 20.f));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs CmdTDPowerTimeScale(TEXT("TD.Power.TimeScale"), TEXT("TD.Power.TimeScale <x>: global time dilation."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDPowerCheatTimeScale));
	FAutoConsoleCommandWithWorldAndArgs CmdTDPowerUse(TEXT("TD.Power.Use"), TEXT("TD.Power.Use <Signature1|Signature2|Ability>: press a power on the player's ship."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDPowerCheatUse));
	FAutoConsoleCommandWithWorldAndArgs CmdTDPowerShip(TEXT("TD.Power.Ship"), TEXT("TD.Power.Ship <Rift|Lancer|Bastion>: switch ships."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDPowerCheatShip));
	FAutoConsoleCommandWithWorldAndArgs CmdTDPowerTurn(TEXT("TD.Power.Turn"), TEXT("TD.Power.Turn <yaw> [pitch]: turn the player's ship."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDPowerCheatTurn));
	FAutoConsoleCommandWithWorldAndArgs CmdTDPowerTargets(TEXT("TD.Power.Targets"), TEXT("TD.Power.Targets [count] [distance]: still Shard Fighters ahead."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDPowerCheatTargets));
	FAutoConsoleCommandWithWorldAndArgs CmdTDPowerReady(TEXT("TD.Power.Ready"), TEXT("TD.Power.Ready: clear every squad power cooldown."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDPowerCheatReady));
}

#endif // !UE_BUILD_SHIPPING

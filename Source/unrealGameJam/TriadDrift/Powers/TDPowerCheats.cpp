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
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TriadDrift/Core/TDTypes.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Powers/TDPowerComponent.h"
#include "TriadDrift/Powers/TDPortalBlasterPower.h"
#include "Engine/Engine.h"
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

	ETDPowerInput TDPowerCheatSlot(const FString& Which)
	{
		if (Which.Equals(TEXT("Signature2"), ESearchCase::IgnoreCase) || Which == TEXT("2")) { return ETDPowerInput::Signature2; }
		if (Which.Equals(TEXT("Ability"), ESearchCase::IgnoreCase) || Which.Equals(TEXT("E"), ESearchCase::IgnoreCase)) { return ETDPowerInput::Ability; }
		return ETDPowerInput::Signature1;
	}

	// Release a held power (Bastion's missile swarm fires on release with the locks it painted while held).
	void TDPowerCheatRelease(const TArray<FString>& Args, UWorld* World)
	{
		APawn* Pawn = TDPowerCheatPlayerPawn(World);
		const ETDPowerInput Slot = TDPowerCheatSlot(Args.Num() > 0 ? Args[0] : TEXT("Signature1"));
		UTDPowerComponent::RoutePowerInput(Pawn, Slot, false);
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Power.Release %s on %s"), Args.Num() > 0 ? *Args[0] : TEXT("Signature1"), *GetNameSafe(Pawn));
	}

	// Press, hold for Seconds, release: the hold-to-lock powers in a scripted session (the swarm paints, then fires).
	void TDPowerCheatHold(const TArray<FString>& Args, UWorld* World)
	{
		APawn* Pawn = TDPowerCheatPlayerPawn(World);
		const FString Which = Args.Num() > 0 ? Args[0] : TEXT("Signature1");
		const ETDPowerInput Slot = TDPowerCheatSlot(Which);
		const float Seconds = Args.Num() > 1 ? FMath::Max(.05f, FCString::Atof(*Args[1])) : 1.5f;
		if (!Pawn || !World)
		{
			return;
		}
		UTDPowerComponent::RoutePowerInput(Pawn, Slot, true);
		TWeakObjectPtr<APawn> Weak(Pawn);
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle, [Weak, Slot]()
		{
			if (APawn* P = Weak.Get())
			{
				UTDPowerComponent::RoutePowerInput(P, Slot, false);
				UE_LOG(LogTriadDrift, Display, TEXT("TD.Power.Hold: released on %s"), *GetNameSafe(P));
			}
		}, Seconds, false);
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Power.Hold %s on %s for %.1f s"), *Which, *GetNameSafe(Pawn), Seconds);
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

	// TD.Power.ToPortal [index=0] [speed m/s=300] [seconds=4]: point the player's Rift at one of its open portals and hold
	// that speed (TD.World.Hold), so a scripted session flies through it (and a missile on its tail follows).
	void TDPowerCheatToPortal(const TArray<FString>& Args, UWorld* World)
	{
		APawn* Pawn = TDPowerCheatPlayerPawn(World);
		const UTDPortalBlasterPower* Blaster = Pawn ? Pawn->FindComponentByClass<UTDPortalBlasterPower>() : nullptr;
		const int32 Index = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
		const AActor* Portal = Blaster && Blaster->GetPortals().IsValidIndex(Index) ? Cast<AActor>(Blaster->GetPortals()[Index].Get()) : nullptr;
		if (!Portal)
		{
			UE_LOG(LogTriadDrift, Display, TEXT("TD.Power.ToPortal: no open portal %d on the player's ship"), Index);
			return;
		}
		const FVector To = Portal->GetActorLocation() - Pawn->GetActorLocation();
		const FRotator R = To.Rotation();
		Pawn->SetActorRotation(R);
		if (AController* C = Pawn->GetController())
		{
			C->SetControlRotation(R);
		}
		const float Speed = Args.Num() > 1 ? FCString::Atof(*Args[1]) : 300.f;
		const float Seconds = Args.Num() > 2 ? FCString::Atof(*Args[2]) : 4.f;
		if (GEngine)
		{
			GEngine->Exec(World, *FString::Printf(TEXT("TD.World.Hold %.1f %.0f"), Seconds, Speed));
		}
		UE_LOG(LogTriadDrift, Display, TEXT("TD.Power.ToPortal: portal %d of %d is %.0f m away, flying at it at %.0f m/s for %.1f s"), Index, Blaster->GetPortals().Num(), To.Size() / 100.f, Speed, Seconds);
		// A trace of the pass, every 0.1 s for 2 s: the distance to the portal and how far off its axis the ship is, so a
		// miss (aim) can be told from a crossing that did not register.
		TWeakObjectPtr<APawn> WeakPawn(Pawn);
		TWeakObjectPtr<const AActor> WeakPortal(Portal);
		for (int32 Step = 1; Step <= 20; ++Step)
		{
			FTimerHandle H;
			World->GetTimerManager().SetTimer(H, [WeakPawn, WeakPortal, Step]()
			{
				const APawn* P = WeakPawn.Get();
				const AActor* Q = WeakPortal.Get();
				if (P && Q)
				{
					const FVector D = P->GetActorLocation() - Q->GetActorLocation();
					const float Along = FVector::DotProduct(D, Q->GetActorForwardVector());
					UE_LOG(LogTriadDrift, Display, TEXT("TD.Power.ToPortal trace %.1f s: %.0f m from the portal, %.0f m along its axis, %.0f m off it, speed %.0f m/s"),
						Step * .1f, D.Size() / 100.f, Along / 100.f, (D - Along * Q->GetActorForwardVector()).Size() / 100.f, P->GetVelocity().Size() / 100.f);
				}
				else
				{
					UE_LOG(LogTriadDrift, Display, TEXT("TD.Power.ToPortal trace %.1f s: %s"), Step * .1f, Q ? TEXT("no pawn") : TEXT("the portal is gone"));
				}
			}, Step * .1f, false);
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
	FAutoConsoleCommandWithWorldAndArgs CmdTDPowerRelease(TEXT("TD.Power.Release"), TEXT("TD.Power.Release <Signature1|Signature2|Ability>: release a held power on the player's ship."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDPowerCheatRelease));
	FAutoConsoleCommandWithWorldAndArgs CmdTDPowerHold(TEXT("TD.Power.Hold"), TEXT("TD.Power.Hold <Signature1|Signature2|Ability> [seconds=1.5]: press, hold, release (the missile swarm's locks)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDPowerCheatHold));
	FAutoConsoleCommandWithWorldAndArgs CmdTDPowerShip(TEXT("TD.Power.Ship"), TEXT("TD.Power.Ship <Rift|Lancer|Bastion>: switch ships."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDPowerCheatShip));
	FAutoConsoleCommandWithWorldAndArgs CmdTDPowerToPortal(TEXT("TD.Power.ToPortal"), TEXT("TD.Power.ToPortal [index=0] [speed m/s=300] [seconds=4]: aim the player's Rift at its open portal and hold speed."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDPowerCheatToPortal));
	FAutoConsoleCommandWithWorldAndArgs CmdTDPowerTurn(TEXT("TD.Power.Turn"), TEXT("TD.Power.Turn <yaw> [pitch]: turn the player's ship."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDPowerCheatTurn));
	FAutoConsoleCommandWithWorldAndArgs CmdTDPowerTargets(TEXT("TD.Power.Targets"), TEXT("TD.Power.Targets [count] [distance]: still Shard Fighters ahead."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDPowerCheatTargets));
	FAutoConsoleCommandWithWorldAndArgs CmdTDPowerReady(TEXT("TD.Power.Ready"), TEXT("TD.Power.Ready: clear every squad power cooldown."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&TDPowerCheatReady));
}

#endif // !UE_BUILD_SHIPPING

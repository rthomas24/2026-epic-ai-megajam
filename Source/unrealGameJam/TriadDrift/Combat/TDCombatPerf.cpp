// Triad Drift - combat game-thread timing (Teammate 2). See the header.

#include "TriadDrift/Combat/TDCombatPerf.h"

#include "HAL/IConsoleManager.h"

bool FTDCombatPerf::bEnabled = false;
uint64 FTDCombatPerf::Cycles[static_cast<int32>(ETDCombatPerf::Count)] = {};
uint32 FTDCombatPerf::Calls[static_cast<int32>(ETDCombatPerf::Count)] = {};

void FTDCombatPerf::Reset()
{
	for (int32 I = 0; I < static_cast<int32>(ETDCombatPerf::Count); ++I)
	{
		Cycles[I] = 0;
		Calls[I] = 0;
	}
}

const TCHAR* FTDCombatPerf::Name(ETDCombatPerf Which)
{
	switch (Which)
	{
	case ETDCombatPerf::Projectiles: return TEXT("Projectiles");
	case ETDCombatPerf::ProjectileLaunch: return TEXT("ProjectileLaunch");
	case ETDCombatPerf::Weapons: return TEXT("Weapons");
	case ETDCombatPerf::Targeting: return TEXT("Targeting");
	case ETDCombatPerf::Powers: return TEXT("Powers");
	case ETDCombatPerf::Pickups: return TEXT("Pickups");
	case ETDCombatPerf::FlightHUD: return TEXT("FlightHUD");
	case ETDCombatPerf::Flashes: return TEXT("Flashes");
	default: return TEXT("?");
	}
}

FString FTDCombatPerf::Report(int32 Frames)
{
	FString Out;
	for (int32 I = 0; I < static_cast<int32>(ETDCombatPerf::Count); ++I)
	{
		const ETDCombatPerf W = static_cast<ETDCombatPerf>(I);
		Out += FString::Printf(TEXT("  %-17s %8.2f ms  %7u calls  %.3f ms/frame\n"), Name(W), Milliseconds(W), Calls[I],
			Frames > 0 ? Milliseconds(W) / Frames : 0.0);
	}
	return Out;
}

#if !UE_BUILD_SHIPPING
namespace TDCombatPerfCommands
{
	static FAutoConsoleCommand CmdOn(TEXT("TD.Perf.Combat"), TEXT("TD.Perf.Combat 1|0: time combat systems on the game thread (TD.Perf.CombatDump prints and resets)."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			FTDCombatPerf::bEnabled = Args.Num() == 0 || Args[0] != TEXT("0");
			FTDCombatPerf::Reset();
		}));
	static FAutoConsoleCommand CmdDump(TEXT("TD.Perf.CombatDump"), TEXT("TD.Perf.CombatDump [frames]: log the combat timers, then reset them."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			UE_LOG(LogTemp, Display, TEXT("TD combat perf:\n%s"), *FTDCombatPerf::Report(Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0));
			FTDCombatPerf::Reset();
		}));
}
#endif

#if !UE_BUILD_SHIPPING
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "TriadDrift/Combat/TDWeaponComponent.h"

namespace TDWeaponDevCommands
{
	// Scripted captures (the Lancer tracer-origin check): hold the flown ship's trigger without input.
	static FAutoConsoleCommandWithWorldAndArgs CmdHoldFire(TEXT("TD.Weapons.HoldFire"),
		TEXT("TD.Weapons.HoldFire 1|0 [secondary]: hold or release the flown ship's primary (or secondary) trigger (dev)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
			APawn* Pawn = PC ? PC->GetPawn() : nullptr;
			UTDWeaponComponent* Guns = Pawn ? Pawn->FindComponentByClass<UTDWeaponComponent>() : nullptr;
			if (!Guns)
			{
				UE_LOG(LogTemp, Warning, TEXT("TD.Weapons.HoldFire: no flown ship with weapons."));
				return;
			}
			const bool bHold = Args.Num() == 0 || Args[0] != TEXT("0");
			const ETDWeaponSlot Slot = Args.Num() > 1 && Args[1].StartsWith(TEXT("sec")) ? ETDWeaponSlot::Secondary : ETDWeaponSlot::Primary;
			Guns->SetTriggerHeld(Slot, bHold);
			UE_LOG(LogTemp, Display, TEXT("TD.Weapons.HoldFire: %s %s"), Slot == ETDWeaponSlot::Primary ? TEXT("primary") : TEXT("secondary"), bHold ? TEXT("held") : TEXT("released"));
		}));
}
#endif

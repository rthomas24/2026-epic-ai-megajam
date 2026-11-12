// Triad Drift - debug cheats (ENG-19). Owner: Teammate 5. Logic is compiled out of Shipping.

#include "TriadDrift/Flow/TDCheatManager.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Flow/TDFlowSettings.h"
#include "TriadDrift/Flow/TDFlowReflection.h"
#include "TriadDrift/Flow/TDSpaceGameMode.h"
#include "TriadDrift/Flow/TDJourney.h"
#include "TriadDrift/Core/TDCombatRegistry.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

bool UTDCheatManager::bGodMode = false;

namespace TDCheats
{
	template <typename TEnum>
	bool ParseEnum(const FString& Text, TEnum& Out)
	{
		const UEnum* Enum = StaticEnum<TEnum>();
		if (!Enum || Text.IsEmpty())
		{
			return false;
		}
		for (int32 i = 0; i < Enum->NumEnums() - 1; ++i) // skip _MAX
		{
			const FString Short = Enum->GetNameStringByIndex(i);
			const FString Display = Enum->GetDisplayNameTextByIndex(i).ToString();
			if (Short.Equals(Text, ESearchCase::IgnoreCase) || Display.Replace(TEXT(" "), TEXT("")).Equals(Text, ESearchCase::IgnoreCase))
			{
				Out = static_cast<TEnum>(Enum->GetValueByIndex(i));
				return true;
			}
		}
		return false;
	}

	template <typename TEnum>
	FString ListEnum()
	{
		const UEnum* Enum = StaticEnum<TEnum>();
		TArray<FString> Names;
		for (int32 i = 0; Enum && i < Enum->NumEnums() - 1; ++i)
		{
			Names.Add(Enum->GetNameStringByIndex(i));
		}
		return FString::Join(Names, TEXT(", "));
	}

	UTDGameInstance* GetGI(UWorld* World)
	{
		return World ? Cast<UTDGameInstance>(World->GetGameInstance()) : nullptr;
	}

	AActor* FindWaveDirector(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}
		if (const ATDSpaceGameMode* GM = World->GetAuthGameMode<ATDSpaceGameMode>())
		{
			if (AActor* WD = GM->GetWaveDirector())
			{
				return WD;
			}
		}
		UClass* Class = TDFlowReflection::ResolveClass(UTDFlowSettings::Get()->WaveDirectorClass);
		if (!Class)
		{
			return nullptr;
		}
		for (TActorIterator<AActor> It(World, Class); It; ++It)
		{
			return *It;
		}
		return nullptr;
	}

	void ApplyGod(UWorld* World, bool bOn)
	{
		if (!World)
		{
			return;
		}
		TArray<AActor*> Players;
		if (UTDCombatRegistry* Registry = UTDCombatRegistry::Get(World))
		{
			Registry->GetActorsOfTeam(ETDTeam::Player, Players, true);
		}
		// Also cover a player pawn that isn't registered (e.g. a free-fly sandbox pawn).
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr)
			{
				Players.AddUnique(Pawn);
			}
		}
		for (AActor* Actor : Players)
		{
			if (UTDHealthComponent* Health = UTDStatics::GetHealth(Actor))
			{
				Health->SetInvulnerable(bOn);
			}
			else if (Actor)
			{
				Actor->SetCanBeDamaged(!bOn);
			}
		}
	}

	void Log(const FString& Msg)
	{
		UE_LOG(LogTriadDrift, Display, TEXT("[TD cheat] %s"), *Msg);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, Msg);
		}
	}

	/** A refused cheat: a warning in the log and the console, and red on screen for long enough to read. */
	void LogError(const FString& Msg)
	{
		UE_LOG(LogTriadDrift, Warning, TEXT("[TD cheat] %s"), *Msg);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, Msg);
		}
	}
}

void UTDCheatManager::InstallOn(APlayerController* PC)
{
#if !UE_BUILD_SHIPPING
	if (!PC || PC->GetNetMode() == NM_Client || (PC->CheatManager && PC->CheatManager->IsA<UTDCheatManager>()))
	{
		return;
	}
	PC->CheatClass = UTDCheatManager::StaticClass();
	if (PC->CheatManager)
	{
		// The engine already made the stock one (standalone allows cheats): swap it out.
		PC->CheatManager->ReceiveEndPlay();
		PC->CheatManager = nullptr;
	}
	PC->AddCheats(true);
#endif
}

// ------------------------------------------------------------------------------------------------
// Logic
// ------------------------------------------------------------------------------------------------

bool UTDCheatManager::DoPhase(UWorld* World, const FString& PhaseName)
{
#if !UE_BUILD_SHIPPING
	UTDGameInstance* GI = TDCheats::GetGI(World);
	ETDGamePhase Phase;
	if (!GI || !TDCheats::ParseEnum(PhaseName, Phase))
	{
		TDCheats::Log(FString::Printf(TEXT("TD.Phase <%s>"), *TDCheats::ListEnum<ETDGamePhase>()));
		return false;
	}
	GI->SetPhase(Phase);
	TDCheats::Log(FString::Printf(TEXT("Phase -> %s"), *UEnum::GetValueAsString(Phase)));
	return true;
#else
	return false;
#endif
}

bool UTDCheatManager::DoUnlock(UWorld* World, const FString& AbilityName, const FString& ShipName)
{
#if !UE_BUILD_SHIPPING
	UTDGameInstance* GI = TDCheats::GetGI(World);
	if (GI && AbilityName.Equals(TEXT("All"), ESearchCase::IgnoreCase))
	{
		for (const TPair<ETDAbilityId, ETDShipId>& Owner : UTDFlowSettings::Get()->AbilityOwners)
		{
			GI->UnlockAbility(Owner.Key, Owner.Value);
		}
		TDCheats::Log(TEXT("Unlocked every ability on its owner ship"));
		return true;
	}
	ETDAbilityId Ability;
	if (!GI || !TDCheats::ParseEnum(AbilityName, Ability) || Ability == ETDAbilityId::None)
	{
		TDCheats::Log(FString::Printf(TEXT("TD.Unlock <%s> [%s]"), *TDCheats::ListEnum<ETDAbilityId>(), *TDCheats::ListEnum<ETDShipId>()));
		return false;
	}
	ETDShipId Ship = ETDShipId::Rift;
	if (!TDCheats::ParseEnum(ShipName, Ship))
	{
		const ETDShipId* Owner = UTDFlowSettings::Get()->AbilityOwners.Find(Ability);
		Ship = Owner ? *Owner : ETDShipId::Rift;
	}
	GI->UnlockAbility(Ability, Ship);
	TDCheats::Log(FString::Printf(TEXT("Unlocked %s for %s"), *UEnum::GetValueAsString(Ability), *UEnum::GetValueAsString(Ship)));
	return true;
#else
	return false;
#endif
}

bool UTDCheatManager::DoGod(UWorld* World)
{
#if !UE_BUILD_SHIPPING
	bGodMode = !bGodMode;
	TDCheats::ApplyGod(World, bGodMode);
	TDCheats::Log(bGodMode ? TEXT("God mode ON") : TEXT("God mode OFF"));
	return bGodMode;
#else
	return false;
#endif
}

int32 UTDCheatManager::DoKillAll(UWorld* World)
{
#if !UE_BUILD_SHIPPING
	UTDCombatRegistry* Registry = World ? UTDCombatRegistry::Get(World) : nullptr;
	if (!Registry)
	{
		return 0;
	}
	AActor* Killer = nullptr;
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		Killer = PC->GetPawn();
	}
	TArray<AActor*> Enemies;
	Registry->GetActorsOfTeam(ETDTeam::Enemy, Enemies, true);
	int32 Killed = 0;
	for (AActor* Enemy : Enemies)
	{
		UTDHealthComponent* Health = UTDStatics::GetHealth(Enemy);
		if (Health && Health->IsAlive())
		{
			Health->Kill(Killer, ETDDamageKind::Environmental);
			++Killed;
		}
	}
	TDCheats::Log(FString::Printf(TEXT("Killed %d enemies"), Killed));
	return Killed;
#else
	return 0;
#endif
}

bool UTDCheatManager::DoSkipWave(UWorld* World)
{
#if !UE_BUILD_SHIPPING
	AActor* WD = TDCheats::FindWaveDirector(World);
	const bool bOk = TDFlowReflection::Call(WD, TEXT("SkipWave"));
	TDCheats::Log(bOk ? TEXT("Wave skipped") : TEXT("No wave director to skip"));
	return bOk;
#else
	return false;
#endif
}

bool UTDCheatManager::DoTravel(UWorld* World, const FString& DestinationName)
{
#if !UE_BUILD_SHIPPING
	UTDGameInstance* GI = TDCheats::GetGI(World);
	ETDDestination Dest;
	if (!GI || !TDCheats::ParseEnum(DestinationName, Dest))
	{
		TDCheats::Log(FString::Printf(TEXT("TD.Travel <%s>"), *TDCheats::ListEnum<ETDDestination>()));
		return false;
	}
	const bool bOk = GI->TravelTo(Dest, NAME_None);
	TDCheats::Log(bOk ? FString::Printf(TEXT("Travelling to %s"), *UEnum::GetValueAsString(Dest))
	                  : FString::Printf(TEXT("%s has no map yet"), *UEnum::GetValueAsString(Dest)));
	return bOk;
#else
	return false;
#endif
}

bool UTDCheatManager::DoTake(UWorld* World)
{
#if !UE_BUILD_SHIPPING
	UTDGameInstance* GI = TDCheats::GetGI(World);
	if (!GI)
	{
		return false;
	}
	GI->ReportPlayerDefeated();
	TDCheats::Log(TEXT("CUT! (forced retake)"));
	return true;
#else
	return false;
#endif
}

bool UTDCheatManager::DoStage(UWorld* World, const FString& StageName)
{
#if !UE_BUILD_SHIPPING
	ETDJourneyStage Stage;
	int32 Wave = 0;
	if (!FTDJourney::Parse(StageName, Stage, &Wave))
	{
		TDCheats::LogError(FString::Printf(TEXT("TD.Stage: unknown stage '%s'. Stages: %s, or Wave1..Wave5"), *StageName, *FTDJourney::ListStages()));
		return false;
	}
	const bool bOk = FTDJourney::Go(World, Stage, Wave);
	TDCheats::Log(FString::Printf(TEXT("Stage %s %s"), *StageName, bOk ? TEXT("started") : TEXT("FAILED (map missing?)")));
	return bOk;
#else
	return false;
#endif
}

// ------------------------------------------------------------------------------------------------
// Exec functions (cheat manager on the player controller)
// ------------------------------------------------------------------------------------------------

void UTDCheatManager::TDPhase(const FString& PhaseName) { DoPhase(GetWorld(), PhaseName); }
void UTDCheatManager::TDUnlock(const FString& AbilityName, const FString& ShipName) { DoUnlock(GetWorld(), AbilityName, ShipName); }
void UTDCheatManager::TDGod() { DoGod(GetWorld()); }
void UTDCheatManager::TDKillAll() { DoKillAll(GetWorld()); }
void UTDCheatManager::TDSkipWave() { DoSkipWave(GetWorld()); }
void UTDCheatManager::TDTravel(const FString& DestinationName) { DoTravel(GetWorld(), DestinationName); }
void UTDCheatManager::TDTake() { DoTake(GetWorld()); }
void UTDCheatManager::TDStage(const FString& StageName) { DoStage(GetWorld(), StageName); }

// ------------------------------------------------------------------------------------------------
// "TD.*" console commands (work with any controller)
// ------------------------------------------------------------------------------------------------

#if !UE_BUILD_SHIPPING
namespace TDCheats
{
	static FString Arg(const TArray<FString>& Args, int32 Index)
	{
		return Args.IsValidIndex(Index) ? Args[Index] : FString();
	}

	static FAutoConsoleCommandWithWorldAndArgs CmdPhase(TEXT("TD.Phase"), TEXT("TD.Phase <phase>: jump to a game phase."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { UTDCheatManager::DoPhase(World, Arg(Args, 0)); }));

	static FAutoConsoleCommandWithWorldAndArgs CmdUnlock(TEXT("TD.Unlock"), TEXT("TD.Unlock <ability> [ship]: unlock an ability."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { UTDCheatManager::DoUnlock(World, Arg(Args, 0), Arg(Args, 1)); }));

	static FAutoConsoleCommandWithWorldAndArgs CmdGod(TEXT("TD.God"), TEXT("TD.God: toggle invulnerability for the player's team."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) { UTDCheatManager::DoGod(World); }));

	static FAutoConsoleCommandWithWorldAndArgs CmdKillAll(TEXT("TD.KillAll"), TEXT("TD.KillAll: kill every enemy."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) { UTDCheatManager::DoKillAll(World); }));

	static FAutoConsoleCommandWithWorldAndArgs CmdSkipWave(TEXT("TD.SkipWave"), TEXT("TD.SkipWave: skip the current wave."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) { UTDCheatManager::DoSkipWave(World); }));

	static FAutoConsoleCommandWithWorldAndArgs CmdTravel(TEXT("TD.Travel"), TEXT("TD.Travel <destination>: travel to a destination."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { UTDCheatManager::DoTravel(World, Arg(Args, 0)); }));

	static FAutoConsoleCommandWithWorldAndArgs CmdStage(TEXT("TD.Stage"), TEXT("TD.Stage <Waves1to2|VerdanceArrival|VerdanceOnFoot|Waves3to4|CinderArrival|CinderOnFoot|Wave5|DominionAssault|ReactorExposed|Victory>: progress and world jump to that journey stage."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World) { UTDCheatManager::DoStage(World, Arg(Args, 0)); }));

	static FAutoConsoleCommandWithWorldAndArgs CmdTake(TEXT("TD.Take"), TEXT("TD.Take: force CUT! and restart the current scene as the next take."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World) { UTDCheatManager::DoTake(World); }));
}
#endif

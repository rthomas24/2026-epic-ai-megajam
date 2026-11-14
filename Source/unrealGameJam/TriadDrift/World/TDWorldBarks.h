// Triad Drift - ENG-43, T4's lane: the enemy- and world-side bark triggers (dialogs.md rows marked "new, T4").
// Owner: Teammate 4. Each id is said through T2's FTDCombatBarks (squad routing, priorities) at its dialogs.md trigger;
// this adds a per-id cooldown so a busy fight never repeats a line every frame. Nicholas owns the words.

#pragma once

#include "CoreMinimal.h"
#include "TriadDrift/Combat/TDCombatBarks.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Core/TDTypes.h"
#include "Engine/World.h"

namespace TDWorldBarkIds
{
	inline const FName NewEnemyInterceptor(TEXT("Bark_NewEnemy_Interceptor"));
	inline const FName NewEnemyGunship(TEXT("Bark_NewEnemy_Gunship"));
	inline const FName NewEnemyBomber(TEXT("Bark_NewEnemy_Bomber"));
	inline const FName NewEnemyCarrier(TEXT("Bark_NewEnemy_Carrier"));
	inline const FName MinesAhead(TEXT("Bark_MinesAhead"));
	inline const FName CarrierLaunch(TEXT("Bark_CarrierLaunch"));
	inline const FName WaveIncoming(TEXT("Bark_WaveIncoming"));
	inline const FName WaveCleared(TEXT("Bark_WaveCleared"));
	inline const FName FieldBlocked(TEXT("Bark_FieldBlocked"));
	inline const FName GateEnter(TEXT("Bark_Gate_Enter"));
	inline const FName DomGeneratorHit(TEXT("Bark_Dom_GeneratorHit"));
	inline const FName DomGeneratorDown(TEXT("Bark_Dom_GeneratorDown"));
	inline const FName DomTurrets(TEXT("Bark_Dom_Turrets"));
	inline const FName DomFieldDown(TEXT("Bark_Dom_FieldDown"));
	inline const FName DomTractor(TEXT("Bark_Dom_Tractor"));
	inline const FName DomEscapeFlight(TEXT("Bark_Dom_EscapeFlight"));
	inline const FName DomClear(TEXT("Bark_Dom_Clear"));
}

struct FTDWorldBarks
{
	/** Who says it (dialogs.md speaker column). */
	enum class EWho : uint8 { Kato, Wingman, FlownPilot };

	/**
	 * Says BarkId unless the same id was said in this world less than CooldownSeconds ago. True if requested.
	 * FlownPilot needs Ship (the flown ship); the others only the world.
	 */
	static bool Say(const UObject* WorldContext, FName BarkId, ETDBarkPriority Priority, EWho Who, float CooldownSeconds, const AActor* Ship = nullptr)
	{
		const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
		if (!World) { return false; }
		struct FLast { TWeakObjectPtr<const UWorld> World; double Time = -1e9; };
		static TMap<FName, FLast> Last;
		FLast& L = Last.FindOrAdd(BarkId);
		const double Now = World->GetTimeSeconds();
		if (L.World.Get() == World && Now - L.Time < CooldownSeconds) { return false; }
		L.World = World;
		L.Time = Now;
		UE_LOG(LogTemp, Log, TEXT("TD world bark: %s"), *BarkId.ToString());	// played sessions read the log for the trigger
		switch (Who)
		{
		case EWho::Kato: return FTDCombatBarks::Say(WorldContext, ETDShipId::Lancer, BarkId, Priority);
		case EWho::FlownPilot: return Ship ? FTDCombatBarks::SayAsPilot(Ship, BarkId, Priority) : FTDCombatBarks::SayAsWingman(WorldContext, BarkId, Priority);
		default:
			// No squad (a test world, a cutscene map): Kato says it, so the request is never lost.
			return ATDSquadManager::Get(WorldContext) ? FTDCombatBarks::SayAsWingman(WorldContext, BarkId, Priority)
				: FTDCombatBarks::Say(WorldContext, ETDShipId::Lancer, BarkId, Priority);
		}
	}
};

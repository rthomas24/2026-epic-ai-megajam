// Triad Drift - Combat (Teammate 2). See header.

#include "TriadDrift/Combat/TDCombatBarks.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogTDCombatBarks, Log, All);

namespace TDCombatBarksPrivate
{
	static TMap<FName, int32> Requests;

	static TAutoConsoleVariable<int32> CVarLog(TEXT("TD.Combat.LogBarks"), 0,
		TEXT("1: log every combat / power bark T2 asks for and whether the squad limiter played it."), ECVF_Default);
}

bool FTDCombatBarks::Say(const UObject* WorldContext, ETDShipId Speaker, FName BarkId, ETDBarkPriority Priority)
{
	using namespace TDCombatBarksPrivate;
	++Requests.FindOrAdd(BarkId);
	ATDSquadManager* Squad = WorldContext ? ATDSquadManager::Get(WorldContext) : nullptr;
	const bool bPlayed = Squad && Squad->RequestSquadBark(BarkId, Speaker, Priority);
	UE_CLOG(CVarLog.GetValueOnGameThread() != 0, LogTDCombatBarks, Display, TEXT("TD combat bark %s by %s: %s"), *BarkId.ToString(),
		*TDBarkIds::SpeakerFor(Speaker).ToString(), bPlayed ? TEXT("played") : (Squad ? TEXT("held") : TEXT("no squad")));
	return bPlayed;
}

bool FTDCombatBarks::IsPlayerFlown(const AActor* Actor)
{
	const ATDShip* Ship = Cast<ATDShip>(Actor);
	return Ship && (Ship->IsPlayerFlown() || Ship->IsPlayerControlled());
}

bool FTDCombatBarks::GetShipId(const AActor* Actor, ETDShipId& OutShip)
{
	if (const ATDShip* Ship = Cast<ATDShip>(Actor))
	{
		OutShip = Ship->GetShipId();
		return true;
	}
	return false;
}

bool FTDCombatBarks::SayAsPilot(const AActor* Ship, FName BarkId, ETDBarkPriority Priority)
{
	ETDShipId Id;
	if (!IsPlayerFlown(Ship) || !GetShipId(Ship, Id))
	{
		return false;
	}
	return Say(Ship, Id, BarkId, Priority);
}

bool FTDCombatBarks::SayAsWingman(const UObject* WorldContext, FName BarkId, ETDBarkPriority Priority, int32 Preferred)
{
	const ATDSquadManager* Squad = WorldContext ? ATDSquadManager::Get(WorldContext) : nullptr;
	if (!Squad)
	{
		return false;
	}
	const ETDShipId Flown = Squad->GetActiveShipId();
	auto IsWingman = [&](int32 I)
	{
		const ETDShipId S = static_cast<ETDShipId>(I);
		return I >= 0 && I < 3 && S != Flown && Squad->IsShipAlive(S);
	};
	int32 Pick = IsWingman(Preferred) ? Preferred : INDEX_NONE;
	for (int32 I = 0; I < 3 && Pick == INDEX_NONE; ++I)
	{
		Pick = IsWingman(I) ? I : INDEX_NONE;
	}
	return Pick != INDEX_NONE && Say(WorldContext, static_cast<ETDShipId>(Pick), BarkId, Priority);
}

int32 FTDCombatBarks::GetRequestCount(FName BarkId)
{
	const int32* N = TDCombatBarksPrivate::Requests.Find(BarkId);
	return N ? *N : 0;
}

void FTDCombatBarks::ResetRequestCounts()
{
	TDCombatBarksPrivate::Requests.Reset();
}

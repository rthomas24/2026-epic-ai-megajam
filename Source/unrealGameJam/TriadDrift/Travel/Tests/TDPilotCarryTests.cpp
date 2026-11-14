// Docking (or any travel) with a ship: that ship's pilot is the one who goes on - the interior parks that ship and puts that
// pilot on foot (ATDFirstPersonCharacter::GetOnFootPilotShip reads the last ship). Run: -Filter TriadDrift.Travel.PilotCarry

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1PilotCarry, "TriadDrift.Travel.PilotCarry.TheFlownShipGoesOnForEachShip", TD_TEST_FLAGS)
bool FTDT1PilotCarry::RunTest(const FString& Parameters)
{
	for (const ETDShipId Id : { ETDShipId::Rift, ETDShipId::Lancer, ETDShipId::Bastion })
	{
		FTDTestWorld W(UTDGameInstance::StaticClass());
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
		ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 20000.f));
		if (!TestNotNull(TEXT("gi"), GI) || !TestNotNull(TEXT("ship"), Ship)) { return false; }
		Ship->SetShipId(Id);
		Ship->SetPlayerControlled(true);
		W.SpawnLocalPlayerController(Ship);
		W.Tick();
		// The last landing was another ship's.
		GI->SetLastShip(Id == ETDShipId::Bastion ? ETDShipId::Rift : ETDShipId::Bastion);
		// The docking hands off to the interior: the level change starts.
		FCoreUObjectDelegates::PreLoadMap.Broadcast(TEXT("/Game/TriadDrift/Maps/L_DominionInterior"));
		TestEqual(*FString::Printf(TEXT("%s docked: its pilot goes on foot (the interior parks it)"), *UEnum::GetValueAsString(Id)),
			ATDFirstPersonCharacter::GetOnFootPilotShip(W.GetWorld()), Id);
	}
	{
		// On foot (no flown ship): the last ship is left as it was.
		FTDTestWorld W(UTDGameInstance::StaticClass());
		UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
		if (!GI) { return false; }
		W.SpawnPlayerController();
		GI->SetLastShip(ETDShipId::Lancer);
		FCoreUObjectDelegates::PreLoadMap.Broadcast(TEXT("/Game/TriadDrift/Maps/L_Galaxy"));
		TestEqual(TEXT("on foot: unchanged"), ATDFirstPersonCharacter::GetOnFootPilotShip(W.GetWorld()), ETDShipId::Lancer);
	}
	return true;
}

#endif

// The Lead (Nov 14): docking with each ship type ends in the interior with that ship parked in the hangar. Two halves of
// one hand-off: (1) in the galaxy, the squad flies ship X into the bay's cone; the tractor takes it, it glides in and the
// bay asks for the interior exactly once with X still the flown ship; (2) the level change records X (the descent
// subsystem's PreLoadMap), and the interior built after it parks X's hull on the pad and puts X's pilot on foot.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Travel.DockPark

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Ground/TDFirstPersonCharacter.h"
#include "TriadDrift/Ground/Planets/TDPlanetCourse.h"
#include "TriadDrift/Ground/Tests/TDGroundTestHelpers.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Squad/TDShipLibrary.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDDockingBay.h"
#include "TriadDrift/World/TDShieldGenerator.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "UObject/UObjectGlobals.h"

namespace TDT1DockPark
{
	/** A revealed Dominion on the boarding finale with its four generators down (the bay opens). */
	inline ATDDominion* BayOpen(FTDTestWorld& W)
	{
		UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
		GI->Progress.bDominionRevealed = true;
		GI->Progress.Phase = ETDGamePhase::DominionAssault;
		ATDDominion* D = W.Spawn<ATDDominion>(FVector(400000.f, 0.f, 0.f));
		D->BoardingMode = ETDBoardingMode::ForceOn;
		AActor* Killer = W.Spawn<AActor>(FVector(300000.f, 0.f, 0.f));
		W.Advance(D->ProgressPollSeconds + D->WarpInSeconds + 0.5f);
		for (ATDShieldGenerator* G : D->GetGenerators()) { UTDStatics::ApplyTDDamage(G, 1000000.f, ETDDamageKind::Explosive, Killer, Killer); W.Advance(0.2f); }
		W.Advance(0.5f);
		return D;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1DockPark, "TriadDrift.Travel.DockPark.EachShipDocksAndIsParkedInTheHangar", TD_TEST_FLAGS)
bool FTDT1DockPark::RunTest(const FString& Parameters)
{
	for (const ETDShipId Id : { ETDShipId::Rift, ETDShipId::Lancer, ETDShipId::Bastion })
	{
		const FString Name = UEnum::GetValueAsString(Id);
		// (1) The galaxy: the squad flies Id into the cone.
		{
			FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
			UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
			ATDDominion* D = TDT1DockPark::BayOpen(W);
			ATDDockingBay* Bay = D ? D->GetDockingBay() : nullptr;
			if (!TestNotNull(*(Name + TEXT(": bay open")), Bay)) { return false; }
			APlayerController* PC = W.SpawnPlayerController();
			ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
			if (!TestNotNull(TEXT("pc"), PC) || !TestNotNull(TEXT("squad"), Squad)) { return false; }
			Squad->SetSquadPlayerController(PC);
			// The squad forms up 2.5 km out on the axis, outside the cone's pull until the flown ship moves in.
			const FVector Out = Bay->GetOutward();
			Squad->SpawnSquad(FTransform((-Out).Rotation(), Bay->GetMouth() + Out * 250000.f));
			W.Advance(0.5f);
			if (Squad->GetActiveShipId() != Id)
			{
				Squad->RequestSwitchTo(Id);
				W.Advance(0.6f);
			}
			ATDShip* Flown = Squad->GetActiveShip();
			if (!TestTrue(*(Name + TEXT(": flown")), Flown && Squad->GetActiveShipId() == Id && Flown->IsPlayerFlown())) { return false; }
			// 400 m out on the axis, nose on the mouth: the player's final approach.
			Flown->TeleportSpacecraft(Bay->GetMouth() + Out * 40000.f, (-Out).Rotation(), false);
			bool bHeld = false;
			for (float T = 0.f; T < 30.f && Bay->GetDockingState() != ETDDockingState::Docked; T += 1.f / 60.f)
			{
				W.Tick(1.f / 60.f);
				bHeld |= Bay->GetDockingState() == ETDDockingState::Gliding;
			}
			TestTrue(*(Name + TEXT(": the tractor takes it and it glides in")), bHeld);
			TestEqual(*(Name + TEXT(": docked")), (int32)Bay->GetDockingState(), (int32)ETDDockingState::Docked);
			TestTrue(*(Name + TEXT(": the bay asks for the interior once")), GI->Travels.Num() == 1 && GI->Travels[0] == ETDDestination::DominionInterior);
			TestEqual(*(Name + TEXT(": the ship that docked is still the flown one")), Squad->GetActiveShipId(), Id);
			TestTrue(*(Name + TEXT(": ... and it is inside the mouth")),
				FVector::DotProduct(Squad->GetActiveShip()->GetActorLocation() - Bay->GetMouth(), Out) < 0.f);
		}
		// (2) The level change and the interior that loads after it.
		{
			TDGroundTest::FNoSaveWrites NoWrites;
			FTDTestWorld W(UTDGameInstance::StaticClass());
			UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
			ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 20000.f));
			if (!TestNotNull(TEXT("gi"), GI) || !TestNotNull(TEXT("ship"), Ship)) { return false; }
			GI->SetSaveSlotOverride(TEXT("TriadDrift_AutomationTest"));
			Ship->SetShipId(Id);
			Ship->SetPlayerControlled(true);
			W.SpawnLocalPlayerController(Ship);
			W.Tick();
			GI->SetLastShip(Id == ETDShipId::Bastion ? ETDShipId::Rift : ETDShipId::Bastion); // the last landing was another ship's
			FCoreUObjectDelegates::PreLoadMap.Broadcast(TEXT("/Game/TriadDrift/Maps/L_Dominion_Interior"));
			TestEqual(*(Name + TEXT(": the docked ship is recorded at the level change")), GI->GetProgress().LastShip, Id);

			GI->NotifyLevelEntered(ETDDestination::DominionInterior);
			GI->GetMutableProgress().Phase = ETDGamePhase::DominionInterior;
			FActorSpawnParameters Params;
			Params.bDeferConstruction = true;
			ATDPlanetCourse* Course = W.GetWorld()->SpawnActor<ATDPlanetCourse>(ATDPlanetCourse::StaticClass(), FTransform::Identity, Params);
			if (!TestNotNull(TEXT("interior"), Course)) { return false; }
			Course->PlanetId = TEXT("DominionInterior");
			Course->bInteriorTravel = false;
			Course->FinishSpawning(FTransform::Identity);
			W.Advance(0.2f);
			TestEqual(*(Name + TEXT(": its pilot is on foot")), ATDFirstPersonCharacter::GetOnFootPilotShip(W.GetWorld()), Id);
			const UTDShipVisuals* Art = UTDShipLibrary::LoadVisuals(UTDShipLibrary::Get(Id));
			// The hangar parks the hull mesh, or (parts-only art) the first part as the fuselage.
			UStaticMesh* Hull = Art && !Art->Mesh.IsNull() ? Art->Mesh.LoadSynchronous() : nullptr;
			for (int32 i = 0; Art && !Hull && i < Art->Parts.Num(); ++i) { Hull = Art->Parts[i].Mesh.IsNull() ? nullptr : Art->Parts[i].Mesh.LoadSynchronous(); }
			if (!Hull)
			{
				AddInfo(Name + TEXT(": no ship art in this checkout - the parked mesh is not compared"));
				continue;
			}
			const UStaticMeshComponent* Parked = Course->GetParkedShip();
			if (!TestNotNull(*(Name + TEXT(": a ship is parked in the hangar")), Parked)) { continue; }
			TestTrue(*FString::Printf(TEXT("%s: the parked hull is its own (%s, want %s)"), *Name, *GetNameSafe(Parked->GetStaticMesh()), *Hull->GetName()),
				Parked->GetStaticMesh() == Hull);
		}
	}
	return true;
}

#endif

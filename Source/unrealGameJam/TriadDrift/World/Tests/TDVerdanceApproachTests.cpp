// Story (storyline 14-18, reduced): the way to Verdance - convoys with scanning gear (one scannable by holding
// close), and a transponder that flickers. Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.Approach

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDVerdanceApproach.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDApproachTest
{
	ATDSquadManager* Squad(FTDTestWorld& W)
	{
		APlayerController* PC = W.SpawnLocalPlayerController();
		ATDSquadManager* S = W.Spawn<ATDSquadManager>();
		S->SetSquadPlayerController(PC);
		S->SpawnSquad(FTransform::Identity);
		W.Advance(0.3f);
		return S;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDApproachOnlyOnTheWay, "TriadDrift.World.Approach.ConvoysAndTransponderShowOnTheWayToVerdance", TD_TEST_FLAGS)
bool FTDApproachOnlyOnTheWay::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	GI->Progress.Phase = ETDGamePhase::Waves1to2;
	ATDGalaxyLayout* L = ATDGalaxyLayout::SpawnDefaultGalaxy(W.GetWorld());
	if (!TestNotNull(TEXT("layout"), L)) { return false; }
	L->Build();
	W.Advance(0.6f);
	TArray<ATDArmadaConvoy*> Convoys;
	ATDTransponderBeacon* Beacon = nullptr;
	for (AActor* A : L->GetVerdanceApproach())
	{
		if (ATDArmadaConvoy* C = Cast<ATDArmadaConvoy>(A)) { Convoys.Add(C); }
		if (ATDTransponderBeacon* B = Cast<ATDTransponderBeacon>(A)) { Beacon = B; }
	}
	TestEqual(TEXT("two convoys"), Convoys.Num(), 2);
	if (!TestNotNull(TEXT("transponder"), Beacon) || Convoys.Num() == 0) { return false; }
	TestFalse(TEXT("waves 1-2: no convoys yet"), Convoys[0]->IsActive());
	TestFalse(TEXT("waves 1-2: no transponder yet"), Beacon->IsActive());
	GI->Progress.Phase = ETDGamePhase::Verdance;
	const FVector Before = Convoys[0]->GetHaulerLocation(0);
	W.Advance(3.f);
	TestTrue(TEXT("on the way to Verdance: convoys run"), Convoys[0]->IsActive() && !Convoys[0]->IsHidden());
	TestTrue(TEXT("they move along their lane"), FVector::Dist(Before, Convoys[0]->GetHaulerLocation(0)) > 1000.f);
	TestTrue(TEXT("transponder shown"), Beacon->IsActive());
	// It flickers: on and off within a few seconds.
	bool bSawOn = false, bSawOff = false;
	for (int32 k = 0; k < 120; ++k) { W.Tick(1.f / 30.f); bSawOn |= Beacon->IsLit(); bSawOff |= !Beacon->IsLit(); }
	TestTrue(TEXT("the transponder flickers"), bSawOn && bSawOff);
	GI->Progress.Phase = ETDGamePhase::Waves3to4;
	W.Advance(0.6f);
	TestFalse(TEXT("gone again after Verdance"), Convoys[0]->IsActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDApproachScan, "TriadDrift.World.Approach.HoldingCloseScansTheConvoyOnce", TD_TEST_FLAGS)
bool FTDApproachScan::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	ATDSquadManager* Squad = TDApproachTest::Squad(W);
	ATDArmadaConvoy* C = W.Spawn<ATDArmadaConvoy>(FVector(200000.f, 0.f, 0.f));
	C->SetupLane(FVector(200000.f, 0.f, 0.f), FVector(900000.f, 0.f, 0.f), 3, /*bScannable*/ true);
	C->SetActiveOverride(1);
	W.Advance(0.6f);
	ATDShip* Ship = Squad->GetActiveShip();
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	auto Escort = [&](float Seconds, float Off)
	{
		for (float T = 0.f; T < Seconds; T += 1.f / 30.f)
		{
			Ship->SetActorLocation(C->GetHaulerLocation(0) + FVector(0.f, Off, 2000.f));	// flying alongside the lead hauler
			W.Tick(1.f / 30.f);
		}
	};
	Escort(1.5f, 15000.f);
	Escort(0.5f, 60000.f);	// drifted out: the hold starts over
	TestTrue(*FString::Printf(TEXT("drifting out resets the hold (%.2f)"), C->GetScanProgress01()), C->GetScanProgress01() < 0.05f);
	TestFalse(TEXT("not read yet"), C->IsScanned());
	Escort(C->ScanSeconds + 0.3f, 15000.f);
	TestTrue(TEXT("held close long enough: read"), C->IsScanned());
	const UTDStorySubsystem* Story = UTDStorySubsystem::Get(C);
	FTDTransmission Entry;
	if (Story && UTDStorySubsystem::FindEntry(TEXT("A2_ConvoyScanned"), Entry))	// once T3's line exists
	{
		TestTrue(TEXT("A2_ConvoyScanned played"), Story->HasPlayed(TEXT("A2_ConvoyScanned")));
	}
	return true;
}

#endif

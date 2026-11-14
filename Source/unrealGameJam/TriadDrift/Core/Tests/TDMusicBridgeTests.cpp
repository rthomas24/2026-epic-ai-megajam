// N-082 the music event bridge. Run: .\Tools\td-test.ps1 -Filter TriadDrift.Core.MusicBridge

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Core/TDMusicEventBridge.h"
#include "TriadDrift/Core/Tests/TDMusicBridgeTestListener.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Director/Menu/TDMenuBackdrop.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Input/TDMusicDirectorSubsystem.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "TriadDrift/Travel/TDDescentSubsystem.h"
#include "TriadDrift/Travel/TDLandingComponent.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flight/Tests/TDFlightTestHelpers.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMusicBridgeForwards, "TriadDrift.Core.MusicBridge.ForwardsEveryMomentWithItsWorld", TD_TEST_FLAGS)
bool FTDMusicBridgeForwards::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDMusicEventBridge* B = UTDMusicEventBridge::GetMusicEventBridge(W.GetWorld());
	if (!TestNotNull(TEXT("bridge"), B)) { return false; }
	UTDMusicBridgeTestListener* L = NewObject<UTDMusicBridgeTestListener>();
	L->AddToRoot();
	ON_SCOPE_EXIT { L->RemoveFromRoot(); };
	B->OnMusicEvent.AddDynamic(L, &UTDMusicBridgeTestListener::Handle);
	UWorld* World = W.GetWorld();

	// Phase (the event bus).
	if (UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance())) { GI->SetPhase(ETDGamePhase::DominionAssault); }
	const auto* P = L->Last(UTDMusicEventBridge::PhaseChanged);
	TestTrue(TEXT("PhaseChanged forwarded with the phase name"), P && P->Detail == FName(TEXT("DominionAssault")));
	TestEqual(TEXT("... and the snapshot has it"), B->GetMusicState().Phase, FName(TEXT("DominionAssault")));

	// Planet revealed (the descent).
	if (UTDDescentSubsystem* D = UTDDescentSubsystem::Get(World)) { D->OnRevealed.Broadcast(ETDDestination::Verdance); }
	const auto* R = L->Last(UTDMusicEventBridge::PlanetRevealed);
	TestTrue(TEXT("PlanetRevealed forwarded (Verdance) from this world"), R && R->Detail == FName(TEXT("Verdance")) && R->World.Get() == World);

	// Victory began / stinger / ended (the menu backdrop).
	ATDMenuBackdrop::OnVictoryBegan.Broadcast(nullptr);
	TestTrue(TEXT("VictoryBegan forwarded; the snapshot is in victory"), L->Last(UTDMusicEventBridge::VictoryBegan) && B->GetMusicState().bVictory);
	ATDMenuBackdrop::OnStinger.Broadcast(nullptr);
	TestTrue(TEXT("VictoryStinger forwarded"), L->Last(UTDMusicEventBridge::VictoryStinger) && B->GetMusicState().bVictoryStingerPlayed);
	ATDMenuBackdrop::OnVictoryEnded.Broadcast(nullptr);
	TestTrue(TEXT("VictoryEnded forwarded; out of victory"), L->Last(UTDMusicEventBridge::VictoryEnded) && !B->GetMusicState().bVictory);

	// Transmissions (the story subsystem, bound by the bridge on its tick).
	B->Tick(0.016f);
	UTDStorySubsystem* Story = UTDStorySubsystem::Get(World);
	if (TestNotNull(TEXT("story"), Story))
	{
		FTDTransmissionView View;
		View.bActive = true;
		View.Id = TEXT("TX_Test");
		Story->OnTransmission.Broadcast(View);
		TestTrue(TEXT("TransmissionStarted forwarded with its id; the snapshot is on the air"), L->Last(UTDMusicEventBridge::TransmissionStarted) && L->Last(UTDMusicEventBridge::TransmissionStarted)->Detail == View.Id && B->GetMusicState().bTransmissionActive);
		Story->OnTransmissionEnded.Broadcast(View.Id, true);
		const auto* E = L->Last(UTDMusicEventBridge::TransmissionEnded);
		TestTrue(TEXT("TransmissionEnded forwarded (completed)"), E && E->Detail == View.Id && E->bFlag && !B->GetMusicState().bTransmissionActive);
	}

	// The flown ship's take-off (landing Ascending), and not a wingman's.
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 2000.f));
	APlayerController* PC = W.SpawnLocalPlayerController(Ship);
	if (TestNotNull(TEXT("ship"), Ship) && TestNotNull(TEXT("pc"), PC))
	{
		Ship->SetPlayerControlled(true);
		B->Tick(0.016f);
		Ship->GetLanding()->OnLandingStateChanged.Broadcast(Ship->GetLanding(), ETDLandingState::Ascending);
		const auto* T = L->Last(UTDMusicEventBridge::Takeoff);
		TestTrue(TEXT("Takeoff forwarded for the flown ship"), T != nullptr && T->World.Get() == World);
		const int32 Before = L->Events.Num();
		Ship->GetLanding()->OnLandingStateChanged.Broadcast(Ship->GetLanding(), ETDLandingState::Descending);
		TestEqual(TEXT("other landing states: nothing"), L->Events.Num(), Before);
	}

	// The music director's reflected calls (N-074 / N-063).
	UTDMusicDirectorSubsystem* M = UTDMusicDirectorSubsystem::GetMusicDirector(World);
	TestNotNull(TEXT("GetMusicDirector"), M);
	for (const TCHAR* Fn : { TEXT("RequestExclusiveScore"), TEXT("ReleaseExclusiveScore"), TEXT("IsExclusiveScoreHeld"), TEXT("IsQuietFlight"), TEXT("IsIdleMusicPlaying"), TEXT("GetMusicDirector") })
	{
		TestNotNull(*FString::Printf(TEXT("%s is reflected"), Fn), UTDMusicDirectorSubsystem::StaticClass()->FindFunctionByName(Fn));
	}
	TestNotNull(TEXT("GetMusicState is reflected"), UTDMusicEventBridge::StaticClass()->FindFunctionByName(TEXT("GetMusicState")));
	return true;
}

#endif

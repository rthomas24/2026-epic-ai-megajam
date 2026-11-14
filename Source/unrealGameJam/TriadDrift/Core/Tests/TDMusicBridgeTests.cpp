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
#include "TriadDrift/Travel/TDPlanetTravelRig.h"
#include "TriadDrift/Travel/TDLandingPad.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Flight/Tests/TDFlightTestHelpers.h"
#include "Misc/ScopeExit.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Ground/TDGroundActors.h"
#include "UObject/UObjectGlobals.h"

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

// Nick's audio (N-053 / the Lead, Nov 13): the credits come in when the stinger's last line ends, not on a timer; the
// snapshot tells a descent from a revealed planet and a parked ship from a flying one.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMusicBridgeStingerDone, "TriadDrift.Core.MusicBridge.StingerDoneAfterTheLastLine", TD_TEST_FLAGS)
bool FTDMusicBridgeStingerDone::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDMusicEventBridge* B = UTDMusicEventBridge::GetMusicEventBridge(W.GetWorld());
	UTDStorySubsystem* Story = UTDStorySubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("bridge"), B) || !TestNotNull(TEXT("story"), Story)) { return false; }
	UTDMusicBridgeTestListener* L = NewObject<UTDMusicBridgeTestListener>();
	L->AddToRoot();
	ON_SCOPE_EXIT { L->RemoveFromRoot(); };
	B->OnMusicEvent.AddDynamic(L, &UTDMusicBridgeTestListener::Handle);
	Story->ResetPlayed();
	auto Count = [L](FName Id) { int32 N = 0; for (const auto& E : L->Events) { N += E.Id == Id ? 1 : 0; } return N; };
	auto IndexOf = [L](FName Id, FName Detail) { for (int32 i = 0; i < L->Events.Num(); ++i) { if (L->Events[i].Id == Id && (Detail.IsNone() || L->Events[i].Detail == Detail)) { return i; } } return (int32)INDEX_NONE; };

	// 1. The full stinger: done once, completed, after the last line's end and before VictoryEnded.
	ATDMenuBackdrop::OnVictoryBegan.Broadcast(nullptr);
	ATDMenuBackdrop::OnStinger.Broadcast(nullptr);
	for (const FName& Id : UTDMusicEventBridge::StingerLines()) { UTDStorySubsystem::Fire(W.GetWorld(), Id); } // as PlayStinger does
	B->Tick(0.016f);
	TestFalse(TEXT("lines queued: not done yet"), B->GetMusicState().bVictoryStingerDone);
	float Waited = 0.f;
	for (; Waited < 90.f && !B->GetMusicState().bVictoryStingerDone; Waited += 0.1f) { W.Advance(0.1f, 1.f / 30.f); B->Tick(0.1f); }
	AddInfo(FString::Printf(TEXT("stinger done after %.1f s"), Waited));
	const int32 Done = IndexOf(UTDMusicEventBridge::VictoryStingerDone, NAME_None);
	const int32 LastEnd = IndexOf(UTDMusicEventBridge::TransmissionEnded, TEXT("C9_Kato_Decades"));
	TestTrue(TEXT("VictoryStingerDone fired, completed"), Done != INDEX_NONE && L->Events[Done].bFlag);
	TestTrue(TEXT("... after C9_Kato_Decades ended"), LastEnd != INDEX_NONE && LastEnd < Done);
	TestTrue(TEXT("... after the stinger"), IndexOf(UTDMusicEventBridge::VictoryStinger, NAME_None) < Done);
	ATDMenuBackdrop::OnVictoryEnded.Broadcast(nullptr);
	TestEqual(TEXT("once per stinger"), Count(UTDMusicEventBridge::VictoryStingerDone), 1);
	TestTrue(TEXT("before VictoryEnded"), Done < IndexOf(UTDMusicEventBridge::VictoryEnded, NAME_None));

	// 2. A second stinger in the same run: the lines were heard already and never fire again: done at once, not stuck.
	ATDMenuBackdrop::OnVictoryBegan.Broadcast(nullptr);
	ATDMenuBackdrop::OnStinger.Broadcast(nullptr);
	for (const FName& Id : UTDMusicEventBridge::StingerLines()) { UTDStorySubsystem::Fire(W.GetWorld(), Id); }
	B->Tick(0.016f);
	TestEqual(TEXT("lines already heard: done on the next tick"), Count(UTDMusicEventBridge::VictoryStingerDone), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMusicBridgeStingerCut, "TriadDrift.Core.MusicBridge.StingerCutShortWhenTheScreenCloses", TD_TEST_FLAGS)
bool FTDMusicBridgeStingerCut::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDMusicEventBridge* B = UTDMusicEventBridge::GetMusicEventBridge(W.GetWorld());
	UTDStorySubsystem* Story = UTDStorySubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("bridge"), B) || !TestNotNull(TEXT("story"), Story)) { return false; }
	UTDMusicBridgeTestListener* L = NewObject<UTDMusicBridgeTestListener>();
	L->AddToRoot();
	ON_SCOPE_EXIT { L->RemoveFromRoot(); };
	B->OnMusicEvent.AddDynamic(L, &UTDMusicBridgeTestListener::Handle);
	Story->ResetPlayed();
	// 3. The screen closes mid-stinger: done (cut short) first, then VictoryEnded.
	ATDMenuBackdrop::OnVictoryBegan.Broadcast(nullptr);
	ATDMenuBackdrop::OnStinger.Broadcast(nullptr);
	for (const FName& Id : UTDMusicEventBridge::StingerLines()) { UTDStorySubsystem::Fire(W.GetWorld(), Id); }
	B->Tick(0.016f);
	const int32 Mark = L->Events.Num();
	ATDMenuBackdrop::OnVictoryEnded.Broadcast(nullptr);
	TestTrue(TEXT("cut short: VictoryStingerDone(false) then VictoryEnded"), L->Events.Num() == Mark + 2 && L->Events[Mark].Id == UTDMusicEventBridge::VictoryStingerDone
		&& !L->Events[Mark].bFlag && L->Events[Mark + 1].Id == UTDMusicEventBridge::VictoryEnded);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMusicBridgeLateFlags, "TriadDrift.Core.MusicBridge.SnapshotTellsDescentRevealAndParked", TD_TEST_FLAGS)
bool FTDMusicBridgeLateFlags::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDMusicEventBridge* B = UTDMusicEventBridge::GetMusicEventBridge(W.GetWorld());
	if (!TestNotNull(TEXT("bridge"), B)) { return false; }
	TestFalse(TEXT("start: not revealed"), B->GetMusicState().bPlanetRevealed);
	if (UTDDescentSubsystem* D = UTDDescentSubsystem::Get(W.GetWorld())) { D->OnRevealed.Broadcast(ETDDestination::Verdance); }
	TestTrue(TEXT("after the reveal: revealed"), B->GetMusicState().bPlanetRevealed);
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 2000.f));
	APlayerController* PC = W.SpawnLocalPlayerController(Ship);
	if (TestNotNull(TEXT("ship"), Ship) && TestNotNull(TEXT("pc"), PC))
	{
		Ship->SetPlayerControlled(true);
		B->Tick(0.016f);
		UTDLandingComponent* Land = Ship->GetLanding();
		Land->OnLandingStateChanged.Broadcast(Land, ETDLandingState::Descending);
		TestFalse(TEXT("descending: not parked"), B->GetMusicState().bLanded);
		Land->OnLandingStateChanged.Broadcast(Land, ETDLandingState::Landed);
		TestTrue(TEXT("landed: parked"), B->GetMusicState().bLanded);
		Land->OnLandingStateChanged.Broadcast(Land, ETDLandingState::CanopyClosing);
		TestTrue(TEXT("canopy closing: still parked"), B->GetMusicState().bLanded);
		Land->OnLandingStateChanged.Broadcast(Land, ETDLandingState::Ascending);
		TestFalse(TEXT("take-off: not parked"), B->GetMusicState().bLanded);
		TestTrue(TEXT("... and still revealed (flying on the planet)"), B->GetMusicState().bPlanetRevealed);
	}
	FCoreUObjectDelegates::PreLoadMap.Broadcast(TEXT("/Game/TriadDrift/Maps/L_Galaxy"));
	TestFalse(TEXT("a level change: the next map starts unrevealed"), B->GetMusicState().bPlanetRevealed);
	return true;
}

// N-089: on foot on a planet, exploration <-> combat (a crew encounter running), with the planet as DetailId.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMusicBridgeGround, "TriadDrift.Core.MusicBridge.GroundCombatAndAllClear", TD_TEST_FLAGS)
bool FTDMusicBridgeGround::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance());
	UTDMusicEventBridge* B = UTDMusicEventBridge::GetMusicEventBridge(W.GetWorld());
	if (!TestNotNull(TEXT("bridge"), B) || !TestNotNull(TEXT("gi"), GI)) { return false; }
	UTDMusicBridgeTestListener* L = NewObject<UTDMusicBridgeTestListener>();
	L->AddToRoot();
	ON_SCOPE_EXIT { L->RemoveFromRoot(); };
	B->OnMusicEvent.AddDynamic(L, &UTDMusicBridgeTestListener::Handle);
	auto Count = [L](FName Id) { int32 N = 0; for (const auto& E : L->Events) { N += E.Id == Id ? 1 : 0; } return N; };

	ATDEncounterVolume* E = W.Spawn<ATDEncounterVolume>(FVector(0.f, 0.f, 150.f));
	if (!TestNotNull(TEXT("encounter"), E)) { return false; }
	E->Count = 2;
	E->bTriggerOnOverlap = false;
	W.Advance(0.1f);

	// In space an encounter means nothing to this signal (PhaseChanged covers space).
	GI->NotifyLevelEntered(ETDDestination::Galaxy);
	E->Activate();
	B->Tick(0.016f);
	TestEqual(TEXT("in the galaxy: no GroundCombat"), Count(UTDMusicEventBridge::GroundCombat), 0);

	GI->NotifyLevelEntered(ETDDestination::Cinder);
	B->Tick(0.016f);
	const auto* C = L->Last(UTDMusicEventBridge::GroundCombat);
	TestTrue(TEXT("on Cinder with a fight running: GroundCombat (Cinder) from this world"), C && C->Detail == FName(TEXT("Cinder")) && C->World.Get() == W.GetWorld());
	TestTrue(TEXT("... and the snapshot says so"), B->GetMusicState().bGroundCombat);
	B->Tick(0.016f);
	TestEqual(TEXT("once per start"), Count(UTDMusicEventBridge::GroundCombat), 1);

	for (const TWeakObjectPtr<AActor>& M : E->GetSpawned())
	{
		if (UTDHealthComponent* H = M.IsValid() ? UTDStatics::GetHealth(M.Get()) : nullptr) { H->Kill(nullptr, ETDDamageKind::Melee); }
	}
	TestTrue(TEXT("the encounter cleared"), E->IsCleared());
	B->Tick(0.016f);
	const auto* A = L->Last(UTDMusicEventBridge::GroundAllClear);
	TestTrue(TEXT("all clear: GroundAllClear (Cinder)"), A && A->Detail == FName(TEXT("Cinder")));
	TestFalse(TEXT("... exploration in the snapshot"), B->GetMusicState().bGroundCombat);
	return true;
}

// N-089: the reveal (the final record, Cinematic 08) ends: Cathedral Pulse hands over.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMusicBridgeReveal, "TriadDrift.Core.MusicBridge.RevealEndedWhenCinematic08Finishes", TD_TEST_FLAGS)
bool FTDMusicBridgeReveal::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UTDMusicEventBridge* B = UTDMusicEventBridge::GetMusicEventBridge(W.GetWorld());
	UTDStorySubsystem* Story = UTDStorySubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("bridge"), B) || !TestNotNull(TEXT("story"), Story)) { return false; }
	UTDMusicBridgeTestListener* L = NewObject<UTDMusicBridgeTestListener>();
	L->AddToRoot();
	ON_SCOPE_EXIT { L->RemoveFromRoot(); };
	B->OnMusicEvent.AddDynamic(L, &UTDMusicBridgeTestListener::Handle);
	B->Tick(0.016f); // binds the world's story
	Story->ResetPlayed();
	TestTrue(TEXT("Cinematic08 plays"), UTDStorySubsystem::PlaySequence(W.GetWorld(), TEXT("Cinematic08")));
	TestNull(TEXT("not while it plays"), L->Last(UTDMusicEventBridge::RevealEnded));
	float Waited = 0.f;
	for (; Waited < 300.f && Story->IsSequencePlaying(TEXT("Cinematic08")); Waited += 0.25f) { W.Advance(0.25f, 1.f / 20.f); }
	AddInfo(FString::Printf(TEXT("Cinematic08 ran %.1f s"), Waited));
	const auto* R = L->Last(UTDMusicEventBridge::RevealEnded);
	TestTrue(TEXT("RevealEnded when it finishes (completed), from this world"), R && R->bFlag && R->World.Get() == W.GetWorld());
	// Another sequence finishing is not the reveal.
	const int32 N = L->Events.Num();
	Story->OnSequenceFinished.Broadcast(TEXT("Docking"), false);
	bool bAny = false;
	for (int32 i = N; i < L->Events.Num(); ++i) { bAny |= L->Events[i].Id == UTDMusicEventBridge::RevealEnded; }
	TestFalse(TEXT("another sequence: no RevealEnded"), bAny);
	// Skipped: RevealEnded(false).
	Story->OnSequenceFinished.Broadcast(TEXT("Cinematic08"), true);
	const auto* S = L->Last(UTDMusicEventBridge::RevealEnded);
	TestTrue(TEXT("skipped: RevealEnded(false)"), S && !S->bFlag);
	return true;
}

// #138 (Nick's side's source review; compile-only by the Lead's rule): three focused cases for fca00389.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMusicBridgeOtherInstance, "TriadDrift.Core.MusicBridge.AnotherGameInstancesBackdropIsRejectedFirst", TD_TEST_FLAGS)
bool FTDMusicBridgeOtherInstance::RunTest(const FString& Parameters)
{
	FTDTestWorld Mine(UTDGameInstance::StaticClass());
	FTDTestWorld Other(UTDGameInstance::StaticClass());
	UTDMusicEventBridge* B = UTDMusicEventBridge::GetMusicEventBridge(Mine.GetWorld());
	if (!TestNotNull(TEXT("bridge"), B)) { return false; }
	UTDMusicBridgeTestListener* L = NewObject<UTDMusicBridgeTestListener>();
	L->AddToRoot();
	ON_SCOPE_EXIT { L->RemoveFromRoot(); };
	B->OnMusicEvent.AddDynamic(L, &UTDMusicBridgeTestListener::Handle);
	ATDMenuBackdrop* Theirs = Other.Spawn<ATDMenuBackdrop>();
	ATDMenuBackdrop* Ours = Mine.Spawn<ATDMenuBackdrop>();
	if (!TestNotNull(TEXT("their backdrop"), Theirs) || !TestNotNull(TEXT("our backdrop"), Ours)) { return false; }
	L->Events.Reset();
	ATDMenuBackdrop::OnVictoryBegan.Broadcast(Theirs);
	ATDMenuBackdrop::OnStinger.Broadcast(Theirs);
	const FTDMusicBridgeState After = B->GetMusicState();
	TestFalse(TEXT("another instance's victory: our snapshot is untouched"), After.bVictory || After.bVictoryStingerPlayed || After.bVictoryStingerDone);
	TestNull(TEXT("... and nothing was forwarded"), L->Last(UTDMusicEventBridge::VictoryBegan));
	TestNull(TEXT("... nor the stinger"), L->Last(UTDMusicEventBridge::VictoryStinger));
	ATDMenuBackdrop::OnVictoryEnded.Broadcast(Theirs);
	TestNull(TEXT("... nor its end"), L->Last(UTDMusicEventBridge::VictoryEnded));
	// The control: our own backdrop.
	ATDMenuBackdrop::OnVictoryBegan.Broadcast(Ours);
	TestTrue(TEXT("our own backdrop: forwarded, in victory"), L->Last(UTDMusicEventBridge::VictoryBegan) != nullptr && B->GetMusicState().bVictory);
	ATDMenuBackdrop::OnVictoryEnded.Broadcast(Ours);
	TestFalse(TEXT("... and out again"), B->GetMusicState().bVictory);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMusicBridgeAfterDeinit, "TriadDrift.Core.MusicBridge.NoCallbacksAfterDeinitialize", TD_TEST_FLAGS)
bool FTDMusicBridgeAfterDeinit::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UWorld* World = W.GetWorld();
	UTDMusicEventBridge* B = UTDMusicEventBridge::GetMusicEventBridge(World);
	UTDStorySubsystem* Story = UTDStorySubsystem::Get(World);
	if (!TestNotNull(TEXT("bridge"), B) || !TestNotNull(TEXT("story"), Story)) { return false; }
	UTDMusicBridgeTestListener* L = NewObject<UTDMusicBridgeTestListener>();
	L->AddToRoot();
	ON_SCOPE_EXIT { L->RemoveFromRoot(); };
	B->OnMusicEvent.AddDynamic(L, &UTDMusicBridgeTestListener::Handle);
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 2000.f));
	APlayerController* PC = W.SpawnLocalPlayerController(Ship);
	if (!TestNotNull(TEXT("ship"), Ship) || !TestNotNull(TEXT("pc"), PC)) { return false; }
	Ship->SetPlayerControlled(true);
	B->Tick(0.016f); // binds the story and the flown ship's landing
	B->Deinitialize();
	L->Events.Reset();
	FTDTransmissionView View;
	View.bActive = true;
	View.Id = TEXT("TX_AfterDeinit");
	Story->OnTransmission.Broadcast(View);
	Story->OnTransmissionEnded.Broadcast(View.Id, true);
	Story->OnSequenceFinished.Broadcast(TEXT("Cinematic08"), false);
	UTDLandingComponent* Land = Ship->GetLanding();
	Land->OnLandingStateChanged.Broadcast(Land, ETDLandingState::Ascending);
	if (UTDGameInstance* GI = Cast<UTDGameInstance>(W.GetGameInstance())) { GI->SetPhase(ETDGamePhase::Wave5); }
	ATDMenuBackdrop::OnVictoryBegan.Broadcast(nullptr);
	if (UTDDescentSubsystem* D = UTDDescentSubsystem::Get(World)) { D->OnRevealed.Broadcast(ETDDestination::Cinder); }
	TestEqual(TEXT("after Deinitialize: no story, landing, phase, victory or reveal callback arrives"), L->Events.Num(), 0);
	TestFalse(TEXT("... and the snapshot did not move"), B->GetMusicState().bTransmissionActive || B->GetMusicState().bVictory || B->GetMusicState().bPlanetRevealed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDMusicBridgeLateBind, "TriadDrift.Core.MusicBridge.LateBindSeedsTheLineOnAirAndTheParkedShip", TD_TEST_FLAGS)
bool FTDMusicBridgeLateBind::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UWorld* World = W.GetWorld();
	UTDMusicEventBridge* B = UTDMusicEventBridge::GetMusicEventBridge(World);
	UTDStorySubsystem* Story = UTDStorySubsystem::Get(World);
	if (!TestNotNull(TEXT("bridge"), B) || !TestNotNull(TEXT("story"), Story)) { return false; }
	UTDMusicBridgeTestListener* L = NewObject<UTDMusicBridgeTestListener>();
	L->AddToRoot();
	ON_SCOPE_EXIT { L->RemoveFromRoot(); };
	B->OnMusicEvent.AddDynamic(L, &UTDMusicBridgeTestListener::Handle);
	// Before the bridge's first tick (the test world does not tick game-instance tickables): a line goes on air ...
	UTDStorySubsystem::PlaySequence(World, TEXT("HangarArrival"));
	for (int32 i = 0; i < 120 && !Story->GetCurrentTransmission().bActive; ++i) { W.Tick(1.f / 30.f); }
	const FTDTransmissionView OnAir = Story->GetCurrentTransmission();
	// ... and the flown ship is already parked on a pad.
	ATDPlanetTravelRig* Rig = ATDPlanetTravelRig::SpawnForPlanet(World, TEXT("Verdance"), FTransform(FVector(0.f, 0.f, 200.f)), { TEXT("Obj_Test") });
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, 3000.f));
	APlayerController* PC = W.SpawnLocalPlayerController(Ship);
	if (!TestNotNull(TEXT("rig"), Rig) || !TestNotNull(TEXT("ship"), Ship) || !TestNotNull(TEXT("pc"), PC)) { return false; }
	Ship->SetPlayerControlled(true);
	const bool bParked = Rig->GetPlayerPad() && Ship->GetLanding()->SnapLanded(Rig->GetPlayerPad());
	L->Events.Reset();
	B->Tick(0.016f); // the late bind
	const FTDMusicBridgeState S = B->GetMusicState();
	if (OnAir.bActive)
	{
		TestTrue(TEXT("a line already on air at bind: the snapshot has it"), S.bTransmissionActive && S.Transmission == OnAir.Id);
	}
	else { AddInfo(TEXT("no line went on air in this world: the transmission half is not exercised")); }
	if (bParked) { TestTrue(TEXT("a ship already landed at bind: the snapshot is parked"), S.bLanded); }
	else { AddInfo(TEXT("the ship could not be parked in this world: the parked half is not exercised")); }
	TestNull(TEXT("no catch-up TransmissionStarted"), L->Last(UTDMusicEventBridge::TransmissionStarted));
	TestNull(TEXT("no catch-up Takeoff"), L->Last(UTDMusicEventBridge::Takeoff));
	return true;
}

#endif

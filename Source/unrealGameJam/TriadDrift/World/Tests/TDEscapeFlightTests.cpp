// ENG-31 escape flight: back from the interior (Phase Escape) the Dominion burns and sheds debris; fly clear in
// time for the final blast and victory, or the clock runs out and the take restarts at the bay mouth.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.Escape

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "TriadDrift/World/TDEscapeDebris.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/Flow/TDJourney.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Story/TDStorySubsystem.h"
#include "EngineUtils.h"

namespace TDEscapeTest
{
	struct FSetup
	{
		ATDDominion* Dominion = nullptr;
		ATDSquadManager* Squad = nullptr;
		ATDShip* Ship = nullptr;
		UTDWaveTestGameInstance* GI = nullptr;
	};

	/** Phase Escape (back from the interior), the Dominion in its escape flight, the squad at the bay mouth. */
	FSetup Start(FTDTestWorld& W, float EscapeSeconds = 60.f)
	{
		FSetup S;
		S.GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
		S.GI->Progress.bDominionRevealed = true;
		S.GI->Progress.bDominionDefensesDown = true;
		S.GI->Progress.Phase = ETDGamePhase::Escape;
		S.Dominion = W.Spawn<ATDDominion>(FVector(400000.f, 0.f, 0.f));
		S.Dominion->EscapeSeconds = EscapeSeconds;
		W.Advance(S.Dominion->ProgressPollSeconds + 0.2f);
		APlayerController* PC = W.SpawnLocalPlayerController();
		S.Squad = W.Spawn<ATDSquadManager>();
		S.Squad->SetSquadPlayerController(PC);
		FTransform At;
		FTDJourney::FindStageStart(W.GetWorld(), TDReturnPoints::DominionHangarExit, At);
		S.Squad->SpawnSquad(At);
		W.Advance(0.2f);
		S.Ship = S.Squad->GetActiveShip();
		return S;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEscapeStarts, "TriadDrift.World.Escape.StartsAtTheBayMouthFromProgress", TD_TEST_FLAGS)
bool FTDEscapeStarts::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	TDEscapeTest::FSetup S = TDEscapeTest::Start(W);
	ATDDominion* D = S.Dominion;
	TestEqual(TEXT("escape flight"), (int32)D->GetState(), (int32)ETDDominionState::EscapeFlight);
	TestFalse(TEXT("the hull is there, burning"), D->IsHidden());
	TestTrue(TEXT("the reactor is gone (destroyed inside)"), !D->GetReactor() || D->GetReactor()->IsHidden());
	// The hangar exit resolves from the live Dominion: out of the bow, outside the hull, facing out.
	FTransform At;
	TestTrue(TEXT("Dominion_Hangar_Exit resolves"), FTDJourney::FindStageStart(W.GetWorld(), TDReturnPoints::DominionHangarExit, At));
	TestTrue(TEXT("it is GetEscapeStart"), At.GetLocation().Equals(D->GetEscapeStart().GetLocation(), 1.f));
	const FVector Local = D->GetActorTransform().InverseTransformPosition(At.GetLocation());
	TestTrue(TEXT("outside the hull"), !D->GetLocalHullBounds().IsInside(Local));
	TestTrue(TEXT("facing out of the bow"), FVector::DotProduct(At.GetRotation().GetForwardVector(), D->GetActorForwardVector()) > 0.99f);
	TestTrue(TEXT("the squad starts there"), S.Ship && FVector::Dist(S.Ship->GetActorLocation(), At.GetLocation()) < 5000.f);
	TestTrue(*FString::Printf(TEXT("60 s on the clock (%.1f)"), D->GetEscapeTimeLeft()), D->GetEscapeTimeLeft() > 58.f);
	TestTrue(TEXT("the clear line is well beyond the hull"), D->GetEscapeExitRadius() > D->GetLocalHullBounds().GetExtent().Size() * 2.f);
	const UTDStorySubsystem* Story = UTDStorySubsystem::Get(D);
	TestTrue(TEXT("the flight's own line played (G9_Wren_PullAway)"), Story && Story->HasPlayed(TEXT("G9_Wren_PullAway")));
	TestFalse(TEXT("the flight does not repeat the on-foot run's \"Ninety seconds\" (A5_Escape)"), Story && Story->HasPlayed(TEXT("A5_Escape")));
	// A real run: from the mouth, the clear line takes well over half the clock at the squad's boost-limited ~80 m/s.
	const float Run = (D->GetEscapeExitRadius() - FVector::Dist(D->GetEscapeStart().GetLocation(), D->GetActorLocation())) / 8000.f;
	TestTrue(*FString::Printf(TEXT("clearing takes most of the clock (%.0f s of %.0f s at 80 m/s)"), Run, D->EscapeSeconds), Run > D->EscapeSeconds * 0.5f && Run < D->EscapeSeconds * 0.85f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEscapeClear, "TriadDrift.World.Escape.FlyingClearBlowsTheDominionThenVictory", TD_TEST_FLAGS)
bool FTDEscapeClear::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	TDEscapeTest::FSetup S = TDEscapeTest::Start(W);
	if (!TestNotNull(TEXT("ship"), S.Ship)) { return false; }
	W.Advance(3.f);
	TestFalse(TEXT("not clear at the mouth"), S.Dominion->IsEscapeClear());
	// Out past the clear line.
	S.Ship->SetActorLocation(S.Dominion->GetActorLocation() + S.Dominion->GetActorForwardVector() * (S.Dominion->GetEscapeExitRadius() + 5000.f));
	W.Advance(0.2f);
	TestTrue(TEXT("clear"), S.Dominion->IsEscapeClear());
	TestTrue(TEXT("final blast"), S.Dominion->HasBreakupFinalBlast());
	TestTrue(TEXT("the hull is gone"), S.Dominion->IsHidden());
	const UTDHealthComponent* H = UTDStatics::GetHealth(S.Ship);
	TestTrue(TEXT("the squad is safe now"), H && H->IsInvulnerable());
	W.Advance(S.Dominion->EscapeVictoryDelay + 0.5f);
	TestTrue(TEXT("victory"), S.GI->Progress.Phase == ETDGamePhase::Victory);
	TestEqual(TEXT("victory once"), S.GI->Phases.FilterByPredicate([](ETDGamePhase P) { return P == ETDGamePhase::Victory; }).Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEscapeFail, "TriadDrift.World.Escape.TooSlowCutsAndTheRetakeRestartsAtTheMouth", TD_TEST_FLAGS)
bool FTDEscapeFail::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	TDEscapeTest::FSetup S = TDEscapeTest::Start(W, /*EscapeSeconds*/ 6.f);
	if (!TestNotNull(TEXT("ship"), S.Ship)) { return false; }
	S.Ship->SetFlightParked(true);	// the pilot never leaves
	S.Dominion->EscapeDebrisInterval = 0.f;
	W.Advance(7.f);
	TestTrue(TEXT("the clock ran out: CUT"), S.Dominion->HasEscapeFailed());
	TestFalse(TEXT("not clear"), S.Dominion->IsEscapeClear());
	TestTrue(TEXT("no victory"), S.GI->Progress.Phase == ETDGamePhase::Escape);
	// The next take of scene "Escape": the clock restarts and the hull is back.
	if (UTDEventBus* Bus = UTDEventBus::Get(S.Dominion)) { Bus->BroadcastTakeStarted(S.Dominion->EscapeSceneId, 2); }
	W.Advance(0.1f);
	TestFalse(TEXT("retake: failure cleared"), S.Dominion->HasEscapeFailed());
	TestFalse(TEXT("retake: the hull is back"), S.Dominion->IsHidden());
	TestTrue(*FString::Printf(TEXT("retake: the clock restarted (%.1f)"), S.Dominion->GetEscapeTimeLeft()), S.Dominion->GetEscapeTimeLeft() > 5.5f);
	TestEqual(TEXT("still the escape flight"), (int32)S.Dominion->GetState(), (int32)ETDDominionState::EscapeFlight);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDEscapeDebrisHurts, "TriadDrift.World.Escape.DebrisFallsAcrossTheWayOutAndHurts", TD_TEST_FLAGS)
bool FTDEscapeDebrisHurts::RunTest(const FString&)
{
	FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
	TDEscapeTest::FSetup S = TDEscapeTest::Start(W);
	if (!TestNotNull(TEXT("ship"), S.Ship)) { return false; }
	S.Ship->SetFlightParked(true);
	W.Advance(5.f);
	TestTrue(*FString::Printf(TEXT("debris keeps coming (%d)"), S.Dominion->GetEscapeDebrisSpawned()), S.Dominion->GetEscapeDebrisSpawned() >= 4);
	// A chunk that reaches a squad ship hurts it and bursts.
	UTDHealthComponent* H = UTDStatics::GetHealth(S.Ship);
	if (!TestNotNull(TEXT("health"), H)) { return false; }
	const float Before = H->GetHull() + H->GetShield();
	ATDEscapeDebris* Chunk = W.Spawn<ATDEscapeDebris>(S.Ship->GetActorLocation() + FVector(0.f, 0.f, 200.f));
	Chunk->Launch(1500.f, FVector::ZeroVector, FRotator::ZeroRotator);
	W.Advance(0.1f);
	TestTrue(*FString::Printf(TEXT("the ship took damage (%.0f -> %.0f)"), Before, H->GetHull() + H->GetShield()), H->GetHull() + H->GetShield() < Before);
	TestFalse(TEXT("the chunk burst"), IsValid(Chunk) && !Chunk->IsActorBeingDestroyed());
	return true;
}

#endif

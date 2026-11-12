// Automation tests for the Dominion assault and space finale (ENG-18).
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.World

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDShieldGenerator.h"
#include "TriadDrift/World/TDDominionReactor.h"
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Core/TDHealthComponent.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Core/TDPresentation.h"
#include "TriadDrift/Enemies/TDEnemyTurret.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "TriadDrift/Director/TDDirectorSubsystem.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"

namespace
{
	struct FTDDominionTestRig
	{
		FTDTestWorld W{ UTDWaveTestGameInstance::StaticClass() };
		UTDWaveTestGameInstance* GI = nullptr;
		ATDDominion* D = nullptr;
		UTDWaveTestListener* L = nullptr;
		AActor* Player = nullptr;	// the killer: the Director only stages player kills

		explicit FTDDominionTestRig(bool bRevealed, bool bDefensesDown = false)
		{
			GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
			GI->Progress.bDominionRevealed = bRevealed;
			if (bRevealed) { GI->Progress.Phase = ETDGamePhase::DominionAssault; }
			GI->Progress.bDominionDefensesDown = bDefensesDown;
			L = NewObject<UTDWaveTestListener>();
			L->AddToRoot();
			L->Bind(W.GetWorld());
			Player = W.Spawn<AActor>(FVector(150000.f, 0.f, 0.f));
			USceneComponent* Root = NewObject<USceneComponent>(Player, TEXT("Root"));
			Player->SetRootComponent(Root);
			Root->RegisterComponent();
			UTDHealthComponent* PH = NewObject<UTDHealthComponent>(Player, TEXT("Health"));
			PH->DefaultTeam = ETDTeam::Player;
			PH->RegisterComponent();
			D = W.Spawn<ATDDominion>(FVector(200000.f, 0.f, 0.f));
			D->MaxLaunchedAlive = 0;	// unlimited, deterministic counting
		}
		~FTDDominionTestRig() { L->RemoveFromRoot(); }

		void ArriveAndRaiseShields()
		{
			W.Advance(D->WarpInSeconds + 0.1f);
		}

		float Hit(AActor* Target, float Amount)
		{
			return UTDStatics::ApplyTDDamage(Target, Amount, ETDDamageKind::Explosive, Player, Player);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldDominionArrival, "TriadDrift.World.Dominion.WarpsInWhenRevealed", TD_TEST_FLAGS)
bool FTDWorldDominionArrival::RunTest(const FString& Parameters)
{
	FTDDominionTestRig R(/*bRevealed*/ false);
	TestTrue(TEXT("hidden before the reveal"), R.D->GetState() == ETDDominionState::Hidden && R.D->IsHidden());
	R.W.Advance(2.f);
	TestTrue(TEXT("stays hidden"), R.D->GetState() == ETDDominionState::Hidden);
	TestEqual(TEXT("no generators yet"), R.D->GetGenerators().Num(), 0);

	// Wave 5 cleared -> flow reveals the Dominion; it notices within a poll.
	R.GI->Progress.bDominionRevealed = true; R.GI->Progress.Phase = ETDGamePhase::DominionAssault;
	R.W.Advance(R.D->ProgressPollSeconds + 0.05f);
	TestTrue(TEXT("warping in"), R.D->GetState() == ETDDominionState::WarpingIn);
	TestFalse(TEXT("hull visible"), R.D->IsHidden());

	R.W.Advance(R.D->WarpInSeconds);
	TestTrue(TEXT("shields up after the warp-in"), R.D->GetState() == ETDDominionState::ShieldsUp);
	TestEqual(TEXT("4 generators"), R.D->GetGeneratorsRemaining(), 4);
	const int32 Turrets = R.D->GetTurrets().Num();
	TestTrue(TEXT("8-12 turrets"), Turrets >= 8 && Turrets <= 12);
	TestEqual(TEXT("generator 800 HP"), R.D->GetGenerators()[0]->GetHealth()->MaxHull, 800.f);
	for (ATDShieldGenerator* G : R.D->GetGenerators())
	{
		TestTrue(TEXT("generators sit outside the main shield"), FVector::Dist(G->GetActorLocation(), R.D->GetActorLocation()) > R.D->ShieldRadius);
	}
	for (ATDEnemyTurret* T : R.D->GetTurrets())
	{
		TestTrue(TEXT("turret shots are telegraphed charge shots"), T->Guns.TelegraphSeconds > 0.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldDominionShield, "TriadDrift.World.Dominion.ShieldBlocksUntilGeneratorsDown", TD_TEST_FLAGS)
bool FTDWorldDominionShield::RunTest(const FString& Parameters)
{
	FTDDominionTestRig R(true);
	R.ArriveAndRaiseShields();
	ATDDominionReactor* Reactor = R.D->GetReactor();
	if (!TestNotNull(TEXT("reactor"), Reactor)) { return false; }

	TestEqual(TEXT("main shield blocks the reactor"), R.Hit(Reactor, 500.f), 0.f);
	TestEqual(TEXT("reactor untouched"), Reactor->GetTotalHealth01(), 1.f);
	TestFalse(TEXT("reactor untargetable while shielded"), Reactor->GetHealth()->bTargetable);

	TArray<ATDShieldGenerator*> Gens = R.D->GetGenerators();
	R.Hit(Gens[0], 400.f);
	TestTrue(TEXT("generators take damage while the shield is up"), Gens[0]->GetHealth()->GetHull() < 800.f);
	R.Hit(Gens[0], 400.f);
	TestFalse(TEXT("generator dead"), Gens[0]->IsAlive());
	TestEqual(TEXT("3 left"), R.D->GetGeneratorsRemaining(), 3);
	TestTrue(TEXT("GeneratorKill highlight"), R.L->Highlights.Contains(ETDHighlightKind::GeneratorKill));
	TestEqual(TEXT("still blocked with 3 generators up"), R.Hit(Reactor, 100.f), 0.f);

	for (int32 i = 1; i < 4; ++i) { R.Hit(Gens[i], 800.f); }
	TestEqual(TEXT("4 GeneratorKill highlights"), R.L->Highlights.FilterByPredicate([](ETDHighlightKind K) { return K == ETDHighlightKind::GeneratorKill; }).Num(), 4);
	TestTrue(TEXT("reactor exposed"), R.D->GetState() == ETDDominionState::ReactorExposed && Reactor->IsExposed());
	TestTrue(TEXT("flow told defenses are down"), R.GI->Progress.bDominionRevealed && R.GI->Progress.bDominionDefensesDown);
	TestTrue(TEXT("reactor targetable"), Reactor->GetHealth()->bTargetable);
	TestEqual(TEXT("damage lands now"), R.Hit(Reactor, 100.f), 100.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldReactorPhases, "TriadDrift.World.Dominion.ReactorThreePhasesThenVictory", TD_TEST_FLAGS)
bool FTDWorldReactorPhases::RunTest(const FString& Parameters)
{
	FTDDominionTestRig R(true);
	R.ArriveAndRaiseShields();
	R.D->DebugDropShields();
	ATDDominionReactor* Reactor = R.D->GetReactor();
	TestTrue(TEXT("exposed"), Reactor->IsExposed());
	TestEqual(TEXT("phase 1"), Reactor->GetPhase(), 1);
	TestEqual(TEXT("stage 1"), R.D->GetStage(), 1);
	const float CooldownP1 = R.D->GetTurrets()[0]->BurstCooldownSeconds;

	// A huge hit stops at the phase boundary.
	R.Hit(Reactor, 99999.f);
	TestEqual(TEXT("phase 2 after crossing the first third"), Reactor->GetPhase(), 2);
	TestEqual(TEXT("one phase per hit"), Reactor->GetTotalHealth01(), 2.f / 3.f, 0.01f);
	TestEqual(TEXT("phase bar refilled"), Reactor->GetPhaseHealth01(), 1.f, 0.01f);
	TestTrue(TEXT("turret fire escalates"), R.D->GetTurrets()[0]->BurstCooldownSeconds < CooldownP1);
	TestEqual(TEXT("invulnerable beat after the phase change"), R.Hit(Reactor, 100.f), 0.f);

	R.W.Advance(Reactor->PhaseChangeInvulnerableSeconds + 0.1f);
	R.Hit(Reactor, 99999.f);
	TestEqual(TEXT("phase 3"), Reactor->GetPhase(), 3);
	TestEqual(TEXT("stage 3"), R.D->GetStage(), 3);
	R.W.Advance(Reactor->PhaseChangeInvulnerableSeconds + 0.1f);
	R.Hit(Reactor, 99999.f);
	TestFalse(TEXT("reactor destroyed"), Reactor->IsAlive());
	TestTrue(TEXT("ReactorKill highlight"), R.L->Highlights.Contains(ETDHighlightKind::ReactorKill));
	TestTrue(TEXT("breakup started"), R.D->GetState() == ETDDominionState::Breakup);
	TestFalse(TEXT("no Victory before the breakup plays"), R.GI->Phases.Contains(ETDGamePhase::Victory));

	R.W.Advance(R.D->BreakupSeconds - 0.2f);
	TestFalse(TEXT("still breaking up"), R.GI->Phases.Contains(ETDGamePhase::Victory));
	R.W.Advance(2.5f);	// + the Director's ReactorKill slow-mo beat (game time runs slow for ~1 s)
	TestTrue(TEXT("SetPhase(Victory)"), R.GI->Progress.Phase == ETDGamePhase::Victory);
	R.W.Advance(2.f);
	TestEqual(TEXT("Victory set once"), R.GI->Phases.FilterByPredicate([](ETDGamePhase P) { return P == ETDGamePhase::Victory; }).Num(), 1);
	TestEqual(TEXT("no level travel (Victory is a phase on L_Galaxy)"), R.GI->Travels.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldDominionLaunches, "TriadDrift.World.Dominion.LaunchesEscalate", TD_TEST_FLAGS)
bool FTDWorldDominionLaunches::RunTest(const FString& Parameters)
{
	FTDDominionTestRig R(true);
	R.ArriveAndRaiseShields();
	const float I0 = R.D->Stages[0].LaunchInterval;
	R.W.Advance(I0 * 0.5f + 0.1f);
	TestEqual(TEXT("first launch half an interval in"), R.D->GetLaunchCount(), 1);
	TestEqual(TEXT("first group = 4 fighters"), R.D->GetLaunchedAlive(), 4);
	R.W.Advance(I0);
	TestEqual(TEXT("second launch one interval later"), R.D->GetLaunchCount(), 2);

	R.D->DebugDropShields();
	const int32 AtExpose = R.D->GetLaunchCount();
	R.W.Advance(R.D->Stages[1].LaunchInterval + 0.1f);
	TestTrue(TEXT("launches continue in the reactor phase"), R.D->GetLaunchCount() > AtExpose);

	// The cap holds.
	FTDDominionTestRig Capped(true);
	Capped.D->MaxLaunchedAlive = 5;
	Capped.ArriveAndRaiseShields();
	Capped.W.Advance(Capped.D->Stages[0].LaunchInterval * 4.f);
	TestTrue(TEXT("never more than MaxLaunchedAlive"), Capped.D->GetLaunchedAlive() <= 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldDominionResume, "TriadDrift.World.Dominion.ResumeWithDefensesDown", TD_TEST_FLAGS)
bool FTDWorldDominionResume::RunTest(const FString& Parameters)
{
	FTDDominionTestRig R(/*bRevealed*/ true, /*bDefensesDown*/ true);
	TestTrue(TEXT("straight to the reactor"), R.D->GetState() == ETDDominionState::ReactorExposed);
	TestEqual(TEXT("generators already down"), R.D->GetGeneratorsRemaining(), 0);
	TestTrue(TEXT("reactor exposed"), R.D->GetReactor() && R.D->GetReactor()->IsExposed());
	TestEqual(TEXT("no duplicate SetDominionState on resume"), R.GI->DominionStateCalls, 0);
	TestEqual(TEXT("no generator highlights on resume"), R.L->Highlights.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldDirectorTags, "TriadDrift.World.Dominion.DirectorTagsProduceHighlights", TD_TEST_FLAGS)
bool FTDWorldDirectorTags::RunTest(const FString& Parameters)
{
	FTDDominionTestRig R(true);
	R.ArriveAndRaiseShields();
	ATDShieldGenerator* G = R.D->GetGenerators()[0];
	TestTrue(TEXT("generator tagged"), G->ActorHasTag(TDDirectorTags::Generator));
	TestTrue(TEXT("reactor tagged"), R.D->GetReactor()->ActorHasTag(TDDirectorTags::Reactor));

	R.Hit(G, 800.f);
	TestTrue(TEXT("player generator kill -> GeneratorKill"), R.L->Highlights.Contains(ETDHighlightKind::GeneratorKill));

	ATDEnemyShip* C = R.W.Spawn<ATDEnemyShip>(FVector(0.f, 80000.f, 0.f));
	C->InitFromSpec(TDEnemyIds::HiveCarrier);
	TestTrue(TEXT("carrier tagged"), C->ActorHasTag(TDDirectorTags::Carrier));
	ATDEnemyShip* F = R.W.Spawn<ATDEnemyShip>(FVector(0.f, 90000.f, 0.f));
	F->InitFromSpec(TDEnemyIds::ShardFighter);
	TestFalse(TEXT("fighters aren't"), F->ActorHasTag(TDDirectorTags::Carrier));
	R.W.Advance(1.5f);
	R.Hit(C, 5000.f);
	TestTrue(TEXT("player carrier kill -> CarrierKill"), R.L->Highlights.Contains(ETDHighlightKind::CarrierKill));

	R.D->DebugDropShields();
	ATDDominionReactor* Reactor = R.D->GetReactor();
	for (int32 i = 0; i < 3; ++i)
	{
		R.W.Advance(Reactor->PhaseChangeInvulnerableSeconds + 0.1f);
		R.Hit(Reactor, 99999.f);
	}
	TestFalse(TEXT("reactor dead"), Reactor->IsAlive());
	TestTrue(TEXT("player reactor kill -> ReactorKill"), R.L->Highlights.Contains(ETDHighlightKind::ReactorKill));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldDominionVisuals, "TriadDrift.World.Dominion.VisualsReachReactorAndTurrets", TD_TEST_FLAGS)
bool FTDWorldDominionVisuals::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
	Cast<UTDWaveTestGameInstance>(W.GetGameInstance())->Progress.bDominionRevealed = true; Cast<UTDWaveTestGameInstance>(W.GetGameInstance())->Progress.Phase = ETDGamePhase::DominionAssault;
	UTDDominionVisuals* DV = NewObject<UTDDominionVisuals>();
	DV->ReactorMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cone.Cone")));
	DV->GeneratorMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cone.Cone")));
	UTDEnemyVisuals* TV = NewObject<UTDEnemyVisuals>();

	ATDDominion* D = W.GetWorld()->SpawnActorDeferred<ATDDominion>(ATDDominion::StaticClass(), FTransform::Identity);
	D->Visuals = DV;
	D->TurretVisuals = TV;
	D->MaxLaunchedAlive = 0;
	D->FinishSpawning(FTransform::Identity);
	W.Advance(D->WarpInSeconds + 0.2f);

	const UStaticMeshComponent* RM = D->GetReactor() ? D->GetReactor()->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	TestTrue(TEXT("reactor uses the asset's ReactorMesh"), RM && RM->GetStaticMesh() && RM->GetStaticMesh()->GetName() == TEXT("Cone"));
	const UStaticMeshComponent* GM = D->GetGenerators().Num() ? D->GetGenerators()[0]->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	TestTrue(TEXT("generator uses the asset's GeneratorMesh"), GM && GM->GetStaticMesh() && GM->GetStaticMesh()->GetName() == TEXT("Cone"));
	TestTrue(TEXT("turrets get the turret visuals"), D->GetTurrets().Num() > 0 && D->GetTurrets()[0]->Visuals == TV);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldVictorySafe, "TriadDrift.World.Dominion.VictoryIsSafeAndSetOnce", TD_TEST_FLAGS)
bool FTDWorldVictorySafe::RunTest(const FString& Parameters)
{
	FTDDominionTestRig R(true);
	R.ArriveAndRaiseShields();
	R.D->MaxLaunchedAlive = 10;
	R.W.Advance(R.D->Stages[0].LaunchInterval);	// some fighters are out
	const int32 FightersBefore = R.D->GetLaunchedAlive();
	TestTrue(TEXT("fighters in the air before the kill"), FightersBefore > 0);

	R.D->DebugDropShields();
	ATDDominionReactor* Reactor = R.D->GetReactor();
	for (int32 i = 0; i < 3; ++i)
	{
		R.W.Advance(Reactor->PhaseChangeInvulnerableSeconds + 0.1f);
		R.Hit(Reactor, 99999.f);
	}
	TestFalse(TEXT("reactor dead"), Reactor->IsAlive());
	R.W.Tick();
	TestEqual(TEXT("every enemy fighter dies with the reactor"), R.D->GetLaunchedAlive(), 0);
	TestTrue(TEXT("the squad can't be hurt during the breakup"), UTDStatics::GetHealth(R.Player)->IsInvulnerable());
	TestEqual(TEXT("damage on the squad is ignored"), UTDStatics::ApplyTDDamage(R.Player, 50.f, ETDDamageKind::Energy, nullptr, nullptr), 0.f);

	R.W.Advance(R.D->BreakupSeconds + 2.5f);
	TestTrue(TEXT("phase set to Victory"), R.GI->Progress.Phase == ETDGamePhase::Victory && R.D->IsVictoryAccepted());
	TestTrue(TEXT("squad still protected at the victory screen"), UTDStatics::GetHealth(R.Player)->IsInvulnerable());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

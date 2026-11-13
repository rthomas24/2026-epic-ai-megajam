// Story clue: from wave 3 on, rarely, an enemy pre-empts a ship switch or a dodge.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.Enemies.Foresight

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Enemies/TDArmadaForesight.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyAIController.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Tests/TDTestWorld.h"

namespace TDForesightTest
{
	struct FSetup
	{
		ATDSquadManager* Squad = nullptr;
		ATDEnemyShip* Enemy = nullptr;
		UTDArmadaForesight* Foresight = nullptr;
		UTDEventBus* Bus = nullptr;
		int32 Heard = 0;
	};

	void Start(FTDTestWorld& W, FSetup& S, bool bFacing)
	{
		APlayerController* PC = W.SpawnLocalPlayerController();
		S.Squad = W.Spawn<ATDSquadManager>();
		S.Squad->SetSquadPlayerController(PC);
		S.Squad->SpawnSquad(FTransform::Identity);
		W.Advance(0.5f);
		const ATDShip* Flown = S.Squad->GetActiveShip();
		const FVector At = Flown->GetActorLocation() + FVector(150000.f, 20000.f, 0.f);
		S.Enemy = W.Spawn<ATDEnemyShip>(At);
		S.Enemy->InitFromSpec(TDEnemyIds::ShardFighter);
		// Facing the squad (a dodge tell needs an enemy that is already looking that way), or facing away.
		S.Enemy->SetActorRotation(((Flown->GetActorLocation() - At) * (bFacing ? 1.f : -1.f)).Rotation());
		S.Foresight = UTDArmadaForesight::Get(W.GetWorld());
		S.Bus = UTDEventBus::Get(W.GetWorld());
		W.Advance(1.2f);	// the foresight binds to the squad's dodges
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDForesightSwitch, "TriadDrift.Enemies.Foresight.AnEnemyIsAlreadyOnTheNewShip", TD_TEST_FLAGS)
bool FTDForesightSwitch::RunTest(const FString&)
{
	FTDTestWorld W;
	TDForesightTest::FSetup S;
	TDForesightTest::Start(W, S, /*bFacing*/ false);
	if (!TestNotNull(TEXT("foresight"), S.Foresight) || !TestNotNull(TEXT("bus"), S.Bus) || !TestNotNull(TEXT("enemy"), S.Enemy)) { return false; }
	S.Foresight->Chance = 1.f;
	S.Foresight->CooldownSeconds = 0.f;

	// Waves 1-2: never.
	S.Bus->BroadcastWaveStarted(2, 0.3f);
	S.Bus->BroadcastShipSwitched(ETDShipId::Lancer, ETDShipId::Rift);
	TestEqual(TEXT("no tell before wave 3"), S.Foresight->GetTellCount(), 0);

	// Wave 3: the switch is pre-empted, the enemy's nose already on the new ship.
	S.Bus->BroadcastWaveStarted(3, 0.5f);
	ATDShip* NewShip = S.Squad->GetShip(ETDShipId::Bastion);
	S.Bus->BroadcastShipSwitched(ETDShipId::Bastion, ETDShipId::Lancer);
	TestEqual(TEXT("one tell"), S.Foresight->GetTellCount(), 1);
	const FVector ToNew = (NewShip->GetActorLocation() - S.Enemy->GetActorLocation()).GetSafeNormal();
	TestTrue(TEXT("nose already on the new ship"), FVector::DotProduct(S.Enemy->GetActorForwardVector(), ToNew) > 0.99f);
	TestTrue(TEXT("guns on"), S.Enemy->AreGunsFiring());

	// Rare: the cooldown holds the next one back.
	S.Foresight->CooldownSeconds = 40.f;
	S.Bus->BroadcastShipSwitched(ETDShipId::Rift, ETDShipId::Bastion);
	TestEqual(TEXT("cooldown: no second tell"), S.Foresight->GetTellCount(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDForesightDodge, "TriadDrift.Enemies.Foresight.AnEnemyFiresWhereTheDodgeGoes", TD_TEST_FLAGS)
bool FTDForesightDodge::RunTest(const FString&)
{
	FTDTestWorld W;
	TDForesightTest::FSetup S;
	TDForesightTest::Start(W, S, /*bFacing*/ true);
	if (!TestNotNull(TEXT("foresight"), S.Foresight) || !TestNotNull(TEXT("enemy"), S.Enemy)) { return false; }
	S.Foresight->Chance = 1.f;
	S.Foresight->CooldownSeconds = 0.f;
	S.Foresight->SetWave(4);
	ATDShip* Ship = S.Squad->GetActiveShip();
	const FVector Right = Ship->GetActorRightVector();
	const FVector Before = Ship->GetActorLocation();
	const bool bDodged = Ship->TryDodge(1.f);
	if (!TestTrue(TEXT("dodged"), bDodged)) { return false; }
	TestEqual(TEXT("one tell"), S.Foresight->GetTellCount(), 1);
	// The enemy aims ahead of the ship, on the side it dodged to.
	const FVector Aim = S.Enemy->GetActorForwardVector();
	const FVector ToBefore = (Before - S.Enemy->GetActorLocation()).GetSafeNormal();
	TestTrue(TEXT("aimed off the ship's old position, toward the dodge side"), FVector::DotProduct(Aim - ToBefore, Right) > 0.f);
	TestTrue(TEXT("guns on"), S.Enemy->AreGunsFiring());
	return true;
}

#endif

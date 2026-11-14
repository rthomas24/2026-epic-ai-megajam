// Board N-070 (Nick's UI; Teammate 2): an ability award gets one reward presentation, and a flight hint of its own the
// next time the player flies the owning ship; a later award gets its own presentation and its own hint.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Powers/TDAbilityRewards.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Tests/TDTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDAbilityRewardFlow, "TriadDrift.Powers.Abilities.RewardPresentationAndFlightHint", TD_TEST_FLAGS)
bool FTDAbilityRewardFlow::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
	ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(nullptr, ATDSpacePlayerController::StaticClass()));
	UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
	UTDAbilityRewardSubsystem* Rewards = UTDAbilityRewardSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("squad"), Squad) || !TestNotNull(TEXT("controller"), PC) || !TestNotNull(TEXT("bus"), Bus) || !TestNotNull(TEXT("rewards"), Rewards))
	{
		return false;
	}
	Rewards->ResetRewards();
	Squad->SetSquadPlayerController(PC);
	Squad->SpawnSquad(FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 20000.f)));
	W.Advance(.3f);
	const UWorld* World = W.GetWorld();

	// Fly something other than Bastion.
	const ETDShipId Other = Squad->GetActiveShipId() == ETDShipId::Bastion ? ETDShipId::Rift : Squad->GetActiveShipId();
	if (Squad->GetActiveShipId() != Other)
	{
		Squad->RequestSwitchTo(Other);
		W.Advance(1.f);
	}
	TestEqual(TEXT("flying another ship"), Squad->GetActiveShipId(), Other);

	ETDAbilityId A = ETDAbilityId::None;
	ETDShipId S = ETDShipId::Rift;
	bool bNow = true;
	TestFalse(TEXT("nothing pending at the start"), Rewards->GetPendingAbilityReward(A, S, bNow, World));

	// Gravity Well collected (as the on-foot objective awards it).
	Bus->BroadcastAbilityUnlocked(ETDAbilityId::GravityWell, ETDShipId::Bastion);
	TestTrue(TEXT("a reward is pending"), Rewards->GetPendingAbilityReward(A, S, bNow, World));
	TestTrue(TEXT("...Gravity Well for Bastion"), A == ETDAbilityId::GravityWell && S == ETDShipId::Bastion);
	TestFalse(TEXT("...not usable now (another ship)"), bNow);
	Rewards->MarkAbilityRewardPresented(ETDAbilityId::GravityWell);
	TestFalse(TEXT("presented once"), Rewards->GetPendingAbilityReward(A, S, bNow, World));

	TestFalse(TEXT("another ship: no flight hint"), Rewards->GetAbilityFlightHint(A, World));
	Squad->RequestSwitchTo(ETDShipId::Bastion);
	W.Advance(1.f);
	TestEqual(TEXT("now flying Bastion"), Squad->GetActiveShipId(), ETDShipId::Bastion);
	TestTrue(TEXT("Bastion: the flight hint is due"), Rewards->GetAbilityFlightHint(A, World) && A == ETDAbilityId::GravityWell);
	Rewards->MarkAbilityFlightHintShown(ETDAbilityId::GravityWell);
	TestFalse(TEXT("the hint shows once"), Rewards->GetAbilityFlightHint(A, World));

	// The next unlock gets its own presentation and its own hint.
	Bus->BroadcastAbilityUnlocked(ETDAbilityId::Cloak, ETDShipId::Rift);
	TestTrue(TEXT("Cloak pending"), Rewards->GetPendingAbilityReward(A, S, bNow, World) && A == ETDAbilityId::Cloak && S == ETDShipId::Rift);
	Rewards->MarkAbilityRewardPresented(ETDAbilityId::Cloak);
	TestFalse(TEXT("flying Bastion: no Cloak hint"), Rewards->GetAbilityFlightHint(A, World));
	Squad->RequestSwitchTo(ETDShipId::Rift);
	W.Advance(1.f);
	TestTrue(TEXT("Rift: Cloak's own hint (not suppressed by Gravity Well's)"), Rewards->GetAbilityFlightHint(A, World) && A == ETDAbilityId::Cloak);

	// Usable now: a reward collected while flying its ship.
	Bus->BroadcastAbilityUnlocked(ETDAbilityId::EMPPulse, ETDShipId::Rift);
	TestTrue(TEXT("EMP pending, usable now (flying Rift)"), Rewards->GetPendingAbilityReward(A, S, bNow, World) && A == ETDAbilityId::EMPPulse && bNow);
	Rewards->ResetRewards();
	return true;
}

#endif

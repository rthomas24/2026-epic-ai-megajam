// Board N-072 (Nick's UI; Teammate 2): the UI host mounts the reward screen for each pending award, by name against
// Nick's contract (a stand-in with the same names here): on foot the award shows with bCanUseNow false, dismissal
// acknowledges only that award, and the next distinct unlock gets its own presentation; a device change refreshes the key.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UObject/StrongObjectPtr.h"
#include "TriadDrift/Core/TDEventBus.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Powers/TDAbilityRewards.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/UI/TDUIHostSubsystem.h"
#include "TriadDrift/UI/Tests/TDRewardStandInWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDRewardMount, "TriadDrift.UI.Reward.HostMountsPendingAwards", TD_TEST_FLAGS)
bool FTDRewardMount::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	W.SpawnLocalPlayerController(nullptr, ATDSpacePlayerController::StaticClass()); // no ship: on foot as far as rewards go
	UTDUIHostSubsystem* Host = UTDUIHostSubsystem::Get(W.GetWorld());
	UTDEventBus* Bus = UTDEventBus::Get(W.GetWorld());
	UTDAbilityRewardSubsystem* Rewards = UTDAbilityRewardSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("host"), Host) || !TestNotNull(TEXT("bus"), Bus) || !TestNotNull(TEXT("rewards"), Rewards))
	{
		return false;
	}
	Rewards->ResetRewards();
	Host->bAddToViewport = false;
	Host->RewardWidgetClassOverride = UTDRewardStandInWidget::StaticClass();

	Host->UpdateAbilityReward();
	TestNull(TEXT("nothing pending: nothing mounted"), Host->GetRewardWidget());

	Bus->BroadcastAbilityUnlocked(ETDAbilityId::GravityWell, ETDShipId::Bastion);
	Host->UpdateAbilityReward();
	UTDRewardStandInWidget* Screen = Cast<UTDRewardStandInWidget>(Host->GetRewardWidget());
	if (!TestNotNull(TEXT("the award mounts the reward screen"), Screen))
	{
		return false;
	}
	TestTrue(TEXT("presented Gravity Well"), Screen->bActive && Screen->Data.AbilityId == FName(TEXT("GravityWell")));
	TestTrue(*FString::Printf(TEXT("ability and ship names (%s / %s)"), *Screen->Data.AbilityName.ToString(), *Screen->Data.ShipName.ToString()),
		Screen->Data.AbilityName.ToString().Contains(TEXT("GRAVITY")) && Screen->Data.ShipName.ToString().Contains(TEXT("BASTION")));
	TestFalse(TEXT("on foot: not usable now"), Screen->Data.bCanUseNow);
	TestTrue(*FString::Printf(TEXT("availability names the ship (%s)"), *Screen->Data.AvailabilityText.ToString()),
		Screen->Data.AvailabilityText.ToString().Contains(TEXT("BASTION")));
	TestFalse(TEXT("an activation key is supplied"), Screen->Data.ActivationKey.IsEmpty());
	TestEqual(TEXT("focus asked once"), Screen->Focuses, 1);
	Host->UpdateAbilityReward();
	TestEqual(TEXT("unchanged: not presented again"), Screen->Presents, 1);

	// A second award while the first is up waits its turn.
	Bus->BroadcastAbilityUnlocked(ETDAbilityId::Cloak, ETDShipId::Rift);
	Host->UpdateAbilityReward();
	TestEqual(TEXT("still Gravity Well on screen"), Screen->Data.AbilityId, FName(TEXT("GravityWell")));

	// Dismissal: that award is acknowledged, the next distinct one presents at once on the same screen.
	Screen->RequestDismiss();
	ETDAbilityId A;
	ETDShipId S;
	bool bNow;
	TestTrue(TEXT("Cloak is the pending one now"), Rewards->GetPendingAbilityReward(A, S, bNow, W.GetWorld()) && A == ETDAbilityId::Cloak);
	TestTrue(TEXT("Cloak presented at once"), Screen->bActive && Screen->Data.AbilityId == FName(TEXT("Cloak")));
	TestEqual(TEXT("host shows Cloak"), Host->GetPresentedReward(), ETDAbilityId::Cloak);
	TestEqual(TEXT("one screen instance"), Cast<UTDRewardStandInWidget>(Host->GetRewardWidget()), Screen);

	// A stale dismissal (not the award on screen) acknowledges nothing.
	Host->HandleRewardDismissed(TEXT("GravityWell"));
	TestTrue(TEXT("stale: Cloak still pending"), Rewards->GetPendingAbilityReward(A, S, bNow, W.GetWorld()) && A == ETDAbilityId::Cloak);
	Screen->RequestDismiss();
	TestFalse(TEXT("all acknowledged"), Rewards->GetPendingAbilityReward(A, S, bNow, W.GetWorld()));
	TestEqual(TEXT("host idle"), Host->GetPresentedReward(), ETDAbilityId::None);
	Rewards->ResetRewards();
	return true;
}

#endif

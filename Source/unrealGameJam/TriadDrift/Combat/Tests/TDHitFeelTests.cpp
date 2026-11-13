// Hit feel (Lead's polish list, Nov 13; Teammate 2). Run: .\Tools\td-test.ps1 -Filter TriadDrift.Combat.HitFeel

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/PlayerController.h"
#include "TriadDrift/Combat/TDProjectileSubsystem.h"
#include "TriadDrift/Core/TDStatics.h"
#include "TriadDrift/Enemies/TDEnemyLibrary.h"
#include "TriadDrift/Enemies/TDEnemyShip.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Powers/Tests/TDPowerTestCommon.h"
#include "TriadDrift/Squad/TDSquadManager.h"
#include "TriadDrift/Tests/TDTestWorld.h"

// A player kill of something big freezes time for a beat; small kills, wingmen's kills and kills during a director
// slow-mo do not (the slow-mo is the bigger beat).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDHitFeelBigKill, "TriadDrift.Combat.HitFeel.BigKillHitStop", TD_TEST_FLAGS)
bool FTDHitFeelBigKill::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	APlayerController* PC = W.SpawnLocalPlayerController();
	ATDSquadManager* Squad = W.Spawn<ATDSquadManager>();
	Squad->SetSquadPlayerController(PC);
	Squad->SpawnSquad(FTransform::Identity);
	W.Advance(.5f);
	ATDShip* Flown = Squad->GetShip(Squad->GetActiveShipId());
	ATDShip* Wingman = Squad->GetShip(Squad->GetActiveShipId() == ETDShipId::Rift ? ETDShipId::Lancer : ETDShipId::Rift);
	UTDProjectileSubsystem* Combat = UTDProjectileSubsystem::Get(W.GetWorld());
	UTDFeelSubsystem* Feel = UTDFeelSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("flown ship"), Flown) || !TestNotNull(TEXT("combat subsystem"), Combat) || !TestNotNull(TEXT("feel"), Feel))
	{
		return false;
	}
	auto Kill = [&](const FName Id, AActor* Killer, const FVector& At)
	{
		ATDEnemyShip* Enemy = TDPowerTest::SpawnEnemyShip(W, Id, At);
		W.Tick();
		const int32 Before = Combat->GetBigKillHitStops();
		UTDStatics::ApplyTDDamage(Enemy, 1e7f, ETDDamageKind::Energy, Killer, Killer, At, FVector::ForwardVector, true);
		const bool bStopped = Combat->GetBigKillHitStops() > Before;
		const float Dilation = Feel->GetCurrentDilation();
		W.Advance(.5f); // let the hit-stop end before the next case
		return TPair<bool, float>(bStopped, Dilation);
	};

	const auto Fighter = Kill(TDEnemyIds::ShardFighter, Flown, FVector(8000.f, 0.f, 0.f));
	TestFalse(TEXT("player kills a fighter: no hit-stop"), Fighter.Key);
	const auto Carrier = Kill(TDEnemyIds::HiveCarrier, Flown, FVector(20000.f, 0.f, 0.f));
	TestTrue(TEXT("player kills a carrier: hit-stop"), Carrier.Key);
	TestTrue(TEXT("player kills a carrier: time slows at once"), Carrier.Value < .5f);
	const auto ByWing = Kill(TDEnemyIds::HiveCarrier, Wingman, FVector(20000.f, 20000.f, 0.f));
	TestFalse(TEXT("a wingman's carrier kill: no hit-stop on the player's screen"), ByWing.Key);
	Feel->RequestSlowMo(1.f, .3f);
	const auto InSlowMo = Kill(TDEnemyIds::HiveCarrier, Flown, FVector(20000.f, -20000.f, 0.f));
	TestFalse(TEXT("during a director slow-mo: no stacked hit-stop"), InSlowMo.Key);
	Feel->CancelAll();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

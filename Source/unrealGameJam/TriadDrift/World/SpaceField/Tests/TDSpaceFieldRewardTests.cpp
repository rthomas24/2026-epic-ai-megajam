// Triad Drift - space field rewards through T2's drop hook (Teammate 6). Needs lead/int with T2's
// UTDPickupSubsystem::HandleFieldObjectDestroyed (fb1d870f or later). Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.SpaceField

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/IConsoleManager.h"
#include "Misc/ScopeExit.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/World/SpaceField/TDSpaceField.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/Combat/Pickups/TDPickup.h"
#include "TriadDrift/Combat/Pickups/TDPickupSubsystem.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Input/TDSpacePlayerController.h"
#include "TriadDrift/Squad/TDSquadManager.h"

namespace TDSpaceFieldRewardTest
{
	struct FFixture
	{
		IConsoleVariable* PickupsVar = IConsoleManager::Get().FindConsoleVariable(TEXT("td.Pickups"));
		int32 OldPickups = 1;
		FTDTestWorld W;
		ATDGalaxyLayout* Layout = nullptr;
		ATDSpaceField* Field = nullptr;
		ATDSquadManager* Squad = nullptr;
		UTDPickupSubsystem* Pickups = nullptr;
		ATDShip* Flown = nullptr;

		FFixture() { if (PickupsVar) { OldPickups = PickupsVar->GetInt(); PickupsVar->Set(1, ECVF_SetByCode); } }
		~FFixture() { if (PickupsVar) { PickupsVar->Set(OldPickups, ECVF_SetByCode); } }

		bool Init(FAutomationTestBase& T)
		{
			Layout = ATDGalaxyLayout::SpawnDefaultGalaxy(W.GetWorld());
			W.Advance(0.1f);
			Field = ATDSpaceField::SpawnFor(Layout);
			Squad = W.Spawn<ATDSquadManager>();
			ATDSpacePlayerController* PC = Cast<ATDSpacePlayerController>(W.SpawnLocalPlayerController(nullptr, ATDSpacePlayerController::StaticClass()));
			if (!T.TestNotNull(TEXT("field"), Field) || !T.TestNotNull(TEXT("squad"), Squad) || !T.TestNotNull(TEXT("controller"), PC)) { return false; }
			Squad->SetSquadPlayerController(PC);
			Squad->SpawnSquad(FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 20000.f)));
			W.Advance(0.3f);
			Pickups = UTDPickupSubsystem::Get(W.GetWorld());
			Flown = Squad->GetActiveShip();
			if (!T.TestNotNull(TEXT("pickups"), Pickups) || !T.TestNotNull(TEXT("flown ship"), Flown)) { return false; }
			Pickups->Rng.Initialize(1234);
			return true;
		}

		/** A live piece (special or not, of a class) with nothing else within 80 m, so its drop is easy to find. */
		int32 FindLone(bool bSpecial, int32 Class = -1) const
		{
			const TArray<FTDSpaceFieldPiece>& P = Field->GetPieces();
			for (int32 i = 0; i < P.Num(); ++i)
			{
				if (!Field->IsPieceAlive(i) || Field->IsPieceSpecial(i) != bSpecial || P[i].Kind == ETDSpaceFieldKind::Wreck) { continue; }
				if (Class >= 0 && Field->GetPieceSizeClass(i) != Class) { continue; }
				bool bLone = true;
				for (int32 j = 0; j < P.Num() && bLone; ++j) { bLone = j == i || !Field->IsPieceAlive(j) || FVector::Dist(P[i].Location, P[j].Location) > 8000.f; }
				if (bLone) { return i; }
			}
			return INDEX_NONE;
		}

		ATDPickup* PickupNear(const FVector& At, float Radius) const
		{
			for (ATDPickup* P : Pickups->GetPickups()) { if (IsValid(P) && FVector::Dist(P->GetActorLocation(), At) < Radius) { return P; } }
			return nullptr;
		}

		int32 CollectedTotal() const
		{
			int32 N = 0;
			for (int32 T = 0; T <= static_cast<int32>(ETDPickupType::Boost); ++T) { N += Pickups->GetCollected(static_cast<ETDPickupType>(T)); }
			return N;
		}
	};
}

// Ryan: shoot a special asteroid and it pays out. The break by the player's ship spawns a reward at the rock (T2's hook:
// mostly a Boost refill), flying the ship to it collects it; a plain rock or debris only counts toward "Debris cleared";
// a break nobody in the squad caused gives nothing.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSpaceFieldSpecialReward, "TriadDrift.World.SpaceField.Break.SpecialDropsARewardThePlayerCollects", TD_TEST_FLAGS)
bool FTDSpaceFieldSpecialReward::RunTest(const FString& Parameters)
{
	TDSpaceFieldRewardTest::FFixture F;
	if (!F.Init(*this)) { return false; }

	// A special, broken by the flown ship: one reward appears where it was.
	const int32 S = F.FindLone(true);
	if (!TestTrue(TEXT("a lone special"), S != INDEX_NONE)) { return false; }
	const FVector At = F.Field->GetPieces()[S].Location;
	const float R = F.Field->GetPieces()[S].Size;
	const int32 DebrisBefore = F.Pickups->GetDebrisThisScene();
	TestTrue(TEXT("the special breaks"), F.Field->DamagePiece(S, 1000000.f, F.Flown));
	ATDPickup* Reward = F.PickupNear(At, R + 1000.f);
	if (!TestNotNull(TEXT("a reward spawned at the special"), Reward)) { return false; }
	AddInfo(FString::Printf(TEXT("reward type %d (Boost = %d)"), (int32)Reward->GetType(), (int32)ETDPickupType::Boost));
	TestEqual(TEXT("the break counts toward Debris cleared"), F.Pickups->GetDebrisThisScene(), DebrisBefore + 1);

	// The player flies to it: collected.
	const int32 CollectedBefore = F.CollectedTotal();
	F.Flown->TeleportSpacecraft(Reward->GetActorLocation() - FVector(500.f, 0.f, 0.f), FRotator::ZeroRotator, false);
	F.W.Advance(0.6f);
	TestEqual(TEXT("the reward is collected by the flown ship"), F.CollectedTotal(), CollectedBefore + 1);
	TestFalse(TEXT("and gone"), IsValid(Reward));

	// Plain rocks and debris: Debris cleared, no pickup. (Batch M adds T2's boost top-up by size; asserted then.)
	for (const int32 Class : { 2, 1, 0 })
	{
		const int32 P = F.FindLone(false, Class);
		if (!TestTrue(*FString::Printf(TEXT("a lone plain piece of class %d"), Class), P != INDEX_NONE)) { return false; }
		const FVector PAt = F.Field->GetPieces()[P].Location;
		const int32 Debris = F.Pickups->GetDebrisThisScene();
		F.Field->DamagePiece(P, 1000000.f, F.Flown);
		TestEqual(*FString::Printf(TEXT("class %d counts toward Debris cleared"), Class), F.Pickups->GetDebrisThisScene(), Debris + 1);
		TestNull(*FString::Printf(TEXT("class %d drops no pickup"), Class), F.PickupNear(PAt, 5000.f));
	}

	// A special broken by nobody in the squad (no instigator): no reward, no stat.
	const int32 S2 = F.FindLone(true);
	if (!TestTrue(TEXT("another special"), S2 != INDEX_NONE)) { return false; }
	const FVector At2 = F.Field->GetPieces()[S2].Location;
	const int32 Debris2 = F.Pickups->GetDebrisThisScene();
	F.Field->DamagePiece(S2, 1000000.f, nullptr);
	TestNull(TEXT("no reward for a break nobody caused"), F.PickupNear(At2, F.Field->GetPieces()[S2].Size + 1000.f));
	TestEqual(TEXT("and no stat"), F.Pickups->GetDebrisThisScene(), Debris2);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

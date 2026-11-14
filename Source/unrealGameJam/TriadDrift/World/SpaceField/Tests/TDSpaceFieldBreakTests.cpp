// Triad Drift - space field destructible tests (Teammate 6). Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.SpaceField

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/World/SpaceField/TDSpaceField.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "Components/InstancedStaticMeshComponent.h"

namespace TDSpaceFieldBreakTest
{
	struct FField
	{
		FTDTestWorld W;
		ATDGalaxyLayout* Layout = nullptr;
		ATDSpaceField* Field = nullptr;
		FField()
		{
			Layout = ATDGalaxyLayout::SpawnDefaultGalaxy(W.GetWorld());
			W.Advance(0.1f);
			Field = ATDSpaceField::SpawnFor(Layout);
			W.Advance(0.1f);
		}
		/** The first alive piece of a size class (not a wreck, not special). */
		int32 Find(uint8 Class, bool bSpecial = false) const
		{
			for (int32 i = 0; i < Field->GetPieces().Num(); ++i)
			{
				if (Field->IsPieceAlive(i) && Field->GetPieceSizeClass(i) == Class && Field->IsPieceSpecial(i) == bSpecial
					&& Field->GetPieces()[i].Kind != ETDSpaceFieldKind::Wreck)
				{
					return i;
				}
			}
			return INDEX_NONE;
		}
		int32 AliveNear(const FVector& At, float Radius, uint8 Class) const
		{
			int32 N = 0;
			for (int32 i = 0; i < Field->GetPieces().Num(); ++i)
			{
				N += Field->IsPieceAlive(i) && Field->GetPieceSizeClass(i) == Class && FVector::Dist(Field->GetPieces()[i].Location, At) < Radius ? 1 : 0;
			}
			return N;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSpaceFieldEveryPieceSolid, "TriadDrift.World.SpaceField.Break.EveryPieceHasCollisionAndHealth", TD_TEST_FLAGS)
bool FTDSpaceFieldEveryPieceSolid::RunTest(const FString& Parameters)
{
	TDSpaceFieldBreakTest::FField F;
	if (!TestNotNull(TEXT("field"), F.Field)) { return false; }
	const int32 N = F.Field->GetPieces().Num();
	int32 NoCollider = 0, NoHealth = 0, Specials = 0, Eligible = 0;
	for (int32 i = 0; i < N; ++i)
	{
		FTransform T;
		NoCollider += F.Field->GetColliderTransform(i, T) ? 0 : 1;
		NoHealth += F.Field->GetPieceHealth(i) > 0.f ? 0 : 1;
		Specials += F.Field->IsPieceSpecial(i) ? 1 : 0;
		const ETDSpaceFieldKind K = F.Field->GetPieces()[i].Kind;
		Eligible += (K == ETDSpaceFieldKind::Belt || K == ETDSpaceFieldKind::Cluster) && F.Field->GetPieceSizeClass(i) >= 1 && !F.Field->GetPieces()[i].bAdopted ? 1 : 0;
	}
	TestEqual(TEXT("every piece has a collision sphere"), NoCollider, 0);
	TestEqual(TEXT("every piece has health"), NoHealth, 0);
	TestEqual(TEXT("one collision instance per piece"), F.Field->GetCollision()->GetInstanceCount(), N);
	TestTrue(*FString::Printf(TEXT("the collision blocks (profile %s)"), *F.Field->GetCollision()->GetCollisionProfileName().ToString()),
		F.Field->GetCollision()->GetCollisionProfileName() == TEXT("BlockAll"));
	AddInfo(FString::Printf(TEXT("%d pieces, %d specials of %d eligible (1 in %.1f)"), N, Specials, Eligible, Eligible / FMath::Max(1.f, (float)Specials)));
	TestTrue(TEXT("about one asteroid in 15 is special (1 in 10-25)"), Specials * 10 <= Eligible && Specials * 25 >= Eligible);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSpaceFieldSmallBreaks, "TriadDrift.World.SpaceField.Break.SmallBreaksAndItsCollisionGoes", TD_TEST_FLAGS)
bool FTDSpaceFieldSmallBreaks::RunTest(const FString& Parameters)
{
	TDSpaceFieldBreakTest::FField F;
	if (!TestNotNull(TEXT("field"), F.Field)) { return false; }
	const int32 P = F.Find(0);
	if (!TestTrue(TEXT("a small piece"), P != INDEX_NONE)) { return false; }
	const FVector At = F.Field->GetPieces()[P].Location;
	TestFalse(TEXT("a weak shot (4) does not break it"), F.Field->DamagePiece(P, 4.f, nullptr));
	TestTrue(TEXT("two cannon shots (8 + 8) break it"), F.Field->DamagePiece(P, 8.f, nullptr));
	TestFalse(TEXT("gone"), F.Field->IsPieceAlive(P));
	TestEqual(TEXT("broken count"), F.Field->GetBrokenCount(), 1);
	FTransform T;
	TestFalse(TEXT("no collider transform for a dead piece"), F.Field->GetColliderTransform(P, T));
	// Nothing solid is left where it was: an overlap at its centre finds no field collision.
	FHitResult Hit;
	const bool bHit = F.W.GetWorld()->SweepSingleByChannel(Hit, At - FVector(500.f, 0.f, 0.f), At + FVector(500.f, 0.f, 0.f), FQuat::Identity, ECC_WorldDynamic,
		FCollisionShape::MakeSphere(50.f));
	TestFalse(TEXT("its collision sphere is gone"), bHit && Hit.GetComponent() == F.Field->GetCollision() && F.Field->PieceForHit(Hit.GetComponent(), Hit.Item) == P);
	TestFalse(TEXT("a dead piece takes no more damage"), F.Field->DamagePiece(P, 100.f, nullptr));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSpaceFieldSplits, "TriadDrift.World.SpaceField.Break.MediumSplitsIntoSmallsLargeIntoMediums", TD_TEST_FLAGS)
bool FTDSpaceFieldSplits::RunTest(const FString& Parameters)
{
	TDSpaceFieldBreakTest::FField F;
	if (!TestNotNull(TEXT("field"), F.Field)) { return false; }
	for (const uint8 Class : { (uint8)1, (uint8)2 })
	{
		const int32 P = F.Find(Class);
		if (!TestTrue(TEXT("a piece of the class"), P != INDEX_NONE)) { return false; }
		const FVector At = F.Field->GetPieces()[P].Location;
		const float R = F.Field->GetPieces()[P].Size;
		const int32 Before = F.AliveNear(At, R, Class - 1);
		const int32 Colliders = F.Field->GetCollision()->GetInstanceCount();
		TestFalse(TEXT("one shot does not break it"), F.Field->DamagePiece(P, 8.f, nullptr));
		TestTrue(TEXT("enough breaks it"), F.Field->DamagePiece(P, 100000.f, nullptr));
		const int32 After = F.AliveNear(At, R, Class - 1);
		TestTrue(*FString::Printf(TEXT("class %d splits into 2-3 of class %d (%d new)"), Class, Class - 1, After - Before), After - Before >= 2 && After - Before <= 3);
		// The dead piece's collision slot is reused by a fragment: at most two new collision instances.
		TestTrue(TEXT("collision slots recycled"), F.Field->GetCollision()->GetInstanceCount() <= Colliders + 2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSpaceFieldRamAndSplash, "TriadDrift.World.SpaceField.Break.RamSmashesMediumsLargeStopsItSplashBreaks", TD_TEST_FLAGS)
bool FTDSpaceFieldRamAndSplash::RunTest(const FString& Parameters)
{
	TDSpaceFieldBreakTest::FField F;
	if (!TestNotNull(TEXT("field"), F.Field)) { return false; }
	// Ram through a medium: broken, no block.
	const int32 M = F.Find(1);
	if (!TestTrue(TEXT("a medium"), M != INDEX_NONE)) { return false; }
	const FVector MAt = F.Field->GetPieces()[M].Location;
	FVector Block;
	const bool bBlockedM = F.Field->RamSweep(MAt - FVector(5000.f, 0.f, 0.f), MAt + FVector(5000.f, 0.f, 0.f), 600.f, nullptr, Block);
	TestFalse(TEXT("the ram smashed the medium"), F.Field->IsPieceAlive(M));
	if (bBlockedM) { AddInfo(TEXT("a large rock was also on the medium's line")); }
	// Ram into a large: stopped short of it, the rock stands.
	const int32 L = F.Find(2);
	if (!TestTrue(TEXT("a large"), L != INDEX_NONE)) { return false; }
	const FVector LAt = F.Field->GetPieces()[L].Location;
	const FVector From = LAt - FVector(20000.f, 0.f, 0.f);
	TestTrue(TEXT("a large stops the ram"), F.Field->RamSweep(From, LAt + FVector(20000.f, 0.f, 0.f), 600.f, nullptr, Block));
	TestTrue(TEXT("the stop is before the rock"), FVector::Dist(Block, From) <= FVector::Dist(LAt, From));
	// Splash on a small: broken.
	const int32 S = F.Find(0);
	if (!TestTrue(TEXT("a small"), S != INDEX_NONE)) { return false; }
	F.Field->Splash(F.Field->GetPieces()[S].Location + FVector(300.f, 0.f, 0.f), 1500.f, 40.f, nullptr);
	TestFalse(TEXT("a missile's splash breaks a small"), F.Field->IsPieceAlive(S));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSpaceFieldShipImpact, "TriadDrift.World.SpaceField.Break.ShipImpactBySpeed", TD_TEST_FLAGS)
bool FTDSpaceFieldShipImpact::RunTest(const FString& Parameters)
{
	TDSpaceFieldBreakTest::FField F;
	if (!TestNotNull(TEXT("field"), F.Field)) { return false; }
	FTransform T;
	auto ItemOf = [&F](int32 Piece) { int32 Found = INDEX_NONE; for (int32 i = 0; i < 1000000 && Found == INDEX_NONE; ++i) { if (F.Field->PieceForHit(F.Field->GetCollision(), i) == Piece) { Found = i; } if (i > F.Field->GetCollision()->GetInstanceCount()) { break; } } return Found; };
	const int32 L = F.Find(2);
	const int32 S = F.Find(0);
	const int32 M = F.Find(1);
	if (!TestTrue(TEXT("pieces"), L != INDEX_NONE && S != INDEX_NONE && M != INDEX_NONE)) { return false; }
	// Full boost (108 m/s) head-on into a large: it stands, the ship bounces (false).
	const float Hp = F.Field->GetPieceHealth(L);
	TestFalse(TEXT("a large stands a full-boost hit"), F.Field->ShipImpact(F.Field->GetCollision(), ItemOf(L), 10800.f, 1.f, nullptr, false));
	TestTrue(TEXT("but takes damage"), F.Field->GetPieceHealth(L) < Hp);
	// A scrape-speed hit on a small shatters it and lets the ship through (true).
	TestTrue(TEXT("a small shatters on a 30 m/s hit"), F.Field->ShipImpact(F.Field->GetCollision(), ItemOf(S), 3000.f, 1.f, nullptr, false));
	// Ramming: a medium is smashed and the ship keeps going.
	TestTrue(TEXT("the ram smashes a medium"), F.Field->ShipImpact(F.Field->GetCollision(), ItemOf(M), 2000.f, 0.5f, nullptr, true));
	TestFalse(TEXT("gone"), F.Field->IsPieceAlive(M));
	// A hit that names something else is ignored.
	TestFalse(TEXT("not a field collider"), F.Field->ShipImpact(nullptr, 0, 10000.f, 1.f, nullptr, false));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

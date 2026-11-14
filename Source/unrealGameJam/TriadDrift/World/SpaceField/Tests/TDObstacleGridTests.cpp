// Triad Drift - the obstacle grid in UTDWorldSubsystem (Teammate 6): same answers as the plain loop it replaced, removal.
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.SpaceField

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/World/TDWorldSubsystem.h"
#include "TriadDrift/World/SpaceField/TDSpaceField.h"
#include "TriadDrift/World/TDGalaxyLayout.h"

namespace TDObstacleGridTest
{
	/** The original linear FindAvoidance (T4), as the reference. */
	static bool Reference(const TArray<FSphere>& Obstacles, const FVector& Origin, const FVector& Direction, float ProbeDistance, float ShipRadius, FVector& Out)
	{
		const FVector Dir = Direction.GetSafeNormal();
		const FSphere* Nearest = nullptr;
		float NearestAlong = TNumericLimits<float>::Max();
		for (const FSphere& S : Obstacles)
		{
			if (S.W < 0.f) { continue; }
			const FVector To = S.Center - Origin;
			const float Along = FVector::DotProduct(To, Dir);
			const float Reach = S.W + ShipRadius;
			if (Along < -Reach || Along > ProbeDistance + Reach) { continue; }
			if ((To - Dir * Along).SizeSquared() <= FMath::Square(Reach) && Along < NearestAlong) { Nearest = &S; NearestAlong = Along; }
		}
		if (!Nearest) { return false; }
		const FVector To = Nearest->Center - Origin;
		FVector Lateral = -(To - Dir * FVector::DotProduct(To, Dir));
		if (Lateral.SizeSquared() < 1.f) { Lateral = FVector::CrossProduct(Dir, FVector::RightVector); if (Lateral.SizeSquared() < 1e-4f) { Lateral = FVector::UpVector; } }
		Lateral.Normalize();
		const float Urgency = FMath::Clamp(1.f - NearestAlong / FMath::Max(1.f, ProbeDistance), 0.25f, 1.f);
		Out = (Dir * (1.f - Urgency) + Lateral * (0.6f + Urgency)).GetSafeNormal();
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDObstacleGridSame, "TriadDrift.World.SpaceField.ObstacleGridMatchesTheLoopAndRemoves", TD_TEST_FLAGS)
bool FTDObstacleGridSame::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	UTDWorldSubsystem* S = UTDWorldSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("subsystem"), S)) { return false; }
	S->ClearObstacles();
	FRandomStream Rng(42);
	for (int32 i = 0; i < 3000; ++i)
	{
		const float R = i % 100 == 0 ? Rng.FRandRange(15000.f, 60000.f) : Rng.FRandRange(300.f, 9000.f);	// some big (the Dominion, planets)
		S->AddObstacle(FVector(Rng.FRandRange(-600000.f, 600000.f), Rng.FRandRange(-600000.f, 600000.f), Rng.FRandRange(-200000.f, 200000.f)), R);
	}
	for (int32 i = 0; i < 300; i += 7) { S->RemoveObstacle(i); }
	S->RemoveObstacle(0);	// twice: no-op
	TestEqual(TEXT("removed count"), S->GetActiveObstacleCount(), 3000 - 43);
	int32 Mismatch = 0, Hits = 0;
	for (int32 q = 0; q < 4000; ++q)
	{
		const FVector O(Rng.FRandRange(-600000.f, 600000.f), Rng.FRandRange(-600000.f, 600000.f), Rng.FRandRange(-200000.f, 200000.f));
		const FVector D(Rng.GetUnitVector());
		const float Probe = Rng.FRandRange(5000.f, 60000.f);
		FVector A, B;
		const bool bA = S->FindAvoidance(O, D, Probe, 600.f, A);
		const bool bB = TDObstacleGridTest::Reference(S->GetObstacles(), O, D, Probe, 600.f, B);
		Hits += bB ? 1 : 0;
		if (bA != bB || (bA && !A.Equals(B, 1e-3f))) { ++Mismatch; }
		bool bIn = false;
		for (const FSphere& Sp : S->GetObstacles()) { bIn |= Sp.W >= 0.f && FVector::DistSquared(O, Sp.Center) <= FMath::Square(Sp.W + 1000.f); }
		if (bIn != S->IsInsideObstacle(O, 1000.f)) { ++Mismatch; }
	}
	AddInfo(FString::Printf(TEXT("4000 probes, %d hit something"), Hits));
	TestTrue(TEXT("enough probes hit something to mean it"), Hits > 200);
	TestEqual(TEXT("the grid answers exactly like the loop"), Mismatch, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSpaceFieldObstaclesFollowBreaks, "TriadDrift.World.SpaceField.Break.ObstaclesFollowBreaks", TD_TEST_FLAGS)
bool FTDSpaceFieldObstaclesFollowBreaks::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDGalaxyLayout* Layout = ATDGalaxyLayout::SpawnDefaultGalaxy(W.GetWorld());
	W.Advance(0.1f);
	ATDSpaceField* Field = ATDSpaceField::SpawnFor(Layout);
	W.Advance(0.1f);
	UTDWorldSubsystem* S = UTDWorldSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("field"), Field) || !TestNotNull(TEXT("subsystem"), S)) { return false; }
	// A medium is an obstacle; once broken it is not, and its smalls are not.
	int32 M = INDEX_NONE;
	for (int32 i = 0; i < Field->GetPieces().Num() && M == INDEX_NONE; ++i)
	{
		if (Field->IsPieceAlive(i) && Field->GetPieceSizeClass(i) == 1 && Field->GetPieces()[i].Kind != ETDSpaceFieldKind::Wreck) { M = i; }
	}
	if (!TestTrue(TEXT("a medium"), M != INDEX_NONE)) { return false; }
	const FVector At = Field->GetPieces()[M].Location;
	const float R = Field->GetPieces()[M].Size * 0.5f;
	FVector Out;
	TestTrue(TEXT("a wingman probing at a medium steers away"), S->FindAvoidance(At - FVector(R + 5000.f, 0.f, 0.f), FVector::ForwardVector, R + 6000.f, 600.f, Out));
	const int32 Active = S->GetActiveObstacleCount();
	Field->DamagePiece(M, 100000.f, nullptr);
	TestEqual(TEXT("its obstacle is gone (smalls are not obstacles)"), S->GetActiveObstacleCount(), Active - 1);
	// A large splits into mediums: those are obstacles.
	int32 L = INDEX_NONE;
	for (int32 i = 0; i < Field->GetPieces().Num() && L == INDEX_NONE; ++i)
	{
		if (Field->IsPieceAlive(i) && Field->GetPieceSizeClass(i) == 2 && Field->GetPieces()[i].Kind != ETDSpaceFieldKind::Wreck) { L = i; }
	}
	if (!TestTrue(TEXT("a large"), L != INDEX_NONE)) { return false; }
	const int32 Before = S->GetActiveObstacleCount();
	Field->DamagePiece(L, 1000000.f, nullptr);
	const int32 Delta = S->GetActiveObstacleCount() - Before;
	TestTrue(*FString::Printf(TEXT("a large becomes 2-3 medium obstacles (net %+d)"), Delta), Delta >= 1 && Delta <= 2);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

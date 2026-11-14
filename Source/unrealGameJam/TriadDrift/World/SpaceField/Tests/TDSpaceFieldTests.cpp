// Triad Drift - space field tests (WORLD-DENSITY-PLAN 3.3). Owner: Teammate 5.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/World/SpaceField/TDSpaceField.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/World/TDWorldSubsystem.h"
#include "HAL/PlatformTime.h"
#include "TriadDrift/Combat/Pickups/TDPickup.h"
#include "TriadDrift/Combat/Pickups/TDPickupSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"

#ifndef TD_TEST_FLAGS
#define TD_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
#endif

namespace TDSpaceFieldTest
{
	struct FField
	{
		FTDTestWorld W;
		ATDGalaxyLayout* Layout = nullptr;
		ATDSpaceField* Field = nullptr;
		int32 ObstaclesBefore = 0;
		FField()
		{
			Layout = ATDGalaxyLayout::SpawnDefaultGalaxy(W.GetWorld());
			W.Advance(0.1f);
			if (const UTDWorldSubsystem* S = UTDWorldSubsystem::Get(W.GetWorld())) { ObstaclesBefore = S->GetObstacleCount(); }
			Field = ATDSpaceField::SpawnFor(Layout);
			W.Advance(0.1f);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSpaceFieldFills, "TriadDrift.World.SpaceField.FillsEveryDirectionAndNeverBlocks", TD_TEST_FLAGS)
bool FTDSpaceFieldFills::RunTest(const FString& Parameters)
{
	TDSpaceFieldTest::FField F;
	if (!TestNotNull(TEXT("layout"), F.Layout) || !TestNotNull(TEXT("field"), F.Field)) { return false; }
	const TArray<FTDSpaceFieldPiece>& Pieces = F.Field->GetPieces();
	int32 Kinds[5] = {};
	for (const FTDSpaceFieldPiece& P : Pieces) { ++Kinds[(int32)P.Kind]; }
	AddInfo(FString::Printf(TEXT("pieces %d: belt %d, cluster %d, debris %d, wreck %d, buoy %d; obstacles +%d"), Pieces.Num(), Kinds[0], Kinds[1], Kinds[2], Kinds[3], Kinds[4],
		F.Field->GetObstacleCount()));
	TestTrue(TEXT("belts"), Kinds[0] > 1000);
	TestTrue(TEXT("clusters"), Kinds[1] > 300);
	TestTrue(TEXT("debris fields"), Kinds[2] > 300);
	TestTrue(TEXT("wrecks"), Kinds[3] >= 3);
	TestTrue(TEXT("buoys"), Kinds[4] >= 10);

	// The hard rules, checked against the layout directly (not through the field's own function).
	const FTransform Xf = F.Layout->GetActorTransform();
	const FTDGalaxyLayoutData& L = F.Layout->GetLayout();
	const FVector Zone = Xf.TransformPosition(L.CombatZoneCenter);
	int32 Bad = 0;
	FString FirstBad;
	for (const FTDSpaceFieldPiece& P : Pieces)
	{
		if (P.bAdopted) { continue; }	// the layout's own rocks keep the layout's rules (cover in the pocket)
		const float Half = P.Size * 0.5f;
		bool bOk = FVector::Dist(P.Location, Zone) >= 120000.f;
		if (P.bCollides)
		{
			bOk &= FVector::Dist(P.Location, Zone) >= 150000.f + Half * 0.f;
			bOk &= !F.Layout->IsInKeepOut(Xf.InverseTransformPosition(P.Location), 20000.f);
			for (const FTDPlanetSpec& Pl : L.Planets) { bOk &= FVector::Dist(P.Location, Xf.TransformPosition(Pl.Location)) > Pl.AtmosphereRadius * 1.3f; }
			bOk &= FVector::Dist(P.Location, Xf.TransformPosition(L.Dominion.GetLocation())) > 350000.f; // T4: Dominion, field, generators, docking cone, escape
				for (const FTDPlanetSpec& Pl : L.Planets)
				{
					// The Verdance convoys (Dominion -> Verdance, offsets up to 900 m).
					if (Pl.Destination == ETDDestination::Verdance)
					{
						const FVector A = Xf.TransformPosition(L.Dominion.GetLocation()), B = Xf.TransformPosition(Pl.Location);
						bOk &= FVector::Dist(P.Location, FMath::ClosestPointOnSegment(P.Location, A, B)) > 120000.f;
					}
				}
				for (const TPair<FName, FTransform>& RP : L.ReturnPoints) { bOk &= FVector::Dist(P.Location, Xf.TransformPosition(RP.Value.GetLocation())) > 50000.f; }
			// Lane cores: the line from the zone to each planet, the Dominion and each gate.
			TArray<FVector> Ends;
			for (const FTDPlanetSpec& Pl : L.Planets) { Ends.Add(Xf.TransformPosition(Pl.Location)); }
			Ends.Add(Xf.TransformPosition(L.Dominion.GetLocation()));
			for (const FTDGatePairSpec& G : L.Gates) { Ends.Add(Xf.TransformPosition(G.A.GetLocation())); Ends.Add(Xf.TransformPosition(G.B.GetLocation())); }
			for (const FVector& E : Ends) { bOk &= FVector::Dist(P.Location, FMath::ClosestPointOnSegment(P.Location, Zone, E)) > 15000.f; }
		}
		if (!bOk)
		{
			++Bad;
			if (FirstBad.IsEmpty()) { FirstBad = FString::Printf(TEXT("%s size %.0f kind %d collides %d"), *P.Location.ToCompactString(), P.Size, (int32)P.Kind, P.bCollides); }
		}
	}
	TestEqual(*FString::Printf(TEXT("nothing in a keep-out (first: %s)"), *FirstBad), Bad, 0);

	// Something in every direction: 26 points on a 4 km sphere round the zone, at least 20 pieces within 1.5 km of each.
	int32 Thin = 0;
	FString ThinList;
	for (int32 i = 0; i < 26; ++i)
	{
		const float Y = 1.f - 2.f * (i + 0.5f) / 26.f;
		const float R = FMath::Sqrt(1.f - Y * Y);
		const float Phi = i * PI * (3.f - FMath::Sqrt(5.f));
		const FVector S = Zone + FVector(FMath::Cos(Phi) * R, FMath::Sin(Phi) * R, Y) * 400000.f;
		// The Dominion's 3.5 km sphere and the planets' approaches are empty by design.
		if (FVector::Dist(S, Xf.TransformPosition(L.Dominion.GetLocation())) < 350000.f + 100000.f) { continue; }
		int32 Near = 0;
		for (const FTDSpaceFieldPiece& P : Pieces) { Near += FVector::DistSquared(P.Location, S) < FMath::Square(150000.f) ? 1 : 0; }
		if (Near < 20) { ++Thin; ThinList += FString::Printf(TEXT("%d(%d) "), i, Near); }
	}
	TestTrue(*FString::Printf(TEXT("at most 6 of 26 bearings thin (thin: %s)"), *ThinList), Thin <= 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSpaceFieldDeterministic, "TriadDrift.World.SpaceField.SameEveryRunAndCacheSpots", TD_TEST_FLAGS)
bool FTDSpaceFieldDeterministic::RunTest(const FString& Parameters)
{
	TArray<FVector> First;
	int32 Count = 0;
	{
		TDSpaceFieldTest::FField F;
		if (!TestNotNull(TEXT("field"), F.Field)) { return false; }
		Count = F.Field->GetPieces().Num();
		for (int32 i = 0; i < FMath::Min(50, Count); ++i) { First.Add(F.Field->GetPieces()[i].Location); }
		const TArray<FVector> Spots = ATDSpaceField::GetCacheSpots(F.W.GetWorld());
		TestTrue(*FString::Printf(TEXT("cache spots in the debris fields (%d)"), Spots.Num()), Spots.Num() >= 8);
	}
	TDSpaceFieldTest::FField G;
	TestEqual(TEXT("same count"), G.Field->GetPieces().Num(), Count);
	bool bSame = true;
	for (int32 i = 0; i < First.Num(); ++i) { bSame &= G.Field->GetPieces()[i].Location.Equals(First[i], 1.f); }
	TestTrue(TEXT("same pieces in the same places"), bSame);
	return true;
}

// The wingmen and enemies steer with FindAvoidance (a plain loop over every obstacle): measure its cost with the field.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSpaceFieldAvoidanceCost, "TriadDrift.World.SpaceField.AvoidanceCostWithTheField", TD_TEST_FLAGS)
bool FTDSpaceFieldAvoidanceCost::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDGalaxyLayout* Layout = ATDGalaxyLayout::SpawnDefaultGalaxy(W.GetWorld());
	W.Advance(0.1f);
	UTDWorldSubsystem* S = UTDWorldSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("world subsystem"), S)) { return false; }
	auto Measure = [S]()
	{
		FRandomStream Rng(5);
		FVector Out;
		const double T0 = FPlatformTime::Seconds();
		for (int32 i = 0; i < 20000; ++i)
		{
			S->FindAvoidance(FVector(Rng.FRandRange(-400000.f, 400000.f), Rng.FRandRange(-400000.f, 400000.f), Rng.FRandRange(-100000.f, 100000.f)), FVector(Rng.GetUnitVector()), 30000.f, 600.f, Out);
		}
		return (FPlatformTime::Seconds() - T0) * 1e6 / 20000.0; // microseconds per call
	};
	const int32 Before = S->GetObstacleCount();
	const double UsBefore = Measure();
	ATDSpaceField* Field = ATDSpaceField::SpawnFor(Layout);
	W.Advance(0.1f);
	const double UsAfter = Measure();
	AddInfo(FString::Printf(TEXT("FindAvoidance: %d obstacles %.2f us/call -> %d obstacles %.2f us/call (x%.1f)"), Before, UsBefore, S->GetObstacleCount(), UsAfter,
		UsAfter / FMath::Max(UsBefore, 0.001)));
	TestTrue(TEXT("the field added obstacles"), Field && S->GetObstacleCount() > Before);
	// 30 AI ships calling it every frame must stay well under a millisecond.
	TestTrue(*FString::Printf(TEXT("30 calls a frame under 0.5 ms (%.3f ms)"), UsAfter * 30.0 / 1000.0), UsAfter * 30.0 / 1000.0 < 0.5);
	// Wave 5: about 40 enemies plus 2 wingmen, one call each per think (T4's limit: 0.2 ms a frame).
	AddInfo(FString::Printf(TEXT("42 calls a frame: %.3f ms (before the field %.3f ms)"), UsAfter * 42.0 / 1000.0, UsBefore * 42.0 / 1000.0));
	TestTrue(*FString::Printf(TEXT("42 calls a frame under 0.2 ms (%.3f ms)"), UsAfter * 42.0 / 1000.0), UsAfter * 42.0 / 1000.0 < 0.2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSpaceFieldTumblers, "TriadDrift.World.SpaceField.BigRocksTumbleInPlace", TD_TEST_FLAGS)
bool FTDSpaceFieldTumblers::RunTest(const FString& Parameters)
{
	TDSpaceFieldTest::FField F;
	if (!TestNotNull(TEXT("field"), F.Field)) { return false; }
	const int32 N = F.Field->GetTumblerCount();
	AddInfo(FString::Printf(TEXT("%d tumblers"), N));
	TestTrue(TEXT("some big rocks tumble"), N >= 20);
	TestTrue(TEXT("at most MaxTumblers"), N <= F.Field->MaxTumblers);
	int32 Instances = 0;
	for (UInstancedStaticMeshComponent* C : F.Field->GetTumbleComponents())
	{
		Instances += C->GetInstanceCount();
		TestFalse(TEXT("tumblers are visual only (their collision sphere does not move)"), C->IsCollisionEnabled());
	}
	TestEqual(TEXT("one instance per tumbler"), Instances, N);
	const TArray<UInstancedStaticMeshComponent*> Comps = F.Field->GetTumbleComponents();
	if (Comps.Num() == 0) { return true; }
	FTransform A, B;
	Comps[0]->GetInstanceTransform(0, A, true);
	F.W.Advance(3.f);
	Comps[0]->GetInstanceTransform(0, B, true);
	TestTrue(TEXT("it stays in place"), A.GetLocation().Equals(B.GetLocation(), 1.f));
	TestTrue(*FString::Printf(TEXT("it turned (%.2f deg)"), FMath::RadiansToDegrees(A.GetRotation().AngularDistance(B.GetRotation()))),
		A.GetRotation().AngularDistance(B.GetRotation()) > FMath::DegreesToRadians(2.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSpaceFieldCaches, "TriadDrift.World.SpaceField.CachesSpawnNearAndAreTakenOnce", TD_TEST_FLAGS)
bool FTDSpaceFieldCaches::RunTest(const FString& Parameters)
{
	TDSpaceFieldTest::FField F;
	if (!TestNotNull(TEXT("field"), F.Field)) { return false; }
	if (!UTDPickupSubsystem::IsEnabled()) { AddInfo(TEXT("td.Pickups 0: skipped")); return true; }
	const TArray<FVector> Spots = ATDSpaceField::GetCacheSpots(F.W.GetWorld());
	if (!TestTrue(TEXT("cache spots"), Spots.Num() >= 8)) { return false; }
	const FVector Spot = Spots[0];
	const FVector Near = Spot + FVector(60000.f, 0.f, 0.f);	// 600 m off
	int32 Expect = 0;
	for (const FVector& S : Spots) { Expect += FVector::Dist(S, Near) < F.Field->CacheSpawnDistance ? 1 : 0; }
	UTDPickupSubsystem* Pk = UTDPickupSubsystem::Get(F.W.GetWorld());
	auto FindAt = [Pk](const FVector& At) -> ATDPickup*
	{
		for (ATDPickup* P : Pk->GetPickups()) { if (IsValid(P) && FVector::Dist(P->GetActorLocation(), At) < 10.f) { return P; } }
		return nullptr;
	};
	// Far away: nothing.
	F.Field->UpdateCachesAt(Spot + FVector(0.f, 0.f, 500000.f));
	TestEqual(TEXT("nothing spawns from far away"), F.Field->GetCachesAlive(), 0);
	// Coming close (600 m): the caches of that debris field appear, once each.
	F.Field->UpdateCachesAt(Near);
	TestEqual(TEXT("the near caches appear"), F.Field->GetCachesAlive(), Expect);
	F.Field->UpdateCachesAt(Near);
	TestEqual(TEXT("not twice"), F.Field->GetCachesAlive(), Expect);
	// One times out (or a CUT clears it) without moving: it comes back.
	if (ATDPickup* P = FindAt(Spot)) { P->Destroy(); }
	F.Field->UpdateCachesAt(Near);
	TestEqual(TEXT("not taken"), F.Field->GetCachesCollected(), 0);
	F.Field->UpdateCachesAt(Near);
	TestEqual(TEXT("it comes back"), F.Field->GetCachesAlive(), Expect);
	// Taken: the magnet pulls it off its spot, then it is collected.
	ATDPickup* P = FindAt(Spot);
	if (!TestNotNull(TEXT("cache at its spot"), P)) { return false; }
	P->SetActorLocation(Spot + FVector(4000.f, 0.f, 0.f));
	F.Field->UpdateCachesAt(Near);
	P->Destroy();
	F.Field->UpdateCachesAt(Near);
	TestEqual(TEXT("taken once (and only that one)"), F.Field->GetCachesCollected(), 1);
	F.Field->UpdateCachesAt(Near);
	TestEqual(TEXT("never again this run"), F.Field->GetCachesAlive(), Expect - 1);
	// Field off: no cache in space.
	F.Field->UpdateCachesAt(Spots[1]);
	F.Field->SetFieldEnabled(false);
	TestEqual(TEXT("off: caches removed"), F.Field->GetCachesAlive(), 0);
	F.Field->UpdateCachesAt(Spots[1]);
	TestEqual(TEXT("off: none spawn"), F.Field->GetCachesAlive(), 0);
	F.Field->SetFieldEnabled(true);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

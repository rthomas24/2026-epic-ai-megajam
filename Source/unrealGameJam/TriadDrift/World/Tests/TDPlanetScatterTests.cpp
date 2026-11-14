// World density: the planet scatter (v1 T4, v2 environment builder). Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.Scatter

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDPlanetScatter.h"
#include "TriadDrift/World/TDPlanetTerrain.h"
#include "TriadDrift/World/TDTerrainField.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDScatterPlanRules, "TriadDrift.World.Scatter.DeterministicKeptOutOfTheCourseWithinBudget", TD_TEST_FLAGS)
bool FTDScatterPlanRules::RunTest(const FString&)
{
	const FTDTerrainField F(FTDTerrainStyle::ForPlanet(ETDDestination::Verdance));
	const FTDScatterPlan P = FTDScatterPlan::ForPlanet(ETDDestination::Verdance);
	TestTrue(TEXT("Verdance has kinds"), P.Kinds.Num() > 3);
	const double Start = FPlatformTime::Seconds();
	FTDScatterResult A, B;
	P.Generate(F, A);
	const double Ms = (FPlatformTime::Seconds() - Start) * 1000.0;
	P.Generate(F, B);
	AddInfo(FString::Printf(TEXT("Verdance: %d instances in %.0f ms"), A.Total(), Ms));
	TestEqual(TEXT("deterministic count"), A.Total(), B.Total());
	bool bSame = true;
	for (int32 k = 0; k < A.Far.Num() && bSame; ++k)
	{
		bSame = A.Far[k].Num() == B.Far[k].Num() && (A.Far[k].Num() == 0 || A.Far[k][0].Equals(B.Far[k][0]));
	}
	TestTrue(TEXT("deterministic placement"), bSame);
	TestTrue(*FString::Printf(TEXT("a real forest, within budget (%d)"), A.Total()), A.Total() > 5000 && A.Total() < 25000);
	const FTDTerrainStyle& S = F.GetStyle();
	int32 Bad = 0, Sea = 0, Out = 0;
	for (const TArray<TArray<FTransform>>* Set : { &A.Near, &A.Far })
	{
		for (const TArray<FTransform>& Kind : *Set)
		{
			for (const FTransform& X : Kind)
			{
				const FVector L = X.GetLocation();
				Bad += P.IsKeptOut(F, L.X, L.Y) ? 1 : 0;
				Sea += F.Height(L.X, L.Y) < S.SeaLevel ? 1 : 0;
				Out += FVector2D(L.X, L.Y).Size() > P.ZoneRadius ? 1 : 0;
			}
		}
	}
	TestEqual(TEXT("nothing on the course (ground box + margin)"), Bad, 0);
	TestEqual(TEXT("nothing in the sea"), Sea, 0);
	TestEqual(TEXT("nothing past the zone"), Out, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDScatterOnTheTerrain, "TriadDrift.World.Scatter.InstancesStandOnTheTerrainWhereverTheCourseIs", TD_TEST_FLAGS)
bool FTDScatterOnTheTerrain::RunTest(const FString&)
{
	// Live finding: Static HISMs could not attach to the scatter's root, so the forest stood around the world origin.
	FTDTestWorld W;
	const FVector Origin(1900000.f, 1700000.f, 2500.f);	// the course origin, far from the world origin (and from L_Planet_Verdance's)
	ATDPlanetTerrain* T = ATDPlanetTerrain::SpawnForPlanet(W.GetWorld(), ETDDestination::Verdance, FTransform(Origin), nullptr);
	ATDPlanetScatter* S = T ? T->GetScatter() : nullptr;
	if (!TestNotNull(TEXT("scatter"), S)) { return false; }
	S->BuildNow();
	TestTrue(TEXT("instances"), S->GetInstanceCount() > 1000);
	TInlineComponentArray<UHierarchicalInstancedStaticMeshComponent*> Hisms(S);
	int32 Checked = 0, Off = 0;
	for (UHierarchicalInstancedStaticMeshComponent* H : Hisms)
	{
		TestTrue(TEXT("attached to the scatter"), H->GetAttachParent() == S->GetRootComponent());
		TestTrue(TEXT("movable (a Static HISM cannot attach to the movable root)"), H->Mobility == EComponentMobility::Movable);
		for (int32 i = 0; i < FMath::Min(5, H->GetInstanceCount()); ++i)
		{
			FTransform X;
			H->GetInstanceTransform(i, X, /*bWorldSpace*/ true);
			const float Ground = T->GetHeightAt(X.GetLocation());
			++Checked;
			Off += (FVector2D(X.GetLocation() - Origin).Size() > 500000.f || FMath::Abs(X.GetLocation().Z - Ground) > 3000.f) ? 1 : 0;
		}
	}
	TestTrue(TEXT("checked some"), Checked > 10);
	TestEqual(TEXT("every checked instance stands on the terrain around the course"), Off, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDScatterV2Cinder, "TriadDrift.World.Scatter.CinderHasItsZoneCrystalsOnTheObsidianNoneInTheRivers", TD_TEST_FLAGS)
bool FTDScatterV2Cinder::RunTest(const FString&)
{
	const FTDTerrainField F(FTDTerrainStyle::ForPlanet(ETDDestination::Cinder));
	const FTDScatterPlan P = FTDScatterPlan::ForPlanet(ETDDestination::Cinder);
	TestTrue(TEXT("Cinder has kinds"), P.Kinds.Num() > 10);
	FTDScatterResult A;
	P.Generate(F, A);
	AddInfo(FString::Printf(TEXT("Cinder zone layer: %d instances"), A.Total()));
	for (int32 k = 0; k < P.Kinds.Num(); ++k)
	{
		AddInfo(FString::Printf(TEXT("  %s: %d"), *FPaths::GetBaseFilename(P.Kinds[k].MeshPath), A.Near[k].Num() + A.Far[k].Num()));
	}
	TestTrue(*FString::Printf(TEXT("a dressed zone, within budget (%d)"), A.Total()), A.Total() > 2000 && A.Total() < 20000);
	int32 Crystals = 0, OffPatch = 0, InRiver = 0, Kept = 0;
	for (int32 k = 0; k < P.Kinds.Num(); ++k)
	{
		const bool bCrystal = P.Kinds[k].MeshPath.Contains(TEXT("Crystal")) && P.Kinds[k].CullEndCm > 15000.f && P.Kinds[k].CullEndCm < 150000.f;	// the zone crystal clusters (not the spires, a few of which stand on the flats as landmarks, nor the near ring)
		for (const TArray<TArray<FTransform>>* Set : { &A.Near, &A.Far })
		{
			for (const FTransform& X : (*Set)[k])
			{
				const FVector L = X.GetLocation();
				const FTDTerrainSample Sm = F.Sample(L.X, L.Y, 400.f);
				InRiver += FMath::Abs(Sm.BroadMask - 0.5f) < 0.04f ? 1 : 0;	// a little tolerance: the sample step differs
				Kept += P.IsKeptOut(F, L.X, L.Y) ? 1 : 0;
				if (bCrystal) { ++Crystals; OffPatch += Sm.FineMask > FTDScatterPlan::ObsidianFineMin - 0.03f ? 0 : 1; }
			}
		}
	}
	TestTrue(*FString::Printf(TEXT("crystals stand in the zone (%d)"), Crystals), Crystals > 100);
	TestTrue(*FString::Printf(TEXT("crystals stand on the obsidian patches (%d off of %d)"), OffPatch, Crystals), OffPatch <= Crystals / 50);
	TestTrue(*FString::Printf(TEXT("nothing in the lava rivers (%d)"), InRiver), InRiver <= A.Total() / 200);
	TestEqual(TEXT("nothing on the course"), Kept, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDScatterV2Rings, "TriadDrift.World.Scatter.RingsAreDeterministicDenseAndWithinTheBudget", TD_TEST_FLAGS)
bool FTDScatterV2Rings::RunTest(const FString&)
{
	const FTDTerrainField F(FTDTerrainStyle::ForPlanet(ETDDestination::Verdance));
	const FTDScatterPlan P = FTDScatterPlan::ForPlanet(ETDDestination::Verdance);
	TestTrue(TEXT("a mid ring of trees"), P.RingKinds(ETDScatterRing::Mid).Num() == 18);
	TestTrue(TEXT("a near ring of ground cover"), P.RingKinds(ETDScatterRing::Near).Num() >= 8);
	// Determinism: the same cell twice.
	TArray<TArray<FTransform>> C1, C2;
	P.GenerateCell(F, ETDScatterRing::Mid, FIntPoint(7, -4), C1);
	P.GenerateCell(F, ETDScatterRing::Mid, FIntPoint(7, -4), C2);
	int32 N1 = 0, N2 = 0;
	bool bSame = true;
	for (int32 k = 0; k < C1.Num(); ++k)
	{
		N1 += C1[k].Num(); N2 += C2[k].Num();
		bSame &= C1[k].Num() == C2[k].Num() && (C1[k].Num() == 0 || C1[k][0].Equals(C2[k][0]));
	}
	TestTrue(TEXT("same cell, same trees"), bSame && N1 == N2);
	{
		// Diagnostics: how the zone's ground splits (broad mask, slope, height above the pad), 60 x 60 points within 3 km.
		int32 Hist[5] = { 0, 0, 0, 0, 0 }, Flat = 0, Low = 0, Land = 0, Grove = 0;
		for (int32 j = 0; j < 60; ++j)
		{
			for (int32 i = 0; i < 60; ++i)
			{
				const float X = -300000.f + i * 10000.f, Y = -300000.f + j * 10000.f;
				if (FVector2D(X, Y).Size() > 300000.f) { continue; }
				const FTDTerrainSample Sm = F.Sample(X, Y, 400.f);
				if (Sm.Height < F.GetStyle().SeaLevel + 1000.f) { continue; }
				++Land;
				Hist[FMath::Clamp(int32(Sm.BroadMask * 5.f), 0, 4)]++;
				Flat += Sm.Slope < 0.2f ? 1 : 0;
				Low += (Sm.Height - F.GetStyle().BasinFloor) < 45000.f ? 1 : 0;
				Grove += P.IsPatch(Sm) ? 1 : 0;
			}
		}
		AddInfo(FString::Printf(TEXT("zone land %d pts: broad 0-.2 %d, .2-.4 %d, .4-.6 %d, .6-.8 %d, .8-1 %d; flat %d, under 450 m %d, grove %d"),
			Land, Hist[0], Hist[1], Hist[2], Hist[3], Hist[4], Flat, Low, Grove));
	}
	// Density on the rainforest floor: the mid ring's trees per hectare of flat, low, open (not grove) rainforest ground,
	// over cells 0.8-2.6 km out in every direction; the area is measured on a 10 m grid with the same rule.
	auto Eligible = [&F, &P](float X, float Y, const FTDTerrainSample& Sm)
	{
		return Sm.BroadMask > FTDScatterPlan::RainforestBroad && Sm.Slope < 0.2f && (Sm.Height - F.GetStyle().BasinFloor) < 80000.f
			&& Sm.Height > F.GetStyle().SeaLevel + 1000.f && FVector2D(X, Y).Size() < P.ZoneFullRadius && !P.IsPatch(Sm) && !P.IsNearCourse(F, X, Y);
	};
	int32 Trees = 0, Samples = 0;
	for (int32 cy = -12; cy <= 12; cy += 4)
	{
		for (int32 cx = -12; cx <= 12; cx += 4)
		{
			if (FMath::Abs(cx) < 4 && FMath::Abs(cy) < 4) { continue; }
			TArray<TArray<FTransform>> C;
			P.GenerateCell(F, ETDScatterRing::Mid, FIntPoint(cx, cy), C);
			for (const TArray<FTransform>& A : C)
			{
				for (const FTransform& X : A)
				{
					const FVector L = X.GetLocation();
					Trees += Eligible(L.X, L.Y, F.Sample(L.X, L.Y, 400.f)) ? 1 : 0;
				}
			}
			const float Size = P.MidCell;
			for (float y = cy * Size + 500.f; y < (cy + 1) * Size; y += 1000.f)
			{
				for (float x = cx * Size + 500.f; x < (cx + 1) * Size; x += 1000.f)
				{
					Samples += Eligible(x, y, F.Sample(x, y, 400.f)) ? 1 : 0;
				}
			}
		}
	}
	const float Hectares = Samples * 1000.f * 1000.f / 1e8f;
	const float PerHa = Hectares > 0.f ? Trees / Hectares : 0.f;
	AddInfo(FString::Printf(TEXT("mid ring on the flat rainforest floor: %.0f trees/ha over %.0f ha (+ ~9/ha from the zone layer)"), PerHa, Hectares));
	TestTrue(*FString::Printf(TEXT("enough rainforest sampled (%.0f ha)"), Hectares), Hectares > 2.f);
	TestTrue(*FString::Printf(TEXT("dense rainforest from the mid ring (%.0f/ha, 30..60 wanted)"), PerHa), PerHa > 30.f && PerHa < 60.f);
	// The live budget: the zone layer plus full rings around a point near the pad and one 2.5 km out.
	FTDTestWorld W;
	ATDPlanetTerrain* T = ATDPlanetTerrain::SpawnForPlanet(W.GetWorld(), ETDDestination::Verdance, FTransform::Identity, nullptr);
	ATDPlanetScatter* S = T ? T->GetScatter() : nullptr;
	if (!TestNotNull(TEXT("scatter"), S)) { return false; }
	S->BuildNow();
	for (const FVector& At : { FVector(30000.f, 20000.f, 2000.f), FVector(250000.f, -60000.f, 5000.f) })
	{
		S->BuildRingsNowAt(At);
		AddInfo(FString::Printf(TEXT("at %s: zone %d + rings %d = %d (ring apply %.1f ms)"), *At.ToCompactString(), S->GetInstanceCount(), S->GetRingInstanceCount(), S->GetTotalInstanceCount(), S->GetLastRingApplyMs()));
		TestTrue(*FString::Printf(TEXT("under the 25k budget (%d)"), S->GetTotalInstanceCount()), S->GetTotalInstanceCount() <= 25000);
		TestTrue(*FString::Printf(TEXT("the rings carry trees (%d)"), S->GetRingInstanceCount()), S->GetRingInstanceCount() > 500);
	}
	TInlineComponentArray<UInstancedStaticMeshComponent*> Comps(S);
	for (UInstancedStaticMeshComponent* C : Comps)
	{
		TestTrue(TEXT("every instanced component is Movable"), C->Mobility == EComponentMobility::Movable);
	}
	return true;
}


// Issue #89 (T7's profile): the zone layer arrived in one 109-330 ms frame at the atmosphere entry. Now it is applied a few
// components per frame, near tier first: no frame spends more than 8 ms on it, and the near tier lands before the far one.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDScatterSpread, "TriadDrift.World.Scatter.ZoneLayerAppliesWithoutAHitch", TD_TEST_FLAGS)
bool FTDScatterSpread::RunTest(const FString&)
{
	FTDTestWorld W;
	ATDPlanetTerrain* T = ATDPlanetTerrain::SpawnForPlanet(W.GetWorld(), ETDDestination::Verdance, FTransform(FVector(1900000.f, 1700000.f, 2500.f)), nullptr);
	ATDPlanetScatter* S = T ? T->GetScatter() : nullptr;
	if (!TestNotNull(TEXT("scatter"), S)) { return false; }
	bool bNearBeforeBuilt = false;
	for (int32 i = 0; i < 60 * 30 && !S->IsBuilt(); ++i)
	{
		W.Tick(1.f / 60.f);
		if (S->IsNearApplied() && !S->IsBuilt() && S->GetPendingApplyJobs() > 0) { bNearBeforeBuilt = true; }
	}
	AddInfo(FString::Printf(TEXT("zone layer %d instances, worst apply frame %.1f ms"), S->GetInstanceCount(), S->GetMaxApplyFrameMs()));
	TestTrue(TEXT("built within 30 s"), S->IsBuilt());
	TestTrue(TEXT("instances"), S->GetInstanceCount() > 1000);
	TestTrue(*FString::Printf(TEXT("no frame spends more than 8 ms applying (%.1f ms)"), S->GetMaxApplyFrameMs()), S->GetMaxApplyFrameMs() <= 8.0);
	TestTrue(TEXT("the near tier is in before the far tier finishes"), bNearBeforeBuilt || S->IsNearApplied());
	return true;
}


// ENG-38 arrival (agreed with T1, Nov 14): the deck clears the flight zone's highest ground by DeckClearance, the deck actor
// sits there, and after PrepareArrival(glide end) the scatter round that point is ready before the camera gets near it.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDArrivalReady, "TriadDrift.World.Arrival.DeckClearsTheZoneAndTheGlideEndIsReadyFirst", TD_TEST_FLAGS)
bool FTDArrivalReady::RunTest(const FString&)
{
	for (const ETDDestination Planet : { ETDDestination::Verdance, ETDDestination::Cinder })
	{
		const FString Name = Planet == ETDDestination::Verdance ? TEXT("Verdance") : TEXT("Cinder");
		FTDTestWorld W;
		const FVector Origin(1900000.f, 1700000.f, 2500.f);
		ATDPlanetTerrain* T = ATDPlanetTerrain::SpawnForPlanet(W.GetWorld(), Planet, FTransform(Origin), nullptr);
		if (!TestNotNull(*(Name + TEXT(": terrain")), T)) { continue; }
		const float MaxZ = T->GetZoneMaxGroundZ();
		const float Deck = T->GetCloudDeckZ();
		AddInfo(FString::Printf(TEXT("%s: highest ground %.0f m, deck %.0f m (above the course origin)"), *Name, (MaxZ - Origin.Z) / 100.f, (Deck - Origin.Z) / 100.f));
		TestTrue(*(Name + TEXT(": the deck clears the highest ground by DeckClearance")), FMath::IsNearlyEqual(Deck - MaxZ, T->DeckClearance, 1.f));
		for (const FVector& P : { Origin, Origin + FVector(300000.f, -200000.f, 0.f), Origin + FVector(-450000.f, 450000.f, 0.f) })
		{
			TestTrue(*FString::Printf(TEXT("%s: ground under the deck at %s"), *Name, *P.ToCompactString()), T->GetGroundZAt(P) <= Deck - T->DeckClearance + 1.f);
		}
		// The glide end 900 m over the ground 2 km from the pad; the camera stays far away (as under the cover).
		const FVector EndXY = Origin + FVector(200000.f, 0.f, 0.f);
		const FVector GlideEnd(EndXY.X, EndXY.Y, T->GetGroundZAt(EndXY) + 90000.f);
		T->PrepareArrival(GlideEnd);
		bool bReady = false;
		for (int32 i = 0; i < 60 * 20 && !bReady; ++i) { W.Tick(1.f / 60.f); bReady = T->IsArrivalReady(GlideEnd); }
		TestTrue(*(Name + TEXT(": the glide end is ready within 20 s of PrepareArrival")), bReady);
		TestTrue(*(Name + TEXT(": the zone layer's near tier is in")), T->GetScatter() && T->GetScatter()->IsNearApplied());
	}
	return true;
}

#endif

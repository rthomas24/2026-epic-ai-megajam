// World density v1: the planet scatter. Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.Scatter

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
	TestEqual(TEXT("Cinder has no list yet (v1)"), FTDScatterPlan::ForPlanet(ETDDestination::Cinder).Kinds.Num(), 0);
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

#endif

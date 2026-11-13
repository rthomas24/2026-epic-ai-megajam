// World density v1: the planet scatter. Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.Scatter

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDPlanetScatter.h"
#include "TriadDrift/World/TDPlanetTerrain.h"
#include "TriadDrift/World/TDTerrainField.h"
#include "TriadDrift/Tests/TDTestWorld.h"

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

#endif

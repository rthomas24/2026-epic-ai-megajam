// Triad Drift - the space field takes over the galaxy layout's rocks (Teammate 6). Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.SpaceField

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/World/SpaceField/TDSpaceField.h"
#include "TriadDrift/World/TDGalaxyLayout.h"
#include "Components/InstancedStaticMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDSpaceFieldAdopt, "TriadDrift.World.SpaceField.Break.AdoptsTheLayoutRocksExactlyAndOffRestoresThem", TD_TEST_FLAGS)
bool FTDSpaceFieldAdopt::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDGalaxyLayout* Layout = ATDGalaxyLayout::SpawnDefaultGalaxy(W.GetWorld());
	W.Advance(0.1f);
	// The layout's rocks as they are today.
	TArray<FTransform> Before;
	TArray<UInstancedStaticMeshComponent*> LayoutComps;
	for (UInstancedStaticMeshComponent* C : Layout->GetAsteroidLooks()) { LayoutComps.Add(C); }
	for (UInstancedStaticMeshComponent* C : Layout->GetDebrisLooks()) { LayoutComps.Add(C); }
	for (UInstancedStaticMeshComponent* C : Layout->GetDressingComponents()) { if (C->GetName().StartsWith(TEXT("LaneRock"))) { LayoutComps.Add(C); } }
	for (UInstancedStaticMeshComponent* C : LayoutComps)
	{
		for (int32 i = 0; i < C->GetInstanceCount(); ++i) { FTransform T; C->GetInstanceTransform(i, T, true); Before.Add(T); }
	}
	if (!TestTrue(TEXT("the layout has rocks"), Before.Num() > 100)) { return false; }

	ATDSpaceField* Field = ATDSpaceField::SpawnFor(Layout);
	W.Advance(0.1f);
	if (!TestNotNull(TEXT("field"), Field)) { return false; }
	int32 Adopted = 0, Cover = 0;
	TSet<FIntVector> At;
	for (int32 i = 0; i < Field->GetPieces().Num(); ++i)
	{
		const FTDSpaceFieldPiece& P = Field->GetPieces()[i];
		if (!P.bAdopted) { continue; }
		++Adopted;
		At.Add(FIntVector(FMath::RoundToInt(P.Location.X), FMath::RoundToInt(P.Location.Y), FMath::RoundToInt(P.Location.Z)));
		Cover += Field->GetPieceHealth(i) >= Field->CoverHealthScale * Field->MediumHealth - 1.f && P.bCollides ? 1 : 0;
	}
	TestEqual(TEXT("every layout rock and piece is taken over"), Adopted, Before.Num());
	int32 Missing = 0;
	for (const FTransform& T : Before)
	{
		const FVector L = T.GetLocation();
		Missing += At.Contains(FIntVector(FMath::RoundToInt(L.X), FMath::RoundToInt(L.Y), FMath::RoundToInt(L.Z))) ? 0 : 1;
	}
	TestEqual(TEXT("each one in exactly the same place"), Missing, 0);
	AddInfo(FString::Printf(TEXT("%d adopted, %d cover rocks with x%.0f health"), Adopted, Cover, Field->CoverHealthScale));
	TestTrue(TEXT("the combat zone keeps cover rocks that take many hits"), Cover >= 10);

	// The layout's own components: hidden and not solid while the field owns the rocks.
	bool bAnyVisible = false;
	for (UInstancedStaticMeshComponent* C : LayoutComps) { bAnyVisible |= C->IsVisible(); }
	TestFalse(TEXT("the layout's rock looks are hidden"), bAnyVisible);
	TestFalse(TEXT("the layout's rock collision is off"), Layout->GetAsteroidCollision()->IsCollisionEnabled());

	// td.SpaceField 0: the layout exactly as today (untouched instances, visible, solid).
	Field->SetFieldEnabled(false);
	bool bAllVisible = true;
	int32 Same = 0, N = 0;
	for (UInstancedStaticMeshComponent* C : LayoutComps)
	{
		bAllVisible &= C->IsVisible();
		for (int32 i = 0; i < C->GetInstanceCount(); ++i) { FTransform T; C->GetInstanceTransform(i, T, true); Same += T.Equals(Before[N], 0.01f) ? 1 : 0; ++N; }
	}
	TestTrue(TEXT("off: the layout's rocks show again"), bAllVisible);
	TestTrue(TEXT("off: the layout's rock collision is back"), Layout->GetAsteroidCollision()->IsCollisionEnabled());
	TestEqual(TEXT("off: every layout rock exactly as before"), Same, Before.Num());
	Field->SetFieldEnabled(true);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

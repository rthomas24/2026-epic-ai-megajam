// ENG-34: the planet height field. Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.Terrain

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDTerrainField.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/World/TDPlanetTerrain.h"
#include "Engine/World.h"
#include "PhysicsEngine/BodySetup.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTerrainDeterministic, "TriadDrift.World.Terrain.SeededAndDeterministic", TD_TEST_FLAGS)
bool FTDTerrainDeterministic::RunTest(const FString&)
{
	const FTDTerrainField A(FTDTerrainStyle::ForPlanet(ETDDestination::Verdance));
	const FTDTerrainField B(FTDTerrainStyle::ForPlanet(ETDDestination::Verdance));
	const FTDTerrainField C(FTDTerrainStyle::ForPlanet(ETDDestination::Cinder));
	int32 Same = 0, Differs = 0;
	for (int32 i = 0; i < 200; ++i)
	{
		// Outside the shared flat course basin (identical on every planet by design).
		const float X = 450000.f + i * 1501.f, Y = -700000.f + i * 7003.f;
		Same += A.Height(X, Y) == B.Height(X, Y) ? 1 : 0;
		Differs += FMath::Abs(A.Height(X, Y) - C.Height(X, Y)) > 100.f ? 1 : 0;
	}
	TestEqual(TEXT("same planet, same terrain every run"), Same, 200);
	TestTrue(*FString::Printf(TEXT("the two planets differ (%d of 200)"), Differs), Differs > 150);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTerrainBasin, "TriadDrift.World.Terrain.CourseBasinStaysFlat", TD_TEST_FLAGS)
bool FTDTerrainBasin::RunTest(const FString&)
{
	for (const ETDDestination P : { ETDDestination::Verdance, ETDDestination::Cinder })
	{
		const FTDTerrainField F(FTDTerrainStyle::ForPlanet(P));
		const FVector2D E = F.GetStyle().BasinHalfExtent;
		float MaxInside = -1e9f;
		for (float X = -E.X; X <= E.X; X += E.X / 20.f)
		{
			for (float Y = -E.Y; Y <= E.Y; Y += E.Y / 20.f) { MaxInside = FMath::Max(MaxInside, F.Height(X, Y)); }
		}
		TestTrue(*FString::Printf(TEXT("planet %d: the course basin sits under the course slab (max %.0f cm)"), (int32)P, MaxInside), MaxInside <= 0.f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTerrainMountains, "TriadDrift.World.Terrain.MountainsInEveryDirection", TD_TEST_FLAGS)
bool FTDTerrainMountains::RunTest(const FString&)
{
	for (const ETDDestination P : { ETDDestination::Verdance, ETDDestination::Cinder })
	{
		const FTDTerrainField F(FTDTerrainStyle::ForPlanet(P));
		// From the pad, every 30-degree bearing meets real relief (>= 300 m) within the walkable core.
		for (int32 Deg = 0; Deg < 360; Deg += 30)
		{
			const FVector2D Dir(FMath::Cos(FMath::DegreesToRadians(Deg)), FMath::Sin(FMath::DegreesToRadians(Deg)));
			float Peak = -1e9f;
			for (float R = 0.f; R <= F.GetStyle().CoreHalfSize; R += 5000.f) { Peak = FMath::Max(Peak, F.Height(Dir.X * R, Dir.Y * R)); }
			TestTrue(*FString::Printf(TEXT("planet %d bearing %d: relief %.0f m"), (int32)P, Deg, Peak / 100.f), Peak >= 30000.f);
		}
		// The horizon closes: the far ring stands higher than the core edge on average.
		float Core = 0.f, Far = 0.f;
		for (int32 Deg = 0; Deg < 360; Deg += 15)
		{
			const FVector2D Dir(FMath::Cos(FMath::DegreesToRadians(Deg)), FMath::Sin(FMath::DegreesToRadians(Deg)));
			Core += F.Height(Dir.X * F.GetStyle().CoreHalfSize, Dir.Y * F.GetStyle().CoreHalfSize);
			Far += F.Height(Dir.X * F.GetStyle().FarHalfSize * 0.9f, Dir.Y * F.GetStyle().FarHalfSize * 0.9f);
		}
		TestTrue(TEXT("distant ranges rise above the core"), Far > Core);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTerrainWalkable, "TriadDrift.World.Terrain.MostOfTheCoreIsWalkable", TD_TEST_FLAGS)
bool FTDTerrainWalkable::RunTest(const FString&)
{
	for (const ETDDestination P : { ETDDestination::Verdance, ETDDestination::Cinder })
	{
		const FTDTerrainField F(FTDTerrainStyle::ForPlanet(P));
		int32 Walkable = 0, N = 0;
		const float H = F.GetStyle().CoreHalfSize;
		for (float X = -H; X <= H; X += H / 25.f)
		{
			for (float Y = -H; Y <= H; Y += H / 25.f)
			{
				const FTDTerrainSample S = F.Sample(X, Y);
				Walkable += S.Slope < 0.3f ? 1 : 0;	// about 45 degrees
				++N;
				TestTrue(TEXT("masks in 0..1"), S.Height01 >= 0.f && S.Height01 <= 1.f && S.BroadMask >= 0.f && S.BroadMask <= 1.f);
			}
		}
		TestTrue(*FString::Printf(TEXT("planet %d: %d%% of the core is walkable"), (int32)P, 100 * Walkable / N), Walkable * 100 / N >= 60);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTerrainActor, "TriadDrift.World.Terrain.WorldBuildsAroundTheCourse", TD_TEST_FLAGS)
bool FTDTerrainActor::RunTest(const FString&)
{
	for (const ETDDestination P : { ETDDestination::Verdance, ETDDestination::Cinder })
	{
		FTDTestWorld W;
		const FVector Origin(12345.f, -6789.f, 500.f);	// the course origin (pad), anywhere in the map
		ATDPlanetTerrain* T = ATDPlanetTerrain::SpawnForPlanet(W.GetWorld(), P, FTransform(Origin), nullptr);
		if (!TestNotNull(TEXT("terrain"), T)) { return false; }
		TestTrue(TEXT("tagged"), T->ActorHasTag(ATDPlanetTerrain::TerrainTag));
		TestEqual(TEXT("16 x 16 km core in 1 km chunks"), T->GetChunkCount(), 256);
		T->FlushAll();
		// The basin under the course is just under its floor; collision exists and matches the field in the mountains.
		TestTrue(TEXT("course basin just under the floor"), FMath::IsNearlyEqual(T->GetHeightAt(Origin), Origin.Z - 60.f, 1.f));
		// Incremental refinement finishes with nothing pending (collision cooks async meanwhile).
		for (int32 k = 0; k < 600 && T->GetPendingCount() > 0; ++k) { W.Tick(1.f / 30.f); }
		TestEqual(TEXT("refinement settles"), T->GetPendingCount(), 0);
		W.Advance(1.f);
		const FVector Far = Origin + FVector(600000.f, 300000.f, 0.f);
		const float Expected = T->GetHeightAt(Far);
		FHitResult Hit;
		FCollisionQueryParams Q(TEXT("TDTerrainTest"), /*bTraceComplex*/ true);
		const bool bHit = W.GetWorld()->LineTraceSingleByChannel(Hit, Far + FVector(0, 0, 400000.f), Far - FVector(0, 0, 400000.f), ECC_WorldStatic, Q);
		{
			FHitResult VHit;
			const bool bV = W.GetWorld()->LineTraceSingleByChannel(VHit, Far + FVector(0, 0, 400000.f), Far - FVector(0, 0, 400000.f), ECC_Visibility, Q);
			UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(T->GetRootComponent());
			FString Sweep;
			for (const float K : { 0.05f, 0.1f, 0.2f, 0.3f, 0.5f, 0.7f, 1.f })
			{
				const FVector SP = Origin + FVector(600000.f, 300000.f, 0.f) * K;
				FHitResult H2;
				const bool b2 = W.GetWorld()->LineTraceSingleByChannel(H2, SP + FVector(0, 0, 400000.f), SP - FVector(0, 0, 400000.f), ECC_WorldStatic, Q);
				Sweep += FString::Printf(TEXT(" %.2f:%d(z %.0f vs %.0f)"), K, b2, H2.ImpactPoint.Z, T->GetHeightAt(SP));
			}
			AddInfo(TEXT("diag3:") + Sweep);
			FString Row;
			for (int32 Cx = -8; Cx < 8; ++Cx)
			{
				const FVector RP = Origin + FVector((Cx + 0.5f) * 100000.f, 50000.f, 0.f);
				FHitResult H3;
				const bool b3 = W.GetWorld()->LineTraceSingleByChannel(H3, RP + FVector(0, 0, 400000.f), RP - FVector(0, 0, 400000.f), ECC_WorldStatic, Q);
				Row += FString::Printf(TEXT(" %d:%d/L%d"), Cx, b3, T->GetChunkLod(FIntPoint(Cx, 0)));
			}
			AddInfo(TEXT("diag4:") + Row);
			FHitResult OHit;
			const bool bO = W.GetWorld()->LineTraceSingleByChannel(OHit, Origin + FVector(0, 0, 100000.f), Origin - FVector(0, 0, 100000.f), ECC_WorldStatic, Q);
			const UBodySetup* BS = Root ? Root->GetBodySetup() : nullptr;
			AddInfo(FString::Printf(TEXT("diag2: origin hit %d (%s), body valid %d, trimeshes %d, cooked %d"), bO, *GetNameSafe(OHit.GetActor()),
				Root && Root->GetBodyInstance() && Root->GetBodyInstance()->IsValidBodyInstance() ? 1 : 0,
				BS ? BS->TriMeshGeometries.Num() : -1, BS && BS->bCreatedPhysicsMeshes ? 1 : 0));
			AddInfo(FString::Printf(TEXT("diag: static hit %d (%s), vis hit %d (%s), collision enabled %d, body %d, registered %d, objtype %d"),
				bHit, *GetNameSafe(Hit.GetActor()), bV, *GetNameSafe(VHit.GetActor()), Root ? (int32)Root->GetCollisionEnabled() : -1,
				Root && Root->GetBodySetup() ? 1 : 0, Root && Root->IsRegistered() ? 1 : 0, Root ? (int32)Root->GetCollisionObjectType() : -1));
		}
		TestTrue(TEXT("terrain collision under a mountain point"), bHit && Hit.GetActor() == T);
		if (bHit) { TestTrue(*FString::Printf(TEXT("collision at the field height (%.0f vs %.0f cm)"), Hit.ImpactPoint.Z, Expected), FMath::Abs(Hit.ImpactPoint.Z - Expected) < 2500.f); }
	}
	return true;
}

#endif

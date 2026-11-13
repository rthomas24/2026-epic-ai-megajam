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
		const FVector2D BC = F.GetStyle().BasinCentre;
		float MaxInside = -1e9f;
		for (float X = -E.X; X <= E.X; X += E.X / 20.f)
		{
			for (float Y = -E.Y; Y <= E.Y; Y += E.Y / 20.f) { MaxInside = FMath::Max(MaxInside, F.Height(BC.X + X, BC.Y + Y)); }
		}
		TestTrue(*FString::Printf(TEXT("planet %d: the course basin sits under the course slab (max %.0f cm)"), (int32)P, MaxInside), MaxInside <= 0.f);
		// Around the slab the ground never drops below its top (no raised-table edge): a ring 0-3 km beyond the basin.
		float MinRing = 1e9f;
		for (int32 Deg = 0; Deg < 360; Deg += 10)
		{
			const FVector2D Dir(FMath::Cos(FMath::DegreesToRadians(Deg)), FMath::Sin(FMath::DegreesToRadians(Deg)));
			// Along each bearing, from the slab edge out to 2.5 km beyond the basin box (measured from its nearest point).
			for (float R = 0.f; R < 250000.f; R += 20000.f)
			{
				const FVector2D Q = BC + FVector2D(FMath::Clamp(Dir.X * 1e7f, -(float)E.X, (float)E.X), FMath::Clamp(Dir.Y * 1e7f, -(float)E.Y, (float)E.Y)) + Dir * R;
				MinRing = FMath::Min(MinRing, F.Height(Q.X, Q.Y));
			}
		}
		TestTrue(*FString::Printf(TEXT("planet %d: no drop below the slab top around it (min %.0f cm)"), (int32)P, MinRing), MinRing >= F.GetStyle().BasinFloor - 1.f);
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
			// Out to just past the core: a sea bearing meets its far shore and the skyline there.
			for (float R = 0.f; R <= F.GetStyle().CoreHalfSize * 1.3f; R += 5000.f) { Peak = FMath::Max(Peak, F.Height(Dir.X * R, Dir.Y * R)); }
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
		TestTrue(TEXT("terrain collision under a mountain point"), bHit && Hit.GetActor() == T);
		if (bHit) { TestTrue(*FString::Printf(TEXT("collision at the field height (%.0f vs %.0f cm)"), Hit.ImpactPoint.Z, Expected), FMath::Abs(Hit.ImpactPoint.Z - Expected) < 2500.f); }
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTerrainHeroAndRegions, "TriadDrift.World.Terrain.HeroPeakAndNamedRegions", TD_TEST_FLAGS)
bool FTDTerrainHeroAndRegions::RunTest(const FString&)
{
	for (const ETDDestination P : { ETDDestination::Verdance, ETDDestination::Cinder })
	{
		const FTDTerrainField F(FTDTerrainStyle::ForPlanet(P));
		const FTDTerrainStyle& S = F.GetStyle();
		// The hero summit stands 1.2-2.5 km tall and is the highest point of the walkable core.
		float HeroTop = -1e9f;
		for (float DX = -S.HeroRadius * 0.4f; DX <= S.HeroRadius * 0.4f; DX += S.HeroRadius * 0.05f)
		{
			for (float DY = -S.HeroRadius * 0.4f; DY <= S.HeroRadius * 0.4f; DY += S.HeroRadius * 0.05f) { HeroTop = FMath::Max(HeroTop, F.Height(S.HeroCentre.X + DX, S.HeroCentre.Y + DY)); }
		}
		TestTrue(*FString::Printf(TEXT("planet %d hero peak %.0f m"), (int32)P, HeroTop / 100.f), HeroTop >= 120000.f && HeroTop <= 260000.f);
		float CoreTop = -1e9f;
		for (float X = -S.CoreHalfSize; X <= S.CoreHalfSize; X += 20000.f)
		{
			for (float Y = -S.CoreHalfSize; Y <= S.CoreHalfSize; Y += 20000.f)
			{
				if (FVector2D::Distance(FVector2D(X, Y), S.HeroCentre) > S.HeroRadius) { CoreTop = FMath::Max(CoreTop, F.Height(X, Y)); }
			}
		}
		TestTrue(*FString::Printf(TEXT("planet %d: the hero (%.0f m) is the tallest in the core (next %.0f m)"), (int32)P, HeroTop / 100.f, CoreTop / 100.f), HeroTop > CoreTop);
		// Visible from the course: nothing between the pad and the hero's foot rises into the line to its summit.
		bool bClear = true;
		const FVector Eye(0.f, 0.f, 200.f), Top(S.HeroCentre.X, S.HeroCentre.Y, HeroTop);
		const float Foot = FMath::Max(0.1f, 1.f - S.HeroRadius / FMath::Max(1.f, (float)S.HeroCentre.Size()));
		FString Where;
		for (float T = 0.05f; T < Foot; T += 0.01f)
		{
			const FVector Q = FMath::Lerp(Eye, Top, T);
			if (F.Height(Q.X, Q.Y) >= Q.Z && Where.IsEmpty())
			{
				Where = FString::Printf(TEXT("blocked at %.0f m out (ground %.0f m, line %.0f m, top %.0f m at %s)"), FVector2D(Q.X, Q.Y).Size() / 100.f,
					F.Height(Q.X, Q.Y) / 100.f, Q.Z / 100.f, HeroTop / 100.f, *FVector2D(Top.X, Top.Y).ToString());
			}
			bClear &= F.Height(Q.X, Q.Y) < Q.Z;
		}
		TestTrue(*FString::Printf(TEXT("planet %d: the hero peak is visible from the pad %s"), (int32)P, *Where), bClear);
		if (S.bVolcano)
		{
			const float Rim = F.Height(S.HeroCentre.X + S.CraterRadius * 1.3f, S.HeroCentre.Y);
			const float Floor = F.Height(S.HeroCentre.X, S.HeroCentre.Y);
			const float Ring = F.Height(S.HeroCentre.X + S.CraterRadius * 0.95f, S.HeroCentre.Y);
			TestTrue(*FString::Printf(TEXT("the volcano has a crater (floor %.0f m, rim %.0f m, outside %.0f m)"), Floor / 100.f, Ring / 100.f, Rim / 100.f), Floor < Ring - 5000.f);
		}
		// Named regions: each sector is weighted out in the world and absent in the course basin.
		for (const float B : { S.RiftBearing, S.MesaBearing, S.BasinBearing, S.PlateauBearing })
		{
			const FVector2D Dir(FMath::Cos(FMath::DegreesToRadians(B)), FMath::Sin(FMath::DegreesToRadians(B)));
			TestTrue(*FString::Printf(TEXT("planet %d region at %.0f deg exists"), (int32)P, B), F.RegionWeight(Dir.X * 650000.f, Dir.Y * 650000.f, B) > 0.9f);
			TestEqual(TEXT("no region inside the course basin"), F.RegionWeight(Dir.X * 100000.f, Dir.Y * 100000.f, B), 0.f);
		}
		TestTrue(TEXT("the sea basin dips below the sea level"), F.Height(F.SeaCentre().X, F.SeaCentre().Y) < S.SeaLevel);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTerrainMaterialData, "TriadDrift.World.Terrain.MaterialDataMatchesTheTerrain", TD_TEST_FLAGS)
bool FTDTerrainMaterialData::RunTest(const FString&)
{
	for (const ETDDestination P : { ETDDestination::Verdance, ETDDestination::Cinder })
	{
		const FTDTerrainField F(FTDTerrainStyle::ForPlanet(P));
		TestTrue(*FString::Printf(TEXT("planet %d relief %.0f m"), (int32)P, F.ReliefMeters()), F.ReliefMeters() > 2000.f && F.ReliefMeters() < 4000.f);
	}
	// Cinder: the lava mask is 0.5 on a river's centre line and falls / rises away from it, where the valleys are carved.
	const FTDTerrainField C(FTDTerrainStyle::ForPlanet(ETDDestination::Cinder));
	int32 Agree = 0, N = 0;
	for (float X = 450000.f; X < 780000.f; X += 3000.f)
	{
		const float Y = 120000.f;
		const float RN = C.RiverNoise(X, Y);
		if (FMath::Abs(RN) < 0.02f)
		{
			++N;
			Agree += FMath::Abs(C.Sample(X, Y).BroadMask - FMath::Clamp(0.5f + RN * 4.5f, 0.f, 1.f)) < 0.01f ? 1 : 0;
		}
	}
	TestEqual(TEXT("Cinder's broad mask is the river noise (lava follows the beds)"), Agree, N);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDTerrainApproach, "TriadDrift.World.Terrain.ArrivalCorridorStaysClear", TD_TEST_FLAGS)
bool FTDTerrainApproach::RunTest(const FString&)
{
	for (const ETDDestination P : { ETDDestination::Verdance, ETDDestination::Cinder })
	{
		const FTDTerrainField F(FTDTerrainStyle::ForPlanet(P));
		// T5's travel rig: the squad starts 600 m behind the pad, 300 m up, and descends straight to it.
		int32 Blocked = 0;
		for (float T = 0.f; T <= 1.f; T += 0.01f)
		{
			const FVector Q = FMath::Lerp(FVector(-60000.f, 0.f, 30000.f), FVector(0.f, 0.f, 500.f), T);
			for (const float Side : { -2200.f, 0.f, 2200.f })	// the wingmen fly beside the leader
			{
				if (Q.Z > 3000.f) { Blocked += F.Height(Q.X, Q.Y + Side) > Q.Z - 1500.f ? 1 : 0; }	// the last 30 m is the pad
			}
		}
		TestEqual(*FString::Printf(TEXT("planet %d: the arrival descent clears the terrain by 15 m"), (int32)P), Blocked, 0);
	}
	return true;
}

#endif

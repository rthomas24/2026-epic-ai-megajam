// ENG-38 stage 1: the globe <-> flat-world mapping (entry -> arrival, take-off -> globe) and the first view built
// around the arrival. Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.Descent

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDPlanetEntry.h"
#include "TriadDrift/World/TDPlanetTerrain.h"
#include "TriadDrift/World/TDPlanetAtmosphere.h"
#include "TriadDrift/World/TDTerrainField.h"
#include "TriadDrift/World/TDGlobeBake.h"
#include "TriadDrift/Tests/TDTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDescentMapping, "TriadDrift.World.Descent.GlobeAndGroundMapBothWays", TD_TEST_FLAGS)
bool FTDDescentMapping::RunTest(const FString&)
{
	// The landing zone (the pole) is the pad.
	TestTrue(TEXT("the pole is the pad"), FTDPlanetEntry::GroundFromGlobe(FTDGlobePoint()).IsNearlyZero(1.f));
	// Inside the ring the mapping is exact both ways.
	FRandomStream R(31337);
	for (int32 k = 0; k < 200; ++k)
	{
		const FVector2D XY(R.FRandRange(-420000.f, 420000.f), R.FRandRange(-420000.f, 420000.f));
		const FVector2D Back = FTDPlanetEntry::GroundFromGlobe(FTDPlanetEntry::GlobeFromGround(XY));
		if (!Back.Equals(XY, 1.f)) { AddError(FString::Printf(TEXT("round trip (%.0f, %.0f) -> (%.0f, %.0f)"), XY.X, XY.Y, Back.X, Back.Y)); break; }
	}
	// Every entry on the globe arrives within the 6 km ring, in its bearing; beyond the spread it clamps to the ring.
	for (int32 k = 0; k < 400; ++k)
	{
		FTDGlobePoint P;
		P.LatDeg = R.FRandRange(-90.f, 90.f);
		P.LonDeg = R.FRandRange(-180.f, 180.f);
		const FVector2D XY = FTDPlanetEntry::GroundFromGlobe(P);
		if (XY.Size() > FTDPlanetEntry::MaxArrivalRadius + 1.f) { AddError(TEXT("an entry arrived outside the ring")); break; }
		if (P.AngleFromPadDeg() > FTDPlanetEntry::SpreadDeg + 0.1f && FMath::Abs(XY.Size() - FTDPlanetEntry::MaxArrivalRadius) > 1.f) { AddError(TEXT("a far entry did not clamp to the ring")); break; }
		if (XY.Size() > 1.f)
		{
			const float Bearing = FMath::RadiansToDegrees(FMath::Atan2(XY.Y, XY.X));
			if (FMath::Abs(FMath::FindDeltaAngleDegrees(Bearing, P.LonDeg)) > 0.1f) { AddError(TEXT("bearing is not the longitude")); break; }
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDescentArrival, "TriadDrift.World.Descent.ArrivalIsAboveTheGroundHeadedAlongTheEntry", TD_TEST_FLAGS)
bool FTDDescentArrival::RunTest(const FString&)
{
	for (const ETDDestination Planet : { ETDDestination::Verdance, ETDDestination::Cinder })
	{
		const FTDTerrainField F(FTDTerrainStyle::ForPlanet(Planet));
		FTDGlobePoint P;
		P.LatDeg = 60.f;	// 30 deg from the landing zone: half way out the spread, half the ring from the pad
		P.LonDeg = 45.f;
		const FVector Up = P.ToUnit();
		// Flying toward the landing zone (north, along the surface) and a little down.
		const FVector ToPole = (FVector::UpVector - Up * FVector::DotProduct(FVector::UpVector, Up)).GetSafeNormal();
		const FTransform A = FTDPlanetEntry::ArrivalFromGlobe(Planet, P, ToPole * 9000.f - Up * 2000.f, 150000.f);
		const FVector L = A.GetLocation();
		TestTrue(*FString::Printf(TEXT("planet %d: half the ring out (%.0f m)"), (int32)Planet, FVector2D(L.X, L.Y).Size() / 100.f), FMath::IsNearlyEqual((float)FVector2D(L.X, L.Y).Size(), FTDPlanetEntry::MaxArrivalRadius * 0.5f, 10.f));
		TestTrue(*FString::Printf(TEXT("planet %d: 1.5 km above the ground"), (int32)Planet), FMath::IsNearlyEqual(L.Z - FMath::Max(F.Height(L.X, L.Y), F.GetStyle().BasinFloor), 150000.f, 1.f));
		const FVector Fwd = A.GetRotation().GetForwardVector();
		const FVector ToPad = (-FVector(L.X, L.Y, 0.f)).GetSafeNormal();
		TestTrue(*FString::Printf(TEXT("planet %d: flying toward the landing zone faces the pad (%.3f)"), (int32)Planet, FVector::DotProduct(Fwd, ToPad)), FVector::DotProduct(Fwd, ToPad) > 0.999f);
		TestTrue(TEXT("level (no pitch or roll)"), FMath::IsNearlyZero(A.Rotator().Pitch, 0.01f) && FMath::IsNearlyZero(A.Rotator().Roll, 0.01f));
		// A near-radial dive (5% along the surface, either side): stable, facing the pad, never flipping.
		const FTransform DiveA = FTDPlanetEntry::ArrivalFromGlobe(Planet, P, -Up * 10000.f + ToPole * 500.f, 150000.f);
		const FTransform DiveB = FTDPlanetEntry::ArrivalFromGlobe(Planet, P, -Up * 10000.f - ToPole * 500.f, 150000.f);
		TestTrue(TEXT("a near-radial dive faces the pad, whichever way the sliver points"),
			FVector::DotProduct(DiveA.GetRotation().GetForwardVector(), ToPad) > 0.999f && FVector::DotProduct(DiveB.GetRotation().GetForwardVector(), ToPad) > 0.999f);
		// Straight down: face the pad.
		const FTransform Down = FTDPlanetEntry::ArrivalFromGlobe(Planet, P, -Up * 5000.f, 150000.f);
		TestTrue(TEXT("a vertical entry faces the pad"), FVector::DotProduct(Down.GetRotation().GetForwardVector(), ToPad) > 0.999f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDescentTakeOff, "TriadDrift.World.Descent.TakeOffComesOutWhereAndHowYouLeft", TD_TEST_FLAGS)
bool FTDDescentTakeOff::RunTest(const FString&)
{
	FRandomStream R(4242);
	for (int32 k = 0; k < 50; ++k)
	{
		FTDGlobePoint P;
		P.LatDeg = R.FRandRange(35.f, 89.f);
		P.LonDeg = R.FRandRange(-180.f, 180.f);
		const FVector Up = P.ToUnit();
		FVector V = FVector(R.FRandRange(-1.f, 1.f), R.FRandRange(-1.f, 1.f), R.FRandRange(-1.f, 1.f));
		V = (V - Up * FVector::DotProduct(V, Up)).GetSafeNormal();	// along the surface
		const FTransform A = FTDPlanetEntry::ArrivalFromGlobe(ETDDestination::Verdance, P, V, 150000.f);
		FVector Dir;
		const FTDGlobePoint Out = FTDPlanetEntry::GlobeFromDeparture(A, Dir, 30.f);
		const FVector2D Arrived(A.GetLocation().X, A.GetLocation().Y);
		if (!FTDPlanetEntry::GroundFromGlobe(Out).Equals(Arrived, 1.f))	// back to the same ground point, within 1 cm
		{
			AddError(FString::Printf(TEXT("departure point (%.2f, %.2f) vs entry (%.2f, %.2f)"), Out.LatDeg, Out.LonDeg, P.LatDeg, P.LonDeg));
			break;
		}
		const FVector Surface = (Dir - Up * FVector::DotProduct(Dir, Up)).GetSafeNormal();
		if (FVector::DotProduct(Surface, V) < 0.999f) { AddError(TEXT("take-off heading does not match the arrival heading")); break; }
		if (!FMath::IsNearlyEqual(FMath::RadiansToDegrees(FMath::Asin(FVector::DotProduct(Dir, Up))), 30.f, 0.1f)) { AddError(TEXT("take-off does not climb 30 deg")); break; }
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDescentGalaxySide, "TriadDrift.World.Descent.GalaxyGlobeFollowsThePlanetActor", TD_TEST_FLAGS)
bool FTDDescentGalaxySide::RunTest(const FString&)
{
	FTDTestWorld W;
	ATDPlanetAtmosphere* Planet = W.Spawn<ATDPlanetAtmosphere>(FVector(500000.f, -200000.f, 30000.f));
	if (!TestNotNull(TEXT("planet"), Planet)) { return false; }
	Planet->Configure(ETDDestination::Verdance, TEXT("Verdance_Orbit"), 60000.f, 75000.f, FLinearColor::Green);
	Planet->SetActorRotation(FRotator(20.f, 70.f, -10.f));
	// The planet's +Z is the landing zone.
	FVector GV;
	const FTDGlobePoint Pole = FTDPlanetEntry::GlobeFromWorld(Planet, Planet->GetPlanetCenter() + Planet->GetGlobeQuat().GetUpVector() * 75000.f, FVector::ZeroVector, GV);
	TestTrue(*FString::Printf(TEXT("the globe frame's up is the landing zone (lat %.2f)"), Pole.LatDeg), Pole.LatDeg > 89.9f);
	FRandomStream R(77);
	for (int32 k = 0; k < 50; ++k)
	{
		FTDGlobePoint P;
		P.LatDeg = R.FRandRange(-80.f, 80.f);
		P.LonDeg = R.FRandRange(-180.f, 180.f);
		const FVector GlobeDir = FVector(R.FRandRange(-1.f, 1.f), R.FRandRange(-1.f, 1.f), R.FRandRange(-1.f, 1.f)).GetSafeNormal();
		FVector WorldDir;
		const FVector At = FTDPlanetEntry::WorldFromGlobe(Planet, P, GlobeDir, WorldDir);
		if (!FMath::IsNearlyEqual((float)FVector::Dist(At, Planet->GetPlanetCenter()), Planet->GetAtmosphereRadius(), 1.f)) { AddError(TEXT("not on the atmosphere shell")); break; }
		FVector BackDir;
		const FTDGlobePoint Back = FTDPlanetEntry::GlobeFromWorld(Planet, At, WorldDir * 9000.f, BackDir);
		if (FVector::DotProduct(Back.ToUnit(), P.ToUnit()) < 0.99999f || !BackDir.GetSafeNormal().Equals(GlobeDir, 1e-3f)) { AddError(TEXT("world <-> globe round trip")); break; }
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDescentFirstView, "TriadDrift.World.Descent.FirstViewBuildsAroundTheArrival", TD_TEST_FLAGS)
bool FTDDescentFirstView::RunTest(const FString&)
{
	for (const ETDDestination Planet : { ETDDestination::Verdance, ETDDestination::Cinder })
	{
		FTDTestWorld W;
		ATDPlanetTerrain* T = ATDPlanetTerrain::SpawnForPlanet(W.GetWorld(), Planet, FTransform::Identity, nullptr);
		if (!TestNotNull(TEXT("terrain"), T)) { return false; }
		AddInfo(FString::Printf(TEXT("planet %d: spawn-time build %.0f ms"), (int32)Planet, T->GetSetupMs()));
		// An arrival 6 km out, 150 m up: final LOD and collision under it before the first frame.
		FTDGlobePoint P;
		P.LatDeg = 90.f - FTDPlanetEntry::SpreadDeg * 6.f / 7.f;
		P.LonDeg = -120.f;
		const FVector Up = P.ToUnit();
		const FTransform A = FTDPlanetEntry::ArrivalFromGlobe(Planet, P, -Up, 15000.f);
		const double Ms = T->BuildFirstViewAt(A.GetLocation());
		AddInfo(FString::Printf(TEXT("planet %d: first view around the arrival %.0f ms"), (int32)Planet, Ms));
		TestEqual(TEXT("LOD0 under the arrival"), T->GetChunkLodAt(A.GetLocation()), 0);
		FHitResult Hit;
		FCollisionQueryParams Q(TEXT("TDDescentFirstView"), true);
		const FVector L = A.GetLocation();
		const bool bHit = W.GetWorld()->LineTraceSingleByChannel(Hit, L, L - FVector(0.f, 0.f, 400000.f), ECC_WorldStatic, Q);
		TestTrue(TEXT("ground (collision) under the arrival at once"), bHit && Hit.GetActor() == T);
		// From 3 km up nothing within reach is LOD0 (detail resolves on the way down).
		const FVector High = L + FVector(0.f, 0.f, 300000.f);
		T->BuildFirstViewAt(High);
		TestEqual(TEXT("from 3 km up: no LOD0 under the ship"), T->GetChunkLodAt(High) == 0 ? 0 : 1, 1);
		TestTrue(*FString::Printf(TEXT("the first view is fast enough to hide (%.0f ms)"), Ms), Ms < 3000.0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDescentGlobeBake, "TriadDrift.World.Descent.GlobeShowsTheSameWorldAsTheGround", TD_TEST_FLAGS)
bool FTDDescentGlobeBake::RunTest(const FString&)
{
	// The globe's landing-zone cap uses exactly the descent mapping: 60 deg from the pole is the 6 km ring.
	TestTrue(TEXT("the descent ring"), FMath::IsNearlyEqual(FTDGlobeBake::GroundDistanceForAngle(FTDPlanetEntry::SpreadDeg), FTDPlanetEntry::MaxArrivalRadius, 1.f));
	TestTrue(TEXT("out to the far ring"), FMath::IsNearlyEqual(FTDGlobeBake::GroundDistanceForAngle(120.f), FTDTerrainStyle::ForPlanet(ETDDestination::Verdance).FarHalfSize, 1.f));
	float Last = -1.f;
	for (float A = 0.f; A <= 180.f; A += 5.f) { const float D = FTDGlobeBake::GroundDistanceForAngle(A); TestTrue(TEXT("monotonic"), D >= Last); Last = D; }
	// The pad's beacon at the pole.
	FLinearColor B, E;
	FTDGlobeBake::SampleAt(ETDDestination::Verdance, 90.f, 0.f, B, E);
	TestTrue(TEXT("beacon at the landing zone"), E.G > 0.5f && B.GetLuminance() > 0.5f);
	// Verdance: the hero massif is rock / snow where the ground has it, the jungle around the pad is green.
	const FTDTerrainStyle VS = FTDTerrainStyle::ForPlanet(ETDDestination::Verdance);
	const FVector2D Hero = VS.HeroCentre;
	const FTDGlobePoint HP = FTDPlanetEntry::GlobeFromGround(Hero);
	FLinearColor HeroB, HeroE, PadB, PadE;
	FTDGlobeBake::SampleAt(ETDDestination::Verdance, HP.LatDeg, HP.LonDeg, HeroB, HeroE);
	{
		const FTDTerrainField VF(VS);
		const FTDTerrainSample HS = VF.Sample(Hero.X, Hero.Y, 2000.f);
		AddInfo(FString::Printf(TEXT("hero sample: h %.0f m, h01 %.2f, slope %.2f, broad %.2f; globe (%.2f, %.2f)"), HS.Height / 100.f, HS.Height01, HS.Slope, HS.BroadMask, HP.LatDeg, HP.LonDeg));
	}
	const FTDGlobePoint Near = FTDPlanetEntry::GlobeFromGround(FVector2D(60000.f, 0.f));
	FTDGlobeBake::SampleAt(ETDDestination::Verdance, Near.LatDeg, Near.LonDeg, PadB, PadE);
	TestTrue(*FString::Printf(TEXT("the massif reads lighter (rock) than the basin (%.2f vs %.2f)"), HeroB.GetLuminance(), PadB.GetLuminance()), HeroB.GetLuminance() > PadB.GetLuminance());
	TestTrue(TEXT("the basin is green"), PadB.G > PadB.R && PadB.G > PadB.B);
	// Cinder: somewhere on the landing-zone cap, lava glows.
	float MaxGlow = 0.f;
	for (float Lat = 35.f; Lat < 89.f; Lat += 2.f)
	{
		for (float Lon = -180.f; Lon < 180.f; Lon += 6.f)
		{
			FTDGlobeBake::SampleAt(ETDDestination::Cinder, Lat, Lon, B, E);
			MaxGlow = FMath::Max(MaxGlow, E.R);
		}
	}
	TestTrue(*FString::Printf(TEXT("Cinder's lava glows from orbit (%.2f)"), MaxGlow), MaxGlow > 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDescentCurvature, "TriadDrift.World.Descent.FarRingBendsUnderTheHorizonCoreStaysExact", TD_TEST_FLAGS)
bool FTDDescentCurvature::RunTest(const FString&)
{
	FTDTestWorld W;
	ATDPlanetTerrain* T = ATDPlanetTerrain::SpawnForPlanet(W.GetWorld(), ETDDestination::Verdance, FTransform::Identity, nullptr);
	if (!TestNotNull(TEXT("terrain"), T)) { return false; }
	const float Core = T->GetField()->GetStyle().CoreHalfSize;
	const float Far = T->GetField()->GetStyle().FarHalfSize;
	TestEqual(TEXT("no bend inside the core (the gameplay ground)"), T->CurvatureDrop(Core * 0.99f, Core * 0.99f), 0.f);
	const float Edge = T->CurvatureDrop(Far, 0.f);
	TestTrue(*FString::Printf(TEXT("the far edge drops under the horizon (%.0f m)"), Edge / 100.f), Edge > 50000.f && Edge < 200000.f);
	TestTrue(TEXT("it grows outward"), T->CurvatureDrop(Core + 1000000.f, 0.f) < Edge);
	return true;
}

#endif

// Automation tests for the galaxy layout, planets and obstacle avoidance (ENG-17).
// Run: .\Tools\td-test.ps1 -Filter TriadDrift.World

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDGalaxyLayout.h"
#include "TriadDrift/World/TDPlanetAtmosphere.h"
#include "TriadDrift/World/TDPortalGate.h"
#include "TriadDrift/World/TDDominion.h"
#include "TriadDrift/World/TDWorldSubsystem.h"
#include "TriadDrift/Waves/Tests/TDWaveTestTypes.h"
#include "TriadDrift/Flight/TDSpacecraft.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "TriadDrift/Travel/TDTravelSubsystem.h"
#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/Flight/TDShip.h"
#include "Components/InstancedStaticMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldLayoutData, "TriadDrift.World.Layout.DefaultData", TD_TEST_FLAGS)
bool FTDWorldLayoutData::RunTest(const FString& Parameters)
{
	const FTDGalaxyLayoutData L = ATDGalaxyLayout::MakeDefaultLayout();
	TestEqual(TEXT("3 gate pairs"), L.Gates.Num(), 3);
	TestEqual(TEXT("2 planets"), L.Planets.Num(), 2);
	TestTrue(TEXT("Verdance + Cinder"), L.Planets[0].Destination == ETDDestination::Verdance && L.Planets[1].Destination == ETDDestination::Cinder);
	for (const FName Name : { TDReturnPoints::VerdanceOrbit, TDReturnPoints::CinderOrbit, TDReturnPoints::DominionHangarExit })
	{
		TestTrue(*FString::Printf(TEXT("return point %s"), *Name.ToString()), L.ReturnPoints.Contains(Name));
	}
	auto Inside = [&L](const FVector& P) { return FVector::Dist(P, L.BoundaryCenter) < L.BoundaryRadius * 0.9f; };
	TestTrue(TEXT("Dominion inside the play space"), Inside(L.Dominion.GetLocation()));
	for (const FTDPlanetSpec& P : L.Planets)
	{
		TestTrue(TEXT("planet inside the play space"), Inside(P.Location));
		TestTrue(TEXT("planets are far from the combat zone (hyperdrive / gate distance)"), FVector::Dist(P.Location, L.CombatZoneCenter) > 300000.f);
		const FVector Orbit = L.ReturnPoints[P.ReturnPoint].GetLocation();
		TestTrue(TEXT("orbit return point is outside the atmosphere"), FVector::Dist(Orbit, P.Location) > P.AtmosphereRadius);
	}
	TestTrue(TEXT("flank pair opens only for waves 3-5"), L.Gates[2].OpenPhases.Contains(ETDGamePhase::Waves3to4) && !L.Gates[2].OpenPhases.Contains(ETDGamePhase::Waves1to2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldLayoutBuild, "TriadDrift.World.Layout.BuildsEverything", TD_TEST_FLAGS)
bool FTDWorldLayoutBuild::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDGalaxyLayout* G = ATDGalaxyLayout::SpawnDefaultGalaxy(W.GetWorld());
	if (!TestNotNull(TEXT("layout spawned"), G)) { return false; }
	TestTrue(TEXT("SpawnDefaultGalaxy reuses the existing one"), ATDGalaxyLayout::SpawnDefaultGalaxy(W.GetWorld()) == G);
	TestTrue(TEXT("built on BeginPlay"), G->IsBuilt());

	TestTrue(TEXT("asteroids placed"), G->GetAsteroidCount() > 100);
	TestEqual(TEXT("2 planets"), G->GetPlanets().Num(), 2);
	TestNotNull(TEXT("Dominion spawned"), G->GetDominion());
	TestTrue(TEXT("Dominion hidden until revealed"), G->GetDominion() && G->GetDominion()->GetState() == ETDDominionState::Hidden);

	ATDPortalGate* VZ = G->FindGate(TEXT("Gate_Verdance_Zone"));
	ATDPortalGate* VO = G->FindGate(TEXT("Gate_Verdance_Orbit"));
	TestTrue(TEXT("Verdance gate pair linked"), VZ && VO && VZ->GetPartner() == VO && VO->GetPartner() == VZ);
	TestTrue(TEXT("zone gates open"), VZ && VZ->IsGateActive());
	ATDPortalGate* Flank = G->FindGate(TEXT("Gate_Flank_West"));
	TestTrue(TEXT("flank gate closed in waves 1-2"), Flank && !Flank->IsGateActive());

	UTDWorldSubsystem* WS = UTDWorldSubsystem::Get(W.GetWorld());
	FVector C; float R = 0.f;
	TestTrue(TEXT("play bounds set"), WS && WS->GetPlayBounds(C, R) && R == G->GetLayout().BoundaryRadius);
	TestTrue(TEXT("obstacles registered"), WS && WS->GetObstacleCount() > 100);
	TestFalse(TEXT("the dogfight pocket is clear"), WS && WS->IsInsideObstacle(FVector::ZeroVector, 5000.f));
	TestFalse(TEXT("the player start is clear"), WS && WS->IsInsideObstacle(G->GetPlayerStart().GetLocation(), 1500.f));

	FTransform T;
	TestTrue(TEXT("find Verdance_Orbit"), G->FindReturnPoint(TDReturnPoints::VerdanceOrbit, T));
	TestFalse(TEXT("unknown return point"), G->FindReturnPoint(TEXT("Nowhere"), T));
	TestFalse(TEXT("none return point"), G->FindReturnPoint(NAME_None, T));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldAtmosphere, "TriadDrift.World.Planet.AtmosphereOpensAndTravels", TD_TEST_FLAGS)
bool FTDWorldAtmosphere::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	GI->Progress.Phase = ETDGamePhase::Waves1to2;

	ATDPlanetAtmosphere* P = W.Spawn<ATDPlanetAtmosphere>(FVector(200000.f, 0.f, 0.f));
	P->Configure(ETDDestination::Verdance, TDReturnPoints::VerdanceOrbit, 20000.f, 40000.f, FLinearColor::Green);
	W.Tick();
	TestFalse(TEXT("closed before the marker"), P->IsOpen());

	ATDSpacecraft* Ship = W.Spawn<ATDSpacecraft>(FVector(0.f, 0.f, 0.f), FRotator(0.f, 90.f, 0.f));	// tangent: coasting keeps it near the radius
	W.SpawnPlayerController(Ship);
	Ship->SetSpeedOverride(0.f, 0.f);	// hold position inside the atmosphere radius
	Ship->TeleportSpacecraft(FVector(200000.f - 28000.f, 0.f, 0.f), FRotator(0.f, 90.f, 0.f), false);
	W.Advance(2.f);
	TestEqual(TEXT("closed atmosphere doesn't take you down"), GI->Travels.Num(), 0);

	// Wave 2 cleared -> flow phase Verdance. (Re-place the ship: it coasts at min speed.)
	Ship->TeleportSpacecraft(FVector(200000.f - 28000.f, 0.f, 0.f), FRotator(0.f, 90.f, 0.f), false);
	GI->Progress.Phase = ETDGamePhase::Verdance;
	W.Advance(1.1f);
	TestTrue(TEXT("open in the Verdance phase"), P->IsOpen());
	TestTrue(TEXT("player controls the ship"), Ship->IsPlayerControlled());
	TestTrue(*FString::Printf(TEXT("ship inside the atmosphere (%.0f < %.0f)"), FVector::Dist(Ship->GetActorLocation(), P->GetActorLocation()), P->GetAtmosphereRadius()), FVector::Dist(Ship->GetActorLocation(), P->GetActorLocation()) < P->GetAtmosphereRadius());
	W.Advance(P->EntryDelaySeconds + 0.2f);
	TestTrue(TEXT("TravelTo(Verdance)"), GI->Travels.Num() == 1 && GI->Travels[0] == ETDDestination::Verdance);
	W.Advance(3.f);
	TestEqual(TEXT("travel requested once"), GI->Travels.Num(), 1);

	// Story moved on: closed again.
	GI->Progress.Phase = ETDGamePhase::Waves3to4;
	W.Advance(1.1f);
	TestFalse(TEXT("closed after the planet phase"), P->IsOpen());

	// Wingmen (not player-controlled) never trigger it.
	ATDPlanetAtmosphere* C = W.Spawn<ATDPlanetAtmosphere>(FVector(-200000.f, 0.f, 0.f));
	C->Configure(ETDDestination::Cinder, TDReturnPoints::CinderOrbit, 20000.f, 40000.f, FLinearColor::Red);
	GI->Progress.Phase = ETDGamePhase::Cinder;
	ATDSpacecraft* Wingman = W.Spawn<ATDSpacecraft>(FVector(-200000.f + 28000.f, 0.f, 0.f));
	Wingman->SetSpeedOverride(0.f, 0.f);
	W.Advance(1.5f);
	TestTrue(TEXT("Cinder open"), C->IsOpen());
	TestEqual(TEXT("AI ship doesn't trigger travel"), GI->Travels.Num(), 1);
	(void)Wingman;
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldAvoidance, "TriadDrift.World.Obstacles.AvoidanceMaths", TD_TEST_FLAGS)
bool FTDWorldAvoidance::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	UTDWorldSubsystem* WS = UTDWorldSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("world subsystem"), WS)) { return false; }
	WS->AddObstacle(FVector(5000.f, 200.f, 0.f), 1000.f);

	FVector Avoid;
	TestTrue(TEXT("obstacle ahead detected"), WS->FindAvoidance(FVector::ZeroVector, FVector::ForwardVector, 8000.f, 200.f, Avoid));
	TestTrue(TEXT("steers away from the side the obstacle is on"), Avoid.Y < -0.3f);
	TestFalse(TEXT("too short a probe -> clear"), WS->FindAvoidance(FVector::ZeroVector, FVector::ForwardVector, 2000.f, 200.f, Avoid));
	TestFalse(TEXT("pointing away -> clear"), WS->FindAvoidance(FVector::ZeroVector, -FVector::ForwardVector, 8000.f, 200.f, Avoid));
	TestFalse(TEXT("passing wide -> clear"), WS->FindAvoidance(FVector(0.f, 3000.f, 0.f), FVector::ForwardVector, 8000.f, 200.f, Avoid));
	TestTrue(TEXT("inside test"), WS->IsInsideObstacle(FVector(5000.f, 200.f, 500.f)));

	WS->AddObstacle(FVector(5000.f, 0.f, 0.f), 1000.f);	// dead centre: still picks a direction
	TestTrue(TEXT("dead-centre obstacle"), WS->FindAvoidance(FVector::ZeroVector, FVector::ForwardVector, 8000.f, 200.f, Avoid) && !Avoid.ContainsNaN() && FMath::Abs(Avoid.X) < 0.99f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldLayoutTravel, "TriadDrift.World.Layout.TravelHandOffAndDressing", TD_TEST_FLAGS)
bool FTDWorldLayoutTravel::RunTest(const FString& Parameters)
{
	FTDTestWorld W;
	ATDGalaxyLayout* G = ATDGalaxyLayout::SpawnDefaultGalaxy(W.GetWorld());
	UTDTravelSubsystem* Travel = UTDTravelSubsystem::Get(W.GetWorld());
	if (!TestNotNull(TEXT("travel subsystem"), Travel)) { return false; }

	FVector C; float R = 0.f;
	TestTrue(TEXT("play bounds handed to flight (T1)"), Travel->GetPlayBounds(C, R) && FMath::IsNearlyEqual(R, G->GetLayout().BoundaryRadius));

	for (ATDPlanetAtmosphere* P : G->GetPlanets())
	{
		FTransform Orbit;
		TestTrue(TEXT("orbit return point exists"), G->FindReturnPoint(P->GetReturnPoint(), Orbit));
		const float D = FVector::Dist(Orbit.GetLocation(), P->GetPlanetCenter());
		TestTrue(*FString::Printf(TEXT("%s orbit outside the entry trigger (%.0f > %.0f)"), *P->GetWellId().ToString(), D, P->GetAtmosphereRadius()), D > P->GetAtmosphereRadius());
		FTDTravelGravityWell Well;
		TestTrue(*FString::Printf(TEXT("%s orbit inside its gravity well"), *P->GetWellId().ToString()), Travel->FindWellContaining(Orbit.GetLocation(), Well));
		TestTrue(TEXT("orbit faces away from the planet"), FVector::DotProduct(Orbit.GetRotation().GetForwardVector(), (Orbit.GetLocation() - P->GetPlanetCenter()).GetSafeNormal()) > 0.5f);
	}
	FTDTravelGravityWell DWell;
	TestTrue(TEXT("Dominion approach well"), G->GetDominion() && Travel->FindWellContaining(G->GetDominion()->GetActorLocation(), DWell));

	// Placeholder dressing can hide for Nick's art without touching gameplay.
	UTDWorldSubsystem* WS = UTDWorldSubsystem::Get(W.GetWorld());
	const int32 Obstacles = WS->GetObstacleCount();
	G->SetHidePlaceholderDressing(true);
	const UInstancedStaticMeshComponent* Ast = G->FindComponentByClass<UInstancedStaticMeshComponent>();
	TestTrue(TEXT("asteroid meshes hidden"), Ast && !Ast->IsVisible());
	TestTrue(TEXT("collision kept"), Ast && Ast->IsCollisionEnabled());
	TestEqual(TEXT("obstacles unchanged"), WS->GetObstacleCount(), Obstacles);
	G->SetHidePlaceholderDressing(false);
	TestTrue(TEXT("visible again"), Ast && Ast->IsVisible());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDWorldPlanetEntry, "TriadDrift.World.Planet.SquadEntryStopsAboveTheSurface", TD_TEST_FLAGS)
bool FTDWorldPlanetEntry::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDWaveTestGameInstance::StaticClass());
	UTDWaveTestGameInstance* GI = Cast<UTDWaveTestGameInstance>(W.GetGameInstance());
	GI->Progress.Phase = ETDGamePhase::Verdance;
	ATDPlanetAtmosphere* P = W.Spawn<ATDPlanetAtmosphere>(FVector(100000.f, 0.f, 0.f));
	P->Configure(ETDDestination::Verdance, TDReturnPoints::VerdanceOrbit, 20000.f, 32000.f, FLinearColor::Green);
	W.Tick();
	TestTrue(TEXT("open"), P->IsOpen());

	// A real squad ship, player-controlled, flying straight at the planet at cruise.
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(100000.f - 40000.f, 0.f, 0.f));
	W.SpawnPlayerController(Ship);
	FTDFlightInput In;
	In.Throttle = 1.f;
	Ship->SetFlightInput(In);
	UTDAtmosphereEntryComponent* Entry = Ship->GetAtmosphereEntry();
	if (!TestNotNull(TEXT("squad ships carry T1's entry component"), Entry)) { return false; }

	float Closest = TNumericLimits<float>::Max();
	bool bEntered = false;
	for (int32 i = 0; i < 60 * 6; ++i)
	{
		W.Tick();
		bEntered |= Entry->IsEntering();
		Closest = FMath::Min(Closest, FVector::Dist(Ship->GetActorLocation(), P->GetPlanetCenter()));
	}
	TestTrue(TEXT("the atmosphere started the ENG-20 entry (seen committed at least once)"), bEntered || GI->Travels.Num() == 1);
	TestTrue(*FString::Printf(TEXT("the dive stops above the surface (closest %.0f cm from the centre, surface %.0f)"), Closest, P->GetPlanetRadius()), Closest > P->GetPlanetRadius());
	TestEqual(TEXT("exactly one travel to Verdance"), GI->Travels.Num(), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

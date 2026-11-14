// T2's live finding (the Lead, Nov 14): a planet reached by the cheat travel glided from the spawn height (285 m, 1 s).
// Every covered arrival on a planet - with or without a recorded entry point, first visit or a re-entry - is placed over
// the cloud deck and glides from there; a Continue (no cover) never glides. Run: -Filter TriadDrift.Travel.Descent.EveryEntryPath

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/Travel/TDDescentSubsystem.h"
#include "TriadDrift/Travel/TDAtmosphereZones.h"
#include "TriadDrift/World/TDPlanetTerrain.h"
#include "TriadDrift/World/TDCloudDeck.h"
#include "TriadDrift/Flight/TDShip.h"
#include "TriadDrift/Flight/TDShipMovementComponent.h"
#include "TriadDrift/Flow/TDGameInstance.h"
#include "TriadDrift/Tests/TDTestWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDT1GlideEveryEntryPath, "TriadDrift.Travel.Descent.EveryEntryPathGlidesFromOverTheDeck", TD_TEST_FLAGS)
bool FTDT1GlideEveryEntryPath::RunTest(const FString& Parameters)
{
	FTDTestWorld W(UTDGameInstance::StaticClass());
	UWorld* World = W.GetWorld();
	UTDDescentSubsystem* D = UTDDescentSubsystem::Get(World);
	ATDPlanetTerrain* Terrain = ATDPlanetTerrain::SpawnForPlanet(World, ETDDestination::Verdance, FTransform::Identity, nullptr);
	ATDAtmosphereFlightZone* Zone = W.Spawn<ATDAtmosphereFlightZone>(FVector(0.f, 0.f, 59500.f));
	if (!TestNotNull(TEXT("descent"), D) || !TestNotNull(TEXT("terrain"), Terrain) || !TestNotNull(TEXT("zone"), Zone)) { return false; }
	Zone->SetZoneExtent(FVector(250000.f, 250000.f, 60000.f));
	if (ATDCloudDeck* Deck = ATDCloudDeck::EnsureForWorld(World, ETDDestination::Verdance)) { Deck->Build(); }
	const float Ground = Terrain->GetHeightAt(FVector::ZeroVector);
	ATDShip* Ship = W.Spawn<ATDShip>(FVector(0.f, 0.f, Ground + 28500.f));
	if (!TestNotNull(TEXT("ship"), Ship)) { return false; }
	Ship->SetPlayerControlled(true);
	W.SpawnLocalPlayerController(Ship);
	W.Advance(1.2f);
	TestTrue(TEXT("zone fitted"), Zone->IsFittedToTerrain());

	// The cheat / stage / flow travel: no recorded entry point. Twice: the first visit and a re-entry after a take-off.
	for (int32 Visit = 1; Visit <= 2; ++Visit)
	{
		const FString V = FString::Printf(TEXT("visit %d (no recorded entry): "), Visit);
		Ship->TeleportSpacecraft(FVector(0.f, 0.f, Ground + 28500.f), FRotator::ZeroRotator, false); // the spawn height (285 m)
		D->SetEntryCover(1.f);
		D->BeginCoveredTravel(ETDDestination::Verdance);
		D->NotifyPostLoadForTests(World);
		float StartZ = 0.f;
		for (int32 i = 0; i < 60 * 12 && !D->IsGliding(); ++i)
		{
			++GFrameCounter; // a test world's ticks don't advance the engine's frame counter
			W.Tick(1.f / 60.f);
			D->Tick(1.f / 60.f);
			if (D->IsGliding()) { StartZ = Ship->GetActorLocation().Z; }
		}
		TestTrue(*(V + TEXT("placed over the deck")), D->HasArrivedOverDeck());
		TestTrue(*(V + TEXT("the glide begins")), D->IsGliding());
		const float Want = D->GetDescentDeckZ() + UTDDescentSubsystem::GlideAboveDeckCm;
		AddInfo(FString::Printf(TEXT("%sglide starts at %.0f m, deck %.0f m (%.1f s at %.0f m/s)"), *V, StartZ / 100.f, D->GetDescentDeckZ() / 100.f, D->GetGlideSeconds(), D->GetGlideSpeedNow() / 100.f));
		TestTrue(*FString::Printf(TEXT("%sstarts 300 m over the deck (%.0f vs %.0f m)"), *V, StartZ / 100.f, Want / 100.f), FMath::Abs(StartZ - Want) <= 3000.f);
		TestTrue(*(V + TEXT("the squad is released from the zone for the glide")), Ship->GetShipMovement()->IsBoundaryReleased());
		D->EndGlide(TEXT("test"));
		for (int32 i = 0; i < 90; ++i) { D->Tick(1.f / 60.f); }
	}
	// A Continue (or any arrival with no cover): no placement, no glide.
	Ship->TeleportSpacecraft(FVector(0.f, 0.f, Ground + 28500.f), FRotator::ZeroRotator, false);
	for (int32 i = 0; i < 180; ++i) { ++GFrameCounter; W.Tick(1.f / 60.f); D->Tick(1.f / 60.f); }
	TestFalse(TEXT("no cover (Continue): no glide"), D->IsGliding());
	return true;
}

#endif

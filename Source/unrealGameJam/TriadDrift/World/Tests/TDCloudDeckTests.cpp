// The planet cloud deck (environment builder). Run: .\Tools\td-test.ps1 -Filter TriadDrift.World.CloudDeck

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TriadDrift/World/TDCloudDeck.h"
#include "TriadDrift/World/TDPlanetTerrain.h"
#include "TriadDrift/Flow/TDPlaceholderSky.h"
#include "TriadDrift/Tests/TDTestWorld.h"
#include "ProceduralMeshComponent.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

namespace TDCloudDeckTest
{
	static TArray<ATDCloudDeck*> Decks(UWorld* World)
	{
		TArray<ATDCloudDeck*> Out;
		for (TActorIterator<ATDCloudDeck> It(World); It; ++It) { Out.Add(*It); }
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDCloudDeckOnPlanetsOnly, "TriadDrift.World.CloudDeck.SpawnsOnBothPlanetsNotInSpace", TD_TEST_FLAGS)
bool FTDCloudDeckOnPlanetsOnly::RunTest(const FString&)
{
	for (const ETDDestination Planet : { ETDDestination::Verdance, ETDDestination::Cinder })
	{
		FTDTestWorld W;
		const ETDPlaceholderSkyStyle Style = Planet == ETDDestination::Cinder ? ETDPlaceholderSkyStyle::Cinder : ETDPlaceholderSkyStyle::Verdance;
		ATDPlaceholderSky::SpawnIfNeeded(W.GetWorld(), Style);
		ATDPlaceholderSky::SpawnIfNeeded(W.GetWorld(), Style);	// a second call must not add a second deck
		TArray<ATDCloudDeck*> D = TDCloudDeckTest::Decks(W.GetWorld());
		const FString Name = Planet == ETDDestination::Cinder ? TEXT("Cinder") : TEXT("Verdance");
		if (!TestEqual(*(Name + TEXT(": one deck")), D.Num(), 1)) { continue; }
		TestEqual(*(Name + TEXT(": the deck knows its planet")), (int32)D[0]->GetPlanet(), (int32)Planet);
		W.Advance(14.f, 1.f / 10.f);	// the terrain waits for a course (none here: 10 s), then the deck builds on it
		TestTrue(*(Name + TEXT(": built")), D[0]->IsBuilt());
		UProceduralMeshComponent* M = D[0]->GetMesh();
		TestTrue(*(Name + TEXT(": has geometry")), M && M->GetNumSections() == 1 && M->GetProcMeshSection(0)->ProcVertexBuffer.Num() > 500);
		TestTrue(*(Name + TEXT(": visual only (no collision)")), M && M->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
		TestFalse(*(Name + TEXT(": no shadow")), M && M->CastShadow);
		TestTrue(*(Name + TEXT(": has its material")), M && M->GetMaterial(0) && M->GetMaterial(0)->GetName().Contains(TEXT("CloudDeck_") + Name));
		// ENG-38 (agreed with T1, Nov 14): the deck clears the flight zone's highest ground by DeckClearance (T1 raises
		// the zone ceiling to deck + 500 m and arrives at deck + 300 m).
		ATDPlanetTerrain* T = nullptr;
		for (TActorIterator<ATDPlanetTerrain> It(W.GetWorld()); It; ++It) { T = *It; }
		const float PadZ = T ? T->GetActorLocation().Z : 0.f;
		const float Up = D[0]->GetActorLocation().Z - PadZ;
		TestTrue(*FString::Printf(TEXT("%s: deck %.0f m over the pad, the pad corridor's rule, inside 1.1-1.6 km"), *Name, Up / 100.f),
			T && FMath::IsNearlyEqual(D[0]->GetActorLocation().Z, T->GetCloudDeckZ(), 1.f) && Up >= T->DeckMinAbovePad - 1.f && Up <= T->DeckMaxAbovePad + 1.f);
		// The edge bends down with the terrain's far ring (never a visible lip over the horizon).
		if (M && M->GetNumSections() == 1)
		{
			const TArray<FProcMeshVertex>& Vb = M->GetProcMeshSection(0)->ProcVertexBuffer;
			TestTrue(*(Name + TEXT(": the rim is bent down")), Vb.Last().Position.Z < -100000.f);
			TestTrue(*(Name + TEXT(": the middle is flat")), FMath::IsNearlyZero(Vb[0].Position.Z));
		}
	}
	{
		FTDTestWorld W;
		ATDPlaceholderSky::SpawnIfNeeded(W.GetWorld(), ETDPlaceholderSkyStyle::Space);
		W.Advance(1.f);
		TestEqual(TEXT("space: no deck"), TDCloudDeckTest::Decks(W.GetWorld()).Num(), 0);
	}
	{
		IConsoleVariable* Cv = IConsoleManager::Get().FindConsoleVariable(TEXT("td.CloudDeck"));
		if (TestNotNull(TEXT("td.CloudDeck exists"), Cv))
		{
			const int32 Was = Cv->GetInt();
			Cv->Set(0, ECVF_SetByCode);
			FTDTestWorld W;
			ATDPlaceholderSky::SpawnIfNeeded(W.GetWorld(), ETDPlaceholderSkyStyle::Verdance);
			TestEqual(TEXT("td.CloudDeck 0: no deck"), TDCloudDeckTest::Decks(W.GetWorld()).Num(), 0);
			Cv->Set(Was, ECVF_SetByCode);
		}
	}
	return true;
}


// ENG-38 (agreed with T1): during the glide the haze follows T1's Descent01 - thick at the arrival over the deck, the planet's
// normal fog at the glide end, easing (no step); with no glide (-1) the altitude ramp as before.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTDDescentFogRamp, "TriadDrift.World.CloudDeck.DescentHazeFollowsTheGlide", TD_TEST_FLAGS)
bool FTDDescentFogRamp::RunTest(const FString&)
{
	const float Max = 12.f, Low = 150000.f, High = 500000.f;
	TestEqual(TEXT("thick at the arrival"), ATDPlaceholderSky::ComputeDescentFogScale(0.f, 999999.f, Max, Low, High), Max, 0.01f);
	TestEqual(TEXT("normal at the glide end"), ATDPlaceholderSky::ComputeDescentFogScale(1.f, 0.f, Max, Low, High), 1.f, 0.01f);
	float Prev = Max + 1.f, BiggestStep = 0.f;
	for (int32 i = 0; i <= 100; ++i)
	{
		const float S = ATDPlaceholderSky::ComputeDescentFogScale(i / 100.f, 0.f, Max, Low, High);
		TestTrue(TEXT("never thickens as the glide goes on"), S <= Prev + KINDA_SMALL_NUMBER);
		if (i > 0) { BiggestStep = FMath::Max(BiggestStep, Prev - S); }
		Prev = S;
	}
	TestTrue(*FString::Printf(TEXT("no step: at most %.2f per 1%% of the glide"), BiggestStep), BiggestStep < (Max - 1.f) * 0.05f);
	TestEqual(TEXT("no glide, low: the altitude ramp's normal"), ATDPlaceholderSky::ComputeDescentFogScale(-1.f, 0.f, Max, Low, High), 1.f, 0.01f);
	TestEqual(TEXT("no glide, high: the altitude ramp's thick"), ATDPlaceholderSky::ComputeDescentFogScale(-1.f, 900000.f, Max, Low, High), Max, 0.01f);
	return true;
}

#endif
